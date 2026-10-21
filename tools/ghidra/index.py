#!/usr/bin/env python3
"""Ghidra function census (pyghidra, standalone).

  new:      ghidra_index.py new <binary> <project_dir> <name> <oep_hex> <out.csv>
            import + set OEP + full analysis + Spore-style discovery pass + re-analysis + export
  existing: ghidra_index.py existing <binary> <project_dir> <name> <out.csv>
            open the already-analyzed program read-only and export

CSV: address,name,size_bytes,instructions,thunk,block   (one row per function)
"""
import sys
import pyghidra

pyghidra.start()


def discover(prog, monitor):
    import jpype
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    mem, fm, listing = prog.getMemory(), prog.getFunctionManager(), prog.getListing()
    text = mem.getBlock(".text")
    lo = text.getStart()
    space = prog.getAddressFactory().getDefaultAddressSpace()
    cands = set()
    for r in prog.getRelocationTable().getRelocations():
        try:
            v = mem.getInt(r.getAddress()) & 0xFFFFFFFF
        except Exception:
            continue
        if text.contains(space.getAddress(v)):
            cands.add(v)
    jdata = jpype.JByte[int(text.getSize())]
    text.getBytes(lo, jdata)
    data = bytes(b & 0xFF for b in jdata)
    base = lo.getOffset()
    for i in range(16, len(data) - 1, 16):
        if data[i - 1] == 0xCC and data[i] != 0xCC:
            cands.add(base + i)
    made = 0
    for v in sorted(cands):
        a = space.getAddress(v)
        if fm.getFunctionContaining(a) is not None:
            continue
        if listing.getInstructionAt(a) is None:
            if listing.getDefinedDataContaining(a) is not None or listing.getInstructionContaining(a) is not None:
                continue
            DisassembleCommand(a, None, True).applyTo(prog, monitor)
            if listing.getInstructionAt(a) is None:
                continue
        if CreateFunctionCmd(a).applyTo(prog, monitor):
            made += 1
    print("discovery: candidates=%d created=%d" % (len(cands), made))


def export(prog, out):
    listing, mem = prog.getListing(), prog.getMemory()
    with open(out, "w") as fh:
        fh.write("address,name,size_bytes,instructions,thunk,block\n")
        for f in prog.getFunctionManager().getFunctions(True):
            body = f.getBody()
            n = sum(1 for _ in listing.getInstructions(body, True))
            blk = mem.getBlock(f.getEntryPoint())
            fh.write("%s,%s,%d,%d,%d,%s\n" % (f.getEntryPoint(), f.getName().replace(",", ";"),
                                              body.getNumAddresses(), n, int(f.isThunk()),
                                              blk.getName() if blk else "EXTERNAL"))


mode = sys.argv[1]
if mode == "new":
    binary, pdir, name, oep, out = sys.argv[2:7]
    with pyghidra.open_program(binary, project_location=pdir, project_name=name, analyze=False) as flat:
        from ghidra.app.cmd.disassemble import DisassembleCommand
        from ghidra.program.model.symbol import SourceType
        from ghidra.program.util import GhidraProgramUtilities
        prog, monitor = flat.getCurrentProgram(), flat.getMonitor()
        a = flat.toAddr(int(oep, 16))
        DisassembleCommand(a, None, True).applyTo(prog, monitor)
        f = flat.getFunctionAt(a) or flat.createFunction(a, "entry_oep")
        f.setName("entry_oep", SourceType.USER_DEFINED)
        prog.getSymbolTable().addExternalEntryPoint(a)
        pyghidra.analyze(prog, monitor)
        discover(prog, monitor)
        pyghidra.analyze(prog, monitor)
        GhidraProgramUtilities.markProgramAnalyzed(prog)
        print("functions:", prog.getFunctionManager().getFunctionCount())
        export(prog, out)
else:
    binary, pdir, name, out = sys.argv[2:6]
    with pyghidra.open_program(binary, project_location=pdir, project_name=name, analyze=False) as flat:
        export(flat.getCurrentProgram(), out)
        print("functions:", flat.getCurrentProgram().getFunctionManager().getFunctionCount())
