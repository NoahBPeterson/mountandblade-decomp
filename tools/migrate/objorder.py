#!/usr/bin/env python3
"""Print an object's sections in header order (the order link 7.10 lays them out within the object).

usage: objorder.py <file.obj> [--all]   (default: skip .debug/.drectve sections)
"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "difftest"))
import coff

o = coff.Coff(sys.argv[1])
by_sec = {}
for s in o.syms.values():
    if s.secno > 0 and s.cls in (2, 3, 6) and not s.name.startswith((".", "$")):
        by_sec.setdefault(s.secno, []).append(s.name)
for sec in o.sections:
    if sec.discard and "--all" not in sys.argv:
        continue
    print("%3d %-10s %6d align%-3d sel%d %s" % (sec.index + 1, sec.name, sec.size, sec.align, sec.comdat_sel,
                                           " ".join(by_sec.get(sec.index + 1, [])[:3])))
