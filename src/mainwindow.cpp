#include "mainwindow.h"
#include "web_page.h"
#include "styles.h"
#include "folder_picker.h"
#include "file_picker.h"
#include "tools/pdf_tools.h"
#include "tools/doc_tools.h"
#include "tools/office_tools.h"
#include "tools/archive_tools.h"
#include "tools/student_tools.h"
#include "tools/translate.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>
#include <QSplitter>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCursor>
#include <QProcess>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QTimer>
#include <QThread>

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

static const QRegularExpression s_navPattern(
    R"(^(https?://|ftp://|file://|about:|chrome-extension://))",
    QRegularExpression::CaseInsensitiveOption
);
static const QRegularExpression s_ipPattern(
    R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}(:\d+)?$)"
);
static const QRegularExpression s_hostPortPattern(
    R"(^[\w.-]+:\d+(/.*)?$)"
);
static const QRegularExpression s_hostnamePattern(
    R"(^[a-zA-Z][\w-]*$)"
);
static const QString s_searchUrl = "https://duckduckgo.com/?q=";

static bool toolExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

static bool confirmInstall(const QString &cmd, const QString &toolName, QWidget *parent) {
    if (toolExists(cmd)) return true;
    QMessageBox::warning(parent, "Tool Not Found",
        QString("%1 is not installed.\n\nInstall it with:\n  sudo apt install %2")
            .arg(toolName).arg(cmd));
    return false;
}

// ── TabWidget ─────────────────────────────────────────────────────────────

TabWidget::TabWidget(const QString &url, QWebEngineProfile *profile, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(this);
    layout->addWidget(m_splitter);

    m_browser = new QWebEngineView();
    m_splitter->addWidget(m_browser);

    m_pdfViewer = new QWebEngineView();
    m_pdfViewer->setVisible(false);
    m_splitter->addWidget(m_pdfViewer);

    if (profile) {
        auto *page = new CustomWebPage(profile, this);
        connect(page, &QWebEnginePage::newWindowRequested, this, &TabWidget::onNewWindow);
        m_browser->setPage(page);

        auto *pdfPage = new CustomWebPage(profile, this);
        m_pdfViewer->setPage(pdfPage);
    }

    if (!url.isEmpty()) {
        m_browser->setUrl(QUrl(url));
        checkPdf(QUrl(url));
    }

    connect(m_browser, &QWebEngineView::urlChanged, this, &TabWidget::checkPdf);
}

void TabWidget::checkPdf(const QUrl &url) {
    QString str = url.toString().toLower();
    if (str.endsWith(".pdf") || (str.startsWith("file://") && str.endsWith(".pdf"))) {
        m_pdfViewer->setUrl(url);
        m_pdfViewer->setVisible(true);
        m_splitter->setSizes({width() / 2, width() / 2});
    } else {
        m_pdfViewer->setVisible(false);
    }
}

void TabWidget::onNewWindow(QWebEngineNewWindowRequest &request) {
    QString url = request.requestedUrl().toString();
    auto *main = qobject_cast<MainWindow*>(window());
    if (main && !url.isEmpty()) {
        auto *tw = main->newTab(url);
        if (tw && tw->browser()) {
            request.openIn(tw->browser()->page());
        }
    }
}

// ── MainWindow ────────────────────────────────────────────────────────────

MainWindow::MainWindow(bool isPrivate, QWidget *parent)
    : QMainWindow(parent), m_isPrivate(isPrivate)
{
    setWindowTitle(QString("SwordFish Browser") + (isPrivate ? " (Private)" : ""));

    m_settings = new QSettings("SwordFish", "Browser", this);
    m_configDir = configDir();
    m_dataFile = m_configDir + "/data.json";

    QString appDir = QCoreApplication::applicationDirPath();
    m_home = m_settings->value("home_url", "https://duckduckgo.com").toString();
    // If a legacy file:// home.html path was saved by the old Python version, reset it
    if (m_home.startsWith("file://") || m_home.isEmpty()) {
        m_home = "https://duckduckgo.com";
        m_settings->setValue("home_url", m_home);
    }

    // Load theme preference
    m_darkMode = m_settings->value("dark_mode", false).toBool();

    if (isPrivate) {
        m_profile = new QWebEngineProfile(this);
    } else {
        m_profile = new QWebEngineProfile("SwordFish", this);
        QString profileDir = m_configDir + "/browser_profile";
        QDir().mkpath(profileDir);
        m_profile->setPersistentStoragePath(profileDir);
        m_profile->setCachePath(profileDir + "/cache");
        m_profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    }

    m_profile->setHttpAcceptLanguage("en-US,en;q=0.9");
    m_profile->setHttpUserAgent(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
    );

    m_channel = new QWebChannel(this);

    auto *interceptor = new AdBlocker();
    Q_UNUSED(interceptor);

    if (!isPrivate) {
        loadData();
        m_autoSaveTimer = new QTimer(this);
        connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::saveData);
        m_autoSaveTimer->start(30000);
    }

    buildUi();
    restoreWindow();
    injectAdblock();
    applyTheme();
}

QString MainWindow::configDir() {
    QString base;
#if defined(Q_OS_WIN)
    base = QDir::homePath();
#elif defined(Q_OS_ANDROID)
    base = QDir::homePath();
#else
    base = QDir::homePath() + "/.config";
#endif
    QString path = base + "/SwordFish";
    QDir().mkpath(path);
    return path;
}

void MainWindow::loadData() {
    QFile file(m_dataFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_data = doc.object();
        file.close();
    }
    if (m_data.isEmpty()) {
        m_data["bookmarks"] = QJsonArray();
        m_data["history"] = QJsonArray();
    }
}

void MainWindow::saveData() {
    QFile file(m_dataFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_data).toJson());
        file.close();
    }
}

QString MainWindow::downloadDir() {
    QString def;
#if defined(Q_OS_WIN)
    def = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#elif defined(Q_OS_ANDROID)
    def = QDir::homePath() + "/../sdcard/Download";
#else
    def = QDir::homePath() + "/Downloads";
#endif
    return m_settings->value("download_dir", def).toString();
}

void MainWindow::buildUi() {
    auto *navbar = addToolBar("Navigation");
    navbar->setMovable(false);

    struct NavBtn { QString label; void(MainWindow::*slot)(); };
    std::vector<NavBtn> navBtns = {
        {"\u25c0", &MainWindow::back},
        {"\u25b6", &MainWindow::forward},
        {"\u21bb", &MainWindow::reload},
        {"\u2302", &MainWindow::navigateHome},
    };
    for (auto &btn : navBtns) {
        auto *action = new QAction(btn.label, this);
        connect(action, &QAction::triggered, this, btn.slot);
        navbar->addAction(action);
    }

    m_urlBar = new QLineEdit();
    m_urlBar->setPlaceholderText("Search with Google or enter address");
    connect(m_urlBar, &QLineEdit::returnPressed, this, &MainWindow::navigateToUrl);
    navbar->addWidget(m_urlBar);

    auto *bmBtn = new QAction("\u2606 Bookmark", this);
    connect(bmBtn, &QAction::triggered, this, &MainWindow::showBookmarksMenu);
    navbar->addAction(bmBtn);

    auto *dlBtn = new QAction("\u2b07 Download", this);
    connect(dlBtn, &QAction::triggered, this, &MainWindow::showDownloadMenu);
    navbar->addAction(dlBtn);

    auto *toolsBtn = new QAction("\U0001f527 Tools", this);
    connect(toolsBtn, &QAction::triggered, this, &MainWindow::showToolsMenu);
    navbar->addAction(toolsBtn);

    auto *cfgBtn = new QAction("\u2699", this);
    connect(cfgBtn, &QAction::triggered, this, &MainWindow::showSettingsMenu);
    navbar->addAction(cfgBtn);

    // ── Theme toggle button ──
    m_themeBtn = new QPushButton(m_darkMode ? "☀ Light" : "🌙 Dark", this);
    m_themeBtn->setFixedHeight(28);
    m_themeBtn->setMinimumWidth(80);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_themeBtn->setStyleSheet(
        "QPushButton { background-color: #0077b6; color: white; border: none;"
        "  border-radius: 4px; padding: 0 12px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0096c7; }");
    connect(m_themeBtn, &QPushButton::clicked, this, [this]() {
        m_darkMode = !m_darkMode;
        m_settings->setValue("dark_mode", m_darkMode);
        applyTheme();
    });
    navbar->addWidget(m_themeBtn);
    m_themeAction = nullptr;  // using widget instead

    m_tabs = new QTabWidget();
    m_tabs->setTabsClosable(true);
    m_tabs->setDocumentMode(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int idx) {
        if (m_tabs->count() > 1) {
            auto *w = qobject_cast<TabWidget*>(m_tabs->widget(idx));
            if (w && w->browser()) {
                w->browser()->disconnect();
            }
            m_tabs->removeTab(idx);
            w->deleteLater();
        } else {
            close();
        }
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) {
        auto *br = currentBrowser();
        if (br) m_urlBar->setText(br->url().toString());
    });

    auto *tabBtn = new QPushButton("TAB");
    tabBtn->setFixedSize(50, 28);
    tabBtn->setStyleSheet("background-color: transparent; color: #023e8a; font-size: 13px; font-weight: bold; border: none;");
    connect(tabBtn, &QPushButton::clicked, this, [this]() { newTab(); });
    m_tabs->setCornerWidget(tabBtn);

    setCentralWidget(m_tabs);

    if (!m_isPrivate) {
        restoreTabs();
    } else {
        newTab(m_home);
    }
}

QWebEngineView *MainWindow::currentBrowser() {
    auto *w = qobject_cast<TabWidget*>(m_tabs->currentWidget());
    return w ? w->browser() : nullptr;
}

TabWidget *MainWindow::newTab(const QString &url) {
    auto *tw = new TabWidget(url.isEmpty() ? m_home : url, m_profile);
    int idx = m_tabs->addTab(tw, "New Tab");
    m_tabs->setCurrentIndex(idx);
    auto *br = tw->browser();
    if (br->page()) {
        br->page()->setWebChannel(m_channel);
    }
    connect(br, &QWebEngineView::titleChanged, this, [this, tw, br](const QString &t) {
        updateTabTitle(tw, br, t);
    });
    connect(br, &QWebEngineView::urlChanged, this, &MainWindow::recordHistory);
    return tw;
}

void MainWindow::updateTabTitle(TabWidget *tw, QWebEngineView *br, const QString &title) {
    int idx = m_tabs->indexOf(tw);
    if (idx >= 0) {
        QString shortTitle = title.length() > 20 ? title.left(20) + "\u2026" : title;
        m_tabs->setTabText(idx, shortTitle.isEmpty() ? "Tab" : shortTitle);
        m_tabs->setTabToolTip(idx, title);
    }
}

void MainWindow::restoreWindow() {
    QSize geoSize = m_settings->value("window_size").toSize();
    QPoint geoPos = m_settings->value("window_pos").toPoint();
    bool maximized = m_settings->value("maximized", true).toBool();

    if (!geoSize.isEmpty()) resize(geoSize);
    if (!geoPos.isNull()) move(geoPos);
    if (maximized) showMaximized(); else show();
}

void MainWindow::restoreTabs() {
    QJsonArray tabs = m_data["tabs"].toArray();
    if (!tabs.isEmpty()) {
        for (const auto &v : tabs) {
            newTab(v.toString());
        }
        int active = m_data["active_tab"].toInt(0);
        if (active < m_tabs->count()) m_tabs->setCurrentIndex(active);
    } else {
        newTab(m_home);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_isPrivate) { QMainWindow::closeEvent(event); return; }
    m_autoSaveTimer->stop();
    m_settings->setValue("window_size", size());
    m_settings->setValue("window_pos", pos());
    m_settings->setValue("maximized", isMaximized());
    m_settings->setValue("home_url", m_home);

    QJsonArray tabsArr;
    for (int i = 0; i < m_tabs->count(); i++) {
        auto *w = qobject_cast<TabWidget*>(m_tabs->widget(i));
        if (w && w->browser()) {
            QString url = w->browser()->url().toString();
            if (!url.isEmpty()) tabsArr.append(url);
        }
    }
    m_data["tabs"] = tabsArr;
    m_data["active_tab"] = m_tabs->currentIndex();
    saveData();
    QMainWindow::closeEvent(event);
}

// ── Navigation ────────────────────────────────────────────────────────────

void MainWindow::back() { auto *br = currentBrowser(); if (br) br->back(); }
void MainWindow::forward() { auto *br = currentBrowser(); if (br) br->forward(); }
void MainWindow::reload() { auto *br = currentBrowser(); if (br) br->reload(); }
void MainWindow::navigateHome() { auto *br = currentBrowser(); if (br) br->setUrl(QUrl(m_home)); }

void MainWindow::navigateToUrl() {
    auto *br = currentBrowser();
    if (!br) return;
    QString raw = m_urlBar->text().trimmed();
    if (raw.isEmpty()) return;

    QString url;
    if (s_navPattern.match(raw).hasMatch()) {
        url = raw;
    } else if (raw.contains(".") && !raw.contains(" ")) {
        url = "http://" + raw;
    } else if (s_ipPattern.match(raw).hasMatch()) {
        url = "http://" + raw;
    } else if (s_hostPortPattern.match(raw).hasMatch()) {
        url = "http://" + raw;
    } else if (s_hostnamePattern.match(raw).hasMatch()) {
        url = "http://" + raw;
    } else {
        url = s_searchUrl + raw.replace(" ", "+");
    }
    br->setUrl(QUrl(url));
}

void MainWindow::applyTheme() {
    QString sheet = m_darkMode ? Styles::getDarkStyleSheet() : Styles::getStyleSheet();
    qApp->setStyleSheet(sheet);

    // Inject or remove dark CSS from web pages
    if (m_darkMode)
        injectDarkMode();
    else
        removeDarkMode();

    // Update the tab "new tab" button colour
    auto *tabBtn = qobject_cast<QPushButton*>(m_tabs->cornerWidget());
    if (tabBtn) {
        tabBtn->setStyleSheet(m_darkMode
            ? "background-color: transparent; color: #00d2ff; font-size: 13px; font-weight: bold; border: none;"
            : "background-color: transparent; color: #023e8a; font-size: 13px; font-weight: bold; border: none;");
    }

    // Update theme toggle button
    if (m_themeBtn) {
        m_themeBtn->setText(m_darkMode ? "☀ Light" : "🌙 Dark");
        m_themeBtn->setToolTip(m_darkMode ? "Switch to Light mode" : "Switch to Dark mode");
        m_themeBtn->setStyleSheet(m_darkMode
            ? "QPushButton { background-color: rgba(0,60,100,0.8); color: #00d2ff;"
              "  border: 1px solid rgba(0,210,255,0.4); border-radius: 4px;"
              "  padding: 0 12px; font-size: 13px; font-weight: bold; }"
              "QPushButton:hover { background-color: rgba(0,100,160,0.9); color: #fff; }"
            : "QPushButton { background-color: #0077b6; color: white; border: none;"
              "  border-radius: 4px; padding: 0 12px; font-size: 13px; font-weight: bold; }"
              "QPushButton:hover { background-color: #0096c7; }");
    }
}

void MainWindow::recordHistory(const QUrl &url) {
    if (m_isPrivate) return;
    QString str = url.toString();
    if (m_seenUrls.count(str)) return;
    m_seenUrls.insert(str);
    auto *br = currentBrowser();
    QString title = br ? br->title() : str;
    QJsonArray hist = m_data["history"].toArray();
    QJsonObject entry;
    entry["url"] = str;
    entry["title"] = title;
    hist.append(entry);
    while (hist.size() > 200) hist.removeFirst();
    m_data["history"] = hist;
}

// ── Bookmarks ─────────────────────────────────────────────────────────────

void MainWindow::showBookmarksMenu() {
    QMenu menu(this);

    auto *add = new QAction("\u2795  Bookmark this page", this);
    connect(add, &QAction::triggered, this, [this]() {
        auto *br = currentBrowser();
        if (!br) return;
        QString url = br->url().toString();
        QString title = br->title().isEmpty() ? url : br->title();
        QJsonArray bms = m_data["bookmarks"].toArray();
        for (const auto &b : bms) {
            if (b.toObject()["url"].toString() == url) {
                QMessageBox::information(this, "Bookmark", "Already bookmarked!");
                return;
            }
        }
        QJsonObject bm;
        bm["url"] = url;
        bm["title"] = title;
        bms.append(bm);
        m_data["bookmarks"] = bms;
        saveData();
        QMessageBox::information(this, "Bookmark", QString("Saved:\n%1").arg(title));
    });
    menu.addAction(add);

    auto *histMenu = menu.addMenu("\U0001f55b  History");
    QJsonArray hist = m_data["history"].toArray();
    int start = std::max(0, static_cast<int>(hist.size()) - 20);
    for (int i = hist.size() - 1; i >= start; i--) {
        QJsonObject entry = hist[i].toObject();
        auto *a = new QAction(entry["title"].toString().left(60), this);
        connect(a, &QAction::triggered, this, [this, entry]() {
            newTab(entry["url"].toString());
        });
        histMenu->addAction(a);
    }

    QJsonArray bms = m_data["bookmarks"].toArray();
    if (!bms.isEmpty()) {
        menu.addSeparator();
        for (const auto &b : bms) {
            QJsonObject bm = b.toObject();
            auto *a = new QAction("\U0001f516 " + bm["title"].toString().left(50), this);
            connect(a, &QAction::triggered, this, [this, bm]() {
                newTab(bm["url"].toString());
            });
            menu.addAction(a);
        }
    }

    menu.exec(QCursor::pos());
}

// ── Settings Menu ─────────────────────────────────────────────────────────

void MainWindow::showSettingsMenu() {
    QMenu menu(this);

    auto *newPrivate = new QAction("\U0001f575\ufe0f  New Private Window", this);
    connect(newPrivate, &QAction::triggered, this, &MainWindow::openPrivateWindow);
    menu.addAction(newPrivate);
    menu.addSeparator();

    auto *setHome = new QAction("\U0001f3e0  Set current page as Home", this);
    connect(setHome, &QAction::triggered, this, [this]() {
        auto *br = currentBrowser();
        if (br) {
            m_home = br->url().toString();
            m_settings->setValue("home_url", m_home);
            QMessageBox::information(this, "Home", QString("Home set to:\n%1").arg(m_home));
        }
    });
    menu.addAction(setHome);

    auto *setDl = new QAction("\U0001f4c1  Change download folder", this);
    connect(setDl, &QAction::triggered, this, [this]() {
        FolderPickerDialog dlg(downloadDir(), this);
        if (dlg.exec() == QDialog::Accepted && !dlg.selectedPath().isEmpty()) {
            m_settings->setValue("download_dir", dlg.selectedPath());
            QMessageBox::information(this, "Download Folder",
                                     QString("Saved:\n%1").arg(dlg.selectedPath()));
        }
    });
    menu.addAction(setDl);

    auto *clearHist = new QAction("\U0001f5d1  Clear history", this);
    connect(clearHist, &QAction::triggered, this, [this]() {
        m_data["history"] = QJsonArray();
        m_seenUrls.clear();
        saveData();
        QMessageBox::information(this, "History", "History cleared.");
    });
    if (m_isPrivate) clearHist->setEnabled(false);
    menu.addAction(clearHist);

    auto *clearCache = new QAction("\U0001f5d1  Clear cache", this);
    connect(clearCache, &QAction::triggered, this, [this]() {
        m_profile->clearHttpCache();
        QMessageBox::information(this, "Cache", "Cache cleared.");
    });
    if (m_isPrivate) clearCache->setEnabled(false);
    menu.addAction(clearCache);

    auto *clearBm = new QAction("\U0001f5d1  Clear bookmarks", this);
    connect(clearBm, &QAction::triggered, this, [this]() {
        m_data["bookmarks"] = QJsonArray();
        saveData();
        QMessageBox::information(this, "Bookmarks", "Bookmarks cleared.");
    });
    if (m_isPrivate) clearBm->setEnabled(false);
    menu.addAction(clearBm);

    menu.addSeparator();

    auto *blockMenu = menu.addMenu("\U0001f6e1  Adblock Level");
    QStringList levels = {"none", "low", "medium", "ultimate"};
    QString currentLevel = "low";
    for (const auto &level : levels) {
        auto *a = new QAction(level == "none" ? "Disabled (Off)" : level.toUpper(), this);
        a->setCheckable(true);
        a->setChecked(level == currentLevel);
        connect(a, &QAction::triggered, this, [this, level]() {
            getBlocker().setLevel(
                level == "none" ? AdBlocker::Level::None :
                level == "low" ? AdBlocker::Level::Low :
                level == "medium" ? AdBlocker::Level::Medium :
                AdBlocker::Level::Ultimate
            );
            QMessageBox::information(this, "Adblock Level",
                                     QString("Set to %1").arg(level));
        });
        blockMenu->addAction(a);
    }

    menu.addSeparator();
    auto *about = new QAction(QString("Info: %1").arg(m_configDir), this);
    about->setEnabled(false);
    menu.addAction(about);

    menu.exec(QCursor::pos());
}

void MainWindow::openPrivateWindow() {
    auto *w = new MainWindow(true);
    w->show();
}

// ── Download Menu ─────────────────────────────────────────────────────────

void MainWindow::showDownloadMenu() {
    QMenu menu(this);

    if (!toolExists("yt-dlp")) {
        auto *a = new QAction("yt-dlp not installed - Download disabled", this);
        a->setEnabled(false);
        menu.addAction(a);
        menu.addSeparator();
        auto *hint = new QAction("Install: pip install yt-dlp", this);
        hint->setEnabled(false);
        menu.addAction(hint);
        menu.exec(QCursor::pos());
        return;
    }

    auto *videoMenu = menu.addMenu("\U0001f3ac  Video");
    struct VideoFmt { QString label; QString fmt; };
    std::vector<VideoFmt> videoFormats = {
        {"144p",         "bestvideo[height<=144]+bestaudio/best[height<=144]"},
        {"360p",         "bestvideo[height<=360]+bestaudio/best[height<=360]"},
        {"480p",         "bestvideo[height<=480]+bestaudio/best[height<=480]"},
        {"720p  (HD)",   "bestvideo[height<=720]+bestaudio/best[height<=720]"},
        {"1080p (FHD)",  "bestvideo[height<=1080]+bestaudio/best[height<=1080]"},
        {"4K    (best)", "bestvideo+bestaudio/best"},
    };
    for (auto &fmt : videoFormats) {
        auto *a = new QAction(fmt.label, this);
        connect(a, &QAction::triggered, this, [this, fmt]() {
            QProcess::startDetached("yt-dlp",
                QStringList() << "-f" << fmt.fmt
                              << "-o" << (downloadDir() + "/%(title)s.%(ext)s")
                              << currentBrowser()->url().toString());
        });
        videoMenu->addAction(a);
    }

    auto *audioMenu = menu.addMenu("\U0001f3b5  Audio only");
    struct AudioFmt { QString label; QStringList args; };
    std::vector<AudioFmt> audioFormats = {
        {"MP3  (128k)", QStringList() << "-x" << "--audio-format" << "mp3" << "--audio-quality" << "128K"},
        {"MP3  (320k)", QStringList() << "-x" << "--audio-format" << "mp3" << "--audio-quality" << "0"},
        {"M4A  (best)", QStringList() << "-x" << "--audio-format" << "m4a"},
        {"OGG  (best)", QStringList() << "-x" << "--audio-format" << "vorbis"},
    };
    for (auto &fmt : audioFormats) {
        auto *a = new QAction(fmt.label, this);
        connect(a, &QAction::triggered, this, [this, fmt]() {
            QStringList args = fmt.args;
            args << "-o" << (downloadDir() + "/%(title)s.%(ext)s")
                 << currentBrowser()->url().toString();
            QProcess::startDetached("yt-dlp", args);
        });
        audioMenu->addAction(a);
    }

    menu.exec(QCursor::pos());
}

// ── Adblock Injection ─────────────────────────────────────────────────────

void MainWindow::injectAdblock() {
    QString script = R"(
(function() {
    'use strict';
    const AD_SELECTORS = [
        '.adsbygoogle', '.adsense', '.advertisement',
        '.ad-container', '.ad-wrap', '.ad-placeholder', '.ad-unit',
        '.ad-banner', '.ad-slot', '.ad-box',
        '#ad-sidebar', '#ad-banner', '#ad-container', '#ad-wrap',
        'iframe[src*="doubleclick"]', 'iframe[src*="googlead"]',
        'ins.adsbygoogle',
        '.video-ads', '.ytp-ad-module', '.ytd-ad-slot-renderer',
        '#masthead-ad', '#player-ads',
        'ytd-rich-shelf-renderer[is-shorts]',
        'ytd-reel-shelf-renderer',
        'a[title="Shorts"]', '[title="Shorts"]',
    ];
    const AD_KEYWORDS = ['doubleclick', 'googlead', 'adservice', 'adserver', 'adnxs', 'adzerk'];

    function cleanup() {
        AD_SELECTORS.forEach(sel => {
            document.querySelectorAll(sel).forEach(el => el.remove());
        });
        document.querySelectorAll('img').forEach(el => {
            if (el.src && !el.src.startsWith('data:') && AD_KEYWORDS.some(k => el.src.includes(k)))
                el.remove();
        });
        ['adblock','adblocker','ad-block','ad-blocker'].forEach(cls => {
            document.querySelectorAll('.' + cls).forEach(el => el.remove());
        });
    }
    cleanup();
    let timer = null;
    new MutationObserver(() => {
        if (timer) return;
        timer = setTimeout(() => { timer = null; cleanup(); }, 300);
    }).observe(document.documentElement, { childList: true, subtree: true });
})();
)";

    auto *webScript = new QWebEngineScript();
    webScript->setName("adblock");
    webScript->setSourceCode(script);
    webScript->setInjectionPoint(QWebEngineScript::DocumentReady);
    webScript->setWorldId(QWebEngineScript::MainWorld);
    webScript->setRunsOnSubFrames(true);
    m_profile->scripts()->insert(*webScript);
}

void MainWindow::injectDarkMode() {
    // Remove any existing dark mode script first
    removeDarkMode();

    // Proper dark mode CSS — directly sets dark colors, NO invert filter.
    // Strategy: use CSS custom properties + forced-colors override on background/surface
    // elements. Text is left at whatever the site sets so readability is preserved.
    // Images, video, canvas are completely untouched.
    QString css = R"CSS(
/* ── Force dark color-scheme so browsers respect prefers-color-scheme ── */
:root {
    color-scheme: dark !important;
    --sf-bg:       #0d1117 !important;
    --sf-surface:  #161b22 !important;
    --sf-surface2: #1c2128 !important;
    --sf-border:   #30363d !important;
    --sf-text:     #e6edf3 !important;
    --sf-muted:    #8b949e !important;
    --sf-accent:   #00b4d8 !important;
}

/* ── Page background ── */
html {
    background-color: #0d1117 !important;
    color: #e6edf3 !important;
}
body {
    background-color: #0d1117 !important;
    color: #e6edf3 !important;
}

/* ── Common surface elements ── */
header, nav, footer, aside, main, section, article,
[role="banner"], [role="navigation"], [role="main"],
[role="complementary"], [role="contentinfo"] {
    background-color: #161b22 !important;
    border-color: #30363d !important;
}

/* ── Sidebars, panels, cards, boxes ── */
div, span, li, ul, ol, dl, dt, dd,
[class*="sidebar"], [class*="panel"], [class*="card"],
[class*="box"], [class*="container"], [class*="wrap"],
[class*="widget"], [class*="banner"], [class*="modal"],
[class*="dialog"], [class*="drawer"], [class*="sheet"],
[class*="overlay"], [class*="popup"], [class*="tooltip"],
[class*="header"], [class*="footer"], [class*="nav"],
[class*="menu"], [class*="toolbar"], [class*="bar"],
[id*="sidebar"], [id*="panel"], [id*="header"],
[id*="footer"], [id*="nav"], [id*="menu"] {
    background-color: inherit !important;
    border-color: #30363d !important;
}

/* ── Inputs, buttons, selects ── */
input:not([type="submit"]):not([type="button"]):not([type="reset"]):not([type="checkbox"]):not([type="radio"]):not([type="range"]):not([type="color"]),
textarea, select {
    background-color: #161b22 !important;
    color: #e6edf3 !important;
    border: 1px solid #30363d !important;
}
input::placeholder, textarea::placeholder {
    color: #6e7681 !important;
}

/* ── Text — only override truly white/near-white backgrounds that would blind ── */
p, h1, h2, h3, h4, h5, h6, li, td, th, label, span, a {
    color: inherit;
}

/* ── Tables ── */
table { border-color: #30363d !important; }
tr, th, td {
    background-color: inherit !important;
    border-color: #30363d !important;
}
tr:nth-child(even) { background-color: #161b22 !important; }

/* ── YouTube specific ── */
ytd-app, #page-manager, ytd-browse, ytd-search,
#masthead, #masthead-container, ytd-masthead,
ytd-guide-renderer, #guide-inner-content,
ytd-mini-guide-renderer,
ytd-watch-flexy, #secondary, #primary,
ytd-rich-grid-renderer, ytd-section-list-renderer,
ytd-two-column-browse-results-renderer {
    background-color: #0d1117 !important;
    color: #e6edf3 !important;
}
ytd-thumbnail, ytd-playlist-thumbnail { background: transparent !important; }
yt-formatted-string, .ytd-video-primary-info-renderer,
.ytd-channel-name, #video-title, #title {
    color: #e6edf3 !important;
}
#description, ytd-expander {
    background-color: #161b22 !important;
    color: #c9d1d9 !important;
}

/* ── Video & images — NEVER touch ── */
video, img, canvas, picture, svg image,
iframe, embed, object {
    filter: none !important;
    opacity: 1 !important;
}

/* ── Scrollbars ── */
::-webkit-scrollbar { width: 8px; height: 8px; }
::-webkit-scrollbar-track { background: #0d1117; }
::-webkit-scrollbar-thumb {
    background: #30363d;
    border-radius: 4px;
}
::-webkit-scrollbar-thumb:hover { background: #00b4d8; }

/* ── Links ── */
a:not([class]) { color: #58a6ff !important; }
a:visited:not([class]) { color: #bc8cff !important; }

/* ── Code blocks ── */
code, pre, kbd, samp {
    background-color: #161b22 !important;
    color: #79c0ff !important;
    border-color: #30363d !important;
}
)CSS";

    QString script = QString(R"JS(
(function() {
    const STYLE_ID = '__sf_darkmode__';
    if (document.getElementById(STYLE_ID)) return;

    // Apply dark bg immediately before DOM loads to avoid white flash
    document.documentElement.style.backgroundColor = '#0d1117';
    document.documentElement.style.color = '#e6edf3';

    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = %1;
    (document.head || document.documentElement).appendChild(style);

    // Re-apply on dynamic content changes (SPAs like YouTube)
    let scheduled = false;
    new MutationObserver(() => {
        if (scheduled) return;
        scheduled = true;
        requestAnimationFrame(() => {
            scheduled = false;
            if (!document.getElementById(STYLE_ID)) {
                (document.head || document.documentElement).appendChild(style);
            }
        });
    }).observe(document.documentElement, { childList: true, subtree: false });
})();
)JS").arg("`" + css + "`");

    auto *s = new QWebEngineScript();
    s->setName("sf_darkmode");
    s->setSourceCode(script);
    s->setInjectionPoint(QWebEngineScript::DocumentCreation);
    s->setWorldId(QWebEngineScript::MainWorld);
    s->setRunsOnSubFrames(true);
    m_profile->scripts()->insert(*s);

    // Also apply to already-open tabs
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *tw = qobject_cast<TabWidget*>(m_tabs->widget(i));
        if (tw && tw->browser())
            tw->browser()->page()->runJavaScript(script);
    }
}

void MainWindow::removeDarkMode() {
    // Remove the script from profile so new tabs don't get it
    auto scripts = m_profile->scripts()->toList();
    for (const auto &s : scripts) {
        if (s.name() == "sf_darkmode") {
            m_profile->scripts()->remove(s);
            break;
        }
    }
    // Remove the injected style from all open tabs
    QString removeScript = R"JS(
(function() {
    const el = document.getElementById('__sf_darkmode__');
    if (el) el.remove();
    document.documentElement.style.backgroundColor = '';
    document.documentElement.style.color = '';
})();
)JS";
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *tw = qobject_cast<TabWidget*>(m_tabs->widget(i));
        if (tw && tw->browser())
            tw->browser()->page()->runJavaScript(removeScript);
    }
}

// ── Tools Menu ────────────────────────────────────────────────────────────

void MainWindow::showToolsMenu() {
    QMenu menu(this);

    auto *hub = new QAction("\U0001f680  Tools Hub (Web)", this);
    connect(hub, &QAction::triggered, this, &MainWindow::openToolsHub);
    menu.addAction(hub);
    menu.addSeparator();

    auto *langMenu = menu.addMenu("\U0001f310  Language");
    auto *t1 = new QAction("Translate", this);
    connect(t1, &QAction::triggered, this, &MainWindow::openTranslate);
    langMenu->addAction(t1);
    auto *t2 = new QAction("YouTube Transcript", this);
    connect(t2, &QAction::triggered, this, &MainWindow::openTranscript);
    langMenu->addAction(t2);

    auto *webMenu = menu.addMenu("\U0001f50d  Web");
    auto *t3 = new QAction("Search", this);
    connect(t3, &QAction::triggered, this, &MainWindow::openSearch);
    webMenu->addAction(t3);
    auto *t4 = new QAction("Weather", this);
    connect(t4, &QAction::triggered, this, &MainWindow::openWeather);
    webMenu->addAction(t4);

    auto *docMenu = menu.addMenu("\U0001f4c4  Documents");
    auto *pdfSub = docMenu->addMenu("PDF Tools");
    auto *t5 = new QAction("Merge PDFs", this);
    connect(t5, &QAction::triggered, this, &MainWindow::openPdfMerge);
    pdfSub->addAction(t5);
    auto *t6 = new QAction("Split PDF", this);
    connect(t6, &QAction::triggered, this, &MainWindow::openPdfSplit);
    pdfSub->addAction(t6);

    auto *wordSub = docMenu->addMenu("Word");
    auto *t7 = new QAction("DOCX \u2192 PDF", this);
    connect(t7, &QAction::triggered, this, &MainWindow::openWordToPdf);
    wordSub->addAction(t7);
    auto *t8 = new QAction("PDF \u2192 DOCX", this);
    connect(t8, &QAction::triggered, this, &MainWindow::openPdfToWord);
    wordSub->addAction(t8);

    auto *excelSub = docMenu->addMenu("Excel");
    auto *t9 = new QAction("XLSX \u2192 PDF", this);
    connect(t9, &QAction::triggered, this, &MainWindow::openXlsxToPdf);
    excelSub->addAction(t9);
    auto *t10 = new QAction("PDF \u2192 XLSX", this);
    connect(t10, &QAction::triggered, this, &MainWindow::openPdfToXlsx);
    excelSub->addAction(t10);
    auto *t11 = new QAction("CSV \u2192 XLSX", this);
    connect(t11, &QAction::triggered, this, &MainWindow::openCsvToXlsx);
    excelSub->addAction(t11);
    auto *t12 = new QAction("XLSX \u2192 CSV", this);
    connect(t12, &QAction::triggered, this, &MainWindow::openXlsxToCsv);
    excelSub->addAction(t12);

    auto *pptSub = docMenu->addMenu("PowerPoint");
    auto *t13 = new QAction("PPTX \u2192 PDF", this);
    connect(t13, &QAction::triggered, this, &MainWindow::openPptxToPdf);
    pptSub->addAction(t13);
    auto *t14 = new QAction("PDF \u2192 PPTX", this);
    connect(t14, &QAction::triggered, this, &MainWindow::openPdfToPptx);
    pptSub->addAction(t14);

    auto *otherSub = docMenu->addMenu("Other");
    auto *t15 = new QAction("Image \u2192 PDF", this);
    connect(t15, &QAction::triggered, this, &MainWindow::openImageToPdf);
    otherSub->addAction(t15);
    auto *t16 = new QAction("PDF \u2192 Image", this);
    connect(t16, &QAction::triggered, this, &MainWindow::openPdfToImage);
    otherSub->addAction(t16);
    auto *t17 = new QAction("Text \u2192 PDF", this);
    connect(t17, &QAction::triggered, this, &MainWindow::openTextToPdf);
    otherSub->addAction(t17);
    auto *t18 = new QAction("PDF \u2192 Text", this);
    connect(t18, &QAction::triggered, this, &MainWindow::openPdfToText);
    otherSub->addAction(t18);

    auto *utilMenu = menu.addMenu("\U0001f527  Utilities");
    auto *t19 = new QAction("Archive Tools (Zip/7z/Tar)", this);
    connect(t19, &QAction::triggered, this, &MainWindow::openArchiveTools);
    utilMenu->addAction(t19);
    auto *t20 = new QAction("Timer", this);
    connect(t20, &QAction::triggered, this, &MainWindow::openTimer);
    utilMenu->addAction(t20);
    auto *t21 = new QAction("QR Code Generator", this);
    connect(t21, &QAction::triggered, this, &MainWindow::openQr);
    utilMenu->addAction(t21);
    auto *t22 = new QAction("Unit Converter", this);
    connect(t22, &QAction::triggered, this, &MainWindow::openUnitConverter);
    utilMenu->addAction(t22);
    auto *t23 = new QAction("Calculator", this);
    connect(t23, &QAction::triggered, this, &MainWindow::openCalculator);
    utilMenu->addAction(t23);
    auto *t24 = new QAction("Programmer's Converter (Base)", this);
    connect(t24, &QAction::triggered, this, &MainWindow::openProgrammerCalc);
    utilMenu->addAction(t24);
    auto *t25 = new QAction("Note Taker", this);
    connect(t25, &QAction::triggered, this, &MainWindow::openNoteTaker);
    utilMenu->addAction(t25);

    menu.exec(QCursor::pos());
}

void MainWindow::openToolsHub() {
    QString appDir = QCoreApplication::applicationDirPath();
    QString path = appDir + "/../src/tools.html";
    if (QFile::exists(path)) {
        newTab("file://" + path);
    }
}

// ── Tool Dialogs ──────────────────────────────────────────────────────────

void MainWindow::openTranslate() {
    QDialog dlg(this);
    dlg.setObjectName("ToolDialog");
    dlg.setWindowTitle("Translate");
    dlg.setFixedWidth(400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *title = new QLabel("\U0001f310 Translator");
    title->setObjectName("Title");
    layout->addWidget(title);

    auto *textInput = new QTextEdit();
    textInput->setPlaceholderText("Enter text\u2026");
    textInput->setMaximumHeight(80);
    layout->addWidget(textInput);

    auto *row = new QHBoxLayout();
    auto *langCombo = new QComboBox();
    QMap<QString, QString> languages = {
        {"Bangla", "bn"}, {"Hindi", "hi"}, {"Spanish", "es"}, {"French", "fr"},
        {"German", "de"}, {"Japanese", "ja"}, {"Korean", "ko"}, {"Chinese", "zh"}
    };
    for (auto it = languages.begin(); it != languages.end(); ++it)
        langCombo->addItem(it.key());
    row->addWidget(langCombo);

    auto *btn = new QPushButton("Translate");
    row->addWidget(btn);
    layout->addLayout(row);

    auto *resultBox = new QTextEdit();
    resultBox->setObjectName("ResultBox");
    resultBox->setReadOnly(true);
    resultBox->setMaximumHeight(100);
    layout->addWidget(resultBox);

    connect(btn, &QPushButton::clicked, this, [textInput, langCombo, resultBox, &languages]() {
        QString text = textInput->toPlainText();
        QString lang = languages[langCombo->currentText()];
        resultBox->setPlainText("Translating\u2026");
        resultBox->setPlainText(TranslateTools::translateText(text, lang));
    });

    dlg.exec();
}

void MainWindow::openTranscript() {
    if (!confirmInstall("yt-dlp", "yt-dlp", this)) return;

    QDialog dlg(this);
    dlg.setObjectName("ToolDialog");
    dlg.setWindowTitle("Transcript");
    dlg.setFixedWidth(400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *title = new QLabel("\U0001f3ac YouTube Transcript");
    title->setObjectName("Title");
    layout->addWidget(title);

    auto *urlInput = new QLineEdit();
    urlInput->setPlaceholderText("Paste YouTube URL\u2026");
    auto *br = currentBrowser();
    if (br) {
        QString curUrl = br->url().toString();
        if (curUrl.contains("youtube")) urlInput->setText(curUrl);
    }
    layout->addWidget(urlInput);

    auto *fetchBtn = new QPushButton("Fetch Text");
    layout->addWidget(fetchBtn);

    auto *resultBox = new QTextEdit();
    resultBox->setObjectName("ResultBox");
    resultBox->setReadOnly(true);
    resultBox->setMaximumHeight(200);
    layout->addWidget(resultBox);

    connect(fetchBtn, &QPushButton::clicked, this, [urlInput, fetchBtn, resultBox]() {
        QString url = urlInput->text().trimmed();
        if (url.isEmpty()) return;
        fetchBtn->setEnabled(false);
        resultBox->setPlainText("Loading\u2026");
        // Use yt-dlp to get subtitles
        QProcess proc;
        proc.start("yt-dlp", QStringList() << "--write-auto-sub" << "--skip-download"
                                            << "--sub-lang" << "en" << "-o" << "/tmp/sf_transcript"
                                            << url);
        proc.waitForFinished(30000);
        // Read subtitle file
        QFile subFile("/tmp/sf_transcript.en.vtt");
        if (subFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            resultBox->setPlainText(QString::fromUtf8(subFile.readAll()));
            subFile.close();
        } else {
            resultBox->setPlainText("No transcript found.");
        }
        fetchBtn->setEnabled(true);
    });

    dlg.exec();
}

void MainWindow::openSearch() {
    QDialog dlg(this);
    dlg.setObjectName("ToolDialog");
    dlg.setFixedWidth(400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *title = new QLabel("\U0001f50d Search PDF");
    title->setObjectName("Title");
    layout->addWidget(title);

    auto *queryInput = new QLineEdit();
    queryInput->setPlaceholderText("Topic (e.g. quantum computing)\u2026");
    layout->addWidget(queryInput);

    auto *searchBtn = new QPushButton("Search PDFs");
    layout->addWidget(searchBtn);

    connect(searchBtn, &QPushButton::clicked, this, [&dlg, queryInput, this]() {
        QString q = queryInput->text().trimmed();
        if (q.isEmpty()) return;
        newTab("https://duckduckgo.com/?q=" + q + "+filetype:pdf");
        dlg.accept();
    });
    connect(queryInput, &QLineEdit::returnPressed, searchBtn, &QPushButton::click);

    dlg.exec();
}

void MainWindow::openWeather() {
    QDialog dlg(this);
    dlg.setObjectName("ToolDialog");
    dlg.setFixedWidth(300);
    auto *layout = new QVBoxLayout(&dlg);

    auto *title = new QLabel("\U0001f321 Weather");
    title->setObjectName("Title");
    layout->addWidget(title);

    auto *cityInput = new QLineEdit();
    cityInput->setPlaceholderText("City (e.g. Dhaka)");
    layout->addWidget(cityInput);

    auto *res = new QLabel("Enter city to see weather.");
    res->setWordWrap(true);
    layout->addWidget(res);

    connect(cityInput, &QLineEdit::returnPressed, this, [cityInput, res]() {
        QString city = cityInput->text().trimmed().isEmpty() ? "Dhaka" : cityInput->text().trimmed();
        res->setText("Loading\u2026");
        // Use curl to fetch weather
        QProcess proc;
        proc.start("curl", QStringList() << "-s"
            << QString("https://api.open-meteo.com/v1/forecast?latitude=0&longitude=0&current_weather=true&q=" + city));
        proc.waitForFinished(10000);
        QByteArray data = proc.readAllStandardOutput();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonObject cw = obj["current_weather"].toObject();
        if (!cw.isEmpty()) {
            res->setText(QString("<b>%1</b>: %2\u00b0C\nWind: %3 km/h")
                .arg(city)
                .arg(cw["temperature"].toDouble())
                .arg(cw["windspeed"].toDouble()));
        } else {
            res->setText("Weather data unavailable.");
        }
    });

    dlg.exec();
}

void MainWindow::openTimer() {
    QDialog dlg(this);
    dlg.setWindowTitle("Timer");
    auto *layout = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout();
    auto *hours = new QSpinBox(); hours->setRange(0, 24);
    auto *mins = new QSpinBox(); mins->setRange(0, 59); mins->setValue(5);
    auto *secs = new QSpinBox(); secs->setRange(0, 59);
    form->addRow("Hours:", hours);
    form->addRow("Minutes:", mins);
    form->addRow("Seconds:", secs);
    layout->addLayout(form);

    auto *startBtn = new QPushButton("Start Timer");
    layout->addWidget(startBtn);

    auto *status = new QLabel("");
    layout->addWidget(status);

    connect(startBtn, &QPushButton::clicked, this, [hours, mins, secs, startBtn, status, &dlg]() {
        int total = hours->value() * 3600 + mins->value() * 60 + secs->value();
        if (total <= 0) { status->setText("Set a valid duration."); return; }
        startBtn->setEnabled(false);

        auto *timer = new QTimer(&dlg);
        int remaining = total;
        QObject::connect(timer, &QTimer::timeout, [timer, &remaining, status, startBtn, &dlg]() {
            remaining--;
            if (remaining <= 0) {
                timer->stop();
                status->setText("\u23f0 Time's up!");
                startBtn->setEnabled(true);
                return;
            }
            int h = remaining / 3600;
            int m = (remaining % 3600) / 60;
            int s = remaining % 60;
            status->setText(QString("\u23f1 %1:%2:%3")
                .arg(h, 2, 10, QChar('0'))
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0')));
        });
        timer->start(1000);
        status->setText(QString("Timer set for %1s").arg(total));
    });

    dlg.exec();
}

void MainWindow::openPdfMerge() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF Merger");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *fileList = new QListWidget();
    layout->addWidget(new QLabel("Selected PDFs:"));
    layout->addWidget(fileList);

    auto *btnLayout = new QHBoxLayout();
    auto *addBtn = new QPushButton("Add PDFs");
    auto *removeBtn = new QPushButton("Remove Selected");
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    layout->addLayout(btnLayout);

    auto *mergeBtn = new QPushButton("Merge & Save As\u2026");
    layout->addWidget(mergeBtn);

    connect(addBtn, &QPushButton::clicked, this, [&dlg, fileList]() {
        QStringList files = FilePicker::getOpenFileNames(&dlg, "Select PDFs", "", "PDFs (*.pdf)");
        for (const auto &f : files) fileList->addItem(f);
    });
    connect(removeBtn, &QPushButton::clicked, this, [fileList]() {
        for (auto *item : fileList->selectedItems())
            fileList->takeItem(fileList->row(item));
    });
    connect(mergeBtn, &QPushButton::clicked, this, [&dlg, fileList]() {
        if (fileList->count() < 2) {
            QMessageBox::warning(&dlg, "PDF Merge", "Select at least 2 PDFs.");
            return;
        }
        QString out = FilePicker::getSaveFileName(&dlg, "Save Merged PDF", "", "PDFs (*.pdf)");
        if (out.isEmpty()) return;
        QStringList paths;
        for (int i = 0; i < fileList->count(); i++)
            paths.append(fileList->item(i)->text());
        QString result = PdfTools::mergeDocuments(paths, out);
        QMessageBox::information(&dlg, "PDF Merge", QString("Merged to:\n%1").arg(result));
    });

    dlg.exec();
}

void MainWindow::openPdfSplit() {
    QDialog dlg(this);
    dlg.setWindowTitle("Split PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *fileBtn = new QPushButton("Select PDF to Split");
    layout->addWidget(fileBtn);
    auto *result = new QLabel("");
    layout->addWidget(result);

    connect(fileBtn, &QPushButton::clicked, this, [&dlg, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDFs (*.pdf)");
        if (path.isEmpty()) return;
        QString outDir = FilePicker::getExistingDirectory(&dlg, "Output Directory");
        if (outDir.isEmpty()) return;
        QStringList paths = DocTools::splitPdf(path, outDir);
        result->setText(QString("Created %1 files in:\n%2").arg(paths.size()).arg(outDir));
        QMessageBox::information(&dlg, "Split PDF", QString("Created %1 page files.").arg(paths.size()));
    });

    dlg.exec();
}

void MainWindow::openWordToPdf() {
    QDialog dlg(this);
    dlg.setWindowTitle("Word \u2192 PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *btn = new QPushButton("Select Word (.docx) file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar();
    progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select DOCX", "", "Word (*.docx)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save PDF", "", "PDF (*.pdf)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true);
        result->setText("Converting\u2026 please wait.");
        QString res = DocTools::wordToPdf(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
        QMessageBox::information(&dlg, "Success", QString("PDF saved to:\n%1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openPdfToWord() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF \u2192 Word");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *btn = new QPushButton("Select PDF file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar();
    progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save DOCX", "", "Word (*.docx)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true);
        result->setText("Converting\u2026 please wait.");
        QString res = DocTools::pdfToWord(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
        QMessageBox::information(&dlg, "Success", QString("Word file saved to:\n%1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openImageToPdf() {
    QDialog dlg(this);
    dlg.setWindowTitle("Image \u2192 PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *fileList = new QListWidget();
    layout->addWidget(new QLabel("Selected images:"));
    layout->addWidget(fileList);

    auto *btnLayout = new QHBoxLayout();
    auto *addBtn = new QPushButton("Add Images");
    auto *removeBtn = new QPushButton("Remove");
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    layout->addLayout(btnLayout);

    auto *convertBtn = new QPushButton("Convert to PDF\u2026");
    layout->addWidget(convertBtn);

    connect(addBtn, &QPushButton::clicked, this, [&dlg, fileList]() {
        QStringList files = FilePicker::getOpenFileNames(&dlg, "Select Images", "",
            "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
        for (const auto &f : files) fileList->addItem(f);
    });
    connect(removeBtn, &QPushButton::clicked, this, [fileList]() {
        for (auto *item : fileList->selectedItems())
            fileList->takeItem(fileList->row(item));
    });
    connect(convertBtn, &QPushButton::clicked, this, [&dlg, fileList]() {
        if (fileList->count() == 0) {
            QMessageBox::warning(&dlg, "Image to PDF", "Add at least one image.");
            return;
        }
        QString out = FilePicker::getSaveFileName(&dlg, "Save PDF", "", "PDF (*.pdf)");
        if (out.isEmpty()) return;
        QStringList paths;
        for (int i = 0; i < fileList->count(); i++)
            paths.append(fileList->item(i)->text());
        DocTools::imageToPdf(paths, out);
        QMessageBox::information(&dlg, "Success", QString("PDF saved to:\n%1").arg(out));
    });

    dlg.exec();
}

void MainWindow::openTextToPdf() {
    QDialog dlg(this);
    dlg.setWindowTitle("Text \u2192 PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *textEdit = new QTextEdit();
    textEdit->setPlaceholderText("Enter or paste text here\u2026");
    layout->addWidget(textEdit);

    auto *btn = new QPushButton("Save as PDF");
    layout->addWidget(btn);

    connect(btn, &QPushButton::clicked, this, [&dlg, textEdit]() {
        QString text = textEdit->toPlainText().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(&dlg, "Text to PDF", "Enter some text.");
            return;
        }
        QString out = FilePicker::getSaveFileName(&dlg, "Save PDF", "", "PDF (*.pdf)");
        if (out.isEmpty()) return;
        DocTools::textToPdf(text, out);
        QMessageBox::information(&dlg, "Success", QString("PDF saved to:\n%1").arg(out));
    });

    dlg.exec();
}

void MainWindow::openXlsxToPdf() {
    QDialog dlg(this);
    dlg.setWindowTitle("Excel \u2192 PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select Excel (.xlsx) file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar(); progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select XLSX", "", "Excel (*.xlsx)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save PDF", "", "PDF (*.pdf)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true); result->setText("Converting\u2026");
        QString res = OfficeTools::xlsxToPdf(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openPdfToXlsx() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF \u2192 Excel");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select PDF file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar(); progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save XLSX", "", "Excel (*.xlsx)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true); result->setText("Extracting\u2026");
        QString res = OfficeTools::pdfToXlsx(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openCsvToXlsx() {
    QDialog dlg(this);
    dlg.setWindowTitle("CSV \u2192 Excel");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select CSV file");
    layout->addWidget(btn);
    auto *result = new QLabel("");
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select CSV", "", "CSV (*.csv)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save XLSX", "", "Excel (*.xlsx)");
        if (out.isEmpty()) return;
        OfficeTools::csvToXlsx(path, out);
        QMessageBox::information(&dlg, "Success", QString("Excel saved to:\n%1").arg(out));
    });

    dlg.exec();
}

void MainWindow::openXlsxToCsv() {
    QDialog dlg(this);
    dlg.setWindowTitle("Excel \u2192 CSV");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select Excel (.xlsx) file");
    layout->addWidget(btn);
    auto *result = new QLabel("");
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select XLSX", "", "Excel (*.xlsx)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save CSV", "", "CSV (*.csv)");
        if (out.isEmpty()) return;
        OfficeTools::xlsxToCsv(path, out);
        QMessageBox::information(&dlg, "Success", QString("CSV saved to:\n%1").arg(out));
    });

    dlg.exec();
}

void MainWindow::openPptxToPdf() {
    QDialog dlg(this);
    dlg.setWindowTitle("PowerPoint \u2192 PDF");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select PowerPoint (.pptx) file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar(); progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PPTX", "", "PowerPoint (*.pptx)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save PDF", "", "PDF (*.pdf)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true); result->setText("Converting\u2026");
        QString res = OfficeTools::pptxToPdf(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openPdfToPptx() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF \u2192 PowerPoint");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select PDF file\u2026");
    layout->addWidget(btn);
    auto *progress = new QProgressBar(); progress->setRange(0, 0); progress->setVisible(false);
    layout->addWidget(progress);
    auto *result = new QLabel(""); result->setWordWrap(true);
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, btn, progress, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save PPTX", "", "PowerPoint (*.pptx)");
        if (out.isEmpty()) return;
        btn->setEnabled(false); progress->setVisible(true); result->setText("Converting\u2026");
        QString res = OfficeTools::pdfToPptx(path, out);
        progress->setVisible(false); btn->setEnabled(true);
        result->setText(QString("\u2714 Saved: %1").arg(res));
    });

    dlg.exec();
}

void MainWindow::openPdfToImage() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF \u2192 Image");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select PDF file");
    layout->addWidget(btn);
    auto *result = new QLabel("");
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        QString outDir = FilePicker::getExistingDirectory(&dlg, "Select output folder");
        if (outDir.isEmpty()) return;
        QStringList files = OfficeTools::pdfToImage(path, outDir);
        QMessageBox::information(&dlg, "Success",
            QString("%1 image(s) saved to:\n%2").arg(files.size()).arg(outDir));
    });

    dlg.exec();
}

void MainWindow::openPdfToText() {
    QDialog dlg(this);
    dlg.setWindowTitle("PDF \u2192 Text");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);
    auto *btn = new QPushButton("Select PDF file");
    layout->addWidget(btn);
    auto *result = new QLabel("");
    layout->addWidget(result);

    connect(btn, &QPushButton::clicked, this, [&dlg, result]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select PDF", "", "PDF (*.pdf)");
        if (path.isEmpty()) return;
        QString out = FilePicker::getSaveFileName(&dlg, "Save TXT", "", "Text (*.txt)");
        if (out.isEmpty()) return;
        OfficeTools::pdfToText(path, out);
        QMessageBox::information(&dlg, "Success", QString("Text saved to:\n%1").arg(out));
    });

    dlg.exec();
}

void MainWindow::openArchiveTools() {
    QDialog dlg(this);
    dlg.setWindowTitle("Archive Tools");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *title = new QLabel("\U0001f4e6 Archive Tools (Zip, 7z, Tar)");
    title->setObjectName("Title");
    layout->addWidget(title);

    auto *tabWidget = new QTabWidget();
    layout->addWidget(tabWidget);

    // Create Archive tab
    auto *zipTab = new QWidget();
    auto *zipLayout = new QVBoxLayout(zipTab);
    auto *fileList = new QListWidget();
    zipLayout->addWidget(new QLabel("Files to archive:"));
    zipLayout->addWidget(fileList);

    auto *btnRow = new QHBoxLayout();
    auto *addBtn = new QPushButton("Add Files");
    auto *remBtn = new QPushButton("Remove");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(remBtn);
    zipLayout->addLayout(btnRow);

    auto *fmtCombo = new QComboBox();
    fmtCombo->addItems({"Zip", "7z", "Tar (.tar.gz)"});
    zipLayout->addWidget(new QLabel("Format:"));
    zipLayout->addWidget(fmtCombo);

    auto *goBtn = new QPushButton("Create Archive");
    zipLayout->addWidget(goBtn);
    tabWidget->addTab(zipTab, "Create Archive");

    // Extract tab
    auto *unzipTab = new QWidget();
    auto *unzipLayout = new QVBoxLayout(unzipTab);
    auto *unBtn = new QPushButton("Select Archive to Extract");
    unzipLayout->addWidget(unBtn);
    auto *unRes = new QLabel("");
    unzipLayout->addWidget(unRes);
    tabWidget->addTab(unzipTab, "Extract Archive");

    connect(addBtn, &QPushButton::clicked, this, [&dlg, fileList]() {
        QStringList files = FilePicker::getOpenFileNames(&dlg, "Select Files");
        for (const auto &f : files) fileList->addItem(f);
    });
    connect(remBtn, &QPushButton::clicked, this, [fileList]() {
        for (auto *item : fileList->selectedItems())
            fileList->takeItem(fileList->row(item));
    });
    connect(goBtn, &QPushButton::clicked, this, [&dlg, fileList, fmtCombo]() {
        if (fileList->count() == 0) return;
        QString fmt = fmtCombo->currentText();
        QString ext = fmt.contains("Zip") ? ".zip" : fmt.contains("7z") ? ".7z" : ".tar.gz";
        QString out = FilePicker::getSaveFileName(&dlg, "Save Archive", "archive" + ext);
        if (out.isEmpty()) return;
        QStringList paths;
        for (int i = 0; i < fileList->count(); i++)
            paths.append(fileList->item(i)->text());
        if (fmt.contains("Zip")) ArchiveTools::zipFiles(paths, out);
        else if (fmt.contains("7z")) ArchiveTools::sevenZipFiles(paths, out);
        else ArchiveTools::tarFiles(paths, out);
        QMessageBox::information(&dlg, "Success", QString("Archive created:\n%1").arg(out));
    });
    connect(unBtn, &QPushButton::clicked, this, [&dlg, unRes]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select Archive", "",
            "Archives (*.zip *.7z *.tar.gz *.tgz)");
        if (path.isEmpty()) return;
        QString outDir = FilePicker::getExistingDirectory(&dlg, "Select Extraction Folder");
        if (outDir.isEmpty()) return;
        if (path.endsWith(".zip")) ArchiveTools::unzipFile(path, outDir);
        else if (path.endsWith(".7z")) ArchiveTools::unSevenZipFile(path, outDir);
        else ArchiveTools::untarFile(path, outDir);
        QMessageBox::information(&dlg, "Success", QString("Extracted to:\n%1").arg(outDir));
    });

    dlg.exec();
}

void MainWindow::openQr() {
    QDialog dlg(this);
    dlg.setWindowTitle("QR Code Generator");
    dlg.setMinimumWidth(400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *textInput = new QLineEdit();
    textInput->setPlaceholderText("Enter text or URL\u2026");
    layout->addWidget(textInput);

    auto *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(new QLabel("Size:"));
    auto *sizeSpin = new QSpinBox();
    sizeSpin->setRange(5, 40); sizeSpin->setValue(10);
    sizeLayout->addWidget(sizeSpin);
    sizeLayout->addStretch();
    layout->addLayout(sizeLayout);

    auto *genBtn = new QPushButton("Generate & Save");
    layout->addWidget(genBtn);

    auto *preview = new QLabel("");
    layout->addWidget(preview);

    connect(genBtn, &QPushButton::clicked, this, [&dlg, textInput, sizeSpin, preview]() {
        QString text = textInput->text().trimmed();
        if (text.isEmpty()) {
            QMessageBox::warning(&dlg, "QR Code", "Enter text or URL.");
            return;
        }
        QString out = FilePicker::getSaveFileName(&dlg, "Save QR Code", "qrcode.png",
            "PNG (*.png);;JPEG (*.jpg);;All (*)");
        if (out.isEmpty()) return;
        StudentTools::generateQr(text, out, sizeSpin->value());
        QMessageBox::information(&dlg, "Success", QString("QR saved to:\n%1").arg(out));
    });
    connect(textInput, &QLineEdit::returnPressed, genBtn, &QPushButton::click);

    dlg.exec();
}

void MainWindow::openUnitConverter() {
    QDialog dlg(this);
    dlg.setWindowTitle("Unit Converter");
    dlg.setMinimumWidth(500);
    auto *layout = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout();
    auto *valueInput = new QDoubleSpinBox();
    valueInput->setDecimals(4);
    valueInput->setRange(0.0, 999999999.0);
    valueInput->setValue(1.0);
    form->addRow("Value:", valueInput);

    auto *catCombo = new QComboBox();
    catCombo->addItems({"length", "weight", "temperature", "data", "speed", "area", "volume"});
    form->addRow("Category:", catCombo);

    auto *fromCombo = new QComboBox();
    auto *toCombo = new QComboBox();
    form->addRow("From:", fromCombo);
    form->addRow("To:", toCombo);
    layout->addLayout(form);

    auto *resultLabel = new QLabel("");
    layout->addWidget(resultLabel);

    auto *convertBtn = new QPushButton("Convert");
    layout->addWidget(convertBtn);

    QMap<QString, QStringList> unitsMap = {
        {"length", {"meter","kilometer","centimeter","millimeter","mile","yard","foot","inch"}},
        {"weight", {"kilogram","gram","milligram","pound","ounce","ton"}},
        {"temperature", {"celsius","fahrenheit","kelvin"}},
        {"data", {"byte","kilobyte","megabyte","gigabyte","terabyte"}},
        {"speed", {"m/s","km/h","mph","knot"}},
        {"area", {"sq_meter","sq_kilometer","sq_mile","sq_yard","sq_foot","acre","hectare"}},
        {"volume", {"liter","milliliter","gallon","quart","pint","cup","cubic_meter"}},
    };

    auto updateUnits = [&](const QString &cat) {
        fromCombo->clear();
        toCombo->clear();
        QStringList units = unitsMap.value(cat);
        fromCombo->addItems(units);
        toCombo->addItems(units);
        if (toCombo->count() > 1) toCombo->setCurrentIndex(1);
    };
    connect(catCombo, &QComboBox::currentTextChanged, updateUnits);
    updateUnits(catCombo->currentText());

    connect(convertBtn, &QPushButton::clicked, this, [valueInput, fromCombo, toCombo, catCombo, resultLabel]() {
        double val = valueInput->value();
        QString from = fromCombo->currentText();
        QString to = toCombo->currentText();
        QString cat = catCombo->currentText();
        try {
            double result = StudentTools::convertUnit(val, from, to, cat);
            resultLabel->setText(QString("%1 %2 = %3 %4").arg(val).arg(from).arg(result, 0, 'g', 10).arg(to));
        } catch (const std::exception &e) {
            resultLabel->setText(QString("Error: %1").arg(e.what()));
        }
    });

    dlg.exec();
}

void MainWindow::openCalculator() {
    QDialog dlg(this);
    dlg.setWindowTitle("Calculator");
    dlg.setMinimumWidth(350);
    auto *layout = new QVBoxLayout(&dlg);

    auto *display = new QLineEdit();
    display->setPlaceholderText("Enter expression (e.g. 2+2*5)");
    display->setMinimumHeight(40);
    layout->addWidget(display);

    auto *resultLabel = new QLabel("");
    layout->addWidget(resultLabel);

    auto *grid = new QVBoxLayout();
    QStringList buttons[] = {
        {"7","8","9","/"},
        {"4","5","6","*"},
        {"1","2","3","-"},
        {"0",".","%","+"},
        {"C","="},
    };
    for (const auto &rowBtns : buttons) {
        auto *row = new QHBoxLayout();
        for (const auto &text : rowBtns) {
            auto *btn = new QPushButton(text);
            btn->setMinimumWidth(50);
            connect(btn, &QPushButton::clicked, this, [display, resultLabel, text]() {
                if (text == "C") {
                    display->clear();
                    resultLabel->clear();
                } else if (text == "=") {
                    QString res = StudentTools::calculate(display->text());
                    resultLabel->setText("= " + res);
                } else {
                    display->setText(display->text() + text);
                }
            });
            row->addWidget(btn);
        }
        grid->addLayout(row);
    }
    layout->addLayout(grid);

    connect(display, &QLineEdit::returnPressed, this, [display, resultLabel]() {
        QString res = StudentTools::calculate(display->text());
        resultLabel->setText("= " + res);
    });

    dlg.exec();
}

void MainWindow::openProgrammerCalc() {
    QDialog dlg(this);
    dlg.setWindowTitle("Programmer's Converter (Base)");
    dlg.setMinimumWidth(400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout();
    auto *valInput = new QLineEdit();
    auto *fromBase = new QComboBox();
    fromBase->addItems({"dec", "bin", "hex", "oct"});
    auto *toBase = new QComboBox();
    toBase->addItems({"bin", "hex", "oct", "dec"});

    form->addRow("Value:", valInput);
    form->addRow("From Base:", fromBase);
    form->addRow("To Base:", toBase);
    layout->addLayout(form);

    auto *resultLabel = new QLabel("Result: ");
    resultLabel->setStyleSheet("font-weight: bold; font-size: 14pt; color: #2980b9; margin-top: 10px;");
    layout->addWidget(resultLabel);

    auto doConvert = [valInput, fromBase, toBase, resultLabel]() {
        QString v = valInput->text().trimmed();
        if (v.isEmpty()) { resultLabel->setText("Result: "); return; }
        resultLabel->setText("Result: " + StudentTools::programmerCalc(v, fromBase->currentText(), toBase->currentText()));
    };

    connect(valInput, &QLineEdit::textChanged, doConvert);
    connect(fromBase, &QComboBox::currentIndexChanged, doConvert);
    connect(toBase, &QComboBox::currentIndexChanged, doConvert);

    dlg.exec();
}

void MainWindow::openNoteTaker() {
    QDialog dlg(this);
    dlg.setWindowTitle("Note Taker");
    dlg.setMinimumSize(500, 400);
    auto *layout = new QVBoxLayout(&dlg);

    auto *textEdit = new QTextEdit();
    textEdit->setPlaceholderText("Write your notes here\u2026");
    layout->addWidget(textEdit);

    auto *btnLayout = new QHBoxLayout();
    auto *saveBtn = new QPushButton("\U0001f4be Save");
    auto *clearBtn = new QPushButton("\U0001f5d1 Clear");
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(clearBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    auto *status = new QLabel("");
    layout->addWidget(status);

    connect(saveBtn, &QPushButton::clicked, this, [&dlg, textEdit, status]() {
        QString text = textEdit->toPlainText().trimmed();
        if (text.isEmpty()) { status->setText("Nothing to save."); return; }
        QString out = FilePicker::getSaveFileName(&dlg, "Save Note", "note.txt",
            "Text (*.txt);;All (*)");
        if (out.isEmpty()) return;
        StudentTools::saveNote(text, out);
        status->setText("Saved to " + out);
        QMessageBox::information(&dlg, "Note Saved", QString("Saved to:\n%1").arg(out));
    });
    connect(clearBtn, &QPushButton::clicked, this, [textEdit, status]() {
        textEdit->clear();
        status->setText("Cleared.");
    });

    dlg.exec();
}
