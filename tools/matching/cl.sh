#!/bin/sh
# Run the Visual C++ Toolkit 2003 compiler (cl 13.10.3052, VC .NET 2003) under wine.
# mount&blade.exe was linked by link 7.10; the Rich header is gone (Valve's VLV block), so the exact
# cl 13.10 build is inferred from matches. Toolchain: work/toolchain/vc71tk, extracted from your own
# VCToolkitSetup.exe by tools/setup_toolchain.py (which also sets up the headers below and the wine prefix).
# Headers: vc71tk CRT/STL + Windows SDK 6.0A (from vc9sp1) for windows.h / d3d9.h / dinput.h.
# Wine prefix: work/toolchain/wineprefix-mb, our own (create with `WINEPREFIX=$PWD/work/toolchain/wineprefix-mb
# wineboot -i`). Sharing spore-decomp's prefix meant one wineserver for both projects' waves: a one-line
# file took 6.5-14.6 s to compile; in our own prefix it takes ~2 s. Keep it warm during waves with
# tools/matching/keepalive.sh (wine stops its services ~3 s after the last program exits; a cold
# compile costs ~6-8 s).
root=$(cd "$(dirname "$0")/../.." && pwd)
tc="$root/work/toolchain/vc71tk"
winpath() { printf 'Z:%s' "$1" | tr '/' '\\'; }
export WINEPREFIX="${CL_WINEPREFIX:-$root/work/toolchain/wineprefix-mb}" WINEDEBUG=-all
# DirectX SDK June 2010 (work/ext/dxsdk_jun10, extracted Include + Lib/x86) comes LAST: only d3dx9*.h is taken from
# it; SDK 6.0A's d3d9.h/dinput.h win. (The game used d3dx9_31 = Oct 2006 SDK; the D3DX API it calls is unchanged.)
# work/toolchain/sal holds ONLY VC9's sal.h (copied from vc9sp1/VC/include): SDK 6.0A's specstrings.h includes
# <sal.h>, which VC7.1 lacks, so windows.h/d3d9.h/dinput.h failed (C1083). The Windows 10 SDK sal.h does not
# work with cl 13.10 (ConcurrencySal.h, unknown pragmas). Keep it after the VC7.1 headers.
export INCLUDE="$(winpath "$root/src/include");$(winpath "$root/match/include");$(winpath "$tc/include");$(winpath "$root/work/toolchain/sal");$(winpath "$root/work/toolchain/vc9sp1/WinSDK/Include");$(winpath "$root/work/ext/dxsdk_jun10/DXSDK/Include")"
export LIB="$(winpath "$tc/lib");$(winpath "$root/work/toolchain/vc9sp1/WinSDK/Lib")"
export WINEPATH="$(winpath "$tc/bin")"
# Output goes through a temp file, never the caller's pipes: wine's services inherit cl's stdio and
# would hold a caller's pipe open until the wine session ends (see spore-decomp tools/matching/cl.sh).
tmp=$(mktemp "${TMPDIR:-/tmp}/clsh.XXXXXX")
wine "$tc/bin/cl.exe" "$@" </dev/null >"$tmp" 2>&1
rc=$?
cat "$tmp"; rm -f "$tmp"
exit $rc
