"""Mutants of our build for the self-test: a layer that proves a mutant equivalent is broken for this function, so
its proof of the real build is not accepted.

Ported from spore-decomp tools/equiv/mutate.py (commit aaaf5c79f). Kinds: an immediate changed by +1, a store
turned into nops, a register operand renamed (one occurrence, so the data flow changes), two adjacent dependent
instructions swapped, and (new, for the frame-slot model) a frame-slot displacement moved by 4 in one instruction.
Relocated fields are never touched."""
import random
from dataclasses import replace

import capstone
from capstone import x86_const as X

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
MD.detail = True


def _reloc_bytes(f):
    return {o + k for o in f.relocs for k in range(4)}


def _dependent(a, b):
    """A register dependence (not esp, not flags: swapping two esp adjustments or two flag writers with dead flags
    gives an equivalent mutant)."""
    try:
        ra, wa = a.regs_access()
        rb, wb = b.regs_access()
    except capstone.CsError:
        return False
    skip = {X.X86_REG_ESP, X.X86_REG_EFLAGS}
    wa, wb, ra, rb = set(wa) - skip, set(wb) - skip, set(ra) - skip, set(rb) - skip
    return bool(wa & (rb | wb)) or bool(wb & ra)


def candidates(f):
    rb = _reloc_bytes(f)
    out = []
    ins = list(MD.disasm(f.code, f.va))
    for k, i in enumerate(ins):
        off = i.address - f.va
        span = set(range(off, off + i.size))
        if span & rb or i.mnemonic in ("ret", "int3", "nop") or i.mnemonic.startswith("j") or i.mnemonic == "call":
            continue
        # immediate +1
        if i.operands and i.operands[-1].type == X.X86_OP_IMM and i.mnemonic not in ("push",):
            ib = getattr(i, "imm_size", 0)
            if ib in (1, 4) and getattr(i, "imm_offset", 0):
                pos = off + i.imm_offset
                out.append(("imm+1 at +%#x %s %s" % (off, i.mnemonic, i.op_str), [(pos, (f.code[pos] + 1) & 0xff)]))
        # store -> nops
        if i.operands and i.operands[0].type == X.X86_OP_MEM and i.mnemonic.startswith("mov") \
                and (i.operands[0].access & capstone.CS_AC_WRITE):
            out.append(("dropped store at +%#x %s %s" % (off, i.mnemonic, i.op_str),
                        [(off + j, 0x90) for j in range(i.size)]))
        # reg-reg mov: flip the source register field
        if i.mnemonic == "mov" and len(i.operands) == 2 and all(o.type == X.X86_OP_REG for o in i.operands) \
                and i.size == 2 and i.bytes[0] in (0x89, 0x8B):
            m = i.bytes[1]
            nm = (m & 0xC7) | ((((m >> 3) & 7) ^ 1) << 3)
            out.append(("register changed at +%#x %s %s" % (off, i.mnemonic, i.op_str), [(off + 1, nm)]))
        # a frame slot moved: [esp+disp8] -> [esp+disp8+4] in this one instruction
        for o in i.operands:
            if o.type == X.X86_OP_MEM and o.mem.base and i.reg_name(o.mem.base) == "esp" and not o.mem.index \
                    and i.disp_size == 1 and 0 <= o.mem.disp <= 0x7b:
                pos = off + i.disp_offset
                out.append(("frame slot moved at +%#x %s %s" % (off, i.mnemonic, i.op_str),
                            [(pos, (f.code[pos] + 4) & 0xff)]))
        # two adjacent dependent instructions swapped (same block: neither is a branch)
        if k + 1 < len(ins):
            j = ins[k + 1]
            jo = j.address - f.va
            if not (set(range(jo, jo + j.size)) & rb) and not j.mnemonic.startswith(("j", "ret")) and \
                    j.mnemonic not in ("call", "int3") and _dependent(i, j) and bytes(i.bytes) != bytes(j.bytes):
                swapped = bytes(f.code[jo:jo + j.size]) + bytes(f.code[off:off + i.size])
                out.append(("swapped dependent +%#x %s %s / %s %s" % (off, i.mnemonic, i.op_str, j.mnemonic, j.op_str),
                            [(off + n, v) for n, v in enumerate(swapped)]))
    return out


def mutants(f, k=5, seed=1, covered=None):
    """[(description, Func)] up to k mutants, deterministic for a seed; at least one of each kind available.
    covered (M&B): offsets of the instructions the proving layer executed (angr: on some explored path); a mutant
    elsewhere is left out, since code the model never runs (after a call to a noreturn function such as mb_error)
    cannot make a mutant differ, and its equivalent mutant would fail the self-test for nothing."""
    c = candidates(f)
    if covered is not None:
        cov = set(covered)
        starts = sorted(i.address - f.va for i in MD.disasm(f.code, f.va))
        import bisect

        def ins_of(pos):
            return starts[bisect.bisect_right(starts, pos) - 1]
        c = [(d, p) for d, p in c if ins_of(min(q for q, _ in p)) in cov]
    rng = random.Random(seed)
    rng.shuffle(c)
    kinds = {}
    for desc, patch in c:
        kinds.setdefault(desc.split(" at ")[0].split(" +")[0], (desc, patch))
    pick = list(kinds.values())
    for x in c:
        if len(pick) >= k:
            break
        if x not in pick:
            pick.append(x)
    out = []
    for desc, patch in pick[:max(k, len(kinds))]:
        b = bytearray(f.code)
        for pos, v in patch:
            b[pos] = v
        out.append((desc, replace(f, code=bytes(b), meta=dict(f.meta, mutant=desc))))
    return out
