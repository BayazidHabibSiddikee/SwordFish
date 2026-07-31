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
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QSplitter>
#include <QJsonObject>
#include <QJsonArray>
#include <set>
#include "adblocker.h"

class CustomWebPage;

class TabWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabWidget(const QString &url, QWebEngineProfile *profile, QWidget *parent = nullptr);
    QWebEngineView *browser() const { return m_browser; }
    QWebEngineView *pdfViewer() const { return m_pdfViewer; }

private slots:
    void checkPdf(const QUrl &url);
    void onNewWindow(QWebEngineNewWindowRequest &request);

private:
    QSplitter *m_splitter;
    QWebEngineView *m_browser;
    QWebEngineView *m_pdfViewer;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(bool isPrivate = false, QWidget *parent = nullptr);
    TabWidget *newTab(const QString &url = QString());

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void restoreWindow();
    void injectAdblock();
    void injectDarkMode();
    void removeDarkMode();
    void restoreTabs();
    void applyTheme();
    void setupDns();
    QWebEngineView *currentBrowser();
    void updateTabTitle(TabWidget *tw, QWebEngineView *br, const QString &title);
    void recordHistory(const QUrl &url);
    void loadData();
    void saveData();
    QString configDir();
    QString downloadDir();

    bool m_isPrivate;
    bool m_darkMode = false;
    QAction *m_themeAction = nullptr;
    QPushButton *m_themeBtn = nullptr;
    QWebEngineProfile *m_profile;
    QWebChannel *m_channel;
    QTabWidget *m_tabs;
    QLineEdit *m_urlBar;
    QSettings *m_settings;
    QTimer *m_autoSaveTimer;

    QJsonObject m_data;
    std::set<QString> m_seenUrls;
    QString m_home;
    QString m_configDir;
    QString m_dataFile;
};
