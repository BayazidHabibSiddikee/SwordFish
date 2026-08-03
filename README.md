# 🗡️ SwordFish Web Browser

![SwordFish Logo](icon.png)

SwordFish is a **privacy-first, power-user browser** built with **C++ and Qt6/WebEngine**. Designed for researchers, developers, and students who need a high-performance browsing experience with a suite of 20+ integrated productivity tools.

> **v2.0** — Rewritten from Python/PySide6 to C++/Qt6 for native performance, lower memory usage, and a proper packaging pipeline.

---

## ⬇️ Download

| Platform | File | How to install |
|---|---|---|
| **Linux** (.deb) | [swordfish-2.0.0-Linux.deb](dist/swordfish-2.0.0-Linux.deb) | `sudo dpkg -i swordfish-2.0.0-Linux.deb` |
| **Linux** (.tar.gz) | [swordfish-2.0.0-Linux.tar.gz](dist/swordfish-2.0.0-Linux.tar.gz) | Extract and run `SwordFish` |
| **Windows** (.exe) | Build from source (see below) | Requires Qt6 on Windows |

---

## 🚀 Key Features

### 🌐 Browsing
- **Multi-tab** with tab pinning, muting, and keyboard shortcuts
- **Privacy-first** — local profile storage, no external telemetry, dedicated **Stealth/Private Mode**
- **Advanced Adblocker** — 4-level protection (None / Low / Medium / Ultimate), setting persists across restarts
- **DNS-over-HTTPS** — AdGuard by default, switchable to Cloudflare, NextDNS, Google, or System DNS. Applied at startup before the network stack initializes
- **YouTube Shorts blocked** — Shorts URLs intercepted and redirected; Shorts shelf and sidebar link hidden everywhere on YouTube
- **Media Downloader** — integrated `yt-dlp` for video (up to 4K) and audio (MP3/M4A/OGG)
- **English-Only Mode** — force English locale across all web content
- **Reading Mode**, **Picture-in-Picture**, **Media Controls Bar**

### 🛠️ Tools Hub
Open from the **🔧 Tools** menu. The hub runs as an embedded web page with full dark/light mode support.

| Category | Tools |
|---|---|
| **PDF & Docs** | Word↔PDF, Excel↔PDF, PPTX↔PDF, Image↔PDF, Text→PDF, Merge/Split PDF, Extract Text, CSV↔Excel |
| **Language & Media** | Translator, YouTube Transcript |
| **Utilities** | Calculator, Unit Converter, Programmer Calc (Bin/Hex/Oct/Dec), Archive Tools (Zip/7z/Tar), Timer, QR Generator, Weather, Note Taker, Terminal |

> **📦 Optional Dependencies** — The Tools Hub has a built-in dependency manager. Open the hub and the **📦 Optional Dependencies** section automatically checks which tools are installed. Click **⬇️ Install** next to any missing group (e.g. LibreOffice for Office tools, yt-dlp for media download, qpdf for PDF merge). A system password prompt will appear to authorize the install — no terminal needed.

### 🧩 Extensions (UserScripts)
- Load Greasemonkey/Tampermonkey-compatible `.user.js` scripts from `~/.config/SwordFish/extensions/`
- **Install from Tools Hub** — paste a Greasy Fork page URL or any `.user.js` direct link in the install bar; auto-downloads, validates, and activates immediately
- **Install from manager** — Ctrl+Shift+E → 🌐 Install from URL
- Toggle scripts on/off per-session, reload all without restarting

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

### 🐧 Linux — Quick install (.deb)

```bash
# Download and install
wget https://github.com/BayazidHabibSiddikee/SwordFish/raw/main/dist/swordfish-2.0.0-Linux.deb
sudo dpkg -i swordfish-2.0.0-Linux.deb

# If dependencies are missing:
sudo apt-get install -f
```

Launch from the app menu or run `SwordFish` in the terminal.

### 🐧 Linux — Build from source

**1. Clone and build:**
```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFish.git
cd SwordFish
chmod +x install_cpp.sh
./install_cpp.sh
```
This installs Qt6, CMake, and build tools, then compiles the binary.

**2. Install the binary:**
```bash
mkdir -p ~/.local/bin
cp build/SwordFish ~/.local/bin/SwordFish

# Desktop entry and icon
mkdir -p ~/.local/share/applications ~/.local/share/icons/hicolor/256x256/apps
cp icon.png ~/.local/share/icons/hicolor/256x256/apps/swordfish.png
cat > ~/.local/share/applications/swordfish.desktop << EOF
[Desktop Entry]
Version=2.0
Type=Application
Name=SwordFish
Exec=$HOME/.local/bin/SwordFish %U
Icon=swordfish
Terminal=false
Categories=Network;WebBrowser;Qt;
EOF
update-desktop-database ~/.local/share/applications
```

**After updating, rebuild and redeploy:**
```bash
git pull && cmake --build build --parallel $(nproc) && cp build/SwordFish ~/.local/bin/SwordFish
```

### 🪟 Windows — Build from source

> A pre-built `.exe` installer requires building on Windows (Qt WebEngine cannot be cross-compiled from Linux).

```cmd
REM In an Administrator Command Prompt:
requirements.bat

REM Then in a Qt6 MSVC Developer Prompt:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && cpack -G NSIS
REM → produces SwordFish-2.0.0-win64.exe in dist/
```

---

## 📦 Optional Tool Dependencies

These are **not needed to run the browser** — only needed for specific tools. Install them from the Tools Hub or manually:

| Tools | Package | Install |
|---|---|---|
| PDF Merge / Split | `qpdf` | `sudo apt install qpdf` |
| Word / Excel / PPTX ↔ PDF | `libreoffice` | `sudo apt install libreoffice` |
| PDF → Image / Text | `poppler-utils` | `sudo apt install poppler-utils` |
| Text → PDF | `enscript` + `ghostscript` | `sudo apt install enscript ghostscript` |
| 7-Zip archives | `p7zip-full` | `sudo apt install p7zip-full` |
| QR Code Generator | `qrencode` | `sudo apt install qrencode` |
| Media Download + YT Transcript | `yt-dlp` | `sudo apt install yt-dlp` |
| Translator | `python3` + `deep-translator` | `sudo apt install python3 python3-pip` |

Or just open **🔧 Tools → Tools Hub → 📦 Optional Dependencies** and click Install.

---

## 📁 Project Structure

```
SwordFish/
├── src/
│   ├── main.cpp                 # Entry point, DNS-over-HTTPS init
│   ├── mainwindow.cpp/.h        # Main window, tabs, menus, ToolsBackend
│   ├── adblocker.cpp/.h         # Network-level ad/tracker blocking
│   ├── extension_system.cpp/.h  # UserScript loader + install-from-URL
│   ├── password_manager.cpp/.h  # AES-256 password vault
│   ├── file_picker.cpp/.h       # Home-restricted file picker
│   ├── folder_picker.cpp/.h     # Home-restricted folder picker
│   ├── media_bar.cpp/.h         # Media playback controls
│   ├── reading_mode.cpp/.h      # Reader mode
│   ├── pip_window.cpp/.h        # Picture-in-Picture
│   ├── sync_manager.cpp/.h      # Bookmark/history sync
│   ├── web_page.cpp/.h          # Custom QWebEnginePage
│   ├── styles.cpp/.h            # One Dark theme stylesheet
│   ├── tools/                   # PDF, Office, Archive, Student tool wrappers
│   ├── tools.html               # Tools Hub UI (Qt resource, QWebChannel)
│   ├── qrcode.png               # Donate QR (bundled in resources)
│   └── resources.qrc            # Qt resource bundle
├── dist/
│   ├── swordfish-2.0.0-Linux.deb    # Debian/Ubuntu installer
│   └── swordfish-2.0.0-Linux.tar.gz # Portable archive
├── packaging/linux/             # .desktop file, man page
├── CMakeLists.txt
├── install_cpp.sh               # Dependency installer + build script
├── build_packages.sh            # Build all Linux packages (.deb/.rpm/AppImage)
└── package_windows.sh           # Windows build + NSIS installer script
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
| C++ compiler | C++17 (GCC 10+, Clang 12+, MSVC 2019+) |
| Qt6 | Core, Gui, Widgets, Network, WebEngineCore, WebEngineWidgets, WebChannel |

---

*Built for power users who demand privacy and productivity.*
