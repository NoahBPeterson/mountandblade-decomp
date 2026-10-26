#!/usr/bin/env python3
"""d3dtrace.py: run an exe from the game folder under CrossOver with a +d3d9 trace, press Play, wait, end it.

usage: d3dtrace.py EXE OUT.log [--secs N]

EXE is copied into the game folder as mb_trace.exe (the original exe is never touched); the trace is CrossOver's
--cx-log output. Compare two traces with the draw/texture summary at the end of the log (or tools of your choice).
"""
import argparse, os, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "relink"))
sys.path.insert(0, os.path.join(ROOT, "tools", "hybrid"))
GAME_WIN = r"C:\Program Files (x86)\Steam\steamapps\common\Mount and Blade" + "\\"
COPY = "mb_trace.exe"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("out")
    ap.add_argument("--secs", type=int, default=120)
    a = ap.parse_args()
    import crashrun as C
    import capture as K
    game = os.path.join(ROOT, "game")
    shutil.copyfile(a.exe, os.path.join(game, COPY))
    out = os.path.abspath(a.out)
    p = subprocess.Popen([C.CX, "--bottle", "Steam", "--cx-log", out, "--debugmsg", "+d3d9,+d3d", "--cx-app",
                          GAME_WIN + COPY], cwd=game, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    t0 = time.time()
    K.press(a.secs, "mb_trace")
    time.sleep(max(0, a.secs - (time.time() - t0)))
    for line in subprocess.run(["ps", "-axo", "pid=,command="], capture_output=True, text=True).stdout.splitlines():
        pid, cmd = line.strip().split(None, 1)
        if cmd.startswith(GAME_WIN + COPY):
            os.kill(int(pid), 9)
    p.wait(30)
    print("%s: %.1f MB" % (os.path.relpath(out, ROOT), os.path.getsize(out) / 1e6))


if __name__ == "__main__":
    main()
