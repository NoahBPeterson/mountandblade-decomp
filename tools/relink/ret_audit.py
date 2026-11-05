#!/usr/bin/env python3
"""ret_audit.py: find functions our source declares void whose original callers use a return value.

usage: ret_audit.py [--map resolve_map.json] [--all]

call_diff compares which functions get called and an equivalence PASS does not always compare eax, so a wrong return type gets
past both: `void Sel::first(int*)` (0x489890) left another value in eax for a caller that reads the hidden-return pointer. This
sweeps the original's .text once: after each direct call it follows the fall-through for a few instructions and records whether
eax is read before it is written, or st(0) is stored. A branch ends the walk, so a reader on a jump target is missed. Every target
whose definition's mangled name returns void is listed. Most are harmless: a caller compiled against the void declaration takes the
result from its own out argument. SPELLING lines are the real bugs: a glue alias with a return value bound to a void definition (glue/glue.json beside --map). Every row, with non-byte-exact rows first (--all lists byte-exact rows too; there our code
is the original's whatever the declaration says). Each row is graded: preserved (the callee never writes eax: cl kept a live value
across it, not a return), body-set, tail-set (written within 8 instructions of a ret, the likely real returns), st0. Runs in one process.
"""
import argparse, collections, glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
EAX = {"eax", "ax", "al", "ah"}
PARTIAL = ("mov", "movzx", "movsx", "fnstsw", "sete", "setne", "setg", "setge", "setl", "setle", "seta", "setae", "setb", "setbe", "setp", "setnp", "sets", "setns", "lahf", "or")
STOP = ("ret", "retn", "jmp", "call", "int3", "hlt")
WALK = 12


def returns_void(name):
    """True / False from the mangled return type; None when the name has none to read (ctor, dtor, template, C name)."""
    if not name.startswith("?") or name.startswith(("??0", "??1", "??_", "??$")) or "?$" in name:
        return None
    m = re.match(r"\?[^@]+(?:@[^@]+)*@@(?:Y[A-Z]|[CKSDLT][A-Z]|[A-Z][A-Z][A-Z])(.)", name)
    return m.group(1) == "X" if m else None


def eax_read_after(md, code, va):
    """How the instructions after a call at (code, va) use the return registers: 'eax', 'st0' or None."""
    n = 0
    for i in md.disasm(code, va):
        n += 1
        if n > WALK or i.mnemonic in STOP or i.mnemonic.startswith("j"):
            return None
        if n == 1 and i.mnemonic in ("fstp", "fst", "fxch"):
            return "st0"
        if i.mnemonic == "xor" and i.op_str == "eax, eax" or i.op_str.startswith("eax, 0xffffffff") or i.mnemonic == "fnstsw":
            return None
        if i.mnemonic in PARTIAL and i.op_str.split(",")[0] in EAX:     # the register's old value is not used (or al,.. aside)
            return None
        rd, wr = i.regs_access()
        rd = {i.reg_name(r) for r in rd}
        wr = {i.reg_name(r) for r in wr}
        if rd & EAX:
            return "eax"
        if "eax" in wr:
            return None
    return None


def callee_sets_eax(md, img, va):
    """Does eax get written at all in the function, and in the block before a ret? cl keeps a value in eax across a call to a
    callee it can see does not touch eax, so a caller's read proves nothing unless the callee writes it."""
    try:
        lo, hi = img.func_extent(va)
    except Exception:
        return None, None
    any_w = tail_w = False
    last = -99
    for k, i in enumerate(md.disasm(img.read(lo, hi - lo), lo)):
        if i.mnemonic in ("ret", "retn"):
            if k - last <= 8:
                tail_w = True
            continue
        if "eax" in {i.reg_name(r) for r in i.regs_access()[1]} and not i.mnemonic.startswith("call"):
            any_w = True
            last = k
    return any_w, tail_w


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "standalone", "resolve_map.json"))
    ap.add_argument("--all", action="store_true")
    a = ap.parse_args()
    import capstone
    import resolve as R
    img = R.Image()
    rm = json.load(open(a.map))
    defined = {int(v, 16): d[0] for v, d in rm["defined_at"].items()}
    defined.update({int(v, 16): d[0] for v, d in rm["static_at"].items()})
    exact = set()
    for mf in glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True):
        for l in open(mf):
            p = l.split()
            if len(p) >= 3:
                try:
                    exact.add(int(p[2], 16))
                except ValueError:
                    pass
    sec = img.section(".text")
    lo, size = sec if isinstance(sec, tuple) else (sec.VirtualAddress + 0x400000, sec.Misc_VirtualSize)
    code = img.read(lo, size)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    sites = collections.defaultdict(lambda: collections.Counter())
    calls = collections.Counter()
    fast = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    fast.skipdata = True
    for addr, sz, mn, ops in fast.disasm_lite(code, lo):
        if mn != "call" or not ops.startswith("0x"):
            continue
        t = int(ops, 16)
        if t not in defined:
            continue
        calls[t] += 1
        after = addr + sz
        use = eax_read_after(md, code[after - lo:after - lo + 64], after)
        if use:
            sites[t][use] += 1
    rows = []
    for t, c in sites.items():
        if returns_void(defined[t]) is True:
            rows.append((t in exact, -sum(c.values()), t, defined[t], dict(c), calls[t]))
    rows.sort()
    shown = 0
    for ex, _n, t, name, c, total in rows:
        if ex and not a.all:
            continue
        shown += 1
        any_w, tail_w = callee_sets_eax(md, img, t)
        kind = "st0" if "st0" in c else "preserved" if any_w is False else "tail-set" if tail_w else "body-set"
        print("%08x  %-10s %-9s %s of %d call sites use %s  %s" % (t, "exact" if ex else "NOT-EXACT", kind, sum(c.values()), total, ",".join(sorted(c)), name[:90]))
    # the case that is a bug by itself: one address, a void definition, and a spelling elsewhere in the source that reads the result
    glue = os.path.join(os.path.dirname(a.map), "glue", "glue.json")
    aliases = json.load(open(glue))["aliases"] if os.path.exists(glue) else {}
    for alias, target in sorted(aliases.items()):
        target = target if isinstance(target, str) else target[0]
        if returns_void(target) is True and returns_void(alias) is False:
            print("SPELLING  %s is bound to the void %s" % (alias[:70], target[:70]))
    print("%d void-declared functions whose original callers use a return value (%d shown, %d byte-exact)" % (len(rows), shown, sum(1 for r in rows if r[0])))


if __name__ == "__main__":
    main()
