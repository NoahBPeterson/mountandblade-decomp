#!/usr/bin/env python3
"""Static equivalence: is our compiled function the original's computation with only registers and stack slots chosen
differently?

usage: rename_equiv.py [--filter SUBSTR] [--va VA ...] [--json OUT] [-v]
       (default: every src/**/*.nonmatching.txt row)

No emulation. Both instruction lists are paired one to one: the same count, the same mnemonics, the same operand
shapes and sizes, branches to the same instruction index, and every non-stack memory operand / immediate address /
call target resolving to the same original address (tools/difftest/resolve.py, the harness's own resolver; a
read-only constant of ours with no original address counts when its bytes equal the original's). Registers and stack
slots may differ freely, as long as the DATAFLOW is the same:

  for every value an instruction reads (a register, the bytes of a stack slot), the instructions that may have
  produced it (its reaching definitions over the shared control-flow graph) are the same paired instructions in both
  builds; a value live at entry must come from the same register / argument slot.

Interfaces are reads too: a call reads eax/ecx/edx and the 64 bytes above esp (its arguments) and clobbers eax/ecx/
edx; a ret reads eax/edx, and in EACH build every callee-saved register (ebx/esi/edi/ebp) must hold its entry value
at ret (untouched, or restored by the pop of its own push). Pushes/pops of different callee-saved registers in the
two builds are fine (each saves what it uses). Flags need no check: the instruction streams and CFGs are identical.
A function that takes a local's address (lea of a stack slot) keeps its stack slots in the dataflow only when all
locals move by one common delta (a pointer may reach any of them); anything else is UNKNOWN, as is a jump table.

Verdicts: SAME (equivalent by construction), DIFF (first difference; normal for a nonmatching row, and says nothing
against equivalence), UNKNOWN (outside these rules). ~0.3 s to load, then milliseconds per function.
"""
import argparse, json, os, struct, sys, time
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "matching"))
import capstone                                   # noqa: E402
from capstone import x86_const as X               # noqa: E402
import resolve as R, coff, slice as SL            # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True

FAM = {}
for fam, names in {"eax": "eax ax al ah", "ebx": "ebx bx bl bh", "ecx": "ecx cx cl ch", "edx": "edx dx dl dh",
                   "esi": "esi si", "edi": "edi di", "ebp": "ebp bp", "esp": "esp sp"}.items():
    for n in names.split():
        FAM[n] = fam
CALLEE_SAVED = ("ebx", "esi", "edi", "ebp")
CALL_ARG_BYTES = 0x40


class Unknown(Exception):
    pass


class Diff(Exception):
    pass


def disasm(code, base):
    out, pos = [], 0
    while pos < len(code):
        i = next(md.disasm(bytes(code[pos:pos + 16]), base + pos, 1), None)
        if i is None:
            raise Unknown("undecodable byte at +%x" % pos)
        out.append(i)
        pos += i.size
    return out


def is_reg(op, i, name):
    return op.type == X.X86_OP_REG and i.reg_name(op.reg) == name


_POPS = {}


def callee_pops(img, va):
    """Bytes the original function at va removes on return (its `ret N`), None when unknown."""
    if va in _POPS:
        return _POPS[va]
    n = None
    try:
        lo, hi = img.func_extent(va)
        if lo != va:
            lo, hi = va, va + 0x2000           # not an indexed start (a CRT helper): read straight from va
    except Exception:
        lo, hi = va, va + 0x2000
    try:
        code = bytes(img.read(lo, hi - lo))
        pos = 0
        while pos < len(code):
            i = next(md.disasm(code[pos:pos + 16], lo + pos, 1), None)
            if i is None:
                break
            if i.mnemonic in ("ret", "retn"):
                n = i.operands[0].imm if i.operands else 0
                break
            if i.mnemonic == "jmp" and i.operands and i.operands[0].type == X.X86_OP_IMM and not (lo <= i.operands[0].imm < hi):
                break                           # a tail jump elsewhere: unknown
            pos += i.size
    except Exception:
        n = None
    _POPS[va] = n
    return n


class Side:
    def __init__(self, insns, call_target=None, img=None):
        self.insns = insns
        self.idx = {i.address: k for k, i in enumerate(insns)}
        self.frame_ebp = len(insns) > 1 and insns[0].mnemonic == "push" and insns[0].op_str == "ebp" \
            and insns[1].mnemonic == "mov" and insns[1].op_str == "ebp, esp"
        d, self.depth = 0, []
        pending = 0                          # bytes pushed since the last call (its arguments, as far as we know)
        for k, i in enumerate(insns):
            self.depth.append(d)
            m, ops = i.mnemonic, i.operands
            if m == "call":
                n = None
                if ops and ops[0].type == X.X86_OP_IMM and img is not None:
                    t = call_target(i) if call_target else None
                    n = callee_pops(img, t) if t is not None else None
                if n is None:
                    # unknown callee (virtual, import, unresolved): it pops its arguments itself (stdcall/thiscall, the
                    # COM and Win32 convention) unless the caller cleans up right after (cdecl: add esp / pop)
                    cdecl = False
                    for nx in insns[k + 1:k + 8]:      # the caller's cleanup, before the next branch or call
                        if nx.mnemonic == "add" and nx.operands and is_reg(nx.operands[0], nx, "esp"):
                            cdecl = True; break
                        if nx.mnemonic.startswith("j") or nx.mnemonic in ("call", "ret", "retn"):
                            break
                    n = 0 if cdecl else pending
                d -= n
                pending = 0
                continue
            if m == "push":
                pending += 2 if ops[0].size == 2 else 4
                d += 2 if ops[0].size == 2 else 4
            elif m == "pop":
                d -= 4
            elif m in ("sub", "add") and ops and is_reg(ops[0], i, "esp"):
                if ops[1].type != X.X86_OP_IMM:
                    raise Unknown("esp adjusted by a register")
                d += ops[1].imm if m == "sub" else -ops[1].imm
            elif m == "and" and ops and is_reg(ops[0], i, "esp"):
                d += 0x100000          # aligned frame (after an ebp frame): its esp slots are a separate group
            elif self.frame_ebp and m == "mov" and i.op_str == "esp, ebp":
                d = 4                  # esp = ebp = entry esp - 4
            elif self.frame_ebp and m == "lea" and is_reg(ops[0], i, "esp") and ops[1].mem.base and \
                    i.reg_name(ops[1].mem.base) == "ebp" and not ops[1].mem.index:
                d = 4 - ops[1].mem.disp    # lea esp, [ebp - n]: the epilogue's frame release
            elif m == "lea" and is_reg(ops[0], i, "esp") and ops[1].mem.base and i.reg_name(ops[1].mem.base) == "esp" \
                    and not ops[1].mem.index:
                d -= ops[1].mem.disp       # lea esp, [esp + n] (n = 0: cl's alignment no-op)
            elif ops and any(is_reg(o, i, "esp") and (o.access & capstone.CS_AC_WRITE) for o in ops) and m not in ("push", "pop"):
                raise Unknown("esp written by %s" % m)

    def slot(self, k, mem, i):
        """Canonical offset (relative to the entry esp) of an esp/ebp-frame memory operand, else None."""
        base = i.reg_name(mem.base) if mem.base else None
        if base == "esp" or (base == "ebp" and self.frame_ebp):
            if mem.index:
                raise Unknown("indexed stack access")
            return mem.disp - self.depth[k] if base == "esp" else mem.disp - 4
        return None


def succs(side, k):
    i = side.insns[k]
    m = i.mnemonic
    if m in ("ret", "retn"):
        return []
    out = []
    if m.startswith("j") or m.startswith("loop") or m == "jecxz":
        t = i.operands[0].imm if i.operands and i.operands[0].type == X.X86_OP_IMM else None
        if t is not None and t in side.idx:
            out.append(side.idx[t])
        if m == "jmp":
            return out
    if k + 1 < len(side.insns):
        out.append(k + 1)
    return out


def accesses(side, k, slots_tracked):
    """[(key, loc, kind)] for instruction k: kind 'u' (use) or 'd' (def); loc is a register family or ('s', byte).
    Keys identify the same access in the paired instruction of the other build."""
    i = side.insns[k]
    m = i.mnemonic
    acc = []

    def reg(key, name, kind):
        f = FAM.get(name)
        if f is None or f == "esp":
            return
        acc.append((key, f, kind))
        if kind == "d" and name != f:           # a partial write keeps the rest of the register
            acc.append((key + ("part",), f, "u"))

    def slot_bytes(key, canon, size, kind):
        if not slots_tracked:
            return
        for b in range(size):
            acc.append((key + (b,), ("s", canon + b), kind))

    zero_idiom = m in ("xor", "sub") and len(i.operands) == 2 and all(op.type == X.X86_OP_REG for op in i.operands) \
        and i.operands[0].reg == i.operands[1].reg          # xor r, r: the result is 0 whatever r held
    for j, op in enumerate(i.operands):
        rd, wr = bool(op.access & capstone.CS_AC_READ), bool(op.access & capstone.CS_AC_WRITE)
        if zero_idiom:
            rd = False
        if op.type == X.X86_OP_REG:
            if rd: reg(("op", j), i.reg_name(op.reg), "u")
            if wr: reg(("op", j), i.reg_name(op.reg), "d")
        elif op.type == X.X86_OP_MEM:
            mem = op.mem
            sl = side.slot(k, mem, i)
            if sl is None:
                if mem.base: reg(("op", j, "base"), i.reg_name(mem.base), "u")
                if mem.index: reg(("op", j, "index"), i.reg_name(mem.index), "u")
            elif m != "lea":
                if rd or (not rd and not wr): slot_bytes(("op", j, "mem"), sl, op.size, "u")
                if wr: slot_bytes(("op", j, "mem"), sl, op.size, "d")
    # implicit registers (identical names on both sides: same mnemonic)
    try:
        r_imp, w_imp = i.regs_access()
    except Exception:
        r_imp, w_imp = i.regs_read, i.regs_write
    explicit = {i.reg_name(op.reg) for op in i.operands if op.type == X.X86_OP_REG}
    for r in r_imp:
        n = i.reg_name(r)
        if n not in explicit and n in FAM and not any(op.type == X.X86_OP_MEM and (i.reg_name(op.mem.base) == n or i.reg_name(op.mem.index) == n) for op in i.operands):
            reg(("imp", n), n, "u")
    for r in w_imp:
        n = i.reg_name(r)
        if n not in explicit and n in FAM:
            reg(("imp", n), n, "d")
    d = side.depth[k]
    if m == "push":
        slot_bytes(("push",), -(d + 4), 4, "d")
    elif m == "pop":
        slot_bytes(("pop",), -d, 4, "u")
    elif m == "call":
        for r in ("eax", "ecx", "edx"):
            acc.append((("call", r), r, "u"))
        slot_bytes(("call", "args"), -d, CALL_ARG_BYTES, "u")
        for r in ("eax", "ecx", "edx"):
            acc.append((("call", r), r, "d"))
    elif m in ("ret", "retn"):
        for r in ("eax", "edx"):
            acc.append((("ret", r), r, "u"))
    return acc


def reaching(side, slots_tracked, acc_cache):
    """RD_in[k]: {loc: frozenset(def ids)}, a def id being (k, key) or "entry". A location absent from the map has only
    its entry value at k; None for an instruction no path from the entry reaches."""
    n = len(side.insns)
    preds = defaultdict(list)
    for k in range(n):
        for s_ in succs(side, k):
            preds[s_].append(k)
    acc = [accesses(side, k, slots_tracked) for k in range(n)]
    gen = []
    for k in range(n):
        g = {}
        for key, loc, kind in acc[k]:
            if kind == "d":
                g[loc] = frozenset([(k, key)])
        gen.append(g)
    rd_in, rd_out = [None] * n, [None] * n
    work, inq = [0], {0}
    while work:
        k = work.pop()
        inq.discard(k)
        srcs = [rd_out[p] for p in preds[k] if rd_out[p] is not None]
        if k == 0:
            srcs.append({})                    # the entry: every location holds its entry value
        locs = set()
        for o_ in srcs:
            locs |= set(o_)
        rin = {}
        for loc in locs:
            ds = set()
            for o_ in srcs:
                ds |= o_.get(loc, {"entry"})
            rin[loc] = frozenset(ds)
        if rin == rd_in[k] and rd_out[k] is not None:
            continue
        rd_in[k] = rin
        out = dict(rin)
        out.update(gen[k])
        rd_out[k] = out
        for s_ in succs(side, k):
            if s_ not in inq:
                work.append(s_); inq.add(s_)
    return rd_in, acc


def compare(o, m, ours_target, ours_const):
    if len(o.insns) != len(m.insns):
        raise Diff("instruction count %d vs %d" % (len(o.insns), len(m.insns)))
    lea_slots = False
    frame_pairs = set()
    slot_pairs = {}
    for k, (a, b) in enumerate(zip(o.insns, m.insns)):
        where = "#%d %s %s | %s %s" % (k, a.mnemonic, a.op_str, b.mnemonic, b.op_str)
        if a.mnemonic != b.mnemonic or len(a.operands) != len(b.operands) or bytes(a.prefix) != bytes(b.prefix):
            raise Diff(where)
        is_branch = a.mnemonic.startswith("j") or a.mnemonic in ("call", "loop", "loope", "loopne", "jecxz")
        if is_branch and a.operands and a.operands[0].type == X.X86_OP_IMM:
            ta, tbl = a.operands[0].imm, b.operands[0].imm
            if a.mnemonic == "call" or (a.mnemonic == "jmp" and ta not in o.idx):
                tb = ours_target(b.address + b.size - 4)
                if tb is None:
                    raise Unknown(where + ": call target not resolved")
                if tb != ta:
                    raise Diff(where + ": target %x vs %x" % (ta, tb))
                continue
            if ta not in o.idx or tbl not in m.idx:
                raise Unknown(where + ": branch leaves the function")
            if o.idx[ta] != m.idx[tbl]:
                raise Diff(where + ": branch to #%d vs #%d" % (o.idx[ta], m.idx[tbl]))
            continue
        if a.mnemonic == "jmp" and a.operands and a.operands[0].type == X.X86_OP_MEM:
            raise Unknown(where + ": indirect jump (jump table)")
        for j, (pa, pb) in enumerate(zip(a.operands, b.operands)):
            if pa.type != pb.type or pa.size != pb.size:
                raise Diff(where + ": operand shape")
            if pa.type == X.X86_OP_REG:
                na, nb = a.reg_name(pa.reg), b.reg_name(pb.reg)
                if len(na) == 2 and na[-1] in "lh" and (len(nb) != 2 or nb[-1] != na[-1]):
                    raise Diff(where + ": byte register %s vs %s" % (na, nb))
                if (na == "esp") != (nb == "esp") or (na in FAM) != (nb in FAM) or (na not in FAM and na != nb):
                    raise Diff(where + ": register %s vs %s" % (na, nb))
            elif pa.type == X.X86_OP_IMM:
                at = b.address + b.imm_offset if b.imm_offset else None
                tb = ours_target(at) if at is not None else None
                if at is not None and at in m.reloc_offs:
                    if tb is None:
                        raise Unknown(where + ": immediate address not resolved")
                    if (tb & 0xffffffff) != (pa.imm & 0xffffffff):
                        raise Diff(where + ": address %x vs %x" % (pa.imm & 0xffffffff, tb))
                elif pa.imm != pb.imm:
                    if a.mnemonic in ("sub", "add") and is_reg(a.operands[0], a, "esp") and is_reg(b.operands[0], b, "esp"):
                        frame_pairs.add((a.mnemonic, pa.imm, pb.imm))
                        continue
                    raise Diff(where + ": immediate %#x vs %#x" % (pa.imm, pb.imm))
            elif pa.type == X.X86_OP_MEM:
                ma, mb = pa.mem, pb.mem
                if ma.segment != mb.segment or ma.scale != mb.scale or bool(ma.base) != bool(mb.base) or bool(ma.index) != bool(mb.index):
                    raise Diff(where + ": address form")
                sa, sb = o.slot(k, ma, a), m.slot(k, mb, b)
                if (sa is None) != (sb is None):
                    raise Diff(where + ": stack vs non-stack operand")
                if sa is not None:
                    if (sa >= 0) != (sb >= 0) or (sa >= 0 and sa != sb):
                        raise Diff(where + ": argument slot %d vs %d" % (sa, sb))
                    if a.mnemonic == "lea":
                        lea_slots = True
                    slot_pairs.setdefault(sa, set()).add(sb)
                    continue
                at = b.address + b.disp_offset if b.disp_offset else None
                if at is not None and at in m.reloc_offs:
                    tb = ours_target(at)
                    if tb is None:
                        cb = ours_const(at, pa.size)
                        if cb is None or a.mnemonic == "lea":
                            raise Unknown(where + ": address not resolved")
                        if cb != ("orig", ma.disp & 0xffffffff, pa.size):
                            raise Diff(where + ": constant differs")
                        continue
                    if (tb & 0xffffffff) != (ma.disp & 0xffffffff):
                        raise Diff(where + ": address %x vs %x" % (ma.disp & 0xffffffff, tb))
                elif ma.disp != mb.disp:
                    raise Diff(where + ": displacement %#x vs %#x" % (ma.disp, mb.disp))
    if len({(mn, x, y) for mn, x, y in frame_pairs}) > 4:
        raise Unknown("several frame adjustments differ")
    slots_tracked = True
    if lea_slots:
        deltas = {sb - sa for sa, sbs in slot_pairs.items() if sa < 0 for sb in sbs}
        if len(deltas) > 1:
            raise Unknown("a local's address is taken and the locals do not move together")
    # dataflow
    acc_o, acc_m = [], []
    rd_o, ac_o = reaching(o, slots_tracked, acc_o)
    rd_m, ac_m = reaching(m, slots_tracked, acc_m)

    for k in range(len(o.insns)):
        if rd_o[k] is None or rd_m[k] is None:
            if (rd_o[k] is None) != (rd_m[k] is None):
                raise Diff("#%d reachable on one side only" % k)
            continue
        uo = {key: loc for key, loc, kind in ac_o[k] if kind == "u"}
        um = {key: loc for key, loc, kind in ac_m[k] if kind == "u"}
        if set(uo) != set(um):
            raise Diff("#%d %s %s: different reads" % (k, o.insns[k].mnemonic, o.insns[k].op_str))
        for key, la in uo.items():
            lb = um[key]
            da, db = rd_o[k].get(la, {"entry"}), rd_m[k].get(lb, {"entry"})
            if {x for x in da if x != "entry"} != {x for x in db if x != "entry"}:
                raise Diff("#%d %s %s | %s %s: reads %s from different instructions" % (
                    k, o.insns[k].mnemonic, o.insns[k].op_str, m.insns[k].mnemonic, m.insns[k].op_str, _lname(la)))
            if ("entry" in da) != ("entry" in db):
                raise Diff("#%d: %s may be the entry value on one side only" % (k, _lname(la)))
            if "entry" in da and la != lb:
                save = o.insns[k].mnemonic == "push" and la in CALLEE_SAVED and lb in CALLEE_SAVED
                if not save:
                    raise Diff("#%d: reads entry %s vs entry %s" % (k, _lname(la), _lname(lb)))
    for side, rd, acs, name in ((o, rd_o, ac_o, "original"), (m, rd_m, ac_m, "ours")):
        for k, i in enumerate(side.insns):
            if i.mnemonic in ("ret", "retn") and rd[k] is not None:
                for r in CALLEE_SAVED:
                    if not _restored(side, rd, acs, k, r):
                        raise Unknown("%s: %s not restored at #%d" % (name, r, k))
    return True


def _lname(loc):
    return "stack[%d]" % loc[1] if isinstance(loc, tuple) else loc


def _restored(side, rd, acs, k, r):
    """At instruction k, does register r hold its entry value (never written, or popped from its own push)?"""
    ds = rd[k].get(r)
    if not ds:
        return True
    for dd in ds:
        if dd == "entry":
            continue                       # untouched on this path
        dk, key = dd
        i = side.insns[dk]
        if i.mnemonic != "pop":
            return False
        cell = ("s", -side.depth[dk])
        sds = rd[dk].get(cell)
        if not sds or len(sds) != 1 or "entry" in sds:
            return False
        (pk, _pkey), = sds
        p = side.insns[pk]
        if p.mnemonic != "push" or not p.operands or p.operands[0].type != X.X86_OP_REG or FAM.get(p.reg_name(p.operands[0].reg)) != r:
            return False
        if rd[pk].get(r):
            return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter"); ap.add_argument("--va", nargs="*"); ap.add_argument("--json"); ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    t0 = time.time()
    import sweep_nonmatching as SW
    import chk
    img = R.Image(); rs = R.Resolver(img, R.SymbolDB(img))
    rows = [r for r in SW.rows() if r["src"]]
    if a.va:
        want = {v.lower().zfill(8) for v in a.va}
        rows = [r for r in rows if r["va"] in want]
    if a.filter:
        rows = [r for r in rows if a.filter in r["src"] or a.filter in r["va"]]
    fc = SW.fn_cache()
    out, counts = [], defaultdict(int)
    for r in rows:
        va = int(r["va"], 16)
        src = os.path.join(ROOT, r["src"])
        prev = fc.get(r["va"], {})
        flags = (prev.get("flags") or "/O2 /MT /Gy /TP").split()
        res = dict(va=r["va"], src=r["src"])
        try:
            obj = SW.cached_obj(src, flags) or SL.compile_obj(src, flags)[0]
            if not obj:
                raise Unknown("no object")
            c = coff.Coff(obj)
            sym = prev.get("symbol")
            if not sym or sym not in c.by_name:
                cands = chk.marker_syms(src, va, list(c.functions()))
                if not cands:
                    raise Unknown("no symbol for the marker")
                sym = cands[0]
            s = c.by_name[sym]
            sec, f0, f1 = c.func_extent(s)
            code = bytes(sec.data[f0:f1])
            lo, hi = img.func_extent(va)
            resol = rs.resolve(c, s, va, R.source_annotations([src]))
            by_off, sym_at = {}, {}
            for off, si, t in sec.relocs:
                if f0 <= off < f1:
                    add = struct.unpack_from("<i", sec.data, off)[0]
                    sym_at[off - f0] = (si, add)
                    tv = resol.addr.get(si)
                    by_off[off - f0] = None if tv is None else tv + add   # symbol + addend (REL32 and DIR32 alike)
            o = Side(disasm(bytes(img.read(lo, hi - lo)).rstrip(b"\xcc"), lo),
                     call_target=lambda i: i.operands[0].imm, img=img)
            mi = Side(disasm(code, 0), call_target=lambda i: by_off.get(i.address + i.size - 4), img=img)
            mi.reloc_offs = set(by_off)
            o.reloc_offs = set()

            def target(at):
                return by_off.get(at)

            def const(at, size):
                """A read-only constant of ours with no original address: ('orig', va, size) when its bytes equal the
                original's at that va -- the caller passes the original's operand va and compares."""
                si, add = sym_at.get(at, (None, 0))
                ts = c.syms.get(si)
                if ts is None or not ts.defined:
                    return None
                tsec = c.sections[ts.secno - 1]
                if tsec.is_code or tsec.writable:
                    return None
                mine = bytes(tsec.data[ts.value + add: ts.value + add + size])
                return ("bytes", mine)
            # resolve ours' constant bytes against the original operand inside compare: wrap
            pairs = {}

            def const_cmp(at, size):
                v = const(at, size)
                if v is None:
                    return None
                return v
            # compare() expects ('orig', va, size) equality: emulate by checking bytes here per operand
            k_of = {i.address: k for k, i in enumerate(mi.insns)}

            def ours_const(at, size):
                v = const(at, size)
                if v is None:
                    return None
                ins_k = max(kk for addr, kk in k_of.items() if addr <= at)
                oi = o.insns[ins_k] if ins_k < len(o.insns) else None
                if oi is None:
                    return None
                for op in oi.operands:
                    if op.type == X.X86_OP_MEM and not op.mem.base and not op.mem.index:
                        if bytes(img.read(op.mem.disp & 0xffffffff, size)) == v[1]:
                            return ("orig", op.mem.disp & 0xffffffff, size)
                return ("bytes", v[1])
            compare(o, mi, target, ours_const)
            res.update(verdict="SAME")
        except Diff as e:
            res.update(verdict="DIFF", why=str(e)[:200])
        except Unknown as e:
            res.update(verdict="UNKNOWN", why=str(e)[:200])
        except Exception as e:
            res.update(verdict="UNKNOWN", why="error: %s: %s" % (type(e).__name__, str(e)[:150]))
        counts[res["verdict"]] += 1
        out.append(res)
        if a.v or res["verdict"] == "SAME":
            print("%s %-8s %s %s" % (r["va"], res["verdict"], r["src"].split("/")[-1], res.get("why", "")))
    if a.json:
        json.dump(out, open(a.json, "w"), indent=1)
    print("%d rows in %.1f s: %s" % (len(out), time.time() - t0, dict(counts)))


if __name__ == "__main__":
    main()
