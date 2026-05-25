import os
import random
import requests

PROXIES_FILE = os.path.join(os.path.dirname(__file__), "proxies.txt")


def load_proxies():
    proxies = []
    if os.path.exists(PROXIES_FILE):
        with open(PROXIES_FILE) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    proxies.append(line)
    return proxies


def save_proxies(proxies):
    with open(PROXIES_FILE, "w") as f:
        f.write("# One proxy per line\n")
        f.write("# Format: scheme://host:port\n")
        f.write("# scheme: socks5, socks4, http, https\n")
        for p in proxies:
            f.write(p + "\n")


def get_random_proxy(proxies=None):
    """Return a random proxy from the list, or ``None`` if the list is empty.

    ``random.choice`` raises ``IndexError`` on an empty list, so we guard
    against that and simply return ``None`` when no proxies are available.
    """
    if proxies is None:
        proxies = load_proxies()
    return random.choice(proxies) if proxies else None


def get_current_ip(proxy_url=None, timeout=8):
    """Fetch public IP. If proxy_url given, route through that proxy."""
    proxies = {}
    if proxy_url:
        scheme = proxy_url.split("://")[0]
        # requests uses lowercase keys
        proxies = {"http": proxy_url, "https": proxy_url}
    try:
        r = requests.get("https://api.ipify.org", proxies=proxies or None, timeout=timeout)
        return r.text.strip()
    except Exception:
        try:
            r = requests.get("https://httpbin.org/ip", proxies=proxies or None, timeout=timeout)
            return r.json().get("origin", "Unknown")
        except Exception:
            return "Unreachable"


def chromeflag_for_proxy(proxy_url):
    """Convert proxy URL to Chromium --proxy-server flag value."""
    return proxy_url


def is_proxy_working(proxy_url, timeout=5):
    """Check if the given proxy is functional."""
    try:
        proxies = {"http": proxy_url, "https": proxy_url}
        r = requests.get("https://api.ipify.org", proxies=proxies, timeout=timeout)
        return r.status_code == 200
    except Exception:
        return False
