#!/usr/bin/env python3
"""Byte-exact verifier for 32-bit x86: compare one function in a COFF .obj against the
original image at a VA, masking relocated bytes on both sides.

usage: cmpobj.py <image> <obj> <symbol> <va-hex> [--len N] [--quiet]

- Our side: bytes covered by IMAGE_REL_I386_DIR32/DIR32NB/REL32/SECREL relocations are masked.
- Original side: bytes covered by base relocations (.reloc, absolute addresses) are masked.
- Length defaults to the size of the symbol's COMDAT section (compile with /Gy).
Exit 0 iff all unmasked bytes are identical.
"""
import argparse, bisect, os, struct, sys
import pefile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import imgcache

REL_SIZES = {0x06: 4, 0x07: 4, 0x14: 4, 0x0B: 4, 0x0A: 2}  # DIR32, DIR32NB, REL32, SECREL, SECTION


def coff_constants(path):
    """{symbol-table index: expected bytes} for the compiler constants an object references:
    `__real@<hex>` (float/double, value in the name), `__int64@<hex>`, and `??_C@` string literals
    (bytes from their COMDAT up to the terminator)."""
    d = open(path, "rb").read()
    _, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + nsym * 18
    out = {}
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        raw = d[o:o + 8]
        if raw[:4] == b"\0\0\0\0":
            so = strtab + struct.unpack_from("<I", raw, 4)[0]
            nm = d[so:d.index(b"\0", so)]
        else:
            nm = raw.split(b"\0")[0]
        value, secno, _typ, _cls, naux = struct.unpack_from("<IHHBB", d, o + 8)
        if nm.startswith((b"__real@", b"__int64@")):
            hx = nm.split(b"@", 1)[1].decode()
            try:
                out[i] = int(hx, 16).to_bytes(len(hx) // 2, "little")
            except ValueError:
                pass
        elif nm.startswith(b"??_C@") and 0 < secno < 0xFF00:
            so = 20 + optsz + 40 * (secno - 1)
            size, ptr = struct.unpack_from("<II", d, so + 16)
            blob = d[ptr + value:ptr + size]
            wide = nm.startswith(b"??_C@_1")
            end = next((k for k in range(0, len(blob) - 1, 2) if blob[k:k + 2] == b"\0\0"), len(blob)) + 2 \
                if wide else blob.find(b"\0") + 1
            if end > 0:
                out[i] = blob[:end]
        i += 1 + naux
    return out


def constant_diffs(consts, relocs, off, theirs, read, mine=None):
    """Masked relocations hide WHICH constant an instruction uses: `fcom [eps]` matches whether eps is 0.0
    or 1e-10. mount&blade.exe has no .reloc, so a DIR32 operand in the original is the absolute address;
    read what it points at and compare it with the constant our object references. -> list of offsets
    (relative to the function) of mismatching constant operands. `read(va, n)` reads the image.
    `mine` (our function's bytes) supplies each DIR32's addend: copying a string literal with dword loads
    references literal+4, +8, ..., so the original's operand points that far into the same constant."""
    bad = []
    for vaddr, symidx, typ in relocs:
        if typ != 0x06 or symidx not in consts or not 0 <= vaddr - off <= len(theirs) - 4:
            continue
        want = consts[symidx]
        addend = struct.unpack_from("<i", mine, vaddr - off)[0] if mine is not None and vaddr - off + 4 <= len(mine) else 0
        if not 0 <= addend < len(want):
            addend = 0
        va = struct.unpack_from("<I", theirs, vaddr - off)[0] - addend
        try:
            have = read(va, len(want))
        except Exception:
            have = None
        if have != want:
            bad.append(vaddr - off)
    return bad


def parse_coff(path):
    d = open(path, "rb").read()
    machine, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    if machine != 0x14C:
        raise SystemExit("not an x86 COFF object")
    strtab = symptr + nsym * 18
    def name_of(raw):
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            return d[strtab + off: d.index(b"\0", strtab + off)].decode()
        return raw.split(b"\0")[0].decode()
    secs = []
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        raw = d[o:o + 8]
        size, ptr, relptr, _, nrel = struct.unpack_from("<IIIIH", d, o + 16)
        nm = raw.split(b"\0")[0].decode()
        if nm.startswith("/"):
            off = int(nm[1:]); nm = d[strtab + off: d.index(b"\0", strtab + off)].decode()
        relocs = [struct.unpack_from("<IIH", d, relptr + 10 * k) for k in range(nrel)]
        secs.append({"name": nm, "data": d[ptr:ptr + size] if ptr else b"\0" * size, "relocs": relocs})
    syms = {}
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        value, secno, typ, cls, naux = struct.unpack_from("<IHHBB", d, o + 8)
        secno = secno if secno < 0xFF00 else secno - 0x10000
        nm = name_of(d[o:o + 8])
        if secno > 0 and cls in (2, 3) and (typ >> 4) == 2:  # function symbols
            syms.setdefault(nm, (secno - 1, value))
        i += 1 + naux
    return secs, syms

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image"); ap.add_argument("obj"); ap.add_argument("symbol"); ap.add_argument("va")
    ap.add_argument("--len", type=int); ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args()
    secs, syms = parse_coff(a.obj)
    if a.symbol not in syms:
        cands = [s for s in syms if a.symbol in s]
        if len(cands) != 1:
            raise SystemExit("symbol %r not found; candidates: %s" % (a.symbol, ", ".join(sorted(syms))))
        a.symbol = cands[0]
    si, off = syms[a.symbol]
    sec = secs[si]
    length = a.len or (len(sec["data"]) - off)
    mine = sec["data"][off:off + length]
    mask_mine = set()
    for vaddr, _sym, typ in sec["relocs"]:
        n = REL_SIZES.get(typ, 4)
        for k in range(n):
            if off <= vaddr + k < off + length:
                mask_mine.add(vaddr + k - off)

    pe = pefile.PE(a.image, fast_load=True)
    va = int(a.va, 16)
    rva = va - pe.OPTIONAL_HEADER.ImageBase
    orig = pe.get_data(rva, length)
    mask_orig = set()
    rel = imgcache.relocs(a.image)  # sorted HIGHLOW reloc RVAs, cached
    i = bisect.bisect_left(rel, rva - 3)
    while i < len(rel) and rel[i] < rva + length:
        for k in range(4):
            if 0 <= rel[i] + k - rva < length:
                mask_orig.add(rel[i] + k - rva)
        i += 1
    mask = mask_mine | mask_orig
    diffs = [i for i in range(length) if i not in mask and orig[i] != mine[i]]
    read = lambda v, n: pe.get_data(v - pe.OPTIONAL_HEADER.ImageBase, n)
    diffs += constant_diffs(coff_constants(a.obj), sec["relocs"], off, orig, read, mine)
    ok = not diffs and len(mine) == length
    if not a.quiet or not ok:
        print("%s @ %08x len %d: %s (%d diff bytes, %d masked)" % (
            a.symbol, va, length, "MATCH" if ok else "DIFF", len(diffs), len(mask)))
    if not ok and not a.quiet:
        def row(b, m):
            return " ".join("..".format() if i in m else "%02x" % b[i] for i in range(len(b)))
        print(" orig:", row(orig, mask))
        print(" mine:", row(mine, mask))
        print(" diff@", diffs[:32])
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
