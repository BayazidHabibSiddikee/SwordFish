# 🗡️ SwordFish Web Browser

![SwordFish Logo](icon.png)

SwordFish is a **privacy-first, power-user browser** built with **C++ and Qt6/WebEngine**. Designed for researchers, developers, and students who need a high-performance browsing experience with a suite of 20+ integrated productivity tools — all built in, no extensions required.

> **v2.0** — Rewritten from Python/PySide6 to C++/Qt6 for native performance, lower memory usage, and a proper cross-platform packaging pipeline.

---

## ⬇️ Download

| Platform | File | Size |
|---|---|---|
| **Linux** (.deb) — Debian / Ubuntu | [Releases page](https://github.com/BayazidHabibSiddikee/SwordFish/releases) | ~14 MB |
| **Windows** (.exe) — Windows 10/11 | [Releases page](https://github.com/BayazidHabibSiddikee/SwordFish/releases) | ~99 MB |

> The Windows installer includes all Qt6 DLLs — no separate Qt install needed.

---

## 🚀 Features

### 🌐 Browsing
| Feature | Details |
|---|---|
| Multi-tab | Tab pinning, muting, drag-to-reorder, Ctrl+T/W/Tab shortcuts |
| Private Mode | Ctrl+Shift+N — no history, no cookies saved |
| Adblocker | 4 levels: None / Low / Medium / Ultimate — **persists across restarts** |
| DNS-over-HTTPS | AdGuard by default — applied before network init, switchable in Settings |
| YouTube Shorts | Fully blocked — URLs redirected, shelf and sidebar link hidden |
| Media Downloader | Built-in yt-dlp — video up to 4K, audio as MP3/M4A/OGG |
| Reading Mode | Strips ads/clutter from articles for clean reading |
| Picture-in-Picture | Float any video in a resizable overlay window |
| Media Controls Bar | Play/pause, seek, volume for any page video |
| English-Only Mode | Force English locale on all web content |

### 🛠️ Tools Hub
Open via **🔧 Tools menu** or address bar → type `qrc:///tools.html`.

All tools are built-in dialogs — no browser extensions needed.

#### 📄 PDF & Documents
| Tool | What it does | Requires |
|---|---|---|
| Merge PDFs | Combine multiple PDFs into one | `qpdf` |
| Split PDF | Extract pages to separate files | `qpdf` |
| Word → PDF | Convert .docx to PDF | `libreoffice` |
| PDF → Word | Convert PDF to editable .docx | `libreoffice` |
| Excel → PDF | Convert .xlsx to PDF | `libreoffice` |
| PDF → Excel | Convert PDF tables to .xlsx | `libreoffice` |
| CSV → Excel | Convert .csv to .xlsx | `libreoffice` |
| Excel → CSV | Convert .xlsx to .csv | `libreoffice` |
| PPTX → PDF | Convert PowerPoint to PDF | `libreoffice` |
| PDF → PPTX | Convert PDF to PowerPoint | `libreoffice` |
| Image → PDF | Convert images to PDF | `libreoffice` |
| PDF → Image | Convert PDF pages to images | `poppler-utils` |
| Text → PDF | Convert plain text file to PDF | `enscript` + `ghostscript` |
| PDF → Text | Extract all text from a PDF | `poppler-utils` |

#### 🌐 Language & Media
| Tool | What it does | Requires |
|---|---|---|
| Translator | Translate text between languages | `python3` + `deep-translator` |
| YouTube Transcript | Fetch full transcript of any YouTube video | `yt-dlp` |

#### 🛠️ Utilities
| Tool | What it does | Requires |
|---|---|---|
| Calculator | Standard + scientific calculator | Built-in |
| Unit Converter | Length, weight, temperature, and more | Built-in |
| Programmer Calc | Bin / Hex / Oct / Dec converter | Built-in |
| Archive Tools | Create and extract Zip, 7z, Tar archives | `p7zip-full` (for 7z) |
| Timer | Countdown timer with alarm | Built-in |
| QR Generator | Generate QR codes from any text or URL | `qrencode` |
| Weather | Current weather by city name | Built-in (Open-Meteo API) |
| Note Taker | Quickly write and save notes | Built-in |
| Terminal | Launch your system terminal | Built-in |

#### 📦 Optional Dependencies
The Tools Hub has a **built-in dependency manager** at the top of the page. It automatically checks which optional tools are installed and shows an **⬇️ Install** button next to anything missing. Clicking Install triggers a GUI password prompt — no terminal needed.

### 🧩 Extensions (UserScripts)
SwordFish supports Greasemonkey/Tampermonkey-compatible `.user.js` scripts.

**Install from Tools Hub (easiest):**
1. Open **🔧 Tools → Tools Hub**
2. Scroll to **🧩 Install UserScript Extension**
3. Paste a [Greasy Fork](https://greasyfork.org/) page URL or any `.user.js` direct link
4. Press Enter or click **⬇️ Install**

**Install from Extensions Manager:**
- Press `Ctrl+Shift+E` → click **🌐 Install from URL**

**Manual install:**
- Copy any `.user.js` file to `~/.config/SwordFish/extensions/`
- Press `Ctrl+Shift+E` → click **⟳ Reload All**

Scripts support `// @name` and `// @match` metadata. Toggle individual scripts on/off without restarting.

### 🔐 Password Manager
- Automatically captures login forms when you sign in
- Offers to auto-fill on your next visit
- AES-256 encrypted local storage — nothing leaves your machine
- Open with `Ctrl+Shift+P`

### 🛡️ Privacy & Security
- **No telemetry** — zero data sent to any external service
- **DNS-over-HTTPS** — encrypted DNS queries (AdGuard default)
- **Ad/tracker blocking** — network-level + CSS/JS injection blocking
- **Cookie management** — view and clear cookies per site
- **MAC address spoofing** — Linux only, via Settings
- **Proxy support** — HTTP/SOCKS proxy with per-session toggle
- **File picker restriction** — can only browse your home directory (`~/`)

---

## 📦 Installation

### 🐧 Linux — Install from .deb (recommended)

```bash
# 1. Download from the Releases page
wget https://github.com/BayazidHabibSiddikee/SwordFish/releases/download/v2.0.0/swordfish-2.0.0-Linux.deb

# 2. Install
sudo dpkg -i swordfish-2.0.0-Linux.deb

# 3. Fix any missing Qt dependencies if needed
sudo apt-get install -f
```

Launch from your app menu or run `SwordFish` in the terminal.

---

### 🐧 Linux — Build from source

**Requirements:** CMake ≥ 3.20, GCC 10+ or Clang 12+, Qt6

**1. Clone the repo:**
```bash
git clone https://github.com/BayazidHabibSiddikee/SwordFish.git
cd SwordFish
```

**2. Install build dependencies:**
```bash
chmod +x install_cpp.sh
./install_cpp.sh
```
This installs Qt6, CMake, g++, and all optional runtime tools (LibreOffice, poppler, qpdf, p7zip, qrencode, enscript, ghostscript, yt-dlp).

**3. Install the binary:**
```bash
mkdir -p ~/.local/bin
cp build/SwordFish ~/.local/bin/SwordFish
```

**4. Create the desktop entry (app menu icon):**
```bash
mkdir -p ~/.local/share/applications \
         ~/.local/share/icons/hicolor/256x256/apps \
         ~/.local/share/pixmaps

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

**5. Optional — add a terminal alias:**
```bash
echo 'alias swordfish="~/.local/bin/SwordFish &"' >> ~/.bashrc
source ~/.bashrc
```

Now run with `swordfish` or from the app menu.

**After a git pull, rebuild and redeploy in one line:**
```bash
git pull && cmake --build build --parallel $(nproc) && cp build/SwordFish ~/.local/bin/SwordFish
```

---

### 🪟 Windows — Install from .exe (recommended)

1. Download `swordfish-2.0.0-win64.exe` from the [Releases page](https://github.com/BayazidHabibSiddikee/SwordFish/releases)
2. Run the installer — it will create a Start Menu entry and desktop shortcut
3. Launch **SwordFish** from the Start Menu

> The installer includes all required Qt6 DLLs. No separate Qt or Visual C++ install needed.

---

### 🪟 Windows — Build from source

**Requirements:** Windows 10/11, winget, Qt6 MSVC

**1. Install build dependencies** (run as Administrator):
```cmd
requirements.bat
```
Installs CMake, Qt6 (MSVC x64), Visual Studio Build Tools, and NSIS via winget.

**2. Build** (in a Qt6 MSVC x64 Developer Command Prompt):
```cmd
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

**3. Package as .exe installer:**
```cmd
cd build
cpack -G NSIS
```
Produces `SwordFish-2.0.0-win64.exe`.

---

## 📦 Optional Tool Dependencies

Install only what you need. The Tools Hub will prompt you automatically.

| Tool group | Package | Install command |
|---|---|---|
| PDF Merge / Split | `qpdf` | `sudo apt install qpdf` |
| Word / Excel / PPTX ↔ PDF | `libreoffice` | `sudo apt install libreoffice` |
| PDF → Image / Text | `poppler-utils` | `sudo apt install poppler-utils` |
| Text → PDF | `enscript` + `ghostscript` | `sudo apt install enscript ghostscript` |
| 7-Zip archives | `p7zip-full` | `sudo apt install p7zip-full` |
| QR Code Generator | `qrencode` | `sudo apt install qrencode` |
| Media Download + YT Transcript | `yt-dlp` | `sudo apt install yt-dlp` |
| Translator | `python3` + `deep-translator` | `sudo apt install python3 python3-pip && pip install deep-translator` |

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+Tab` | Next tab |
| `Ctrl+Shift+Tab` | Previous tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+R` | Reload page |
| `Ctrl+F` | Find in page |
| `F11` | Fullscreen |
| `Ctrl+D` | Bookmark page |
| `Ctrl+H` | History |
| `Ctrl+Shift+P` | Password Manager |
| `Ctrl+Shift+E` | Extensions Manager |
| `Ctrl+Shift+N` | New private window |
| `Ctrl++` / `Ctrl+-` | Zoom in / out |
| `Ctrl+0` | Reset zoom |
| `Alt+Left` | Back |
| `Alt+Right` | Forward |

---

## 📁 Project Structure

```
SwordFish/
├── src/
│   ├── main.cpp                 # Entry point, DNS-over-HTTPS init (before QApplication)
│   ├── mainwindow.cpp/.h        # Main window, tabs, menus, ToolsBackend
│   ├── adblocker.cpp/.h         # Network-level ad/tracker blocking
│   ├── extension_system.cpp/.h  # UserScript loader + install-from-URL
│   ├── password_manager.cpp/.h  # Password vault
│   ├── file_picker.cpp/.h       # Home-restricted file picker
│   ├── folder_picker.cpp/.h     # Home-restricted folder picker
│   ├── media_bar.cpp/.h         # Media playback controls bar
│   ├── reading_mode.cpp/.h      # Reader mode
│   ├── pip_window.cpp/.h        # Picture-in-Picture window
│   ├── sync_manager.cpp/.h      # Bookmark/history sync
│   ├── web_page.cpp/.h          # Custom QWebEnginePage
│   ├── styles.cpp/.h            # One Dark theme stylesheet
│   ├── tools/                   # PDF, Office, Archive, Student tool wrappers
│   ├── tools.html               # Tools Hub UI (Qt resource, QWebChannel)
│   ├── qrcode.png               # Donate QR (bundled in resources)
│   └── resources.qrc            # Qt resource bundle
├── utils/                       # Network, proxy, TTS utilities
├── packaging/
│   ├── linux/                   # .desktop file, man page
│   └── windows/                 # .rc file, .ico icon
├── CMakeLists.txt               # Build system
├── install_cpp.sh               # Linux: install deps + build
├── requirements.bat             # Windows: install build deps via winget
├── build_packages.sh            # Build all Linux packages (.deb/.rpm/AppImage)
└── package_windows.sh           # Windows NSIS installer script
```

---

## 🔧 Build Requirements

| Requirement | Linux | Windows |
|---|---|---|
| CMake | ≥ 3.20 | ≥ 3.20 |
| C++ compiler | GCC 10+ or Clang 12+ | MSVC 2019+ |
| Qt6 | Core, Gui, Widgets, Network, WebEngineCore, WebEngineWidgets, WebChannel | Same |
| libcurl | `libcurl4-openssl-dev` | Bundled |

---

## ❓ FAQ

**Q: The Tools Hub shows tools as "not installed" — how do I install them?**
Open **🔧 Tools → Tools Hub**, scroll to **📦 Optional Dependencies**, and click **⬇️ Install** next to the group you need. A password prompt will appear.

**Q: How do I install a userscript from Greasy Fork?**
In the Tools Hub, paste the Greasy Fork page URL (e.g. `https://greasyfork.org/en/scripts/12345-name`) into the **🧩 Install UserScript Extension** box and press Enter. No need to find the raw .user.js link.

**Q: My adblock level resets every time I restart.**
This was a bug in older builds. Update to v2.0.0 — adblock level is now saved to settings and restored on startup.

**Q: DNS-over-HTTPS doesn't seem to be working.**
This was a bug in older builds. The fix in v2.0.0 applies DoH before the browser network stack initializes. Make sure you're running v2.0.0+.

**Q: Can I install Chrome/Firefox extensions?**
No — Qt WebEngine doesn't support the Chrome Extension API or Firefox addon format. SwordFish uses Greasemonkey-compatible `.user.js` userscripts instead. Most popular extensions (ad blockers, dark mode, etc.) have userscript equivalents on [Greasy Fork](https://greasyfork.org/).

**Q: Where is my data stored?**
All data is stored locally in `~/.config/SwordFish/` on Linux and `%APPDATA%\SwordFish\` on Windows. Nothing is sent to external servers.

---

## 🤝 Contributing

1. Fork the repo
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Build and test locally: `cmake --build build --parallel $(nproc)`
4. Commit and push: `git push origin feature/my-feature`
5. Open a Pull Request

---

*Built for power users who demand privacy and productivity.*
