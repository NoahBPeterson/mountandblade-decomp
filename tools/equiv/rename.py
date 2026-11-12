"""Layer 1: register renaming, frame-slot renaming and scheduling (rename_core.py). Milliseconds; proves
equivalence when the two functions differ only by a consistent renaming of registers and frame slots and a
reordering of independent instructions. Sound by construction (same CFG, same def-use graph); it never refutes, a
rejection is `unknown`.

Ported from spore-decomp tools/equiv/rename.py (commit aaaf5c79f): M&B has no /LTCG builds, so there is one call
model (std where facts.std_call_safe, all registers otherwise); the original callees' entry registers and `ret N`
come from facts.py."""
import time

import facts
import func as FN
import rename_core

rename_core.STD_SAFE = facts.std_call_safe
rename_core.ENTRY_REGS = facts.retail_entry
rename_core.CALLEE_POP = facts.callee_pop
rename_core.TOKEN_OK = FN.const_ok


def prove(retail, ours, level, hints=None, timeout=10.0, frame_slots=True):
    t = time.time()
    FN.set_consts(ours)
    rename_core.FRAME_SLOTS = frame_slots
    try:
        ok, info = rename_core.check(retail.code, retail.va, retail.relocs, ours.code, ours.va, ours.relocs,
                                     level=2 if level == "L2" else 1, ret64=bool(ours.meta.get("ret64")))
    finally:
        rename_core.FRAME_SLOTS = True
    return {"status": "proved" if ok else "unknown", "seconds": round(time.time() - t, 3),
            "detail": rename_core.info_text(info), "info": info if ok else None, "hints": {}}
