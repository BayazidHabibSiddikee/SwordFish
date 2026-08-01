#!/usr/bin/env bash
# build_packages.sh — Build all SwordFish packages for Linux
# Produces: .deb, .rpm, .tar.gz, and .AppImage (if linuxdeploy available)
#
# Usage:
#   ./build_packages.sh              # build all
#   ./build_packages.sh --deb        # only .deb
#   ./build_packages.sh --rpm        # only .rpm
#   ./build_packages.sh --appimage   # only AppImage
#   ./build_packages.sh --tar        # only .tar.gz
#   ./build_packages.sh --clean      # clean then build all
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR"
BUILD_DIR="$ROOT/build_release"
DIST="$ROOT/dist"
VERSION="$(grep -m1 'project(SwordFish' "$ROOT/CMakeLists.txt" | grep -oP 'VERSION\s+\K[\d.]+')"

# Colours
GREEN='\033[0;32m'; CYAN='\033[0;36m'; RED='\033[0;31m'; NC='\033[0m'
info()  { echo -e "${CYAN}→ $*${NC}"; }
ok()    { echo -e "${GREEN}✔  $*${NC}"; }
error() { echo -e "${RED}✗  $*${NC}"; exit 1; }

echo -e "${CYAN}"
echo "╔══════════════════════════════════════════╗"
echo "║   SwordFish Package Builder  v${VERSION}      ║"
echo "╚══════════════════════════════════════════╝"
echo -e "${NC}"

# ── Args ─────────────────────────────────────────────────────────────────────
BUILD_DEB=0; BUILD_RPM=0; BUILD_TAR=0; BUILD_APPIMAGE=0; CLEAN=0
if [[ $# -eq 0 ]]; then
    BUILD_DEB=1; BUILD_RPM=1; BUILD_TAR=1; BUILD_APPIMAGE=1
fi
for arg in "$@"; do
    case $arg in
        --deb)      BUILD_DEB=1 ;;
        --rpm)      BUILD_RPM=1 ;;
        --tar)      BUILD_TAR=1 ;;
        --appimage) BUILD_APPIMAGE=1 ;;
        --clean)    CLEAN=1; BUILD_DEB=1; BUILD_RPM=1; BUILD_TAR=1; BUILD_APPIMAGE=1 ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ $CLEAN -eq 1 ]]; then
    info "Cleaning $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR" "$DIST"

# ── CMake configure + build ───────────────────────────────────────────────────
info "Configuring CMake (Release)..."
cmake -B "$BUILD_DIR" -S "$ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

info "Building with $(nproc) cores..."
cmake --build "$BUILD_DIR" -j"$(nproc)"
ok "Build complete"

# ── .deb ─────────────────────────────────────────────────────────────────────
if [[ $BUILD_DEB -eq 1 ]]; then
    info "Building .deb package..."
    if command -v dpkg-deb &>/dev/null || command -v cpack &>/dev/null; then
        cd "$BUILD_DIR"
        cpack -G DEB
        mv "$BUILD_DIR"/*.deb "$DIST/" 2>/dev/null && \
            ok ".deb → $DIST/swordfish_${VERSION}_amd64.deb" || \
            echo "  (no .deb produced — check cpack output)"
        cd "$ROOT"
    else
        echo "  dpkg-deb / cpack not found, skipping .deb"
    fi
fi

# ── .rpm ─────────────────────────────────────────────────────────────────────
if [[ $BUILD_RPM -eq 1 ]]; then
    info "Building .rpm package..."
    if command -v rpmbuild &>/dev/null || command -v cpack &>/dev/null; then
        cd "$BUILD_DIR"
        cpack -G RPM
        mv "$BUILD_DIR"/*.rpm "$DIST/" 2>/dev/null && \
            ok ".rpm → $DIST/" || \
            echo "  (no .rpm produced — check cpack output)"
        cd "$ROOT"
    else
        echo "  rpmbuild / cpack not found, skipping .rpm"
    fi
fi

# ── .tar.gz ───────────────────────────────────────────────────────────────────
if [[ $BUILD_TAR -eq 1 ]]; then
    info "Building .tar.gz archive..."
    cd "$BUILD_DIR"
    cpack -G TGZ
    mv "$BUILD_DIR"/*.tar.gz "$DIST/" 2>/dev/null && \
        ok ".tar.gz → $DIST/" || \
        echo "  (no .tar.gz produced)"
    cd "$ROOT"
fi

# ── AppImage ──────────────────────────────────────────────────────────────────
if [[ $BUILD_APPIMAGE -eq 1 ]]; then
    info "Building AppImage..."
    if [[ -f "$ROOT/package_appimage.sh" ]]; then
        bash "$ROOT/package_appimage.sh"
        ok "AppImage → $DIST/SwordFish-${VERSION}-x86_64.AppImage"
    else
        echo "  package_appimage.sh not found, skipping AppImage"
    fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}══════════════════════════════════════════${NC}"
echo -e "${GREEN}  Packages built in: $DIST/${NC}"
echo -e "${GREEN}══════════════════════════════════════════${NC}"
ls -lh "$DIST"/ 2>/dev/null || true
