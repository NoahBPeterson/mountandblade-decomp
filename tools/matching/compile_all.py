#!/usr/bin/env python3
"""compile_all.py: compile every source that should compile (match/slices/*/<id>.cpp and src/**/*.cpp) and list
the ones that do not. run_all.py only compiles sources named in a manifest, so a slice or module whose rows are
all nonmatching can stop compiling unnoticed (s00520520 was committed with 16 copies of one block).

usage: compile_all.py [--jobs N] [--filter substr] [--json out.json]
Flags: the most common flags column of the source's manifest rows, else the defaults (/O2 /MT /Gy /TP).
Exit 0 iff everything compiled."""
import argparse, collections, glob, json, os, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
import slice as S                                   # noqa: E402

DEFAULT = ["/O2", "/MT", "/Gy", "/TP"]


def manifest_flags(path):
    if not os.path.exists(path):
        return None
    c = collections.Counter()
    for line in open(path, encoding="utf-8", errors="replace"):
        if not line.strip() or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) > 3:
            c[tuple(parts[3:])] += 1
    return list(c.most_common(1)[0][0]) if c else None


def sources():
    out = []
    for d in sorted(glob.glob(os.path.join(ROOT, "match", "slices", "*"))):
        sid = os.path.basename(d)
        src = os.path.join(d, sid + ".cpp")
        if os.path.exists(src):
            out.append((src, manifest_flags(os.path.join(d, "manifest.txt")) or DEFAULT))
    for src in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        out.append((src, manifest_flags(src[:-4] + ".manifest.txt") or DEFAULT))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--filter")
    ap.add_argument("--json")
    a = ap.parse_args()
    todo = [s for s in sources() if not a.filter or a.filter in s[0]]

    def one(item):
        src, flags = item
        obj, log = S.compile_obj(src, flags)
        return src, flags, obj is not None, log
    fails = []
    with ThreadPoolExecutor(a.jobs) as ex:
        for src, flags, ok, log in ex.map(one, todo):
            if not ok:
                errs = [l.strip() for l in (log or "").splitlines() if " error " in l or "fatal error" in l]
                fails.append({"src": os.path.relpath(src, ROOT), "flags": " ".join(flags), "errors": errs[:5]})
    for f in fails:
        print("FAIL %s  [%s]" % (f["src"], f["flags"]))
        for e in f["errors"][:3]:
            print("    " + e[-200:])
    print("%d / %d sources compiled" % (len(todo) - len(fails), len(todo)))
    if a.json:
        json.dump(fails, open(a.json, "w"), indent=1)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
