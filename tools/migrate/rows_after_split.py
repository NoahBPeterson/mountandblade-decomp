#!/usr/bin/env python3
"""rows_after_split.py: settle the manifest rows of the parts of a split file (restructure.py b).

usage: rows_after_split.py PART.cpp ...   (the parts of ONE old file)

1. A row whose symbol the part's object no longer defines (a static the part does not call any more, an implicit /
   inline function that is emitted where it is used) moves to the part whose object defines it.
2. A row whose bytes no longer match (chk.py) leaves the .manifest.txt for the part's .nonmatching.txt with the figure
   chk.py measures (aligned bytes), so nothing is dropped silently.
Prints what it did. Idempotent (a settled row is not in the manifest any more).
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.join(ROOT, "tools", "port"))
os.environ.setdefault("MB_PP", "0")
import slice as S      # noqa: E402
import objcmp as O     # noqa: E402

FLAGS = ["/O2", "/MT", "/Gy", "/TP"]


def rows(path):
    if not os.path.exists(path):
        return []
    return [l.rstrip("\n") for l in open(path, encoding="utf-8", errors="replace") if l.strip()]


def main(parts):
    syms = {}
    for p in parts:
        obj, log = S.compile_obj(os.path.abspath(os.path.join(ROOT, p)), FLAGS)
        if not obj:
            sys.exit("compile failed: %s\n%s" % (p, log[-800:]))
        from coff import Coff
        syms[p] = set(Coff(obj).functions())      # statics and COMDATs too (O.symbols suffixes a non-external name with its section)
    man = {p: rows(os.path.join(ROOT, p[:-4] + ".manifest.txt")) for p in parts}
    # 1. rows whose symbol lives in another part
    for p in parts:
        keep = []
        for l in man[p]:
            f = l.split()
            if f[1] in syms[p] or f[1].startswith(("$L", "_$E", "static", "compiler-generated")):
                keep.append(l); continue
            home = [q for q in parts if f[1] in syms[q]]
            if home:
                man[home[0]].append(l.replace("../" + p, "../" + home[0], 1) if ("../" + p) in l else l)
                print("moved  %s %s: %s -> %s" % (f[2], f[1][:40], os.path.basename(p), os.path.basename(home[0])))
            else:
                keep.append(l); print("LOST   %s %s: defined by no part" % (f[2], f[1][:50]))
        man[p] = keep
    for p in parts:
        # keep the manifest in address order (a moved row lands in place)
        man[p].sort(key=lambda l: int(l.split()[2], 16))
    # 2. rows that no longer match
    for p in parts:
        vas = [l.split()[2] for l in man[p]]
        if not vas:
            open(os.path.join(ROOT, p[:-4] + ".manifest.txt"), "w").write("")
            continue
        mp = os.path.join(ROOT, p[:-4] + ".manifest.txt")
        open(mp, "w").write("\n".join(man[p]) + "\n")
        ra = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "matching", "run_all.py"), "--jobs", "3", "--manifest", mp],
                            cwd=ROOT, capture_output=True, text=True)
        bad = {m.group(1).lower() for m in re.finditer(r"^(?:DIFF|FAIL)\s+\S+\s+([0-9a-fA-F]{8})\b", ra.stdout, re.M)}
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "matching", "chk.py"), p, ",".join(sorted(bad))],
                           cwd=ROOT, capture_output=True, text=True) if bad else None
        fig = {v: (1, 0) for v in bad}
        for m in re.finditer(r"^([0-9a-f]{8})\s+\d+\s+(\d+) diff\s+aligned (\d+)", r.stdout if r else "", re.M):
            fig[m.group(1)] = (max(1, int(m.group(2))), int(m.group(3)))
        keep, drop = [], []
        for l in man[p]:
            va = l.split()[2].lower()
            d, al = fig.get(va, (0, 0))
            (drop if d else keep).append((l, al))
        nmp = os.path.join(ROOT, p[:-4] + ".nonmatching.txt")
        nm = rows(nmp)
        for l, al in drop:
            f = l.split()
            nm.append("%s\t%d\t%s: byte-exact before the file was split (tools/migrate/restructure.py b); the part no longer sees the "
                      "callee bodies / statics of the old single object. Measured %d aligned bytes (chk.py)." % (f[2], al, f[1], al))
            print("demote %s %s: aligned %d" % (f[2], f[1][:50], al))
        nm.sort(key=lambda l: int(l.split()[0], 16))
        open(nmp, "w").write("\n".join(nm) + ("\n" if nm else ""))
        open(os.path.join(ROOT, p[:-4] + ".manifest.txt"), "w").write("\n".join(l for l, _ in keep) + ("\n" if keep else ""))


if __name__ == "__main__":
    main(sys.argv[1:])
