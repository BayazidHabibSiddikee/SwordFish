#include "mainwindow.h"
#include "web_page.h"
#include "styles.h"
#include "folder_picker.h"
#include "file_picker.h"
#include "pip_window.h"
#include "password_manager.h"
#include "extension_system.h"
#include "sync_manager.h"
#include "media_bar.h"
#include "reading_mode.h"

#include <QWebEngineGlobalSettings>
#include "tools/pdf_tools.h"
#include "tools/doc_tools.h"
#include "tools/office_tools.h"
#include "tools/archive_tools.h"
#include "tools/student_tools.h"
#include "tools/translate.h"

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
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

    connect(m_browser, &QWebEngineView::urlChanged, this, &TabWidget::checkPdf);

    // NOTE: do NOT call setUrl here.
    // Callers (newTab) must call setWebChannel on the page first, then loadUrl().
    // Stored so loadUrl() can be called later.
    m_pendingUrl = url;
}

void TabWidget::loadUrl(const QString &url) {
    QString target = url.isEmpty() ? m_pendingUrl : url;
    if (!target.isEmpty()) {
        m_browser->setUrl(QUrl(target));
        checkPdf(QUrl(target));
    }
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

// ── ToolsBackend ──────────────────────────────────────────────────────────

ToolsBackend::ToolsBackend(QObject *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow) {}

void ToolsBackend::run_tool(const QString &name) {
    auto *mw = qobject_cast<MainWindow*>(m_mainWindow);
    if (!mw) return;

    // Map tool name (from tools.html onclick) → MainWindow slot
    if      (name == "pdf_merge")        QMetaObject::invokeMethod(mw, "openPdfMerge");
    else if (name == "pdf_split")        QMetaObject::invokeMethod(mw, "openPdfSplit");
    else if (name == "word_to_pdf")      QMetaObject::invokeMethod(mw, "openWordToPdf");
    else if (name == "pdf_to_word")      QMetaObject::invokeMethod(mw, "openPdfToWord");
    else if (name == "xlsx_to_pdf")      QMetaObject::invokeMethod(mw, "openXlsxToPdf");
    else if (name == "pdf_to_xlsx")      QMetaObject::invokeMethod(mw, "openPdfToXlsx");
    else if (name == "csv_to_xlsx")      QMetaObject::invokeMethod(mw, "openCsvToXlsx");
    else if (name == "xlsx_to_csv")      QMetaObject::invokeMethod(mw, "openXlsxToCsv");
    else if (name == "pptx_to_pdf")      QMetaObject::invokeMethod(mw, "openPptxToPdf");
    else if (name == "pdf_to_pptx")      QMetaObject::invokeMethod(mw, "openPdfToPptx");
    else if (name == "image_to_pdf")     QMetaObject::invokeMethod(mw, "openImageToPdf");
    else if (name == "pdf_to_image")     QMetaObject::invokeMethod(mw, "openPdfToImage");
    else if (name == "text_to_pdf")      QMetaObject::invokeMethod(mw, "openTextToPdf");
    else if (name == "pdf_to_text")      QMetaObject::invokeMethod(mw, "openPdfToText");
    else if (name == "translate")        QMetaObject::invokeMethod(mw, "openTranslate");
    else if (name == "transcript")       QMetaObject::invokeMethod(mw, "openTranscript");
    else if (name == "archive")          QMetaObject::invokeMethod(mw, "openArchiveTools");
    else if (name == "timer")            QMetaObject::invokeMethod(mw, "openTimer");
    else if (name == "qr")               QMetaObject::invokeMethod(mw, "openQr");
    else if (name == "calculator")       QMetaObject::invokeMethod(mw, "openCalculator");
    else if (name == "unit_converter")   QMetaObject::invokeMethod(mw, "openUnitConverter");
    else if (name == "programmer_calc")  QMetaObject::invokeMethod(mw, "openProgrammerCalc");
    else if (name == "weather")          QMetaObject::invokeMethod(mw, "openWeather");
    else if (name == "note")             QMetaObject::invokeMethod(mw, "openNoteTaker");
    else if (name == "web_terminal") {
        // Open a system terminal in a new tab via xterm.js served locally,
        // or simply launch the system terminal and close the card gracefully.
        QProcess::startDetached("bash", QStringList() << "-c"
            << "x-terminal-emulator || gnome-terminal || xterm || konsole");
    }
    else {
        qWarning() << "ToolsBackend::run_tool — unknown tool:" << name;
    }
}

void ToolsBackend::install_extension(const QString &url) {
    auto *mw = qobject_cast<MainWindow*>(m_mainWindow);
    if (!mw || !mw->extensions()) return;

    mw->extensions()->installFromUrl(url, [this](bool success, const QString &msg) {
        emit extensionInstalled(success, msg);
    });
}

// ── Dependency groups ────────────────────────────────────────────────────
// Each group maps to the apt packages needed and the binaries to probe.
struct DepGroup {
    const char *id;
    const char *label;
    const char *description;
    QStringList aptPackages;   // what to install
    QStringList probeCommands; // what to check with `which`
};

static const DepGroup DEP_GROUPS[] = {
    {
        "pdf",
        "PDF Tools (Merge / Split)",
        "Required for merging and splitting PDF files",
        {"qpdf"},
        {"qpdf"}
    },
    {
        "office",
        "Office Converter (Word / Excel / PPTX ↔ PDF)",
        "Required for converting between Office formats and PDF",
        {"libreoffice"},
        {"libreoffice"}
    },
    {
        "poppler",
        "PDF Extraction (PDF → Image / Text)",
        "Required for extracting images and text from PDFs",
        {"poppler-utils"},
        {"pdftotext", "pdftoppm"}
    },
    {
        "text2pdf",
        "Text → PDF (enscript + ghostscript)",
        "Required for converting plain text files to PDF",
        {"enscript", "ghostscript"},
        {"enscript", "gs"}
    },
    {
        "archive",
        "Archive Tools (7z)",
        "Required for creating and extracting 7-zip archives",
        {"p7zip-full"},
        {"7z"}
    },
    {
        "qr",
        "QR Code Generator",
        "Required for generating QR codes",
        {"qrencode"},
        {"qrencode"}
    },
    {
        "ytdlp",
        "Media Downloader + YouTube Transcript (yt-dlp)",
        "Required for downloading videos/audio and fetching YouTube transcripts",
        {"yt-dlp"},
        {"yt-dlp"}
    },
    {
        "python",
        "Translator + Advanced Tools (Python 3)",
        "Required for the Translator, PDF→Text fallback, and unit conversion",
        {"python3", "python3-pip"},
        {"python3"}
    },
};

static bool cmdExists(const QString &cmd) {
    QProcess p;
    p.start("which", QStringList() << cmd);
    p.waitForFinished(3000);
    return p.exitCode() == 0;
}

void ToolsBackend::check_deps() {
    QJsonObject result;
    for (const auto &g : DEP_GROUPS) {
        bool installed = true;
        for (const QString &cmd : g.probeCommands) {
            if (!cmdExists(cmd)) { installed = false; break; }
        }
        QJsonObject entry;
        entry["label"]       = QString::fromUtf8(g.label);
        entry["description"] = QString::fromUtf8(g.description);
        entry["installed"]   = installed;
        result[QString::fromUtf8(g.id)] = entry;
    }
    emit depsStatus(QString::fromUtf8(
        QJsonDocument(result).toJson(QJsonDocument::Compact)));
}

void ToolsBackend::install_deps(const QString &group) {
    // Find the group
    const DepGroup *found = nullptr;
    for (const auto &g : DEP_GROUPS) {
        if (group == QString::fromUtf8(g.id)) { found = &g; break; }
    }
    if (!found) {
        QJsonObject p; p["group"] = group; p["state"] = "failed";
        p["msg"] = "Unknown dependency group: " + group;
        emit installProgress(QString::fromUtf8(
            QJsonDocument(p).toJson(QJsonDocument::Compact)));
        return;
    }

    // Emit "installing" state
    {
        QJsonObject p; p["group"] = group; p["state"] = "installing";
        p["msg"] = QString("Installing: %1…").arg(
            QString::fromUtf8(found->label));
        emit installProgress(QString::fromUtf8(
            QJsonDocument(p).toJson(QJsonDocument::Compact)));
    }

    // Special case: yt-dlp — prefer pip install if apt version is old
    QStringList pkgs = found->aptPackages;
    QString groupId  = group;

    // Build the install command: pkexec runs with GUI auth prompt
    // We use bash -c so we can chain apt-get update + install
    QStringList aptArgs = pkgs;
    aptArgs.prepend("-y");
    aptArgs.prepend("install");
    aptArgs.prepend("apt-get");

    auto *proc = new QProcess(this);
    proc->setProgram("pkexec");
    proc->setArguments(aptArgs);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this, proc, group, found, groupId](int code, QProcess::ExitStatus) {
            proc->deleteLater();
            QJsonObject p;
            p["group"] = group;
            if (code == 0) {
                p["state"] = "done";
                p["msg"]   = QString("%1 installed successfully.")
                                 .arg(QString::fromUtf8(found->label));
            } else {
                QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
                p["state"] = "failed";
                p["msg"]   = err.isEmpty()
                    ? QString("Installation failed (exit %1). "
                              "Try: sudo apt install %2")
                          .arg(code)
                          .arg(found->aptPackages.join(" "))
                    : err;
            }
            emit installProgress(QString::fromUtf8(
                QJsonDocument(p).toJson(QJsonDocument::Compact)));
        });

    proc->start();
}

// ── MainWindow ────────────────────────────────────────────────────────────

MainWindow::MainWindow(bool isPrivate, QWidget *parent)
    : QMainWindow(parent), m_isPrivate(isPrivate)
{
    setWindowTitle(QString("SwordFish Browser") + (isPrivate ? " (Private)" : ""));

    m_settings = new QSettings("SwordFish", "Browser", this);
    m_configDir = configDir();
    m_dataFile = m_configDir + "/data.json";

    setupDns();

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

    // Register the tools backend so tools.html can call backend.run_tool(name)
    m_toolsBackend = new ToolsBackend(this, this);
    m_channel->registerObject("backend", m_toolsBackend);

    // Wire network-level ad blocking to the profile
    m_profile->setUrlRequestInterceptor(&getBlocker());

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

    // ── Feature init ──────────────────────────────────────────────────────
    m_passwords  = new PasswordManager(m_configDir + "/passwords.json", this);
    m_passwords->load();

    m_extensions = new ExtensionSystem(m_configDir + "/extensions", m_profile, this);
    m_extensions->loadAll();

    m_sync = new SyncManager(this);
    connect(m_sync, &SyncManager::syncFileChanged, this, [this](const QString &path) {
        QJsonObject imported = m_sync->importData(path);
        if (!imported.isEmpty()) {
            // Merge silently
            QJsonArray bm = m_data["bookmarks"].toArray();
            QSet<QString> seen;
            for (const auto &v : bm) seen.insert(v.toObject()["url"].toString());
            for (const auto &v : imported["bookmarks"].toArray()) {
                if (!seen.contains(v.toObject()["url"].toString())) bm.append(v);
            }
            m_data["bookmarks"] = bm;
            saveData();
        }
    });

    m_reader = new ReadingMode(this);
    connect(m_reader, &ReadingMode::activated,   this, [this]{ /* could update btn */ });
    connect(m_reader, &ReadingMode::deactivated, this, [this]{ });

    // Password capture: connect to tab URL changes
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) {
        auto *br = currentBrowser();
        if (br && br->page() && m_passwords)
            m_passwords->injectCapture(br->page());
    });
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

void MainWindow::setupDns() {
    // DNS provider templates — empty string = system DNS
    static const QMap<QString, QString> k_providers = {
        { "AdGuard",    "https://dns.adguard-dns.com/dns-query" },
        { "Cloudflare", "https://cloudflare-dns.com/dns-query"  },
        { "NextDNS",    "https://dns.nextdns.io/dns-query"      },
        { "Google",     "https://dns.google/dns-query"          },
        { "System",     ""                                       },
    };

    QString provider = m_settings->value("dns_provider", "AdGuard").toString();
    if (!k_providers.contains(provider)) provider = "AdGuard";

    QString tmpl = k_providers.value(provider);

    QWebEngineGlobalSettings::DnsMode mode;
    if (tmpl.isEmpty()) {
        mode.secureMode     = QWebEngineGlobalSettings::SecureDnsMode::SystemOnly;
        mode.serverTemplates = {};
    } else {
        mode.secureMode      = QWebEngineGlobalSettings::SecureDnsMode::SecureWithFallback;
        mode.serverTemplates = { tmpl };
    }

    QWebEngineGlobalSettings::setDnsMode(mode);
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
    m_navbar = addToolBar("Navigation");
    m_navbar->setMovable(false);
    auto *navbar = m_navbar;

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
            if (w) {
                // Save URL for reopen
                if (w->browser() && !w->browser()->url().isEmpty()
                    && w->browser()->url().toString() != "about:blank") {
                    QJsonArray closed = m_data["closed_tabs"].toArray();
                    closed.append(w->browser()->url().toString());
                    // Keep last 20 closed tabs
                    while (closed.size() > 20) closed.removeFirst();
                    m_data["closed_tabs"] = closed;
                }
                // Must set page to nullptr before destroying the widget
                if (w->browser()) {
                    w->browser()->stop();
                    w->browser()->setPage(nullptr);
                }
                if (w->pdfViewer()) {
                    w->pdfViewer()->stop();
                    w->pdfViewer()->setPage(nullptr);
                }
            }
            m_tabs->removeTab(idx);
            if (w) w->deleteLater();
        } else {
            close();
        }
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) {
        auto *br = currentBrowser();
        if (br) m_urlBar->setText(br->url().toString());
        // Re-attach media bar to the newly active tab if it's visible
        if (m_mediaBar && m_mediaBar->isVisible() && br)
            m_mediaBar->attachTo(br);
    });

    auto *tabBtn = new QPushButton("+");
    tabBtn->setFixedSize(32, 28);
    tabBtn->setCursor(Qt::PointingHandCursor);
    tabBtn->setStyleSheet(m_darkMode
        ? "QPushButton { background-color: #3e4451; color: #abb2bf; font-size: 16px; font-weight: bold; border: none; border-radius: 4px; }"
          "QPushButton:hover { background-color: #4b5263; color: #abb2bf; }"
          "QPushButton:pressed { background-color: #2c313c; }"
        : "QPushButton { background-color: #caf0f8; color: #023e8a; font-size: 16px; font-weight: bold; border: none; border-radius: 4px; }"
          "QPushButton:hover { background-color: #ade8f4; color: #023e8a; }"
          "QPushButton:pressed { background-color: #90e0ef; }");
    connect(tabBtn, &QPushButton::clicked, this, [this]() { newTab(); });
    m_tabs->setCornerWidget(tabBtn);

    // ── Find bar (hidden until Ctrl+F) ──
    m_findBar = new QWidget(this);
    auto *findLayout = new QHBoxLayout(m_findBar);
    findLayout->setContentsMargins(6, 3, 6, 3);
    findLayout->setSpacing(4);
    m_findEdit = new QLineEdit(m_findBar);
    m_findEdit->setPlaceholderText("Find in page…");
    m_findEdit->setMaximumWidth(280);
    m_findStatus = new QLabel("", m_findBar);
    m_findStatus->setMinimumWidth(60);
    auto *findPrevBtn = new QPushButton("▲", m_findBar);
    auto *findNextBtn = new QPushButton("▼", m_findBar);
    auto *findCloseBtn = new QPushButton("✕", m_findBar);
    for (auto *b : {findPrevBtn, findNextBtn, findCloseBtn}) {
        b->setFixedSize(26, 26);
    }
    findLayout->addWidget(new QLabel("Find:"));
    findLayout->addWidget(m_findEdit);
    findLayout->addWidget(m_findStatus);
    findLayout->addWidget(findPrevBtn);
    findLayout->addWidget(findNextBtn);
    findLayout->addStretch();
    findLayout->addWidget(findCloseBtn);
    m_findBar->setVisible(false);

    // Wire find bar
    connect(m_findEdit, &QLineEdit::textChanged, this, &MainWindow::findNext);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    connect(findNextBtn,  &QPushButton::clicked, this, &MainWindow::findNext);
    connect(findPrevBtn,  &QPushButton::clicked, this, &MainWindow::findPrev);
    connect(findCloseBtn, &QPushButton::clicked, this, &MainWindow::closeFindBar);

    // ── Central widget: tabs + find bar + media bar stacked ──
    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_tabs);
    centralLayout->addWidget(m_findBar);

    // Media bar (hidden by default)
    m_mediaBar = new MediaBar(central);
    m_mediaBar->setVisible(false);
    centralLayout->addWidget(m_mediaBar);
    connect(m_mediaBar, &MediaBar::pipRequested, this, &MainWindow::openPip);

    setCentralWidget(central);

    // ── Tab bar context menu ──
    m_tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabs->tabBar(), &QTabBar::customContextMenuRequested,
            this, &MainWindow::showTabContextMenu);

    setupShortcuts();

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

TabWidget *MainWindow::currentTabWidget() {
    return qobject_cast<TabWidget*>(m_tabs->currentWidget());
}

TabWidget *MainWindow::newTab(const QString &url) {
    auto *tw = new TabWidget(url.isEmpty() ? m_home : url, m_profile);
    int idx = m_tabs->addTab(tw, "New Tab");
    m_tabs->setCurrentIndex(idx);
    auto *br = tw->browser();
    if (br->page()) {
        // Set WebChannel BEFORE loading the URL — required for qrc:///tools.html
        // so qt.webChannelTransport is available when the page's JS runs.
        br->page()->setWebChannel(m_channel);
    }
    tw->loadUrl(); // Now safe to navigate — channel is already attached
    connect(br, &QWebEngineView::titleChanged, this, [this, tw, br](const QString &t) {
        updateTabTitle(tw, br, t);
    });
    connect(br, &QWebEngineView::urlChanged, this, &MainWindow::recordHistory);
    // Block YouTube Shorts — redirect to YouTube homepage
    connect(br, &QWebEngineView::urlChanged, this, [br](const QUrl &url) {
        QString s = url.toString();
        if (s.contains("youtube.com/shorts", Qt::CaseInsensitive) ||
            s.contains("youtube.com/reels", Qt::CaseInsensitive)) {
            br->setUrl(QUrl("https://www.youtube.com"));
        }
    });
    // Inject password capture on every navigation in this tab
    connect(br, &QWebEngineView::loadFinished, this, [this, br](bool) {
        if (br && br->page() && m_passwords)
            m_passwords->injectCapture(br->page());
    });
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
    int opened = 0;
    for (const auto &v : tabs) {
        QString url = v.toString();
        // Skip stale file:// URLs — they point to old installation paths that
        // may no longer exist (e.g. old Python version's tools.html).
        if (url.startsWith("file://")) continue;
        newTab(url);
        ++opened;
    }
    if (opened == 0) {
        newTab(m_home);
    } else {
        int active = m_data["active_tab"].toInt(0);
        if (active < m_tabs->count()) m_tabs->setCurrentIndex(active);
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
    // Block YouTube Shorts
    if (url.contains("youtube.com/shorts", Qt::CaseInsensitive) ||
        url.contains("youtube.com/reels", Qt::CaseInsensitive)) {
        url = "https://www.youtube.com";
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
            ? "QPushButton { background-color: #3e4451; color: #abb2bf; font-size: 16px; font-weight: bold; border: none; border-radius: 4px; }"
              "QPushButton:hover { background-color: #4b5263; color: #abb2bf; }"
              "QPushButton:pressed { background-color: #2c313c; }"
            : "QPushButton { background-color: #caf0f8; color: #023e8a; font-size: 16px; font-weight: bold; border: none; border-radius: 4px; }"
              "QPushButton:hover { background-color: #ade8f4; color: #023e8a; }"
              "QPushButton:pressed { background-color: #90e0ef; }");
    }

    // Update theme toggle button
    if (m_themeBtn) {
        m_themeBtn->setText(m_darkMode ? "☀ Light" : "🌙 Dark");
        m_themeBtn->setToolTip(m_darkMode ? "Switch to Light mode" : "Switch to Dark mode");
        m_themeBtn->setStyleSheet(m_darkMode
            ? "QPushButton { background-color: #3e4451; color: #abb2bf;"
              "  border: none; border-radius: 4px;"
              "  padding: 0 12px; font-size: 13px; font-weight: bold; }"
              "QPushButton:hover { background-color: #4b5263; color: #abb2bf; }"
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
    // Read actual current level from the singleton
    auto lvl = getBlocker().level();
    QString currentLevel =
        lvl == AdBlocker::Level::None     ? "none"     :
        lvl == AdBlocker::Level::Low      ? "low"      :
        lvl == AdBlocker::Level::Medium   ? "medium"   : "ultimate";
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
            m_settings->setValue("adblock_level", level);
            QMessageBox::information(this, "Adblock Level",
                                     QString("Set to %1").arg(level));
        });
        blockMenu->addAction(a);
    }

    menu.addSeparator();

    // ── DNS over HTTPS ──
    auto *dnsMenu = menu.addMenu("\U0001f512  DNS over HTTPS");
    struct DnsEntry { QString label; QString key; };
    const QList<DnsEntry> dnsProviders = {
        { "AdGuard (default)",  "AdGuard"    },
        { "Cloudflare",         "Cloudflare" },
        { "NextDNS",            "NextDNS"    },
        { "Google",             "Google"     },
        { "System (no DoH)",    "System"     },
    };
    QString currentDns = m_settings->value("dns_provider", "AdGuard").toString();
    for (const auto &entry : dnsProviders) {
        auto *a = new QAction(entry.label, this);
        a->setCheckable(true);
        a->setChecked(entry.key == currentDns);
        connect(a, &QAction::triggered, this, [this, entry]() {
            m_settings->setValue("dns_provider", entry.key);
            setupDns();
            QMessageBox::information(this, "DNS",
                QString("DNS set to: %1\nTakes effect for new connections.").arg(entry.label));
        });
        dnsMenu->addAction(a);
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
    QString script = R"JS(
(function() {
    'use strict';
    if (!document || !document.documentElement) return;

    const AD_SELECTORS = [
        // Generic ad classes
        '.adsbygoogle', '.adsense', '.advertisement', '.ads-container',
        '.ad-container', '.ad-wrap', '.ad-placeholder', '.ad-unit',
        '.ad-banner', '.ad-slot', '.ad-box', '.ad-label', '.ad-wrapper',
        '[class*="sponsor"]', '[class*="promoted"]',
        // Generic ad IDs
        '#ad-sidebar', '#ad-banner', '#ad-container', '#ad-wrap', '#ads',
        // Ad iframes
        'iframe[src*="doubleclick"]', 'iframe[src*="googlead"]',
        'iframe[src*="adservice"]', 'iframe[src*="googlesyndication"]',
        'ins.adsbygoogle',
        // YouTube specific
        '.video-ads', '.ytp-ad-module', '.ytd-ad-slot-renderer',
        'ytd-ad-slot-renderer', 'ytd-in-feed-ad-layout-renderer',
        'ytd-banner-promo-renderer', 'ytd-statement-banner-renderer',
        '#masthead-ad', '#player-ads', '.ytd-display-ad-renderer',
        'ytd-promoted-sparkles-web-renderer', 'ytd-promoted-video-renderer',
        'ytd-compact-promoted-video-renderer',
        // YouTube Shorts — hide all Shorts UI everywhere
        'ytd-rich-shelf-renderer[is-shorts]',
        'ytd-reel-shelf-renderer',
        'ytd-reel-video-renderer',
        'ytd-reel-item-renderer',
        'ytd-shorts',
        '#shorts-container',
        '#shorts-inner-container',
        'a[title="Shorts"]',
        'a[href="/shorts"]',
        'ytd-guide-entry-renderer a[href="/shorts"]',
        'ytd-mini-guide-entry-renderer a[href="/shorts"]',
        '[title="Shorts"]',
    ];

    const AD_URL_KEYWORDS = [
        'doubleclick.net', 'googleadservices', 'googlesyndication',
        'adservice.google', 'adnxs.com', 'adzerk', 'moatads',
        'scorecardresearch', 'outbrain', 'taboola', 'pagead',
        'viewthroughconversion', 'generate_204',
    ];

    function removeAds() {
        AD_SELECTORS.forEach(sel => {
            try {
                document.querySelectorAll(sel).forEach(el => {
                    el.remove();
                });
            } catch (_) {}
        });
        // Remove ad images/scripts by URL
        document.querySelectorAll('img[src], script[src]').forEach(el => {
            const src = el.src || '';
            if (src && !src.startsWith('data:') &&
                AD_URL_KEYWORDS.some(k => src.includes(k))) {
                el.remove();
            }
        });
        // Skip-ad button: auto-click
        const skipBtn = document.querySelector(
            '.ytp-skip-ad-button, .ytp-ad-skip-button, [class*="skip-ad"]');
        if (skipBtn) skipBtn.click();
        // Mute/fast-forward through unskippable ads
        const adVideo = document.querySelector('.ad-showing video');
        if (adVideo && !adVideo.muted) {
            adVideo.muted = true;
            adVideo.playbackRate = 16;
        }
    }

    removeAds();

    // Re-run on DOM changes (YouTube is a SPA)
    let pending = false;
    new MutationObserver(() => {
        if (pending) return;
        pending = true;
        setTimeout(() => { pending = false; removeAds(); }, 250);
    }).observe(document.documentElement, { childList: true, subtree: true });
})();
)JS";

    auto *webScript = new QWebEngineScript();
    webScript->setName("adblock");
    webScript->setSourceCode(script);
    webScript->setInjectionPoint(QWebEngineScript::DocumentReady);
    webScript->setWorldId(QWebEngineScript::MainWorld);
    webScript->setRunsOnSubFrames(false);  // main frame only — subframes are sandboxed
    m_profile->scripts()->insert(*webScript);
}

void MainWindow::injectDarkMode() {
    removeDarkMode();  // clean slate

    // ── Strategy ──────────────────────────────────────────────────────────
    // We inject TWO scripts:
    //  1. A <style> with One Dark CSS applied to all pages. On YouTube we rely
    //     on color-scheme:dark + targeted ytd-* selectors.
    //  2. A JS fixer that runs ONLY on YouTube to keep the stylesheet in place
    //     across SPA navigations.
    // NOTE: YouTube Shorts is fully blocked at the navigation level, so we no
    //       longer need any Shorts-specific guards here.
    // ──────────────────────────────────────────────────────────────────────

    // ── 1. CSS ────────────────────────────────────────────────────────────
    QString css = R"CSS(
/* SwordFish Dark Mode — One Dark palette
   bg:#282c34  bg2:#21252b  surface:#2c313c  border:#3e4451
   text:#abb2bf  accent:#61afef  green:#98c379  amber:#e5c07b  red:#e06c75 */

/* ── color-scheme hint ── */
:root { color-scheme: dark !important; }

/* ══════════════════════════════════════════════════════════════
   NON-YOUTUBE PAGES — broad rules safe here
   ══════════════════════════════════════════════════════════════ */
:not(ytd-app):not(ytd-app *) html,
html:not(.ytd-app) {
    background-color: #282c34;
    color: #abb2bf;
}

/* Fallback: applies everywhere but YouTube overrides with its own theme */
html { background-color: #282c34 !important; color: #abb2bf !important; }
body { background-color: #282c34 !important; color: #abb2bf !important; }

/* Text — NOTE: no span/div — too broad, breaks layout overlays */
p, h1, h2, h3, h4, h5, h6,
li, dt, dd, caption, figcaption,
label, legend, summary, blockquote, cite, q {
    color: #abb2bf !important;
}

a         { color: #61afef !important; }
a:visited { color: #c678dd !important; }
a:hover   { color: #528bff !important; }
h1, h2, h3 { color: #e5c07b !important; }
h4, h5, h6 { color: #61afef !important; }

small, time { color: #5c6370 !important; }

:not(ytd-app) header, :not(ytd-app) nav, :not(ytd-app) footer,
:not(ytd-app) aside, :not(ytd-app) main, :not(ytd-app) section, :not(ytd-app) article {
    background-color: #282c34 !important;
    color: #abb2bf !important;
}

[class*="card"], [class*="panel"], [class*="box"],
[class*="modal"], [class*="dialog"], [class*="popup"] {
    background-color: #2c313c !important;
    color: #abb2bf !important;
}

input:not([type="submit"]):not([type="button"]):not([type="reset"])
    :not([type="checkbox"]):not([type="radio"]),
textarea, select {
    background-color: #21252b !important;
    color: #abb2bf !important;
    border: 1px solid #3e4451 !important;
}
input::placeholder, textarea::placeholder { color: #5c6370 !important; }

/* Buttons — scoped to NON-YouTube pages only */
:not(ytd-app) button, :not(ytd-app) [type="button"],
:not(ytd-app) [type="submit"], :not(ytd-app) [role="button"] {
    background-color: #3e4451 !important;
    color: #abb2bf !important;
}

table { border-color: #3e4451 !important; }
th    { color: #e5c07b !important; background-color: #21252b !important; }
td    { color: #abb2bf !important; }
tr:nth-child(even) { background-color: #2c313c !important; }

code, pre, kbd { background-color: #21252b !important; color: #98c379 !important; }

/* ══════════════════════════════════════════════════════════════
   YOUTUBE — targeted selectors for dark theme
   ══════════════════════════════════════════════════════════════ */

/* App shell */
ytd-app, ytd-browse, ytd-search, ytd-watch-flexy,
#page-manager, ytd-two-column-browse-results-renderer {
    background-color: #282c34 !important; color: #abb2bf !important;
}

/* Masthead */
#masthead, ytd-masthead, #masthead-container {
    background-color: #21252b !important; color: #abb2bf !important;
    border-bottom: 1px solid #3e4451 !important;
}

/* Guide / sidebar */
ytd-guide-renderer, ytd-mini-guide-renderer,
ytd-guide-entry-renderer, ytd-guide-section-renderer {
    background-color: #21252b !important; color: #abb2bf !important;
}
ytd-guide-entry-renderer:hover,
ytd-guide-entry-renderer[active] { background-color: #3e4451 !important; }

/* Watch page */
ytd-video-primary-info-renderer, ytd-video-secondary-info-renderer,
ytd-watch-metadata { background-color: #282c34 !important; }
#video-title { color: #abb2bf !important; }
ytd-channel-name, ytd-channel-name a { color: #61afef !important; }

/* Description */
#description, ytd-expander, ytd-text-inline-expander {
    background-color: #2c313c !important; color: #abb2bf !important;
}

/* Comments */
ytd-comments, ytd-comment-thread-renderer,
ytd-comment-renderer, ytd-comment-view-model {
    background-color: #282c34 !important; color: #abb2bf !important;
}
#author-text, #author-text a { color: #61afef !important; }
#content-text { color: #abb2bf !important; }

/* Related / sidebar */
#secondary, ytd-compact-video-renderer { background-color: #282c34 !important; }

/* Video grid */
ytd-rich-grid-renderer, ytd-rich-item-renderer,
ytd-grid-renderer, ytd-item-section-renderer {
    background-color: #282c34 !important;
}
a#video-title, ytd-rich-grid-media #video-title { color: #abb2bf !important; }

/* Search */
ytd-search, ytd-video-renderer, ytd-channel-renderer {
    background-color: #282c34 !important; color: #abb2bf !important;
}

/* Dropdowns / menus */
ytd-popup-container, tp-yt-paper-listbox, tp-yt-paper-item {
    background-color: #21252b !important; color: #abb2bf !important;
}

/* Buttons on YouTube (subscribe etc.) */
ytd-subscribe-button-renderer button, ytd-button-renderer button {
    background-color: #3e4451 !important; color: #abb2bf !important;
}

/* ── VIDEO PLAYER — never touch ── */
#movie_player, .html5-video-player, .html5-video-container,
.ytp-chrome-bottom, .ytp-chrome-top,
ytd-player, [id*="player"], video, video * {
    background-color: transparent !important;
    background: transparent !important;
    color: inherit !important;
    filter: none !important;
    opacity: 1 !important;
}

/* ── Media / visuals ── */
video, img, canvas, picture, iframe, embed, object {
    filter: none !important; opacity: 1 !important;
}

/* ── Scrollbars ── */
::-webkit-scrollbar { width: 8px; height: 8px; }
::-webkit-scrollbar-track { background: #282c34; }
::-webkit-scrollbar-thumb { background: #3e4451; border-radius: 4px; }
::-webkit-scrollbar-thumb:hover { background: #4b5263; }
)CSS";

    // ── 2. JS Script ──────────────────────────────────────────────────────
    QString script = QString(R"JS(
(function() {
    if (!document || !document.documentElement) return;
    const STYLE_ID = '__sf_darkmode__';
    if (document.getElementById(STYLE_ID)) return;

    // Immediate bg flash prevention
    document.documentElement.style.setProperty('--sf-flash-bg', '#282c34');
    document.documentElement.style.backgroundColor = '#282c34';

    // Inject stylesheet
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = %1;
    (document.head || document.documentElement).appendChild(style);

    // ── YouTube-only dark mode helper ─────────────────────────────────
    // Applies targeted dark styles to YouTube elements via the stylesheet above.
    if (!location.hostname.includes('youtube.com')) return;

    // Map of element → original inline style string before we touched it
    const __sfPatched = new WeakMap();

    function patchEl(el) {
        if (__sfPatched.has(el)) return;
        __sfPatched.set(el, el.getAttribute('style') || '');
        // Nothing to set — YouTube has its own dark theme.
        // We only ensure our global body/html rules don't bleed in.
        // The stylesheet handles ytd-* selectors above.
    }

    // Override body bg only — do NOT touch any inline styles of yt elements
    // The CSS stylesheet above handles everything via ytd-* selectors.
    // We just ensure the stylesheet stays in place on SPA navigation.
    let raf = null;
    new MutationObserver(() => {
        if (raf) return;
        raf = requestAnimationFrame(() => {
            raf = null;
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
    s->setRunsOnSubFrames(false);
    m_profile->scripts()->insert(*s);

    // Apply to already-open tabs
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *tw = qobject_cast<TabWidget*>(m_tabs->widget(i));
        if (tw && tw->browser())
            tw->browser()->page()->runJavaScript(script);
    }
}

void MainWindow::removeDarkMode() {
    // 1. Remove profile script so new tabs don't get it
    auto scripts = m_profile->scripts()->toList();
    for (const auto &s : scripts) {
        if (s.name() == "sf_darkmode") {
            m_profile->scripts()->remove(s);
            break;
        }
    }

    // 2. Remove injected <style> and reset inline styles on all open tabs
    const QString removeScript = R"JS(
(function() {
    // Remove our style tag
    const el = document.getElementById('__sf_darkmode__');
    if (el) el.remove();

    // Reset the flash-prevention inline styles we set on documentElement
    document.documentElement.style.backgroundColor = '';
    document.documentElement.style.color = '';
    document.documentElement.style.removeProperty('--sf-flash-bg');

    // On YouTube: nothing else to undo — we never set inline styles
    // on ytd-* elements. The stylesheet removal above is sufficient.
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
    // tools.html is compiled into the binary as a Qt resource (qrc://).
    // Using qrc:// instead of file:// is required for QWebChannel to work —
    // Qt WebEngine injects qt.webChannelTransport only on non-file:// origins.
    newTab("qrc:///tools.html");
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

// Helper: run a tool function, catch std::exception and show a warning dialog.
// Usage: runTool(parentWidget, [&]{ /* tool call */ });
// Returns true on success.
static bool runTool(QWidget *parent, std::function<void()> fn) {
    try {
        fn();
        return true;
    } catch (const std::exception &e) {
        QMessageBox::warning(parent, "Tool Error",
            QString("<b>Tool failed:</b><br>%1<br><br>"
                    "Make sure the required program is installed.")
                .arg(QString::fromStdString(e.what())));
        return false;
    } catch (...) {
        QMessageBox::warning(parent, "Tool Error", "An unknown error occurred.");
        return false;
    }
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
        QString result;
        if (runTool(&dlg, [&]{ result = PdfTools::mergeDocuments(paths, out); }))
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
        QStringList paths;
        if (!runTool(&dlg, [&]{ paths = DocTools::splitPdf(path, outDir); })) return;
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = DocTools::wordToPdf(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) { result->setText(QString("\u2714 Saved: %1").arg(res));
            QMessageBox::information(&dlg, "Success", QString("PDF saved to:\n%1").arg(res)); }
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = DocTools::pdfToWord(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) { result->setText(QString("\u2714 Saved: %1").arg(res));
            QMessageBox::information(&dlg, "Success", QString("Word file saved to:\n%1").arg(res)); }
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
        if (runTool(&dlg, [&]{ DocTools::imageToPdf(paths, out); }))
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
        if (runTool(&dlg, [&]{ DocTools::textToPdf(text, out); }))
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = OfficeTools::xlsxToPdf(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) result->setText(QString("\u2714 Saved: %1").arg(res));
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = OfficeTools::pdfToXlsx(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) result->setText(QString("\u2714 Saved: %1").arg(res));
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
        if (runTool(&dlg, [&]{ OfficeTools::csvToXlsx(path, out); }))
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
        if (runTool(&dlg, [&]{ OfficeTools::xlsxToCsv(path, out); }))
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = OfficeTools::pptxToPdf(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) result->setText(QString("\u2714 Saved: %1").arg(res));
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
        QString res;
        bool ok = runTool(&dlg, [&]{ res = OfficeTools::pdfToPptx(path, out); });
        progress->setVisible(false); btn->setEnabled(true);
        if (ok) result->setText(QString("\u2714 Saved: %1").arg(res));
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
        QStringList files;
        if (runTool(&dlg, [&]{ files = OfficeTools::pdfToImage(path, outDir); }))
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
        if (runTool(&dlg, [&]{ OfficeTools::pdfToText(path, out); }))
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
        bool ok = runTool(&dlg, [&]{
            if (fmt.contains("Zip")) ArchiveTools::zipFiles(paths, out);
            else if (fmt.contains("7z")) ArchiveTools::sevenZipFiles(paths, out);
            else ArchiveTools::tarFiles(paths, out);
        });
        if (ok) QMessageBox::information(&dlg, "Success", QString("Archive created:\n%1").arg(out));
    });
    connect(unBtn, &QPushButton::clicked, this, [&dlg, unRes]() {
        QString path = FilePicker::getOpenFileName(&dlg, "Select Archive", "",
            "Archives (*.zip *.7z *.tar.gz *.tgz)");
        if (path.isEmpty()) return;
        QString outDir = FilePicker::getExistingDirectory(&dlg, "Select Extraction Folder");
        if (outDir.isEmpty()) return;
        bool ok = runTool(&dlg, [&]{
            if (path.endsWith(".zip")) ArchiveTools::unzipFile(path, outDir);
            else if (path.endsWith(".7z")) ArchiveTools::unSevenZipFile(path, outDir);
            else ArchiveTools::untarFile(path, outDir);
        });
        if (ok) QMessageBox::information(&dlg, "Success", QString("Extracted to:\n%1").arg(outDir));
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
        if (runTool(&dlg, [&]{ StudentTools::generateQr(text, out, sizeSpin->value()); }))
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
        if (runTool(&dlg, [&]{ StudentTools::saveNote(text, out); })) {
            status->setText("Saved to " + out);
            QMessageBox::information(&dlg, "Note Saved", QString("Saved to:\n%1").arg(out));
        }
    });
    connect(clearBtn, &QPushButton::clicked, this, [textEdit, status]() {
        textEdit->clear();
        status->setText("Cleared.");
    });

    dlg.exec();
}

// ── Keyboard Shortcuts ────────────────────────────────────────────────────

void MainWindow::setupShortcuts() {
    // Shortcuts that map directly to void() slots
    struct SC { QKeySequence key; void(MainWindow::*slot)(); };
    const SC shortcuts[] = {
        { QKeySequence("F11"),              &MainWindow::toggleFullscreen    },
        { QKeySequence("F5"),               &MainWindow::reload              },
        { QKeySequence("Ctrl+W"),           &MainWindow::closeCurrentTab     },
        { QKeySequence("Ctrl+L"),           &MainWindow::focusUrlBar         },
        { QKeySequence("Ctrl+F"),           &MainWindow::openFindBar         },
        { QKeySequence("Ctrl+R"),           &MainWindow::reload              },
        { QKeySequence("Ctrl+H"),           &MainWindow::showBookmarksMenu   },
        { QKeySequence("Ctrl+Shift+N"),     &MainWindow::openPrivateWindow   },
        { QKeySequence("Ctrl+Shift+T"),     &MainWindow::reopenLastTab       },
        { QKeySequence("Ctrl+D"),           &MainWindow::bookmarkCurrentPage },
        { QKeySequence("Ctrl+Plus"),        &MainWindow::zoomIn              },
        { QKeySequence("Ctrl+Equal"),       &MainWindow::zoomIn              },
        { QKeySequence("Ctrl+Minus"),       &MainWindow::zoomOut             },
        { QKeySequence("Ctrl+0"),           &MainWindow::zoomReset           },
        { QKeySequence("Ctrl+S"),           &MainWindow::savePage            },
        { QKeySequence("Ctrl+U"),           &MainWindow::viewSource          },
        { QKeySequence("Ctrl+P"),           &MainWindow::printPage           },
        { QKeySequence("Alt+Left"),         &MainWindow::back                },
        { QKeySequence("Alt+Right"),        &MainWindow::forward             },
        { QKeySequence("Ctrl+Tab"),         &MainWindow::nextTab             },
        { QKeySequence("Ctrl+Shift+Tab"),   &MainWindow::prevTab             },
    };
    for (auto &sc : shortcuts) {
        auto *a = new QAction(this);
        a->setShortcut(sc.key);
        a->setShortcutContext(Qt::WindowShortcut);
        connect(a, &QAction::triggered, this, sc.slot);
        addAction(a);
    }
    // Ctrl+T — newTab has a default param so use lambda
    auto *newTabAction = new QAction(this);
    newTabAction->setShortcut(QKeySequence("Ctrl+T"));
    newTabAction->setShortcutContext(Qt::WindowShortcut);
    connect(newTabAction, &QAction::triggered, this, [this]() { newTab(); });
    addAction(newTabAction);

    // New feature shortcuts
    struct SC2 { QKeySequence key; void(MainWindow::*slot)(); };
    const SC2 extras[] = {
        { QKeySequence("Ctrl+Shift+P"), &MainWindow::openPip            },
        { QKeySequence("Ctrl+Shift+K"), &MainWindow::openPasswordManager},
        { QKeySequence("Ctrl+Shift+A"), &MainWindow::autofillPassword   },
        { QKeySequence("Ctrl+Shift+E"), &MainWindow::openExtensions     },
        { QKeySequence("Ctrl+Shift+S"), &MainWindow::openSync           },
        { QKeySequence("Ctrl+Shift+M"), &MainWindow::toggleMediaBar     },
        { QKeySequence("Ctrl+Shift+R"), &MainWindow::toggleReadingMode  },
    };
    for (auto &sc : extras) {
        auto *a = new QAction(this);
        a->setShortcut(sc.key);
        a->setShortcutContext(Qt::WindowShortcut);
        connect(a, &QAction::triggered, this, sc.slot);
        addAction(a);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_findBar && m_findBar->isVisible()) {
            closeFindBar();
            return;
        }
        if (m_fullscreen) {
            toggleFullscreen();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

// ── Fullscreen ────────────────────────────────────────────────────────────

void MainWindow::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;
    if (m_fullscreen) {
        if (m_navbar) m_navbar->hide();
        showFullScreen();
    } else {
        if (m_navbar) m_navbar->show();
        showNormal();
        // Restore maximized if it was maximized before
        if (m_settings->value("maximized", true).toBool())
            showMaximized();
    }
}

// ── Find in page ──────────────────────────────────────────────────────────

void MainWindow::openFindBar() {
    if (!m_findBar) return;
    m_findBar->setVisible(true);
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void MainWindow::closeFindBar() {
    if (!m_findBar) return;
    m_findBar->setVisible(false);
    auto *br = currentBrowser();
    if (br) br->findText(QString());  // clear highlight
    if (m_findStatus) m_findStatus->setText("");
}

void MainWindow::findNext() {
    auto *br = currentBrowser();
    if (!br || !m_findEdit) return;
    QString term = m_findEdit->text();
    if (term.isEmpty()) { if (m_findStatus) m_findStatus->setText(""); return; }
    br->findText(term, {}, [this](const QWebEngineFindTextResult &r) {
        if (m_findStatus) {
            if (r.numberOfMatches() == 0)
                m_findStatus->setText("No results");
            else
                m_findStatus->setText(QString("%1/%2")
                    .arg(r.activeMatch()).arg(r.numberOfMatches()));
        }
    });
}

void MainWindow::findPrev() {
    auto *br = currentBrowser();
    if (!br || !m_findEdit) return;
    QString term = m_findEdit->text();
    if (term.isEmpty()) return;
    br->findText(term, QWebEnginePage::FindBackward, [this](const QWebEngineFindTextResult &r) {
        if (m_findStatus) {
            if (r.numberOfMatches() == 0)
                m_findStatus->setText("No results");
            else
                m_findStatus->setText(QString("%1/%2")
                    .arg(r.activeMatch()).arg(r.numberOfMatches()));
        }
    });
}

// ── Zoom ──────────────────────────────────────────────────────────────────

QString MainWindow::hostOf(const QUrl &url) const {
    return url.host().toLower();
}

void MainWindow::applyZoom(QWebEngineView *br, const QString &host) {
    double factor = m_zoomLevels.value(host, 1.0);
    br->setZoomFactor(factor);
}

void MainWindow::zoomIn() {
    auto *br = currentBrowser();
    if (!br) return;
    QString host = hostOf(br->url());
    double f = qMin(m_zoomLevels.value(host, 1.0) + 0.1, 5.0);
    m_zoomLevels[host] = f;
    br->setZoomFactor(f);
}

void MainWindow::zoomOut() {
    auto *br = currentBrowser();
    if (!br) return;
    QString host = hostOf(br->url());
    double f = qMax(m_zoomLevels.value(host, 1.0) - 0.1, 0.25);
    m_zoomLevels[host] = f;
    br->setZoomFactor(f);
}

void MainWindow::zoomReset() {
    auto *br = currentBrowser();
    if (!br) return;
    QString host = hostOf(br->url());
    m_zoomLevels[host] = 1.0;
    br->setZoomFactor(1.0);
}

// ── URL bar focus ─────────────────────────────────────────────────────────

void MainWindow::focusUrlBar() {
    if (m_urlBar) {
        m_urlBar->setFocus();
        m_urlBar->selectAll();
    }
}

// ── Tab helpers ───────────────────────────────────────────────────────────

void MainWindow::closeCurrentTab() {
    int idx = m_tabs->currentIndex();
    if (m_tabs->count() > 1) {
        m_tabs->tabCloseRequested(idx);  // reuse the existing close logic
    } else {
        close();
    }
}

void MainWindow::nextTab() {
    int next = (m_tabs->currentIndex() + 1) % m_tabs->count();
    m_tabs->setCurrentIndex(next);
}

void MainWindow::prevTab() {
    int prev = (m_tabs->currentIndex() - 1 + m_tabs->count()) % m_tabs->count();
    m_tabs->setCurrentIndex(prev);
}

void MainWindow::reopenLastTab() {
    if (!m_data.contains("closed_tabs")) return;
    QJsonArray closed = m_data["closed_tabs"].toArray();
    if (closed.isEmpty()) return;
    QString url = closed.last().toString();
    closed.removeLast();
    m_data["closed_tabs"] = closed;
    newTab(url);
}

void MainWindow::bookmarkCurrentPage() {
    auto *br = currentBrowser();
    if (!br) return;
    QString url = br->url().toString();
    QString title = br->title();
    if (url.isEmpty() || url == "about:blank") return;
    QJsonArray bm = m_data["bookmarks"].toArray();
    for (const auto &v : bm)
        if (v.toObject()["url"].toString() == url) return; // already bookmarked
    QJsonObject entry;
    entry["url"]   = url;
    entry["title"] = title.isEmpty() ? url : title;
    bm.append(entry);
    m_data["bookmarks"] = bm;
    saveData();
}

// ── Tab features ──────────────────────────────────────────────────────────

void MainWindow::duplicateTab() {
    auto *br = currentBrowser();
    if (br) newTab(br->url().toString());
}

void MainWindow::pinTab() {
    auto *tw = currentTabWidget();
    if (!tw) return;
    tw->isPinned = !tw->isPinned;
    int idx = m_tabs->indexOf(tw);
    QString title = m_tabs->tabText(idx);
    if (tw->isPinned) {
        if (!title.startsWith("📌")) m_tabs->setTabText(idx, "📌 " + title);
    } else {
        if (title.startsWith("📌 ")) m_tabs->setTabText(idx, title.mid(3));
    }
}

void MainWindow::muteTab() {
    auto *tw = currentTabWidget();
    if (!tw || !tw->browser() || !tw->browser()->page()) return;
    tw->isMuted = !tw->isMuted;
    tw->browser()->page()->setAudioMuted(tw->isMuted);
    int idx = m_tabs->indexOf(tw);
    QString title = m_tabs->tabText(idx);
    if (tw->isMuted) {
        if (!title.startsWith("🔇")) m_tabs->setTabText(idx, "🔇 " + title);
    } else {
        if (title.startsWith("🔇 ")) m_tabs->setTabText(idx, title.mid(3));
    }
}

void MainWindow::detachTab() {
    int idx = m_tabs->currentIndex();
    if (m_tabs->count() <= 1) return;
    auto *tw = qobject_cast<TabWidget*>(m_tabs->widget(idx));
    if (!tw) return;
    QString url = tw->browser() ? tw->browser()->url().toString() : m_home;
    // Close here, open in new window
    if (tw->browser()) { tw->browser()->stop(); tw->browser()->setPage(nullptr); }
    if (tw->pdfViewer()) { tw->pdfViewer()->stop(); tw->pdfViewer()->setPage(nullptr); }
    m_tabs->removeTab(idx);
    tw->deleteLater();
    auto *w = new MainWindow(m_isPrivate);
    w->newTab(url);
    w->show();
}

void MainWindow::moveTabLeft() {
    int idx = m_tabs->currentIndex();
    if (idx > 0) m_tabs->tabBar()->moveTab(idx, idx - 1);
}

void MainWindow::moveTabRight() {
    int idx = m_tabs->currentIndex();
    if (idx < m_tabs->count() - 1) m_tabs->tabBar()->moveTab(idx, idx + 1);
}

void MainWindow::showTabContextMenu(const QPoint &pos) {
    int idx = m_tabs->tabBar()->tabAt(pos);
    if (idx < 0) return;
    m_tabs->setCurrentIndex(idx);

    QMenu menu(this);
    auto *tw = qobject_cast<TabWidget*>(m_tabs->widget(idx));

    menu.addAction("New Tab",         this, [this]() { newTab(); });
    menu.addAction("Duplicate Tab",   this, &MainWindow::duplicateTab);
    menu.addSeparator();

    auto *pinA = menu.addAction(tw && tw->isPinned ? "Unpin Tab" : "Pin Tab");
    connect(pinA, &QAction::triggered, this, &MainWindow::pinTab);

    auto *muteA = menu.addAction(tw && tw->isMuted ? "Unmute Tab" : "Mute Tab");
    connect(muteA, &QAction::triggered, this, &MainWindow::muteTab);

    menu.addSeparator();
    menu.addAction("Move Left",  this, &MainWindow::moveTabLeft);
    menu.addAction("Move Right", this, &MainWindow::moveTabRight);
    menu.addSeparator();
    menu.addAction("Detach to Window", this, &MainWindow::detachTab);
    menu.addSeparator();

    auto *closeA = menu.addAction("Close Tab");
    connect(closeA, &QAction::triggered, this, [this, idx]() {
        emit m_tabs->tabCloseRequested(idx);
    });
    auto *closeOthers = menu.addAction("Close Other Tabs");
    connect(closeOthers, &QAction::triggered, this, [this, idx]() {
        for (int i = m_tabs->count() - 1; i >= 0; --i)
            if (i != idx) emit m_tabs->tabCloseRequested(i);
    });
    menu.addAction("Reopen Closed Tab", this, &MainWindow::reopenLastTab);

    menu.exec(m_tabs->tabBar()->mapToGlobal(pos));
}

// ── Page actions ──────────────────────────────────────────────────────────

void MainWindow::savePage() {
    auto *br = currentBrowser();
    if (!br) return;
    QString suggested = QFileInfo(br->url().path()).fileName();
    if (suggested.isEmpty()) suggested = "page";
    if (!suggested.contains('.')) suggested += ".html";
    QString path = FilePicker::getSaveFileName(this, "Save Page",
        downloadDir() + "/" + suggested,
        "HTML (*.html *.htm);;All (*)");
    if (path.isEmpty()) return;
    br->page()->save(path);
}

void MainWindow::viewSource() {
    auto *br = currentBrowser();
    if (!br) return;
    QString url = "view-source:" + br->url().toString();
    newTab(url);
}

void MainWindow::printPage() {
    auto *br = currentBrowser();
    if (!br) return;
    QString path = FilePicker::getSaveFileName(this, "Print to PDF",
        downloadDir() + "/page.pdf", "PDF (*.pdf)");
    if (path.isEmpty()) return;
    br->page()->printToPdf(path);
    QMessageBox::information(this, "Print", QString("PDF saved to:\n%1").arg(path));
}

void MainWindow::copyPageUrl() {
    auto *br = currentBrowser();
    if (!br) return;
    QApplication::clipboard()->setText(br->url().toString());
}

void MainWindow::showPageInfo() {
    auto *br = currentBrowser();
    if (!br) return;
    QUrl url = br->url();
    QString info = QString(
        "URL:      %1\n"
        "Title:    %2\n"
        "Host:     %3\n"
        "Scheme:   %4\n"
        "Zoom:     %5%"
    ).arg(url.toString())
     .arg(br->title())
     .arg(url.host())
     .arg(url.scheme())
     .arg(qRound(br->zoomFactor() * 100));
    QMessageBox::information(this, "Page Info", info);
}

// ── Picture-in-Picture ────────────────────────────────────────────────────

void MainWindow::openPip() {
    auto *br = currentBrowser();
    if (!br) return;
    if (m_pip) {
        m_pip->close();
        m_pip->deleteLater();
        m_pip = nullptr;
        return;
    }
    m_pip = new PipWindow(br, nullptr);  // no parent — independent window
    connect(m_pip, &QObject::destroyed, this, [this]() { m_pip = nullptr; });
    m_pip->setAttribute(Qt::WA_DeleteOnClose);
    m_pip->show();
}

// ── Password Manager ──────────────────────────────────────────────────────

void MainWindow::openPasswordManager() {
    if (m_passwords) m_passwords->showManagerDialog(this);
}

void MainWindow::autofillPassword() {
    auto *br = currentBrowser();
    if (!br || !m_passwords) return;
    QString host = br->url().host();
    auto creds = m_passwords->credentialsForHost(host);
    if (creds.isEmpty()) {
        // Prompt to save
        bool ok;
        QString user = QInputDialog::getText(this, "Save Password",
            "Username for " + host + ":", QLineEdit::Normal, "", &ok);
        if (!ok || user.isEmpty()) return;
        QString pass = QInputDialog::getText(this, "Save Password",
            "Password:", QLineEdit::Password, "", &ok);
        if (!ok) return;
        m_passwords->addCredential({host, user, pass, br->url().toString()});
        QMessageBox::information(this, "Password Saved",
            QString("Saved credentials for %1").arg(host));
    } else {
        m_passwords->autofill(br->page(), host);
    }
}

// ── Extensions ───────────────────────────────────────────────────────────

void MainWindow::openExtensions() {
    if (m_extensions) m_extensions->showManagerDialog(this);
}

// ── Sync ──────────────────────────────────────────────────────────────────

void MainWindow::openSync() {
    if (m_sync) m_sync->showSyncDialog(m_data, this);
}

// ── Media Controls ────────────────────────────────────────────────────────

void MainWindow::toggleMediaBar() {
    if (!m_mediaBar) return;
    bool show = !m_mediaBar->isVisible();
    m_mediaBar->setVisible(show);
    if (show) {
        auto *br = currentBrowser();
        if (br) m_mediaBar->attachTo(br);
    } else {
        m_mediaBar->detach();
    }
}

// ── Reading Mode ──────────────────────────────────────────────────────────

void MainWindow::toggleReadingMode() {
    auto *br = currentBrowser();
    if (!br || !m_reader) return;

    if (!m_reader->isActive()) {
        // Activate — connect titleChanged to detect the in-page exit button
        m_readerTitleConn = connect(br, &QWebEngineView::titleChanged,
            this, [this, br](const QString &title) {
                if (title == "__sf_exit_reader__" && m_reader && m_reader->isActive()) {
                    m_reader->toggle(br->page());
                    QObject::disconnect(m_readerTitleConn);
                }
            });
    } else {
        QObject::disconnect(m_readerTitleConn);
    }
    m_reader->toggle(br->page());
}
