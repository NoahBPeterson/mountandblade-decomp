#!/usr/bin/env python3
"""Equivalence layer: symbolic execution of both functions with angr, compared with Z3 (tools/equiv/LAYERS.md).

Copied from spore-decomp tools/equiv/angr_layer.py at commit 82de4ab6c (the same owner), adapted to M&B's Func
(tools/equiv/func.py: ours sits at va 0 in its object, retail callee facts come from work/mb.analysis.bin through
func.image()) and with one addition, the literal-constant check: a DIR32 of ours to a compiler constant (__real@,
__int64@, ??_C@ strings) must be paired with a retail address holding the same bytes (func.const_ok, the check
chk.py makes), else the bijection is rejected (the uninterpreted memory model would otherwise accept a different
constant, since both sides then read the same retail address).

Install (once): angr lives in its own venv, so the project venv keeps its own capstone/pefile versions:

    python3.12 -m venv work/angr-venv && work/angr-venv/bin/pip install angr==10.0.6 pefile capstone
    (Windows: py -3 -m venv work\\angr-venv && work\\angr-venv\\Scripts\\pip install angr==10.0.6 pefile capstone)

`prove()` runs in any interpreter (it needs only capstone and, for retail callee facts, the analysis image); the
solving runs in a child process of the angr interpreter: $EQUIV_ANGR_PYTHON, else work/angr-venv, else the current
interpreter when it can import angr. The child is killed at `timeout` (its whole process group), and it never
starts processes of its own.

Model (one machine state, shared by both versions):
- Inputs: every GPR and the flags are symbols; esp is symbolic with the ABI preconditions 0x10000 <= esp <= 0x7fff0000
  and esp % 4 == 0; DF = 0; x87 stack empty (ftop 0, tags 0), rounding modes round-to-nearest.
- Memory is an uninterpreted array M0 (byte-addressed, Z3 array theory), written through a per-path write log;
  pointers may alias anything, including the live stack. One assumption (the compiler makes it too, whenever it
  moves a push across a load): an address computed from the entry state alone never points into the unallocated
  stack within 1 MB below the entry esp. Memory below esp is dead at calls and returns and is not compared.
- Relocated fields: ours is placed at the retail VA, and each of its relocated fields is rewritten to the retail
  address it is paired with by one consistent bijection (proposed by aligning the relocation-bearing instructions,
  or taken from hints; the same trust the byte-match check places in relocations).
- Calls are events. A callee is unknown code, identical in both versions: its outputs are fresh symbols named by
  the call's index on the path (shared by the two versions), which is sound because the call's inputs are proved
  equal first. L1 inputs: esp, the argument registers its decorated name implies (eax/ecx/edx when unknown), DF,
  the x87 stack, and memory at and above esp except the caller's private frame (saved registers; locals beyond the
  argument window when the window is known and no frame address escaped). L2 inputs: every register and all
  memory at and above esp. Outputs: eax/ecx/edx/flags/xmm/x87 registers (L2: every GPR but esp) and memory outside
  the caller's private frame. esp after the call = esp + 4 + ret N (from the retail callee's `ret N`, else a shared
  symbol in {0, 4, .., 64}); a callee without any reachable `ret` is noreturn and ends the path.
- Counterexamples are reported as "refuted" only when they do not rest on a guess: a heuristic relocation
  pairing, a register read only because an unknown callee's convention might read it, a difference inside the
  caller's own frame at an L1 call, x87 precision, or given hints all make the result "unknown" instead.
- Paths: each version is explored separately (angr, VEX semantics). Paths are paired by their branch-decision bit
  strings, a pairing is used only after its path conditions are proved equal; unpaired paths are compared with
  every path of the other version whose condition overlaps. A pair is equivalent when no input in the overlap
  makes any observable differ: event sequence, call targets and inputs, return target, registers of the level,
  esp, DF, memory.
- Loops (back edges of the static CFG, and rep-prefixed instructions): explored up to N visits of the head. A state
  that arrives once more is accepted only if it provably never leaves the loop: one generic iteration's exit
  conditions depend only on a register set U that the iteration maps to itself, the state's U values are a fixed
  point, no exit is feasible from it and the body writes no memory (then it diverges, and is compared as a path
  ending in `diverge`). N doubles from 2 to 64 until every such state diverges; otherwise the result is unknown.
- x87 arithmetic is modelled with 64-bit doubles (VEX); a proof or refutation is kept only when both versions run
  the same sequence of x87 instructions (mnemonic and memory size), else the result is unknown.

Hints (small, no instructions or values): {"v": 1, "bij": [[retail field offset, ours field offset], ...],
"loops": {"retail"|"ours": {"<head offset hex>": [N, reason]}}, "pairs": [[retail bits, ours bits], ...] for paths
not paired with the same bit string, "fallback": [retail bits, ...]}. Given hints are only checked; a wrong one
gives "unknown" (never trusted).

    work/angr-venv is only the solver's interpreter; run the layer from M's own environment:
    uv run python tools/equiv/angr_layer.py SRC VA [--symbol SYM] [--level L1|L2] [--timeout S] [--hints FILE]
                                            [--save-hints FILE]
"""
import json, os, re, struct, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ANGR_VERSION = "10.0.6"
MAX_BOUND = 64
GPR = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
CALLEE_SAVED = ["ebx", "esi", "edi", "ebp"]
XMM = ["xmm%d" % k for k in range(8)]


# ====================================================================== shared helpers (no angr)
def _md(detail=False):
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = detail
    return md


def decode(code, base):
    """[(capstone insn)] for linear disassembly (MSVC code has no embedded data before the end)."""
    return list(_md(True).disasm(code, base))


def _imm_target(i):
    try:
        return int(i.op_str, 16) if re.fullmatch(r"0x[0-9a-f]+", i.op_str) else None
    except ValueError:
        return None


def classify(code, base):
    """Static facts of one function: {addr: kind} for call/ret/tail/ijmp/trap, branch targets, loop heads."""
    ins = decode(code, base)
    end = base + len(code)
    by = {i.address: i for i in ins}
    kinds, succ = {}, {}
    for i in ins:
        m, a = i.mnemonic, i.address
        fall = a + i.size
        t = _imm_target(i)
        if m == "call":
            kinds[a] = "call"
            succ[a] = [fall]
        elif m in ("ret", "retn"):
            kinds[a] = "ret"
            succ[a] = []
        elif m == "jmp":
            if t is None:
                kinds[a] = "ijmp"
                succ[a] = []
            elif base <= t < end:
                succ[a] = [t]
            else:
                kinds[a] = "tail"
                succ[a] = []
        elif m.startswith("j") or m.startswith("loop"):
            if t is not None and not (base <= t < end):
                kinds[a] = "tailcc"
            succ[a] = [fall] + ([t] if t is not None and base <= t < end else [])
        elif m in ("int3", "ud2", "hlt", "int"):
            kinds[a] = "trap"
            succ[a] = []
        else:
            succ[a] = [fall]
        if m.startswith(("rep ", "repe ", "repne ", "repz ", "repnz ")):
            kinds.setdefault(a, "rep")
    # loop heads: targets of back edges in a DFS from the entry
    heads, color = set(), {}
    stack = [(base, iter(succ.get(base, [])))]
    color[base] = 1
    while stack:
        a, it = stack[-1]
        nxt = next(it, None)
        if nxt is None:
            color[a] = 2
            stack.pop()
            continue
        if nxt not in by:
            continue
        c = color.get(nxt, 0)
        if c == 1:
            heads.add(nxt)
        elif c == 0:
            color[nxt] = 1
            stack.append((nxt, iter(succ.get(nxt, []))))
    reach = {a for a, c in color.items() if c}
    for a, k in kinds.items():
        if k == "rep" and a in reach:
            heads.add(a)
    return {"ins": ins, "kinds": kinds, "succ": succ, "heads": heads, "reach": reach}


def loop_body(info, head):
    """instructions of the natural loop of head: reachable from head (not through head) and reaching head"""
    succ = info["succ"]
    fwd, todo = set(), [head]
    while todo:
        a = todo.pop()
        for b in succ.get(a, []):
            if b != head and b not in fwd and b in succ:
                fwd.add(b)
                todo.append(b)
    pred = {}
    for a, bs in succ.items():
        for b in bs:
            pred.setdefault(b, []).append(a)
    back, todo = set(), [head]
    while todo:
        a = todo.pop()
        for b in pred.get(a, []):
            if b != head and b not in back:
                back.add(b)
                todo.append(b)
    return {head} | (fwd & back)


def x87_signature(code, base):
    """Ordered x87 operations (mnemonic + memory operand size): rounding points of the 80-bit unit."""
    out = []
    for i in decode(code, base):
        if i.mnemonic.startswith("f") and i.mnemonic not in ("fwait",):
            sz = re.search(r"(byte|word|dword|qword|tbyte|xword) ptr", i.op_str)
            out.append(i.mnemonic + (":" + sz.group(1) if sz else ""))
    return out


_X87_SHUFFLE = ("fxch", "ffree", "fincstp", "fdecstp", "fnop", "fwait", "wait")
_X87_FAMILY = (("fiadd", "add"), ("fadd", "add"), ("fisubr", "sub"), ("fisub", "sub"), ("fsub", "sub"),
               ("fimul", "mul"), ("fmul", "mul"), ("fidivr", "div"), ("fidiv", "div"), ("fdiv", "div"),
               ("fucom", "cmp"), ("fcom", "cmp"), ("ficom", "cmp"), ("ftst", "cmp"))


def x87_signature_norm(code, base):
    """(M&B) The x87 work up to stack shuffling and operand order: a sorted multiset of arithmetic families
    (add/sub/mul/div/cmp, register or memory operand with its size), loads and stores to memory with their sizes,
    and every other operation by name; fxch, fld/fst/fstp st(i) are dropped. Only used to decide whether running
    the solver can be worth it: when even this differs, the strict rule (x87_signature) cannot pass either."""
    out = []
    for i in decode(code, base):
        m = i.mnemonic
        if not m.startswith("f") or m in _X87_SHUFFLE:
            continue
        mem = re.search(r"(byte|word|dword|qword|tbyte|xword) ptr", i.op_str)
        if m in ("fld", "fst", "fstp") and not mem:
            continue
        fam = next((f for p, f in _X87_FAMILY if m.startswith(p)), None)
        if fam:
            # an operation with a memory operand is a load of it plus the operation (the conversion to the
            # register format is exact): fmul dword [m] ~ fld dword [m]; fmul st(1)
            if mem:
                out.append("load:" + mem.group(1) + (":int" if m.startswith("fi") else ""))
            out.append(fam)
        elif m in ("fld", "fild", "fbld"):
            out.append("load:" + mem.group(1) + (":int" if m == "fild" else ""))
        elif m in ("fst", "fstp", "fist", "fistp", "fisttp"):
            out.append("store:" + mem.group(1) + (":int" if m.startswith("fi") else ""))
        else:
            out.append(m.rstrip("p") + (":" + mem.group(1) if mem else ""))
    return sorted(out)


# ---------------------------------------------------------------------- decorated names (MSVC)
_SIMPLE = {c: 4 for c in "CDEFGHIJK"}
_SIMPLE.update({"M": 4, "N": 8, "O": 8})
_UNDER = {"N": 4, "J": 8, "K": 8, "W": 4, "D": 4, "E": 4, "F": 4, "G": 4, "H": 4, "I": 4}


class _Mangled:
    def __init__(self, s, i):
        self.s, self.i, self.back = s, i, []

    def peek(self, n=1):
        return self.s[self.i:self.i + n]

    def take(self, n=1):
        r = self.s[self.i:self.i + n]
        self.i += n
        return r

    def name_frags(self):
        """skip a qualified name 'a@b@@' (no templates)"""
        while True:
            c = self.peek()
            if c == "":
                raise ValueError
            if c == "@":
                self.take()
                return
            if c.isdigit():
                self.take()
                continue
            if c == "?":
                raise ValueError("template or special name")
            j = self.s.index("@", self.i)
            self.i = j + 1

    def type(self):
        """size in bytes of one type (pass-by-value), None when unknown; advances past it"""
        start = self.i
        c = self.take()
        if c == "":
            raise ValueError
        if c in _SIMPLE:
            return _SIMPLE[c]
        if c == "X":
            return 0
        if c == "_":
            d = self.take()
            if d not in _UNDER:
                raise ValueError
            sz = _UNDER[d]
        elif c in "PQRSAB":
            q = self.take()
            if q == "E":
                q = self.take()
            if q == "6":                                  # function pointer
                self.take()                               # calling convention
                self.type()
                self.params()
                self.take() if self.peek() == "Z" else None
            else:
                self.type()
            sz = 4
        elif c in "VUT":
            self.name_frags()
            sz = None
        elif c == "W":
            self.take()
            self.name_frags()
            sz = 4
        elif c.isdigit():
            return self.back[int(c)] if int(c) < len(self.back) else None
        else:
            raise ValueError("type %r" % c)
        if self.i - start > 1 and len(self.back) < 10:
            self.back.append(sz)
        return sz

    def params(self):
        """[sizes] of the parameter list, or 'varargs'"""
        if self.peek() == "X":
            self.take()
            return []
        out = []
        while True:
            c = self.peek()
            if c == "@":
                self.take()
                return out
            if c == "Z":
                self.take()
                return "varargs"
            if c == "":
                raise ValueError
            out.append(self.type())


def name_facts(nm):
    """Facts a decorated name gives: reads (argument registers), W (stack argument bytes), ret64, fret (float
    return), conv. Missing keys are unknown."""
    if not nm:
        return {}
    if nm.startswith("__imp_"):
        nm = nm[6:]
    f = {}
    if nm.startswith(("__chkstk", "__alloca_probe")):
        return {"chkstk": True, "reads": ["eax"]}
    if nm.startswith(("__ftol2", "__ftol")):
        return {"reads": [], "W": 0, "fdelta": 1, "ret64": True}
    if nm.startswith("__CI"):
        # (M&B) the CRT's x87 intrinsics take their arguments on the x87 stack and leave the result there:
        # two arguments for pow/atan2/fmod (one value popped), one for the others
        return {"reads": [], "W": 0, "fdelta": 1 if nm[4:] in ("pow", "atan2", "fmod") else 0, "fret": True}
    m = re.fullmatch(r"_([A-Za-z0-9_$]+)@(\d+)", nm)
    if m:
        return {"reads": [], "W": int(m.group(2)), "conv": "stdcall"}
    m = re.fullmatch(r"@([A-Za-z0-9_$]+)@(\d+)", nm)
    if m:
        return {"reads": ["ecx", "edx"], "conv": "fastcall"}
    if nm.startswith("_") and "@" not in nm:
        return {"reads": [], "conv": "cdecl"}
    if not nm.startswith("?"):
        return {}
    m = re.search(r"@@(?:[QRUVABEFIJMN]([A-D])|([CDKLSTYZ]))([AEGI])", nm)
    if not m:
        return {}
    conv = {"A": "cdecl", "E": "thiscall", "G": "stdcall", "I": "fastcall"}[m.group(3)]
    f["conv"] = conv
    f["reads"] = {"cdecl": [], "thiscall": ["ecx"], "stdcall": [], "fastcall": ["ecx", "edx"]}[conv]
    p = _Mangled(nm, m.end())
    try:
        if p.peek() == "?":                               # return type cv-qualifier for class returns
            p.take(2)
        rt = p.peek(2)
        f["ret64"] = rt in ("_J", "_K")
        f["void"] = rt[:1] == "X"
        f["fret"] = rt[:1] in ("M", "N", "O")
        if p.peek() == "@":                               # constructors/destructors have no return type
            p.take()
        else:
            p.type()
        ps = p.params()
        if ps != "varargs" and all(x is not None for x in ps):
            if conv == "fastcall":
                regs, W = 0, 0
                for x in ps:
                    if x <= 4 and regs < 2:
                        regs += 1
                    else:
                        W += (x + 3) & ~3
                f["W"] = W
            else:
                f["W"] = sum((x + 3) & ~3 for x in ps)
    except (ValueError, IndexError):
        pass
    return f


# ====================================================================== parent side
def _angr_python():
    env = os.environ.get("EQUIV_ANGR_PYTHON")
    if env:
        return env
    for p in (os.path.join(ROOT, "work", "angr-venv", "bin", "python"),
              os.path.join(ROOT, "work", "angr-venv", "Scripts", "python.exe")):
        if os.path.exists(p):
            return p
    try:
        import importlib.util
        if importlib.util.find_spec("angr"):
            return sys.executable
    except Exception:
        pass
    return None


def _tok_key(tok):
    """A token as a hashable key and (name, addend) when it has a symbol name."""
    if tok[0] == "abs":
        return ("abs", tok[1]), None
    _, nm, typ, addend = tok
    addend = addend if addend < 0x80000000 else addend - (1 << 32)
    return ("sym", nm, addend), (nm, addend)


def _reloc_shapes(code, base, relocs):
    """[(field offset, shape)] in code order; shape abstracts registers, keeps mnemonic and operand role."""
    out = []
    for i in decode(code, base):
        o0, o1 = i.address - base, i.address - base + i.size
        for o in sorted(relocs):
            if o0 <= o < o1:
                ops = re.sub(r"\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?bp|e?sp|xmm\d)\b", "R", i.op_str)
                # (M&B) every number, decimal too: ours' relocated fields hold 0, which capstone prints as "0"
                # (spore's DLL builds had real addresses); st(i) keeps its index
                ops = re.sub(r"(?<![\w(])(0x[0-9a-f]+|\d+)\b", "K", ops)
                out.append((o, "%s %s @%d" % (i.mnemonic, ops, o - o0)))
    return out


def propose_bijection(retail, ours):
    """[(retail field offset, ours field offset)], clean: an alignment of the relocation-bearing instructions."""
    import difflib
    ra = _reloc_shapes(retail.code, retail.va, retail.relocs)
    ob = _reloc_shapes(ours.code, ours.va, ours.relocs)
    sm = difflib.SequenceMatcher(None, [s for _, s in ra], [s for _, s in ob], autojunk=False)
    pairs, clean = [], True
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            pairs += [(ra[i1 + k][0], ob[j1 + k][0], True) for k in range(i2 - i1)]
        else:
            clean = False
            if tag == "replace":
                for k in range(min(i2 - i1, j2 - j1)):
                    if ra[i1 + k][1].split()[0] == ob[j1 + k][1].split()[0]:
                        pairs.append((ra[i1 + k][0], ob[j1 + k][0], False))
    return [(a, b) for a, b, _ in pairs], clean


def build_mapping(retail, ours, pairs, folds=()):
    """ours key -> retail address, checked: one-to-one, and one base per symbol name. -> (map, problems)
    folds (region mode): ours keys allowed to share their retail address with another key (functions the
    original's linker folded, regions.fold_checker)."""
    m, inv, base, problems = {}, {}, {}, []
    folds = {tuple(f) for f in folds}
    for ro, oo in pairs:
        if ro not in retail.relocs or oo not in ours.relocs:
            problems.append("pair (%x, %x) is not a relocated field" % (ro, oo))
            continue
        x = retail.relocs[ro][1]
        k, nmad = _tok_key(ours.relocs[oo])
        if k in m and m[k] != x or x in inv and inv[x] != k and k not in folds:
            problems.append("inconsistent pairing at retail +%x / ours +%x" % (ro, oo))
            continue
        if x in inv and inv[x] != k:
            m[k] = x                                   # a fold: inv keeps the first key
            continue
        if nmad:
            b = (x - nmad[1]) & 0xffffffff
            if base.setdefault(nmad[0], b) != b:
                problems.append("symbol %s paired with two bases" % nmad[0])
                continue
        m[k], inv[x] = x, k
    for oo, tok in ours.relocs.items():               # same symbol, other addend: its base is known
        k, nmad = _tok_key(tok)
        if k not in m and nmad and nmad[0] in base:
            x = (base[nmad[0]] + nmad[1]) & 0xffffffff
            if x not in inv:
                m[k], inv[x] = x, k
    return m, problems


def pair_constants_by_content(retail, ours, pairs):
    """(M&B) The alignment of relocation-bearing instructions can pair a literal constant of ours with a retail
    field holding another constant (when the two compilers order the loads differently). For each DIR32 of ours to
    a known literal (func.const_ok knows its bytes): drop a pair whose retail target holds other bytes, and pair it
    with a retail field whose target holds the same bytes (the check chk.py makes). The result is still checked by
    build_mapping (one-to-one) and const_problems."""
    try:
        if HERE not in sys.path:
            sys.path.insert(0, HERE)
        import func as FN
    except BaseException:
        return pairs
    if not hasattr(FN, "const_ok"):
        return pairs
    FN.set_consts(ours)
    consts = (ours.meta or {}).get("consts") or {}
    is_const = {o for o, t in ours.relocs.items() if t[0] == "sym" and t[2] == 0x06 and t[1] in consts}
    if not is_const:
        return pairs
    out, done = [], set()
    for ro, oo in pairs:
        if oo in is_const and ro in retail.relocs and not FN.const_ok(retail.relocs[ro], ours.relocs[oo]):
            continue
        out.append((ro, oo))
        done.add(oo)
    used = {ro for ro, _ in out}
    for oo in sorted(is_const - done):
        tok = ours.relocs[oo]
        cands = [ro for ro, t in sorted(retail.relocs.items()) if t[0] == "abs" and FN.const_ok(t, tok)]
        free = [ro for ro in cands if ro not in used] or cands
        if free:
            out.append((free[0], oo))
            used.add(free[0])
    return out


_SYMDB = []


REL32_TYPES = (0x14,)


def symbol_want(tok, db=None):
    """(M&B) the original's address the symbols file gives a relocation token of ours: the symbol's one known
    address, plus the addend for a data reference (sym+k, DIR32: g_skill_tbl+0xbc must be the original's
    g_skill_tbl + 0xbc); a call (REL32) names the symbol itself. None when the name has no single known address."""
    if tok[0] != "sym":
        return None
    try:
        db = db or _symdb()
    except BaseException:
        return None
    vas = set(db.mangled.get(tok[1], ()))
    if len(vas) != 1:
        return None
    a = next(iter(vas))
    if tok[2] in REL32_TYPES:
        return a
    add = tok[3] if tok[3] < 0x80000000 else tok[3] - (1 << 32)
    return (a + add) & 0xffffffff


def symbol_problems(ours, mapping):
    """(M&B) The pairing must not contradict the project's symbols file: when a symbol of ours (a function or a
    named global) has exactly one plausible address in tools/difftest/resolve.SymbolDB (symbols/*), the retail
    address paired with it must be that one, plus the addend for a reference sym+k (0x520270 paired g_skill_tbl+0xbc
    with the original's g_skill_tbl+0x78 before addends were checked). Unknown names are not checked (the
    bijection stays the trusted pairing, as in the byte-match check)."""
    try:
        db = _symdb()
    except BaseException:
        return []
    out, seen = [], set()
    for o, tok in sorted(ours.relocs.items()):
        k, _nm = _tok_key(tok)
        if k[0] != "sym" or k not in mapping or k in seen:
            continue
        seen.add(k)
        want = symbol_want(tok, db)
        if want is not None and want != mapping[k]:
            out.append("%s%s is 0x%x in the symbols file, paired with 0x%x" % (
                k[1][:50], "%+#x" % k[2] if k[2] and tok[2] not in REL32_TYPES else "", want, mapping[k]))
    return out


def _symdb():
    if not _SYMDB:
        if HERE not in sys.path:
            sys.path.insert(0, HERE)
        import func as FN
        import resolve as R
        _SYMDB.append(R.SymbolDB(FN.image()))
    return _SYMDB[0]


def pair_by_symbols(retail, ours, pairs):
    """(M&B) Where the symbols file knows the address of one of our symbols (exactly one entry), pair each field
    of ours naming it with a retail field holding that address (+ addend), instead of the shape alignment, which
    swaps pairs when the two compilers order the instructions differently (g_game/g_items in 0x46b190). The result
    is checked like any pairing (build_mapping, symbol_problems)."""
    try:
        db = _symdb()
    except BaseException:
        return pairs
    by_val = {}
    for ro, t in sorted(retail.relocs.items()):
        if t[0] == "abs":
            by_val.setdefault(t[1], []).append(ro)
    fixed = {}
    for oo, t in sorted(ours.relocs.items()):
        if t[0] != "sym":
            continue
        vas = set(db.mangled.get(t[1], ()))
        if len(vas) != 1:
            continue
        add = t[3] if t[3] < 0x80000000 else t[3] - (1 << 32)
        want = next(iter(vas)) if t[2] == 0x14 else (next(iter(vas)) + add) & 0xffffffff
        # no retail field holds the address: any proposed pair for this field is wrong (symbol_problems would
        # reject the whole pairing); leave the field unpaired instead (a fresh address: a call there or a read of
        # it cannot match anything of the original, so no proof can rest on it)
        fixed[oo] = by_val.get(want) or []
    if not fixed:
        return pairs
    out = [(ro, oo) for ro, oo in pairs if oo not in fixed]
    used = {ro for ro, _ in out}
    for oo, cands in sorted(fixed.items()):
        if not cands:
            continue
        free = [ro for ro in cands if ro not in used] or cands
        out.append((free[0], oo))
        used.add(free[0])
    return out


def const_problems(ours, mapping):
    """Every DIR32 of ours to a literal constant must map to a retail address holding the same bytes
    (func.const_ok). A constant of ours left unpaired gets a fresh address and is caught there (clean = False)."""
    try:
        if HERE not in sys.path:
            sys.path.insert(0, HERE)
        import func as FN
    except BaseException:
        return []
    if not hasattr(FN, "const_ok"):
        return []
    FN.set_consts(ours)
    out = []
    for o, tok in sorted(ours.relocs.items()):
        if tok[0] != "sym" or tok[2] != 0x06:
            continue
        k, _nm = _tok_key(tok)
        if k not in mapping:
            continue
        if not FN.const_ok(("abs", mapping[k]), tok):     # mapping[k]: the paired retail field value
            out.append("constant %s at ours +%x has other bytes in the original" % (tok[1][:40], o))
    return out


def _rel32_fields(code, base):
    out = {}
    for i in decode(code, base):
        b = i.bytes
        if (b[0] in (0xE8, 0xE9) and i.size == 5) or (b[0] == 0x0F and 0x80 <= b[1] <= 0x8F and i.size == 6):
            out[i.address - base + i.size - 4] = i.address + i.size
    return out


def patch_ours(retail, ours, mapping):
    """ours' bytes placed at the retail VA, with every relocated field rewritten to its paired retail address
    (an unpaired one gets a fresh address that nothing in retail uses). -> (code, unpaired count, names by addr)"""
    code = bytearray(ours.code)
    rel32 = _rel32_fields(ours.code, ours.va)
    fresh, unpaired, names = 0x7ff00000, 0, {}
    used = {t[1] for t in retail.relocs.values()}
    for o, tok in sorted(ours.relocs.items()):
        k, nmad = _tok_key(tok)
        if k in mapping:
            x = mapping[k]
        else:
            while fresh in used:
                fresh += 0x1000
            x, unpaired = fresh, unpaired + 1
            mapping[k] = x
            used.add(x)
        if nmad:
            names[x] = nmad[0]
        if o in rel32:
            struct.pack_into("<i", code, o, ((x - (retail.va + o + 4) + 0x80000000) & 0xffffffff) - 0x80000000)
        else:
            struct.pack_into("<I", code, o, x)
    return bytes(code), unpaired, names


def _dll_names(ours):
    """{address in the DLL: decorated name} from the /LTCG link map next to the DLL"""
    dll = ours.meta.get("dll") if ours.meta else None
    if not dll:
        return {}
    mp = os.path.join(os.path.dirname(dll), "ltcg.map")
    if not os.path.exists(mp):
        return {}
    out = {}
    for line in open(mp, encoding="latin-1"):
        mm = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+f\s", line)
        if mm:
            out.setdefault(int(mm.group(2), 16), mm.group(1))
    return out


def scan_callee(va, budget=4000):
    """Retail callee facts from the analysis image: {'retn': N} when every reachable ret pops N, 'noreturn' when
    the reachable code (direct jumps followed, calls not entered) has no ret and no indirect jump."""
    try:
        if HERE not in sys.path:
            sys.path.insert(0, HERE)
        import func as FN
        img = FN.image()
    except BaseException:
        return {}
    md = _md()
    seen, todo, rets, opaque = set(), [va], set(), False
    while todo and len(seen) < budget:
        a = todo.pop()
        while a not in seen and len(seen) < budget:
            seen.add(a)
            try:
                b = bytes(img.read(a, 16)) if img.in_text(a) else b""
            except Exception:
                opaque = True
                break
            i = next(md.disasm(b, a), None)
            if i is None:
                opaque = True
                break
            m = i.mnemonic
            if m in ("ret", "retn"):
                rets.add(int(i.op_str, 16) if i.op_str else 0)
                break
            if m in ("int3", "ud2", "hlt"):
                break
            t = _imm_target(i)
            if m == "jmp":
                if t is None:
                    opaque = True
                else:
                    todo.append(t)
                break
            if m.startswith("j") and m != "jmp":
                if t is not None:
                    todo.append(t)
            a = i.address + i.size
    if todo:
        opaque = True
    out = {}
    if len(rets) == 1:
        out["retn"] = rets.pop()
    elif not rets and not opaque:
        out["noreturn"] = True
    return out


def _call_sites(code, base):
    """{call instruction offset: ('direct', target) | ('mem', disp32) | ('other', None)}"""
    out = {}
    for i in decode(code, base):
        if i.mnemonic in ("call", "jmp") or (i.mnemonic.startswith("j") and _imm_target(i) is not None):
            t = _imm_target(i)
            if t is not None:
                out[i.address - base] = ("direct", t)
            else:
                mm = re.fullmatch(r"dword ptr \[(0x[0-9a-f]+)\]", i.op_str)
                out[i.address - base] = ("mem", int(mm.group(1), 16)) if mm else ("other", None)
    return out


def prepare(retail, ours, level, hints, folds=()):
    """Everything the worker needs, as JSON-able data, plus notes for the detail line."""
    notes = []
    if hints and "bij" in hints:
        pairs, clean = [tuple(p) for p in hints["bij"]], True
    else:
        pairs, clean = propose_bijection(retail, ours)
        pairs = pair_constants_by_content(retail, ours, pairs)
        pairs = pair_by_symbols(retail, ours, pairs)
    mapping, problems = build_mapping(retail, ours, pairs, folds)
    cp = const_problems(ours, mapping) + symbol_problems(ours, mapping)
    if cp:      # never a mere note: a proof over a wrongly paired constant would be a false proof
        raise HintError("relocation pairing check failed (literal constant / symbols file): " + "; ".join(cp[:3]))
    if problems:
        if hints and "bij" in hints:
            raise HintError("hint bijection rejected: " + "; ".join(problems[:3]))
        clean = False
        notes += problems[:3]
    pcode, unpaired, oname = patch_ours(retail, ours, mapping)
    if unpaired:
        clean = False
        notes.append("%d relocated field(s) of ours unpaired" % unpaired)
    dll = _dll_names(ours)
    inv = {}
    for k, x in mapping.items():
        if k[0] == "sym":
            inv.setdefault(x, k[1])
        elif k[1] in dll:
            inv.setdefault(x, dll[k[1]])
    # callee facts, keyed by retail target address (direct) or "m<addr>" (call [addr])
    facts = {}
    for side_code in (retail.code, pcode):
        for o, (kind, t) in _call_sites(side_code, retail.va).items():
            if kind == "other" or (kind == "direct" and retail.va <= t < retail.va + len(retail.code)):
                continue
            key = ("%x" % t) if kind == "direct" else ("m%x" % t)
            if key in facts:
                continue
            f = {}
            nm = inv.get(t) or oname.get(t)
            f.update(name_facts(nm))
            if kind == "direct":
                sc = scan_callee(t)
                if "retn" in sc:
                    f["retn"] = sc["retn"]
                    f["retn_code"] = True           # from the original callee's own `ret N` (not a name)
                    if f.get("conv") in ("stdcall", "thiscall") or (f.get("conv") is None and sc["retn"]):
                        f["W"] = sc["retn"] if f.get("conv") != "fastcall" else f.get("W")
                if sc.get("noreturn"):
                    f["noreturn"] = True
            elif nm:
                m = re.search(r"@(\d+)$", nm)
                if m and f.get("conv") == "stdcall":
                    f["retn"] = int(m.group(1))
            if f.get("conv") == "cdecl" and "retn" not in f:
                f["retn"] = 0
            f["name"] = nm or ""
            facts[key] = f
    own = name_facts(ours.meta.get("symbol") if ours.meta else ours.name) or name_facts(ours.name)
    ret64 = own.get("ret64", True) if own else True
    # M&B: a void function's eax at its return is not part of the calling contract when the original writes eax
    # (kill set, facts.py): no caller, even one compiled with the callee's kill set, can read it. L1 only; prove.py
    # asks for L1 only when the kill sets are the same.
    void_eax_dead = False
    if own.get("void"):
        try:
            if HERE not in sys.path:
                sys.path.insert(0, HERE)
            import facts as FA
            void_eax_dead = "eax" in FA.retail_kill(retail.va)
        except BaseException:
            void_eax_dead = False
    sa, sb = x87_signature(retail.code, retail.va), x87_signature(pcode, retail.va)
    job = {
        "level": level, "facts": facts, "ret64": ret64, "clean": clean, "void_eax_dead": void_eax_dead,
        "argw": own_argw(own, retail),
        "frame_same": frame_alloc(retail.code, retail.va) == frame_alloc(pcode, retail.va),
        "x87": bool(sa or sb), "x87_same": sa == sb,
        "x87_norm_same": sa == sb or x87_signature_norm(retail.code, retail.va) == x87_signature_norm(pcode, retail.va),
        "sides": {"retail": {"code": retail.code.hex(), "base": retail.va},
                  "ours": {"code": pcode.hex(), "base": retail.va}},
        "hints": hints or {}, "bij": [list(p) for p in pairs], "notes": notes,
    }
    return job


def own_argw(own, retail):
    """(M&B) the function's own stack argument bytes: from its decorated name, else (callee-pops conventions) from
    the original's `ret N` when every return pops the same N; 0 when unknown"""
    w = own.get("W") if own else None
    if not isinstance(w, int) and own and own.get("conv") in ("thiscall", "stdcall"):
        ns = {int(i.op_str, 16) if i.op_str else 0 for i in decode(retail.code, retail.va) if i.mnemonic == "ret"}
        w = ns.pop() if len(ns) == 1 else None
    return w if isinstance(w, int) and 0 <= w <= 0x400 else 0


def frame_alloc(code, base):
    """bytes the prologue allocates (register pushes + sub esp, imm) before the first branch or call"""
    n = 0
    for i in decode(code, base):
        m = i.mnemonic
        if m == "push" and re.fullmatch(r"e[a-z]{2}", i.op_str):
            n += 4
        elif m == "sub" and i.op_str.startswith("esp, "):
            try:
                n += int(i.op_str[5:], 0)
            except ValueError:
                return None
        elif m.startswith(("j", "call", "ret", "loop")):
            break
    return n


class HintError(Exception):
    pass


def prove(retail, ours, level, hints, timeout):
    """LAYERS.md API. Runs the solver in a child process (killed at `timeout`)."""
    t0 = time.time()
    res = lambda st, d, h=None: {"status": st, "seconds": round(time.time() - t0, 2), "detail": d,
                                 "hints": h or {}}
    if level not in ("L1", "L2"):
        return res("unknown", "level must be L1 or L2")
    py = _angr_python()
    if not py:
        return res("unknown", "angr is not installed (see tools/equiv/angr_layer.py docstring)")
    try:
        job = prepare(retail, ours, level, hints)
    except HintError as e:
        return res("unknown", str(e))
    except Exception as e:
        return res("unknown", "prepare failed: %s: %s" % (type(e).__name__, e))
    if job["x87"] and not job["x87_norm_same"]:
        # (M&B) no proof can pass the x87 rule, and a counterexample would not be trusted: do not run the solver
        return res("unknown", "x87 instruction sequences differ (not even the same arithmetic up to stack "
                   "shuffling): the 64-bit x87 model cannot prove it")
    job["timeout"] = max(1.0, timeout - (time.time() - t0) - 1.0)
    kw = {}
    if os.name == "nt":
        kw["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        kw["start_new_session"] = True
    env = dict(os.environ, PYTHONHASHSEED="0")
    p = subprocess.Popen([py, "-I", os.path.abspath(__file__), "--worker"], stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, **kw)
    try:
        out, err = p.communicate(json.dumps(job).encode(), timeout=max(1.0, timeout - (time.time() - t0)))
    except subprocess.TimeoutExpired:
        _kill(p)
        return res("unknown", "timeout after %.0fs" % timeout)
    except BaseException:
        _kill(p)
        raise
    try:
        r = json.loads(out.decode().strip().splitlines()[-1])
    except Exception:
        return res("unknown", "worker failed: %s" % err.decode(errors="replace")[-600:])
    r["seconds"] = round(time.time() - t0, 2)
    if job["notes"]:
        r["detail"] = r.get("detail", "") + " [bijection: %s]" % "; ".join(job["notes"])
    return r


FAST_TIMEOUT = 30.0         # a mutant run's timeout (prove_mutant)


def prove_mutant(retail, ours, level, base_hints=None, timeout=120.0):
    """The self-test's run on a mutant (prove.py): at most FAST_TIMEOUT seconds, with the original proof's
    relocation pairing (only the pairing: its loop bounds and path pairs are about the original build). -> the
    layer result plus "decided": True when proved or when the run found a difference (a trusted or untrusted
    counterexample), False for a timeout or anything unsupported; prove.py reruns an undecided mutant fully."""
    h = {"bij": base_hints["bij"]} if base_hints and base_hints.get("bij") else None
    r = prove(retail, ours, level, h, min(timeout, FAST_TIMEOUT))
    if h is not None and r["status"] != "proved" and re.search(r"hint bijection|pairing check", r["detail"]):
        r = prove(retail, ours, level, None, min(timeout, FAST_TIMEOUT))
    d = r.get("detail") or ""
    r["decided"] = r["status"] in ("proved", "refuted") or (
        bool(re.search(r"difference: |x87 instruction sequences differ", d)) and
        not re.search(r"timeout|solver returned unknown|more than \d+ paths", d.split(" | ")[0]))
    return r


def _kill(p):
    try:
        if os.name == "nt":
            p.kill()
        else:
            import signal
            os.killpg(p.pid, signal.SIGKILL)
    except Exception:
        try:
            p.kill()
        except Exception:
            pass
    try:
        p.wait(5)
    except Exception:
        pass


# ====================================================================== worker side (angr + z3)
class Unsupported(Exception):
    pass


class GiveUp(Exception):
    pass


X87_UF_USED = set()        # x87 operations modelled as uninterpreted functions on this run (see install_x87_ops)


def install_x87_ops():
    """M&B addition. angr 10.0.6 has no implementation of the VEX ops the x87 lifts fsqrt, fsin, fcos, fptan,
    fpatan, fscale, fyl2x, fyl2xp1 and fprem/fprem1 to (Iop_SqrtF64, Iop_SinF64, ...: in irop.unsupported, stepping
    raises UnsupportedIROpError). Iop_SqrtF64 becomes the IEEE square root (claripy fpSqrt, correctly rounded, as
    the x87 is under the 53-bit precision control the MSVC CRT sets); the others become uninterpreted functions of
    their arguments: a fresh 64-bit symbol per distinct argument tuple (by structural hash), the same symbol for the
    same arguments on both sides. That over-approximates the real function (a proof holds for every function), so
    proofs stay sound; a counterexample that involves one is not trusted (reported as unknown)."""
    from angr import claripy as C
    from angr.engines.vex.claripy import irop as I
    from angr.state_plugins.sim_action_object import SimActionObject

    class FOp:
        _float = True

        def __init__(self, name, fn):
            self.name, self.fn = name, fn

        def calculate(self, *args):
            args = [a.to_claripy() if isinstance(a, SimActionObject) else a for a in args]
            return self.fn(args)

    def sqrt(args):
        return C.fpSqrt(I.translate_rm(args[0]), args[1].raw_to_fp())

    def uf(name, nres_bits=64):
        def f(args):
            X87_UF_USED.add(name)
            h = abs(hash((name,) + tuple(a.hash() for a in args))) & 0xffffffffffff
            v = C.BVS("uf_%s_%x" % (name[4:], h), nres_bits, explicit_name=True)
            return v.raw_to_fp() if nres_bits == 64 else v
        return f

    I.operations["Iop_SqrtF64"] = FOp("Iop_SqrtF64", sqrt)
    for n in ("Iop_SinF64", "Iop_CosF64", "Iop_TanF64", "Iop_AtanF64", "Iop_ScaleF64", "Iop_Yl2xF64",
              "Iop_Yl2xp1F64", "Iop_PRemF64", "Iop_PRem1F64", "Iop_2xm1F64"):
        I.operations[n] = FOp(n, uf(n))
    for n in ("Iop_PRemC3210F64", "Iop_PRem1C3210F64"):
        I.operations[n] = FOp(n, uf(n, 32))
    for n in list(I.operations):
        I.unsupported.discard(n) if hasattr(I.unsupported, "discard") else None


def worker_main():
    import logging
    logging.disable(logging.CRITICAL)
    install_x87_ops()
    job = json.loads(sys.stdin.read())
    if job.get("regions"):
        return region_worker_main(job)
    t0 = time.time()
    try:
        r = Worker(job).run()
    except (Unsupported, GiveUp) as e:
        r = {"status": "unknown", "detail": str(e), "hints": {}}
    except Exception as e:
        import traceback
        r = {"status": "unknown", "detail": "error: %s: %s | %s" % (
            type(e).__name__, e, traceback.format_exc().strip().splitlines()[-3][:200]), "hints": {}}
    r["seconds"] = round(time.time() - t0, 2)
    sys.stdout.write(json.dumps(r) + "\n")
    sys.stdout.flush()


def serve_main():
    """(region mode) a persistent worker (regions._Pool): one job per input line, {"regions": [...]} alone reuses
    the previous line's job; one result line per region, then {"done": 1}"""
    import logging
    logging.disable(logging.CRITICAL)
    install_x87_ops()
    base = None
    for ln in sys.stdin:
        if not ln.strip():
            continue
        msg = json.loads(ln)
        if set(msg) != {"regions"}:
            base = {k: v for k, v in msg.items() if k != "regions"}
        if base is not None:
            region_worker_main(dict(base, regions=msg["regions"]))
        sys.stdout.write('{"done": 1}\n')
        sys.stdout.flush()


def region_worker_main(job):
    """(region mode, tools/equiv/regions.py) one result line per region of job["regions"], each with its own
    deadline; the projects are loaded once"""
    regs = job.pop("regions")
    for reg in regs:
        t0 = time.time()
        X87_UF_USED.clear()
        try:
            r = Worker(dict(job, region=reg, timeout=reg["timeout"], hints=reg.get("hints") or {})).run_region()
        except (Unsupported, GiveUp) as e:
            r = {"status": "unknown", "detail": str(e), "hints": {}}
        except Exception as e:
            import traceback
            r = {"status": "unknown", "detail": "error: %s: %s | %s" % (
                type(e).__name__, e, traceback.format_exc().strip().splitlines()[-3][:200]), "hints": {}}
        r["seconds"] = round(time.time() - t0, 2)
        r["x"], r["y"], r["pi"] = reg["x"], reg["y"], reg["pi"]
        r["Fk"] = f_text(reg.get("F"))
        sys.stdout.write(json.dumps(r) + "\n")
        sys.stdout.flush()


_PROJ = {}                  # (side, code) -> (project, info), shared by the Workers of one process


def _lin(x):
    """x as ({hash: (coef, term)}, const) over +, - and constants."""
    if x.concrete:
        return {}, x.concrete_value
    if x.op == "__add__":
        terms, k = {}, 0
        for a in x.args:
            t, c = _lin(a)
            k += c
            for h, (co, term) in t.items():
                terms[h] = (terms.get(h, (0, term))[0] + co, term)
        return terms, k
    if x.op == "__sub__" and len(x.args) == 2:
        t1, c1 = _lin(x.args[0])
        t2, c2 = _lin(x.args[1])
        for h, (co, term) in t2.items():
            t1[h] = (t1.get(h, (0, term))[0] - co, term)
        return t1, c1 - c2
    return {x.hash(): (1, x)}, 0


def same(a, b):
    """structurally equal ASTs (angr 10's AST.identical() ignores variable names: never use it)"""
    return a is b or a.hash() == b.hash()


def const_diff(x, a):
    t1, c1 = _lin(x)
    t2, c2 = _lin(a)
    for h, (co, term) in t2.items():
        t1[h] = (t1.get(h, (0, term))[0] - co, term)
    if any(co % (1 << 32) for co, _ in t1.values()):
        return None
    return (c1 - c2) % (1 << 32)


FP_BOUNDARY = ("fpToIEEEBV", "fpToSBV", "fpToUBV", "fpEQ", "fpLT", "fpLEQ", "fpGT", "fpGEQ", "fpNE", "fpIsNaN",
               "fpIsInf")


class Worker:
    def __init__(self, job):
        import angr
        from angr import claripy as C
        import z3
        self.angr, self.C, self.z3 = angr, C, z3
        self.job = job
        self.level = job["level"]
        self.deadline = time.time() + job["timeout"]
        self.facts = job["facts"]
        self.hints = job.get("hints") or {}
        self.base_syms = {}          # name -> (epoch, addr AST)
        self.base_by_key = {}
        self.assume = []             # claripy constraints that hold for every input (ABI, call facts)
        self.IN = {r: C.BVS("in_" + r, 32, explicit_name=True) for r in GPR}
        self.IN["flags"] = C.BVS("in_flags", 32, explicit_name=True)
        self.IN["fpreg"] = C.BVS("in_fpreg", 512, explicit_name=True)
        for x in XMM:
            self.IN[x] = C.BVS("in_" + x, 128, explicit_name=True)
        esp = self.IN["esp"]
        self.assume += [C.UGE(esp, 0x10000), C.ULE(esp, 0x7fff0000), (esp & 3) == 0]
        self.PROBE = C.BVS("x_probe", 32, explicit_name=True)
        self.zmemo = {}
        self.zmemo_abs = {}
        self.abs_mode = False
        self.zarr = {}
        self.stats = {}
        self._proj = {}
        self.guessed = set()        # observables that exist only because a callee's convention is unknown
        self.covered = set()        # (M&B) offsets of ours' instructions executed on some explored path
        self.frame_model = False    # the per-object frame model (region mode only; set below)
        self.entry_F = None         # the start cut's frame correspondence (frame model stage 1)
        self.cur_side = None
        # region mode (tools/equiv/regions.py): start both versions at a cut pair, stop at the stops of each side
        self.region = job.get("region")
        self.cur_stops = set()
        if self.region:
            rg = self.region
            self.stops = {"retail": set(rg["stops_r"]), "ours": set(rg["stops_o"])}
            self.live_at = {"retail": {int(k, 16): set(v) for k, v in rg["live_r"].items()},
                            "ours": {int(k, 16): set(v) for k, v in rg["live_o"].items()}}
            self.cands = {"retail": set(rg.get("cands_r", ())), "ours": set(rg.get("cands_o", ()))}
            self.start = {"retail": rg["x"], "ours": rg["y"]}
            # the register correspondence pi of the start cut: ours' pi[r] holds the original's r; registers of
            # ours outside it are independent symbols
            self.pi = dict(zip(PI_REGS, rg["pi"]))
            self.IN2 = {r: C.BVS("jn_" + r, 32, explicit_name=True) for r in GPR + ["flags"]}
            # the stack is not inside the module image (Windows allocates it apart): without the whole-function
            # model's entry-pointer assumption, a push could otherwise overwrite a global
            lo, hi = job.get("image") or (0x400000, 0x400000)
            if hi > lo:
                self.assume.append(C.Or(C.ULE(esp + 0x100000, lo), C.UGE(esp, hi + 0x100000)))
            self.exits = set()
            self.x87_bad = False
            self._exit_pi = None
            self.region_dz = rg.get("dz") or {}
            self.nf_names = {"in_" + r for r in self.region_dz.get("nf", ())}
            # the per-object frame model (LAYERS.md): live frame slots per stop (offset intervals below the entry
            # esp, or "all"); the exit/call memory obligation compares only these, under a slot correspondence
            self.frame_model = bool(job.get("frame_model")) and bool(self.region_dz)
            self.frame_live = {"retail": _frame_live(job.get("frame_r")), "ours": _frame_live(job.get("frame_o"))}
            self.frame_dead = {"retail": {int(a, 16) for a in job.get("frame_dead_r") or ()},
                               "ours": {int(a, 16) for a in job.get("frame_dead_o") or ()}}
            # stage 1 of the frame model: the cut's frame correspondence F (None: the standard interface, ours'
            # frame at the same addresses as the original's), and the calls after which no frame address can have
            # escaped (regions._noesc_calls: their callee cannot reach the frame above its stack arguments)
            self.entry_F = tuple(tuple(t) for t in rg["F"]) if rg.get("F") is not None else None
            self.noesc = {"retail": {int(a, 16) for a in job.get("noesc_r") or ()},
                          "ours": {int(a, 16) for a in job.get("noesc_o") or ()}}
            self._exit_F = None
            if self.entry_F is not None and not self.frame_model:
                raise Unsupported("an explicit frame correspondence needs the frame model and a known cut depth")
            # the frame zone of the entry-pointer assumption: from 1 MB below the cut's esp to the return address
            # and the function's own parameters, depth bytes above it
            self.job = dict(self.job, argw=(self.region_dz["depth"] + (self.region_dz.get("w") or 0))
                            if self.region_dz else 0)

    # ------------------------------------------------------------------ memory
    def base_byte(self, epoch, addr):
        k = (epoch, addr.hash())
        s = self.base_by_key.get(k)
        if s is None:
            nm = "m%s_%d" % (epoch, len(self.base_syms))
            s = self.C.BVS(nm, 8, explicit_name=True)
            self.base_syms[nm] = (epoch, addr)
            self.base_by_key[k] = s
        return s

    def make_mem_plugin(self):
        W = self
        angr, C = self.angr, self.C

        class LogMem(angr.SimStatePlugin):
            SUPPORTS_CONCRETE_LOAD = True

            def __init__(self, log=(), epoch="0"):
                super().__init__()
                self.log = log
                self.epoch = epoch

            def concrete_load(self, addr, size, **kw):
                mem = self.state.project.loader.memory
                lo, hi = self.state.project.loader.min_addr, self.state.project.loader.max_addr + 1
                return mem.load(addr, min(size, hi - addr)) if lo <= addr < hi else b""

            @angr.SimStatePlugin.memo
            def copy(self, memo):
                return LogMem(self.log, self.epoch)

            def merge(self, *a, **k):
                return False

            def widen(self, *a, **k):
                return False

            @staticmethod
            def _bv(v, n=None):
                if isinstance(v, int):
                    return C.BVV(v, (n or 4) * 8)
                if isinstance(v, bytes):
                    return C.BVV(int.from_bytes(v, "big"), len(v) * 8)
                return v

            def store(self, addr, data, size=None, endness=None, condition=None, **kw):
                addr = self._bv(addr)
                noal = W.note_addr(self.state, addr)
                if isinstance(size, int) and isinstance(data, int):
                    data = C.BVV(data, size * 8)
                data = self._bv(data)
                if type(data).__name__.startswith("FP"):
                    data = C.fpToIEEEBV(data)
                n = data.size() // 8
                if isinstance(size, int) and size < n:
                    data = data[size * 8 - 1:0] if endness != "Iend_BE" else data[n * 8 - 1:(n - size) * 8]
                    n = size
                if condition is not None and not (condition.concrete and condition.is_true()):
                    data = C.If(condition, data, self.load(addr, n, endness=endness))
                bs = [data.get_byte(n - 1 - k) for k in range(n)]
                if endness == "Iend_BE":
                    bs = bs[::-1]
                self.log = self.log + (("w", addr, tuple(bs), noal),)

            def load(self, addr, size=None, endness=None, condition=None, fallback=None, **kw):
                addr = self._bv(addr)
                dz = W.note_addr(self.state, addr)
                n = size if isinstance(size, int) else size.concrete_value
                bs = [W.read_byte(addr + k, self.log, self.epoch, dz and n <= 16) for k in range(n)]
                v = C.Concat(*(bs[::-1] if endness != "Iend_BE" else bs)) if n > 1 else bs[0]
                if condition is not None and fallback is not None:
                    v = C.If(condition, v, fallback)
                return v

        return LogMem

    def note_addr(self, st, addr):
        """dead-stack assumption: an address computed from the entry state alone (inputs other than esp, memory
        at entry, constants) never points into the unallocated stack below the entry esp (the compiler assumes the
        same when it moves pushes across loads). Kept per path, as part of its condition."""
        if st is None:
            return False
        vs = addr.variables
        if "in_esp" in vs:
            return False
        if self.region:
            # region mode: only what the static provenance pass proved at the cut (regions._cut_dz): registers
            # that hold no frame address, memory read at the cut when no frame address had escaped, constants
            if not self.region_dz or not all(self.dz_trusted(v) for v in vs):
                return False
        elif not all(v.startswith(("in_", "m0_")) for v in vs):
            # (M&B) also an address computed from callee results or memory read after calls, while no address
            # of the frame has escaped on this path: no callee was given one, so none can hand one back (a
            # callee's own dead frame or parameters are not valid objects after it returns)
            if st.globals.get("escaped", True) or not all(v.startswith(("in_", "m", "c")) and
                                                          not v.startswith(("mg", "ra", "rn")) for v in vs):
                return False
        C, e = self.C, self.IN["esp"]
        # (region mode: W includes the depth of the cut below the entry esp, so the zone is the whole frame)
        # not in [esp - 1 MB, esp + 4 + W), modulo 2^32; (M&B) W = the function's own stack argument bytes when
        # its decorated name gives them: its parameters (and return address) are its own objects, which no pointer
        # that existed before the call can point to (C++ object lifetime; the compiler relies on it when it keeps a
        # by-value argument in a register across stores through `this`, 0x43dcb0)
        W = self.job.get("argw") or 0
        c = C.UGE(e + (3 + W) - addr, 0x100000 + 4 + W)
        st.globals["dz"] = st.globals.get("dz", ()) + (c,)
        return True

    def dz_trusted(self, v):
        memo = self.__dict__.setdefault("_dzt", {})
        if v not in memo:
            memo[v] = False
            if v in self.nf_names:
                memo[v] = True
            elif v.startswith("m0_") and self.region_dz.get("noesc") and v in self.base_syms:
                memo[v] = all(self.dz_trusted(w) for w in self.base_syms[v][1].variables if w != "in_esp") and \
                    "in_esp" not in self.base_syms[v][1].variables
        return memo[v]

    def read_byte(self, x, log, epoch="0", dz=False):
        """dz (M&B addition): x = base + j (j < 16) where the path assumes base not in [esp - 1 MB, esp) (note_addr).
        Then a write at in_esp - k (16 <= k + j, k <= 64 KB - 16, i.e. the function's own pushes and locals) cannot
        alias x and is skipped instead of becoming an If on the symbolic offset: esp >= 0x10000 keeps the write
        range [esp - k, esp - k + n) from wrapping, and it lies inside [esp - 1 MB, esp). The If chains made Z3
        bit-blast float conversions of memory read at constant addresses (0x51db70 timed out on it)."""
        C = self.C
        if self.frame_model:
            cur = self.frame_base(x, epoch, dz, bool(log) and log[0][0] == "fmap")
        else:
            cur = self.base_byte(epoch, x)
        base0 = cur
        inesp = self.IN["esp"]
        # (M&B) the mirror image: x is in the function's own stack (in_esp + dx, dx in [-(64 KB - 16), 3 + W]) and
        # the write address is computed from the entry state alone; the store noted the no-alias assumption for it
        # (note_addr: not in [esp - 1 MB, esp + 4 + W)), so a write of n <= 16 bytes there cannot cover x
        dx = const_diff(x, inesp)
        W = self.job.get("argw") or 0
        x_own = dx is not None and (dx >= (1 << 32) - (0x10000 - 16) or dx <= 3 + W)
        x_img = self.in_image(x)
        for ent in log:
            if ent[0] == "w":
                a, bs = ent[1], ent[2]
                d = const_diff(x, a)
                if d is not None:
                    if d < len(bs):
                        cur = bs[d]
                    continue
                if x_own and len(bs) <= 16 and len(ent) > 3 and ent[3]:
                    continue
                if x_img and len(bs) <= 16 and self.near_esp(a):
                    continue
                if dz:
                    de = const_diff(a, inesp)                 # a = in_esp + de (mod 2^32)
                    if de is not None and de >= (1 << 32) - (0x10000 - 16) and de + len(bs) <= (1 << 32):
                        continue
                    if de is not None and self.region:
                        # region mode: the zone note_addr assumes for the read is [esp - 1 MB, esp + 4 + W) with W
                        # = the cut's depth + the own parameters, i.e. the whole frame above the cut's esp too; a
                        # write inside [esp - 1 MB + 16, esp + 4 + W) cannot cover base + j (j < 16): base would be
                        # in the zone (modulo 2^32, the same interval argument)
                        sd = de - (1 << 32) if de >= 0x80000000 else de
                        if -(0x100000 - 16) <= sd and sd + len(bs) <= 4 + W:
                            continue
                off = x - a
                sel = cur
                for k in reversed(range(len(bs))):
                    sel = C.If(off == k, bs[k], sel)
                cur = sel if len(bs) == 1 else C.If(C.ULT(off, len(bs)), sel, cur)
            elif ent[0] == "fmap":              # (frame model) ours' log under an explicit F: see frame_base
                continue
            else:                               # ("havoc", epoch, protected ranges)
                _, ep, prot = ent
                inside = []
                decided_in = False
                for lo, hi in prot:
                    d, w = const_diff(x, lo), const_diff(hi, lo)
                    if d is not None and w is not None:
                        if d < w:
                            decided_in = True
                            break
                        continue
                    if dz and self.own_range(lo, hi):
                        continue           # (M&B) x is noted not to point into the function's own stack
                    if x_img and self.near_esp(lo) and self.near_esp(hi):
                        continue           # region mode: a global is not in the stack (in_image)
                    inside.append(C.ULT(x - lo, hi - lo))
                if decided_in:
                    continue
                fresh = self.base_byte(ep, x)
                cur = C.If(C.Or(*inside), cur, fresh) if inside else fresh
        if self.region and epoch == "0" and cur is base0:
            # region mode: memory below the cut's esp is not part of the interface (the regions only compare memory
            # at and above esp), so a region must not read it before writing it
            dx = const_diff(x, inesp)
            if dx is not None and dx >= (1 << 32) - 0x100000:
                raise Unsupported("a region reads the stack below the cut's esp (esp%+d)" % (dx - (1 << 32)))
        return cur

    def frame_base(self, x, epoch, dz, ours):
        """(frame model) the cut's memory at x before the region writes it. A frame address (in_esp + c, c in
        [0, depth): at or above the cut's esp, below the entry esp E) gets a canonical key, so the two versions'
        reads of one slot are one term. ours (its log starts with the ("fmap",) marker: the cut has an explicit F):
        R relates ours' frame to the original's only through F, so ours' byte at E - o (inside an F slot
        (r_lo, o_lo, n)) is the original's byte at E - r_lo + (o_lo - o), and every other byte of ours' frame is a
        symbol of its own (the "0o" epoch: unrelated to anything of the original). A read of ours through an
        address that is neither esp-relative nor noted outside the frame (dz) may hit the remapped frame: refused.
        The comparison probe is exempt: every comparison restricts it to addresses outside the frame."""
        inesp = self.IN["esp"]
        d = const_diff(x, inesp)
        if d is None:
            if ours and self.entry_F is not None and not dz and not same(x, self.PROBE):
                raise Unsupported("a read of ours through a pointer that may point into its remapped frame "
                                  "(frame correspondence)")
            return self.base_byte(epoch, x)
        sd = d - (1 << 32) if d >= 0x80000000 else d
        depth = self.region_dz.get("depth")
        if ours and self.entry_F is not None and depth is not None and 0 <= sd < depth:
            off = depth - sd
            for r_lo, o_lo, n in self.entry_F:
                if o_lo - n < off <= o_lo:
                    sd = depth - r_lo + (o_lo - off)
                    break
            else:
                return self.base_byte(epoch + "o", inesp + self.C.BVV(sd & 0xffffffff, 32))
        return self.base_byte(epoch, inesp + self.C.BVV(sd & 0xffffffff, 32))

    def in_image(self, x):
        """region mode: x is a constant address inside the module image. The region model assumes the stack is
        not within 1 MB of the image (Worker.__init__: esp + 1 MB <= image lo or esp >= image hi + 1 MB, with
        0x10000 <= esp <= 0x7fff0000), so no address in_esp + d with |d| < 1 MB - 16 (near_esp) reaches it: a write
        of at most 16 bytes there cannot cover x (a wrapped one lies above 2^32 - 1 MB, above the image). Skipping
        such writes keeps reads of globals free of If chains over the frame stores, which made the path conditions
        of the two versions (frame slots at different offsets) differ structurally and Z3 time out on them."""
        if not self.region or not x.concrete:
            return False
        lo, hi = self.job.get("image") or (0, 0)
        return lo < hi <= (1 << 32) - 0x200000 and lo <= x.concrete_value < hi

    def near_esp(self, a):
        d = const_diff(a, self.IN["esp"])
        if d is None:
            return False
        sd = d - (1 << 32) if d >= 0x80000000 else d
        return -(0x100000 - 16) < sd < 0x100000 - 16

    def own_range(self, lo, hi):
        """[lo, hi) inside [in_esp - (64 KB - 16), in_esp + 4 + W): the zone note_addr's assumption excludes"""
        inesp = self.IN["esp"]
        W = self.job.get("argw") or 0
        dl, dh = const_diff(lo, inesp), const_diff(hi, inesp)
        if dl is None or dh is None:
            return False
        sl = dl - (1 << 32) if dl >= 0x80000000 else dl
        sh = dh - (1 << 32) if dh >= 0x80000000 else dh
        return -(0x10000 - 16) <= sl <= sh <= 4 + W

    def entry_only(self, a):
        """is the address computed from the entry state alone (note_addr's condition for the assumption)?"""
        memo = self.__dict__.setdefault("_eo", {})
        h = a.hash()
        if h not in memo:
            vs = a.variables
            memo[h] = "in_esp" not in vs and all(v.startswith(("in_", "m0_")) for v in vs)
        return memo[h]

    def read_word(self, x, log, n=4):
        bs = [self.read_byte(x + k, log) for k in range(n)]
        return self.C.Concat(*bs[::-1])

    # ------------------------------------------------------------------ z3
    def z(self, e, subst=None):
        """claripy AST -> z3 (base memory bytes become Select on per-epoch arrays)."""
        z3 = self.z3
        absm = getattr(self, "abs_mode", False)
        memo = (self.zmemo_abs if absm else self.zmemo) if subst is None else {}
        stack = [e]
        while stack:
            x = stack[-1]
            h = x.hash()
            if h in memo:
                stack.pop()
                continue
            if absm and x.op in FP_BOUNDARY:
                # (M&B) abstraction: a float term seen from the integer world is a free variable per structure
                stack.pop()
                tag = ("%x" % h).replace("-", "n")         # the whole structural hash: no two terms share a name
                memo[h] = z3.Bool("fpb_" + tag) if x.op.startswith(("fpEQ", "fpLT", "fpLEQ", "fpGT", "fpGEQ",
                                                                    "fpNE", "fpIs")) \
                    else z3.BitVec("fpa_%s_%d" % (tag, x.size()), x.size())
                continue
            kids = [a for a in x.args if hasattr(a, "op") and hasattr(a, "hash")]
            if x.op == "BVS" and x.args[0] in self.base_syms:
                kids = [self.base_syms[x.args[0]][1]]
            elif x.op == "BVS" and subst and x.args[0] in subst:
                kids = [subst[x.args[0]]]
            pend = [k for k in kids if k.hash() not in memo]
            if pend:
                stack.extend(pend)
                continue
            stack.pop()
            memo[h] = self._z1(x, memo, subst)
        return memo[e.hash()]

    def _arr(self, epoch):
        z3 = self.z3
        if epoch not in self.zarr:
            self.zarr[epoch] = z3.Array("M" + str(epoch), z3.BitVecSort(32), z3.BitVecSort(8))
        return self.zarr[epoch]

    def _z1(self, x, memo, subst):
        z3 = self.z3
        op, args = x.op, x.args
        A = lambda k: memo[args[k].hash()]
        if op == "BVS":
            nm = args[0]
            if nm in self.base_syms:
                ep, addr = self.base_syms[nm]
                return z3.Select(self._arr(ep), memo[addr.hash()])
            if subst and nm in subst:
                return memo[subst[nm].hash()]
            return z3.BitVec(nm, x.size())
        if op == "BVV":
            return z3.BitVecVal(args[0], args[1])
        if op == "BoolV":
            return z3.BoolVal(args[0])
        if op == "BoolS":
            return z3.Bool(args[0])
        n = [memo[a.hash()] if hasattr(a, "hash") and a.hash() in memo else a for a in args]
        if op == "__add__":
            r = n[0]
            for v in n[1:]:
                r = r + v
            return r
        if op == "__sub__":
            return n[0] - n[1]
        if op == "__mul__":
            r = n[0]
            for v in n[1:]:
                r = r * v
            return r
        if op in ("__and__", "__or__", "__xor__"):
            f = {"__and__": lambda a, b: a & b, "__or__": lambda a, b: a | b, "__xor__": lambda a, b: a ^ b}[op]
            r = n[0]
            for v in n[1:]:
                r = f(r, v)
            return r
        simple = {
            "__invert__": lambda a: ~a, "__neg__": lambda a: -a,
            "__lshift__": lambda a, b: a << b, "__rshift__": lambda a, b: a >> b, "LShR": z3.LShR,
            "__floordiv__": z3.UDiv, "__mod__": z3.URem, "SDiv": lambda a, b: a / b, "SMod": z3.SRem,
            "__eq__": lambda a, b: a == b, "__ne__": lambda a, b: a != b,
            "ULT": z3.ULT, "ULE": z3.ULE, "UGT": z3.UGT, "UGE": z3.UGE,
            "__lt__": z3.ULT, "__le__": z3.ULE, "__gt__": z3.UGT, "__ge__": z3.UGE,
            "SLT": lambda a, b: a < b, "SLE": lambda a, b: a <= b, "SGT": lambda a, b: a > b,
            "SGE": lambda a, b: a >= b, "RotateLeft": z3.RotateLeft, "RotateRight": z3.RotateRight,
            "Not": z3.Not, "If": z3.If,
        }
        if op in simple:
            return simple[op](*n)
        if op == "And":
            return z3.And(*n)
        if op == "Or":
            return z3.Or(*n)
        if op == "Xor":
            return z3.Xor(*n)
        if op == "Concat":
            return z3.Concat(*n) if len(n) > 1 else n[0]
        if op == "Extract":
            return z3.Extract(n[0], n[1], n[2])
        if op == "ZeroExt":
            return z3.ZeroExt(n[0], n[1])
        if op == "SignExt":
            return z3.SignExt(n[0], n[1])
        if op == "Reverse":
            v = n[0]
            sz = v.size()
            return z3.Concat(*[z3.Extract(8 * k + 7, 8 * k, v) for k in range(sz // 8)]) if sz > 8 else v
        return self._zfp(x, op, n)

    def _zsort(self, s):
        z3 = self.z3
        nm = str(s)
        if "32" in nm or "FLOAT" in nm.upper() and "DOUBLE" not in nm.upper():
            return z3.Float32()
        return z3.Float64()

    def _zrm(self, rm):
        z3 = self.z3
        s = str(rm)
        for k, v in (("NearestTiesEven", z3.RNE()), ("NearestTiesAway", z3.RNA()), ("TowardsZero", z3.RTZ()),
                     ("TowardsPositive", z3.RTP()), ("TowardsNegative", z3.RTN())):
            if k in s:
                return v
        raise Unsupported("rounding mode %s" % s)

    def _hidden_rm(self, x):
        """the rounding mode of a conversion (angr 10 keeps it out of .args; smtlib() shows it)"""
        C = self.C
        k = x.args[0]
        leaf = C.FPS("_d", C.FSORT_FLOAT if k.size() == 32 else C.FSORT_DOUBLE) if type(k).__name__ == "FP" \
            else C.BVS("_d", k.size())
        txt = x.replace(k, leaf).smtlib()
        m = re.search(r"\b(RNE|RNA|RTZ|RTP|RTN)\b", txt)
        if not m:
            raise Unsupported("rounding mode of %s" % x.op)
        return {"RNE": self.z3.RNE, "RNA": self.z3.RNA, "RTZ": self.z3.RTZ, "RTP": self.z3.RTP,
                "RTN": self.z3.RTN}[m.group(1)]()

    def _zfp(self, x, op, n):
        z3 = self.z3
        isrm = lambda v: "RM" in type(v).__name__ or "RM_" in str(v)
        if op == "FPS":
            return z3.FP(n[0], self._zsort(n[1]))
        if op == "FPV":
            # angr 10: FPV(value,) with the sort only in the size (args[1] in older claripy)
            srt = self._zsort(n[1]) if len(n) > 1 else (z3.Float32() if x.size() == 32 else z3.Float64())
            return z3.FPVal(n[0], srt)
        if op in ("fpAdd", "fpSub", "fpMul", "fpDiv"):
            a, b, rm = (n[0], n[1], n[2]) if isrm(n[2]) else (n[1], n[2], n[0])
            return {"fpAdd": z3.fpAdd, "fpSub": z3.fpSub, "fpMul": z3.fpMul, "fpDiv": z3.fpDiv}[op](
                self._zrm(rm), a, b)
        if op == "fpSqrt":
            a, rm = (n[0], n[1]) if isrm(n[1]) else (n[1], n[0])
            return z3.fpSqrt(self._zrm(rm), a)
        if op in ("fpNeg", "fpAbs"):
            return {"fpNeg": z3.fpNeg, "fpAbs": z3.fpAbs}[op](n[0])
        if op in ("fpEQ", "fpLT", "fpLEQ", "fpGT", "fpGEQ", "fpNE"):
            return {"fpEQ": z3.fpEQ, "fpLT": z3.fpLT, "fpLEQ": z3.fpLEQ, "fpGT": z3.fpGT, "fpGEQ": z3.fpGEQ,
                    "fpNE": z3.fpNEQ}[op](n[0], n[1])
        if op in ("fpIsNaN", "fpIsInf"):
            return {"fpIsNaN": z3.fpIsNaN, "fpIsInf": z3.fpIsInf}[op](n[0])
        if op in ("fpMin", "fpMax"):
            return {"fpMin": z3.fpMin, "fpMax": z3.fpMax}[op](n[0], n[1])
        if op == "fpToIEEEBV":
            return z3.fpToIEEEBV(n[0])
        if op == "bvToFP":
            return z3.fpBVToFP(n[0], z3.Float32() if x.size() == 32 else z3.Float64())
        if op in ("fpToFp", "fpToFP", "fpToFPSigned", "fpToFPUnsigned", "fpToSBV", "fpToUBV"):
            rm = self._hidden_rm(x)
            v = n[0]
            if op in ("fpToSBV", "fpToUBV"):
                return (z3.fpToSBV if op == "fpToSBV" else z3.fpToUBV)(rm, v, z3.BitVecSort(x.size()))
            srt = z3.Float32() if x.size() == 32 else z3.Float64()
            if op == "fpToFPSigned":
                return z3.fpSignedToFP(rm, v, srt)
            if op == "fpToFPUnsigned":
                return z3.fpUnsignedToFP(rm, v, srt)
            return z3.fpFPToFP(rm, v, srt)
        if op == "fpFP":
            return z3.fpFP(n[0], n[1], n[2])
        raise Unsupported("claripy op %s" % op)

    def solver(self):
        s = self.z3.Solver()
        for a in self.assume:
            s.add(self.z(a))
        return s

    def check(self, s, *extra):
        rem = self.deadline - time.time()
        if rem < 0.5:
            raise GiveUp("timeout (solver)")
        s.set("timeout", int(min(rem, 600, getattr(self, "qcap", None) or 600) * 1000))
        r = s.check(*extra)
        self.stats["queries"] = self.stats.get("queries", 0) + 1
        if r == self.z3.unknown:
            raise GiveUp("solver returned unknown (%s)" % s.reason_unknown())
        return r == self.z3.sat

    def feasible(self, st, cap=0.5):
        """False only when the path condition is unsatisfiable (Z3 within `cap` seconds); unknown -> True."""
        z3 = self.z3
        rem = self.deadline - time.time()
        if rem < 0.5:
            raise GiveUp("timeout (solver)")
        pc = self.pc(st)
        if self.has_fp(pc):
            # Z3 does not honour its timeout while bit-blasting floating point (80 s on 0x408bd0): keep the path
            self.stats["prune_fp_kept"] = self.stats.get("prune_fp_kept", 0) + 1
            return True
        q = self.solver()
        q.add(self.z(pc))
        q.set("timeout", int(min(cap, rem) * 1000))
        r = q.check()
        self.stats["prune_queries"] = self.stats.get("prune_queries", 0) + 1
        return r != z3.unsat

    def has_fp(self, e):
        """does the claripy AST contain a floating-point term (memoized per node)?"""
        memo = self.__dict__.setdefault("_fpmemo", {})
        seen, stack = set(), [e]
        while stack:
            x = stack.pop()
            h = x.hash()
            if h in seen:
                continue
            seen.add(h)
            v = memo.get(h)
            if v is True:
                memo[e.hash()] = True
                return True
            if v is False:
                continue
            if x.op.startswith("fp") or x.op in ("FPS", "FPV", "bvToFP"):
                memo[h] = memo[e.hash()] = True
                return True
            stack.extend(a for a in x.args if hasattr(a, "op") and hasattr(a, "hash"))
            if x.op == "BVS" and x.args[0] in self.base_syms:
                stack.append(self.base_syms[x.args[0]][1])
        for h in seen:
            memo[h] = False
        return False

    def pc(self, st):
        cs = list(st.solver.constraints) + list(st.globals.get("dz", ()))
        return self.C.And(*cs) if cs else self.C.true()

    # ------------------------------------------------------------------ exploration
    def project(self, side):
        if side in self._proj:
            return self._proj[side]
        sd = self.job["sides"][side]
        key = (side, sd["code"], sd["base"])
        if key in _PROJ:
            self._proj[side] = _PROJ[key]
            return _PROJ[key]
        code = bytes.fromhex(sd["code"])
        base = sd["base"]
        proj = self.angr.load_shellcode(code, arch="x86", start_offset=0, load_address=base)
        info = classify(code, base)
        info["code"], info["base"], info["end"] = code, base, base + len(code)
        info["by"] = {i.address: i for i in info["ins"]}
        self._proj[side] = (proj, info)
        if self.region:
            # (a persistent region worker serves one proof and its mutants: keep the latest few projects)
            while len(_PROJ) >= 4:
                _PROJ.pop(next(iter(_PROJ)))
            _PROJ[key] = (proj, info)
        return proj, info

    def entry_state(self, proj, info, side=None):
        angr, C = self.angr, self.C
        rg = self.region if side else None
        st = proj.factory.blank_state(addr=self.start[side] if rg else info["base"],
                                      remove_options={angr.options.UNICORN} | angr.options.simplification)
        st.register_plugin("memory", self.make_mem_plugin()())
        if rg and side == "ours" and self.entry_F is not None:
            st.memory.log = (("fmap",),)
        if rg and side == "ours":
            # pi[r] = "v" (ours' v holds the original's r) or "v:8" / "v:16" (only the low bits are related)
            inv = {v.split(":")[0]: (r, int(v.split(":")[1]) if ":" in v else 32) for r, v in self.pi.items() if v}

            def own(r):
                if r == "esp":
                    return self.IN[r]
                if r not in inv:
                    return self.IN2[r]
                o, w = inv[r]
                if w == 32 or r == "flags":
                    return self.IN[o]
                return C.Concat(self.IN2[r][31:w], self.IN[o][w - 1:0])
        else:
            own = lambda r: self.IN[r]
        for r in GPR:
            setattr(st.regs, r, own(r))
        st.regs.cc_op, st.regs.cc_dep1, st.regs.cc_dep2, st.regs.cc_ndep = 0, own("flags"), 0, 0
        st.regs.d = 1
        if rg and rg["d"]:
            # region mode: the cut pair's x87 stack holds d values (shared: R(x, y) says they are equal)
            d = rg["d"]
            ftop = (-d) & 7
            st.regs.ftop = ftop
            st.regs.fptag = sum(1 << (8 * ((ftop + k) & 7)) for k in range(d))
        else:
            st.regs.ftop = 0
            st.regs.fptag = 0
        st.regs.fpreg = self.IN["fpreg"]
        st.regs.fpround = 0
        st.regs.sseround = 0
        for x in XMM:
            setattr(st.regs, x, self.IN[x])
        # M&B: angr's own satisfiability check of every fork is skipped (LAZY_SOLVES): on x87 code it is a query
        # in floating-point theory that took 26 s per fork on Mesh transform (0x408bd0). Forks are pruned by
        # feasible() instead, with a short cap (an unknown answer keeps the path, which is sound: an extra path is
        # one more obligation, and the comparison checks every path condition again).
        st.options.add(angr.options.LAZY_SOLVES)
        st.globals["events"] = ()
        st.globals["br"] = ""
        st.globals["visits"] = ()
        st.globals["escaped"] = bool(rg)
        if rg:
            st.globals["start"] = True
            st.globals["x87"] = ()
            st.globals["cands"] = ()
        return st

    def snapshot(self, s):
        g = {r: getattr(s.regs, r) for r in GPR}
        g["eflags"] = s.regs.eflags
        g["d"] = s.regs.d
        g["ftop"] = s.regs.ftop
        g["fptag"] = s.regs.fptag
        g["fpreg"] = s.regs.fpreg
        for x in XMM:
            g[x] = getattr(s.regs, x)
        return g

    def stop_len(self, info, a):
        """instructions to execute from a before reaching a special instruction or a loop head"""
        n, x = 0, a
        while True:
            i = info["by"].get(x)
            if i is None:
                return max(n, 1)
            if n and (x in info["kinds"] or x in info["heads"] or x in self.cur_stops):
                return n
            n += 1
            if x in info["kinds"]:
                return n
            m = i.mnemonic
            if m.startswith("j") or m.startswith("loop") or m in ("ret", "call"):
                return n
            x = i.address + i.size

    def last_ins(self, info, a, k):
        x, i = a, None
        for _ in range(k):
            i = info["by"].get(x)
            if i is None:
                return None
            m = i.mnemonic
            if m.startswith("j") or m.startswith("loop"):
                return i
            x = i.address + i.size
        return i

    def callee(self, info, i):
        """(target AST, facts) of a call/jmp instruction"""
        C = self.C
        t = _imm_target(i)
        if t is not None:
            return C.BVV(t, 32), dict(self.facts.get("%x" % t, {})), "%x" % t
        return None, None, None

    def operand_target(self, s, i):
        """evaluate the register/memory operand of an indirect call/jmp"""
        from capstone import x86_const as X
        op = i.operands[0]
        if op.type == X.X86_OP_REG:
            return getattr(s.regs, i.reg_name(op.reg))
        if op.type == X.X86_OP_MEM:
            m = op.mem
            if m.segment != 0:
                raise Unsupported("segment-relative call at %x" % i.address)
            a = self.C.BVV(m.disp & 0xffffffff, 32)
            if m.base:
                a = a + getattr(s.regs, i.reg_name(m.base))
            if m.index:
                a = a + getattr(s.regs, i.reg_name(m.index)) * m.scale
            return s.memory.load(a, 4, endness="Iend_LE")
        raise Unsupported("call operand at %x" % i.address)

    def private_ranges(self, s, esp_ev, W):
        """the caller's frame a callee may not touch: [esp+4+W, in_esp+4) when W is known and nothing escaped;
        else the saved-register slots and the return address."""
        C, inesp = self.C, self.IN["esp"]
        saved = []
        for ent in s.memory.log:
            if ent[0] != "w":
                continue
            d = const_diff(ent[1], inesp)
            if d is None or not (0xffff0000 <= d):
                continue
            a = ent[1]
            if any(const_diff(a, lo) == 0 for lo, _ in saved):
                continue
            bs = [self.read_byte(a + k, s.memory.log) for k in range(4)]
            for r in CALLEE_SAVED:
                want = [self.IN[r][8 * k + 7:8 * k] for k in range(4)]
                if all(same(b, w) for b, w in zip(bs, want)):
                    saved.append((a, a + 4))
                    break
        if self.region:
            # region mode: only the callee-saved registers this region itself saved are protected (the whole-
            # function model's trust: callees do not write their caller's saved registers); the rest of the
            # frame may have escaped
            return saved, saved
        saved.append((inesp, inesp + 4))
        # (M&B) the function's own stack parameters [in_esp + 4, in_esp + 4 + argw) are its own objects too: cl
        # spills into a dead parameter slot (0x42c350 keeps a char in its tstring* parameter's slot across a call)
        own = self.job.get("argw") or 0
        if W is not None and not s.globals["escaped"]:
            return [(esp_ev + 4 + W, inesp + 4 + own)] + saved, saved
        if own and not s.globals["escaped"]:
            return saved + [(inesp + 4, inesp + 4 + own)], saved
        return saved, saved

    def escapes(self, s, regs_visible, esp_ev, W):
        """does a frame address become visible to a callee (argument registers, memory outside the frame)?"""
        if s.globals["escaped"]:
            return True
        name = "in_esp"
        for r in regs_visible:
            if name in getattr(s.regs, r).variables:
                return True
        inesp = self.IN["esp"]
        for ent in s.memory.log:
            if ent[0] != "w":
                continue
            if not any(name in b.variables for b in ent[2]):
                continue
            d = const_diff(ent[1], inesp)
            if d is None:
                return True
            if W is None:
                return True
            de = const_diff(ent[1], esp_ev + 4 + W)
            if de is None or de >= 0x80000000:          # below the private frame: an argument or callee area
                return True
        return False

    def arg_bytes(self, info, i, f):
        """(frame model) the stack argument bytes of a call from evidence in the code, not from a name (a decomp's
        declaration may be wrong): a callee-pops callee's own `ret N` (N > 0, read from the original's code), else
        the cleanup right after this call site (`add esp, N`, or pops of N/4 registers) when the callee pops
        nothing. The cleanup right after a call removes at least that call's arguments (they are the top of the
        stack); more (a merged cleanup) only protects less. Each version's call site gives its own count; the
        call obligation requires the two equal (the original's is the evidence). None: unknown."""
        from capstone import x86_const as X
        r = f.get("retn")
        if f.get("retn_code") and isinstance(r, int) and r > 0:
            return r
        if r != 0:
            return None
        if f.get("retn_code") and f.get("conv") in ("thiscall", "stdcall", "fastcall"):
            # a callee-pops convention whose own code pops nothing: no stack arguments (a decomp that declared a
            # cdecl callee this way would push nothing where the original pushes: esp differs at the call)
            return 0
        nx = info["by"].get(i.address + i.size)
        if nx is None:
            return None
        ops = nx.operands
        if nx.mnemonic == "add" and len(ops) == 2 and ops[0].type == X.X86_OP_REG and \
                nx.reg_name(ops[0].reg) == "esp" and ops[1].type == X.X86_OP_IMM and 0 < ops[1].imm < 0x10000:
            return ops[1].imm
        n, a = 0, i.address + i.size
        while n < 4:
            p = info["by"].get(a)
            if p is None or p.mnemonic != "pop" or not p.operands or p.operands[0].type != X.X86_OP_REG or \
                    p.operands[0].size != 4 or p.reg_name(p.operands[0].reg) == "esp":
                break
            n += 1
            a = p.address + p.size
        return 4 * n if n else None

    def do_call(self, s, i, info, tail=False):
        """the call event and the callee's effect; returns the successor state or None (path ends)"""
        C = self.C
        L2 = self.level == "L2"
        k = len(s.globals["events"])
        tgt, f, key = self.callee(info, i)
        if tgt is None:
            tgt = self.operand_target(s, i)
            mm = re.fullmatch(r"dword ptr \[(0x[0-9a-f]+)\]", i.op_str)
            key = ("m%x" % int(mm.group(1), 16)) if mm else None
            f = dict(self.facts.get(key, {})) if key else {}
            if not mm and "reads" not in f:
                f["reads"], f["guess"] = ["ecx", "edx"], True    # thiscall or fastcall at most
        esp0 = s.regs.esp
        if tail:
            esp_ev = esp0
        else:
            esp_ev = esp0 - 4
            s.memory.store(esp_ev, C.BVS("ra%d" % k, 32, explicit_name=True), endness="Iend_LE")
            s.regs.esp = esp_ev
        retn = f.get("retn")
        if retn is None:
            rn = C.BVS("rn%d" % k, 32, explicit_name=True)
            self.assume += [C.ULE(rn, 64), (rn & 3) == 0]
            retn = rn
        W = f.get("W")
        reads = f.get("reads")
        if reads is None:
            reads, f["guess"] = ["eax", "ecx", "edx"], True
        visible = [r for r in GPR if r != "esp"] if L2 else list(reads)
        esc = self.escapes(s, visible, esp_ev, W)
        if esc:
            s.globals["escaped"] = True
        prot, saved = self.private_ranges(s, esp_ev, W)
        pframe = False
        wp = self.arg_bytes(info, i, f)
        noesc = getattr(self, "noesc", None) or {}
        pwhy = "a frame address may have escaped" if i.address not in noesc.get(self.cur_side, ()) else \
            ("no code evidence of the argument bytes of %s" % (f.get("name") or "?")[:40]) if wp is None else \
            "the frame bounds"
        if self.region and self.frame_model and not tail and wp is not None and \
                i.address in noesc.get(self.cur_side, ()):
            # (frame model) no frame address can have escaped before this call (the static provenance pass, on
            # every path from the entry): the callee reaches its stack arguments [esp+4, esp+4+W) and nothing of
            # the frame above them (the ABI trust the whole-function model applies when nothing escaped). Its
            # havoc leaves [esp+4+W, E) alone, so each version keeps its own frame across the call, and the call
            # obligation compares only the arguments there (the frame is related again at the next stop, F')
            top = self.IN["esp"] + self.region_dz["depth"]
            w = const_diff(top, esp_ev + 4 + wp)
            if w is not None and w < 0x100000:
                prot = list(prot) + [(esp_ev + 4 + wp, top)]
                pframe = True
        ev = dict(kind="call", target=tgt, key=key or "", regs=self.snapshot(s), esp=esp_ev, log=s.memory.log,
                  reads=reads, W=W, prot=prot, escaped=esc, name=f.get("name", ""), at=i.address,
                  guess=reads if f.get("guess") else [], pframe=pframe, wp=wp, pwhy=None if pframe else pwhy)
        s.globals["events"] = s.globals["events"] + (ev,)
        if f.get("noreturn"):
            ev2 = dict(kind="noreturn")
            s.globals["events"] = s.globals["events"] + (ev2,)
            return None
        # effects
        fresh = lambda nm, bits: C.BVS("c%d_%s" % (k, nm), bits, explicit_name=True)
        kills = [r for r in GPR if r != "esp"] if L2 else ["eax", "ecx", "edx"]
        if f.get("chkstk"):
            kills = ["eax"]
        for r in kills:
            setattr(s.regs, r, fresh(r, 32))
        s.regs.cc_op, s.regs.cc_dep1, s.regs.cc_dep2, s.regs.cc_ndep = 0, fresh("flags", 32), 0, 0
        for x in XMM:
            setattr(s.regs, x, fresh(x, 128))
        fd = f.get("fdelta", -1 if f.get("fret") else 0)
        if fd:
            s.regs.ftop = s.regs.ftop + fd
            if fd < 0:
                s.regs.fptag = s.regs.fptag | (C.BVV(1, 64) << ((s.regs.ftop & 7).zero_extend(32) * 8))
            else:
                s.regs.fptag = s.regs.fptag & ~(C.BVV(0xff, 64) << (((s.regs.ftop - 1) & 7).zero_extend(32) * 8))
        s.regs.fpreg = fresh("fpreg", 512)
        s.memory.log = s.memory.log + (("havoc", str(k + 1), tuple(prot)),)
        if f.get("chkstk"):
            s.regs.esp = esp0 - ev["regs"]["eax"]
        else:
            s.regs.esp = esp_ev + 4 + retn
        if tail:
            return self.do_ret(s, None, after_tail=True)
        s.regs.ip = i.address + i.size
        return s

    def do_ret(self, s, i, after_tail=False):
        n = 0 if i is None else (int(i.op_str, 16) if i.op_str else 0)
        esp = s.regs.esp
        if after_tail:
            # the callee returned to our caller: its esp already includes the return
            ev = dict(kind="ret", regs=self.snapshot(s), esp_after=esp, target=None, log=s.memory.log)
        else:
            ev = dict(kind="ret", regs=self.snapshot(s), esp_after=esp + 4 + n, esp_at=esp,
                      target=s.memory.load(esp, 4, endness="Iend_LE"), log=s.memory.log)
        s.globals["events"] = s.globals["events"] + (ev,)
        return None

    def explore(self, side, bound, gen=None):
        """-> (finished states, dropped states at loop heads)"""
        proj, info = self.project(side)
        self.info = getattr(self, "info", {})
        self.info[side] = info
        st = self.entry_state(proj, info, side if self.region else None)
        self.cur_stops = self.stops[side] if self.region else set()
        self.cur_side = side
        done, dropped, todo, steps = [], [], [st], 0
        maxpaths = 3000
        while todo:
            if time.time() > self.deadline:
                raise GiveUp("timeout (exploring %s)" % side)
            s = todo.pop()
            a = s.addr if not s.regs.ip.symbolic else None
            if a is None:
                raise Unsupported("symbolic jump target in %s" % side)
            if not (info["base"] <= a < info["end"]) or a not in info["by"]:
                raise Unsupported("%s jumps to %x (outside the decoded function)" % (side, a))
            kind = info["kinds"].get(a)
            if self.region:
                if not s.globals["start"]:
                    if a in self.cur_stops:
                        s.globals["events"] = s.globals["events"] + (dict(
                            kind="exit", at=a, regs=self.snapshot(s), esp=s.regs.esp, log=s.memory.log),)
                        done.append(s)
                        continue
                    if a in self.cands[side]:
                        # a candidate cut: a leader where the other version may stop (its blocks differ)
                        ev = dict(kind="exit", at=a, regs=self.snapshot(s), esp=s.regs.esp, log=s.memory.log)
                        s.globals["cands"] = s.globals["cands"] + ((s.globals["events"] + (ev,),
                                                                    s.globals["x87"]),)
                s.globals["start"] = False
            if a in info["heads"]:
                vis = s.globals["visits"]
                n = sum(1 for x in vis if x == a) + 1
                s.globals["visits"] = vis + (a,)
                b = bound.get(a, MAX_BOUND) if isinstance(bound, dict) else bound
                if n > b:
                    dropped.append(s)
                    continue
            i = info["by"][a]
            if side == "ours" and kind:
                self.covered.add(a - info["base"])
            if kind == "call":
                t = _imm_target(i)
                if t is not None and info["base"] <= t < info["end"]:
                    raise Unsupported("call into the function itself at %x" % a)
                r = self.do_call(s, i, info)
                if r is None:
                    done.append(s)
                else:
                    todo.append(r)
                continue
            if kind == "ret":
                self.do_ret(s, i)
                done.append(s)
                continue
            if kind == "tail":
                self.do_call(s, i, info, tail=True)
                done.append(s)
                continue
            if kind == "ijmp":
                raise Unsupported("indirect jump at +%x in %s (switch table?)" % (a - info["base"], side))
            if kind == "tailcc":
                raise Unsupported("conditional tail jump at +%x in %s" % (a - info["base"], side))
            if kind == "trap":
                s.globals["events"] = s.globals["events"] + (dict(kind="trap"),)
                done.append(s)
                continue
            k = self.stop_len(info, a)
            if side == "ours":
                x = a
                for _ in range(k):
                    ii = info["by"].get(x)
                    if ii is None:
                        break
                    self.covered.add(x - info["base"])
                    x = ii.address + ii.size
            try:
                succ = proj.factory.successors(s, num_inst=k)
            except Unsupported as e:
                raise Unsupported("+%x in %s: %s" % (a - info["base"], side, e))     # the memory model's own refusal
            except Exception as e:
                raise Unsupported("angr cannot step +%x in %s: %s" % (a - info["base"], side, type(e).__name__))
            steps += 1
            if succ.unconstrained_successors:
                raise Unsupported("unconstrained successor at +%x in %s" % (a - info["base"], side))
            fl = succ.flat_successors
            if self.region:
                sig, x = [], a
                for _ in range(k):
                    ii = info["by"].get(x)
                    if ii is None:
                        break
                    if ii.mnemonic.startswith("f") and ii.mnemonic not in ("fwait",):
                        sz = re.search(r"(byte|word|dword|qword|tbyte|xword) ptr", ii.op_str)
                        sig.append(ii.mnemonic + (":" + sz.group(1) if sz else ""))
                    x = ii.address + ii.size
                if sig:
                    for t in fl:
                        t.globals["x87"] = t.globals["x87"] + tuple(sig)
            last = self.last_ins(info, a, k)
            fall = last.address + last.size if last is not None else None
            for t in fl:
                jk = t.history.jumpkind
                if jk in ("Ijk_SigSEGV", "Ijk_MapFail"):
                    # a fault exit VEX adds to an instruction: movaps/movdqa on an address that is not 16-aligned
                    # (SIGSEGV), a segment translation that fails. With LAZY_SOLVES angr keeps it whatever its
                    # guard; an infeasible one is dropped, a feasible one ends the path in a TRAP event (the
                    # process faults), compared like int3/ud2: the other version must fault on the same inputs
                    if len(fl) > 1:
                        t.globals["br"] = t.globals["br"] + ("0" if t.addr == fall else "1")
                    if not self.feasible(t):
                        continue
                    t.globals["events"] = t.globals["events"] + (dict(kind="trap"),)
                    done.append(t)
                    continue
                if jk not in ("Ijk_Boring",):
                    raise Unsupported("jumpkind %s at +%x in %s" % (jk, a - info["base"], side))
                if len(fl) > 1:
                    t.globals["br"] = t.globals["br"] + ("0" if t.addr == fall else "1")
                    if not self.feasible(t):
                        continue
                todo.append(t)
            if len(done) + len(todo) + len(dropped) > maxpaths:
                raise GiveUp("more than %d paths in %s" % (maxpaths, side))
        self.stats["steps_" + side] = self.stats.get("steps_" + side, 0) + steps
        return done, dropped

    # ------------------------------------------------------------------ loops
    def iteration(self, side, head):
        """one generic iteration of the loop at head: [(constraints, out regs, exits?, wrote memory?)]"""
        C = self.C
        proj, info = self.project(side)
        st = self.entry_state(proj, info)
        st.regs.ip = head
        g = {r: C.BVS("g_" + r, 32, explicit_name=True) for r in GPR + ["flags"]}
        for r in GPR:
            setattr(st.regs, r, g[r])
        st.regs.cc_op, st.regs.cc_dep1, st.regs.cc_dep2, st.regs.cc_ndep = 0, g["flags"], 0, 0
        st.memory.epoch = "g"
        body = loop_body(info, head)
        out, todo, steps = [], [st], 0
        while todo:
            if time.time() > self.deadline:
                raise GiveUp("timeout (loop analysis)")
            s = todo.pop()
            a = s.addr
            if s is not st and a == head:
                out.append((list(s.solver.constraints), s, False))
                continue
            if a not in body:
                out.append((list(s.solver.constraints), s, True))
                continue
            if a in info["kinds"] and info["kinds"][a] != "rep":
                return None                                  # a call or return inside the loop
            if a != head and a in info["heads"]:
                return None                                  # a nested loop
            k = 1 if a == head else self.stop_len(info, a)
            succ = proj.factory.successors(s, num_inst=k)
            steps += 1
            if steps > 400 or succ.unconstrained_successors:
                return None
            for t in succ.flat_successors:
                if t.history.jumpkind != "Ijk_Boring":
                    return None
                todo.append(t)
        return g, out

    def diverges(self, side, s):
        """True when the dropped state s provably never leaves its loop (see module docstring)"""
        C, z3 = self.C, self.z3
        head = s.addr
        cache = self.__dict__.setdefault("_iter_cache", {})
        if (side, head) not in cache:
            cache[(side, head)] = self.iteration(side, head)
        r = cache[(side, head)]
        if r is None:
            return False, "loop body has calls, nested loops or too many paths"
        g, paths = r
        for cons, t, ex in paths:
            if t.memory.log:
                return False, "loop body writes memory"
        outv = lambda t, u: t.regs.eflags if u == "flags" else getattr(t.regs, u)
        # U: the registers the exit decisions depend on, closed under "the next value of u depends on"
        need = set()
        for cons, t, ex in paths:
            for c in cons:
                need |= set(c.variables)
        U, todo = set(), list(need)
        while todo:
            v = todo.pop()
            if not v.startswith("g_"):
                return False, "loop exit depends on memory or on %s" % v
            u = v[2:]
            if u in U:
                continue
            U.add(u)
            for cons, t, ex in paths:
                if not ex:
                    todo += [w for w in outv(t, u).variables if w[2:] not in U]
        sval = {u: (s.regs.eflags if u == "flags" else getattr(s.regs, u)) for u in U}
        subst = {"g_" + u: sval[u] for u in U}
        q = self.solver()
        q.add(self.z(self.pc(s)))
        bad = []
        for cons, t, ex in paths:
            c = self.z(C.And(*cons), subst) if cons else z3.BoolVal(True)
            if ex:
                bad.append(c)
            else:
                ne = [self.z(outv(t, u), subst) != self.z(sval[u]) for u in sorted(U)]
                bad.append(z3.And(c, z3.Or(*ne)) if ne else z3.BoolVal(False))
        if self.check(q, z3.Or(*bad)):
            return False, "a state at the bound can still leave the loop"
        return True, "diverge:%s" % ",".join(sorted(U))

    def explore_bounded(self, side):
        """explore with a loop bound that has a checked reason. -> (done, diverging, loop hints)"""
        hl = (self.hints.get("loops") or {}).get(side)
        info = self.project(side)[1]
        heads = sorted(info["heads"])
        if not heads:
            done, dropped = self.explore(side, MAX_BOUND)
            return done, [], {}
        if hl is not None:
            bound = {info["base"] + int(k, 16): v[0] for k, v in hl.items()}
            if set(bound) != set(heads):
                raise GiveUp("hint loops do not name the loop heads of %s" % side)
            seq = [bound]
        else:
            seq = [{h: n for h in heads} for n in (2, 4, 8, 16, 32, MAX_BOUND)]
        last = "no bound"
        prev_done = None
        for bound in seq:
            done, dropped = self.explore(side, bound)
            dropped = [s for s in dropped if self.feasible(s, 10.0)]
            # M&B: when doubling the bound only added more exited paths and states still reach it, the trip count
            # depends on the inputs and no bound up to 64 can cover the loop (unless diverging is proved below):
            # stop early instead of exploring up to 64 (114 s per such function before)
            grew = prev_done is not None and len(done) > prev_done
            prev_done = len(done)
            reasons, ok = {}, True
            for s in dropped:
                d, why = self.diverges(side, s)
                if not d:
                    ok, last = False, why
                    break
                reasons[s.addr] = why
            if ok:
                for s in dropped:
                    s.globals["events"] = s.globals["events"] + (dict(kind="diverge", esp=s.regs.esp,
                                                                      log=s.memory.log),)
                hints = {"%x" % (h - info["base"]): [bound[h], reasons.get(h, "exits within bound")]
                         for h in heads}
                return done, dropped, hints
            if hl is not None:
                raise GiveUp("hint loop bound for %s rejected: %s" % (side, last))
            if grew:
                raise GiveUp("loop in %s: trip count depends on the inputs (exits keep appearing up to bound %d): "
                             "%s" % (side, max(bound.values()), last))
        raise GiveUp("loop in %s not bounded up to %d iterations: %s" % (side, MAX_BOUND, last))

    # ------------------------------------------------------------------ comparison
    def observables(self, ea, eb):
        """[(name, a, b)] or a string naming a structural difference"""
        C = self.C
        L2 = self.level == "L2"
        ka = [(e["kind"], e.get("key", "")) for e in ea]
        kb = [(e["kind"], e.get("key", "")) for e in eb]
        if [k for k, _ in ka] != [k for k, _ in kb]:
            return "event sequence %s vs %s" % ([k for k, _ in ka], [k for k, _ in kb])
        obs, regions = [], []
        inesp = self.IN["esp"]
        for n, (x, y) in enumerate(zip(ea, eb)):
            tag = "%s#%d" % (x["kind"], n)
            if x["kind"] in ("noreturn", "trap"):
                continue
            if x["kind"] == "call":
                obs.append((tag + " target", x["target"], y["target"]))
                if L2:
                    regs = GPR + ["eflags", "d", "ftop", "fptag", "fpreg"] + XMM
                else:
                    reads = sorted(set(x["reads"]) | set(y["reads"]))
                    regs = ["esp", "d", "ftop", "fptag"] + reads
                    self.guessed |= {"%s %s" % (tag, r) for r in set(x["guess"]) | set(y["guess"])}
                obs += [("%s %s" % (tag, r), x["regs"][r], y["regs"][r]) for r in regs]
                if not L2:
                    obs += self.x87_valid(tag, x["regs"], y["regs"])
                lo = x["esp"]
                excl = [] if (L2 or x["escaped"] or y["escaped"]) else list(x["prot"]) + list(y["prot"])
                regions.append((tag + " memory", lo, excl, x["log"], y["log"],
                                x["W"] if x["W"] == y["W"] else None,
                                {"frame": True, "ar": x.get("at"), "ao": y.get("at"),
                                 "pf": (x.get("pframe"), y.get("pframe")), "wp": (x.get("wp"), y.get("wp")),
                                 "pwhy": x.get("pwhy") or y.get("pwhy")}))
            elif x["kind"] == "ret":
                if x["target"] is not None or y["target"] is not None:
                    if x["target"] is None or y["target"] is None:
                        return "tail call vs return at %s" % tag
                    obs.append((tag + " return target", x["target"], y["target"]))
                obs.append((tag + " esp", x["esp_after"], y["esp_after"]))
                if L2:
                    regs = [r for r in GPR if r != "esp"] + ["eflags", "d", "ftop", "fptag", "fpreg"] + XMM
                else:
                    regs = (["eax"] if not self.job.get("void_eax_dead") else []) + \
                        ["ebx", "esi", "edi", "ebp", "d", "ftop", "fptag"] + (["edx"] if self.job["ret64"] else [])
                obs += [("%s %s" % (tag, r), x["regs"][r], y["regs"][r]) for r in regs]
                if not L2:
                    obs += self.x87_valid(tag, x["regs"], y["regs"])
                regions.append((tag + " memory", x["esp_after"], [], x["log"], y["log"], None,
                                {"ret": True} if self.entry_F is not None else None))
            elif x["kind"] == "diverge":
                excl = [] if L2 else [(x["esp"], inesp + 4)]
                regions.append((tag + " memory", x["esp"], excl, x["log"], y["log"], None, None))
            elif x["kind"] == "exit":
                # region mode: the interface R(x', y', pi') of the stop pair (tools/equiv/regions.py); pi' was
                # chosen by match_exit (proved pairs of live registers)
                pi = self._exit_pi or {}
                obs += [("%s %s" % (tag, r), x["regs"][r], y["regs"][r]) for r in ["esp", "d", "ftop", "fptag"] + XMM]
                obs += [("%s %s/%s" % (tag, r, v), _low(x["regs"]["eflags" if r == "flags" else r], v),
                         _low(y["regs"]["eflags" if v == "flags" else v.split(":")[0]], v))
                        for r, v in sorted(pi.items()) if v]
                obs += self.x87_valid(tag, x["regs"], y["regs"])
                regions.append((tag + " memory", x["esp"], [], x["log"], y["log"], None,
                                {"frame": True, "ar": x.get("at"), "ao": y.get("at"), "exit": True}))
        return obs, regions

    def x87_valid(self, tag, ra, rb):
        out = []
        for k in range(8):
            ta, tb = ra["fptag"][8 * k + 7:8 * k], rb["fptag"][8 * k + 7:8 * k]
            va, vb = ra["fpreg"][64 * k + 63:64 * k], rb["fpreg"][64 * k + 63:64 * k]
            if ta.concrete and ta.concrete_value == 0 and tb.concrete and tb.concrete_value == 0:
                continue
            out.append(("%s st-slot%d" % (tag, k), self.C.If(ta != 0, va, 0), self.C.If(tb != 0, vb, 0)))
        return out

    def compare_pair(self, sa, sb, pcs, known_equal_pc):
        """None when equivalent on the overlap, else a counterexample dict"""
        C, z3 = self.C, self.z3
        ea, eb = sa.globals["events"], sb.globals["events"]
        self._pair_escaped = bool(sa.globals.get("escaped") or sb.globals.get("escaped"))
        q = self.solver()
        q.add(pcs[0])
        if not known_equal_pc:
            q.add(pcs[1])
        # (M&B) the original returns to its caller: at its ret, esp is the entry esp. This only matters when an
        # unknown callee's `ret N` (an indirect call: the shared symbol rnK) left esp symbolic; it fixes rnK to the
        # value the original's own code was compiled for, instead of asking Z3 for all 17 values (with every later
        # stack read an If on the symbolic offset, this made float-heavy functions time out)
        for e in ea:
            if self.region:
                break               # region mode: in_esp is the cut's esp, not the entry esp
            if e["kind"] == "ret" and e.get("esp_at") is not None and e["target"] is not None \
                    and any(v.startswith("rn") for v in e["esp_at"].variables):
                q.add(self.z(e["esp_at"] == self.IN["esp"]))
        if self.region:
            self._exit_pi = None
            self._exit_F = None
            if ea and eb and ea[-1]["kind"] == "exit" and eb[-1]["kind"] == "exit" and \
                    [e["kind"] for e in ea] == [e["kind"] for e in eb]:
                self._exit_pi = self.match_exit(ea[-1], eb[-1], q)
        r = self.observables(ea, eb)
        if isinstance(r, str):
            if not self.check(q):
                return None
            return self.cex(q, r, [], None)
        obs, regions = r
        for a_, b_ in ((a, b) for _n, a, b in obs):
            if a_.size() != b_.size():
                return {"what": _n + " (size)"}
        if self.abstract_equal(sa, sb, obs, regions, known_equal_pc):
            self.stats["abstract_proofs"] = self.stats.get("abstract_proofs", 0) + 1
            return None
        diffs = []
        guessed = self.guessed
        for name, a, b in obs:
            if a.size() != b.size():
                return {"what": name + " (size)"}
            if same(a, b):
                continue
            diffs.append((name, self.z(a) != self.z(b)))
        x = self.PROBE
        zx = self.z(x)
        for name, lo, excl, la, lb, _W, fr in _reg7(regions):
            if la is lb:
                continue
            if self.frame_model and fr and fr.get("frame"):
                try:
                    diffs += self.frame_region_diffs(name, lo, la, lb, fr, q)
                except Unsupported as e:
                    # a frame word one version never wrote below its cut esp (esp differs there): a difference
                    diffs.append(("%s (%s)" % (name, str(e)[:80]), z3.BoolVal(True)))
                continue
            ma, mb = self.read_byte(x, la), self.read_byte(x, lb)
            cond = [z3.UGE(zx, self.z(lo))]
            if fr and fr.get("ret"):
                # (frame model, an explicit F at the start) ours' frame is related only through F: the probe stays
                # above the entry esp, and the return must leave esp there (above the frame)
                top = self.IN["esp"] + self.region_dz["depth"]
                cond.append(z3.UGE(zx, self.z(top)))
                diffs.append((name + " esp below the frame", z3.ULT(self.z(lo), self.z(top + 4))))
            if same(ma, mb):
                continue
            for elo, ehi in excl:
                cond.append(z3.Not(z3.ULT(zx - self.z(elo), self.z(ehi) - self.z(elo))))
            diffs.append((name, z3.And(*cond + [self.z(ma) != self.z(mb)])))
        if not diffs:
            return None
        strict = [d for n, d in diffs if n not in guessed]
        if self.region:
            # region mode: a region never refutes, so an undecided query only costs time; the float abstraction
            # already failed to prove these observables equal, and Z3 at the full float semantics took the whole
            # region budget on frame-slot differences (0x4952f0): cap it at REGION_QCAP seconds
            self.qcap = REGION_QCAP
            try:
                if strict and self.check(q, z3.Or(*strict)):
                    return self.cex(q, None, [(n, d) for n, d in diffs if n not in guessed], regions)
            except GiveUp as e:
                return {"what": ", ".join(n for n, _d in diffs if n not in guessed)[:300],
                        "undecided": "not equal in the float abstraction; the full query: %s" % e}
            finally:
                self.qcap = None
        elif strict and self.check(q, z3.Or(*strict)):
            return self.cex(q, None, [(n, d) for n, d in diffs if n not in guessed], regions)
        weak = [d for n, d in diffs if n in guessed]
        if not weak or not self.check(q, z3.Or(*weak)):
            return None
        c = self.cex(q, None, [(n, d) for n, d in diffs if n in guessed], regions)
        c["assumed_reads"] = True
        return c

    def abstract_equal(self, sa, sb, obs, regions, known_equal_pc):
        """(M&B) The same comparison with every float term seen from the integer world (fpToIEEEBV, float
        compares, ...) replaced by a free variable per structural hash. A model of the real query is a model of
        this one (give each variable its term's value), so 'no difference' here is 'no difference' there. Only
        when float semantics is needed (or there is a real difference) does the full query run, which with float
        stores in the write log is where float-heavy functions spent their time."""
        if not any(self.has_fp(a) or self.has_fp(b) for _n, a, b in obs) and not regions:
            return False
        if self.entry_F is not None:
            return False            # (frame model) the probe comparisons need the frame bounds: the full query
        z3 = self.z3
        self.abs_mode = True
        try:
            q = self.solver()
            q.add(self.z(self.pc(sa)))
            if not known_equal_pc:
                q.add(self.z(self.pc(sb)))
            for e in sa.globals["events"]:
                if self.region:
                    break
                if e["kind"] == "ret" and e.get("esp_at") is not None and e["target"] is not None \
                        and any(v.startswith("rn") for v in e["esp_at"].variables):
                    q.add(self.z(e["esp_at"] == self.IN["esp"]))
            diffs = [self.z(a) != self.z(b) for _n, a, b in obs if not same(a, b)]
            x = self.PROBE
            zx = self.z(x)
            for name, lo, excl, la, lb, _W, fr in _reg7(regions):
                if la is lb:
                    continue
                if self.frame_model and fr and fr.get("frame"):
                    # the frame correspondence is not a float question; let the full query decide it
                    return False
                ma, mb = self.read_byte(x, la), self.read_byte(x, lb)
                if same(ma, mb):
                    continue
                cond = [z3.UGE(zx, self.z(lo))]
                for elo, ehi in excl:
                    cond.append(z3.Not(z3.ULT(zx - self.z(elo), self.z(ehi) - self.z(elo))))
                diffs.append(z3.And(*cond + [self.z(ma) != self.z(mb)]))
            if not diffs:
                return True
            rem = self.deadline - time.time()
            q.set("timeout", int(max(0.5, min(rem / 3, 30)) * 1000))
            return q.check(z3.Or(*diffs)) == z3.unsat
        finally:
            self.abs_mode = False

    def cex(self, q, what, diffs, regions):
        z3 = self.z3
        m = q.model()
        out = {}
        if getattr(self, "_pair_escaped", False):
            out["escaped"] = True
        if what:
            out["what"] = what
        else:
            bad = [n for n, d in diffs if z3.is_true(m.eval(d, model_completion=True))]
            out["what"] = ", ".join(bad[:4]) or diffs[0][0]
            mem = [n for n in bad if n.endswith("memory")]
            if mem:
                ev = lambda e: m.eval(self.z(e), model_completion=True).as_long()
                addr = ev(self.PROBE)
                out["addr"] = hex(addr)
                reg = [r for r in regions if r[0] == mem[0]][0]
                lo, top = ev(reg[1]), ev(self.IN["esp"]) + 4 + (self.job.get("argw") or 0)
                argtop = lo + 4 + reg[5] if isinstance(reg[5], int) else lo
                out["frame"] = lo <= addr < top and addr >= argtop
        for r in GPR:
            out["in_" + r] = hex(m.eval(self.z(self.IN[r]), model_completion=True).as_long())
        return out

    def run(self):
        t0 = time.time()
        level = self.level
        A, Ad, la = self.explore_bounded("retail")
        B, Bd, lb = self.explore_bounded("ours")
        A, B = A + Ad, B + Bd
        texp = time.time() - t0
        hints_out = {"v": 1, "bij": self.job["bij"], "loops": {}}
        if la:
            hints_out["loops"]["retail"] = la
        if lb:
            hints_out["loops"]["ours"] = lb
        if not hints_out["loops"]:
            del hints_out["loops"]
        zpc, cpc = {}, {}
        feasible = {}
        for side, S in (("a", A), ("b", B)):
            for s in S:
                zpc[id(s)] = self.z(self.pc(s))
                cpc[id(s)] = self.pc(s).hash()
        # pairing
        hp = {a: b for a, b in self.hints.get("pairs", [])}
        hfall = set(self.hints.get("fallback", []))
        given = "pairs" in self.hints or "fallback" in self.hints
        bysig = {}
        for sb in B:
            bysig.setdefault(sb.globals["br"], []).append(sb)
        siga = {sa.globals["br"] for sa in A}
        for x, y in hp.items():
            if x not in siga or y not in bysig:
                raise GiveUp("hint pairs name a path that does not exist (%s -> %s)" % (x, y))
        if not hfall <= siga:
            raise GiveUp("hint fallback names a path that does not exist")
        pairs_out, fallback_out = [], []
        unA, usedB = [], set()
        cexs = []
        matched = 0
        for sa in A:
            sig = sa.globals["br"]
            if given and sig in hfall:
                unA.append(sa)
                fallback_out.append(sig)
                continue
            want = hp.get(sig, sig)
            cands = [sb for sb in bysig.get(want, []) if id(sb) not in usedB]
            ok = None
            for sb in cands:
                q = self.solver()
                if cpc[id(sa)] == cpc[id(sb)] or not self.check(q, z3_xor(self.z3, zpc[id(sa)], zpc[id(sb)])):
                    ok = sb
                    break
            if ok is None:
                if given:
                    raise GiveUp("hint pairing rejected for path %s" % (sig or "(entry)"))
                unA.append(sa)
                fallback_out.append(sig)
                continue
            usedB.add(id(ok))
            matched += 1
            if want != sig:
                pairs_out.append([sig, want])
            c = self.compare_pair(sa, ok, (zpc[id(sa)], zpc[id(ok)]), True)
            if c:
                cexs.append(c)
                break
        if not cexs and unA:
            unB = [sb for sb in B if id(sb) not in usedB]
            for sa in unA:
                q = self.solver()
                q.add(zpc[id(sa)])
                if not self.check(q):
                    continue                     # infeasible under the full memory model
                for sb in unB:
                    q2 = self.solver()
                    q2.add(zpc[id(sa)], zpc[id(sb)])
                    if not self.check(q2):
                        continue
                    c = self.compare_pair(sa, sb, (zpc[id(sa)], zpc[id(sb)]), False)
                    if c:
                        cexs.append(c)
                        break
                if cexs:
                    break
        if pairs_out:
            hints_out["pairs"] = pairs_out
        if fallback_out:
            hints_out["fallback"] = fallback_out
        nloops = sum(len(v) for v in hints_out.get("loops", {}).values())
        summary = ("%s: retail %d paths (%d diverging), ours %d (%d); %d paired by equal path condition, %d by "
                   "overlap; explore %.1fs, %d queries" % (level, len(A), len(Ad), len(B), len(Bd), matched,
                                                          len(unA), texp, self.stats.get("queries", 0)))
        if nloops:
            summary += "; loops " + json.dumps(hints_out["loops"], separators=(",", ":"))
        if cexs:
            c = cexs[0]
            st = "refuted"
            why = []
            if not self.job["clean"]:
                st, why = "unknown", why + ["relocation pairing is heuristic"]
            if self.job["x87"] and not self.job["x87_same"]:
                st, why = "unknown", why + ["x87 sequences differ (64-bit model)"]
            if st == "refuted" and (re.search(r"call#\d+ esp", c["what"]) or (
                    not self.job.get("frame_same", True) and re.search(r"esp|memory", c["what"]))):
                # M&B: a different frame size moves every stack address a callee can see; the strict model calls
                # that a difference, but it is not one a caller can depend on
                st, why = "unknown", why + ["the frame layout differs (esp at a call)"]
            if st == "refuted" and c.get("escaped"):
                # (M&B) a frame address was handed to a callee: the model then lets any pointer from memory alias
                # the whole frame, also locals whose address was never taken (0x43df60), which C++ rules out
                st, why = "unknown", why + ["a frame address escaped (the model's aliasing is coarser than C++)"]
            if st == "refuted" and X87_UF_USED:
                st, why = "unknown", why + ["x87 %s modelled as uninterpreted functions" % ",".join(
                    sorted(n[4:] for n in X87_UF_USED))]
            if st == "refuted" and c.get("assumed_reads"):
                st, why = "unknown", why + ["the differing register is read only if the unknown callee's "
                                            "convention reads it"]
            if self.hints:
                st, why = "unknown", why + ["checked against given hints"]
            if st == "refuted" and c.get("frame") and level == "L1":
                st, why = "unknown", why + ["the memory difference is in the caller's own frame"]
            d = "difference: %s" % json.dumps(c, separators=(",", ":"))
            if why:
                d = "counterexample in the model, not trusted (%s): %s" % ("; ".join(why), d)
            return {"status": st, "detail": d + " | " + summary, "hints": {}}
        if self.job["x87"] and not self.job["x87_same"]:
            return {"status": "unknown", "detail": "equal in the 64-bit x87 model, but the x87 instruction "
                    "sequences differ | " + summary, "hints": hints_out}
        return {"status": "proved", "detail": summary, "hints": hints_out, "covered": sorted(self.covered)}


PI_REGS = ["eax", "ecx", "edx", "ebx", "ebp", "esi", "edi", "flags"]     # regions.PI_REGS
REGION_QCAP = 20.0


# ====================================================================== the per-object frame model (region mode)
def f_text(F):
    """regions.f_text: an explicit frame correspondence as text, None for the standard interface"""
    return None if F is None else ";".join("%x>%x:%x" % tuple(t) for t in F)


def _reg7(regions):
    """region memory tuples padded to 7 (older callers pass 6)"""
    return [r if len(r) == 7 else (tuple(r) + (None,)) for r in regions]


def _frame_live(d):
    """{addr hex: "all" | [[lo, hi), ...]} -> {addr int: "all" | [(lo, hi), ...]}"""
    if not d:
        return {}
    return {int(k, 16): (v if v == "all" else [tuple(iv) for iv in v]) for k, v in d.items()}


def _frame_live_pred(self, at, side):
    """the predicate 'frame byte-offset off (below the entry esp) is live at this stop'. at None or a stop the
    static analysis marked "all", or absent: every offset live (conservative)."""
    if at is None:
        return lambda off: True
    fl = self.frame_live.get(side, {}).get(at, "all")
    if fl == "all":
        return lambda off: True
    return lambda off: any(lo <= off < hi for lo, hi in fl)


def _frame_words(self, log, inesp, depth, lo):
    """{offset below entry esp: 4-byte word AST} for every 4-aligned frame word [lo, entry) that `log` writes.
    offset = depth - w, where w = addr - inesp (so w in [lo - inesp, depth))."""
    lo_off = const_diff(lo, inesp)
    if lo_off is None:
        return {}
    lo_w = lo_off - (1 << 32) if lo_off >= 0x80000000 else lo_off
    touched = set()
    for ent in log:
        if ent[0] != "w":
            continue
        d = const_diff(ent[1], inesp)
        if d is None:
            continue
        dw = d - (1 << 32) if d >= 0x80000000 else d
        for k in range(len(ent[2])):
            b = dw + k
            if lo_w <= b < depth:
                touched.add(b)
    words = sorted({b - (b % 4) for b in touched})
    out = {}
    for w in words:
        if lo_w <= w and w + 4 <= depth + 4:
            # simplify the word read back as Concat(Extract(..)) so const_diff can see a frame pointer in it
            out[depth - w] = self.C.simplify(self.read_word(inesp + w, log))
    return out


def _frame_region_diffs(self, name, lo, la, lb, fr, q):
    """the frame-model obligation for one call/exit memory region: bytes at/above the entry esp equal, and every
    LIVE frame word matched to a word of the other side with an equal value (or a frame pointer to a matched
    object). A live word with no partner is an unmatched slot -> an unconditional difference (the region fails).
    Returns [(name, z3 term)]."""
    C, z3 = self.C, self.z3
    inesp = self.IN["esp"]
    depth = self.region_dz.get("depth")
    diffs = []
    if depth is None:
        ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
        if not same(ma, mb):
            zx = self.z(self.PROBE)
            diffs.append((name, z3.And(z3.UGE(zx, self.z(lo)), self.z(ma) != self.z(mb))))
        return diffs
    if fr.get("exit"):
        return _frame_exit_diffs(self, name, lo, la, lb, fr, q, depth)
    pa, pb = fr.get("pf") or (False, False)
    if pa != pb:
        # one version's call keeps its frame across the callee, the other's lets the callee write it: the two
        # havocs are not the same model of one callee
        return [(name + " frame protection differs", z3.BoolVal(True))]
    if pa:
        return _frame_call_args_diffs(self, name, lo, la, lb, fr, depth)
    if self.entry_F is not None:
        # an explicit F and a callee that may reach the frame: the callee's havoc gives both versions the same
        # bytes at the same addresses, which models one callee only when it saw the same frame: every frame
        # byte equal at the same address (the standard obligation; untouched bytes too, since ours' frame is
        # remapped)
        entry = inesp + depth
        zx = self.z(self.PROBE)
        ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
        if not same(ma, mb):
            diffs.append((name + " caller", z3.And(z3.UGE(zx, self.z(entry)), self.z(ma) != self.z(mb))))
        dl = const_diff(lo, inesp)
        if dl is None:
            return diffs + [(name + " esp unknown", z3.BoolVal(True))]
        dep = depth - (dl - (1 << 32) if dl >= 0x80000000 else dl)
        if dep % 4 or dep < 0 or dep > 0x10000:
            return diffs + [(name + " frame not word aligned", z3.BoolVal(True))]
        for off in range(4, dep + 1, 4):
            try:
                a_, b_ = self.read_word(entry - off, la), self.read_word(entry - off, lb)
            except Unsupported:
                return diffs + [(name + " frame word below the cut's esp", z3.BoolVal(True))]
            if not same(a_, b_):
                diffs.append((name + " frame -%#x (unprotected: %s)" % (off, fr.get("pwhy")),
                              self.z(a_) != self.z(b_)))
        return diffs
    # Soundness guard: the correspondence may relax the frame here only when the frame is dead on the far side of
    # this cut on BOTH sides (this implementation does not carry F past a call's whole-frame havoc or into the next
    # region, so a relaxed slot read later would read a wrong value). Otherwise fall back to whole-frame byte
    # equality (the standard region obligation), which is always sound.
    entry = inesp + depth
    zx = self.z(self.PROBE)
    live_r = _frame_live_pred(self, fr.get("ar"), "retail")
    live_o = _frame_live_pred(self, fr.get("ao"), "ours")
    ar, ao = fr.get("ar"), fr.get("ao")
    safe = (ar is None or ar in self.frame_dead.get("retail", set())) and \
           (ao is None or ao in self.frame_dead.get("ours", set()))
    if not safe:
        # cross-offset correspondence is unsound here (F is not carried past this cut's havoc or into the next
        # region); require byte-equality at the SAME address for every LIVE frame word, and exclude words that are
        # dead from here on (never read again -> the relaxation is unobservable). Caller memory (>= entry esp) is
        # byte-equal as usual.
        ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
        if not same(ma, mb):
            diffs.append((name + " caller", z3.And(z3.UGE(zx, self.z(entry)), self.z(ma) != self.z(mb))))
        wr = _frame_words(self, la, inesp, depth, lo)
        wo = _frame_words(self, lb, inesp, depth, lo)
        for off in sorted(set(wr) | set(wo)):
            if not any(live_r(off - k) or live_o(off - k) for k in range(4)):
                continue                                  # dead on both sides from here: excluded
            addr = entry - off
            a_ = wr.get(off) if off in wr else self.read_word(addr, la)
            b_ = wo.get(off) if off in wo else self.read_word(addr, lb)
            if not same(a_, b_):
                diffs.append((name + " frame -%#x (unprotected: %s)" % (off, fr.get("pwhy")),
                              self.z(a_) != self.z(b_)))
        return diffs
    ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
    if not same(ma, mb):
        diffs.append((name + " caller", z3.And(z3.UGE(zx, self.z(entry)), self.z(ma) != self.z(mb))))
    vr = {o: v for o, v in _frame_words(self, la, inesp, depth, lo).items() if any(live_r(o - k) for k in range(4))}
    vo = {o: v for o, v in _frame_words(self, lb, inesp, depth, lo).items() if any(live_o(o - k) for k in range(4))}
    save = getattr(self, "qcap", None)
    self.qcap = REGION_QCAP

    def peq(a, b):
        if same(a, b):
            return True
        try:
            return not self.check(q, self.z(a) != self.z(b))
        except GiveUp:
            return False

    def fp_off(word):
        c = const_diff(word, inesp)
        if c is None:
            d = self.C.simplify(word - inesp)       # read_word rebuilds Concat(Extract..); let claripy fold it
            if d.concrete:
                c = d.concrete_value
            else:
                # a frame pointer stored and read back is a byte Concat const_diff cannot parse: ask the solver
                # for word - inesp and check it is a unique constant under the path condition
                try:
                    zd = self.z(d)
                    if not self.check(q):
                        return None
                    c = q.model().eval(zd, model_completion=True).as_long()
                    if self.check(q, zd != c):
                        return None
                except (GiveUp, Exception):
                    return None
        cw = c - (1 << 32) if c >= 0x80000000 else c
        return depth - cw
    try:
        obj, used = {}, set()
        for o_r in sorted(vr):                       # content matching
            for o_o in sorted(vo):
                if o_o not in used and peq(vr[o_r], vo[o_o]):
                    obj[o_r] = o_o
                    used.add(o_o)
                    break
        rem_o = [o for o in sorted(vo) if o not in used]
        for o_r in [o for o in sorted(vr) if o not in obj]:
            tr = fp_off(vr[o_r])                      # frame-pointer relation: targets must be matched objects
            if tr is None:
                continue
            for o_o in list(rem_o):
                to = fp_off(vo[o_o])
                if to is not None and (obj.get(tr) == to or tr == to):
                    obj[o_r] = o_o
                    used.add(o_o)
                    rem_o.remove(o_o)
                    break
    finally:
        self.qcap = save
    for o_r in sorted(vr):
        if o_r not in obj:
            diffs.append((name + " frame slot -%#x" % o_r, z3.BoolVal(True)))
    for o_o in sorted(vo):
        if o_o not in used:
            diffs.append((name + " frame slot (ours) -%#x" % o_o, z3.BoolVal(True)))
    return diffs


def _frame_call_args_diffs(self, name, lo, la, lb, fr, depth):
    """(frame model) a call whose callee cannot reach the frame above its arguments on either side (do_call's
    pframe): the callee reads its stack arguments [esp, esp+4+W) (the return address and W bytes; esp is equal,
    an observable of the call) and memory outside the frame, at or above the entry esp; the frame above the
    arguments is no input of it (and keeps its contents across the call: the havoc protects it)"""
    z3 = self.z3
    inesp = self.IN["esp"]
    wa, wb = fr.get("wp")
    if wa != wb or not isinstance(wa, int):
        return [(name + " argument bytes differ", z3.BoolVal(True))]
    entry = inesp + depth
    zx = self.z(self.PROBE)
    diffs = []
    ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
    if not same(ma, mb):
        diffs.append((name + " caller", z3.And(z3.UGE(zx, self.z(entry)), self.z(ma) != self.z(mb))))
    for k in range(0, 4 + wa):
        a_, b_ = self.read_byte(lo + k, la), self.read_byte(lo + k, lb)
        if not same(a_, b_):
            diffs.append((name + " argument byte %d" % k, self.z(a_) != self.z(b_)))
    return diffs


def _frame_exit_diffs(self, name, lo, la, lb, fr, q, depth):
    """(frame model stage 1) the memory obligation of R(x', y', pi', d', F') at a stop pair, with F' chosen here.
    esp is equal (an observable of the exit). Memory at and above the entry esp E: equal at the same addresses.
    The frame [esp, E): the words live at the stop on either side (frame_liveness; "all": every word). When every
    such word is equal at the same address, F' = None: the standard interface (it also assumes the dead words
    equal, which no continuation can observe: each path writes a dead byte before it reads it). Otherwise F' pairs
    words of the two versions with proved-equal contents (the identity first, then the first equal word of the
    other side; one to one) and is EXACTLY what the next region assumes: the paired slots equal, every other frame
    byte of ours unrelated to anything of the original's. A live word with no partner is simply not in F' (the next
    region sees an unrelated value there: weaker, never wrong). The pairs are obligations here (diffs)."""
    C, z3 = self.C, self.z3
    inesp = self.IN["esp"]
    entry = inesp + depth
    zx = self.z(self.PROBE)
    diffs = []
    ma, mb = self.read_byte(self.PROBE, la), self.read_byte(self.PROBE, lb)
    if not same(ma, mb):
        diffs.append((name + " caller", z3.And(z3.UGE(zx, self.z(entry)), self.z(ma) != self.z(mb))))
    dl = const_diff(lo, inesp)
    dep = None if dl is None else depth - (dl - (1 << 32) if dl >= 0x80000000 else dl)
    if dep is None or dep % 4 or dep < 0 or dep > 0x10000:
        if self.entry_F is None:
            # the frame's extent is unknown here (esp after a callee of unknown `ret N`): the standard
            # obligation, every byte at and above esp equal at the same address, and F' = None
            self._exit_F = None
            if not same(ma, mb):
                diffs.append((name + " frame", z3.And(z3.UGE(zx, self.z(lo)), self.z(ma) != self.z(mb))))
            return diffs
        return diffs + [(name + " esp unknown or unaligned at the stop", z3.BoolVal(True))]
    live_r = _frame_live_pred(self, fr.get("ar"), "retail")
    live_o = _frame_live_pred(self, fr.get("ao"), "ours")
    offs = list(range(4, dep + 1, 4))
    def word(o, log):
        try:
            return C.simplify(self.read_word(entry - o, log))
        except Unsupported:
            return None             # a word this version never wrote below the cut's esp: garbage, unrelated
    wa = {o: word(o, la) for o in offs}
    wb = {o: word(o, lb) for o in offs}
    # garbage on both sides (frame allocated in this region, written by neither): no obligation; the standard
    # interface treats these bytes the same way (its probe comparison reads one base symbol on both sides)
    offs = [o for o in offs if wa[o] is not None or wb[o] is not None]
    LA = {o for o in offs if any(live_r(o - k) for k in range(4))}
    LB = {o for o in offs if any(live_o(o - k) for k in range(4))}
    save = getattr(self, "qcap", None)
    self.qcap = REGION_QCAP
    budget = [64]                    # solver queries for the cross-offset search

    def peq(a, b, cross=False):
        if a is None or b is None:
            return False
        if same(a, b):
            return True
        if cross:
            if budget[0] <= 0:
                return False
            budget[0] -= 1
        try:
            return not self.check(q, self.z(a) != self.z(b))
        except GiveUp:
            return False
    try:
        need = sorted(LA | LB)
        if all(peq(wa[o], wb[o]) for o in need):
            for o in need:
                if not same(wa[o], wb[o]):
                    diffs.append((name + " frame -%#x" % o, self.z(wa[o]) != self.z(wb[o])))
            self._exit_F = None
            return diffs
        pairs, used = {}, set()
        for o in need:                                    # the identity where it holds
            if o in LA and o in LB and peq(wa[o], wb[o]):
                pairs[o] = o
                used.add(o)
        for o in sorted(LA):                              # the original's live words: an equal word of ours
            if o in pairs:
                continue
            c = [p for p in sorted(LB) if p not in used and wa[o] is not None and wb[p] is not None and
                 same(wa[o], wb[p])] or \
                [p for p in sorted(LB) if p not in used and peq(wa[o], wb[p], cross=True)][:1]
            if c:
                pairs[o] = c[0]
                used.add(c[0])
    finally:
        self.qcap = save
    # the next region places F' at its own E (its cut's static depth, regions._cut_dz): the stop's static depths must
    # be the depth this region computed, or F' would name other bytes there
    for side, at in (("r", fr.get("ar")), ("o", fr.get("ao"))):
        sd_ = (self.region.get("deps_" + side) or {}).get("%x" % at) if at is not None else None
        if sd_ is not None and sd_ != dep:
            return diffs + [(name + " stop depth %d (static) vs %d" % (sd_, dep), z3.BoolVal(True))]
    F = []
    for o_r in sorted(pairs):
        o_o = pairs[o_r]
        if not same(wa[o_r], wb[o_o]):
            diffs.append((name + " frame -%#x/-%#x" % (o_r, o_o), self.z(wa[o_r]) != self.z(wb[o_o])))
        # merge with the previous slot when both sides continue it downwards in memory (offsets grow by 4 as
        # addresses fall): (r_lo, o_lo, n) covers E - r_lo .. E - r_lo + n - 1
        if F and F[-1][0] + 4 == o_r and F[-1][1] + 4 == o_o:
            F[-1] = (o_r, o_o, F[-1][2] + 4)
        else:
            F.append((o_r, o_o, 4))
    self._exit_F = tuple(F)
    return diffs


Worker.frame_region_diffs = _frame_region_diffs


def _note_pair(self, sa, sb):
    """region mode: a compared path pair with no difference; its stop pair (with the x87 depth and the register
    correspondence) is an obligation of the caller, and its x87 sequences must be the same (the 64-bit model's
    rule)"""
    if not self.region:
        return
    if sa.globals["x87"] != sb.globals["x87"]:
        self.x87_bad = True
    ea, eb = sa.globals["events"], sb.globals["events"]
    if ea and eb and ea[-1]["kind"] == "exit" and eb[-1]["kind"] == "exit":
        ft = ea[-1]["regs"]["ftop"]
        if not ft.concrete:
            raise Unsupported("symbolic x87 stack top at a stop")
        pi = self._exit_pi or {}
        ex = (ea[-1]["at"], eb[-1]["at"], (-ft.concrete_value) & 7, tuple(pi.get(r) for r in PI_REGS))
        if self.frame_model:
            ex = ex + (self._exit_F,)
        self.exits.add(ex)


def _match_exit(self, xa, xb, q):
    """region mode: the register correspondence at a stop pair: for every register live at the original's stop
    (or ours'), a register of the other side with the same value: the same term, else (one solver query) the same
    register; registers with no partner are left unrelated (a weaker interface, never a wrong one)"""
    every = set(PI_REGS)
    lr = self.live_at["retail"].get(xa["at"], every)
    lo = self.live_at["ours"].get(xb["at"], every)
    if self.level == "L2":
        lr = lo = every
    g7 = PI_REGS[:7]
    ra = {r: xa["regs"][r] for r in g7}
    rb = {r: xb["regs"][r] for r in g7}
    pi, used = {}, set()
    for r in g7:
        if r in lr:
            c = [v for v in g7 if v not in used and same(ra[r], rb[v])]
            if c:
                v = r if r in c else c[0]
                pi[r] = v
                used.add(v)
    for v in g7:
        if v in lo and v not in used:
            c = [r for r in g7 if r not in pi and same(ra[r], rb[v])]
            if c:
                r = v if v in c else c[0]
                pi[r] = v
                used.add(v)

    def proved_eq(a, b):
        if same(a, b):
            return True
        try:
            return not self.check(q, self.z(a) != self.z(b))
        except GiveUp:
            return False
    for r in g7:
        if (r in lr or r in lo) and r not in pi and r not in used and proved_eq(ra[r], rb[r]):
            pi[r] = r
            used.add(r)
    # a live register with no full partner: its low 16 or 8 bits may still be related ("v:16" / "v:8", the
    # interface relates only those; cl's bools in al over a garbage upper part)
    for r in g7:
        if r not in lr or r in pi:
            continue
        for w in (16, 8):
            c = [v for v in g7 if v not in used and same(ra[r][w - 1:0], rb[v][w - 1:0])]
            v = (r if r in c else c[0]) if c else (r if r not in used and proved_eq(ra[r][w - 1:0],
                                                                                    rb[r][w - 1:0]) else None)
            if v:
                pi[r] = "%s:%d" % (v, w)
                used.add(v)
                break
    if ("flags" in lr or "flags" in lo) and proved_eq(xa["regs"]["eflags"], xb["regs"]["eflags"]):
        pi["flags"] = "flags"
    return pi


def _low(x, v):
    """the bits of x a pi entry v relates"""
    w = int(v.split(":")[1]) if v and ":" in v else 32
    return x if w >= x.size() else x[w - 1:0]


class _View:
    """a path cut at a candidate leader: the events up to there, the path's condition (a stronger condition than
    the prefix's own: sound, the other paths through the candidate are compared with their own conditions)"""

    def __init__(self, st, events, x87):
        self.solver = st.solver
        self.globals = {"events": events, "x87": x87, "escaped": True, "dz": st.globals.get("dz", ()),
                        "br": st.globals["br"]}


def _region_compare(self, sa, sb, pcs, known):
    """compare a path pair; when their event sequences differ, the cut at a candidate leader of one or both"""
    kinds = lambda v: [e["kind"] for e in v.globals["events"]]
    if kinds(sa) == kinds(sb):
        c = self.compare_pair(sa, sb, pcs, known)
        if not c:
            self.note_pair(sa, sb)
        return c
    va = [sa] + [_View(sa, ev, x) for ev, x in sa.globals["cands"]]
    vb = [sb] + [_View(sb, ev, x) for ev, x in sb.globals["cands"]]
    first = None
    order = [(a, b) for a in va for b in vb]
    order.sort(key=lambda ab: (ab[0] is not sa and ab[1] is not sb, len(ab[0].globals["events"])))
    for a, b in order:
        if kinds(a) != kinds(b):
            continue
        c = self.compare_pair(a, b, pcs, known)
        if not c:
            self.note_pair(a, b)
            return None
        first = first or c
    return first or self.compare_pair(sa, sb, pcs, known)


def _region_unsat(self, terms, xor=False):
    """region mode: True only when the conjunction of the claripy terms (with xor: their exclusive or, i.e. the two
    path conditions differ) is proved unsatisfiable: first with
    float terms abstracted to free variables (an over-approximation: unsatisfiable there is unsatisfiable), then,
    without float terms, the full query. With float terms the full query is skipped (Z3 does not honour its timeout
    while bit-blasting them; on update_ai 0x4465e0 one path-condition comparison took the whole minute): the answer
    is then "maybe satisfiable", which only adds path pairs to compare (sound)."""
    z3 = self.z3

    def q():
        zs = [self.z(t) for t in terms]
        return [z3.Xor(zs[0], zs[1])] if xor else zs
    self.abs_mode = True
    try:
        if not self.check(self.solver(), *q()):
            return True
    except GiveUp:
        pass
    finally:
        self.abs_mode = False
    if any(self.has_fp(t) for t in terms):
        return False
    try:
        return not self.check(self.solver(), *q())
    except GiveUp:
        return False


def _run_region(self):
    t0 = time.time()
    A, Ad = self.explore("retail", MAX_BOUND)
    B, Bd = self.explore("ours", MAX_BOUND)
    if Ad or Bd:
        raise GiveUp("a loop inside a region (a loop head that is not a stop)")
    texp = time.time() - t0
    z3 = self.z3
    zpc = {id(s): self.z(self.pc(s)) for s in A + B}
    cpc = {id(s): self.pc(s).hash() for s in A + B}
    bysig = {}
    for sb in B:
        bysig.setdefault(sb.globals["br"], []).append(sb)
    used, unA, cexs, matched, overlap = set(), [], [], 0, 0
    for sa in A:
        ok = None
        for sb in bysig.get(sa.globals["br"], []):
            if id(sb) in used:
                continue
            if cpc[id(sa)] == cpc[id(sb)] or self.region_unsat([self.pc(sa), self.pc(sb)], xor=True):
                ok = sb
                break
        if ok is None:
            unA.append(sa)
            continue
        used.add(id(ok))
        matched += 1
        c = self.region_compare(sa, ok, (zpc[id(sa)], zpc[id(ok)]), True)
        if c:
            cexs.append(c)
            break
    unB = [sb for sb in B if id(sb) not in used]
    if not cexs:
        for sa in unA:
            if self.region_unsat([self.pc(sa)]):
                continue
            for sb in unB:
                if self.region_unsat([self.pc(sa), self.pc(sb)]):
                    continue
                overlap += 1
                c = self.region_compare(sa, sb, (zpc[id(sa)], zpc[id(sb)]), False)
                if c:
                    cexs.append(c)
                    break
            if cexs:
                break
    summary = ("%s region: retail %d paths, ours %d; %d paired by equal path condition, %d pairs by overlap; "
               "%d stop pairs; explore %.1fs, %d queries" % (self.level, len(A), len(B), matched, overlap,
                                                              len(self.exits), texp, self.stats.get("queries", 0)))
    if cexs:
        return {"status": "unknown", "detail": "region: difference in the model (a region never refutes: its "
                "interface is stronger than equivalence): %s | %s" % (json.dumps(cexs[0], separators=(",", ":"))[:700],
                                                                       summary), "hints": {}}
    if self.x87_bad:
        return {"status": "unknown", "detail": "region: equal in the 64-bit x87 model, but a path pair runs "
                "different x87 instruction sequences | " + summary, "hints": {}}
    return {"status": "proved", "detail": summary, "hints": {}, "covered": sorted(self.covered),
            "exits": sorted(self.exits, key=str)}


Worker.note_pair = _note_pair
Worker.match_exit = _match_exit
Worker.region_compare = _region_compare
Worker.run_region = _run_region
Worker.region_unsat = _region_unsat


def z3_xor(z3, a, b):
    return z3.Xor(a, b)


# ====================================================================== command line
def main():
    if "--worker" in sys.argv:
        return worker_main()
    if "--serve" in sys.argv:
        return serve_main()
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?")
    ap.add_argument("va", nargs="?")
    ap.add_argument("--symbol")
    ap.add_argument("--objpair", nargs=3, metavar=("OURS_OBJ", "RETAIL_OBJ", "SYMBOL"),
                    help="object-vs-object mode (tools/equiv/objpair.py)")
    ap.add_argument("--level", default="L1")
    ap.add_argument("--timeout", type=float, default=600)
    ap.add_argument("--hints")
    ap.add_argument("--save-hints")
    a = ap.parse_args()
    sys.path.insert(0, HERE)
    import func as FN
    if a.objpair:
        import objpair
        fr, fo = objpair.setup(*a.objpair)
    else:
        if not (a.src and a.va):
            ap.error("src and va are required (or --objpair)")
        va = int(a.va, 16)
        fr, fo = FN.retail_func(va), FN.our_func(a.src, va, a.symbol)
    r = prove(fr, fo, a.level, json.load(open(a.hints)) if a.hints else None, a.timeout)
    if a.save_hints:
        json.dump(r["hints"], open(a.save_hints, "w"))
    print(json.dumps({k: v for k, v in r.items() if k != "hints"}, indent=1))
    print("hints: %d bytes" % len(json.dumps(r["hints"], separators=(",", ":"))))


if __name__ == "__main__":
    main()
