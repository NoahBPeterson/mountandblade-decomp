#!/usr/bin/env python3
"""Record a sweep's EQUIVALENT results (tools/equiv/sweep.py) in docs/equiv_proofs.json (maintainers).

  uv run python tools/equiv/record_sweep.py work/equiv/angr_sweep.jsonl [more.jsonl ...] [--dry-run] [--force]

Ported from spore-decomp tools/equiv/record_sweep.py (commit 82de4ab6c), for M&B's record (docs/equiv_proofs.json
rows + bundles, tools/equiv/prove.py record()) instead of spore's equivalent.txt; M&B has no pull-request claims or
known-failure list, and MATCH rows are not recorded here (byte-exact rows are insn_exact.py's business).
For each EQUIVALENT row: our function is compiled again (the object cache) and its build hash compared with the one
in the row's bundle. Equal: the proof is about this very build and is recorded as it is. Different (the source
changed since the sweep) or no bundle: it is proved again with the layer that proved it, and recorded only if that
proof holds (with its mutation self-test). Rows already recorded with the same build hash are skipped (--force
records them again). Prints what it did.
"""
import argparse, json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("results", nargs="+")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    import func as FN
    import prove
    doc = prove.load_proofs()
    have = doc.setdefault("rows", {})
    rows = {}
    for path in a.results:
        for ln in open(path):
            if ln.strip():
                r = json.loads(ln)
                if r.get("verdict") == "EQUIVALENT":
                    rows[r["va"]] = r                 # a later file wins
    n = {"recorded": 0, "re-proved": 0, "skipped": 0, "failed": 0}
    for k, r in sorted(rows.items()):
        va = int(k, 16)
        b = r.get("bundle")
        try:
            ours = FN.our_func(os.path.join(ROOT, r["src"]), va, r.get("symbol"))
        except SystemExit as e:
            n["failed"] += 1
            print("FAILED %s: cannot build ours: %s" % (k, str(e)[:200]))
            continue
        cur = FN.build_hash(ours)
        if not a.force and have.get(k, {}).get("build") == cur:
            n["skipped"] += 1
            continue
        if b and b.get("build") == cur:
            if a.dry_run:
                print("would record %s (%s %s)" % (k, r["layer"], r["level"]))
                continue
            prove.record(va, {"bundle": b, "detail": r.get("detail")}, r["src"], doc, quiet=True)
            n["recorded"] += 1
            print("recorded %s (%s %s)" % (k, r["layer"], r["level"]))
            continue
        if a.dry_run:
            print("would re-prove %s with %s (%s)" % (k, r["layer"], "build changed" if b else "no bundle"))
            continue
        try:
            res = prove.run(os.path.join(ROOT, r["src"]), va, r.get("symbol"), level=r.get("level"),
                            layers=[r["layer"]], log=lambda s: None)
        except SystemExit as e:
            res = {"verdict": "ERROR", "detail": str(e)}
        if res["verdict"] == "EQUIVALENT":
            prove.record(va, res, r["src"], doc, quiet=True)
            n["re-proved"] += 1
            print("re-proved and recorded %s (%s %s)" % (k, res["layer"], res["level"]))
        else:
            n["failed"] += 1
            print("FAILED %s: %s %s" % (k, res["verdict"], (res.get("detail") or "")[:200]))
    if not a.dry_run and (n["recorded"] or n["re-proved"]):
        prove.save_proofs(doc)
    print(n)
    return 1 if n["failed"] else 0


if __name__ == "__main__":
    sys.exit(main())
