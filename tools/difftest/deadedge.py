"""Static dead-edge prover for equiv.py (--prove-dead): z3 proves that a never-taken branch edge cannot be taken.

An uncovered edge A->B counts as covered ONLY IF the condition for taking it is unsatisfiable on EVERY path
that reaches A, using nothing but facts the function itself establishes. Anything not modelled makes the edge
UNKNOWN (never proven), and unknown edges stay missed.

Model (symbolic execution over the function's CFG, original and ours each separately):
- the entry state is all fresh symbols: eight 32-bit registers, five flags (cf zf sf of pf) and the whole memory
  as a z3 Array(BV32 -> BV8). Nothing about arguments, globals or the stack is assumed.
- a basic block's output is the exact symbolic effect of its instructions. A block with several predecessors
  takes an if-then-else over the incoming edge predicates, a block's reach predicate is the OR of its incoming
  edge predicates (edge = reach(pred) AND branch condition).
- calls: eax ecx edx and the flags become fresh symbols, ALL of memory becomes a fresh array (the callee may
  write anything), esp becomes esp+k for a fresh k (the callee's stack cleanup is unknown). ebx esi edi ebp are
  ASSUMED preserved (the callee-saved convention of the code VC7.1 generates); a call to a known prolog helper
  (the `clobber_all` hook: __EH_prolog / __SEH_prolog / __chkstk) clobbers every register.
- loops: back edges (edge b->h where h dominates b) are CUT, and the head h gets, per state component x, either
  a fresh symbol or -- only when x is a proven loop invariant -- the value x had on entry. The invariants are
  found Houdini-style and checked inductively: assume "x at the head equals x on entry" for the candidate set,
  and require for every back edge that "reach(b) AND back-edge taken AND out_b.x != head.x" is UNSAT; any
  candidate that fails (SAT or solver timeout) is dropped and the check repeats until nothing is dropped.
  Heads are processed innermost first, with the not-yet-processed (outer) heads fully fresh, so an inner
  invariant never depends on an outer hypothesis that could later be withdrawn. Soundness: every real execution
  prefix to A is a path in the cut graph whose last visit of each loop head carries the real state at that visit;
  by induction on iterations the invariants hold at every visit, so the real run satisfies the formula and a
  UNSAT query rules it out. Irreducible CFGs (a retreating edge whose target does not dominate its source) are
  UNKNOWN.
- memory: byte-precise (Array select/store, aliasing decided by z3). Loads of memory the function has not
  written are the entry array, so they are unconstrained symbols that agree with each other until a store or a
  call. fs:/gs: stores havoc memory, fs:/gs: loads are fresh symbols.
- x87 instructions touch no integer state except `fnstsw ax` / fcomi (fresh ax / flags) and the stores to memory
  (fresh bytes of the store width), so an x87-fed branch condition is an unconstrained symbol: never proven.
- an instruction that is not modelled havocs every register, the flags and memory. Unresolved indirect jumps
  make the whole function UNKNOWN (a block could be entered from a state the model does not see).
- self-check: every edge the harness actually COVERED must be satisfiable in the model; if one is UNSAT the model
  is wrong and no edge of the function is proven.
"""
import time

import capstone
from capstone import x86_const as X

try:
    import z3
except ImportError:             # equiv runs without it; --prove-dead then reports every edge unknown
    z3 = None

FULLS = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")
FLAGS = ("cf", "zf", "sf", "of", "pf")
PART = {}
for _f in ("eax", "ebx", "ecx", "edx"):
    PART[_f] = (_f, 0, 32)
    PART[_f[1] + "x"] = (_f, 0, 16)
    PART[_f[1] + "l"] = (_f, 0, 8)
    PART[_f[1] + "h"] = (_f, 8, 8)
for _f in ("esi", "edi", "ebp", "esp"):
    PART[_f] = (_f, 0, 32)
    PART[_f[1:]] = (_f, 0, 16)

_CC = {"e": "e", "z": "e", "ne": "ne", "nz": "ne", "b": "b", "nae": "b", "c": "b", "ae": "ae", "nb": "ae", "nc": "ae",
       "be": "be", "na": "be", "a": "a", "nbe": "a", "l": "l", "nge": "l", "ge": "ge", "nl": "ge", "le": "le",
       "ng": "le", "g": "g", "nle": "g", "s": "s", "ns": "ns", "p": "p", "pe": "p", "np": "np", "po": "np",
       "o": "o", "no": "no"}
_PREFIXES = ("rep ", "repe ", "repne ", "repz ", "repnz ", "lock ")
_STRING = ("movs", "stos", "lods", "scas", "cmps", "ins", "outs")
# x87 mnemonics that write their memory operand (the operand size is the store width) / that only read it
_X87_STORE = {"fst", "fstp", "fist", "fistp", "fisttp", "fbstp", "fnstcw", "fstcw", "fnstsw", "fstsw"}
_X87_READ = {"fld", "fild", "fbld", "fldcw", "fadd", "fiadd", "fsub", "fisub", "fsubr", "fisubr", "fmul", "fimul",
             "fdiv", "fidiv", "fdivr", "fidivr", "fcom", "fcomp", "ficom", "ficomp", "fucom", "fucomp"}
# integer instructions whose register/flag effects regs_access reports completely and that write memory only
# through an explicit destination operand (handled below)
_GENERIC = {"mul", "div", "idiv", "bt", "bts", "btr", "btc", "bsf", "bsr", "rol", "ror", "rcl", "rcr", "shld", "shrd",
            "bswap", "sahf", "lahf", "cld", "std", "clc", "stc", "cmc", "rdtsc", "cpuid", "xadd", "cmpxchg", "pause",
            "imul"}
_TERM = (X.X86_INS_RET, X.X86_INS_RETF, X.X86_INS_INT3, X.X86_INS_HLT, X.X86_INS_UD2)


class Unsup(Exception):
    pass


# ---------------------------------------------------------------------------------------------- state

class St:
    __slots__ = ("r", "f", "mem")

    def __init__(self, r, f, mem):
        self.r, self.f, self.mem = r, f, mem

    def copy(self):
        return St(dict(self.r), dict(self.f), self.mem)

    def get(self, x):
        return self.mem if x == "mem" else self.r[x] if x in FULLS else self.f[x]

    def put(self, x, v):
        if x == "mem":
            self.mem = v
        elif x in FULLS:
            self.r[x] = v
        else:
            self.f[x] = v


COMPS = FULLS + FLAGS + ("mem",)


def _fresh(tag, kind):
    if kind == "mem":
        return z3.Array(tag, z3.BitVecSort(32), z3.BitVecSort(8))
    if kind == "flag":
        return z3.Bool(tag)
    return z3.BitVec(tag, 32)


def fresh_state(tag):
    return St({r: _fresh("%s_%s" % (tag, r), "reg") for r in FULLS},
              {f: _fresh("%s_%s" % (tag, f), "flag") for f in FLAGS}, _fresh("%s_mem" % tag, "mem"))


def cond_expr(cc, f):
    cf, zf, sf, of, pf = f["cf"], f["zf"], f["sf"], f["of"], f["pf"]
    return {"e": zf, "ne": z3.Not(zf), "b": cf, "ae": z3.Not(cf), "be": z3.Or(cf, zf), "a": z3.And(z3.Not(cf), z3.Not(zf)),
            "l": z3.Xor(sf, of), "ge": z3.Not(z3.Xor(sf, of)), "le": z3.Or(zf, z3.Xor(sf, of)),
            "g": z3.And(z3.Not(zf), z3.Not(z3.Xor(sf, of))), "s": sf, "ns": z3.Not(sf), "p": pf, "np": z3.Not(pf),
            "o": of, "no": z3.Not(of)}[cc]


def jcc_cc(ins):
    mn = ins.mnemonic
    if mn.startswith("j") and mn[1:] in _CC:
        return _CC[mn[1:]]
    return None


def msb(x, w):
    return z3.Extract(w - 1, w - 1, x) == z3.BitVecVal(1, 1)


def parity(r):
    low = z3.Extract(7, 0, r)
    x = low ^ z3.LShR(low, 4)
    x = x ^ z3.LShR(x, 2)
    x = x ^ z3.LShR(x, 1)
    return z3.Extract(0, 0, x) == z3.BitVecVal(0, 1)


# ------------------------------------------------------------------------------------ block executor

class Exec:
    def __init__(self, md, addr, clobber_all):
        self.md, self.addr, self.n, self.clobber_all = md, addr, 0, clobber_all
        self.s = None
        self.ins = None
        self.eas = {}

    def nm(self, role):
        self.n += 1
        return "%s_%x_%d" % (role, self.addr, self.n)

    # -- state access
    def reg_rd(self, name):
        if name not in PART:
            raise Unsup("register " + name)
        f, lo, w = PART[name]
        v = self.s.r[f]
        return v if w == 32 else z3.Extract(lo + w - 1, lo, v)

    def reg_wr(self, name, val):
        f, lo, w = PART[name]
        if w == 32:
            self.s.r[f] = val
            return
        old = self.s.r[f]
        parts = []
        if lo + w < 32:
            parts.append(z3.Extract(31, lo + w, old))
        parts.append(val)
        if lo > 0:
            parts.append(z3.Extract(lo - 1, 0, old))
        self.s.r[f] = z3.Concat(*parts) if len(parts) > 1 else parts[0]

    def load(self, ea, n):
        bs = [z3.Select(self.s.mem, ea + z3.BitVecVal(i, 32)) for i in range(n)]
        return z3.Concat(*reversed(bs)) if n > 1 else bs[0]

    def store(self, ea, val, n):
        m = self.s.mem
        for i in range(n):
            m = z3.Store(m, ea + z3.BitVecVal(i, 32), z3.Extract(8 * i + 7, 8 * i, val))
        self.s.mem = m

    def ea(self, op):
        m = op.mem
        v = z3.BitVecVal(m.disp & 0xFFFFFFFF, 32)
        if m.base:
            v = v + self.reg_rd(self.ins.reg_name(m.base))
        if m.index:
            v = v + self.reg_rd(self.ins.reg_name(m.index)) * z3.BitVecVal(m.scale, 32)
        return v

    def seg(self, op):
        return op.type == X.X86_OP_MEM and op.mem.segment in (X.X86_REG_FS, X.X86_REG_GS)

    def width(self, op):
        if op.type == X.X86_OP_REG:
            nm = self.ins.reg_name(op.reg)
            if nm not in PART:
                raise Unsup("register " + str(nm))
            return PART[nm][2]
        if op.type == X.X86_OP_MEM:
            return op.size * 8
        raise Unsup("operand width")

    def rd(self, i, w=None):
        op = self.ins.operands[i]
        if op.type == X.X86_OP_IMM:
            if w is None:
                raise Unsup("imm width")
            return z3.BitVecVal(op.imm & ((1 << w) - 1), w)
        if op.type == X.X86_OP_REG:
            return self.reg_rd(self.ins.reg_name(op.reg))
        if op.type == X.X86_OP_MEM:
            if self.seg(op):
                return z3.BitVec(self.nm("seg"), op.size * 8)
            ea = self.eas[i] if i in self.eas else self.ea(op)
            return self.load(ea, op.size)
        raise Unsup("operand")

    def wr(self, i, val):
        op = self.ins.operands[i]
        if op.type == X.X86_OP_REG:
            nm = self.ins.reg_name(op.reg)
            if nm not in PART:
                raise Unsup("register " + str(nm))
            self.reg_wr(nm, val)
        elif op.type == X.X86_OP_MEM:
            if self.seg(op):
                self.s.mem = _fresh(self.nm("segmem"), "mem")
                return
            ea = self.eas[i] if i in self.eas else self.ea(op)
            self.store(ea, val, op.size)
        else:
            raise Unsup("write to immediate")

    def fresh_flags(self):
        for f in FLAGS:
            self.s.f[f] = z3.Bool(self.nm("fl_" + f))

    def havoc_all(self):
        for r in FULLS:
            self.s.r[r] = z3.BitVec(self.nm("hv_" + r), 32)
        self.fresh_flags()
        self.s.mem = _fresh(self.nm("hv_mem"), "mem")

    def zsp(self, res, w):
        self.s.f["zf"] = res == z3.BitVecVal(0, w)
        self.s.f["sf"] = msb(res, w)
        self.s.f["pf"] = parity(res)

    # -- instruction
    def step(self, ins, s):
        self.ins, self.s = ins, s
        mn = ins.mnemonic
        for p in _PREFIXES:
            if mn.startswith(p):
                mn = mn[len(p):]
        try:
            self.eas = {}
            for i, op in enumerate(ins.operands):
                if op.type == X.X86_OP_MEM and not self.seg(op):
                    self.eas[i] = self.ea(op)
            self._step(ins, mn)
        except Unsup:
            self.s = s
            self.havoc_all()

    def _step(self, ins, mn):
        ops, s = ins.operands, self.s
        k = len(ops)
        if mn in ("nop", "fnop", "wait", "fwait", "prefetchnta", "prefetcht0", "prefetcht1", "prefetcht2", "emms"):
            return
        if any(o.type == X.X86_OP_REG and (self.ins.reg_name(o.reg) or "") not in PART and not
               (self.ins.reg_name(o.reg) or "").startswith("st") for o in ops) and mn[0] != "f":
            raise Unsup("non-integer register operand")
        if mn == "mov" or mn == "movabs":
            w = self.width(ops[0])
            if ops[0].type == X.X86_OP_REG and self.ins.reg_name(ops[0].reg) not in PART:
                raise Unsup("segment/control reg")
            self.wr(0, self.rd(1, w))
        elif mn in ("movzx", "movsx"):
            wd, ws = self.width(ops[0]), self.width(ops[1])
            v = self.rd(1)
            self.wr(0, (z3.ZeroExt if mn == "movzx" else z3.SignExt)(wd - ws, v))
        elif mn == "lea":
            self.wr(0, self.ea(ops[1]))
        elif mn == "xchg":
            a, b = self.rd(0), self.rd(1)
            self.wr(0, b)
            self.wr(1, a)
        elif mn in ("add", "sub", "cmp", "and", "or", "xor", "test"):
            w = self.width(ops[0])
            a, b = self.rd(0, w), self.rd(1, w)
            if mn == "add":
                r = a + b
                s.f["cf"] = z3.ULT(r, a)
                s.f["of"] = msb(~(a ^ b) & (a ^ r), w)
            elif mn in ("sub", "cmp"):
                r = a - b
                s.f["cf"] = z3.ULT(a, b)
                s.f["of"] = msb((a ^ b) & (a ^ r), w)
            else:
                r = a & b if mn in ("and", "test") else a | b if mn == "or" else a ^ b
                s.f["cf"] = z3.BoolVal(False)
                s.f["of"] = z3.BoolVal(False)
            self.zsp(r, w)
            if mn not in ("cmp", "test"):
                self.wr(0, r)
        elif mn in ("adc", "sbb"):
            w = self.width(ops[0])
            a, b = self.rd(0, w), self.rd(1, w)
            c = z3.If(s.f["cf"], z3.BitVecVal(1, w), z3.BitVecVal(0, w))
            r = a + b + c if mn == "adc" else a - b - c
            self.fresh_flags()
            self.zsp(r, w)
            self.wr(0, r)
        elif mn in ("inc", "dec"):
            w = self.width(ops[0])
            a = self.rd(0)
            one = z3.BitVecVal(1, w)
            r = a + one if mn == "inc" else a - one
            s.f["of"] = r == z3.BitVecVal(1 << (w - 1), w) if mn == "inc" else a == z3.BitVecVal(1 << (w - 1), w)
            self.zsp(r, w)
            self.wr(0, r)
        elif mn == "neg":
            w = self.width(ops[0])
            a = self.rd(0)
            r = -a
            s.f["cf"] = a != z3.BitVecVal(0, w)
            s.f["of"] = a == z3.BitVecVal(1 << (w - 1), w)
            self.zsp(r, w)
            self.wr(0, r)
        elif mn == "not":
            self.wr(0, ~self.rd(0))
        elif mn in ("shl", "sal", "shr", "sar"):
            w = self.width(ops[0])
            a = self.rd(0)
            if k == 1:
                cnt = 1
            elif ops[1].type == X.X86_OP_IMM:
                cnt = ops[1].imm & 31
            else:
                cnt = None
            if cnt is None:
                c = self.rd(1)
                c = z3.ZeroExt(w - 8, c & z3.BitVecVal(31, 8)) if w > 8 else z3.Extract(w - 1, 0, c & z3.BitVecVal(31, 8))
                r = a << c if mn in ("shl", "sal") else z3.LShR(a, c) if mn == "shr" else a >> c
                self.fresh_flags()
                self.wr(0, r)
                return
            if cnt == 0:
                return
            c = z3.BitVecVal(cnt, w)
            r = a << c if mn in ("shl", "sal") else z3.LShR(a, c) if mn == "shr" else a >> c
            self.zsp(r, w)
            bit = None
            if cnt <= w:
                bit = (w - cnt) if mn in ("shl", "sal") else (cnt - 1)
            s.f["cf"] = (z3.Extract(bit, bit, a) == z3.BitVecVal(1, 1)) if bit is not None else z3.Bool(self.nm("shcf"))
            if cnt == 1:
                s.f["of"] = (z3.Xor(msb(r, w), s.f["cf"]) if mn in ("shl", "sal") else
                             msb(a, w) if mn == "shr" else z3.BoolVal(False))
            else:
                s.f["of"] = z3.Bool(self.nm("shof"))
            self.wr(0, r)
        elif mn == "imul" and k >= 2:
            w = self.width(ops[0])
            if k == 2:
                r = self.rd(0) * self.rd(1, w)
            else:
                r = self.rd(1, w) * self.rd(2, w)
            self.fresh_flags()
            self.wr(0, r)
        elif mn == "cdq":
            s.r["edx"] = s.r["eax"] >> 31
        elif mn == "cwde":
            s.r["eax"] = z3.SignExt(16, z3.Extract(15, 0, s.r["eax"]))
        elif mn == "cbw":
            self.reg_wr("ax", z3.SignExt(8, self.reg_rd("al")))
        elif mn == "cwd":
            self.reg_wr("dx", self.reg_rd("ax") >> 15)
        elif mn == "push":
            if self.width_push(ops[0]) != 32:
                raise Unsup("16-bit push")
            v = self.rd(0, 32)
            s.r["esp"] = s.r["esp"] - z3.BitVecVal(4, 32)
            self.store(s.r["esp"], v, 4)
        elif mn == "pop":
            if ops[0].type == X.X86_OP_REG and self.ins.reg_name(ops[0].reg) in ("esp", "sp"):
                raise Unsup("pop esp")
            if self.width(ops[0]) != 32:
                raise Unsup("16-bit pop")
            v = self.load(s.r["esp"], 4)
            s.r["esp"] = s.r["esp"] + z3.BitVecVal(4, 32)
            self.eas = {i: self.ea(op) for i, op in enumerate(ops) if op.type == X.X86_OP_MEM and not self.seg(op)}
            self.wr(0, v)
        elif mn == "leave":
            s.r["esp"] = s.r["ebp"]
            s.r["ebp"] = self.load(s.r["esp"], 4)
            s.r["esp"] = s.r["esp"] + z3.BitVecVal(4, 32)
        elif mn == "call":
            tgt = ops[0].imm & 0xFFFFFFFF if ops[0].type == X.X86_OP_IMM else None
            if tgt is not None and self.clobber_all(tgt):
                self.havoc_all()
                return
            for r in ("eax", "ecx", "edx"):
                s.r[r] = z3.BitVec(self.nm("call_" + r), 32)
            s.r["esp"] = s.r["esp"] + z3.BitVec(self.nm("call_k"), 32)
            self.fresh_flags()
            s.mem = _fresh(self.nm("call_mem"), "mem")
        elif mn[:3] == "set" and mn[3:] in _CC:
            c = cond_expr(_CC[mn[3:]], s.f)
            self.wr(0, z3.If(c, z3.BitVecVal(1, 8), z3.BitVecVal(0, 8)))
        elif mn[:4] == "cmov" and mn[4:] in _CC:
            c = cond_expr(_CC[mn[4:]], s.f)
            self.wr(0, z3.If(c, self.rd(1), self.rd(0)))
        elif mn in ("loop", "loope", "loopne", "loopz", "loopnz", "jecxz", "jcxz"):
            if mn.startswith("loop"):
                s.r["ecx"] = z3.BitVec(self.nm("loop_ecx"), 32)
        elif mn.split(" ")[-1].rstrip("bwdq") in _STRING and not any(
                o.type == X.X86_OP_REG and self.ins.reg_name(o.reg).startswith(("xmm", "mm")) for o in ops):
            for r in ("eax", "ecx", "esi", "edi"):
                s.r[r] = z3.BitVec(self.nm("str_" + r), 32)
            self.fresh_flags()
            s.mem = _fresh(self.nm("str_mem"), "mem")
        elif mn[0] == "f":
            self.x87(ins, mn)
        elif mn in _GENERIC:
            self.generic(ins)
        else:
            raise Unsup("unmodelled " + mn)

    def width_push(self, op):
        if op.type == X.X86_OP_IMM:
            return 32
        return self.width(op)

    def x87(self, ins, mn):
        mem = [(i, o) for i, o in enumerate(ins.operands) if o.type == X.X86_OP_MEM]
        if mem:
            i, o = mem[0]
            if mn in _X87_STORE:
                if self.seg(o):
                    self.s.mem = _fresh(self.nm("segmem"), "mem")
                else:
                    self.store(self.eas[i], z3.BitVec(self.nm("fst"), o.size * 8), o.size)
            elif mn not in _X87_READ:
                raise Unsup("x87 memory form " + mn)
        self.generic(ins, allow_mem=False)

    def generic(self, ins, allow_mem=True):
        """Registers/flags per regs_access (all fresh); an explicit memory destination gets fresh bytes."""
        try:
            _r, w = ins.regs_access()
        except capstone.CsError:
            raise Unsup("regs_access")
        for rg in w:
            if rg == X.X86_REG_EFLAGS:
                self.fresh_flags()
                continue
            nm = ins.reg_name(rg)
            if nm in PART:
                f = PART[nm][0]
                self.s.r[f] = z3.BitVec(self.nm("gen_" + f), 32)
            elif nm.startswith(("st", "mm", "xmm", "fp", "ymm", "cr", "dr", "es", "cs", "ss", "ds", "fs", "gs")) or nm in (
                    "fpsw", "fpcw", "mxcsr"):
                continue
            else:
                raise Unsup("write to " + nm)
        if allow_mem:
            for i, o in enumerate(ins.operands):
                if o.type == X.X86_OP_MEM and (o.access & capstone.CS_AC_WRITE or (i == 0 and allow_mem)):
                    if self.seg(o):
                        self.s.mem = _fresh(self.nm("segmem"), "mem")
                    else:
                        self.store(self.eas[i], z3.BitVec(self.nm("genm"), o.size * 8), o.size)
        if ins.mnemonic in ("fnsave", "fsave", "fnstenv", "fstenv", "fxsave", "frstor", "fldenv"):
            raise Unsup("x87 environment")


# ---------------------------------------------------------------------------------------------- prover

class Blk:
    __slots__ = ("start", "addrs", "last", "outs")

    def __init__(self, start):
        self.start, self.addrs, self.last, self.outs = start, [], None, []


def _fmt(ins):
    return "%s %s at %x" % (ins.mnemonic, ins.op_str, ins.address)


class Prover:
    """Dead-edge prover for one side's Cfg (see the module docstring)."""

    def __init__(self, cfg, clobber_all=None, timeout_ms=4000, deadline=None, max_blocks=1500, extra_entries=()):
        self.cfg = cfg
        self.extra = {a for a in extra_entries if a in cfg.insns and a != cfg.entry}
        self.clobber_all = clobber_all or (lambda t: False)
        self.tmo = timeout_ms
        self.deadline = deadline
        self.problem = None
        self.nq = 0
        if z3 is None:
            self.problem = "z3 not installed"
            return
        try:
            self._build(max_blocks)
        except Unsup as e:
            self.problem = str(e)

    # ---- graph
    def _build(self, max_blocks):
        cfg = self.cfg
        insns, succ = cfg.insns, cfg.succ
        leaders = {cfg.entry} | self.extra
        for a, ins in insns.items():
            nxt = a + ins.size
            sc = succ.get(a, [])
            plain = (len(sc) == 1 and sc[0][0] == nxt and ins.id != X.X86_INS_JMP and not ins.group(capstone.CS_GRP_JUMP))
            if ins.id == X.X86_INS_JMP:
                op = ins.operands[0]
                if op.type != X.X86_OP_IMM and not (op.type == X.X86_OP_MEM and op.mem.index != 0 and sc):
                    # an indirect jump the CFG could not resolve: the code it reaches is entered from states we do not see
                    raise Unsup("unresolved indirect jump at %x" % a)
            if ins.id == X.X86_INS_CALL or plain:
                continue
            for dst, _e, _c in sc:
                if dst in insns:
                    leaders.add(dst)
            if nxt in insns:
                leaders.add(nxt)
        blocks = {}
        for l in sorted(leaders):
            if l not in insns:
                continue
            b = Blk(l)
            a = l
            while True:
                b.addrs.append(a)
                ins = insns[a]
                nxt = a + ins.size
                sc = succ.get(a, [])
                ends = ins.id == X.X86_INS_JMP or ins.group(capstone.CS_GRP_JUMP) or ins.id in _TERM or not sc \
                    or nxt not in insns or nxt in leaders or sc[0][0] != nxt
                if ins.mnemonic in ("loop", "loope", "loopne", "loopz", "loopnz", "jecxz", "jcxz"):
                    ends = True
                if ends:
                    break
                a = nxt
            b.last = a
            blocks[l] = b
        if len(blocks) > max_blocks:
            raise Unsup("%d blocks (limit %d)" % (len(blocks), max_blocks))
        self.blocks = blocks
        self.blk_of = {a: b.start for b in blocks.values() for a in b.addrs}
        # successors with their branch condition
        for b in blocks.values():
            last = insns[b.last]
            nxt = b.last + last.size
            cc = jcc_cc(last) if last.id != X.X86_INS_JMP and last.group(capstone.CS_GRP_JUMP) else None
            tgt = last.operands[0].imm & 0xFFFFFFFF if last.operands and last.operands[0].type == X.X86_OP_IMM else None
            for dst, e, _c in succ.get(b.last, []):
                if dst not in blocks:
                    if e is None:
                        continue
                    cond = ("cc", cc, dst == tgt) if cc and tgt != nxt else None
                    b.outs.append((None, e, cond))
                    continue
                cond = None
                if cc and tgt != nxt and e is not None:
                    cond = ("cc", cc, dst == tgt)
                b.outs.append((dst, e, cond))
        # dominators over blocks + a virtual root
        ROOT = -1
        self.ROOT = ROOT
        preds = {s: [] for s in blocks}
        preds[cfg.entry].append(ROOT)
        for x in self.extra:
            preds[x].append(ROOT)
        for b in blocks.values():
            for dst, _e, _c in b.outs:
                if dst is not None and b.start not in preds[dst]:
                    preds[dst].append(b.start)
        order, seen = [], set()
        stack = [(ROOT, iter([cfg.entry] + sorted(self.extra)))]
        seen.add(ROOT)
        while stack:
            n, itr = stack[-1]
            for m in itr:
                if m not in seen:
                    seen.add(m)
                    stack.append((m, iter([d for d, _e, _c in blocks[m].outs if d is not None])))
                    break
            else:
                order.append(n)
                stack.pop()
        rpo = order[::-1]
        idx = {n: i for i, n in enumerate(rpo)}
        idom = {ROOT: ROOT}

        def inter(a, b):
            while a != b:
                while idx[a] > idx[b]:
                    a = idom[a]
                while idx[b] > idx[a]:
                    b = idom[b]
            return a
        changed = True
        while changed:
            changed = False
            for n in rpo[1:]:
                ps = [p for p in preds[n] if p in idom]
                nd = ps[0]
                for p in ps[1:]:
                    nd = inter(nd, p)
                if idom.get(n) != nd:
                    idom[n], changed = nd, True

        def dominates(h, n):
            while True:
                if n == h:
                    return True
                if n == ROOT or n not in idom:
                    return False
                n = idom[n]
        self.idom, self.dominates = idom, dominates
        self.depth = {}
        for n in rpo:
            d, x = 0, n
            while x != ROOT:
                x = idom[x]
                d += 1
            self.depth[n] = d
        self.reach_blocks = set(rpo) - {ROOT}
        self.back = set()
        for b in blocks.values():
            if b.start not in self.reach_blocks:
                continue
            for dst, _e, _c in b.outs:
                if dst is not None and dominates(dst, b.start):
                    self.back.add((b.start, dst))
        self.heads = sorted({h for _b, h in self.back}, key=lambda h: -self.depth[h])
        self.in_edges = {s: [] for s in blocks}      # non-back incoming edges: (pred, cond)
        for b in blocks.values():
            if b.start not in self.reach_blocks:
                continue
            for dst, e, c in b.outs:
                if dst is not None and (b.start, dst) not in self.back:
                    self.in_edges[dst].append((b.start, e, c))
        # acyclicity of the cut graph (else irreducible)
        indeg = {s: len(v) for s, v in self.in_edges.items() if s in self.reach_blocks}
        work = [s for s, d in indeg.items() if d == 0]
        if cfg.entry not in indeg:
            raise Unsup("entry unreachable")
        topo = []
        outs = {s: [] for s in indeg}
        for s in indeg:
            for p, _e, _c in self.in_edges[s]:
                outs[p].append(s)
        while work:
            n = work.pop()
            topo.append(n)
            for m in outs[n]:
                indeg[m] -= 1
                if indeg[m] == 0:
                    work.append(m)
        if len(topo) != len(indeg):
            raise Unsup("irreducible control flow")
        self.topo = topo
        self.inv = {h: set(COMPS) for h in self.heads}
        self.bounds = {h: set() for h in self.heads}     # (reg, "ule"|"uge", c): bound holding at every visit to the head
        self.init = fresh_state("in")
        # constants a counter loop compares against: cmp/sub immediates (and +-1 around them)
        ks = set()
        for ins in cfg.insns.values():
            if ins.mnemonic in ("cmp", "sub", "add") and len(ins.operands) == 2 and ins.operands[1].type == X.X86_OP_IMM:
                v = ins.operands[1].imm & 0xFFFFFFFF
                ks.update(((v - 1) & 0xFFFFFFFF, v, (v + 1) & 0xFFFFFFFF))
        self.consts = sorted(ks)[:60] if len(ks) <= 60 else sorted(k for k in ks if k < 0x10000)[:60]
        self.pending = set(self.heads)           # heads whose invariant set is not computed yet: fully fresh
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.md.detail = True

    # ---- encoding
    def cond_of(self, ins, cond, st):
        if cond is None:
            return z3.BoolVal(True)
        _t, cc, taken = cond
        c = cond_expr(cc, st.f)
        return c if taken else z3.Not(c)

    def encode(self):
        """Symbolic state of every block under the current invariant sets. -> (S_in, S_out, reach)."""
        cfg = self.cfg
        S, O, R = {}, {}, {}
        init = self.init
        for n in self.topo:
            inc = []
            if n == cfg.entry:
                inc.append((z3.BoolVal(True), init))
            elif n in self.extra:
                inc.append((z3.BoolVal(True), fresh_state("X%x" % n)))
            for p, e, c in self.in_edges[n]:
                ex = z3.And(R[p], self.cond_of(cfg.insns[self.blocks[p].last], c, O[p]))
                inc.append((ex, O[p]))
            R[n] = z3.Or(*[x for x, _s in inc]) if len(inc) > 1 else inc[0][0]
            if n in self.inv and n in self.heads:
                keep = set() if n in self.pending else self.inv[n]
                st = fresh_state("H%x" % n)
                for x in keep:
                    st.put(x, self.merge(inc, x, n))
                if n not in self.pending and self.bounds[n]:
                    R[n] = z3.And(R[n], *[self.bound_pred(b, st.r[b[0]]) for b in sorted(self.bounds[n])])
            elif len(inc) == 1:
                st = inc[0][1]
            else:
                st = St({}, {}, None)
                for x in COMPS:
                    st.put(x, self.merge(inc, x, n))
            S[n] = st
            ex = Exec(self.md, n, self.clobber_all)
            o = st.copy()
            for a in self.blocks[n].addrs:
                ins = cfg.insns[a]
                if a == self.blocks[n].last and (ins.id == X.X86_INS_JMP or ins.id in _TERM or (
                        ins.group(capstone.CS_GRP_JUMP) and ins.mnemonic not in ("loop", "loope", "loopne", "loopz",
                                                                                  "loopnz", "jecxz", "jcxz"))):
                    break
                ex.step(ins, o)
                o = ex.s
            O[n] = o
        return S, O, R

    def merge(self, inc, x, n):
        d = _fresh("D%x_%s" % (n, x), "mem" if x == "mem" else "flag" if x in FLAGS else "reg")
        v = d
        for ex, s in reversed(inc):
            v = z3.If(ex, s.get(x), v)
        return v

    # ---- solving
    def check(self, *fs):
        if self.deadline is not None and time.time() > self.deadline:
            return "unknown"
        self.nq += 1
        s = z3.Solver()
        s.set("timeout", self.tmo)
        s.add(*fs)
        r = s.check()
        return "sat" if r == z3.sat else "unsat" if r == z3.unsat else "unknown"

    @staticmethod
    def bound_pred(b, v):
        reg, kind, c = b
        return z3.ULE(v, z3.BitVecVal(c, 32)) if kind == "ule" else z3.UGE(v, z3.BitVecVal(c, 32))

    def refute(self, prefix, items):
        """items: {id: violation expr}. The ids whose violation is satisfiable together with prefix (model-guided: every
        model found drops all the violations it satisfies); a solver timeout drops everything still alive."""
        alive = dict(items)
        dropped = set()
        while alive:
            if self.deadline is not None and time.time() > self.deadline:
                return dropped | set(alive)
            self.nq += 1
            s = z3.Solver()
            s.set("timeout", self.tmo)
            s.add(*prefix)
            s.add(z3.Or(*alive.values()))
            r = s.check()
            if r == z3.unsat:
                break
            if r != z3.sat:
                return dropped | set(alive)
            m = s.model()
            hit = [k for k, v in alive.items() if z3.is_true(m.eval(v, model_completion=True))]
            if not hit:
                return dropped | set(alive)
            for k in hit:
                dropped.add(k)
                del alive[k]
        return dropped

    def inv_note(self, h):
        """Short text of a head's invariants: registers equal to their entry value, and the tightest bound per register."""
        lo, hi = {}, {}
        for r, k, c in self.bounds[h]:
            if k == "ule":
                hi[r] = min(hi.get(r, c), c)
            elif c:
                lo[r] = max(lo.get(r, c), c)
        bs = ["%s in [%#x, %s]" % (r, lo.get(r, 0), "%#x" % hi[r] if r in hi else "2^32-1") for r in sorted(set(lo) | set(hi))
              if r in hi or lo.get(r, 0)]
        return "head %x: %s" % (h, ", ".join(["%s unchanged" % x for x in sorted(self.inv[h] & set(FULLS) - {"esp"})] + bs))

    def back_cond(self, b, h, O):
        cfg = self.cfg
        cs = [c for d, _e, c in self.blocks[b].outs if d == h]
        return z3.Or(*[self.cond_of(cfg.insns[self.blocks[b].last], c, O[b]) for c in cs])

    def infer_invariants(self):
        """Houdini, per loop head, innermost first. Phase 1: the components equal to their entry value at every visit.
        Phase 2: for the registers that changed, an unsigned bound `r <=u c` / `r >=u c` (c from the function's compare
        immediates and the entry values) that holds on entry and is preserved by the body. Both inductive."""
        cfg = self.cfg
        for h in self.heads:
            self.pending.discard(h)
            backs = [b for b, hh in sorted(self.back) if hh == h]
            while self.inv[h]:
                S, O, R = self.encode()
                items = {}
                for b in backs:
                    for x in sorted(self.inv[h]):
                        if not z3.eq(O[b].get(x), S[h].get(x)):
                            items[(b, x)] = z3.Not(O[b].get(x) == S[h].get(x))
                drop = set()
                for b in backs:
                    pre = [R[b], self.back_cond(b, h, O)]
                    drop |= {x for (bb, x) in self.refute(pre, {k: v for k, v in items.items() if k[0] == b})}
                if not drop:
                    break
                self.inv[h] -= drop
            # phase 2: bounds on the registers that are not loop-invariant
            regs = [r for r in FULLS if r != "esp" and r not in self.inv[h]]
            if not regs:
                continue
            S, O, R = self.encode()
            entries = [(z3.BoolVal(True), self.init)] if h == cfg.entry else []
            for p, e, c in self.in_edges[h]:
                entries.append((z3.And(R[p], self.cond_of(cfg.insns[self.blocks[p].last], c, O[p])), O[p]))
            ks = set(self.consts)
            for _ex, st in entries:
                for r in regs:
                    v = z3.simplify(st.r[r])
                    if z3.is_bv_value(v):
                        ks.add(v.as_long())
            cand = {(r, k, c) for r in regs for c in ks for k in ("ule", "uge")}
            for ex, st in entries:
                bad = self.refute([ex], {b: z3.Not(self.bound_pred(b, st.r[b[0]])) for b in cand})
                cand -= bad
            while cand:
                self.bounds[h] = set(cand)
                S, O, R = self.encode()
                bad = set()
                for b in backs:
                    pre = [R[b], self.back_cond(b, h, O)]
                    bad |= self.refute(pre, {c: z3.Not(self.bound_pred(c, O[b].r[c[0]])) for c in cand})
                if not bad:
                    break
                cand -= bad
            self.bounds[h] = set(cand)

    # ---- API
    def edge_query(self, edge, enc):
        S, O, R = enc
        a, dst = edge
        n = self.blk_of.get(a)
        if n is None or n not in R or self.blocks[n].last != a:
            return None
        for d, e, c in self.blocks[n].outs:
            if e == edge:
                return n, R[n], self.cond_of(self.cfg.insns[a], c, O[n]), c
        return None

    def prove(self, missed, covered=()):
        """-> {edge: ("dead", note) | ("unknown", reason)}"""
        res = {}
        if self.problem:
            return {e: ("unknown", self.problem) for e in missed}
        try:
            self.infer_invariants()
            enc = self.encode()
        except Unsup as e:
            return {e_: ("unknown", str(e)) for e_ in missed}
        # self-check against the edges the harness really took
        t0 = time.time()
        for e in sorted(covered):
            q = self.edge_query(e, enc)
            if q is None:
                continue
            if self.check(q[1], q[2]) == "unsat":
                return {e_: ("unknown", "model refuted by covered edge %08x->%08x (not proving anything)" % e)
                        for e_ in missed}
            if time.time() - t0 > 60:
                break
        for e in missed:
            q = self.edge_query(e, enc)
            if q is None:
                res[e] = ("unknown", "edge not modelled")
                continue
            n, reach, cond, c = q
            if c is None:
                res[e] = ("unknown", "no branch condition (jump-table, loop/jecxz or unconditional edge)")
                continue
            if self.check(reach) != "sat":
                res[e] = ("unknown", "source block unreachable or undecided in the model")
                continue
            r = self.check(reach, cond)
            if r == "unsat":
                ins = self.cfg.insns[e[0]]
                fl = self.cfg.cmp_before.get(e[0])
                inv = "; ".join(self.inv_note(h) for h in self.heads if self.dominates(h, n))
                res[e] = ("dead", "%s %s: %s -- condition unsatisfiable on every path to %x%s" % (
                    ins.mnemonic, "taken" if c[2] else "fall-through", ("flags from " + _fmt(fl)) if fl else "flags from an earlier block",
                    e[0], " (inside loops; invariants: %s)" % inv if inv else ""))
            elif r == "sat":
                res[e] = ("unknown", "reachable in the model (branch condition satisfiable)")
            else:
                res[e] = ("unknown", "solver timeout")
        return res


def code_refs(code, base, addrs):
    """Addresses in `addrs` that non-branch instructions of the linearly swept bytes use as an immediate or displacement:
    code labels whose address is taken (C++ catch continuations returned by a funclet, `push offset L`). Such a block can
    be entered from a state the CFG does not show, so the prover gives it an extra, fully unknown entry."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    out, off = set(), 0
    while off < len(code):
        got = False
        for ins in md.disasm(code[off:], base + off):
            got = True
            off = ins.address - base + ins.size
            if ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL) or ins.id in (X.X86_INS_JMP,):
                continue
            for op in ins.operands:
                v = op.imm if op.type == X.X86_OP_IMM else op.mem.disp if op.type == X.X86_OP_MEM else None
                if v is not None and (v & 0xFFFFFFFF) in addrs:
                    out.add(v & 0xFFFFFFFF)
        if not got:
            off += 1
    return out


def prove_dead(cfg, missed, covered=(), clobber_all=None, timeout_ms=4000, budget_s=240, extra_entries=()):
    """{edge: ("dead", note) | ("unknown", reason)} for the missed edges of one side. Never raises: any internal
    error makes every edge unknown."""
    missed = sorted(missed)
    try:
        p = Prover(cfg, clobber_all, timeout_ms, time.time() + budget_s, extra_entries=extra_entries)
        return p.prove(missed, covered)
    except Exception as e:      # a prover bug must never become a proof
        return {m: ("unknown", "prover error: %s: %s" % (type(e).__name__, e)) for m in missed}
