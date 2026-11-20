#!/bin/sh
# tools/port/lldb_crash.sh [extra lldb commands...]: run the native port under lldb in game/ (90 s cap, silent audio) and
# print the crash site: backtrace, source, locals. The process is killed afterwards.
cd "$(dirname "$0")/../../game" || exit 1
set -- -k "bt 12" -k "source list -c 14" -k "frame variable" "$@"
MBA_OUTPUT_GAIN=0 timeout 90 lldb --batch -o "settings set target.disable-aslr false" -o run "$@" -k kill -k quit -- ../build/port/macos-arm64/bin/mb 2>&1 | rg -v "^(warning|Process .* launched|\s*$)" | tail -120
