#include "web_page.h"
#include "file_picker.h"
#include <QStandardPaths>

CustomWebPage::CustomWebPage(QWebEngineProfile *profile, QWidget *parent)
    : QWebEnginePage(profile, parent) {}

QStringList CustomWebPage::chooseFiles(FileSelectionMode mode,
                                 const QStringList &oldFiles,
                                 const QStringList &acceptedMimeTypes) {
    Q_UNUSED(oldFiles);
    Q_UNUSED(acceptedMimeTypes);

    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    if (mode == FileSelectOpen) {
        QString path = FilePicker::getOpenFileName(nullptr, "Select File", home);
        return path.isEmpty() ? QStringList() : QStringList() << path;
    } else if (mode == FileSelectOpenMultiple) {
        return FilePicker::getOpenFileNames(nullptr, "Select Files", home);
    }
    return {};
}
