#!/usr/bin/env python3
"""crashrun.py: run the standalone exe under winedbg and report where it crashes, by function name.

usage: crashrun.py [--exe work/relink/standalone/mb_standalone.exe] [--timeout 90] [--play] [--hang] [--bottle Steam]

Copies the exe into the game folder (as tools/hybrid/play.sh does: mb_standalone.exe next to the original, which is
never touched), starts it under CrossOver's winedbg with scripted commands (continue; on an exception: registers,
backtrace, 512 dwords of stack), then names every address with the link map: the faulting EIP, the backtrace, and
each stack dword that points into our .text (a manual stack walk: frame-pointer-less code stops winedbg's own
backtrace early). If the game is still running after --timeout seconds, it is left running and reported as such.
The raw debugger log is kept in work/relink/standalone/dbg.log.
"""
import argparse, os, re, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
CX = os.path.expanduser("~/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine")
WINPATH = r"C:\Program Files (x86)\Steam\steamapps\common\Mount and Blade\mb_standalone.exe"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(ROOT, "work", "relink", "standalone", "mb_standalone.exe"))
    ap.add_argument("--timeout", type=int, default=90)
    ap.add_argument("--bottle", default=os.environ.get("CX_BOTTLE", "Steam"))
    ap.add_argument("--play", action="store_true", help="press Return at the launcher (Play)")
    ap.add_argument("--hang", action="store_true", help="when it is still running at the timeout and spinning (CPU "
                    ">= 50%%), break in and report where, then end it")
    ap.add_argument("--cmds", default="", help="extra winedbg commands run at the fault (';'-separated), e.g. "
                    "'x /4x $ecx' (their output is in dbg.log)")
    ap.add_argument("--break", dest="brk", action="append", default=[], help="stop at this address (hex or a map "
                    "symbol, e.g. __purecall) and report as for a fault")
    a = ap.parse_args()
    game = os.path.join(ROOT, "game")
    shutil.copyfile(a.exe, os.path.join(game, "mb_standalone.exe"))
    log_path = os.path.join(ROOT, "work", "relink", "standalone", "dbg.log")
    report = "info reg\nbt 40\nx /3072x $esp\n" + "".join(c.strip() + "\n" for c in a.cmds.split(";") if c.strip())
    hung = False
    with open(log_path, "w") as log:
        p = subprocess.Popen([CX, "--bottle", a.bottle, "--cx-app", "winedbg", WINPATH], cwd=game,
                             stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT, text=True,
                             env=dict(os.environ, WINEDEBUG="-all"))
        for b in a.brk:
            p.stdin.write("break *0x%x\n" % symbol_address(b))
        p.stdin.write("cont\n")
        p.stdin.flush()
        if a.play:
            press_play(a.timeout)
        t0 = time.time()
        while p.poll() is None and time.time() - t0 < a.timeout:
            time.sleep(1)
            if stopped(open(log_path, errors="replace").read()):
                break
        if p.poll() is None and not stopped(open(log_path, errors="replace").read()):
            # still going: if it is spinning, break in (SIGINT to winedbg, as Ctrl-C) and read where it is
            cpu = "0"
            for line in subprocess.run(["ps", "-axo", "pcpu=,command="], capture_output=True, text=True).stdout.splitlines():
                c, cmd = line.strip().split(None, 1)
                if cmd.startswith(WINPATH):              # the game itself (winedbg's command line only contains it)
                    cpu = c
            if not a.hang or float(cpu.strip() or 0) < 50:
                p.stdin.close()
                print("still running after %d s, %s%% CPU (left running: the game window is up?)" % (a.timeout, cpu.strip()))
                return 0
            hung = True
            p.stdin.close()
            text = hang_dump(a, game, log_path)
            print("hung: spinning after %d s; attached and read every thread" % a.timeout)
            return report_text(text, hung=True)
        p.stdin.write(report + "quit\n")
        p.stdin.flush()
        p.stdin.close()
        try:
            p.wait(60)
        except subprocess.TimeoutExpired:
            p.kill()
    return report_text(open(log_path, errors="replace").read())


def stopped(text):
    return "Unhandled exception" in text or "Stopped on breakpoint" in text


def symbol_address(s):
    try:
        return int(s, 16)
    except ValueError:
        import build
        return build.read_map(os.path.join(ROOT, "work", "relink", "standalone", "mb_standalone.map"))[s]


def press_play(limit):
    """Wait for the launcher window and press Return (its default button is Play): System Events key code 36."""
    t0 = time.time()
    while time.time() - t0 < limit:
        r = subprocess.run(["osascript", "-e", 'tell application "System Events" to get position of window 1 of '
                            '(first process whose name contains "mb_standalone")'], capture_output=True, text=True)
        if r.returncode == 0:
            time.sleep(2)
            subprocess.run(["osascript", "-e", 'tell application "System Events" to set frontmost of (first process '
                            'whose name contains "mb_standalone") to true', "-e", "delay 0.5", "-e",
                            # only into the game: a Return sent while another app kept the focus lands there
                            'tell application "System Events" to if (name of (first process whose frontmost is true)) '
                            'contains "mb_standalone" then key code 36'], capture_output=True)
            return True
        time.sleep(2)
    return False


def game_procs():
    """[(unix pid, cpu %, command)] of the game and of winedbg processes debugging it."""
    out = []
    for line in subprocess.run(["ps", "-axo", "pid=,pcpu=,command="], capture_output=True, text=True).stdout.splitlines():
        pid, cpu, cmd = line.strip().split(None, 2)
        if cmd.startswith(WINPATH) or ("winedbg" in cmd and "mb_standalone" in cmd):
            out.append((int(pid), float(cpu), cmd))
    return out


def dbg(a, cmds):
    return subprocess.run([CX, "--bottle", a.bottle, "--cx-app", "winedbg"], input=cmds, capture_output=True, text=True,
                          env=dict(os.environ, WINEDEBUG="-all"), timeout=120).stdout


def hang_dump(a, game, log_path):
    """The game spins under the debugger, which cannot be interrupted from outside: end that run (its own two
    processes, by pid), start the game again without a debugger, wait until it spins, then attach a debugger and
    dump every game thread; the game is ended afterwards."""
    for pid, _c, _cmd in game_procs():
        os.kill(pid, 9)
    time.sleep(2)
    q = subprocess.Popen([CX, "--bottle", a.bottle, "--cx-app", WINPATH], cwd=game, stdin=subprocess.DEVNULL,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=dict(os.environ, WINEDEBUG="-all"))
    t0 = time.time()
    while time.time() - t0 < a.timeout + 60 and not any(c > 50 for _p, c, cmd in game_procs() if cmd.startswith(WINPATH)):
        time.sleep(2)
    procs = dbg(a, "info process\nquit\n")
    m = re.search(r"([0-9a-f]{8}) +\d+ +\\_ 'mb_standalone.exe'", procs)
    if not m:
        return "no game process to attach to\n" + procs
    wpid = m[1]
    threads = dbg(a, "attach 0x%s\ninfo thread\ndetach\nquit\n" % wpid)
    block = threads[threads.find(wpid + " (D)"):] if wpid + " (D)" in threads else threads
    tids = re.findall(r"^\t([0-9a-f]{8})\s+-?\d+", block.split("\n0000")[0], re.M)
    cmds = "attach 0x%s\n" % wpid + "".join("thread 0x%s\ninfo reg\nbt 20\nx /256x $esp\n" % t for t in tids) + \
           "kill\nquit\n"
    text = dbg(a, cmds)
    open(log_path, "w").write(text)
    q.wait(30)
    return text


def report_text(text, hung=False):
    """Name the fault (or, for a hang, each thread's position) and the code addresses on each stack."""
    m = re.search(r"Unhandled exception: (.*)|(Stopped on breakpoint.*)", text)
    if not hung:
        print((m[1] or m[2]) if m else "exited without an exception")
    tool = os.path.join(os.path.dirname(os.path.abspath(__file__)), "addr2sym.py")
    blocks = [b for b in re.split(r"(?=EIP:[0-9a-f]{8})", text) if b.startswith("EIP:")]
    for blk in blocks:
        eip = blk[4:12]
        if not 0x401000 <= int(eip, 16) < 0x560000 and hung and "mb_standalone" not in blk:
            continue                                     # a thread waiting in the system with nothing of ours
        walk = []
        for line in blk.splitlines():
            mm = re.match(r"\s*0x0*([0-9a-f]+):\s+((?:[0-9a-f]{8}\s*)+)$", line)
            if mm:
                for i, w in enumerate(mm[2].split()):
                    v = int(w, 16)
                    if 0x401000 <= v < 0x560000:
                        walk.append((int(mm[1], 16) + 4 * i, v))
        bt = re.findall(r"^\s*=?>?\s*\d+ 0x0*([0-9a-f]+) in mb_standalone", blk, re.M)
        names = subprocess.run([sys.executable, tool, eip] + bt + ["%x" % v for _a, v in walk[:60]],
                               capture_output=True, text=True).stdout.splitlines()
        regs = re.search(r"EAX:.*\n.*ESI:.*", blk)
        print("thread at %s" % names[0] if names else eip)
        if regs:
            print("  " + regs[0].replace("\n", "\n  "))
        for n in names[1:1 + len(bt)]:
            print("  bt  " + n)
        for (at, _v), n in zip(walk[:60], names[1 + len(bt):]):
            print("  [%08x] %s" % (at, n))
    return 1

if __name__ == "__main__":
    sys.exit(main())
