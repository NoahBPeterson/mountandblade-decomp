"""x87 lifter: a 32-bit function's x87 stack code as SSE2-style scalar-double register code, for the equivalence
layers (tools/equiv/LAYERS.md). STOKE's x86-64 semantics have no x87, so ddec_layer.py lowers each x87 instruction
through this module; the result is a pure function of the code (lift(code, base)) with every lifted operation keyed
by the offset of the original instruction, so another layer (angr) can interpret the same operations later.

What it does
  1. Decodes the x87 instructions by their opcode bytes (D8-DF, Intel SDM vol. 2 opcode maps; register forms of
     fsub/fsubr and fdiv/fdivr are taken from the bytes, never from a disassembler's mnemonic, because AT&T tools
     swap their names).
  2. Tracks the x87 stack depth statically per instruction. The function entry has depth 0 (ABI). A call ends a
     region; its continuation starts a new region whose depth is the smallest of 0, 1, 2 under which the region
     never underflows (a callee that returns a float leaves 1: the consumer of st(0) shows it; __ftol2 and the _CI*
     intrinsics take arguments on the x87 stack, so the depth at a call is not forced to 0), a region with a ret
     must agree with the function's return depth when that is known. The depth must agree at every join; anything
     else is rejected.
  3. Maps stack position p (0 = the bottom of the stack, st(i) = position depth-1-i) to a virtual register x<p>
     (x0..x7). fxch and fstp st(i) are renamings inside a basic block (no code); every block ends with the identity
     mapping restored by register moves ("move" ops) so that blocks join with the same mapping.
  4. Lowers each instruction to ops (below): loads/stores with float/double/int conversions, add/sub/mul/div,
     sqrt, sign operations, constants, comparisons into EFLAGS (fcomi) or into the FPU condition codes (fcom),
     and fnstsw ax.

Ops (tuples; x = virtual register number, 0..7 stack positions, 8/9 scratch; mem = Mem):
  ("load", x, mem, kind)     x := value of mem; kind f32 (float, exact), f64 (double), i16/i32/i64 (fild)
  ("store", x, mem, kind)    mem := x converted; kind f32 (rounded to float), f64, i32 (fistp, current rounding)
  ("copy", d, s)             d := s (the value; the destination's other lane, if any, is left alone)
  ("move", d, s)             d := s (whole register; used for the block-end permutation)
  ("const", x, bits)         x := the double with these 64 bits (fld1, fldz)
  ("arith", op, d, s)        d := d op s, op in add sub mul div (commutative: add mul)
  ("neg", x) ("abs", x) ("sqrt", x)
  ("cmp", a, b, "eflags")    ZF PF CF := a ? b exactly as fcomi/ucomisd (unordered: all 1), OF SF AF := 0
  ("cmp", a, b, "sw")        FPU condition codes C0 C2 C3 := a ? b (fcom: C0 = less, C3 = equal, all = unordered)
  ("fnstsw", top)            ax := status word: AH = C3<<6 | TOP<<3 | C2<<2 | C0 (C1 = 0, B = 0), AL = 0
  ("zero", x)                x := 0 (an empty stack slot at an exit: not observable)
  ("clearhi", x)             the register's other lane := 0 (a live slot at an exit)
An exit (call, ret, jump out) gets the identity mapping, "clearhi" for live slots and "zero" for empty ones.

Semantic model, stated exactly. A proof over the lifted code is a proof for the x87 machine when:
  (a) every x87 value the function computes is exactly representable as an IEEE double. With the precision control
      at 24 bits (the game's D3D9 render thread, docs/codegen.md 9.1) or 53 bits (the MSVC CRT default), every
      result is rounded to at most 53 significand bits, so this only fails for values outside the double exponent
      range: overflow to a magnitude above DBL_MAX or below the double denormals that the 15-bit x87 exponent still
      holds. Then spilling to a double slot and reloading is the identity ("store f64" + "load f64"), as the model
      says; spilling to a float is NOT (f32 store/load are conversions the proof cannot remove).
  (b) the precision control, the rounding control and the exception masks are the same at both functions' entries
      and are not changed inside (fldcw is rejected); exceptions are masked (no traps; B = ES = 0 in fnstsw).
  (c) the result of each arithmetic operation, conversion and comparison is a function of its operands only (true
      for x87 under (b)). ddec_layer.py hands these operations to STOKE as uninterpreted functions (add_double, ...),
      so the proof holds for every such function: it does not depend on 24 vs 53 bits of precision or on how ties
      round. The only algebra assumed is that fadd and fmul are commutative and that fcom(a, b) is fcom(b, a) with
      less/greater exchanged (x87 holds bit for bit, NaN payloads included; tools/equiv/ddec/x87/4-x87-lifter.patch).
  (d) the status word's sticky exception flags (AL after fnstsw ax) are never read: the lifter rejects code that
      reads al/ax/eax after fnstsw before overwriting it; at a call or return they are taken as 0 on both sides
      (callees do not read eax, and the status word is not returned).
  (e) x87 registers above the stack top (empty, tag 11) are not observable, nor is TOP except through fnstsw.
  (f) fld/fild/fst/fstp/fist/fistp/fxch/fld1/fldz between a compare and fnstsw ax leave C0 C2 C3 alone (the SDM
      says undefined; every x86 implementation keeps them, and cl 13.10's own code relies on it: fcomp m;
      fstp st(0); fnstsw ax). C1 is taken as 0 (no stack fault); cl's masks (test ah,5/0x41/0x44/0x45) do not
      read it.
This is the model angr's VEX x87 lifting already makes for (a) (64-bit doubles), minus its fixed rounding.

Rejected (Reject, the layer says unknown): fsin fcos fsincos fptan fpatan fyl2x fyl2xp1 f2xm1 fscale fprem fprem1
fxtract frndint fxam, 80-bit loads/stores (fld/fstp tbyte), fbld/fbstp, fldcw/fnstcw/fldenv/fnstenv/fsave/frstor/
fninit/fnclex, ffree/fincstp/fdecstp, fcmov*, fisttp, fistp m16/m64, fldpi/fldl2e/fldl2t/fldlg2/fldln2 (not
doubles), fnstsw to memory, sahf, fnstsw without a compare before it in its block (only data movement between),
a stack under- or overflow, a depth
that differs at a join, a function that also uses xmm registers, flags live across an fcom (its lowering writes
EFLAGS where the x87 instruction does not) and flags other than ZF/PF/CF read after fcomi.
"""
import re
from dataclasses import dataclass, field

import capstone
from capstone import x86_const as X

_MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_MD.detail = True

ONE = 0x3FF0000000000000
NREG = 8


class Reject(Exception):
    pass


@dataclass(frozen=True)
class Mem:
    """A memory operand of the original instruction (32-bit addressing)."""
    seg: str
    base: str
    index: str
    scale: int
    disp: int

    def intel(self, size):
        s = {2: "word", 4: "dword", 8: "qword", 10: "tbyte"}.get(size, "?")
        parts = [p for p in (self.base, ("%s*%d" % (self.index, self.scale)) if self.index else "") if p]
        if self.disp or not parts:
            parts.append(hex(self.disp & 0xFFFFFFFF))
        return "%s ptr %s[%s]" % (s, (self.seg + ":") if self.seg else "", " + ".join(parts))


@dataclass
class Lifted:
    """Result of lift(): per original instruction offset (from base) its ops, plus the block-end moves."""
    base: int
    ops: dict = field(default_factory=dict)       # offset -> [op]       (x87 instructions)
    pre: dict = field(default_factory=dict)       # offset -> [op]       before a terminator / exit
    post: dict = field(default_factory=dict)      # offset -> [op]       after the last instruction of a block
    depth: dict = field(default_factory=dict)     # offset -> depth before the instruction
    roots: dict = field(default_factory=dict)     # region entry offset -> depth (0 = the function entry)
    ret_depth: int = 0                            # depth at every ret (1: the function returns st(0))
    x87: list = field(default_factory=list)       # offsets of the x87 instructions
    signature: list = field(default_factory=list)  # x87 operations in address order (name + memory size)

    @property
    def any(self):
        return bool(self.x87)


def is_x87(i):
    b = i.bytes
    k = 0
    while k < len(b) and b[k] in (0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x66, 0x67, 0xF2, 0xF3):
        k += 1
    return k < len(b) and 0xD8 <= b[k] <= 0xDF


def _mem(i):
    for o in i.operands:
        if o.type == X.X86_OP_MEM:
            m = o.mem
            return Mem(i.reg_name(m.segment) if m.segment else "", i.reg_name(m.base) if m.base else "",
                       i.reg_name(m.index) if m.index else "", m.scale, m.disp)
    raise Reject("no memory operand in %s %s" % (i.mnemonic, i.op_str))


_ARITH = ["add", "mul", "com", "comp", "sub", "subr", "div", "divr"]


def decode(i):
    """-> a small description of one x87 instruction: (what, args...). Raises Reject for unsupported ones."""
    b = i.bytes
    k = 0
    while b[k] in (0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x66, 0x67, 0xF2, 0xF3):
        if b[k] == 0x66:
            raise Reject("operand-size prefix on an x87 instruction")
        k += 1
    op, modrm = b[k], b[k + 1]
    mod, reg, rm = modrm >> 6, (modrm >> 3) & 7, modrm & 7
    nm = "%s %s" % (i.mnemonic, i.op_str)
    if mod != 3:
        mem = _mem(i)
        if op in (0xD8, 0xDC, 0xDA, 0xDE):
            kind = {0xD8: "f32", 0xDC: "f64", 0xDA: "i32", 0xDE: "i16"}[op]
            return ("marith", _ARITH[reg], mem, kind)
        if op == 0xD9:
            if reg == 0:
                return ("mload", mem, "f32")
            if reg in (2, 3):
                return ("mstore", mem, "f32", reg == 3)
        if op == 0xDD:
            if reg == 0:
                return ("mload", mem, "f64")
            if reg in (2, 3):
                return ("mstore", mem, "f64", reg == 3)
        if op == 0xDB:
            if reg == 0:
                return ("mload", mem, "i32")
            if reg in (2, 3):
                return ("mstore", mem, "i32", reg == 3)
        if op == 0xDF:
            if reg == 0:
                return ("mload", mem, "i16")
            if reg == 5:
                return ("mload", mem, "i64")
        raise Reject("unsupported x87 memory instruction %s" % nm)
    if op == 0xD8:                                  # st(0) := st(0) op st(i); fcom/fcomp st(i)
        return ("rarith0", _ARITH[reg], rm)
    if op == 0xDC:                                  # st(i) := st(i) op st(0) (Intel names: E0 fsubr, E8 fsub ...)
        if reg in (2, 3):
            return ("rarith0", _ARITH[reg], rm)     # DC D0/D8 are aliases of fcom/fcomp
        return ("rarithi", {0: "add", 1: "mul", 4: "subr", 5: "sub", 6: "divr", 7: "div"}[reg], rm, False)
    if op == 0xDE:
        if reg == 3 and rm == 1:
            return ("fcompp",)
        if reg in (0, 1, 4, 5, 6, 7):
            return ("rarithi", {0: "add", 1: "mul", 4: "subr", 5: "sub", 6: "divr", 7: "div"}[reg], rm, True)
        raise Reject("unsupported x87 instruction %s" % nm)
    if op == 0xD9:
        if reg == 0:
            return ("fldst", rm)
        if reg == 1:
            return ("fxch", rm)
        if modrm == 0xD0:
            return ("nop",)
        simple = {0xE0: "fchs", 0xE1: "fabs", 0xE4: "ftst", 0xE8: "fld1", 0xEE: "fldz", 0xFA: "fsqrt"}
        if modrm in simple:
            return (simple[modrm],)
        raise Reject("unsupported x87 instruction %s" % nm)
    if op == 0xDD:
        if reg == 2:
            return ("fstst", rm, False)
        if reg == 3:
            return ("fstst", rm, True)
        if reg in (4, 5):
            return ("rarith0", "com" if reg == 4 else "comp", rm)      # fucom/fucomp
        raise Reject("unsupported x87 instruction %s" % nm)
    if op == 0xDA and modrm == 0xE9:
        return ("fcompp",)                                             # fucompp
    if op == 0xDB and reg in (5, 6):
        return ("fcomi", rm, False)                                    # fucomi / fcomi
    if op == 0xDF and reg in (5, 6):
        return ("fcomi", rm, True)                                     # fucomip / fcomip
    if op == 0xDF and modrm == 0xE0:
        return ("fnstsw",)
    raise Reject("unsupported x87 instruction %s" % nm)


def _effect(d):
    """(reads: how many slots must be on the stack, delta) for a decoded instruction."""
    w = d[0]
    if w == "marith":
        return (1, -1 if d[1] == "comp" else 0)
    if w == "mload":
        return (0, 1)
    if w == "mstore":
        return (1, -1 if d[3] else 0)
    if w == "rarith0":
        return (max(1, d[2] + 1), -1 if d[1] == "comp" else 0)
    if w == "rarithi":
        return (max(1, d[2] + 1), -1 if d[3] else 0)
    if w == "fcompp":
        return (2, -2)
    if w == "fldst":
        return (d[1] + 1, 1)
    if w == "fxch":
        return (max(1, d[1] + 1), 0)
    if w == "fstst":
        return (max(1, d[1] + 1), -1 if d[2] else 0)
    if w == "fcomi":
        return (max(1, d[1] + 1), -1 if d[2] else 0)
    if w in ("fchs", "fabs", "ftst", "fsqrt"):
        return (1, 0)
    if w in ("fld1", "fldz"):
        return (0, 1)
    return (0, 0)


def signature_of(i):
    """The x87 operation as the angr layer's x87_signature names it (mnemonic + memory size)."""
    sz = re.search(r"(byte|word|dword|qword|tbyte|xword) ptr", i.op_str)
    return i.mnemonic + (":" + sz.group(1) if sz else "")


# ------------------------------------------------------------------------------------------------- control flow
def _kind(i, base, end):
    m = i.mnemonic
    if m == "call":
        return "call"
    if m in ("ret", "retn"):
        return "ret"
    if m == "jmp":
        op = i.operands[0]
        if op.type == X.X86_OP_IMM and base <= op.imm < end:
            return "jmp"
        return "exit"
    if i.group(capstone.CS_GRP_JUMP):
        return "jcc"
    if m in ("int3", "hlt", "ud2", "int"):
        return "trap"
    return "seq"


class _Code:
    def __init__(self, code, base):
        self.base, self.end = base, base + len(code)
        self.ins = {i.address: i for i in _MD.disasm(code, base)}
        self.order = sorted(self.ins)
        self.kind = {a: _kind(i, base, self.end) for a, i in self.ins.items()}

    def succ(self, a):
        i, k = self.ins[a], self.kind[a]
        if k in ("call", "ret", "exit", "trap"):
            return []
        if k == "jmp":
            return [i.operands[0].imm]
        out = [a + i.size]
        if k == "jcc":
            out.append(i.operands[0].imm)
        return [s for s in out if s in self.ins]

    def leaders(self, roots):
        out = set(roots)
        for a in self.order:
            k = self.kind[a]
            if k in ("jmp", "jcc"):
                t = self.ins[a].operands[0].imm
                if t in self.ins:
                    out.add(t)
            if k in ("jcc", "call"):
                n = a + self.ins[a].size
                if n in self.ins:
                    out.add(n)
        return out


# ------------------------------------------------------------------------------------------------- flags liveness
_FL = {"C": ("CF",), "P": ("PF",), "A": ("AF",), "Z": ("ZF",), "S": ("SF",), "O": ("OF",)}


def _bits(names):
    out = 0
    for n in names:
        out |= getattr(X, n, 0)
    return out


_TEST = {f: _bits(["X86_EFLAGS_TEST_" + f + "F"]) for f in "CPAZSO"}
_WRITE = {f: _bits(["X86_EFLAGS_%s_%sF" % (w, f) for w in ("MODIFY", "RESET", "SET", "UNDEFINED")])
          for f in "CPAZSO"}


def _flag_rw(i, x87d):
    """-> (read set, write set) of status flags. x87 instructions: fcomi writes ZF PF CF, the rest none."""
    if x87d is not None:
        return set(), ({"Z", "P", "C"} if x87d[0] == "fcomi" else set())
    ef = getattr(i, "eflags", 0) or 0
    r = {f for f in "CPAZSO" if ef & _TEST[f]}
    w = {f for f in "CPAZSO" if ef & _WRITE[f]}
    if i.mnemonic in ("pushf", "pushfd", "lahf"):
        r |= set("CPAZSO")
    return r, w


def _flags_live(c, dec):
    """-> {address: flags live after the instruction}. Exits (calls, rets, jumps out) have none live (callees
    do not read the caller's flags; ddec_layer.py makes the same assumption)."""
    rw = {a: _flag_rw(c.ins[a], dec.get(a)) for a in c.order}
    live_in = {a: set() for a in c.order}
    changed = True
    while changed:
        changed = False
        for a in reversed(c.order):
            out = set()
            for s in c.succ(a):
                out |= live_in.get(s, set())
            r, w = rw[a]
            new = r | (out - w)
            if new != live_in[a]:
                live_in[a], changed = new, True
    after = {}
    for a in c.order:
        out = set()
        for s in c.succ(a):
            out |= live_in.get(s, set())
        after[a] = out
    return after


_AL_REGS = {"al", "ax", "eax"}


def _reads_al(i):
    try:
        r, w = i.regs_access()
    except capstone.CsError:
        return True, False
    rn = {i.reg_name(x) for x in r}
    wn = {i.reg_name(x) for x in w}
    if i.mnemonic in ("xor", "sub") and len(i.operands) == 2 and all(o.type == X.X86_OP_REG for o in i.operands) \
            and i.operands[0].reg == i.operands[1].reg:
        rn = set()                                   # zero idiom
    full_write = bool(wn & {"eax", "al", "ax"}) and i.mnemonic not in ("xchg", "cmpxchg")
    return bool(rn & _AL_REGS), full_write


def _check_al_dead(c, start):
    """The sticky exception flags fnstsw ax puts in AL must not be read by the function itself (model (d))."""
    seen, todo = set(), list(c.succ(start))
    while todo:
        a = todo.pop()
        if a in seen:
            continue
        seen.add(a)
        i = c.ins[a]
        if c.kind[a] in ("call", "ret", "exit"):
            continue
        rd, wr = _reads_al(i)
        if rd:
            raise Reject("al is read after fnstsw ax at +%#x (%s %s)" % (a - c.base, i.mnemonic, i.op_str))
        if wr:
            continue
        todo.extend(c.succ(a))


# ------------------------------------------------------------------------------------------------- depth analysis
def _region(c, root, d0, dec, fixed):
    """Propagate the depth from root through its region (stopping at exits). -> ({address: depth}, ret depths,
    exits {address: depth}) or raise Reject."""
    dep, todo, rets = {}, [(root, d0)], set()
    while todo:
        a, d = todo.pop()
        if a in dep:
            if dep[a] != d:
                raise Reject("x87 stack depth %d or %d at +%#x (paths disagree)" % (dep[a], d, a - c.base))
            continue
        if a in fixed and fixed[a] != d:
            raise Reject("x87 stack depth %d or %d at +%#x (regions disagree)" % (fixed[a], d, a - c.base))
        dep[a] = d
        k = c.kind[a]
        if k == "trap":
            continue
        if a in dec:
            need, delta = _effect(dec[a])
            if d < need:
                raise Reject("x87 stack underflow at +%#x" % (a - c.base))
            d2 = d + delta
            if d2 > NREG:
                raise Reject("x87 stack overflow at +%#x" % (a - c.base))
        else:
            d2 = d
        if k == "ret":
            rets.add(d)
        for s in c.succ(a):
            todo.append((s, d2))
    return dep, rets


def lift(code, base, ret_depth=None, call_post=None):
    """-> Lifted. ret_depth: the function's return depth when known (1 for a float/double return, from its
    decorated name); call_post: {call address: depth after it} when known. Raises Reject."""
    c = _Code(code, base)
    dec, sig = {}, []
    for a in c.order:
        i = c.ins[a]
        if is_x87(i):
            dec[a] = decode(i)
            sig.append(signature_of(i))
        elif i.mnemonic in ("fwait", "wait"):
            dec[a] = ("nop",)
    L = Lifted(base)
    L.signature = sig
    L.x87 = [a - base for a in c.order if a in dec and dec[a][0] != "nop"]
    if not L.x87:
        return L
    for i in c.ins.values():
        if re.search(r"\bx?mm\d", i.op_str):
            raise Reject("the function uses xmm/mmx registers as well as x87")
        if i.mnemonic == "sahf":
            raise Reject("sahf is not supported")
    # regions: the entry, then each call's continuation in address order
    calls = [a for a in c.order if c.kind[a] == "call"]
    fixed = {}
    dep0, rets0 = _region(c, base, 0, dec, fixed)
    fixed.update(dep0)
    R = ret_depth
    if rets0:
        if len(rets0) > 1:
            raise Reject("the function returns with x87 depths %s" % sorted(rets0))
        r = next(iter(rets0))
        if R is not None and R != r:
            raise Reject("the function returns with x87 depth %d, its type says %d" % (r, R))
        R = r
    roots = {base: 0}
    pending = []
    for a in calls:
        n = a + c.ins[a].size
        if n not in c.ins or c.kind[n] == "trap":
            continue
        pending.append((a, n))
    # first the continuations whose depth follows from the code, then the ambiguous ones (with R known by then)
    for rnd in (0, 1):
        left = []
        for a, n in pending:
            if n in roots:
                continue
            if a not in fixed:
                continue                        # unreachable call
            pre = fixed[a]
            opts = [call_post[a]] if call_post and a in call_post else [0, 1, 2]
            ok = []
            for d in opts:
                try:
                    dp, rs = _region(c, n, d, dec, fixed)
                except Reject:
                    continue
                if len(rs) > 1:
                    continue
                if rs and R is not None and next(iter(rs)) != R:
                    continue
                ok.append((d, dp, rs))
            if not ok:
                raise Reject("no x87 depth after the call at +%#x fits (depth %d before it)" % (a - base, pre))
            amb = len(ok) > 1 and any(o[2] for o in ok) and R is None
            if amb and rnd == 0:
                left.append((a, n))
                continue
            d, dp, rs = ok[0]                       # the smallest depth that fits
            if rs:
                R = next(iter(rs))
            fixed.update(dp)
            roots[n] = d
        pending = left
        if not pending:
            break
    L.roots = {a - base: d for a, d in roots.items()}
    L.ret_depth = R or 0
    if L.ret_depth > 1:
        raise Reject("the function returns with x87 depth %d" % L.ret_depth)
    L.depth = {a - base: d for a, d in fixed.items()}
    # checks that need the whole function
    live = _flags_live(c, dec)
    for a, d in dec.items():
        if a not in fixed:
            continue
        w = d[0]
        sw_cmp = (w == "marith" and d[1] in ("com", "comp")) or (w == "rarith0" and d[1] in ("com", "comp")) or \
            w in ("fcompp", "ftst")
        if sw_cmp and live[a]:
            raise Reject("status flags %s are live across %s at +%#x" % ("".join(sorted(live[a])),
                                                                         c.ins[a].mnemonic, a - base))
        if w == "fcomi" and live[a] - {"Z", "P", "C"}:
            raise Reject("flags %s read after fcomi at +%#x" % ("".join(sorted(live[a] - {"Z", "P", "C"})),
                                                                a - base))
        if w == "fnstsw":
            _check_al_dead(c, a)
    _lower(c, dec, fixed, roots, L)
    return L


# ------------------------------------------------------------------------------------------------- lowering
_REV = {"subr": "sub", "divr": "div"}


def _lower(c, dec, depth, roots, L):
    leaders = c.leaders(roots)
    blocks = _blocks(c, depth, leaders)
    # condition codes pending from a compare reach fnstsw across jumps too (cl merges the fnstsw ax; test ah
    # tails of several compares): a must-analysis over the blocks; a segment entry has none
    first = {b[0]: n for n, b in enumerate(blocks)}
    preds = {n: [] for n in range(len(blocks))}
    for n, b in enumerate(blocks):
        for s in c.succ(b[-1]):
            if s in first:
                preds[first[s]].append(n)
    pin = {n: (blocks[n][0] not in roots and bool(preds[n])) for n in range(len(blocks))}
    pout = {}
    changed = True
    while changed:
        changed = False
        for n, b in enumerate(blocks):
            p = pin[n] and all(pout.get(q, True) for q in preds[n])
            perm = list(range(depth[b[0]]))
            for a in b:
                if a in dec and dec[a][0] != "fnstsw":
                    _, perm, p = _one(dec[a], perm, p, depth[a], a - c.base)
                elif a in dec:
                    p = False
            if pout.get(n) != p:
                pout[n], changed = p, True
    for n, blk in enumerate(blocks):
        _lower_block(c, dec, depth, L, blk, pin[n] and all(pout[q] for q in preds[n]))


def _blocks(c, depth, leaders):
    blocks, cur = [], None
    for a in c.order:
        if a not in depth:
            cur = None
            continue
        if cur is None or a in leaders:
            cur = []
            blocks.append(cur)
        cur.append(a)
        if c.kind[a] in ("jmp", "jcc", "call", "ret", "exit", "trap"):
            cur = None
    return blocks


def _lower_block(c, dec, depth, L, blk, pend):
    d = depth[blk[0]]
    perm = list(range(d))                    # position -> register
    for a in blk:
        i = c.ins[a]
        k = c.kind[a]
        off = a - c.base
        last = a == blk[-1]
        if len(perm) != depth[a]:
            raise Reject("internal: depth %d, mapping of %d at +%#x" % (depth[a], len(perm), off))
        if a in dec:
            ops, perm, pend = _one(dec[a], perm, pend, depth[a], off)
            if ops:
                L.ops[off] = ops
        # integer instructions keep the pending condition codes (the lowering holds them in its own register)
        if k in ("call", "ret", "exit"):
            n = len(perm)
            if k == "ret":
                n = L.ret_depth
                if len(perm) != n:
                    raise Reject("ret at +%#x with x87 depth %d" % (off, len(perm)))
            fix = _normalise(perm)
            fix += [("clearhi", p) for p in range(n)] + [("zero", p) for p in range(n, NREG)]
            L.pre[off] = fix
        elif k in ("jmp", "jcc"):
            fix = _normalise(perm)
            if fix:
                L.pre[off] = fix
        elif last and k != "trap":
            fix = _normalise(perm)
            if fix:
                L.post[off] = fix
        if k in ("call", "ret", "exit", "jmp", "jcc"):
            perm = list(range(len(perm)))


def _normalise(perm):
    """moves that bring position p into register p (a parallel move; register 9 breaks cycles)."""
    want = {p: r for p, r in enumerate(perm) if r != p}     # position p (register p) must receive register r
    out = []
    while want:
        srcs = set(want.values())
        free = [p for p in want if p not in srcs]            # destinations nobody still needs to read
        if free:
            p = free[0]
            out.append(("move", p, want.pop(p)))
            continue
        p = next(iter(want))                                  # a cycle: save p, then shift
        out.append(("move", 9, p))
        for q, r in list(want.items()):
            if r == p:
                want[q] = 9
    return out


def _free(perm):
    for r in range(NREG):
        if r not in perm:
            return r
    raise Reject("x87 stack overflow")


# data movement between a compare and fnstsw (cl 13.10: fcomp m; fstp st(0); fnstsw ax / fcomp m; fld m; fnstsw ax):
# the SDM calls C0 C2 C3 undefined after them, every x86 implementation leaves them alone, and the original code
# relies on it (model (f))
_KEEPS_CC = ("mload", "mstore", "fldst", "fxch", "fstst", "fld1", "fldz")


def _one(d, perm, pend, depth, off):
    """ops of one instruction; -> (ops, new perm, pending condition codes)."""
    w = d[0]
    perm = list(perm)
    ops = []
    st = lambda i: perm[len(perm) - 1 - i]          # noqa: E731

    def pop():
        perm.pop()

    def push():
        r = _free(perm)
        perm.append(r)
        return r

    if w == "nop":
        return [], perm, pend
    if w == "fxch":                                  # a renaming, no code
        i = d[1]
        a, b = len(perm) - 1, len(perm) - 1 - i
        perm[a], perm[b] = perm[b], perm[a]
        return [], perm, pend
    if w == "fnstsw":
        if not pend:
            raise Reject("fnstsw ax at +%#x without a compare right before it" % off)
        return [("fnstsw", (NREG - len(perm)) & 7)], perm, False
    if w == "mload":
        r = push()
        ops.append(("load", r, d[1], d[2]))
    elif w == "mstore":
        if d[2] == "i32" or d[2] in ("f32", "f64"):
            ops.append(("store", st(0), d[1], d[2]))
        else:
            raise Reject("x87 store kind %s" % d[2])
        if d[3]:
            pop()
    elif w == "marith":
        op, mem, kind = d[1], d[2], d[3]
        ops.append(("load", 8, mem, kind))
        if op in ("com", "comp"):
            ops.append(("cmp", st(0), 8, "sw"))
            if op == "comp":
                pop()
            return ops, perm, True
        if op in _REV:                               # st0 := m op st0
            ops += [("arith", _REV[op], 8, st(0)), ("copy", st(0), 8)]
        else:
            ops.append(("arith", op, st(0), 8))
    elif w == "rarith0":                             # st0 := st0 op sti; fcom(p) st(i)
        op, i = d[1], d[2]
        if op in ("com", "comp"):
            ops.append(("cmp", st(0), st(i), "sw"))
            if op == "comp":
                pop()
            return ops, perm, True
        if op in _REV:                               # st0 := sti op st0
            ops += [("copy", 8, st(i)), ("arith", _REV[op], 8, st(0)), ("copy", st(0), 8)]
        else:
            ops.append(("arith", op, st(0), st(i)))
    elif w == "rarithi":                             # sti := sti op st0 (op as Intel names it); pop
        op, i, p = d[1], d[2], d[3]
        if op in _REV:                               # sti := st0 op sti
            if p:                                    # compute into st0's register and rename it to st(i)
                r0, ri = st(0), st(i)
                ops.append(("arith", _REV[op], r0, ri))
                perm[len(perm) - 1 - i] = r0
                perm.pop()
                return ops, perm, False
            ops += [("copy", 8, st(0)), ("arith", _REV[op], 8, st(i)), ("copy", st(i), 8)]
        else:
            ops.append(("arith", op, st(i), st(0)))
        if p:
            pop()
    elif w == "fcompp":
        ops.append(("cmp", st(0), st(1), "sw"))
        pop()
        pop()
        return ops, perm, True
    elif w == "fldst":
        src = st(d[1])
        r = push()
        ops.append(("copy", r, src))
    elif w == "fstst":
        i, p = d[1], d[2]
        if p:
            if i:
                perm[len(perm) - 1 - i] = perm[-1]
            pop()
        elif i:
            ops.append(("copy", st(i), st(0)))
    elif w == "fcomi":
        ops.append(("cmp", st(0), st(d[1]), "eflags"))
        if d[2]:
            pop()
    elif w == "fchs":
        ops.append(("neg", st(0)))
    elif w == "fabs":
        ops.append(("abs", st(0)))
    elif w == "fsqrt":
        ops.append(("sqrt", st(0)))
    elif w == "ftst":
        ops += [("const", 8, 0), ("cmp", st(0), 8, "sw")]
        return ops, perm, True
    elif w == "fld1":
        ops.append(("const", push(), ONE))
    elif w == "fldz":
        ops.append(("const", push(), 0))
    else:
        raise Reject("x87 %s" % w)
    return ops, perm, pend and w in _KEEPS_CC


def lift_func(f, ret_depth=None, call_post=None):
    """lift() of a tools/equiv/func.Func (its code at its own va)."""
    return lift(f.code, f.va, ret_depth, call_post)
