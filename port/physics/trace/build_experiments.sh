#!/bin/sh
# Builds probe/newton_experiments.cpp for Windows against the game's real Newton.dll (game/Newton.dll, linked directly
# by GNU ld). Run the exe on the laptop next to a COPY of Newton.dll; the native build is target physics_newton_experiments.
set -e
here=$(cd "$(dirname "$0")" && pwd)
root=$here/../../..
out=${1:-$root/work/physics_traces/bin}
mkdir -p "$out"
i686-w64-mingw32-g++ -O2 -static -o "$out/newton_experiments.exe" "$here/probe/newton_experiments.cpp" "$root/game/Newton.dll"
echo "$out/newton_experiments.exe"
