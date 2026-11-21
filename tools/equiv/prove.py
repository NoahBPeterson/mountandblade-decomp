#!/usr/bin/env python3
"""prove: is our build of a function behaviourally equivalent to the original, by a static proof?

  uv run python tools/equiv/prove.py <src.cpp> <va> [--symbol SYM] [--flags "..."] [--level L1|L2] [--json]
                                     [--layers rename,angr,ddec] [--timeout S] [--mutants N] [--no-frame-slots]
                                     [--record] [--recheck] [--frame-model]
  uv run python tools/equiv/prove.py --recheck-all [--record] [--frame-model]   # every row of docs/equiv_proofs.json

Layer ordering, acceptance, --record and --recheck ported from spore-decomp tools/contrib/prove.py (commit
82de4ab6c; the rename-only first version came from aaaf5c79f). Layers run cheapest first (tools/equiv/LAYERS.md):
  rename   register renaming + frame-slot renaming + independent reordering (rename_core.py; milliseconds)
  angr     symbolic execution with Z3 (angr_layer.py, in work/angr-venv; seconds to minutes)
  ddec     STOKE's DDEC in Docker (ddec_layer.py, ddec/Dockerfile; loops; skipped when Docker or the image is
           missing; no x87)
Verdicts:
  MATCH       byte-exact (nothing to prove)
  EQUIVALENT  some layer proves it at L2 (every register at every call and ret), or at L1 (the calling contract)
              with the same kill set and entry registers as the original (tools/equiv/facts.py), AND that layer
              rejects every mutant of our build (tools/equiv/mutate.py); no earlier layer refuted it
  EQUIVALENT-O  (--frame-model / EQUIV_FRAME_MODEL=1) no layer proves it unconditionally, but the region layer does
              under the per-object frame model (LAYERS.md "Status"), resting on its assumptions O, T and I, with the
              same self-test (every mutant rejected under the frame model). Recorded as tier PROVEN-O, never as a
              plain proof: the unconditional layers always run first, so a proof that needs no frame model stays
              EQUIVALENT
  REFUTED     a layer found a counterexample it trusts (the rename layer never refutes)
  UNKNOWN     anything else, with each layer's reason
--record (on EQUIVALENT or EQUIVALENT-O) writes the row of docs/equiv_proofs.json (va -> src, symbol, layer, level,
detail, build, flags, bundle; an EQUIVALENT-O row also "tier": "PROVEN-O" and "assumes": ["O", "T", "I"]) and
the proof bundle docs/equiv_proofs/<va>.json (layer, level, build hash, facts, the layer's hints; no game bytes), or work/equiv/proofs/<va>.json when it is over spore's size cap (2 KB + 1 KB per KB of code).
--recheck re-verifies a recorded proof: when our build hash still matches, the recorded layer runs with its hints
(it must not search, only check); when it does not, the function is proved again from scratch with every layer.
With --record a recheck also writes the result back (a new build hash, or the row removed when it no longer proves).
"""
import argparse, datetime, hashlib, importlib, json, os, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import func as FN    # noqa: E402
import facts         # noqa: E402
import mutate        # noqa: E402

LAYERS = ("rename", "angr", "ddec")
MODULE = {"rename": "rename", "angr": "angr_layer", "ddec": "ddec_layer", "region": "regions"}
TIMEOUT = {"rename": 30.0, "angr": 120.0, "ddec": 600.0, "region": 1800.0}
# "region" (regions.py: the angr layer's cut-point decomposition, switch tables and big functions) is opt-in:
# --layers rename,angr,region; a recorded region proof is rechecked with it
PROVE_VERSION = "2"
# the self-test's fast mutant runs (a layer's prove_mutant; LAYERS.md "Self-test"); EQUIV_FULL_MUTANTS=1 turns
# them off (every mutant gets the full run, as before)
FAST_MUTANTS = os.environ.get("EQUIV_FULL_MUTANTS") != "1"
# the verdicts that are proofs, and the tier each is recorded as (docs/equiv_proofs.json "tier"; a row without one is
# PROVEN; tools/difftest/verdicts.py turns PROVEN-O into its own category, proven_o)
PROVED = ("EQUIVALENT", "EQUIVALENT-O")
TIER = {"EQUIVALENT": "PROVEN", "EQUIVALENT-O": "PROVEN-O"}
# what a frame-model proof rests on beyond an unconditional one (LAYERS.md "The per-object frame model", stage 2)
FRAME_ASSUMES = ["O", "T", "I"]


def frame_model_env():
    return os.environ.get("EQUIV_FRAME_MODEL") == "1"
PROOFS = os.path.join(ROOT, "docs", "equiv_proofs.json")
BUNDLES = os.path.join(ROOT, "docs", "equiv_proofs")
BIG_BUNDLES = os.path.join(ROOT, "work", "equiv", "proofs")


def asm_transcribed(src, va):
    """An __asm/naked/_emit transcription never counts (docs/matching.md)."""
    try:
        import asm_audit
        t = asm_audit.transcribed(os.path.join(ROOT, src)) if hasattr(asm_audit, "transcribed") else {}
        return t.get("%08x" % va)
    except Exception:
        return None


def load_layer(name):
    try:
        return importlib.import_module(MODULE.get(name, name))
    except ImportError as e:
        return str(e)


def tool_versions(layer=None):
    v = {"prove": PROVE_VERSION}
    if layer == "angr":
        v["angr"] = "10.0.6"
    if layer == "ddec":
        from ddec_layer import STOKE_VERSION
        v["stoke"] = STOKE_VERSION
    if layer == "region":
        v["angr"], v["region"] = "10.0.6", "1"
    if layer in ("angr", "region"):
        v["angr_lock"] = angr_lock()
    return v


def angr_lock():
    """the first 12 hex digits of the sha256 of tools/equiv/angr-requirements.txt (the pinned angr venv)"""
    try:
        return hashlib.sha256(open(os.path.join(HERE, "angr-requirements.txt"), "rb").read()).hexdigest()[:12]
    except OSError:
        return "none"


def retail_hash(retail):
    return hashlib.sha256(retail.code).hexdigest()


def layer_prove(name, mod, retail, ours, lv, hints, timeout, frame_slots, frame_model=False, judge=None):
    """frame_model: the region layer's per-object frame model (explicit, so EQUIV_FRAME_MODEL never reaches a layer
    behind prove.py's back); the other layers have none. judge: an external judge's pairing hints (--hints DIR,
    tools/equiv/judge_hints.py): candidates the region layer checks where it uses them; the other layers ignore them"""
    if name == "rename":
        return mod.prove(retail, ours, lv, hints, timeout, frame_slots=frame_slots)
    if name == "region":
        return mod.prove(retail, ours, lv, hints, timeout, frame_model=bool(frame_model), judge=judge)
    return mod.prove(retail, ours, lv, hints, timeout)


def layer_mutant(name, fast, retail, m, lv, hints, timeout, frame_model=False, judge=None):
    if name == "region":
        return fast(retail, m, lv, hints, timeout, frame_model=bool(frame_model), judge=judge)
    return fast(retail, m, lv, hints, timeout)


def run(src, va, symbol=None, flags=None, level=None, k_mutants=6, frame_slots=True, log=print, timeout=None,
        layers=LAYERS, hints=None, frame_model=None, judge=None):
    """timeout: seconds per layer run (None: TIMEOUT per layer). hints: {layer: hints} (a recheck). frame_model: see
    run_funcs. judge: a directory (or file) of external pairing hints (--hints; tools/equiv/judge_hints.py), or the
    hints already loaded"""
    t0 = time.time()
    retail = FN.retail_func(va)
    ours = FN.our_func(src, va, symbol, flags)
    asm = asm_transcribed(ours.meta["src"], va)
    if asm:
        return {"verdict": "ERROR", "detail": "assembly transcription (%s): write it as C++ first" % asm}
    if isinstance(judge, str):
        import judge_hints
        judge = judge_hints.load(judge, va)
        log("judge hints: %s" % ("none for 0x%08x" % va if judge is None else "%s row, %d register, %d slot, %d loop-head, "
                                 "%d global pairs" % (judge["verdict"], len(judge["regs"]), len(judge["slots"]),
                                                      len(judge["at"]), len(judge["globals"]))))
    return run_funcs(retail, ours, level, k_mutants, frame_slots, log, timeout, layers, hints,
                     t_build=time.time() - t0, frame_model=frame_model, judge=judge)


def run_funcs(retail, ours, level=None, k_mutants=6, frame_slots=True, log=print, timeout=None, layers=LAYERS,
              hints=None, t_build=0.0, match_shortcut=True, frame_model=None, judge=None):
    """run() on two loaded sides (the M&B pair, or an object pair: tools/equiv/objpair.py). match_shortcut=False
    runs the layers on a byte-exact pair too (the corpus' positive controls).
    frame_model (the region layer's per-object frame model, LAYERS.md "Status"): None = EQUIV_FRAME_MODEL=1;
    False = never; True = every layer unconditionally first, and only when none proves it the region layer again
    under the frame model (a proof then is EQUIVALENT-O: it rests on assumptions O, T and I); "only" = the region
    layer only under the frame model (the hinted recheck of a recorded EQUIVALENT-O proof)."""
    if frame_model is None:
        frame_model = frame_model_env()
    va = retail.va
    if match_shortcut and FN.is_byte_exact(retail, ours):
        return {"verdict": "MATCH", "detail": "byte-exact: nothing to prove", "symbol": ours.meta["symbol"],
                "build": FN.build_hash(ours)}
    FN.set_consts(ours)
    same, why, fa = facts.same(retail, ours)
    lv = level or ("L1" if same else "L2")
    log("facts: kill %s, entry %s; %s -> proving at %s" % (fa["kill"], fa["entry"],
                                                           "same as ours" if same else why, lv))
    base = {"symbol": ours.meta["symbol"], "level": lv, "facts_same": same, "facts_why": why,
            "insns": [len(list(FN.MD.disasm(retail.code, retail.va))), len(list(FN.MD.disasm(ours.code, 0)))],
            "build_s": round(t_build, 3), "build": FN.build_hash(ours), "layers": {}}
    t1 = time.time()
    reasons = []

    def attempt(name, fm):
        """one layer (fm: the region layer under the frame model) -> a final result dict, or None (not proved)"""
        mod = load_layer(name)
        key = name + ("-O" if fm else "")
        if isinstance(mod, str):
            log("  %-10s skipped (%s)" % (key, mod))
            base["layers"][key] = ["skipped", mod[:200]]
            return None
        tmo = (timeout.get(name) if isinstance(timeout, dict) else timeout) or TIMEOUT.get(name, 120.0)
        h = (hints or {}).get(name) if hints else None
        r = layer_prove(name, mod, retail, ours, lv, h, tmo, frame_slots, fm, judge)
        log("  %-10s %-8s %6.2fs  %s" % (key, r["status"], r["seconds"], r["detail"][:200]))
        jl = (r.get("info") or {}).get("judge")
        if jl:
            # the hint log: every candidate the layer proved and used, rejected at a stop, or dropped unchecked
            for kind in ("used", "rejected", "dropped"):
                for line in jl.get(kind) or ():
                    log("  %-10s judge hint %s: %s" % (key, kind, line[:200]))
        base["layers"][key] = [r["status"], r["detail"][:300], r["seconds"]]
        if r.get("info"):
            base.setdefault("layer_info", {})[key] = r["info"]
        base["check_s"] = round(time.time() - t1, 3)
        if r["status"] == "refuted":
            return dict(base, verdict="REFUTED", layer=name, detail=r["detail"])
        if r["status"] != "proved":
            reasons.append("%s: %s" % (key, r["detail"]))
            return None
        bad, tried = [], []
        tm = time.time()
        fast = getattr(mod, "prove_mutant", None) if FAST_MUTANTS else None
        for desc, m in (mutate.mutants(ours, k_mutants, seed=va, covered=r.get("covered")) if k_mutants else ()):
            how = "full"
            if fast is not None:
                # LAYERS.md "Self-test": a short run that stops at the first failure; only a DECIDED failure
                # rejects the mutant, anything undecided (a timeout, too many paths, ...) gets the full run. A
                # frame-model proof's mutants run under the frame model too
                mr = layer_mutant(name, fast, retail, m, lv, r.get("hints") or h, tmo, fm, judge)
                how = "fast"
                if mr["status"] != "proved" and not mr.get("decided"):
                    mr = layer_prove(name, mod, retail, m, lv, None, tmo, frame_slots, fm, judge)
                    how = "fast-undecided+full"
            else:
                mr = layer_prove(name, mod, retail, m, lv, None, tmo, frame_slots, fm, judge)
            tried.append([desc, mr["status"], how, mr.get("seconds")])
            if mr["status"] == "proved":
                bad.append(desc)
        base["check_s"] = round(time.time() - t1, 3)
        base["mutants_s"] = round(time.time() - tm, 2)
        if tried:
            log("  %-10s self-test: %d mutants in %.1fs (%s)" % (key, len(tried), time.time() - tm, ", ".join(
                "%s %s %.1fs" % (t[2], t[1], t[3] or 0) for t in tried)))
        if bad:
            log("  %-10s self-test FAILED: proved mutant(s) %s; its proof is not accepted" % (key, "; ".join(bad)))
            reasons.append("%s: self-test failed: proved mutant(s) %s" % (key, "; ".join(bad)))
            base["layers"][key][0] = "selftest-failed"
            return None
        bundle = {"va": "%08x" % va, "level": lv, "retail_fp": retail_hash(retail), "build": FN.build_hash(ours),
                  "src": ours.meta["src"], "symbol": ours.meta["symbol"], "flags": " ".join(ours.meta["flags"]),
                  "facts": {"kill": fa["kill"], "entry": fa["entry"]}, "layer": name,
                  "hints": r.get("hints") or {}, "tools": tool_versions(name)}
        verdict = "EQUIVALENT"
        if fm:
            verdict = "EQUIVALENT-O"
            bundle.update(frame_model=True, assumes=list(FRAME_ASSUMES), tier=TIER[verdict])
        return dict(base, verdict=verdict, layer=name, detail=r["detail"], info=r.get("info"), mutants=tried,
                    bundle=bundle, **({"assumes": list(FRAME_ASSUMES), "tier": TIER[verdict]} if fm else {}))

    if frame_model == "only":
        plan = [(n, n == "region") for n in layers]
    else:
        plan = [(n, False) for n in layers]
        if frame_model and "region" in layers:
            # unconditional first: a proof that needs no frame model is never downgraded to EQUIVALENT-O
            plan.append(("region", True))
    for name, fm in plan:
        out = attempt(name, fm)
        if out is not None:
            return out
    return dict(base, verdict="UNKNOWN", layer=(layers[-1] if layers else None),
                detail=" | ".join(reasons) or "no layer ran")


# ---------------------------------------------------------------------------------------------- the record
def bundle_limit(size):
    return 2048 + 1024 * ((size + 1023) // 1024)


def load_proofs():
    try:
        return json.load(open(PROOFS))
    except (OSError, ValueError):
        return {"what": "", "rows": {}}


def save_proofs(doc):
    doc["what"] = ("functions proven equivalent by tools/equiv/prove.py (static: the rename layer, angr symbolic "
                   "execution or STOKE DDEC; L1 = calling contract, L2 = every register; see tools/equiv/LAYERS.md). "
                   "Each row has our build hash (func.build_hash) and its proof bundle (docs/equiv_proofs/<va>.json, "
                   "or work/equiv/proofs/ when large); prove.py --recheck re-verifies it. "
                   "tools/difftest/verdicts.py --apply-proofs applies it to docs/equiv_verdicts.json.")
    doc["when"] = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    try:
        doc["commit"] = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short=8", "HEAD"], capture_output=True,
                                       text=True).stdout.strip()
    except OSError:
        pass
    doc["rows"] = dict(sorted(doc.get("rows", {}).items()))
    tmp = PROOFS + ".tmp"
    with open(tmp, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    os.replace(tmp, PROOFS)


def proof_state(va, row):
    """Is the recorded proof of the build in the tree now? -> ("current" | "stale" | "unknown", note). The row's "build"
    (func.build_hash: our function's code with relocated fields zeroed + each relocation's offset and symbol, written by
    record()) is compared with the hash of the function as it compiles today (the same compile cache prove.py uses).
    "unknown": the row has no build hash or the source no longer compiles/has the symbol; callers keep such a proof."""
    if not row.get("build"):
        return "unknown", "no build hash recorded"
    try:
        cur = FN.build_hash(FN.our_func(os.path.join(ROOT, row["src"]), int(va, 16), row.get("symbol")))
    except (SystemExit, Exception) as e:       # our_func exits on a compile failure
        return "unknown", "cannot build ours: %s" % str(e)[:120]
    if cur == row["build"]:
        return "current", ""
    return "stale", "proof of %s is of an older build of %s (build hash differs)" % (row.get("proved", "?"), row["src"])


def current_proofs(rows=None):
    """({va: row} of the proofs that still hold, {va: note} of the stale ones, {va: note} of the unverifiable ones kept).
    Everything that turns docs/equiv_proofs.json into a verdict goes through here."""
    if rows is None:
        rows = load_proofs().get("rows", {})
    ok, stale, unknown = {}, {}, {}
    for va, row in rows.items():
        st, note = proof_state(va, row)
        if st == "stale":
            stale[va] = note
            continue
        ok[va] = row
        if st == "unknown":
            unknown[va] = note
    return ok, stale, unknown


def bundle_path(va, size_ok=True):
    return os.path.join(BUNDLES if size_ok else BIG_BUNDLES, "%08x.json" % va)


def record(va, res, src, doc=None, quiet=False):
    """Write the row of docs/equiv_proofs.json and the bundle. doc: an already loaded proofs document (saved by the
    caller) or None (load and save here). A frame-model proof (its bundle says frame_model) gets tier PROVEN-O and
    its assumptions in the row; an unconditional row has no tier field (PROVEN)."""
    b = res["bundle"]
    if res.get("verdict") == "EQUIVALENT-O" and not b.get("frame_model"):
        raise SystemExit("prove.record: an EQUIVALENT-O result without a frame-model bundle")
    data = json.dumps(b, separators=(",", ":"), sort_keys=True)
    try:
        size = FN.image().func_extent(va)[1] - va
    except Exception:
        size = 0
    small = len(data) <= bundle_limit(size)
    path = bundle_path(va, small)
    other = bundle_path(va, not small)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(data + "\n")
    if os.path.exists(other):
        os.remove(other)
    own = doc is None
    lock = None
    if own:
        # parallel --record runs (a sweep of several prove.py processes) each load, add their row and save: hold an
        # exclusive lock across the load-modify-save so no run drops another's row
        import fcntl
        lock = open(os.path.join(ROOT, "work", "equiv_proofs.lock"), "w")
        fcntl.flock(lock, fcntl.LOCK_EX)
    doc = doc if doc is not None else load_proofs()
    doc.setdefault("rows", {})["%08x" % va] = {
        "src": os.path.relpath(os.path.join(ROOT, src), ROOT).replace(os.sep, "/"),
        "symbol": b["symbol"], "layer": b["layer"], "level": b["level"], "detail": (res.get("detail") or "")[:300],
        "build": b["build"], "flags": b["flags"], "bundle": os.path.relpath(path, ROOT).replace(os.sep, "/"),
        "proved": datetime.date.today().isoformat()}
    if b.get("frame_model"):
        doc["rows"]["%08x" % va].update(tier=TIER["EQUIVALENT-O"], assumes=list(b.get("assumes") or FRAME_ASSUMES))
    if own:
        save_proofs(doc)
        lock.close()
    if not quiet:
        print("recorded %08x (%s %s%s) in %s; bundle %s (%d bytes)" % (va, b["layer"], b["level"],
                                                                   ", PROVEN-O" if b.get("frame_model") else "",
                                                                   os.path.relpath(PROOFS, ROOT),
                                                                   os.path.relpath(path, ROOT), len(data)))


def find_bundle(va, row=None):
    for p in ([os.path.join(ROOT, row["bundle"])] if row and row.get("bundle") else []) + \
             [bundle_path(va, True), bundle_path(va, False)]:
        if os.path.exists(p):
            return json.load(open(p))
    return None


def recheck(va, row, log=print, k_mutants=6, timeout=None, frame_model=None):
    """-> (result, how). A recorded proof is re-verified: build hash equal -> the recorded layer with its hints
    (a PROVEN-O proof under the frame model, a plain one without); else proved again with every layer, the frame
    model allowed (after every unconditional layer) when the row is PROVEN-O or frame_model (None:
    EQUIV_FRAME_MODEL=1) allows it."""
    if frame_model is None:
        frame_model = frame_model_env()
    b = find_bundle(va, row)
    src = (b or {}).get("src") or row["src"]
    sym = (b or {}).get("symbol") or row.get("symbol")
    level = (b or {}).get("level") or row.get("level")
    cur = FN.our_func(os.path.join(ROOT, src), va, sym)
    if b and b.get("build") == FN.build_hash(cur):
        layer = b["layer"]
        res = run(os.path.join(ROOT, src), va, sym, level=level, layers=[layer], hints={layer: b.get("hints")},
                  k_mutants=k_mutants, log=log, timeout=timeout,
                  frame_model="only" if b.get("frame_model") else False)
        return res, "hinted recheck (%s%s)" % (layer, ", frame model" if b.get("frame_model") else "")
    why = "no bundle" if not b else "our build changed since the proof (build hash differs)"
    log("  %s: proving again" % why)
    extra = (b["layer"],) if b and b.get("layer") in MODULE and b.get("layer") not in LAYERS else ()
    was_o = bool((b or {}).get("frame_model") or row.get("tier") == TIER["EQUIVALENT-O"])
    fm = bool(frame_model or was_o)
    if was_o and "region" not in LAYERS + extra:
        extra += ("region",)        # (a plain row gets the frame model only if its layers include region already)
    res = run(os.path.join(ROOT, src), va, sym, k_mutants=k_mutants, log=log, timeout=timeout,
              layers=LAYERS + extra, frame_model=fm)
    return res, "re-proved (%s)" % why


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?")
    ap.add_argument("va", nargs="?")
    ap.add_argument("--symbol")
    ap.add_argument("--flags")
    ap.add_argument("--level", choices=("L1", "L2"))
    ap.add_argument("--layers", default=",".join(LAYERS))
    ap.add_argument("--timeout", type=float, help="seconds per layer run (default: rename 30, angr 120, ddec 600)")
    ap.add_argument("--layer-timeouts", help="per layer, e.g. angr=60,region=300 (the others: --timeout or the "
                    "defaults)")
    ap.add_argument("--mutants", type=int, default=6)
    ap.add_argument("--no-frame-slots", action="store_true", help="frame operands by offset only (the spore rule)")
    ap.add_argument("--frame-model", action="store_true", help="when no layer proves it unconditionally, try the "
                    "region layer under the per-object frame model (EQUIV_FRAME_MODEL=1 does the same): a proof is "
                    "then EQUIVALENT-O, recorded as tier PROVEN-O (assumptions O, T, I; LAYERS.md \"Status\")")
    ap.add_argument("--record", action="store_true", help="on EQUIVALENT, write docs/equiv_proofs.json + bundle")
    ap.add_argument("--recheck", action="store_true", help="re-verify the recorded proof of va")
    ap.add_argument("--recheck-all", action="store_true", help="re-verify every row of docs/equiv_proofs.json")
    ap.add_argument("--remote", action="store_true", help="run the angr solver on the Windows laptop "
                    "(tools/equiv/remote.py; EQUIV_REMOTE=1 does the same), locally when it declines")
    ap.add_argument("--hints", metavar="DIR", help="an external judge's pairing hints (<8-hex va>.json per function, "
                    "tools/equiv/judge_hints.py): candidate register / frame-slot / global pairs the region layer "
                    "tries where its own checks prove them; never trusted")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--objpair", nargs=3, metavar=("OURS_OBJ", "RETAIL_OBJ", "SYMBOL"),
                    help="object-vs-object mode (tools/equiv/objpair.py): ours and the 'original' both from COFF "
                    "objects, relocations paired by symbol name; never recorded")
    ap.add_argument("--ours-symbol", help="--objpair: ours' decorated name when it differs from SYMBOL's canon")
    ap.add_argument("--no-match-shortcut", action="store_true", help="run the layers on a byte-exact pair too")
    a = ap.parse_args()
    if a.mutants <= 0 and a.record:
        ap.error("--record needs the mutant self-test (--mutants > 0)")
    fm = a.frame_model or frame_model_env()
    if a.remote or os.environ.get("EQUIV_REMOTE", "1") == "1":
        import remote                   # the angr solver on the laptop (tools/equiv/remote.py), local when declined
        remote.enable()
    if a.layer_timeouts:
        lt = {k: float(v) for k, v in (x.split("=") for x in a.layer_timeouts.split(",") if x)}
        a.timeout = {n: lt.get(n, a.timeout) for n in set(TIMEOUT) | set(lt)}
    if a.objpair:
        if a.record or a.recheck or a.recheck_all:
            ap.error("--objpair proofs are never recorded or rechecked")
        import objpair
        retail, ours = objpair.setup(a.objpair[0], a.objpair[1], a.objpair[2], a.ours_symbol)
        t = time.time()
        layers = [x.replace("_layer", "") for x in a.layers.split(",") if x]
        res = run_funcs(retail, ours, a.level, a.mutants, frame_slots=not a.no_frame_slots, timeout=a.timeout,
                        layers=layers, match_shortcut=not a.no_match_shortcut, frame_model=fm)
        print("%s  %s  %s  (%.2fs)" % (res["verdict"], a.objpair[2],
                                       "%s %s" % (res["level"], res["layer"]) if res["verdict"] in PROVED
                                       else "", time.time() - t))
        if res.get("detail"):
            print("  " + res["detail"][:600])
        if a.json:
            print(json.dumps(res, indent=1, default=str))
        return 0 if res["verdict"] in PROVED + ("MATCH",) else 1
    if a.recheck_all or a.recheck:
        doc = load_proofs()
        rows = doc.get("rows", {})
        if a.recheck:
            if not a.va and a.src:
                a.va = a.src
            k = "%08x" % int(a.va, 16)
            if k not in rows:
                raise SystemExit("0x%s has no row in docs/equiv_proofs.json" % k)
            todo = [k]
        else:
            todo = sorted(rows)
        bad = 0
        for k in todo:
            t = time.time()
            res, how = recheck(int(k, 16), rows[k], log=(print if a.recheck else (lambda s: None)),
                               k_mutants=a.mutants, timeout=a.timeout, frame_model=fm)
            ok = res["verdict"] in PROVED
            print("%-10s %s  %s; %s %s  (%.1fs)" % ("OK" if ok else "FAILED", k, how, res["verdict"],
                                                     res.get("layer") or "", time.time() - t))
            if not ok:
                bad += 1
                print("  " + (res.get("detail") or "")[:300])
            if a.record:
                if ok:
                    record(int(k, 16), res, rows[k]["src"], doc, quiet=True)
                elif res["verdict"] != "MATCH":
                    rows.pop(k, None)
                    print("  removed from docs/equiv_proofs.json")
        if a.record:
            save_proofs(doc)
        return 1 if bad else 0
    if not (a.src and a.va):
        ap.error("src and va are required")
    va = int(a.va, 16)
    t = time.time()
    layers = [x.replace("_layer", "") for x in a.layers.split(",") if x]
    res = run(a.src, va, a.symbol, a.flags.split() if a.flags else None, a.level, a.mutants,
              frame_slots=not a.no_frame_slots, timeout=a.timeout, layers=layers, frame_model=fm, judge=a.hints)
    tag = "%s %s" % (res["level"], res["layer"]) if res["verdict"] in PROVED else ""
    print("%s  0x%08x  %s  (%.2fs)" % (res["verdict"], va, tag, time.time() - t))
    if res.get("detail"):
        print("  " + res["detail"][:600])
    if a.json:
        print(json.dumps(res, indent=1, default=str))
    if a.record and res["verdict"] in PROVED:
        record(va, res, a.src)
    return 0 if res["verdict"] in PROVED + ("MATCH",) else 1


if __name__ == "__main__":
    sys.exit(main())
