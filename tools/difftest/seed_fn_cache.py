#!/usr/bin/env python3
"""Seed work/equiv_fn_cache.json from existing sweep results, so the first fingerprinted sweep skips unchanged rows.

usage: seed_fn_cache.py DIR [DIR ...]      (later dirs win; default: the sources docs/equiv_verdicts.json lists)
A row is seeded from its newest result only when the function's bytes in the newest cached object of its source that
existed when the result was written (the object that run compiled; work/difftest/obj keeps them all) equal the bytes in
today's object. A row edited since then is left to run."""
import glob, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sweep_nonmatching as SW   # noqa: E402
import slice as SL               # noqa: E402

ROOT = SW.ROOT
dirs = sys.argv[1:] or [os.path.join(ROOT, d) for d in json.load(open(os.path.join(ROOT, "docs", "equiv_verdicts.json")))["sources"]]
rows = {r["va"]: r for r in SW.rows() if r["src"]}
best = {}
for d in dirs:
    for js in glob.glob(os.path.join(d, "*.json")):
        va = os.path.basename(js).rsplit("_", 1)[-1][:-5]
        if va in rows and (va not in best or os.path.getmtime(js) >= os.path.getmtime(best[va])):
            best[va] = js
import fn_hash   # noqa: E402
by_stem = {}
for o in glob.glob(os.path.join(SL.OBJDIR, "*.obj")):
    by_stem.setdefault(os.path.basename(o).rsplit("_", 1)[0], []).append(o)
seeded = stale = bad = 0
fc = SW.fn_cache()
for va, js in best.items():
    r = rows[va]
    try:
        d = json.load(open(js)); d = d[0] if isinstance(d, list) and d else d
    except ValueError:
        bad += 1; continue
    if d.get("verdict") in (None, "CRASH", "TIMEOUT") or not d.get("symbol"):
        bad += 1; continue
    src = os.path.join(ROOT, r["src"])
    obj = SW.cached_obj(src, (d.get("flags") or "").split())
    if not obj:
        stale += 1; continue
    k = SW.row_key(r, d["symbol"], d.get("flags"), [])
    if k:
        # the result stands only if the function's bytes in the newest object that existed when it was written (the one
        # the run compiled) equal today's. (Not "today's object is older than the result": an edit that was later
        # reverted leaves today's object old while the result came from the edited one.)
        t = os.path.getmtime(js)
        then = [o for o in by_stem.get(SL._stem(src), []) if os.path.getmtime(o) <= t]
        old_sha = then and fn_hash.fn_sha(max(then, key=os.path.getmtime), d["symbol"])
        if not old_sha or old_sha != k.split("|")[0]:
            stale += 1; continue
    if k:
        fc[va] = {"key": k, "json": os.path.abspath(js), "verdict": d["verdict"], "symbol": d["symbol"], "flags": d.get("flags")}
        seeded += 1
SW.save_fn_cache()
print("%d rows seeded, %d changed since their last result (will run), %d without a usable result, %d rows in all"
      % (seeded, stale, bad, len(rows)))
