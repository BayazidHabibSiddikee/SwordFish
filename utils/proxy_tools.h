#pragma once
// utils/proxy_tools.h — Proxy list management and IP check utilities
// Equivalent of the Python utils/proxy_tools.py

#include <QString>
#include <QStringList>

namespace ProxyTools {

    /**
     * Load proxies from the proxies.txt file next to this source.
     * Each line: scheme://host:port (lines starting with # are ignored).
     */
    QStringList loadProxies(const QString &proxiesFilePath = QString());

    /**
     * Save the proxy list to proxies.txt.
     */
    void saveProxies(const QStringList &proxies, const QString &proxiesFilePath = QString());

    /**
     * Return a random proxy from the list, or empty string if the list is empty.
     */
    QString getRandomProxy(const QStringList &proxies = QStringList());

    /**
     * Fetch the current public IP, optionally through a proxy.
     * @param proxyUrl  e.g. "socks5://127.0.0.1:9050" or empty for no proxy.
     * @param timeoutSec  HTTP request timeout in seconds.
     * @return IP address string, or "Unreachable" on failure.
     */
    QString getCurrentIp(const QString &proxyUrl = QString(), int timeoutSec = 8);

    /**
     * Check whether a given proxy URL is functional.
     * Returns true if a test request through it succeeds within timeoutSec.
     */
    bool isProxyWorking(const QString &proxyUrl, int timeoutSec = 5);

    /**
     * Convert a proxy URL to a Chromium --proxy-server flag value.
     * For SOCKS5 proxies, strips the "socks5://" prefix for Chromium's format.
     */
    QString chromeFlagForProxy(const QString &proxyUrl);

}  // namespace ProxyTools
