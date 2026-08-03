#include "knowledge_hub.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QObject>

namespace KnowledgeHub {

static QByteArray fetchUrl(const QString &url, int timeoutMs = 15000) {
    QNetworkAccessManager mgr;
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SwordFish/2.0");
    QNetworkReply *reply = mgr.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        return data;
    }
    reply->deleteLater();
    return {};
}

QJsonObject getWeatherData(const QString &city) {
    // Use python geopy + open-meteo
    QProcess proc;
    proc.start("python3", QStringList() << "-c"
        << QString("from geopy.geocoders import Nominatim; "
                   "import requests, json; "
                   "g = Nominatim(user_agent='swordfish'); "
                   "loc = g.geocode('%1'); "
                   "if not loc: print(json.dumps({'error': 'Could not geocode %1'})); exit(); "
                   "r = requests.get(f'https://api.open-meteo.com/v1/forecast?latitude={loc.latitude}&longitude={loc.longitude}&current_weather=true', timeout=10); "
                   "d = r.json(); "
                   "cw = d.get('current_weather', {}); "
                   "print(json.dumps({'city': '%1', 'temperature': cw.get('temperature'), "
                   "'windspeed': cw.get('windspeed'), 'time': cw.get('time', '')}))")
        .arg(city));
    proc.waitForFinished(15000);

    QByteArray data = proc.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.object();
}

QJsonArray getFloodData() {
    QByteArray data = fetchUrl("https://eonet.gsfc.nasa.gov/api/v3/events?categories=floods&status=open");
    if (data.isEmpty()) return {{"error", "Failed to fetch"}};

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray events = doc["events"].toArray();
    QJsonArray results;

    for (const auto &e : events) {
        QJsonObject ev = e.toObject();
        QJsonArray geom = ev["geometry"].toArray();
        QJsonObject first = geom.isEmpty() ? QJsonObject() : geom[0].toObject();
        QJsonObject entry;
        entry["title"] = ev["title"];
        entry["date"] = first["date"];
        entry["coordinates"] = first["coordinates"];
        results.append(entry);
    }
    return results;
}

QJsonObject getRouteData(const QString &startCity, const QString &endCity) {
    QProcess geoProc;
    geoProc.start("python3", QStringList() << "-c"
        << QString("from geopy.geocoders import Nominatim; import json; "
                   "g = Nominatim(user_agent='swordfish'); "
                   "s = g.geocode('%1'); e = g.geocode('%2'); "
                   "if s and e: print(json.dumps({'start': [s.latitude, s.longitude], 'end': [e.latitude, e.longitude]})) "
                   "else: print('{}')")
        .arg(startCity).arg(endCity));
    geoProc.waitForFinished(10000);

    QJsonDocument geoDoc = QJsonDocument::fromJson(geoProc.readAllStandardOutput());
    QJsonObject geo = geoDoc.object();
    if (geo.isEmpty()) return {{"error", "Geocoding failed"}};

    QJsonArray startCoords = geo["start"].toArray();
    QJsonArray endCoords = geo["end"].toArray();
    double slat = startCoords[0].toDouble(), slon = startCoords[1].toDouble();
    double elat = endCoords[0].toDouble(), elon = endCoords[1].toDouble();

    QString url = QString("http://router.project-osrm.org/route/v1/driving/"
                          "%1,%2;%3,%4?overview=full&geometries=geojson")
                      .arg(slon).arg(slat).arg(elon).arg(elat);
    QByteArray data = fetchUrl(url, 20000);
    if (data.isEmpty()) return {{"error", "Routing failed"}};

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray routes = doc["routes"].toArray();
    if (routes.isEmpty()) return {{"error", "No routes found"}};

    QJsonObject route = routes[0].toObject();
    QJsonObject result;
    result["distance_km"] = route["distance"].toDouble() / 1000.0;
    result["duration_mins"] = route["duration"].toDouble() / 60.0;
    result["geometry"] = route["geometry"];
    return result;
}

QJsonArray searchPlacesInCity(const QString &city, const QString &query, int limit) {
    Q_UNUSED(city); Q_UNUSED(query); Q_UNUSED(limit);
    // Placeholder - requires Overpass API integration
    return {};
}

QString scrapeContent(const QString &url) {
    Q_UNUSED(url);
    return "Scraping requires Python runtime.";
}

QJsonArray searchWeb(const QString &query, int maxResults) {
    Q_UNUSED(query); Q_UNUSED(maxResults);
    // Placeholder - requires DuckDuckGo API
    return {};
}

} // namespace KnowledgeHub
