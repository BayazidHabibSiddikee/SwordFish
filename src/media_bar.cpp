// src/media_bar.cpp — Floating media controls bar
#include "media_bar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTimer>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonObject>

MediaBar::MediaBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(48);
    setStyleSheet(
        "QWidget { background: rgba(10,20,40,0.92); border-top: 1px solid #00b4d8; }"
        "QPushButton { background: transparent; color: #00d2ff; border: none; font-size: 16px; padding: 4px 8px; border-radius: 4px; }"
        "QPushButton:hover { background: rgba(0,180,220,0.2); }"
        "QSlider::groove:horizontal { background: #1e2a3a; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #00d2ff; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #00b4d8; border-radius: 2px; }"
        "QLabel { color: #00d2ff; font-size: 11px; background: transparent; }"
    );

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    m_titleLabel = new QLabel("No media", this);
    m_titleLabel->setMaximumWidth(160);
    m_titleLabel->setStyleSheet("color: #7ee8fa; font-size: 11px;");

    m_prevBtn  = new QPushButton("⏮", this);
    m_playBtn  = new QPushButton("▶", this);
    m_stopBtn  = new QPushButton("⏹", this);
    m_nextBtn  = new QPushButton("⏭", this);
    m_muteBtn  = new QPushButton("🔊", this);
    m_pipBtn   = new QPushButton("⧉ PiP", this);

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setFixedWidth(200);
    m_seekSlider->setToolTip("Seek");

    m_volSlider = new QSlider(Qt::Horizontal, this);
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(100);
    m_volSlider->setFixedWidth(80);
    m_volSlider->setToolTip("Volume");

    m_timeLabel = new QLabel("0:00 / 0:00", this);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_prevBtn);
    layout->addWidget(m_playBtn);
    layout->addWidget(m_stopBtn);
    layout->addWidget(m_nextBtn);
    layout->addWidget(m_seekSlider);
    layout->addWidget(m_timeLabel);
    layout->addStretch();
    layout->addWidget(m_muteBtn);
    layout->addWidget(m_volSlider);
    layout->addWidget(m_pipBtn);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MediaBar::pollMediaState);

    connect(m_playBtn,  &QPushButton::clicked, this, &MediaBar::onPlayPause);
    connect(m_stopBtn,  &QPushButton::clicked, this, &MediaBar::onStop);
    connect(m_prevBtn,  &QPushButton::clicked, this, &MediaBar::onPrev);
    connect(m_nextBtn,  &QPushButton::clicked, this, &MediaBar::onNext);
    connect(m_muteBtn,  &QPushButton::clicked, this, &MediaBar::onMute);
    connect(m_pipBtn,   &QPushButton::clicked, this, &MediaBar::onPip);
    connect(m_volSlider,&QSlider::valueChanged, this, &MediaBar::onVolume);
    connect(m_seekSlider,&QSlider::sliderPressed,  this, [this]{ m_seeking=true; });
    connect(m_seekSlider,&QSlider::sliderReleased, this, [this]{
        m_seeking = false;
        onSeek(m_seekSlider->value());
    });
}

void MediaBar::attachTo(QWebEngineView *view) {
    m_view = view;
    m_timer->start();
}

void MediaBar::detach() {
    m_view = nullptr;
    m_timer->stop();
    m_titleLabel->setText("No media");
    m_timeLabel->setText("0:00 / 0:00");
    m_playBtn->setText("▶");
}

void MediaBar::toggleVisible() {
    setVisible(!isVisible());
}

void MediaBar::runJS(const QString &js) {
    if (m_view && m_view->page())
        m_view->page()->runJavaScript(js);
}

// Format seconds → "M:SS"
static QString fmtTime(double s) {
    int t = (int)s;
    return QString("%1:%2").arg(t/60).arg(t%60, 2, 10, QChar('0'));
}

void MediaBar::pollMediaState() {
    if (!m_view || !m_view->page()) return;
    m_view->page()->runJavaScript(R"JS(
(function() {
    const v = document.querySelector('video');
    if (!v) return JSON.stringify({found:false});
    return JSON.stringify({
        found:    true,
        playing:  !v.paused && !v.ended,
        current:  v.currentTime,
        duration: v.duration || 0,
        volume:   v.volume,
        muted:    v.muted,
        title:    document.title || ''
    });
})()
)JS", [this](const QVariant &result) {
        QString json = result.toString();
        if (json.isEmpty()) return;
        QJsonObject o = QJsonDocument::fromJson(json.toUtf8()).object();
        if (!o["found"].toBool()) {
            m_titleLabel->setText("No media");
            return;
        }
        updateUi(o["playing"].toBool(), o["current"].toDouble(),
                 o["duration"].toDouble(), o["volume"].toDouble(),
                 o["muted"].toBool());
        // Update title
        QString title = o["title"].toString();
        if (title.length() > 24) title = title.left(21) + "…";
        m_titleLabel->setText(title);
    });
}

void MediaBar::updateUi(bool playing, double current, double duration,
                        double volume, bool muted) {
    m_playBtn->setText(playing ? "⏸" : "▶");
    m_muteBtn->setText(muted ? "🔇" : "🔊");
    m_timeLabel->setText(fmtTime(current) + " / " + fmtTime(duration));
    if (!m_seeking && duration > 0)
        m_seekSlider->setValue((int)(current / duration * 1000));
    m_volSlider->blockSignals(true);
    m_volSlider->setValue((int)(volume * 100));
    m_volSlider->blockSignals(false);
}

void MediaBar::onPlayPause() {
    runJS("(function(){const v=document.querySelector('video');if(v){if(v.paused)v.play();else v.pause();}})();");
}
void MediaBar::onStop() {
    runJS("(function(){const v=document.querySelector('video');if(v){v.pause();v.currentTime=0;}})();");
}
void MediaBar::onPrev() {
    runJS("(function(){const v=document.querySelector('video');if(v)v.currentTime=Math.max(0,v.currentTime-10);})();");
}
void MediaBar::onNext() {
    runJS("(function(){const v=document.querySelector('video');if(v)v.currentTime=Math.min(v.duration,v.currentTime+10);})();");
}
void MediaBar::onMute() {
    runJS("(function(){const v=document.querySelector('video');if(v)v.muted=!v.muted;})();");
}
void MediaBar::onVolume(int val) {
    double v = val / 100.0;
    runJS(QString("(function(){const v=document.querySelector('video');if(v)v.volume=%1;})();").arg(v));
}
void MediaBar::onSeek(int val) {
    runJS(QString("(function(){const v=document.querySelector('video');if(v&&v.duration)v.currentTime=v.duration*%1/1000;})();").arg(val));
}
void MediaBar::onPip() {
    emit pipRequested();
}
