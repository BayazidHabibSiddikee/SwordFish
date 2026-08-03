#pragma once

#include <QWebEnginePage>
#include <QWebEngineProfile>

class CustomWebPage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit CustomWebPage(QWebEngineProfile *profile, QWidget *parent = nullptr);

protected:
    QStringList chooseFiles(FileSelectionMode mode,
                     const QStringList &oldFiles,
                     const QStringList &acceptedMimeTypes) override;
};
