#!/usr/bin/env python3
"""Pairing hints from an external static judge, as CANDIDATES for the region layer (prove.py / sweep.py --hints DIR).

The judge (vs2005-cpp tools/judge.py, its docs/c2/judge.md section 4) writes one <8-hex va>.json per function:
  {"va", "verdict": EQUIVALENT | MISMATCH | UNKNOWN,
   "regs":  {"ours:<reg>": "orig:<reg>"},                 our register holding the original's value at its loop heads
   "slots": {"ours:esp-0x20": "orig:esp-0x24"},           frame slots, offsets from esp at entry
   "globals": {"ours:<decorated symbol>": "orig:0x<addr>"}, "assumed": [the globals no project map anchored],
   "at": {"0x<orig block>": {"regs": {...}, "slots": {...}}}}  the same per loop head of the original
A MISMATCH / UNKNOWN row's hints hold only up to where the judge stopped.

Nothing here is trusted. The region layer uses a hint only where its own machinery checks it (LAYERS.md "Judge
hints"): a register pair at a stop is put into the exit correspondence pi' only when the solver proves the two values
equal there (angr_layer._match_exit), a frame-slot pair into F' only when the two words are proved equal there
(angr_layer._frame_exit_diffs, frame model only), and either becomes an obligation of the region that establishes
it, exactly like a pair the layer found itself. A global pair is used only when the symbols file or a literal's
content confirms it (check_globals); an unanchored one is dropped. A hint that fails its check is dropped and
logged; a wrong hint can therefore cost a proof (never: it is only ever tried in addition to the layer's own
choices), and can never make one.

  load(dir, va) -> the parsed hints or None;  pairing(h) -> the region worker's job["pairing"]
"""
import json, os, re

GPR7 = ("eax", "ecx", "edx", "ebx", "ebp", "esi", "edi")      # regions.PI_REGS[:7]
LITERAL = ("__real@", "__int64@", "??_C@", "__xmm@")


def _side(s, want):
    if not isinstance(s, str) or not s.startswith(want + ":"):
        return None
    return s[len(want) + 1:]


def _slot(s):
    """'esp-0x20' -> 0x20 (the offset below the entry esp, the frame model's o); 'esp+0x8' -> -8; else None"""
    m = re.fullmatch(r"esp([+-])(0x[0-9a-fA-F]+|\d+)", s or "")
    if not m:
        return None
    v = int(m.group(2), 0)
    return v if m.group(1) == "-" else -v


def _regs(d, log):
    """{"ours:r": "orig:q"} -> {q: r} (the original's register -> ours'), one to one; the rest dropped with a note"""
    out, used = {}, set()
    for k, v in sorted((d or {}).items()):
        r, q = _side(k, "ours"), _side(v, "orig")
        if r not in GPR7 or q not in GPR7:
            log.append("register hint %s -> %s: not a general register pair" % (k, v))
            continue
        if q in out or r in used:
            log.append("register hint %s -> %s: not one to one, dropped" % (k, v))
            out.pop(q, None)
            continue
        out[q] = r
        used.add(r)
    return out


def _slots(d, log):
    """{"ours:esp-0x20": "orig:esp-0x24"} -> {0x24: 0x20} (the original's frame offset -> ours'), frame words only
    (below the entry esp, 4-aligned), one to one"""
    out, used = {}, set()
    for k, v in sorted((d or {}).items()):
        o, q = _slot(_side(k, "ours")), _slot(_side(v, "orig"))
        if o is None or q is None:
            log.append("slot hint %s -> %s: not an esp offset" % (k, v))
            continue
        if o <= 0 or q <= 0:
            continue                # at or above the entry esp: the caller's memory, equal at the same address anyway
        if o % 4 or q % 4:
            log.append("slot hint %s -> %s: not a word slot" % (k, v))
            continue
        if q in out or o in used:
            log.append("slot hint %s -> %s: not one to one, dropped" % (k, v))
            out.pop(q, None)
            continue
        out[q] = o
        used.add(o)
    return out


def load(path, va):
    """the hints for va from a directory of <8-hex va>.json (or one file) -> dict or None. The result:
    {"va", "verdict", "regs": {orig r: ours r}, "slots": {orig o: ours o}, "at": {orig addr: {"regs", "slots"}},
     "globals": {our symbol: orig addr}, "assumed": set of our symbols, "notes": [what was dropped while parsing]}"""
    if os.path.isdir(path):
        path = os.path.join(path, "%08x.json" % va)
    if not os.path.exists(path):
        return None
    try:
        raw = json.load(open(path))
    except (OSError, ValueError):
        return None
    try:
        if int(str(raw.get("va")), 16) != va:
            return None
    except (TypeError, ValueError):
        return None
    notes = []
    at = {}
    for a, v in (raw.get("at") or {}).items():
        try:
            at[int(a, 16)] = {"regs": _regs((v or {}).get("regs"), notes), "slots": _slots((v or {}).get("slots"), notes)}
        except (TypeError, ValueError):
            notes.append("loop head %r: not an address" % (a,))
    glb = {}
    for k, v in (raw.get("globals") or {}).items():
        s, a = _side(k, "ours"), _side(v, "orig")
        try:
            glb[s] = int(a, 16)
        except (TypeError, ValueError):
            notes.append("global hint %s -> %s: not an address" % (k, v))
    return {"va": va, "verdict": raw.get("verdict"), "regs": _regs(raw.get("regs"), notes),
            "slots": _slots(raw.get("slots"), notes), "at": at, "globals": glb,
            "assumed": {_side(s, "ours") for s in raw.get("assumed") or ()}, "notes": notes}


def pairing(h):
    """the region worker's job["pairing"] (JSON): {"regs": {q: r}, "slots": {"%x" q: o}, "at": {"%x" addr: {"regs",
    "slots"}}}; None when h has no register or slot pair"""
    if not h:
        return None
    sl = lambda d: {"%x" % q: o for q, o in sorted(d.items())}
    at = {"%x" % a: {"regs": dict(v["regs"]), "slots": sl(v["slots"])} for a, v in sorted(h["at"].items())
          if v["regs"] or v["slots"]}
    if not (h["regs"] or h["slots"] or at):
        return None
    return {"regs": dict(h["regs"]), "slots": sl(h["slots"]), "at": at}


def at_regs(p, addr):
    """the hinted register map at the original's stop addr (its loop-head map, else the function-wide one)"""
    if not p:
        return {}
    a = (p.get("at") or {}).get("%x" % addr)
    return dict(p.get("regs") or {}, **((a or {}).get("regs") or {}))


def at_slots(p, addr):
    """the hinted slot map {orig o: ours o} at the original's stop addr"""
    if not p:
        return {}
    a = (p.get("at") or {}).get("%x" % addr)
    d = dict(p.get("slots") or {}, **((a or {}).get("slots") or {}))
    return {int(q, 16): o for q, o in d.items()}


def check_globals(ours, h, symbol_want, const_ok):
    """the hinted global pairs that the project's own checks confirm -> ({ours relocation key: orig addr}, [notes]).
    Confirmed: the symbols file gives the symbol exactly this address (symbol_want), or the symbol is a literal whose
    bytes at the original's address are ours' (const_ok). Everything else is dropped: an unanchored pairing cannot be
    checked here (the judge's own "assumed" list names most of them), and a contradicted one is wrong."""
    ok, notes = {}, []
    if not h or not h.get("globals"):
        return ok, notes
    by_name = {}
    for _o, tok in sorted(ours.relocs.items()):
        if tok[0] == "sym":
            by_name.setdefault(tok[1], tok)
    for s, addr in sorted(h["globals"].items()):
        tok = by_name.get(s)
        if tok is None:
            notes.append("global hint %s: no relocation of ours names it" % s[:60])
            continue
        tok0 = (tok[0], tok[1], tok[2], 0)
        want = symbol_want(tok0)
        if want is not None:
            if want != addr:
                notes.append("global hint %s -> 0x%x rejected: the symbols file has 0x%x" % (s[:60], addr, want))
                continue
            ok[s] = addr
            continue
        if s.startswith(LITERAL):
            if const_ok(("abs", addr), tok0):
                ok[s] = addr
            else:
                notes.append("global hint %s -> 0x%x rejected: other bytes in the original" % (s[:60], addr))
            continue
        notes.append("global hint %s -> 0x%x dropped: unanchored (no symbols-file entry, not a literal)" % (
            s[:60], addr))
    return ok, notes
