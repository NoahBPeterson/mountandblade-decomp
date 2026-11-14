"""Object-vs-object mode: both sides of an equivalence check from COFF objects (tools/equiv/LAYERS.md, "Object
pairs").

    prove.py --objpair OURS.obj RETAIL.obj SYMBOL          (the cross-compiler corpus: ours = cl 13.10, retail = cl 14)

The layers were written for the M&B pair (the original image work/mb.analysis.bin vs our object). Here the "retail"
side is an object too. It is LINKED into a synthetic image (ObjImage): every section that is not debug/linker info
is placed at an address from 0x401000 up, every undefined symbol gets an address of its own in an external area,
and every DIR32/DIR32NB/REL32 relocation is applied, so the retail function's bytes are what a linker would emit and
its relocated fields hold absolute addresses ("abs" tokens, taken from the relocation table, not guessed by value).
func.image() then returns this image (const_ok reads literals from it, retail_tables reads jump tables, facts.py
reads defined callees), and a stand-in for the symbols file (angr_layer._symdb) names, for EVERY plainly named symbol
of ours, the address of the retail symbol with the same name:

- exact pairing by name: the two objects were compiled from the same source, so a call or data reference of ours to
  name N must be retail's reference to N. Names are compared after `canon` (cl 13.10 declares __m128 a struct, cl 14
  a union: U__m128@@ vs T__m128@@; cl 14 calls __ftol2_sse where cl 13.10 calls __ftol2, the same truncation with an
  SSE2 fast path). A name of ours that retail does not reference gets a FRESH address no retail field holds, so it
  can pair with nothing (symbol_want then rejects every proposal; in the M&B mode an unknown name is unconstrained).
- labels and section symbols ($LN12#20+48, .rdata#3) are not names: a switch table label pairs through the dispatch
  (regions.py lock-step); any other label of ours inside the function's own section (a byte index table read by
  movzx) is checked BY CONTENT like a literal (ours' bytes from the label to the next label must be the bytes at the
  paired retail address), and data labels elsewhere by content up to the end of their section.

Callee facts. A defined retail callee is read from the linked image as in the M&B mode (kill set, entry registers,
`ret N`). An external has no code, so facts.py treats it as unknown (kills every volatile register, reads eax, ecx,
edx) and its `ret N` comes from its decorated name (stdcall/thiscall/fastcall: the declared argument bytes, cdecl:
0), the C++ ABI the caller was compiled against (EXT_POP; facts.callee_pop and angr_layer.scan_callee consult it).

install() replaces the process-wide hooks (func._img, angr_layer._SYMDB, the facts.py memos) for one pair; the sweep
(tools/equiv/corpus/sweep_corpus.py) runs each pair in its own process.
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import func as FN  # noqa: E402  (puts tools/difftest and tools/matching on sys.path)
from coff import Coff, REL_DIR32, REL_DIR32NB, REL_REL32  # noqa: E402

BASE = 0x400000
TEXT0 = 0x401000
EXT_STRIDE = 0x10


def canon(nm):
    """the name both compilers give one symbol"""
    nm = nm.replace("U__m128@@", "T__m128@@")
    if nm == "__ftol2_sse":
        return "__ftol2"
    return nm


def plain(nm):
    """a name obj_func's tokens carry as is (not a label or a section symbol)"""
    return nm.startswith(("?", "_", "@"))


class ObjImage:
    """func.image() stand-in: one object linked at BASE. read / in_text / func_extent as resolve.Image."""

    def __init__(self, path):
        self.path = path
        c = self.coff = Coff(path)
        self.secva = {}
        cur = TEXT0
        for s in c.sections:
            if s.discard or not s.size:
                continue
            al = max(16, s.align or 16)
            cur = (cur + al - 1) // al * al
            self.secva[s.index + 1] = cur
            cur += s.size
        self.ext = {}                      # canon name -> address (undefined symbols)
        cur = (cur + 0xfff) // 0x1000 * 0x1000
        self.ext_lo = cur
        for s in sorted(c.syms.values(), key=lambda s: s.index):
            if not s.defined and s.secno == 0 and canon(s.name) not in self.ext:
                self.ext[canon(s.name)] = cur
                cur += EXT_STRIDE
        self.size = (cur + 0x10000 - BASE + 0xfff) // 0x1000 * 0x1000
        self.data = {}
        for s in c.sections:
            if s.index + 1 not in self.secva:
                continue
            b = bytearray(s.data)
            va = self.secva[s.index + 1]
            for v, si, typ in s.relocs:
                a = self.sym_addr(c.syms.get(si))
                if a is None or v + 4 > len(b):
                    continue
                old = struct.unpack_from("<I", b, v)[0]
                if typ == REL_DIR32:
                    struct.pack_into("<I", b, v, (old + a) & 0xffffffff)
                elif typ == REL_DIR32NB:
                    struct.pack_into("<I", b, v, (old + a - BASE) & 0xffffffff)
                elif typ == REL_REL32:
                    struct.pack_into("<I", b, v, (old + a - (va + v + 4)) & 0xffffffff)
            self.data[va] = bytes(b)
        self.code_secs = {self.secva[s.index + 1]: s.size for s in c.sections
                          if s.index + 1 in self.secva and s.is_code}
        self.funcs = {}
        for nm, s in c.functions().items():
            if s.secno in self.secva:
                sec, f0, f1 = c.func_extent(s)
                lo = self.secva[s.secno] + f0
                self.funcs[lo] = (lo, self.secva[s.secno] + f1)
        # every name -> its addresses (several for a name defined twice: static labels)
        self.names = {}
        for s in c.syms.values():
            a = self.sym_addr(s)
            if a is not None and s.name and plain(s.name):
                self.names.setdefault(canon(s.name), set()).add(a)

    def sym_addr(self, s):
        if s is None:
            return None
        if s.defined:
            if s.secno not in self.secva:
                return None
            return self.secva[s.secno] + s.value
        if s.secno == -1:                  # absolute
            return s.value
        return self.ext.get(canon(s.name))

    @property
    def base(self):
        return BASE

    def read(self, va, n):
        for a, b in self.data.items():
            if a <= va and va + n <= a + len(b):
                return b[va - a:va - a + n]
        raise ValueError("unmapped %x" % va)

    def in_text(self, va):
        return any(a <= va < a + n for a, n in self.code_secs.items())

    def func_extent(self, va):
        for lo, ext in self.funcs.items():
            if lo <= va < ext[1]:
                return ext
        raise ValueError("no function at %x" % va)


class NameDB:
    """angr_layer._symdb() stand-in: .mangled = {our name: {retail address}}"""

    def __init__(self, mangled):
        self.mangled = mangled


EXT_POP = {}            # retail external address -> bytes its callee pops (from the decorated name), or None


def name_pop(nm):
    import angr_layer as AL
    f = AL.name_facts(nm)
    conv = f.get("conv")
    if conv == "cdecl":
        return 0
    if conv in ("stdcall", "thiscall", "fastcall") and isinstance(f.get("W"), int):
        return f["W"]
    return None


def _hooks():
    """install facts.callee_pop / angr_layer.scan_callee wrappers that know the externals (once per process)"""
    import angr_layer as AL
    import facts
    if getattr(facts.callee_pop, "_objpair", False):
        return
    cp, sc = facts.callee_pop, AL.scan_callee

    def callee_pop(va):
        if va in EXT_POP:
            return EXT_POP[va]
        return cp(va)
    callee_pop._objpair = True

    def scan_callee(va, budget=4000):
        if va in EXT_POP:
            return {"retn": EXT_POP[va]} if EXT_POP[va] is not None else {}
        return sc(va, budget)
    facts.callee_pop = callee_pop
    AL.scan_callee = scan_callee


def _labels(c, secno):
    """sorted offsets of every symbol in a section (labels included)"""
    return sorted({s.value for s in c.syms.values() if s.secno == secno})


def load(ours_obj, retail_obj, symbol, ours_symbol=None):
    """-> (retail Func, ours Func, ObjImage, NameDB). symbol: retail's decorated name; ours_symbol: ours' (default:
    the same name, or the one of ours with the same canon)."""
    img = ObjImage(retail_obj)
    rc = img.coff
    rs = rc.by_name.get(symbol)
    if rs is None or not rs.defined:
        raise SystemExit("%s: no function %s" % (retail_obj, symbol))
    oc = Coff(ours_obj)
    osym = ours_symbol
    if osym is None:
        osym = symbol if symbol in oc.by_name else next(
            (n for n, s in oc.functions().items() if canon(n) == canon(symbol)), None)
    if osym is None or osym not in oc.by_name:
        raise SystemExit("%s: no function matching %s" % (ours_obj, symbol))
    # ---- retail: the linked function
    sec, f0, f1 = rc.func_extent(rs)
    va = img.secva[rs.secno] + f0
    full = bytes(img.read(va, f1 - f0))
    ins = list(FN.MD.disasm(full, va))
    end = FN.code_end(ins, va, va + len(full))
    code = full[:end - va]
    toks = {}
    for v, si, typ in sec.relocs:
        if not f0 <= v < f0 + len(code) or v + 4 > f0 + len(code):
            continue
        o = v - f0
        if typ == REL_REL32:
            t = (va + o + 4 + struct.unpack_from("<i", code, o)[0]) & 0xffffffff
            if va <= t < va + len(code):
                continue
            toks[o] = ("abs", t)
        elif typ in (REL_DIR32, REL_DIR32NB):
            toks[o] = ("abs", struct.unpack_from("<I", code, o)[0])
    retail = FN.Func(va, code, toks, "%s (retail obj)" % symbol, "retail",
                     {"size": len(code), "obj": retail_obj, "symbol": symbol})
    # ---- ours: the object as the M&B mode loads it
    ours = FN.obj_func(ours_obj, osym)
    ours.meta.update({"src": "objpair:" + os.path.basename(ours_obj), "flags": [], "ret64": FN.ret64(osym),
                      "objpair": True})
    # ---- the name pairing (exact) and the labels' contents
    used = {t[1] for t in toks.values()}
    fresh = [img.ext_lo + 0x100000 - EXT_STRIDE]

    def new_addr():
        while fresh[0] in used:
            fresh[0] -= EXT_STRIDE
        a = fresh[0]
        used.add(a)
        fresh[0] -= EXT_STRIDE
        return a
    mangled = {}
    for s in oc.syms.values():
        if not s.name or not plain(s.name) or s.name in mangled:
            continue
        vas = img.names.get(canon(s.name))
        mangled[s.name] = set(vas) if vas else {new_addr()}
    consts = dict(ours.meta.get("consts") or {})
    os_ = oc.by_name[osym]
    osec, of0, of1 = oc.func_extent(os_)
    dispatch_labels = set()
    for i in FN.MD.disasm(ours.code, 0):
        if i.mnemonic == "jmp" and i.disp_offset and (i.disp_offset + i.address) in ours.relocs:
            dispatch_labels.add(ours.relocs[i.address + i.disp_offset][1])
    for o, t in ours.relocs.items():
        nm = t[1]
        if t[2] != REL_DIR32 or plain(nm) or nm in consts or nm in dispatch_labels:
            continue
        m = re.search(r"#(\d+)(?:\+([0-9a-f]+))?$", nm)
        if not m:
            continue
        secno, val = int(m.group(1)), int(m.group(2) or "0", 16)
        if not 0 < secno <= len(oc.sections):
            continue
        s = oc.sections[secno - 1]
        if s.discard:
            continue
        nxt = [x for x in _labels(oc, secno) if x > val]
        hi = min(nxt) if nxt and secno == os_.secno else len(s.data)
        blob = bytes(s.data[val:hi])
        if s.relocs and any(val <= v < hi for v, _si, _t in s.relocs):
            continue                       # relocated contents: no byte check possible (left to the bijection)
        if blob:
            consts[nm] = blob              # const_ok: the paired retail address must hold these bytes
    ours.meta["consts"] = consts
    db = NameDB(mangled)
    EXT_POP.clear()
    for nm, a in img.ext.items():
        EXT_POP[a] = name_pop(nm)
    return retail, ours, img, db


def install(img, db):
    """make this process's hooks serve the pair (func.image(), the symbols file, the facts memos)"""
    import angr_layer as AL
    import facts
    FN._img = img
    AL._SYMDB[:] = [db]
    for m in (facts._code_memo, facts._kill_memo, facts._entry_memo, facts._pop_memo):
        m.clear()
    _hooks()
    try:
        import regions
        regions._CACHE.clear()
        regions._COFF.clear()
    except ImportError:
        pass


def setup(ours_obj, retail_obj, symbol, ours_symbol=None):
    """load + install -> (retail, ours)"""
    retail, ours, img, db = load(ours_obj, retail_obj, symbol, ours_symbol)
    install(img, db)
    return retail, ours
