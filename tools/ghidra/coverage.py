#!/usr/bin/env python3
"""Report .text coverage: bytes in function bodies, instructions outside any function, undefined bytes.
Usage: ghidra_coverage.py <binary> <project_dir> <project_name>"""
import sys, pyghidra
pyghidra.start()
with pyghidra.open_program(sys.argv[1], project_location=sys.argv[2], project_name=sys.argv[3], analyze=False) as flat:
    from ghidra.program.model.address import AddressSet
    p = flat.getCurrentProgram(); listing = p.getListing(); fm = p.getFunctionManager()
    t = p.getMemory().getBlock(".text"); text = AddressSet(t.getStart(), t.getEnd())
    body = AddressSet()
    for f in fm.getFunctions(True): body.add(f.getBody())
    inb = body.intersect(text)
    out = text.subtract(body)
    n_out = 0; bytes_out = 0
    for ins in listing.getInstructions(out, True): n_out += 1; bytes_out += ins.getLength()
    undef = 0; undef_nonpad = 0
    mem = p.getMemory()
    for rng in listing.getUndefinedRanges(out, False, flat.getMonitor()):
        undef += rng.getLength()
    data = 0
    for d in listing.getDefinedData(out, True): data += d.getLength()
    print("text=%d in_functions=%d outside=%d | outside: instr=%d (%d bytes) defined_data=%d undefined=%d"
          % (text.getNumAddresses(), inb.getNumAddresses(), out.getNumAddresses(), n_out, bytes_out, data, undef))
