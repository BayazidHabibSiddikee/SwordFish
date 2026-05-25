@echo off
setlocal enabledelayedexpansion
:: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
::  🗡️  SwordFish Browser — Installer (Windows)
:: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

:: Always work from the folder this .bat file lives in,
:: regardless of where the user launched it from.
cd /d "%~dp0"

echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   SwordFish Browser - Windows Installer
echo   Repo dir: %~dp0
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.

:: ── 0. Locate Python ─────────────────────────────────────────────────────────
:: Try  py  launcher first (installed with official Python for Windows),
:: then fall back to plain  python  and  python3.
set PYTHON=
for %%C in (py python python3) do (
    if not defined PYTHON (
        %%C --version >nul 2>&1 && set PYTHON=%%C
    )
)

if not defined PYTHON (
    echo [ERROR] Python not found.
    echo         Download and install it from https://www.python.org/downloads/
    echo         Make sure to tick "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)
echo [OK] Python found: %PYTHON%
%PYTHON% --version
echo.

:: ── 1. Upgrade pip (silently) ─────────────────────────────────────────────────
echo [1/4] Upgrading pip...
%PYTHON% -m pip install --upgrade pip --quiet
echo.

:: ── 2. Install Python packages ───────────────────────────────────────────────
echo [2/4] Installing Python packages...

:: Prefer requirements.txt if it exists, otherwise install the known list
:: directly so the script works even without that file in the repo.
if exist "requirements.txt" (
    echo       (using requirements.txt)
    %PYTHON% -m pip install -r requirements.txt
) else (
    echo       (requirements.txt not found — installing known package list)
    %PYTHON% -m pip install ^
        PySide6 yt-dlp pypdf arrow deep-translator ^
        youtube-transcript-api requests duckduckgo-search ^
        geopy folium beautifulsoup4 httpx pyttsx3 ^
        python-docx pikepdf img2pdf qrcode fpdf2 adblockparser ^
        mammoth pymupdf pdf2docx pandas openpyxl pdfplumber
)

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] pip install failed. Check the errors above.
    pause
    exit /b 1
)
echo.

:: ── 3. Install ffmpeg ────────────────────────────────────────────────────────
echo [3/4] Installing ffmpeg...
ffmpeg -version >nul 2>&1
if %errorlevel% equ 0 (
    echo       ffmpeg is already on PATH — skipping.
) else (
    :: Try winget (available on Windows 10 1709+ with App Installer)
    winget --version >nul 2>&1
    if %errorlevel% equ 0 (
        winget install --id Gyan.FFmpeg -e --silent
        if !errorlevel! neq 0 (
            echo  [!] winget install failed. Install ffmpeg manually:
            echo      1. Download from https://ffmpeg.org/download.html
            echo      2. Extract the archive somewhere (e.g. C:\ffmpeg)
            echo      3. Add C:\ffmpeg\bin to your System PATH.
        ) else (
            echo       ffmpeg installed via winget.
            echo  [!] You may need to restart this window for ffmpeg to be on PATH.
        )
    ) else (
        echo  [!] winget not available on this system.
        echo      Install ffmpeg manually:
        echo      1. Download from https://ffmpeg.org/download.html
        echo      2. Extract the archive somewhere (e.g. C:\ffmpeg)
        echo      3. Add C:\ffmpeg\bin to your System PATH.
    )
)
echo.

:: ── 4. Create a Start Menu shortcut ─────────────────────────────────────────
echo [4/4] Creating Start Menu shortcut...

set REPO_DIR=%~dp0
:: Strip trailing backslash
if "%REPO_DIR:~-1%"=="\" set REPO_DIR=%REPO_DIR:~0,-1%

set SHORTCUT_DIR=%APPDATA%\Microsoft\Windows\Start Menu\Programs
set SHORTCUT=%SHORTCUT_DIR%\SwordFish Browser.lnk
set LAUNCHER=%REPO_DIR%\swordfish.bat
set ICON=%REPO_DIR%\icon.ico

:: Generate swordfish.bat so it always cd's to the right place before launching
(
    echo @echo off
    echo cd /d "%REPO_DIR%"
    echo start "" "%PYTHON%" "%REPO_DIR%\src\main.py" %%*
) > "%LAUNCHER%"
echo       Launcher written: %LAUNCHER%

:: Use PowerShell to create the .lnk shortcut (no extra tools needed)
set PS_SCRIPT=%TEMP%\sf_shortcut_%RANDOM%.ps1
(
    echo $ws  = New-Object -ComObject WScript.Shell
    echo $lnk = $ws.CreateShortcut('%SHORTCUT%'^)
    echo $lnk.TargetPath      = '%LAUNCHER%'
    echo $lnk.WorkingDirectory = '%REPO_DIR%'
    if exist "%ICON%" (
        echo $lnk.IconLocation = '%ICON%'
    ) else (
        echo $lnk.IconLocation = '%PYTHON%,0'
    )
    echo $lnk.Description = 'SwordFish Browser'
    echo $lnk.Save(^)
) > "%PS_SCRIPT%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%" >nul 2>&1
del "%PS_SCRIPT%" >nul 2>&1

if exist "%SHORTCUT%" (
    echo       Start Menu shortcut created successfully.
) else (
    echo  [!] Could not create Start Menu shortcut.
    echo      You can still launch with:  swordfish.bat
)

if not exist "%ICON%" (
    echo  [!] icon.ico not found at %ICON%
    echo      The shortcut will use the Python icon instead.
    echo      Place icon.ico in the repo root and re-run to fix.
)
echo.

:: ── 5. Verify installs ───────────────────────────────────────────────────────
echo ── Verification ──────────────────────────────────────────────────────────
%PYTHON% -c "import PySide6;  print('  [OK] PySide6')"   2>nul || echo   [MISSING] PySide6
%PYTHON% -c "import yt_dlp;   print('  [OK] yt-dlp')"    2>nul || echo   [MISSING] yt-dlp
%PYTHON% -c "import mammoth;  print('  [OK] mammoth')"   2>nul || echo   [MISSING] mammoth
%PYTHON% -c "import fitz;     print('  [OK] pymupdf')"   2>nul || echo   [MISSING] pymupdf
%PYTHON% -c "import pdf2docx; print('  [OK] pdf2docx')"  2>nul || echo   [MISSING] pdf2docx
ffmpeg -version >nul 2>&1 && echo   [OK] ffmpeg || echo   [MISSING] ffmpeg ^(restart terminal after install^)
echo.

echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Installation Complete! 🎉
echo.
echo   Launch options:
echo     • Start Menu  →  search "SwordFish Browser"
echo     • Double-click:  swordfish.bat  in the repo folder
echo     • Terminal:      python src\main.py
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
pause
