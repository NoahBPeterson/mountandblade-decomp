#!/usr/bin/env python3
"""Verify a set of slices: run_all.py on each manifest (MATCH count vs rows) and the bookkeeping gate
(every batch VA of the slice in exactly one of manifest/nonmatching/partial; no foreign VAs).
usage: verify_slices.py <slice-list.json | id ...> [--json out.json]
A slice list file is a JSON list of ids (or of {"ids": [...]} groups, as work/wf1_done.json)."""
import glob, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = lambda *p: os.path.join(ROOT, *p)
args = [a for a in sys.argv[1:] if not a.startswith("--")]
out_json = sys.argv[sys.argv.index("--json") + 1] if "--json" in sys.argv else None
if out_json in args:
    args.remove(out_json)
ids = []
for a in args:
    if a.endswith(".json"):
        for x in json.load(open(a)):
            ids += x["ids"] if isinstance(x, dict) else [x]
    else:
        ids.append(a)
vas_of = {}
for f in glob.glob(W("work/batches/*.json")):
    for s in json.load(open(f)):
        vas_of.setdefault(s["id"], {v.lower().zfill(8) for v in s["vas"]})


def rows(path, col):
    out = []
    if os.path.exists(path):
        for line in open(path):
            t = line.split("#", 1)[0].split()
            if len(t) > col:
                out.append(t[col].lower().replace("0x", "").zfill(8))
    return out


report = {}
for sid in ids:
    d = W("match/slices", sid)
    man = rows(os.path.join(d, "manifest.txt"), 2)
    seen = {}
    for f, col in (("manifest.txt", 2), ("nonmatching.txt", 0), ("partial.txt", 0)):
        for v in rows(os.path.join(d, f), col):
            seen.setdefault(v, []).append(f)
    want = vas_of.get(sid, set())
    book = {"missing": sorted(want - set(seen)), "dup": sorted(v for v, f in seen.items() if len(f) > 1),
            "foreign": sorted(set(seen) - want)}
    exact = 0
    if man:
        r = subprocess.run([sys.executable, W("tools/matching/run_all.py"), "--manifest", os.path.join(d, "manifest.txt")],
                           capture_output=True, text=True)
        m = re.search(r"(\d+) / (\d+) functions byte-exact", r.stdout)
        exact = int(m.group(1)) if m else 0
        bad = [l for l in r.stdout.splitlines() if l and not l.startswith(("MATCH", "\n")) and "byte-exact" not in l]
    else:
        bad = []
    report[sid] = {"rows": len(man), "exact": exact, "nonmatching": len(rows(os.path.join(d, "nonmatching.txt"), 0)),
                   "partial": len(rows(os.path.join(d, "partial.txt"), 0)), "functions": len(want), "book": book,
                   "failed_rows": bad[:10]}
    ok = exact == len(man) and not any(book.values())
    print("%s %s exact %d/%d rows, nonmatching %d, partial %d, of %d%s" % (
        sid, "OK  " if ok else "FAIL", exact, len(man), report[sid]["nonmatching"], report[sid]["partial"], len(want),
        "" if not any(book.values()) else "  book=" + json.dumps({k: v for k, v in book.items() if v})), flush=True)
if out_json:
    json.dump(report, open(out_json, "w"), indent=1)
T = lambda k: sum(r[k] for r in report.values())
print("TOTAL slices %d: exact %d of %d manifest rows, nonmatching %d, partial %d, functions %d; slices failing %d" % (
    len(report), T("exact"), T("rows"), T("nonmatching"), T("partial"), T("functions"),
    sum(1 for r in report.values() if r["exact"] != r["rows"] or any(r["book"].values()))))
