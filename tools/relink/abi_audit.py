#!/usr/bin/env python3
"""abi_audit.py: check every call site in the standalone exe for an argument/ABI mismatch between caller and callee.

usage: abi_audit.py [--exe PATH] [--map PATH] [--resolve-map PATH] [--json PATH] [--filter SUBSTR]

Why: six recent standalone crashes were interface mismatches that byte-exactness of a function body cannot see. The
body of a function can match the original byte for byte and still be called wrongly, or call wrongly, because the
compiled *declaration* is wrong. This tool compares, at each direct call, the number of argument bytes the caller
pushes with the number the callee pops via `ret N` (its cleanup). A row means the stack pointer is off by
pushed - pops when the call returns: either the caller leaves bytes the callee did not take (callee `ret N` with
N < pushed: the caller's own `ret` then jumps into garbage / the next frame reads the wrong slots), or the callee
pops bytes the caller never pushed (N > pushed: the callee's `ret` lands above the caller's frame). In both cases
the crash surfaces elsewhere, usually inside the callee or at the return, never at the call.

The two classes seen here, both invisible in either function's bytes:
  * arity / convention, the add_order_arrow class: the original is `?add_order_arrow@MissionAgents@c49bea0@@QAEXH@Z`
    (__thiscall, one int, so `ret 4`), and a call compiled against a two-argument declaration pushed 8. A nearby
    member of the same family is `?set_track@MissionUi@@QAEXHPAUmesh@rgl@@@Z` (__thiscall, two arguments, `ret 8`)
    reached through a two-argument function-pointer cast: the call resolved to a different mangled symbol and the
    bytes of both functions still matched.
  * a virtual slot called through a `__cdecl` function-pointer cast, the set_track class: the slot's original body is
    `__thiscall`, so the caller must pass the object in `ecx`; the cast makes the call push `this` as a cdecl
    argument instead, and the callee then uses `ecx` = the vtable pointer as `this`. The pushed/popped counts can
    still agree here, which is why the site is only visible as the crash inside the callee; the tool also lists
    indirect calls separately, and names the vtable slot when the register provably holds a matched vtable's
    pointer, so these can be checked by hand.

Method: disassemble .text (capstone, x86-32) one function at a time, the functions being the map's `f` symbols,
with the map's code labels ($L...) used to re-sync the decode when capstone drifts inside a data island. For each
direct `call rel32`, compute the bytes of argument space set up before it as a stack-delta dataflow over the
function's blocks (not a plain backwards walk: the arguments of a call can be pushed before the branch that guards
it). `push` is -4, `sub esp, N` is -N, `pop`/`add esp, N` give it back and a direct call adds back what its callee
pops. Local-frame `sub esp, N` (matched to an `add esp, N` the function itself executes) is not argument space and
neither is a push of a callee-saved register the function pops. The site is reported unreliable, never guessed at,
when two paths reach it with different stack levels, when an indirect call (class-specific convention) or an odd esp
write taints the path, when the callee's cleanup is not a single `ret N` (uniform over its returns, one `jmp` thunk
resolved), or when the caller's leftover space is its own stack temporary (freed later by the caller) or part of
building an exception that is thrown (those never return). The callee's cleanup is the `ret N` uniform over its
returns. False negatives are fine here; false positives are not.

Indirect calls (`call [reg]`, `call [reg+off]`) are recorded with their pushed bytes but never compared: the slot's
convention is class-specific. Where the base register is provably a matched vtable's pointer (a `mov reg, <vtable>`
or `mov reg, [<global vptr>]` in the same block, and the address is a vtable in resolve_map.json), the slot target is
read out of the image and named.

Output: a table of mismatches, worst pushed-vs-pops delta first, then by module; the summary line counts direct
calls checked, mismatched, unreliable and indirect. --json writes the same rows (plus every indirect and unreliable
site) to a file, so the next session can pick the worklist up; --filter limits rows to sites whose caller, callee or
module contains SUBSTR.
"""
import argparse
import bisect
import collections
import json
import os
import re
import struct
import sys

import capstone
import pefile
from capstone import x86 as X86

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STANDALONE = os.path.join(ROOT, "work", "relink", "standalone")
CALLEE_SAVED = ("ebx", "esi", "edi", "ebp")
MAP_RE = re.compile(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s(.*)$")


def read_map(path):
    """The /MAP file: ({name: va}, {va: function name}, {va: module}, [(va, name)] in address order).

    Line format as in build.py's read_map (publics first, then statics): a symbol is a name, an absolute address, an
    optional `f` flag and the .obj it came from. A name listed at two addresses (a static of the same name in two
    objects) is dropped, as build.py does. The module is the .obj stem: link order adds a per-run hash suffix that
    nothing outside the linker knows about, and everything in tools/relink groups by the stem."""
    sym, funcs, modules, ordered = {}, {}, {}, []
    for line in open(path, errors="replace"):
        m = MAP_RE.match(line)
        if not m:
            continue
        name, va, rest = m[1], int(m[2], 16), m[3].split()
        sym[name] = va if sym.get(name, va) == va else None
        if rest and rest[0] == "f" and va not in funcs:
            funcs[va] = name
        obj = next((t for t in rest if t.endswith(".obj")), "")
        if obj and va not in modules:
            modules[va] = os.path.basename(obj).rsplit("_", 1)[0]
        ordered.append((va, name))
    ordered.sort()
    return sym, funcs, modules, ordered


class Image:
    """The mapped image (get_memory_mapped_image, as crashrun.py/cmpdis.py read it), its base and its .text."""

    def __init__(self, path):
        self.path = path
        self.pe = pefile.PE(path, fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.mem = self.pe.get_memory_mapped_image()
        text = next(s for s in self.pe.sections if s.Name.rstrip(b"\x00") == b".text")
        self.text_va = self.base + text.VirtualAddress
        self.text_end = self.text_va + text.Misc_VirtualSize
        self.text = self.mem[text.VirtualAddress:text.VirtualAddress + text.Misc_VirtualSize]

    def u32(self, va):
        off = va - self.base
        if off < 0 or off + 4 > len(self.mem):
            return None
        return struct.unpack_from("<I", self.mem, off)[0]

    def in_text(self, va):
        return self.text_va <= va < self.text_end


class Region:
    """One function: its address range, its instructions, and what the push walk needs to know about them."""

    def __init__(self, start, end, insns, bad_from):
        self.start = start
        self.end = end
        self.insns = insns
        self.bad_from = bad_from                      # first address capstone could not decode (a data island)
        self.prologue_end = start                     # end of the entry pushes / stack allocation
        self.prologue_saves = []                      # callee-saved registers pushed by the entry sequence
        self.local_subs = {}                          # {sub esp address: size} of the local frame, not arguments
        self.temp_pushes = set()                      # pushes of a register that held nothing yet: stack temporaries


def decode(img, starts, labels):
    """{function va: Region}, decoding each function in segments that begin at a map symbol inside it.

    A function is decoded from its start, and again from every map label inside it: MSVC switches end with a
    `$L`-labelled target, and a data island (an inline jump table, a float pool) otherwise drifts the linear decode
    and every later `push`/`call` it finds is fiction. Decoding each segment from its own label limits the damage to
    that segment and keeps the addresses of the segments after it exact."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    regions = {}
    for i, start in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else img.text_end
        bounds = [start] + [a for a in labels if start < a < end] + [end]
        insns, bad = [], None
        for b0, b1 in zip(bounds, bounds[1:]):
            chunk = img.text[b0 - img.text_va:b1 - img.text_va]
            got = list(md.disasm(chunk, b0))
            insns.extend(got)
            used = sum(len(x.bytes) for x in got)
            if used != b1 - b0 and bad is None:
                bad = b0 + used
        regions[start] = Region(start, end, insns, bad)
    for reg in regions.values():
        classify_frame(reg, regions)
        written = set()
        for ins in reg.insns:
            ops = ins.operands
            if ins.mnemonic == "push" and ops and ops[0].type == X86.X86_OP_REG:
                if ins.reg_name(ops[0].reg) not in written:
                    reg.temp_pushes.add(ins.address)   # `push ecx` with ecx undefined reserves four bytes
            m = ins.mnemonic
            if ops and ops[0].type == X86.X86_OP_REG and not (m.startswith("cmp") or m.startswith("test")
                                                              or m == "push" or m == "call" or m == "jmp"):
                written.add(ins.reg_name(ops[0].reg))
        # the entry sequence: saved registers, the frame, the local stack. Pushes in it are not arguments, and the
        # registers it saves are popped again at the end, so those pops are not stack space either.
        idx = 0
        for k in range(min(16, len(reg.insns))):
            if reg.insns[k].mnemonic == "mov" and "fs:[0]" in reg.insns[k].op_str \
                    and reg.insns[k].op_str.replace(" ", "").endswith(",esp"):
                idx = k + 1                     # the end of a __try frame setup: its pushes are not arguments
                reg.prologue_end = reg.insns[k].address + len(reg.insns[k].bytes)
                break
        while idx < len(reg.insns):
            ins = reg.insns[idx]
            m, ops = ins.mnemonic, ins.operands
            if m == "push" and ops and ops[0].type == X86.X86_OP_REG and ins.reg_name(ops[0].reg) in CALLEE_SAVED:
                reg.prologue_saves.append(ins.reg_name(ops[0].reg))
            elif m == "sub" and ops and is_esp(ins, ops[0]) and ops[1].type == X86.X86_OP_IMM:
                if ins.address not in reg.local_subs:
                    break                   # a by-value argument's space, not the frame: the dataflow must count it
            elif m == "and" and ops and is_esp(ins, ops[0]):
                pass
            elif m == "mov" and ops and ops[0].type == X86.X86_OP_REG and is_esp(ins, ops[-1]):
                pass
            elif m == "lea" and ops and is_esp(ins, ops[0]) and len(ops) > 1 and ops[1].type == X86.X86_OP_MEM \
                    and ops[1].mem.base == X86.REG_ESP:
                pass
            elif m == "mov" and ins.op_str == "edi, edi":
                pass
            else:
                break
            reg.prologue_end = ins.address + len(ins.bytes)
            idx += 1
    return regions


def is_esp(ins, op):
    return op.type == X86.X86_OP_REG and ins.reg_name(op.reg) == "esp"


def classify_frame(reg, regions):
    """Split the function's `sub esp, N` instructions into the local frame (and temporaries) and argument space.

    Every reservation of stack -- a `push`, a `sub esp, N` -- is put on a model stack, and whatever frees stack
    pops it: a `call` frees exactly the bytes its callee pops with `ret N`, a `pop` frees four, an `add esp, N`
    frees N. A `sub esp, N` that a call frees is a by-value argument; one the caller frees itself is local (a
    buffer, a temporary string, a by-value return slot). MSVC uses `sub esp, N` for both, and misreading one shifts
    every later call in the function by N, so it is worth simulating. When the model stack is lost -- an indirect
    call, whose convention is unknown -- an `add esp, N` still names the reservation it frees by size.

    Fills reg.local_subs."""
    pending, subs = [], {}                          # [(size, is_sub, address)] top last; {address: size} unknown

    def release(total, by_call=False):
        left = total
        while left > 0 and pending:
            size, is_sub, address = pending[-1]
            if size > left:
                break
            pending.pop()
            left -= size
            subs.pop(address, None)
            if is_sub and not by_call:
                reg.local_subs[address] = size
        if left > 0 and not by_call:                # the model lost the reservation: match by size alone
            for target in (total, left, 0):
                for address, size in reversed(list(subs.items())):
                    if size == target:
                        reg.local_subs[address] = size
                        subs.pop(address)
                        pending.clear()
                        return

    for ins in reg.insns:
        m, ops = ins.mnemonic, ins.operands
        if m == "leave" or (m in ("mov", "lea") and ops and is_esp(ins, ops[0]) and not (
                ops[1].type == X86.X86_OP_MEM and ops[1].mem.base and ins.reg_name(ops[1].mem.base) == "esp"
                and not ops[1].mem.disp)):
            for size, is_sub, address in pending:
                if is_sub:
                    reg.local_subs[address] = size
            reg.local_subs.update(subs)
            pending, subs = [], {}
            continue
        if m == "push":
            pending.append((4, False, ins.address))
            continue
        if m == "sub" and ops and is_esp(ins, ops[0]) and len(ops) == 2 and ops[1].type == X86.X86_OP_IMM:
            pending.append((ops[1].imm, True, ins.address))
            subs[ins.address] = ops[1].imm
            continue
        if m == "add" and ops and is_esp(ins, ops[0]) and len(ops) == 2 and ops[1].type == X86.X86_OP_IMM:
            release(ops[1].imm)
            continue
        if m == "pop" and pending:
            release(4)
            continue
        if m == "call" and ops and ops[0].type == X86.X86_OP_IMM:
            pops = callee_cleanup(regions, ops[0].imm & 0xFFFFFFFF)[0]
            if pops is not None:
                release(pops, by_call=True)


def branch_targets(regions):
    """Every address something jumps or calls to (block boundaries, used when resolving an indirect slot)."""
    targets = set()
    for reg in regions.values():
        for ins in reg.insns:
            m, ops = ins.mnemonic, ins.operands
            if ops and ops[0].type == X86.X86_OP_IMM and (m == "call" or m == "jmp" or m.startswith("j")
                                                          or m.startswith("loop")):
                targets.add(ops[0].imm & 0xFFFFFFFF)
    return targets


def stack_states(region, regions):
    """Per instruction: esp relative to the post-prologue level, the frame level, pending saves, and a taint flag.

    A forward dataflow over the function's blocks, because the arguments of a call can be pushed before the branch
    that guards it (`push 0; test ...; jp L; call f; L: call f`) and only a merge at the join sees them. `push` is
    -4, `sub esp, N` is -N (argument space for a by-value struct, or a local buffer), `pop` and `add esp, N` give it
    back, and a direct call adds back what its callee pops with `ret N`.

    The frame level is where the last `add esp, N` put esp: that instruction is the compiler saying "these bytes are
    free again", so everything pushed after it is this call's. Without that, a function with a local buffer
    (`sub esp, 0x400 ... add esp, 0x400`) would look like every later call had a 1 KB argument.

    Two paths meeting with different values mean the function's stack is not a function of its address (or our model
    of it is wrong): the whole function is then reported unreliable rather than guessed at. An indirect call, whose
    slot's convention is class-specific, and any esp write other than an immediate add/sub taint the rest of the
    function the same way.

    Returns (deltas, frames, save_pushes, tainted, clean); the argument bytes at a call are frames[i] - deltas[i]."""
    insns = region.insns
    n = len(insns)
    at = {ins.address: i for i, ins in enumerate(insns)}
    deltas, frames, saves, tainted = [None] * n, [0] * n, [0] * n, [True] * n
    dead = [False] * n
    start = 0
    while start < n and insns[start].address < region.prologue_end:
        start += 1
    if start >= n:
        return deltas, frames, saves, tainted, False
    deltas[start], frames[start], saves[start], tainted[start] = 0, 0, 0, False

    def flow(i, pending_saves):
        """(delta change, save-count change, taint or None, new frame level or None) of instruction i."""
        ins = insns[i]
        m, ops = ins.mnemonic, ins.operands
        if m == "push":
            if not ops:
                return 0, 0, True, None
            op = ops[0]
            if op.type == X86.X86_OP_IMM:
                return -4, 0, None, None
            if op.type in (X86.X86_OP_REG, X86.X86_OP_MEM) and op.size == 4:
                callee_saved = op.type == X86.X86_OP_REG and ins.reg_name(op.reg) in CALLEE_SAVED
                ambiguous = callee_saved or ins.address in region.temp_pushes
                return -4, 1 if ambiguous else 0, None, None
            return 0, 0, True, None
        if m == "pop":
            name = ins.reg_name(ops[0].reg) if ops and ops[0].type == X86.X86_OP_REG else ""
            if name in CALLEE_SAVED:
                if pending_saves:
                    return 4, -1, None, None     # undoes a push we counted
                if name in region.prologue_saves:
                    return 0, 0, None, None      # undoes an entry push we did not count
            return 4, 0, None, None
        if m in ("popal", "popad", "popfd", "popf", "leave", "pushal", "pushad", "pushfd"):
            return 0, 0, True, None
        if m == "call":
            if ops and ops[0].type == X86.X86_OP_IMM:
                pops, _ = callee_cleanup(regions, ops[0].imm & 0xFFFFFFFF)
                if pops is not None:
                    return pops, 0, None, None
            return 0, 0, True, None
        if ops and is_esp(ins, ops[0]):
            if m in ("sub", "add") and len(ops) == 2 and ops[1].type == X86.X86_OP_IMM:
                if m == "sub" and ins.address in region.local_subs:
                    return -ops[1].imm, 0, None, "reset"   # the local frame moves down, arguments are relative to it
                if m == "sub":
                    return -ops[1].imm, 0, None, "keep"    # argument space: the callee's ret N frees it
                return ops[1].imm, 0, None, "reset"        # "these bytes are free again": the frame level moves up
            return 0, 0, True, None
        return 0, 0, None, None

    def successors(i):
        ins = insns[i]
        m, ops = ins.mnemonic, ins.operands
        if m.startswith("ret"):
            return []
        if m == "jmp" or m.startswith("loop"):
            return [at[ops[0].imm & 0xFFFFFFFF]] if ops and ops[0].type == X86.X86_OP_IMM \
                and (ops[0].imm & 0xFFFFFFFF) in at else []
        out = [i + 1]
        if m.startswith("j") and ops and ops[0].type == X86.X86_OP_IMM and (ops[0].imm & 0xFFFFFFFF) in at:
            out.append(at[ops[0].imm & 0xFFFFFFFF])
        return [j for j in out if j < n]

    for _round in range(64):
        changed = False
        for i in range(start, n):
            if deltas[i] is None or dead[i]:
                continue
            d, s, t, frame = flow(i, saves[i])
            out_d = deltas[i] + d
            out_f = out_d if frame == "reset" else max(frames[i], out_d)
            out_s, out_t = max(0, saves[i] + s), tainted[i] or bool(t)
            for j in successors(i):
                if dead[j]:
                    continue
                if deltas[j] is None:
                    deltas[j], frames[j], saves[j], tainted[j] = out_d, out_f, out_s, out_t
                    changed = True
                elif (deltas[j], frames[j]) != (out_d, out_f):
                    dead[j], deltas[j] = True, None
                    changed = True
                elif (out_s, out_t) != (saves[j], tainted[j]):
                    saves[j], tainted[j] = max(saves[j], out_s), tainted[j] or out_t
                    changed = True
        if not changed:
            break
    else:
        return deltas, frames, saves, tainted, False
    return deltas, frames, saves, tainted, not any(dead)


def slot_reused(region, idx, deltas, leftover, targets):
    """Do the next instructions read or write the space the callee did not pop (a stack temporary)?"""
    for k in range(idx + 1, min(idx + 9, len(region.insns))):
        ins = region.insns[k]
        m = ins.mnemonic
        if m in ("call", "jmp") or m.startswith("ret") or m.startswith("j") or m == "leave" \
                or ins.address in targets or deltas[k] is None:
            return False
        for op in ins.operands:
            if op.type == X86.X86_OP_MEM and op.mem.base and ins.reg_name(op.mem.base) == "esp" \
                    and -leftover <= deltas[k] + op.mem.disp < 0:
                return True
    return False


def throw_ahead(region, idx, funcs, sym_at):
    """Is this call part of building and throwing an exception?

    `_Xlen`/`_Xran` and the like build a string, a `logic_error` and then call `__CxxThrowException`, which never
    returns: the exception object stays on the stack by design, so the bytes their callees do not pop are not an ABI
    mismatch. Recognised by a throw within the next few instructions of the same straight line."""
    for k in range(idx + 1, min(idx + 12, len(region.insns))):
        ins = region.insns[k]
        if ins.mnemonic in ("jmp",) or ins.mnemonic.startswith("ret"):
            return False
        if ins.mnemonic.startswith("j") and ins.address not in funcs:
            return False
        if ins.mnemonic == "call" and ins.operands and ins.operands[0].type == X86.X86_OP_IMM:
            if "ThrowException" in sym_at(ins.operands[0].imm & 0xFFFFFFFF):
                return True
    return False


def leftover_released(region, idx, deltas, frames):
    """Does the code after the call give back the space the callee did not pop?

    A caller that leaves argument space on the stack because it was never an argument -- MSVC reserves stack
    temporaries with `sub esp, N` or with a `push` of a register that holds nothing yet -- releases that space
    itself: with an `add esp, N` or a `pop` somewhere before the function returns, so the stack comes back to the
    frame level. A caller that pushed bytes its callee does not pop never gets there: its epilogue only undoes the
    frame, and the extra bytes stay on the stack (that is the crash)."""
    frame = frames[idx]
    for k in range(idx + 1, len(region.insns)):
        if deltas[k] is not None and deltas[k] >= frame:
            return True
    return False


def possible_push_totals(pushed, saves):
    """Every reading of the setup: a pending callee-saved push may be a register save, not an argument."""
    return {pushed - 4 * k for k in range(saves + 1)}


def callee_cleanup(regions, va, depth=0):
    """(bytes the callee pops with ret, reason or None), following a jmp-only thunk one level."""
    reg = regions.get(va)
    if reg is None or depth > 8:
        return None, "callee not in .text"
    if reg.bad_from is not None:
        return None, "callee has undecodable bytes"
    body = [x for x in reg.insns if x.mnemonic not in ("nop", "int3")]
    if body and body[0].mnemonic == "jmp" and body[0].operands and body[0].operands[0].type == X86.X86_OP_IMM:
        return callee_cleanup(regions, body[0].operands[0].imm & 0xFFFFFFFF, depth + 1)
    vals = set()
    for ins in reg.insns:
        if ins.mnemonic == "ret":
            vals.add(ins.operands[0].imm if ins.operands else 0)
        elif ins.mnemonic.startswith("ret"):
            return None, "far return"
    if not vals:
        return None, "no ret (tail call only)"
    if len(vals) > 1:
        return None, "mixed ret %s" % sorted(vals)
    return vals.pop(), None


def writes_reg(ins, reg_id):
    """Does this instruction write the register (so a value we track for it does not reach the call)?"""
    if not ins.operands:
        return False
    op = ins.operands[0]
    if op.type != X86.X86_OP_REG or op.reg != reg_id:
        return False
    m = ins.mnemonic
    if m.startswith("cmp") or m.startswith("test") or m in ("push", "call", "jmp"):
        return False
    return True


def vtable_pointer(region, idx, targets, img, vtables):
    """The vtable a `call [reg+off]`'s base register provably holds, if a mov in this block says so."""
    op = region.insns[idx].operands[0]
    if op.type != X86.X86_OP_MEM or not op.mem.base or op.mem.index:
        return None
    reg = op.mem.base
    j = idx - 1
    while j >= 0 and idx - j <= 12:
        ins = region.insns[j]
        m, ops = ins.mnemonic, ins.operands
        if m in ("call", "jmp") or m.startswith("ret") or m.startswith("j") or ins.address in targets:
            return None
        if m == "mov" and len(ops) == 2 and ops[0].type == X86.X86_OP_REG and ops[0].reg == reg:
            src = ops[1]
            val = None
            if src.type == X86.X86_OP_IMM:
                val = src.imm & 0xFFFFFFFF
            elif src.type == X86.X86_OP_MEM and not src.mem.base and not src.mem.index:
                val = img.u32(src.mem.disp)             # a global object's vptr
            return val if val in vtables else None
        if writes_reg(ins, reg):
            return None
        j -= 1
    return None


def symbol_at(va, funcs, names_at):
    """The name of the function starting at va, else the label at va, else ''."""
    if va in funcs:
        return funcs[va]
    names = names_at.get(va)
    if not names:
        return ""
    return min(names, key=lambda n: (0 if n.startswith("?") else 1, len(n)))


def symbol_near(va, ordered, vas):
    """The name of the symbol at or below va (calls that land inside a function / a slot entry)."""
    i = bisect.bisect_right(vas, va) - 1
    return ordered[i][1] if i >= 0 else ""


def module_of(va, modules, starts):
    """The object a function came from: its own entry, or the function whose range contains it."""
    if va in modules:
        return modules[va]
    i = bisect.bisect_right(starts, va) - 1
    return modules.get(starts[i], "") if i >= 0 else ""


def load_vtables(path, sym):
    """{our vtable va: (name, slots)} from resolve_map.json's matched vtables (empty if the file is not there)."""
    try:
        rmap = json.load(open(path))
    except (OSError, ValueError):
        return {}
    out = {}
    for name, e in rmap.get("vtables", {}).items():
        va = sym.get(name)
        if va is not None:
            out[va] = (name, e.get("slots", 0))
    return out


def audit(img, sym, funcs, modules, ordered, vtable_path):
    starts = sorted(v for v in funcs if img.in_text(v))
    labels = sorted({va for va, name in ordered if img.in_text(va) and name.startswith("$")} - set(starts))
    regions = decode(img, starts, labels)
    targets = branch_targets(regions)
    vtables = load_vtables(vtable_path, sym)
    vas = [va for va, _ in ordered]
    names_at = collections.defaultdict(list)
    for va, name in ordered:
        names_at[va].append(name)

    mismatches, indirect, unreliable = [], [], []
    stat = collections.Counter()
    for start in starts:
        reg = regions[start]
        caller = symbol_at(start, funcs, names_at)
        caller_va = "%08x" % start
        module = modules.get(start, "")
        deltas, frames, saves, tainted, clean = stack_states(reg, regions)

        for idx, ins in enumerate(reg.insns):
            if ins.mnemonic != "call" or not ins.operands:
                continue
            op = ins.operands[0]
            known = clean and deltas[idx] is not None and not tainted[idx] and frames[idx] >= deltas[idx]
            pushed = frames[idx] - deltas[idx] if known else None
            why = None if known else ("ambiguous stack at a join" if not clean else "stack not statically known")

            site = {"caller_va": caller_va, "caller": caller, "module": module, "call_va": "%08x" % ins.address}
            if op.type == X86.X86_OP_MEM:
                stat["indirect"] += 1
                base = ins.op_str
                vt = vtable_pointer(reg, idx, targets, img, vtables)
                row = dict(site, kind="indirect", pushed=pushed, target=base)
                if vt is not None:
                    slot_va = vt + (op.mem.disp or 0)
                    target = img.u32(slot_va) or 0
                    row.update(kind="vslot", vtable=vtables[vt][0], slot_va="%08x" % slot_va,
                               callee_va="%08x" % target, callee=symbol_at(target, funcs, names_at)
                               or symbol_near(target, ordered, vas))
                    stat["vslot"] += 1
                indirect.append(row)
                continue

            stat["direct"] += 1
            callee_va = op.imm & 0xFFFFFFFF
            callee = symbol_at(callee_va, funcs, names_at) or symbol_near(callee_va, ordered, vas)
            named = dict(callee_va="%08x" % callee_va, callee=callee)
            if reg.bad_from is not None and ins.address >= reg.bad_from:
                unreliable.append(dict(site, reason="caller has undecodable bytes", **named))
                continue
            if "ThrowException" in callee:
                unreliable.append(dict(site, reason="exception throw (never returns)", **named))
                continue
            if why:
                unreliable.append(dict(site, reason=why, **named))
                continue
            if not img.in_text(callee_va):
                unreliable.append(dict(site, reason="direct call outside .text", **named))
                continue
            pops, why = callee_cleanup(regions, callee_va)
            if pops is None:
                unreliable.append(dict(site, reason=why, callee_va="%08x" % callee_va, callee=callee))
                continue
            stat["checked"] += 1
            if pops == 0:
                stat["cdecl"] += 1                       # the caller cleans up: nothing to compare
                continue
            totals = possible_push_totals(pushed, saves[idx])
            if pops in totals:
                stat["match"] += 1
                continue
            kind = "over-pop" if pops > pushed else "under-pop"
            if kind == "under-pop" and (slot_reused(reg, idx, deltas, pushed - pops, targets)
                                        or leftover_released(reg, idx, deltas, frames)
                                        or throw_ahead(reg, idx, funcs,
                                                       lambda va: symbol_at(va, funcs, names_at)
                                                       or symbol_near(va, ordered, vas))):
                stat["temporary"] += 1               # the leftover slots are the caller's own temporaries
                continue
            stat[kind] += 1
            mismatches.append(dict(site, kind=kind, callee_va="%08x" % callee_va, callee=callee,
                                   pushed=pushed, pops=pops, delta=pushed - pops,
                                   pushed_candidates=sorted(t for t in totals if t >= 0)))
    mismatches.sort(key=lambda r: (r["delta"], r["module"], r["caller_va"]))
    indirect.sort(key=lambda r: (r["module"], r["caller_va"], r["call_va"]))
    unreliable.sort(key=lambda r: (r["module"], r["caller_va"], r["call_va"]))
    return stat, mismatches, indirect, unreliable


def matches(row, needle):
    if not needle:
        return True
    hay = " ".join(str(row.get(k, "")) for k in ("caller", "callee", "module", "kind", "target", "vtable"))
    return needle.lower() in hay.lower()


def main():
    ap = argparse.ArgumentParser(description="Check call-site argument bytes against each callee's `ret N` cleanup.")
    ap.add_argument("--exe", default=os.path.join(STANDALONE, "mb_standalone.exe"))
    ap.add_argument("--map", default=os.path.join(STANDALONE, "mb_standalone.map"))
    ap.add_argument("--resolve-map", default=os.path.join(STANDALONE, "resolve_map.json"))
    ap.add_argument("--json", metavar="PATH", help="write the rows to PATH as JSON")
    ap.add_argument("--filter", metavar="SUBSTR", default="", help="only rows whose symbols or module contain SUBSTR")
    a = ap.parse_args()
    for p in (a.exe, a.map):
        if not os.path.exists(p):
            sys.exit("%s: no such file (is the build running? build.py removes the exe and map it replaces)" % p)
    sym, funcs, modules, ordered = read_map(a.map)
    img = Image(a.exe)
    stat, mismatches, indirect, unreliable = audit(img, sym, funcs, modules, ordered, a.resolve_map)
    print("summary: %d direct calls checked, %d mismatched, %d unreliable, %d indirect "
          "(%d direct calls in .text, %d cdecl, %d matched, %d vtable slots resolved)"
          % (stat["checked"], len(mismatches), len(unreliable), stat["indirect"], stat["direct"], stat["cdecl"],
             stat["match"], stat["vslot"]))

    rows = [r for r in mismatches if matches(r, a.filter)]
    if rows:
        print("mismatches: pushed != the callee's ret N (delta = pushed - pops; a caller that pushes more leaves the "
              "extra bytes on the stack, a callee that pops more eats the caller's frame)")
        print("%-10s  %-52s  %-52s  %6s  %5s  %s" % ("caller_va", "caller_symbol", "callee_symbol", "pushed",
                                                     "pops", "kind"))
        for r in rows:
            print("%-10s  %-52s  %-52s  %6d  %5d  %s"
                  % (r["caller_va"], r["caller"][:52], r["callee"][:52], r["pushed"], r["pops"], r["kind"]))
    else:
        print("no mismatches%s" % (" for %r" % a.filter if a.filter else ""))
        sample = [r for r in indirect + unreliable if matches(r, a.filter)][:15]
        print("%-10s  %-52s  %-52s  %6s  %5s  %s" % ("caller_va", "caller_symbol", "callee_symbol", "pushed",
                                                     "pops", "kind"))
        for r in sample:
            callee = r.get("callee") or r.get("target", r.get("vtable", ""))
            print("%-10s  %-52s  %-52s  %6s  %5s  %s"
                  % (r["caller_va"], r["caller"][:52], callee[:52], r.get("pushed", ""), "", r["kind"]))
    if a.json:
        json.dump({"exe": os.path.relpath(a.exe, ROOT), "map": os.path.relpath(a.map, ROOT), "summary": dict(stat),
                   "mismatches": [r for r in mismatches if matches(r, a.filter)],
                   "indirect": [r for r in indirect if matches(r, a.filter)],
                   "unreliable": [r for r in unreliable if matches(r, a.filter)]},
                  open(a.json, "w"), indent=1)
        print("-> %s" % a.json)
    return 0


if __name__ == "__main__":
    sys.exit(main())
