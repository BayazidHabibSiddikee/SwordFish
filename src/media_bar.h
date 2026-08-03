#pragma once
// src/media_bar.h — Floating media controls bar (play/pause/seek/vol/PiP)

#include <QWidget>

class QWebEngineView;
class QPushButton;
class QSlider;
class QLabel;
class QTimer;

class MediaBar : public QWidget {
    Q_OBJECT
public:
    explicit MediaBar(QWidget *parent = nullptr);

    /** Attach to a web view — polls media state every second. */
    void attachTo(QWebEngineView *view);
    void detach();

    /** Show/hide the bar. */
    void toggleVisible();

signals:
    void pipRequested();

private slots:
    void pollMediaState();
    void onPlayPause();
    void onStop();
    void onPrev();
    void onNext();
    void onVolume(int val);
    void onSeek(int val);
    void onMute();
    void onPip();

private:
    void runJS(const QString &js);
    void updateUi(bool playing, double currentTime, double duration,
                  double volume, bool muted);

    QWebEngineView *m_view   = nullptr;
    QTimer         *m_timer  = nullptr;

    QPushButton *m_prevBtn   = nullptr;
    QPushButton *m_playBtn   = nullptr;
    QPushButton *m_stopBtn   = nullptr;
    QPushButton *m_nextBtn   = nullptr;
    QPushButton *m_muteBtn   = nullptr;
    QPushButton *m_pipBtn    = nullptr;
    QSlider     *m_seekSlider= nullptr;
    QSlider     *m_volSlider = nullptr;
    QLabel      *m_timeLabel = nullptr;
    QLabel      *m_titleLabel= nullptr;
    bool         m_seeking   = false;
};
