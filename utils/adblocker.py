import os
import re
import threading
import time
from urllib.parse import urlparse

_AD_DOMAIN_SUFFIXES = {
    # Google / DoubleClick
    "doubleclick.net", "googlesyndication.com", "googleadservices.com",
    "google-analytics.com", "googletagmanager.com", "googletagservices.com",
    "googleadsserving.com", "googleadsserving.cn",
    "pagead2.googlesyndication.com", "pagead1.googlesyndication.com",
    "analytics.google.com", "googleads.g.doubleclick.net",
    "ad.doubleclick.net", "ad-g.doubleclick.net",
    "cm.g.doubleclick.net", "securepubads.g.doubleclick.net",
    "tpc.googlesyndication.com", "adservice.google.com",
    "adservice.google.co.uk", "adservice.google.de",
    "adservice.google.fr", "adservice.google.co.jp",
    "fundingchoicesmessages.google.com",
    # Facebook
    "connect.facebook.net", "facebook.com/tr",
    "www.facebook.com/tr", "pixel.facebook.com",
    "an.facebook.com", "an3.facebook.com",
    "web.facebook.com/tr", "static.xx.fbcdn.net/rsrc",
    "atdmt.com", "facebook.net/story",
    # Amazon
    "c.amazon-adsystem.com", "s.amazon-adsystem.com",
    "aax.amazon-adsystem.com", "aax-us-east.amazon-adsystem.com",
    "amazon-adsystem.com", "amazonadsi.com",
    # Microsoft / Bing
    "bat.bing.com", "c.bing.com",
    "creativecdn.com", "bing-ads.com",
    # Twitter / X
    "static.ads-twitter.com", "ads.twitter.com",
    "t.co/ads", "analytics.twitter.com",
    # LinkedIn
    "ads.linkedin.com", "www.linkedin.com/analytics",
    "linkedin.com/px", "snap.licdn.com",
    # Pinterest
    "ct.pinterest.com", "ads.pinterest.com",
    "analytics.pinterest.com",
    # Reddit
    "www.redditstatic.com/ads", "reddit.com/r/ads",
    # Ad Exchanges / SSPs
    "adnxs.com", "ib.adnxs.com", "secure.adnxs.com",
    "adsrvr.org", "adzerk.net", "adzerk.com",
    "e.adzerk.net", "engine.adzerk.net",
    "appnexus.com", "adserver.adtechus.com",
    "adserver.adtech.de", "advertising.com",
    "pubmatic.com", "ads.pubmatic.com",
    "openx.net", "openx.com",
    "rubiconproject.com", "contextweb.com",
    "ads.contextweb.com", "bid.contextweb.com",
    "sync.contextweb.com",
    "casalemedia.com", "criteo.com", "criteo.net",
    "bidswitch.net", "indexww.com",
    "adform.net", "adform.com",
    "sovrn.com", "lijit.com",
    "sharethrough.com", "native.sharethrough.com",
    "media.net", "adpushup.com",
    # Analytics / Tracking
    "scorecardresearch.com", "b.scorecardresearch.com",
    "quantserve.com", "pixel.quantserve.com",
    "edge.quantserve.com", "secure.quantserve.com",
    "krxd.net", "moatads.com",
    "adsafeprotected.com", "cdn.adsafeprotected.com",
    "servedby.flashtalking.com", "flashtalking.com",
    "serving-sys.com", "servedbyadbutler.com",
    "amplitude.com", "api.amplitude.com",
    "cdn.segment.com", "api.segment.io",
    "cdn.mxpnl.com", "api.mixpanel.com",
    "browser.sentry-cdn.com", "app.getsentry.com",
    "hotjar.com", "static.hotjar.com",
    "inspectlet.com", "luckyorange.com",
    "fullstory.com", "mouseflow.com",
    "crazyegg.com", "clarity.ms",
    "adroll.com", "d.adroll.com",
    "jads.co", "adgebra.co.in",
    # Taboola / Outbrain
    "cdn.taboola.com", "trc.taboola.com",
    "taboola.com", "outbrain.com",
    "widgets.outbrain.com", "odb.outbrain.com",
    # Yahoo / Verizon
    "ads.yahoo.com", "adserver.yahoo.com",
    "analytics.yahoo.com", "pixel.yahoo.com",
    "ads.yimg.com", "ad.yieldmanager.com",
    "yieldmo.com", "nexage.com",
    # TikTok
    "analytics.tiktok.com", "ads.tiktok.com",
    "p16-ads.tiktok.com", "ads-sg.tiktok.com",
    # Snapchat
    "ads.snapchat.com", "tr.snapchat.com",
    "sc-static.net/scevent",
    # Spotify
    "spotify.com/ads", "analytics.spotify.com",
    # Misc Ad CDNs
    "ad-delivery.net", "ad-staging.com",
    "adserver.com", "adserver.co",
    "adserver.org", "adserver.net",
    "adtago.com", "adtech.com",
    "adtech.de", "adtechus.com",
    "adzerk.net", "adzerk.com",
    "popads.net", "popadscdn.net",
    "exoclick.com", "exosrv.com",
    "trafficfactory.com", "clickadu.com",
    "propellerads.com", "pushcrew.com",
    "onesignal.com", "notifyvisitors.com",
}

AD_DOMAINS = _AD_DOMAIN_SUFFIXES | {
    "pornhub.com", "xvideos.com", "xnxx.com", "xhamster.com",
    "redtube.com", "youporn.com", "tube8.com", "spankbang.com",
    "porntube.com", "eporner.com", "pornhd.com", "porn.com",
    "playboy.com", "onlyfans.com", "stripchat.com", "chaturbate.com",
    "cam4.com", "livejasmin.com", "myfreecams.com", "cams.com",
    "adultfriendfinder.com", "hentai.com", "hentaihaven.org",
    "nhentai.net", "e-hentai.org", "exhentai.org",
    "pornhubpremium.com", "pornhub.org", "porndude.com",
    "bongacams.com", "bangbros.com", "brazzers.com",
    "naughtyamerica.com", "realitykings.com", "mofos.com",
    "vixen.com", "blacked.com", "tushy.com",
}

_AD_PATH_PATTERNS = re.compile(
    r"/(ads?|pagead|adserver|adservice|advert|banner|"
    r"impression|tracking|pixel|analytics|beacon|"
    r"collect|event|click|conversion|retargeting|"
    r"affiliate|sponsor|promoted|prebid|"
    r"doubleclick|googleads|facebookimp|"
    r"popunders?|pop-ups?|exitintent|"
    r"ad-delivery|ad-staging|adtago|"
    r"traffic|clickadu|propeller|pushcrew|"
    r"exoclick|exosrv|popads|"
    r"utm_source|utm_medium|utm_campaign|fbclid|gclid)", re.I
)

_ADULT_KEYWORDS = re.compile(
    r"\b(porn|sex|adult|xxx|nude|erotic|hentai|naked|"
    r"vixen|brazzers|bangbros|chaturbate|onlyfans)\b", re.I
)

VIOLENT_KEYWORDS = re.compile(
    r"\b(kill|murder|suicide|bomb|terroris|behead|"
    r"shoot|stab|massacre|genocide|execution|"
    r"torture|abuse|violence|assault|weapon)\b", re.I
)


def _domain_matches(url: str) -> bool:
    try:
        domain = urlparse(url).hostname or ""
        domain = domain.lower()
        while domain:
            if domain in AD_DOMAINS:
                return True
            dot = domain.find(".")
            if dot < 0:
                break
            domain = domain[dot + 1:]
    except Exception:
        pass
    return False


def _path_matches(url: str) -> bool:
    return bool(_AD_PATH_PATTERNS.search(url))


class AdBlocker:
    # Blocking levels: 'none', 'low', 'medium', 'ultimate'
    _DEFAULT_LEVEL = "medium"
    _VALID_LEVELS = {"none", "low", "medium", "ultimate"}

    def __init__(self, level: str = _DEFAULT_LEVEL):
        if level not in self._VALID_LEVELS:
            raise ValueError(f"Invalid blocking level: {level}. Choose from {self._VALID_LEVELS}")
        self._level = level
        self._cache = {}
        self._ready = True
        self._adblock_parser_rules = None
        self._adblock_parser_ready = False
        self._ready_event = threading.Event()
        
        self._cache_dir = os.path.join(os.path.expanduser("~"), ".cache", "adblock")
        os.makedirs(self._cache_dir, exist_ok=True)
        self._rules_path = os.path.join(self._cache_dir, "easylist.txt")

        # Load adblock parser only for medium or ultimate levels
        if self._level in {"medium", "ultimate"}:
            self._parser_thread = threading.Thread(target=self._init_adblockparser, daemon=True)
            self._parser_thread.start()
        else:
            self._ready_event.set()

    def set_level(self, level: str):
        """Change blocking level at runtime."""
        if level not in self._VALID_LEVELS:
            raise ValueError(f"Invalid blocking level: {level}. Choose from {self._VALID_LEVELS}")
        if self._level != level:
            self._level = level
            # Reset cache when level changes
            self._cache.clear()
            # Ensure parser is running if needed
            if self._level in {"medium", "ultimate"} and not self._adblock_parser_ready:
                self._ready_event.clear()
                self._parser_thread = threading.Thread(target=self._init_adblockparser, daemon=True)
                self._parser_thread.start()
            elif self._level not in {"medium", "ultimate"}:
                self._ready_event.set()

    def _init_adblockparser(self):
        try:
            from adblockparser import AdblockRules
            t0 = time.time()
            
            content = ""
            # Try to load from cache first
            if os.path.exists(self._rules_path):
                # Check if older than 7 days
                if time.time() - os.path.getmtime(self._rules_path) < 7 * 24 * 3600:
                    with open(self._rules_path, "r", encoding="utf-8") as f:
                        content = f.read()
            
            if not content:
                import requests
                r = requests.get(
                    "https://easylist.to/easylist/easylist.txt",
                    timeout=10,
                    headers={"User-Agent": "SwordFish/1.0"},
                )
                if r.status_code == 200:
                    content = r.text
                    with open(self._rules_path, "w", encoding="utf-8") as f:
                        f.write(content)
            
            if content:
                lines = [l.strip() for l in content.splitlines()
                         if l.strip() and not l.startswith("!") and not l.startswith("[")]
                self._adblock_parser_rules = AdblockRules(lines)
                t1 = time.time()
                print(f"[Adblock] Parser loaded {len(lines)} rules in {t1-t0:.1f}s")
                self._adblock_parser_ready = True
                self._ready_event.set()
        except Exception as e:
            print(f"[Adblock] Parser init failed: {e}")
            self._ready_event.set()


    def wait_ready(self, timeout=1):
        self._ready_event.wait(timeout)

    def should_block(self, url: str, source_url: str = "") -> bool:
        if self._level == "none":
            return False
        cached = self._cache.get(url)
        if cached is not None:
            return cached

        # Fast checks always applied (domain, path)
        # Exclude JavaScript files to avoid breaking page content
        parsed = urlparse(url)
        if parsed.path.lower().endswith('.js'):
            self._cache[url] = False
            return False
        if _domain_matches(url) or _domain_matches(source_url):
            self._cache[url] = True
            return True
        if _path_matches(url):
            self._cache[url] = True
            return True

        # Level-specific behavior
        if self._level == "low":
            # Low level: skip adblock parser, only fast checks
            self._cache[url] = False
        elif self._level == "medium":
            # Medium level: use parser if ready
            if not self._adblock_parser_ready:
                self.wait_ready(timeout=0.2)
            if self._adblock_parser_ready and self._adblock_parser_rules:
                try:
                    if self._adblock_parser_rules.should_block(url):
                        self._cache[url] = True
                        return True
                except Exception:
                    pass
            self._cache[url] = False
        else:  # ultimate
            if not self._adblock_parser_ready:
                self.wait_ready(timeout=0.2)
            if self._adblock_parser_ready and self._adblock_parser_rules:
                try:
                    if self._adblock_parser_rules.should_block(url):
                        self._cache[url] = True
                        return True
                except Exception:
                    pass
            lower = url.lower()
            if any(kw in lower for kw in ["/ad.", "/ad-", "/ad_", "adserver",
                                          "adservice", "advert", "banner",
                                          "tracking", "pixel", "beacon",
                                          "popup", "popunder"]):
                self._cache[url] = True
                return True
            self._cache[url] = False

        if len(self._cache) > 10000:
            self._cache.clear()
        return False

    def should_block_request(self, url: str, source_url: str = "") -> bool:
        return self.should_block(url, source_url)

    def check_content_violent(self, text: str, title: str = "") -> bool:
        if VIOLENT_KEYWORDS.search(title or ""):
            return True
        if VIOLENT_KEYWORDS.search(text or ""):
            return True
        return False

    def check_content_adult(self, text: str, title: str = "") -> bool:
        if _ADULT_KEYWORDS.search(title or ""):
            return True
        if _ADULT_KEYWORDS.search(text or ""):
            return True
        return False


_blocker = None


def get_blocker(level="low"):
    global _blocker
    if _blocker is None:
        _blocker = AdBlocker(level)
    elif _blocker._level != level:
        _blocker.set_level(level)
    return _blocker
