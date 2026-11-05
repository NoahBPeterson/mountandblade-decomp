#!/usr/bin/env python3
"""guard_copy.py: mark the PE sections of a MB_COPY_GUARD=1 build no-access, so any read or write of the copy of the original's
data crashes with its address. The relocation scan cannot see a computed or out-of-bounds access; this does, for the code a run reaches.

usage: MB_COPY_GUARD=1 build.py --faithful --out DIR; guard_copy.py DIR/mb_standalone.exe (prints the sections' ranges)
"""
import struct, sys

GUARD = (b".mbro", b".mbrw", b".mbzi")
RWX = 0x20000000 | 0x40000000 | 0x80000000

d = bytearray(open(sys.argv[1], "rb").read())
pe = struct.unpack_from("<I", d, 0x3c)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
opt = struct.unpack_from("<H", d, pe + 20)[0]
base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
n = 0
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    name = bytes(d[o:o + 8]).rstrip(b"\0")
    vsize, va = struct.unpack_from("<II", d, o + 8)
    print("%-8s %08x..%08x" % (name.decode(), base + va, base + va + vsize))
    if name in GUARD:
        c = struct.unpack_from("<I", d, o + 36)[0]
        struct.pack_into("<I", d, o + 36, c & ~RWX)
        n += 1
if n != len(GUARD):
    sys.exit("%d of %d guard sections found: build with MB_COPY_GUARD=1" % (n, len(GUARD)))
open(sys.argv[1], "wb").write(d)
print("%d sections set no-access" % n)
