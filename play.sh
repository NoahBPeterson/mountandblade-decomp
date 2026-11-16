#!/bin/sh
# play.sh: build the play build (graphics + cursor patches on top of the default sets) and start it under CrossOver.
#
#   ./play.sh            build (about 2 min; unchanged files come from the compile cache), then launch
#   ./play.sh --no-build launch the last build as it is
#   ./play.sh --faithful build and launch the byte-exact reconstruction, no patches
#
# The launcher window opens; Configure has the graphics settings (anisotropy, render scale, FSR) on its Video tab.
# Quitting the game ends the script. No debugger is attached (crashrun --nodebug), so clicks are not slowed down.
set -e
cd "$(dirname "$0")"

build=1
args="--patches gfx --patches play"
out="work/relink/patched-gfx+play"
for a in "$@"; do
    case "$a" in
        --no-build) build=0 ;;
        --faithful) args="--faithful"; out="work/relink/faithful_play" ;;
        -h|--help) sed -n 2,9p "$0"; exit 0 ;;
        *) echo "play.sh: unknown option $a" >&2; exit 2 ;;
    esac
done

if [ "$build" = 1 ]; then
    echo "building ($args) ..."
    .venv/bin/python tools/relink/build.py $args --out "$out" > work/play_build.log 2>&1 || {
        echo "build failed; see work/play_build.log" >&2; tail -5 work/play_build.log >&2; exit 1; }
fi
[ -f "$out/mb_standalone.exe" ] || { echo "no build at $out (run without --no-build)" >&2; exit 1; }

if pgrep -f 'mb_standalone.exe' > /dev/null; then
    echo "the game is already running; quit it first" >&2
    exit 1
fi
echo "launching $out/mb_standalone.exe ..."
exec .venv/bin/python tools/relink/crashrun.py --exe "$out/mb_standalone.exe" --nodebug --timeout 86400
