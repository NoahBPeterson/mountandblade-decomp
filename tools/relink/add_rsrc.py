#!/usr/bin/env python3
"""add_rsrc.py: give a linked exe the original image's resources (dialogs, icons, version info, string tables).

usage: add_rsrc.py EXE [--from work/mb.analysis.bin]

The toolchain has no cvtres.exe, and the resources exist only as the original's compiled .rsrc section, so the
section is appended to EXE as it is: the resource directory tree is position-independent (its offsets are relative
to the section), except each IMAGE_RESOURCE_DATA_ENTRY's OffsetToData, an RVA, which is rebased to the new section.
Updates the resource data directory, NumberOfSections and SizeOfImage. EXE must not already have resources.
"""
import argparse, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def pe_layout(d):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec, = struct.unpack_from("<H", d, pe + 6)
    optsz, = struct.unpack_from("<H", d, pe + 20)
    opt = pe + 24
    secs = []
    for i in range(nsec):
        o = opt + optsz + 40 * i
        name = d[o:o + 8].rstrip(b"\0")
        vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, o + 8)
        secs.append((name, vsize, va, rsize, rptr, o))
    return pe, opt, optsz, secs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("--from", dest="src", default=os.path.join(ROOT, "work", "mb.analysis.bin"))
    a = ap.parse_args()
    s = open(a.src, "rb").read()
    _pe, sopt, _sz, ssecs = pe_layout(s)
    rs = next(x for x in ssecs if x[0] == b".rsrc")
    old_rva, size = struct.unpack_from("<II", s, sopt + 96 + 8 * 2)
    raw = bytearray(s[rs[4]:rs[4] + rs[3]])
    base_off = old_rva - rs[2]            # resource directory offset inside the section (normally 0)

    d = bytearray(open(a.exe, "rb").read())
    pe, opt, optsz, secs = pe_layout(d)
    if struct.unpack_from("<II", d, opt + 96 + 8 * 2)[1]:
        sys.exit("%s already has resources" % a.exe)
    salign, falign = struct.unpack_from("<II", d, opt + 32)
    hdr_end = secs[-1][5] + 40
    first_raw = min(x[4] for x in secs if x[4])
    if hdr_end + 40 > first_raw:
        sys.exit("no room for another section header")
    up = lambda v, al: (v + al - 1) // al * al
    new_va = up(max(x[2] + max(x[1], x[3]) for x in secs), salign)
    new_raw = up(len(d), falign)

    # rebase every data entry: walk the tree
    def walk(off, depth):
        _ch, _ts, _maj, _min, nnamed, nid = struct.unpack_from("<IIHHHH", raw, base_off + off)
        for i in range(nnamed + nid):
            _name, ent = struct.unpack_from("<II", raw, base_off + off + 16 + 8 * i)
            if ent & 0x80000000:
                walk(ent & 0x7FFFFFFF, depth + 1)
            else:
                rva, = struct.unpack_from("<I", raw, base_off + ent)
                struct.pack_into("<I", raw, base_off + ent, rva - rs[2] + new_va)
    walk(0, 0)

    data = bytes(raw) + b"\0" * (up(len(raw), falign) - len(raw))
    d += b"\0" * (new_raw - len(d)) + data
    hdr = struct.pack("<8sIIIIIIHHI", b".rsrc", rs[1], new_va, len(data), new_raw, 0, 0, 0, 0, 0x40000040)
    d[hdr_end:hdr_end + 40] = hdr
    struct.pack_into("<H", d, pe + 6, len(secs) + 1)
    struct.pack_into("<I", d, opt + 56, up(new_va + rs[1], salign))          # SizeOfImage
    struct.pack_into("<II", d, opt + 96 + 8 * 2, new_va + base_off, size)     # resource directory
    struct.pack_into("<I", d, opt + 64, 0)                                    # CheckSum (not verified for exes)
    open(a.exe, "wb").write(d)
    print("%s: .rsrc (%d bytes) at RVA %#x" % (a.exe, rs[1], new_va))
    return 0


if __name__ == "__main__":
    sys.exit(main())
