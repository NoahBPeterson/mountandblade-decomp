#!/usr/bin/env python3
"""Run prove.py's layers over many functions and summarise the verdicts (maintainers; needs the game image).

  uv run python tools/equiv/sweep.py [--same-count] [--va VA ...] [--vas-file F] [--jobs 4] [--layers rename,angr]
                                     [--timeout S] [--skip-proven] [--out work/equiv/rename_sweep.jsonl]
                                     [--resume] [--summary-only]

EQUIVALENT rows carry the proof bundle (with the build hash); tools/equiv/record_sweep.py records them.
Economics: --no-mutants proves without the self-test (its proofs are PROVED-UNTESTED, never recorded) and then re-runs
only the rows that proved WITH the self-test (the proving layer checking its own hints), so no row pays for mutants
before it has a proof; --two-pass T1 runs every row with T1 seconds per layer first and a second pass with --timeout
only for rows that timed out (a decided unknown is not retried). --remote: the angr solver on the laptop.

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


def candidates(same_count, vas=None, skip_proven=False):
    rows = json.load(open(os.path.join(ROOT, "docs", "insn_exact.json")))["rows"]
    proven = set()
    if skip_proven:
        try:
            proven = set(json.load(open(os.path.join(ROOT, "docs", "equiv_proofs.json")))["rows"])
        except (OSError, ValueError, KeyError):
            pass
    out = []
    for va, r in sorted(rows.items()):
        if r.get("verdict") != "DIFF" or not r.get("src") or va in proven:
            continue
        if same_count and r.get("insns") != r.get("ours"):
            continue
        if vas and int(va, 16) not in vas:
            continue
        out.append((int(va, 16), r["src"], r.get("symbol")))
    return out


def one(va, src, symbol, layers=("rename",), timeout=None, k_mutants=6, hints=None, passno=1):
    """k_mutants=0 (--no-mutants): a proof without the self-test is not EQUIVALENT but PROVED-UNTESTED (never
    recorded; selftest_rows re-runs it with mutants). hints: {layer: hints} (that re-run: the layer only checks)."""
    import io
    import contextlib
    import prove
    import rename_core
    buf = io.StringIO()
    t = time.time()
    rename_core.CONV_MISMATCH.clear()
    try:
        with contextlib.redirect_stdout(buf):
            res = prove.run(os.path.join(ROOT, src), va, symbol, log=lambda s: buf.write(s + "\n"),
                            layers=list(layers), timeout=timeout, k_mutants=k_mutants, hints=hints)
            if res["verdict"] == "EQUIVALENT" and not k_mutants:
                res["verdict"] = UNTESTED
            if res["verdict"] == "EQUIVALENT" and res.get("layer") == "rename":
                base = prove.run(os.path.join(ROOT, src), va, symbol, level=res["level"], frame_slots=False,
                                 log=lambda s: None)
                res["needs_slots"] = base["verdict"] != "EQUIVALENT"
    except SystemExit as e:
        res = {"verdict": "ERROR", "detail": str(e)[-300:]}
    except Exception as e:
        res = {"verdict": "ERROR", "detail": "%s: %s" % (type(e).__name__, str(e)[-300:])}
    keep = ("verdict", "level", "layer", "detail", "symbol", "facts_same", "facts_why", "insns", "info",
            "needs_slots", "mutants", "build_s", "check_s", "layers", "build", "bundle")
    out = {"va": "%08x" % va, "src": src}
    if passno != 1:
        out["pass"] = passno
    out.update({k: res[k] for k in keep if k in res})
    out["detail"] = (out.get("detail") or "")[:600]
    if rename_core.CONV_MISMATCH:
        out["conv_mismatch"] = sorted(set(rename_core.CONV_MISMATCH))[:10]
    out["seconds"] = round(time.time() - t, 3)
    return out


UNTESTED = "PROVED-UNTESTED"     # proved with --no-mutants: not EQUIVALENT until the self-test passes


def timed_out(r):
    """did the row end in a timeout of some layer rather than in a decided unknown (two-pass mode)?"""
    if r.get("verdict") != "UNKNOWN":
        return False
    return any(st == "unknown" and re.search(r"timeout", d or "") for st, d, *_ in (r.get("layers") or {}).values())


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


LAYER_REASONS = [
    ("timeout", r"timeout"),
    ("loop: input-dependent trip count", r"trip count depends on the inputs"),
    ("loop: calls/nested/too many paths", r"loop body has calls, nested loops"),
    ("loop: writes memory", r"loop body writes memory"),
    ("loop: other", r"not bounded|can still leave the loop|loop exit depends"),
    ("switch table / indirect jump", r"indirect jump|switch"),
    ("x87 sequences differ", r"x87 instruction sequences differ|x87 sequences differ"),
    ("x87 uninterpreted", r"uninterpreted"),
    ("event sequence differs", r"event sequence"),
    ("counterexample untrusted", r"counterexample in the model, not trusted"),
    ("pairing check (constant/symbols file)", r"pairing check failed"),
    ("unsupported IR op", r"UnsupportedIROp|cannot step"),
    ("too many paths", r"more than \d+ paths"),
    ("ddec unsupported", r"ddec: unsupported"),
    ("ddec not proved", r"ddec: not proved"),
    ("ddec image/docker", r"docker image|Docker not found"),
    ("solver unknown", r"solver returned unknown"),
    ("worker error", r"worker failed|error:"),
]


def layer_reason(d):
    for name, pat in LAYER_REASONS:
        if re.search(pat, d or ""):
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
    if eq:
        lines.append("EQUIVALENT by layer: %s" % ", ".join("%s %d" % kv for kv in sorted(
            collections.Counter(r.get("layer") for r in eq).items())))
    for name in ("rename", "angr", "ddec"):
        st = collections.Counter((r.get("layers") or {}).get(name, ["-"])[0] for r in rows
                                 if name in (r.get("layers") or {}))
        if st:
            lines.append("layer %s: %s" % (name, ", ".join("%s %d" % kv for kv in st.most_common())))
            un_l = [r["layers"][name][1] for r in rows if r.get("layers", {}).get(name, [""])[0] == "unknown"]
            if name != "rename" and un_l:
                rc = collections.Counter(layer_reason(d) for d in un_l)
                lines.append("  %s unknown by reason: %s" % (name, ", ".join("%s %d" % kv for kv in rc.most_common())))
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
    ap.add_argument("--layers", default="rename", help="comma list, cheapest first (rename,angr,ddec)")
    ap.add_argument("--timeout", type=float, help="seconds per layer run (prove.py's defaults otherwise)")
    ap.add_argument("--skip-proven", action="store_true", help="leave out rows already in docs/equiv_proofs.json")
    ap.add_argument("--vas-file", help="only the VAs listed in this file (one hex VA per line, or a sweep .jsonl)")
    ap.add_argument("--out", default=OUT)
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--remote", action="store_true", help="run the angr solver on the Windows laptop "
                    "(tools/equiv/remote.py; EQUIV_REMOTE=1 does the same), locally when it declines")
    ap.add_argument("--no-mutants", action="store_true", help="prove without the mutant self-test; rows that prove "
                    "are then re-run WITH mutants (the proving layer, checking its own hints) before they count as "
                    "EQUIVALENT (record_sweep.py records only those)")
    ap.add_argument("--two-pass", type=float, metavar="T1", help="a first pass with T1 seconds per layer, then a "
                    "second pass with --timeout (or prove.py's defaults) only for rows whose first pass ended in a "
                    "timeout (not a decided unknown)")
    ap.add_argument("--summary-only", action="store_true")
    a = ap.parse_args()
    if a.remote or os.environ.get("EQUIV_REMOTE") == "1":
        import remote                   # the angr solver on the laptop (tools/equiv/remote.py), local when declined
        remote.enable()
    if a.jobs > 6:
        sys.exit("--jobs at most 6 on this machine")
    if a.summary_only:
        rows = [json.loads(l) for l in open(a.out) if l.strip()]
        print(summary(rows))
        return 0
    vas = {int(v, 16) for v in a.va} if a.va else None
    if a.vas_file:
        vf = set()
        for ln in open(a.vas_file):
            ln = ln.strip()
            if ln:
                vf.add(int(json.loads(ln)["va"] if ln.startswith("{") else ln.split()[0], 16))
        vas = vf if vas is None else vas & vf
    layers = tuple(x.strip() for x in a.layers.split(",") if x.strip())
    cands = candidates(a.same_count, vas, a.skip_proven)
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
    km = 0 if a.no_mutants else 6
    t_first = a.two_pass if a.two_pass else a.timeout
    with open(a.out, "a" if a.resume else "w") as out, ProcessPoolExecutor(a.jobs) as ex:
        def batch(items, label, **kw):
            futs = [ex.submit(one, *c, **dict(kw, **extra)) for c, extra in items]
            done = []
            for k, f in enumerate(as_completed(futs), 1):
                r = f.result()
                done.append(r)
                out.write(json.dumps(r) + "\n")
                out.flush()
                if k % 50 == 0 or k == len(futs):
                    print("%s%d/%d" % (label, k, len(futs)), flush=True)
            return done
        rows = batch([(c, {}) for c in todo], "", layers=layers, timeout=t_first, k_mutants=km)
        final = {r["va"]: r for r in rows}
        by_va = {"%08x" % c[0]: c for c in todo}
        if a.two_pass:
            again = [by_va[r["va"]] for r in rows if timed_out(r)]
            print("two-pass: %d of %d rows timed out at %.0f s; second pass with %s" % (
                len(again), len(rows), a.two_pass, "%.0f s" % a.timeout if a.timeout else "prove.py's defaults"))
            for r in batch([(c, {}) for c in again], "pass 2: ", layers=layers, timeout=a.timeout, k_mutants=km,
                           passno=2):
                final[r["va"]] = r
        if a.no_mutants:
            unt = [r for r in final.values() if r["verdict"] == UNTESTED]
            print("no-mutants: %d rows proved; re-running them with the mutant self-test" % len(unt))
            items = [(by_va[r["va"]], {"layers": (r["layer"],), "hints": {r["layer"]: (r.get("bundle") or {}).get(
                "hints")}}) for r in unt]
            for r in batch(items, "self-test: ", timeout=a.timeout, k_mutants=6, passno="selftest"):
                final[r["va"]] = r
        rows = list(final.values())
    if a.two_pass or a.no_mutants:
        # the out file ends with one line per row, the last pass's (a resumed sweep keeps the earlier lines)
        keep = [json.loads(l) for l in open(a.out) if l.strip()] if a.resume else []
        fin = {r["va"]: r for r in keep}
        fin.update(final)
        with open(a.out, "w") as out:
            for r in fin.values():
                out.write(json.dumps(r) + "\n")
    print("compile/warm %.1f s, checks %.1f s wall" % (t1 - t0, time.time() - t1))
    if a.resume and os.path.exists(a.out):
        rows = [json.loads(l) for l in open(a.out) if l.strip()]
    print(summary(rows, time.time() - t1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
