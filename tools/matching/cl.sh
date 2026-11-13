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
# Machine-wide compile slots (MB_CL_SLOTS, default one per CPU), shared by every lane: worktrees symlink work/, so
# the lock files are the same. Without them five lanes' builds ran ~65 cl.exe at once on 10 cores and a full
# recompile took 47 min instead of a few; a waiting compile costs nothing. lockf -t 0 exits 75 (EX_TEMPFAIL) when
# the slot is held; cl.exe itself returns 0 or 2.
slots=${MB_CL_SLOTS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 8)}
locks="$root/work/locks"; mkdir -p "$locks"
# The Windows laptop's compile server comes FIRST (tools/remote/rcl.py: native cl.exe, 36 slots, 0.3-2 s a compile against
# several seconds under wine here; a batch of 30 compiles took 3.9 s there and 760 s queued here; docs/remote_worker.md).
# rcl.py exits 75 when the laptop is busy or unreachable (quick: 1 s connect timeout, then a 30 s "down" marker) and 76
# when this command can only run here (not configured, odd flags, a header that was not shipped, an anonymous namespace);
# then the local slots take it. Off unless ~/.config/mb-compile-server exists; MB_RC=0 turns it off.
remote=1
[ -r "$HOME/.config/mb-compile-server" ] || [ -n "$MB_RC_ENDPOINT" ] || remote=0
[ "$MB_RC" = 0 ] && remote=0
command -v python3 >/dev/null 2>&1 || remote=0
# No silent spill onto this machine (2026-11-13: the laptop's server had been down for hours and every compile ran here
# under wine, unnoticed): busy (75) waits for a laptop slot; unreachable (77) fails loudly unless MB_RC_ALLOW_LOCAL=1.
# 76 (this command can only run here by design) still compiles locally. Every local compile while remote is configured
# is logged to work/locks/local_compiles.log.
waited=0
while :; do
    if [ "$remote" = 1 ]; then
        python3 "$root/tools/remote/rcl.py" "$@" </dev/null >"$tmp" 2>&1
        rc=$?
        if [ "$rc" -eq 77 ] && [ "$MB_RC_ALLOW_LOCAL" != 1 ]; then
            echo "cl.sh: ERROR: the laptop compile server is unreachable; not compiling on this Mac." >&2
            printf '%s\n' 'cl.sh: start it on the laptop: powershell -File D:\mb\rc\rcstart.ps1' >&2
            echo "cl.sh: (docs/remote_worker.md), or set MB_RC_ALLOW_LOCAL=1 to compile here deliberately." >&2
            rm -f "$tmp"; exit 77
        fi
        if [ "$rc" -eq 75 ] && [ "$MB_RC_ALLOW_LOCAL" != 1 ]; then
            waited=$((waited + 1)); sleep 0.5; continue            # busy: wait for a laptop slot
        fi
        if [ "$rc" -eq 76 ] || [ "$rc" -eq 75 ] || [ "$rc" -eq 77 ]; then
            echo "$(date '+%F %T') rc=$rc $*" >> "$locks/local_compiles.log"
            remote=0
        else
            break
        fi
    fi
    i=1
    while [ "$i" -le "$slots" ]; do
        lockf -t 0 -k "$locks/cl.$i" wine "$tc/bin/cl.exe" "$@" </dev/null >"$tmp" 2>&1
        rc=$?
        [ "$rc" -ne 75 ] && break 2
        i=$((i + 1))
    done
    sleep 0.2
done
cat "$tmp"; rm -f "$tmp"
exit $rc
