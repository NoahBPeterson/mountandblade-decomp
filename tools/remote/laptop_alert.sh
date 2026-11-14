#!/bin/sh
# laptop_alert.sh: the Windows laptop is not available for compiles. Turns the Mac's output volume up (unmuted) and says so.
# Called by tools/remote/rcl.py when it cannot reach the compile server (at most once a minute: work/locks/laptop_alert.last),
# and by hand. MB_LAPTOP_IGNORE=1 (the user said "ignore the laptop for now") silences it.
[ "${MB_LAPTOP_IGNORE:-0}" = "1" ] && exit 0
osascript -e 'set volume output volume 80 without output muted' >/dev/null 2>&1
say "Laptop not available for compiles" >/dev/null 2>&1
exit 0
