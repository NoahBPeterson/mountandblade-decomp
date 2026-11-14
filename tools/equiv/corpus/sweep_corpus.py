#!/usr/bin/env python3
"""Run the prover stack over a stratified sample of the cross-compiler corpus (build_pairs.py, index_pairs.py) and
summarise proof rates per size bucket and the blockers.

  uv run python tools/equiv/corpus/sweep_corpus.py [--corpus work/fnv_corpus] [--n 300] [--ident 50] [--seed 1]
        [--jobs 5] [--remote] [--remote-only] [--frame-model] [--pass1 angr=60,region=300] [--pass2 angr=120,region=1200]
        [--out work/equiv/corpus/baseline.jsonl] [--resume] [--summary-only] [--json-summary F]

Every pair is equivalent by construction (one source, two compilers; UB and compiler bugs aside), so a pair that
does not prove is a blocker of the prover. Each pair runs in its own process (prove.py --objpair, no mutants,
layers rename,angr,region), first with the --pass1 timeouts, then the pairs whose first run ended in a timeout
again with --pass2. Sample: --n differing pairs (same-frame or frame-size) spread evenly over the size buckets
(instructions of the cl 14 side), within a bucket pairs with a frame-size difference or an escaping frame address
weighted 3:1, and --ident identical pairs (positive controls, run without the byte-exact shortcut).

--remote runs angr on the laptop (tools/equiv/remote.py); --remote-only also refuses the local fallback (a busy or
unreachable laptop then fails the region as a worker error instead of starting angr on this Mac). Results go to a
jsonl under work/ (the corpus has no licence: never commit its rows; the summary's counts are ours).
"""
import argparse, collections, json, os, random, re, signal, subprocess, sys, threading, time
from concurrent.futures import ThreadPoolExecutor, as_completed

HERE = os.path.dirname(os.path.abspath(__file__))
EQ = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(EQ))
BUCKETS = ((0, 20), (20, 50), (50, 100), (100, 300), (300, 1000), (1000, 1 << 30))


def bucket(n):
    for lo, hi in BUCKETS:
        if lo <= n < hi:
            return "%d-%s" % (lo, hi if hi < 1 << 30 else "")
    return "?"


def sample(rows, n, ident, seed):
    rng = random.Random(seed)
    diff = [r for r in rows if r["cls"] in ("same-frame", "frame-size")]
    per = {b: [r for r in diff if bucket(r["n_b"]) == bucket(b[0])] for b in BUCKETS}
    want, left, out = {}, n, []
    # an even share per bucket, the remainder of the small buckets to the others
    order = sorted(BUCKETS, key=lambda b: len(per[b]))
    for k, b in enumerate(order):
        want[b] = min(len(per[b]), left // (len(order) - k))
        left -= want[b]
    for b in BUCKETS:
        pool = list(per[b])
        rng.shuffle(pool)
        w = [3 if (r["cls"] == "frame-size" or r["esc_a"] or r["esc_b"]) else 1 for r in pool]
        pick = []
        while pool and len(pick) < want[b]:
            k = rng.choices(range(len(pool)), weights=w)[0]
            pick.append(pool.pop(k))
            w.pop(k)
        out += pick
    ids = [r for r in rows if r["cls"] == "identical"]
    rng.shuffle(ids)
    ids.sort(key=lambda r: bucket(r["n_b"]))
    step = max(1, len(ids) // max(1, ident))
    out += ids[::step][:ident]
    return out


def key(r):
    return "%s|%s" % (r["source"], r["symbol"])


def run_one(r, corpus, stems, tmo, remote, remote_only, frame_model, passno):
    st = stems[r["source"]]
    a = os.path.join(corpus, "vc71", st + ".obj")
    b = os.path.join(corpus, "vc8", st + ".obj")
    total = 30 + sum(v for k, v in tmo.items()) + 120
    cmd = ["uv", "run", "--quiet", "python", os.path.join(EQ, "prove.py"), "--objpair", a, b, r["symbol"],
           "--layers", "rename,angr,region", "--mutants", "0", "--json",
           "--layer-timeouts", ",".join("%s=%g" % kv for kv in sorted(tmo.items()))]
    if r["cls"] == "identical":
        cmd.append("--no-match-shortcut")
    if remote:
        cmd.append("--remote")
    env = dict(os.environ)
    if remote_only:
        env["EQUIV_ANGR_LOCAL"] = "/usr/bin/false"
    if frame_model:
        env["EQUIV_FRAME_MODEL"] = "1"
    if "region" in tmo:
        env["EQUIV_REGION_TIMEOUT"] = str(min(120.0, max(30.0, tmo["region"] / 5)))
    t = time.time()
    p = subprocess.Popen(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
                         start_new_session=True)
    try:
        out, _ = p.communicate(timeout=total)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL)
        p.communicate()
        out = b""
        res = {"verdict": "UNKNOWN", "detail": "sweep: hard timeout after %.0fs" % total, "layers": {}}
    else:
        txt = out.decode(errors="replace")
        k = txt.find("\n{")
        res = None
        if k >= 0:
            try:
                res = json.loads(txt[k + 1:])
            except ValueError:
                res = None
        if res is None:
            res = {"verdict": "ERROR", "detail": txt[-400:], "layers": {}}
    row = {"key": key(r), "cls": r["cls"], "bucket": bucket(r["n_b"]), "n_a": r["n_a"], "n_b": r["n_b"],
           "frame_a": r["frame_a"], "frame_b": r["frame_b"], "esc": bool(r["esc_a"] or r["esc_b"]),
           "verdict": res.get("verdict"), "layer": res.get("layer"), "level": res.get("level"),
           "facts_same": res.get("facts_same"), "detail": (res.get("detail") or "")[:1500],
           "layers": res.get("layers") or {}, "seconds": round(time.time() - t, 1), "pass": passno,
           "regions": _region_summary(res)}
    row["blocker"] = blocker(row)
    return row


def _region_summary(res):
    """the region layer's per-region statuses and first-failure reasons (for the blocker histogram)"""
    regs = (((res.get("layer_info") or {}).get("region") or res.get("info") or {}).get("regions") or {})
    return {"n": len(regs), "proved": sum(1 for v in regs.values() if v[0] == "proved"),
            "fails": [v[1][:400] for v in regs.values() if v[0] != "proved"][:6]}


# blocker classes, first match wins (over the region layer's failing regions, else the last layer's reason)
BLOCKERS = [
    ("worker/remote", r"worker ended without a result|worker failed|angr: the laptop declined|error: "),
    ("timeout", r"timeout|hard timeout"),
    ("paths", r"more than \d+ paths"),
    ("x87 order", r"x87 instruction sequences|different x87|x87 depth|x87 .* not modelled|symbolic x87"),
    ("frame: esp delta", r"call#\d+ esp|exit#\d+ esp| esp\b.*differ|esp differs"),
    ("frame layout", r"frame slot|memory|frame -0x|reads the stack below"),
    ("register relation", r"not related|without the identity|registers the callee reads|/e[a-ds][xip]|flags"),
    ("pairing", r"pairing|bijection|hint|relocated field"),
    ("event sequence", r"event sequence|tail call vs return"),
    ("switch / indirect", r"indirect jump|switch|jumps to|symbolic jump"),
    ("segment / SEH", r"segment|fs:|Ijk_|jumpkind"),
    ("loop", r"loop"),
    ("unsupported", r"Unsupported|not supported|cannot step|unconstrained|claripy op|rounding mode"),
    ("conflict", r"conflict"),
]


def blocker(row):
    if row["verdict"] in ("EQUIVALENT", "MATCH", "PROVED-UNTESTED"):
        return None
    fails = (row.get("regions") or {}).get("fails") or []
    texts = fails + [row.get("detail") or ""]
    for name, pat in BLOCKERS:
        if any(re.search(pat, t) for t in texts):
            return name
    return "other"


def timed_out(row):
    return row["verdict"] == "UNKNOWN" and row.get("blocker") == "timeout"


def summary(rows):
    lines = []
    ok = lambda r: r["verdict"] in ("EQUIVALENT", "MATCH")
    c = collections.Counter(r["verdict"] for r in rows)
    lines.append("%d pairs: %s" % (len(rows), ", ".join("%s %d" % kv for kv in sorted(c.items()))))
    for cls in ("identical", "same-frame", "frame-size"):
        s = [r for r in rows if r["cls"] == cls]
        if s:
            lines.append("  %-11s %3d/%-3d proved (%.0f%%)" % (cls, sum(map(ok, s)), len(s), 100.0 * sum(map(ok, s)) /
                                                             len(s)))
    lines.append("differing pairs by size bucket (instructions, cl 14 side):")
    diff = [r for r in rows if r["cls"] != "identical"]
    for lo, hi in BUCKETS:
        b = bucket(lo)
        s = [r for r in diff if r["bucket"] == b]
        if not s:
            continue
        e = [r for r in s if r["esc"]]
        f = [r for r in s if r["cls"] == "frame-size"]
        lines.append("  %-9s %3d/%-3d proved (%3.0f%%); escaping %d/%d, frame-size %d/%d" % (
            b, sum(map(ok, s)), len(s), 100.0 * sum(map(ok, s)) / len(s), sum(map(ok, e)), len(e),
            sum(map(ok, f)), len(f)))
    lay = collections.Counter(r["layer"] for r in rows if ok(r) and r["verdict"] == "EQUIVALENT")
    lines.append("proved by layer: %s" % ", ".join("%s %d" % kv for kv in lay.most_common()))
    bl = collections.Counter(r["blocker"] for r in diff if not ok(r))
    lines.append("blockers (differing pairs not proved): %s" % ", ".join("%s %d" % kv for kv in bl.most_common()))
    bi = collections.Counter(r["blocker"] for r in rows if r["cls"] == "identical" and not ok(r))
    if bi:
        lines.append("identical pairs not proved: %s" % ", ".join("%s %d" % kv for kv in bi.most_common()))
    lines.append("time: %.0f s summed over pairs" % sum(r.get("seconds", 0) for r in rows))
    return "\n".join(lines)


def parse_tmo(s):
    return {k: float(v) for k, v in (x.split("=") for x in s.split(",") if x)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--corpus", default=os.path.join(ROOT, "work", "fnv_corpus"))
    ap.add_argument("--n", type=int, default=300)
    ap.add_argument("--ident", type=int, default=50)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--jobs", type=int, default=5)
    ap.add_argument("--remote", action="store_true")
    ap.add_argument("--remote-only", action="store_true")
    ap.add_argument("--frame-model", action="store_true")
    ap.add_argument("--pass1", default="rename=30,angr=60,region=300")
    ap.add_argument("--pass2", default="rename=30,angr=120,region=1200")
    ap.add_argument("--only", help="a file of keys (source|symbol), one per line: run just these")
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "equiv", "corpus", "baseline.jsonl"))
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--summary-only", action="store_true")
    ap.add_argument("--json-summary")
    a = ap.parse_args()
    if a.jobs > 6:
        sys.exit("--jobs at most 6 (the laptop's angr slots)")
    if a.summary_only:
        rows = list({r["key"]: r for r in (json.loads(l) for l in open(a.out) if l.strip())}.values())
        print(summary(rows))
        return 0
    rows = json.load(open(os.path.join(a.corpus, "pairs.json")))
    stems = {u["source"]: u["stem"] for u in json.load(open(os.path.join(a.corpus, "index.json")))}
    todo = sample(rows, a.n, a.ident, a.seed)
    if a.only:
        keys = {l.strip() for l in open(a.only) if l.strip()}
        todo = [r for r in rows if key(r) in keys]
    seen = {}
    if a.resume and os.path.exists(a.out):
        seen = {r["key"]: r for r in (json.loads(l) for l in open(a.out) if l.strip())}
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    lock = threading.Lock()
    final = dict(seen)

    def batch(items, tmo, passno):
        with open(a.out, "a") as out, ThreadPoolExecutor(a.jobs) as ex:
            futs = [ex.submit(run_one, r, a.corpus, stems, tmo, a.remote, a.remote_only, a.frame_model, passno)
                    for r in items]
            for k, f in enumerate(as_completed(futs), 1):
                row = f.result()
                with lock:
                    final[row["key"]] = row
                    out.write(json.dumps(row) + "\n")
                    out.flush()
                if k % 10 == 0 or k == len(futs):
                    print("pass %s: %d/%d" % (passno, k, len(futs)), flush=True)
    first = [r for r in todo if key(r) not in seen]
    print("%d pairs in the sample, %d to run" % (len(todo), len(first)), flush=True)
    batch(first, parse_tmo(a.pass1), 1)
    by = {key(r): r for r in todo}
    again = [by[k] for k, v in final.items() if k in by and v.get("pass") == 1 and timed_out(v)]
    if again and a.pass2:
        print("pass 2: %d pairs timed out" % len(again), flush=True)
        batch(again, parse_tmo(a.pass2), 2)
    res = [final[key(r)] for r in todo if key(r) in final]
    s = summary(res)
    print(s)
    if a.json_summary:
        json.dump({"summary": s, "n": len(res)}, open(a.json_summary, "w"), indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
