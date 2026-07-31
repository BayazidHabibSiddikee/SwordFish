#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SwordFish");
    QApplication::setOrganizationName("SwordFish");

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--logging-level=3");

    MainWindow window;
    window.show();

    return app.exec();
}
