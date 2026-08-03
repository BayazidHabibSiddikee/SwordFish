// src/tools/youtube_transcript.cpp — Fetch YouTube video transcripts
// Equivalent of the Python tools/youtube_transcript.py
// Uses yt-dlp to download the subtitle/auto-caption VTT file, then parses it.

#include "youtube_transcript.h"

#include <QProcess>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>

namespace YouTubeTranscript {

// ── Helpers ──────────────────────────────────────────────────────────────

static bool commandExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

/**
 * Strip VTT formatting tags and timestamp lines, returning plain text.
 * VTT format has lines like:
 *   00:00:00.000 --> 00:00:03.000
 *   WEBVTT
 *   <c.colorCCCCCC>text here</c>
 */
static QString parseVtt(const QString &vttContent) {
    QStringList lines = vttContent.split('\n');
    QString result;
    static const QRegularExpression timestampLine(R"(\d{2}:\d{2}:\d{2}\.\d{3}\s*-->)");
    static const QRegularExpression htmlTags(R"(<[^>]+>)");
    static const QRegularExpression position(R"(^(WEBVTT|Kind:|Language:|NOTE|align:|position:))");

    for (const QString &line : lines) {
        QString l = line.trimmed();
        if (l.isEmpty()) continue;
        if (position.match(l).hasMatch()) continue;
        if (timestampLine.match(l).hasMatch()) continue;
        // Strip numeric cue IDs
        bool isNum = false;
        l.toInt(&isNum);
        if (isNum) continue;

        // Remove HTML tags
        l.remove(htmlTags);
        l = l.trimmed();
        if (!l.isEmpty()) {
            // Avoid duplicate consecutive lines (VTT often repeats)
            if (!result.endsWith(l))
                result += l + ' ';
        }
    }
    return result.trimmed();
}

// ── Public API ────────────────────────────────────────────────────────────

QString extractVideoId(const QString &url) {
    // youtu.be/<id>
    QRegularExpression shortForm(R"(youtu\.be/([A-Za-z0-9_\-]{11}))");
    auto m = shortForm.match(url);
    if (m.hasMatch()) return m.captured(1);

    // youtube.com/watch?v=<id>
    QRegularExpression longForm(R"([?&]v=([A-Za-z0-9_\-]{11}))");
    m = longForm.match(url);
    if (m.hasMatch()) return m.captured(1);

    return QString();
}

QString extractYouTubeUrl(const QString &text) {
    QRegularExpression re(
        R"((https?://(?:www\.)?(?:youtube\.com/watch\?v=|youtu\.be/)[\w\-]+))"
    );
    auto m = re.match(text);
    return m.hasMatch() ? m.captured(1) : QString();
}

QString getTranscript(const QString &videoUrl, int maxChars) {
    if (!commandExists("yt-dlp")) {
        fprintf(stderr, "[YouTubeTranscript] yt-dlp not found. Install: pip install yt-dlp\n");
        return QString();
    }

    QString videoId = extractVideoId(videoUrl);
    if (videoId.isEmpty()) {
        fprintf(stderr, "[YouTubeTranscript] Could not extract video ID from: %s\n",
                videoUrl.toUtf8().constData());
        return QString();
    }

    // Use a temporary directory so subtitle files don't clutter the workspace
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) return QString();

    QString outTemplate = tmpDir.path() + "/" + videoId;

    // Download subtitles only (no video)
    // Prefer en manual subs, then auto-generated en, then any language
    QStringList args;
    args << "--skip-download"
         << "--write-subs"
         << "--write-auto-subs"
         << "--sub-lang"   << "en"
         << "--sub-format" << "vtt"
         << "-o" << outTemplate
         << videoUrl;

    QProcess p;
    p.start("yt-dlp", args);
    p.waitForFinished(60000);  // 60 second timeout

    // Find any .vtt file that was written
    QDir dir(tmpDir.path());
    QStringList vttFiles = dir.entryList({"*.vtt"}, QDir::Files);

    if (vttFiles.isEmpty()) {
        fprintf(stderr, "[YouTubeTranscript] No subtitles available for: %s\n",
                videoUrl.toUtf8().constData());
        return QString();
    }

    // Prefer English subtitles; fall back to the first file found
    QString chosenFile;
    for (const auto &f : vttFiles) {
        if (f.contains(".en.") || f.endsWith(".en.vtt")) { chosenFile = f; break; }
    }
    if (chosenFile.isEmpty()) chosenFile = vttFiles.first();

    QFile vttFile(dir.filePath(chosenFile));
    if (!vttFile.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QString vttContent = QTextStream(&vttFile).readAll();
    vttFile.close();

    QString transcript = parseVtt(vttContent);

    if (maxChars > 0 && transcript.size() > maxChars)
        transcript = transcript.left(maxChars);

    fprintf(stderr, "[YouTubeTranscript] Transcript ready: %d chars\n", transcript.size());
    return transcript;
}

}  // namespace YouTubeTranscript
