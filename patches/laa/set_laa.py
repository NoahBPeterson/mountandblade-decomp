#!/usr/bin/env python3
"""set_laa.py: SKETCH, untested in the game. Set IMAGE_FILE_LARGE_ADDRESS_AWARE (0x0020) in a PE's COFF header.

usage: set_laa.py IN.exe OUT.exe [--clear]

Ticket docs/tickets/020-large-address-aware.md. The relinked exe (work/relink/standalone/mb_standalone.exe) and the
unpacked original both have Characteristics 0x010e / 0x010f, i.e. the bit is clear. This is the byte-level equivalent of
adding /LARGEADDRESSAWARE to the link line in tools/relink/build.py (link_extra, ~line 414): use the link flag for real
builds; use this script to try the effect on an existing exe without relinking.

Only touches the 2-byte Characteristics field (and zeroes nothing else). The optional-header CheckSum is 0 in these
images (link 7.10 without /RELEASE), so it needs no fix-up; the script reports it so a non-zero one can be recomputed.
"""
import struct
import sys

IMAGE_FILE_LARGE_ADDRESS_AWARE = 0x0020


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    src, dst = argv[1], argv[2]
    clear = "--clear" in argv[3:]
    d = bytearray(open(src, "rb").read())
    if d[:2] != b"MZ":
        sys.exit("not a PE")
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    if d[pe:pe + 4] != b"PE\0\0":
        sys.exit("bad PE signature")
    chars_off = pe + 4 + 18                     # IMAGE_FILE_HEADER.Characteristics
    old = struct.unpack_from("<H", d, chars_off)[0]
    new = (old & ~IMAGE_FILE_LARGE_ADDRESS_AWARE) if clear else (old | IMAGE_FILE_LARGE_ADDRESS_AWARE)
    struct.pack_into("<H", d, chars_off, new)
    csum_off = pe + 4 + 20 + 64                 # IMAGE_OPTIONAL_HEADER32.CheckSum
    csum = struct.unpack_from("<I", d, csum_off)[0]
    open(dst, "wb").write(d)
    print("Characteristics %#06x -> %#06x (file offset %#x); CheckSum field %#x%s" % (
        old, new, chars_off, csum, "" if csum == 0 else "  (non-zero: recompute, e.g. with pefile.generate_checksum)"))


if __name__ == "__main__":
    main(sys.argv)
