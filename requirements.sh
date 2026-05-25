#!/bin/bash
# ── SwordFish Setup Script ─────────────────────────────────────────────────────

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  SwordFish — Dependency Installer"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ── Detect distro ──────────────────────────────────────────────────────────────
if command -v dnf &>/dev/null; then
    PKG="dnf"
elif command -v apt &>/dev/null; then
    PKG="apt"
elif command -v pacman &>/dev/null; then
    PKG="pacman"
elif command -v pkg &>/dev/null && [ -n "$TERMUX_VERSION" ]; then
    PKG="termux"
else
    PKG="unknown"
fi

echo "→ Detected package manager: $PKG"

# ── System packages (ffmpeg, avr toolchain) ────────────────────────────────────
install_system_deps() {
    echo ""
    echo "[1/3] Installing system packages..."
    case $PKG in
        dnf)
            sudo dnf install -y ffmpeg avr-gcc avr-libc avrdude
            ;;
        apt)
            sudo apt update -y
            sudo apt install -y ffmpeg gcc-avr avr-libc avrdude
            ;;
        pacman)
            sudo pacman -Sy --noconfirm ffmpeg avr-gcc avr-libc avrdude
            ;;
        termux)
            pkg install -y ffmpeg avr-toolchain avrdude
            ;;
        *)
            echo "  [!] Unknown distro — skipping system packages."
            echo "      Install manually: ffmpeg, avr-gcc, avr-libc, avrdude"
            ;;
    esac
}

# ── Python packages ────────────────────────────────────────────────────────────
install_python_deps() {
    echo ""
    echo "[2/3] Installing Python packages..."

    # Use --break-system-packages only if needed (newer Debian/Ubuntu/Fedora)
    PIP_FLAGS=""
    if python3 -m pip install --dry-run pip &>/dev/null; then
        : # no flag needed
    else
        PIP_FLAGS="--break-system-packages"
    fi

    python3 -m pip install $PIP_FLAGS \
        PySide6 \
        yt-dlp

    # Termux can't run PySide6 — use a note instead
    if [ "$PKG" = "termux" ]; then
        echo "  [!] Termux: PySide6 GUI won't work on Android."
        echo "      Server (main.py) and AVR tools still work fine."
    fi
}

# ── Permissions + desktop shortcut ─────────────────────────────────────────────
setup_launcher() {
    echo ""
    echo "[3/3] Setting up launcher..."

    chmod +x swordfish.sh 2>/dev/null && echo "  ✓ swordfish.sh is executable"

    if [ -f "swordfish.desktop" ] && [ "$PKG" != "termux" ]; then
        # Replace placeholder with real path
        REAL_DIR="$(pwd)"
        REAL_USER="$(whoami)"
        sed -i "s|/home/YOUR_USERNAME/swordfish|$REAL_DIR|g" swordfish.desktop
        sed -i "s|YOUR_USERNAME|$REAL_USER|g" swordfish.desktop

        mkdir -p ~/.local/share/applications
        cp swordfish.desktop ~/.local/share/applications/
        update-desktop-database ~/.local/share/applications/ 2>/dev/null
        echo "  ✓ App shortcut installed → search 'SwordFish' in your launcher"
    fi
}

# ── Run ────────────────────────────────────────────────────────────────────────
install_system_deps
install_python_deps
setup_launcher

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Done! Run with:  ./swordfish.sh"
echo "  Or directly:     python3 main.py"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"