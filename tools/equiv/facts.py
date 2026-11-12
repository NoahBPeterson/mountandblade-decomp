"""Register facts a caller can depend on (acceptance step 2 of prove.py), and the original callees' facts the
renaming layer's call model needs.

Ported from spore-decomp tools/equiv/facts.py (commit aaaf5c79f); the image access is M&B's (func.image(), function
extents from work/index + symbols/extent_merge.txt), and entry registers follow calls transitively (cl 13.10's
same-TU custom conventions pass arguments in eax: a callee that hands eax on to its own callee reads it).

kill set:    the volatile registers (eax, ecx, edx, xmm0-7) a function's code can write, through its own calls.
             A caller compiled with knowledge of the callee (same source file) keeps a value in a register the callee
             does not write.
entry regs:  the registers read before being written on some path from entry (push of a callee-saved register
             and zero idioms like xor r, r are not reads). A function that reads one has a register argument.

Callee kill sets come from the original image, transitively (memoized). An indirect call, an import, a function
not in the index, or a recursion cycle counts as killing every volatile register (and, for entry registers, as
reading eax, ecx and edx).
"""
import capstone
from capstone import x86_const as X

from func import image, code_end

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True

VOLATILE = ["eax", "ecx", "edx"] + ["xmm%d" % k for k in range(8)]
ALL_VOL = frozenset(VOLATILE)
SAVED = {"ebx", "esi", "edi", "ebp"}
ALLREGS = {"eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"} | {"xmm%d" % k for k in range(8)}
FAM = {}
for f, parts in (("eax", "al ah ax"), ("ecx", "cl ch cx"), ("edx", "dl dh dx"), ("ebx", "bl bh bx"),
                 ("esi", "si"), ("edi", "di"), ("ebp", "bp"), ("esp", "sp")):
    FAM[f] = f
    for p in parts.split():
        FAM[p] = f
for k in range(8):
    FAM["xmm%d" % k] = "xmm%d" % k
ZERO_IDIOM = ("xor", "sub", "pxor", "xorps", "xorpd")


def _regs(i):
    try:
        rd, wr = i.regs_access()
    except capstone.CsError:
        return set(), set()
    r = {FAM.get(i.reg_name(x)) for x in rd} - {None}
    w = {FAM.get(i.reg_name(x)) for x in wr} - {None}
    from rename_core import op_access          # capstone misses e.g. the register of `test [mem], reg`
    for k, o in enumerate(i.operands):
        if o.type == X.X86_OP_REG and op_access(i, o, k) & 1:
            r |= {FAM.get(i.reg_name(o.reg))} - {None}
    if i.mnemonic in ZERO_IDIOM and len(i.operands) == 2 and all(o.type == X.X86_OP_REG for o in i.operands) \
            and i.operands[0].reg == i.operands[1].reg:
        r -= {FAM.get(i.reg_name(i.operands[0].reg))}
    if i.mnemonic == "push" and i.operands and i.operands[0].type == X.X86_OP_REG \
            and FAM.get(i.reg_name(i.operands[0].reg)) in SAVED:
        r -= {FAM.get(i.reg_name(i.operands[0].reg))}      # saving a callee-saved register is not a use
    return r, w


def _call_target(i):
    """('direct', va) | ('indirect', None) | None"""
    if i.mnemonic == "call" or (i.mnemonic == "jmp" and i.operands and i.operands[0].type == X.X86_OP_IMM):
        op = i.operands[0]
        if op.type == X.X86_OP_IMM:
            return ("direct", op.imm & 0xffffffff)
        if i.mnemonic == "call":
            return ("indirect", None)
    return None


_code_memo = {}


def retail_code(va):
    """(code, lo) of the original function at va, or None when va is not an indexed function start."""
    if va in _code_memo:
        return _code_memo[va]
    img = image()
    res = None
    try:
        lo, hi = img.func_extent(va)
        if lo == va and img.in_text(va):
            code = bytes(img.read(lo, hi - lo))
            first = next(MD.disasm(code[:16], lo, 1), None)
            if first is not None and first.mnemonic == "jmp" and first.operands[0].type == X.X86_OP_MEM:
                res = None                              # an import thunk: jmp [IAT]
            else:
                ins = list(MD.disasm(code, lo))
                res = (code[:code_end(ins, lo, hi) - lo], lo)
    except Exception:
        res = None
    _code_memo[va] = res
    return res


_kill_memo = {}


def retail_kill(va, _stack=None):
    """Kill set of the original function at va (frozenset of volatile registers)."""
    if va in _kill_memo:
        return _kill_memo[va]
    _stack = _stack or set()
    if va in _stack:
        return ALL_VOL
    rc = retail_code(va)
    if rc is None:
        return ALL_VOL
    code, lo = rc
    out = set()
    _stack = _stack | {va}
    for i in MD.disasm(code, va):
        _, w = _regs(i)
        out |= w & ALL_VOL
        t = _call_target(i)
        if t:
            if t[0] == "indirect":
                out = set(ALL_VOL)
                break
            if not va <= t[1] < va + len(code):
                out |= retail_kill(t[1], _stack)
        if len(out) == len(ALL_VOL):
            break
    res = frozenset(out)
    _kill_memo[va] = res              # a cycle counted as ALL_VOL: an over-approximation, safe to keep
    return res


def calls(f):
    """[(offset, ('direct', target) | ('indirect', None) | ('sym', name))] in address order."""
    out = []
    for i in MD.disasm(f.code, f.va):
        t = _call_target(i)
        if not t:
            continue
        off = i.address - f.va
        tok = f.relocs.get(off + i.size - 4)
        if t[0] == "direct" and tok is not None:
            if tok[0] == "sym":
                t = ("sym", tok[1])
            elif f.va <= t[1] < f.va + len(f.code):
                continue
            else:
                t = ("direct", tok[1])
        elif t[0] == "direct" and f.va <= t[1] < f.va + len(f.code):
            continue
        out.append((off, t))
    return out


def own_writes(f):
    out = set()
    for i in MD.disasm(f.code, f.va):
        out |= _regs(i)[1] & ALL_VOL
    return out


def entry_regs(f, callee_entry=None):
    """Registers read before written on some path from entry (may-read analysis over the CFG). callee_entry(i) ->
    the registers a call/tail jump i reads at its target (for the original's transitive facts); None: calls read
    nothing."""
    ins = list(MD.disasm(f.code, f.va))
    if not ins:
        return set()
    idx = {i.address: k for k, i in enumerate(ins)}
    succ = {}
    for k, i in enumerate(ins):
        m = i.mnemonic
        nxt = ins[k + 1].address if k + 1 < len(ins) else None
        tgt = None
        if m.startswith("j") and i.operands and i.operands[0].type == X.X86_OP_IMM:
            tgt = i.operands[0].imm & 0xffffffff
        if m == "jmp":
            s = [tgt] if tgt in idx else []
        elif m.startswith("j"):
            s = ([tgt] if tgt in idx else []) + ([nxt] if nxt is not None else [])
        elif m.startswith("ret") or m == "int3":
            s = []
        else:
            s = [nxt] if nxt is not None else []
        succ[k] = [idx[a] for a in s if a in idx]
    defined = {0: frozenset()}
    work = [0]
    reads = set()
    while work:
        k = work.pop()
        d = defined[k]
        i = ins[k]
        r, w = _regs(i)
        reads |= (r & ALLREGS) - d
        t = _call_target(i)
        if t and callee_entry is not None and not (t[0] == "direct" and f.va <= t[1] < f.va + len(f.code)):
            reads |= (set(callee_entry(t)) & ALLREGS) - d
        if i.mnemonic == "call":
            w = w | ALL_VOL
        nd = d | (w & ALLREGS)
        for s in succ[k]:
            old = defined.get(s)
            new = nd if old is None else (old & nd)
            if old is None or new != old:
                defined[s] = frozenset(new)
                work.append(s)
    return reads


_entry_memo = {}
_UNKNOWN_ENTRY = frozenset({"eax", "ecx", "edx"})


def retail_entry(va, _stack=frozenset()):
    """Registers the original function at va reads before writing them, following its calls and tail jumps."""
    if va in _entry_memo:
        return _entry_memo[va]
    if va in _stack or len(_stack) > 40:
        return _UNKNOWN_ENTRY
    rc = retail_code(va)
    if rc is None:
        return _UNKNOWN_ENTRY
    from func import Func
    code, lo = rc
    st = _stack | {va}

    def ce(t):
        if t[0] == "indirect":
            return {"ecx", "edx"}
        return retail_entry(t[1], st)
    res = frozenset(entry_regs(Func(va, code), ce))
    _entry_memo[va] = res
    return res


def facts(f, retail_side=None):
    """{'kill': sorted, 'entry': sorted, 'calls': n}. For our side, pass the original Func: our k-th call is the
    original's k-th call (same order), so the callee kill sets come from the original targets."""
    cs = calls(f)
    kill = set(own_writes(f))
    ref = calls(retail_side) if retail_side is not None else cs
    if len(ref) != len(cs):
        kill = set(ALL_VOL)
    else:
        for _off, t in ref:
            kill |= retail_kill(t[1]) if t[0] == "direct" else ALL_VOL
    return {"kill": sorted(kill), "entry": sorted(entry_regs(f)), "calls": len(cs)}


def same(retail, ours):
    """(True, why, facts) when ours can stand in for the original in every caller that knows its register use."""
    a = facts(retail)
    b = facts(ours, retail)
    if a["calls"] != b["calls"]:
        return False, "the original makes %d calls, ours %d" % (a["calls"], b["calls"]), a
    if a["kill"] != b["kill"]:
        return False, "kill set differs: original %s, ours %s" % (a["kill"], b["kill"]), a
    if a["entry"] != b["entry"]:
        return False, "entry registers differ: original %s, ours %s" % (a["entry"], b["entry"]), a
    return True, "", a


def std_call_safe(target_va):
    """A call may be modelled with the standard convention (callee reads its arguments, writes eax/ecx/edx) only
    when the callee really kills eax, ecx and edx; otherwise a caller may keep a value in one of them across it."""
    return {"eax", "ecx", "edx"} <= retail_kill(target_va)


_pop_memo = {}


def callee_pop(va):
    """Bytes the original function at va removes on return (its first `ret N`), None when unknown (import thunk,
    not an indexed function, a function without a ret)."""
    if va in _pop_memo:
        return _pop_memo[va]
    rc = retail_code(va)
    n = None
    if rc is not None:
        code, lo = rc
        rets = set()
        for i in MD.disasm(code, lo):
            if i.mnemonic in ("ret", "retn"):
                rets.add(i.operands[0].imm if i.operands else 0)
        if len(rets) == 1:
            n = rets.pop()
    _pop_memo[va] = n
    return n
