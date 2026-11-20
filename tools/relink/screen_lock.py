"""One GUI run on the Mac's screen at a time: smoke.py (--host mac) and tools/port/run_native.py both drive the
keyboard/mouse and take screenshots, so two at once spoil each other (2026-11-20: a native run started a second after
a gate's smoke and the smoke scored 76.8). hold() blocks until work/locks/screen.lock is free and keeps it until the
process exits; a pgrep check before launching races the other tool's start, the lock does not."""
import fcntl, os, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PATH = os.path.join(ROOT, "work", "locks", "screen.lock")
_held = None


def hold(who):
    global _held
    os.makedirs(os.path.dirname(PATH), exist_ok=True)
    f = open(PATH, "a+")
    try:
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        f.seek(0)
        print("waiting for the screen (held by: %s)" % (f.read().strip() or "?"), file=sys.stderr, flush=True)
        t = time.time()
        fcntl.flock(f, fcntl.LOCK_EX)
        print("got the screen after %.0f s" % (time.time() - t), file=sys.stderr, flush=True)
    f.seek(0)
    f.truncate()
    f.write("%s pid %d\n" % (who, os.getpid()))
    f.flush()
    _held = f
