#!/bin/sh
# Run the Visual C++ Toolkit 2003 compiler (cl 13.10.3052, VC .NET 2003) under wine.
# mount&blade.exe was linked by link 7.10; the Rich header is gone (Valve's VLV block), so the exact
# cl 13.10 build is inferred from matches. Toolchain: work/toolchain/vc71tk (symlink to spore-decomp's
# copy, extracted from VCToolkitSetup.exe). Headers: vc71tk CRT/STL + Windows SDK 6.0A (from vc9sp1)
# for windows.h / d3d9.h / dinput.h. Shares spore-decomp's wine prefix.
root=$(cd "$(dirname "$0")/../.." && pwd)
tc="$root/work/toolchain/vc71tk"
winpath() { printf 'Z:%s' "$1" | tr '/' '\\'; }
export WINEPREFIX="${CL_WINEPREFIX:-$root/work/toolchain/wineprefix}" WINEDEBUG=-all
export INCLUDE="$(winpath "$root/src/include");$(winpath "$tc/include");$(winpath "$root/work/toolchain/vc9sp1/WinSDK/Include")"
export LIB="$(winpath "$tc/lib");$(winpath "$root/work/toolchain/vc9sp1/WinSDK/Lib")"
export WINEPATH="$(winpath "$tc/bin")"
# Output goes through a temp file, never the caller's pipes: wine's services inherit cl's stdio and
# would hold a caller's pipe open until the wine session ends (see spore-decomp tools/matching/cl.sh).
tmp=$(mktemp "${TMPDIR:-/tmp}/clsh.XXXXXX")
wine "$tc/bin/cl.exe" "$@" </dev/null >"$tmp" 2>&1
rc=$?
cat "$tmp"; rm -f "$tmp"
exit $rc
