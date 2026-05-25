#!/bin/bash
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#  🗡️ SwordFish Browser — Unified Installer (Linux)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   Installing SwordFish Browser..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 1. Detect Package Manager
if command -v dnf &>/dev/null; then PKG="dnf"
elif command -v apt &>/dev/null; then PKG="apt"
elif command -v pacman &>/dev/null; then PKG="pacman"
else PKG="unknown"; fi

# 2. Install System Dependencies
echo "[1/4] Installing system dependencies..."
case $PKG in
    dnf)    sudo dnf install -y ffmpeg python3-pip ;;
    apt)    sudo apt update && sudo apt install -y ffmpeg python3-pip ;;
    pacman) sudo pacman -Sy --noconfirm ffmpeg python-pip ;;
    *)      echo "  [!] Manual install required for: ffmpeg, pip" ;;
esac

# 3. Install Python Dependencies
echo "[2/4] Installing Python libraries..."
PIP_FLAGS=""
# Check for PEP 668 (Managed Environments)
if python3 -m pip install --dry-run PySide6 2>&1 | grep "externally-managed-environment" > /dev/null; then
    PIP_FLAGS="--break-system-packages"
fi

python3 -m pip install $PIP_FLAGS PySide6 yt-dlp pypdf arrow pygame deep-translator \
    youtube-transcript-api requests duckduckgo-search \
    geopy folium beautifulsoup4 httpx pyttsx3 \
    python-docx pikepdf img2pdf qrcode fpdf2 adblockparser \
    mammoth pymupdf pdf2docx pandas openpyxl pdfplumber

# 4. Set Permissions
echo "[3/4] Setting permissions..."
chmod +x swordfish.sh
chmod +x utils/integrity.py

# 5. Setup Desktop Integration
echo "[4/4] Creating system menu entry..."
REAL_DIR="$(pwd)"
DESKTOP_FILE="swordfish.desktop"

if [ -f "$DESKTOP_FILE" ]; then
    # Create a fresh copy of the desktop file with absolute paths
    mkdir -p ~/.local/share/applications
    cp "$DESKTOP_FILE" ~/.local/share/applications/swordfish.desktop
    
    sed -i "s|FULL_PATH_TO_SH|$REAL_DIR/swordfish.sh|g" ~/.local/share/applications/swordfish.desktop
    sed -i "s|FULL_PATH_TO_ICON|$REAL_DIR/icon.png|g" ~/.local/share/applications/swordfish.desktop
    
    update-desktop-database ~/.local/share/applications/ 2>/dev/null
    echo "  ✓ Desktop icon added successfully."
else
    echo "  [!] swordfish.desktop not found. Skipping shortcut."
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   Installation Complete! 🎉"
echo "   Search for 'SwordFish' in your apps."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
