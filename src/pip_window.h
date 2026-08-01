#pragma once
// src/pip_window.h — Floating Picture-in-Picture window

#include <QWidget>
#include <QWebEngineView>

class QPushButton;
class QLabel;

/**
 * PipWindow
 * A small always-on-top floating window that shows a QWebEngineView
 * playing in picture-in-picture mode.
 *
 * Usage:
 *   PipWindow *pip = new PipWindow(sourceView);
 *   pip->show();
 */
class PipWindow : public QWidget {
    Q_OBJECT
public:
    explicit PipWindow(QWebEngineView *sourceView, QWidget *parent = nullptr);
    ~PipWindow() override;

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void close_();
    void toggleSize();

private:
    QWebEngineView *m_view   = nullptr;
    QPushButton    *m_closeBtn = nullptr;
    QPushButton    *m_sizeBtn  = nullptr;
    QPoint          m_dragStart;
    bool            m_dragging = false;
    bool            m_large    = false;
};
