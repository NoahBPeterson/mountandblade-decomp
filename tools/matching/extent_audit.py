#!/usr/bin/env python3
"""extent_audit.py: for every manifest row: does our compiled COMDAT extend past the index extent of the VA (so run_all compared
only a prefix: functions with EH handlers are split at the handler in the index)? If so, byte-compare the whole COMDAT (relocations masked) against the original."""
import glob, json, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "difftest"))
os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import slice as S, coff, equiv as E
img = E.context()["img"]
rows = []
for m in glob.glob("match/slices/*/manifest.txt") + glob.glob("src/**/*.manifest.txt", recursive=True):
    for l in open(m):
        p = l.split("#", 1)[0].split()
        if len(p) >= 3:
            try:
                rows.append((m, os.path.normpath(os.path.join("match", p[0])), p[1], int(p[2], 16), p[3:] or ["/O2", "/MT", "/Gy", "/TP"]))
            except ValueError:
                pass
cache = {}
out = {"short_ok": 0, "short_bad": [], "fit": 0, "err": 0}
for m, src, sym, va, fl in rows:
    k = (src, tuple(fl))
    try:
        if k not in cache:
            obj, _ = S.compile_obj(src, list(fl))
            cache[k] = coff.Coff(obj) if obj else None
        c = cache[k]
        s = c.by_name.get(sym) if c else None
        if s is None:
            out["err"] += 1; continue
        sec = c.sections[s.secno - 1]
        n = len(sec.data) - s.value
        lo, hi = img.func_extent(va)
        if n <= hi - lo:
            out["fit"] += 1; continue
        mask = set()
        for o, _si, _t in sec.relocs:
            mask |= set(range(o, o + 4))
        orig = img.read(va, n)
        d = [i for i in range(n) if (s.value + i) not in mask and sec.data[s.value + i] != orig[i]]
        if d:
            out["short_bad"].append(["%08x" % va, m, n, hi - lo, len(d), "%08x" % (va + d[0])])
        else:
            out["short_ok"] += 1
    except Exception as e:
        out["err"] += 1
print(json.dumps({k: (v if not isinstance(v, list) else len(v)) for k, v in out.items()}))
for r in out["short_bad"][:40]:
    print(*r)
