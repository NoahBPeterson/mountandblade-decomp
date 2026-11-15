#!/bin/sh
# patches/gfx/shaders/build_shaders.sh: recompile gfx_fsr.hlsl into gfx_shaders.h (committed; the build never runs this).
#
# Builds hlslc.exe (hlslc.c, cl 13.10 through tools/matching/cl.sh, linked with the toolkit's link.exe under wine) in
# work/gfx_shaders/ and runs it under wine next to a copy of the game's d3dx9_31.dll, whose HLSL compiler it calls.
set -e
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
w="$root/work/gfx_shaders"
mkdir -p "$w"
winpath() { printf 'Z:%s' "$1" | tr '/' '\\'; }
tc="$root/work/toolchain"
export WINEPREFIX="$tc/wineprefix-mb" WINEDEBUG=-all
# the game's own (Microsoft) d3dx9_31.dll, not wine's builtin one (which compiles with vkd3d-shader)
export WINEDLLOVERRIDES="d3dx9_31=n"
sh "$root/tools/matching/cl.sh" /nologo /c /O1 /MT /W3 "$(winpath "$here/hlslc.c")" /Fo"$(winpath "$w/hlslc.obj")"
LIB="$(winpath "$tc/vc71tk/lib");$(winpath "$tc/vc9sp1/WinSDK/Lib")" \
    wine "$tc/vc71tk/bin/link.exe" /NOLOGO /SUBSYSTEM:CONSOLE /OUT:"$(winpath "$w/hlslc.exe")" "$(winpath "$w/hlslc.obj")" kernel32.lib \
    > "$w/link.log" 2>&1 < /dev/null || { cat "$w/link.log"; exit 1; }
cp "$root/game/d3dx9_31.dll" "$w/"
cp "$here/gfx_fsr.hlsl" "$w/"
cd "$w"
wine hlslc.exe gfx_fsr.hlsl gfx_shaders.h \
    g_vs_quad:vs_quad:vs_3_0 g_ps_copy:ps_copy:ps_3_0 g_ps_easu:ps_easu:ps_3_0 g_ps_rcas:ps_rcas:ps_3_0 g_ps_down:ps_down:ps_3_0 \
    > "$w/hlslc.log" 2>&1 < /dev/null || { cat "$w/hlslc.log"; exit 1; }
cat "$w/hlslc.log"
tr -d '\r' < "$w/gfx_shaders.h" > "$here/gfx_shaders.h"
