#!/bin/bash
# ── SwordFish Launcher ────────────────────────────────────────────────────────
# Edit INSTALL_DIR to wherever you put your project files
INSTALL_DIR="$HOME/.swordfish"

cd "$INSTALL_DIR" || { echo "ERROR: $INSTALL_DIR not found"; exit 1; }

# Open the browser (if you want standalone browser without server, comment above 2 lines)
python3 src/main.py

# When the browser window closes, also kill the server
kill $SERVER_PID 2>/dev/null
