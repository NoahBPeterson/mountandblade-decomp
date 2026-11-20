"""One GUI run on the Mac's screen at a time: smoke.py (--host mac) and tools/port/run_native.py both drive the
keyboard/mouse and take screenshots, so two at once spoil each other (2026-11-20: a native run started a second after
a gate's smoke and the smoke scored 76.8). hold() blocks until work/locks/screen.lock is free and keeps it until the
process exits; a pgrep check before launching races the other tool's start, the lock does not."""
import fcntl, os, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PATH = os.path.join(ROOT, "work", "locks", "screen.lock")
_held_all = []


def hold(who, name="screen"):
    """name: the lock (work/locks/<name>.lock); "relink" serialises tools/relink/build.py runs in one tree."""
    path = os.path.join(os.path.dirname(PATH), name + ".lock")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    f = open(path, "a+")
    try:
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        f.seek(0)
        print("waiting for the " + name + " lock (held by: %s)" % (f.read().strip() or "?"), file=sys.stderr, flush=True)
        t = time.time()
        fcntl.flock(f, fcntl.LOCK_EX)
        print("got the " + name + " lock after %.0f s" % (time.time() - t), file=sys.stderr, flush=True)
    f.seek(0)
    f.truncate()
    f.write("%s pid %d\n" % (who, os.getpid()))
    f.flush()
    _held_all.append(f)
