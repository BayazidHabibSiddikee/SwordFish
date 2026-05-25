#!/usr/bin/env python3
"""
Randomise MAC address of a network interface and request a new DHCP lease.

Usage:
    sudo python -m utils.network_fix --iface eth0
"""

import argparse
import os
import random
import re
import shutil
import subprocess
import sys


def random_mac():
    first = random.randrange(0x00, 0xFF) & 0b11111110 | 0b00000010
    mac = [first] + [random.randrange(0x00, 0xFF) for _ in range(5)]
    return ":".join(f"{b:02x}" for b in mac)


def run_cmd(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Command failed: {cmd}\n{result.stderr}", file=sys.stderr)
        sys.exit(result.returncode)
    return result.stdout.strip()


def set_mac(iface, mac):
    run_cmd(f"ip link set dev {iface} down")
    run_cmd(f"ip link set dev {iface} address {mac}")
    run_cmd(f"ip link set dev {iface} up")


def request_dhcp(iface):
    for client in ("dhclient", "dhcpcd", "dhcpcd5", "nmcli"):
        if shutil.which(client):
            if client == "nmcli":
                run_cmd(f"nmcli device disconnect {iface} && nmcli device connect {iface}")
            else:
                run_cmd(f"{client} -r {iface} 2>/dev/null") # Release
                run_cmd(f"{client} -v {iface} 2>/dev/null") # Request
            return client
    return None


def check_connectivity(host="8.8.8.8", timeout=3):
    """Check if the internet is reachable."""
    try:
        subprocess.run(
            ["ping", "-c", "1", "-W", str(timeout), host],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True
        )
        return True
    except Exception:
        return False


def detect_interface():
    """Detect the default network interface."""
    try:
        out = subprocess.run(
            "ip route get 1.1.1.1",
            shell=True, capture_output=True, text=True, timeout=5
        ).stdout.strip()
        m = re.search(r"dev\s+(\S+)", out)
        if m:
            return m.group(1)
    except Exception:
        pass
    # fallback
    for iface in ("eth0", "wlan0", "wlp2s0", "enp3s0"):
        if os.path.exists(f"/sys/class/net/{iface}/operstate"):
            return iface
    return "eth0"


def change_ip(iface=None):
    """Change MAC & request DHCP. Returns (success: bool, msg: str)."""
    if os.geteuid() != 0:
        return False, "Root required. Run via pkexec/sudo."

    if iface is None:
        iface = detect_interface()

    mac = random_mac()
    try:
        set_mac(iface, mac)
    except Exception as e:
        return False, f"Failed to set MAC: {e}"

    dhcp = request_dhcp(iface)
    if dhcp:
        return True, f"MAC → {mac} on {iface}, DHCP via {dhcp}"
    else:
        return True, f"MAC → {mac} on {iface} (renew DHCP lease manually)"


def main():
    parser = argparse.ArgumentParser(description="Randomise MAC / reset IP")
    parser.add_argument("--iface", required=True, help="Network interface (e.g., eth0, wlan0)")
    args = parser.parse_args()

    if os.geteuid() != 0:
        sys.exit("This script must be run as root (sudo).")

    mac = random_mac()
    set_mac(args.iface, mac)
    dhcp = request_dhcp(args.iface)
    if dhcp:
        print(f"OK — MAC {mac}, DHCP via {dhcp}")
    else:
        print(f"OK — MAC {mac} (renew DHCP lease manually)")


if __name__ == "__main__":
    main()
