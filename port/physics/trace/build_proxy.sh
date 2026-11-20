#!/bin/sh
# Builds the logging Newton.dll proxy (proxy/newton_proxy.c) for the original 32-bit game: NewtLg.dll (see RECORDING.md).
# Needs i686-w64-mingw32-gcc (Homebrew mingw-w64). Output: ${1:-work/physics_traces/bin}/NewtLg.dll
set -e
here=$(cd "$(dirname "$0")" && pwd)
out=${1:-$here/../../../work/physics_traces/bin}
mkdir -p "$out"
i686-w64-mingw32-gcc -O2 -std=gnu99 -Wall -Wno-unused-function -shared -static-libgcc -o "$out/NewtLg.dll" \
    "$here/proxy/newton_proxy.c" -Wl,--enable-stdcall-fixup
i686-w64-mingw32-objdump -p "$out/NewtLg.dll" | awk '/\+base\[.* Newton/{n++} END{print n " exported names (want 41)"}'
