// utils/proxy_tools.cpp — Proxy list management and IP check utilities
// Equivalent of the Python utils/proxy_tools.py

#include "proxy_tools.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QRandomGenerator>
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>

namespace ProxyTools {

// ── Helpers ──────────────────────────────────────────────────────────────

static QString defaultProxiesPath() {
    // proxies.txt sits next to the binary (or the source utils/ dir at runtime)
    return QCoreApplication::applicationDirPath() + "/../utils/proxies.txt";
}

static QString resolvedPath(const QString &override) {
    return override.isEmpty() ? defaultProxiesPath() : override;
}

// ── Public API ────────────────────────────────────────────────────────────

QStringList loadProxies(const QString &proxiesFilePath) {
    QStringList result;
    QFile f(resolvedPath(proxiesFilePath));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return result;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#'))
            result << line;
    }
    return result;
}

void saveProxies(const QStringList &proxies, const QString &proxiesFilePath) {
    QFile f(resolvedPath(proxiesFilePath));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    QTextStream out(&f);
    out << "# One proxy per line\n";
    out << "# Format: scheme://host:port\n";
    out << "# scheme: socks5, socks4, http, https\n";
    for (const auto &p : proxies) out << p << '\n';
}

QString getRandomProxy(const QStringList &proxiesArg) {
    QStringList list = proxiesArg.isEmpty() ? loadProxies() : proxiesArg;
    if (list.isEmpty()) return QString();
    int idx = static_cast<int>(QRandomGenerator::global()->bounded(static_cast<quint32>(list.size())));
    return list.at(idx);
}

QString getCurrentIp(const QString &proxyUrl, int timeoutSec) {
    // Use curl — widely available and supports socks5/http proxies natively
    QStringList curlArgs;
    curlArgs << "-s" << "--max-time" << QString::number(timeoutSec);

    if (!proxyUrl.isEmpty())
        curlArgs << "--proxy" << proxyUrl;

    curlArgs << "https://api.ipify.org";

    QProcess p;
    p.start("curl", curlArgs);
    p.waitForFinished((timeoutSec + 2) * 1000);

    QString ip = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (!ip.isEmpty() && p.exitCode() == 0) return ip;

    // Fallback endpoint
    QStringList fallbackArgs = curlArgs;
    fallbackArgs.removeLast();
    fallbackArgs << "https://httpbin.org/ip";

    QProcess p2;
    p2.start("curl", fallbackArgs);
    p2.waitForFinished((timeoutSec + 2) * 1000);

    QString out2 = QString::fromUtf8(p2.readAllStandardOutput()).trimmed();
    // Parse "origin" field from JSON manually (avoid pulling in a full parser here)
    QRegularExpression re(R"--("origin"\s*:\s*"([^"]+)")--");
    auto m = re.match(out2);
    if (m.hasMatch()) return m.captured(1);

    return QStringLiteral("Unreachable");
}

bool isProxyWorking(const QString &proxyUrl, int timeoutSec) {
    if (proxyUrl.isEmpty()) return false;
    QProcess p;
    p.start("curl", QStringList()
            << "-s" << "-o" << "/dev/null"
            << "--max-time" << QString::number(timeoutSec)
            << "--proxy" << proxyUrl
            << "https://api.ipify.org");
    p.waitForFinished((timeoutSec + 2) * 1000);
    return p.exitCode() == 0;
}

QString chromeFlagForProxy(const QString &proxyUrl) {
    // Chromium accepts socks5://host:port directly, same as our stored format
    return proxyUrl;
}

}  // namespace ProxyTools
