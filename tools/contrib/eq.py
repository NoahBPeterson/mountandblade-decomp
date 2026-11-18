#!/usr/bin/env python3
"""eq: tools/difftest/equiv.py behind a machine-wide limit of 6 runs at once (16 GB Mac; wave sessions use this).

  .venv/bin/python tools/contrib/eq.py <module stem> 0x<va> [equiv.py options]

Same arguments as equiv.py (e.g. `eq.py agent_logic 0x43bdb0 --seed 7`). Waits for a free slot, then runs it with
--time-limit 120 unless you give one. Verdicts: PASS (every branch outcome taken, all results identical), WEAK
(identical, some branch never taken), FAIL (a mismatch: the behaviour differs, so the change is wrong).
"""
import fcntl, json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SLOTS = 6
D = os.path.join(os.path.expanduser("~"), ".cache", "mb-decomp", "equiv_slots")


def main():
    a = sys.argv[1:]
    if not a or a[0] in ("-h", "--help"):
        print(__doc__); return
    if len(a) < 2:
        sys.exit("usage: eq.py <module stem> 0x<va> [options]")
    stem, va = a[0], "%08x" % int(a[1], 16)
    py = os.path.join(ROOT, ".venv", "bin", "python")
    t = subprocess.run([py, "tools/contrib/task.py", "0x" + va], cwd=ROOT, capture_output=True, text=True).stdout
    m = re.search(r"^source\s+(\S+\.cpp)", t, re.M)
    try:   # the sweep's per-row options (tools/difftest/sweep_nonmatching.py row_opts)
        opts = json.load(open(os.path.join(ROOT, "tools", "difftest", "row_opts.json"))).get("%s %s" % (stem, va), {}).get("args", [])
    except (OSError, ValueError):
        opts = []
    a = [stem, va, "--fp-tol", "--max-mismatch", "200"] + opts + a[2:]
    if m and "--src" not in a:
        a += ["--src", os.path.join(ROOT, m.group(1))]
    if "--time-limit" not in a:
        a += ["--time-limit", "120"]
    os.makedirs(D, exist_ok=True)
    lock = None
    while lock is None:
        for k in range(SLOTS):
            fh = open(os.path.join(D, str(k)), "w")
            try:
                fcntl.flock(fh, fcntl.LOCK_EX | fcntl.LOCK_NB); lock = fh; break
            except OSError:
                fh.close()
        else:
            time.sleep(3)
    sys.stdout.flush()
    sys.exit(subprocess.run([os.path.join(ROOT, ".venv", "bin", "python"), "tools/difftest/equiv.py"] + a,
                            cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
