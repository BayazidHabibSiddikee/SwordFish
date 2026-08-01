#!/usr/bin/env bash
# package_appimage.sh — Build a portable Linux AppImage for SwordFish
# Requires: cmake, linuxdeploy, linuxdeploy-plugin-qt
# Usage: ./package_appimage.sh [--clean]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR"
BUILD_DIR="$ROOT/build_appimage"
APPDIR="$BUILD_DIR/AppDir"
VERSION="$(grep -m1 'project(SwordFish' "$ROOT/CMakeLists.txt" | grep -oP 'VERSION\s+\K[\d.]+')"
OUTPUT="$ROOT/dist/SwordFish-${VERSION}-x86_64.AppImage"

echo "══════════════════════════════════════════"
echo "  SwordFish AppImage Builder  v${VERSION}"
echo "══════════════════════════════════════════"

# ── Clean ────────────────────────────────────────────────────────────────────
if [[ "${1:-}" == "--clean" ]]; then
    echo "→ Cleaning build_appimage/..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR" "$ROOT/dist"

# ── Download linuxdeploy if not present ──────────────────────────────────────
LINUXDEPLOY="$BUILD_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$BUILD_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"

if [[ ! -f "$LINUXDEPLOY" ]]; then
    echo "→ Downloading linuxdeploy..."
    curl -fsSL -o "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

if [[ ! -f "$LINUXDEPLOY_QT" ]]; then
    echo "→ Downloading linuxdeploy-plugin-qt..."
    curl -fsSL -o "$LINUXDEPLOY_QT" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_QT"
fi

# ── Build Release ────────────────────────────────────────────────────────────
echo "→ Configuring CMake..."
cmake -B "$BUILD_DIR/cmake" -S "$ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

echo "→ Building..."
cmake --build "$BUILD_DIR/cmake" -j"$(nproc)"

echo "→ Installing into AppDir..."
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR/cmake"

# ── Copy extra assets ────────────────────────────────────────────────────────
# Icon at standard AppDir location
install -Dm644 "$ROOT/icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/swordfish.png"
install -Dm644 "$ROOT/icon.png" "$APPDIR/swordfish.png"

# Desktop file
install -Dm644 "$ROOT/packaging/linux/swordfish.desktop" \
    "$APPDIR/usr/share/applications/swordfish.desktop"
install -Dm644 "$ROOT/packaging/linux/swordfish.desktop" \
    "$APPDIR/swordfish.desktop"

# ── Build AppImage ────────────────────────────────────────────────────────────
echo "→ Building AppImage..."
export QMAKE
QMAKE=$(which qmake6 2>/dev/null || which qmake 2>/dev/null || echo "")
export OUTPUT

cd "$BUILD_DIR"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --plugin qt \
    --output appimage \
    --desktop-file "$APPDIR/swordfish.desktop" \
    --icon-file "$APPDIR/swordfish.png"

# Move output to dist/
mv "$BUILD_DIR"/SwordFish*.AppImage "$OUTPUT" 2>/dev/null || \
    mv "$BUILD_DIR"/*.AppImage "$OUTPUT" 2>/dev/null || true

echo ""
echo "✔  AppImage ready: $OUTPUT"
