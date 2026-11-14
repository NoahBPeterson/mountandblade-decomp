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
    row["x87"], row["reals"] = x87_class(a, b, r["symbol"])
    row["blocker"] = blocker(row)
    return row


_X87_CONST_LOADS = ("fld1", "fldz", "fldpi", "fldl2e", "fldl2t", "fldlg2", "fldln2")


def x87_shape(f):
    """(strict, norm, const-normalised) x87 signatures of one side: the strict one (angr_layer.x87_signature, the
    rule a proof needs), up to stack shuffling and operand order (x87_signature_norm), and that with every literal
    load (fld1/fldz/..., a memory operand naming __real@) written as one 'const' load, so a pair whose x87 work
    differs only in HOW literals are loaded (cl 14 loads more of them with fld1/fldz or keeps them on the stack;
    cl 13.10 reads more __real@) is told apart from one whose operations or their order differ"""
    sys.path.insert(0, EQ)
    import angr_layer as AL
    strict = AL.x87_signature(f.code, 0)
    norm = AL.x87_signature_norm(f.code, 0)
    out = []
    for i in AL.decode(f.code, 0):
        m = i.mnemonic
        if not m.startswith("f") or m in AL._X87_SHUFFLE:
            continue
        lit = m in _X87_CONST_LOADS or any(
            (i.address + o) in f.relocs and f.relocs[i.address + o][1].startswith("__real@")
            for o in (i.disp_offset,) if o)
        mem = re.search(r"(byte|word|dword|qword|tbyte|xword) ptr", i.op_str)
        if m in ("fld", "fst", "fstp") and not mem and not lit:
            continue
        fam = next((fm for pp, fm in AL._X87_FAMILY if m.startswith(pp)), None)
        if lit:
            out.append("load:const")
            if fam:
                out.append(fam)
            continue
        if fam:
            if mem:
                out.append("load:" + mem.group(1) + (":int" if m.startswith("fi") else ""))
            out.append(fam)
        elif m in ("fld", "fild", "fbld"):
            out.append("load:" + mem.group(1) + (":int" if m == "fild" else ""))
        elif m in ("fst", "fstp", "fist", "fistp", "fisttp"):
            out.append("store:" + mem.group(1) + (":int" if m.startswith("fi") else ""))
        else:
            out.append(m.rstrip("p") + (":" + mem.group(1) if mem else ""))
    nreal = sum(1 for t in f.relocs.values() if t[1].startswith("__real@"))
    return strict, norm, sorted(out), nreal


def _moves_only(na, nb):
    """the normalised signatures differ only in DATA MOVEMENT through the x87: float loads and stores (not integer
    ones) and fldz/fld1/... constant loads, no arithmetic. cl 14 copies floats with fld m/fstp m, stores 0.0 and
    1.0 with fldz/fld1 + fstp and stores one value to several places with fst where cl 13.10 moves the bits with
    integer movs; the arithmetic is the same"""
    ca, cb = collections.Counter(na), collections.Counter(nb)
    mv = lambda k: (k.startswith(("load:", "store:")) and not k.endswith(":int")) or k in _X87_CONST_LOADS
    return all(mv(k) for d in (ca - cb, cb - ca) for k in d)


def x87_class(a_obj, b_obj, sym):
    """the pair's x87 relation: none (no x87 code), same (strict signatures equal), order (the same operations up to
    shuffling, in another order or stack discipline), literal (they differ only in how literals are loaded), moves
    (only in data movement through the x87: _moves_only),
    ops (the operations themselves differ); plus the __real@ reference counts (ours, retail)"""
    sys.path.insert(0, EQ)
    import func as FN
    import objpair
    try:
        fa = FN.obj_func(a_obj, sym if sym in objpair.Coff(a_obj).by_name else sym.replace("T__m128@@", "U__m128@@"))
        fb = FN.obj_func(b_obj, sym)
    except SystemExit:
        return None, None
    (sa, na, ca, ra), (sb, nb, cb, rb) = x87_shape(fa), x87_shape(fb)
    if not sa and not sb:
        c = "none"
    elif sa == sb:
        c = "same"
    elif na == nb:
        c = "order"
    elif ca == cb:
        c = "literal"
    elif _moves_only(na, nb):
        c = "moves"
    else:
        c = "ops"
    return c, [ra, rb]


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
            if name == "x87 order":
                # split by the pair's static x87 relation (x87_class): order vs literal use vs other operations
                return {"order": "x87 order", "literal": "x87 literal use", "moves": "x87 float moves",
                        "ops": "x87 ops differ"}.get(
                    row.get("x87"), "x87 (region path)")
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
    xc = collections.Counter(r.get("x87") for r in diff)
    xn = collections.Counter(r.get("x87") for r in diff if ok(r))
    lines.append("x87 relation of the differing pairs (proved/all): %s" % ", ".join(
        "%s %d/%d" % (k, xn[k], v) for k, v in xc.most_common()))
    lr = [r["reals"] for r in diff if r.get("reals")]
    if lr:
        lines.append("__real@ references: ours (cl 13.10) %d, retail (cl 14) %d; pairs with fewer in cl 14: %d of %d" % (
            sum(a for a, _b in lr), sum(b for _a, b in lr), sum(1 for a, b in lr if b < a), len(lr)))
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
