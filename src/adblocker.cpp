#include "adblocker.h"
#include <QUrl>
#include <QRegularExpression>
#include <algorithm>
#include <cctype>

std::unordered_set<std::string> AdBlocker::s_adDomains = {
    "doubleclick.net", "googlesyndication.com", "googleadservices.com",
    "google-analytics.com", "googletagmanager.com", "googletagservices.com",
    "googleadsserving.com", "adservice.google.com",
    "connect.facebook.net", "facebook.com/tr",
    "www.facebook.com/tr", "pixel.facebook.com",
    "c.amazon-adsystem.com", "s.amazon-adsystem.com",
    "amazon-adsystem.com", "amazonadsi.com",
    "bat.bing.com", "c.bing.com",
    "static.ads-twitter.com", "ads.twitter.com",
    "ads.linkedin.com", "snap.licdn.com",
    "ct.pinterest.com", "ads.pinterest.com",
    "adnxs.com", "ib.adnxs.com",
    "adsrvr.org", "adzerk.net", "adzerk.com",
    "pubmatic.com", "ads.pubmatic.com",
    "openx.net", "openx.com",
    "rubiconproject.com", "casalemedia.com",
    "criteo.com", "criteo.net", "bidswitch.net",
    "scorecardresearch.com", "quantserve.com",
    "amplitude.com", "hotjar.com",
    "cdn.taboola.com", "trc.taboola.com",
    "ads.yahoo.com", "adserver.yahoo.com",
    "analytics.tiktok.com", "ads.tiktok.com",
    "ads.snapchat.com", "tr.snapchat.com",
    "popads.net", "exoclick.com",
    "propellerads.com", "clickadu.com",
    "pornhub.com", "xvideos.com", "xnxx.com", "xhamster.com",
    "redtube.com", "youporn.com", "spankbang.com",
    "porntube.com", "eporner.com", "pornhd.com",
    "bongacams.com", "brazzers.com", "chaturbate.com",
    "onlyfans.com", "stripchat.com",
};

std::regex AdBlocker::s_adPathPattern(
    R"(/(ads?|pagead|adserver|adservice|advert|banner|impression|tracking|pixel|analytics|beacon|collect|event|click|conversion|retargeting|affiliate|sponsor|promoted|prebid|doubleclick|googleads|popunders?|pop-ups?|exitintent|ad-delivery|ad-staging|adtago|traffic|clickadu|propeller|pushcrew|exoclick|exosrv|popads|utm_source|utm_medium|utm_campaign|fbclid|gclid))",
    std::regex::icase
);

std::regex AdBlocker::s_adultKeywords(
    R"(\b(porn|sex|adult|xxx|nude|erotic|hentai|naked|vixen|brazzers|bangbros|chaturbate|onlyfans)\b)",
    std::regex::icase
);

std::regex AdBlocker::s_violentKeywords(
    R"(\b(kill|murder|suicide|bomb|terroris|behead|shoot|stab|massacre|genocide|execution|torture|abuse|violence|assault|weapon)\b)",
    std::regex::icase
);

static std::string toLowerStr(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

AdBlocker::AdBlocker(Level level) : m_level(level) {}

void AdBlocker::setLevel(Level level) {
    m_level = level;
    m_cache.clear();
}

bool AdBlocker::domainMatches(const QString &url) const {
    QUrl qurl(url);
    std::string domain = qurl.host().toLower().toStdString();

    while (!domain.empty()) {
        if (s_adDomains.count(domain)) return true;
        auto dot = domain.find('.');
        if (dot == std::string::npos) break;
        domain = domain.substr(dot + 1);
    }
    return false;
}

bool AdBlocker::pathMatches(const QString &url) const {
    std::string urlStr = url.toStdString();
    return std::regex_search(urlStr, s_adPathPattern);
}

bool AdBlocker::shouldBlock(const QString &url, const QString &sourceUrl) const {
    if (m_level == Level::None) return false;

    std::string key = url.toStdString();
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second;

    QUrl qurl(url);
    QString path = qurl.path().toLower();
    if (path.endsWith(".js")) {
        m_cache[key] = false;
        return false;
    }

    if (domainMatches(url) || domainMatches(sourceUrl)) {
        m_cache[key] = true;
        return true;
    }
    if (pathMatches(url)) {
        m_cache[key] = true;
        return true;
    }

    if (m_level == Level::Low) {
        m_cache[key] = false;
        return false;
    }

    if (m_level == Level::Ultimate) {
        std::string lower = url.toLower().toStdString();
        std::vector<std::string> keywords = {
            "/ad.", "/ad-", "/ad_", "adserver", "adservice",
            "advert", "banner", "tracking", "pixel", "beacon",
            "popup", "popunder"
        };
        for (const auto &kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                m_cache[key] = true;
                return true;
            }
        }
    }

    if (m_cache.size() > 10000) m_cache.clear();
    m_cache[key] = false;
    return false;
}

bool AdBlocker::checkContentViolent(const QString &text, const QString &title) const {
    std::string t = title.toStdString();
    std::string txt = text.toStdString();
    return std::regex_search(t, s_violentKeywords) || std::regex_search(txt, s_violentKeywords);
}

bool AdBlocker::checkContentAdult(const QString &text, const QString &title) const {
    std::string t = title.toStdString();
    std::string txt = text.toStdString();
    return std::regex_search(t, s_adultKeywords) || std::regex_search(txt, s_adultKeywords);
}

static AdBlocker s_blocker;
AdBlocker &getBlocker() { return s_blocker; }
