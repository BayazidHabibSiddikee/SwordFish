# 🗡️ SwordFish Web Browser

![SwordFish Logo](icon.png)

SwordFish is a **privacy-first, power-user browser** built with **C++ and Qt6/WebEngine**. Designed for researchers, developers, and students who need a high-performance browsing experience with a suite of 20+ integrated productivity tools.

> **v2.0** — Rewritten from Python/PySide6 to C++/Qt6 for native performance, lower memory usage, and a proper packaging pipeline.

---

## 🚀 Key Features

### 🌐 Browsing
- **Multi-tab** with tab pinning, muting, and keyboard shortcuts
- **Privacy-first** — local profile storage, no external telemetry, dedicated **Stealth/Private Mode**
- **Advanced Adblocker** — 4-level protection (None / Low / Medium / Ultimate), setting persists across restarts
- **DNS-over-HTTPS** — AdGuard by default, switchable to Cloudflare, NextDNS, Google, or System DNS. Applied at startup before the network stack initializes.
- **YouTube Shorts blocked** — Shorts URLs are intercepted and redirected; Shorts shelf hidden from the YouTube homepage and sidebar
- **Media Downloader** — integrated `yt-dlp` for video (up to 4K) and audio (MP3/M4A/OGG)
- **English-Only Mode** — force English locale across all web content
- **Reading Mode**, **Picture-in-Picture**, **Media Controls Bar**

### 🛠️ Tools Hub
Open from the **🔧 Tools** menu. All tools are built-in — no downloads required.

| Category | Tools |
|---|---|
| **PDF & Docs** | Word↔PDF, Excel↔PDF, PPTX↔PDF, Image↔PDF, Text→PDF, Merge/Split PDF, Extract Text |
| **Utilities** | Calculator, Unit Converter, Programmer Calc (Bin/Hex/Oct/Dec), Archive Tools (Zip/7z/Tar) |
| **Media & Info** | YouTube Transcript, Translator, Weather, QR Generator, Note Taker, Timer |
| **System** | Launch Terminal |

### 🧩 Extensions (UserScripts)
- Load Greasemonkey/Tampermonkey-compatible `.user.js` scripts from `~/.config/SwordFish/extensions/`
- **Install from URL** — paste a Greasy Fork page URL or any `.user.js` direct link; auto-downloads, validates, and activates immediately
- Toggle scripts on/off per-session, reload all without restarting
- Open via **Ctrl+Shift+E**

### 🔐 Password Manager
- Captures login forms automatically
- AES-256 encrypted local storage
- Open via **Ctrl+Shift+P**

### 🛡️ Security & Privacy
- Public IP display, connectivity tester, proxy management
- MAC address spoofing (Linux)
- Cookie management
- File/folder pickers restricted to home directory — no access to `/`, `/etc`, system paths

---

## 📦 Installation

### 🐧 Linux

**1. Install build dependencies:**
```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFish.git
cd SwordFish
chmod +x install_cpp.sh
./install_cpp.sh
```
This installs Qt6, CMake, and all runtime tools (LibreOffice, poppler, p7zip, etc.) then builds the binary.

**2. Install the binary:**
```bash
mkdir -p ~/.local/bin
cp build/SwordFish ~/.local/bin/SwordFish
```

**3. Install the desktop entry and icon:**
```bash
mkdir -p ~/.local/share/applications ~/.local/share/icons/hicolor/256x256/apps ~/.local/share/pixmaps
cp icon.png ~/.local/share/icons/hicolor/256x256/apps/swordfish.png
cp icon.png ~/.local/share/pixmaps/swordfish.png
cat > ~/.local/share/applications/swordfish.desktop << EOF
[Desktop Entry]
Version=2.0
Type=Application
Name=SwordFish
GenericName=Web Browser
Comment=Privacy-first power-user web browser
Exec=$HOME/.local/bin/SwordFish %U
TryExec=$HOME/.local/bin/SwordFish
Icon=swordfish
Terminal=false
StartupNotify=true
StartupWMClass=SwordFish
Categories=Network;WebBrowser;Qt;
MimeType=text/html;text/xml;application/xhtml+xml;x-scheme-handler/http;x-scheme-handler/https;
Actions=NewWindow;NewPrivateWindow;

[Desktop Action NewWindow]
Name=New Window
Exec=$HOME/.local/bin/SwordFish

[Desktop Action NewPrivateWindow]
Name=New Private Window
Exec=$HOME/.local/bin/SwordFish --private
EOF
update-desktop-database ~/.local/share/applications
```

Launch from the app menu or run:
```bash
~/.local/bin/SwordFish
```

**After updating the source, rebuild and redeploy with:**
```bash
cmake --build build --parallel $(nproc)
cp build/SwordFish ~/.local/bin/SwordFish
```

### 🪟 Windows
```cmd
# In an Administrator Command Prompt:
requirements.bat
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
build\Release\SwordFish.exe
```

---

## 📁 Project Structure

```
SwordFish/
├── src/
│   ├── main.cpp              # Entry point, DNS-over-HTTPS init
│   ├── mainwindow.cpp/.h     # Main window, tabs, all menus
│   ├── adblocker.cpp/.h      # Network-level ad/tracker blocking
│   ├── extension_system.cpp/.h  # UserScript loader + install-from-URL
│   ├── password_manager.cpp/.h  # AES-256 password vault
│   ├── file_picker.cpp/.h    # Home-restricted file picker
│   ├── folder_picker.cpp/.h  # Home-restricted folder picker
│   ├── media_bar.cpp/.h      # Media playback controls
│   ├── reading_mode.cpp/.h   # Reader mode
│   ├── pip_window.cpp/.h     # Picture-in-Picture
│   ├── sync_manager.cpp/.h   # Bookmark/history sync
│   ├── web_page.cpp/.h       # Custom QWebEnginePage
│   ├── styles.cpp/.h         # One Dark theme stylesheet
│   ├── tools.html            # Tools Hub UI (Qt resource, QWebChannel)
│   └── resources.qrc         # Qt resource bundle
├── tools/                    # External tool scripts (Python helpers)
├── utils/                    # Network and TTS utilities
├── packaging/linux/          # .desktop file, man page
├── CMakeLists.txt
└── install_cpp.sh            # Dependency installer + build script
```

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+R` | Reload |
| `Ctrl+Shift+P` | Password Manager |
| `Ctrl+Shift+E` | Extensions Manager |
| `Ctrl+Shift+N` | New private window |
| `Ctrl+D` | Bookmark page |

---

## 🔧 Build Requirements

| Requirement | Version |
|---|---|
| CMake | ≥ 3.20 |
| C++ compiler | C++17 (GCC 10+, Clang 12+) |
| Qt6 | Core, Gui, Widgets, Network, WebEngineCore, WebEngineWidgets, WebChannel |
| Runtime tools | LibreOffice, poppler-utils, p7zip, qpdf, qrencode, enscript, ghostscript, yt-dlp |

---

*Built for power users who demand privacy and productivity.*
