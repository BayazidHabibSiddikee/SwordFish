#!/bin/bash
# SwordFish C++ Browser Launcher
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1
exec "$SCRIPT_DIR/build/SwordFish" "$@"
