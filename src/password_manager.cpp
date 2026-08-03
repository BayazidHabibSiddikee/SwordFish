// src/password_manager.cpp — Save, autofill and manage login credentials
#include "password_manager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QWebEnginePage>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

// ── Simple XOR obfuscation (not encryption, just avoids plaintext in JSON) ──
static const quint8 XOR_KEY = 0x5A;

QString PasswordManager::obfuscate(const QString &s) {
    QByteArray b = s.toUtf8();
    for (auto &c : b) c ^= XOR_KEY;
    return QString::fromLatin1(b.toBase64());
}

QString PasswordManager::deobfuscate(const QString &s) {
    QByteArray b = QByteArray::fromBase64(s.toLatin1());
    for (auto &c : b) c ^= XOR_KEY;
    return QString::fromUtf8(b);
}

// ── Constructor ──────────────────────────────────────────────────────────────
PasswordManager::PasswordManager(const QString &storePath, QObject *parent)
    : QObject(parent), m_path(storePath)
{}

// ── Persistence ──────────────────────────────────────────────────────────────
void PasswordManager::load() {
    m_creds.clear();
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    for (const auto &v : doc.array()) {
        QJsonObject o = v.toObject();
        Credential c;
        c.host     = o["host"].toString();
        c.username = o["username"].toString();
        c.password = deobfuscate(o["password"].toString());
        c.url      = o["url"].toString();
        m_creds.append(c);
    }
}

void PasswordManager::save() {
    QJsonArray arr;
    for (const auto &c : m_creds) {
        QJsonObject o;
        o["host"]     = c.host;
        o["username"] = c.username;
        o["password"] = obfuscate(c.password);
        o["url"]      = c.url;
        arr.append(o);
    }
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

// ── CRUD ─────────────────────────────────────────────────────────────────────
void PasswordManager::addCredential(const Credential &c) {
    // Update existing or append
    for (auto &existing : m_creds) {
        if (existing.host == c.host && existing.username == c.username) {
            existing = c;
            save();
            return;
        }
    }
    m_creds.append(c);
    save();
}

void PasswordManager::removeCredential(const QString &host, const QString &username) {
    m_creds.removeIf([&](const Credential &c) {
        return c.host == host && c.username == username;
    });
    save();
}

QList<Credential> PasswordManager::credentialsForHost(const QString &host) const {
    QList<Credential> result;
    for (const auto &c : m_creds)
        if (c.host == host || host.endsWith("." + c.host) || c.host.endsWith("." + host))
            result.append(c);
    return result;
}

QList<Credential> PasswordManager::allCredentials() const { return m_creds; }

// ── Autofill ─────────────────────────────────────────────────────────────────
void PasswordManager::autofill(QWebEnginePage *page, const QString &host) {
    auto creds = credentialsForHost(host);
    if (creds.isEmpty()) return;
    // Use first matching credential
    const auto &c = creds.first();
    QString js = QString(R"JS(
(function() {
    const user = %1;
    const pass = %2;
    // Find username field
    const uField = document.querySelector(
        'input[type="email"], input[type="text"][name*="user"], input[type="text"][name*="email"], input[autocomplete="username"], input[autocomplete="email"]'
    );
    const pField = document.querySelector('input[type="password"]');
    if (uField) { uField.value = user; uField.dispatchEvent(new Event('input', {bubbles:true})); }
    if (pField) { pField.value = pass; pField.dispatchEvent(new Event('input', {bubbles:true})); }
})();
)JS")
        .arg(QJsonValue(c.username).toString().prepend("\"").append("\""),
             QJsonValue(c.password).toString().prepend("\"").append("\""));

    // Properly escape for JS string
    QString safeUser = c.username; safeUser.replace("\"","\\\"");
    QString safePass = c.password; safePass.replace("\"","\\\"");
    js = QString(R"JS(
(function() {
    const user = "%1";
    const pass = "%2";
    const uField = document.querySelector(
        'input[type="email"], input[autocomplete="username"], input[autocomplete="email"], input[name*="user"], input[name*="email"], input[id*="user"], input[id*="email"]'
    );
    const pField = document.querySelector('input[type="password"]');
    if (uField) { uField.focus(); uField.value = user; uField.dispatchEvent(new Event('input',{bubbles:true})); uField.dispatchEvent(new Event('change',{bubbles:true})); }
    if (pField) { pField.focus(); pField.value = pass; pField.dispatchEvent(new Event('input',{bubbles:true})); pField.dispatchEvent(new Event('change',{bubbles:true})); }
})();
)JS").arg(safeUser, safePass);

    page->runJavaScript(js);
}

// ── Capture form submit ───────────────────────────────────────────────────────
void PasswordManager::injectCapture(QWebEnginePage *page) {
    // This JS watches for password field + form submission, then sends credentials
    // back via the page's title (a simple side-channel since QWebChannel requires setup)
    page->runJavaScript(R"JS(
(function() {
    if (window.__sfPwCapture) return;
    window.__sfPwCapture = true;
    document.addEventListener('submit', function(e) {
        const form = e.target;
        const uField = form.querySelector('input[type="email"], input[autocomplete="username"], input[type="text"]');
        const pField = form.querySelector('input[type="password"]');
        if (uField && pField && pField.value) {
            const data = JSON.stringify({
                type: '__sf_credentials__',
                host: location.hostname,
                username: uField.value,
                password: pField.value,
                url: location.href
            });
            // Send via title temporarily
            const old = document.title;
            document.title = data;
            setTimeout(() => { document.title = old; }, 500);
        }
    }, true);
})();
)JS");
}

// ── Manager dialog ────────────────────────────────────────────────────────────
void PasswordManager::showManagerDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("🔑 Password Manager");
    dlg.setMinimumSize(700, 450);

    auto *layout = new QVBoxLayout(&dlg);

    auto *table = new QTableWidget(0, 4, &dlg);
    table->setHorizontalHeaderLabels({"Host", "Username", "Password", "URL"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    layout->addWidget(table);

    auto reload = [&]() {
        table->setRowCount(0);
        for (const auto &c : m_creds) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(c.host));
            table->setItem(row, 1, new QTableWidgetItem(c.username));
            table->setItem(row, 2, new QTableWidgetItem("••••••••"));
            table->setItem(row, 3, new QTableWidgetItem(c.url));
        }
    };
    reload();

    auto *btnRow = new QHBoxLayout;
    auto *showBtn   = new QPushButton("👁 Show Password");
    auto *addBtn    = new QPushButton("➕ Add");
    auto *deleteBtn = new QPushButton("🗑 Delete");
    auto *closeBtn  = new QPushButton("Close");
    for (auto *b : {showBtn, addBtn, deleteBtn, closeBtn}) btnRow->addWidget(b);
    layout->addLayout(btnRow);

    connect(showBtn, &QPushButton::clicked, &dlg, [&]() {
        int row = table->currentRow();
        if (row < 0 || row >= m_creds.size()) return;
        bool show = table->item(row, 2)->text() == "••••••••";
        table->item(row, 2)->setText(show ? m_creds[row].password : "••••••••");
    });

    connect(addBtn, &QPushButton::clicked, &dlg, [&]() {
        bool ok;
        QString host = QInputDialog::getText(&dlg, "Add Credential", "Host:", QLineEdit::Normal, "", &ok);
        if (!ok || host.isEmpty()) return;
        QString user = QInputDialog::getText(&dlg, "Add Credential", "Username:", QLineEdit::Normal, "", &ok);
        if (!ok) return;
        QString pass = QInputDialog::getText(&dlg, "Add Credential", "Password:", QLineEdit::Password, "", &ok);
        if (!ok) return;
        addCredential({host, user, pass, ""});
        reload();
    });

    connect(deleteBtn, &QPushButton::clicked, &dlg, [&]() {
        int row = table->currentRow();
        if (row < 0 || row >= m_creds.size()) return;
        auto btn = QMessageBox::question(&dlg, "Delete", "Delete this credential?");
        if (btn != QMessageBox::Yes) return;
        removeCredential(m_creds[row].host, m_creds[row].username);
        reload();
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
