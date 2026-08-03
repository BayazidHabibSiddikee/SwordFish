@echo off
setlocal enabledelayedexpansion
:: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
::  🗡️  SwordFish Browser v2.0 — Windows Build Dependencies Installer
::  Run this in an Administrator Command Prompt before building.
::  Requires: winget (Windows 10 1709+ / Windows 11)
:: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
cd /d "%~dp0"

echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   SwordFish Browser — Windows Dependencies
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.

:: ── Check winget ─────────────────────────────────────────────────────────────
winget --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] winget not found.
    echo         Install the App Installer from the Microsoft Store, then re-run.
    pause
    exit /b 1
)
echo [OK] winget found.
echo.

:: ── 1. CMake ─────────────────────────────────────────────────────────────────
echo [1/4] Installing CMake...
cmake --version >nul 2>&1
if %errorlevel% equ 0 (
    echo       Already installed.
) else (
    winget install --id Kitware.CMake -e --silent
)

:: ── 2. Qt6 (MSVC 64-bit) ─────────────────────────────────────────────────────
echo [2/4] Installing Qt6...
echo       Note: Qt6 is large (~4GB). This will take a while.
echo       If you already have Qt6 installed, press Ctrl+C and set:
echo         set CMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2019_64
echo.
winget install --id Qt.Qt -e --silent 2>nul || (
    echo       winget could not install Qt automatically.
    echo       Download the Qt Online Installer from https://www.qt.io/download-qt-installer
    echo       Install: Qt 6.x ^ WebEngine ^ WebChannel ^ Network components
)

:: ── 3. Visual Studio Build Tools (C++ compiler) ───────────────────────────────
echo [3/4] Installing MSVC Build Tools...
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --silent ^
    --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" 2>nul || (
    echo       If Visual Studio is already installed, this step can be skipped.
)

:: ── 4. NSIS (for creating .exe installer) ────────────────────────────────────
echo [4/4] Installing NSIS (for .exe packaging)...
winget install --id NSIS.NSIS -e --silent 2>nul || echo       (optional — only needed to build installer)

echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo   Dependencies installed.
echo.
echo   BUILD STEPS (run in a Qt6 MSVC Developer Prompt):
echo     cmake -B build -DCMAKE_BUILD_TYPE=Release
echo     cmake --build build --parallel
echo     cd build ^& cpack -G NSIS
echo.
echo   Or use package_windows.sh --native in Git Bash / MSYS2.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
pause
