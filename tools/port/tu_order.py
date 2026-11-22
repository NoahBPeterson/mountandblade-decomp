#!/usr/bin/env python3
"""Write port/link/tu_order.txt: every src/ TU in the original image's link order (its lowest row VA).

The original's static constructors run in .CRT$XCU order, which is the link order of the objects, and its static
destructors run in the reverse order (atexit). ld64 / GNU ld run a native build's initialisers in input-file order,
so port/cmake/native_link.cmake links the src/ objects in this order: a global destroyed at exit then still sees the
globals it uses (g_gui's widgets release meshes that g_resmgr owns: gui_manager.cpp is after resmgr.cpp in the image).
Rows: <tu>.manifest.txt (VA in column 3) and <tu>.nonmatching.txt (VA in column 1). TUs with no row go last, by path.

usage: tu_order.py [--check]     (--check: exit 1 if the file is out of date)
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import sidecars as SC  # noqa: E402  (where the sidecar files live)
OUT = os.path.join(ROOT, "port", "link", "tu_order.txt")
HEX = re.compile(r"^[0-9a-fA-F]{8}$")


def lowest_va(cpp):
    best = None
    for path, col in ((SC.manifest_for(cpp), 2), (SC.nonmatching_for(cpp), 0)):
        if not os.path.exists(path):
            continue
        for line in open(path, encoding="utf-8", errors="replace"):
            f = line.split()
            if len(f) > col and HEX.match(f[col]):
                va = int(f[col], 16)
                best = va if best is None else min(best, va)
    return best


def main():
    tus = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    tus = [t for t in tus if "/_dataaudit_" not in t]
    keyed = []
    for t in tus:
        va = lowest_va(t)
        keyed.append((va is None, va or 0, os.path.relpath(t, ROOT)))
    keyed.sort()
    text = "".join("%s\n" % rel for _, _, rel in keyed)
    if "--check" in sys.argv:
        sys.exit(0 if os.path.exists(OUT) and open(OUT).read() == text else 1)
    open(OUT, "w").write(text)
    print("wrote %s: %d TUs (%d without rows, last)" % (os.path.relpath(OUT, ROOT), len(keyed), sum(k[0] for k in keyed)))


if __name__ == "__main__":
    main()
