#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <regex>
#include <QUrl>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>

class AdBlocker : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT
public:
    enum class Level { None, Low, Medium, Ultimate };

    explicit AdBlocker(QObject *parent = nullptr, Level level = Level::Medium);

    void setLevel(Level level);
    Level level() const { return m_level; }

    // QWebEngineUrlRequestInterceptor — actually blocks network requests
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    bool shouldBlock(const QString &url, const QString &sourceUrl = QString()) const;
    bool checkContentViolent(const QString &text, const QString &title = QString()) const;
    bool checkContentAdult(const QString &text, const QString &title = QString()) const;

private:
    bool domainMatches(const QString &url) const;
    bool pathMatches(const QString &url) const;

    Level m_level;
    mutable std::unordered_map<std::string, bool> m_cache;
    static std::unordered_set<std::string> s_adDomains;
    static std::regex s_adPathPattern;
    static std::regex s_adultKeywords;
    static std::regex s_violentKeywords;
};

AdBlocker &getBlocker();
