"""Runtime smoke test of a relinked build: launch it in the game folder, capture the window while it loads and at the main
menu, close it, and compare the menu with a reference shot of a known-good build.

usage: smoke.py --exe DIR/mb_standalone.exe [--host mac|win|auto] [--out DIR] [--ref work/smoke/ref_menu.png] [--save-ref] [--timeout 420]

--host mac (default) runs under CrossOver here; --host win runs natively on the Windows laptop (docs/remote_worker.md, "Smoke test on the
laptop"; reference work/smoke/ref_menu_win.png); --host auto takes the Mac when no game runs there, else the laptop when it is free.

The byte, equivalence and data gates cannot see a build that runs wrong while every function is right (D2, 2026-11-02:
two owners for one piece of data left every loading-screen message and the whole loading screen drawn under the menu).
This is the check that can. The launcher is passed with crashrun --play (Return at the launcher: it brings the game
window to the front for a moment). Exit status: 0 menu matches the reference, 1 it does not (or the game died or never reached
the menu), 2 usage, 3 a game is already running (it is never killed: it may be the user's).

Shots go to --out (default work/smoke/<exe dir name>): load_NNN.png every 20 s while loading, menu.png 8 s and menu_2.png 14 s after the
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


def run_mac(a, exe, out):
    """Run the build under CrossOver; returns (status, menus) as the checks below expect."""
    tool = winshot_tool()
    log = os.path.join(GAME, "rgl_log.txt")
    t0 = time.time()
    run = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "relink", "crashrun.py"), "--exe", exe,
                            "--timeout", str(a.timeout + 60), "--play"], stdout=open(os.path.join(out, "crashrun.log"), "w"),
                           stderr=subprocess.STDOUT, cwd=ROOT)
    menu, menus, n, status = None, [], 0, "timeout"
    try:
        while time.time() - t0 < a.timeout:
            time.sleep(5)
            if run.poll() is not None and not game_pids():
                status = "game exited (see %s)" % os.path.join(out, "crashrun.log")
                break
            fresh = os.path.exists(log) and os.path.getmtime(log) > t0
            if fresh and "Finished All" in open(log, errors="replace").read():
                # two shots 6 s apart (a capture can land mid-transition: 15.0 once, 0.0 on the rerun); a broken menu
                # is wrong in every frame, so the better of the two is the verdict. A failed window capture is retried.
                menus = []
                for k, wait in enumerate((8, 6)):
                    time.sleep(wait)
                    m = os.path.join(out, "menu.png" if k == 0 else "menu_2.png")
                    for _ in range(3):
                        if shot(tool, m):
                            menus.append(m)
                            break
                        time.sleep(2)
                menu = menus[0] if menus else None
                status = "menu" if menus else "menu reached, no window to capture"
                break
            if int(time.time() - t0) // 20 > n:
                n += 1
                shot(tool, os.path.join(out, "load_%03d.png" % (n * 20)))
    finally:
        for p in game_pids():            # the run's own game: it was not running before we launched it
            subprocess.run(["kill", str(p)], capture_output=True)
        try:
            run.wait(timeout=60)
        except subprocess.TimeoutExpired:     # crashrun keeps waiting on the debugger of the game it no longer has
            run.kill()
    print("%s after %d s; shots in %s" % (status, time.time() - t0, os.path.relpath(out, ROOT)))
    return status, menus


# ---- the Windows laptop (native, no wine) ----------------------------------------------------------------------------
WIN_GAME = r"D:\SteamLibrary\steamapps\common\Mount and Blade"     # the Steam install on the laptop
WIN_DIR = r"D:\mb\smoke"                                             # our scratch dir there (everything is deleted after a run)
WIN_TASK = "mb_smoke"


def win_cfg():
    """(ssh argv, scp argv, user@host) from the environment (see tools/remote/worker.py), or None when not set up."""
    import shlex
    e = [os.environ.get(k) for k in ("MB_WORKER_SSH", "MB_WORKER_SCP", "MB_WORKER_HOST")]
    return (shlex.split(e[0]), shlex.split(e[1]), e[2]) if all(e) else None


def win_ssh(cfg, cmd, tries=3):
    for i in range(tries):
        try:
            r = subprocess.run(cfg[0] + [cmd], capture_output=True, text=True, timeout=90, errors="replace")
            if r.returncode == 0 or i == tries - 1:
                return r.stdout
        except subprocess.TimeoutExpired:
            pass
        time.sleep(2)
    return ""


def win_scp(cfg, src, dst, tries=3):
    for _ in range(tries):
        try:
            if subprocess.run(cfg[1] + [src, dst], capture_output=True, timeout=300).returncode == 0:
                return True
        except subprocess.TimeoutExpired:
            pass
        time.sleep(2)
    return False


def win_busy(cfg):
    """True when a game of ours (mb_standalone.exe) or a previous smoke task is on the laptop."""
    o = win_ssh(cfg, 'tasklist /fi "imagename eq mb_standalone.exe" /nh') + win_ssh(cfg, "schtasks /query /tn " + WIN_TASK + " 2>nul")
    return "mb_standalone" in o or WIN_TASK in o


def run_win(a, exe, out, cfg):
    """Run the build on the laptop in the logged-in user's desktop session (a scheduled task with /IT, created and deleted
    here), press Return at the launcher, wait for "Finished All", fetch two menu shots. Returns (status, menus)."""
    host = cfg[2]
    user = host.split("@")[0]
    t0 = time.time()
    rd = WIN_DIR + "\\out"
    run_cmd = os.path.join(out, "run.cmd")
    with open(run_cmd, "w", newline="") as f:
        f.write('@powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File %s\\winsmoke.ps1 -GameDir "%s" -Out %s -Timeout %d\r\n'
                % (WIN_DIR, WIN_GAME, rd, a.timeout))
    win_ssh(cfg, "if not exist %s mkdir %s" % (WIN_DIR, WIN_DIR))
    status, menus, pid = "timeout", [], None
    try:
        if not (win_scp(cfg, exe, "%s:%s/mb_standalone.exe" % (host, WIN_DIR.replace("\\", "/")))
                and win_scp(cfg, os.path.join(ROOT, "tools", "relink", "winsmoke.ps1"), "%s:%s/winsmoke.ps1" % (host, WIN_DIR.replace("\\", "/")))
                and win_scp(cfg, run_cmd, "%s:%s/run.cmd" % (host, WIN_DIR.replace("\\", "/")))):
            return "could not copy the files to the laptop", []
        # the exe goes next to the original under a distinct name (as the Mac does); the install is otherwise untouched
        win_ssh(cfg, 'if exist %s rmdir /s /q %s' % (rd, rd))
        win_ssh(cfg, 'copy /y %s\\mb_standalone.exe "%s\\mb_standalone.exe"' % (WIN_DIR, WIN_GAME))
        # /IT: runs only while the user is logged on, in their session, so the game gets the interactive desktop
        r = win_ssh(cfg, 'schtasks /create /tn %s /tr "%s\\run.cmd" /sc once /st 00:00 /ru %s /it /f' % (WIN_TASK, WIN_DIR, user))
        win_ssh(cfg, "schtasks /run /tn " + WIN_TASK)
        while time.time() - t0 < a.timeout + 60:
            time.sleep(5)
            o = win_ssh(cfg, 'if exist %s\\done (echo DONE) & type %s\\status.txt 2>nul' % (rd, rd))
            if "DONE" in o:
                st = o.replace("DONE", "").strip()
                if st.startswith("menu"):
                    status = "menu"
                    got = [(n, os.path.join(out, n)) for n in ("menu.png", "menu_2.png")]
                    menus = [p for n, p in got if win_scp(cfg, "%s:%s/out/%s" % (host, WIN_DIR.replace("\\", "/"), n), p)]
                    status = "menu" if menus else "menu reached, no window to capture"
                    secs = st.split()[-1]
                    print("time to the menu on the laptop: %s s" % secs)
                else:
                    status = st or "game gave no status"
                break
    finally:
        # end only the game this run started (the pid the script recorded), then leave nothing behind
        win_ssh(cfg, 'for /f %%p in (%s\\game.pid) do taskkill /pid %%p /fi "imagename eq mb_standalone.exe" /f' % rd)
        win_ssh(cfg, "schtasks /end /tn %s & schtasks /delete /tn %s /f" % (WIN_TASK, WIN_TASK))
        win_ssh(cfg, 'del /q "%s\\mb_standalone.exe" & rmdir /s /q %s' % (WIN_GAME, WIN_DIR))
        os.remove(run_cmd)
    print("%s after %d s; shots in %s" % (status, time.time() - t0, os.path.relpath(out, ROOT)))
    return status, menus


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--host", choices=("mac", "win", "auto"), default="mac")
    ap.add_argument("--out")
    ap.add_argument("--ref", help="reference menu (default work/smoke/ref_menu.png, ref_menu_win.png for --host win)")
    ap.add_argument("--save-ref", action="store_true", help="store this run's menu as the reference (a known-good build)")
    ap.add_argument("--timeout", type=int, default=420, help="seconds to reach the menu")
    a = ap.parse_args()
    host, cfg = a.host, win_cfg()
    if host == "auto":
        if not game_pids():
            host = "mac"
        elif cfg and not win_busy(cfg):
            host = "win"
        else:
            print("busy: a game is running on the Mac and the laptop is %s" % ("busy too" if cfg else "not configured (MB_WORKER_SSH)"))
            return 3
    if host == "mac" and game_pids():
        print("a game is already running (pids %s): not touching it" % game_pids())
        return 3
    if host == "win":
        if not cfg:
            print("the laptop is not configured: set MB_WORKER_SSH, MB_WORKER_SCP, MB_WORKER_HOST (docs/remote_worker.md)")
            return 2
        if win_busy(cfg):
            print("a game is already running on the laptop: not touching it")
            return 3
    ref = a.ref or os.path.join(ROOT, "work", "smoke", "ref_menu_win.png" if host == "win" else "ref_menu.png")
    exe = os.path.abspath(a.exe)
    out = a.out or os.path.join(ROOT, "work", "smoke", os.path.basename(os.path.dirname(exe)) + ("_win" if host == "win" else ""))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    status, menus = run_win(a, exe, out, cfg) if host == "win" else run_mac(a, exe, out)
    menu = menus[0] if menus else None
    print("host: " + host)
    if status != "menu":
        return 1
    if a.save_ref:
        os.makedirs(os.path.dirname(ref), exist_ok=True)
        shutil.copy(menu, ref)
        print("saved as the reference menu: %s" % os.path.relpath(ref, ROOT))
        return 0
    if not os.path.exists(ref):
        print("no reference menu (%s): run a known-good build with --save-ref" % os.path.relpath(ref, ROOT))
        return 1
    s = min(score(m, ref) for m in menus)
    ok = s <= THRESHOLD
    print("menu vs reference: %.1f (threshold %.0f) -> %s" % (s, THRESHOLD, "OK" if ok else "DIFFERS"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
