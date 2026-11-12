#!/usr/bin/env python3
"""Run prove.py's rename layer over many functions and summarise the verdicts (maintainers; needs the game image).

  uv run python tools/equiv/sweep.py [--same-count] [--va VA ...] [--jobs 4] [--out work/equiv/rename_sweep.jsonl]
                                     [--resume] [--summary-only]

Ported in spirit from spore-decomp tools/equiv/sweep.py (commit aaaf5c79f). Candidates: the DIFF rows of
docs/insn_exact.json (source file and symbol from there), --same-count only those whose instruction count equals
the original's (the only ones the rename layer can accept: it pairs instructions one to one). One JSON line per
function goes to --out (rewritten unless --resume, which skips the VAs already in it). Each line has the verdict,
the layer's reason, and for EQUIVALENT rows whether the proof needed frame-slot renaming (the same pair checked
again with frame operands compared by offset, the spore rule). Nothing is recorded in manifests or docs.
Objects are compiled first, one per module (tools/difftest/slice.compile_obj: the cache, then cl.sh / the laptop
compile server), then the checks run in --jobs processes.
"""
import argparse, collections, json, os, re, sys, time
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
OUT = os.path.join(ROOT, "work", "equiv", "rename_sweep.jsonl")


def candidates(same_count, vas=None):
    rows = json.load(open(os.path.join(ROOT, "docs", "insn_exact.json")))["rows"]
    out = []
    for va, r in sorted(rows.items()):
        if r.get("verdict") != "DIFF" or not r.get("src"):
            continue
        if same_count and r.get("insns") != r.get("ours"):
            continue
        if vas and int(va, 16) not in vas:
            continue
        out.append((int(va, 16), r["src"], r.get("symbol")))
    return out


def one(va, src, symbol):
    import io
    import contextlib
    import prove
    import rename_core
    buf = io.StringIO()
    t = time.time()
    rename_core.CONV_MISMATCH.clear()
    try:
        with contextlib.redirect_stdout(buf):
            res = prove.run(os.path.join(ROOT, src), va, symbol, log=lambda s: buf.write(s + "\n"))
            if res["verdict"] == "EQUIVALENT":
                base = prove.run(os.path.join(ROOT, src), va, symbol, level=res["level"], frame_slots=False,
                                 log=lambda s: None)
                res["needs_slots"] = base["verdict"] != "EQUIVALENT"
    except SystemExit as e:
        res = {"verdict": "ERROR", "detail": str(e)[-300:]}
    except Exception as e:
        res = {"verdict": "ERROR", "detail": "%s: %s" % (type(e).__name__, str(e)[-300:])}
    keep = ("verdict", "level", "layer", "detail", "symbol", "facts_same", "facts_why", "insns", "info",
            "needs_slots", "mutants", "build_s", "check_s")
    out = {"va": "%08x" % va, "src": src}
    out.update({k: res[k] for k in keep if k in res})
    out["detail"] = (out.get("detail") or "")[:400]
    if rename_core.CONV_MISMATCH:
        out["conv_mismatch"] = sorted(set(rename_core.CONV_MISMATCH))[:10]
    out["seconds"] = round(time.time() - t, 3)
    return out


REASONS = [
    ("instruction count", r"^instruction count"),
    ("different CFG", r"different number of basic blocks|CFG differs|block \d+ \(A \+0x[0-9a-f]+\) has \d+ instructions"),
    ("calls differ", r"calls in A"),
    ("memory order / count", r"memory operations in A"),
    ("def-use mismatch", r"reads a different producer|use slots differ|unmapped definition"),
    ("frame-size flags/eax read", r"frame-size instruction"),
    ("instruction shapes", r"no instruction of B's block matches"),
    ("frame model", r"stack depth|ret at stack depth|below esp|writes esp|frame model|unknown argument bytes|"
                    r"tail jump at stack depth|__chkstk|reaches the return address|unreachable code uses|pop into esp"),
    ("switch table", r"indirect jump"),
    ("search budget", r"search budget|too long"),
    ("self-test", r"self-test failed"),
    ("disassembly", r"disassembly stops|middle of an instruction"),
]


def reason_class(r):
    d = r.get("detail") or ""
    for name, pat in REASONS:
        if re.search(pat, d):
            # a block-level search failure names the first mismatching pair: split by its cause
            if name == "instruction shapes" or (name == "def-use mismatch" and d.startswith("block")):
                pass
            return name
    return "other"


def summary(rows, wall=None):
    c = collections.Counter(r["verdict"] for r in rows)
    lines = ["%d functions: %s" % (len(rows), ", ".join("%s %d" % kv for kv in sorted(c.items())))]
    eq = [r for r in rows if r["verdict"] == "EQUIVALENT"]
    if eq:
        lv = collections.Counter(r["level"] for r in eq)
        lines.append("EQUIVALENT by level: %s; needed frame-slot renaming: %d; delta mode: %d; moved instructions: %d"
                     % (", ".join("%s %d" % kv for kv in sorted(lv.items())), sum(1 for r in eq if r.get("needs_slots")),
                        sum(1 for r in eq if (r.get("info") or {}).get("mode") == "delta"),
                        sum(1 for r in eq if (r.get("info") or {}).get("moved"))))
    un = [r for r in rows if r["verdict"] == "UNKNOWN"]
    if un:
        rc = collections.Counter(reason_class(r) for r in un)
        lines.append("UNKNOWN by reason: " + ", ".join("%s %d" % kv for kv in rc.most_common()))
    er = [r for r in rows if r["verdict"] == "ERROR"]
    if er:
        lines.append("ERROR: " + "; ".join("%s %s" % (r["va"], r["detail"][:80]) for r in er[:5]))
    cm = [r for r in rows if r.get("conv_mismatch")]
    if cm:
        lines.append("callconv disagreed with the original's ret N in %d rows: %s" % (
            len(cm), "; ".join("%s %s" % (r["va"], r["conv_mismatch"][0]) for r in cm[:5])))
    t = sum(r.get("seconds", 0) for r in rows)
    tc = [r["check_s"] for r in rows if "check_s" in r]
    tb = [r["build_s"] for r in rows if "build_s" in r]
    lines.append("time %.1f s total per row (incl. compiles in workers); object+image load %.1f s; rename check "
                 "incl. mutants %.2f s total, max %.2f s, over %d rows%s" % (
                     t, sum(tb), sum(tc), max(tc or [0]), len(tc), "; wall %.1f s" % wall if wall else ""))
    return "\n".join(lines)


def warm(cands, jobs):
    """Compile each module once (cached objects come back immediately)."""
    import func as FN
    srcs = sorted({os.path.join(ROOT, s) for _va, s, _sym in cands})

    def c(s):
        try:
            FN.compile_src(s)
            return None
        except SystemExit as e:
            return "%s: %s" % (os.path.relpath(s, ROOT), str(e)[:200])
    with ThreadPoolExecutor(max(1, jobs)) as ex:
        for err in ex.map(c, srcs):
            if err:
                print("compile:", err, file=sys.stderr)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--same-count", action="store_true", help="only rows with the original's instruction count")
    ap.add_argument("--va", nargs="*", help="only these VAs (hex)")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--out", default=OUT)
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--summary-only", action="store_true")
    a = ap.parse_args()
    if a.jobs > 6:
        sys.exit("--jobs at most 6 on this machine")
    if a.summary_only:
        rows = [json.loads(l) for l in open(a.out) if l.strip()]
        print(summary(rows))
        return 0
    vas = {int(v, 16) for v in a.va} if a.va else None
    cands = candidates(a.same_count, vas)
    seen = set()
    if a.resume and os.path.exists(a.out):
        seen = {json.loads(l)["va"] for l in open(a.out) if l.strip()}
    todo = [c for c in cands if "%08x" % c[0] not in seen]
    print("%d functions to check (%d already in %s)" % (len(todo), len(seen), os.path.relpath(a.out, ROOT)))
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    t0 = time.time()
    warm(todo, a.jobs)
    t1 = time.time()
    rows = []
    with open(a.out, "a" if a.resume else "w") as out, ProcessPoolExecutor(a.jobs) as ex:
        futs = [ex.submit(one, *c) for c in todo]
        for k, f in enumerate(as_completed(futs), 1):
            r = f.result()
            rows.append(r)
            out.write(json.dumps(r) + "\n")
            out.flush()
            if k % 50 == 0 or k == len(futs):
                print("%d/%d" % (k, len(futs)), flush=True)
    print("compile/warm %.1f s, checks %.1f s wall" % (t1 - t0, time.time() - t1))
    if a.resume and os.path.exists(a.out):
        rows = [json.loads(l) for l in open(a.out) if l.strip()]
    print(summary(rows, time.time() - t1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
