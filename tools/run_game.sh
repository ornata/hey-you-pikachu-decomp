#!/usr/bin/env bash
# Launch the built ROM in an N64 emulator.
#
# ares is the recommended emulator for this game: it is one of the few
# that emulates the VRU (Voice Recognition Unit) that Hey You, Pikachu!
# ships with — without it the game boots but cannot hear you.
set -euo pipefail

ROM="${1:?usage: run_game.sh <rom.z64>}"

if command -v ares >/dev/null 2>&1; then
    exec ares "$ROM"
fi
if [ -d "/Applications/ares.app" ]; then
    exec open -a ares "$ROM"
fi
echo "no emulator found. install ares:  brew install --cask ares" >&2
exit 1
