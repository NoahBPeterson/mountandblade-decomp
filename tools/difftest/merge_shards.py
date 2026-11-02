#!/usr/bin/env python3
"""merge_shards.py: combine the JSON results of `equiv.py --shard K/N` runs of one row into one verdict.

usage: merge_shards.py OUT.json SHARD.json [SHARD.json ...]

The shards partition the saved (regress / cover) inputs, so together they replay each exactly once, and each draws
fresh inputs from its own seed. Every part of the gate combines soundly across them:
  * mismatches: any shard with a mismatch makes the row FAIL (its first mismatch is reported);
  * valid inputs: each is a real comparison of both builds, so the counts add;
  * edges: an edge one shard covered was executed, so coverage is the union. An edge stays missed unless some shard
    covered it; it counts as unreachable only when EVERY shard listed it as unreachable (with that shard's reason).
The verdict thresholds are equiv.py's: FAIL on any mismatch; UNSUPPORTED when valid < max(10, inputs // 20) or a
shard is UNSUPPORTED for a reason other than a shortage of valid inputs / wall time; PASS when valid >= 200 and
neither side misses an edge; WEAK otherwise. A -FP suffix on any shard carries over.
"""
import json, sys

MIN_VALID = 200


def load(p):
    d = json.load(open(p))
    return d[0] if isinstance(d, list) else d


def main():
    out_path, paths = sys.argv[1], sys.argv[2:]
    shards = [load(p) for p in paths]
    assert shards, "no shards"
    names = {(d.get("slice"), d.get("va")) for d in shards}
    assert len(names) == 1, "shards of different rows: %s" % names
    tags = sorted(d.get("shard") or "?" for d in shards)
    ns = {t.split("/")[-1] for t in tags}
    assert len(ns) == 1 and len(tags) == int(ns.pop()) and len(set(tags)) == len(tags), \
        "need exactly one result per shard K/N, got %s" % tags

    valid = sum(d.get("valid", 0) for d in shards)
    ran = sum(d.get("inputs", 0) for d in shards)
    disc = {}
    for d in shards:
        for k, v in (d.get("discarded") or {}).items():
            disc[k] = disc.get(k, 0) + v
    fp = any(str(d.get("verdict", "")).endswith("-FP") for d in shards)
    edges = {}
    for side in ("orig", "ours"):
        notcov, unr_all, total = None, None, None
        for d in shards:
            e = (d.get("edges") or {}).get(side) or {}
            total = e.get("total", total)
            nc = set(e.get("missed", [])) | set(e.get("unreachable", []))
            un = set(e.get("unreachable", []))
            notcov = nc if notcov is None else notcov & nc
            unr_all = un if unr_all is None else unr_all & un
        notcov, unr_all = notcov or set(), unr_all or set()
        unreachable = sorted(notcov & unr_all)
        missed = sorted(notcov - unr_all)
        total = total or 0
        edges[side] = {"total": total, "covered": total - len(notcov), "required": total - len(unreachable),
                       "unreachable": unreachable, "missed_n": len(missed), "missed": missed}

    fails = [d for d in shards if d.get("verdict") == "FAIL"]
    hard = [d for d in shards if d.get("verdict") == "UNSUPPORTED" and d.get("valid", 0) > 0 and
            "time limit" not in str(d.get("reason", "")) and "valid" not in str(d.get("reason", ""))]
    ets = "; ".join("%s %d/%d" % (s, edges[s]["covered"], edges[s]["total"]) for s in ("orig", "ours"))
    if fails:
        f = fails[0]
        v = "FAIL"
        why = "shard %s: %s" % (f.get("shard"), f.get("reason"))
    elif hard:
        v, why = "UNSUPPORTED", "shard %s: %s" % (hard[0].get("shard"), hard[0].get("reason"))
    elif valid < max(10, ran // 20):
        top = sorted(disc.items(), key=lambda x: -x[1])[:3]
        v, why = "UNSUPPORTED", "only %d/%d inputs valid (discards: %s)" % (valid, ran, ", ".join("%s=%d" % x for x in top))
    else:
        need = []
        if valid < MIN_VALID:
            need.append("%d valid inputs (need %d)" % (valid, MIN_VALID))
        for s in ("orig", "ours"):
            if edges[s]["missed_n"]:
                need.append("%s misses %d edge(s)" % (s, edges[s]["missed_n"]))
        v = "WEAK" if need else "PASS"
        why = ("%s; edges %s" % (", ".join(need), ets)) if need else \
            "%d valid inputs, all edges covered (%s)" % (valid, ets)
        if fp:
            v += "-FP"
    res = dict(shards[0])
    res.update(verdict=v, reason="%s [merged from %d shards]" % (why, len(shards)), valid=valid, inputs=ran,
               discarded=disc, edges=edges, mismatches=sum(d.get("mismatches", 0) if isinstance(d.get("mismatches"), int)
                                                          else len(d.get("mismatches") or []) for d in shards),
               first_mismatch=(fails[0].get("first_mismatch") if fails else None),
               seconds=max(d.get("seconds", 0) for d in shards), shard=None,
               merged_shards=[{"shard": d.get("shard"), "seed": d.get("seed"), "verdict": d.get("verdict"),
                               "valid": d.get("valid"), "inputs": d.get("inputs"), "seconds": d.get("seconds"),
                               "reason": d.get("reason")} for d in shards])
    json.dump([res], open(out_path, "w"), indent=1, default=str)
    print("%s  %s" % (v, res["reason"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
