#!/usr/bin/env python3
"""task: everything needed to work on one function (a "context pack", printed). Ported from spore-decomp.

  .venv/bin/python tools/contrib/task.py 0x004e3eb0
  .venv/bin/python tools/contrib/task.py --list [--max-aligned 16] [--max-size 1500] [--max-tries 2]

One function: its source file, status (byte-exact / complete but not byte-exact yet), the row's note, compiler flags,
the original's size, the file's includes, the function's current source, and what to do next.

--list: open work, closest to a match first (the aligned figure of its .nonmatching.txt row: bytes of the instructions
that still differ), then smallest. "tries" counts commits that rewrote the row's note (one per earlier attempt that
did not reach a match; bulk commits touching more than 30 rows are not counted): a row many have tried is a poor
first pick. Rows whose aligned figure is 0 are symbol-less fragments, not work, and are left out.
"""
import argparse, collections, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path[:0] = [os.path.join(ROOT, "tools", "matching"), os.path.join(ROOT, "tools", "difftest")]


def rows():
    """[(va, aligned, note, src, nonmatching file)] from every src/**/*.nonmatching.txt."""
    out = []
    for nm in glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True):
        src = nm[:-len(".nonmatching.txt")] + ".cpp"
        for l in open(nm, encoding="utf-8", errors="replace"):
            p = l.rstrip("\n").split(None, 2)
            if len(p) < 2 or not re.fullmatch(r"[0-9a-fA-F]{8}", p[0]):
                continue
            try:
                al = int(p[1])
            except ValueError:
                continue
            out.append((int(p[0], 16), al, p[2] if len(p) > 2 else "", src, nm))
    return out


def manifest_row(va):
    for mf in glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True):
        for l in open(mf, encoding="utf-8", errors="replace"):
            p = l.split("#", 1)[0].split()
            if len(p) >= 3:
                try:
                    if int(p[2], 16) == va:
                        return mf, p
                except ValueError:
                    pass
    return None, None


def tries():
    """{va: commits that rewrote its nonmatching note} (commits touching > 30 rows are bookkeeping, not attempts)."""
    log = subprocess.run(["git", "log", "-p", "--format=@@C %H", "--", "*.nonmatching.txt"], cwd=ROOT,
                         capture_output=True, text=True, errors="replace").stdout
    per, cur = collections.defaultdict(set), None
    for l in log.splitlines():
        if l.startswith("@@C "):
            cur = l[4:]
        else:
            m = re.match(r"\+([0-9a-f]{8})\s", l)
            if m:
                per[cur].add(int(m.group(1), 16))
    n = collections.Counter()
    for vas in per.values():
        if len(vas) <= 30:
            n.update(vas)
    return n


def orig_size(va):
    from card import load_funcs, bounds
    starts, _ = load_funcs()
    em = set()
    for l in open(os.path.join(ROOT, "symbols", "extent_merge.txt")):
        p = l.split("#", 1)[0].split()
        if p:
            em.add(int(p[0], 16))
    return len(bounds(va, [a for a in starts if a not in em]))


def body(src, va):
    t = open(src, errors="replace").read()
    m = re.search(r"(?mi)^//.*@\s*0x0*%x\b.*$" % va, t)
    if not m:
        return None
    i = t.find("{", m.end())
    depth, j = 0, i
    while 0 <= j < len(t):
        depth += {"{": 1, "}": -1}.get(t[j], 0)
        if depth == 0:
            break
        j += 1
    return t[m.start():j + 1]


def flags_for(src):
    mf = src[:-4] + ".manifest.txt"
    if os.path.exists(mf):
        for l in open(mf):
            p = l.split("#", 1)[0].split()
            if len(p) >= 4 and os.path.basename(p[0]) == os.path.basename(src):
                return p[3:]
    return ["/O2", "/MT", "/Gy", "/TP"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("va", nargs="?")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--max-aligned", type=int, default=64)
    ap.add_argument("--max-size", type=int, default=1500)
    ap.add_argument("--max-tries", type=int, default=99)
    a = ap.parse_args()
    if a.list:
        t = tries()
        sizes = {}
        out = []
        for va, al, note, src, _nm in rows():
            if not (0 < al <= a.max_aligned) or t.get(va, 0) > a.max_tries:
                continue
            if body(src, va) is None:
                continue
            sizes[va] = orig_size(va)
            if sizes[va] > a.max_size:
                continue
            out.append((al, sizes[va], va, t.get(va, 0), os.path.relpath(src, ROOT), note))
        out.sort()
        print("%d functions with complete source that are not byte-exact yet (aligned <= %d, size <= %d), closest first:"
              % (len(out), a.max_aligned, a.max_size))
        print("  %-10s  %7s  %5s  %5s  %s" % ("function", "aligned", "size", "tries", "source / last note"))
        for al, size, va, n, src, note in out:
            print("  0x%08x  %7d  %5d  %5d  %s  %s" % (va, al, size, n, src, note[:80]))
        return 0
    if not a.va:
        ap.error("give a function address, or --list")
    va = int(a.va, 16)
    hit = [r for r in rows() if r[0] == va]
    mf, mrow = manifest_row(va)
    print("function   0x%08x   (original: %d bytes)" % (va, orig_size(va)))
    if mrow and not hit:
        print("status     byte-exact already (%s): pick another" % os.path.relpath(mf, ROOT))
        return 0
    if not hit:
        print("status     no source row: no .nonmatching.txt lists it")
        return 0
    _va, al, note, src, nm = hit[0]
    print("source     %s" % os.path.relpath(src, ROOT))
    print("status     complete, not byte-exact yet: %d aligned bytes differ (%s)" % (al, os.path.relpath(nm, ROOT)))
    print("tries      %d earlier attempt(s) rewrote its note" % tries().get(va, 0))
    print("note       %s" % note[:600])
    print("flags      %s" % " ".join(flags_for(src)))
    incs = re.findall(r'(?m)^\s*#\s*include\s*([<"][^">]+[">])', open(src, errors="replace").read())
    print("includes   %s" % (" ".join(incs) or "none"))
    b = body(src, va)
    print("\n----- current source -----\n%s\n--------------------------\n" % (b or "(no `// @ 0x%x` marker found)" % va))
    print("next: edit the function in %s (other functions, headers and class layouts may change too),\n"
          "      then: .venv/bin/python tools/contrib/check.py 0x%08x\n"
          "      Rules and the full loop: AGENTS.md" % (os.path.relpath(src, ROOT), va))
    return 0


if __name__ == "__main__":
    sys.exit(main())
