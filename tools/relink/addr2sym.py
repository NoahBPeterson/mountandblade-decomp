#!/usr/bin/env python3
"""addr2sym.py: name addresses of the standalone exe (a crash backtrace) from its link map.

usage: addr2sym.py ADDR... [--map work/relink/standalone/mb_standalone.map] [-d]

Prints the function (symbol + offset, object) containing each address; -d also disassembles the instructions
leading up to it (for a return address: the call that was made).
"""
import argparse, bisect, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addrs", nargs="+")
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "standalone", "mb_standalone.map"))
    ap.add_argument("-d", action="store_true")
    a = ap.parse_args()
    syms = []
    for line in open(a.map, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)", line)
        if m:
            syms.append((int(m[2], 16), m[1], m[3]))
    syms.sort()
    starts = [s[0] for s in syms]
    exe = a.map[:-4] + ".exe"
    for t in a.addrs:
        v = int(t, 16)
        i = bisect.bisect_right(starts, v) - 1
        s = syms[i] if i >= 0 else (0, "?", "?")
        print("%08x  %s+%#x  (%s)" % (v, s[1], v - s[0], s[2]))
        if a.d:
            import capstone, pefile
            pe = pefile.PE(exe, fast_load=True)
            md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
            lo = max(s[0], v - 0x30)
            for ins in md.disasm(pe.get_data(lo - pe.OPTIONAL_HEADER.ImageBase, v - lo + 8), lo):
                print("    %s%08x  %s %s" % ("=> " if ins.address == v else "   ", ins.address, ins.mnemonic, ins.op_str))
    return 0


if __name__ == "__main__":
    sys.exit(main())
