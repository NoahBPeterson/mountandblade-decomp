#!/bin/sh
# Run a hybrid build under CrossOver from the game's own folder (the game loads Modules/ and CommonRes/
# relative to the exe). The build is copied next to mount&blade.exe under its own name; the original exe is
# never touched, and deleting the copy undoes everything.
#
# usage: tools/hybrid/play.sh [work/hybrid/mb.pass.exe] [copy name, default mb_hybrid.exe]
# env:   CX_BOTTLE (default Steam), MB_GAME (default ./game, the symlink to the game folder)
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
src="${1:-$root/work/hybrid/mb.pass.exe}"
name="${2:-mb_hybrid.exe}"
game="${MB_GAME:-$root/game}"
cx="$HOME/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine"
[ -f "$src" ] || { echo "no build at $src (tools/hybrid/build.py)"; exit 1; }
[ -f "$game/mount&blade.exe" ] || { echo "no mount&blade.exe in $game"; exit 1; }
case "$name" in "mount&blade.exe") echo "refusing to overwrite the original exe"; exit 1;; esac
cp "$src" "$game/$name"
echo "copied $(basename "$src") -> $game/$name; starting it in bottle ${CX_BOTTLE:-Steam}"
cd "$game"
exec "$cx" --bottle "${CX_BOTTLE:-Steam}" --cx-app "$name"
