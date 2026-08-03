# SwordFish Browser v2.0 — Project Map & Status

> Status legend: ✅ Done | 🔧 In Progress | ⚠️ Known Issue | ❌ Broken

---

## Part 1 — Core Browser Engine
**Files:** `src/main.cpp`, `src/mainwindow.h/.cpp`, `src/web_page.h/.cpp`

- **1.1** Tab Management — newTab, closeTab, pinTab, muteTab, reopen closed tab ✅
- **1.2** Navigation — back/forward/reload/home, URL bar, history ✅
- **1.3** Find in Page — Ctrl+F, findNext/findPrev ✅
- **1.4** Fullscreen — F11 toggle ✅
- **1.5** Zoom — per-host zoom, Ctrl+/-/0 ✅
- **1.6** Tab Restore — save/restore on close/open, skip stale file:// URLs ✅
- **1.7** Keyboard Shortcuts — full table in setupShortcuts() ✅
- **1.8** YouTube Shorts blocked — URL redirect + CSS hiders ✅
- **1.9** QWebChannel race fix — setWebChannel() before loadUrl() in newTab() ✅

---

## Part 2 — Ad Blocker
**Files:** `src/adblocker.h/.cpp`

- **2.1** Domain/path matching — static domain list + regex ✅
- **2.2** JS injection — CSS selector removal + skip-ad auto-click ✅
- **2.3** Network interceptor — QWebEngineUrlRequestInterceptor ✅
- **2.4** Persistence — adblock level saved/loaded via QSettings ✅

---

## Part 3 — DNS-over-HTTPS
**Files:** `src/main.cpp`

- **3.1** Applied before QApplication (before Chromium network init) ✅
- **3.2** Provider saved/loaded from QSettings ✅
- **3.3** Switchable: AdGuard / Cloudflare / NextDNS / Google / System ✅

---

## Part 4 — Dark Mode
**Files:** `src/mainwindow.cpp` — `injectDarkMode()`, `removeDarkMode()`

- **4.1** CSS injection — DocumentCreation script, One Dark palette ✅
- **4.2** YouTube selectors — masthead, sidebar, comments, related ✅
- **4.3** Shorts removed — no longer needs exclusion guards ✅
- **4.4** SPA re-injection — MutationObserver with 200ms debounce ✅

---

## Part 5 — Tools Hub
**Files:** `src/tools.html`, `src/resources.qrc`, `src/mainwindow.h/.cpp` (ToolsBackend)

- **5.1** Embedded as Qt resource (qrc://) — QWebChannel works ✅
- **5.2** ToolsBackend class — Q_INVOKABLE run_tool(), install_extension(), check_deps(), install_deps() ✅
- **5.3** Dark/light mode — CSS custom properties, prefers-color-scheme ✅
- **5.4** QR donate image — bundled in resources.qrc ✅
- **5.5** Extension install bar — paste URL, press Enter or Install ✅
- **5.6** Optional Dependencies section — per-group check + install via pkexec ✅

---

## Part 6 — Password Manager
**Files:** `src/password_manager.h/.cpp`

- **6.1** Credential store — XOR-obfuscated JSON ✅
- **6.2** Autofill — JS fills fields, fires input/change events ✅
- **6.3** Form capture — JS submit listener → credentialCaptured signal ✅
- **6.4** Manager dialog — table view, show/hide, add, delete ✅

---

## Part 7 — Extension System (UserScripts)
**Files:** `src/extension_system.h/.cpp`

- **7.1** Script discovery — `~/.config/SwordFish/extensions/*.js` ✅
- **7.2** Metadata parsing — `// @name`, `// @match` ✅
- **7.3** Script injection — JS URL guard enforces @match ✅
- **7.4** Manager dialog — toggle, reload, open folder ✅
- **7.5** Install from URL — Greasy Fork page URL or raw .user.js link ✅
- **7.6** Install from Tools Hub — no dialog needed ✅

---

## Part 8 — Sync Manager
**Files:** `src/sync_manager.h/.cpp`

- **8.1** Export — bookmarks + history to JSON ✅
- **8.2** Import/merge — deduplicates by URL ✅
- **8.3** File watcher — QFileSystemWatcher ✅

---

## Part 9 — Media Controls Bar
**Files:** `src/media_bar.h/.cpp`

- **9.1** Controls — play/pause, ±10s skip, mute, seek/volume sliders ✅
- **9.2** JS bridge — polls video element every 1s ✅
- **9.3** Re-attaches on tab switch ✅

---

## Part 10 — File & Folder Pickers
**Files:** `src/file_picker.h/.cpp`, `src/folder_picker.h/.cpp`

- **10.1** Custom styled open/save/multi-file dialogs ✅
- **10.2** Home directory restriction — navigateTo/Up clamped to ~/ ✅
- **10.3** Places sidebar — Drives section removed, only home subdirs ✅
- **10.4** confirmSelection() — final guard strips paths outside ~/ ✅

---

## Part 11 — Tools (External CLI wrappers)
**Files:** `src/tools/*.h/.cpp`

| Tool | Binary required | Status |
|---|---|---|
| PDF Merge/Split | `qpdf` | ✅ |
| Word/PPTX/Excel ↔ PDF | `libreoffice` | ✅ |
| PDF → Image/Text | `pdftotext`, `pdftoppm` (poppler-utils) | ✅ |
| Text → PDF | `enscript`, `gs` (ghostscript) | ✅ |
| Archive (7z) | `7z` (p7zip-full) | ✅ |
| QR Generator | `qrencode` | ✅ |
| Translator | `python3` + `deep-translator` | ✅ |
| YouTube Transcript | `yt-dlp` | ✅ |
| Media Download | `yt-dlp` | ✅ |

---

## Part 12 — Build & Packaging
**Files:** `CMakeLists.txt`, `install_cpp.sh`, `build_packages.sh`, `package_windows.sh`

| Format | Command | Status |
|---|---|---|
| Linux binary | `cmake --build build` | ✅ |
| Linux .deb | `cpack -G DEB` | ✅ → `dist/swordfish-2.0.0-Linux.deb` |
| Linux .tar.gz | `cpack -G TGZ` | ✅ → `dist/swordfish-2.0.0-Linux.tar.gz` |
| Linux .rpm | `cpack -G RPM` | ✅ (requires rpmbuild) |
| Linux AppImage | `./package_appimage.sh` | ✅ (requires linuxdeploy) |
| Windows .exe | `cpack -G NSIS` on Windows | ✅ (requires Qt6 + NSIS on Windows) |

---

## Known Issues / Next Steps

| ID | Description | Status |
|----|-------------|--------|
| W1 | Windows .exe requires building natively on Windows — Qt WebEngine can't be cross-compiled from Linux | ⚠️ by design |
| S1 | Password store uses XOR obfuscation — acceptable for local use, not cryptographically strong | ⚠️ known |
