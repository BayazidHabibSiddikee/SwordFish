#!/usr/bin/env bash
# package_windows.sh — Cross-compile SwordFish for Windows and create NSIS installer
# Requires MXE (M cross environment) or a Windows build machine with Qt6 + NSIS
#
# On Linux with MXE:
#   Install MXE: https://mxe.cc/
#   MXE_PREFIX=/opt/mxe
#   TARGET=x86_64-w64-mingw32.static
#
# On Windows (run in Git Bash / MSYS2 with Qt6 + CMake + NSIS installed):
#   Just run: bash package_windows.sh --native
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR"
VERSION="$(grep -m1 'project(SwordFish' "$ROOT/CMakeLists.txt" | grep -oP 'VERSION\s+\K[\d.]+')"
DIST="$ROOT/dist"
mkdir -p "$DIST"

NATIVE="${1:-}"

if [[ "$NATIVE" == "--native" ]]; then
    # ── Native Windows build (run this in MSYS2 / Qt MinGW shell) ────────────
    echo "══════════════════════════════════════════"
    echo "  SwordFish Windows Native Build  v${VERSION}"
    echo "══════════════════════════════════════════"

    BUILD_DIR="$ROOT/build_win"
    cmake -B "$BUILD_DIR" -S "$ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install"

    cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"
    cmake --install "$BUILD_DIR"

    # Deploy Qt DLLs
    if command -v windeployqt6 &>/dev/null; then
        echo "→ Running windeployqt6..."
        windeployqt6 --release --no-translations \
            "$BUILD_DIR/install/bin/SwordFish.exe"
    elif command -v windeployqt &>/dev/null; then
        windeployqt --release --no-translations \
            "$BUILD_DIR/install/bin/SwordFish.exe"
    fi

    # Build NSIS installer
    if command -v makensis &>/dev/null; then
        echo "→ Building NSIS installer..."
        cd "$BUILD_DIR"
        cpack -G NSIS
        mv "$BUILD_DIR"/*.exe "$DIST/" 2>/dev/null || true
        echo "✔  Installer: $DIST/SwordFish-${VERSION}-win64.exe"
    else
        echo "⚠  NSIS not found. Install NSIS to create .exe installer."
        echo "   Binary at: $BUILD_DIR/install/bin/SwordFish.exe"
    fi

else
    # ── Cross-compile via MXE on Linux ────────────────────────────────────────
    MXE_PREFIX="${MXE_PREFIX:-/opt/mxe}"
    TARGET="${MXE_TARGET:-x86_64-w64-mingw32.static}"
    TOOLCHAIN="$MXE_PREFIX/usr/$TARGET/share/cmake/mxe-conf.cmake"

    if [[ ! -f "$TOOLCHAIN" ]]; then
        echo "MXE toolchain not found at $TOOLCHAIN"
        echo "Options:"
        echo "  1. Install MXE: https://mxe.cc/"
        echo "  2. Set MXE_PREFIX=/path/to/mxe"
        echo "  3. Build natively on Windows: bash package_windows.sh --native"
        exit 1
    fi

    echo "→ Cross-compiling for Windows via MXE..."
    BUILD_DIR="$ROOT/build_win_cross"
    cmake -B "$BUILD_DIR" -S "$ROOT" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install"

    cmake --build "$BUILD_DIR" -j"$(nproc)"
    cmake --install "$BUILD_DIR"

    # Package
    cd "$BUILD_DIR"
    cpack -G NSIS
    mv "$BUILD_DIR"/*.exe "$DIST/" 2>/dev/null || true
    echo "✔  Windows installer: $DIST/"
fi
