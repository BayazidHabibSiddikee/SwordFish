#include <QApplication>
#include <QLocale>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSurfaceFormat>

#include "mainwindow.h"

// ── Version info (also set in CMakeLists.txt) ─────────────────────────────
#ifndef APP_VERSION
#define APP_VERSION "2.0.0"
#endif
#ifndef APP_NAME
#define APP_NAME "SwordFish"
#endif

int main(int argc, char *argv[]) {
    // ── High-DPI support ──────────────────────────────────────────────────
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // ── App metadata ──────────────────────────────────────────────────────
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationDisplayName(APP_NAME " Browser");
    QApplication::setApplicationVersion(APP_VERSION);
    QApplication::setOrganizationName("SwordFish");
    QApplication::setOrganizationDomain("swordfish.browser");

    // ── App icon (packaged or from source tree) ───────────────────────────
    auto loadIcon = []() -> QIcon {
        // Installed path: /usr/share/icons/hicolor/…  or same dir as binary
        QStringList candidates = {
            QCoreApplication::applicationDirPath() + "/icon.png",
            QCoreApplication::applicationDirPath() + "/../share/swordfish/icon.png",
            QCoreApplication::applicationDirPath() + "/../share/icons/hicolor/256x256/apps/swordfish.png",
            "/usr/share/icons/hicolor/256x256/apps/swordfish.png",
            "/usr/local/share/icons/hicolor/256x256/apps/swordfish.png",
        };
        for (const QString &p : candidates)
            if (QFile::exists(p)) return QIcon(p);
        return QIcon();
    };
    QIcon appIcon = loadIcon();
    if (!appIcon.isNull()) app.setWindowIcon(appIcon);

    // ── Locale ────────────────────────────────────────────────────────────
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    // ── Chromium / WebEngine flags ────────────────────────────────────────
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        "--logging-level=3 "
        "--disable-logging "
        "--no-sandbox "
        "--enable-gpu-rasterization "
        "--enable-zero-copy");

    // ── Handle --version / --about CLI flags ──────────────────────────────
    const QStringList args = QCoreApplication::arguments();
    if (args.contains("--version") || args.contains("-v")) {
        QTextStream(stdout) << APP_NAME " Browser " APP_VERSION "\n";
        return 0;
    }

    // ── Open URL from command line (e.g. xdg-open) ───────────────────────
    QString startUrl;
    for (const QString &arg : args) {
        if (arg.startsWith("http://") || arg.startsWith("https://")
         || arg.startsWith("file://"))
            startUrl = arg;
    }

    MainWindow window;
    if (!startUrl.isEmpty())
        window.newTab(startUrl);
    window.show();

    return app.exec();
}
