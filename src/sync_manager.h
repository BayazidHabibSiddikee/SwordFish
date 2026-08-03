#pragma once
// src/sync_manager.h — Export/import bookmarks+history, optional file-based sync

#include <QObject>
#include <QString>
#include <QJsonObject>

class QWidget;

class SyncManager : public QObject {
    Q_OBJECT
public:
    explicit SyncManager(QObject *parent = nullptr);

    /** Export bookmarks + history to a JSON file. Returns path on success. */
    QString exportData(const QJsonObject &data, const QString &path = QString(),
                       QWidget *parent = nullptr);

    /** Import from a JSON file. Merges into existing data. */
    QJsonObject importData(const QString &path = QString(),
                           QWidget *parent = nullptr);

    /** Watch a sync file and emit syncFileChanged when it updates. */
    void watchFile(const QString &path);
    void stopWatching();

    /** Show sync dialog (export/import/watch). */
    void showSyncDialog(QJsonObject &data, QWidget *parent = nullptr);

signals:
    void syncFileChanged(const QString &path);

private:
    class Private;
    Private *d;
};
