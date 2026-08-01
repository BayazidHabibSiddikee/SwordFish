#pragma once
// src/reading_mode.h — Inject Readability-style clean article view

#include <QObject>
#include <QString>

class QWebEnginePage;
class QWidget;

class ReadingMode : public QObject {
    Q_OBJECT
public:
    explicit ReadingMode(QObject *parent = nullptr);

    /** Toggle reading mode on the given page. */
    void toggle(QWebEnginePage *page);

    bool isActive() const { return m_active; }

signals:
    void activated();
    void deactivated();

private:
    void inject(QWebEnginePage *page);
    void restore(QWebEnginePage *page);

    bool    m_active = false;
    QString m_savedHtml;
};
