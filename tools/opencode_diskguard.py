#!/usr/bin/env python3
"""Disk guard for opencode waves: prints one line per event (run it under a monitor), every 20 s:

  - a file under work/match/ larger than --max-file GB (default 2): kills the processes writing it (lsof) and deletes
    it. Only wave scratch lives there; a session's `cat a f b > f` grew one to 142 GB and filled the disk (2026-11-18).
  - free space on the repo's volume below --min-free GB (default 30): SIGSTOPs every opencode_broker.py (sessions then
    wait on their next tool call) and prints DISK LOW. `kill -CONT` the broker once space is back.
  - the laptop compile server not answering: prints LAPTOP DOWN.

usage: opencode_diskguard.py [--max-file 2] [--min-free 30]
"""
import argparse, os, shutil, socket, subprocess, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MATCH = os.path.join(ROOT, "work", "match")


def say(msg):
    print(time.strftime("%H:%M:%S"), msg, flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-file", type=float, default=2)
    ap.add_argument("--min-free", type=float, default=30)
    a = ap.parse_args()
    low = False
    while True:
        for d, _, fs in os.walk(MATCH):
            for f in fs:
                p = os.path.join(d, f)
                try:
                    sz = os.path.getsize(p)
                except OSError:
                    continue
                if sz > a.max_file * 2**30:
                    pids = subprocess.run(["lsof", "-t", p], capture_output=True, text=True).stdout.split()
                    for pid in pids:
                        subprocess.run(["kill", "-9", pid])
                    os.remove(p)
                    say("RUNAWAY %s (%.1f GB): killed %s, deleted" % (os.path.relpath(p, ROOT), sz / 2**30, pids or "no writer"))
        free = shutil.disk_usage(ROOT).free / 2**30
        if free < a.min_free and not low:
            subprocess.run(["pkill", "-STOP", "-f", "opencode_broker.py"])
            say("DISK LOW: %.0f GB free; brokers stopped (SIGSTOP)" % free)
            low = True
        elif free >= a.min_free and low:
            say("disk back to %.0f GB free; brokers still stopped: kill -CONT them when ready" % free)
            low = False
        try:
            socket.create_connection(("100.84.111.92", 47813), timeout=3).close()
        except OSError:
            say("LAPTOP DOWN")
        time.sleep(20)


if __name__ == "__main__":
    main()
