// utils/tts.cpp — Text-to-Speech using system TTS engines
// Equivalent of the Python utils/tts.py (which used pyttsx3)
// Uses piper (preferred for quality), then espeak-ng, then espeak as fallbacks.

#include "tts.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace TTS {

// ── Helpers ──────────────────────────────────────────────────────────────

static bool commandExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

/**
 * Try to find a piper model (.onnx) file.
 * Looks in common install locations.
 */
static QString findPiperModel() {
    QStringList candidates = {
        QDir::homePath() + "/.piper-voices/en_US-amy-medium.onnx",
        QDir::homePath() + "/.piper-voices/en_US-lessac-medium.onnx",
        QDir::homePath() + "/.piper-voices/en_US-ryan-medium.onnx",
        "/usr/share/piper-voices/en_US-amy-medium.onnx",
        "/usr/share/piper-voices/en_US-lessac-medium.onnx",
    };

    // Also scan ~/.piper-voices for any .onnx file
    QDir voiceDir(QDir::homePath() + "/.piper-voices");
    if (voiceDir.exists()) {
        const auto entries = voiceDir.entryList({"*.onnx"}, QDir::Files);
        for (const auto &e : entries)
            candidates.prepend(voiceDir.filePath(e));
    }

    for (const auto &path : candidates) {
        if (QFile::exists(path)) return path;
    }
    return QString();
}

// ── Public API ────────────────────────────────────────────────────────────

bool isAvailable() {
    return commandExists("piper") || commandExists("espeak-ng") || commandExists("espeak");
}

void speakFemale(const QString &text) {
    if (text.trimmed().isEmpty()) return;

    // 1. Try piper (highest quality)
    if (commandExists("piper")) {
        QString model = findPiperModel();
        if (!model.isEmpty()) {
            // piper reads from stdin: echo "text" | piper --model <m> --output_raw | aplay
            QProcess echo;
            echo.start("sh", QStringList() << "-c" <<
                QString("echo %1 | piper --model %2 --output_raw | aplay -r 22050 -f S16_LE -c 1 -q 2>/dev/null")
                    .arg(QString(text).replace("'", "'\\''").prepend("'").append("'"),
                         QString(model).replace("'", "'\\''").prepend("'").append("'")));
            echo.waitForFinished(30000);
            return;
        }
    }

    // 2. Try espeak-ng (good quality, female voice)
    if (commandExists("espeak-ng")) {
        QProcess p;
        p.start("espeak-ng", QStringList()
                << "-v" << "en-us+f3"   // female voice variant
                << "-s" << "160"        // speech rate (~160 wpm)
                << text);
        p.waitForFinished(30000);
        return;
    }

    // 3. Fallback to plain espeak
    if (commandExists("espeak")) {
        QProcess p;
        p.start("espeak", QStringList()
                << "-v" << "en+f3"
                << "-s" << "160"
                << text);
        p.waitForFinished(30000);
        return;
    }

    // No TTS engine available — print to stderr
    fprintf(stderr, "[TTS] (no engine) %s\n", text.toUtf8().constData());
}

}  // namespace TTS
