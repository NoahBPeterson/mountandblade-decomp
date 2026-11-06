#!/usr/bin/env python3
"""sweep_status.py: what a sweep directory holds right now. Safe to run while the sweep is writing.

usage: sweep_status.py [DIR=work/sweep_1105] [--fails] [--row NAME] [--cut]
  --fails     list the FAIL rows: mismatched / valid inputs and what differed first
  --cut       list the rows still without a verdict because they ran out of time
  --row NAME  one row's whole result, e.g. --row camera_0049c2c0
"""
import argparse, collections, glob, json, os, re


def load(path):
    try:
        d = json.load(open(path))
        return d[0] if isinstance(d, list) and d else d
    except (OSError, ValueError):
        return None                     # being replaced this moment


def kind(d):
    r, v = d.get("reason") or "", d.get("verdict")
    m = re.search(r"time limit (\d+)s", r)
    if m and v == "UNSUPPORTED":
        return "Cut short at %s s" % m[1]
    if "symbol not found" in r:
        return "No function to call (checked by comdat_check)"
    if v == "UNSUPPORTED":
        return "No verdict, other reasons"
    return v or "?"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir", nargs="?", default="work/sweep_1105")
    ap.add_argument("--fails", action="store_true")
    ap.add_argument("--cut", action="store_true")
    ap.add_argument("--row")
    a = ap.parse_args()
    if a.row:
        print(json.dumps(load(os.path.join(a.dir, a.row + ".json")), indent=1))
        return
    rows = {}
    for f in glob.glob(os.path.join(a.dir, "*.json")):
        d = load(f)
        if d and os.path.basename(f) != "tally.json":
            rows[os.path.basename(f)[:-5]] = d
    c = collections.Counter(kind(d) for d in rows.values())
    early = sum(1 for d in rows.values() if d.get("stopped_full_coverage"))
    order = ["PASS", "PASS-FP", "WEAK", "WEAK-FP", "FAIL"]
    w = max([len(k) for k in c] + [6])
    print("%-*s  %5s" % (w, "Result", "Rows"))
    for k in order + sorted(k for k in c if k not in order):
        if c[k]:
            print("%-*s  %5d" % (w, k, c[k]))
    print("%-*s  %5d" % (w, "Total", len(rows)))
    print("PASS rows that stopped early at 100%% coverage: %d" % early)
    if a.fails:
        print()
        for n, d in sorted(rows.items()):
            if d.get("verdict") == "FAIL":
                fm = d.get("first_mismatch") or {}
                print("%-34s %4s/%-4s %s" % (n, d.get("mismatches"), d.get("valid"), str(fm.get("observable"))[:50]))
    if a.cut:
        print()
        for n, d in sorted(rows.items()):
            if kind(d).startswith("Cut short"):
                print("%-34s %s" % (n, kind(d)))


if __name__ == "__main__":
    main()
