#!/usr/bin/env python3
"""Decompile every function into work/decomp/c/<64KiB block>.c plus work/decomp/index.csv
(address,name,bytes,status,lines). Jump-table limit raised to 4096 (the script interpreter at
0x49fa10 has 2356 cases; Ghidra's default 1024 silently truncates it).
usage: export_decomp.py <binary> <project_dir> <project_name> <out_dir>"""
import os, sys, pyghidra
pyghidra.start()
binary, pdir, name, out = sys.argv[1:5]
os.makedirs(os.path.join(out, "c"), exist_ok=True)
with pyghidra.open_program(binary, project_location=pdir, project_name=name, analyze=False) as flat:
    from ghidra.app.decompiler import DecompInterface, DecompileOptions
    prog, mon = flat.getCurrentProgram(), flat.getMonitor()
    opts = DecompileOptions(); opts.grabFromProgram(prog)
    opts.setMaxJumpTableEntries(4096); opts.setMaxInstructions(200000); opts.setMaxPayloadMBytes(512)
    di = DecompInterface(); di.setOptions(opts); di.toggleCCode(True); di.toggleSyntaxTree(False)
    di.setSimplificationStyle("decompile"); di.openProgram(prog)
    idx = open(os.path.join(out, "index.csv"), "w")
    idx.write("address,name,bytes,status,lines\n")
    cur, fh = None, None
    for f in prog.getFunctionManager().getFunctions(True):
        if f.isExternal():
            continue
        ep = f.getEntryPoint().getOffset()
        if ep >> 16 != cur:
            fh and fh.close()
            cur = ep >> 16
            fh = open(os.path.join(out, "c", "%04x0000.c" % cur), "w")
        status = "ok"
        try:
            res = di.decompileFunction(f, 600 if f.getBody().getNumAddresses() > 20000 else 60, mon)
            if res is not None and res.decompileCompleted():
                code = res.getDecompiledFunction().getC()
            else:
                status, code = "fail", "/* decompile failed: %s */\n" % (res.getErrorMessage() if res else "null")
        except Exception as e:
            status, code = "error", "/* exception: %s */\n" % e
        fh.write("/* ==== %s @ %08x ==== */\n%s\n" % (f.getName(True), ep, code))
        idx.write("%08x,%s,%d,%s,%d\n" % (ep, f.getName(True).replace(",", ";"), f.getBody().getNumAddresses(),
                                          status, code.count("\n")))
    fh and fh.close(); idx.close()
