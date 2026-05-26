import sys
import os
import re
import subprocess
import json
import threading
import urllib.parse

os.environ["LANG"]     = "en_US.UTF-8"
os.environ["LANGUAGE"] = "en_US"
os.environ["LC_ALL"]   = "en_US.UTF-8"

def get_platform():
    if sys.platform.startswith("win"):  return "windows"
    if "TERMUX_VERSION" in os.environ:  return "android"
    if sys.platform == "darwin":        return "mac"
    return "linux"

OS = get_platform()
os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = "--logging-level=3"

from PySide6.QtCore    import QUrl, QSettings, QLocale, QSize, QPoint, QTimer, Slot, QObject
from PySide6.QtWidgets import (QApplication, QMainWindow, QToolBar, QLineEdit,
                               QMenu, QInputDialog, QMessageBox, QTabWidget,
                               QWidget, QVBoxLayout, QDialog, QLabel,
                               QPushButton, QTextEdit, QFormLayout,
                               QComboBox, QSpinBox, QHBoxLayout, QProgressBar,
                                QFileDialog, QListWidget, QListWidgetItem,
                                QFrame, QSplitter)
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtWebEngineCore    import (QWebEngineScript, QWebEngineProfile,
                                         QWebEnginePage, QWebEngineUrlRequestInterceptor)
from PySide6.QtWebChannel import QWebChannel
from PySide6.QtGui   import QAction, QCursor, QIcon

if hasattr(sys, '_MEIPASS'):
    ROOT = sys._MEIPASS
else:
    ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

sys.path.insert(0, ROOT)

from utils.adblocker import get_blocker
from src.styles import apply_theme

def config_dir():
    if OS == "windows":
        base = os.environ.get("APPDATA", os.path.expanduser("~"))
    elif OS == "android":
        base = os.path.expanduser("~")
    else:
        base = os.path.join(os.path.expanduser("~"), ".config")
    path = os.path.join(base, "SwordFish")
    os.makedirs(path, exist_ok=True)
    return path

CONFIG_DIR  = config_dir()
DATA_FILE   = os.path.join(CONFIG_DIR, "data.json")
PROFILE_DIR = os.path.join(CONFIG_DIR, "browser_profile")

def load_data():
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {"bookmarks": [], "history": []}

def save_data(data):
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def get_download_dir():
    s = QSettings("SwordFish", "Browser")
    default = (
        os.path.join(os.environ.get("USERPROFILE",""), "Downloads") if OS == "windows"
        else "/sdcard/Download" if OS == "android"
        else os.path.expanduser("~/Downloads")
    )
    return s.value("download_dir", default)

def run_detached(cmd):
    if OS == "windows":
        subprocess.Popen(cmd, creationflags=0x00000008, close_fds=True)
    else:
        subprocess.Popen(cmd, start_new_session=True)


_NAV_PATTERN = re.compile(
    r"^(https?://|ftp://|file://|about:|chrome-extension://)", re.IGNORECASE
)
_IP_PATTERN = re.compile(
    r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}(:\d+)?$"
)
_HOST_PORT_PATTERN = re.compile(
    r"^[\w.-]+:\d+(/.*)?$"
)
_HOSTNAME_PATTERN = re.compile(
    r"^[a-zA-Z][\w-]*$"
)
_SEARCH_URL = "https://www.google.com/search?q="


class BlockInterceptor(QWebEngineUrlRequestInterceptor):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.blocker = get_blocker()

    def interceptRequest(self, info):
        url = info.requestUrl().toString()
        source = info.firstPartyUrl().toString()
        if self.blocker.should_block(url, source):
            info.block(True)


class Backend(QObject):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window

    @Slot(str)
    def run_tool(self, name):
        if name == "translate": self.main_window._open_translate()
        elif name == "transcript": self.main_window._open_transcript()
        elif name == "search": self.main_window._open_search()
        elif name == "weather": self.main_window._open_weather()
        elif name == "pdf_merge": self.main_window._open_pdf_merge()
        elif name == "pdf_split": self.main_window._open_pdf_split()
        elif name == "word_to_pdf": self.main_window._open_word_to_pdf()
        elif name == "pdf_to_word": self.main_window._open_pdf_to_word()
        elif name == "xlsx_to_pdf": self.main_window._open_xlsx_to_pdf()
        elif name == "pdf_to_xlsx": self.main_window._open_pdf_to_xlsx()
        elif name == "csv_to_xlsx": self.main_window._open_csv_to_xlsx()
        elif name == "xlsx_to_csv": self.main_window._open_xlsx_to_csv()
        elif name == "pptx_to_pdf": self.main_window._open_pptx_to_pdf()
        elif name == "pdf_to_pptx": self.main_window._open_pdf_to_pptx()
        elif name == "image_to_pdf": self.main_window._open_image_to_pdf()
        elif name == "pdf_to_image": self.main_window._open_pdf_to_image()
        elif name == "text_to_pdf": self.main_window._open_text_to_pdf()
        elif name == "pdf_to_text": self.main_window._open_pdf_to_text()
        elif name == "archive": self.main_window._open_archive_tools()
        elif name == "timer": self.main_window._open_timer()
        elif name == "qr": self.main_window._open_qr()
        elif name == "calculator": self.main_window._open_calculator()
        elif name == "note": self.main_window._open_note_taker()

class TabWidget(QWidget):
    def __init__(self, url=None, profile=None):
        super().__init__()
        self._layout = QVBoxLayout(self)
        self._layout.setContentsMargins(0, 0, 0, 0)

        self.splitter = QSplitter(self)
        self._layout.addWidget(self.splitter)

        self.browser = QWebEngineView()
        self.splitter.addWidget(self.browser)

        self.pdf_viewer = QWebEngineView()
        self.pdf_viewer.setVisible(False)
        self.splitter.addWidget(self.pdf_viewer)

        if profile:
            page = QWebEnginePage(profile, self)
            page.newWindowRequested.connect(self._on_new_window)
            self.browser.setPage(page)
            
            pdf_page = QWebEnginePage(profile, self)
            self.pdf_viewer.setPage(pdf_page)

        if url:
            self.browser.setUrl(QUrl(url))
            self._check_pdf(QUrl(url))
        
        self.browser.urlChanged.connect(self._check_pdf)

    def _check_pdf(self, qurl):
        url = qurl.toString().lower()
        if url.endswith(".pdf") or (url.startswith("file://") and url.endswith(".pdf")):
            self.pdf_viewer.setUrl(qurl)
            self.pdf_viewer.setVisible(True)
            self.splitter.setSizes([self.width() // 2, self.width() // 2])
        else:
            self.pdf_viewer.setVisible(False)

    def _on_new_window(self, request):
        from PySide6.QtWebEngineCore import QWebEngineNewWindowRequest
        url = request.requestedUrl().toString()
        if not url:
            url = request.url().toString()
        main = self.window()
        if url and hasattr(main, '_new_tab'):
            tw = main._new_tab(url)
            if tw and tw.browser:
                request.openIn(tw.browser)


class Main(QMainWindow):
    def __init__(self, is_private=False):
        super().__init__()
        self.is_private = is_private
        self.setWindowTitle("SwordFish Browser" + (" (Private)" if is_private else ""))

        self.settings = QSettings("SwordFish", "Browser")
        self.home = self.settings.value("home_url", "https://duckduckgo.com")

        if is_private:
            self.profile = QWebEngineProfile(self) # Off-the-record by default if no name
        else:
            self.profile = QWebEngineProfile("SwordFish", self)
            self.profile.setPersistentStoragePath(PROFILE_DIR)
            self.profile.setCachePath(os.path.join(PROFILE_DIR, "cache"))
            self.profile.setPersistentCookiesPolicy(
                QWebEngineProfile.AllowPersistentCookies
            )
        
        self.profile.setHttpAcceptLanguage("en-US,en;q=0.9")

        self.backend = Backend(self)
        self.channel = QWebChannel(self)
        self.channel.registerObject("backend", self.backend)

        self.interceptor = BlockInterceptor(self)
        self.profile.setUrlRequestInterceptor(self.interceptor)

        if not is_private:
            self.data = load_data()
            self._seen_urls = {e["url"] for e in self.data.get("history", [])}
            self._auto_save_timer = QTimer(self)
            self._auto_save_timer.timeout.connect(lambda: save_data(self.data))
            self._auto_save_timer.start(30000)
        else:
            self.data = {"bookmarks": [], "history": [], "tabs": []}
            self._seen_urls = set()

        self._build_ui()
        self._restore_window()
        self._inject_adblock()

    def _build_ui(self):
        navbar = QToolBar("Navigation")
        navbar.setMovable(False)
        self.addToolBar(navbar)

        # Reverting to original symbols as requested
        for label, slot in [
            ("◀", self._back),
            ("▶", self._forward),
            ("↻", self._reload),
            ("⌂", self.navigate_home),
        ]:
            btn = QAction(label, self)
            btn.triggered.connect(slot)
            navbar.addAction(btn)

        self.url_bar = QLineEdit()
        self.url_bar.setPlaceholderText("Search with Google or enter address")
        self.url_bar.returnPressed.connect(self.navigate_to_url)
        navbar.addWidget(self.url_bar)

        bm_btn = QAction("☆ Bookmark", self)
        bm_btn.triggered.connect(self.show_bookmarks_menu)
        navbar.addAction(bm_btn)

        dl_btn = QAction("⬇ Download", self)
        dl_btn.triggered.connect(self.show_download_menu)
        navbar.addAction(dl_btn)

        tools_btn = QAction("🔧 Tools", self)
        tools_btn.triggered.connect(self.show_tools_menu)
        navbar.addAction(tools_btn)

        cfg_btn = QAction("⚙", self)
        cfg_btn.triggered.connect(self.show_settings_menu)
        navbar.addAction(cfg_btn)

        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.tabCloseRequested.connect(self._close_tab)
        self.tabs.currentChanged.connect(self._on_tab_changed)
        self.tabs.setDocumentMode(True)

        tab_btn = QPushButton("TAB")
        tab_btn.setFixedSize(50, 28)
        tab_btn.setStyleSheet("background-color: transparent; color: #023e8a; font-size: 13px; font-weight: bold; border: none;")
        tab_btn.clicked.connect(lambda: self._new_tab())
        self.tabs.setCornerWidget(tab_btn)

        self.setCentralWidget(self.tabs)

        if not self.is_private:
            self._restore_tabs()
        else:
            self._new_tab(self.home)


    def current_browser(self):
        w = self.tabs.currentWidget()
        return w.browser if w else None


    def _update_tab_title(self, tw, br, title):
        idx = self.tabs.indexOf(tw)
        if idx >= 0:
            short = title[:20] + "…" if len(title) > 20 else title
            self.tabs.setTabText(idx, short or "Tab")
            self.tabs.setTabToolTip(idx, title)

    def _close_tab(self, idx):
        if self.tabs.count() > 1:
            w = self.tabs.widget(idx)
            if w and w.browser:
                try:
                    w.browser.titleChanged.disconnect()
                except TypeError:
                    pass
            self.tabs.removeTab(idx)
            w.deleteLater()
        else:
            self.close()

    def _on_tab_changed(self, idx):
        br = self.current_browser()
        if br:
            self.url_bar.setText(br.url().toString())

    def _on_url_changed(self, qurl):
        br = self.current_browser()
        if br and br.url().toString() == qurl.toString():
            self.url_bar.setText(qurl.toString())

    def _back(self):
        br = self.current_browser()
        if br: br.back()

    def _forward(self):
        br = self.current_browser()
        if br: br.forward()

    def _reload(self):
        br = self.current_browser()
        if br: br.reload()

    def navigate_home(self):
        br = self.current_browser()
        if br: br.setUrl(QUrl(self.home))

    def navigate_to_url(self):
        br = self.current_browser()
        if not br: return
        raw = self.url_bar.text().strip()
        if not raw: return

        if _NAV_PATTERN.match(raw):
            url = raw
        elif "." in raw and " " not in raw:
            url = "http://" + raw
        elif _IP_PATTERN.match(raw):
            url = "http://" + raw
        elif _HOST_PORT_PATTERN.match(raw):
            url = "http://" + raw
        elif _HOSTNAME_PATTERN.match(raw):
            url = "http://" + raw
        else:
            url = _SEARCH_URL + raw.replace(" ", "+")
        br.setUrl(QUrl(url))

    def _record_history(self, qurl):
        if self.is_private:
            return
        url = qurl.toString()
        if url in self._seen_urls:
            return
        self._seen_urls.add(url)
        title = self.current_browser().title() if self.current_browser() else url
        self.data["history"].append({"url": url, "title": title})
        self.data["history"] = self.data["history"][-200:]

    def _restore_window(self):
        geo_size = self.settings.value("window_size")
        geo_pos  = self.settings.value("window_pos")
        maximized = self.settings.value("maximized", True, type=bool)
        if geo_size:
            self.resize(geo_size)
        if geo_pos:
            self.move(geo_pos)
        if maximized:
            self.showMaximized()
        else:
            self.show()

    def closeEvent(self, event):
        if self.is_private:
            super().closeEvent(event)
            return
        
        self._auto_save_timer.stop()
        self.settings.setValue("window_size",  self.size())
        self.settings.setValue("window_pos",   self.pos())
        self.settings.setValue("maximized",    self.isMaximized())
        self.settings.setValue("home_url",     self.home)
        self.data["tabs"] = []
        for i in range(self.tabs.count()):
            w = self.tabs.widget(i)
            if w and w.browser:
                url = w.browser.url().toString()
                if url:
                    self.data["tabs"].append(url)
        self.data["active_tab"] = self.tabs.currentIndex()
        save_data(self.data)
        super().closeEvent(event)

    def _restore_tabs(self):
        saved = self.data.get("tabs", [])
        if saved:
            for url in saved:
                self._new_tab(url)
            active = self.data.get("active_tab", 0)
            if active < self.tabs.count():
                self.tabs.setCurrentIndex(active)
        else:
            self._new_tab(self.home)

    def _new_tab(self, url=None):
        """Create a new tab, optionally loading *url*.

        Returns the :class:`TabWidget` instance so the caller can attach
        signal handlers (title updates, URL tracking, history recording).
        """
        tw = TabWidget(url or self.home, self.profile)
        idx = self.tabs.addTab(tw, "New Tab")
        self.tabs.setCurrentIndex(idx)
        br = tw.browser
        if br.page():
            br.page().setWebChannel(self.channel)
        # Update the tab title when the page title changes
        br.titleChanged.connect(lambda t, tw=tw, br=br: self._update_tab_title(tw, br, t))
        # Track URL changes for the address bar and history
        br.urlChanged.connect(self._on_url_changed)
        br.urlChanged.connect(self._record_history)
        return tw
    def show_bookmarks_menu(self):
        menu = QMenu(self)

        add = QAction("➕  Bookmark this page", self)
        add.triggered.connect(self._add_bookmark)
        menu.addAction(add)

        history_menu = menu.addMenu("🕓  History")
        for entry in reversed(self.data["history"][-20:]):
            a = QAction(entry["title"][:60], self)
            a.triggered.connect(lambda checked, u=entry["url"]: self._open_in_tab(u))
            history_menu.addAction(a)

        if self.data["bookmarks"]:
            menu.addSeparator()
            for bm in self.data["bookmarks"]:
                a = QAction("🔖 " + bm["title"][:50], self)
                a.triggered.connect(lambda checked, u=bm["url"]: self._open_in_tab(u))
                menu.addAction(a)

        menu.exec(QCursor.pos())

    def _open_in_tab(self, url):
        self._new_tab(url)

    def _add_bookmark(self):
        br = self.current_browser()
        if not br: return
        url   = br.url().toString()
        title = br.title() or url
        if any(b["url"] == url for b in self.data["bookmarks"]):
            QMessageBox.information(self, "Bookmark", "Already bookmarked!")
            return
        self.data["bookmarks"].append({"url": url, "title": title})
        save_data(self.data)
        QMessageBox.information(self, "Bookmark", f"Saved:\n{title}")

    def show_settings_menu(self):
        menu = QMenu(self)

        new_private = QAction("🕵️  New Private Window", self)
        new_private.triggered.connect(self._open_private_window)
        menu.addAction(new_private)

        menu.addSeparator()

        set_home = QAction("🏠  Set current page as Home", self)
        set_home.triggered.connect(self._set_home)
        menu.addAction(set_home)

        set_dl = QAction("📁  Change download folder", self)
        set_dl.triggered.connect(self._change_download_dir)
        menu.addAction(set_dl)

        clear_hist = QAction("🗑  Clear history", self)
        clear_hist.triggered.connect(self._clear_history)
        if self.is_private: clear_hist.setEnabled(False)
        menu.addAction(clear_hist)

        clear_bm = QAction("🗑  Clear bookmarks", self)
        clear_bm.triggered.connect(self._clear_bookmarks)
        if self.is_private: clear_bm.setEnabled(False)
        menu.addAction(clear_bm)

        menu.addSeparator()
        block_menu = menu.addMenu("🛡  Adblock Level")
        current_level = get_blocker()._level
        for level in ["none", "low", "medium", "ultimate"]:
            label = "Disabled (Off)" if level == "none" else level.title()
            a = QAction(label, self)
            a.setCheckable(True)
            a.setChecked(level == current_level)
            a.triggered.connect(lambda checked, lvl=level: self._set_block_level(lvl))
            block_menu.addAction(a)

        menu.addSeparator()
        about = QAction(f"ℹ  Config: {CONFIG_DIR}", self)
        about.setEnabled(False)
        menu.addAction(about)

        menu.exec(QCursor.pos())

    def _open_private_window(self):
        self.new_window = Main(is_private=True)
        self.new_window.show()

    def _set_block_level(self, level):
        get_blocker().set_level(level)
        QMessageBox.information(self, "Adblock Level", f"Set to {level.title()}")

    def _set_home(self):
        br = self.current_browser()
        if br:
            self.home = br.url().toString()
            self.settings.setValue("home_url", self.home)
            QMessageBox.information(self, "Home", f"Home set to:\n{self.home}")

    def _change_download_dir(self):
        current = get_download_dir()
        new_dir, ok = QInputDialog.getText(
            self, "Download Folder", "Enter path:", text=current
        )
        if ok and new_dir.strip():
            self.settings.setValue("download_dir", new_dir.strip())
            QMessageBox.information(self, "Download Folder", f"Saved:\n{new_dir.strip()}")

    def _clear_history(self):
        self.data["history"] = []
        self._seen_urls.clear()
        save_data(self.data)
        QMessageBox.information(self, "History", "History cleared.")

    def _clear_bookmarks(self):
        self.data["bookmarks"] = []
        save_data(self.data)
        QMessageBox.information(self, "Bookmarks", "Bookmarks cleared.")

    def show_download_menu(self):
        VIDEO_FORMATS = [
            ("144p",         "bestvideo[height<=144]+bestaudio/best[height<=144]"),
            ("360p",         "bestvideo[height<=360]+bestaudio/best[height<=360]"),
            ("480p",         "bestvideo[height<=480]+bestaudio/best[height<=480]"),
            ("720p  (HD)",   "bestvideo[height<=720]+bestaudio/best[height<=720]"),
            ("1080p (FHD)",  "bestvideo[height<=1080]+bestaudio/best[height<=1080]"),
            ("4K    (best)", "bestvideo+bestaudio/best"),
        ]
        AUDIO_FORMATS = [
            ("MP3  (128k)", {"format": "bestaudio", "extract_audio": True, "audio_format": "mp3", "audio_quality": "128K"}),
            ("MP3  (320k)", {"format": "bestaudio", "extract_audio": True, "audio_format": "mp3", "audio_quality": "0"}),
            ("M4A  (best)", {"format": "bestaudio[ext=m4a]/bestaudio", "extract_audio": True, "audio_format": "m4a"}),
            ("OGG  (best)", {"format": "bestaudio", "extract_audio": True, "audio_format": "vorbis"}),
        ]

        menu = QMenu(self)
        video_menu = menu.addMenu("🎬  Video")
        for label, fmt in VIDEO_FORMATS:
            a = QAction(label, self)
            a.triggered.connect(lambda checked, f=fmt: self._download("video", f))
            video_menu.addAction(a)

        audio_menu = menu.addMenu("🎵  Audio only")
        for label, opts in AUDIO_FORMATS:
            a = QAction(label, self)
            a.triggered.connect(lambda checked, o=opts: self._download("audio", o))
            audio_menu.addAction(a)

        menu.exec(QCursor.pos())

    def _download(self, mode, fmt):
        br = self.current_browser()
        if not br: return
        url = br.url().toString()
        dl_dir = get_download_dir()
        os.makedirs(dl_dir, exist_ok=True)

        dlg = QDialog(self)
        dlg.setWindowTitle("Downloading…")
        layout = QVBoxLayout(dlg)
        status = QLabel(f"Downloading from:\n{url[:80]}")
        layout.addWidget(status)
        progress = QProgressBar()
        progress.setRange(0, 0)
        layout.addWidget(progress)
        log_box = QTextEdit()
        log_box.setReadOnly(True)
        log_box.setMaximumHeight(120)
        layout.addWidget(log_box)
        close_btn = QPushButton("Close")
        close_btn.setEnabled(False)
        layout.addWidget(close_btn)

        def progress_hook(d):
            if d.get("status") == "downloading":
                p = d.get("_percent_str", "").strip()
                s = d.get("_speed_str", "").strip()
                msg = f"Downloading {p} at {s}"
                log_box.append(msg)
            elif d.get("status") == "finished":
                f = d.get("filename", "")
                log_box.append(f"Finished: {f}")
                progress.setRange(0, 100)
                progress.setValue(100)
                status.setText(f"Saved to:\n{f}")
                close_btn.setEnabled(True)

        def do_download():
            try:
                import yt_dlp
                ydl_opts = {
                    "outtmpl": os.path.join(dl_dir, "%(title)s.%(ext)s"),
                    "progress_hooks": [progress_hook],
                    "quiet": True,
                    "no_warnings": True,
                }
                if mode == "video":
                    ydl_opts["format"] = fmt
                else:
                    ydl_opts["format"] = fmt["format"]
                    ydl_opts["extractaudio"] = True
                    if fmt.get("audio_format"):
                        ydl_opts["audioformat"] = fmt["audio_format"]
                    if fmt.get("audio_quality"):
                        ydl_opts["audioquality"] = fmt["audio_quality"]

                with yt_dlp.YoutubeDL(ydl_opts) as ydl:
                    ydl.download([url])
            except Exception as e:
                log_box.append(f"Error: {e}")
                status.setText(f"Download failed: {e}")
                close_btn.setEnabled(True)

        close_btn.clicked.connect(dlg.accept)

        from PySide6.QtCore import QThread
        class DownloadThread(QThread):
            def run(self):
                do_download()

        t = DownloadThread()
        t.finished.connect(lambda: close_btn.setEnabled(True))
        t.start()

        dlg.exec()

    def _inject_adblock(self):
        script = QWebEngineScript()
        script.setName("adblock")
        script.setSourceCode(r"""
(function() {
    'use strict';

    const AD_SELECTORS = [
        '.adsbygoogle', '.adsense', '.advertisement',
        '.ad-container', '.ad-wrap', '.ad-placeholder', '.ad-unit',
        '.ad-banner', '.ad-slot', '.ad-box',
        '#ad-sidebar', '#ad-banner', '#ad-container', '#ad-wrap',
        '#ad-box', '#ad-slot', '#ad-unit', '#ad-holder',
        'iframe[src*="doubleclick"]', 'iframe[src*="googlead"]',
        'iframe[src*="ad."]', 'iframe[src*="ad-"]',
        'ins.adsbygoogle',
        '.video-ads', '.ytp-ad-module', '.ytd-ad-slot-renderer',
        '.ytp-ad-image-overlay', '.ytp-ad-overlay-container',
        '.ytp-ad-text-overlay', '.ytp-ad-player-overlay',
        '#masthead-ad', '#player-ads', '#merchandise-shelf',
    ];

    const AD_KEYWORDS = ['doubleclick', 'googlead', 'adservice',
                         'adserver', 'adnxs', 'adzerk',
                         'scorecardresearch', 'quantserve'];

    function removeElements() {
        AD_SELECTORS.forEach(sel => {
            document.querySelectorAll(sel).forEach(el => el.remove());
        });
    }

    function removeAdImages() {
        document.querySelectorAll('img').forEach(el => {
            if (el.src && !el.src.startsWith('data:') &&
                AD_KEYWORDS.some(k => el.src.includes(k))) {
                el.remove();
            }
        });
    }

    function bypassAdblockDetectors() {
        var classes = [
            'adblock', 'adblocker', 'ad-block', 'ad-blocker',
            'anti-adblock', 'adblock-detected', 'adblock_msg',
        ];
        classes.forEach(function(cls) {
            document.querySelectorAll('.' + cls).forEach(function(el) { el.remove(); });
        });
    }

    function cleanup() {
        removeElements();
        removeAdImages();
        bypassAdblockDetectors();
    }

    cleanup();

    var debounceTimer = null;
    new MutationObserver(function() {
        if (debounceTimer) return;
        debounceTimer = setTimeout(function() {
            debounceTimer = null;
            removeElements();
        }, 300);
    }).observe(document.documentElement, {
        childList: true,
        subtree: true,
    });
})();
""")
        script.setInjectionPoint(QWebEngineScript.DocumentReady)
        script.setWorldId(QWebEngineScript.MainWorld)
        script.setRunsOnSubFrames(True)
        self.profile.scripts().insert(script)

    def show_tools_menu(self):
        menu = QMenu(self)

        hub = QAction("🚀  Tools Hub (Web)", self)
        hub.triggered.connect(self._open_tools_hub)
        menu.addAction(hub)
        menu.addSeparator()

        lang_menu = menu.addMenu("🌐  Language")
        t = QAction("Translate", self)
        t.triggered.connect(self._open_translate)
        lang_menu.addAction(t)
        t = QAction("YouTube Transcript", self)
        t.triggered.connect(self._open_transcript)
        lang_menu.addAction(t)

        web_menu = menu.addMenu("🔍  Web")
        t = QAction("Search", self)
        t.triggered.connect(self._open_search)
        web_menu.addAction(t)
        t = QAction("Weather", self)
        t.triggered.connect(self._open_weather)
        web_menu.addAction(t)

        doc_menu = menu.addMenu("📄  Documents")
        pdf_sub = doc_menu.addMenu("PDF Tools")
        t = QAction("Merge PDFs", self)
        t.triggered.connect(self._open_pdf_merge)
        pdf_sub.addAction(t)
        t = QAction("Split PDF", self)
        t.triggered.connect(self._open_pdf_split)
        pdf_sub.addAction(t)

        word_sub = doc_menu.addMenu("Word")
        t = QAction("DOCX → PDF", self)
        t.triggered.connect(self._open_word_to_pdf)
        word_sub.addAction(t)
        t = QAction("PDF → DOCX", self)
        t.triggered.connect(self._open_pdf_to_word)
        word_sub.addAction(t)

        excel_sub = doc_menu.addMenu("Excel")
        t = QAction("XLSX → PDF", self)
        t.triggered.connect(self._open_xlsx_to_pdf)
        excel_sub.addAction(t)
        t = QAction("PDF → XLSX", self)
        t.triggered.connect(self._open_pdf_to_xlsx)
        excel_sub.addAction(t)
        t = QAction("CSV → XLSX", self)
        t.triggered.connect(self._open_csv_to_xlsx)
        excel_sub.addAction(t)
        t = QAction("XLSX → CSV", self)
        t.triggered.connect(self._open_xlsx_to_csv)
        excel_sub.addAction(t)

        ppt_sub = doc_menu.addMenu("PowerPoint")
        t = QAction("PPTX → PDF", self)
        t.triggered.connect(self._open_pptx_to_pdf)
        ppt_sub.addAction(t)
        t = QAction("PDF → PPTX", self)
        t.triggered.connect(self._open_pdf_to_pptx)
        ppt_sub.addAction(t)

        other_sub = doc_menu.addMenu("Other")
        t = QAction("Image → PDF", self)
        t.triggered.connect(self._open_image_to_pdf)
        other_sub.addAction(t)
        t = QAction("PDF → Image", self)
        t.triggered.connect(self._open_pdf_to_image)
        other_sub.addAction(t)
        t = QAction("Text → PDF", self)
        t.triggered.connect(self._open_text_to_pdf)
        other_sub.addAction(t)
        t = QAction("PDF → Text", self)
        t.triggered.connect(self._open_pdf_to_text)
        other_sub.addAction(t)

        util_menu = menu.addMenu("🔧  Utilities")
        t = QAction("Archive Tools (Zip/7z/Tar)", self)
        t.triggered.connect(self._open_archive_tools)
        util_menu.addAction(t)
        t = QAction("VPN / Proxy", self)
        t.triggered.connect(self._open_proxy)
        util_menu.addAction(t)
        t = QAction("Timer", self)
        t.triggered.connect(self._open_timer)
        util_menu.addAction(t)
        t = QAction("QR Code Generator", self)
        t.triggered.connect(self._open_qr)
        util_menu.addAction(t)
        t = QAction("Unit Converter", self)
        t.triggered.connect(self._open_unit_converter)
        util_menu.addAction(t)
        t = QAction("Calculator", self)
        t.triggered.connect(self._open_calculator)
        util_menu.addAction(t)
        t = QAction("Programmer's Converter (Base)", self)
        t.triggered.connect(self._open_programmer_calc)
        util_menu.addAction(t)
        t = QAction("Note Taker", self)
        t.triggered.connect(self._open_note_taker)
        util_menu.addAction(t)

        menu.exec(QCursor.pos())

    def _open_tools_hub(self):
        path = os.path.join(ROOT, "src", "tools.html")
        if os.path.exists(path):
            self._new_tab("file://" + path)

    def _open_archive_tools(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Archive Tools")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        title = QLabel("📦 Archive Tools (Zip, 7z, Tar)")
        title.setObjectName("Title")
        layout.addWidget(title)

        tab_widget = QTabWidget()
        layout.addWidget(tab_widget)

        # Zip/Tar Tab
        zip_tab = QWidget()
        zip_layout = QVBoxLayout(zip_tab)
        
        file_list = QListWidget()
        zip_layout.addWidget(QLabel("Files to archive:"))
        zip_layout.addWidget(file_list)
        
        btn_row = QHBoxLayout()
        add_btn = QPushButton("Add Files")
        rem_btn = QPushButton("Remove")
        btn_row.addWidget(add_btn)
        btn_row.addWidget(rem_btn)
        zip_layout.addLayout(btn_row)
        
        fmt_combo = QComboBox()
        fmt_combo.addItems(["Zip", "7z", "Tar (.tar.gz)"])
        zip_layout.addWidget(QLabel("Format:"))
        zip_layout.addWidget(fmt_combo)
        
        go_btn = QPushButton("Create Archive")
        zip_layout.addWidget(go_btn)
        tab_widget.addTab(zip_tab, "Create Archive")

        # Unzip/Untar Tab
        unzip_tab = QWidget()
        unzip_layout = QVBoxLayout(unzip_tab)
        un_btn = QPushButton("Select Archive to Extract")
        unzip_layout.addWidget(un_btn)
        un_res = QLabel("")
        unzip_layout.addWidget(un_res)
        tab_widget.addTab(unzip_tab, "Extract Archive")

        def add_files():
            files, _ = QFileDialog.getOpenFileNames(dlg, "Select Files")
            for f in files: file_list.addItem(f)

        def rem_files():
            for i in file_list.selectedItems(): file_list.takeItem(file_list.row(i))

        def do_zip():
            if file_list.count() == 0: return
            fmt = fmt_combo.currentText()
            ext = ".zip" if fmt == "Zip" else ".7z" if fmt == "7z" else ".tar.gz"
            out, _ = QFileDialog.getSaveFileName(dlg, "Save Archive", f"archive{ext}")
            if not out: return
            
            paths = [file_list.item(i).text() for i in range(file_list.count())]
            try:
                from tools.archive_tools import zip_files, seven_zip_files, tar_files
                if fmt == "Zip": zip_files(paths, out)
                elif fmt == "7z": seven_zip_files(paths, out)
                else: tar_files(paths, out)
                QMessageBox.information(dlg, "Success", f"Archive created:\n{out}")
            except Exception as e: QMessageBox.critical(dlg, "Error", str(e))

        def do_unzip():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select Archive", "", "Archives (*.zip *.7z *.tar.gz *.tgz)")
            if not path: return
            out_dir = QFileDialog.getExistingDirectory(dlg, "Select Extraction Folder")
            if not out_dir: return
            
            try:
                from tools.archive_tools import unzip_file, unseven_zip_file, untar_file
                if path.endswith(".zip"): unzip_file(path, out_dir)
                elif path.endswith(".7z"): unseven_zip_file(path, out_dir)
                else: untar_file(path, out_dir)
                QMessageBox.information(dlg, "Success", f"Extracted to:\n{out_dir}")
            except Exception as e: QMessageBox.critical(dlg, "Error", str(e))

        add_btn.clicked.connect(add_files)
        rem_btn.clicked.connect(rem_files)
        go_btn.clicked.connect(do_zip)
        un_btn.clicked.connect(do_unzip)
        dlg.exec()


    def _open_translate(self):
        dlg = QDialog(self)
        dlg.setObjectName("ToolDialog")
        dlg.setWindowTitle("Translate")
        dlg.setFixedWidth(400)
        layout = QVBoxLayout(dlg)

        title = QLabel("🌐 Translator")
        title.setObjectName("Title")
        layout.addWidget(title)

        text_input = QTextEdit()
        text_input.setPlaceholderText("Enter text…")
        text_input.setMaximumHeight(80)
        layout.addWidget(text_input)

        row = QHBoxLayout()
        lang_combo = QComboBox()
        languages = {
            "Bangla": "bn", "Hindi": "hi", "Spanish": "es", "French": "fr", "German": "de",
            "Japanese": "ja", "Korean": "ko", "Chinese": "zh"
        }
        for name in languages: lang_combo.addItem(name)
        row.addWidget(lang_combo)
        
        btn = QPushButton("Translate")
        row.addWidget(btn)
        layout.addLayout(row)

        result_box = QTextEdit()
        result_box.setObjectName("ResultBox")
        result_box.setReadOnly(True)
        result_box.setMaximumHeight(100)
        layout.addWidget(result_box)

        btn.clicked.connect(lambda: self._do_translate(
            text_input.toPlainText(), languages[lang_combo.currentText()], result_box
        ))

        dlg.exec()

    def _do_translate(self, text, lang_code, result_box):
        try:
            from tools.translate import translate_text
            translated = translate_text(text, lang_code)
            result_box.setPlainText(translated)
        except Exception as e:
            result_box.setPlainText(f"Error: {e}")

    def _fill_yt_url(self, url_input):
        br = self.current_browser()
        if not br: return
        url = br.url().toString()
        from urllib.parse import urlparse, parse_qs
        parsed = urlparse(url)
        if "youtube.com" in parsed.netloc or "youtu.be" in parsed.netloc:
            url_input.setText(url)
            QMessageBox.information(self, "URL Filled", "Current YouTube URL detected!")
        else:
            QMessageBox.warning(self, "Not YouTube", "Current page is not a YouTube video.")

    def _open_transcript(self):
        dlg = QDialog(self)
        dlg.setObjectName("ToolDialog")
        dlg.setWindowTitle("Transcript")
        dlg.setFixedWidth(400)
        layout = QVBoxLayout(dlg)

        title = QLabel("🎬 YouTube Transcript")
        title.setObjectName("Title")
        layout.addWidget(title)

        url_input = QLineEdit()
        url_input.setPlaceholderText("Paste YouTube URL…")
        current_url = self.current_browser().url().toString() if self.current_browser() else ""
        if "youtube" in current_url: url_input.setText(current_url)
        layout.addWidget(url_input)

        fetch_btn = QPushButton("Fetch Text")
        layout.addWidget(fetch_btn)

        result_box = QTextEdit()
        result_box.setObjectName("ResultBox")
        result_box.setReadOnly(True)
        result_box.setMaximumHeight(200)
        layout.addWidget(result_box)

        def do_fetch():
            url = url_input.text().strip()
            if not url: return
            fetch_btn.setEnabled(False)
            result_box.setPlainText("Loading…")
            try:
                from tools.youtube_transcript import get_youtube_transcript
                text = get_youtube_transcript(url)
                result_box.setPlainText(text or "No transcript found.")
            except Exception as e: result_box.setPlainText(f"Error: {e}")
            finally: fetch_btn.setEnabled(True)

        fetch_btn.clicked.connect(do_fetch)
        dlg.exec()

    def _open_search(self):
        dlg = QDialog(self)
        dlg.setObjectName("ToolDialog")
        dlg.setFixedWidth(400)
        layout = QVBoxLayout(dlg)

        title = QLabel("🔍 Web Search")
        title.setObjectName("Title")
        layout.addWidget(title)

        query_input = QLineEdit()
        query_input.setPlaceholderText("Query…")
        layout.addWidget(query_input)

        result_box = QTextEdit()
        result_box.setObjectName("ResultBox")
        result_box.setReadOnly(True)
        result_box.setMaximumHeight(200)
        layout.addWidget(result_box)

        def do_search():
            q = query_input.text().strip()
            if not q: return
            try:
                from tools.knowledge_hub import search_web
                res = search_web(q)
                out = "\n\n".join([f"• {r['title']}\n{r['href']}" for r in res[:5]])
                result_box.setPlainText(out or "No results.")
            except Exception as e: result_box.setPlainText(f"Error: {e}")

        query_input.returnPressed.connect(do_search)
        dlg.exec()

    def _open_weather(self):
        dlg = QDialog(self)
        dlg.setObjectName("ToolDialog")
        dlg.setFixedWidth(300)
        layout = QVBoxLayout(dlg)

        title = QLabel("🌡 Weather")
        title.setObjectName("Title")
        layout.addWidget(title)

        city_input = QLineEdit()
        city_input.setPlaceholderText("City (e.g. Dhaka)")
        layout.addWidget(city_input)

        res = QLabel("Enter city to see weather.")
        res.setWordWrap(True)
        layout.addWidget(res)

        def do_weather():
            city = city_input.text().strip() or "Dhaka"
            try:
                from tools.knowledge_hub import get_weather_data
                w = get_weather_data(city)
                if "error" in w: res.setText(w["error"])
                else:
                    res.setText(f"<b>{w['city']}</b>: {w['temperature']}°C\n"
                               f"Wind: {w['windspeed']} km/h\n"
                               f"Time: {w['time']}")
            except Exception as e: res.setText(f"Error: {e}")

        city_input.returnPressed.connect(do_weather)
        dlg.exec()

    def _open_timer(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Timer")
        layout = QVBoxLayout(dlg)

        form = QFormLayout()
        hours = QSpinBox()
        hours.setRange(0, 24)
        mins = QSpinBox()
        mins.setRange(0, 59)
        mins.setValue(5)
        secs = QSpinBox()
        secs.setRange(0, 59)
        form.addRow("Hours:", hours)
        form.addRow("Minutes:", mins)
        form.addRow("Seconds:", secs)
        layout.addLayout(form)

        start_btn = QPushButton("Start Timer")
        layout.addWidget(start_btn)

        status = QLabel("")
        layout.addWidget(status)

        def do_timer():
            total = hours.value() * 3600 + mins.value() * 60 + secs.value()
            if total <= 0:
                status.setText("Set a valid duration.")
                return
            start_btn.setEnabled(False)
            remaining = [total]
            timer = QTimer(dlg)
            timer.timeout.connect(lambda: _tick(remaining, timer, status, start_btn, dlg))
            timer.start(1000)
            status.setText(f"Timer set for {total}s")

        def _tick(remaining, timer, status_label, btn, parent):
            remaining[0] -= 1
            if remaining[0] <= 0:
                timer.stop()
                status_label.setText("⏰ Time's up!")
                btn.setEnabled(True)
                try:
                    alarm_file = os.path.join(ROOT, "alarm.wav")
                    if os.path.exists(alarm_file):
                        if OS == "windows":
                            cmd = ["powershell", "-c", f"(New-Object Media.SoundPlayer '{alarm_file}').PlaySync()"]
                            subprocess.Popen(cmd)
                        else:
                            # Use ffplay (since ffmpeg is a dependency)
                            subprocess.Popen(["ffplay", "-nodisp", "-autoexit", alarm_file], 
                                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                except Exception:
                    pass
                return
            mins, secs = divmod(remaining[0], 60)
            hrs, mins = divmod(mins, 60)
            status_label.setText(f"⏱ {hrs:02d}:{mins:02d}:{secs:02d}")

        start_btn.clicked.connect(do_timer)
        dlg.exec()

    def _open_pdf_merge(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF Merger")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        file_list = QListWidget()
        layout.addWidget(QLabel("Selected PDFs:"))
        layout.addWidget(file_list)

        btn_layout = QHBoxLayout()
        add_btn = QPushButton("Add PDFs")
        remove_btn = QPushButton("Remove Selected")
        btn_layout.addWidget(add_btn)
        btn_layout.addWidget(remove_btn)
        layout.addLayout(btn_layout)

        merge_btn = QPushButton("Merge & Save As…")
        layout.addWidget(merge_btn)

        def add_files():
            files, _ = QFileDialog.getOpenFileNames(dlg, "Select PDFs", "", "PDFs (*.pdf)")
            for f in files:
                file_list.addItem(f)

        def remove_selected():
            for item in file_list.selectedItems():
                file_list.takeItem(file_list.row(item))

        def do_merge():
            if file_list.count() < 2:
                QMessageBox.warning(dlg, "PDF Merge", "Select at least 2 PDFs.")
                return
            out_path, _ = QFileDialog.getSaveFileName(dlg, "Save Merged PDF", "", "PDFs (*.pdf)")
            if not out_path:
                return
            try:
                from tools.pdf import merge_documents
                merge_documents([file_list.item(i).text() for i in range(file_list.count())], out_path)
                QMessageBox.information(dlg, "PDF Merge", f"Merged to:\n{out_path}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        add_btn.clicked.connect(add_files)
        remove_btn.clicked.connect(remove_selected)
        merge_btn.clicked.connect(do_merge)
        dlg.exec()

    def _open_pdf_split(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Split PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        file_btn = QPushButton("Select PDF to Split")
        layout.addWidget(file_btn)
        result = QLabel("")
        layout.addWidget(result)

        def do_split():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDFs (*.pdf)")
            if not path: return
            out_dir = QFileDialog.getExistingDirectory(dlg, "Output Directory")
            if not out_dir: return
            try:
                from tools.doc_tools import split_pdf
                paths = split_pdf(path, out_dir)
                result.setText(f"Created {len(paths)} files in:\n{out_dir}")
                QMessageBox.information(dlg, "Split PDF", f"Created {len(paths)} page files.")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        file_btn.clicked.connect(do_split)
        dlg.exec()

    def _open_word_to_pdf(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Word → PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        btn = QPushButton("Select Word (.docx) file…")
        layout.addWidget(btn)
        progress = QProgressBar()
        progress.setRange(0, 0)   # indeterminate spinner
        progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel("")
        result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select DOCX", "", "Word (*.docx)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PDF", "", "PDF (*.pdf)")
            if not out: return

            btn.setEnabled(False)
            progress.setVisible(True)
            result.setText("Converting… please wait.")

            def worker():
                try:
                    from tools.doc_tools import word_to_pdf
                    word_to_pdf(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(out_path, err):
                progress.setVisible(False)
                btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}")
                    QMessageBox.critical(dlg, "Conversion Error", str(err))
                else:
                    result.setText(f"✔ Saved: {out_path}")
                    QMessageBox.information(dlg, "Success", f"PDF saved to:\n{out_path}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pdf_to_word(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF → Word")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        btn = QPushButton("Select PDF file…")
        layout.addWidget(btn)
        progress = QProgressBar()
        progress.setRange(0, 0)
        progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel("")
        result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDF (*.pdf)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save DOCX", "", "Word (*.docx)")
            if not out: return

            btn.setEnabled(False)
            progress.setVisible(True)
            result.setText("Converting… please wait.")

            def worker():
                try:
                    from tools.doc_tools import pdf_to_word
                    pdf_to_word(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(out_path, err):
                progress.setVisible(False)
                btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}")
                    QMessageBox.critical(dlg, "Conversion Error", str(err))
                else:
                    result.setText(f"✔ Saved: {out_path}")
                    QMessageBox.information(dlg, "Success", f"Word file saved to:\n{out_path}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_image_to_pdf(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Image → PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        file_list = QListWidget()
        layout.addWidget(QLabel("Selected images:"))
        layout.addWidget(file_list)

        btn_layout = QHBoxLayout()
        add_btn = QPushButton("Add Images")
        remove_btn = QPushButton("Remove")
        btn_layout.addWidget(add_btn)
        btn_layout.addWidget(remove_btn)
        layout.addLayout(btn_layout)

        convert_btn = QPushButton("Convert to PDF…")
        layout.addWidget(convert_btn)

        def add_files():
            files, _ = QFileDialog.getOpenFileNames(dlg, "Select Images", "",
                "Images (*.png *.jpg *.jpeg *.bmp *.webp)")
            for f in files:
                file_list.addItem(f)

        def remove_selected():
            for item in file_list.selectedItems():
                file_list.takeItem(file_list.row(item))

        def do_convert():
            if file_list.count() == 0:
                QMessageBox.warning(dlg, "Image to PDF", "Add at least one image.")
                return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PDF", "", "PDF (*.pdf)")
            if not out: return
            try:
                from tools.doc_tools import image_to_pdf
                image_to_pdf([file_list.item(i).text() for i in range(file_list.count())], out)
                QMessageBox.information(dlg, "Success", f"PDF saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        add_btn.clicked.connect(add_files)
        remove_btn.clicked.connect(remove_selected)
        convert_btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_text_to_pdf(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Text → PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        text_edit = QTextEdit()
        text_edit.setPlaceholderText("Enter or paste text here…")
        layout.addWidget(text_edit)

        btn = QPushButton("Save as PDF")
        layout.addWidget(btn)

        def do_save():
            text = text_edit.toPlainText().strip()
            if not text:
                QMessageBox.warning(dlg, "Text to PDF", "Enter some text.")
                return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PDF", "", "PDF (*.pdf)")
            if not out: return
            try:
                from tools.doc_tools import text_to_pdf
                text_to_pdf(text, out)
                QMessageBox.information(dlg, "Success", f"PDF saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        btn.clicked.connect(do_save)
        dlg.exec()

    def _open_xlsx_to_pdf(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Excel → PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select Excel (.xlsx) file…")
        layout.addWidget(btn)
        progress = QProgressBar(); progress.setRange(0, 0); progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel(""); result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select XLSX", "", "Excel (*.xlsx)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PDF", "", "PDF (*.pdf)")
            if not out: return
            btn.setEnabled(False); progress.setVisible(True); result.setText("Converting…")

            def worker():
                try:
                    from tools.office_tools import xlsx_to_pdf
                    xlsx_to_pdf(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(o, err):
                progress.setVisible(False); btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}"); QMessageBox.critical(dlg, "Error", str(err))
                else:
                    result.setText(f"✔ Saved: {o}"); QMessageBox.information(dlg, "Success", f"PDF saved to:\n{o}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pdf_to_xlsx(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF → Excel")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select PDF file…")
        layout.addWidget(btn)
        progress = QProgressBar(); progress.setRange(0, 0); progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel(""); result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDF (*.pdf)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save XLSX", "", "Excel (*.xlsx)")
            if not out: return
            btn.setEnabled(False); progress.setVisible(True); result.setText("Extracting…")

            def worker():
                try:
                    from tools.office_tools import pdf_to_xlsx
                    pdf_to_xlsx(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(o, err):
                progress.setVisible(False); btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}"); QMessageBox.critical(dlg, "Error", str(err))
                else:
                    result.setText(f"✔ Saved: {o}"); QMessageBox.information(dlg, "Success", f"Excel saved to:\n{o}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_csv_to_xlsx(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("CSV → Excel")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select CSV file")
        layout.addWidget(btn)
        result = QLabel("")
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select CSV", "", "CSV (*.csv)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save XLSX", "", "Excel (*.xlsx)")
            if not out: return
            try:
                from tools.office_tools import csv_to_xlsx
                csv_to_xlsx(path, out)
                QMessageBox.information(dlg, "Success", f"Excel saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_xlsx_to_csv(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Excel → CSV")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select Excel (.xlsx) file")
        layout.addWidget(btn)
        result = QLabel("")
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select XLSX", "", "Excel (*.xlsx)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save CSV", "", "CSV (*.csv)")
            if not out: return
            try:
                from tools.office_tools import xlsx_to_csv
                xlsx_to_csv(path, out)
                QMessageBox.information(dlg, "Success", f"CSV saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pptx_to_pdf(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PowerPoint → PDF")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select PowerPoint (.pptx) file…")
        layout.addWidget(btn)
        progress = QProgressBar(); progress.setRange(0, 0); progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel(""); result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PPTX", "", "PowerPoint (*.pptx)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PDF", "", "PDF (*.pdf)")
            if not out: return
            btn.setEnabled(False); progress.setVisible(True); result.setText("Converting…")

            def worker():
                try:
                    from tools.office_tools import pptx_to_pdf
                    pptx_to_pdf(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(o, err):
                progress.setVisible(False); btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}"); QMessageBox.critical(dlg, "Error", str(err))
                else:
                    result.setText(f"✔ Saved: {o}"); QMessageBox.information(dlg, "Success", f"PDF saved to:\n{o}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pdf_to_pptx(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF → PowerPoint")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select PDF file…")
        layout.addWidget(btn)
        progress = QProgressBar(); progress.setRange(0, 0); progress.setVisible(False)
        layout.addWidget(progress)
        result = QLabel(""); result.setWordWrap(True)
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDF (*.pdf)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save PPTX", "", "PowerPoint (*.pptx)")
            if not out: return
            btn.setEnabled(False); progress.setVisible(True); result.setText("Converting…")

            def worker():
                try:
                    from tools.office_tools import pdf_to_pptx
                    pdf_to_pptx(path, out)
                    QTimer.singleShot(0, lambda: _done(out, None))
                except Exception as exc:
                    QTimer.singleShot(0, lambda e=exc: _done(None, e))

            def _done(o, err):
                progress.setVisible(False); btn.setEnabled(True)
                if err:
                    result.setText(f"Error: {err}"); QMessageBox.critical(dlg, "Error", str(err))
                else:
                    result.setText(f"✔ Saved: {o}"); QMessageBox.information(dlg, "Success", f"PPTX saved to:\n{o}")

            threading.Thread(target=worker, daemon=True).start()

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pdf_to_image(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF → Image")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select PDF file")
        layout.addWidget(btn)
        result = QLabel("")
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDF (*.pdf)")
            if not path: return
            out_dir = QFileDialog.getExistingDirectory(dlg, "Select output folder")
            if not out_dir: return
            try:
                from tools.office_tools import pdf_to_image
                files = pdf_to_image(path, out_dir)
                QMessageBox.information(dlg, "Success", f"{len(files)} image(s) saved to:\n{out_dir}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_pdf_to_text(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("PDF → Text")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)
        btn = QPushButton("Select PDF file")
        layout.addWidget(btn)
        result = QLabel("")
        layout.addWidget(result)

        def do_convert():
            path, _ = QFileDialog.getOpenFileName(dlg, "Select PDF", "", "PDF (*.pdf)")
            if not path: return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save TXT", "", "Text (*.txt)")
            if not out: return
            try:
                from tools.office_tools import pdf_to_text
                pdf_to_text(path, out)
                QMessageBox.information(dlg, "Success", f"Text saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_qr(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("QR Code Generator")
        dlg.setMinimumWidth(400)
        layout = QVBoxLayout(dlg)

        text_input = QLineEdit()
        text_input.setPlaceholderText("Enter text or URL…")
        layout.addWidget(text_input)

        size_layout = QHBoxLayout()
        size_layout.addWidget(QLabel("Size:"))
        size_spin = QSpinBox()
        size_spin.setRange(5, 40)
        size_spin.setValue(10)
        size_layout.addWidget(size_spin)
        size_layout.addStretch()
        layout.addLayout(size_layout)

        gen_btn = QPushButton("Generate & Save")
        layout.addWidget(gen_btn)

        preview = QLabel("")
        layout.addWidget(preview)

        def do_generate():
            text = text_input.text().strip()
            if not text:
                QMessageBox.warning(dlg, "QR Code", "Enter text or URL.")
                return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save QR Code", "qrcode.png",
                "PNG (*.png);;JPEG (*.jpg);;All (*)")
            if not out: return
            try:
                from tools.student_tools import generate_qr
                generate_qr(text, out, size_spin.value())
                QMessageBox.information(dlg, "Success", f"QR saved to:\n{out}")
            except Exception as e:
                QMessageBox.critical(dlg, "Error", str(e))

        gen_btn.clicked.connect(do_generate)
        text_input.returnPressed.connect(gen_btn.click)
        dlg.exec()

    def _open_unit_converter(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Unit Converter")
        dlg.setMinimumWidth(500)
        layout = QVBoxLayout(dlg)

        form = QFormLayout()
        value_input = QSpinBox()
        value_input.setRange(0, 999999999)
        value_input.setValue(1)
        form.addRow("Value:", value_input)

        cat_combo = QComboBox()
        cat_combo.addItems(["length", "weight", "temperature", "data", "speed", "area", "volume"])
        form.addRow("Category:", cat_combo)

        from_combo = QComboBox()
        to_combo = QComboBox()
        form.addRow("From:", from_combo)
        form.addRow("To:", to_combo)
        layout.addLayout(form)

        result_label = QLabel("")
        layout.addWidget(result_label)

        convert_btn = QPushButton("Convert")
        layout.addWidget(convert_btn)

        units_map = {
            "length": ["meter","kilometer","centimeter","millimeter","mile","yard","foot","inch"],
            "weight": ["kilogram","gram","milligram","pound","ounce","ton"],
            "temperature": ["celsius","fahrenheit","kelvin"],
            "data": ["byte","kilobyte","megabyte","gigabyte","terabyte"],
            "speed": ["m/s","km/h","mph","knot"],
            "area": ["sq_meter","sq_kilometer","sq_mile","sq_yard","sq_foot","acre","hectare"],
            "volume": ["liter","milliliter","gallon","quart","pint","cup","cubic_meter"],
        }

        def update_units(cat):
            from_combo.clear()
            to_combo.clear()
            for u in units_map.get(cat, []):
                from_combo.addItem(u)
                to_combo.addItem(u)
            if to_combo.count() > 1:
                to_combo.setCurrentIndex(1)

        cat_combo.currentTextChanged.connect(update_units)
        update_units(cat_combo.currentText())

        def do_convert():
            try:
                from tools.student_tools import convert_unit
                val = value_input.value()
                from_u = from_combo.currentText()
                to_u = to_combo.currentText()
                cat = cat_combo.currentText()
                result = convert_unit(val, from_u, to_u, cat)
                result_label.setText(f"{val} {from_u} = {result:.6g} {to_u}")
            except Exception as e:
                result_label.setText(f"Error: {e}")

        convert_btn.clicked.connect(do_convert)
        dlg.exec()

    def _open_calculator(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Calculator")
        dlg.setMinimumWidth(350)
        layout = QVBoxLayout(dlg)

        display = QLineEdit()
        display.setPlaceholderText("Enter expression (e.g. 2+2*5)")
        display.setMinimumHeight(40)
        layout.addWidget(display)

        result_label = QLabel("")
        layout.addWidget(result_label)

        grid = QHBoxLayout()
        buttons = [
            ["7","8","9","/"],
            ["4","5","6","*"],
            ["1","2","3","-"],
            ["0",".","%","+"],
            ["C","="],
        ]
        for row_btns in buttons:
            row = QHBoxLayout()
            for text in row_btns:
                btn = QPushButton(text)
                btn.setMinimumWidth(50)
                btn.clicked.connect(lambda checked, t=text: _btn_click(t))
                row.addWidget(btn)
            grid.addLayout(row)

        layout.addLayout(grid)

        def _btn_click(text):
            if text == "C":
                display.clear()
                result_label.clear()
            elif text == "=":
                expr = display.text()
                try:
                    from tools.student_tools import calculate
                    result_label.setText(f"= {calculate(expr)}")
                except Exception as e:
                    result_label.setText(f"Error: {e}")
            else:
                display.setText(display.text() + text)

        display.returnPressed.connect(lambda: _btn_click("="))
        dlg.exec()

    def _open_programmer_calc(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Programmer's Converter (Base)")
        dlg.setMinimumWidth(400)
        layout = QVBoxLayout(dlg)

        form = QFormLayout()
        val_input = QLineEdit()
        from_base = QComboBox()
        from_base.addItems(["dec", "bin", "hex", "oct"])
        to_base = QComboBox()
        to_base.addItems(["bin", "hex", "oct", "dec"])
        
        form.addRow("Value:", val_input)
        form.addRow("From Base:", from_base)
        form.addRow("To Base:", to_base)
        layout.addLayout(form)

        result_label = QLabel("Result: ")
        result_label.setStyleSheet("font-weight: bold; font-size: 14pt; color: #2980b9; margin-top: 10px;")
        layout.addWidget(result_label)

        def do_convert():
            v = val_input.text().strip()
            if not v:
                result_label.setText("Result: ")
                return
            fb = from_base.currentText()
            tb = to_base.currentText()
            from tools.student_tools import programmer_calc
            result_label.setText(f"Result: {programmer_calc(v, fb, tb)}")

        val_input.textChanged.connect(do_convert)
        from_base.currentIndexChanged.connect(do_convert)
        to_base.currentIndexChanged.connect(do_convert)
        
        dlg.exec()

    def _open_note_taker(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Note Taker")
        dlg.setMinimumSize(500, 400)
        layout = QVBoxLayout(dlg)

        text_edit = QTextEdit()
        text_edit.setPlaceholderText("Write your notes here…")
        layout.addWidget(text_edit)

        btn_layout = QHBoxLayout()
        save_btn = QPushButton("💾 Save")
        clear_btn = QPushButton("🗑 Clear")
        btn_layout.addWidget(save_btn)
        btn_layout.addWidget(clear_btn)
        btn_layout.addStretch()
        layout.addLayout(btn_layout)

        status = QLabel("")
        layout.addWidget(status)

        def do_save():
            text = text_edit.toPlainText().strip()
            if not text:
                status.setText("Nothing to save.")
                return
            out, _ = QFileDialog.getSaveFileName(dlg, "Save Note", "note.txt",
                "Text (*.txt);;All (*)")
            if not out: return
            try:
                from tools.student_tools import save_note
                save_note(text, out)
                status.setText(f"Saved to {out}")
                QMessageBox.information(dlg, "Note Saved", f"Saved to:\n{out}")
            except Exception as e:
                status.setText(f"Error: {e}")

        def do_clear():
            text_edit.clear()
            status.setText("Cleared.")

        save_btn.clicked.connect(do_save)
        clear_btn.clicked.connect(do_clear)
        dlg.exec()

    def _open_proxy(self):
        from utils.proxy_tools import load_proxies, get_random_proxy, get_current_ip, is_proxy_working
        from utils.network_fix import check_connectivity
        from PySide6.QtCore import QSettings
        import subprocess, shutil

        dlg = QDialog(self)
        dlg.setWindowTitle("Network & Security")
        dlg.setMinimumWidth(600)
        dlg.setMinimumHeight(500)
        layout = QVBoxLayout(dlg)

        # Status Section
        status_group = QFrame()
        status_group.setStyleSheet("background-color: #ffffff; border: 1px solid #caf0f8; border-radius: 10px; padding: 10px;")
        status_layout = QVBoxLayout(status_group)
        
        ip_layout = QHBoxLayout()
        self.ip_display = QLabel("Fetching IP…")
        self.ip_display.setStyleSheet("font-size:18px; font-weight:bold; color: #0077b6;")
        ip_layout.addWidget(QLabel("🌐 Public IP:"))
        ip_layout.addWidget(self.ip_display)
        ip_layout.addStretch()
        status_layout.addLayout(ip_layout)

        conn_layout = QHBoxLayout()
        self.conn_display = QLabel("Checking…")
        conn_layout.addWidget(QLabel("📡 Connectivity:"))
        conn_layout.addWidget(self.conn_display)
        conn_layout.addStretch()
        status_layout.addLayout(conn_layout)
        
        layout.addWidget(status_group)

        # Proxy Section
        layout.addWidget(QLabel("<b>Proxy Management</b> (Restart required to apply)"))
        proxy_list = QListWidget()
        layout.addWidget(proxy_list)

        btn_row = QHBoxLayout()
        test_btn = QPushButton("🧪 Test Selected")
        random_btn = QPushButton("🎲 Random & Apply")
        disable_btn = QPushButton("✖ Disable Proxy")
        disable_btn.setStyleSheet("background-color: #d90429; color: white;")
        btn_row.addWidget(test_btn)
        btn_row.addWidget(random_btn)
        btn_row.addWidget(disable_btn)
        layout.addLayout(btn_row)

        sep = QFrame()
        sep.setFrameShape(QFrame.HLine)
        sep.setFrameShadow(QFrame.Sunken)
        layout.addWidget(sep)

        # MAC Spoofing Section
        layout.addWidget(QLabel("<b>MAC Spoofing (Linux Only)</b>"))
        mac_info = QLabel("Changes local MAC and requests new DHCP lease.")
        mac_info.setStyleSheet("color: #666666; font-size: 11px;")
        layout.addWidget(mac_info)

        iface_label = QLabel("Interface: Detecting…")
        layout.addWidget(iface_label)

        mac_status = QLabel("")
        mac_status.setWordWrap(True)
        layout.addWidget(mac_status)

        if OS == "linux":
            spoof_btn = QPushButton("🔄 Randomize MAC & Reset Connection")
            layout.addWidget(spoof_btn)
        else:
            spoof_btn = None

        def update_status():
            self.ip_display.setText("Fetching…")
            self.conn_display.setText("Checking…")
            
            def run():
                ip = get_current_ip()
                connected = check_connectivity()
                self.ip_display.setText(ip)
                self.conn_display.setText("Connected ✅" if connected else "Disconnected ❌")
                self.conn_display.setStyleSheet("color: #023e8a;")
            
            threading.Thread(target=run, daemon=True).start()

        def populate_list():
            proxy_list.clear()
            for p in load_proxies():
                item = QListWidgetItem(p)
                proxy_list.addItem(item)

        def do_test():
            item = proxy_list.currentItem()
            if not item: return
            p = item.text()
            item.setText(f"{p} (Testing…)")
            
            def run():
                working = is_proxy_working(p)
                item.setText(f"{p} ({'Working ✅' if working else 'Failed ❌'})")
            
            threading.Thread(target=run, daemon=True).start()

        def do_apply_proxy(p):
            s = QSettings("SwordFish", "Browser")
            s.setValue("proxy_url", p)
            s.setValue("proxy_active", "true")
            QMessageBox.information(dlg, "Proxy Set", f"Proxy set to: {p}\n\nRestart browser to apply changes.")

        def do_disable():
            s = QSettings("SwordFish", "Browser")
            s.setValue("proxy_url", "")
            s.setValue("proxy_active", "false")
            QMessageBox.information(dlg, "Proxy Disabled", "Proxy has been disabled.\n\nRestart browser to apply changes.")

        def do_spoof():
            iface = detect_iface()
            pkexec = shutil.which("pkexec") or shutil.which("sudo") or ""
            mac_status.setText("Running spoof script…")
            
            def run():
                script = os.path.join(ROOT, "utils", "network_fix.py")
                try:
                    r = subprocess.run(
                        [pkexec, sys.executable, script, "--iface", iface],
                        capture_output=True, text=True, timeout=30
                    )
                    if r.returncode == 0:
                        mac_status.setText(f"✅ Success on {iface}. Reconnecting…")
                        update_status()
                    else:
                        mac_status.setText(f"❌ Failed: {r.stderr.strip()}")
                except Exception as e:
                    mac_status.setText(f"❌ Error: {e}")
            
            threading.Thread(target=run, daemon=True).start()

        def detect_iface():
            try:
                out = subprocess.run("ip route get 1.1.1.1", shell=True, capture_output=True, text=True).stdout
                import re
                m = re.search(r"dev\s+(\S+)", out)
                return m.group(1) if m else "wlan0"
            except: return "wlan0"

        iface = detect_iface()
        iface_label.setText(f"Interface: {iface}")

        populate_list()
        update_status()
        
        test_btn.clicked.connect(do_test)
        random_btn.clicked.connect(lambda: do_apply_proxy(get_random_proxy()))
        disable_btn.clicked.connect(do_disable)
        if spoof_btn: spoof_btn.clicked.connect(do_spoof)
        proxy_list.itemDoubleClicked.connect(lambda item: do_apply_proxy(item.text()))

        dlg.exec()

if __name__ == "__main__":
    from PySide6.QtCore import QLocale
    QLocale.setDefault(QLocale(QLocale.English, QLocale.UnitedStates))

    from PySide6.QtCore import QSettings as _QS
    _s = _QS("SwordFish", "Browser")
    if _s.value("proxy_active", "false") == "true":
        _val = _s.value("proxy_url", "")
        _proxy = str(_val).strip() if _val else ""
        if _proxy:
            os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = f"--proxy-server={_proxy} --logging-level=3"

    app = QApplication(sys.argv)
    QApplication.setApplicationName("SwordFish")
    
    # Apply modern theme
    apply_theme(app)
    
    window = Main()
    sys.exit(app.exec())
