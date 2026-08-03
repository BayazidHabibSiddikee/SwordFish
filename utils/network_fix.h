#pragma once
// utils/network_fix.h — Randomise MAC address and request a new DHCP lease

#include <QString>

namespace NetworkFix {

    /**
     * Generate a random locally-administered unicast MAC address.
     * e.g. "02:ab:cd:ef:01:23"
     */
    QString randomMac();

    /**
     * Detect the default network interface by parsing `ip route get 1.1.1.1`.
     * Falls back to eth0, wlan0, wlp2s0, enp3s0 if detection fails.
     */
    QString detectInterface();

    /**
     * Bring iface down, change its MAC, bring it back up.
     * Requires root privileges.
     */
    bool setMac(const QString &iface, const QString &mac, QString &errorOut);

    /**
     * Ask a DHCP client (dhclient / dhcpcd / nmcli) to request a new lease.
     * Returns the name of the client that was used, or empty string if none found.
     */
    QString requestDhcp(const QString &iface);

    /**
     * Check internet connectivity by pinging host (default 8.8.8.8).
     */
    bool checkConnectivity(const QString &host = "8.8.8.8", int timeoutSec = 3);

    /**
     * Main entry point: randomise MAC and get a new DHCP lease.
     * @param iface  Interface name; pass empty string to auto-detect.
     * @param ok     Set to true on success.
     * @return Human-readable status message.
     */
    QString changeIp(const QString &iface = QString(), bool *ok = nullptr);

}  // namespace NetworkFix
