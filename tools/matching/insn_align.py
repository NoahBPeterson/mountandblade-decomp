#!/usr/bin/env python3
"""insn_align.py: align our compiled function's instructions with the original's and report what really
differs (identical / changed operands / only in the original / only in ours), instead of a positional
differing-byte count, which balloons when one instruction shifts the rest.

usage: insn_align.py <src.cpp> <va-hex> [<symbol-substring>] [--flags "..."] [--show]
  Relocated operands and branch targets are masked; the second figure also masks stack-slot numbers.
  The symbol defaults to the one chk.py would pick (the `// @ 0x<va>` marker)."""
import difflib, os, re, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "difftest"))
import os
os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import capstone, coff, slice as S, equiv as E
args = [a for a in sys.argv[1:] if a != "--show"]
flags = ["/O2", "/MT", "/Gy", "/TP"]
if "--flags" in args:
    i = args.index("--flags"); flags = args[i + 1].split(); del args[i:i + 2]
src, va = args[0], int(args[1], 16)
want = args[2] if len(args) > 2 else None
img = E.context()["img"]
obj, log = S.compile_obj(src, flags)
if not obj:
    sys.exit("compile failed: " + (log or "")[-400:])
c = coff.Coff(obj)
if want:
    n = [k for k in c.functions() if want in k][0]
else:
    n, _how = S.find_symbol(c, os.path.basename(src).rsplit(".", 1)[0], va, src)
    n = n[0] if isinstance(n, list) else n
s = c.by_name[n]; sec = c.sections[s.secno - 1]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
lo, hi = img.func_extent(va)
O = list(md.disasm(img.read(lo, max(hi - lo, len(sec.data) - s.value)), 0)); M = list(md.disasm(bytes(sec.data[s.value:]), 0))
RELOC = {off - s.value for off, _si, _t in sec.relocs}
lo_img, hi_img = 0x400000, 0x10000000
def norm(i, stack=True, ours=False):
    t = i.mnemonic + " " + i.op_str
    if ours and any(i.address <= r < i.address + i.size for r in RELOC):
        t = re.sub(r"\[(0x[0-9a-f]+|0)\]", "[G]", t); t = re.sub(r"(, |^\S+ )(0x[0-9a-f]+|0)$", r"\1IMM", t)
        t = re.sub(r" \+ 0x[0-9a-f]+\]|\]", lambda m: m.group(0), t)
    if not ours:
        t = re.sub(r"0x[0-9a-f]+", lambda m: "IMM" if lo_img <= int(m.group(0), 16) < hi_img else m.group(0), t)
        t = t.replace("[IMM]", "[G]").replace(" + IMM]", "]")    # an indexed absolute: ours prints it without disp
    t = re.sub(r"\[0x[0-9a-f]+\]|\[0\]", "[G]", t)                 # relocated absolute operands
    if i.mnemonic.startswith("j") or i.mnemonic == "call": t = i.mnemonic
    if not stack: t = re.sub(r"\[esp( \+ 0x[0-9a-f]+)?\]", "[esp+S]", t)
    return t
def stats(stack):
    a = [norm(i, stack) for i in O]; b = [norm(i, stack, True) for i in M]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    same = sum(bl.size for bl in sm.get_matching_blocks())
    ops = sm.get_opcodes()
    rep = sum(max(i2 - i1, j2 - j1) for t, i1, i2, j1, j2 in ops if t == "replace")
    dele = sum(i2 - i1 for t, i1, i2, j1, j2 in ops if t == "delete"); ins = sum(j2 - j1 for t, i1, i2, j1, j2 in ops if t == "insert")
    return len(a), len(b), same, rep, dele, ins, sm, a, b
o_n, m_n, same, rep, dele, ins, sm, a, b = stats(True)
_, _, same2, rep2, dele2, ins2, _, _, _ = stats(False)
print("%-34s orig %d insns, ours %d | identical %d, changed %d, only-orig %d, only-ours %d | ignoring stack-slot numbers: identical %d, changed %d"
      % (src.split("/")[-1], o_n, m_n, same, rep, dele, ins, same2, rep2))
if "--show" in sys.argv:
    for t, i1, i2, j1, j2 in sm.get_opcodes():
        if t == "equal": continue
        print("  %-7s orig[%d:%d] %s" % (t, i1, i2, " ; ".join(a[i1:i2])))
        print("          ours[%d:%d] %s" % (j1, j2, " ; ".join(b[j1:j2])))
