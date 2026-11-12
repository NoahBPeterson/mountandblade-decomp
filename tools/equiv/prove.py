#!/usr/bin/env python3
"""prove: is our build of a function behaviourally equivalent to the original, by a static proof?

  uv run python tools/equiv/prove.py <src.cpp> <va> [--symbol SYM] [--flags "..."] [--level L1|L2] [--json]
                                     [--no-frame-slots] [--mutants N]

Ported from spore-decomp tools/contrib/prove.py (commit aaaf5c79f), with the one layer M&B has so far:
  rename   register renaming + frame-slot renaming + independent reordering (tools/equiv/rename_core.py)
Verdicts:
  MATCH       byte-exact (nothing to prove)
  EQUIVALENT  the layer proves it at L2 (every register at every call and ret), or at L1 (the calling contract)
              with the same kill set and entry registers as the original (tools/equiv/facts.py), AND the same
              layer rejects every mutant of our build (tools/equiv/mutate.py)
  UNKNOWN     anything else, with the layer's reason (different CFG, instruction shapes, memory order, ...)
  REFUTED     a layer found a counterexample (the rename layer never refutes; kept for layers to come)
Nothing is recorded anywhere: the verdict is printed (and returned by run()).
"""
import argparse, json, os, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import func as FN    # noqa: E402
import facts         # noqa: E402
import mutate        # noqa: E402

LAYERS = ("rename",)
PROVE_VERSION = "1"


def asm_transcribed(src, va):
    """An __asm/naked/_emit transcription never counts (docs/matching.md)."""
    try:
        import asm_audit
        t = asm_audit.transcribed(os.path.join(ROOT, src)) if hasattr(asm_audit, "transcribed") else {}
        return t.get("%08x" % va)
    except Exception:
        return None


def run(src, va, symbol=None, flags=None, level=None, k_mutants=6, frame_slots=True, log=print, timeout=30.0):
    t0 = time.time()
    retail = FN.retail_func(va)
    ours = FN.our_func(src, va, symbol, flags)
    t_build = time.time() - t0
    asm = asm_transcribed(ours.meta["src"], va)
    if asm:
        return {"verdict": "ERROR", "detail": "assembly transcription (%s): write it as C++ first" % asm}
    if FN.is_byte_exact(retail, ours):
        return {"verdict": "MATCH", "detail": "byte-exact: nothing to prove", "symbol": ours.meta["symbol"]}
    same, why, fa = facts.same(retail, ours)
    lv = level or ("L1" if same else "L2")
    log("facts: kill %s, entry %s; %s -> proving at %s" % (fa["kill"], fa["entry"],
                                                           "same as ours" if same else why, lv))
    base = {"symbol": ours.meta["symbol"], "level": lv, "facts_same": same, "facts_why": why,
            "insns": [len(list(FN.MD.disasm(retail.code, retail.va))), len(list(FN.MD.disasm(ours.code, 0)))],
            "build_s": round(t_build, 3)}
    t1 = time.time()
    for name in LAYERS:
        mod = __import__(name)
        r = mod.prove(retail, ours, lv, None, timeout, frame_slots=frame_slots)
        log("  %-10s %-8s %6.3fs  %s" % (name, r["status"], r["seconds"], r["detail"][:200]))
        base["check_s"] = round(time.time() - t1, 3)
        if r["status"] == "refuted":
            return dict(base, verdict="REFUTED", layer=name, detail=r["detail"])
        if r["status"] != "proved":
            return dict(base, verdict="UNKNOWN", layer=name, detail=r["detail"])
        bad, tried = [], []
        for desc, m in mutate.mutants(ours, k_mutants, seed=va):
            mr = mod.prove(retail, m, lv, None, timeout, frame_slots=frame_slots)
            tried.append(desc)
            if mr["status"] == "proved":
                bad.append(desc)
        if bad:
            log("  %-10s self-test FAILED: proved mutant(s) %s; its proof is not accepted" % (name, "; ".join(bad)))
            return dict(base, verdict="UNKNOWN", layer=name, detail="self-test failed: proved mutant(s) %s" % "; ".join(bad),
                        mutants=tried)
        base["check_s"] = round(time.time() - t1, 3)
        bundle = {"va": "%08x" % va, "level": lv, "build": FN.build_hash(ours), "src": ours.meta["src"],
                  "symbol": ours.meta["symbol"], "flags": " ".join(ours.meta["flags"]),
                  "facts": {"kill": fa["kill"], "entry": fa["entry"]}, "layer": name,
                  "info": r["info"], "tools": {"prove": PROVE_VERSION}}
        return dict(base, verdict="EQUIVALENT", layer=name, detail=r["detail"], info=r["info"], mutants=tried,
                    bundle=bundle)
    return dict(base, verdict="UNKNOWN", detail="no layer proved it")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("va")
    ap.add_argument("--symbol")
    ap.add_argument("--flags")
    ap.add_argument("--level", choices=("L1", "L2"))
    ap.add_argument("--mutants", type=int, default=6)
    ap.add_argument("--no-frame-slots", action="store_true", help="frame operands by offset only (the spore rule)")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()
    va = int(a.va, 16)
    t = time.time()
    res = run(a.src, va, a.symbol, a.flags.split() if a.flags else None, a.level, a.mutants,
              frame_slots=not a.no_frame_slots)
    tag = ""
    if res["verdict"] == "EQUIVALENT":
        tag = "%s %s" % (res["level"], res["layer"])
    print("%s  0x%08x  %s  (%.2fs)" % (res["verdict"], va, tag, time.time() - t))
    if res.get("detail"):
        print("  " + res["detail"])
    if a.json:
        print(json.dumps(res, indent=1, default=str))
    return 0 if res["verdict"] in ("EQUIVALENT", "MATCH") else 1


if __name__ == "__main__":
    sys.exit(main())
