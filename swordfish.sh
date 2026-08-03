#!/bin/bash
# SwordFish Browser Launcher
# Launches the C++ binary from ~/.local/bin or the build directory.
exec "$(dirname "$(readlink -f "$0")")/build/SwordFish" "$@"
