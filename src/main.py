import sys
import os
import subprocess
import json

# ── Force English BEFORE Qt loads anything ────────────────────────────────────
os.environ["LANG"]     = "en_US.UTF-8"
os.environ["LANGUAGE"] = "en_US"
os.environ["LC_ALL"]   = "en_US.UTF-8"

#Just Bayazid HS's things -_-
# ── Platform detection ────────────────────────────────────────────────────────
def get_platform():
    if sys.platform.startswith("win"):  return "windows"
    if "TERMUX_VERSION" in os.environ:  return "android"
    if sys.platform == "darwin":        return "mac"
    return "linux"

OS = get_platform()
os.environ["QTWEBENGINE_CHROMIUM_FLAGS"] = "--logging-level=3"

from PySide6.QtCore    import QUrl, QSettings, QLocale, QSize, QPoint
from PySide6.QtWidgets import QApplication, QMainWindow, QToolBar, QLineEdit, QMenu, QInputDialog, QMessageBox
from PySide6.QtWebEngineWidgets import QWebEngineView
from PySide6.QtWebEngineCore    import QWebEngineScript, QWebEngineProfile
from PySide6.QtGui   import QAction, QCursor

# ── Config file path (stores bookmarks + history as JSON) ─────────────────────
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
DATA_FILE   = os.path.join(CONFIG_DIR, "data.json")       # bookmarks, history
PROFILE_DIR = os.path.join(CONFIG_DIR, "browser_profile") # cookies, cache

# ── JSON data helpers ─────────────────────────────────────────────────────────
def load_data():
    if os.path.exists(DATA_FILE):
        with open(DATA_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {"bookmarks": [], "history": []}

def save_data(data):
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

def get_download_dir():
    # Read from QSettings first so user can change it
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


# ── Main Window ───────────────────────────────────────────────────────────────
class Main(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SwordFish Browser")

        # ── QSettings: window geometry, home page, etc. ───────────────────────
        self.settings = QSettings("SwordFish", "Browser")
        # Stored where the OS prefers:
        #   Linux  → ~/.config/SwordFish/Browser.conf
        #   Windows→ Registry: HKCU\Software\SwordFish\Browser
        #   macOS  → ~/Library/Preferences/com.SwordFish.Browser.plist

        self.home = self.settings.value("home_url", "https://duckduckgo.com")

        # ── Persistent browser profile (cookies, cache, logins) ───────────────
        self.profile = QWebEngineProfile("SwordFish", self)
        self.profile.setPersistentStoragePath(PROFILE_DIR)
        self.profile.setCachePath(os.path.join(PROFILE_DIR, "cache"))
        self.profile.setPersistentCookiesPolicy(
            QWebEngineProfile.AllowPersistentCookies
        )
        # Force English in the browser's Accept-Language header
        self.profile.setHttpAcceptLanguage("en-US,en;q=0.9")

        # ── Browser view using persistent profile ─────────────────────────────
        from PySide6.QtWebEngineCore import QWebEnginePage
        page = QWebEnginePage(self.profile, self)
        self.browser = QWebEngineView()
        self.browser.setPage(page)
        self.browser.setUrl(QUrl(self.home))
        self.setCentralWidget(self.browser)

        # ── Navbar ────────────────────────────────────────────────────────────
        navbar = QToolBar("Navigation")
        navbar.setMovable(False)
        self.addToolBar(navbar)

        for label, slot in [
            ("◀", self.browser.back),
            ("▶", self.browser.forward),
            ("↻", self.browser.reload),
            ("⌂", self.navigate_home),
        ]:
            btn = QAction(label, self)
            btn.triggered.connect(slot)
            navbar.addAction(btn)

        self.url_bar = QLineEdit()
        self.url_bar.setPlaceholderText("Enter URL or search…")
        self.url_bar.returnPressed.connect(self.navigate_to_url)
        navbar.addWidget(self.url_bar)
        self.browser.urlChanged.connect(self.update_url)

        # Bookmark button
        bm_btn = QAction("☆ Bookmark", self)
        bm_btn.triggered.connect(self.show_bookmarks_menu)
        navbar.addAction(bm_btn)

        # Download button
        dl_btn = QAction("⬇ Download", self)
        dl_btn.triggered.connect(self.show_download_menu)
        navbar.addAction(dl_btn)

        # Settings button
        cfg_btn = QAction("⚙", self)
        cfg_btn.triggered.connect(self.show_settings_menu)
        navbar.addAction(cfg_btn)

        # ── Load JSON data & restore window state ─────────────────────────────
        self.data = load_data()
        self._restore_window()
        self._inject_adblock()

        # Save to history when page changes
        self.browser.urlChanged.connect(self._record_history)

    # ── Window state save / restore ───────────────────────────────────────────
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
        # Save window state on close
        self.settings.setValue("window_size",  self.size())
        self.settings.setValue("window_pos",   self.pos())
        self.settings.setValue("maximized",    self.isMaximized())
        self.settings.setValue("home_url",     self.home)
        save_data(self.data)
        super().closeEvent(event)

    # ── History ───────────────────────────────────────────────────────────────
    def _record_history(self, qurl):
        url = qurl.toString()
        title = self.browser.title() or url
        entry = {"url": url, "title": title}
        # Avoid duplicate consecutive entries
        if not self.data["history"] or self.data["history"][-1]["url"] != url:
            self.data["history"].append(entry)
            # Keep last 200 entries only
            self.data["history"] = self.data["history"][-200:]

    # ── Bookmarks ─────────────────────────────────────────────────────────────
    def show_bookmarks_menu(self):
        menu = QMenu(self)

        add = QAction("➕  Bookmark this page", self)
        add.triggered.connect(self._add_bookmark)
        menu.addAction(add)

        history_menu = menu.addMenu("🕓  History")
        for entry in reversed(self.data["history"][-20:]):
            a = QAction(entry["title"][:60], self)
            a.triggered.connect(lambda checked, u=entry["url"]: self.browser.setUrl(QUrl(u)))
            history_menu.addAction(a)

        if self.data["bookmarks"]:
            menu.addSeparator()
            for bm in self.data["bookmarks"]:
                a = QAction("🔖 " + bm["title"][:50], self)
                a.triggered.connect(lambda checked, u=bm["url"]: self.browser.setUrl(QUrl(u)))
                menu.addAction(a)

        menu.exec(QCursor.pos())

    def _add_bookmark(self):
        url   = self.browser.url().toString()
        title = self.browser.title() or url
        # Don't duplicate
        if any(b["url"] == url for b in self.data["bookmarks"]):
            QMessageBox.information(self, "Bookmark", "Already bookmarked!")
            return
        self.data["bookmarks"].append({"url": url, "title": title})
        save_data(self.data)
        QMessageBox.information(self, "Bookmark", f"Saved:\n{title}")

    # ── Settings menu ─────────────────────────────────────────────────────────
    def show_settings_menu(self):
        menu = QMenu(self)

        set_home = QAction("🏠  Set current page as Home", self)
        set_home.triggered.connect(self._set_home)
        menu.addAction(set_home)

        set_dl = QAction("📁  Change download folder", self)
        set_dl.triggered.connect(self._change_download_dir)
        menu.addAction(set_dl)

        clear_hist = QAction("🗑  Clear history", self)
        clear_hist.triggered.connect(self._clear_history)
        menu.addAction(clear_hist)

        clear_bm = QAction("🗑  Clear bookmarks", self)
        clear_bm.triggered.connect(self._clear_bookmarks)
        menu.addAction(clear_bm)

        menu.addSeparator()
        about = QAction(f"ℹ  Config: {CONFIG_DIR}", self)
        about.setEnabled(False)
        menu.addAction(about)

        menu.exec(QCursor.pos())

    def _set_home(self):
        self.home = self.browser.url().toString()
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
        save_data(self.data)
        QMessageBox.information(self, "History", "History cleared.")

    def _clear_bookmarks(self):
        self.data["bookmarks"] = []
        save_data(self.data)
        QMessageBox.information(self, "Bookmarks", "Bookmarks cleared.")

    # ── Download dropdown ─────────────────────────────────────────────────────
    def show_download_menu(self):
        menu = QMenu(self)
        video_menu = menu.addMenu("🎬  Video")
        for label, fmt in [
            ("144p",         "bestvideo[height<=144]+bestaudio/best[height<=144]"),
            ("360p",         "bestvideo[height<=360]+bestaudio/best[height<=360]"),
            ("480p",         "bestvideo[height<=480]+bestaudio/best[height<=480]"),
            ("720p  (HD)",   "bestvideo[height<=720]+bestaudio/best[height<=720]"),
            ("1080p (FHD)",  "bestvideo[height<=1080]+bestaudio/best[height<=1080]"),
            ("4K    (best)", "bestvideo+bestaudio/best"),
        ]:
            a = QAction(label, self)
            a.triggered.connect(lambda checked, f=fmt: self.download("video", f))
            video_menu.addAction(a)

        audio_menu = menu.addMenu("🎵  Audio only")
        for label, fmt in [
            ("MP3  (128k)", "bestaudio --extract-audio --audio-format mp3 --audio-quality 128K"),
            ("MP3  (320k)", "bestaudio --extract-audio --audio-format mp3 --audio-quality 0"),
            ("M4A  (best)", "bestaudio[ext=m4a]/bestaudio --extract-audio --audio-format m4a"),
            ("OGG  (best)", "bestaudio --extract-audio --audio-format vorbis"),
        ]:
            a = QAction(label, self)
            a.triggered.connect(lambda checked, f=fmt: self.download("audio", f))
            audio_menu.addAction(a)

        menu.addSeparator()
        plain = QAction("📄  Plain file (wget/curl)", self)
        plain.triggered.connect(lambda: self.download("plain", ""))
        menu.addAction(plain)
        menu.exec(QCursor.pos())

    def download(self, mode, fmt):
        url    = self.browser.url().toString()
        dl_dir = get_download_dir()
        os.makedirs(dl_dir, exist_ok=True)
        print(f"[Download] mode={mode} url={url}")
        if mode == "plain":
            self._plain_download(url, dl_dir)
            return
        out = os.path.join(dl_dir, "%(title)s.%(ext)s")
        if mode == "audio":
            base_fmt = fmt.split()[0]
            cmd = ["yt-dlp", "-f", base_fmt, "--extract-audio", "--audio-quality", "0", "-o", out, url]
            if "--audio-format" in fmt:
                parts = fmt.split()
                cmd += ["--audio-format", parts[parts.index("--audio-format") + 1]]
            if "--audio-quality" in fmt:
                parts = fmt.split()
                cmd[cmd.index("0")] = parts[parts.index("--audio-quality") + 1]
        else:
            cmd = ["yt-dlp", "-f", fmt, "-o", out, url]
        try:
            run_detached(cmd)
        except FileNotFoundError:
            self._plain_download(url, dl_dir)

    def _plain_download(self, url, dl_dir):
        try:
            if OS == "windows":
                run_detached(["curl", "-L", "-o", os.path.join(dl_dir, "download"), url])
            else:
                run_detached(["wget", "-P", dl_dir, url])
        except FileNotFoundError:
            print("[Download] No downloader found. pip install yt-dlp")

    # ── Navigation ────────────────────────────────────────────────────────────
    def navigate_home(self):
        self.browser.setUrl(QUrl(self.home))

    def navigate_to_url(self):
        url = self.url_bar.text().strip()
        if not url: return
        if "." in url and " " not in url:
            if not url.startswith("http"):
                url = "http://" + url
        else:
            url = "https://www.google.com/search?q=" + url.replace(" ", "+")
        self.browser.setUrl(QUrl(url))

    def update_url(self, q):
        self.url_bar.setText(q.toString())

    # ── Adblock ───────────────────────────────────────────────────────────────
    def _inject_adblock(self):
        script = QWebEngineScript()
        script.setName("adblock")
        script.setSourceCode("""
            setInterval(() => {
                ['.ad','.ads','.adsense','#ad-sidebar',
                 '.ytd-ad-slot-renderer','[id^="google_ads"]',
                 'iframe[src*="doubleclick"]'].forEach(sel =>
                    document.querySelectorAll(sel).forEach(el =>
                        el.style.display = 'none'));
            }, 1000);
        """)
        script.setInjectionPoint(QWebEngineScript.DocumentReady)
        script.setWorldId(QWebEngineScript.MainWorld)
        script.setRunsOnSubFrames(True)
        self.browser.page().profile().scripts().insert(script)


if __name__ == "__main__":
    import threading
    # ── Force English locale in Qt itself ─────────────────────────────────────
    from PySide6.QtCore import QLocale
    QLocale.setDefault(QLocale(QLocale.English, QLocale.UnitedStates))

    app = QApplication(sys.argv)
    QApplication.setApplicationName("SwordFish")
    window = Main()
    sys.exit(app.exec())
