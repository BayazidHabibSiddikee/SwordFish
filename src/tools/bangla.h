#pragma once
// src/tools/bangla.h — Bangla text translator and voice assistant
// Equivalent of the Python tools/bangla.py

#include <QObject>
#include <QString>

/**
 * Translates Bangla text to English using the LibreTranslate HTTP API
 * (or any compatible endpoint available locally / via network).
 * Falls back to a simple echo if translation is unavailable.
 *
 * TTS output uses the TTS namespace (espeak-ng / piper).
 */
class BanglaTranslator : public QObject {
    Q_OBJECT

public:
    explicit BanglaTranslator(QObject *parent = nullptr);

    /**
     * Translate banglaText (bn → en) using LibreTranslate.
     * Returns the translated English string, or empty on failure.
     * @param apiUrl   Base URL of the LibreTranslate instance (default: public API).
     * @param apiKey   Optional API key (empty for key-less instances).
     */
    QString translate(const QString &banglaText,
                      const QString &apiUrl = "https://libretranslate.com",
                      const QString &apiKey = QString());

    /**
     * Generate a simple rule-based reply to an English phrase.
     * (Mirrors the Python reply() method.)
     */
    QString reply(const QString &englishText);

    /**
     * Speak text aloud via the TTS utility.
     */
    void speak(const QString &text);

signals:
    /** Emitted when translation completes with the English result. */
    void translated(const QString &english);

    /** Emitted on translation error. */
    void errorOccurred(const QString &message);
};
