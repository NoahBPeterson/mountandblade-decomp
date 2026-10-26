#!/usr/bin/env python3
"""textcheck.py: run an exe to the main menu and say whether the menu button text is drawn.

usage: textcheck.py EXE [--secs N] [--shot OUT.png]
       textcheck.py --measure PNG

The exe is copied into the game folder as mb_check.exe (the original is never touched), started under CrossOver,
Play is pressed, and once game/rgl_log.txt says "Finished All..." the game window is captured (winshot.py, works
behind other windows) and the game ended. The button text is dark red on light parchment; `text` is the fraction
of dark pixels in the button column (x 38-80 %, y 30-88 % of the window), where the parchment alone has almost
none. Prints "text=<fraction> TEXT|NOTEXT|NOMENU" and exits 0 / 1 / 2.
"""
import argparse, os, shutil, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "relink"))
GAME_WIN = r"C:\Program Files (x86)\Steam\steamapps\common\Mount and Blade" + "\\"
COPY = "mb_check.exe"
THRESH = 0.004


def measure(path):
    from PIL import Image
    im = Image.open(path).convert("RGB")
    w, h = im.size
    px = im.load()
    n = dark = 0
    for y in range(int(h * 0.30), int(h * 0.88), 2):
        for x in range(int(w * 0.38), int(w * 0.80), 2):
            r, g, b = px[x, y]
            n += 1
            if r + g + b < 150:
                dark += 1
    return dark / max(1, n)


def run(exe, secs, shot):
    import crashrun as C
    import capture as K
    game = os.path.join(ROOT, "game")
    log = os.path.join(game, "rgl_log.txt")
    shutil.copyfile(exe, os.path.join(game, COPY))
    t0 = time.time()
    p = subprocess.Popen([C.CX, "--bottle", "Steam", "--cx-app", GAME_WIN + COPY], cwd=game,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=dict(os.environ, WINEDEBUG="-all"))
    K.press(secs, "mb_check")
    ok = False
    while time.time() - t0 < secs:
        try:
            if os.path.getmtime(log) >= t0 and "Finished All" in open(log, errors="replace").read():
                ok = True
                break
        except OSError:
            pass
        time.sleep(2)
    frac = None
    if ok:
        time.sleep(8)                                   # the menu's first frames
        for _ in range(5):
            if os.path.exists(shot):
                os.remove(shot)
            r = subprocess.run([sys.executable, os.path.join(HERE, "winshot.py"), shot, "mb_check"],
                               capture_output=True, text=True)
            if os.path.exists(shot) and r.returncode in (0, 2, 3):
                frac = measure(shot)
                break
            alive = any(l.strip().split(None, 1)[-1].startswith(GAME_WIN + COPY) for l in subprocess.run(
                ["ps", "-axo", "pid=,command="], capture_output=True, text=True).stdout.splitlines() if l.strip())
            print("winshot: %s (game %s)" % ((r.stdout + r.stderr).strip()[-200:], "running" if alive else "gone"),
                  file=sys.stderr)
            if not alive:
                break
            time.sleep(5)
    for line in subprocess.run(["ps", "-axo", "pid=,command="], capture_output=True, text=True).stdout.splitlines():
        pid, cmd = line.strip().split(None, 1)
        if cmd.startswith(GAME_WIN + COPY):
            os.kill(int(pid), 9)
    p.wait(30)
    return frac


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", nargs="?")
    ap.add_argument("--secs", type=int, default=240)
    ap.add_argument("--shot", default=os.path.join(ROOT, "work", "hybrid", "textcheck.png"))
    ap.add_argument("--measure")
    a = ap.parse_args()
    frac = measure(a.measure) if a.measure else run(a.exe, a.secs, a.shot)
    if frac is None:
        print("text=- NOMENU")
        return 2
    print("text=%.4f %s" % (frac, "TEXT" if frac > THRESH else "NOTEXT"))
    return 0 if frac > THRESH else 1


if __name__ == "__main__":
    sys.exit(main())
