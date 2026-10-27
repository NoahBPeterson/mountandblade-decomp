#!/usr/bin/env python3
"""loadprofile.py: where the wall time goes between launching the game and the mission being up.

The game writes one line per step to game/rgl_log.txt (engine init, map build, resource loads, mission init).
This watches that file while the game runs, stamps every new line with its arrival time, and reports the
biggest gaps with the line that was in flight and the process CPU% during the gap (so a gap that is CPU-bound
is told apart from one that is waiting on a window/IO).

usage: loadprofile.py [--keys 36,36,36] [--timeout 600] [--idle 25] [--out /tmp/loadprof.txt]
"""
import argparse, os, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CX = os.path.expanduser("~/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine")
GAME = os.path.join(ROOT, "game")
LOG = os.path.join(GAME, "rgl_log.txt")


def game_pid(fallback):
    """The launcher forks, so the real game is the process whose command line is the exe."""
    try:
        out = subprocess.run(["pgrep", "-f", "mb_standalone.exe"], capture_output=True, text=True).stdout.split()
        return int(out[-1]) if out else fallback
    except Exception:
        return fallback


def cpu(pid):
    try:
        out = subprocess.run(["ps", "-o", "pcpu=", "-p", str(pid)], capture_output=True, text=True).stdout.strip()
        return float(out) if out else 0.0
    except Exception:
        return 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keys", default="", help="comma-separated key codes to send once the menu window is up")
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--idle", type=int, default=25, help="stop after this many seconds without a new log line")
    ap.add_argument("--out", default="/tmp/loadprof.txt")
    ap.add_argument("--attach", action="store_true", help="do not launch: watch the log of the game already running "
                    "(so a human can click through the menus while it records)")
    a = ap.parse_args()

    if a.attach:
        p = None
        pid = game_pid(0)
    else:
        for pat in ("mb_standalone", "winedbg"):
            subprocess.run(["pkill", "-f", pat], capture_output=True)
        time.sleep(1.0)
        try:
            os.remove(LOG)
        except OSError:
            pass
        p = subprocess.Popen([CX, "--bottle", "Steam", "--cx-app", "mb_standalone.exe"], cwd=GAME,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        pid = game_pid(p.pid)
    t0 = time.time()
    pos = os.path.getsize(LOG) if a.attach else 0
    rows, sent, last_line = [], False, t0
    while time.time() - t0 < a.timeout:
        grew = False
        try:
            with open(LOG, "rb") as f:
                f.seek(pos)
                chunk = f.read()
                pos = f.tell()
        except OSError:
            chunk = b""
        pid = game_pid(pid)
        if chunk:
            grew = True
            last_line = time.time()
            for line in chunk.decode("utf-8", "replace").splitlines():
                rows.append((time.time() - t0, line.strip(), cpu(pid)))
        if not sent and a.keys and any("Switching to menu window" in r[1] for r in rows):
            sent = True
            time.sleep(5)
            for k in [x for x in a.keys.split(",") if x]:
                subprocess.run(["osascript", "-e",
                    'tell application "System Events" to set frontmost of (first process whose name contains '
                    '"mb_standalone") to true', "-e", "delay 0.4",
                    "-e", 'tell application "System Events" to key code %s' % k], capture_output=True)
                time.sleep(3)
        if p is not None and p.poll() is not None:
            rows.append((time.time() - t0, "<process exited>", 0.0))
            break
        if rows and time.time() - last_line > a.idle:
            rows.append((time.time() - t0, "<idle %ds>" % a.idle, 0.0))
            break
        time.sleep(0.2)

    gaps = []
    prev = 0.0
    for t, line, c in rows:
        gaps.append((t - prev, line, c))
        prev = t
    gaps.sort(reverse=True)
    total = rows[-1][0] if rows else 0.0
    with open(a.out, "w") as f:
        f.write("total %.1f s over %d log lines\n\nslowest gaps (seconds, then the line that arrived at its end):\n" % (total, len(rows)))
        for d, line, c in gaps[:30]:
            f.write("%7.2f s  cpu %5.1f%%  %s\n" % (d, c, line[:110]))
        f.write("\ntimeline:\n")
        prev = 0.0
        for t, line, c in rows:
            if t - prev >= 1.0:
                f.write("%7.2f s (+%5.2f) %s\n" % (t, t - prev, line[:110]))
            prev = t
    print(open(a.out).read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
