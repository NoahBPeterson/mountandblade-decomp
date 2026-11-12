"""The two sides of an equivalence check: the original function (from work/mb.analysis.bin) and our build of it
(from the compiled object).

Ported from spore-decomp tools/equiv/func.py (commit aaaf5c79f), adapted to M&B: the unpacked exe has no .reloc, so
the original's relocated fields are recognised by value (a 4-byte displacement/immediate holding an image address, as
chk.aligned_diff masks them), and the rel32 branches that leave the function.

Func.relocs maps the offset of each 4-byte relocated field to a token:
  original:  ("abs", value)                     value = the absolute address, or the destination of a rel32 call/jmp
  ours:      ("sym", name, coff_type, addend)   a COFF relocation (type 6 DIR32, 7 DIR32NB, 0x14 REL32)
Layers compare tokens through one consistent bijection (the trust the byte-match check places in relocations), plus
the constant check chk.py does: a DIR32 to a __real@/__int64@/??_C@ literal must point, in the original, at the same
bytes (const_ok).

Compiles go through tools/difftest/slice.compile_obj (tools/matching/cl.sh: the laptop compile server when it is up),
the same object cache insn_exact.py, rename_equiv.py and equiv.py use.
"""
import os, re, struct, sys
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _d in ("tools/difftest", "tools/matching"):
    _p = os.path.join(ROOT, _d)
    if _p not in sys.path:
        sys.path.append(_p)

import capstone  # noqa: E402
from capstone import x86_const as X  # noqa: E402

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True
_img = None


@dataclass
class Func:
    va: int                      # where the code sits (ours: 0, the object's own section offset base)
    code: bytes
    relocs: dict = field(default_factory=dict)
    name: str = ""
    side: str = ""               # "retail" or "ours"
    meta: dict = field(default_factory=dict)


def image():
    """tools/difftest/resolve.Image: bytes, extents and imports of work/mb.analysis.bin (loaded once per process)."""
    global _img
    if _img is None:
        import resolve
        if not os.path.exists(os.path.join(ROOT, "work", "mb.analysis.bin")):
            raise SystemExit("needs work/mb.analysis.bin (tools/steamstub21_analysis_image.py, from your own game)")
        _img = resolve.Image()
    return _img


def code_end(insns, lo, hi):
    """Jump tables sit after the code (MSVC): stop at the lowest table address an indexed operand names
    (insn_exact.code_end)."""
    end = hi
    for i in insns:
        if i.mnemonic in ("jmp", "movzx", "mov") and len(i.operands) and any(
                o.type == X.X86_OP_MEM and o.mem.index and o.mem.base == 0 for o in i.operands):
            for o in i.operands:
                if o.type == X.X86_OP_MEM and lo < (o.mem.disp & 0xffffffff) < end:
                    end = o.mem.disp & 0xffffffff
    return end


def retail_tokens(code, base, img_lo, img_hi):
    """4-byte fields of the original that hold an image address (displacement or immediate), and rel32 branch
    fields whose destination is outside the function -> {offset: ("abs", value)}."""
    out = {}
    hi = base + len(code)
    for i in MD.disasm(code, base):
        off = i.address - base
        b = i.bytes
        rel32 = (b[0] in (0xE8, 0xE9) and i.size == 5) or (b[0] == 0x0F and 0x80 <= b[1] <= 0x8F and i.size == 6)
        if rel32:
            t = i.operands[0].imm & 0xffffffff
            if not base <= t < hi:
                out[off + i.size - 4] = ("abs", t)
            continue
        for at, n in ((i.disp_offset, i.disp_size), (i.imm_offset, i.imm_size)):
            if at and n == 4:
                v = struct.unpack_from("<I", code, off + at)[0]
                if img_lo <= v < img_hi:
                    out[off + at] = ("abs", v)
    return out


def retail_func(va):
    img = image()
    lo, hi = img.func_extent(va)
    if lo != va:
        raise SystemExit("0x%08x is not a function start in the index" % va)
    code = bytes(img.read(lo, hi - lo))
    ins = list(MD.disasm(code, lo))
    end = code_end(ins, lo, hi)
    code = code[:end - lo]
    toks = retail_tokens(code, va, img.base, img.base + img.size)
    return Func(va, code, toks, "0x%08x" % va, "retail", {"size": len(code)})


def default_flags(src):
    """The flags insn_exact.py / the equivalence sweep use: the row's cached flags when equiv has run on it, else the
    default plus src/game/include for src/game modules."""
    import slice as S
    fl = list(S.DEFAULT_FLAGS)
    rel = os.path.relpath(os.path.abspath(src), ROOT).replace(os.sep, "/")
    if rel.startswith("src/game/"):
        fl.append("/Isrc/game/include")
    return fl


def compile_src(src, flags=None):
    import slice as S
    flags = list(flags) if flags else default_flags(src)
    # relative /I paths resolve against work/match or the repo root (slice.resolve_include_flags), not the cwd
    obj, log = S.compile_obj(os.path.abspath(src), flags)
    if not obj:
        raise SystemExit("COMPILE FAILED\n" + (log or "")[-2000:])
    return obj, flags


def find_symbol(obj, src, va, size=None):
    """Our symbol for va: slice.find_symbol (manifest, symbols file, marker), then chk.marker_syms, then
    insn_exact.marker_symbol, the same order the sweeps use."""
    import slice as S
    from coff import Coff
    c = Coff(obj)
    sid = os.path.basename(src).rsplit(".", 1)[0]
    sym, how = S.find_symbol(c, sid, va, src, size)
    if isinstance(sym, list):
        sym = sym[0]
    if sym is None:
        import chk
        cands = chk.marker_syms(src, va, list(c.functions()))
        sym = cands[0] if cands else None
    if sym is None:
        import insn_exact
        sym = insn_exact.marker_symbol(c, src, va)
    if sym is None:
        raise SystemExit("0x%08x: compiled function not found: %s" % (va, how))
    return sym


def obj_func(obj, sym):
    from coff import Coff
    import cmpobj
    c = Coff(obj)
    s = c.by_name.get(sym)
    if s is None:
        raise SystemExit("%s: no symbol %s" % (obj, sym))
    sec, f0, f1 = c.func_extent(s)
    data = bytes(sec.data[f0:f1])
    consts = cmpobj.coff_constants(obj)          # {symbol index: expected bytes}
    toks, const_bytes = {}, {}
    for v, si, typ in sec.relocs:
        if f0 <= v < f1 and typ in (0x06, 0x07, 0x14):
            ts = c.syms.get(si)
            nm = ts.name if ts else "?%d" % si
            if ts is not None and ts.cls == 3 and ts.value == 0 and nm.startswith("."):
                nm = "%s#%d" % (nm, ts.secno)       # a section symbol: name the place by its section
            elif ts is not None and not ts.name.startswith(("?", "_", "@")) and ts.defined:
                nm = "%s#%d+%x" % (nm, ts.secno, ts.value)
            addend = struct.unpack_from("<i", data, v - f0)[0]
            toks[v - f0] = ("sym", nm, typ, addend)
            if si in consts:
                const_bytes[nm] = consts[si]
    end = obj_code_end(data, toks, s.secno, f0)
    if end < len(data):
        data = data[:end]
        toks = {o: t for o, t in toks.items() if o + 4 <= end}
    return Func(0, data, toks, sym, "ours", {"obj": obj, "symbol": sym, "consts": const_bytes})


def obj_code_end(data, toks, secno, f0):
    """code_end for our side: in the object an indexed operand's displacement is a DIR32 relocation (the field
    holds 0), so the table address is the target of that relocation, a label in the function's own section
    ('$L123#<secno>+<value>'). Without this the switch tables (and their byte index tables) after the code were
    decoded as instructions (Operation::execute 0x49fa10: 19862 instructions instead of ~15130)."""
    end = len(data)
    for i in MD.disasm(data, 0):
        if i.mnemonic in ("jmp", "movzx", "mov") and any(
                o.type == X.X86_OP_MEM and o.mem.index and o.mem.base == 0 for o in i.operands):
            t = toks.get(i.address + i.disp_offset) if i.disp_offset else None
            if not t:
                continue
            m = re.search(r"#(\d+)\+([0-9a-f]+)$", t[1])
            if m and int(m.group(1)) == secno:
                v = int(m.group(2), 16) + (t[3] if t[3] < 0x80000000 else t[3] - (1 << 32)) - f0
                if i.address + i.size <= v < end:
                    end = v
        if i.address >= end:
            break
    return end


def ret64(symbol):
    """True when the decorated name returns a 64-bit integer (__int64 / unsigned __int64: edx:eax)."""
    return bool(symbol) and re.search(r"@@(?:Y[A-Z]|[A-Z][A-D][A-Z])_[JK]", symbol) is not None


def our_func(src, va, symbol=None, flags=None):
    obj, flags = compile_src(src, flags)
    size = None
    try:
        lo, hi = image().func_extent(va)
        size = hi - lo
    except Exception:
        pass
    sym = symbol
    if sym is not None:
        from coff import Coff
        if sym not in Coff(obj).by_name:
            sym = None
    if sym is None:
        sym = find_symbol(obj, src, va, size)
    f = obj_func(obj, sym)
    f.meta.update({"src": os.path.relpath(os.path.abspath(src), ROOT), "flags": flags, "ret64": ret64(sym)})
    return f


def const_ok(retail_tok, our_tok):
    """A DIR32 to one of our compiler constants (__real@, __int64@, ??_C@ strings) must point, in the original, at
    the same bytes (chk.py's constant check). Other pairs: True (identity is left to the bijection)."""
    if not (isinstance(our_tok, tuple) and our_tok[0] == "sym" and our_tok[2] == 0x06):
        return True
    want = _CONSTS.get(our_tok[1])
    if want is None:
        return True
    if not (isinstance(retail_tok, tuple) and retail_tok[0] == "abs"):
        return False
    add = our_tok[3]
    if not 0 <= add < len(want):
        add = 0
    try:
        have = bytes(image().read(retail_tok[1] - add, len(want)))
    except Exception:
        return False
    return have == want


_CONSTS = {}


def set_consts(f):
    """Make const_ok know our function's constants (call once per checked pair)."""
    _CONSTS.clear()
    _CONSTS.update(f.meta.get("consts") or {})


def is_byte_exact(retail, ours):
    """Same length and the same bytes outside our relocated fields, and every literal constant agrees."""
    if len(retail.code) != len(ours.code):
        return False
    mask = {o + k for o in ours.relocs for k in range(4)}
    if any(retail.code[i] != ours.code[i] for i in range(len(ours.code)) if i not in mask):
        return False
    set_consts(ours)
    for o, t in ours.relocs.items():
        if t[2] == 0x06 and not const_ok(("abs", struct.unpack_from("<I", retail.code, o)[0]), t):
            return False
    return True


def build_hash(f):
    """sha256 of the code with every relocated field zeroed, plus the relocated offsets and their symbols."""
    import hashlib
    import json
    b = bytearray(f.code)
    for o in f.relocs:
        b[o:o + 4] = b"\0\0\0\0"
    return hashlib.sha256(bytes(b) + json.dumps(sorted((o, list(t)) for o, t in f.relocs.items())).encode()).hexdigest()
