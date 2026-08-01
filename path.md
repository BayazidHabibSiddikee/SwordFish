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
- ⚠️ **1.A** `mainwindow.h` has 6 duplicate `#include` lines (pip_window, password_manager, extension_system, sync_manager, media_bar, reading_mode included twice) — harmless due to `#pragma once` but messy
- ⚠️ **1.B** `newTab()` does not inject password capture on the newly created tab — only fires on `currentChanged` signal, so the first tab opened at startup gets no capture
- ⚠️ **1.C** AdBlocker interceptor at line 192 is created but **never wired to `m_profile`** via `setUrlRequestInterceptor()` — network-level blocking is completely non-functional (only the JS-injected CSS blocker works)
- ⚠️ **1.D** Adblock level in Settings menu always shows "LOW" as checked regardless of actual level — `currentLevel` is hardcoded to `"low"`, not read from `getBlocker().level()`

---

## Part 2 — Ad Blocker
**Files:** `src/adblocker.h`, `src/adblocker.cpp`

### Subparts
- **2.1 Domain/path matching** — static domain list + regex path patterns ✅
- **2.2 JS injection blocker** — CSS selector removal + skip-ad auto-click (in mainwindow.cpp injectAdblock) ✅
- **2.3 Network interceptor** — shouldBlock() method exists but NOT connected to profile ❌

### Known Issues
- ❌ **2.A** `AdBlocker` extends nothing — it needs to extend `QWebEngineUrlRequestInterceptor` and be set on the profile with `m_profile->setUrlRequestInterceptor()` for network-level blocking to work

---

## Part 3 — Dark Mode
**Files:** `src/mainwindow.cpp` — `injectDarkMode()`, `removeDarkMode()`

### Subparts
- **3.1 CSS stylesheet injection** — DocumentCreation script, One Dark palette ✅
- **3.2 YouTube-specific selectors** — masthead, sidebar, comments, related videos ✅
- **3.3 Shorts fix** — second stylesheet + JS fixer with URL polling 🔧
- **3.4 Video/image exclusion** — `video, img, canvas` excluded from filter ✅
- **3.5 SPA re-injection** — MutationObserver re-appends style on navigation ✅

### Known Issues
- 🔧 **3.A** Shorts still broken — `span, div` removed from broad rule (✅ fixed in last commit), Shorts reset CSS block present, JS fixer present — needs runtime verification
- ⚠️ **3.B** `subtree: true` on MutationObserver may cause performance issues on content-heavy pages — consider throttling more aggressively

---

## Part 4 — Password Manager
**Files:** `src/password_manager.h`, `src/password_manager.cpp`

### Subparts
- **4.1 Credential store** — XOR-obfuscated JSON at `~/.config/SwordFish/passwords.json` ✅
- **4.2 Autofill** — JS injection fills username+password fields, fires input/change events ✅
- **4.3 Form capture** — JS listens for submit, signals `credentialCaptured` ✅
- **4.4 Manager dialog** — table view, show/hide password, add, delete ✅

### Known Issues
- ⚠️ **4.A** `injectCapture()` is only called on `currentChanged` — tabs opened at startup don't get capture until you switch away and back
- ⚠️ **4.B** XOR obfuscation is not real security — passwords are trivially reversible. Acceptable for a local browser but worth noting.

---

## Part 5 — Extension System
**Files:** `src/extension_system.h`, `src/extension_system.cpp`

### Subparts
- **5.1 Script discovery** — scans `~/.config/SwordFish/extensions/*.js` ✅
- **5.2 Metadata parsing** — reads `// @name` and `// @match` headers ✅
- **5.3 Script injection** — injects into QWebEngineProfile scripts collection ✅
- **5.4 Manager dialog** — list view, toggle on/off, reload all, open folder ✅

### Known Issues
- ⚠️ **5.A** `@match` pattern is parsed but not enforced — all enabled scripts run on every page regardless of match pattern

---

## Part 6 — Sync Manager
**Files:** `src/sync_manager.h`, `src/sync_manager.cpp`

### Subparts
- **6.1 Export** — bookmarks + history to JSON with version + timestamp ✅
- **6.2 Import/merge** — deduplicates by URL ✅
- **6.3 File watcher** — QFileSystemWatcher signals `syncFileChanged` ✅
- **6.4 Sync dialog** — export, import, watch/stop UI ✅

### Known Issues
- None critical ✅

---

## Part 7 — Media Controls Bar
**Files:** `src/media_bar.h`, `src/media_bar.cpp`

### Subparts
- **7.1 Controls** — play/pause, stop, ±10s skip, mute button ✅
- **7.2 Sliders** — seek slider (0–1000), volume slider (0–100) ✅
- **7.3 JS bridge** — polls `document.querySelector('video')` every 1s ✅
- **7.4 PiP trigger** — emits `pipRequested()` signal ✅

### Known Issues
- ⚠️ **7.A** Media bar is attached to current browser on toggle — if user switches tabs, bar still polls old tab's video. Should re-attach on `currentChanged`.

---

## Part 8 — Picture-in-Picture
**Files:** `src/pip_window.h`, `src/pip_window.cpp`

### Subparts
- **8.1 Floating window** — frameless, always-on-top, draggable by title area ✅
- **8.2 Size toggle** — small (320×200) / large (640×400) ✅
- **8.3 Native PiP trigger** — JS `requestPictureInPicture()` on first video ✅

### Known Issues
- ⚠️ **8.A** `m_pip->close()` does not properly delete — `destroyed` signal resets pointer but window may linger. Should use `deleteLater()`.

---

## Part 9 — Reading Mode
**Files:** `src/reading_mode.h`, `src/reading_mode.cpp`

### Subparts
- **9.1 Content extraction** — tries semantic tags, falls back to largest div ✅
- **9.2 Reader HTML** — clean dark serif layout, exit button ✅
- **9.3 Restore** — re-writes original HTML on exit ✅

### Known Issues
- ⚠️ **9.A** Exit button uses `document.title` hack to signal C++ side — but C++ side never connects to `titleChanged` to detect `__sf_exit_reader__`. Exit only works from JS side (button click re-writes HTML directly).

---

## Part 10 — Tools
**Files:** `src/tools/*.cpp`, `src/tools/*.h`

### Subparts
- **10.1 PDF Tools** — merge, split (`src/tools/pdf_tools.cpp`) ✅
- **10.2 Doc Tools** — DOCX↔PDF, image→PDF, text→PDF (`src/tools/doc_tools.cpp`) ✅
- **10.3 Office Tools** — XLSX↔PDF, CSV↔XLSX, PPTX↔PDF, PDF→image/text (`src/tools/office_tools.cpp`) ✅
- **10.4 Archive Tools** — zip/7z/tar create+extract (`src/tools/archive_tools.cpp`) ✅
- **10.5 Student Tools** — calculator, unit converter, QR generator, note taker (`src/tools/student_tools.cpp`) ✅
- **10.6 Translate** — Google Translate URL bridge (`src/tools/translate.cpp`) ✅
- **10.7 YouTube Transcript** — yt-dlp subtitle fetch (`src/tools/youtube_transcript.cpp`) ✅

### Known Issues
- ⚠️ **10.A** Tools that call external programs (libreoffice, pdftk, etc.) have no progress indication beyond a spinner — errors are silently ignored if tool missing

---

## Part 11 — Styles & Themes
**Files:** `src/styles.h`, `src/styles.cpp`

### Subparts
- **11.1 Light theme** — Qt stylesheet ✅
- **11.2 Dark theme (One Dark)** — Qt stylesheet ✅

---

## Part 12 — File & Folder Pickers
**Files:** `src/file_picker.h`, `src/file_picker.cpp`, `src/folder_picker.h`, `src/folder_picker.cpp`

### Subparts
- **12.1 FilePicker** — custom styled open/save/multi-file dialogs ✅
- **12.2 FolderPicker** — custom styled directory browser ✅

---

## Part 13 — Build & Packaging
**Files:** `CMakeLists.txt`, `packaging/`, `package_appimage.sh`, `build_packages.sh`

### Subparts
- **13.1 CMake build** — Qt6, WebEngineWidgets, WebChannel ✅
- **13.2 Linux .deb/.rpm** — packaging scripts ✅
- **13.3 AppImage** — `package_appimage.sh` ✅
- **13.4 Windows .exe** — `package_windows.sh` ✅

---

## Fix Queue (Priority Order)

| ID   | Part | Severity | Status | Description |
|------|------|----------|--------|-------------|
| 1.A  | 1    | Low      | ✅ Fixed | Duplicate includes in mainwindow.h removed |
| 1.C  | 2    | High     | ✅ Fixed | AdBlocker wired to profile via `setUrlRequestInterceptor()` — network blocking now works |
| 1.D  | 1    | Medium   | ✅ Fixed | Adblock level checkbox reads actual `getBlocker().level()` |
| 1.B  | 4    | Medium   | ✅ Fixed | Password capture injected on `loadFinished` in every new tab |
| 3.A  | 3    | High     | 🔧 In Progress | Shorts dark mode — `span`/`div` removed from broad rule, Shorts reset block present |
| 5.A  | 5    | Medium   | ✅ Fixed | `@match` pattern enforced via JS URL guard wrapping injected scripts |
| 7.A  | 7    | Medium   | ✅ Fixed | Media bar re-attaches to active tab on `currentChanged` |
| 8.A  | 8    | Low      | ✅ Fixed | PiP uses `deleteLater()` + `WA_DeleteOnClose`, no parent to avoid double-free |
| 9.A  | 9    | Medium   | ✅ Fixed | Reading mode exit button detected via `titleChanged` signal on C++ side |
