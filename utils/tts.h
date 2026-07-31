#pragma once
// utils/tts.h — Text-to-Speech using piper, espeak, or espeak-ng
// Equivalent of the Python utils/tts.py

#include <QString>

namespace TTS {

    /**
     * Speak text aloud using piper (preferred), espeak-ng, or espeak.
     * Silently does nothing if no TTS engine is available.
     * @param text  The text to speak.
     */
    void speakFemale(const QString &text);

    /**
     * Returns true if at least one TTS engine is available.
     */
    bool isAvailable();

}  // namespace TTS
