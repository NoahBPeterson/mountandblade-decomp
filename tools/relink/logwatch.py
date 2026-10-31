#!/usr/bin/env python3
"""logwatch.py: stamp each new line of the game's rgl_log.txt with the wall-clock time it appeared.

The log has no timestamps; this polls it (every 20 ms by default) and writes `<epoch ms>\t<line>` rows, so load phases
("Loading Textures...", "load_map_data complete.", "begin_setup", ...) get start times without touching the game.
Run it before launching (it waits for the log to be truncated by the new run) and stop it with Ctrl-C or --seconds.
"""
import argparse, os, sys, time

LOG = os.path.expanduser("~/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/"
                         "steamapps/common/Mount and Blade/rgl_log.txt")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--log", default=LOG)
    ap.add_argument("--seconds", type=float, default=1800)
    ap.add_argument("--poll", type=float, default=0.02)
    a = ap.parse_args()
    start = time.time()
    pos, partial = None, ""
    with open(a.out, "w") as out:
        out.write("%d\tlogwatch start\n" % int(start * 1000))
        while time.time() - start < a.seconds:
            try:
                size = os.path.getsize(a.log)
            except OSError:
                size = 0
            if pos is None:
                pos = size
            if size < pos:                          # the game truncated the log: a new run
                pos, partial = 0, ""
                out.write("%d\tlog truncated (new run)\n" % int(time.time() * 1000))
            if size > pos:
                with open(a.log, "rb") as f:
                    f.seek(pos)
                    data = f.read(size - pos).decode("latin-1")
                pos = size
                now = int(time.time() * 1000)
                lines = (partial + data).split("\n")
                partial = lines.pop()
                for line in lines:
                    out.write("%d\t%s\n" % (now, line.rstrip("\r")))
                out.flush()
            time.sleep(a.poll)


if __name__ == "__main__":
    sys.exit(main())
