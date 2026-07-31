// src/tools/bangla.cpp — Bangla text translator and voice assistant
// Equivalent of the Python tools/bangla.py
// Uses LibreTranslate HTTP API via curl for translation (no Python deps).

#include "bangla.h"
#include "../../utils/tts.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

// ── Helpers ──────────────────────────────────────────────────────────────

static QString curlPost(const QString &url, const QString &jsonBody, int timeoutSec = 10) {
    QProcess p;
    p.start("curl", QStringList()
            << "-s"
            << "--max-time" << QString::number(timeoutSec)
            << "-X" << "POST"
            << "-H" << "Content-Type: application/json"
            << "-d" << jsonBody
            << url);
    p.waitForFinished((timeoutSec + 2) * 1000);
    if (p.exitCode() != 0) return QString();
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

// ── BanglaTranslator ──────────────────────────────────────────────────────

BanglaTranslator::BanglaTranslator(QObject *parent)
    : QObject(parent)
{}

QString BanglaTranslator::translate(const QString &banglaText,
                                    const QString &apiUrl,
                                    const QString &apiKey) {
    if (banglaText.trimmed().isEmpty()) return QString();

    // Build LibreTranslate JSON payload
    QJsonObject payload;
    payload["q"]      = banglaText;
    payload["source"] = "bn";
    payload["target"] = "en";
    payload["format"] = "text";
    if (!apiKey.isEmpty())
        payload["api_key"] = apiKey;

    QString jsonBody = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QString endpoint = apiUrl.trimmed().trimmed();
    if (!endpoint.endsWith('/')) endpoint += '/';
    endpoint += "translate";

    QString response = curlPost(endpoint, jsonBody);
    if (response.isEmpty()) {
        emit errorOccurred("Translation request failed (no response).");
        return QString();
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred("Bad JSON from translation API: " + err.errorString());
        return QString();
    }

    QJsonObject obj = doc.object();
    if (obj.contains("translatedText")) {
        QString result = obj["translatedText"].toString();
        emit translated(result);
        return result;
    }

    // Some endpoints return { "error": "..." }
    if (obj.contains("error")) {
        emit errorOccurred("Translation API error: " + obj["error"].toString());
    }
    return QString();
}

QString BanglaTranslator::reply(const QString &englishText) {
    QString t = englishText.toLower();

    if (t.contains("hello") || t.contains("hi") || t.contains("hey"))
        return QStringLiteral("Hello! How can I help you?");
    if (t.contains("how are you"))
        return QStringLiteral("I'm doing great! How about you?");
    if (t.contains("your name") || t.contains("who are you"))
        return QStringLiteral("I am a Bangla voice translator.");
    if (t.contains("time"))
        return QString("The current time is %1")
            .arg(QDateTime::currentDateTime().toString("h:mm AP"));
    if (t.contains("date") || t.contains("today"))
        return QString("Today is %1")
            .arg(QDateTime::currentDateTime().toString("MMMM d, yyyy"));
    if (t.contains("thank"))
        return QStringLiteral("You're welcome!");
    if (t.contains("bye") || t.contains("goodbye")
     || t.contains("quit") || t.contains("exit"))
        return QStringLiteral("Goodbye! Have a great day!");

    return QString("I heard: %1. I'm still learning!").arg(englishText);
}

void BanglaTranslator::speak(const QString &text) {
    TTS::speakFemale(text);
}
