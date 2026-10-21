#!/usr/bin/env python3
"""Create functions at given VAs (disassembling first if needed) in the mb Ghidra project.
Used for code pointers Ghidra's analysis and fixups.py miss, e.g. callbacks loaded as immediates
(`mov [wc.lpfnWndProc], 0x401000`): the image has no .reloc to find them by.
usage: create_functions.py <binary> <project_dir> <project_name> <va-hex> [...]"""
import sys, pyghidra
pyghidra.start()
binary, pdir, name, vas = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
with pyghidra.open_program(binary, project_location=pdir, project_name=name, analyze=False) as flat:
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    p, mon = flat.getCurrentProgram(), flat.getMonitor()
    tx = p.startTransaction("create functions")
    try:
        for v in vas:
            a = flat.toAddr(int(v, 16))
            if p.getFunctionManager().getFunctionAt(a) is not None:
                print("%s: exists" % v); continue
            if p.getListing().getInstructionAt(a) is None:
                DisassembleCommand(a, None, True).applyTo(p, mon)
            ok = CreateFunctionCmd(a).applyTo(p, mon)
            f = p.getFunctionManager().getFunctionAt(a)
            print("%s: %s (%d bytes)" % (v, "created" if ok else "FAILED", f.getBody().getNumAddresses() if f else 0))
    finally:
        p.endTransaction(tx, True)
