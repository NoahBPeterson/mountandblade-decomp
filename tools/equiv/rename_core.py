#!/usr/bin/env python3
"""Register-renaming + scheduling + frame-slot-renaming equivalence checker for two x86-32 functions
(A = the original, B = ours).

Ported from spore-decomp tools/equiv/rename_core.py (commit aaaf5c79f; unchanged there since), extended for M&B's
non-/LTCG cl 13.10 code with a FRAME MODEL. Everything of the original rule stands:

ACCEPT means: the two functions have the same CFG (blocks in the same layout order, the same successors), and a
bijection pi between the instructions of each pair of corresponding blocks such that
  * each instruction and its image have the same operation and operand shape, registers abstracted (a renameable
    register operand is "the j-th distinct register family of this instruction", with its part: l8/h8/16/32/xmm);
    implicit registers, fixed registers (st(i), segments), immediates and scales must be equal; relocated fields
    are symbols compared through one global bijection (plus the literal-constant check of func.const_ok); branch
    targets are block indices;
  * the def-use graph is the same under pi: for every use slot of every instruction (register families, the flag
    groups CF / OSZAP / DF, the x87 state FPU, MXCSR, and -- new -- frame bytes), the set of definitions that reach
    it in A maps under pi onto the set that reaches the corresponding slot in B. Entry values are pinned;
  * memory operations keep their relative order in each block;
  * pinned uses model the ABI at calls and rets.

FRAME MODEL (new). esp is not a register here: its value is tracked statically as the DEPTH below the entry esp at
every instruction (push/pop, add/sub esp, imm, call minus the bytes the callee pops, __chkstk plus its eax). The
depth must agree at every join, be 0 at every ret and tail jump, and nothing may access the stack below esp; any
other write of esp (leave, mov esp, and esp...) is rejected. A stack operand [esp+disp] then names the entry-relative
offset k = disp - depth, so the same slot is recognised at different depths:
  * k >= 0 (the return address and the incoming arguments): ordinary memory, compared by k exactly (pinned);
  * k < 0 (the function's own frame): a FRAME SLOT. Two modes, for the whole function pair:
    - slots mode (no frame address escapes in either function: no lea of a frame slot, no esp-indexed operand, no
      esp read as a value): every frame byte is a resource of the def-use graph (resource ("s", k)); a slot operand
      of size n uses/defines the n bytes k..k+n-1 with keys naming the byte index inside the operand, push defines
      the 4 bytes below the old esp, pop uses the 4 at esp. Slot accesses are NOT memory operations (they can be
      reordered like register instructions) and their offsets are abstracted out of the shape. So the slot
      renaming is per value (like a register web), and overlap is respected automatically: a read of bytes 4..7
      of a qword store must read bytes 4..7 of the corresponding store. A call uses (and clobbers) the bytes of
      its outgoing arguments POSITIONALLY relative to its esp: the N bytes at esp..esp+N, where N is the bytes the
      callee pops (`ret N`, from the original callee), or the argument bytes its decorated name gives
      (callconv.py), or, when neither is known, the whole frame (both functions must then have the same depth).
      A frame byte nobody wrote before reading holds an undefined value, the same "undef" in both functions;
    - delta mode (some frame address escapes): frame operands stay memory operations (ordered), and every frame
      offset of B must be A's plus one global constant D (frame operands, frame addresses taken by lea / mov r,
      esp, the slot a push writes, a pop reads and a call's return address): one translation of the whole frame.
  * `add/sub esp, imm` match whatever their immediates (the depth check covers them); their flags must be dead.
    `mov eax, N` right before a __chkstk call matches any N; that eax may only be read by the __chkstk call.
  * callee-saved registers: a push of ebx/esi/edi/ebp whose value is the register's entry value (a save) matches
    a save of another callee-saved register; at a ret, a callee-saved register that holds its entry value in both
    functions (untouched, or popped from its own save; checked per function over the def-use graph, slots mode
    only) is not compared between them.
x87: st(i) are fixed operands, the x87 state is one pinned resource (FPU) every x87 instruction reads and writes.
Calls: the k-th call of A is paired with the k-th call of B. A call reads, besides the frame bytes above, the
registers of its convention from B's decorated callee name (thiscall ecx, fastcall ecx+edx, cdecl/stdcall none,
__chkstk eax, indirect ecx+edx), plus the registers the ORIGINAL callee reads before writing them (facts.entry_regs:
cl's same-TU custom conventions pass arguments in eax), and writes eax/ecx/edx, flags, FPU -- the standard model,
used only where the original callee really kills eax, ecx and edx (facts.std_call_safe); otherwise it reads and
writes every register.

Levels: 1 = ABI outputs (ret reads eax, ebx, esi, edi, ebp, FPU; ret64 adds edx);
        2 = all registers (ret reads every GPR and FPU; every call reads every GPR).
"""
import sys
import capstone
from capstone import x86_const as X

import callconv

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True

GPR = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
FAM = {}
for f, l8, h8, w in (("eax", "al", "ah", "ax"), ("ecx", "cl", "ch", "cx"), ("edx", "dl", "dh", "dx"),
                     ("ebx", "bl", "bh", "bx")):
    FAM.update({f: (f, "32"), l8: (f, "l8"), h8: (f, "h8"), w: (f, "16")})
for f, w in (("esi", "si"), ("edi", "di"), ("ebp", "bp"), ("esp", "sp")):
    FAM.update({f: (f, "32"), w: (f, "16")})
for k in range(8):
    FAM["xmm%d" % k] = ("xmm%d" % k, "xmm")
RENAMEABLE = {"eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"} | {"xmm%d" % k for k in range(8)}
XMM = ["xmm%d" % k for k in range(8)]
STATUS = ["CF", "PF", "AF", "ZF", "SF", "OF"]
CALLEE_SAVED = ("ebx", "esi", "edi", "ebp")


def _bits(prefix, names):
    return sum(getattr(X, "X86_EFLAGS_%s_%s" % (prefix, n), 0) for n in names)


F_TEST = _bits("TEST", STATUS + ["DF"])
F_WR = {n: _bits("MODIFY", [n]) | _bits("SET", [n]) | _bits("RESET", [n]) | _bits("UNDEFINED", [n]) for n in STATUS + ["DF"]}
IMPLICIT_MEM = ("push", "pop", "call", "ret", "leave", "enter", "movs", "stos", "lods", "cmps", "scas", "pushal",
                "popal", "pushfd", "popfd", "pushf", "popf")
SHIFTS = ("shl", "shr", "sar", "sal", "rol", "ror", "rcl", "rcr", "shld", "shrd")
FULL_XMM = ("movaps", "movups", "movapd", "movupd", "movdqa", "movdqu", "movd", "movq")
DELTA_TAGS = ("fslot", "faddr", "fpush", "fpop", "fcall")
CHKSTK = ("__chkstk", "__alloca_probe", "_chkstk", "_alloca_probe")

CANON_TEST = False      # optional peephole: test r32, imm<0x80 == test r8, imm
STD_SAFE = None         # callable(original callee va) -> True when the standard call model is sound (facts.py)
ENTRY_REGS = None       # callable(original callee va) -> registers the callee reads before writing (facts.py)
CALLEE_POP = None       # callable(original callee va) -> bytes the callee pops (its ret N), None if unknown
TOKEN_OK = None         # callable(token A, token B) -> False when a pair is known wrong (func.const_ok)
FRAME_SLOTS = True      # False: frame operands are compared by offset only (delta mode with D = 0 always)
CONV_MISMATCH = []      # (callee name, the original callee's ret N, callconv's pops) where they disagree


class Reject(Exception):
    pass


X87_STORE = {"fst", "fstp", "fist", "fistp", "fisttp", "fbstp", "fnstcw", "fstcw", "fnstsw", "fstsw", "fnstenv",
             "fstenv", "fnsave", "fsave"}


def op_access(i, op, idx=None):
    """capstone 5's operand access with its known errors fixed: x87 stores report their memory operand as read,
    `test r/m, r` reports neither operand, `test eax, imm` (a9) reports eax written, a shift count in cl (shld/shrd)
    is reported as 0, cmpxchg's destination as read only. Unknown (0): read for a register, read+write for memory."""
    m = i.mnemonic
    if m in ("test", "cmp", "bt"):
        return 1
    if m.startswith("f") and op.type == X.X86_OP_MEM:
        return 2 if m in X87_STORE else 1
    if m in ("cmpxchg", "xadd", "xchg") and idx == 0:
        return 3
    a = op.access
    if not a:
        return 1 if op.type == X.X86_OP_REG else 3
    return a


class Ins:
    __slots__ = ("addr", "off", "size", "text", "mnem", "shape", "uses", "defs", "mem", "kind", "target", "rd",
                 "depth", "poison", "poison_reader", "chk", "cs", "save", "relaxed", "comm")


def s32(v):
    v &= 0xffffffff
    return v - (1 << 32) if v & 0x80000000 else v


def _is_esp(i, op):
    return op.type == X.X86_OP_REG and i.reg_name(op.reg) == "esp"


def _esp_adjust(i):
    """add/sub esp, imm and lea esp, [esp + disp] -> signed bytes pushed (positive = deeper), else None."""
    if i.mnemonic in ("add", "sub") and len(i.operands) == 2 and _is_esp(i, i.operands[0]) and \
            i.operands[1].type == X.X86_OP_IMM:
        v = s32(i.operands[1].imm)
        return v if i.mnemonic == "sub" else -v
    if i.mnemonic == "lea" and _is_esp(i, i.operands[0]) and i.operands[1].mem.base and \
            i.reg_name(i.operands[1].mem.base) == "esp" and not i.operands[1].mem.index:
        return -s32(i.operands[1].mem.disp)          # lea esp, [esp]: cl's alignment no-op
    return None


def _padding(i):
    """cl's alignment filler between blocks: lea esp, [esp]; lea r, [r]; mov r, r; nop."""
    if i.mnemonic == "nop":
        return True
    if i.mnemonic in ("lea", "mov") and len(i.operands) == 2 and i.operands[0].type == X.X86_OP_REG:
        r = i.operands[0].reg
        o = i.operands[1]
        if o.type == X.X86_OP_REG:
            return o.reg == r
        return o.mem.base == r and not o.mem.index and o.mem.disp == 0 and i.mnemonic == "lea"
    return False


BIG = 0x100000          # depth of the region below an `and esp, -2^n` (an ebp frame's aligned area): its own origin


def frame_bytes(d):
    return d - BIG if d >= BIG else d


def _is(i, k, name):
    return len(i.operands) > k and i.operands[k].type == X.X86_OP_REG and i.reg_name(i.operands[k].reg) == name


def is_mov_ebp_esp(i):
    return i.mnemonic == "mov" and _is(i, 0, "ebp") and _is(i, 1, "esp")


def is_mov_esp_ebp(i):
    return i.mnemonic == "mov" and _is(i, 0, "esp") and _is(i, 1, "ebp")


def is_and_esp(i):
    return i.mnemonic == "and" and _is(i, 0, "esp") and len(i.operands) == 2 and i.operands[1].type == X.X86_OP_IMM


def disasm_all(code, base):
    """capstone, plus the WinLicense region markers WL_MARKER emits (`jmp +0x10` over "WL  " id 0 "WL  "): the 16
    data bytes are skipped. -> (instructions, {address of the jmp: data bytes})"""
    out, wl, pos = [], {}, 0
    while pos < len(code):
        i = next(MD.disasm(code[pos:pos + 16], base + pos, 1), None)
        if i is None:
            raise Reject("disassembly stops at +%#x (data in code?)" % pos)
        out.append(i)
        pos += i.size
        if i.bytes[:2] == b"\xeb\x10" and code[pos:pos + 4] == b"WL  " and code[pos + 12:pos + 16] == b"WL  ":
            wl[i.address] = bytes(code[pos:pos + 16])
            pos += 16
    return out, wl


# ---- the frame pass: static esp depth, escapes --------------------------------------------------------------------
def frame_pass(raw, base, size, calls):
    """-> (depth list: bytes below the entry esp before each instruction, None where unreachable;
           escapes: True when a frame address can reach a register or memory)."""
    n = len(raw)
    hi = base + size
    idx = {i.address: k for k, i in enumerate(raw)}
    callno, c = {}, 0
    for k, i in enumerate(raw):
        if i.mnemonic == "call":
            callno[k] = c
            c += 1
    depth = [None] * n
    depth[0] = 0
    work = [0]
    escapes = False
    ebp0 = None                                      # depth at `mov ebp, esp` (an ebp frame)
    ebpframe = any(is_mov_ebp_esp(i) for i in raw)
    while work:
        k = work.pop()
        d = depth[k]
        i = raw[k]
        m = i.mnemonic
        nd = d
        succ = [k + 1] if k + 1 < n else []
        adj = _esp_adjust(i)
        if m in ("pushal", "pusha", "popal", "popa", "leave", "enter"):
            raise Reject("+%#x %s: not handled by the frame model" % (i.address - base, m))
        if ebpframe:
            try:
                _r0, w0 = i.regs_access()
            except capstone.CsError:
                w0 = []
            if any(i.reg_name(r) in ("ebp", "bp") for r in w0) and not is_mov_ebp_esp(i) and \
                    not (m == "pop" and _is(i, 0, "ebp")):
                raise Reject("+%#x %s %s: ebp written in an ebp-frame function" % (i.address - base, m, i.op_str))
            if m == "pop" and _is(i, 0, "ebp") and not (k + 1 < n and raw[k + 1].mnemonic in ("ret", "retn")):
                raise Reject("+%#x pop ebp not right before a ret" % (i.address - base))
            for j, op in enumerate(i.operands):
                if op.type == X.X86_OP_REG and i.reg_name(op.reg) in ("ebp", "bp") and op_access(i, op, j) & 1 and \
                        not is_mov_esp_ebp(i) and not (m == "push" and ebp0 is None):
                    raise Reject("+%#x %s %s: the frame pointer used as a value" % (i.address - base, m, i.op_str))
                if op.type == X.X86_OP_MEM and op.mem.base and i.reg_name(op.mem.base) == "ebp":
                    if ebp0 is None or op.mem.index or s32(op.mem.disp) - ebp0 < 0:
                        raise Reject("+%#x %s %s: an ebp-frame access outside the incoming arguments"
                                     % (i.address - base, m, i.op_str))
        if is_mov_ebp_esp(i):
            if d >= BIG or (ebp0 is not None and ebp0 != d):
                raise Reject("+%#x mov ebp, esp at another depth" % (i.address - base))
            ebp0 = d
        elif is_mov_esp_ebp(i):
            if ebp0 is None:
                raise Reject("+%#x mov esp, ebp without an ebp frame" % (i.address - base))
            nd = ebp0
        elif is_and_esp(i):
            v = s32(i.operands[1].imm)
            if ebp0 is None or d >= BIG or v >= 0 or (-v) & (-v - 1):
                raise Reject("+%#x and esp, %#x outside an ebp frame" % (i.address - base, v & 0xffffffff))
            nd = BIG
        elif m == "push" or m.startswith("pushf"):
            nd = d + (i.operands[0].size if i.operands else 4)
            if i.operands and _is_esp(i, i.operands[0]):
                escapes = True
        elif m == "pop" or m.startswith("popf"):
            if i.operands and (_is_esp(i, i.operands[0]) or (i.operands[0].type == X.X86_OP_MEM and
                                                             i.operands[0].mem.base and i.reg_name(i.operands[0].mem.base) == "esp")):
                raise Reject("+%#x %s %s: pop into esp / an esp-based slot" % (i.address - base, m, i.op_str))
            nd = d - (i.operands[0].size if i.operands else 4)
        elif adj is not None:
            nd = d + adj
        elif m == "call":
            info = calls[callno[k]]
            if info["chk"]:
                p = raw[k - 1] if k else None
                if not (p is not None and p.mnemonic == "mov" and len(p.operands) == 2 and
                        p.reg_name(p.operands[0].reg) == "eax" and p.operands[1].type == X.X86_OP_IMM):
                    raise Reject("+%#x: __chkstk without mov eax, N before it" % (i.address - base))
                nd = d + s32(p.operands[1].imm)
            else:
                nd = d - info["pop"]
        elif m in ("ret", "retn"):
            if d != 0:
                raise Reject("+%#x ret at stack depth %d (a callee's argument bytes guessed wrong?)" % (i.address - base, d))
            succ = []
        elif m == "jmp" or (m.startswith("j") and m != "jmp") or m in ("loop", "loope", "loopne", "jecxz"):
            op = i.operands[0]
            if op.type != X.X86_OP_IMM:
                succ = []                           # an indirect jump: rejected later (switch tables)
            else:
                t = op.imm & 0xffffffff
                inside = base <= t < hi
                if inside and t not in idx:
                    raise Reject("branch at +%#x goes into the middle of an instruction" % (i.address - base))
                if m == "jmp":
                    succ = [idx[t]] if inside else []
                    if not inside and d != 0:
                        raise Reject("+%#x tail jump at stack depth %d" % (i.address - base, d))
                elif inside:
                    succ = succ + [idx[t]]
        elif m in ("int3", "ud2", "hlt", "int"):
            succ = []
        else:
            try:
                _r, w = i.regs_access()
            except capstone.CsError:
                w = []
            if any(i.reg_name(r) in ("esp", "sp") for r in w):
                raise Reject("+%#x %s %s writes esp" % (i.address - base, m, i.op_str))
        # escapes and stack operands
        for op in i.operands:
            if op.type == X.X86_OP_REG and i.reg_name(op.reg) == "esp" and adj is None and m not in ("push", "pop") \
                    and not (is_mov_ebp_esp(i) or is_mov_esp_ebp(i) or is_and_esp(i)):
                escapes = True                       # esp read as a value (mov r, esp; mov fs:[0], esp)
            if op.type == X.X86_OP_MEM and op.mem.base and i.reg_name(op.mem.base) == "esp":
                if op.mem.index:
                    escapes = True
                    continue
                disp = s32(op.mem.disp)
                kk = disp - d
                if m == "lea":
                    if kk < 0:
                        escapes = True
                    continue
                if disp < 0:
                    raise Reject("+%#x %s %s: below esp" % (i.address - base, m, i.op_str))
                if kk < 0 < kk + op.size:
                    raise Reject("+%#x %s %s: a frame access that reaches the return address" % (i.address - base, m, i.op_str))
                if d >= BIG and kk >= -BIG:
                    raise Reject("+%#x %s %s: above the aligned area of an ebp frame" % (i.address - base, m, i.op_str))
        for s in succ:
            if depth[s] is None:
                depth[s] = nd
                work.append(s)
            elif depth[s] != nd:
                raise Reject("stack depth differs at +%#x (%d vs %d)" % (raw[s].address - base, depth[s], nd))
    for k, i in enumerate(raw):
        if depth[k] is None and not _padding(i):
            try:
                r, w = i.regs_access()
            except capstone.CsError:
                r, w = [], []
            if any(i.reg_name(x) in ("esp", "sp") for x in list(r) + list(w)):
                raise Reject("+%#x: unreachable code uses the stack" % (i.address - base))
    return depth, escapes, ebp0


# ---- call facts ---------------------------------------------------------------------------------------------------
def heuristic_pop(raw, k):
    """Bytes an unknown callee pops: 0 when the caller cleans up right after (cdecl: add esp / pop ecx), else the bytes
    pushed since the previous call (stdcall/thiscall, the COM and Win32 convention). A wrong guess shows as a stack
    depth that disagrees at a join or is not 0 at a ret (rejected)."""
    nx = raw[k + 1] if k + 1 < len(raw) else None
    if nx is not None and nx.mnemonic == "pop" and nx.op_str == "ecx":
        return 0
    if nx is not None and _esp_adjust(nx) is not None and _esp_adjust(nx) < 0:
        return 0
    pro = 0                                      # the prologue's saves and frame allocation are not arguments
    while pro < len(raw) and ((raw[pro].mnemonic == "push" and raw[pro].operands[0].type == X.X86_OP_REG and
                               raw[pro].reg_name(raw[pro].operands[0].reg) in CALLEE_SAVED) or
                              _esp_adjust(raw[pro]) is not None or is_mov_ebp_esp(raw[pro]) or is_and_esp(raw[pro])):
        pro += 1
    pend = 0
    for j in range(k - 1, pro - 1, -1):
        p = raw[j]
        if p.mnemonic == "call":
            break
        if p.mnemonic == "push":
            if p.operands[0].type == X.X86_OP_REG and p.reg_name(p.operands[0].reg) in CALLEE_SAVED and \
                    not _written_before(raw, j, p.reg_name(p.operands[0].reg)):
                continue                         # a late save of a callee-saved register, not an argument
            pend += p.operands[0].size
        elif p.mnemonic == "pop":
            pend -= p.operands[0].size
    return max(pend, 0)


def _written_before(raw, j, reg):
    for i in raw[:j]:
        try:
            _r, w = i.regs_access()
        except capstone.CsError:
            return True
        if any(FAM.get(i.reg_name(x), ("",))[0] == reg for x in w):
            return True
    return False


def call_infos(rawA, baseA, relA, rawB, baseB, relB, sizeA):
    """One record per call pair: chk, pop (bytes the callee removes), args (stack bytes it reads, None = unknown),
    the original's target va and B's callee token."""
    ca = [k for k, i in enumerate(rawA) if i.mnemonic == "call"]
    cb = [k for k, i in enumerate(rawB) if i.mnemonic == "call"]
    if len(ca) != len(cb):
        raise Reject("%d calls in A, %d in B" % (len(ca), len(cb)))
    out = []
    for ka, kb in zip(ca, cb):
        ia, ib = rawA[ka], rawB[kb]
        ta = ia.operands[0].imm & 0xffffffff if ia.operands[0].type == X.X86_OP_IMM else None
        tokb = relB.get(ib.address - baseB + ib.size - 4) if ib.operands[0].type == X.X86_OP_IMM else None
        if tokb is None and ib.operands[0].type == X.X86_OP_MEM:
            tokb = relB.get(ib.address - baseB + ib.disp_offset) if ib.disp_offset else None   # call [__imp_X]
        name = tokb[1] if tokb and tokb[0] == "sym" else None
        nm = name[6:] if name and name.startswith("__imp_") else name
        chk = bool(nm) and nm.startswith(CHKSTK)
        info = callconv.parse(nm) if nm else None
        pop = None
        if ta is not None and CALLEE_POP is not None and not (baseA <= ta < baseA + sizeA):
            pop = CALLEE_POP(ta)
        if pop is not None and info is not None and pop != info["pops"]:
            CONV_MISMATCH.append((name, pop, info["pops"]))     # the parser's own check (callee pops vs name)
        if pop is None and info is not None:
            pop = info["pops"]
        if pop is None:
            pop = heuristic_pop(rawA, ka)
        args = None
        if chk or (nm and nm.startswith(("__ftol", "__CI", "__ftoui", "__dtoui", "__ultod"))):
            args = 0
        elif info is not None:
            args = max(info["args"], pop)
        elif pop > 0:
            args = pop
        out.append({"chk": chk, "pop": pop, "args": args, "ta": ta, "tokb": tokb, "name": name})
    return out


# ---- decoding -----------------------------------------------------------------------------------------------------
def decode(code, base, relocs, level, ret64, calls, mode_hint=None):
    raw = list(MD.disasm(code, base))
    if sum(x.size for x in raw) != len(code):
        raise Reject("disassembly stops at +%#x (data in code?)" % sum(x.size for x in raw))
    depth, escapes = frame_pass(raw, base, len(code), calls)
    return raw, depth, escapes


def build(raw, base, relocs, level, ret64, calls, depth, mode, ebp0=None, wl=None):
    out = []
    c = 0
    for k, i in enumerate(raw):
        info = None
        if i.mnemonic == "call":
            info = calls[c]
            c += 1
        x = _one(i, base, relocs, depth[k], mode, info, ebp0)
        if wl and i.address in wl:
            x.shape.append(("wl", wl[i.address]))
        out.append(x)
    # __chkstk: the frame size in eax may differ
    for k, x in enumerate(out):
        if x.kind == "call" and x.chk and k:
            p = out[k - 1]
            p.shape = [s if not (isinstance(s, tuple) and s and s[0] == "imm") else ("frameimm",) for s in p.shape]
            p.poison = {"eax"}
            x.poison_reader = True
    lo, hi = base, base + sum(i.size for i in raw)
    for x in out:
        _retmodel(x, level, ret64, lo, hi)
    return out


def _one(i, base, relocs, d, mode, callinfo, ebp0=None):
    x = Ins()
    x.addr, x.off, x.size, x.mnem = i.address, i.address - base, i.size, i.mnemonic
    x.text = "%s %s" % (i.mnemonic, i.op_str)
    x.target = None
    x.depth = d
    x.poison, x.poison_reader, x.chk, x.cs, x.save, x.relaxed = set(), False, False, i, False, False
    x.comm = None
    mn = i.mnemonic
    prefix = tuple(p for p in i.prefix if p)
    fields = {}
    for o, n in ((i.disp_offset, i.disp_size), (i.imm_offset, i.imm_size)):
        if o and n == 4 and (x.off + o) in relocs:
            tok = relocs[x.off + o]
            if 0x64 in i.prefix and o == i.disp_offset and tok[0] == "abs" and tok[1] < 0x1000:
                continue
            fields[o] = tok
    local = []
    slots_mode = mode == "slots"
    uses, defs = [], []

    def reg(r):
        nm = i.reg_name(r)
        if nm in FAM and FAM[nm][0] in RENAMEABLE:
            f, part = FAM[nm]
            if f not in local:
                local.append(f)
            return ("R", local.index(f), part), f, part
        return ("P", nm), None, None

    def slot_bytes(key, k0, n, use, dfn):
        for j in range(n):
            if use:
                uses.append(((key, j), ("s", k0 + j)))
            if dfn:
                defs.append(((key, j), ("s", k0 + j)))

    shape = [mn, prefix]
    acc = {}
    fixed_r, fixed_w = set(), set()
    has_mem = False
    narrow = None
    adj = _esp_adjust(i)
    if adj is not None:
        shape.append(("espadj",))
        x.relaxed = True
        x.poison = {"CF", "OSZAP"}
    elif is_mov_esp_ebp(i):
        shape.append(("esprestore",))
        adj = 0
    elif is_and_esp(i):
        shape.append(("espalign", i.operands[1].imm & 0xffffffff))
        x.poison = {"CF", "OSZAP"}
        adj = 0
    if CANON_TEST and mn == "test" and len(i.operands) == 2 and i.operands[0].type == X.X86_OP_REG and \
            i.operands[1].type == X.X86_OP_IMM and 0 <= i.operands[1].imm < 0x80 and \
            i.reg_name(i.operands[0].reg) in ("eax", "ecx", "edx", "ebx", "al", "cl", "dl", "bl"):
        narrow = True
    for opi, op in enumerate(i.operands if adj is None else []):
        if op.type == X.X86_OP_REG:
            nm = i.reg_name(op.reg)
            if nm == "esp":
                if is_mov_ebp_esp(i):
                    shape.append(("ebpset",))
                    continue
                # only in delta mode (anything else is rejected by the frame pass or is an esp adjustment)
                shape.append(("faddr", -d, "esp"))
                continue
            tok, f, part = reg(op.reg)
            if narrow:
                tok = (tok[0], tok[1], "l8")
            shape.append(("reg", tok, 1 if narrow else op.size))
            if f:
                s = acc.setdefault(f, set())
                oa = op_access(i, op, opi)
                if oa & 1:
                    s.add("r")
                if oa & 2:
                    full_x = part == "xmm" and oa == 2 and (mn in FULL_XMM or (
                        mn in ("movss", "movsd") and any(o.type == X.X86_OP_MEM for o in i.operands)))
                    s.add("w" if (part == "32" or full_x) else "pw")
            else:
                (fixed_r.add(nm) if op_access(i, op, opi) & 1 else None)
                (fixed_w.add(nm) if op_access(i, op, opi) & 2 else None)
        elif op.type == X.X86_OP_IMM:
            if i.imm_offset in fields:
                shape.append(("sym", fields[i.imm_offset]))
            else:
                shape.append(("imm", op.imm & 0xffffffff, 1 if narrow else op.size))
        elif op.type == X.X86_OP_MEM:
            m = op.mem
            bn = i.reg_name(m.base) if m.base else None
            acc_rw = op_access(i, op, opi)
            if bn == "esp" and m.index:
                # an indexed frame array (delta mode only: the frame pass counts it as an escape)
                if d is None:
                    raise Reject("stack operand in unreachable code at +%#x" % x.off)
                k = s32(m.disp) - d
                ix = reg(m.index)[0]
                nm = i.reg_name(m.index)
                if nm in FAM and FAM[nm][0] in RENAMEABLE:
                    acc.setdefault(FAM[nm][0], set()).add("r")
                if mn == "lea":
                    shape.append(("argaddrix", k, ix, m.scale) if k >= 0 else ("faddr", k, "lea", ix, m.scale))
                else:
                    has_mem = True
                    shape.append(("stkix", k, op.size, op.access, ix, m.scale) if k >= 0 else
                                 ("fslot", k, op.size, op.access, ix, m.scale))
                continue
            if (bn == "esp" or (bn == "ebp" and ebp0 is not None)) and not m.index:
                if d is None:
                    raise Reject("stack operand in unreachable code at +%#x" % x.off)
                disp = s32(m.disp)
                k = disp - (d if bn == "esp" else ebp0)
                if mn == "lea":
                    shape.append(("argaddr", k) if k >= 0 else ("faddr", k, "lea"))
                    continue
                if k >= 0:
                    has_mem = True
                    shape.append(("stk", k, op.size, op.access))
                    continue
                if slots_mode:
                    shape.append(("slot", op.size, op.access))
                    slot_bytes(("m", opi), k, op.size, bool(acc_rw & 1), bool(acc_rw & 2))
                else:
                    has_mem = True
                    shape.append(("fslot", k, op.size, op.access))
                continue
            has_mem = True
            b = reg(m.base)[0] if m.base else None
            ix = reg(m.index)[0] if m.index else None
            for r in (m.base, m.index):
                if r:
                    nm = i.reg_name(r)
                    if nm in FAM and FAM[nm][0] in RENAMEABLE:
                        acc.setdefault(FAM[nm][0], set()).add("r")
                    elif nm != "esp":
                        fixed_r.add(nm)
            disp = ("sym", fields[i.disp_offset]) if i.disp_offset in fields else ("imm", m.disp & 0xffffffff)
            seg = i.reg_name(m.segment) if m.segment else None
            shape.append(("mem", seg, b, ix, m.scale, disp, op.size, op.access))
            if m.scale == 1 and b and ix and b[0] == "R" and ix[0] == "R" and b[1] != ix[1] and b[2] == ix[2] == "32":
                x.comm = (b[1], ix[1])          # [r1 + r2*1] = [r2 + r1*1]
        else:
            raise Reject("unknown operand type at %#x" % x.addr)
    # implicit registers (esp is the frame model's, not a resource)
    rr, ww = i.regs_access()
    imp = []
    explicit = set()
    for op in i.operands:
        if op.type == X.X86_OP_REG:
            explicit.add(i.reg_name(op.reg))
        elif op.type == X.X86_OP_MEM:
            explicit |= {i.reg_name(r) for r in (op.mem.base, op.mem.index, op.mem.segment) if r}
    for r, md_ in [(r, "r") for r in rr] + [(r, "w") for r in ww]:
        nm = i.reg_name(r)
        if nm in ("eflags", "eip", "flags", "esp", "sp") or nm in explicit:
            continue
        if nm in FAM:
            f, part = FAM[nm]
            if f in local:
                s = acc.setdefault(f, set())
                s.add("r" if md_ == "r" else ("w" if part in ("32", "xmm") else "pw"))
                shape.append(("impx", nm, md_))
                continue
            if f in RENAMEABLE:
                imp.append((nm, md_))
            else:
                (fixed_r if md_ == "r" else fixed_w).add(nm)
        else:
            (fixed_r if md_ == "r" else fixed_w).add(nm)
    shape.append(("imp", tuple(sorted(set(imp)))))
    shape.append(("fixed", tuple(sorted(fixed_r)), tuple(sorted(fixed_w))))
    if mn in ("or", "and") and len(i.operands) == 2 and i.operands[0].type == X.X86_OP_REG and \
            i.operands[1].type == X.X86_OP_IMM and (i.operands[1].imm & 0xffffffff) == (0xffffffff if mn == "or" else 0):
        f, part = FAM.get(i.reg_name(i.operands[0].reg), (None, None))
        if f in acc and part == "32":
            acc[f] = {"w"}                       # or r, -1 / and r, 0: the result does not depend on r
    if mn in ("xor", "sub", "pxor", "xorps", "xorpd") and len(i.operands) == 2 and \
            all(o.type == X.X86_OP_REG for o in i.operands) and i.operands[0].reg == i.operands[1].reg:
        f = FAM.get(i.reg_name(i.operands[0].reg), (None,))[0]
        if f in acc and FAM[i.reg_name(i.operands[0].reg)][1] in ("32", "xmm"):
            acc[f] = {"w"}
    for j, f in enumerate(local):
        s = acc.get(f, set())
        if "r" in s or "pw" in s:
            uses.append((("e", j), f))
        if "w" in s or "pw" in s:
            defs.append((("e", j), f))
    for nm, md_ in sorted(set(imp)):
        f, part = FAM[nm]
        key = ("i", f)
        if md_ == "r" and (key, f) not in uses:
            uses.append((key, f))
        if md_ == "w":
            if (key, f) not in defs:
                defs.append((key, f))
            if part not in ("32",) and (key, f) not in uses:
                uses.append((key, f))
    for nm in sorted(fixed_r):
        f = FAM.get(nm, (nm,))[0]
        uses.append((("f", f), _fixres(f)))
    for nm in sorted(fixed_w):
        f = FAM.get(nm, (nm,))[0]
        defs.append((("f", f), _fixres(f)))
        if nm in FAM and FAM[nm][1] != "32" and (("f", f), f) not in uses:
            uses.append((("f", f), _fixres(f)))
    # the stack bytes push / pop / call touch
    if mn == "push" or mn.startswith("pushf"):
        n = i.operands[0].size if i.operands else 4
        if d is None:
            raise Reject("push in unreachable code at +%#x" % x.off)
        if slots_mode:
            slot_bytes("push", -(d + n), n, False, True)
        else:
            shape.append(("fpush", -(d + n)))
        # a save: push of a callee-saved register (whether its value is the entry value is checked on the graph)
        if i.operands and i.operands[0].type == X.X86_OP_REG and FAM.get(i.reg_name(i.operands[0].reg), ("",))[0] in CALLEE_SAVED:
            x.save = True
    elif mn == "pop" or mn.startswith("popf"):
        n = i.operands[0].size if i.operands else 4
        if slots_mode:
            slot_bytes("pop", -d, n, True, False)
        else:
            shape.append(("fpop", -d))
    elif mn == "call" and not slots_mode:
        shape.append(("fcall", -(d + 4)))
    # flags, in three groups that instructions write as a whole or not at all: CF, OSZAP (OF SF ZF AF PF), DF
    ef = i.eflags
    hasx = any(f.startswith("xmm") for f in local) or any(n_.startswith("xmm") for n_, _ in imp)
    string = mn.rstrip("bwdq") in ("movs", "stos", "lods", "cmps", "scas") and not hasx
    if mn.startswith("f"):
        rd = {"CF", "ZF", "PF"} if mn.startswith("fcmov") else set()
        wr = set(STATUS) if mn in ("fcomi", "fcomip", "fucomi", "fucomip") else set()
    elif hasx:
        rd = set()
        wr = set(STATUS) if mn in ("comiss", "ucomiss", "comisd", "ucomisd") else set()
    else:
        rd = {n_ for n_ in STATUS if ef & _bits("TEST", [n_])}
        wr = {n_ for n_ in STATUS + ["DF"] if ef & F_WR[n_]}
        if string:
            rd.add("DF")
        if mn in ("pushfd", "pushf", "lahf"):
            rd |= set(STATUS) | {"DF"}
        if mn in ("popfd", "popf"):
            wr = set(STATUS) | {"DF"}
        if mn == "sahf":
            wr = set(STATUS) - {"OF"}
        if mn in SHIFTS and not (len(i.operands) >= 2 and i.operands[-1].type == X.X86_OP_IMM and i.operands[-1].imm & 31):
            rd |= wr
    for g, names in (("CF", {"CF"}), ("OSZAP", {"OF", "SF", "ZF", "AF", "PF"}), ("DF", {"DF"})):
        w = wr & names
        r = bool(rd & names) or bool(w and w != names)
        if r:
            uses.append((("F", g), g))
        if w:
            defs.append((("F", g), g))
    x87 = (mn.startswith("f") and not mn.startswith(("fxsave", "fxrstor"))) or any(n_.startswith("st") for n_ in fixed_r | fixed_w)
    if x87 or mn in ("emms",):
        uses.append((("FPU",), "FPU"))
        defs.append((("FPU",), "FPU"))
    if hasx or mn == "stmxcsr":
        uses.append((("MX",), "MXCSR"))
    if mn == "ldmxcsr":
        defs.append((("MX",), "MXCSR"))
    if slots_mode:
        x.mem = (has_mem and mn not in ("lea", "nop") and not mn.startswith("prefetch")) or string or \
            mn in ("call", "ret", "retn") or (mn in ("push", "pop") and has_mem)
    else:
        x.mem = (has_mem and mn not in ("lea", "nop") and not mn.startswith("prefetch")) or string or \
            mn in IMPLICIT_MEM or mn.startswith(("push", "pop", "ret"))
    x.kind = "other"
    if mn == "ret" or mn == "retn":
        x.kind = "ret"
    elif mn == "call":
        x.kind = "call"
        x.chk = bool(callinfo and callinfo["chk"])
    elif mn == "jmp" or (mn.startswith("j") and mn != "jmp") or mn in ("loop", "loope", "loopne", "jecxz"):
        x.kind = "jmp" if mn == "jmp" else "jcc"
    elif mn in ("int3", "ud2", "int", "hlt"):
        x.kind = "trap"
    if x.kind in ("jmp", "jcc", "call"):
        op = i.operands[0]
        if op.type == X.X86_OP_IMM:
            field = x.size - 4 if x.size >= 5 else None
            tok = relocs.get(x.off + field) if field is not None else None
            x.target = ("sym", tok) if tok is not None else ("abs", op.imm & 0xffffffff)
            shape = [s for s in shape if not (isinstance(s, tuple) and s and s[0] in ("imm", "sym"))]
        elif x.kind != "call":
            raise Reject("indirect jump at +%#x (%s): switch tables are not handled" % (x.off, x.text))
    if x.comm is not None:
        regs = [t[1] for t in shape if isinstance(t, tuple) and t and t[0] == "reg" and t[1][0] == "R"]
        regs += [r for t in shape if isinstance(t, tuple) and t and t[0] == "mem" for r in t[2:4] if r and r[0] == "R"]
        fam = [r[1] for r in regs]
        if fam.count(x.comm[0]) != 1 or fam.count(x.comm[1]) != 1 or any(f in (x.comm[0], x.comm[1]) for f, _ in
                                                                           [(j, 0) for j in range(len(local))
                                                                            if ("i", local[j]) in [u for u, _ in uses]]):
            x.comm = ("shape",)                # the registers occur elsewhere too: only the operand order may swap
    x.shape = shape
    x.uses, x.defs = uses, defs
    return x


def _fixres(f):
    if f.startswith("st") or f in ("fpsw", "fpcw", "fpsr", "fpcr"):
        return "FPU"
    return f


CALL_ALL = ["eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"]


def conv_reads(tok):
    """The registers a call can read as arguments, from the callee's decorated name (the object side's relocation).
    None when unknown: the caller then assumes eax, ecx and edx."""
    if tok is None:
        return ["ecx", "edx"]                           # an indirect call: thiscall or fastcall at most
    if not (isinstance(tok, tuple) and tok[0] == "sym"):
        return None
    nm = tok[1]
    if nm.startswith("__imp_"):
        nm = nm[6:]
    if nm.startswith(CHKSTK + ("__ftol", "__CI", "__ftoui", "__dtoui", "__ultod")):
        return ["eax"] if nm.startswith(CHKSTK) else []
    if nm.startswith("@"):
        return ["ecx", "edx"]                           # __fastcall
    if nm.startswith("?"):
        import re
        m = re.search(r"@@(?:[QRUVABEFIJMN][A-D]|[CDKLSTYZ])([AEGI])", nm)
        if not m:
            return None
        return {"A": [], "E": ["ecx"], "G": [], "I": ["ecx", "edx"]}[m.group(1)]
    if nm.startswith("_"):
        return []                                       # C __cdecl or __stdcall
    return None


def _retmodel(x, level, ret64, lo, hi):
    """Pinned uses of rets and tail jumps (the ABI); calls are modelled in pairs by model_calls."""
    if x.kind in ("jmp", "jcc") and x.target and x.target[0] == "abs" and lo <= x.target[1] < hi:
        return
    if x.kind in ("jmp", "jcc") and x.target is not None:
        x.kind = "tail" if x.kind == "jmp" else "tailcc"
    if x.kind in ("ret", "tail", "tailcc"):
        if level == 2:
            reads = CALL_ALL + XMM + ["FPU"]
        else:
            reads = ["eax", "ebx", "esi", "edi", "ebp", "FPU"] + (["edx"] if ret64 else [])
        if x.kind != "ret":
            reads = sorted(set(reads) | set(CALL_ALL) | {"MXCSR"})
        _addpin(x, reads, [])


def model_calls(A, B, calls, level, slots_mode):
    """The k-th call of A and of B read and write the same pinned registers (and, in slots mode, the same frame
    bytes relative to their esp)."""
    ca = [x for x in A if x.kind == "call"]
    cb = [x for x in B if x.kind == "call"]
    if len(ca) != len(cb):
        raise Reject("%d calls in A, %d in B" % (len(ca), len(cb)))
    for n, (a, b) in enumerate(zip(ca, cb)):
        info = calls[n]
        direct = a.target is not None and a.target[0] in ("abs", "sym")
        ta = info["ta"]
        unsafe = (STD_SAFE is not None and ta is not None and not STD_SAFE(ta)) and not info["chk"]
        if level == 2 or unsafe or not direct:
            reads = list(CALL_ALL) + (XMM if level == 2 else [])
            if not direct and level != 2 and not unsafe:
                reads = ["eax", "ecx", "edx"] + ["ebx", "esi", "edi", "ebp"]
        else:
            tok = b.target[1] if b.target else None
            r = conv_reads(tok)
            reads = (["eax", "ecx", "edx"] if r is None else list(r))
            if ENTRY_REGS is not None and ta is not None:
                reads = sorted(set(reads) | (set(ENTRY_REGS(ta)) & set(CALL_ALL + XMM)))
        allw = unsafe or not direct
        writes = (["eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"] if allw else ["eax", "ecx", "edx"]) + XMM
        if info["chk"]:
            writes = ["eax", "ecx"]                  # __chkstk/__alloca_probe: probes the pages, keeps edx
        for x in (a, b):
            _addpin(x, reads + ["FPU", "MXCSR", "DF"], writes + ["CF", "OSZAP", "DF", "FPU", "MXCSR"])
        if slots_mode and not info["chk"]:
            na = info["args"]
            if na is None:
                if a.depth != b.depth:
                    raise Reject("call #%d (A +%#x): unknown argument bytes and frames of different depth (%d vs %d)"
                                 % (n, a.off, frame_bytes(a.depth), frame_bytes(b.depth)))
                na = frame_bytes(a.depth)
            for x in (a, b):
                for j in range(min(na, frame_bytes(x.depth))):
                    x.uses.append((("carg", j), ("s", -x.depth + j)))
                    x.defs.append((("cargw", j), ("s", -x.depth + j)))
                if na > frame_bytes(x.depth):
                    if x.depth >= BIG:
                        raise Reject("call #%d (A +%#x): arguments above the aligned area" % (n, a.off))
                    for j in range(x.depth, na):     # arguments beyond the frame: our own incoming area (memory)
                        x.uses.append((("cargx", j), ("argarea",)))


def _addpin(x, reads, writes):
    for r in reads:
        x.uses.append((("pin", r), r))
    for r in writes:
        if not any(res == r for _, res in x.defs):
            x.defs.append((("pin", r), r))


def blocks(ins, base, size):
    lo, hi = base, base + size
    starts = {ins[0].addr}
    addrs = {i.addr: k for k, i in enumerate(ins)}
    for k, i in enumerate(ins):
        if i.kind in ("jmp", "jcc"):
            t = i.target[1]
            if t not in addrs:
                raise Reject("branch at +%#x goes into the middle of an instruction" % i.off)
            starts.add(t)
        if i.kind in ("jmp", "jcc", "ret", "tail", "tailcc", "trap") and k + 1 < len(ins):
            starts.add(ins[k + 1].addr)
    st = sorted(starts)
    bidx = {a: n for n, a in enumerate(st)}
    bl = []
    for n, a in enumerate(st):
        e = st[n + 1] if n + 1 < len(st) else hi
        bl.append([k for k, i in enumerate(ins) if a <= i.addr < e])
    succ = []
    for n, b in enumerate(bl):
        last = ins[b[-1]]
        s = []
        if last.kind == "jcc" or last.kind == "tailcc":
            s.append(n + 1)
            if last.kind == "jcc":
                s.append(bidx[last.target[1]])
        elif last.kind == "jmp":
            s.append(bidx[last.target[1]])
        elif last.kind in ("ret", "tail", "trap"):
            pass
        elif n + 1 < len(bl):
            s.append(n + 1)
        succ.append(s)
    for i in ins:
        if i.kind in ("jmp", "jcc"):
            i.target = ("blk", bidx[i.target[1]])
    return bl, succ


def _entry_def(r):
    return ("entry", "undef") if isinstance(r, tuple) and r and r[0] == "s" else ("entry", r)


def reaching(ins, bl, succ):
    """For each instruction, rd[slot] = frozenset of def ids reaching that use. Def ids: ('entry', res) or (k, slot).
    A frame byte's entry value is ('entry', 'undef')."""
    nb = len(bl)
    preds = [[] for _ in range(nb)]
    for n, s in enumerate(succ):
        for m in s:
            preds[m].append(n)
    allres = set()
    for i in ins:
        allres |= {r for _, r in i.uses} | {r for _, r in i.defs}
    entry = {r: frozenset([_entry_def(r)]) for r in allres}
    IN = [None] * nb
    OUT = [None] * nb

    def run(n, state, record):
        st = dict(state)
        for k in bl[n]:
            i = ins[k]
            if record:
                i.rd = {slot: st.get(r, frozenset([_entry_def(r)]) if n == 0 else frozenset()) for slot, r in i.uses}
            for slot, r in i.defs:
                st[r] = frozenset([(k, slot)])
        return st

    changed = True
    while changed:
        changed = False
        for n in range(nb):
            if n == 0:
                st = dict(entry)
            else:
                st = {}
            for p in preds[n]:
                if OUT[p] is not None:
                    for r, v in OUT[p].items():
                        st[r] = st.get(r, frozenset()) | v
            if n != 0 and not preds[n]:
                st = dict(entry)
            if st != IN[n] or OUT[n] is None:
                IN[n] = st
                o = run(n, st, False)
                if o != OUT[n]:
                    OUT[n] = o
                    changed = True
    for n in range(nb):
        run(n, IN[n], True)


class Checker:
    def __init__(self, A, B):
        self.A, self.B = A, B
        self.pi = {}
        self.sym = {}
        self.symr = {}
        self.delta = None                 # delta mode: B frame offset - A frame offset
        self.skip_pins = set()            # callee-saved registers restored in both functions (not compared at ret)

    def mapdef(self, d):
        if d[0] == "entry":
            return d
        k, slot = d
        return (self.pi[k], slot) if k in self.pi else None

    def symok(self, a, b, commit):
        if a in self.sym or b in self.symr:
            return self.sym.get(a) == b and self.symr.get(b) == a
        if TOKEN_OK is not None and not TOKEN_OK(a, b):
            return False
        if commit:
            self.sym[a], self.symr[b] = b, a
        return True

    @staticmethod
    def _val(s, t):
        """An original field holding an image address (a token) against our plain constant of the same value."""
        if s[0] == "sym" and t[0] == "imm" and isinstance(s[1], tuple) and s[1][0] == "abs":
            return (s[1][1] & 0xffffffff) == t[1]
        if t[0] == "sym" and s[0] == "imm" and isinstance(t[1], tuple) and t[1][0] == "abs":
            return (t[1][1] & 0xffffffff) == s[1]
        return None

    def shape_eq(self, a, b, commit):
        if len(a.shape) != len(b.shape) or a.kind != b.kind:
            return False
        pend = []
        dpend = []
        for s, t in zip(a.shape, b.shape):
            if isinstance(s, tuple) and s and s[0] in ("sym", "imm") and isinstance(t, tuple) and t and t[0] in ("sym", "imm") \
                    and s[0] != t[0]:
                v = self._val(s, t)
                if not v:
                    return False
            elif isinstance(s, tuple) and s and s[0] == "sym":
                if not isinstance(t, tuple) or t[0] != "sym":
                    return False
                pend.append((s[1], t[1]))
            elif isinstance(s, tuple) and s and s[0] == "mem":
                if not (isinstance(t, tuple) and t[0] == "mem"):
                    return False
                if s[:5] != t[:5] and not (a.comm and b.comm and s[4] == t[4] == 1 and s[:2] == t[:2] and
                                           (s[2], s[3]) == (t[3], t[2])):
                    return False
                if s[6:] != t[6:]:
                    return False
                if s[5][0] != t[5][0]:
                    if not self._val(s[5], t[5]):
                        return False
                elif s[5][0] == "sym":
                    pend.append((s[5][1], t[5][1]))
                elif s[5] != t[5]:
                    return False
            elif isinstance(s, tuple) and s and s[0] in DELTA_TAGS:
                if not (isinstance(t, tuple) and t and t[0] == s[0]) or s[2:] != t[2:]:
                    return False
                if (s[1] < -BIG // 2) != (t[1] < -BIG // 2):
                    return False
                dpend.append((s[1] < -BIG // 2, t[1] - s[1]))
            elif s != t:
                return False
        ta, tb = a.target, b.target
        if (ta is None) != (tb is None):
            return False
        if ta is not None:
            if ta[0] != tb[0]:
                return False
            if ta[0] == "blk" and ta != tb:
                return False
            if ta[0] == "abs":
                return False
            if ta[0] == "sym":
                pend.append((ta[1], tb[1]))
        if dpend:
            want = dict(self.delta or {})
            for reg_, dd in dpend:
                if want.setdefault(reg_, dd) != dd:
                    return False
        if not all(self.symok(x, y, False) for x, y in pend):
            return False
        if commit:
            for x, y in pend:
                self.symok(x, y, True)
            if dpend:
                self.delta = want
        return True

    def uses_ok(self, a, b, final):
        why = self._uses_ok(a, b, final, None)
        if why and a.comm and b.comm and len(a.comm) == 2 and len(b.comm) == 2:
            j1, j2 = b.comm
            swap = {("e", j1): ("e", j2), ("e", j2): ("e", j1)}
            if self._uses_ok(a, b, final, swap) is None:
                return None                 # base and index (scale 1) exchanged: the same address
        return why

    def _uses_ok(self, a, b, final, swap):
        ka = {s for s, _ in a.uses}
        kb = {s for s, _ in b.uses}
        if ka != kb:
            return "use slots differ (%s vs %s)" % (sorted(map(str, ka - kb)), sorted(map(str, kb - ka)))
        resa = dict(a.uses)
        for slot in ka:
            if slot[0] == "pin" and slot[1] in self.skip_pins and a.kind == "ret":
                continue
            da, db = a.rd[slot], b.rd[swap.get(slot, slot) if swap else slot]
            for P, ds, x in ((self.A, da, a), (self.B, db, b)):
                for d in ds:
                    if d[0] != "entry":
                        p = P.ins[d[0]]
                        if p.poison and dict(p.defs).get(d[1]) in p.poison and not x.poison_reader:
                            return "reads %s of +%#x %s (a frame-size instruction)" % (dict(p.defs).get(d[1]), p.off, p.text)
            if a.save and b.save and slot == ("e", 0) and len(da) == 1 and len(db) == 1:
                (xa,), (xb,) = da, db
                if xa[0] == "entry" and xb[0] == "entry" and xa[1] in CALLEE_SAVED and xb[1] in CALLEE_SAVED:
                    continue                    # saving one callee-saved register vs another
            m = [self.mapdef(d) for d in da]
            if None in m:
                if final:
                    return "use %s has an unmapped definition" % (slot,)
                continue
            if frozenset(m) != db:
                return "operand %s (%s) reads a different producer: A %s, B %s" % (
                    self._slotname(slot, resa[slot]), resa[slot], self._defs(self.A, da), self._defs(self.B, db))
        return None

    def _slotname(self, slot, res):
        return {"e": "explicit reg #%s" % (slot[1],) if len(slot) > 1 else "", "i": "implicit", "pin": "ABI",
                "f": "fixed", "F": "flags", "FPU": "x87", "MX": "mxcsr", "carg": "call argument byte",
                "push": "push byte", "pop": "pop byte"}.get(slot[0] if not isinstance(slot[0], tuple) else
                                                           ("frame" if slot[0][0] == "m" else str(slot[0])), str(slot))

    def _defs(self, P, ds):
        out = []
        for d in sorted(ds, key=str):
            if d[0] == "entry":
                out.append("entry" if d[1] != "undef" else "undefined")
            else:
                out.append("+%#x %s" % (P.ins[d[0]].off, P.ins[d[0]].text))
        return "{" + ", ".join(out) + "}"


def restored(P):
    """Callee-saved registers that hold their entry value at every ret of program P (never written, or popped from
    the slot their own save pushed). Slots mode only (needs the frame bytes in the graph)."""
    out = set()
    rets = [k for k, x in enumerate(P.ins) if x.kind == "ret"]
    for r in CALLEE_SAVED:
        ok = True
        for k in rets:
            for d in P.ins[k].rd.get(("pin", r), ()):
                if d[0] == "entry":
                    if d[1] != r:
                        ok = False
                    continue
                p = P.ins[d[0]]
                if p.mnem != "pop" or d[1] != ("e", 0) or not p.cs.operands or \
                        FAM.get(p.cs.reg_name(p.cs.operands[0].reg), ("",))[0] != r:
                    ok = False
                    break
                srcs = set()
                for j in range(4):
                    srcs |= set(p.rd.get(("pop", j), ()))
                if len(srcs) != 4 or len({s[0] for s in srcs}) != 1 or any(s[0] == "entry" for s in srcs):
                    ok = False
                    break
                q = P.ins[next(iter(srcs))[0]]
                if q.mnem != "push" or not q.save or FAM.get(q.cs.reg_name(q.cs.operands[0].reg), ("",))[0] != r or \
                        q.rd.get(("e", 0)) != frozenset([("entry", r)]) or \
                        {s[1] for s in srcs} != {("push", j) for j in range(4)}:
                    ok = False
                    break
            if not ok:
                break
        if ok:
            out.add(r)
    return out


class Prog:
    def __init__(self, ins, base, size):
        self.base = base
        self.ins = ins
        self.bl, self.succ = blocks(self.ins, base, size)

    def finish(self):
        reaching(self.ins, self.bl, self.succ)


def check(codeA, baseA, relA, codeB, baseB, relB, level=1, ret64=False, budget=200000):
    """-> (True, info) or (False, reason)"""
    try:
        rawA, wlA = disasm_all(codeA, baseA)
        rawB, wlB = disasm_all(codeB, baseB)
        if len(rawA) != len(rawB):
            raise Reject("instruction count %d vs %d" % (len(rawA), len(rawB)))
        calls = call_infos(rawA, baseA, relA, rawB, baseB, relB, len(codeA))
        dA, eA, pA = frame_pass(rawA, baseA, len(codeA), calls)
        dB, eB, pB = frame_pass(rawB, baseB, len(codeB), calls)
        mode = "delta" if (eA or eB or not FRAME_SLOTS) else "slots"
        Ai = build(rawA, baseA, relA, level, ret64, calls, dA, mode, pA, wlA)
        Bi = build(rawB, baseB, relB, level, ret64, calls, dB, mode, pB, wlB)
        if not FRAME_SLOTS:
            pass
        A = Prog(Ai, baseA, len(codeA))
        B = Prog(Bi, baseB, len(codeB))
        model_calls(A.ins, B.ins, calls, level, mode == "slots")
        A.finish()
        B.finish()
    except Reject as e:
        return False, str(e)
    if len(A.bl) != len(B.bl):
        return False, "different number of basic blocks (%d vs %d)" % (len(A.bl), len(B.bl))
    if A.succ != B.succ:
        n = next(n for n in range(len(A.succ)) if A.succ[n] != B.succ[n])
        return False, "CFG differs at block %d (A +%#x successors %s, B +%#x successors %s)" % (
            n, A.ins[A.bl[n][0]].off, A.succ[n], B.ins[B.bl[n][0]].off, B.succ[n])
    C = Checker(A, B)
    if mode == "slots":
        C.skip_pins = restored(A) & restored(B)
    if not FRAME_SLOTS:
        C.delta = {False: 0, True: 0}
    steps = [0]
    for n in range(len(A.bl)):
        ba, bb = A.bl[n], B.bl[n]
        if len(ba) != len(bb):
            return False, "block %d (A +%#x) has %d instructions, B %d" % (n, A.ins[ba[0]].off, len(ba), len(bb))
        mema = [k for k in ba if A.ins[k].mem]
        memb = [k for k in bb if B.ins[k].mem]
        if len(mema) != len(memb):
            return False, "block %d: %d memory operations in A, %d in B" % (n, len(mema), len(memb))
        memidx_a = {k: j for j, k in enumerate(mema)}
        memidx_b = {k: j for j, k in enumerate(memb)}
        used = set()
        best = [None]

        def dfs(p):
            steps[0] += 1
            if steps[0] > budget:
                raise Reject("search budget exhausted")
            if p == len(ba):
                return True
            ka = ba[p]
            a = A.ins[ka]
            for kb in bb:
                if kb in used:
                    continue
                b = B.ins[kb]
                if a.mem != b.mem or (a.mem and memidx_a[ka] != memidx_b[kb]):
                    continue
                saved = (dict(C.sym), dict(C.symr), C.delta)
                if not C.shape_eq(a, b, True):
                    C.sym, C.symr, C.delta = saved
                    if best[0] is None or best[0][0] < p:
                        best[0] = (p, "no instruction of B's block matches A +%#x %s" % (a.off, a.text))
                    continue
                C.pi[ka] = kb
                why = C.uses_ok(a, b, False)
                if why is None:
                    used.add(kb)
                    if dfs(p + 1):
                        return True
                    used.discard(kb)
                else:
                    if best[0] is None or best[0][0] <= p:
                        best[0] = (p, "A +%#x %s / B +%#x %s: %s" % (a.off, a.text, b.off, b.text, why))
                del C.pi[ka]
                C.sym, C.symr, C.delta = saved
            if best[0] is None or best[0][0] < p:
                best[0] = (p, "no instruction of B's block matches A +%#x %s" % (a.off, a.text))
            return False

        try:
            ok = dfs(0)
        except Reject as e:
            return False, str(e)
        except RecursionError:
            return False, "block too long for the search"
        if not ok:
            return False, "block %d: %s" % (n, best[0][1] if best[0] else "no match")
    for ka, kb in C.pi.items():
        why = C.uses_ok(A.ins[ka], B.ins[kb], True)
        if why:
            return False, "A +%#x %s / B +%#x %s: %s" % (A.ins[ka].off, A.ins[ka].text, B.ins[kb].off, B.ins[kb].text, why)
    moved = sum(1 for n in range(len(A.bl)) for p, ka in enumerate(A.bl[n]) if C.pi[ka] != B.bl[n][p])
    renamed = sum(1 for ka, kb in C.pi.items() if A.ins[ka].text != B.ins[kb].text and A.ins[ka].kind == "other")
    slotdiff = sum(1 for ka, kb in C.pi.items() if _slot_offsets(A.ins[ka]) != _slot_offsets(B.ins[kb]))
    info = {"insns": len(A.ins), "blocks": len(A.bl), "moved": moved, "renamed": renamed, "mode": mode,
            "slot_renamed": slotdiff, "delta": C.delta, "skip_pins": sorted(C.skip_pins)}
    return True, info


def _slot_offsets(x):
    """Entry-relative frame offsets an instruction touches (to count the instructions whose slots were renamed)."""
    out = []
    i = x.cs
    for op in i.operands:
        if op.type == X.X86_OP_MEM and op.mem.base and i.reg_name(op.mem.base) == "esp" and x.depth is not None:
            k = s32(op.mem.disp) - x.depth
            if k < 0:
                out.append(k)
    if x.mnem == "push" and x.depth is not None:
        out.append(-(x.depth + 4))
    return out


def info_text(info):
    if isinstance(info, str):
        return info
    return "%d instructions, %d blocks, %d moved, %d with other register names, %d with other frame slots (%s mode%s)" % (
        info["insns"], info["blocks"], info["moved"], info["renamed"], info["slot_renamed"], info["mode"],
        ", D %s" % info["delta"] if info.get("delta") else "")


sys.setrecursionlimit(20000)

if __name__ == "__main__":
    print(__doc__)
