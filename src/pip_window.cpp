// src/pip_window.cpp — Floating Picture-in-Picture window
#include "pip_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScreen>
#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWebEngineView>
#include <QWebEnginePage>

PipWindow::PipWindow(QWebEngineView *sourceView, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint |
                      Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(320, 200);
    resize(400, 240);

    // Position bottom-right
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    move(screen.right() - 420, screen.bottom() - 260);

    setStyleSheet(
        "QWidget { background: #0d0d0d; border: 2px solid #00b4d8; border-radius: 8px; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    // ── Title bar ──
    auto *titleBar = new QWidget(this);
    titleBar->setFixedHeight(28);
    titleBar->setStyleSheet("background: #0d1117; border-bottom: 1px solid #00b4d8; border-radius: 0;");
    auto *tbLayout = new QHBoxLayout(titleBar);
    tbLayout->setContentsMargins(8, 0, 4, 0);

    auto *lbl = new QLabel("🎬 Picture-in-Picture", titleBar);
    lbl->setStyleSheet("color: #00d2ff; font-size: 11px; font-weight: bold; background: transparent; border: none;");

    m_sizeBtn = new QPushButton("⤢", titleBar);
    m_closeBtn = new QPushButton("✕", titleBar);
    for (auto *b : {m_sizeBtn, m_closeBtn}) {
        b->setFixedSize(22, 22);
        b->setStyleSheet(
            "QPushButton { background: transparent; color: #00d2ff; border: none; font-size: 12px; }"
            "QPushButton:hover { background: rgba(0,180,216,0.25); border-radius: 4px; }");
    }

    tbLayout->addWidget(lbl);
    tbLayout->addStretch();
    tbLayout->addWidget(m_sizeBtn);
    tbLayout->addWidget(m_closeBtn);

    layout->addWidget(titleBar);

    // ── Web view — inject PiP via JS ──
    m_view = new QWebEngineView(this);
    m_view->setPage(sourceView->page());  // share the page
    layout->addWidget(m_view);

    // Tell the page to enter native PiP on the first video
    sourceView->page()->runJavaScript(R"JS(
        (function() {
            const v = document.querySelector('video');
            if (v) {
                v.requestPictureInPicture().catch(() => {});
            }
        })();
    )JS");

    connect(m_closeBtn, &QPushButton::clicked, this, &PipWindow::close_);
    connect(m_sizeBtn,  &QPushButton::clicked, this, &PipWindow::toggleSize);
}

PipWindow::~PipWindow() = default;

void PipWindow::close_() {
    // Exit native PiP if active
    if (m_view && m_view->page()) {
        m_view->page()->runJavaScript(
            "if (document.pictureInPictureElement) document.exitPictureInPicture();");
    }
    close();
}

void PipWindow::toggleSize() {
    m_large = !m_large;
    if (m_large) resize(640, 390);
    else         resize(400, 240);
}

// ── Drag to move ────────────────────────────────────────────────────────────
void PipWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && e->pos().y() < 30) {
        m_dragging  = true;
        m_dragStart = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(e);
}

void PipWindow::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging)
        move(e->globalPosition().toPoint() - m_dragStart);
    QWidget::mouseMoveEvent(e);
}

void PipWindow::mouseReleaseEvent(QMouseEvent *e) {
    m_dragging = false;
    QWidget::mouseReleaseEvent(e);
}

void PipWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
}
