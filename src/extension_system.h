#pragma once
// src/extension_system.h — Load and run userscripts as browser extensions

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QWebEngineProfile;
class QWidget;

struct UserScript {
    QString name;
    QString path;
    QString source;
    bool    enabled = true;
    QString match;   // URL match pattern e.g. "*youtube.com*"
};

class ExtensionSystem : public QObject {
    Q_OBJECT
public:
    explicit ExtensionSystem(const QString &extensionsDir,
                             QWebEngineProfile *profile,
                             QObject *parent = nullptr);

    void loadAll();
    void reload(const QString &name);
    void unloadAll();
    void setEnabled(const QString &name, bool enabled);

    // Download a .user.js from a URL and save it to the extensions folder.
    // Calls onDone(success, message) when finished.
    void installFromUrl(const QString &url,
                        std::function<void(bool, const QString &)> onDone);

    QList<UserScript> scripts() const { return m_scripts; }
    QString extensionsDir() const { return m_dir; }

    void showManagerDialog(QWidget *parent = nullptr);

private:
    void injectScript(const UserScript &s);
    void removeScript(const QString &name);

    QString            m_dir;
    QWebEngineProfile *m_profile;
    QList<UserScript>  m_scripts;
    QNetworkAccessManager m_nam;
};
