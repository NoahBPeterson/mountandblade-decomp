"""Layer 1: register renaming, frame-slot renaming and scheduling (rename_core.py). Milliseconds; proves
equivalence when the two functions differ only by a consistent renaming of registers and frame slots and a
reordering of independent instructions. Sound by construction (same CFG, same def-use graph); it never refutes, a
rejection is `unknown`.

Ported from spore-decomp tools/equiv/rename.py (commit aaaf5c79f): M&B has no /LTCG builds, so there is one call
model (std where facts.std_call_safe, all registers otherwise); the original callees' entry registers and `ret N`
come from facts.py."""
import os
import time

import facts
import func as FN
import rename_core

rename_core.STD_SAFE = facts.std_call_safe
rename_core.ENTRY_REGS = facts.retail_entry
rename_core.CALLEE_POP = facts.callee_pop
rename_core.TOKEN_OK = FN.const_ok


_HCACHE = {}


def rules_from_env():
    """EQUIV_RENAME_RULES=tailmerge,splice (or `none`, `all`): which P1-D rules are on (default: all)"""
    v = os.environ.get("EQUIV_RENAME_RULES", "all").strip()
    if v in ("", "all"):
        return set(rename_core.ALL_RULES)
    return set() if v == "none" else {r for r in v.split(",") if r in rename_core.ALL_RULES}


def _helper_a(va):
    f = FN.retail_func(va)
    return f.code, f.relocs


def prove_helpers(ours):
    """our callee's code from our own object (the same TU), for the splice rule"""
    obj = ours.meta.get("obj")

    def get(name):
        if not obj:
            return None
        if (obj, name) not in _HCACHE:
            try:
                from coff import Coff
                c = Coff(obj)
                sy = c.by_name.get(name)
                if sy is None or not sy.defined or not sy.secno:
                    raise SystemExit("not defined here")
                f = FN.obj_func(obj, name)
                _HCACHE[(obj, name)] = (f.code, f.relocs)
            except BaseException:          # SystemExit: not defined in this object
                _HCACHE[(obj, name)] = None
        return _HCACHE[(obj, name)]
    return get


def prove(retail, ours, level, hints=None, timeout=10.0, frame_slots=True):
    t = time.time()
    FN.set_consts(ours)
    rename_core.FRAME_SLOTS = frame_slots
    rename_core.RULES = rules_from_env()
    rename_core.HELPER_A = _helper_a
    rename_core.HELPER_B = prove_helpers(ours)
    try:
        ok, info = rename_core.check(retail.code, retail.va, retail.relocs, ours.code, ours.va, ours.relocs,
                                     level=2 if level == "L2" else 1, ret64=bool(ours.meta.get("ret64")))
    finally:
        rename_core.FRAME_SLOTS = True
    return {"status": "proved" if ok else "unknown", "seconds": round(time.time() - t, 3),
            "detail": rename_core.info_text(info), "info": info if ok else None, "hints": {}}
