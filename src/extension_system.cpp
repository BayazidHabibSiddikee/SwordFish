// src/extension_system.cpp — Load userscripts from ~/.config/SwordFish/extensions/
#include "extension_system.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTextEdit>
#include <QNetworkRequest>
#include <QInputDialog>
#include <functional>

ExtensionSystem::ExtensionSystem(const QString &extensionsDir,
                                 QWebEngineProfile *profile,
                                 QObject *parent)
    : QObject(parent), m_dir(extensionsDir), m_profile(profile)
{
    QDir().mkpath(m_dir);

    // Drop a sample script on first run
    QString sample = m_dir + "/example_darkscrollbar.js";
    if (!QFile::exists(sample)) {
        QFile f(sample);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "// ==UserScript==\n"
               << "// @name    Dark Scrollbar\n"
               << "// @match   *\n"
               << "// @version 1.0\n"
               << "// ==/UserScript==\n\n"
               << "(function() {\n"
               << "    const s = document.createElement('style');\n"
               << "    s.textContent = '::-webkit-scrollbar{width:8px;height:8px}"
               << "::-webkit-scrollbar-track{background:#0d1117}"
               << "::-webkit-scrollbar-thumb{background:#00b4d8;border-radius:4px}';\n"
               << "    document.head && document.head.appendChild(s);\n"
               << "})();\n";
        }
    }
}

// ── Parse @metadata from UserScript header ────────────────────────────────
static UserScript parseScript(const QString &path, const QString &source) {
    UserScript s;
    s.path    = path;
    s.source  = source;
    s.enabled = true;
    s.name    = QFileInfo(path).baseName();
    s.match   = "*";  // default: run on all pages

    for (const QString &line : source.split('\n')) {
        QString t = line.trimmed();
        if (t.startsWith("// @name"))
            s.name = t.mid(8).trimmed();
        else if (t.startsWith("// @match"))
            s.match = t.mid(9).trimmed();
    }
    return s;
}

void ExtensionSystem::loadAll() {
    unloadAll();
    m_scripts.clear();

    QDir dir(m_dir);
    const auto entries = dir.entryInfoList({"*.js"}, QDir::Files, QDir::Name);
    for (const QFileInfo &fi : entries) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QString src = QTextStream(&f).readAll();
        UserScript s = parseScript(fi.absoluteFilePath(), src);
        m_scripts.append(s);
        if (s.enabled) injectScript(s);
    }
}

void ExtensionSystem::injectScript(const UserScript &s) {
    // Wrap source in a URL match guard so @match is actually enforced.
    // Pattern '*' means run on all pages (no guard needed).
    QString guardedSource;
    if (s.match.trimmed() == "*" || s.match.trimmed().isEmpty()) {
        guardedSource = s.source;
    } else {
        // Convert glob-style @match to a JS regex:
        // e.g. "https://example.com/*" → escaped, * → .*
        QString pat = QRegularExpression::escape(s.match);
        pat.replace("\\*", ".*");
        guardedSource = QString(
            "(function() {\n"
            "  if (!/%1/.test(location.href)) return;\n"
            "%2\n"
            "})();\n"
        ).arg(pat, s.source);
    }

    QWebEngineScript ws;
    ws.setName("ext_" + s.name);
    ws.setSourceCode(guardedSource);
    ws.setInjectionPoint(QWebEngineScript::DocumentReady);
    ws.setWorldId(QWebEngineScript::MainWorld);
    ws.setRunsOnSubFrames(false);
    m_profile->scripts()->insert(ws);
}

void ExtensionSystem::removeScript(const QString &name) {
    auto list = m_profile->scripts()->toList();
    for (const auto &ws : list)
        if (ws.name() == "ext_" + name)
            m_profile->scripts()->remove(ws);
}

void ExtensionSystem::unloadAll() {
    for (const auto &s : m_scripts)
        removeScript(s.name);
}

void ExtensionSystem::reload(const QString &name) {
    removeScript(name);
    for (auto &s : m_scripts) {
        if (s.name == name) {
            QFile f(s.path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text))
                s.source = QTextStream(&f).readAll();
            if (s.enabled) injectScript(s);
            break;
        }
    }
}

void ExtensionSystem::setEnabled(const QString &name, bool enabled) {
    for (auto &s : m_scripts) {
        if (s.name == name) {
            s.enabled = enabled;
            if (enabled) injectScript(s);
            else         removeScript(name);
            break;
        }
    }
}

// ── Install from URL ──────────────────────────────────────────────────────
void ExtensionSystem::installFromUrl(const QString &rawUrl,
                                     std::function<void(bool, const QString &)> onDone)
{
    // Greasy Fork convenience: if user pastes the script page URL instead of
    // the raw .user.js URL, convert it automatically.
    //   https://greasyfork.org/en/scripts/12345-name  →
    //   https://greasyfork.org/scripts/12345/code/name.user.js
    QString url = rawUrl.trimmed();
    static const QRegularExpression s_gfPage(
        R"(https?://greasyfork\.org/[a-z-]+/scripts/(\d+)(?:-[^/?#]*)?)");
    QRegularExpressionMatch m = s_gfPage.match(url);
    if (m.hasMatch() && !url.endsWith(".user.js")) {
        // Use the direct install URL format Greasy Fork provides
        url = QString("https://greasyfork.org/scripts/%1/code/script.user.js")
                  .arg(m.captured(1));
    }

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36");

    QNetworkReply *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, onDone]() mutable {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onDone(false, "Network error: " + reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            onDone(false, "Downloaded file is empty.");
            return;
        }

        QString source = QString::fromUtf8(data);

        // Must look like a userscript — require ==UserScript== header
        if (!source.contains("==UserScript==")) {
            onDone(false, "The URL does not point to a valid UserScript "
                          "(missing ==UserScript== header).");
            return;
        }

        // Derive a safe filename from @name metadata or the URL
        QString scriptName;
        for (const QString &line : source.split('\n')) {
            QString t = line.trimmed();
            if (t.startsWith("// @name")) {
                scriptName = t.mid(8).trimmed();
                break;
            }
        }
        if (scriptName.isEmpty()) {
            // Fall back to last path segment of URL
            scriptName = QUrl(reply->url()).fileName();
        }
        // Sanitise: keep only alphanumeric, dash, underscore, space
        scriptName.replace(QRegularExpression(R"([^\w\s-])"), "_");
        scriptName = scriptName.trimmed();
        if (!scriptName.endsWith(".user.js") && !scriptName.endsWith(".js"))
            scriptName += ".user.js";

        QString savePath = m_dir + "/" + scriptName;

        // Don't overwrite silently — append a counter if file exists
        if (QFile::exists(savePath)) {
            int n = 1;
            QString base = savePath;
            base.replace(".user.js", "").replace(".js", "");
            while (QFile::exists(savePath))
                savePath = QString("%1_%2.user.js").arg(base).arg(n++);
        }

        QFile f(savePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            onDone(false, "Could not write file: " + savePath);
            return;
        }
        QTextStream(&f) << source;
        f.close();

        onDone(true, scriptName);
    });
}

// ── Manager dialog ────────────────────────────────────────────────────────
void ExtensionSystem::showManagerDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("🧩 Extensions");
    dlg.setMinimumSize(560, 380);

    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QString("Extensions folder: <b>%1</b>").arg(m_dir)));

    auto *list = new QListWidget(&dlg);
    list->setAlternatingRowColors(true);
    layout->addWidget(list);

    auto refresh = [&]() {
        list->clear();
        for (const auto &s : m_scripts) {
            auto *item = new QListWidgetItem(
                QString("%1  [%2]  — match: %3")
                    .arg(s.name, s.enabled ? "✔ ON" : "✗ OFF", s.match));
            item->setData(Qt::UserRole, s.name);
            list->addItem(item);
        }
    };
    refresh();

    auto *btnRow = new QHBoxLayout;
    auto *toggleBtn  = new QPushButton("Toggle On/Off");
    auto *reloadBtn  = new QPushButton("⟳ Reload All");
    auto *installBtn = new QPushButton("🌐 Install from URL");
    auto *openDirBtn = new QPushButton("📂 Open Folder");
    auto *closeBtn   = new QPushButton("Close");
    for (auto *b : {toggleBtn, reloadBtn, installBtn, openDirBtn, closeBtn})
        btnRow->addWidget(b);
    layout->addLayout(btnRow);

    auto *statusLabel = new QLabel("", &dlg);
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);

    connect(toggleBtn, &QPushButton::clicked, &dlg, [&]() {
        auto *item = list->currentItem();
        if (!item) return;
        QString name = item->data(Qt::UserRole).toString();
        for (auto &s : m_scripts) {
            if (s.name == name) { setEnabled(name, !s.enabled); break; }
        }
        refresh();
    });
    connect(reloadBtn,  &QPushButton::clicked, &dlg, [&]() { loadAll(); refresh(); });
    connect(openDirBtn, &QPushButton::clicked, &dlg, [&]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_dir));
    });

    connect(installBtn, &QPushButton::clicked, &dlg, [&]() {
        bool ok;
        QString url = QInputDialog::getText(
            &dlg,
            "Install UserScript from URL",
            "Paste a .user.js URL or a Greasy Fork script page URL:",
            QLineEdit::Normal,
            QString(),
            &ok
        );
        if (!ok || url.trimmed().isEmpty()) return;

        installBtn->setEnabled(false);
        statusLabel->setText("⏳ Downloading…");
        statusLabel->setStyleSheet("color: #555;");

        installFromUrl(url, [&, installBtn, statusLabel](bool success, const QString &msg) {
            installBtn->setEnabled(true);
            if (success) {
                statusLabel->setText("✅ Installed: " + msg);
                statusLabel->setStyleSheet("color: green; font-weight: bold;");
                loadAll();
                refresh();
            } else {
                statusLabel->setText("❌ Failed: " + msg);
                statusLabel->setStyleSheet("color: red;");
            }
        });
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
