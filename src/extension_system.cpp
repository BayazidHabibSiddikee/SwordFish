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
    auto *openDirBtn = new QPushButton("📂 Open Folder");
    auto *closeBtn   = new QPushButton("Close");
    for (auto *b : {toggleBtn, reloadBtn, openDirBtn, closeBtn}) btnRow->addWidget(b);
    layout->addLayout(btnRow);

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
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
