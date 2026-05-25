@echo off
:: ── SwordFish Setup Script (Windows) ─────────────────────────────────────────
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   SwordFish - Dependency Installer
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo.
echo [1/3] Installing Python packages...
pip install PySide6 yt-dlp

echo.
echo [2/3] Installing ffmpeg via winget...
winget install --id Gyan.FFmpeg -e --silent
if %errorlevel% neq 0 (
    echo  [!] winget failed. Download ffmpeg manually from:
    echo      https://ffmpeg.org/download.html
    echo      Extract and add the bin\ folder to your PATH.
)

echo.
echo [3/3] Checking installs...
python -c "import PySide6; print('  PySide6   OK')"
python -c "import yt_dlp;  print('  yt-dlp    OK')"
ffmpeg -version >nul 2>&1 && echo   ffmpeg    OK || echo   ffmpeg    MISSING - add to PATH

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Done! Run with:  python main.py
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
pause