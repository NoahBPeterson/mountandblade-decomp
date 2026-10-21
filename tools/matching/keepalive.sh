#!/bin/sh
# Keep the compiler's wine session warm for N hours (default 4) by running an idle Windows program in the
# foreground. Without it wine stops its services ~3 s after the last cl.exe exits and the next compile pays
# ~6-8 s instead of ~2 s. Start it as a visible background job for the length of a matching wave.
root=$(cd "$(dirname "$0")/../.." && pwd)
hours=${1:-4}
WINEPREFIX="$root/work/toolchain/wineprefix-mb" WINEDEBUG=-all exec wine ping -n $((hours * 3600)) 127.0.0.1 >/dev/null 2>&1
