#!/bin/bash
set -e

echo "=== SwordFish C++ Build Dependencies ==="

# Detect package manager
if command -v apt-get &> /dev/null; then
    PKG="apt-get"
elif command -v dnf &> /dev/null; then
    PKG="dnf"
elif command -v pacman &> /dev/null; then
    PKG="pacman -S"
elif command -v brew &> /dev/null; then
    PKG="brew install"
else
    echo "Unsupported package manager. Install manually:"
    echo "  - Qt6 (Core, Gui, Widgets, Network, WebEngineWidgets, WebChannel)"
    echo "  - CMake >= 3.20"
    echo "  - libcurl-dev"
    echo "  - qpdf (PDF tools)"
    echo "  - libreoffice (document conversion)"
    echo "  - poppler-utils (pdftotext, pdftoppm)"
    echo "  - p7zip-full (7z support)"
    echo "  - qrencode (QR code generation)"
    echo "  - enscript + ghostscript (text to PDF)"
    exit 1
fi

echo "Installing build dependencies..."

if [ "$PKG" = "apt-get" ]; then
    sudo apt-get update
    sudo apt-get install -y \
        cmake g++ \
        qt6-base-dev qt6-webengine-dev qt6-webchannel-dev \
        libcurl4-openssl-dev \
        qpdf \
        libreoffice-core libreoffice-writer libreoffice-calc libreoffice-impress \
        poppler-utils \
        p7zip-full \
        qrencode \
        enscript ghostscript
elif [ "$PKG" = "dnf" ]; then
    sudo dnf install -y \
        cmake gcc-c++ \
        qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtwebchannel-devel \
        libcurl-devel \
        qpdf \
        libreoffice-core libreoffice-writer libreoffice-calc libreoffice-impress \
        poppler-utils \
        p7zip \
        qrencode \
        enscript ghostscript
elif [ "$PKG" = "pacman -S" ]; then
    sudo pacman -S --needed \
        cmake gcc \
        qt6-base qt6-webengine qt6-webchannel \
        curl \
        qpdf \
        libreoffice-fresh \
        poppler \
        p7zip \
        qrencode \
        enscript ghostscript
elif [ "$PKG" = "brew install" ]; then
    brew install cmake qt@6 curl qpdf poppler qrencode enscript ghostscript
fi

echo ""
echo "=== Building SwordFish ==="
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo "Run: ./build/SwordFish"
