#pragma once
// src/password_manager.h — Save, autofill and manage login credentials

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>

struct Credential {
    QString host;
    QString username;
    QString password;   // stored XOR-obfuscated in JSON (not crypto-secure, but not plaintext)
    QString url;
};

class QWebEnginePage;

class PasswordManager : public QObject {
    Q_OBJECT
public:
    explicit PasswordManager(const QString &storePath, QObject *parent = nullptr);

    // Load / save store
    void load();
    void save();

    // CRUD
    void addCredential(const Credential &c);
    void removeCredential(const QString &host, const QString &username);
    QList<Credential> credentialsForHost(const QString &host) const;
    QList<Credential> allCredentials() const;

    // Autofill — injects JS into the page to fill username+password fields
    void autofill(QWebEnginePage *page, const QString &host);

    // Capture — injects JS that listens for form submit and signals back
    void injectCapture(QWebEnginePage *page);

    // Show manager dialog
    void showManagerDialog(QWidget *parent = nullptr);

signals:
    void credentialCaptured(const QString &host, const QString &username, const QString &password);

private:
    static QString obfuscate(const QString &s);
    static QString deobfuscate(const QString &s);

    QString           m_path;
    QList<Credential> m_creds;
};
