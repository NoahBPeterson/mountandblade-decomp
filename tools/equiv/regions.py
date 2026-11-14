#!/usr/bin/env python3
"""Region mode of the angr layer: cut-point decomposition of a function pair (tools/equiv/LAYERS.md, "region").

A whole-function angr run fails on switch tables (an indirect jump) and times out on big functions with many paths
or loops. The region mode proves the same thing piecewise, as a simulation between the two programs:

- A CUT PAIR (x, y, d) is a retail address x, an address y of ours, and the x87 stack depth d there. Its interface
  R(x, y) relates a retail state to a state of ours: the registers in L(x, y) are equal (L = the registers and flags
  live at x in the original or at y in ours, a sound liveness analysis below), esp and DF are equal, both x87
  stacks hold d values and these are equal, and memory at and above esp is equal (all of it: the frame too, at the
  same addresses). Nothing else is assumed: registers outside L are independent symbols on the two sides.
- The set S of cut pairs starts with the entries (and, with hints, nothing else). Each pair in S carries one
  OBLIGATION: from any two states related by R(x, y), both versions either return with the same observables (the
  calling contract of the level, as in the whole-function angr model) or reach pairs (x', y') of S with states
  related by R(x', y'), with the same event sequence in between (calls with their targets, the registers their
  convention reads and memory at and above their esp).
- Obligations are discharged in two ways:
  * LOCK-STEP (no solver): the instructions at x and y are the same instruction up to the relocation bijection
    (one consistent pairing of our relocated fields with the original's addresses, the same trust the byte-match
    check and the angr layer place in relocations, plus func.const_ok and the symbols-file check) and up to branch
    displacements (short or near, the targets are paired instead). From R(x, y) an identical instruction reads
    equal inputs (its uses are in L, memory is equal), so it writes equal outputs, takes the same branch, and the
    successor pair (x2, y2) gets R(x2, y2): L(x, y) contains L(x2, y2) minus the instruction's definitions (both
    liveness analyses are sound and the instruction has the same uses and definitions on both sides). A call
    passes equal registers and memory to the same callee (paired by the bijection), so it returns equal results.
    A switch dispatch `jmp [r*4 + T]` with the same index register pairs entry k of the original's table with
    entry k of ours for every k (both tables have the same length; the range check before it is compared
    instruction by instruction like any other code): each (T_retail[k], T_ours[k]) is added to S.
  * ANGR (Z3): an angr region starts both versions at (x, y) with one shared symbolic state built from R(x, y)
    (registers in L shared, the others per side, shared memory, x87 depth d), runs each to the first STOP of its
    side (an address of S's projection on that side that is a block leader, a loop head, a switch dispatch, a
    rep instruction) or a return, and compares every pair of paths whose conditions overlap: equal events, and
    at the stops the observables of R(x', y') (the live registers of the stop pair, esp, DF, the x87 stack and
    all memory at and above esp) or at a return those of the calling contract. Each stop pair (x', y', d') it
    reaches is added to S. Regions are acyclic (every loop head is a stop), so no loop bound is needed.
- The function is EQUIVALENT when every obligation of S is discharged: by induction over the length of an
  execution, R holds at every cut both executions pass in step, so their event sequences and return observables
  are the same; an execution that runs forever passes infinitely many cuts with the same events. The exit state
  of one region is established for exactly the interface the next region (or lock-step instruction) assumes.

Modelling assumptions on top of the angr layer's (LAYERS.md): memory below esp is dead at every cut (never read
before written; the worker refuses a region that reads below the cut's esp at a concrete address); a switch
table's index stays inside the table (the compiler's range check is compared, not re-proved); at a region start
nothing is known about pointers, so the region mode does not use the whole-function model's "entry pointers do not
point into the frame" or the private-frame protection at calls (every call may write all memory, the saved
registers too): that is weaker, never stronger.

Region proofs never refute (the interface R is stronger than equivalence: a dead frame slot that differs fails a
region without making the functions differ); a failure is `unknown` with the failing region named.

  uv run python tools/equiv/regions.py SRC VA [--symbol SYM] [--level L1|L2] [--timeout S] [--jobs N] [--json]
"""
import hashlib, json, os, re, struct, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import angr_layer as AL  # noqa: E402

GPR7 = ["eax", "ecx", "edx", "ebx", "ebp", "esi", "edi"]
FAM = {}
for _f, _parts in (("eax", "al ah ax"), ("ecx", "cl ch cx"), ("edx", "dl dh dx"), ("ebx", "bl bh bx"),
                   ("esi", "si"), ("edi", "di"), ("ebp", "bp"), ("esp", "sp")):
    FAM[_f] = _f
    for _p in _parts.split():
        FAM[_p] = _f
ALLREGS = frozenset(GPR7 + ["flags"])


class RegionError(Exception):
    pass


# ====================================================================== static side: CFG, tables, liveness
def _is_ijmp_table(i):
    """jmp dword ptr [r*4 + disp] (an MSVC switch dispatch)"""
    from capstone import x86_const as X
    if i.mnemonic != "jmp" or len(i.operands) != 1:
        return False
    o = i.operands[0]
    return o.type == X.X86_OP_MEM and o.mem.index != 0 and o.mem.base == 0 and o.mem.scale == 4 and \
        o.mem.segment == 0


def retail_tables(retail, img=None):
    """{dispatch address: [target addresses]} of the original: each table runs to the next table or the end of the
    function's extent, entries while they point into the function's code."""
    if (retail.meta or {}).get("tables") is not None:      # synthetic tests give them directly
        return {int(k): list(v) for k, v in retail.meta["tables"].items()}
    if img is None:
        import func as FN
        img = FN.image()
    ins = AL.decode(retail.code, retail.va)
    sites = {i.address: i.operands[0].mem.disp & 0xffffffff for i in ins if _is_ijmp_table(i)}
    if not sites:
        return {}
    try:
        hi = img.func_extent(retail.va)[1]
    except Exception:
        hi = max(sites.values()) + 4 * 4096
    starts = sorted(set(sites.values()))
    tabs = {}
    for t in starts:
        nxt = min([s for s in starts if s > t] + [hi])
        ents = []
        for a in range(t, nxt - 3, 4):
            v = struct.unpack("<I", bytes(img.read(a, 4)))[0]
            if not retail.va <= v < retail.va + len(retail.code):
                break
            ents.append(v)
        tabs[t] = ents
    return {a: tabs[t] for a, t in sites.items()}


def our_tables(ours):
    """{dispatch offset: [target offsets]} of ours, from the object: the table is the label the dispatch's DIR32
    names, its entries the DIR32 relocations that follow it (to labels of the same section)."""
    if not any(_is_ijmp_table(i) for i in AL.decode(ours.code, ours.va)):
        return {}
    tabs = (ours.meta or {}).get("tables")
    if tabs is not None:                                  # synthetic tests give them directly
        return {int(k): list(v) for k, v in tabs.items()}
    sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
    from coff import Coff
    c = Coff(ours.meta["obj"])
    s = c.by_name[ours.meta["symbol"]]
    sec, f0, f1 = c.func_extent(s)
    full_end = len(sec.data)
    rel = {v: si for v, si, typ in sec.relocs if typ == 0x06}
    sites = {}
    for i in AL.decode(ours.code, ours.va):
        if not _is_ijmp_table(i):
            continue
        tok = ours.relocs.get(i.address - ours.va + i.disp_offset)
        m = re.search(r"#(\d+)\+([0-9a-f]+)$", tok[1]) if tok else None
        if not m or int(m.group(1)) != s.secno:
            raise RegionError("switch table of ours at +%x: not a label of the function's section" % i.address)
        add = tok[3] if tok[3] < 0x80000000 else tok[3] - (1 << 32)
        sites[i.address] = int(m.group(2), 16) + add
    starts = sorted(set(sites.values()))
    tabs = {}
    for t in starts:
        nxt = min([x for x in starts if x > t] + [full_end])
        ents, a = [], t
        while a + 4 <= nxt and a in rel:
            ts = c.syms.get(rel[a])
            if ts is None or ts.secno != s.secno:
                break
            add = struct.unpack_from("<i", sec.data, a)[0]
            ents.append(ts.value + add - f0)
            a += 4
        tabs[t] = ents
    return {a: tabs[t] for a, t in sites.items()}


X87_PUSH = {"fld", "fild", "fbld", "fld1", "fldz", "fldpi", "fldl2e", "fldl2t", "fldlg2", "fldln2"}
X87_POP1 = {"fstp", "fistp", "fisttp", "fbstp", "faddp", "fsubp", "fsubrp", "fmulp", "fdivp", "fdivrp", "fcomp",
            "fucomp", "ficomp", "fcomip", "fucomip", "fpatan", "fyl2x", "fyl2xp1"}
X87_POP2 = {"fcompp", "fucompp"}
X87_PUSH_EXTRA = {"fptan", "fsincos", "fxtract"}
X87_ZERO = {"fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr", "fiadd", "fisub", "fisubr", "fimul", "fidiv",
            "fidivr", "fchs", "fabs", "fsqrt", "fsin", "fcos", "frndint", "fscale", "fprem", "fprem1", "fxch", "fst",
            "fist", "fcom", "fucom", "ficom", "ftst", "fxam", "fnstsw", "fstsw", "fnstcw", "fstcw", "fldcw",
            "fwait", "wait", "fnclex", "fclex", "fnop", "f2xm1", "fcmovb", "fcmove", "fcmovbe", "fcmovu", "fcmovnb",
            "fcmovne", "fcmovnbe", "fcmovnu", "fcomi", "fucomi"}


def x87_delta(i):
    """values the instruction pushes onto the x87 stack (negative: pops); None when not modelled"""
    m = i.mnemonic
    if not m.startswith("f"):
        return 0
    if m in X87_PUSH or m in X87_PUSH_EXTRA:
        return 1
    if m in X87_POP1:
        return -1
    if m in X87_POP2:
        return -2
    if m in X87_ZERO:
        return 0
    return None


def call_fdelta(name):
    """x87 values a call leaves (a float return: +1, __ftol2: -1), the angr model's rule (do_call)"""
    f = AL.name_facts(name) if name else {}
    return -f.get("fdelta", -1 if f.get("fret") else 0)


class Side:
    """One version's code with its CFG (switch tables included), leaders, loop heads and register liveness."""

    def __init__(self, code, base, relocs, tables, call_name=None):
        self.code, self.base, self.relocs, self.tables = code, base, relocs, tables
        self.end = base + len(code)
        self.ins = AL.decode(code, base)
        self.by = {i.address: i for i in self.ins}
        self.call_name = call_name or (lambda i: None)
        self.call_target = lambda i: None              # the original's callee address of a call (set after the walk)
        self.succ = {}
        self.kind = {}
        for i in self.ins:
            self.succ[i.address], self.kind[i.address] = self._succ(i)
        self.leaders = {base}
        for a, ss in self.succ.items():
            k = self.kind[a]
            if k in ("jcc", "jmp", "table"):
                self.leaders |= set(ss)
                if k == "jcc":
                    self.leaders.add(a + self.by[a].size)
        self.heads = self._heads()
        self.live = None

    def inside(self, t):
        return self.base <= t < self.end

    def internal(self, i):
        """the branch target of i when it is inside the function, else None (a relocated rel32 field of ours
        holds the addend, not a displacement: its target is outside whatever it decodes to)"""
        t = AL._imm_target(i)
        if t is None or not self.inside(t):
            return None
        b = i.bytes
        rel32 = (b[0] in (0xE8, 0xE9) and i.size == 5) or (b[0] == 0x0F and 0x80 <= b[1] <= 0x8F and i.size == 6)
        if rel32 and (i.address - self.base + i.size - 4) in self.relocs:
            return None
        return t

    def _succ(self, i):
        m, a = i.mnemonic, i.address
        fall = a + i.size
        t = AL._imm_target(i)
        if t is not None and self.internal(i) is None:
            t = self.end + 0x10000000           # outside
        if m in ("ret", "retn"):
            return [], "ret"
        if m in ("int3", "ud2", "hlt", "int"):
            return [], "trap"
        if m == "call":
            return [fall], "call"
        if m == "jmp":
            if t is None:
                if a in self.tables:
                    return sorted(set(self.tables[a])), "table"
                return [], "ijmp"
            if self.inside(t):
                return [t], "jmp"
            return [], "tail"
        if m.startswith("j") or m.startswith("loop"):
            if t is not None and not self.inside(t):
                return [fall], "tailcc"
            return [fall] + ([t] if t is not None else []), "jcc"
        if m.startswith(("rep ", "repe ", "repne ", "repz ", "repnz ")):
            return [fall], "rep"
        return [fall], ""

    def _heads(self):
        heads, color = set(), {self.base: 1}
        stack = [(self.base, iter(self.succ.get(self.base, [])))]
        while stack:
            a, it = stack[-1]
            nxt = next(it, None)
            if nxt is None:
                color[a] = 2
                stack.pop()
                continue
            if nxt not in self.by:
                continue
            c = color.get(nxt, 0)
            if c == 1:
                heads.add(nxt)
            elif c == 0:
                color[nxt] = 1
                stack.append((nxt, iter(self.succ.get(nxt, []))))
        self.reach = {a for a, c in color.items() if c}
        heads |= {a for a, k in self.kind.items() if k == "rep"}
        return heads

    def call_reads(self, i):
        t = self.call_target(i)
        if t is None:
            # an unpaired direct callee: anything; an indirect one (virtual, import): no custom convention
            return set(GPR7) if AL._imm_target(i) is not None else set()
        try:
            import facts
            return set(facts.retail_entry(t)) & set(GPR7)
        except Exception:
            return set(GPR7)

    # ---------------------------------------------------------------- liveness (registers and flags)
    def use_def(self, i):
        """(use, def) over GPR families + 'flags'; calls and returns per the conservative call model"""
        from capstone import x86_const as X
        import capstone
        k = self.kind[i.address]
        if k == "call":
            # the callee reads eax, ecx, edx (any convention; cl's same-TU custom conventions too) and, when the
            # original callee is known, what it reads before writing (facts.retail_entry, transitively)
            return {"eax", "ecx", "edx"} | self.call_reads(i), {"eax", "ecx", "edx", "flags"}
        if k == "ret":
            return set(GPR7), set()
        if k in ("tail", "tailcc", "ijmp"):
            return set(ALLREGS), set()
        if k == "trap":
            return set(), set()
        try:
            rd, wr = i.regs_access()
        except capstone.CsError:
            return set(ALLREGS), set()
        use = {FAM.get(i.reg_name(x)) for x in rd} - {None, "esp"}
        df = {FAM.get(i.reg_name(x)) for x in wr} - {None, "esp"}
        from rename_core import op_access
        for n, o in enumerate(i.operands):
            if o.type == X.X86_OP_REG and op_access(i, o, n) & 1:
                use |= {FAM.get(i.reg_name(o.reg))} - {None, "esp"}
        # partial writes (al, ax) keep the rest of the register: a use too
        for n, o in enumerate(i.operands):
            if o.type == X.X86_OP_REG and i.reg_name(o.reg) not in FAM.values() and FAM.get(i.reg_name(o.reg)):
                if op_access(i, o, n) & 2:
                    use.add(FAM[i.reg_name(o.reg)])
        if i.mnemonic in ("xor", "sub") and len(i.operands) == 2 and \
                all(o.type == X.X86_OP_REG for o in i.operands) and i.operands[0].reg == i.operands[1].reg and \
                i.reg_name(i.operands[0].reg) in FAM.values():
            use.discard(FAM[i.reg_name(i.operands[0].reg)])
        fl = getattr(i, "eflags", 0) or 0
        tests = (X.X86_EFLAGS_TEST_CF | X.X86_EFLAGS_TEST_PF | X.X86_EFLAGS_TEST_AF | X.X86_EFLAGS_TEST_ZF |
                 X.X86_EFLAGS_TEST_SF | X.X86_EFLAGS_TEST_OF | X.X86_EFLAGS_TEST_DF)
        if fl & tests or (fl == 0 and (i.mnemonic.startswith(("j", "set", "cmov", "adc", "sbb", "rc", "pushf",
                                                                 "lahf", "loop", "fcmov")))) or \
                i.mnemonic.startswith(("j", "set", "cmov", "adc", "sbb", "rcl", "rcr", "pushf", "lahf")):
            if not (i.mnemonic in ("jmp",) or i.mnemonic.startswith(("jecxz", "jcxz"))):
                use.add("flags")
        full = True
        for f in ("CF", "PF", "AF", "ZF", "SF", "OF"):
            bits = 0
            for kind in ("MODIFY", "RESET", "SET", "UNDEFINED"):
                bits |= getattr(X, "X86_EFLAGS_%s_%s" % (kind, f), 0)
            if not fl & bits:
                full = False
        if full and fl:
            df.add("flags")
        return use, df

    # ---------------------------------------------------------------- provenance (for the entry-pointer assumption)
    def provenance(self, pops):
        """Forward pass from the entry: per instruction (pf, esc, depth) where pf = the registers that may hold a
        frame address (computed from esp) or a value of unknown provenance (loaded from memory or returned by a
        callee after a frame address escaped), esc = a frame address may have escaped (stored to memory, or in a
        register at a call), depth = bytes between esp and the entry esp (a lower bound after `and esp, -2^k`, an
        aligned frame), None when unknown (a callee whose `ret N`
        is unknown, an esp write the pass does not follow, or two depths at a join). pops(i) -> the bytes the
        callee of the call i removes, or None. A frame address stored into the frame itself (push ebp after mov ebp,
        esp) is not an escape until the next call (fst: it may be a stack argument there); meanwhile loads through
        esp or a frame pointer may return it. -> {addr: (frozenset pf, esc, depth, fst)}"""
        from capstone import x86_const as X
        out = {self.base: (frozenset(), False, 0, False)}
        work = [self.base]
        while work:
            a = work.pop()
            i = self.by.get(a)
            if i is None:
                continue
            pf, esc, dep, fst = out[a]
            pf = set(pf)
            m, k = i.mnemonic, self.kind[a]
            rw = reg_rw(i)
            reads, writes = (rw[0], rw[1] | rw[2]) if rw else (set(GPR7), set(GPR7))
            ops = i.operands
            esp_val = any(o.type == X.X86_OP_REG and i.reg_name(o.reg) == "esp" for o in ops) and \
                m not in ("push", "pop") and not (m in ("sub", "add") and ops and ops[0].type == X.X86_OP_REG and
                                                   i.reg_name(ops[0].reg) == "esp")
            esp_lea = m == "lea" and any(o.type == X.X86_OP_MEM and (
                (o.mem.base and i.reg_name(o.mem.base) == "esp") or (o.mem.index and i.reg_name(o.mem.index) == "esp"))
                for o in ops)
            loads = m != "lea" and any(o.type == X.X86_OP_MEM and o.access & 1 for o in ops) or \
                m in ("pop", "popal", "popfd") or m.startswith(("rep movs", "movs", "lods", "rep lods"))
            stores = any(o.type == X.X86_OP_MEM and o.access & 2 for o in ops) or m == "push" or \
                m.startswith(("rep stos", "stos", "rep movs", "movs"))
            src_pf = bool((reads - {"flags"}) & pf) or esp_val or esp_lea

            def framed(o):
                return any(r and FAM.get(i.reg_name(r)) in pf | {"esp"} for r in (o.mem.base, o.mem.index))
            # a frame address stored into the frame itself (push ebp after mov ebp, esp; mov [esp+k], r): not an
            # escape (no callee and no other pointer can reach the frame through it), but loads through the stack
            # pointer or a frame pointer may return it from now on (fst)
            to_frame = (m == "push" and not (ops and ops[0].type == X.X86_OP_MEM)) or (
                m == "mov" and len(ops) == 2 and ops[0].type == X.X86_OP_MEM and ops[0].mem.base and
                i.reg_name(ops[0].mem.base) == "esp" and not ops[0].mem.index and ops[1].type == X.X86_OP_REG)
            load_frame = m in ("pop", "popal", "popfd") or any(
                o.type == X.X86_OP_MEM and o.access & 1 and framed(o) for o in ops)
            nd = dep
            if k == "call":
                if pf & set(GPR7) or fst:
                    esc = True               # (fst: a frame address in the frame may be a stack argument)
                n = pops(i)
                nd = None if (dep is None or n is None) else dep - n
                pf -= {"eax", "ecx", "edx"}
                if esc:
                    pf |= {"eax", "ecx", "edx"}
            else:
                if stores and src_pf:
                    if to_frame:
                        fst = True
                    else:
                        esc = True           # a frame address (or a value that may be one) stored to memory
                if m == "push" and ops and ops[0].type == X.X86_OP_REG and i.reg_name(ops[0].reg) == "esp":
                    esc = True
                for r in writes - {"flags"}:
                    pf.discard(r)
                    if src_pf or (loads and (esc or (fst and load_frame))):
                        pf.add(r)
                if dep is not None:
                    if m == "push" or m.startswith("pushf"):
                        nd = dep + (ops[0].size if ops else 4)
                    elif m == "pop" or m.startswith("popf"):
                        nd = dep - (ops[0].size if ops else 4)
                    elif m in ("sub", "add") and len(ops) == 2 and ops[0].type == X.X86_OP_REG and \
                            i.reg_name(ops[0].reg) == "esp":
                        nd = dep + (ops[1].imm if m == "sub" else -ops[1].imm) if ops[1].type == X.X86_OP_IMM \
                            else None
                    elif m == "and" and len(ops) == 2 and ops[0].type == X.X86_OP_REG and \
                            i.reg_name(ops[0].reg) == "esp" and ops[1].type == X.X86_OP_IMM and \
                            ((-ops[1].imm) & 0xffffffff) in (4, 8, 16, 32, 64):
                        # an aligned frame (and esp, -8): esp moves down by 0 .. 7 bytes; depth stays a LOWER
                        # bound (the frame zone ends at or below the return address: a smaller zone assumes less)
                        nd = dep
                    elif rw is None or "esp" in {FAM.get(i.reg_name(x)) for x in i.regs_access()[1]} and \
                            m not in ("push", "pop", "call", "ret") and not m.startswith(("push", "pop")):
                        nd = None
            st = (frozenset(pf), esc, nd, fst)
            for b in self.succ.get(a, []):
                old = out.get(b)
                if old is None:
                    out[b] = st
                    work.append(b)
                    continue
                d2 = old[2] if old[2] == nd else None
                new = (old[0] | st[0], old[1] or esc, d2, old[3] or fst)
                if new != old:
                    out[b] = new
                    work.append(b)
        return out

    def liveness(self):
        if self.live is not None:
            return self.live
        ud = {a: self.use_def(self.by[a]) for a in self.by}
        pred = {}
        for a, ss in self.succ.items():
            for b in ss:
                pred.setdefault(b, []).append(a)
        live = {a: frozenset() for a in self.by}
        work = list(self.by)
        inwork = set(work)
        while work:
            a = work.pop()
            inwork.discard(a)
            out = set()
            for b in self.succ[a]:
                out |= live.get(b, ALLREGS)          # a successor outside the decoded code: everything live
            u, d = ud[a]
            new = frozenset((out - d) | u)
            if new != live[a]:
                live[a] = new
                for p in pred.get(a, []):
                    if p not in inwork:
                        inwork.add(p)
                        work.append(p)
        self.live = live
        return live

    # ---------------------------------------------------------------- frame-slot liveness (the frame model)
    FRAME_CAP = 0x1000       # frames deeper than this: give up (everything live)

    def frame_use_def(self, i, prov):
        """(use, def, all) over FRAME byte-offsets below the ENTRY esp for one instruction. off = dep - disp (dep:
        bytes esp is below entry esp here). all=True means every frame byte is used (a conservative fallback: an
        escape, an unknown pointer read, an unknown esp depth, an ebp-relative access): then nothing can be proved
        dead. Returns (set, set, bool)."""
        from capstone import x86_const as X
        a = i.address
        st = prov.get(a)
        dep = st[2] if st else None
        k = self.kind[a]
        if k == "call":
            # a call reads its stack arguments [esp+4, esp+4+W); W is unknown for an unknown callee, and after an
            # escape the callee may read any frame byte through the escaped pointer. Conservatively every frame
            # byte is live at a call (the outgoing arguments must correspond): dead-slot relaxation is sound only
            # at a non-call stop, where no callee reads the frame.
            return set(), set(), True
        if k in ("ret", "tail", "tailcc", "ijmp", "trap", "table", "rep"):
            return set(), set(), (k in ("ijmp", "rep"))
        use, dfn = set(), set()
        for o in i.operands:
            if o.type != X.X86_OP_MEM:
                continue
            m = o.mem
            if m.segment:
                return set(), set(), True
            base = self.by[a].reg_name(m.base) if m.base else None
            idx = self.by[a].reg_name(m.index) if m.index else None
            esp_based = base == "esp" and not idx
            if not esp_based or dep is None:
                # an ebp-relative or unknown-pointer or unknown-depth memory access: conservatively, any frame byte
                # may be touched. A read of an unknown frame address makes everything live; a write we cannot
                # localise, so it kills nothing (it adds no def).
                if o.access & 1:
                    return set(), set(), True
                continue
            off = dep - m.disp
            sz = o.size or 4
            rng = range(off, off + sz)
            if o.access & 1:
                use |= set(rng)
            if (o.access & 2) and not (o.access & 1):
                dfn |= set(rng)                      # a full store kills the slot (read-modify keeps it a use)
        return use, dfn, False

    def frame_liveness(self, prov, pops):
        """-> {addr: "all" | frozenset(live frame byte-offsets below entry esp entering addr)}. A sound backward
        dataflow dual to register liveness. "all" = the analysis gave up here (everything live): an escape, an
        unknown pointer/depth/ebp access, or a frame deeper than FRAME_CAP. The frame model compares only the slots
        this marks live; a dead slot that two builds fill differently is then not an obstacle."""
        deps = [prov[a][2] for a in self.by if a in prov and prov[a][2] is not None]
        maxd = max(deps) if deps else 0
        if maxd > self.FRAME_CAP or maxd < 0:
            return {a: "all" for a in self.by}
        FULL = frozenset(range(0, maxd + 4))
        ud = {a: self.frame_use_def(self.by[a], prov) for a in self.by}
        pred = {}
        for a, ss in self.succ.items():
            for b in ss:
                pred.setdefault(b, []).append(a)
        live = {a: frozenset() for a in self.by}
        work = list(self.by)
        inwork = set(work)
        while work:
            a = work.pop()
            inwork.discard(a)
            out = set()
            allsucc = False
            for b in self.succ[a]:
                lo = live.get(b, "miss")
                if lo == "all" or lo == "miss":        # a successor outside the code, or marked all: all live
                    allsucc = True
                    break
                out |= lo
            u, d, allf = ud[a]
            new = "all" if (allsucc or allf) else frozenset((out - d) | u)
            if new != live[a]:
                live[a] = new
                for p in pred.get(a, []):
                    if p not in inwork:
                        inwork.add(p)
                        work.append(p)
        # dead_after[a]: every frame byte is dead on the far side of a (no successor reads the frame from there
        # on). The frame model may relax a slot at a cut (match it at a different offset, or drop a dead one) only
        # when the frame is dead_after the cut: this implementation does not carry the correspondence F forward
        # into the next region or past a call's havoc, so a relaxed slot that is read later would read the wrong
        # (byte-equal-assumed or address-shared-havoc) value. dead_after makes that read impossible.
        dead_after = set()
        for a in self.by:
            ss = self.succ.get(a, [])
            if ss and all(live.get(s, "all") == frozenset() for s in ss):
                dead_after.add(a)
        return live, dead_after


# ====================================================================== the register correspondence
# pi: a tuple over PI_REGS: pi[k] = the register of ours holding the value of the original's PI_REGS[k], or None
# (unrelated). R(x, y, pi) relates exactly those pairs (plus esp, DF, the x87 stack and memory, always).
PI_REGS = GPR7 + ["flags"]
PI0 = tuple(PI_REGS)


def pi_get(pi, r):
    return pi[PI_REGS.index(r)]


def pfam(v):
    """the register of ours a pi entry names ("ecx" or "ecx:8" -> "ecx")"""
    return v.split(":")[0] if v else None


def pwidth(v):
    """the low bits a pi entry relates: "ecx" all 32, "ecx:16" the low 16, "ecx:8" the low 8 (al after mov al, m
    when the upper bytes of eax were unrelated: cl's bools)"""
    return int(v.split(":")[1]) if v and ":" in v else 32


def pi_set(pi, r, s):
    """pi with r -> s, every other register that mapped to s (any width) unrelated"""
    out = [None if (pfam(v) == pfam(s) and PI_REGS[k] != r) else v for k, v in enumerate(pi)]
    out[PI_REGS.index(r)] = s
    return tuple(out)


def pi_drop(pi, r, s):
    """r (original) and s (ours) both unrelated"""
    return tuple(None if (PI_REGS[k] == r or pfam(v) == pfam(s)) else v for k, v in enumerate(pi))


def pi_text(pi):
    d = ["%s>%s" % (r, v) for r, v in zip(PI_REGS, pi) if v != r]
    return ",".join(d) or "id"


def _part(name):
    if name in FAM.values():
        return "32"
    if name.endswith("l"):
        return "l8"
    if name.endswith("h") and len(name) == 2:
        return "h8"
    return "16"


# ====================================================================== lock-step walk
class Bijection:
    """ours relocation key -> retail value; one to one except FOLDS: two functions of ours that the original's
    linker folded into one (identical COMDAT folding: ItemKind::display_name and TroopRec::display_name are both
    0x43a480), accepted by fold_ok. The field pairs [(retail off, ours off)] that made it. Pairs are proposed per
    instruction and committed only when the whole instruction matches."""

    def __init__(self, fold_ok=None):
        self.m, self.inv, self.pairs = {}, {}, []
        self._ps = set()
        self.fold_ok = fold_ok or (lambda k, v, b: False)
        self.folds = {}

    def check(self, ours_tok, rval, pend):
        """can ours_tok pair with rval (given the pending pairs of this instruction)?"""
        k, _ = AL._tok_key(ours_tok)
        m = dict(self.m)
        inv = dict(self.inv)
        for kk, vv, _f, _r, _o in pend:
            m.setdefault(kk, vv)
            inv.setdefault(vv, kk)
        if k in m:
            return (m[k] == rval), False
        if rval in inv and inv[rval] != k:
            if ours_tok[2] == 0x14 and self.fold_ok(k, rval, self):
                return True, True
            return False, False
        return True, False

    def commit(self, pend):
        for k, v, fold, roff, ooff in pend:
            if k not in self.m:
                self.m[k] = v
                if fold:
                    self.folds[k] = v
                else:
                    self.inv.setdefault(v, k)
            if (roff, ooff) not in self._ps:
                self.pairs.append((roff, ooff))
                self._ps.add((roff, ooff))


def _field(bij, R, O, roff, ooff, pend):
    """pair ours' relocated field at ooff with the original's at roff (pending); False when it cannot"""
    rt, ot = R.relocs.get(roff), O.relocs.get(ooff)
    if rt is None or ot is None:
        return False
    # (M&B) a symbol of ours with one address in the symbols file pairs only with that address (+ the addend of a
    # data reference): the walk diverges there instead of committing a pair prepare's symbol_problems rejects
    want = AL.symbol_want(ot)
    if want is not None and want != rt[1]:
        return False
    ok, fold = bij.check(ot, rt[1], pend)
    if not ok:
        return False
    pend.append((AL._tok_key(ot)[0], rt[1], fold, roff, ooff))
    return True


def reg_rw(i):
    """(reads, full writes, partial writes) over register families + 'flags' (DF is not renamed)"""
    from capstone import x86_const as X
    import capstone
    from rename_core import op_access
    try:
        rd, wr = i.regs_access()
    except capstone.CsError:
        return None
    reads = {FAM.get(i.reg_name(x)) for x in rd} - {None}
    full, part = set(), set()
    for x in wr:
        n = i.reg_name(x)
        f = FAM.get(n)
        if f is None:
            continue
        (full if _part(n) == "32" else part).add(f)
    for k, o in enumerate(i.operands):
        if o.type == X.X86_OP_REG and op_access(i, o, k) & 1:
            reads |= {FAM.get(i.reg_name(o.reg))} - {None}
    if i.mnemonic in ("xor", "sub") and len(i.operands) == 2 and \
            all(o.type == X.X86_OP_REG for o in i.operands) and i.operands[0].reg == i.operands[1].reg and \
            _part(i.reg_name(i.operands[0].reg)) == "32":
        reads.discard(FAM[i.reg_name(i.operands[0].reg)])
    # a partial write (al, ax, inc's flags) keeps the rest of the old value: pi keeps the pair only when it was
    # already related (_apply_writes), so it needs no read
    fl = getattr(i, "eflags", 0) or 0
    tests = (X.X86_EFLAGS_TEST_CF | X.X86_EFLAGS_TEST_PF | X.X86_EFLAGS_TEST_AF | X.X86_EFLAGS_TEST_ZF |
             X.X86_EFLAGS_TEST_SF | X.X86_EFLAGS_TEST_OF)
    m = i.mnemonic
    if fl & tests or (m.startswith(("j", "set", "cmov", "adc", "sbb", "rcl", "rcr", "pushf", "lahf", "fcmov"))
                      and m not in ("jmp", "jecxz", "jcxz")):
        reads.add("flags")
    fullf = bool(fl)
    for f in ("CF", "PF", "AF", "ZF", "SF", "OF"):
        bits = 0
        for kind in ("MODIFY", "RESET", "SET", "UNDEFINED"):
            bits |= getattr(X, "X86_EFLAGS_%s_%s" % (kind, f), 0)
        if not fl & bits:
            fullf = False
    if fullf:
        full.add("flags")
    elif fl & ~tests & ~(X.X86_EFLAGS_TEST_DF | X.X86_EFLAGS_MODIFY_DF | X.X86_EFLAGS_RESET_DF |
                         X.X86_EFLAGS_SET_DF):
        part.add("flags")
    return reads - {"esp"}, full - {"esp"}, part - {"esp"}


def _explicit(i):
    """register families an instruction names in its operands (registers, memory bases and indexes)"""
    from capstone import x86_const as X
    out = set()
    for o in i.operands:
        if o.type == X.X86_OP_REG:
            out.add(FAM.get(i.reg_name(o.reg)))
        elif o.type == X.X86_OP_MEM:
            for r in (o.mem.base, o.mem.index):
                if r:
                    out.add(FAM.get(i.reg_name(r)))
    return out - {None}


def _apply_writes(pi, ren, full, part, parts=None):
    """pi after an instruction whose write r (original) is ren[r] in ours. A partial write (al, ah, ax) of equal
    values keeps the low bits related that were related before or that it writes, when they form a prefix: al
    written makes "s:8" of an unrelated register, ax "s:16", ah extends "s:8" to "s:16" (bits 0-7 were already
    equal) and leaves an unrelated register unrelated. parts: {family: [(lo, hi) bit ranges written]}; without it
    (flags, unknown parts) a partial write keeps only a full relation."""
    for r in sorted(full):
        pi = pi_set(pi, r, ren.get(r, r))
    for r in sorted(part - full):
        s = ren.get(r, r)
        cur = pi_get(pi, r)
        w = pwidth(cur) if pfam(cur) == s else 0
        if w == 32:
            continue
        for lo, hi in sorted((parts or {}).get(r, [])):
            if lo <= w:
                w = max(w, hi)
            else:
                w = 0
                break
        if r == "flags" or not (parts or {}).get(r):
            w = 0
        pi = pi_set(pi, r, "%s:%d" % (s, w)) if w else pi_drop(pi, r, s)
    return pi


_PART_BITS = {"l8": (0, 8), "h8": (8, 16), "16": (0, 16), "32": (0, 32)}


def reg_widths(i):
    """({family: bits read}, {family: [(lo, hi) written]}) of the explicit and implicit register operands: a read of
    al needs the low 8 bits related, ah or ax the low 16, anything else (and every address register) all 32"""
    from capstone import x86_const as X
    import capstone
    from rename_core import op_access
    rd, wr = {}, {}
    try:
        ra, wa = i.regs_access()
    except capstone.CsError:
        return None, None

    def need(n):
        f = FAM.get(n)
        if f:
            rd[f] = max(rd.get(f, 0), _PART_BITS[_part(n)][1])
    for x in ra:
        need(i.reg_name(x))
    for k, o in enumerate(i.operands):
        if o.type == X.X86_OP_REG and op_access(i, o, k) & 1:
            need(i.reg_name(o.reg))
        elif o.type == X.X86_OP_MEM:
            for b in (o.mem.base, o.mem.index):
                if b and FAM.get(i.reg_name(b)):
                    rd[FAM[i.reg_name(b)]] = 32
    for x in wa:
        n = i.reg_name(x)
        f = FAM.get(n)
        if f:
            wr.setdefault(f, []).append(_PART_BITS[_part(n)])
    return rd, wr


NONDET = ("rdtsc", "rdtscp", "cpuid", "rdrand", "rdseed", "in", "out", "ins", "outs", "int", "into", "syscall",
          "sysenter", "hlt", "cli", "sti")


RET_REGS = frozenset({"eax", "edx", "ebx", "esi", "edi", "ebp"})


def ins_equal(R, O, x, y, bij, pi, level="L1", ret_regs=RET_REGS):
    """-> (successor pairs, pi after, None) when the instruction at x (original) is the same as ours at y under
    the register correspondence pi, else (None, None, reason). The relocation pairs it needs are committed only on
    success."""
    from capstone import x86_const as X
    ir, io = R.by.get(x), O.by.get(y)
    if ir is None or io is None:
        return None, None, "no instruction"
    if ir.mnemonic != io.mnemonic:
        return None, None, "mnemonic %s / %s" % (ir.mnemonic, io.mnemonic)
    kr, ko = R.kind[x], O.kind[y]
    if kr != ko:
        return None, None, "kind"
    if ir.mnemonic.split()[-1] in NONDET or ir.mnemonic.startswith(("loop", "jecxz", "jcxz")):
        return None, None, "%s not compared instruction by instruction" % ir.mnemonic
    roff, ooff = x - R.base, y - O.base
    pend = []
    every = set(PI_REGS) if level == "L2" else set()
    # ---- branches, calls, returns
    if kr in ("jcc", "jmp", "call", "tail", "tailcc") and AL._imm_target(ir) is not None:
        if AL._imm_target(io) is None:
            return None, None, "branch operand"
        tr, to = R.internal(ir), O.internal(io)
        if kr == "call" and (tr is not None or to is not None):
            return None, None, "call into the function itself"
        if kr in ("jcc", "tailcc") and pi_get(pi, "flags") != "flags":
            return None, None, "flags not related at a conditional branch"
        if kr in ("call", "tail", "tailcc"):
            if ir.size != io.size or ir.bytes[:-4] != io.bytes[:-4]:
                return None, None, "call encoding"
            if not _field(bij, R, O, roff + ir.size - 4, ooff + io.size - 4, pend):
                return None, None, "call target pairing"
            if kr == "call":
                npi, why = _call_rule(R, ir, pi, every)
                if npi is None:
                    return None, None, why
                bij.commit(pend)
                return [(x + ir.size, y + io.size)], npi, None
            if kr == "tail" and pi != PI0:
                return None, None, "tail jump without the identity correspondence"
            bij.commit(pend)
            return ([(x + ir.size, y + io.size)] if kr == "tailcc" else []), pi, None
        if tr is None or to is None:
            return None, None, "branch target inside/outside"
        succ = [(tr, to)] + ([(x + ir.size, y + io.size)] if kr == "jcc" else [])
        return succ, pi, None
    if kr in ("ret", "trap"):
        if ir.bytes != io.bytes:
            return None, None, "bytes"
        if kr == "ret":
            bad = [r for r in sorted(set(ret_regs) | every) if pi_get(pi, r) != r]
            if bad:
                return None, None, "ret without the identity on %s" % ",".join(bad)
        return [], pi, None
    # ---- operand by operand, registers through pi
    ra, rb = reg_rw(ir), reg_rw(io)
    if ra is None or rb is None:
        return None, None, "registers not decoded"
    if list(ir.prefix) != list(io.prefix) or len(ir.operands) != len(io.operands):
        return None, None, "operands"
    ren = {}                         # original family -> ours family, as the operands name them

    def reg_pair(a, b):
        fa, fb = FAM.get(ir.reg_name(a)), FAM.get(io.reg_name(b))
        if fa is None or fb is None:
            return ir.reg_name(a) == io.reg_name(b)
        if _part(ir.reg_name(a)) != _part(io.reg_name(b)):
            return False
        if (fa == "esp") != (fb == "esp"):
            return False
        if ren.get(fa, fb) != fb:
            return False
        ren[fa] = fb
        return True
    for k, (oa, ob) in enumerate(zip(ir.operands, io.operands)):
        if oa.type != ob.type or oa.size != ob.size:
            return None, None, "operand %d" % k
        if oa.type == X.X86_OP_REG:
            if not reg_pair(oa.reg, ob.reg):
                return None, None, "register operand %d" % k
        elif oa.type == X.X86_OP_IMM:
            if ob.size == 4 and io.imm_offset and (ooff + io.imm_offset) in O.relocs:
                if ir.imm_offset != io.imm_offset or not _field(bij, R, O, roff + ir.imm_offset,
                                                                ooff + io.imm_offset, pend):
                    return None, None, "relocated immediate"
            elif oa.imm != ob.imm:
                return None, None, "immediate"
        elif oa.type == X.X86_OP_MEM:
            ma, mb = oa.mem, ob.mem
            if ma.segment != mb.segment or ma.scale != mb.scale or bool(ma.base) != bool(mb.base) or \
                    bool(ma.index) != bool(mb.index):
                return None, None, "memory operand %d" % k
            if ma.base and not reg_pair(ma.base, mb.base):
                return None, None, "memory base %d" % k
            if ma.index and not reg_pair(ma.index, mb.index):
                return None, None, "memory index %d" % k
            if io.disp_offset and io.disp_size == 4 and (ooff + io.disp_offset) in O.relocs:
                if ir.disp_size != 4 or not _field(bij, R, O, roff + ir.disp_offset, ooff + io.disp_offset, pend):
                    return None, None, "relocated displacement"
            elif ma.disp != mb.disp:
                return None, None, "displacement %d" % k
        else:
            return None, None, "operand type"
    # every relocated field of ours in the instruction must have been paired
    seen = {o for _k, _v, _f, _r, o in pend}
    if any(ooff <= o < ooff + io.size and o not in seen for o in O.relocs):
        return None, None, "relocated field of ours not in an operand"
    if kr == "call":                    # an indirect call: its target operand compared above
        for r, v in ren.items():
            if r != "esp" and pi_get(pi, r) != v:
                return None, None, "call target reads %s, not related to ours' %s" % (r, v)
        npi, why = _call_rule(R, ir, pi, every)
        if npi is None:
            return None, None, why
        bij.commit(pend)
        return [(x + ir.size, y + io.size)], npi, None
    # implicit registers are the same registers on both sides
    ea, eb = _explicit(ir), _explicit(io)
    ia = (ra[0] | ra[1] | ra[2]) - ea - {"flags"}
    ib = (rb[0] | rb[1] | rb[2]) - eb - {"flags"}
    if ia != ib:
        return None, None, "implicit registers"
    for r in ia:
        if ren.setdefault(r, r) != r:
            return None, None, "implicit register %s renamed" % r
    ren["flags"] = "flags"
    if {ren.get(r, r) for r in ra[0]} != rb[0] or {ren.get(r, r) for r in ra[1]} != rb[1]:
        return None, None, "register reads/writes"
    rwid, wparts = reg_widths(ir)
    for r in ra[0]:
        v = pi_get(pi, r)
        if pfam(v) != ren.get(r, r) or pwidth(v) < ((rwid or {}).get(r, 32) if r != "flags" else 32):
            return None, None, "reads %s, not related to ours' %s" % (r, ren.get(r, r))
    npi = _apply_writes(pi, ren, ra[1], ra[2], wparts)
    if kr == "table":
        ta, tb = R.tables.get(x), O.tables.get(y)
        if ta is None or tb is None or len(ta) != len(tb):
            return None, None, "switch tables of different length"
        bij.commit(pend)
        return sorted(set(zip(ta, tb))), npi, None
    if kr in ("ijmp", "tail", "tailcc"):
        return None, None, "indirect jump"
    bij.commit(pend)
    return [(x + ir.size, y + io.size)], npi, None


def _call_rule(R, ir, pi, every):
    """a call passes the registers its callee may read (eax, ecx, edx in any convention, what the original callee
    reads before writing, every register at L2) unchanged: they must be related to themselves; afterwards eax,
    ecx, edx and the flags are the callee's results (equal: the same callee, equal inputs) and the callee-saved
    registers keep their pairs"""
    need = {"eax", "ecx", "edx"} | R.call_reads(ir) | every
    bad = [r for r in sorted(need) if pi_get(pi, r) != r]
    if bad:
        return None, "registers the callee reads not related to themselves (%s)" % ",".join(bad)
    npi = pi
    for r in ("eax", "ecx", "edx", "flags"):
        npi = pi_set(npi, r, r)
    return npi, None


def _padding(i):
    if i is None:
        return False
    import rename_core
    return rename_core._padding(i)


def fold_checker(ours, symdb=None, img=None):
    """fold_ok(k, v, bij) for a call target of ours that the pairing already gives to another symbol of ours: the
    original's linker folded identical functions (ICF) into v. Accepted when ours' object defines the function and
    its code equals the original's at v (relocated fields masked), or when the symbols file names v with the same
    member function and signature in another class or template (the linker folds only identical code; the
    pairing of a callee is the trust every layer places in it)."""
    def member(nm):
        m = re.match(r"\?([^@?]+)@(.*?)@@(.*)$", nm or "")
        return (m.group(1), m.group(3)) if m else None

    def ok(k, v, bij):
        if k[0] != "sym" or k[2] != 0:
            return False
        nm = k[1]
        try:
            db = symdb or AL._symdb()
        except BaseException:
            db = None
        if db is not None:
            vas = set(db.mangled.get(nm, ()))
            if vas and vas != {v}:
                return False
            if vas == {v}:
                return True
        code = _our_body(ours, nm)
        if code is not None:
            try:
                import func as FN
                im = img or FN.image()
                lo, hi = im.func_extent(v)
                rc = bytes(im.read(v, hi - lo))
            except Exception:
                rc = None
            body, rel = code
            if rc is not None and len(rc) == len(body):
                mask = {o + q for o in rel for q in range(4)}
                if all(rc[p] == body[p] for p in range(len(body)) if p not in mask):
                    return True
        if db is not None:
            mine = member(nm)
            other = bij.inv.get(v)
            if mine and other and other[0] == "sym" and member(other[1]) == mine:
                return True
        return False
    return ok


def _our_body(ours, nm):
    """(code, relocated offsets) of a function ours' object defines, else None"""
    obj = (ours.meta or {}).get("obj")
    if not obj:
        return None
    try:
        sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
        from coff import Coff
        c = _COFF.get(obj) or _COFF.setdefault(obj, Coff(obj))
        s = c.by_name.get(nm)
        if s is None or not s.defined:
            return None
        sec, f0, f1 = c.func_extent(s)
        return bytes(sec.data[f0:f1]), [v - f0 for v, _si, _t in sec.relocs if f0 <= v < f1]
    except Exception:
        return None


_COFF = {}


class Decomp:
    """S, the lock-step walk and the obligations left for angr regions. A cut is (x, y, pi); its x87 depth d."""

    def __init__(self, R, O, bij=None, level="L1", ret_regs=RET_REGS):
        self.R, self.O, self.level, self.ret_regs = R, O, level, ret_regs
        self.bij = bij or Bijection()
        self.S = {}                  # (x, y, pi) -> x87 depth
        self.edges = {}              # (x, y, pi) -> successor cuts (lock-step)
        self.diverge = {}            # (x, y, pi) -> reason (an angr region starts here)
        self.conflicts = []
        self.parent = {}
        self.forced = set()          # cuts where an angr region starts even though lock-step could go on

    def anchor(self, k):
        """the cut where the renaming chain that ends in the divergence k began: the walk follows a renaming
        (mov esi, [m] / mov eax, [m]) instruction by instruction, but pi relates one register of ours to each of
        the original's, so a later instruction may need a relation pi lost (two registers holding one value); an
        angr region from there sees the whole renaming"""
        chain, p = [k], self.parent.get(k)
        while isinstance(p, tuple) and p in self.edges:
            chain.append(p)
            p = self.parent.get(p)
        chain.reverse()
        for a, b in zip(chain, chain[1:]):
            if a[2] != b[2]:
                return a
        return k

    def add(self, x, y, pi, d, work, frm=None):
        k = (x, y, tuple(pi))
        if k in self.S:
            if self.S[k] != d:
                self.conflicts.append("x87 depth at %x/%x: %d (from %s) and %d (from %s)" % (
                    x, y, self.S[k], self.parent.get(k), d, frm))
            return
        self.S[k] = d
        self.parent[k] = frm
        work.append(k)

    def walk(self, starts):
        work = []
        for st in starts:
            self.add(st[0], st[1], st[2], st[3], work, st[4] if len(st) > 4 else "start")
        while work:
            key = work.pop()
            x, y, pi = key
            d = self.S[key]
            if key in self.forced:
                self.diverge[key] = "renaming chain (anchor of a later divergence)"
                continue
            succ, npi, why = ins_equal(self.R, self.O, x, y, self.bij, pi, self.level, self.ret_regs)
            if succ is None:
                # cl's alignment filler (lea esp, [esp]; mov r, r; nop) changes no state: step over it on one side
                if _padding(self.R.by.get(x)):
                    self.edges[key] = [(x + self.R.by[x].size, y, pi)]
                    self.add(x + self.R.by[x].size, y, pi, d, work, key)
                elif _padding(self.O.by.get(y)):
                    self.edges[key] = [(x, y + self.O.by[y].size, pi)]
                    self.add(x, y + self.O.by[y].size, pi, d, work, key)
                else:
                    self.diverge[key] = why
                continue
            ir = self.R.by[x]
            dd = x87_delta(ir) if self.R.kind[x] != "call" else call_fdelta(self.O.call_name(self.O.by[y]))
            if dd is None:
                self.diverge[key] = "x87 %s not modelled" % ir.mnemonic
                continue
            if self.R.kind[x] == "table" and d:
                self.diverge[key] = "switch with values on the x87 stack"
                continue
            self.edges[key] = [(x2, y2, npi) for x2, y2 in succ]
            for x2, y2 in succ:
                self.add(x2, y2, npi, d + dd, work, key)

    def stops(self):
        """per side: the addresses an angr region stops at"""
        out = []
        for side, proj in ((self.R, {k[0] for k in self.S}), (self.O, {k[1] for k in self.S})):
            st = (side.leaders & proj) | side.heads | {a for a, k in side.kind.items() if k in ("table", "rep")}
            out.append(st)
        return out


# ====================================================================== the layer
def _call_namer(side):
    def nm(i):
        o = i.address - side.base + i.size - 4
        t = side.relocs.get(o)
        if t is None and i.disp_offset:
            t = side.relocs.get(i.address - side.base + i.disp_offset)
        return t[1] if t and t[0] == "sym" else None
    return nm


LAST = {}                   # the last prove()'s sides, walk and region results (reports, tests)
_CACHE = {}                 # region results reused by the self-test: key -> (result, covered, covered bytes sig)


def decompose(retail, ours, tables_r=None, tables_o=None, img=None):
    """-> (R side, O side, Decomp after the walk from the entries)"""
    tr = retail_tables(retail, img) if tables_r is None else tables_r
    to = our_tables(ours) if tables_o is None else tables_o
    to_va = {a + retail.va: [t + retail.va for t in v] for a, v in to.items()}
    R = Side(retail.code, retail.va, dict(retail.relocs), tr)
    # ours is placed at the retail VA (as the angr layer does): addresses of both sides are comparable in reports
    O = Side(ours.code, retail.va, dict(ours.relocs), to_va)
    O.call_name = _call_namer(O)
    R.call_target = lambda i: (AL._imm_target(i) if R.internal(i) is None else None)
    D = Decomp(R, O, Bijection(fold_checker(ours, img=img)))
    D.walk([(retail.va, retail.va, PI0, 0)])
    return R, O, D


def _job_key(retail, x, y, pi, d, bij_sig):
    return (retail.va, x, y, tuple(pi), d, bij_sig)


FAST_PER_REGION = 15.0      # a mutant run's per-region timeout (prove_mutant)


def prove_mutant(retail, ours, level, base_hints=None, timeout=1800.0, jobs=None, log=None):
    """The self-test's run on a mutant (prove.py): a mutant only has to be NOT proved, so the run stops at the
    first region that does not prove, tries no coarser cut variant, never backtracks, gives each region
    FAST_PER_REGION seconds and runs the regions the original proof named under its recorded variant (base_hints:
    the original proof's hints; the regions the mutant does not touch come back from the per-process cache).
    -> the layer result plus "decided": True when the run ended in a decided failure (a difference in the model,
    an x87 sequence difference), False when it was undecided (a timeout, too many paths, a solver unknown, an
    unsupported instruction, hints that do not fit the mutant): prove.py then reruns that mutant fully."""
    r = prove(retail, ours, level, None, timeout, jobs=jobs, log=log, fast={"hints": base_hints})
    r["decided"] = r["status"] == "proved" or _decided(r.get("_fail") or r.get("detail") or "")
    r.pop("_fail", None)
    return r


UNDECIDED = re.compile(r"timeout|more than \d+ paths|solver returned unknown|undecided|worker|error:|"
                       r"not supported|Unsupported|cannot step|jumps to|reads the stack below|symbolic|hint|"
                       r"rounds|conflict|x87 depth|not modelled|loop inside a region")


def _decided(detail):
    """a region failure that is a decided one: the model found a difference (or the x87 sequences differ) and
    nothing in the reason is a resource limit or an unsupported construct"""
    return bool(re.search(r"difference in the model|different x87 instruction sequences", detail)) and \
        not UNDECIDED.search(detail)


def prove(retail, ours, level, hints=None, timeout=1800.0, jobs=None, log=None, per_region=None, fast=None,
          frame_model=None):
    """LAYERS.md API: the region mode as a layer of its own ("region" in prove.py). fast: prove_mutant's mode.
    frame_model (EQUIV_FRAME_MODEL=1, OFF by default): the per-object frame correspondence (LAYERS.md "The
    per-object frame model") — the exit/call memory obligation compares corresponding live frame slots instead of
    the whole frame byte for byte. Experimental; not used by --record unless explicitly enabled."""
    t0 = time.time()
    log = log or (lambda s: None)
    res = lambda st, d, h=None, **kw: dict({"status": st, "seconds": round(time.time() - t0, 2), "detail": d,
                                            "hints": h or {}}, **kw)
    if level not in ("L1", "L2"):
        return res("unknown", "level must be L1 or L2")
    if AL._angr_python() is None:
        return res("unknown", "angr is not installed (see tools/equiv/angr_layer.py docstring)")
    if frame_model is None:
        frame_model = os.environ.get("EQUIV_FRAME_MODEL") == "1"
    jobs = jobs or int(os.environ.get("EQUIV_REGION_JOBS", "1"))
    per_region = per_region or float(os.environ.get("EQUIV_REGION_TIMEOUT", "120"))
    keep_going = bool(os.environ.get("EQUIV_REGION_ALL"))
    if fast is not None:
        per_region, keep_going = min(per_region, FAST_PER_REGION), False
    try:
        R, O, D = decompose(retail, ours)
    except RegionError as e:
        return res("unknown", "region: %s" % e)
    if D.conflicts:
        return res("unknown", "region: " + "; ".join(D.conflicts[:3]))
    hinted = hints.get("regions") if hints else None
    if hints and hints.get("mode") != "region":
        return res("unknown", "region: the hints are not a region bundle")
    fh = (fast or {}).get("hints") or None
    if fh is not None and fh.get("mode") != "region":
        fh = None
    try:
        # a mutant run uses the original proof's pairing when it fits (the cache key of the untouched regions),
        # else its own (a changed literal); either is checked by prepare like any pairing
        try:
            job = _base_job(retail, ours, level, D.bij, fh if fh else hints)
        except AL.HintError:
            if fh is None:
                raise
            job = _base_job(retail, ours, level, D.bij, None)
    except AL.HintError as e:
        return res("unknown", "region: %s" % e)
    set_call_targets(R, O, D.bij)
    LR, LO = R.liveness(), O.liveness()
    PR, PO = R.provenance(_pops(R, D.bij, "retail")), O.provenance(_pops(O, D.bij, "ours"))
    job["frame_model"] = bool(frame_model)
    if frame_model:
        # the frame-slot liveness per side, once for the whole function (the worker looks up any call/exit address):
        # "all" or a list of [lo, hi) offset intervals below the entry esp, plus the addresses with a dead frame
        # on their far side (where the correspondence may be relaxed)
        lr, dr = R.frame_liveness(PR, _pops(R, D.bij, "retail"))
        lo_, do_ = O.frame_liveness(PO, _pops(O, D.bij, "ours"))
        job["frame_r"], job["frame_dead_r"] = _frame_live_job(lr), ["%x" % a for a in sorted(dr)]
        job["frame_o"], job["frame_dead_o"] = _frame_live_job(lo_), ["%x" % a for a in sorted(do_)]
    own_w = job.get("argw") or 0
    bij_sig = hashlib.sha256(json.dumps([job["bij"], job["folds"]]).encode()).hexdigest()[:16]
    done, forced, rounds = {}, set(), 0
    vidx, tried = {}, {}         # key -> index into CUT_VARIANTS; key -> [(variant, stop sets, result)]
    pool = _Pool(jobs)
    try:
        return _prove_loop(retail, ours, level, hints, timeout, jobs, log, per_region, keep_going, t0, res, R, O, D,
                           job, LR, LO, PR, PO, own_w, bij_sig, done, forced, rounds, vidx, tried, hinted, pool,
                           fast=None if fast is None else ((fh or {}).get("regions") or {}))
    finally:
        pool.close()


def _prove_loop(retail, ours, level, hints, timeout, jobs, log, per_region, keep_going, t0, res, R, O, D, job, LR, LO,
                PR, PO, own_w, bij_sig, done, forced, rounds, vidx, tried, hinted, pool, fast=None):
    """fast (a mutant run, prove_mutant): the original proof's region hints; a region they name runs under its
    recorded variant, any other under the finest; a failure is final (no coarser variant, no backtracking)"""
    entry = (retail.va, retail.va, PI0, 0)
    nbt = [0]

    def backtrack(D, key, stops_r, stops_o):
        """a region failed under every variant: when it starts at a stop pair a finer variant of an earlier region
        chose (directly or through lock-step), that region's cut may have split code the compilers moved across
        it (a value computed before the join on one side, after it on the other, is unrelated there): rerun that
        region under its next coarser variant (a different stop set), and the walk of the next round no longer
        reaches this one. Bounded by MAX_BACKTRACK per proof; never with hints."""
        if hinted is not None or fast is not None or nbt[0] >= MAX_BACKTRACK:
            return
        p = D.parent.get(key)
        while isinstance(p, tuple) and len(p) == 3 and p in D.edges:
            p = D.parent.get(p)
        if not (isinstance(p, tuple) and len(p) == 2 and p[0] == "region"):
            return
        p = p[1]
        rp = done.get(p)
        if not rp or rp["status"] != "proved" or "_sets" not in rp or rp.get("cut", "none") == CUT_VARIANTS[-1]:
            return
        # the next round reruns p under the next coarser variant whose stops (computed then, without p's own
        # exits in S) differ from the tried ones; when none differs, p's proof is put back unchanged
        nbt[0] += 1
        tried.setdefault(p, []).append((rp["cut"], rp["_sets"], rp))
        vidx[p] = CUT_VARIANTS.index(rp["cut"]) + 1
        del done[p]
        log("  backtrack: region %s coarser than %s (the region at %s failed)" % (
            _hint_key(retail, *p), rp["cut"], _hint_key(retail, *key)))
    # the registers the calling contract compares at a return (the angr model's rule): edx only for a 64-bit
    # return (or an unknown return type)
    ret_regs = {"eax", "ebx", "esi", "edi", "ebp"} | ({"edx"} if job.get("ret64", True) else set())
    while True:
        rounds += 1
        if rounds > MAX_ROUNDS:
            return res("unknown", "region: more than %d rounds" % MAX_ROUNDS)
        # S: the walk from the entry, closed under the stop pairs of the proved regions it needs
        D = Decomp(R, O, D.bij, level, ret_regs)
        D.forced = forced
        D.walk([entry])
        while True:
            new = []
            for k in list(D.diverge):
                r = done.get(k)
                if r and r["status"] == "proved":
                    for x2, y2, d2, pi2 in r.get("exits", []):
                        if (x2, y2, tuple(pi2)) not in D.S:
                            new.append((x2, y2, tuple(pi2), d2, ("region", k)))
            if not new:
                break
            D.walk(new)
        if D.conflicts:
            return res("unknown", "region: " + "; ".join(D.conflicts[:3]))
        added = False
        for k in sorted(D.diverge, key=str):
            if k in done:
                continue
            a = D.anchor(k)
            if a != k and a not in forced:
                forced.add(a)
                added = True
        if added:
            continue
        needed = sorted(D.diverge, key=lambda k: (k[0], k[1], [v or "" for v in k[2]]))
        bad = [k for k in needed if k in done and done[k]["status"] != "proved"]
        todo = [k for k in needed if k not in done]
        if (bad and not keep_going) or not todo:
            break
        stops_r, stops_o = D.stops()
        batch = []
        for key in todo:
            x, y, pi = key
            d = D.S[key]
            jk = _job_key(retail, x, y, pi, d, bij_sig)
            hit = _CACHE.get(jk)
            if hit is not None and _cov_sig(O, hit[1]) == hit[2] and \
                    CUT_VARIANTS.index(hit[0].get("cut", "none")) >= vidx.get(key, 0):
                done[key] = dict(hit[0], cached=True)
                continue
            hk = _hint_key(retail, x, y, pi)
            hint = (hinted or {}).get(hk)
            if hinted is not None and hint is None:
                return res("unknown", "region: the hints name no region at %s" % hk)
            if fast is not None and hint is None:
                # a mutant run: the original proof's variant for a region it named, else the finest
                hint = dict(fast[hk], cut=fast[hk].get("cut", "none")) if hk in fast else {"cut": CUT_VARIANTS[0]}
            # the cut variant: the hinted one, else the finest not tried yet whose stops differ from the tried ones
            if hint is not None:
                variant = hint.get("cut", "none")
                if variant not in CUT_VARIANTS:
                    return res("unknown", "region: the hints name an unknown cut variant at %s" % hk)
                sets = (region_sets(R, x, stops_r, variant), region_sets(O, y, stops_o, variant))
            else:
                seen = [t[1] for t in tried.get(key, [])]
                while True:
                    variant = CUT_VARIANTS[vidx.get(key, 0)]
                    sets = (region_sets(R, x, stops_r, variant), region_sets(O, y, stops_o, variant))
                    if sets not in seen or variant == CUT_VARIANTS[-1]:
                        break
                    vidx[key] = vidx.get(key, 0) + 1
                if sets in seen:                  # the coarser variants stop at the same places: nothing new
                    ok = [t for t in tried[key] if t[2]["status"] == "proved"]
                    if ok:                        # a backtracked region with no coarser stops: its proof stands
                        done[key] = ok[-1][2]
                        continue
                    done[key] = _failed(tried[key])
                    backtrack(D, key, stops_r, stops_o)
                    continue
            batch.append({"key": key, "x": x, "y": y, "pi": list(pi), "d": d, "jk": jk, "cut": variant,
                          "sets": sets, "hint": {k: v for k, v in (hint or {}).items() if k != "cut"},
                          "dz": _cut_dz(PR, PO, x, y, pi, own_w)})
        rem = timeout - (time.time() - t0)
        if rem < 5 and batch:
            return res("unknown", "region: timeout after %.0fs (%d regions done, %d left)" % (
                timeout, len(done), len(batch)))
        out = _run_regions(job, batch, LR, LO, per_region, rem, pool)
        for b in batch:
            key = b["key"]
            r = out.get((b["x"], b["y"], tuple(b["pi"])),
                        {"status": "unknown", "detail": "no result (worker failed)"})
            r["cut"] = b["cut"]
            log("  region %s (d%d, %s): %s %.1fs %s" % (_hint_key(retail, b["x"], b["y"], b["pi"]), b["d"],
                                                        b["cut"], r["status"], r.get("seconds", 0),
                                                        r["detail"][:300]))
            if r["status"] == "proved":
                r["_sets"] = b["sets"]
                done[key] = r
                cov = sorted(set(r.get("covered") or []))
                _CACHE[b["jk"]] = (r, cov, _cov_sig(O, cov))
                continue
            tried.setdefault(key, []).append((b["cut"], b["sets"], r))
            if hinted is None and fast is None and b["cut"] != CUT_VARIANTS[-1]:
                vidx[key] = CUT_VARIANTS.index(b["cut"]) + 1          # a coarser variant next round
                continue
            done[key] = _failed(tried[key])
            backtrack(D, key, stops_r, stops_o)
    done = {k: done[k] for k in D.diverge if k in done}
    LAST.clear()
    LAST.update(R=R, O=O, D=D, done=done)
    bad = sorted((k for k, v in done.items() if v["status"] != "proved"), key=str)
    nreg = len(done)
    nfold = len(job["folds"])
    nfine = sum(1 for v in done.values() if v.get("cut", "none") != "none")
    summary = ("regions: %d cuts (%d lock-step), %d angr regions (%d proved, %d with fine cuts), %d switch tables, "
               "%d relocation pairs%s, %.0fs" % (
                   len(D.S), len(D.edges), nreg, nreg - len(bad), nfine, len(R.tables), len(job["bij"]),
                   " (%d folded callees)" % nfold if nfold else "", time.time() - t0))
    info = {"regions": {_hint_key(retail, *k): [v["status"], v["detail"][:600], v.get("seconds", 0),
                                                v.get("cut", "none")]
                        for k, v in sorted(done.items(), key=lambda kv: str(kv[0]))},
            "cuts": len(D.S), "lockstep": len(D.edges), "tables": {"%x" % a: len(v) for a, v in R.tables.items()},
            "rounds": rounds}
    if bad:
        k = bad[0]
        return res("unknown", "%d of %d regions not proved, first %s: %s | %s" % (
            len(bad), nreg, _hint_key(retail, *k), done[k]["detail"][:600], summary), info=info)
    covered = sorted({c for v in done.values() for c in (v.get("covered") or [])})
    hints_out = {"v": 1, "mode": "region", "bij": job["bij"], "folds": job["folds"],
                 "regions": {_hint_key(retail, *k): _region_hint(v)
                             for k, v in sorted(done.items(), key=lambda kv: str(kv[0]))}}
    return res("proved", summary, hints_out, covered=covered or None, info=info)


MAX_ROUNDS = 2000
MAX_BACKTRACK = 8


def _failed(ts):
    """every cut variant of a region failed: the finest one's reason first (the coarser ones mostly run out of
    paths)"""
    ts = [t for t in ts if t[2]["status"] != "proved"]
    r = ts[-1][2]
    return dict(r, detail=" || ".join("[%s] %s" % (c, t["detail"][:400]) for c, _s, t in ts),
                seconds=sum(t.get("seconds", 0) for _c, _s, t in ts))


def _region_hint(v):
    """a region's bundle entry: the worker's hints and the cut variant (absent: "none", the bundles before fine
    cuts)"""
    h = dict(v.get("hints") or {})
    if v.get("cut", "none") != "none":
        h["cut"] = v["cut"]
    return h


def _hint_key(retail, x, y, pi):
    return "%x,%x,%s" % (x - retail.va, y - retail.va, pi_text(pi))


def _pops(side, bij, which):
    """pops(call insn) for Side.provenance: the bytes the callee removes. The original's callee from its own code
    (facts.callee_pop: every reachable `ret N` the same), __chkstk / __alloca_probe from the `mov eax, N` before it
    (negative: it allocates), an indirect call: unknown."""
    prev = {}
    for i in side.ins:
        prev[i.address + i.size] = i

    def pops(i):
        t = side.call_target(i)
        tok = side.relocs.get(i.address - side.base + i.size - 4) if which == "ours" else None
        nm = tok[1] if tok and tok[0] == "sym" else None
        if nm is None and t is not None and which == "retail":
            k = bij.inv.get(t)
            nm = k[1] if k and k[0] == "sym" else None
        if nm and nm.startswith(("__chkstk", "__alloca_probe")):
            p = prev.get(i.address)
            if p is not None and p.mnemonic == "mov" and p.op_str.startswith("eax, ") and \
                    p.operands[1].type == 2:
                return -(p.operands[1].imm & 0xffffffff)
            return None
        if t is None:
            return None
        try:
            import facts
            return facts.callee_pop(t)
        except Exception:
            return None
    return pops


def _cut_dz(PR, PO, x, y, pi, own_w):
    """the entry-pointer assumption at a cut: the original's registers that provably hold no frame address (on
    either side: the values are equal and so are the frame zones), whether no frame address escaped before the cut
    (then memory at the cut holds none either), and the stack depth (the frame zone's top). None: not usable."""
    a, b = PR.get(x), PO.get(y)
    if a is None or b is None:
        return None
    dep = a[2] if a[2] is not None else b[2]
    if dep is None or (b[2] is not None and b[2] != a[2]) or dep < 0:
        return None
    nf = [r for r, v in zip(PI_REGS[:7], pi[:7]) if r not in a[0] or (v is not None and pwidth(v) == 32 and
                                                                       v not in b[0])]
    return {"nf": nf, "noesc": not a[1] or not b[1], "depth": dep, "w": own_w}


def set_call_targets(R, O, bij):
    """the original's callee of each direct call: the retail side's own target, ours' through the bijection (an
    unpaired one is unknown: it reads every register)"""
    def ot(i):
        if AL._imm_target(i) is None:
            return None
        tok = O.relocs.get(i.address - O.base + i.size - 4)
        if tok is None:
            return None
        k, _ = AL._tok_key(tok)
        return bij.m.get(k)
    O.call_target = ot
    R.live = O.live = None


def _cov_sig(O, cov):
    h = hashlib.sha256()
    for c in cov:
        i = O.by.get(O.base + c)
        h.update(b"%x:" % c + (bytes(i.bytes) if i is not None else b"-"))
    return h.hexdigest()


def _base_job(retail, ours, level, bij, hints):
    """the angr layer's job for the whole function, with the walk's bijection (completed by the angr layer's own
    proposal for fields the walk did not reach, where consistent; checked by prepare like any hint bijection)"""
    # a fold is symmetric: whichever of the folded keys build_mapping meets second must be allowed to share the
    # address, and the re-pairing below may reorder the pairs (0x49fa10: the walk met TroopRec::display_name
    # first, prepare ItemKind::display_name)
    folds = sorted(set(bij.folds) | {bij.inv[v] for v in bij.folds.values() if v in bij.inv})
    if hints and "bij" in hints:
        pairs = [tuple(p) for p in hints["bij"]]
        folds = [tuple(f) for f in hints.get("folds", [])]
        # the walk's pairing must be the hints' pairing by value (ours' relocation key -> the original's address):
        # the symbols-file and content re-pairing may pair one of ours' fields with another field of the original
        # that holds the same address (0x50fc60: operator delete at +0x52 and at +0x140)
        hm = {}
        for r, o in pairs:
            if r in retail.relocs and o in ours.relocs:
                hm.setdefault(AL._tok_key(ours.relocs[o])[0], set()).add(retail.relocs[r][1])
        if any(hm.get(k) != {v} for k, v in bij.m.items() if k not in bij.folds) or \
                any(v not in hm.get(k, ()) for k, v in bij.folds.items()):
            raise AL.HintError("hint bijection does not contain the pairs the walk makes")
    else:
        pairs = list(bij.pairs)
        have_o = {o for _, o in pairs}
        have_r = {r for r, _ in pairs}
        m, inv = dict(bij.m), dict(bij.inv)
        prop, _clean = AL.propose_bijection(retail, ours)
        for r, o in prop:
            if o in have_o or r in have_r or r not in retail.relocs or o not in ours.relocs:
                continue
            k, _ = AL._tok_key(ours.relocs[o])
            x = retail.relocs[r][1]
            if (k in m and m[k] != x) or (x in inv and inv[x] != k):
                continue
            m[k], inv[x] = x, k
            pairs.append((r, o))
            have_o.add(o)
            have_r.add(r)
        # the positional proposal pairs literal constants by load order, which differs where the two compilers
        # scheduled the loads differently (update_ai 0x4465e0: ours multiplies by -20 before 13): re-pair literals
        # by content and named symbols by the symbols file, as the angr layer's own prepare does; prepare still
        # checks the result (one-to-one, const_problems, symbol_problems)
        pairs = AL.pair_by_symbols(retail, ours, AL.pair_constants_by_content(retail, ours, pairs))
    # one pairing for the whole proof: the lock-step walk trusted bij.m, the regions get `pairs` (checked by
    # prepare against the constants and the symbols file); by value they must agree, else the walk relied on a
    # pairing the checks never saw (0x50fc60: the walk paired operator delete with 0x53570d, the symbols file has
    # it at 0x535712)
    hm = {}
    for r, o in pairs:
        if r in retail.relocs and o in ours.relocs:
            hm.setdefault(AL._tok_key(ours.relocs[o])[0], set()).add(retail.relocs[r][1])
    bad = [k for k, v in bij.m.items() if k not in bij.folds and hm.get(k) != {v}] + \
        [k for k, v in bij.folds.items() if v not in hm.get(k, ())]
    if bad:
        raise AL.HintError("the lock-step walk's relocation pairing differs from the checked one at %s" % (
            ", ".join(str(k[1])[:40] for k in bad[:3])))
    job = AL.prepare(retail, ours, level, {"bij": [list(p) for p in pairs]}, folds=folds)
    job["hints"] = {}
    job["folds"] = [list(f) for f in folds]
    try:
        import func as FN
        im = FN.image()
        job["image"] = [im.base, im.base + im.size]
    except BaseException:
        job["image"] = None
    return job


def _intervals(offs):
    """a set of byte offsets -> sorted [[lo, hi), ...] maximal intervals"""
    out = []
    for o in sorted(offs):
        if out and o == out[-1][1]:
            out[-1][1] = o + 1
        else:
            out.append([o, o + 1])
    return out


def _frame_live_job(F):
    """the frame-slot liveness of a whole side (regions.Side.frame_liveness) -> JSON: {addr hex: "all" | [[lo,hi)]}"""
    return {"%x" % a: ("all" if v == "all" else _intervals(v)) for a, v in F.items()}


def _run_regions(job, batch, LR, LO, per_region, total, pool):
    """run the angr worker on each region of the batch (the pool's processes) -> {(x, y, pi): result}"""
    if not batch:
        return {}
    regions = []
    for b in batch:
        (sr, cr), (so, co) = b["sets"]
        regions.append({"x": b["x"], "y": b["y"], "d": b["d"], "pi": b["pi"], "dz": b.get("dz"),
                        "stops_r": sorted(sr), "stops_o": sorted(so),
                        "cands_r": sorted(cr), "cands_o": sorted(co),
                        "live_r": {"%x" % a: sorted(LR.get(a, ALLREGS)) for a in sr | cr},
                        "live_o": {"%x" % a: sorted(LO.get(a, ALLREGS)) for a in so | co},
                        "hints": b["hint"] or {}, "timeout": per_region})
    return pool.run(job, regions, total)


class _Pool:
    """persistent angr workers (angr_layer.py --serve): a chain of fine cuts is discovered one region per round,
    and starting a worker (angr, the two projects) costs seconds. Each process takes one job line and answers one
    result line per region, then a {"done": 1} line; a process past its deadline is killed (its regions are
    unknown) and replaced on the next batch. close() kills every process."""

    def __init__(self, n):
        self.n = max(1, n or 1)
        self.procs = [None] * self.n
        self.errs = [None] * self.n
        self.sent = [None] * self.n

    def _proc(self, k):
        import tempfile
        p = self.procs[k]
        if p is None or p.poll() is not None:
            kw = {"start_new_session": True} if os.name != "nt" else \
                {"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
            env = dict(os.environ, PYTHONHASHSEED="0")
            self.errs[k] = tempfile.TemporaryFile()
            p = subprocess.Popen([AL._angr_python(), "-I", os.path.join(HERE, "angr_layer.py"), "--serve"],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.errs[k], env=env, **kw)
            self.procs[k] = p
            self.sent[k] = None
        return p

    def _err(self, k):
        f = self.errs[k]
        if f is None:
            return ""
        try:
            f.seek(0)
            return f.read().decode(errors="replace").strip()[-300:]
        except Exception:
            return ""

    def run(self, job, regions, total):
        import threading
        chunks = [regions[k::self.n] for k in range(self.n)]
        out, res = {}, {}

        def talk(k, c):
            p = self._proc(k)
            got = []
            res[k] = got
            try:
                # the base job once per process (it is the same for every batch of one proof), the regions per batch
                base = json.dumps(job, sort_keys=True)
                msg = {"regions": c} if self.sent[k] == base else dict(job, regions=c)
                p.stdin.write((json.dumps(msg) + "\n").encode())
                p.stdin.flush()
                self.sent[k] = base
                for ln in p.stdout:
                    ln = ln.decode(errors="replace")
                    if not ln.startswith("{"):
                        continue
                    try:
                        r = json.loads(ln)
                    except ValueError:
                        continue
                    if r.get("done"):
                        return
                    got.append(r)
            except Exception:
                pass
        ths = []
        t0 = time.time()
        for k, c in enumerate(chunks):
            if c:
                t = threading.Thread(target=talk, args=(k, c), daemon=True)
                t.start()
                ths.append((t, k, c, max(1.0, min(total, sum(r["timeout"] + 20 for r in c)))))
        for t, k, c, lim in ths:
            t.join(max(0.0, lim - (time.time() - t0)))
            if t.is_alive():
                AL._kill(self.procs[k])
                t.join(10)
            for r in res.get(k, []):
                if "x" in r:
                    out[(r["x"], r["y"], tuple(r["pi"]))] = r
            for r in c:
                if (r["x"], r["y"], tuple(r["pi"])) not in out:
                    p = self.procs[k]
                    if p is not None and p.poll() is None and not t.is_alive():
                        AL._kill(p)                          # out of step with us: start a new one next time
                    out[(r["x"], r["y"], tuple(r["pi"]))] = {
                        "status": "unknown", "detail": "worker ended without a result: %s" % self._err(k)}
        return out

    def close(self):
        for p in self.procs:
            if p is not None and p.poll() is None:
                AL._kill(p)
        for f in self.errs:
            try:
                f and f.close()
            except Exception:
                pass


# ====================================================================== fine cuts inside a diverging stretch
# Where the two versions diverge early, the stops of S alone leave one angr region over most of the function, and
# its paths multiply at every branch. A region may stop earlier: at the JOINS of its stretch (instructions where
# paths from its start re-converge). Which stops a region uses changes only where obligations are placed: every stop
# pair a compared path pair reaches is added to S with the interface R the region proved there (LAYERS.md, "Fine
# cuts"). The variants are tried finest first; one that does not prove is replaced by the next, coarser one.
CUT_VARIANTS = ("joins", "pdom", "none")


def _region_graph(side, a, stops):
    """the instructions a region from a can execute before a stop (a included), and their predecessors there"""
    G, todo, pred = {a}, [a], {}
    while todo:
        b = todo.pop()
        for c in side.succ.get(b, []):
            if c not in side.by or c in stops:
                continue
            pred.setdefault(c, set()).add(b)
            if c not in G:
                G.add(c)
                todo.append(c)
    return G, pred


def fine_stops(side, a, stops, variant):
    """the extra stops of a region from a under a cut variant: "joins": every instruction of the stretch with two
    or more predecessors in it (paths from a re-converge there); "pdom": only the first such join that every path
    from a passes (a post-dominator of a in the stretch); "none": no extra stops. Stopping earlier is always
    allowed (the stop pairs reached become cuts of S with their own obligations), so this is a choice of
    granularity, not an assumption."""
    if variant == "none":
        return set()
    G, pred = _region_graph(side, a, stops)
    joins = {c for c in G if c != a and len(pred.get(c, ())) >= 2}
    if variant == "joins" or not joins:
        return joins
    # post-dominators over the (acyclic: every loop head is a stop) stretch, a virtual exit after the stops, the
    # returns and the edges leaving it
    order, seen = [], set()
    for r in [a]:
        stack = [(r, iter(sorted(side.succ.get(r, []))))]
        seen.add(r)
        while stack:
            n, it = stack[-1]
            c = next(it, None)
            if c is None:
                order.append(n)
                stack.pop()
                continue
            if c in G and c not in seen:
                seen.add(c)
                stack.append((c, iter(sorted(side.succ.get(c, [])))))
    EXIT = -1
    pdom = {}
    for n in order:                                    # successors before predecessors (post-order)
        ss = side.succ.get(n, [])
        sets = [pdom[c] if c in G and c in pdom else frozenset([EXIT]) for c in ss] or [frozenset([EXIT])]
        if any(c in G and c not in pdom for c in ss):
            return set()                               # a cycle inside the stretch: not expected, no extra stop
        pdom[n] = frozenset([n]) | frozenset.intersection(*sets)
    chain = [p for p in pdom.get(a, ()) if p != a and p != EXIT and p in joins]
    if not chain:
        return set()
    # the nearest: the one the others post-dominate
    first = max(chain, key=lambda p: len(pdom[p]))
    return {first}


def region_sets(side, a, stops, variant):
    """(stops reached, candidate leaders) of a region from a with the cut variant's extra stops"""
    extra = fine_stops(side, a, stops, variant)
    return _reach_sets(side, a, stops | extra if extra else stops)


def _reach_sets(side, a, stops):
    """the stops a region from a can reach (walking the static CFG, not through stops), and the other leaders on
    the way (candidate cuts: where the other version's block structure differs)"""
    seen, todo, hit, cand = {a}, [a], set(), set()
    while todo:
        b = todo.pop()
        for c in side.succ.get(b, []):
            if c in stops:
                hit.add(c)
                continue
            if c in side.leaders:
                cand.add(c)
            if c not in seen:
                seen.add(c)
                todo.append(c)
    if a in stops:
        hit.add(a)
    return hit, cand - hit


# ====================================================================== command line
def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("va")
    ap.add_argument("--symbol")
    ap.add_argument("--level", default="L1")
    ap.add_argument("--timeout", type=float, default=3600)
    ap.add_argument("--per-region", type=float, default=120)
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--all", action="store_true", help="keep going after a region fails (report every region)")
    ap.add_argument("--frame-model", action="store_true", help="the experimental per-object frame model "
                    "(EQUIV_FRAME_MODEL=1); OFF by default, never used by prove.py --record")
    ap.add_argument("--json")
    a = ap.parse_args()
    if a.jobs > 4:
        sys.exit("--jobs at most 4")
    if a.all:
        os.environ["EQUIV_REGION_ALL"] = "1"
    import func as FN
    va = int(a.va, 16)
    r = prove(FN.retail_func(va), FN.our_func(a.src, va, a.symbol), a.level, None, a.timeout, jobs=a.jobs,
              log=print, per_region=a.per_region, frame_model=a.frame_model)
    print(r["status"], r["seconds"], r["detail"][:600])
    if a.json:
        json.dump(r, open(a.json, "w"), indent=1, default=str)


if __name__ == "__main__":
    main()
