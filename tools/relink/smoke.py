"""Runtime smoke test of a relinked build: launch it in the game folder, capture the window while it loads and at the main
menu, close it, and compare the menu with a reference shot of a known-good build.

usage: smoke.py --exe DIR/mb_standalone.exe [--out DIR] [--ref work/smoke/ref_menu.png] [--save-ref] [--timeout 420]

The byte, equivalence and data gates cannot see a build that runs wrong while every function is right (D2, 2026-11-02:
two owners for one piece of data left every loading-screen message and the whole loading screen drawn under the menu).
This is the check that can. Exit status: 0 menu matches the reference, 1 it does not (or the game died or never reached
the menu), 2 usage, 3 a game is already running (it is never killed: it may be the user's).

Shots go to --out (default work/smoke/<exe dir name>): load_NNN.png every 20 s while loading, menu.png 8 s after the
engine logs "Finished All". The score is the mean absolute difference of the two menus as 320x200 greyscale (0..255);
a correct menu scores a few points (the hover highlight, the fps counter), a broken one tens.
"""
import argparse, os, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = os.path.expanduser("~/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/"
                          "steamapps/common/Mount and Blade")
GAME_PROC = "Mount and Blade.mb_standalone.exe"      # pgrep -f pattern of the running game (and its winedbg parent)
THRESHOLD = 12.0


def winshot_tool():
    """work/tools/winshot: prints the CGWindowID of the largest window whose owner name contains its argument."""
    src = os.path.join(ROOT, "tools", "relink", "winshot.swift")
    exe = os.path.join(ROOT, "work", "tools", "winshot")
    if not os.path.exists(exe) or os.path.getmtime(exe) < os.path.getmtime(src):
        os.makedirs(os.path.dirname(exe), exist_ok=True)
        subprocess.run(["swiftc", "-O", "-o", exe, src], check=True, capture_output=True)
    return exe


def game_pids():
    r = subprocess.run(["pgrep", "-f", GAME_PROC], capture_output=True, text=True)
    return [int(p) for p in r.stdout.split()]


def shot(tool, path):
    wid = subprocess.run([tool, "mb_standalone"], capture_output=True, text=True).stdout.strip()
    if wid in ("", "0"):
        return False
    return subprocess.run(["screencapture", "-x", "-o", "-l", wid, path]).returncode == 0 and os.path.exists(path)


def score(a, b):
    from PIL import Image
    import numpy as np
    x = np.asarray(Image.open(a).convert("L").resize((320, 200)), dtype=float)
    y = np.asarray(Image.open(b).convert("L").resize((320, 200)), dtype=float)
    return float(abs(x - y).mean())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--out")
    ap.add_argument("--ref", default=os.path.join(ROOT, "work", "smoke", "ref_menu.png"))
    ap.add_argument("--save-ref", action="store_true", help="store this run's menu as the reference (a known-good build)")
    ap.add_argument("--timeout", type=int, default=420, help="seconds to reach the menu")
    a = ap.parse_args()
    if game_pids():
        print("a game is already running (pids %s): not touching it" % game_pids())
        return 3
    exe = os.path.abspath(a.exe)
    out = a.out or os.path.join(ROOT, "work", "smoke", os.path.basename(os.path.dirname(exe)))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    tool = winshot_tool()
    log = os.path.join(GAME, "rgl_log.txt")
    t0 = time.time()
    run = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "relink", "crashrun.py"), "--exe", exe,
                            "--timeout", str(a.timeout + 60)], stdout=open(os.path.join(out, "crashrun.log"), "w"),
                           stderr=subprocess.STDOUT, cwd=ROOT)
    menu, n, status = None, 0, "timeout"
    try:
        while time.time() - t0 < a.timeout:
            time.sleep(5)
            if run.poll() is not None and not game_pids():
                status = "game exited (see %s)" % os.path.join(out, "crashrun.log")
                break
            fresh = os.path.exists(log) and os.path.getmtime(log) > t0
            if fresh and "Finished All" in open(log, errors="replace").read():
                time.sleep(8)
                menu = os.path.join(out, "menu.png")
                status = "menu" if shot(tool, menu) else "menu reached, no window to capture"
                break
            if int(time.time() - t0) // 20 > n:
                n += 1
                shot(tool, os.path.join(out, "load_%03d.png" % (n * 20)))
    finally:
        for p in game_pids():            # the run's own game: it was not running before we launched it
            subprocess.run(["kill", str(p)], capture_output=True)
        run.wait(timeout=60)
    print("%s after %d s; shots in %s" % (status, time.time() - t0, os.path.relpath(out, ROOT)))
    if status != "menu":
        return 1
    if a.save_ref:
        os.makedirs(os.path.dirname(a.ref), exist_ok=True)
        shutil.copy(menu, a.ref)
        print("saved as the reference menu: %s" % os.path.relpath(a.ref, ROOT))
        return 0
    if not os.path.exists(a.ref):
        print("no reference menu (%s): run a known-good build with --save-ref" % os.path.relpath(a.ref, ROOT))
        return 1
    s = score(menu, a.ref)
    ok = s <= THRESHOLD
    print("menu vs reference: %.1f (threshold %.0f) -> %s" % (s, THRESHOLD, "OK" if ok else "DIFFERS"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
