// src/tools/timer.cpp — Countdown timer with alarm playback
// Equivalent of the Python tools/timer.py

#include "timer.h"

#include <QTimer>
#include <QProcess>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

// ── Private implementation ────────────────────────────────────────────────

class Timer::Private {
public:
    QTimer *ticker   = nullptr;
    int remaining    = 0;
    bool running     = false;
};

// ── Helpers ──────────────────────────────────────────────────────────────

static bool commandExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

static QString alarmFilePath() {
    // Look next to the binary first, then relative to source layout
    QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/tools/alarm.wav",
        QCoreApplication::applicationDirPath() + "/../tools/alarm.wav",
        QDir::homePath() + "/SwordFish/tools/alarm.wav",
    };
    for (const auto &c : candidates)
        if (QFile::exists(c)) return c;
    return QString();
}

// ── Timer implementation ──────────────────────────────────────────────────

Timer::Timer(QObject *parent)
    : QObject(parent), d(new Private)
{
    d->ticker = new QTimer(this);
    d->ticker->setInterval(1000);
    connect(d->ticker, &QTimer::timeout, this, [this]() {
        if (d->remaining > 0) {
            --d->remaining;
            emit tick(d->remaining);
        }
        if (d->remaining <= 0) {
            d->ticker->stop();
            d->running = false;
            playAlarm();
            emit finished();
        }
    });
}

Timer::~Timer() {
    delete d;
}

void Timer::start(int durationSeconds) {
    if (d->running) cancel();
    if (durationSeconds <= 0) return;

    d->remaining = durationSeconds;
    d->running   = true;
    d->ticker->start();
    emit tick(d->remaining);
}

void Timer::cancel() {
    d->ticker->stop();
    d->running = false;
    emit cancelled();
}

bool Timer::isRunning() const { return d->running; }
int  Timer::remaining()  const { return d->remaining; }

QString Timer::formatDuration(int seconds) {
    if (seconds <= 0) return QStringLiteral("0 seconds");
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    QStringList parts;
    if (h) parts << QString("%1 hour%2").arg(h).arg(h != 1 ? "s" : "");
    if (m) parts << QString("%1 minute%2").arg(m).arg(m != 1 ? "s" : "");
    if (s) parts << QString("%1 second%2").arg(s).arg(s != 1 ? "s" : "");
    return parts.join(' ');
}

void Timer::playAlarm() {
    QString wav = alarmFilePath();
    if (wav.isEmpty()) {
        fprintf(stderr, "[Timer] alarm.wav not found\n");
        return;
    }

    // Try ffplay (ffmpeg suite), then aplay (ALSA), then paplay (PulseAudio)
    if (commandExists("ffplay")) {
        QProcess::startDetached("ffplay", QStringList()
            << "-nodisp" << "-autoexit" << wav);
    } else if (commandExists("aplay")) {
        QProcess::startDetached("aplay", QStringList() << wav);
    } else if (commandExists("paplay")) {
        QProcess::startDetached("paplay", QStringList() << wav);
    } else {
        fprintf(stderr, "[Timer] No audio player found (install ffplay or aplay)\n");
    }
}
