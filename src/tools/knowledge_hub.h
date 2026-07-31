#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

namespace KnowledgeHub {
    QJsonObject getWeatherData(const QString &city = "Dhaka");
    QJsonArray getFloodData();
    QJsonObject getRouteData(const QString &startCity, const QString &endCity);
    QJsonArray searchPlacesInCity(const QString &city, const QString &query = "cafe", int limit = 8);
    QString scrapeContent(const QString &url);
    QJsonArray searchWeb(const QString &query, int maxResults = 5);
}
