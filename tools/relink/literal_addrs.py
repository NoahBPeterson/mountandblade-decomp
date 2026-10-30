#!/usr/bin/env python3
"""literal_addrs.py: find original-image addresses written as literal numbers in our source.

usage: literal_addrs.py [--filter SUBSTR] [--json OUT] [-v]

`((void (*)(char*))0x4bab70)(buf)`, `*(void**)this = (void*)0x5947d8`, `(const char*)0x595788`: the equivalence
harness runs our code inside the original image, so a literal address works there; in the standalone exe it points
into our own image's layout, somewhere else. Each must be a declared symbol with its `// 0x...` annotation, which
the resolver maps for the harness (the same address) and the glue labels for the link.

Scans the compiled objects, not the text, so macros and templates are covered: every 4-byte operand (immediate or
displacement) with no relocation whose value is an address in the original (a function start, or a data address
the original's code uses). A field offset that equals such an address by chance is possible; the report gives the
instruction so each can be judged. Exits 1 when any are found.
"""
import argparse, collections, json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
LO, HI = 0x401000, 0x8ee5b94          # the original's .text through .bss
FAILED = []                           # sources scan() could not compile
# a numeric address cast in the text: `(T*)0x5a9f60`, `*(float*)0x5aa010`, `(void*)0x592848` (the object scan above
# sees only operands with no base register, so an indexed or base-relative use of such a literal escapes it)
TEXT_CAST = re.compile(r"\)\s*0x([5-9a][0-9a-fA-F]{5}|8e[0-9a-fA-F]{5})\b")


def absolute_operands(ins):
    """[(value, kind)] of an instruction's 4-byte operands that can be absolute addresses: a displacement with no
    base register ([0x5a9f60], [eax*4 + 0xa44260]; with a base register it is a field offset, and a 123 MB object
    has offsets in the image's range) as "mem", and a mov / push immediate as "imm" (an and / add / cmp immediate
    is a constant)."""
    from capstone import x86_const as X
    out = []
    if ins.disp_offset and ins.disp_size == 4:
        mem = [o for o in ins.operands if o.type == X.X86_OP_MEM]
        if mem and mem[0].mem.base == 0:
            out.append((struct.unpack_from("<I", bytes(ins.bytes), ins.disp_offset)[0], "mem", ins.disp_offset))
    if ins.imm_offset and ins.imm_size == 4 and ins.mnemonic in ("mov", "push"):
        out.append((struct.unpack_from("<I", bytes(ins.bytes), ins.imm_offset)[0], "imm", ins.imm_offset))
    return out


def original_addresses(md):
    """Values that are addresses in the original: function starts, the .rdata/.data addresses its code uses, and
    the .bss addresses it accesses as memory or that the resolver names as a global. A .bss value the original only
    ever loads as an immediate and nothing names (100000000 = 0x5f5e100) is an integer, not an address."""
    import resolve as R
    img = R.Image()
    out = set(img.start_set)
    starts = sorted(img.start_set)
    t0, tn = img.section(".text")
    for i, s in enumerate(starts):               # per function: a linear sweep stops at the first jump table
        e = starts[i + 1] if i + 1 < len(starts) else t0 + tn
        for ins in md.disasm(img.read(s, e - s), s):
            for v, kind, _k in absolute_operands(ins):
                if 0x549000 <= v < 0x5d7000 or (kind == "mem" and 0x5d7000 <= v < HI):
                    out.add(v)
    # and every address the resolution map names as a global (an object used only as `this`, mov ecx, imm, is never
    # a memory operand in the original): tools/relink/resolve_map.py's output
    for mp in (os.path.join(ROOT, "work", "relink", "standalone", "resolve_map.json"),
               os.path.join(ROOT, "work", "relink", "resolve_map.json")):
        if os.path.exists(mp):
            for d in json.load(open(mp))["names"].values():
                out.update(int(va, 16) for va in d if 0x549000 <= int(va, 16) < HI)
            break
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter")
    ap.add_argument("--json")
    ap.add_argument("-v", action="store_true", help="print every site")
    a = ap.parse_args()
    sites = scan(a.filter)
    by = collections.Counter(s["src"] for s in sites)
    for f, n in by.most_common():
        print("%4d  %s" % (n, f))
    print("%d literal image addresses in %d sources" % (len(sites), len(by)))
    text = text_literals()
    for p, i, v in text:
        print("text: %s:%d  %s" % (p, i, v))
    print("%d numeric address casts in the source text" % len(text))
    if FAILED:
        print("%d sources did not compile: %s" % (len(FAILED), ", ".join(FAILED)))
    if a.v:
        for s in sites:
            print("%s  %s  %s  %s" % (s["src"], s["value"], s["func"][:50], s["insn"]))
    if a.json:
        json.dump(sites, open(a.json, "w"), indent=1)
    return 1 if sites or text or FAILED else 0


def text_literals():
    """[(path, line, value)] of every cast of a number in the original's .rdata/.data/.bss to a pointer or
    integer type in our source (comments excluded)."""
    out = []
    for dp, _dn, fn in os.walk(os.path.join(ROOT, "src")):
        for f in fn:
            if not f.endswith((".cpp", ".h", ".hpp", ".inc")):
                continue
            p = os.path.join(dp, f)
            for i, line in enumerate(open(p, errors="replace"), 1):
                for m in TEXT_CAST.finditer(line.split("//")[0]):
                    v = int(m.group(1), 16)
                    if 0x549000 <= v <= HI:
                        out.append((os.path.relpath(p, ROOT), i, "%08x" % v))
    return out


def scan(filt=None):
    """[{src, func, value, insn, offset}] for every literal image address in our objects."""
    os.chdir(ROOT)
    import capstone
    import compile_all as C
    import slice as S
    from coff import Coff
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    known = original_addresses(md)
    sites = []
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src or (filt and filt not in src):
            continue
        o, _ = S.compile_obj(src, flags)
        if not o:
            FAILED.append(os.path.relpath(src, ROOT))      # an uncompiled source cannot be checked: that is a failure
            continue
        coff = Coff(o)
        funcs = {}
        for s in coff.syms.values():
            if s.defined and s.is_func and s.cls in (2, 3) and s.value == 0:
                funcs.setdefault(s.secno, s.name)
        for i, sec in enumerate(coff.sections):
            if not sec.is_code or not sec.data:
                continue
            relocs = {off for off, _si, _t in sec.relocs}
            for ins in md.disasm(sec.data, 0):
                for v, _kind, k in absolute_operands(ins):
                    if ins.address + k in relocs:
                        continue
                    if v >> 24 and all(((v >> s) & 0xff) < 0x20 for s in (0, 8, 16, 24)):
                        continue            # a switch's byte-index table after the function's ret, disassembled as code
                    if v in known:
                        sites.append({"src": os.path.relpath(src, ROOT), "func": funcs.get(i + 1, "?"),
                                      "value": "%08x" % v, "insn": "%s %s" % (ins.mnemonic, ins.op_str),
                                      "offset": ins.address + k})     # in the function's COMDAT section
    return sites


if __name__ == "__main__":
    sys.exit(main())
