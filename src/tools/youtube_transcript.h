#pragma once
// src/tools/youtube_transcript.h — Fetch YouTube video transcripts
// Equivalent of the Python tools/youtube_transcript.py

#include <QString>

namespace YouTubeTranscript {

    /**
     * Extract a YouTube video ID from a URL.
     * Handles both https://youtu.be/<id> and https://youtube.com/watch?v=<id> forms.
     * Returns empty string if no video ID is found.
     */
    QString extractVideoId(const QString &url);

    /**
     * Extract a YouTube URL from free-form text.
     * Returns the URL if found, or empty string.
     */
    QString extractYouTubeUrl(const QString &text);

    /**
     * Fetch the transcript for a YouTube video using yt-dlp.
     * yt-dlp must be installed: https://github.com/yt-dlp/yt-dlp
     *
     * Attempts to retrieve auto-generated or manual subtitles.
     * Prefers English; falls back to the first available language.
     *
     * @param videoUrl  Full YouTube URL.
     * @param maxChars  Truncate result to this many characters (0 = no limit).
     * @return          Plain-text transcript, or empty string on failure.
     */
    QString getTranscript(const QString &videoUrl, int maxChars = 0);

}  // namespace YouTubeTranscript
