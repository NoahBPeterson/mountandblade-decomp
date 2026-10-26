#!/usr/bin/env python3
"""textbisect.py: find which spliced function hides the main-menu text, by bisection over a hybrid's hooks.

usage: textbisect.py [--from work/hybrid/mb.all.exe.hooks.json] [--tag tb]

Candidates are the hooks of a hybrid that shows the bug (built with build.py --hook-manifest all
--manifest-verdicts all). Each step builds a hybrid hooking half of the remaining candidates (the rest go to
--exclude) and runs textcheck.py on it: NOTEXT -> the culprit is in that half, TEXT -> in the other half.
Assumes one culprit; if both halves show text, the bug needs several hooks together and that is reported.
Log: work/hybrid/<tag>.log.
"""
import argparse, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PY = os.path.join(ROOT, ".venv", "bin", "python")


def has_text(keep, every, tag, n, log):
    ex = os.path.join(ROOT, "work", "hybrid", "%s_%d.exclude" % (tag, n))
    exe = os.path.join(ROOT, "work", "hybrid", "%s_%d.exe" % (tag, n))
    open(ex, "w").write("\n".join(sorted(set(every) - set(keep))) + "\n")
    b = subprocess.run([PY, os.path.join(HERE, "build.py"), "--hook-manifest", "all", "--manifest-verdicts", "all",
                        "--exclude", "@" + ex, "--out", exe], capture_output=True, text=True)
    if b.returncode != 0:
        raise SystemExit("build failed:\n" + b.stdout[-2000:] + b.stderr[-2000:])
    for _attempt in range(3):          # a boot that never reaches the menu is usually the launcher, not the hooks
        r = subprocess.run([PY, os.path.join(HERE, "textcheck.py"), exe, "--secs", "500", "--shot", exe[:-4] + ".png"],
                           capture_output=True, text=True)
        out = (r.stdout.strip().splitlines() or ["?"])[-1]
        if "NOMENU" not in out:
            break
    log.write("step %d: %d hooks -> %s\n" % (n, len(keep), out))
    log.flush()
    if "NOMENU" in out:
        raise SystemExit("step %d: no menu (crash?), see %s" % (n, exe))
    return "NOTEXT" not in out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from", dest="src", default=os.path.join(ROOT, "work", "hybrid", "mb.all.exe.hooks.json"))
    ap.add_argument("--tag", default="tb")
    ap.add_argument("--cand", help="file of candidate VAs to start from (a step already narrowed down)")
    a = ap.parse_args()
    every = [h["va"] for h in json.load(open(a.src))["hooks"]]
    cand = open(a.cand).read().split() if a.cand else list(every)
    log = open(os.path.join(ROOT, "work", "hybrid", a.tag + ".log"), "a")
    n = 0
    while len(cand) > 1:
        half, rest = cand[:len(cand) // 2], cand[len(cand) // 2:]
        n += 1
        if not has_text(half, every, a.tag, n, log):
            cand = half
        else:
            n += 1
            if has_text(rest, every, a.tag, n, log):
                log.write("both halves show text: the bug needs hooks from both (%d candidates)\n" % len(cand))
                print("both halves show text; candidates:", " ".join(cand))
                return 1
            cand = rest
        print("step %d: %d candidates left" % (n, len(cand)), flush=True)
    log.write("culprit: %s\n" % cand[0])
    print("culprit:", cand[0])
    return 0


if __name__ == "__main__":
    sys.exit(main())
