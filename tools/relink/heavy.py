"""heavy.py: one machine-wide gate for the tools that start worker pools (resolve_map, data_size_audit, call_diff,
layout_check). Each worker loads the whole symbol / image analysis (about 1-1.5 GB), and several lanes plus the
coordinator used to start pools at the same time (20 workers on a 16 GB machine). `pool_jobs` waits until no other
heavy pool is running anywhere on the machine (a lock file shared by every worktree), then sizes the pool to the memory
that is actually free; release happens when the process exits.

    import heavy
    jobs = heavy.pool_jobs(requested)          # blocks until this process holds the lock
    with multiprocessing.Pool(jobs) as pool: ...
"""
import fcntl, os, re, subprocess, sys, time

LOCK = os.path.join(os.path.expanduser("~"), ".cache", "mb-decomp", "heavy_pool.lock")
PER_WORKER = 1.6 * 2**30        # resident size of one analysis worker, with headroom
RESERVE = 3 * 2**30             # left for the system, the game and the coordinating process
_held = None


def available_bytes():
    """Free + inactive + speculative + purgeable pages (macOS vm_stat); /proc/meminfo MemAvailable elsewhere."""
    try:
        out = subprocess.run(["vm_stat"], capture_output=True, text=True).stdout
        page = int(re.search(r"page size of (\d+)", out).group(1))
        n = 0
        for key in ("Pages free", "Pages inactive", "Pages speculative", "Pages purgeable"):
            m = re.search(key + r":\s+(\d+)", out)
            if m:
                n += int(m.group(1))
        return n * page
    except (OSError, AttributeError):
        try:
            for line in open("/proc/meminfo"):
                if line.startswith("MemAvailable:"):
                    return int(line.split()[1]) * 1024
        except OSError:
            pass
    return 8 * 2**30


def pool_jobs(requested):
    """Wait for the machine-wide heavy-pool lock, then return how many workers fit in free memory (at least 1,
    at most `requested`)."""
    global _held
    if _held is None:
        os.makedirs(os.path.dirname(LOCK), exist_ok=True)
        f = open(LOCK, "a+")
        try:
            fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            sys.stderr.write("heavy: another worker pool is running; waiting for it (%s)\n" % LOCK)
            t0 = time.time()
            fcntl.flock(f, fcntl.LOCK_EX)
            sys.stderr.write("heavy: waited %d s\n" % (time.time() - t0))
        _held = f                               # kept open (and locked) until this process exits
    fit = int((available_bytes() - RESERVE) // PER_WORKER)
    jobs = max(1, min(int(requested or 1), fit))
    if jobs < (requested or 1):
        sys.stderr.write("heavy: %d workers instead of %d (%.1f GB free)\n" % (jobs, requested, available_bytes() / 2**30))
    return jobs
