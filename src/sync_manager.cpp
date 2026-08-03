// src/sync_manager.cpp — Export/import bookmarks+history + file-based sync
#include "sync_manager.h"
#include "file_picker.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileSystemWatcher>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>

class SyncManager::Private {
public:
    QFileSystemWatcher watcher;
};

SyncManager::SyncManager(QObject *parent)
    : QObject(parent), d(new Private)
{
    connect(&d->watcher, &QFileSystemWatcher::fileChanged,
            this, &SyncManager::syncFileChanged);
}

// ── Export ────────────────────────────────────────────────────────────────
QString SyncManager::exportData(const QJsonObject &data, const QString &path,
                                QWidget *parent) {
    QString savePath = path;
    if (savePath.isEmpty()) {
        savePath = FilePicker::getSaveFileName(parent, "Export Sync Data",
            QDir::homePath() + "/swordfish_sync_"
                + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".json",
            "JSON (*.json);;All (*)");
    }
    if (savePath.isEmpty()) return {};

    // Build export object: bookmarks + history
    QJsonObject exportObj;
    exportObj["version"]   = 2;
    exportObj["exported"]  = QDateTime::currentDateTime().toString(Qt::ISODate);
    exportObj["bookmarks"] = data["bookmarks"].toArray();
    exportObj["history"]   = data["history"].toArray();

    QFile f(savePath);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(parent, "Sync", "Could not write to:\n" + savePath);
        return {};
    }
    f.write(QJsonDocument(exportObj).toJson(QJsonDocument::Indented));
    return savePath;
}

// ── Import ────────────────────────────────────────────────────────────────
QJsonObject SyncManager::importData(const QString &path, QWidget *parent) {
    QString loadPath = path;
    if (loadPath.isEmpty()) {
        loadPath = FilePicker::getOpenFileName(parent, "Import Sync Data",
            QDir::homePath(), "JSON (*.json);;All (*)");
    }
    if (loadPath.isEmpty()) return {};

    QFile f(loadPath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(parent, "Sync", "Could not open:\n" + loadPath);
        return {};
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::warning(parent, "Sync", "Invalid JSON: " + err.errorString());
        return {};
    }
    return doc.object();
}

// ── File watch ────────────────────────────────────────────────────────────
void SyncManager::watchFile(const QString &path) {
    if (!path.isEmpty()) d->watcher.addPath(path);
}

void SyncManager::stopWatching() {
    if (!d->watcher.files().isEmpty())
        d->watcher.removePaths(d->watcher.files());
}

// ── Sync dialog ───────────────────────────────────────────────────────────
void SyncManager::showSyncDialog(QJsonObject &data, QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("🔄 Sync — Bookmarks & History");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    // Export group
    auto *exportGrp = new QGroupBox("Export", &dlg);
    auto *el = new QVBoxLayout(exportGrp);
    auto *exportBtn = new QPushButton("💾 Export bookmarks + history to JSON…");
    el->addWidget(exportBtn);
    layout->addWidget(exportGrp);

    // Import group
    auto *importGrp = new QGroupBox("Import / Merge", &dlg);
    auto *il = new QVBoxLayout(importGrp);
    il->addWidget(new QLabel("Import merges bookmarks and history — duplicates are skipped."));
    auto *importBtn = new QPushButton("📂 Import from JSON…");
    il->addWidget(importBtn);
    layout->addWidget(importGrp);

    // Auto-sync group
    auto *watchGrp = new QGroupBox("Auto-Sync (watch a shared file)", &dlg);
    auto *wl = new QHBoxLayout(watchGrp);
    auto *watchEdit = new QLineEdit(&dlg);
    watchEdit->setPlaceholderText("Path to shared sync JSON (e.g. Dropbox/swordfish_sync.json)");
    if (!d->watcher.files().isEmpty())
        watchEdit->setText(d->watcher.files().first());
    auto *watchBtn  = new QPushButton("Watch");
    auto *stopBtn   = new QPushButton("Stop");
    wl->addWidget(watchEdit); wl->addWidget(watchBtn); wl->addWidget(stopBtn);
    layout->addWidget(watchGrp);

    auto *statusLbl = new QLabel("", &dlg);
    layout->addWidget(statusLbl);

    auto *closeBtn = new QPushButton("Close", &dlg);
    layout->addWidget(closeBtn);

    connect(exportBtn, &QPushButton::clicked, &dlg, [&]() {
        QString path = exportData(data, {}, parent);
        if (!path.isEmpty())
            statusLbl->setText("✔ Exported to: " + path);
    });

    connect(importBtn, &QPushButton::clicked, &dlg, [&]() {
        QJsonObject imported = importData({}, parent);
        if (imported.isEmpty()) return;
        // Merge bookmarks
        QJsonArray bm = data["bookmarks"].toArray();
        QSet<QString> existingUrls;
        for (const auto &v : bm) existingUrls.insert(v.toObject()["url"].toString());
        for (const auto &v : imported["bookmarks"].toArray()) {
            QString url = v.toObject()["url"].toString();
            if (!existingUrls.contains(url)) { bm.append(v); existingUrls.insert(url); }
        }
        data["bookmarks"] = bm;
        // Merge history
        QJsonArray hist = data["history"].toArray();
        QSet<QString> existingHist;
        for (const auto &v : hist) existingHist.insert(v.toObject()["url"].toString());
        for (const auto &v : imported["history"].toArray()) {
            QString url = v.toObject()["url"].toString();
            if (!existingHist.contains(url)) { hist.append(v); existingHist.insert(url); }
        }
        data["history"] = hist;
        statusLbl->setText(QString("✔ Imported %1 bookmarks, %2 history entries")
            .arg(imported["bookmarks"].toArray().size())
            .arg(imported["history"].toArray().size()));
    });

    connect(watchBtn, &QPushButton::clicked, &dlg, [&]() {
        stopWatching();
        watchFile(watchEdit->text().trimmed());
        statusLbl->setText("👁 Watching: " + watchEdit->text().trimmed());
    });
    connect(stopBtn, &QPushButton::clicked, &dlg, [&]() {
        stopWatching();
        statusLbl->setText("Stopped watching.");
    });
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
