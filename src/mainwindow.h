#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebChannel>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineFindTextResult>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QSplitter>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QKeyEvent>
#include <QMap>
#include <set>
#include "adblocker.h"
#include "pip_window.h"
#include "password_manager.h"
#include "extension_system.h"
#include "sync_manager.h"
#include "media_bar.h"
#include "reading_mode.h"

class CustomWebPage;

// ── ToolsBackend ──────────────────────────────────────────────────────────
// Exposed to tools.html via QWebChannel as channel.objects.backend.
// Every card in tools.html calls backend.run_tool(name).
// install_extension(url) triggers an extension install and emits
// extensionInstalled(success, message) when done — tools.html listens for it.
// check_deps() emits depsStatus(json) with installed/missing state per group.
// install_deps(group) runs pkexec apt-get install for the requested group.
class ToolsBackend : public QObject {
    Q_OBJECT
public:
    explicit ToolsBackend(QObject *mainWindow, QObject *parent = nullptr);

public slots:
    Q_INVOKABLE void run_tool(const QString &name);
    Q_INVOKABLE void install_extension(const QString &url);
    Q_INVOKABLE void check_deps();
    Q_INVOKABLE void install_deps(const QString &group);

signals:
    void extensionInstalled(bool success, const QString &message);
    // JSON string: { "group": { "label": "...", "installed": true/false }, ... }
    void depsStatus(const QString &jsonStatus);
    // Emitted during install: { "group": "...", "state": "installing"|"done"|"failed", "msg": "..." }
    void installProgress(const QString &jsonProgress);

private:
    QObject *m_mainWindow;  // MainWindow* — avoid circular include
};

class TabWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabWidget(const QString &url, QWebEngineProfile *profile, QWidget *parent = nullptr);
    QWebEngineView *browser() const { return m_browser; }
    QWebEngineView *pdfViewer() const { return m_pdfViewer; }

    // Call after setWebChannel() to trigger the actual page load.
    // Optionally pass a different URL to override the one given to the constructor.
    void loadUrl(const QString &url = QString());

    bool isPinned = false;
    bool isMuted  = false;

private slots:
    void checkPdf(const QUrl &url);
    void onNewWindow(QWebEngineNewWindowRequest &request);

private:
    QSplitter *m_splitter;
    QWebEngineView *m_browser;
    QWebEngineView *m_pdfViewer;
    QString    m_pendingUrl;   // stored in ctor, navigated in loadUrl()
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(bool isPrivate = false, QWidget *parent = nullptr);
    TabWidget *newTab(const QString &url = QString());
    ExtensionSystem *extensions() const { return m_extensions; }

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void back();
    void forward();
    void reload();
    void navigateHome();
    void navigateToUrl();
    void showBookmarksMenu();
    void showDownloadMenu();
    void showToolsMenu();
    void showSettingsMenu();

    // Find-in-page
    void openFindBar();
    void closeFindBar();
    void findNext();
    void findPrev();

    // Fullscreen
    void toggleFullscreen();

    // Zoom
    void zoomIn();
    void zoomOut();
    void zoomReset();

    // Tab actions
    void duplicateTab();
    void pinTab();
    void muteTab();
    void detachTab();
    void moveTabLeft();
    void moveTabRight();
    void showTabContextMenu(const QPoint &pos);
    void closeCurrentTab();
    void nextTab();
    void prevTab();
    void reopenLastTab();
    void bookmarkCurrentPage();
    void focusUrlBar();

    // Page actions
    void savePage();
    void viewSource();
    void printPage();
    void copyPageUrl();
    void showPageInfo();

    // New features
    void openPip();
    void openPasswordManager();
    void autofillPassword();
    void openExtensions();
    void openSync();
    void toggleMediaBar();
    void toggleReadingMode();

    // Tools
    void openTranslate();
    void openTranscript();
    void openSearch();
    void openWeather();
    void openPdfMerge();
    void openPdfSplit();
    void openWordToPdf();
    void openPdfToWord();
    void openXlsxToPdf();
    void openPdfToXlsx();
    void openCsvToXlsx();
    void openXlsxToCsv();
    void openPptxToPdf();
    void openPdfToPptx();
    void openImageToPdf();
    void openPdfToImage();
    void openTextToPdf();
    void openPdfToText();
    void openArchiveTools();
    void openTimer();
    void openQr();
    void openUnitConverter();
    void openCalculator();
    void openProgrammerCalc();
    void openNoteTaker();
    void openToolsHub();
    void openPrivateWindow();

private:
    void buildUi();
    void setupShortcuts();
    void restoreWindow();
    void injectAdblock();
    void injectDarkMode();
    void removeDarkMode();
    void restoreTabs();
    void applyTheme();
    void setupDns();
    QWebEngineView *currentBrowser();
    TabWidget      *currentTabWidget();
    void updateTabTitle(TabWidget *tw, QWebEngineView *br, const QString &title);
    void recordHistory(const QUrl &url);
    void loadData();
    void saveData();
    QString configDir();
    QString downloadDir();
    void applyZoom(QWebEngineView *br, const QString &host);
    QString hostOf(const QUrl &url) const;

    bool m_isPrivate;
    bool m_darkMode    = false;
    bool m_fullscreen  = false;
    QAction    *m_themeAction = nullptr;
    QPushButton *m_themeBtn  = nullptr;
    QToolBar   *m_navbar     = nullptr;

    // Find bar
    QWidget    *m_findBar    = nullptr;
    QLineEdit  *m_findEdit   = nullptr;
    QLabel     *m_findStatus = nullptr;

    QWebEngineProfile *m_profile;
    QWebChannel *m_channel;
    QTabWidget  *m_tabs;
    QLineEdit   *m_urlBar;
    QSettings   *m_settings;
    QTimer      *m_autoSaveTimer = nullptr;

    QJsonObject m_data;
    std::set<QString> m_seenUrls;
    QString m_home;
    QString m_configDir;
    QString m_dataFile;

    // Per-host zoom levels  (host → factor, e.g. 1.25)
    QMap<QString, double> m_zoomLevels;

    // Features
    PasswordManager  *m_passwords     = nullptr;
    ExtensionSystem  *m_extensions    = nullptr;
    SyncManager      *m_sync          = nullptr;
    MediaBar         *m_mediaBar      = nullptr;
    ReadingMode      *m_reader        = nullptr;
    PipWindow        *m_pip           = nullptr;
    ToolsBackend     *m_toolsBackend  = nullptr;
    QMetaObject::Connection m_readerTitleConn;
};
