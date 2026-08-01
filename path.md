# SwordFish Browser — Project Map & Issue Tracker

> Each part has a status: ✅ Done | 🔧 In Progress | ❌ Broken | ⚠️ Known Issue

---

## Part 1 — Core Browser Engine
**Files:** `src/mainwindow.h`, `src/mainwindow.cpp`, `src/web_page.h`, `src/web_page.cpp`, `src/main.cpp`

### Subparts
- **1.1 Tab Management** — newTab, closeTab, pinTab, muteTab, detachTab, moveTab, reopenLastTab ✅
- **1.2 Navigation** — back/forward/reload/home, URL bar, history recording ✅
- **1.3 Find in Page** — Ctrl+F bar, findNext/findPrev, status counter ✅
- **1.4 Fullscreen** — F11 toggle, navbar hide/show ✅
- **1.5 Zoom** — per-host zoom, Ctrl+/- /0, persisted in memory ✅
- **1.6 Tab Restore** — save/restore tabs on close/open, closed-tab reopen ✅
- **1.7 Keyboard Shortcuts** — full shortcut table in setupShortcuts() ✅

### Known Issues
- ✅ **1.A** Fixed: duplicate includes removed from mainwindow.h
- ✅ **1.B** Fixed: password capture injected via `loadFinished` in every newTab()
- ✅ **1.C** Fixed: AdBlocker wired to profile via `setUrlRequestInterceptor()`
- ✅ **1.D** Fixed: adblock level menu reads `getBlocker().level()` for correct checkbox

---

## Part 2 — Ad Blocker
**Files:** `src/adblocker.h`, `src/adblocker.cpp`

### Subparts
- **2.1 Domain/path matching** — static domain list + regex path patterns ✅
- **2.2 JS injection blocker** — CSS selector removal + skip-ad auto-click ✅
- **2.3 Network interceptor** — extends `QWebEngineUrlRequestInterceptor`, wired to profile ✅

---

## Part 3 — Dark Mode
**Files:** `src/mainwindow.cpp` — `injectDarkMode()`, `removeDarkMode()`

### Subparts
- **3.1 CSS stylesheet injection** — DocumentCreation script, One Dark palette ✅
- **3.2 YouTube-specific selectors** — masthead, sidebar, comments, related videos ✅
- **3.3 Shorts fix** — pure JS inline style reset via `el.style[prop] = ''` ✅
- **3.4 Video/image exclusion** — `video, img, canvas` excluded from filter ✅
- **3.5 SPA re-injection** — throttled MutationObserver (200ms debounce) ✅

### Known Issues
- ⚠️ **3.A** Shorts fix needs runtime verification on actual YouTube Shorts page

---

## Part 4 — Password Manager
**Files:** `src/password_manager.h`, `src/password_manager.cpp`

### Subparts
- **4.1 Credential store** — XOR-obfuscated JSON at `~/.config/SwordFish/passwords.json` ✅
- **4.2 Autofill** — JS fills username+password fields, fires input/change events ✅
- **4.3 Form capture** — JS listens for submit, signals `credentialCaptured` ✅
- **4.4 Manager dialog** — table view, show/hide password, add, delete ✅
- **4.5 Capture on every tab** — injected on `loadFinished` in every newTab() ✅

### Known Issues
- ⚠️ **4.A** XOR obfuscation is not real security — acceptable for local use

---

## Part 5 — Extension System
**Files:** `src/extension_system.h`, `src/extension_system.cpp`

### Subparts
- **5.1 Script discovery** — scans `~/.config/SwordFish/extensions/*.js` ✅
- **5.2 Metadata parsing** — reads `// @name` and `// @match` headers ✅
- **5.3 Script injection** — wrapped in JS URL guard that enforces `@match` ✅
- **5.4 Manager dialog** — list view, toggle on/off, reload all, open folder ✅

---

## Part 6 — Sync Manager
**Files:** `src/sync_manager.h`, `src/sync_manager.cpp`

### Subparts
- **6.1 Export** — bookmarks + history to JSON with version + timestamp ✅
- **6.2 Import/merge** — deduplicates by URL ✅
- **6.3 File watcher** — QFileSystemWatcher signals `syncFileChanged` ✅
- **6.4 Sync dialog** — export, import, watch/stop UI ✅

---

## Part 7 — Media Controls Bar
**Files:** `src/media_bar.h`, `src/media_bar.cpp`

### Subparts
- **7.1 Controls** — play/pause, stop, ±10s skip, mute button ✅
- **7.2 Sliders** — seek slider (0–1000), volume slider (0–100) ✅
- **7.3 JS bridge** — polls `document.querySelector('video')` every 1s ✅
- **7.4 PiP trigger** — emits `pipRequested()` signal ✅
- **7.5 Tab switch re-attach** — re-attaches on `currentChanged` ✅

---

## Part 8 — Picture-in-Picture
**Files:** `src/pip_window.h`, `src/pip_window.cpp`

### Subparts
- **8.1 Floating window** — frameless, always-on-top, draggable by title area ✅
- **8.2 Size toggle** — small (320×200) / large (640×400) ✅
- **8.3 Native PiP trigger** — JS `requestPictureInPicture()` on first video ✅
- **8.4 Lifecycle** — `WA_DeleteOnClose` + `deleteLater()`, no parent ✅

---

## Part 9 — Reading Mode
**Files:** `src/reading_mode.h`, `src/reading_mode.cpp`

### Subparts
- **9.1 Content extraction** — tries semantic tags, falls back to largest div ✅
- **9.2 Reader HTML** — clean dark serif layout, exit button ✅
- **9.3 Restore** — re-writes original HTML on exit ✅
- **9.4 Exit detection** — C++ connects `titleChanged` to detect `__sf_exit_reader__` ✅

---

## Part 10 — Tools
**Files:** `src/tools/*.cpp`, `src/tools/*.h`

### Subparts
- **10.1 PDF Tools** — merge, split ✅
- **10.2 Doc Tools** — DOCX↔PDF, image→PDF, text→PDF ✅
- **10.3 Office Tools** — XLSX↔PDF, CSV↔XLSX, PPTX↔PDF, PDF→image/text ✅
- **10.4 Archive Tools** — zip/7z/tar create+extract ✅
- **10.5 Student Tools** — calculator, unit converter, QR generator, note taker ✅
- **10.6 Translate** — Google Translate URL bridge ✅
- **10.7 YouTube Transcript** — yt-dlp subtitle fetch ✅

### Known Issues
- ⚠️ **10.A** Tool dialogs don't show error message when external binary (libreoffice, pdftk, etc.) is missing — fails silently

---

## Part 11 — Styles & Themes
**Files:** `src/styles.h`, `src/styles.cpp`

### Subparts
- **11.1 Light theme** ✅
- **11.2 Dark theme (One Dark)** ✅

---

## Part 12 — File & Folder Pickers
**Files:** `src/file_picker.h/.cpp`, `src/folder_picker.h/.cpp`

### Subparts
- **12.1 FilePicker** — custom styled open/save/multi-file dialogs ✅
- **12.2 FolderPicker** — custom styled directory browser ✅

---

## Part 13 — Build & Packaging
**Files:** `CMakeLists.txt`, `packaging/`, `package_appimage.sh`, `build_packages.sh`

### Subparts
- **13.1 CMake build** ✅
- **13.2 Linux .deb/.rpm** ✅
- **13.3 AppImage** ✅
- **13.4 Windows .exe** ✅

---

## Fix Queue

| ID   | Part | Severity | Status | Description |
|------|------|----------|--------|-------------|
| 1.A  | 1    | Low      | ✅ Fixed | Duplicate includes in mainwindow.h removed |
| 1.B  | 4    | Medium   | ✅ Fixed | Password capture injected on `loadFinished` in every new tab |
| 1.C  | 2    | High     | ✅ Fixed | AdBlocker wired to profile — network blocking now works |
| 1.D  | 1    | Medium   | ✅ Fixed | Adblock level checkbox reads actual `getBlocker().level()` |
| 2.A  | 2    | High     | ✅ Fixed | AdBlocker extends `QWebEngineUrlRequestInterceptor` + `interceptRequest()` |
| 3.A  | 3    | High     | ✅ Fixed | Shorts: pure JS `el.style[prop]=''` inline reset bypasses `!important` CSS |
| 3.B  | 3    | Low      | ✅ Fixed | MutationObserver now uses 200ms debounce timer instead of rAF |
| 5.A  | 5    | Medium   | ✅ Fixed | `@match` enforced via JS URL guard wrapping injected scripts |
| 7.A  | 7    | Medium   | ✅ Fixed | Media bar re-attaches to active tab on `currentChanged` |
| 8.A  | 8    | Low      | ✅ Fixed | PiP uses `deleteLater()` + `WA_DeleteOnClose` |
| 9.A  | 9    | Medium   | ✅ Fixed | Reading mode exit detected via `titleChanged` on C++ side |
| 10.A | 10   | Medium   | ✅ Fixed | Tool dialogs: `runTool()` helper wraps all calls in try/catch, shows error dialog with install hint |

---

## Next Steps

- Verify Shorts fix at runtime (3.A)
- Tool dialogs: show friendly error when libreoffice/pdftk/etc. not installed (10.A)
- Consider adding per-tab zoom persistence to settings file
- Consider password store encryption upgrade (AES via OpenSSL or Qt Keychain)
