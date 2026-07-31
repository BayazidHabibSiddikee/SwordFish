#pragma once
// src/tools/timer.h — Countdown timer with alarm playback
// Equivalent of the Python tools/timer.py

#include <QObject>
#include <QString>

/**
 * Countdown timer that plays an alarm sound when it expires.
 * Emits signals so it can be driven from the GUI or run headless.
 *
 * Usage (headless):
 *   Timer t;
 *   t.start(300);   // 5 minutes
 *   // wait for finished() signal
 *
 * Usage (GUI):
 *   connect(&t, &Timer::tick,     label, [&](int rem){ label->setText(...); });
 *   connect(&t, &Timer::finished, dialog, &QDialog::accept);
 */
class Timer : public QObject {
    Q_OBJECT

public:
    explicit Timer(QObject *parent = nullptr);
    ~Timer() override;

    /** Start a countdown. Cancels any running timer first. */
    void start(int durationSeconds);

    /** Cancel the running timer without playing the alarm. */
    void cancel();

    /** True if timer is currently counting down. */
    bool isRunning() const;

    /** Seconds remaining (0 when done). */
    int remaining() const;

    /** Format an integer number of seconds as "X hour(s) Y minute(s) Z second(s)". */
    static QString formatDuration(int seconds);

signals:
    /** Emitted every second with remaining seconds. */
    void tick(int secondsRemaining);

    /** Emitted when the countdown reaches zero (alarm already played). */
    void finished();

    /** Emitted if the timer is cancelled before completion. */
    void cancelled();

private:
    void playAlarm();

    class Private;
    Private *d;
};
