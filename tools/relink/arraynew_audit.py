#!/usr/bin/env python3
"""arraynew_audit.py: compare how the original and our build allocate and free arrays.

usage: arraynew_audit.py --ours BUILD_DIR [--all] [--va VA]

MSVC 7.1 gives `new T[n]` an element count in front of the elements (n*size+4 bytes, the count at [p], the array
at p+4) exactly when T has a destructor; for a trivially destructible T it allocates n*size and stores nothing.
The array-deleting destructor (??_E / vector deleting destructor, `delete[] p`) of such a T reads the count back
from [p-4]. If our element type is trivially destructible where the original's was not, the original's own
destructor reads garbage at exit (the scene_props.txt crash in load_module_files, commit 4e4696dc); the reverse
frees p+4 with a plain operator delete[] and corrupts the heap.

For every call to operator new[] (0x535482) in the original image and in our build (build.py's mb_standalone.exe,
with the /MAP file saying where each of our functions starts) this classifies the site as

  cookie    the size is computed with a +4 (add r,4 / lea r,[r*N+4] / push imm) or the result is used as
            `mov [eax],count; lea/add +4` -- the element type has a destructor (or a vector constructor loop)
  plain     the result is used as the array itself

and reports, per original function that our build also defines, the multiset of classifications of both sides.
A function where they differ is a candidate; --all prints every function.

The original's calls are sites of 'call 0x535482' (a jump stub to operator new); ours are calls of the symbol
??_U@YAPAXI@Z, which the map names. Array deletes (operator delete[] 0x53570d) are listed the same way: a
'cookie' delete subtracts 4 from the pointer (lea r,[p-4] / sub r,4 / the pointer loaded from [x] and decremented
by 4 before the push) or goes through a vector deleting destructor.
"""
import argparse, bisect, collections, json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))

NEW_ARR, DEL_ARR = 0x535482, 0x53570d


def classify_new(insns, k):
    """insns[k] is the call; look at the size computation before it and the use of eax after it."""
    pre = insns[max(0, k - 14):k]
    post = insns[k + 1:k + 14]
    size_plus4 = False
    for i in pre:
        t = i.op_str
        if i.mnemonic == "add" and re.search(r", 4$", t) and not t.startswith("esp"):
            size_plus4 = True
        if i.mnemonic == "lea" and re.search(r"\+ 4\]$", t):
            size_plus4 = True
    stores = False
    for i in post:
        t = i.op_str
        if i.mnemonic == "mov" and re.match(r"dword ptr \[(eax|e[a-z]{2})\], ", t) and "eax" in t.split(",")[0]:
            stores = True
        if i.mnemonic in ("lea", "add") and re.search(r"(\[eax \+ [48]\]|^eax, [48]$)", t):
            stores = True
        if i.mnemonic == "mov" and re.search(r"\[eax - 4\]", t):
            stores = True
    return "cookie" if (size_plus4 or stores) else "plain"


def scan_range(md, code, base, sym_new, sym_del):
    """{call index: kind} for calls to the two operators in code."""
    insns = list(md.disasm(code, base))
    out = []
    for k, i in enumerate(insns):
        if i.mnemonic == "call" and i.operands and i.operands[0].type == 2:
            t = i.operands[0].imm & 0xFFFFFFFF
            if t in sym_new:
                out.append(("new", i.address, classify_new(insns, k)))
            elif t in sym_del:
                pre = insns[max(0, k - 8):k]
                m4 = any((x.mnemonic == "lea" and re.search(r"\- 4\]", x.op_str)) or
                         (x.mnemonic == "sub" and re.search(r", 4$", x.op_str)) or
                         (x.mnemonic == "add" and re.search(r", -4$|, 0xfffffffc$", x.op_str)) for x in pre)
                out.append(("del", i.address, "cookie" if m4 else "plain"))
    return out


def read_map(path):
    syms = {}
    for line in open(path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            syms[m[1]] = int(m[2], 16)
    return syms


def main():
    import capstone, pefile
    import resolve as R
    ap = argparse.ArgumentParser()
    ap.add_argument("--ours", required=True)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--va")
    a = ap.parse_args()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    img = R.Image()
    rm = json.load(open(os.path.join(a.ours, "resolve_map.json")))
    # original side, per function
    tb, ts = img.section(".text")
    starts = sorted(img.starts)
    orig = collections.defaultdict(list)
    for fn, nxt in zip(starts, starts[1:] + [tb + ts]):
        if not (tb <= fn < tb + ts):
            continue
        lo, hi = img.func_extent(fn)
        for site in scan_range(md, img.read(lo, hi - lo), lo, {NEW_ARR}, {DEL_ARR}):
            orig[fn].append(site)
    # our side: the exe + map. our function bodies run from a public symbol to the next
    exe = os.path.join(a.ours, "mb_standalone.exe")
    pe = pefile.PE(exe, fast_load=True)
    ibase = pe.OPTIONAL_HEADER.ImageBase
    syms = read_map(os.path.join(a.ours, "mb_standalone.map"))
    new_va = syms.get("??_U@YAPAXI@Z")
    del_va = syms.get("??_V@YAXPAX@Z")
    stubs_new, stubs_del = {new_va} if new_va else set(), {del_va} if del_va else set()
    # the exe may also carry the original's own stubs
    stubs_new.add(NEW_ARR)
    stubs_del.add(DEL_ARR)
    textsec = [s for s in pe.sections if s.Name.rstrip(b"\0") == b".text"][0]
    tlo = ibase + textsec.VirtualAddress
    tdata = pe.get_data(textsec.VirtualAddress, textsec.Misc_VirtualSize)
    ours_starts = sorted({v for v in syms.values() if tlo <= v < tlo + len(tdata)})
    ours_by_name = {n: v for n, v in syms.items()}
    defined = rm["defined_at"]
    rows = []
    for vas, (name, mod) in defined.items():
        va = int(vas, 16)
        ov = ours_by_name.get(name)
        if ov is None or ov not in ours_starts:
            continue
        j = ours_starts.index(ov)
        end = ours_starts[j + 1] if j + 1 < len(ours_starts) else tlo + len(tdata)
        code = tdata[ov - tlo:end - tlo]
        mine = scan_range(md, code, ov, stubs_new, stubs_del)
        theirs = orig.get(va, [])
        rows.append((va, name, mod, theirs, mine))
    n_bad = 0
    for va, name, mod, theirs, mine in sorted(rows):
        if a.va and int(a.va, 16) != va:
            continue
        sig = lambda s: sorted((k, c) for k, _a, c in s)
        if not theirs and not mine:
            continue
        bad = sig(theirs) != sig(mine)
        n_bad += bad
        if bad or a.all or a.va:
            print("%s %08x %s [%s]" % ("DIFF" if bad else "same", va, name, mod))
            print("    orig:", " ".join("%s:%s@%x" % (k, c, ad) for k, ad, c in theirs))
            print("    ours:", " ".join("%s:%s@%x" % (k, c, ad) for k, ad, c in mine))
    covered = {r[0] for r in rows}
    for fn in sorted(orig):
        if fn not in covered:
            print("UNCOVERED %08x %s (no definition of ours found): %s" % (
                fn, img.names.get(fn, ""), " ".join("%s:%s@%x" % (k, c, ad) for k, ad, c in orig[fn])))
    print("%d functions with array new/delete[], %d differ" % (sum(1 for r in rows if r[3] or r[4]), n_bad))


if __name__ == "__main__":
    main()
