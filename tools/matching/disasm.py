#!/usr/bin/env python3
"""Print the original instructions of a function. usage: disasm.py <va-hex> [len]"""
import csv, os, sys
import capstone, pefile
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
va = int(sys.argv[1], 16)
n = int(sys.argv[2], 0) if len(sys.argv) > 2 else next(
    (int(r["size_bytes"]) for r in csv.DictReader(open(os.path.join(ROOT, "work/index/functions.csv")))
     if int(r["address"], 16) == va), 256)
pe = pefile.PE(os.path.join(ROOT, "work/mb.analysis.bin"), fast_load=True)
code = pe.get_data(va - 0x400000, n)
for i in capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32).disasm(code, va):
    print("%08x  %-24s %s %s" % (i.address, i.bytes.hex(), i.mnemonic, i.op_str))
