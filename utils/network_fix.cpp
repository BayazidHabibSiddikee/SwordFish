// utils/network_fix.cpp — Randomise MAC address and request a new DHCP lease
// Equivalent of the Python utils/network_fix.py

#include "network_fix.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>

namespace NetworkFix {

// ── Helpers ──────────────────────────────────────────────────────────────

static bool commandExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

static int runCmd(const QStringList &args, QString *stdoutOut = nullptr) {
    if (args.isEmpty()) return -1;
    QProcess p;
    p.start(args.first(), args.mid(1));
    p.waitForFinished(10000);
    if (stdoutOut) *stdoutOut = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    return p.exitCode();
}

// ── Public API ────────────────────────────────────────────────────────────

QString randomMac() {
    auto *rng = QRandomGenerator::global();

    // First byte: locally administered (bit 1 set) and unicast (bit 0 clear)
    quint8 first = static_cast<quint8>((rng->bounded(256) & 0xFE) | 0x02);

    QStringList parts;
    parts << QString("%1").arg(first, 2, 16, QChar('0'));
    for (int i = 0; i < 5; ++i) {
        quint8 b = static_cast<quint8>(rng->bounded(256));
        parts << QString("%1").arg(b, 2, 16, QChar('0'));
    }
    return parts.join(':');
}

QString detectInterface() {
    QProcess p;
    p.start("sh", QStringList() << "-c" << "ip route get 1.1.1.1 2>/dev/null");
    p.waitForFinished(5000);
    QString out = QString::fromUtf8(p.readAllStandardOutput());

    QRegularExpression re(R"(dev\s+(\S+))");
    auto m = re.match(out);
    if (m.hasMatch()) return m.captured(1);

    // Fallbacks
    for (const QString &iface : {"eth0", "wlan0", "wlp2s0", "enp3s0"}) {
        if (QFile::exists("/sys/class/net/" + iface + "/operstate"))
            return iface;
    }
    return QStringLiteral("eth0");
}

bool setMac(const QString &iface, const QString &mac, QString &errorOut) {
    auto run = [&](const QStringList &args) -> bool {
        QProcess p;
        p.start(args.first(), args.mid(1));
        p.waitForFinished(10000);
        if (p.exitCode() != 0) {
            errorOut = QString::fromUtf8(p.readAllStandardError()).trimmed();
            return false;
        }
        return true;
    };

    if (!run({"ip", "link", "set", "dev", iface, "down"})) return false;
    if (!run({"ip", "link", "set", "dev", iface, "address", mac})) return false;
    if (!run({"ip", "link", "set", "dev", iface, "up"})) return false;
    return true;
}

QString requestDhcp(const QString &iface) {
    // Try dhclient first, then dhcpcd, then nmcli
    if (commandExists("dhclient")) {
        QProcess::execute("dhclient", QStringList() << "-r" << iface);
        QProcess::execute("dhclient", QStringList() << iface);
        return QStringLiteral("dhclient");
    }
    if (commandExists("dhcpcd")) {
        QProcess::execute("dhcpcd", QStringList() << "-k" << iface);
        QProcess::execute("dhcpcd", QStringList() << iface);
        return QStringLiteral("dhcpcd");
    }
    if (commandExists("nmcli")) {
        QProcess::execute("nmcli", QStringList() << "device" << "disconnect" << iface);
        QProcess::execute("nmcli", QStringList() << "device" << "connect" << iface);
        return QStringLiteral("nmcli");
    }
    return QString();  // No DHCP client found
}

bool checkConnectivity(const QString &host, int timeoutSec) {
    QProcess p;
    p.start("ping", QStringList() << "-c" << "1"
            << "-W" << QString::number(timeoutSec) << host);
    p.waitForFinished((timeoutSec + 2) * 1000);
    return p.exitCode() == 0;
}

QString changeIp(const QString &ifaceArg, bool *ok) {
    auto setOk = [&](bool v) { if (ok) *ok = v; };

    // Must be root
    if (QProcess::execute("sh", QStringList() << "-c" << "[ $(id -u) -eq 0 ]") != 0) {
        setOk(false);
        return QStringLiteral("Root required. Run via pkexec/sudo.");
    }

    QString iface = ifaceArg.isEmpty() ? detectInterface() : ifaceArg;
    QString mac = randomMac();

    QString errMsg;
    if (!setMac(iface, mac, errMsg)) {
        setOk(false);
        return QString("Failed to set MAC on %1: %2").arg(iface, errMsg);
    }

    QString dhcp = requestDhcp(iface);
    setOk(true);
    if (!dhcp.isEmpty())
        return QString("MAC → %1 on %2, DHCP via %3").arg(mac, iface, dhcp);
    else
        return QString("MAC → %1 on %2 (renew DHCP lease manually)").arg(mac, iface);
}

}  // namespace NetworkFix
