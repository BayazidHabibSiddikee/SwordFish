#!/bin/bash
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#  🗡️ SwordFish Browser — Unified Installer (Linux)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

INSTALL_DIR="$HOME/.swordfish"
REPO_DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   Installing SwordFish Browser..."
echo "   Target: $INSTALL_DIR"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 1. Detect Package Manager
if command -v dnf &>/dev/null; then PKG="dnf"
elif command -v apt &>/dev/null; then PKG="apt"
elif command -v pacman &>/dev/null; then PKG="pacman"
else PKG="unknown"; fi

# 2. Install System Dependencies
echo "[1/5] Installing system dependencies..."
case $PKG in
    dnf)    sudo dnf install -y ffmpeg python3-pip ;;
    apt)    sudo apt update && sudo apt install -y ffmpeg python3-pip ;;
    pacman) sudo pacman -Sy --noconfirm ffmpeg python-pip ;;
    *)      echo "  [!] Manual install required for: ffmpeg, pip" ;;
esac

# 3. Install Python Dependencies
echo "[2/5] Installing Python libraries..."
PIP_FLAGS=""
if python3 -m pip install --dry-run PySide6 2>&1 | grep "externally-managed-environment" > /dev/null; then
    PIP_FLAGS="--break-system-packages"
fi

python3 -m pip install $PIP_FLAGS PySide6 yt-dlp pypdf arrow pygame deep-translator \
    youtube-transcript-api requests duckduckgo-search \
    geopy folium beautifulsoup4 httpx pyttsx3 \
    python-docx pikepdf img2pdf qrcode fpdf2 adblockparser \
    mammoth pymupdf pdf2docx pandas openpyxl pdfplumber

# 4. Copy Files to Install Directory
echo "[3/5] Deploying files to $INSTALL_DIR..."
mkdir -p "$INSTALL_DIR"
# Copy everything including hidden files (like .manifest.json)
cp -a "$REPO_DIR"/. "$INSTALL_DIR/"
# Clean up any potential git history or build artifacts in the install dir
rm -rf "$INSTALL_DIR/.git" "$INSTALL_DIR/build" "$INSTALL_DIR/dist"

# 5. Set Permissions & Generate Launcher
echo "[4/5] Setting permissions and creating launcher..."
chmod +x "$INSTALL_DIR/utils/integrity.py"

LAUNCHER="$INSTALL_DIR/swordfish.sh"
printf '#!/bin/bash\ncd "%s" || exit 1\nexec python3 "%s/src/main.py" "$@"\n' \
    "$INSTALL_DIR" "$INSTALL_DIR" > "$LAUNCHER"
chmod +x "$LAUNCHER"

# 6. Setup Desktop Integration
echo "[5/5] Creating system menu entry..."
DESKTOP_FILE="$HOME/.local/share/applications/swordfish.desktop"
mkdir -p "$HOME/.local/share/applications"

printf '[Desktop Entry]\nVersion=1.0\nType=Application\nName=SwordFish\nGenericName=Browser\nComment=Lightweight power-user browser\nExec=%s %%u\nIcon=%s\nTerminal=false\nCategories=Network;WebBrowser;\nStartupNotify=true\nMimeType=text/html;text/xml;application/xhtml+xml;application/xml;x-scheme-handler/http;x-scheme-handler/https;\n' \
    "$LAUNCHER" "$INSTALL_DIR/icon.png" > "$DESKTOP_FILE"

update-desktop-database "$HOME/.local/share/applications" 2>/dev/null

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   Installation Complete! 🎉"
echo "   SwordFish is now installed in $INSTALL_DIR"
echo "   You can now launch it from your apps menu."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
