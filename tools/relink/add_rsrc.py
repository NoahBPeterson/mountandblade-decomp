#!/usr/bin/env python3
"""add_rsrc.py: give a linked exe its resources (dialogs, placeholder icons, version info, string tables).

usage: add_rsrc.py EXE [--res resources]

The toolchain has no cvtres.exe, so rsrc_repo.py assembles the .rsrc section from the files in resources/ (see its docstring),
for the RVA the section gets here, and it is appended to EXE: new section header, resource data directory, NumberOfSections
and SizeOfImage are updated from the section's actual size. EXE must not already have resources. Needs no game exe.
"""
import argparse, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rsrc_repo

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
    ap.add_argument("--res", default=rsrc_repo.RES)
    a = ap.parse_args()

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

    raw = rsrc_repo.build_section(new_va, a.res)
    vsize = len(raw)
    data = raw + b"\0" * (up(vsize, falign) - vsize)
    d += b"\0" * (new_raw - len(d)) + data
    hdr = struct.pack("<8sIIIIIIHHI", b".rsrc", vsize, new_va, len(data), new_raw, 0, 0, 0, 0, 0x40000040)
    d[hdr_end:hdr_end + 40] = hdr
    struct.pack_into("<H", d, pe + 6, len(secs) + 1)
    struct.pack_into("<I", d, opt + 56, up(new_va + vsize, salign))          # SizeOfImage
    struct.pack_into("<II", d, opt + 96 + 8 * 2, new_va, vsize)               # resource directory
    struct.pack_into("<I", d, opt + 64, 0)                                    # CheckSum (not verified for exes)
    open(a.exe, "wb").write(d)
    print("%s: .rsrc (%d bytes) at RVA %#x" % (a.exe, vsize, new_va))
    return 0


if __name__ == "__main__":
    sys.exit(main())
