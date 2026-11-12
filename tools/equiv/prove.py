#!/usr/bin/env python3
"""prove: is our build of a function behaviourally equivalent to the original, by a static proof?

  uv run python tools/equiv/prove.py <src.cpp> <va> [--symbol SYM] [--flags "..."] [--level L1|L2] [--json]
                                     [--layers rename,angr,ddec] [--timeout S] [--mutants N] [--no-frame-slots]
                                     [--record] [--recheck]
  uv run python tools/equiv/prove.py --recheck-all [--record]      # every row of docs/equiv_proofs.json

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
  REFUTED     a layer found a counterexample it trusts (the rename layer never refutes)
  UNKNOWN     anything else, with each layer's reason
--record (on EQUIVALENT) writes the row of docs/equiv_proofs.json (va -> src, symbol, layer, level, detail, build,
flags, bundle) and the proof bundle docs/equiv_proofs/<va>.json (layer, level, build hash, facts, the layer's hints;
no game bytes), or work/equiv/proofs/<va>.json when it is over spore's size cap (2 KB + 1 KB per KB of code).
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
MODULE = {"rename": "rename", "angr": "angr_layer", "ddec": "ddec_layer"}
TIMEOUT = {"rename": 30.0, "angr": 120.0, "ddec": 600.0}
PROVE_VERSION = "2"
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
        v["stoke"] = "98d8a0f+ddec-hints-1"
    return v


def retail_hash(retail):
    return hashlib.sha256(retail.code).hexdigest()


def layer_prove(name, mod, retail, ours, lv, hints, timeout, frame_slots):
    if name == "rename":
        return mod.prove(retail, ours, lv, hints, timeout, frame_slots=frame_slots)
    return mod.prove(retail, ours, lv, hints, timeout)


def run(src, va, symbol=None, flags=None, level=None, k_mutants=6, frame_slots=True, log=print, timeout=None,
        layers=LAYERS, hints=None):
    """timeout: seconds per layer run (None: TIMEOUT per layer). hints: {layer: hints} (a recheck)."""
    t0 = time.time()
    retail = FN.retail_func(va)
    ours = FN.our_func(src, va, symbol, flags)
    t_build = time.time() - t0
    asm = asm_transcribed(ours.meta["src"], va)
    if asm:
        return {"verdict": "ERROR", "detail": "assembly transcription (%s): write it as C++ first" % asm}
    if FN.is_byte_exact(retail, ours):
        return {"verdict": "MATCH", "detail": "byte-exact: nothing to prove", "symbol": ours.meta["symbol"],
                "build": FN.build_hash(ours)}
    same, why, fa = facts.same(retail, ours)
    lv = level or ("L1" if same else "L2")
    log("facts: kill %s, entry %s; %s -> proving at %s" % (fa["kill"], fa["entry"],
                                                           "same as ours" if same else why, lv))
    base = {"symbol": ours.meta["symbol"], "level": lv, "facts_same": same, "facts_why": why,
            "insns": [len(list(FN.MD.disasm(retail.code, retail.va))), len(list(FN.MD.disasm(ours.code, 0)))],
            "build_s": round(t_build, 3), "build": FN.build_hash(ours), "layers": {}}
    t1 = time.time()
    reasons = []
    for name in layers:
        mod = load_layer(name)
        if isinstance(mod, str):
            log("  %-10s skipped (%s)" % (name, mod))
            base["layers"][name] = ["skipped", mod[:200]]
            continue
        tmo = timeout or TIMEOUT.get(name, 120.0)
        h = (hints or {}).get(name) if hints else None
        r = layer_prove(name, mod, retail, ours, lv, h, tmo, frame_slots)
        log("  %-10s %-8s %6.2fs  %s" % (name, r["status"], r["seconds"], r["detail"][:200]))
        base["layers"][name] = [r["status"], r["detail"][:300], r["seconds"]]
        base["check_s"] = round(time.time() - t1, 3)
        if r["status"] == "refuted":
            return dict(base, verdict="REFUTED", layer=name, detail=r["detail"])
        if r["status"] != "proved":
            reasons.append("%s: %s" % (name, r["detail"]))
            continue
        bad, tried = [], []
        for desc, m in mutate.mutants(ours, k_mutants, seed=va, covered=r.get("covered")):
            mr = layer_prove(name, mod, retail, m, lv, None, tmo, frame_slots)
            tried.append([desc, mr["status"]])
            if mr["status"] == "proved":
                bad.append(desc)
        base["check_s"] = round(time.time() - t1, 3)
        if bad:
            log("  %-10s self-test FAILED: proved mutant(s) %s; its proof is not accepted" % (name, "; ".join(bad)))
            reasons.append("%s: self-test failed: proved mutant(s) %s" % (name, "; ".join(bad)))
            base["layers"][name][0] = "selftest-failed"
            continue
        bundle = {"va": "%08x" % va, "level": lv, "retail_fp": retail_hash(retail), "build": FN.build_hash(ours),
                  "src": ours.meta["src"], "symbol": ours.meta["symbol"], "flags": " ".join(ours.meta["flags"]),
                  "facts": {"kill": fa["kill"], "entry": fa["entry"]}, "layer": name,
                  "hints": r.get("hints") or {}, "tools": tool_versions(name)}
        return dict(base, verdict="EQUIVALENT", layer=name, detail=r["detail"], info=r.get("info"), mutants=tried,
                    bundle=bundle)
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


def bundle_path(va, size_ok=True):
    return os.path.join(BUNDLES if size_ok else BIG_BUNDLES, "%08x.json" % va)


def record(va, res, src, doc=None, quiet=False):
    """Write the row of docs/equiv_proofs.json and the bundle. doc: an already loaded proofs document (saved by the
    caller) or None (load and save here)."""
    b = res["bundle"]
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
    doc = doc if doc is not None else load_proofs()
    doc.setdefault("rows", {})["%08x" % va] = {
        "src": os.path.relpath(os.path.join(ROOT, src), ROOT).replace(os.sep, "/"),
        "symbol": b["symbol"], "layer": b["layer"], "level": b["level"], "detail": (res.get("detail") or "")[:300],
        "build": b["build"], "flags": b["flags"], "bundle": os.path.relpath(path, ROOT).replace(os.sep, "/"),
        "proved": datetime.date.today().isoformat()}
    if own:
        save_proofs(doc)
    if not quiet:
        print("recorded %08x (%s %s) in %s; bundle %s (%d bytes)" % (va, b["layer"], b["level"],
                                                                   os.path.relpath(PROOFS, ROOT),
                                                                   os.path.relpath(path, ROOT), len(data)))


def find_bundle(va, row=None):
    for p in ([os.path.join(ROOT, row["bundle"])] if row and row.get("bundle") else []) + \
             [bundle_path(va, True), bundle_path(va, False)]:
        if os.path.exists(p):
            return json.load(open(p))
    return None


def recheck(va, row, log=print, k_mutants=6, timeout=None):
    """-> (result, how). A recorded proof is re-verified: build hash equal -> the recorded layer with its hints;
    else proved again with every layer."""
    b = find_bundle(va, row)
    src = (b or {}).get("src") or row["src"]
    sym = (b or {}).get("symbol") or row.get("symbol")
    level = (b or {}).get("level") or row.get("level")
    cur = FN.our_func(os.path.join(ROOT, src), va, sym)
    if b and b.get("build") == FN.build_hash(cur):
        layer = b["layer"]
        res = run(os.path.join(ROOT, src), va, sym, level=level, layers=[layer], hints={layer: b.get("hints")},
                  k_mutants=k_mutants, log=log, timeout=timeout)
        return res, "hinted recheck (%s)" % layer
    why = "no bundle" if not b else "our build changed since the proof (build hash differs)"
    log("  %s: proving again" % why)
    res = run(os.path.join(ROOT, src), va, sym, k_mutants=k_mutants, log=log, timeout=timeout)
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
    ap.add_argument("--mutants", type=int, default=6)
    ap.add_argument("--no-frame-slots", action="store_true", help="frame operands by offset only (the spore rule)")
    ap.add_argument("--record", action="store_true", help="on EQUIVALENT, write docs/equiv_proofs.json + bundle")
    ap.add_argument("--recheck", action="store_true", help="re-verify the recorded proof of va")
    ap.add_argument("--recheck-all", action="store_true", help="re-verify every row of docs/equiv_proofs.json")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()
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
                               k_mutants=a.mutants, timeout=a.timeout)
            ok = res["verdict"] == "EQUIVALENT"
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
              frame_slots=not a.no_frame_slots, timeout=a.timeout, layers=layers)
    tag = "%s %s" % (res["level"], res["layer"]) if res["verdict"] == "EQUIVALENT" else ""
    print("%s  0x%08x  %s  (%.2fs)" % (res["verdict"], va, tag, time.time() - t))
    if res.get("detail"):
        print("  " + res["detail"][:600])
    if a.json:
        print(json.dumps(res, indent=1, default=str))
    if a.record and res["verdict"] == "EQUIVALENT":
        record(va, res, a.src)
    return 0 if res["verdict"] in ("EQUIVALENT", "MATCH") else 1


if __name__ == "__main__":
    sys.exit(main())
