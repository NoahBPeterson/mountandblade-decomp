#!/usr/bin/env python3
"""Differential equivalence test: ORIGINAL function (from the binary) vs OUR compiled function.

usage: equiv.py <slice-id> [<va> ...] [--flags "..."] [--inputs N] [--seed S] [--json out.json]
                [--min-valid K] [--gate edges|insns] [--no-steer] [--ulp U] [--strict-calls]
                [--budget N] [--time-limit S] [--fp-tol] [--fp-realistic]
                [--call-regs [--call-regs-strict]] [-v]

Default VAs: the slice's nonmatching.txt. Our function is compiled (into work/difftest/obj/, never
match/), its relocations are resolved to the ORIGINAL's addresses (tools/difftest/resolve.py), and
both versions run under Unicorn on the same generated inputs; every observable (return registers,
callee-saved registers, esp, x87/SSE result, every written byte of .data/heap/input buffers/caller
stack, the import-call trace and the depth-1 call trace) must agree.

Coverage is BRANCH (edge) coverage on BOTH builds (tools/difftest/cfgcov.py): every conditional jump's
taken and not-taken edge and every jump-table target of the original function and of ours, counted over
valid inputs. An edge the harness cannot reach is listed with its reason in the JSON ("edges" ->
side -> "unreachable") and in the printed result, never dropped silently: statically infeasible x87
status-word branches, NaN-only branches (without --nan), edges only reachable past a call that never
returned (an unmodelled import, a C++ throw or a pure virtual call stopped every run that reached it),
and edges only reachable through such edges. Instruction coverage of the original is still reported.

Inputs are random "pointer soup" plus steering (tools/difftest/steer.py; --no-steer turns it off):
the constants compared before branches (both builds, +-1 / +-1 ulp) join the dictionary; uncovered
branches are replayed with the compare instrumented and the compared input location (argument, entry
register, pool field, .data global, malloc result) is set to the other operand (input-to-state);
mutation prefers inputs that reach uncovered branches; covered compares get boundary probes; fault
repair redirects bad pointers, small-ifies garbage indices and, as a last resort, maps input memory
at the faulting address. A run that stops in `_purecall` is resampled the same way instead of being
discarded: the dword that carried the vtable pointer (an argument, a pool object's vtable field or an
image .data global) and, when that is not the input's, the vtable slot itself get input soup, and the
first resample that gets past the pure call is kept.

Verdicts:
  PASS        no mismatch, >= K valid inputs (--min-valid, default 200) and every edge of BOTH builds
              covered except the listed unreachable ones;
  WEAK        no mismatch, but too few valid inputs or missed edges; the reason names the side(s) and
              how many edges each misses;
  PASS-FP / WEAK-FP  (--fp-tol only) PASS / WEAK where every mismatching input differed only by float rounding
              (judged against the original at 64-bit precision) or was out of float range; stats in the reason;
  FAIL        a mismatch (the first one is printed, its input saved for --replay). A pointer to our own
              copy of a constant/vtable/code label where the original stores the image's address is
              tolerated (with a note) only when the bytes at both are identical; otherwise it is a FAIL
              whose detail names the differing dwords;
  UNSUPPORTED unresolved reference, symbol not found, compile error, or almost every input discarded.
--gate insns restores the legacy gate (original's instruction coverage >= --min-cov, default 60%).
--call-regs is a diagnostic for a FAIL: it captures the whole live register file (eax ebx ecx edx esi edi ebp
esp, xmm0-3) at every recorded call site (depth-1 calls, imports, fake calls) and reports the first difference
that no tolerance explains (an address inside the function's own stack frame, the same pooled constant, our own
copy of a block with identical bytes, --ulp), with the call index, its callee and both values, in the result's
"call_regs" and the printed reason. It never changes a verdict; --call-regs-strict makes such a difference a
mismatch (only for an input whose every other observable agrees, and only at a call pair whose callee and
arguments agree).
"""
import argparse, collections, glob, json, math, os, random, struct, sys, time, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import capstone                                    # noqa: E402
try:
    os.nice(10)                                    # yield the CPU to interactive work
except OSError:
    pass
import numpy as np                                 # noqa: E402
from capstone import x86_const as CX               # noqa: E402
from unicorn import UcError, UC_HOOK_CODE, UC_HOOK_BLOCK, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_PROT_READ, UC_PROT_WRITE    # noqa: E402
from unicorn.x86_const import *                    # noqa: E402,F401,F403

import slice as S                                  # noqa: E402
import resolve as R
import resaudit as RA                              # noqa: E402
import regfacts as RF                                # noqa: E402
import cfgcov as CF                                # noqa: E402
import steer as ST                                 # noqa: E402
import sig as SIG                                  # noqa: E402
import machine as M                                # noqa: E402
from coff import Coff                              # noqa: E402

DEFAULT_INPUTS = 400
DEFAULT_K = 200
DEFAULT_C = 60.0                  # legacy --gate insns only
DEFAULT_BUDGET = 10_000_000
START_BUDGET = 100_000
BUDGET_PROBES = 3
FAIL_DIR = os.environ.get("DIFFTEST_FAIL_DIR") or os.path.join(S.ROOT, "work", "difftest", "fail_inputs")

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_md.detail = True

CS2UC = {}
for _n in ("EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP"):
    CS2UC[getattr(CX, "X86_REG_" + _n)] = globals()["UC_X86_REG_" + _n]

_ctx = {}


def context():
    """Image, symbol database, resolver and machine, built once per process."""
    if not _ctx:
        img = R.Image()
        db = R.SymbolDB(img)
        _ctx.update(img=img, db=db, res=R.Resolver(img, db), m=machine_hooks(M.Machine(img)), arity={}, ecxuse={})
    return _ctx


def machine_hooks(m):
    """Input-controlled allocation failure: the k-th malloc/operator new of a run returns NULL when k is
    in the input's alloc_fail (the null-check branches after `new` are edges like any other)."""
    h = m.handlers.get("malloc")
    if h is not None and h["fn"] is not None and not getattr(m, "_alloc_wrapped", False):
        inner = h["fn"]

        def malloc(m_, a, esp):
            k = m_.alloc_count
            m_.alloc_count += 1
            if k in m_.alloc_fail:
                return 0
            p = inner(m_, a, esp)
            if m_.alloc_log is not None:
                m_.alloc_log.append((k, p))
            return p
        h["fn"] = malloc
        m._alloc_wrapped = True
    m.alloc_fail, m.alloc_count, m.alloc_log = (), 0, None
    return m


# ====================================================================== preparation

class Target:
    pass


def unsupported(t, reason, **kw):
    t.verdict = "UNSUPPORTED"
    t.reason = reason
    for k, v in kw.items():
        setattr(t, k, v)
    return t


def modelled_symbols(t):
    """r.models of the resolver (agent B's model bindings; absent before that landed). Each entry is
    {"symbol", "kind", "addr", "size"}; kind "model-data"/"model-stub" is a harness model, "orig-code" is
    the original's own code at that address and is evidence, not a model."""
    ms = getattr(t, "models", None)
    if ms is None:
        ms = getattr(getattr(t, "res", None), "models", None)
    out = []
    for m in ms or ():
        sym, kind = (m.get("symbol"), m.get("kind")) if isinstance(m, dict) else \
            (getattr(m, "symbol", None), getattr(m, "kind", None))
        out.append({"symbol": sym, "kind": kind})
    return out


def model_note(ms):
    """'; N modelled symbol(s)' for the reason (only model-* kinds are models), or ''."""
    n = sum(1 for m in ms if str(m.get("kind") or "").startswith("model"))
    return "; %d modelled symbol(s)" % n if n else ""


def model_capped(v, ms):
    """A verdict that rests on a harness model can be no better than WEAK: a model proves the row runnable,
    never that our code is equivalent. A FAIL is already stronger evidence and is left alone."""
    if any(str(m.get("kind") or "").startswith("model") for m in ms):
        if v == "PASS":
            return "WEAK"
        if v == "PASS-FP":
            return "WEAK-FP"
    return v


def prepare(sid, va, flags_override=None, log=print, src=None):
    ctx = context()
    img = ctx["img"]
    t = Target()
    t.sid, t.va = sid, va
    t.verdict = None
    t.unresolved, t.eh_unresolved, t.conflicts, t.methods = [], [], [], {}
    t.flags, t.flags_how = S.choose_flags(sid, va, flags_override)
    # an override of only include/define options (`--flags "/Isrc/game/include"`, a flags-less manifest row) is an addition
    # to the defaults, not a replacement: alone it compiled without /O2 /MD /Gy /EHsc /TP and every callee read unresolved
    # (mig14 SPGraphicsLightingManager: bogus "34 unresolved reference(s)" UNSUPPORTED)
    if flags_override and t.flags and all(f.startswith(("/I", "-I", "/D", "-D")) for f in t.flags):
        t.flags, t.flags_how = list(S.DEFAULT_FLAGS) + [f for f in t.flags if f not in S.DEFAULT_FLAGS], t.flags_how + "+defaults"
    src = src or S.source_path(sid)
    t.src = src
    # a module TU (src/game/<dir>/<Stem>.cpp) includes `game/types.h` and the canonical headers: without /Isrc/game/include
    # (an agent passing the slice's flags, or none) the compile failed C1083 and the row read UNSUPPORTED (mig10 papercut)
    if os.path.normpath(src).replace(os.sep, "/").split("/src/game/")[-1] != os.path.normpath(src) or \
            os.path.normpath(src).replace(os.sep, "/").startswith("src/game/"):
        if not any(f.replace("\\", "/").rstrip("/").endswith("src/game/include") for f in t.flags if f.startswith("/I")):
            t.flags = list(t.flags) + ["/Isrc/game/include"]
    if not os.path.exists(src):
        return unsupported(t, "no source %s" % src)
    t.orig_lo, t.orig_hi = img.func_extent(va)
    obj, clog = S.compile_obj(src, t.flags)
    if obj is None and t.flags_how not in ("override", "default"):
        log("  compile failed with %s flags %s; retrying with defaults" % (t.flags_how, " ".join(t.flags)))
        t.flags, t.flags_how = list(S.DEFAULT_FLAGS), "default(after %s failed)" % t.flags_how
        obj, clog = S.compile_obj(src, t.flags)
    if obj is None:
        return unsupported(t, "compile failed: " + clog.strip().splitlines()[-1][:200] if clog.strip() else "compile failed")
    t.obj = obj
    coff = t.coff = Coff(obj)
    sym, how = S.find_symbol(coff, sid, va, src, t.orig_hi - t.orig_lo)
    if isinstance(sym, list):
        sym, how = best_by_size(coff, sym, t.orig_hi - t.orig_lo), how + " -> closest size"
    if sym is None:
        return unsupported(t, "our symbol not found: " + how)
    t.sym, t.sym_how = sym, how
    fsym = coff.by_name[sym]
    t.fsym = fsym
    annots = R.source_annotations([src] + glob.glob(os.path.join(S.slice_dir(sid), "*.h")) + S.sibling_headers(src))
    res = ctx["res"].resolve(coff, fsym, va, annots)
    t.res = res
    t.unresolved, t.eh_unresolved, t.conflicts = res.unresolved, res.eh_unresolved, res.conflicts
    t.methods = {}
    for nm, h in res.method.items():
        k = h.split("(")[0]
        t.methods[k] = t.methods.get(k, 0) + 1
    if res.unresolved:
        return unsupported(t, "%d unresolved reference(s): %s" % (len(res.unresolved), ", ".join(res.unresolved[:5])))
    # signature / argument layout
    try:
        t.sig = SIG.parse(sym)
    except SIG.SigError as e:
        t.sig = None
        t.sig_error = str(e)
    if not sym.startswith("?"):
        # extern "C": the symbol carries no types; read the C declaration from our source
        cname = sym[1:] if sym[:1] in "_@" else sym
        cname = cname.split("@")[0]
        hdrs = [src] + glob.glob(os.path.join(os.path.dirname(src), "*.h"))
        try:
            cs = SIG.parse_c_decl(cname, [open(h, encoding="utf-8", errors="replace").read() for h in hdrs],
                                  [open(h, encoding="utf-8", errors="replace").read() for h in include_headers()])
            pops = sum(n for _k, n in cs.params if n) if cs.params else 0
            stdn = int(sym.rsplit("@", 1)[1]) // 4 if "@" in sym[1:] and sym.rsplit("@", 1)[1].isdigit() else None
            if stdn is None or (stdn == pops and not any(k == "struct" for k, _n in cs.params)):
                t.sig, t.sig_how = cs, "C declaration"
        except (SIG.SigError, OSError) as e:
            t.sig_error = getattr(t, "sig_error", "") + "; C declaration: %s" % e
    analyse_original(t)
    build_layout(t)
    return t


_include_headers = []


def include_headers():
    """Headers under match/include (typedefs for C declarations), listed once per process."""
    if not _include_headers:
        for d, _sub, files in os.walk(os.path.join(S.ROOT, "match", "include")):
            _include_headers.extend(os.path.join(d, f) for f in files if f.endswith((".h", ".hpp")))
        _include_headers.append(os.devnull)
    return [h for h in _include_headers if h != os.devnull]


def best_by_size(coff, names, n0):
    best = None
    for nm in names:
        _s, a, b = coff.func_extent(coff.by_name[nm])
        d = abs((b - a) - n0)
        if best is None or d < best[0]:
            best = (d, nm)
    return best[1]


def analyse_original(t):
    """Recursive-descent disassembly of the original: instruction set (coverage denominator),
    callee-pop size, ECX/EDX use at entry, immediate dictionary for input generation."""
    img = context()["img"]
    lo, hi = t.orig_lo, t.orig_hi
    code = img.read(lo, hi - lo)
    relset = set(img.reloc_positions(lo, hi - lo))

    def rd(a, n):
        return code[a - lo:a - lo + n]

    norel = not img.relocs      # mount&blade.exe has no .reloc: a jump table is any in-function dword run of
                                # in-function targets (MSVC puts it after the body); without this the
                                # original's switch bodies were missing from its CFG, coverage and call sites

    def tt(a):
        if (norel or a in relset) and lo <= a <= hi - 4:
            return struct.unpack_from("<I", code, a - lo)[0]
        return None
    insns, tables = R.recursive_descent(rd, lo, hi, t.va, tt)
    t.orig_insns = insns
    t.orig_cfg = CF.Cfg(insns, t.va, lo, hi, tt)
    pops = set()
    consts = set()
    for a, ins in insns.items():
        if ins.id == CX.X86_INS_RET:
            pops.add(ins.operands[0].imm if ins.operands else 0)
        for op in ins.operands:
            if op.type == CX.X86_OP_IMM:
                v = op.imm & 0xFFFFFFFF
                if not any(a <= p < a + ins.size for p in relset):
                    consts.add(v)
    t.callee_pop = max(pops) if pops else 0
    t.pop_variants = sorted(pops)
    ordered = [insns[a] for a in sorted(insns)]
    t.orig_regs_written = RF.regs_written(ordered)   # registers the original never writes are compared
    t.orig_returns_eax = RF.returns_eax(ordered)     # a void declaration may hide an eax return
    t.dict = sorted(consts | {(c + 1) & 0xFFFFFFFF for c in consts if c < 0x10000} |
                    {(c - 1) & 0xFFFFFFFF for c in consts if 0 < c < 0x10000})[:4096]
    t.entry_reads = entry_reads(insns, t.va)


def entry_reads(insns, entry):
    """Registers among ecx/edx/eax read before written along the entry path (first 40 insns)."""
    read, written = set(), set()
    a = entry
    names = {CX.X86_REG_ECX: "ecx", CX.X86_REG_CX: "ecx", CX.X86_REG_CL: "ecx", CX.X86_REG_CH: "ecx",
             CX.X86_REG_EDX: "edx", CX.X86_REG_DX: "edx", CX.X86_REG_DL: "edx", CX.X86_REG_DH: "edx",
             CX.X86_REG_EAX: "eax", CX.X86_REG_AX: "eax", CX.X86_REG_AL: "eax", CX.X86_REG_AH: "eax"}
    for _ in range(40):
        ins = insns.get(a)
        if ins is None:
            break
        try:
            r, w = ins.regs_access()
        except capstone.CsError:
            break
        ops = ins.operands
        if ins.id in (CX.X86_INS_XOR, CX.X86_INS_SUB, CX.X86_INS_SBB) and len(ops) == 2 \
                and ops[0].type == CX.X86_OP_REG and ops[1].type == CX.X86_OP_REG and ops[0].reg == ops[1].reg:
            r = []          # zeroing (or carry-mask) idiom: the old value is not used
        if len(ops) == 2 and ops[0].type == CX.X86_OP_REG and ops[1].type == CX.X86_OP_IMM and \
                ((ins.id == CX.X86_INS_OR and ops[1].imm & 0xFFFFFFFF == 0xFFFFFFFF) or
                 (ins.id == CX.X86_INS_AND and ops[1].imm == 0)):
            r = []          # 'or eax,-1' / 'and eax,0': constant result
        if ins.id == CX.X86_INS_PUSH:
            r = []          # 'push ecx' is MSVC's 4-byte stack allocation / a save, not a use
        if len(ops) == 2 and ops[0].type == CX.X86_OP_REG and (
                (ins.id == CX.X86_INS_LEA and ops[1].type == CX.X86_OP_MEM and not ops[1].mem.disp
                 and not ops[1].mem.index and ops[1].mem.base == ops[0].reg) or
                (ins.id == CX.X86_INS_MOV and ops[1].type == CX.X86_OP_REG and ops[1].reg == ops[0].reg)):
            r = []          # 'lea ecx,[ecx]' / 'mov ecx,ecx': MSVC's alignment nop, not a use
                            # (static_init 0x548360 read ecx as an argument only through this nop)
        for x in r:
            n = names.get(x)
            if n and n not in written:
                read.add(n)
        for x in w:
            n = names.get(x)
            if n:
                written.add(n)
        if ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL) or ins.group(capstone.CS_GRP_RET):
            break
        a += ins.size
    return read


GPR32 = {CX.X86_REG_EAX: "eax", CX.X86_REG_EBX: "ebx", CX.X86_REG_ECX: "ecx", CX.X86_REG_EDX: "edx",
         CX.X86_REG_ESI: "esi", CX.X86_REG_EDI: "edi", CX.X86_REG_EBP: "ebp", CX.X86_REG_ESP: "esp"}


def reg_arg_use(insns, entry, reg, limit=40):
    """How the original uses the argument our declaration does not pass: (kind, address, instruction).
    'pointer' = the original addresses memory through it, 'int' = it is compared/masked/scaled, 'unknown' =
    copied around only. Follows fall-through (a conditional branch still runs one way) and `mov r32, reg`
    copies, so a this pointer parked in esi still counts; stops at jmp/ret."""
    aliases, int_ev, a = {reg}, None, entry
    for _ in range(limit):
        ins = insns.get(a)
        if ins is None:
            break
        ops, text = ins.operands, "%s %s" % (ins.mnemonic, ins.op_str)
        for op in ops:                      # memory addressed through the register: a pointer
            if op.type == CX.X86_OP_MEM:
                for rr in (op.mem.base, op.mem.index):
                    if GPR32.get(rr) in aliases:
                        return "pointer", a, text
        try:
            rd, wr = ins.regs_access()
        except capstone.CsError:
            break
        reads = {GPR32.get(x) for x in rd} & aliases
        writes = {GPR32.get(x) for x in wr} & aliases
        zeroed = ins.id in (CX.X86_INS_XOR, CX.X86_INS_SUB, CX.X86_INS_SBB) and len(ops) == 2 and \
            ops[0].type == CX.X86_OP_REG and ops[1].type == CX.X86_OP_REG and ops[0].reg == ops[1].reg
        if zeroed:
            reads = set()                   # 'xor eax,eax': the entry value is dead, not an integer argument
        if reads and int_ev is None and ins.id in (
                CX.X86_INS_CMP, CX.X86_INS_TEST, CX.X86_INS_AND, CX.X86_INS_OR, CX.X86_INS_IMUL,
                CX.X86_INS_SHL, CX.X86_INS_SHR, CX.X86_INS_SAR, CX.X86_INS_MOVZX, CX.X86_INS_MOVSX):
            int_ev = (a, text)              # compare/mask/scale of the value: an integer argument
                                            # (bare add/sub is left ambiguous: it is also pointer arithmetic)
        if writes and not reads:
            aliases -= writes               # overwritten: the entry value cannot be used after this
            if not aliases:
                break
        elif ins.id == CX.X86_INS_MOV and len(ops) == 2 and ops[0].type == CX.X86_OP_REG \
                and ops[1].type == CX.X86_OP_REG and GPR32.get(ops[1].reg) in aliases:
            aliases.add(GPR32[ops[0].reg])  # a copy keeps the value live in the destination
        if ins.group(capstone.CS_GRP_CALL):
            aliases -= {"eax", "ecx", "edx"}    # caller-saved: the callee clobbers them
            if not aliases:
                break
        if ins.group(capstone.CS_GRP_RET) or ins.id in (CX.X86_INS_JMP, CX.X86_INS_RET):
            break
        a += ins.size
    if int_ev:
        return "int", int_ev[0], int_ev[1]
    return "unknown", entry, "copied only"


def abi_fix(reg, kind, addr, text):
    """The concrete declaration change for a register argument: which register and which type."""
    what = {"pointer": "a pointer (the original addresses memory through it at %08x: %s)" % (addr, text),
            "int": "an integer (the original does arithmetic/compare on it at %08x: %s)" % (addr, text),
            "unknown": "a value (the original copies it at %08x: %s)" % (addr, text)}[kind]
    how = {"ecx": "make it a thiscall member (this arrives in ecx) or __fastcall with that first parameter",
           "edx": "declare it __fastcall with that pointer as the second parameter (the first would be in ecx)",
           "eax": "no MSVC declaration passes an argument in eax (an LTCG-only register argument; run with --ignore-abi)"}[reg]
    return "%s in %s: %s" % (what, reg, how)


def build_layout(t):
    """t.stack_kinds: kind per stack dword; t.reg_kinds: {reg: kind}; t.ret_kind."""
    sg = t.sig
    notes = []
    kinds = []
    regk = {}
    if sg is not None and sg.params is not None:
        conv = sg.conv
        if sg.member:
            regk["ecx"] = "this"
        if sg.ret == "hidden":
            kinds.append("retptr")
        for k, n in sg.params:
            if k == "struct":
                kinds.append(("struct", None))
            elif k in ("void", "varargs"):
                continue
            else:
                kinds.extend([k] * n if k not in ("double", "i64") else [k + "_lo", k + "_hi"])
        if conv == "fastcall":
            # first two dword-sized args go in ecx, edx
            for reg in ("ecx", "edx"):
                for i, k in enumerate(kinds):
                    if isinstance(k, str) and k in ("int", "ptr", "small", "bool", "unknown"):
                        regk[reg] = kinds.pop(i)
                        break
        nstruct = sum(1 for k in kinds if isinstance(k, tuple))
        known = sum(1 for k in kinds if not isinstance(k, tuple))
        pop_dw = t.callee_pop // 4
        if nstruct:
            if t.callee_pop and nstruct == 1 and pop_dw > known:
                each = pop_dw - known
            else:
                each = 3
                notes.append("by-value struct size unknown: assumed 3 dwords")
            out = []
            for k in kinds:
                out.extend(["soup"] * each if isinstance(k, tuple) else [k])
            kinds = out
        if t.callee_pop and len(kinds) != pop_dw and conv in ("stdcall", "thiscall", "fastcall"):
            notes.append("signature gives %d stack dwords, original pops %d" % (len(kinds), pop_dw))
            kinds = (kinds + ["soup"] * pop_dw)[:pop_dw]
        t.ret_kind = sg.ret
        t.conv = conv
    else:
        n = t.callee_pop // 4 if t.callee_pop else 6
        kinds = ["soup"] * n
        t.ret_kind = "unknown"
        t.conv = "stdcall?" if t.callee_pop else "cdecl?"
        notes.append("no parsed signature: %d soup stack dwords" % n)
    t.abi_mismatch = None
    for r in ("ecx", "edx", "eax"):
        if r in t.entry_reads and r not in regk:
            regk[r] = "soup"
            notes.append("original reads %s at entry (register argument?)" % r)
            if sg is not None and sg.params is not None:
                kind, at, text = reg_arg_use(t.orig_insns, t.va, r)
                t.abi_mismatch = "original reads %s at entry but our declaration passes nothing there " \
                                 "(custom/LTCG register ABI?); fix: pass %s" % (r, abi_fix(r, kind, at, text))
    t.void_hides_eax = False
    if t.ret_kind == "void" and getattr(t, "orig_returns_eax", False):
        # Seven hybrid defects (2026-10-12) were `void` declarations of functions whose original
        # hands a value back in eax (hidden return pointer, `return this`, popped node, copy end);
        # LTCG callers use it. Compare eax as an unknown-kind return and say why.
        t.ret_kind = "unknown"
        t.void_hides_eax = True
        notes.append("declared void but the original writes eax before every ret: eax compared (callers may use it)")
    t.stack_kinds = kinds
    t.reg_kinds = regk
    t.layout_notes = notes


# ====================================================================== inputs

PROFILES = {
    "mixed":    (("ptr", 30), ("zero", 20), ("small", 15), ("float", 20), ("dict", 5), ("rand", 10)),
    "floaty":   (("ptr", 15), ("zero", 10), ("small", 5), ("float", 65), ("dict", 2), ("rand", 3)),
    "zeroish":  (("ptr", 20), ("zero", 60), ("small", 10), ("float", 5), ("dict", 5)),
    "pointery": (("ptr", 60), ("zero", 20), ("small", 10), ("float", 5), ("dict", 5)),
    "smallint": (("ptr", 20), ("zero", 30), ("small", 40), ("float", 5), ("dict", 5)),
}
SPECIAL_F = [0.0, -0.0, 1.0, -1.0, 0.5, 2.0, 1e-30, 1e30, float("inf"), float("-inf"), float("nan")]


def f2u(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


class Input:
    """One test input. Input objects live in the lazily materialised pool: the content of every
    4 KB pool page is a pure function of (seed, profile, page) plus `pool_over` edits, so both
    runs see identical bytes wherever they look. Pointers inside generated pages lead to random
    64 KB slots (distinct objects; aliasing only by chance or with --alias). `overrides` are image
    .data dwords set for this input by fault repair. `pagegen` selects the page generator
    (1: the original per-dword Python one, kept so older saved inputs replay identically)."""

    pagegen = 2

    def copy(self):
        n = Input()
        n.seed, n.profile, n.args, n.pad = self.seed, self.profile, list(self.args), list(self.pad)
        n.pool_over, n.regs, n.xmm, n.overrides = dict(self.pool_over), dict(self.regs), list(self.xmm), dict(self.overrides)
        n.origin, n.touched = self.origin, list(self.touched)
        n.pagegen = self.pagegen
        n.alloc_fail = tuple(getattr(self, "alloc_fail", ()))
        n.extra = dict(getattr(self, "extra", {}))
        return n


NSLOTS = M.POOL_SIZE // M.SLOT
HOT_SLOTS = 8          # slots 0..7 hold the argument / `this` objects


class InputGen:
    """Random inputs ('pointer soup' objects + typed arguments) and their mutations."""

    def __init__(self, t, alias=0.0, nan=False, code=(), fp_real=False):
        self.t = t
        self.alias = alias
        self.fp_real = fp_real      # --fp-realistic: float fields are finite magnitudes in [1e-3, 1e5]
        self.nan = nan
        # Generated values never point into real code (image .text, our object's code): a soup
        # "method pointer" taken from the original's immediates (a jump target inside the function
        # itself) would otherwise run part of the original on the wrong frame. Such values are
        # moved to the unmapped upper half (bit 31 flipped), where a call becomes a fake method.
        self.code = list(code)
        self.dic = [self.fin(v) for v in (t.dict or [0])]
        self.specials = SPECIAL_F if nan else SPECIAL_F[:8]
        self.slot_cache = {}

    def fin(self, v):
        """Unless --nan: never produce a dword whose float32 reading is NaN/Inf (clears the top
        exponent bit), since NaN comparisons are not preserved across equivalent spellings."""
        if not self.nan and (v >> 23) & 0xFF == 0xFF:
            v &= 0xFF7FFFFF
        for lo, hi in self.code:
            if lo <= v < hi:
                return v ^ 0x80000000
        return v

    def slot_ptr(self, rng):
        if self.alias and rng.random() < self.alias:
            slot = rng.randrange(HOT_SLOTS)
        else:
            slot = HOT_SLOTS + rng.randrange(NSLOTS - HOT_SLOTS)
        base = M.POOL_BASE + slot * M.SLOT
        return base if rng.random() < 0.7 else base + 4 * rng.randrange(64)

    def soup(self, rng, profile):
        return self.fin(self._soup(rng, profile))

    def _soup(self, rng, profile):
        kinds, weights = PROFILE_KW[profile]
        k = rng.choices(kinds, weights)[0]
        if k == "ptr":
            return self.slot_ptr(rng)
        if k == "zero":
            return 0
        if k == "small":
            return rng.randrange(9) if rng.random() < 0.9 else 0xFFFFFFFF
        if k == "rand" and self.fp_real and rng.random() < 0.5:
            return f2u(self.freal(rng))
        if k == "float" and self.fp_real:
            return f2u(self.freal(rng))
        if k == "float":
            r = rng.random()
            if r < 0.6:
                return f2u(rng.uniform(-4, 4))
            if r < 0.85:
                return f2u(rng.uniform(-1000, 1000))
            return f2u(rng.choice(SPECIAL_F[:8]))
        if k == "dict":
            return rng.choice(self.dic)
        return rng.getrandbits(32)

    @staticmethod
    def freal(rng):
        """--fp-realistic float: exact 0/+-1 (10%) or +-10^U(-3, 5), the magnitudes of camera/transform data."""
        if rng.random() < 0.1:
            return rng.choice((0.0, 1.0, -1.0))
        return rng.choice((-1.0, 1.0)) * 10.0 ** rng.uniform(-3, 5)

    def slot_pages(self, inp, slot):
        """{page: 4 KB} for the 16 pages of one 64 KB pool slot."""
        pages = range(slot, slot + M.SLOT, 0x1000)
        if inp.pagegen == 1:
            return {p: self.page_v1(inp, p) for p in pages}
        # Fault repair re-runs an input many times with a few pool_over edits: cache the soup.
        key = (inp.seed, inp.profile, slot)
        b = self.slot_cache.get(key)
        if b is None:
            if len(self.slot_cache) >= 512:
                self.slot_cache.clear()
            b = self.slot_cache[key] = self._slot_soup(inp, slot)
        over = [(a, v) for a, v in inp.pool_over.items() if slot <= a < slot + M.SLOT]
        if over:
            b = bytearray(b)
            for a, v in over:
                struct.pack_into("<I", b, a - slot, v)
            b = bytes(b)
        return {pg: b[pg - slot:pg - slot + 0x1000] for pg in pages}

    def _slot_soup(self, inp, slot):
        rng = np.random.default_rng([inp.seed, slot])
        kinds, weights = PROFILE_KW[inp.profile]
        p = np.array(weights, dtype=float)
        n_all = M.SLOT // 4
        k = rng.choice(len(kinds), n_all, p=p / p.sum())
        w = np.zeros(n_all, dtype=np.int64)
        for idx, kind in enumerate(kinds):
            sel = k == idx
            n = int(sel.sum())
            if n and kind != "zero":
                w[sel] = self._soup_n(rng, kind, n)
        w = w.astype(np.uint32)
        if not self.nan:
            w[((w >> 23) & 0xFF) == 0xFF] &= np.uint32(0xFF7FFFFF)
        for lo, hi in self.code:
            w[(w >= lo) & (w < hi)] ^= np.uint32(0x80000000)
        return w.astype("<u4").tobytes()

    def _soup_n(self, rng, kind, n):
        """n dwords of one soup kind, with the same distributions as _soup."""
        if kind == "ptr":
            hot = rng.integers(0, HOT_SLOTS, n)
            cold = HOT_SLOTS + rng.integers(0, NSLOTS - HOT_SLOTS, n)
            slot = np.where(rng.random(n) < self.alias, hot, cold) if self.alias else cold
            off = np.where(rng.random(n) < 0.7, 0, 4 * rng.integers(0, 64, n))
            return M.POOL_BASE + slot * M.SLOT + off
        if kind == "small":
            return np.where(rng.random(n) < 0.9, rng.integers(0, 9, n), 0xFFFFFFFF)
        if kind == "rand" and self.fp_real:
            w = rng.integers(0, 1 << 32, n, dtype=np.int64)
            sel = rng.random(n) < 0.5
            return np.where(sel, self._freal_n(rng, n), w)
        if kind == "float" and self.fp_real:
            return self._freal_n(rng, n)
        if kind == "float":
            r = rng.random(n)
            f = np.where(r < 0.6, rng.uniform(-4, 4, n),
                         np.where(r < 0.85, rng.uniform(-1000, 1000, n), rng.choice(SPECIAL_F[:8], n)))
            return f.astype(np.float32).view(np.uint32).astype(np.int64)
        if kind == "dict":
            return np.array(self.dic, dtype=np.int64)[rng.integers(0, len(self.dic), n)]
        return rng.integers(0, 1 << 32, n, dtype=np.int64)

    def _freal_n(self, rng, n):
        mag = 10.0 ** rng.uniform(-3, 5, n)
        f = np.where(rng.random(n) < 0.5, -mag, mag)
        f = np.where(rng.random(n) < 0.1, rng.choice([0.0, 1.0, -1.0], n), f)
        return f.astype(np.float32).view(np.uint32).astype(np.int64)

    def page_v1(self, inp, page):
        rng = random.Random((inp.seed << 32) ^ page)
        words = [self.soup(rng, inp.profile) for _ in range(1024)]
        for a, v in inp.pool_over.items():
            if page <= a < page + 0x1000:
                words[(a - page) // 4] = v
        return struct.pack("<1024I", *words)

    def ptr(self, rng, hot):
        if rng.random() < 0.04:
            return 0
        return M.POOL_BASE + M.SLOT * hot

    def scalar(self, k, rng, profile, hot=1):
        return self.fin(self._scalar(k, rng, profile, hot))

    def _scalar(self, k, rng, profile, hot=1):
        if k in ("this", "ptr", "retptr"):
            return self.ptr(rng, hot)
        if k == "int":
            r = rng.random()
            if r < 0.4:
                return rng.randrange(9)
            if r < 0.65:
                return rng.choice(self.dic)
            if r < 0.7:
                return rng.choice([0xFFFFFFFF, 0x7FFFFFFF, 0x80000000])
            if r < 0.85:
                return rng.randrange(1000)
            return rng.getrandbits(32)
        if k == "small":
            return rng.randrange(256) if rng.random() < 0.7 else rng.choice(self.dic) & 0xFFFF
        if k == "bool":
            return rng.randrange(2)
        if k == "float" and self.fp_real:
            return f2u(self.freal(rng))
        if k == "float":
            r = rng.random()
            if r < 0.5:
                return f2u(rng.uniform(-10, 10))
            if r < 0.8:
                return f2u(rng.choice(SPECIAL_F[:6]))
            if r < 0.95:
                return f2u(rng.uniform(-1000, 1000))
            return f2u(rng.choice(self.specials))
        return self.soup(rng, profile)

    def args(self, rng, profile):
        words, pending_hi = [], None
        for i, k in enumerate(self.t.stack_kinds):
            if k in ("double_lo", "i64_lo"):
                if k == "double_lo":
                    x = rng.uniform(-10, 10) if rng.random() < 0.7 else rng.choice(self.specials)
                    lo, hi = struct.unpack("<II", struct.pack("<d", x))
                else:
                    v = rng.choice([rng.randrange(9), rng.getrandbits(64), rng.choice(self.dic)])
                    lo, hi = v & 0xFFFFFFFF, (v >> 32) & 0xFFFFFFFF
                words.append(lo)
                pending_hi = hi
            elif k in ("double_hi", "i64_hi"):
                words.append(pending_hi if pending_hi is not None else 0)
                pending_hi = None
            else:
                words.append(self.scalar(k, rng, profile, hot=2 + i % (HOT_SLOTS - 2)))
        return words

    def fresh(self, rng):
        inp = Input()
        inp.seed = rng.getrandbits(32)
        inp.profile = rng.choice(list(PROFILES))
        inp.origin = "fresh"
        inp.pool_over = {}
        inp.touched = []
        inp.args = self.args(rng, inp.profile)
        inp.pad = [self.soup(rng, inp.profile) for _ in range(4)]
        inp.regs = {}
        for r in ("eax", "ecx", "edx"):
            k = self.t.reg_kinds.get(r)
            hot = {"ecx": 1, "edx": 7, "eax": 6}[r]
            inp.regs[r] = self.scalar(k, rng, inp.profile, hot) if k else self.soup(rng, inp.profile)
        inp.xmm = []
        for _ in range(8):
            v = 0
            for j in range(4):
                v |= f2u(rng.uniform(-10, 10)) << (32 * j)
            inp.xmm.append(v)
        inp.overrides = {}
        inp.alloc_fail = ()
        inp.extra = {}
        return inp

    def mutate(self, parent, rng, corpus):
        """Coverage-guided fuzzing step: 1-4 small edits of a corpus input."""
        inp = parent.copy()
        inp.origin = "mutant"
        kinds = self.t.stack_kinds
        for _ in range(rng.randint(1, 4)):
            op = rng.random()
            if op < 0.35 and inp.args:
                i = rng.randrange(len(inp.args))
                k = kinds[i] if i < len(kinds) else "soup"
                if isinstance(k, str) and k.endswith(("_lo", "_hi")):
                    k = "soup"
                r = rng.random()
                if k in ("int", "small", "soup") and r < 0.4:
                    inp.args[i] = self.fin((inp.args[i] + rng.choice([-1, 1, -2, 2, 8, -8, 16, 100])) & 0xFFFFFFFF)
                elif r < 0.6:
                    inp.args[i] = rng.choice(self.dic)
                else:
                    inp.args[i] = self.scalar(k, rng, inp.profile, hot=2 + i % (HOT_SLOTS - 2))
            elif op < 0.8 and inp.touched:
                page = rng.choice(inp.touched)
                j = page + 4 * rng.randrange(1024)
                for jj in range(j, min(page + 0x1000, j + 4 * rng.choice([1, 1, 2, 4, 16])), 4):
                    r = rng.random()
                    if r < 0.2:
                        inp.pool_over[jj] = rng.choice(self.dic)
                    elif r < 0.3 and jj in inp.pool_over:
                        inp.pool_over[jj] = self.fin((inp.pool_over[jj] + rng.choice([-1, 1, 2, -2, 4])) & 0xFFFFFFFF)
                    else:
                        inp.pool_over[jj] = self.soup(rng, rng.choice(list(PROFILES)))
            elif op < 0.88:
                inp.seed = rng.getrandbits(32)      # same args, new object contents
            elif op < 0.94 and len(corpus) > 1:
                other = rng.choice(corpus)
                inp.pool_over.update(other.pool_over)
            else:
                r = rng.choice(["eax", "ecx", "edx"])
                k = self.t.reg_kinds.get(r)
                if k:
                    inp.regs[r] = self.scalar(k, rng, inp.profile, {"ecx": 1, "edx": 7, "eax": 6}[r])
        return inp


PROFILE_KW = {k: tuple(zip(*v)) for k, v in PROFILES.items()}


# ====================================================================== running

REGS = {"eax": UC_X86_REG_EAX, "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX}
CALLEE_SENTINEL = {UC_X86_REG_EBX: 0x0BB0BB00, UC_X86_REG_ESI: 0x05150515, UC_X86_REG_EDI: 0x0D1D0D1D,
                   UC_X86_REG_EBP: 0x0E0B0E0B}
STACK_FILL = b"\xcc"
STACK_END = M.STACK_TOP + 0x1000           # end of the stack mapping (one guard-free page above the top)

# --call-regs: the whole live register file at a call site (the GPRs the trace already compares, plus the
# first four xmm registers). NEVER compared by default: a call is compared through its callee, this/edx and
# stack arguments, so intermediate register state that reaches no call, store or return is invisible.
# Values that are not data: MSVC's uninitialised-stack fills and the sentinels a build leaves in a register that
# never carried an input. A register difference against one of these is a dead value, not a divergence.
FILL_PATTERNS = frozenset((0xCCCCCCCC, 0xCDCDCDCD, 0xDDDDDDDD, 0xFDFDFDFD, 0x0BB0BB00, 0x0D1D0D1D,
                           0xBAADF00D, 0xFEEEFEEE, 0xABABABAB, 0xDEADBEEF))

CALL_REGS = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")
CALL_XMM = ("xmm0", "xmm1", "xmm2", "xmm3")
CALL_REG_IDS = {"eax": UC_X86_REG_EAX, "ebx": UC_X86_REG_EBX, "ecx": UC_X86_REG_ECX, "edx": UC_X86_REG_EDX,
                "esi": UC_X86_REG_ESI, "edi": UC_X86_REG_EDI, "ebp": UC_X86_REG_EBP, "esp": UC_X86_REG_ESP}
CALL_XMM_IDS = {"xmm%d" % k: UC_X86_REG_XMM0 + k for k in range(len(CALL_XMM))}


def call_reg_snapshot(uc):
    """The register file at a call site (--call-regs): {name: int}, xmm as the full 128-bit value."""
    snap = {r: uc.reg_read(i) for r, i in CALL_REG_IDS.items()}
    snap.update({r: uc.reg_read(i) for r, i in CALL_XMM_IDS.items()})
    return snap


class ImportTrace(list):
    """machine.py records one import call as (name, args) and appends it here. With --call-regs the register
    file at that call is captured in the same order: the append happens inside machine's import hook, so the
    values are the ones at the stub entry."""

    def __init__(self, uc, regs):
        super().__init__()
        self.uc, self.regs = uc, regs

    def append(self, ent):
        list.append(self, ent)
        self.regs.append(call_reg_snapshot(self.uc))


class Side:
    """One installed function (original or ours): entry, ranges, call-site hooks."""

    def __init__(self, name, entry, ranges, sites, cfg=None, span=None):
        self.name, self.entry, self.ranges, self.sites = name, entry, ranges, sites
        self.cfg, self.span = cfg, span         # edge model and [lo, hi) of the tested function itself
        self.edges = set()                      # edges covered by valid runs
        self.site_hits, self.site_returned, self.site_stops = {}, {}, {}

    def inside(self, a):
        return any(lo <= a < hi for lo, hi in self.ranges)


class Runner:
    def __init__(self, t, opts, log=print):
        self.t, self.opts, self.log = t, opts, log
        ctx = context()
        self.img, self.m = ctx["img"], ctx["m"]
        self.arity_cache = ctx["arity"]
        self.ecx_cache = ctx["ecxuse"]
        m = self.m
        self.thunk_targets = {}
        obj = m.load_object(t.coff, t.res, t.fsym)
        self.obj = obj
        self.decl_arity = {}      # callee VA -> stack dwords, from our declaration's mangled name
        self.decl_prefix = {}     # callee VA -> comparable leading stack dwords (before a by-value struct)
        for si, va_ in t.res.addr.items():
            try:
                sg = SIG.parse(t.coff.syms[si].name)
            except Exception:
                continue
            if sg.params is None:
                continue
            n_ = 1 if sg.ret == "hidden" else 0
            for k_, d_ in sg.params:      # dwords before the first by-value struct (its size and
                if k_ == "struct":        # padding bytes are unknown, so it is never compared)
                    break
                n_ += d_
            self.decl_prefix.setdefault(va_, n_)
            if sg.conv == "cdecl" and not sg.variadic and not any(k_ == "struct" for k_, _d in sg.params):
                self.decl_arity.setdefault(va_, n_)
        self.our_insns = {}
        self.code_cache = {}      # image function start -> (sorted addrs, insns) for fake-call arity
        self.targets_cache = {}   # id(insns) -> addresses a branch/call in that code lands on (block starts)
        self.fakes = []
        self.orig = Side("orig", t.va, [(t.orig_lo, t.orig_hi)], self._sites_orig(), t.orig_cfg, (t.orig_lo, t.orig_hi))
        self.ours = Side("ours", obj["entry"], obj["code"], self._sites_ours(), self._cfg_ours(), obj["func_range"])
        for side in (self.orig, self.ours):
            side.cfg_static_unreachable = {e for e, k in side.cfg.x87_facts().items() if k == "infeasible" or not opts.nan}
        self.hooks = []
        self.cur = None
        self.calls = []
        # Original's instruction cap: starts low (random inputs often send a loop into the
        # billions) and rises to --budget if a probe shows a real input needs more; see run_function.
        self.budget = min(getattr(opts, "start_budget", opts.budget), opts.budget)
        for side in (self.orig, self.ours):
            for a, info in side.sites.items():
                self.hooks.append(m.uc.hook_add(UC_HOOK_CODE, self._on_site, user_data=(side, info), begin=a, end=a))
        # --transparent: image functions whose call is replaced in the depth-1 trace by the calls they make
        self.transparent = set(getattr(opts, "transparent", None) or ())
        self.tr_stack, self.tr_block = [], None
        for tva in sorted(self.transparent):
            lo, hi = self.img.func_extent(tva)
            code = self.img.read(lo, hi - lo)
            insns, _ = R.recursive_descent(lambda a, k, lo=lo, code=code: code[a - lo:a - lo + k], lo, hi, tva, lambda a: None)
            for a, ins in insns.items():
                if ins.id == CX.X86_INS_CALL or (ins.id == CX.X86_INS_JMP and not self._jmp_internal(ins, lo, hi)):
                    self.hooks.append(m.uc.hook_add(UC_HOOK_CODE, self._on_tsite, user_data=self._site_info(ins, self.img.read),
                                                    begin=a, end=a))
                    r = a + ins.size
                    self.hooks.append(m.uc.hook_add(UC_HOOK_CODE, self._on_tret, begin=r, end=r))
        self.blocks = set()
        self.run_blocks = set()
        self.page_cache = {}
        code = [(a, a + n) for nm, a, n, ch in self.img.sections if ch & 0x20000000]
        self.gen = InputGen(t, opts.alias, opts.nan, code + list(obj["code"]), getattr(opts, "fp_realistic", False))
        self.cov_on = False
        self.cov_side = None
        self.prev_end = None
        self.run_edges = set()
        self.last_site = None
        self.run_sites = {}
        self.reads = None
        for side in (self.orig, self.ours):
            self.hooks.append(m.uc.hook_add(UC_HOOK_BLOCK, self._on_block, user_data=side,
                                            begin=side.span[0], end=side.span[1] - 1))
        nst = len(t.stack_kinds) + 4
        self.frame_dw = nst
        self.esp_entry = M.STACK_TOP - 0x2000 - 4 * (1 + nst)
        self.esp_entry &= ~0xF
        self.esp_entry -= 4         # as after a call from 16-aligned code
        self.args_hi = self.esp_entry + 4 * (1 + nst)
        # The function owns its incoming argument slots (callers never read them back, compilers
        # reuse them as scratch), so pointers into them count as stack pointers ("STK") like
        # pointers into the frame. The 4 padding dwords above them belong to the caller.
        self.stk_hi = self.esp_entry + 4 + 4 * len(t.stack_kinds)
        # read-only constants (image .rdata, our object's code/constants): pointers to byte-identical
        # contents compare equal (the same literal pooled at two addresses)
        self.ro = [(a, a + n) for nm, a, n, ch in self.img.sections if nm == ".rdata"]
        self.ro += [(lo, hi) for lo, hi in m.readonly if obj["map_lo"] <= lo < obj["map_hi"]]

    def close(self):
        for h in self.hooks:
            self.m.uc.hook_del(h)
        self.hooks = []

    # --- static site discovery
    def _site_info(self, ins, read_next):
        nxt = read_next(ins.address + ins.size, 6)
        add = 0
        if nxt[:2] == b"\x83\xc4":
            add = nxt[2] // 4
        elif nxt[:2] == b"\x81\xc4":
            add = struct.unpack_from("<I", nxt, 2)[0] // 4
        return {"ins": ins, "add_esp": add, "is_jmp": ins.id == CX.X86_INS_JMP}

    def _sites_orig(self):
        t = self.t
        sites = {}
        for a, ins in t.orig_insns.items():
            if ins.id == CX.X86_INS_CALL or (ins.id == CX.X86_INS_JMP and not self._jmp_internal(ins, t.orig_lo, t.orig_hi)):
                sites[a] = self._site_info(ins, self.img.read)
        return sites

    def _jmp_internal(self, ins, lo, hi):
        op = ins.operands[0]
        if op.type == CX.X86_OP_IMM:
            return lo <= (op.imm & 0xFFFFFFFF) < hi
        if op.type == CX.X86_OP_MEM and op.mem.index != 0:
            return True        # jump table
        return False

    def _cfg_ours(self):
        """Edge model of OUR tested function (its own COMDAT section, from its entry)."""
        m, obj, coff = self.m, self.obj, self.t.coff
        lo, hi = obj["func_range"]
        i = self.t.fsym.secno - 1
        a0 = obj["sec_addr"][i]
        relpos = {a0 + off for off, _si, typ in coff.sections[i].relocs if typ in (0x06,)}

        def rd(a, n):
            return m.read(a, min(n, hi - a))

        def tt(a):
            if a in relpos:
                v = m.u32(a)
                return v if lo <= v < hi else None
            return None
        insns, _tb = R.recursive_descent(rd, lo, hi, obj["entry"], tt)
        self.ours_func_insns = insns
        return CF.Cfg(insns, obj["entry"], lo, hi, tt)

    def _sites_ours(self):
        m, obj, coff = self.m, self.obj, self.t.coff
        sites = {}
        for i, a0 in obj["sec_addr"].items():
            sec = coff.sections[i]
            if not sec.is_code:
                continue
            lo, hi = a0, a0 + len(sec.data)
            relpos = {a0 + off for off, _si, typ in sec.relocs if typ in (0x06,)}

            def rd(a, n, lo=lo, hi=hi):
                return m.read(a, min(n, hi - a))

            def tt(a, relpos=relpos, lo=lo, hi=hi):
                if a in relpos:
                    v = m.u32(a)
                    return v if lo <= v < hi else None
                return None
            starts = [a0 + s.value for s in coff.syms.values() if s.secno - 1 == i and s.is_func]
            for st in starts or [a0]:
                insns, _tb = R.recursive_descent(rd, lo, hi, st, tt)
                self.our_insns.update(insns)
                for a, ins in insns.items():
                    if ins.id == CX.X86_INS_CALL or (ins.id == CX.X86_INS_JMP and not self._jmp_internal(ins, lo, hi)):
                        sites[a] = self._site_info(ins, m.read)
        return sites

    # --- hooks
    def _on_block(self, uc, address, size, side):
        if not self.cov_on or side is not self.cov_side:
            return
        self.run_blocks.add((address, size))
        pe = self.prev_end
        if pe is not None:
            br = side.cfg.branch_end.get(pe)
            if br is not None:
                src, kind, fall = br
                if kind == "jcc":
                    tgt = side.cfg.insns[src].operands[0].imm & 0xFFFFFFFF
                    self.run_edges.add((src, fall) if address == fall else (src, tgt))
                elif (src, address) in side.cfg.edges:
                    self.run_edges.add((src, address))
        self.prev_end = address + size

    def _eval_target(self, ins):
        uc = self.m.uc
        op = ins.operands[0]
        if op.type == CX.X86_OP_IMM:
            return op.imm & 0xFFFFFFFF
        if op.type == CX.X86_OP_REG:
            return uc.reg_read(CS2UC[op.reg])
        if op.type == CX.X86_OP_MEM:
            mm = op.mem
            a = mm.disp
            if mm.base:
                a += uc.reg_read(CS2UC[mm.base])
            if mm.index:
                a += uc.reg_read(CS2UC[mm.index]) * mm.scale
            if mm.segment:
                return None
            try:
                return self.m.u32(a & 0xFFFFFFFF)
            except Exception:
                return None
        return None

    def _on_site(self, uc, address, size, data):
        side, info = data
        if side is not self.cur:
            return
        if side.span[0] <= address < side.span[1]:
            self.last_site = address
            self.run_sites[address] = self.run_sites.get(address, 0) + 1
        tgt = self._eval_target(info["ins"])
        if tgt is None:
            return
        if side.inside(tgt) or M.STUB_BASE <= tgt < self.m.stub_end:
            return
        if self._thunk(tgt):
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        while self.tr_stack and esp >= self.tr_stack[-1]:
            self.tr_stack.pop()                         # transparent calls that already returned
        self.tr_block = None                            # (also after a transparent function's tail jump)
        if tgt in self.transparent:
            self.tr_stack.append(esp)
            return
        self._record_call(uc, tgt, info, esp)

    def _on_tsite(self, uc, address, size, info):
        """A call inside a --transparent function: recorded as depth 1 when every frame between the tested
        function and this call is transparent (not inside a recorded callee)."""
        if self.cur is None:
            return
        esp = uc.reg_read(UC_X86_REG_ESP)
        while self.tr_stack and esp >= self.tr_stack[-1]:
            self.tr_stack.pop()
        if not self.tr_stack or self.tr_block is not None:
            return
        tgt = self._eval_target(info["ins"])
        if tgt is None or self._thunk(tgt):
            return
        if tgt in self.transparent:
            self.tr_stack.append(esp)
            return
        self.tr_block = esp
        self._record_call(uc, tgt, info, esp)

    def _on_tret(self, uc, address, size, data):
        """Return address of a call inside a transparent function: the recorded callee has returned."""
        if self.tr_block is not None and uc.reg_read(UC_X86_REG_ESP) >= self.tr_block:
            self.tr_block = None

    def _record_call(self, uc, tgt, info, esp):
        try:
            words = struct.unpack("<8I", self.m.read(esp, 32))
        except Exception:
            words = (0,) * 8
        self.calls.append((tgt, uc.reg_read(UC_X86_REG_ECX), uc.reg_read(UC_X86_REG_EDX), words,
                           info["add_esp"], info["is_jmp"]))
        if self.opts.call_regs:
            self.call_regs.append(call_reg_snapshot(uc))

    def _thunk(self, va):
        r = self.thunk_targets.get(va)
        if r is None:
            r = False
            if self.img.in_text(va):
                b = self.img.read(va, 2)
                r = b == b"\xff\x25"
            self.thunk_targets[va] = r
        return r

    # --- callee facts for call-trace normalisation
    def callee_arity(self, va):
        """Dwords the callee pops (ret N), or None for cdecl/unknown."""
        if va in self.arity_cache:
            return self.arity_cache[va]
        n = None
        if self.img.in_text(va):
            lo, hi = self.img.func_extent(va)
            if lo == va:
                code = self.img.read(lo, hi - lo)

                def rd(a, k):
                    return code[a - lo:a - lo + k]
                insns, _ = R.recursive_descent(rd, lo, hi, va, lambda a: None)
                pops = {ins.operands[0].imm if ins.operands else 0 for ins in insns.values() if ins.id == CX.X86_INS_RET}
                if pops and max(pops):
                    n = max(pops) // 4
        self.arity_cache[va] = n
        return n

    def callee_regs(self, va):
        if va in self.ecx_cache:
            return self.ecx_cache[va]
        r = set()
        if self.img.in_text(va):
            lo, hi = self.img.func_extent(va)
            if lo == va:
                code = self.img.read(lo, min(hi - lo, 400))
                insns = {}
                for ins in _md.disasm(code, lo):
                    insns[ins.address] = ins
                r = entry_reads(insns, va)
        self.ecx_cache[va] = r
        return r

    def abi_ok(self, o):
        """Callee-saved registers back at their sentinels and esp popped exactly the declared args."""
        if any(o[nm] != CALLEE_SENTINEL[r] for nm, r in M.CALLEE_SAVED):
            return False
        return o["esp"] == 4 + self.t.callee_pop

    def in_ro(self, v):
        return any(lo <= v < hi for lo, hi in self.ro)

    def norm_key(self, v):
        """Run-independent identity of a value for hashing: stack pointers -> 'STK', pointers to
        read-only constants -> their contents (C string, else 64 bytes)."""
        if in_stack_frame(v, self.stk_hi):
            return "STK"
        if self.in_ro(v):
            try:
                b = self.m.read(v, 64)
            except Exception:
                return v
            z = b.find(b"\0")
            return ("RO", b[:z] if z > 0 else b)
        return v

    # --- calls through generated function pointers ("fake methods")
    def _insns_around(self, site):
        """(sorted addresses, insns) of the code containing site."""
        if self.t.orig_lo <= site < self.t.orig_hi:
            key = "orig"
            if key not in self.code_cache:
                self.code_cache[key] = (sorted(self.t.orig_insns), self.t.orig_insns)
            return self.code_cache[key]
        if any(lo <= site < hi for lo, hi in self.obj["code"]):
            if "ours" not in self.code_cache:
                self.code_cache["ours"] = (sorted(self.our_insns), self.our_insns)
            return self.code_cache["ours"]
        if self.img.in_text(site):
            import bisect
            st = self.img.starts
            i = bisect.bisect_right(st, site) - 1
            if i < 0:
                return None
            f = st[i]
            if f not in self.code_cache:
                lo, hi = self.img.func_extent(f)
                code = self.img.read(lo, max(hi - lo, 1))
                rel = set(self.img.reloc_positions(lo, hi - lo))

                def rd(a, n, lo=lo, code=code):
                    return code[a - lo:a - lo + n]

                def tt(a, lo=lo, hi=hi, code=code, rel=rel):
                    return struct.unpack_from("<I", code, a - lo)[0] \
                        if (a in rel or not self.img.relocs) and lo <= a <= hi - 4 else None
                insns, _ = R.recursive_descent(rd, lo, hi, f, tt)
                self.code_cache[f] = (sorted(insns), insns)
            return self.code_cache[f]
        return None

    def block_starts(self, addrs, insns):
        """Addresses a branch or call in this code can land on: the start of a basic block. `addrs[0]`
        (the function's entry in the image) is a block start too. Attributed here once per code blob."""
        key = id(insns)
        t = self.targets_cache.get(key)
        if t is None:
            t = {addrs[0]} if addrs else set()
            for ins in insns.values():
                if ins.id == CX.X86_INS_CALL or ins.group(capstone.CS_GRP_JUMP):
                    if ins.operands and ins.operands[0].type == CX.X86_OP_IMM:
                        t.add(ins.operands[0].imm & 0xFFFFFFFF)
            self.targets_cache[key] = t
        return t

    def site_arity(self, ret):
        """Stack dwords a call ending at `ret` passes: pushes (and 'sub esp,N' slots) walking back
        from the call to the previous call/branch/stack adjustment. None if not determinable.
        The walk stays inside the call's own basic block: the pushes of a loop's prologue (or of the
        funclet/parent function the site was inlined into) belong to a different path, and counting
        them fabricated 6 arguments for a 1-argument vtable call in a loop (write_soa 0x4e9640)."""
        got = self._insns_around(ret - 1)
        if not got:
            return None
        addrs, insns = got
        import bisect
        i = bisect.bisect_left(addrs, ret) - 1
        if i < 0:
            return None
        call = insns[addrs[i]]
        if call.id != CX.X86_INS_CALL or call.address + call.size != ret:
            return None
        starts = self.block_starts(addrs, insns)
        n = 0
        j = i - 1
        if addrs[i] not in starts:
            while j >= 0 and i - j < 40:
                ins = insns[addrs[j]]
                if ins.address + ins.size != addrs[j + 1]:
                    break                       # gap: not a straight-line predecessor
                if ins.id == CX.X86_INS_PUSH:
                    n += 1
                elif ins.id == CX.X86_INS_SUB and ins.operands[0].type == CX.X86_OP_REG and \
                        ins.operands[0].reg == CX.X86_REG_ESP and ins.operands[1].type == CX.X86_OP_IMM:
                    if ins.operands[1].imm > 0x40:
                        break
                    n += ins.operands[1].imm // 4
                elif ins.id in (CX.X86_INS_CALL, CX.X86_INS_RET, CX.X86_INS_POP, CX.X86_INS_ADD) or \
                        ins.group(capstone.CS_GRP_JUMP):
                    if ins.id == CX.X86_INS_ADD and not (ins.operands[0].type == CX.X86_OP_REG and
                                                         ins.operands[0].reg == CX.X86_REG_ESP):
                        j -= 1
                        continue
                    break
                else:
                    try:
                        _r, w = ins.regs_access()
                    except capstone.CsError:
                        w = []
                    if CX.X86_REG_ESP in w and ins.id not in (CX.X86_INS_MOV,):
                        break
                if addrs[j] in starts:
                    break                   # this instruction starts the block: earlier ones are another path
                j -= 1
        nxt = self.m.read(ret, 3)
        callee_pops = not (nxt[:2] == b"\x83\xc4" or nxt[:2] == b"\x81\xc4")
        return n, callee_pops

    def fake_call(self, target):
        m, uc = self.m, self.m.uc
        esp = uc.reg_read(UC_X86_REG_ESP)
        try:
            ret = m.u32(esp)
        except Exception:
            return None
        if self.img.in_text(target) or M.STUB_BASE <= target < m.stub_end:
            return None
        ar = self.site_arity(ret)
        if ar is None:
            return ("fake-arity", "no call site before %08x" % ret)
        n, pops = ar
        args = []
        for k in range(n):
            args.append(self.norm_key(m.u32(esp + 4 + 4 * k)))
        ecx = self.norm_key(uc.reg_read(UC_X86_REG_ECX))
        h = zlib.crc32(repr((target, ecx, args)).encode())
        k = h % 4
        eax = (0, 1, h % 9, M.POOL_BASE + M.SLOT * (HOT_SLOTS + (h >> 8) % (NSLOTS - HOT_SLOTS)))[k]
        if self._bool_result(ret):
            # the caller tests the return value as a flag (`test al,al`): a do-nothing method reports
            # "the check failed" (0), the answer that lets the caller's error path run instead of
            # sending it into work the method was supposed to do (cam_view 0x4eaa05: the model-record
            # check gates the clipboard copy, whose GlobalAlloc is not modelled)
            eax = 0
        self.fakes.append((target, n, ecx, tuple(args)))
        if self.opts.call_regs:
            self.fake_regs.append(call_reg_snapshot(uc))
        uc.reg_write(UC_X86_REG_EAX, eax)
        uc.reg_write(UC_X86_REG_EDX, 0)
        new_esp = esp + 4 + (4 * n if pops else 0)
        if self._returns_x87(ret):
            # the caller consumes st0 right after the call: return a float on the x87 stack
            m.write(M.SCRATCH + 8, struct.pack("<f", (0.0, 1.0, 0.5, float(h % 9))[(h >> 4) % 4]))
            m.write(new_esp - 4, struct.pack("<I", ret))
            uc.reg_write(UC_X86_REG_ESP, new_esp - 4)
            uc.reg_write(UC_X86_REG_EIP, M.FLD_RET_STUB)
            return True
        uc.reg_write(UC_X86_REG_ESP, new_esp)
        uc.reg_write(UC_X86_REG_EIP, ret)
        return True

    def _bool_result(self, ret):
        """True if the first instruction after the call tests the return value against itself
        (`test al,al` / `test eax,eax`): a method used as a boolean predicate."""
        try:
            ins = next(_md.disasm(self.m.read(ret, 16), ret, 1))
        except Exception:
            return False
        if ins.id != CX.X86_INS_TEST or len(ins.operands) != 2:
            return False
        return all(op.type == CX.X86_OP_REG for op in ins.operands) and \
            ins.operands[0].reg == ins.operands[1].reg

    def _returns_x87(self, ret):
        """True if the code after the call reads an x87 register it did not push itself, i.e. it
        uses a float returned in st0 (the x87 stack is empty at every call in MSVC code)."""
        try:
            code = self.m.read(ret, 96)
        except Exception:
            return False
        depth = 0
        n = 0
        for ins in _md.disasm(code, ret):
            n += 1
            if n > 24 or ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL) or \
                    ins.group(capstone.CS_GRP_RET):
                return False
            mn = ins.mnemonic
            if not mn.startswith("f") or mn in ("fnstcw", "fldcw", "fnstsw", "fstsw", "fwait", "fnclex", "fninit"):
                continue
            sti = [op.reg - CX.X86_REG_ST0 for op in ins.operands
                   if op.type == CX.X86_OP_REG and CX.X86_REG_ST0 <= op.reg <= CX.X86_REG_ST7]
            hi = max(sti) if sti else 0
            if mn.startswith(("fld", "fild")) and mn not in ("fldcw", "fldenv"):
                need, delta = (hi + 1 if sti else 0), 1
            elif mn in ("fcompp", "fucompp"):
                need, delta = 2, -2
            elif mn.endswith("p") or mn in ("fstp", "fistp", "fisttp", "fbstp"):
                need, delta = max(hi + 1, 1), -1
            else:
                need, delta = max(hi + 1, 1), 0
            if need > depth:
                return True
            depth += delta
            if depth < 0:
                return True
        return False

    # --- fault repair
    def _on_read(self, uc, access, address, size, value, _):
        if size == 4:
            try:
                self.reads.append((address, self.m.u32(address)))
            except Exception:
                pass

    def _traced_reads(self, inp):
        """[(address, dword)] of every 4-byte read the original makes from an input buffer, the
        argument area or image .data, in order (the last matching one is the most recent)."""
        m = self.m
        self.reads = []
        ranges = [(M.POOL_BASE, M.POOL_BASE + M.POOL_SIZE), (self.esp_entry, self.args_hi)]
        ranges += [(lo, hi) for _nm, lo, hi in m.writable]
        hs = [m.uc.hook_add(UC_HOOK_MEM_READ, self._on_read, begin=lo, end=hi - 1) for lo, hi in ranges]
        try:
            self.run_side(self.orig, inp)
        finally:
            for h in hs:
                m.uc.hook_del(h)
        reads, self.reads = self.reads, None
        return reads

    def _put_dword(self, out, addr, val):
        """Store val into the input location at addr: a pool object field, an incoming argument slot,
        or an image .data dword (an override both sides start from). None for an address the input
        does not own (read-only constants/tables are not writable)."""
        if M.POOL_BASE <= addr < M.POOL_BASE + M.POOL_SIZE:
            if addr & 3:
                return None
            out.pool_over[addr] = val
        elif self.esp_entry + 4 <= addr < self.args_hi:
            k = (addr - self.esp_entry - 4) // 4
            if addr & 3:
                return None
            if k < len(out.args):
                out.args[k] = val
            else:
                out.pad[k - len(out.args)] = val
        elif self.m.in_image_writable(addr):
            out.overrides[addr] = val
        else:
            return None
        return out

    def _redirect(self, reads, out, val, newp, depth=0):
        """Store newp into the input dword that carried `val`. When that dword is not writable as one
        (a pool address that is not 4-aligned, a read-only table), follow the chain one level: the
        pointer that led to it is an input dword (cam_view 0x4eaa05: [this+0x188] -> unaligned soup
        address -> 0xd -> call [0xd+0x44])."""
        for addr, v in reversed(reads):
            if v != val:
                continue
            if val == 0 and self.m.in_image_writable(addr):
                continue            # a zero global is an empty container/NULL constant, not the input's
            r = self._put_dword(out, addr, newp)
            if r is not None:
                return r
            if depth < 2 and addr != val:
                r = self._redirect(reads, out, addr, newp, depth + 1)
                if r is not None:
                    return r
        return None

    def _reg_def(self, eip, reg, window=12):
        """The last instruction before eip (same straight-line block) that writes `reg`: the instruction
        that built the faulting address."""
        got = self._insns_around(eip - 1)
        if not got:
            return None
        addrs, insns = got
        import bisect
        i = bisect.bisect_left(addrs, eip) - 1
        j = i
        while j >= 0 and i - j < window:
            ins = insns[addrs[j]]
            if j + 1 >= len(addrs) or ins.address + ins.size != addrs[j + 1]:
                break
            try:
                _r, w = ins.regs_access()
            except capstone.CsError:
                w = []
            if reg in w:
                return ins
            if ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL) or ins.id == CX.X86_INS_RET:
                break
            j -= 1
        return None

    def scaled_factor(self, eip, reg):
        """The factor N>1 by which `imul reg, reg, N` / `shl reg, N` scaled reg just before eip, or
        None: an array index that the faulting access multiplied by its element size (cam_view
        0x4ea8fd: `imul eax,eax,0x1f8` then `mov ebp,[eax+ecx+0x3c]`)."""
        ins = self._reg_def(eip, reg)
        if ins is None:
            return None
        ops = ins.operands
        if ins.id == CX.X86_INS_IMUL and len(ops) == 3 and ops[0].type == CX.X86_OP_REG and \
                ops[0].reg == reg and ops[1].type == CX.X86_OP_REG and ops[1].reg == reg and \
                ops[2].type == CX.X86_OP_IMM and ops[2].imm > 1:
            return ops[2].imm & 0xFFFFFFFF
        if ins.id == CX.X86_INS_SHL and len(ops) == 2 and ops[0].type == CX.X86_OP_REG and \
                ops[0].reg == reg and ops[1].type == CX.X86_OP_IMM and 0 < ops[1].imm < 32:
            return 1 << ops[1].imm
        return None

    def repair(self, inp, fault, rng, attempt=0):
        """The original faulted at an unmapped address A. Find the dword the bad pointer V came from
        (the last 4-byte read from the input buffers, image .data or the argument area with
        0 <= A - V < 64K; else an argument/register holding such a V) and make it point into a
        fresh input buffer. Returns the repaired input or None."""
        kind, A, _sz = fault
        if kind not in ("read", "write", "read-prot", "write-prot"):
            return None
        m = self.m
        fault_eip = m.uc.reg_read(UC_X86_REG_EIP)
        fault_regs = {cs: m.uc.reg_read(u) for cs, u in CS2UC.items()}
        reads = self._traced_reads(inp)

        # The faulting instruction's base register is the bad pointer V (exactly, if it was
        # loaded unchanged); otherwise fall back to "some value up to 64K below A".
        base_v = idx_v = None
        base_reg = None
        try:
            eip = fault_eip
            ins = next(_md.disasm(m.read(eip, 16), eip, 1))
            for op in ins.operands:
                if op.type == CX.X86_OP_MEM:
                    if op.mem.base and op.mem.base in CS2UC:
                        base_v = fault_regs.get(op.mem.base)
                        base_reg = op.mem.base
                    if op.mem.index and op.mem.index in CS2UC:
                        idx_v = fault_regs.get(op.mem.index)
                    break
        except Exception:
            base_v = idx_v = base_reg = None
        newp = M.POOL_BASE + M.SLOT * (HOT_SLOTS + rng.randrange(NSLOTS - HOT_SLOTS))
        out = inp.copy()

        def put(addr, val):
            return self._put_dword(out, addr, val)

        def mapped(v):
            return v is not None and (M.POOL_BASE <= v < M.POOL_BASE + M.POOL_SIZE or self.img.in_image(v) or
                                      M.DYN_BASE <= v < M.DYN_BASE + M.DYN_SIZE or
                                      M.STACK_TOP - M.STACK_SIZE <= v < STACK_END)
        # 0. a NULL(+offset) access after an allocation the input made fail, or after 8 repairs that did
        # not get past it: map the null page instead of redirecting yet another zero dword
        if kind in ("read", "write") and A < 0x10000 and (getattr(inp, "alloc_fail", ()) or attempt >= 8) \
                and 0 not in out.extra and self._mappable(0, M.SLOT) and not getattr(self.opts, "no_extra_map", False):
            out.extra[0] = M.SLOT
            return out
        # 1. the bad base pointer, loaded unchanged
        if base_v is not None and not mapped(base_v):
            r = self._redirect(reads, out, base_v, newp)
            if r is not None:
                return r
            for k, v in enumerate(out.args):
                if v == base_v:
                    out.args[k] = newp
                    return out
            for r, v in out.regs.items():
                if v == base_v:
                    out.regs[r] = newp
                    return out
        # 1c. the base register is an index the code scaled by the element size (`imul eax,eax,0x1f8`
        # before `mov [eax+ecx+0x3c]`): the dword it came from is a garbage array index, so resample it
        # small instead of writing a pointer into it and faulting the same way 24 times
        if base_v is not None and not mapped(base_v) and base_reg is not None and A >= 0x10000:
            mul = self.scaled_factor(fault_eip, base_reg)
            if mul:
                for addr, v in reversed(reads):
                    if (v * mul) & 0xFFFFFFFF == base_v:
                        if v == 0 and self.m.in_image_writable(addr):
                            continue    # an index of 0 is indistinguishable from a NULL global pointer
                        r = self._put_dword(out, addr, rng.randrange(4))
                        if r is not None:
                            return r
                for k, v in enumerate(out.args):
                    if (v * mul) & 0xFFFFFFFF == base_v:
                        out.args[k] = rng.randrange(4)
                        return out
        # 1b. a NULL that no input dword carried (an empty global container, a call that returned 0):
        # rewriting some zero .data dword would corrupt the program's own state (mesh_appear 0x4822ac:
        # the resmgr's empty registry vector), so map the null page and let the input soup decide
        if A < 0x10000 and 0 not in out.extra and \
                self._mappable(0, M.SLOT) and not getattr(self.opts, "no_extra_map", False):
            out.extra[0] = M.SLOT
            return out
        # 2. a good base with a garbage index: make the index small
        if idx_v is not None and (base_v is None or mapped(base_v)) and idx_v > 0x100:
            for addr, v in reversed(reads):
                if v == idx_v:
                    return put(addr, rng.randrange(4))
            for k, v in enumerate(out.args):
                if v == idx_v:
                    out.args[k] = rng.randrange(4)
                    return out
        # 3. a pointer loaded and then offset (lea/add) before the access: up to 64K below it
        ref = base_v if base_v is not None and not mapped(base_v) else A
        for addr, v in reversed(reads):
            if 0 <= (ref - v) & 0xFFFFFFFF < 0x10000:
                return put(addr, newp)
        for k, v in enumerate(out.args):
            if 0 <= (ref - v) & 0xFFFFFFFF < 0x10000:
                out.args[k] = newp
                return out
        for r, v in out.regs.items():
            if 0 <= (ref - v) & 0xFFFFFFFF < 0x10000:
                out.regs[r] = newp
                return out
        # 4. no traceable source (a computed or constant address): map input memory at the fault
        if kind in ("read", "write") and len(out.extra) < 4 and not getattr(self.opts, "no_extra_map", False):
            for base, size in ((A & ~(M.SLOT - 1), M.SLOT), (A & ~0xFFF, 0x1000)):
                if self._mappable(base, size):
                    out.extra[base] = size
                    return out
        return None

    def _call_before(self, ret):
        """The `call` instruction whose return address is ret, or None."""
        got = self._insns_around(ret - 1)
        if not got:
            return None
        addrs, insns = got
        import bisect
        i = bisect.bisect_left(addrs, ret) - 1
        if i < 0:
            return None
        ins = insns[addrs[i]]
        if ins.id != CX.X86_INS_CALL or ins.address + ins.size != ret:
            return None
        return ins

    def pure_call_site(self):
        """(vtable pointer, slot address, return address) of the indirect call the run stopped in
        _purecall from, or None. The pure-call stub is reached by one `call [reg(+disp)]` whose
        slot holds the stub: `reg` is the object's vtable pointer (or the object itself, for a
        function-pointer member), and the slot is the abstract method that was called."""
        m = self.m
        try:
            ret = m.u32(m.uc.reg_read(UC_X86_REG_ESP))
        except Exception:
            return None
        ins = self._call_before(ret)
        if ins is None or not ins.operands:
            return None
        op = ins.operands[0]
        if op.type != CX.X86_OP_MEM:
            return None
        regs = {cs: m.uc.reg_read(u) for cs, u in CS2UC.items()}
        base = regs.get(op.mem.base, 0) if op.mem.base else 0
        idx = regs.get(op.mem.index, 0) if op.mem.index else 0
        slot = (base + idx * op.mem.scale + op.mem.disp) & 0xFFFFFFFF
        return base, slot, ret

    def repair_pure(self, inp, rng):
        """The original stopped in _purecall: an indirect call read a vtable slot that is still the
        pure-call stub (an interface object the game would have initialised, or a slot the input
        never set). Resample, from the same input soup the rest of the arguments come from, the dword
        that carried the vtable pointer -- an argument slot, a pool object's vtable field or an image
        .data global -- and, when that dword is not the input's, the slot itself. Returns the
        resampled input or None; the caller keeps the first attempt that gets past the pure call."""
        site = self.pure_call_site()
        if site is None:
            return None
        vptr, slot, _ret = site
        reads = self._traced_reads(inp)
        out = inp.copy()
        newp = M.POOL_BASE + M.SLOT * (HOT_SLOTS + rng.randrange(NSLOTS - HOT_SLOTS))
        for addr, v in reversed(reads):         # the dword the vtable pointer was loaded from
            if v == vptr:
                r = self._put_dword(out, addr, newp)
                if r is not None:
                    return r
        for k, v in enumerate(out.args):        # ... or an argument/register that carried it
            if v == vptr:
                out.args[k] = newp
                return out
        for r, v in out.regs.items():
            if v == vptr:
                out.regs[r] = newp
                return out
        # the vtable pointer is a constant table (no input source): resample the slot that is still
        # the stub, when the input owns it (a pool object's vtable field, or an image .data table)
        return self._put_dword(out, slot, newp)

    def _mappable(self, base, size):
        """True if [base, base+size) is free address space outside the harness's own regions."""
        if base + size > M.TRAP_BASE or M.OBJ_BASE <= base + size and base < M.DYN_BASE + M.DYN_SIZE:
            return False
        for lo, hi, _p in self.m.uc.mem_regions():
            if base <= hi and lo < base + size:
                return False
        return True

    def note_sites(self, side, status, sites, last):
        """Call-site bookkeeping for 'blocked' edges: how often each depth-1 call site returned, and
        how often a run stopped at an unmodelled import (or a handler that cannot go on) inside it."""
        for a, n in sites.items():
            stopped = status[0] != "ok" and a == last
            side.site_hits[a] = side.site_hits.get(a, 0) + n
            side.site_returned[a] = side.site_returned.get(a, 0) + n - (1 if stopped else 0)
        if status[0] == "import" and last is not None and noreturn_stop(status[1]):
            nm = str(status[1]).split(":")[0]
            side.site_stops.setdefault(last, {})
            side.site_stops[last][nm] = side.site_stops[last].get(nm, 0) + 1

    def blocked_calls(self, side):
        """{call site: description} for sites that never returned and stopped at an unmodelled import, a
        C++ throw or a pure virtual call (never an input-dependent handler stop such as a huge malloc)."""
        out = {}
        for a, stops in side.site_stops.items():
            if side.site_returned.get(a, 0) == 0:
                out[a] = "never returned: stopped at %s in all %d runs that reached it" % (
                    "/".join("%s (%s)" % (n, NORETURN_WHY.get(n, "unmodelled import")) for n in sorted(stops)),
                    sum(stops.values()))
        return out

    # --- one run
    def run_side(self, side, inp):
        m, uc, t = self.m, self.m.uc, self.t
        # memory: pool, dyn heap, stack, misc
        cache = self.page_cache
        if cache.get("inp") is not inp:
            cache.clear()
            cache["inp"] = inp

        def lazy(page, inp=inp, cache=cache):
            d = cache.get(page)
            if d is None:
                cache.update(self.gen.slot_pages(inp, page & ~(M.SLOT - 1)))
                d = cache[page]
            return d
        m.take_dirty()              # DYN/MISC back to baseline (normally already clean)
        m.lazy = lazy
        m.pool_pages = {}
        m.pool_slots = []
        m.pool_entry = set()
        for a_, v_ in inp.overrides.items():
            m.write(a_, struct.pack("<I", v_))
        m.dyn_next = M.DYN_BASE
        m.dyn_sizes = {}
        m.rand_state = 1
        m.alloc_fail, m.alloc_count = tuple(getattr(inp, "alloc_fail", ())), 0
        extra_init = {}
        for base, size in sorted(getattr(inp, "extra", {}).items()):
            # memory mapped at a faulting address by fault repair: input soup, compared like pool pages
            m.uc.mem_map(base, size, UC_PROT_READ | UC_PROT_WRITE)
            data = self.gen._slot_soup(inp, base)[:size]
            m.uc.mem_write(base, data)
            for k in range(0, size, 0x1000):
                extra_init[base + k] = data[k:k + 0x1000]
        stk_lo = M.STACK_TOP - M.STACK_SIZE
        m.write(stk_lo, STACK_FILL * (STACK_END - stk_lo))
        frame = struct.pack("<I", M.SENTINEL) + struct.pack("<%dI" % (len(inp.args) + 4), *(inp.args + inp.pad))
        m.write(self.esp_entry, frame)
        regs = dict(CALLEE_SENTINEL)
        for r, v in inp.regs.items():
            regs[REGS[r]] = v
        m.reset_cpu(regs, inp.xmm)
        uc.reg_write(UC_X86_REG_ESP, self.esp_entry)
        entry = {r: uc.reg_read(REGS[r]) for r in RF.GPR_SCRATCH}
        for k in range(8):
            entry["xmm%d" % k] = uc.reg_read(UC_X86_REG_XMM0 + k)
        self.fakes, self.calls = [], []
        self.call_regs, self.fake_regs, self.import_regs = [], [], []
        m.trace_imports = ImportTrace(uc, self.import_regs) if self.opts.call_regs else []
        m.fake_handler = (getattr(self, "fake_override", None) or self.fake_call) if self.opts.fake_calls else None
        self.tr_stack, self.tr_block = [], None
        self.cur = side
        self.cov_on = True
        self.cov_side = side
        self.prev_end = None
        self.run_edges = set()
        self.last_site = None
        self.run_sites = {}
        self.run_blocks = set()
        budget = self.budget if side is self.orig else self.opts.budget * 4
        t0 = time.time()
        status = m.run(side.entry, None, budget, self.opts.run_seconds * (1 if side is self.orig else 4))
        self.cur = None
        self.cov_on = False
        if status[0] == "ok" and self.prev_end is not None:
            br = side.cfg.branch_end.get(self.prev_end)
            if br is not None and br[1] == "jcc":       # last block ended in a jcc taken out of the function
                src = br[0]
                self.run_edges.add((src, side.cfg.insns[src].operands[0].imm & 0xFFFFFFFF))
        o = {"status": status, "time": time.time() - t0, "fault": m.last_fault}
        o["edges"], o["blocks"] = self.run_edges, self.run_blocks
        o["sites"], o["last_site"] = self.run_sites, self.last_site
        self.note_sites(side, status, self.run_sites, self.last_site)
        o["eax"] = uc.reg_read(UC_X86_REG_EAX)
        o["edx"] = uc.reg_read(UC_X86_REG_EDX)
        o["ecx"] = uc.reg_read(UC_X86_REG_ECX)
        o["entry"] = entry
        for k in range(1, 8):
            o["xmm%d" % k] = uc.reg_read(UC_X86_REG_XMM0 + k)
        for nm, r in M.CALLEE_SAVED:
            o[nm] = uc.reg_read(r)
        o["esp"] = (uc.reg_read(UC_X86_REG_ESP) - self.esp_entry) & 0xFFFFFFFF
        fpsw = uc.reg_read(UC_X86_REG_FPSW)
        top = (fpsw >> 11) & 7
        o["fpu_depth"] = (8 - top) % 8
        o["st0"] = uc.reg_read(UC_X86_REG_FP0 + top) if o["fpu_depth"] else None
        # fpcw is only *observed* here; the control word is *set* in machine.py's session-init stub
        # (default 0x007F, --fpcw). The mask drops the reserved bits 6-7 and 13-15 only; precision (bits
        # 8-9) and rounding (bits 10-11) are compared.
        o["fpcw"] = uc.reg_read(UC_X86_REG_FPCW) & 0x1F3F
        o["mxcsr"] = uc.reg_read(UC_X86_REG_MXCSR) & 0xFFC0
        o["xmm0"] = uc.reg_read(UC_X86_REG_XMM0)
        # memory after the run (and undo it)
        mem = m.take_image_dirty()      # only pages written during the run (machine.py write hooks)
        if extra_init:
            o["extra_init"] = extra_init
            mem["extra"] = {}
            for base, size in sorted(inp.extra.items()):
                data = m.read(base, size)
                for k in range(0, size, 0x1000):
                    mem["extra"][base + k] = data[k:k + 0x1000]
                m.uc.mem_unmap(base, size)
        o["pool_init"] = init = dict(m.pool_pages)
        entry = m.pool_entry
        mem["pool"] = m.unmap_pool()
        if side is self.orig:
            # pages to mutate: where each object was first accessed, plus every page written
            inp.touched = sorted(entry | {p for p, d in mem["pool"].items() if d != init[p]})
        o["dyn_hi"] = m.dyn_next
        # every dirty DYN page (also beyond dyn_next, reached through garbage indices) and every
        # dirty MISC page (TEB, TLS, scratch), restored to the baseline for the next run
        mem["heap"], misc = m.take_dirty()
        sp = M.SCRATCH & ~0xFFF
        if sp in misc:                                  # harness scratch (float results) is not compared
            pg = bytearray(misc[sp])
            pg[M.SCRATCH - sp:M.SCRATCH - sp + 16] = m.misc_base_page(sp)[M.SCRATCH - sp:M.SCRATCH - sp + 16]
            misc[sp] = bytes(pg)
        mem["misc"] = misc
        mem["stack"] = m.read(self.args_hi, STACK_END - self.args_hi)
        for (lo, hi), base in zip(self.obj["data_rw"], self.obj["rw_baseline"]):
            cur = m.read(lo, hi - lo)
            mem["ours_data"] = cur
            if cur != base:
                m.write(lo, base)
        o["mem"] = mem
        o["imports"] = list(m.trace_imports)
        o["fakes"] = list(self.fakes)
        o["calls"] = list(self.calls)
        if self.opts.call_regs:
            # index-aligned with the three traces above: the register file at each recorded call (copied: a
            # later ref run reuses the lists)
            o["call_regs"] = {"call": list(self.call_regs), "import": list(self.import_regs),
                              "fake": list(self.fake_regs)}
        return o


# ====================================================================== comparison

def in_stack_frame(v, hi):
    """v points into the function's own stack: its frame (below esp at entry), the return address
    slot or its incoming argument slots (hi = Runner.stk_hi)."""
    return M.STACK_TOP - M.STACK_SIZE <= v < hi


def norm_val(v, hi):
    return "STK" if isinstance(v, int) and in_stack_frame(v, hi) else v


def ulp_close(a, b, ulp, bits=32):
    if bits == 32:
        fa, fb = struct.unpack("<f", struct.pack("<I", a))[0], struct.unpack("<f", struct.pack("<I", b))[0]
        ia, ib = a, b
    else:
        fa, fb = a, b
        ia, ib = a, b
    if math.isnan(fa) or math.isnan(fb):
        return math.isnan(fa) and math.isnan(fb)
    def key(i, bits):
        sign = 1 << (bits - 1)
        return (sign - (i & (sign - 1))) if i & sign else (i + sign)
    return abs(key(ia, bits) - key(ib, bits)) <= ulp


class Comparer:
    def __init__(self, runner, opts):
        self.r, self.opts = runner, opts
        self.tolerated = {"stack-pointer": 0, "inline-calls": 0, "ulp": 0, "const-pointer": 0, "local-copy": 0}
        self.ro = runner.ro
        self.last_kind = None

    def same_constant(self, x, y):
        """Two different pointers into read-only data whose 64 bytes (or C strings) are identical:
        the same literal/constant pooled at different addresses."""
        if x == y or not any(lo <= x < hi for lo, hi in self.ro) or not any(lo <= y < hi for lo, hi in self.ro):
            return False
        m = self.r.m
        try:
            bx, by = m.read(x, 64), m.read(y, 64)
        except Exception:
            return False
        if bx == by:
            return True
        for wide in (False, True):
            step = 2 if wide else 1
            z = b"\0" * step
            ex = next((i for i in range(0, 64, step) if bx[i:i + step] == z), None)
            ey = next((i for i in range(0, 64, step) if by[i:i + step] == z), None)
            if ex is not None and ex == ey and ex > 0 and bx[:ex] == by[:ey]:
                return True
        return False

    def our_section_end(self, y):
        """End of the section of OUR mapped object that contains y, else None."""
        obj, coff = self.r.obj, self.r.t.coff
        for i, a in obj["sec_addr"].items():
            n = len(coff.sections[i].data)
            if a <= y < a + n:
                return a + n
        return None

    def same_call_site(self, x, y):
        """x and y are the addresses stored right after a `call` in the original's and in our code: the
        same call site when both preceding calls target the same address and the instruction at the
        stored address is identical (the two builds may have drifted a few bytes apart)."""
        img, m = context()["img"], self.r.m
        try:
            ca, cb = img.read(x - 5, 5), m.read(y - 5, 5)
            if ca[0] != 0xE8 or cb[0] != 0xE8:
                return False
            ta = (x + struct.unpack_from("<i", ca, 1)[0]) & 0xFFFFFFFF
            tb = (y + struct.unpack_from("<i", cb, 1)[0]) & 0xFFFFFFFF
            return ta == tb and img.read(x, 8) == m.read(y, 8)
        except Exception:
            return False

    def same_local_copy(self, x, y):
        """x is the address the ORIGINAL stores, y the one our code stores: an address in our own mapped
        object, where the linker kept its own copy of something the original keeps in the image
        (a literal/constant pool entry, a vtable, a code label our code stores - a return address left in
        an output buffer) or a copy our code built in the pool/heap. Not such an address -> None.
        Otherwise (equal, note): equal only when the bytes at both are identical - for an address inside
        the function under test, the same offset in our copy, or the return address of the same call.
        A differing block is reported with its first differing dwords: a real difference, never a pass."""
        obj = self.r.obj
        img = context()["img"]
        if obj is None or not isinstance(y, int) or not isinstance(x, int) or not img.in_image(x):
            return None
        if not (obj["map_lo"] <= y < obj["map_hi"] or M.POOL_BASE <= y < M.POOL_BASE + M.POOL_SIZE or
                M.DYN_BASE <= y < M.DYN_BASE + M.DYN_SIZE):
            return None
        m = self.r.m
        lo, hi = obj["func_range"]
        flo, fhi = self.r.t.orig_lo, self.r.t.orig_hi
        if lo <= y < hi and flo <= x < fhi:
            try:
                if y - obj["entry"] == x - flo:
                    # the same position in our copy of the function (a label, or a return address an
                    # image callee copied out of the caller's frame): the address differs by the base only
                    return True, "our copy of the function, +%#x from its entry" % (y - obj["entry"])
                if self.same_call_site(x, y):
                    return True, "the return address of the same call (image +%#x, ours +%#x)" % (
                        x - flo, y - obj["entry"])
                return False, "code at the image's +%#x / our +%#x differs: %s/%s" % (
                    x - flo, y - obj["entry"], img.read(x, 8).hex(), m.read(y, 8).hex())
            except Exception:
                return None
        end = self.our_section_end(y)
        if end is not None:
            n = min(256, end - y)       # our copy's own extent in the object image
        elif M.POOL_BASE <= y < M.POOL_BASE + M.POOL_SIZE or M.DYN_BASE <= y < M.DYN_BASE + M.DYN_SIZE:
            n = 16                      # a copy our code built in the pool/heap: a fixed window, so no
        else:                           # difference inside the compared block can be skipped
            return None
        try:
            bx, by = img.read(x, n), m.read(y, n)
        except Exception:
            return None
        n = min(n, len(bx), len(by))        # a read can come back short (unmapped tail): never unpack past it
        if bx == by:
            return True, "%d byte(s) identical at the image's %08x" % (n, x)
        diffs = []
        for q in range(0, n - 3, 4):
            if bx[q:q + 4] != by[q:q + 4]:
                diffs.append("+%#x: %08x/%08x" % (q, struct.unpack_from("<I", bx, q)[0],
                                                   struct.unpack_from("<I", by, q)[0]))
                if len(diffs) >= 4:
                    break
        return False, "our copy at %08x vs the image's %08x differs (%s)" % (
            y, x, ", ".join(diffs) if diffs else "%d/%d bytes" % (len(bx), len(by)))

    def local_copy_equal(self, x, y):
        """same_local_copy, counting a tolerated difference (see the reason note)."""
        lc = self.same_local_copy(x, y)
        if lc and lc[0]:
            self.tolerated["local-copy"] += 1
            return True
        return False

    def compare(self, a, b, t):
        """-> None if equivalent, else (observable, orig, ours, detail). self.last_kind: triage hint."""
        self.last_kind = None
        rk = t.ret_kind
        if rk in ("int", "ptr", "hidden", "unknown", "small", "bool", "i64"):
            mask = {"bool": 0xFF, "small": 0xFFFF}.get(rk, 0xFFFFFFFF)
            if rk == "small":
                mask = 0xFFFF
            ea, eb = a["eax"] & mask, b["eax"] & mask
            if ea != eb and not (in_stack_frame(ea, self.r.stk_hi) and in_stack_frame(eb, self.r.stk_hi)) \
                    and not (mask == 0xFFFFFFFF and (self.same_constant(ea, eb) or self.local_copy_equal(ea, eb))):
                return ("eax" if mask == 0xFFFFFFFF else "eax&%#x" % mask, "%08x" % ea, "%08x" % eb, "return value")
            if rk == "i64" and a["edx"] != b["edx"]:
                return ("edx", "%08x" % a["edx"], "%08x" % b["edx"], "64-bit return high half")
        for nm, _r in M.CALLEE_SAVED:
            if a[nm] != b[nm]:
                return (nm, "%08x" % a[nm], "%08x" % b[nm], "callee-saved register not preserved identically")
        # Registers the original never writes (statically) and left untouched on this run: ours
        # must leave them too. An LTCG caller may keep a live value in a register its callee is
        # known not to touch (InterpRamp 0x114e780 wrote xmm3: crackling audio, 2026-10-12).
        written = getattr(t, "orig_regs_written", None)
        if written is not None and "entry" in a and "entry" in b:
            cand = [r for r in RF.GPR_SCRATCH + RF.XMM if r not in written]
            if rk != "void":
                cand = [r for r in cand if r != "eax" and not (rk == "i64" and r == "edx")]
            if rk == "m128":
                cand = [r for r in cand if r != "xmm0"]
            for r in cand:
                if a[r] == a["entry"][r] and b[r] != b["entry"][r]:
                    self.last_kind = "clobber"
                    return (r + " (untouched by the original)", "%x" % a["entry"][r], "%x" % b[r],
                            "ours writes a register the original never writes; an LTCG caller may keep a live value there")
        if a["esp"] != b["esp"]:
            return ("esp", "+%#x" % a["esp"], "+%#x" % b["esp"], "esp after return (relative to entry)")
        if a["fpu_depth"] != b["fpu_depth"]:
            return ("x87 depth", a["fpu_depth"], b["fpu_depth"], "values left on the x87 stack")
        if a["fpu_depth"] and a["st0"] != b["st0"]:
            ok = False
            if self.opts.ulp:
                fa, fb = x87_to_float(a["st0"]), x87_to_float(b["st0"])
                ia = struct.unpack("<Q", struct.pack("<d", fa))[0]
                ib = struct.unpack("<Q", struct.pack("<d", fb))[0]
                ok = ulp_close(ia, ib, self.opts.ulp, 64)
                if ok:
                    self.tolerated["ulp"] += 1
            if not ok:
                return ("st0", fmt_x87(a["st0"]), fmt_x87(b["st0"]), "x87 result")
        if a["fpcw"] != b["fpcw"]:
            return ("fpcw", "%04x" % a["fpcw"], "%04x" % b["fpcw"], "x87 control word")
        if a["mxcsr"] != b["mxcsr"]:
            return ("mxcsr", "%08x" % a["mxcsr"], "%08x" % b["mxcsr"], "SSE control")
        if rk == "m128" and a["xmm0"] != b["xmm0"]:
            return ("xmm0", "%032x" % a["xmm0"], "%032x" % b["xmm0"], "SSE return")
        d = self.compare_mem(a, b)
        if d:
            return d
        d = self.compare_imports(a["imports"], b["imports"])
        if d:
            return d
        d = self.compare_calls(a["calls"], b["calls"])
        if d:
            return d
        d = self.compare_fakes(a["fakes"], b["fakes"])
        if d:
            return d
        if getattr(self.opts, "call_regs_strict", False):
            # --call-regs-strict: a live-register difference is a mismatch, not just a diagnostic. Only an input
            # that matched every other observable reaches here, so nothing that already passed can be weakened.
            rd = self.call_reg_diff(a, b)
            if rd and rd.get("reg") and rd.get("before_args"):
                # only at a pair whose callee and normalised arguments still agree: a difference at or past
                # the point where the trace itself diverges is explained by that divergence (an inlined call)
                return (rd["observable"], rd["orig"], rd["ours"], rd["detail"])
        return None

    def compare_fakes(self, fa, fb):
        """Calls through generated pointers (fake methods): same `this` and the same stack
        arguments, in order. cViewer::SetRaster 0x7c4be0 passed (rect, a, b) for (a, b, rect) and
        RegisterGameTexture (a, b, 0, 0) for (key0, key2, a, b); both PASSed before this."""
        for k in range(min(len(fa), len(fb))):
            ta, na, ea, aa = fa[k]
            tb, nb, eb, ab = fb[k]
            if ta != tb:
                return ("fake call #%d" % k, "%08x" % ta, "%08x" % tb, "different generated callee")
            if not self.veq(ea, eb):
                return ("fake call #%d this" % k, fmt_val(ea), fmt_val(eb), "ecx at a call through a generated pointer")
            for i in range(min(na, nb)):
                if not self.veq(aa[i], ab[i]):
                    return ("fake call #%d arg %d" % (k, i), fmt_val(aa[i]), fmt_val(ab[i]),
                            "stack argument of a call through a generated pointer (normalised)")
        return None

    # --- --call-regs: the live register file at every call site

    def reg_value_equal(self, x, y):
        """One register dword explained: the same value, two addresses inside the function's own stack frame,
        the same pooled read-only constant, our own copy of a block whose bytes are identical, or --ulp apart.
        Uses the NON-counting forms of same_constant/local_copy_equal: a diagnostic must not move the tolerated
        tallies or the note a verdict prints."""
        if x == y:
            return True
        if in_stack_frame(x, self.r.stk_hi) and in_stack_frame(y, self.r.stk_hi):
            return True
        if self.same_constant(x, y):
            return True
        lc = self.same_local_copy(x, y)
        if lc is not None and lc[0]:
            return True
        return bool(self.opts.ulp) and ulp_close(x, y, self.opts.ulp)

    def reg_equal(self, name, x, y):
        """A whole register; an xmm register dword by dword (an address can sit in one lane)."""
        if name not in CALL_XMM:
            return self.reg_value_equal(x, y)
        return all(self.reg_value_equal((x >> (32 * i)) & 0xFFFFFFFF, (y >> (32 * i)) & 0xFFFFFFFF)
                   for i in range(4))

    def reg_pair_class(self, name, x, y, callee, reads=()):
        """Why two live-register values differ, for the --call-regs diagnostic:
        'frame-size'  the callee is __chkstk and its argument IS the caller's frame size - our frame legitimately
                      differs (the esp-at-return comparison already covers what matters);
        'fill'        a build left an uninitialised-stack pattern / sentinel there: a dead value, no information;
        'address'     a stack, heap or pool address against something else: a pointer that never reaches the
                      compared observable state (only registers and declared arguments are compared here);
        'pool'        two different pool/heap addresses: allocation order differs;
        'value'       anything else - the two implementations computed different things. The interesting case."""
        if name == "eax" and callee and "chkstk" in callee.lower():
            return "frame-size"
        if name in (reads or ()):
            return "input"           # the callee reads this register at entry: an argument we do not declare
        lo, hi = M.POOL_BASE, M.POOL_BASE + M.POOL_SIZE
        is_pool = lambda v: lo <= v < hi or M.DYN_BASE <= v < M.DYN_BASE + M.DYN_SIZE or 0x50000000 <= v < 0x50080000
        is_stack = lambda v: in_stack_frame(v, self.r.stk_hi) or 0x7D000000 <= v < 0x7E000000
        if x in FILL_PATTERNS or y in FILL_PATTERNS:
            return "dead-fill"
        if is_stack(x) or is_stack(y):
            return "dead-address"
        if is_pool(x) or is_pool(y):
            return "dead-address"
        return "dead"                # internal state the callee never sees: the final-state comparison covers it

    def peq(self, x, y):
        """veq without the tallies (same_constant/local_copy_equal report), for the same reason."""
        if x == y:
            return True
        if not (isinstance(x, int) and isinstance(y, int)):
            return False
        lc = self.same_local_copy(x, y)
        return self.same_constant(x, y) or (lc is not None and lc[0])

    def trace_diff_index(self, kind, a, b):
        """Index of the first pair of `kind` ('call', 'import', 'fake') whose CALLEE, this/edx or normalised
        stack arguments differ, or None: the point where the compared part of the trace already diverges, so a
        register difference at or after it explains nothing. Non-counting (peq)."""
        if kind == "call":
            A, B = self.norm_calls(a["calls"]), self.norm_calls(b["calls"])
            for k in range(min(len(A), len(B))):
                (ta, ea, da), aa = A[k]
                (tb, eb, db), ab = B[k]
                if ta != tb or not self.peq(ea, eb) or not self.peq(da, db) or \
                        not all(self.peq(x, y) for x, y in zip(aa, ab[:len(aa)])):
                    return k
            return min(len(A), len(B)) if len(A) != len(B) else None
        if kind == "import":
            esp = self.r.stk_hi
            ia, ib = a["imports"], b["imports"]
            for k in range(min(len(ia), len(ib))):
                na, va = ia[k][0], tuple(norm_val(v, esp) for v in ia[k][1])
                nb, vb = ib[k][0], tuple(norm_val(v, esp) for v in ib[k][1])
                if na != nb or len(va) != len(vb) or not all(self.peq(x, y) for x, y in zip(va, vb)):
                    return k
            return min(len(ia), len(ib)) if len(ia) != len(ib) else None
        fa, fb = a["fakes"], b["fakes"]
        for k in range(min(len(fa), len(fb))):
            ta, na, ea, aa = fa[k]
            tb, nb, eb, ab = fb[k]
            if ta != tb or not self.peq(ea, eb) or not all(self.peq(x, y) for x, y in zip(aa[:na], ab[:nb])):
                return k
        return min(len(fa), len(fb)) if len(fa) != len(fb) else None

    def trace_callee(self, kind, a, b, k):
        """'<address> <symbol>' of the callee of pair k (both sides when they disagree)."""
        try:
            if kind == "import":
                na, nb = str(a["imports"][k][0]), str(b["imports"][k][0])
                return na if na == nb else "%s vs %s" % (na, nb)
            ta = a["calls"][k][0] if kind == "call" else a["fakes"][k][0]
            tb = b["calls"][k][0] if kind == "call" else b["fakes"][k][0]
        except (IndexError, KeyError):
            return "?"
        img = context()["img"]
        def lab(t):
            nm = img.names.get(t, "")
            return "%08x%s" % (t, " " + nm if nm else "")
        return lab(ta) if ta == tb else "%s vs %s" % (lab(ta), lab(tb))

    def call_reg_diff(self, a, b):
        """--call-regs: the first live-register difference at a call site that no tolerance explains, with the
        call it belongs to. Pairs are aligned by index inside each trace (depth-1 calls, then imports, then fake
        calls - the order the trace comparisons use) and within a trace in call order. None when the flag is off
        or every captured register agrees."""
        if not getattr(self.opts, "call_regs", False):
            return None
        ra, rb = a.get("call_regs") or {}, b.get("call_regs") or {}
        counted, compared = [], 0
        for kind in ("call", "import", "fake"):
            n = min(len(ra.get(kind) or ()), len(rb.get(kind) or ()))
            compared += n
            if n:
                counted.append("%d %s%s" % (n, kind, "" if n == 1 else "s"))
        benign = None
        for kind in ("call", "import", "fake"):
            sa, sb = ra.get(kind) or (), rb.get(kind) or ()
            for k in range(min(len(sa), len(sb))):
                if not sa[k] or not sb[k]:
                    continue
                callee = self.trace_callee(kind, a, b, k)
                try:                    # registers the callee itself reads at entry (entry_reads, first 400 bytes)
                    reads = set() if kind == "import" else self.r.callee_regs(
                        a["calls"][k][0] if kind == "call" else a["fakes"][k][0])
                except (IndexError, KeyError, TypeError):
                    reads = set()
                for nm in CALL_REGS + CALL_XMM:
                    x, y = sa[k].get(nm), sb[k].get(nm)
                    if x is None or y is None or self.reg_equal(nm, x, y):
                        continue
                    lo_x, lo_y = (x & 0xFFFFFFFF, y & 0xFFFFFFFF) if nm in CALL_XMM else (x, y)
                    cls = self.reg_pair_class(nm, lo_x, lo_y, callee, reads)
                    j = self.trace_diff_index(kind, a, b)
                    where = "depth-1" if kind == "call" else kind
                    rec = {"kind": kind, "index": k, "reg": nm, "orig": fmt_reg(nm, x), "ours": fmt_reg(nm, y),
                           "callee": callee, "compared": compared, "traces": ", ".join(counted) or "none",
                           "first_arg_diff": j, "before_args": j is None or k < j, "class": cls,
                           "semantic": cls == "input", "observable": "call #%d reg %s" % (k, nm)}
                    if cls == "input":
                        rec["detail"] = (("live register at a %s call whose callee and normalised this/edx/stack "
                                          "arguments agree: the two implementations computed different values "
                                          "before this call" % where) if (j is None or k < j) else
                                         ("live register at a %s call; the compared callee/arguments already differ "
                                          "at #%d" % (where, j)))
                        return rec
                    if benign is None:      # keep the first explicable difference, in case nothing better shows up
                        benign = rec
        if benign is not None:
            why = {"frame-size": "the argument IS the caller's frame size (our frame legitimately differs)",
                   "dead-fill": "an uninitialised-stack fill / sentinel in a register the callee never reads",
                   "dead-address": "a stack/heap/pool address in a register the callee never reads",
                   "dead": "a register the callee never reads (internal state; the final-state comparison covers it)"}[benign["class"]]
            benign["detail"] = "no semantic divergence at any call; first explicable difference: %s" % why
            return benign
        return {"kind": None, "reg": None, "compared": compared, "traces": ", ".join(counted) or "none",
                "semantic": False}

    def call_reg_note(self, d):
        """The one-line --call-regs diagnostic for the printed reason."""
        if not d:
            return ""
        if not d.get("reg"):
            return "call-regs: no unexplained register difference (%d call site(s) compared: %s)" % (
                d.get("compared", 0), d.get("traces", "none"))
        kind = {"call": "depth-1 call", "import": "import call", "fake": "fake call"}.get(d["kind"], d["kind"])
        return "call-regs: call #%d reg %s: orig %s ours %s (%s to %s, %d call site(s) compared: %s%s)" % (
            d["index"], d["reg"], d["orig"], d["ours"], kind, d["callee"], d["compared"], d["traces"],
            "" if d["before_args"] else "; the compared callee/arguments already differ at #%s" % d["first_arg_diff"])

    def compare_mem(self, a, b):
        ma, mb = a["mem"], b["mem"]
        esp = self.r.stk_hi
        for region in sorted(set(ma) | set(mb)):
            if region == "ours_data":
                continue
            xa, xb = ma.get(region), mb.get(region)
            if region in ("pool", "extra"):
                ik = "pool_init" if region == "pool" else "extra_init"
                xa, xb = xa or {}, xb or {}
                for page in sorted(set(xa) | set(xb)):
                    pa = xa.get(page) or a.get(ik, {}).get(page) or b.get(ik, {}).get(page)
                    pb = xb.get(page) or b.get(ik, {}).get(page) or a.get(ik, {}).get(page)
                    d = self.diff_bytes(pa, pb, page, region, esp)
                    if d:
                        return d
                continue
            if region == "heap" and a["dyn_hi"] != b["dyn_hi"]:
                return ("heap size", "%#x" % (a["dyn_hi"] - M.DYN_BASE), "%#x" % (b["dyn_hi"] - M.DYN_BASE),
                        "bytes allocated through malloc-like imports")
            if isinstance(xa, dict) or isinstance(xb, dict):
                xa, xb = xa or {}, xb or {}
                m = self.r.m
                if region == "heap":
                    def basepg(page):
                        return b"\0" * 0x1000
                elif region == "misc":
                    basepg = m.misc_base_page
                else:
                    def basepg(page, region=region):
                        return m.image_base_page(region, page)
                for page in sorted(set(xa) | set(xb)):
                    pa = xa.get(page) or basepg(page)
                    pb = xb.get(page) or basepg(page)
                    d = self.diff_bytes(pa, pb, page, region, esp)
                    if d:
                        return d
                continue
            if xa == xb:
                continue
            base_addr = {"stack": self.r.args_hi}[region]
            d = self.diff_bytes(xa, xb, base_addr, region, esp)
            if d:
                return d
        od = mb.get("ours_data")
        if od is not None:
            for (lo, hi), base in zip(self.r.obj["data_rw"], self.r.obj["rw_baseline"]):
                if od != base:
                    i = next(k for k in range(len(od)) if od[k] != base[k])
                    return ("ours-private data", "-", "%08x" % (lo + i),
                            "OUR code wrote its own static data (a global the original keeps elsewhere)")
        return None

    def diff_bytes(self, xa, xb, base_addr, region, esp):
        if xa == xb:
            return None
        n = min(len(xa), len(xb))
        i = 0
        while i < n:
            # skip identical 256-byte chunks quickly
            if xa[i:i + 256] == xb[i:i + 256]:
                i += 256
                continue
            j = i
            while j < min(i + 256, n) and xa[j] == xb[j]:
                j += 1
            if j >= n:
                break
            if j >= i + 256:
                i += 256
                continue
            k = j & ~3
            va = struct.unpack_from("<I", xa, k)[0] if k + 4 <= n else None
            vb = struct.unpack_from("<I", xb, k)[0] if k + 4 <= n else None
            if va is not None and in_stack_frame(va, esp) and in_stack_frame(vb, esp):
                self.tolerated["stack-pointer"] += 1
                i = k + 4
                continue
            if va is not None and self.same_constant(va, vb):
                self.tolerated["const-pointer"] += 1
                i = k + 4
                continue
            if va is not None and self.local_copy_equal(va, vb):
                i = k + 4
                continue
            if va is not None and self.opts.ulp and ulp_close(va, vb, self.opts.ulp):
                self.tolerated["ulp"] += 1
                i = k + 4
                continue
            more, pairs = [], []
            for q in range(k, n - 3, 4):
                x, y = struct.unpack_from("<I", xa, q)[0], struct.unpack_from("<I", xb, q)[0]
                if x != y and not (in_stack_frame(x, esp) and in_stack_frame(y, esp)):
                    pairs.append((x, y))
                    if len(more) < 8:
                        more.append("%08x: %08x/%08x" % (base_addr + q, x, y))
                    if len(pairs) >= 64:
                        break
            self.last_kind = classify_pairs(pairs)
            block = ""
            # ours holds an address of its own copy where the original holds the image's (a constant, a
            # vtable, a code label, a copy our code built): same_local_copy already accepted every copy
            # whose bytes are identical, so say how the block differs - the FAIL then names bytes
            lc = self.same_local_copy(pairs[0][0], pairs[0][1]) if pairs else None
            if lc is not None and not lc[0]:
                self.last_kind = "local-address"
                block = "; " + lc[1]
            ulps = ""
            if va is not None and vb is not None:
                fa = struct.unpack("<f", struct.pack("<I", va))[0]
                fb = struct.unpack("<f", struct.pack("<I", vb))[0]
                if math.isfinite(fa) and math.isfinite(fb):
                    def key(i):
                        return (0x80000000 - (i & 0x7FFFFFFF)) if i & 0x80000000 else (i + 0x80000000)
                    dist = abs(key(va) - key(vb))
                    if dist < 1 << 16:
                        ulps = " [as float32: %r vs %r, %d ulp apart]" % (fa, fb, dist)
            return ("memory %s @%08x" % (region, base_addr + k),
                    "%08x" % va if va is not None else xa[k:k + 4].hex(),
                    "%08x" % vb if vb is not None else xb[k:k + 4].hex(),
                    "differing dwords (addr: orig/ours): " + ", ".join(more) + block + ulps)
        if len(xa) != len(xb):
            return ("memory %s" % region, "len %d" % len(xa), "len %d" % len(xb), "")
        return None

    def veq(self, x, y):
        """Normalised values equal, or two pointers to the same read-only constant."""
        if x == y:
            return True
        if isinstance(x, int) and isinstance(y, int):
            if self.same_constant(x, y):
                self.tolerated["const-pointer"] += 1
                return True
            if self.local_copy_equal(x, y):
                return True
        return False

    def seq_eq(self, xs, ys):
        return len(xs) == len(ys) and all(self.veq(x, y) for x, y in zip(xs, ys))

    def compare_imports(self, ia, ib):
        esp = self.r.stk_hi
        na = [(n, tuple(norm_val(v, esp) for v in args)) for n, args in ia]
        nb = [(n, tuple(norm_val(v, esp) for v in args)) for n, args in ib]
        if na == nb:
            return None
        for k in range(max(len(na), len(nb))):
            x = na[k] if k < len(na) else None
            y = nb[k] if k < len(nb) else None
            if x is None or y is None or x[0] != y[0] or not self.seq_eq(x[1], y[1]):
                return ("import call #%d" % k, fmt_call(x), fmt_call(y), "imports called (any depth), args normalised")
        return None

    def norm_calls(self, calls):
        out = []
        esp = self.r.stk_hi
        for tgt, ecx, edx, words, add_esp, is_jmp in calls:
            n = self.r.callee_arity(tgt)
            if n is None:
                n = self.r.decl_arity.get(tgt)
            if n is None:
                n = add_esp
            if tgt in self.r.decl_prefix:
                n = min(n, self.r.decl_prefix[tgt])
            regs = self.r.callee_regs(tgt)
            ent = [tgt, norm_val(ecx, esp) if "ecx" in regs else "-", norm_val(edx, esp) if "edx" in regs else "-"]
            out.append((ent, [norm_val(w, esp) for w in words[:n]] if n else []))
        return out

    def compare_calls(self, ca, cb):
        A, B = self.norm_calls(ca), self.norm_calls(cb)

        def same(x, y):
            if x[0][0] != y[0][0] or not self.seq_eq(x[0][1:], y[0][1:]):
                return False
            n = min(len(x[1]), len(y[1]))
            return self.seq_eq(x[1][:n], y[1][:n])

        def first_diff(A, B):
            for k in range(max(len(A), len(B))):
                x = A[k] if k < len(A) else None
                y = B[k] if k < len(B) else None
                if x is None or y is None or not same(x, y):
                    return k, x, y
            return None
        d = first_diff(A, B)
        if d is None:
            return None
        if not self.opts.strict_calls:
            ta, tb = {x[0][0] for x in A}, {x[0][0] for x in B}
            A2 = [x for x in A if x[0][0] in tb]
            B2 = [x for x in B if x[0][0] in ta]
            if first_diff(A2, B2) is None:
                self.tolerated["inline-calls"] += 1
                return None
            d = first_diff(A2, B2)
        k, x, y = d
        return ("call #%d (depth 1)" % k, fmt_tcall(x), fmt_tcall(y), "callee, this/edx and stack args (normalised)")


def classify_pairs(pairs):
    """Triage hint for differing dwords: float-rounding (all close finite floats), float-special
    (a NaN/Inf on one side), or value."""
    if not pairs:
        return None
    kinds = set()
    for x, y in pairs:
        fx, fy = struct.unpack("<f", struct.pack("<I", x))[0], struct.unpack("<f", struct.pack("<I", y))[0]
        if not math.isfinite(fx) or not math.isfinite(fy):
            kinds.add("float-special")
            continue
        kx = (0x80000000 - (x & 0x7FFFFFFF)) if x & 0x80000000 else (x + 0x80000000)
        ky = (0x80000000 - (y & 0x7FFFFFFF)) if y & 0x80000000 else (y + 0x80000000)
        kinds.add("float-rounding" if abs(kx - ky) <= 1024 else "value")
    if "value" in kinds:
        return "value"
    return "float-special" if "float-special" in kinds else "float-rounding"


# ====================================================================== --fp-tol

FPCW_REF = 0x037F        # 64-bit precision: approximates exact arithmetic for the reference run
FP_STORES = ("movss", "movaps", "movups", "movlps", "movhps", "movsd", "movapd", "movupd", "movlpd", "movhpd", "movq")
FP_BIG = 1e30            # a differing float above this (or Inf/NaN) in the original is "out of range"


def _f32(v):
    return struct.unpack("<f", struct.pack("<I", v))[0]


def _tiny(v):
    """Denormal bit pattern (exponent 0, mantissa != 0): also what small integers look like as floats."""
    return (v & 0x7F800000) == 0 and (v & 0x7FFFFFFF) != 0


def fp_ulp(x, bits=24):
    """Spacing of floats with `bits` significant bits at the magnitude of x."""
    if x == 0 or not math.isfinite(x):
        return 2.0 ** -149 if bits == 24 else 2.0 ** -1074
    return max(2.0 ** (math.frexp(x)[1] - bits), 2.0 ** -149 if bits == 24 else 2.0 ** -1074)


def fp_judge_value(fa, fb, fr, bits=24):
    """(verdict, ulps, ratio, detail) for one differing float: orig at the game's precision (fa), ours (fb)
    and the original at 64-bit precision (fr, the stand-in for the exact result).
    verdict: 'oor' (the original's result or the exact result is Inf/NaN/above 1e30: a range case),
    'real' (NaN/Inf in ours only, or ours is further from the reference than the original), 'ok'."""
    if not math.isfinite(fa) or abs(fa) > FP_BIG or not math.isfinite(fr) or abs(fr) > FP_BIG:
        return "oor", 0.0, None, ""
    if not math.isfinite(fb):
        return "inf", 0.0, None, "ours is %r, original %r (reference %r)" % (fb, fa, fr)
    eo, eu = abs(fa - fr), abs(fb - fr)
    ulp = fp_ulp(max(abs(fr), abs(fa), abs(fb)), bits)
    ratio = eu / eo if eo > 0 else None
    detail = "orig %r ours %r reference %r: err_orig %.3g err_ours %.3g (%.3g ulp)" % (fa, fb, fr, eo, eu, eu / ulp)
    ok = eu <= max(2 * eo, 4 * ulp)
    return ("ok" if ok else "real"), abs(fa - fb) / ulp, ratio, detail


class FpTol:
    """--fp-tol: judge an input where original and ours differ only in floating-point rounding. The original
    is re-run at 64-bit precision (the reference); a differing dword is a float only if both builds and the
    reference hold finite floats, and ours must be at most as far from the reference as twice the original
    (or 4 ulp). Everything else (integers, pointers, call traces, status) stays a real mismatch."""

    def __init__(self, runner, cmp_, opts):
        self.r, self.c, self.opts = runner, cmp_, opts
        self.cw = int(opts.fpcw, 16)
        self.n_fp = self.n_oor = self.n_floats = 0
        self.max_ulp = 0.0
        self.max_ratio = 0.0

    def stats(self, valid, mism):
        return {"compared": valid, "bit_identical": valid - self.n_fp - self.n_oor - mism, "fp_tolerable": self.n_fp,
                "fp_floats": self.n_floats, "max_ulp": round(self.max_ulp, 2), "max_err_ratio": round(self.max_ratio, 2),
                "out_of_range": self.n_oor, "real_mismatches": mism}

    def ref_run(self, inp):
        m = self.r.m
        touched = list(inp.touched)
        m.set_fpcw(FPCW_REF)
        self.fstore = {}        # dword address -> the last write to it was an x87/SSE float store
        code, fvals = {}, set()

        def on_write(uc, access, address, size, value, _):
            pc = uc.reg_read(UC_X86_REG_EIP)
            f = code.get(pc)
            if f is None:
                try:
                    ins = next(_md.disasm(bytes(uc.mem_read(pc, 15)), pc))
                    f = ins.mnemonic.startswith("fst") or ins.mnemonic in FP_STORES
                except (StopIteration, Exception):      # noqa: BLE001
                    f = False
                code[pc] = f
            if size >= 4:
                # an integer store of a value an x87/SSE store wrote earlier is a copied float (spill, then mov to the result)
                v = value & 0xFFFFFFFF
                if f:
                    fvals.add(v)
                self.fstore[address & ~3] = f or v in fvals
        self.huge_in = False        # the run read a dword that is a float Inf/NaN/above 1e30
        self.denorm_in = False      # ... or a denormal float (also what small integers look like)

        def on_read(uc, access, address, size, value, _):
            if size == 4 and not (self.huge_in and self.denorm_in):
                try:
                    v = struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]
                except Exception:      # noqa: BLE001
                    return
                e = (v >> 23) & 0xFF
                if e >= 127 + 100:        # |x| >= 2^100 (1.27e30), Inf or NaN
                    self.huge_in = True
                elif e == 0 and v & 0x7FFFFF:
                    self.denorm_in = True
        h = m.uc.hook_add(UC_HOOK_MEM_WRITE, on_write)
        h2 = m.uc.hook_add(UC_HOOK_MEM_READ, on_read)
        try:
            return self.r.run_side(self.r.orig, inp)
        finally:
            m.uc.hook_del(h)
            m.uc.hook_del(h2)
            m.set_fpcw(self.cw)
            inp.touched = touched

    def page(self, res, others, region, page, ik):
        """The page of `region` as `res` left it (falling back to what it started with in `res` and the other
        runs, then to the baseline); None for a pool page no run touched."""
        m = self.r.m
        x = res["mem"].get(region) or {}
        p = x.get(page)
        for o in [res] + list(others):
            if p is None and ik and o is not None:
                p = o.get(ik, {}).get(page)
        if p is not None:
            return p
        if ik:
            return None
        if region == "heap":
            return b"\0" * 0x1000
        if region == "misc":
            return m.misc_base_page(page)
        return m.image_base_page(region, page)

    def dword_diffs(self, a, b, ref):
        """[(region, page|None, base_addr, dword index, va, vb, vr)] for every differing dword of a and b
        that is not already tolerated as a stack pointer / pooled constant."""
        out = []
        esp = self.r.stk_hi
        for region in sorted(set(a["mem"]) | set(b["mem"])):
            if region == "ours_data":
                continue
            xa, xb = a["mem"].get(region), b["mem"].get(region)
            if isinstance(xa, dict) or isinstance(xb, dict) or region in ("pool", "extra"):
                ik = "pool_init" if region == "pool" else "extra_init" if region == "extra" else None
                pages = sorted(set(xa or {}) | set(xb or {}))
                items = [(pg, self.page(a, (b, ref), region, pg, ik), self.page(b, (a, ref), region, pg, ik),
                          self.page(ref, (a, b), region, pg, ik) if ref else None, pg) for pg in pages]
            else:
                if xa == xb or xa is None or xb is None:
                    continue
                items = [(None, xa, xb, None, self.r.args_hi)]
            for pg, pa, pb, pr, base in items:
                if pa is None or pb is None:
                    continue
                n = min(len(pa), len(pb)) // 4
                wa = np.frombuffer(pa[:4 * n], "<u4")
                wb = np.frombuffer(pb[:4 * n], "<u4")
                wr = np.frombuffer(pr[:4 * n], "<u4") if pr is not None and len(pr) >= 4 * n else None
                for k in np.nonzero(wa != wb)[0]:
                    va, vb = int(wa[k]), int(wb[k])
                    if in_stack_frame(va, esp) and in_stack_frame(vb, esp):
                        continue
                    if self.c.same_constant(va, vb):
                        continue
                    out.append((region, pg, base, int(k), va, vb, int(wr[k]) if wr is not None else va))
        return out

    def judge(self, a, b, inp, t):
        """-> ('fp' | 'oor', None) when only float rounding differs, else ('real', mismatch tuple)."""
        ref = self.ref_run(inp)
        if ref["status"][0] != "ok":
            return "real", ("fp-tol", "-", "-", "reference run (original at 64-bit precision) did not complete: %s" % (ref["status"],))
        floats = []          # (label, fa, fb, fr, bits, edit): edit = (region, page, dword index, orig dword) | "st0" | None
        for_st0 = a["fpu_depth"] and a["fpu_depth"] == b["fpu_depth"] and a["st0"] != b["st0"]
        if for_st0:
            fa, fb = x87_to_float(a["st0"]), x87_to_float(b["st0"])
            fr = x87_to_float(ref["st0"]) if ref["fpu_depth"] == a["fpu_depth"] and ref["st0"] is not None else fa
            pc = (a["fpcw"] >> 8) & 3
            floats.append(("st0", fa, fb, fr, 24 if pc == 0 else 53, "st0"))
        for region, pg, base, k, va, vb, vr in self.dword_diffs(a, b, ref):
            addr = base + 4 * k
            lbl = "memory %s @%08x" % (region, addr)
            if _tiny(va) or _tiny(vr):
                if not self.fstore.get(addr):
                    continue            # small value not stored by an x87/SSE store: an integer, left to compare()
                floats.append((lbl, float("inf"), 0.0, 0.0, 24, (region, pg, k, va)))     # denormal result: range case
                continue
            floats.append((lbl, _f32(va), _f32(vb), _f32(vr), 24, (region, pg, k, va)))
        results = []
        for lbl, fa, fb, fr, bits, edit in floats:
            v, ulps, ratio, detail = fp_judge_value(fa, fb, fr, bits)
            if v == "inf":
                # Inf/NaN only in ours: out of range if the run read a float Inf/NaN/>1e30 input (an intermediate
                # overflowed float range in one build and stayed finite in the other's 80-bit register), else real
                v = "oor" if self.huge_in else "real"
            if v == "real" and self.denorm_in and max(abs(fa), abs(fb)) < 1e-30:
                v = "oor"       # near-underflow results of denormal inputs: the inputs feeding it are range values
            results.append((lbl, v, ulps, ratio, detail, edit))
        # make the judged float differences invisible to the exact comparison; whatever it still finds is not a float
        sane = dict(b)
        sane["mem"] = {k: (dict(v) if isinstance(v, dict) else v) for k, v in b["mem"].items()}
        edits = {}
        for lbl, v, _u, _r, _d, edit in results:
            if v == "real":
                continue
            if edit == "st0":
                sane["st0"] = a["st0"]
            else:
                edits.setdefault(edit[:2], []).append(edit[2:])
        for (region, pg), lst in edits.items():
            if region not in sane["mem"]:
                # the baseline never snapshotted this region (e.g. our object's .data when the original's run wrote
                # nothing there), so compare() does not look at it: nothing to hide (KeyError: '.data', hud 0x4d5410)
                continue
            if pg is None:
                buf = bytearray(sane["mem"][region])
            else:
                buf = bytearray(self.page(b, (a,), region, pg, "pool_init" if region == "pool" else "extra_init" if region == "extra" else None))
            for k, va in lst:
                if 4 * k + 4 <= len(buf):    # the page compare() will look at can be shorter than the one the
                    struct.pack_into("<I", buf, 4 * k, va)   # dword index was computed from: skip the edit, never crash
                # (an unapplied edit only means compare() still sees a float difference: the safe direction)
            if pg is None:
                sane["mem"][region] = bytes(buf)
            else:
                sane["mem"][region][pg] = bytes(buf)
        d = self.c.compare(a, sane, t)
        bad = next(((l_, dt) for l_, v, _u, _r, dt, _e in results if v == "real"), None)
        if d and bad and d[0].startswith("memory"):
            return "real", (bad[0] + " (fp-tol)", "-", "-", bad[1])
        if d:
            return "real", d
        if any(v == "oor" for _l, v, *_r in results):
            # a range case (Inf/NaN/denormal/>1e30 in the original's own result): not judged either way
            self.n_oor += 1
            return "oor", None
        if a["edges"] != ref["edges"]:
            return "real", ("control flow", "-", "-", "the original takes a different path at 64-bit precision (precision-sensitive branch); not judged as rounding")
        if bad:
            return "real", (bad[0] + " (fp-tol)", "-", "-", bad[1])
        for_stats = [(u, r) for _l, _v, u, r, _d, _e in results]
        self.n_fp += 1
        self.n_floats += len(results)
        for ulps, ratio in for_stats:
            self.max_ulp = max(self.max_ulp, ulps)
            if ratio is not None:
                self.max_ratio = max(self.max_ratio, ratio)
        return "fp", None


def x87_to_float(v):
    if v is None:
        return None
    mant, exp = v
    sign = -1.0 if exp & 0x8000 else 1.0
    e = exp & 0x7FFF
    if e == 0x7FFF:
        return float("nan") if mant & 0x7FFFFFFFFFFFFFFF else sign * float("inf")
    if e == 0 and mant == 0:
        return sign * 0.0
    try:
        return sign * math.ldexp(mant, e - 16383 - 63)
    except OverflowError:
        return sign * float("inf")


def fmt_x87(v):
    return "%s (%04x:%016x)" % (repr(x87_to_float(v)), v[1], v[0]) if v else "-"


def fmt_val(v):
    if isinstance(v, int):
        return "%08x" % v
    if isinstance(v, tuple) and v and v[0] == "RO":
        return "RO:" + repr(v[1][:24])
    return str(v)


def fmt_reg(name, v):
    """A captured register value: 8 hex digits for a GPR, 32 for an xmm register."""
    return "%032x" % v if name in CALL_XMM else "%08x" % v


def fmt_call(x):
    if x is None:
        return "(none)"
    n, args = x
    return "%s(%s)" % (n, ", ".join(a if isinstance(a, str) else "%#x" % a for a in args))


def fmt_tcall(x):
    if x is None:
        return "(none)"
    (tgt, ecx, edx), args = x
    nm = context()["img"].names.get(tgt, "")
    return "%08x%s ecx=%s args=[%s]" % (tgt, " " + nm if nm else "", ecx if isinstance(ecx, str) else "%08x" % ecx,
                                         ", ".join(a if isinstance(a, str) else "%x" % a for a in args))


# ====================================================================== driver

MAX_PROCS = int(os.environ.get("DIFFTEST_MAX_PROCS", os.cpu_count() or 2))


def cpu_slot():
    """Hold one of MAX_PROCS machine-wide slots (flock'd files) while emulating, so concurrent
    checkers (batch shards, agents) use at most one core each (DIFFTEST_MAX_PROCS, default all). Blocks until a slot is free."""
    import fcntl
    d = os.path.join("/tmp", "difftest-slots-%d" % os.getuid())
    os.makedirs(d, exist_ok=True)
    while True:
        for i in range(MAX_PROCS):
            fd = os.open(os.path.join(d, "slot%d" % i), os.O_CREAT | os.O_RDWR)
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                return fd
            except OSError:
                os.close(fd)
        time.sleep(0.5)


CACHE_DIR = os.path.join(S.ROOT, "work", "difftest", "equiv_cache")
_UNKEYED_OPTS = ("json", "verbose", "vas", "slice", "replay")
_harness_hash = None


def _cache_key(t, out, opts):
    """Key of everything the emulation depends on: the harness sources, the compiled object (non-debug sections, their
    relocations by symbol name, the symbol table; .debug$S carries the object's own path), the resolution of our
    symbols into the original image, the parsed signature/layout and the options. Same key -> same verdict, so a row
    whose code a header refactor left unchanged is not re-emulated. DIFFTEST_NO_CACHE=1 disables it."""
    global _harness_hash
    if os.environ.get("DIFFTEST_NO_CACHE"):
        return None
    import hashlib
    if _harness_hash is None:
        h = hashlib.sha1()
        for f in sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "*.py"))):
            h.update(open(f, "rb").read())
        _harness_hash = h.hexdigest()
    c = t.coff
    h = hashlib.sha1(_harness_hash.encode())
    for sec in c.sections:
        if sec.discard:
            continue
        h.update(repr((sec.name, sec.flags, sec.size)).encode())
        h.update(sec.data)
        h.update(repr([(r[0], c.syms[r[1]].name if r[1] in c.syms else r[1], r[2]) for r in sec.relocs]).encode())
    h.update(repr([(sy.name, sy.value, sy.secno, sy.cls) for _, sy in sorted(c.syms.items())]).encode())
    h.update(repr(sorted((c.syms[i].name if i in c.syms else str(i), a) for i, a in t.res.addr.items())).encode())
    keep = {k: v for k, v in out.items() if k not in ("seconds",)}
    h.update(json.dumps(keep, sort_keys=True, default=str).encode())
    h.update(json.dumps({k: v for k, v in sorted(vars(opts).items()) if k not in _UNKEYED_OPTS},
                        sort_keys=True, default=str).encode())
    return h.hexdigest()


def test_function(sid, va, opts, log=print):
    fd = cpu_slot()
    try:
        r = _test_function(sid, va, opts, log)
    finally:
        os.close(fd)
    key = r.pop("_cache_key", None)
    if key and not r.get("cached") and r.get("verdict") in ("PASS", "WEAK", "FAIL", "PASS-FP", "WEAK-FP"):
        try:
            os.makedirs(CACHE_DIR, exist_ok=True)
            tmp = os.path.join(CACHE_DIR, "%s.%d.part" % (key, os.getpid()))
            json.dump(r, open(tmp, "w"))
            os.replace(tmp, os.path.join(CACHE_DIR, key + ".json"))
        except OSError:
            pass
    _ledger(r)
    return r


def _ledger(r):
    """Append one line per function result to work/opencode/attempts.jsonl (never in git).
    Model attribution comes from $OPENCODE_LEDGER_MODEL (set by the wave runners); empty if unset.
    DIFFTEST_NO_LEDGER=1 (harness re-grades, self-tests) writes nothing."""
    if os.environ.get("DIFFTEST_NO_LEDGER"):
        return
    try:
        rec = {"t": int(time.time()), "slice": r.get("slice"), "va": r.get("va"),
               "verdict": r.get("verdict"), "seconds": r.get("seconds"),
               "coverage": (r.get("coverage") or {}).get("pct"), "all": (r.get("coverage") or {}).get("all"),
               "edges": {k: v.get("pct") for k, v in (r.get("edges") or {}).items()},
               "valid": r.get("valid"), "inputs": r.get("inputs"), "orig_size": r.get("orig_size"),
               "wave": os.environ.get("OPENCODE_LEDGER_WAVE", ""),
               "model": os.environ.get("OPENCODE_LEDGER_MODEL", "")}
        p = os.path.join(S.ROOT, "work", "opencode", "attempts.jsonl")
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "a") as f:
            f.write(json.dumps(rec) + "\n")
    except Exception:
        pass          # releases the flock


def _test_function(sid, va, opts, log=print):
    t0 = time.time()
    t = prepare(sid, va, opts.flags, log, getattr(opts, "src", None))
    models = modelled_symbols(t)
    out = {"slice": sid, "va": "%08x" % va, "flags": " ".join(t.flags), "flags_from": t.flags_how,
           "symbol": getattr(t, "sym", None), "symbol_from": getattr(t, "sym_how", None),
           "unresolved": t.unresolved, "eh_unresolved": t.eh_unresolved,
           "modelled": models,
           "resolution_conflicts": [{"symbol": c[0], "resolved": "%08x" % c[1], "method": c[2], "aligned": "%08x" % c[3]}
                                    for c in t.conflicts],
           "resolution_methods": t.methods}
    if t.verdict == "UNSUPPORTED":
        out.update(verdict="UNSUPPORTED", reason=t.reason + model_note(models), seconds=round(time.time() - t0, 1))
        return out
    # Resolution audit (resaudit.py): every mapped reference vs the original's operand at the paired instruction.
    # A disagreement means our build would call/read something other than what the original does there, so any
    # PASS or FAIL would be meaningless (three wave-1 FAILs were wrong mappings): report it instead of running.
    aud = RA.audit(t.coff, t.fsym, t.res, t.orig_insns, context()["img"])
    out["resolution_audit"] = {"disagree": aud["disagree"], "unverified": aud["unverified"],
                               "local_copy": aud["local_copy"], "same_code": aud["same_code"],
                               "agree": len(aud["agree"]), "paired_insns": aud["pairs"], "insns": aud["insns"]}
    audit_warning = None
    if aud["disagree"]:
        # The audit pairs operands positionally, so a shifted-but-equivalent instruction stream "disagrees" without
        # saying anything about behaviour. Keep it as evidence and let the emulator decide; --strict-audit restores
        # the veto for a caller that wants the static gate.
        audit_warning = RA.summary(aud)
        out["audit_warning"] = audit_warning
        out["audit_disagree"] = aud["disagree"]
        if getattr(opts, "strict_audit", False):
            out.update(verdict="UNSUPPORTED", reason=audit_warning, seconds=round(time.time() - t0, 1))
            return out
    out.update(signature=str(t.sig) if t.sig else "unparsed (%s)" % getattr(t, "sig_error", "?"),
               stack_args=len(t.stack_kinds), reg_args=t.reg_kinds, callee_pop=t.callee_pop,
               ret_kind=t.ret_kind, layout_notes=t.layout_notes, orig_size=t.orig_hi - t.orig_lo,
               void_hides_eax=t.void_hides_eax, orig_untouched_regs=sorted(r for r in RF.GPR_SCRATCH + RF.XMM
                                                                          if r not in t.orig_regs_written))
    if t.abi_mismatch and not opts.ignore_abi:
        out.update(verdict="UNSUPPORTED", reason=t.abi_mismatch, seconds=round(time.time() - t0, 1))
        return out
    key = _cache_key(t, out, opts)
    if key:
        try:
            hit = json.load(open(os.path.join(CACHE_DIR, key + ".json")))
            hit.update(cached=True, seconds=round(time.time() - t0, 1), cached_seconds=hit.get("seconds"))
            log("  equiv cache hit (same object, resolution and options as an earlier run)")
            return hit
        except (OSError, ValueError):
            out["_cache_key"] = key
    runner = Runner(t, opts, log)
    cmp_ = Comparer(runner, opts)
    fpt = FpTol(runner, cmp_, opts) if getattr(opts, "fp_tol", False) else None
    rng_master = random.Random("%s:%08x:%d" % (sid, va, opts.seed))
    gen = runner.gen
    corpus = []
    valid, discarded, mism, first = 0, {}, 0, None
    sigs = collections.Counter()
    traps = {}
    repaired = 0
    faked = 0
    ran = 0
    ours_probe = 0
    probes = 0
    deadline = time.time() + opts.time_limit
    steer = ST.Steer(runner, gen, opts) if getattr(opts, "steer", True) else None
    hard = opts.inputs + (opts.extra_inputs if steer else 0)
    i = -1
    try:
        while True:
            i += 1
            if i >= hard:
                break
            if i >= opts.inputs and not (steer and corpus and edges_missing(runner) and steer.has_work()):
                break               # past --inputs only while steering still has uncovered branches to try
            if time.time() > deadline:
                out["stopped_early"] = "time limit %ds after %d inputs" % (opts.time_limit, i)
                break
            ran += 1
            rng = random.Random(rng_master.getrandbits(64))
            inp = None
            if steer and corpus and (i >= opts.inputs or rng.random() < opts.steer_share):
                inp = steer.next(rng, corpus)
            if inp is None:
                if corpus and rng.random() < opts.mutate:
                    parent = steer.pick_parent(corpus, rng) if steer else \
                        corpus[-1 - min(int(rng.expovariate(0.15)), len(corpus) - 1)]
                    inp = gen.mutate(parent, rng, corpus)
                else:
                    inp = gen.fresh(rng)
            try:
                a = runner.run_side(runner.orig, inp)
                fixes = pure_fixes = 0
                while time.time() < deadline:
                    st_ = a["status"]
                    if st_[0] == "fault" and a["fault"] and a["fault"][0] != "pool-limit" and fixes < opts.repairs:
                        fixes += 1
                        rep = runner.repair(inp, a["fault"], rng, fixes - 1)
                    elif st_[0] == "import" and str(st_[1]).split(":")[0] == "_purecall" and \
                            pure_fixes < opts.repairs:
                        # a random input drove a vtable call through a slot that is still the pure-call
                        # stub: resample the class pointer/vtable the input carried and keep the first
                        # attempt that gets past it instead of burning the input
                        pure_fixes += 1
                        rep = runner.repair_pure(inp, rng)
                    else:
                        break
                    if rep is None:
                        break
                    inp = rep
                    a = runner.run_side(runner.orig, inp)
                if a["status"][0] == "budget" and runner.budget < opts.budget and probes < BUDGET_PROBES:
                    # Runaway input, or a function whose real runs are long? Retry with the full cap.
                    probes += 1
                    low, runner.budget = runner.budget, opts.budget
                    a2 = runner.run_side(runner.orig, inp)
                    if a2["status"][0] == "budget":
                        runner.budget = low
                    else:
                        a = a2
                        out["budget_raised"] = "input %d needed more than %d instructions" % (i, low)
                st = a["status"][0]
                if st == "import" and ours_probe < OURS_IMPORT_PROBES:
                    # our side's call sites that stop at the same import (for its 'blocked' edges): run it, never compare
                    ours_probe += 1
                    runner.run_side(runner.ours, inp)
            except UcError as e:
                # one input whose run trips the emulator (a hook reading unmapped memory: fake_call's argument
                # peek, save_state 0x4e9620) must not kill the row - discard it and carry on. Setup and compile
                # errors stay fatal above.
                discarded["fault(unmapped)"] = discarded.get("fault(unmapped)", 0) + 1
                if opts.verbose:
                    log("  discard input %d: UcError: %s" % (i, str(e)[:120]))
                continue
            if st != "ok":
                key = "wall-timeout" if st == "wall" else (st if st != "import" else "import:%s" % str(a["status"][1]).split(":")[0])
                if st == "fault" and a["fault"]:
                    key = "fault(%s)" % a["fault"][0]
                if key in ("timeout", "wall", "wall-timeout"):
                    out["wall_timeout"] = out.get("wall_timeout", 0) + 1
                discarded[key] = discarded.get(key, 0) + 1
                continue
            if not runner.abi_ok(a):
                # the original itself returned with clobbered callee-saved registers or a wrong esp:
                # a fake method popped the wrong number of bytes, or the input sent it through
                # non-code (e.g. a vtable pointer into .text). Says nothing about equivalence.
                key = "orig-abi-broken(fake)" if a["fakes"] else "orig-abi-broken"
                discarded[key] = discarded.get(key, 0) + 1
                continue
            if fixes or pure_fixes:
                repaired += 1
            run_blocks = runner.run_blocks
            try:
                b = runner.run_side(runner.ours, inp)
            except UcError as e:
                discarded["fault(unmapped)"] = discarded.get("fault(unmapped)", 0) + 1
                if opts.verbose:
                    log("  discard input %d (ours): UcError: %s" % (i, str(e)[:120]))
                continue
            sb = b["status"]
            if sb[0] == "trap":
                traps[sb[1]] = traps.get(sb[1], 0) + 1
                discarded["ours-trap"] = discarded.get("ours-trap", 0) + 1
                continue
            if b["fakes"] and sb[0] == "ok" and not runner.abi_ok(b):
                discarded["ours-abi-broken(fake)"] = discarded.get("ours-abi-broken(fake)", 0) + 1
                continue
            if a["fakes"] or b["fakes"]:
                fa, fb = {}, {}
                for tg, n, _e, _a in a["fakes"]:
                    fa.setdefault(tg, set()).add(n)
                for tg, n, _e, _a in b["fakes"]:
                    fb.setdefault(tg, set()).add(n)
                if any(len(v) > 1 for v in list(fa.values()) + list(fb.values())) or \
                        any(fa[k] != fb[k] for k in set(fa) & set(fb)):
                    discarded["fake-arity"] = discarded.get("fake-arity", 0) + 1
                    continue
                faked += 1
            valid += 1
            new = bool(run_blocks - runner.blocks) or bool(a["edges"] - runner.orig.edges) or \
                bool(sb[0] == "ok" and b["edges"] - runner.ours.edges)
            runner.blocks |= run_blocks
            runner.orig.edges |= a["edges"]
            if sb[0] == "ok":
                runner.ours.edges |= b["edges"]
            if new:
                corpus.append(inp)
            if steer:
                steer.note_valid(inp, {"orig": a["edges"], "ours": b["edges"] if sb[0] == "ok" else set()})
            cmp_.last_kind = None
            if sb[0] != "ok":
                d = ("execution", "returned normally", "%s: %s" % sb, "ours failed where the original succeeded")
            else:
                d = cmp_.compare(a, b, t)
                if d and fpt is not None:
                    kind_, d_ = fpt.judge(a, b, inp, t)
                    if kind_ == "real":
                        d = d_
                        if d_[0].endswith("(fp-tol)") or d_[0] in ("fp-tol", "control flow"):
                            cmp_.last_kind = None   # an fp judgement replaced the compared observable
                    else:
                        d = None
            if d:
                mism += 1
                sigs[mismatch_sig(d)] += 1
                if first is None and opts.call_regs:
                    # --call-regs: where the live register file at a call site first differs (independent of
                    # which observable the mismatch names, so a register difference can be the explanation)
                    out["call_regs"] = cmp_.call_reg_diff(a, b)
                if first is None:
                    first = {"input": i, "profile": inp.profile, "origin": inp.origin, "observable": d[0],
                             "kind": cmp_.last_kind or ("float-rounding" if d[0] == "st0" else "value"),
                             "orig": d[1], "ours": d[2], "detail": d[3], "args": ["%08x" % w for w in inp.args],
                             "regs": {k: "%08x" % v for k, v in inp.regs.items()},
                             "fake_calls": len(a["fakes"]), "repairs": fixes}
                    if getattr(inp, "alloc_fail", ()):
                        first["alloc_fail"] = list(inp.alloc_fail)      # these malloc/new calls returned NULL
                    if getattr(inp, "extra", {}):
                        first["mapped_at_fault"] = ["%08x+%x" % kv for kv in sorted(inp.extra.items())]
                    try:
                        import pickle
                        os.makedirs(FAIL_DIR, exist_ok=True)
                        pth = os.path.join(FAIL_DIR, "%s_%08x.pkl" % (sid, va))
                        with open(pth, "wb") as f:
                            pickle.dump(dict(inp.__dict__, pagegen=inp.pagegen), f)
                        first["input_file"] = os.path.relpath(pth, S.ROOT)
                    except OSError:
                        pass
                    if opts.verbose:
                        log("  MISMATCH input %d: %s orig=%s ours=%s (%s)" % (i, d[0], d[1], d[2], d[3]))
                if mism >= opts.max_mismatch:
                    break
    finally:
        runner.close()
    out["repaired_inputs"] = repaired
    if steer:
        out["steering"] = dict(steer.stats, constants=steer.n_consts, queued=steer.made)
    covered = set()
    for (ba, bs) in runner.blocks:
        for a in range(ba, ba + bs):
            if a in t.orig_insns:
                covered.add(a)
    total = len(t.orig_insns)
    cov = 100.0 * len(covered) / total if total else 0.0
    out["mismatch_sigs"] = dict(sigs.most_common(40))
    out.update(seed=opts.seed, inputs_planned=opts.inputs, extra_inputs=opts.extra_inputs if getattr(opts, "steer", True) else 0,
               gate=opts.gate, require_all=bool(getattr(opts, "require_all", False)),
               inputs=ran, valid=valid, discarded=discarded, mismatches=mism, first_mismatch=first,
               coverage={"covered_insns": len(covered), "total_insns": total, "pct": round(cov, 1),
                         "all": len(covered) >= total,
                         "missed": ["%08x" % a for a in sorted(set(t.orig_insns) - covered)][:64]},
               tolerated=cmp_.tolerated, ours_traps=traps)
    ecov = out["edges"] = {nm: edge_report(runner, side, opts) for nm, side in (("orig", runner.orig), ("ours", runner.ours))}
    if opts.ulp:
        out["ulp"] = opts.ulp
    if fpt is not None:
        out["fp_tol"] = fpt.stats(valid, mism)
    if out.get("stopped_early"):
        # The input stream is deterministic (--inputs + --seed); the wall clock is not. A row cut short by the
        # row-level deadline must not report a verdict at all: on a loaded machine that turned FAILs into WEAKs
        # (16 of 82 rows in one sweep). Raise --time-limit or run fewer workers.
        v, why = "UNSUPPORTED", ("not a verdict: %s; the run was cut short by the wall-clock limit, everything "
                                 "measured so far is in this JSON" % out["stopped_early"])
    elif out.get("wall_timeout"):
        v, why = "UNSUPPORTED", ("not a verdict: %d input run(s) were cut by the per-run wall-clock cap (--run-seconds); "
                                 "raise it so the verdict depends only on --inputs and --seed" % out["wall_timeout"])
    elif mism and first.get("kind") == "local-address":
        # our code stores the address of its own copy of an object/vtable/constant where the original
        # stores the image's: same_local_copy already accepted every copy whose bytes are identical, so
        # what is left is a real difference - the detail names the differing dwords of the block.
        v, why = "FAIL", ("%d/%d valid inputs mismatched; first: %s (our code stores the address of its own "
                          "copy where the original stores the image's; %s)" % (
                              mism, valid, first["observable"], first.get("detail") or "the copies differ"))
    elif mism:
        v, why = "FAIL", "%d/%d valid inputs mismatched; first: %s" % (mism, valid, first["observable"])
    elif traps:
        v, why = "UNSUPPORTED", "our code reached unresolved symbol(s) %s" % ", ".join(sorted(traps))
    elif valid < max(10, ran // 20):
        top = sorted(discarded.items(), key=lambda x: -x[1])[:3]
        v, why = "UNSUPPORTED", "only %d/%d inputs valid (discards: %s)" % (valid, ran, ", ".join("%s=%d" % x for x in top))
    elif getattr(opts, "gate", "edges") == "insns":
        # legacy gate (before 2026-10-21): instruction coverage of the original only
        if valid >= opts.min_valid and (len(covered) >= total if getattr(opts, "require_all", False) else cov >= opts.min_cov):
            v, why = "PASS", "%d valid inputs, %.1f%% coverage%s [legacy insns gate]" % (
                valid, cov, " (all insns)" if getattr(opts, "require_all", False) else "")
        else:
            need = "100%% (all insns)" if getattr(opts, "require_all", False) else "%.0f%%" % opts.min_cov
            v, why = "WEAK", "%d valid inputs (need %d), %.1f%% coverage (need %s) [legacy insns gate]" % (
                valid, opts.min_valid, cov, need)
    else:
        missed = [nm for nm in ("orig", "ours") if ecov[nm]["missed_n"]]
        edges_txt = "; ".join(edge_summary(nm, ecov[nm]) for nm in ("orig", "ours"))
        all_insns = len(covered) >= total or not getattr(opts, "require_all", False)
        if valid >= opts.min_valid and not missed and all_insns:
            v, why = "PASS", "%d valid inputs, all edges covered (%s), %.1f%% insns" % (valid, edges_txt, cov)
        else:
            need = []
            if valid < opts.min_valid:
                need.append("%d valid inputs (need %d)" % (valid, opts.min_valid))
            for nm in missed:
                need.append("%s misses %d edge(s)" % (nm, ecov[nm]["missed_n"]))
            if not all_insns:
                need.append("--require-all: %d original insns never ran" % (total - len(covered)))
            v, why = "WEAK", "%s; edges %s; %.1f%% insns" % (", ".join(need), edges_txt, cov)
    if cmp_.tolerated.get("local-copy") and v in ("PASS", "WEAK"):
        # our code stored the address of its own copy where the original stores the image's, and the
        # bytes at both were identical: an address difference only (the linker kept two copies)
        why += ("; note: %d address(es) of our own copy of the image's block differed in address only "
                "(the bytes matched)" % cmp_.tolerated["local-copy"])
    if audit_warning and v in ("PASS", "WEAK"):
        why += "; audit advisory (positional pairing): " + audit_warning
    if fpt is not None and v in ("PASS", "WEAK") and fpt.n_fp + fpt.n_oor:
        fs = out["fp_tol"]
        v += "-FP"
        why += ("; fp-tol: %d compared, %d bit-identical, %d fp-tolerable (max %.3g ulp, max err ratio %.3g), "
                "%d out-of-range, %d real mismatches" % (fs["compared"], fs["bit_identical"], fs["fp_tolerable"],
                                                         fs["max_ulp"], fs["max_err_ratio"], fs["out_of_range"],
                                                         fs["real_mismatches"]))
    if v == "FAIL" and aud["local_copy"]:
        # our own copy of a callee ran where the original calls a real function: a call-trace FAIL may be that
        why += "; NOTE " + RA.summary(aud)
    if opts.call_regs and out.get("call_regs"):
        why += "; " + cmp_.call_reg_note(out["call_regs"])
    why += model_note(models)               # a run kept alive by a harness model: say so in the reason ...
    v = model_capped(v, models)             # ... and never report PASS on modelled symbols
    out.update(verdict=v, reason=why, seconds=round(time.time() - t0, 1))
    return out


OURS_IMPORT_PROBES = 40


def mismatch_sig(d):
    """Run-independent signature of one mismatch for the JSON tally: the observable with addresses reduced to
    their low 12 bits (heap/pool bases differ per input, field offsets do not), plus the differing offsets."""
    import re as _re
    obs = _re.sub(r"@([0-9a-f]{5,8})", lambda m: "+" + m.group(1)[-3:], str(d[0]))
    offs = sorted({a[-3:] for a in _re.findall(r"([0-9a-f]{8}): [0-9a-f]{8}/", str(d[3]))})
    return obs + ((" [" + ",".join(offs[:8]) + "]") if offs else "")


NORETURN_WHY = {"_purecall": "pure virtual call", "_CxxThrowException": "C++ throw", "RaiseException": "C++ throw"}


def noreturn_stop(why):
    """A run stopped in an import that can never return to the function: no handler (unmodelled), a C++ throw,
    or a pure virtual call. Handler stops that depend on the input (a huge malloc/memcpy) are not."""
    why = str(why)
    return ":" not in why or why.split(":")[0] in NORETURN_WHY


def edges_missing(runner):
    return any(set(side.cfg.edges) - side.edges - side.cfg_static_unreachable for side in (runner.orig, runner.ours))


def edge_report(runner, side, opts):
    """Edge coverage of one side: totals, the uncovered edges the harness cannot reach (each with its
    reason) and the remaining missed edges (which make the verdict WEAK)."""
    cfg = side.cfg
    allx = set(cfg.edges)
    cov = side.edges & allx
    unr = cfg.unreachable(cov, runner.blocked_calls(side), nan=opts.nan)
    missed = sorted(allx - cov - set(unr))
    req = len(allx) - len(unr)
    return {"total": len(allx), "covered": len(cov), "required": req,
            "pct": round(100.0 * len(cov) / req, 1) if req else 100.0,
            "unreachable": [{"edge": CF.fmt_edge(e), "reason": unr[e]} for e in sorted(unr)],
            "missed_n": len(missed), "missed": [CF.fmt_edge(e) for e in missed[:64]]}


def edge_summary(nm, r):
    s = "%s %d/%d" % (nm, r["covered"], r["required"])
    if r["unreachable"]:
        s += " (+%d unreachable)" % len(r["unreachable"])
    return s


def replay(sid, va, path, opts):
    """Run one saved input (from a FAIL) on both sides and print every observable."""
    import pickle
    t = prepare(sid, va, opts.flags, print, getattr(opts, "src", None))
    if t.verdict:
        print(t.verdict, t.reason)
        return 1
    runner = Runner(t, opts)
    inp = Input()
    inp.__dict__.update(pickle.load(open(path, "rb")))
    inp.__dict__.setdefault("pagegen", 1)       # saved before pagegen existed
    a = runner.run_side(runner.orig, inp)
    b = runner.run_side(runner.ours, inp)
    cmp_ = Comparer(runner, opts)
    print("args", ["%08x" % w for w in inp.args], "regs", {k: "%08x" % v for k, v in inp.regs.items()})
    for nm, o in (("orig", a), ("ours", b)):
        print("%s: %s eax=%08x edx=%08x ebx=%08x esi=%08x edi=%08x ebp=%08x esp=+%x x87=%d fakes=%d" % (
            nm, o["status"], o["eax"], o["edx"], o["ebx"], o["esi"], o["edi"], o["ebp"], o["esp"], o["fpu_depth"],
            len(o["fakes"])))
        for ent in cmp_.norm_calls(o["calls"])[:60]:
            print("    call", fmt_tcall(ent))
        for ent in o["imports"][:40]:
            print("    import", fmt_call((ent[0], tuple(norm_val(v, runner.stk_hi) for v in ent[1]))))
    print("first difference:", cmp_.compare(a, b, t) if a["status"][0] == b["status"][0] == "ok" else "status differs")
    if opts.call_regs and a["status"][0] == b["status"][0] == "ok":
        crd = cmp_.call_reg_diff(a, b)
        if crd:
            print("  " + cmp_.call_reg_note(crd))
    runner.close()
    return 0


def parse_args(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    ap.add_argument("slice")
    ap.add_argument("vas", nargs="*")
    ap.add_argument("--flags")
    ap.add_argument("--src", help="test this source file instead of match/slices/<id>/<id>.cpp")
    ap.add_argument("--inputs", type=int, default=DEFAULT_INPUTS)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--json")
    ap.add_argument("--min-valid", type=int, default=DEFAULT_K)
    ap.add_argument("--min-cov", type=float, default=DEFAULT_C)
    ap.add_argument("--require-all", action="store_true",
                    help="a PASS also requires every original instruction to execute (legacy insns gate: instead of --min-cov)")
    ap.add_argument("--gate", choices=("edges", "insns"), default="edges",
                    help="edges (default): PASS needs every reachable branch edge covered on BOTH builds; "
                         "insns: the legacy gate (original's instruction coverage >= --min-cov)")
    ap.add_argument("--ulp", type=int, default=0)
    ap.add_argument("--strict-calls", action="store_true")
    ap.add_argument("--call-regs", action="store_true",
                    help="capture the full register file (eax ebx ecx edx esi edi ebp esp, xmm0-3) at every "
                         "recorded call site (depth-1 calls, imports, fake calls) and report the first difference "
                         "no tolerance explains: index, callee and values, in the result's 'call_regs' and the "
                         "reason. A diagnostic only - the verdict is unchanged")
    ap.add_argument("--call-regs-strict", action="store_true",
                    help="--call-regs, and a register difference is a mismatch instead of a diagnostic: an input "
                         "that matches every other observable but differs in a live register at a call pair whose "
                         "callee and arguments still agree FAILs")
    ap.add_argument("--strict-audit", action="store_true",
                    help="keep the old behaviour: a resolution-audit disagreement aborts with UNSUPPORTED. By default "
                         "the audit is advisory - the emulator runs anyway and the disagreement is recorded in the "
                         "result (66 rows were blocked by positional operand pairing alone)")
    ap.add_argument("--budget", type=int, default=DEFAULT_BUDGET, help="max instructions per original run (ours: 4x)")
    ap.add_argument("--start-budget", type=int, default=START_BUDGET,
                    help="initial cap; raised to --budget if one of the first %d capped inputs finishes under it" % BUDGET_PROBES)
    ap.add_argument("--time-limit", type=int, default=180, help="seconds per function")
    ap.add_argument("--run-seconds", type=float, default=0,
                    help="wall-clock cap per original run (ours: 4x). Default 0 = off: a wall-clock stop depends on "
                         "machine load, so with it on, any row that hits it reports no verdict (only --inputs and "
                         "--seed decide); the instruction budget (--budget) bounds every run deterministically")
    ap.add_argument("--max-mismatch", type=int, default=5)
    ap.add_argument("--no-fake-calls", dest="fake_calls", action="store_false",
                    help="discard inputs that call through a generated function pointer instead of emulating a do-nothing method")
    ap.add_argument("--transparent", type=lambda v: [int(x, 16) for x in v.split(",")], default=None,
                    help="comma-separated image function VAs (hex) whose calls the depth-1 call trace replaces by the "
                         "calls they make themselves (e.g. a vector deleting destructor one build inlined and the "
                         "other calls); arguments, memory, registers and imports are still compared")
    ap.add_argument("--ignore-abi", action="store_true", help="run even if the original reads an undeclared register argument")
    ap.add_argument("--repairs", type=int, default=24, help="fault repairs per input (0 = off)")
    ap.add_argument("--nan", action="store_true", help="also generate NaN/Inf float bit patterns")
    ap.add_argument("--fpcw", default="%04X" % M.FPCW,
                    help="x87 control word both sides run under (hex; default %04X = 24-bit D3D device-thread state, "
                         "027F = CRT 53-bit state of worker threads)" % M.FPCW)
    ap.add_argument("--fp-tol", action="store_true",
                    help="judge float-only mismatches against the original re-run at 64-bit precision (0x037F): "
                         "rounding-only differences give PASS-FP / WEAK-FP, Inf/NaN/>1e30 cases are counted as out-of-range")
    ap.add_argument("--fp-realistic", action="store_true",
                    help="generate float fields as finite magnitudes in [1e-3, 1e5] (both signs, plus 0/+-1) instead of random bit patterns")
    ap.add_argument("--alias", type=float, default=0.0, help="probability that a generated pointer aliases an argument object")
    ap.add_argument("--mutate", type=float, default=0.6, help="share of inputs mutated from the coverage corpus")
    ap.add_argument("--no-steer", dest="steer", action="store_false",
                    help="no input steering (comparison constants, input-to-state replays, frontier-weighted parents)")
    ap.add_argument("--steer-share", type=float, default=0.35, help="share of inputs taken from the steering queue")
    ap.add_argument("--extra-inputs", type=int, default=DEFAULT_INPUTS,
                    help="inputs beyond --inputs, spent only on steering while an edge is still uncovered")
    ap.add_argument("--no-extra-map", action="store_true",
                    help="fault repair never maps input memory at an untraceable faulting address")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--replay", help="re-run one saved failing input (first_mismatch.input_file) and print details")
    o = ap.parse_args(argv)
    if o.call_regs_strict:
        o.call_regs = True          # strict compares the same capture; capture is what costs the run
    return o


def main(argv=None):
    opts = parse_args(argv)
    context()["m"].set_fpcw(int(opts.fpcw, 16))
    sid = opts.slice
    if opts.vas:
        vas = [int(v, 16) for v in opts.vas]
    else:
        vas = [r[0] for r in S.read_rows(os.path.join(S.slice_dir(sid), "nonmatching.txt"))]
        if not vas:
            sys.exit("%s: no VAs given and nonmatching.txt is empty" % sid)
    if opts.replay:
        return replay(sid, vas[0], opts.replay, opts)
    results = []
    for va in vas:
        print("%s %08x ..." % (sid, va), flush=True)
        try:
            r = test_function(sid, va, opts)
        except Exception as e:      # noqa: BLE001
            # an emulator error escaping a hook (unicorn UcError in fake_call, mig10 SPCheatManager 0x67ec70) killed the
            # whole multi-VA run; the row is UNSUPPORTED and the shared machine is rebuilt for the next VA
            r = {"slice": sid, "va": "%08x" % va, "verdict": "UNSUPPORTED",
                 "reason": "harness error: %s: %s" % (type(e).__name__, str(e)[:160])}
            ctx = context()
            ctx["m"] = machine_hooks(M.Machine(ctx["img"]))
        results.append(r)
        print_result(r)
    if opts.json:
        os.makedirs(os.path.dirname(os.path.abspath(opts.json)), exist_ok=True)
        json.dump(results, open(opts.json, "w"), indent=1)
    return 0 if all(r["verdict"] == "PASS" for r in results) else 1


def print_result(r):
    print("  %-11s %s" % (r["verdict"], r["reason"]))
    if "symbol" in r and r["symbol"]:
        print("    symbol %s (%s); flags %s (%s)" % (r["symbol"][:90], r["symbol_from"], r["flags"], r["flags_from"]))
    if "valid" in r:
        print("    inputs %d, valid %d, discarded %s, coverage %s%% (%d/%d insns), %.1fs" % (
            r["inputs"], r["valid"], r["discarded"] or "{}", r["coverage"]["pct"], r["coverage"]["covered_insns"],
            r["coverage"]["total_insns"], r["seconds"]))
        if r.get("edges"):
            print("    edges: %s" % "; ".join(edge_summary(nm, r["edges"][nm]) for nm in ("orig", "ours")))
            for nm in ("orig", "ours"):
                e = r["edges"][nm]
                if e["missed_n"]:
                    print("      %s missed: %s%s" % (nm, " ".join(e["missed"][:8]), " ..." if e["missed_n"] > 8 else ""))
                groups = {}
                for u in e["unreachable"]:
                    groups.setdefault(u["reason"], []).append(u["edge"])
                for why, es in groups.items():
                    print("      %s unreachable (%d): %s [%s%s]" % (nm, len(es), why, " ".join(es[:4]), " ..." if len(es) > 4 else ""))
        if r.get("fp_tol"):
            print("    fp-tol: %s" % r["fp_tol"])
        tol = {k: v for k, v in r["tolerated"].items() if v}
        if tol:
            print("    tolerated: %s" % tol)
    fm = r.get("first_mismatch")
    if fm:
        print("    first mismatch (input %d, %s, %s): %s\n      orig: %s\n      ours: %s\n      %s" % (
            fm["input"], fm["profile"], fm.get("kind", "?"), fm["observable"], fm["orig"], fm["ours"], fm["detail"]))
        if fm.get("input_file"):
            print("    replay: .venv/bin/python tools/difftest/equiv.py %s %s --replay %s -v" % (
                r.get("slice", "?"), r.get("va", "?"), fm["input_file"]))
    if r.get("resolution_conflicts"):
        print("    resolution conflicts: %s" % r["resolution_conflicts"][:3])


if __name__ == "__main__":
    sys.exit(main())
