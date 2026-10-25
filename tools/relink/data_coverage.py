#!/usr/bin/env python3
"""data_coverage.py: how much of the original's static data (.rdata, .data, .bss) our source defines.

usage: data_coverage.py [--record] [--map work/relink/resolve_map.json] [--json OUT]
       (run tools/relink/resolve_map.py first; --record appends a point to docs/data_history.json, which
        tools/progress.py prints)

Function coverage says nothing about globals: a module that declares `extern int g_x; // 0x5e2464` compiles and
can match byte-exact while nothing in src/ defines g_x. The standalone link then takes that storage from the
original image (tools/relink/glue.py). This tool measures the split.

Items: the original image has no symbols, so data items are recovered from their uses. Every address in
.rdata/.data/.bss that an instruction of the original uses (a memory operand's displacement, or a 4-aligned
immediate), or that our resolution map names, starts an item; an item extends to the next start, so storage that
pointer tables in the data reach (runs of 4+ pointers: the pages of a pool) starts its own items.
An address that game code uses but that our resolution map does not name (a field offset into a global, such as
label+4) is taken as part of the item before it. Sizes are therefore approximate: an item absorbs any padding and
any unreferenced data up to the next start.

Classes, per item:
  defined in source       our source defines a global at this address (a named definition the resolver mapped)
  generated from source   our objects define it as compiler output: string literals, floating-point constants,
                          vtables and RTTI
  borrowed from image     game code uses it, our source only declares it (extern) or names it nowhere
  reached through tables  no instruction uses it; a pointer table in the data does (the pages of a pool)
  library data            used only by the static CRT/C++ library (linked from libcmt/libcpmt, not decompiled)
The import address table is excluded.
"""
import argparse, bisect, collections, contextlib, datetime, io, json, os, re, runpy, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

HIST = os.path.join(ROOT, "docs", "data_history.json")
CLASSES = ("defined in source", "generated from source", "borrowed from image", "reached through tables", "library data")
GENERATED = ("??_C@", "__real@", "__int64@", "__xmm@", "__mask@", "??_7", "??_R", "$SG")


def game_functions():
    argv, sys.argv = sys.argv, [os.path.join(ROOT, "tools", "status.py"), "--no-write"]
    with contextlib.redirect_stdout(io.StringIO()):
        st = runpy.run_path(os.path.join(ROOT, "tools", "status.py"))
    sys.argv = argv
    return st["starts"], st["game"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    ap.add_argument("--json")
    a = ap.parse_args()
    os.chdir(ROOT)
    import capstone
    import resolve as R
    from glue import RDATA, DATA, BSS, IAT_LO, IAT_HI, section_of
    img = R.Image()
    starts, game = game_functions()
    lo_all, hi_all = RDATA[1], BSS[2]

    def in_data(v):
        return lo_all <= v < hi_all and not IAT_LO <= v < IAT_HI and section_of(v) is not None

    # uses by instructions: address -> {"game", "lib"}. A memory operand's displacement always counts; a bare
    # immediate (push / mov reg, imm) only when 4-aligned, since most unaligned ones are constants, not addresses
    uses = collections.defaultdict(set)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    tok = re.compile(r"\[[^\]]*\]|0x[0-9a-f]+")
    t0, tn = img.section(".text")
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else t0 + tn
        owner = "game" if s in game else "lib"
        for ins in md.disasm(img.read(s, e - s), s):
            for t in tok.findall(ins.op_str):
                mem = t.startswith("[")
                for h in re.findall(r"0x([0-9a-f]+)", t):
                    v = int(h, 16)
                    # library immediates into .bss are constants (strlen's 0x01010100 masks), not addresses
                    if in_data(v) and (mem or (v % 4 == 0 and (owner == "game" or section_of(v) is not BSS))):
                        uses[v].add(owner)

    # pointer tables: runs of 4+ consecutive dwords that all point into the data (a page table of a pool, a list of
    # strings); a lone dword that happens to hold such a value is more likely an integer
    tabled = set()
    for sec in (RDATA, DATA):
        data = img.read(sec[1], sec[2] - sec[1])
        run = []
        for off in range(0, len(data) - 3, 4):
            v = struct.unpack_from("<I", data, off)[0]
            if in_data(v) and v % 4 == 0 and not IAT_LO <= sec[1] + off < IAT_HI:
                run.append(v)
                continue
            if len(run) >= 4:
                tabled.update(run)
            run = []
        if len(run) >= 4:
            tabled.update(run)

    rmap = json.load(open(a.map))
    named = collections.defaultdict(lambda: [False, []])      # va -> [defined anywhere, names]
    for n, d in rmap["names"].items():
        for va, e in d.items():
            v = int(va, 16)
            if in_data(v):
                named[v][0] |= e["defined"]
                named[v][1].append(n)

    def classify(v):
        u = uses.get(v, set())
        if "game" in u or v in named:
            if v in named and named[v][0]:
                defs = named[v][1]
                return "generated from source" if all(n.startswith(GENERATED) for n in defs) else "defined in source"
            return "borrowed from image"
        if u:
            return "library data"
        return "reached through tables"

    bounds = sorted(set(uses) | set(named) | tabled)
    items = []                       # [start, end, class]
    for i, v in enumerate(bounds):
        sec = section_of(v)
        nxt = bounds[i + 1] if i + 1 < len(bounds) and bounds[i + 1] < sec[2] else sec[2]
        cls = classify(v)
        unnamed_game = "game" in uses.get(v, set()) and v not in named
        if unnamed_game and items and items[-1][1] == v and section_of(items[-1][0]) is sec:
            items[-1][1] = nxt       # a field of the item before
            continue
        items.append([v, nxt, cls])
    by = {c: [0, 0] for c in CLASSES}
    per_sec = {s[0]: {c: 0 for c in CLASSES} for s in (RDATA, DATA, BSS)}
    for s, e, c in items:
        by[c][0] += 1
        by[c][1] += e - s
        per_sec[section_of(s)[0]][c] += e - s
    total = sum(s[2] - s[1] for s in (RDATA, DATA, BSS)) - (IAT_HI - IAT_LO)
    covered = sum(b for _n, b in by.values())
    rep = {"when": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
           "commit": subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip(),
           "items": {c: by[c][0] for c in CLASSES}, "bytes": {c: by[c][1] for c in CLASSES},
           "section_bytes": {k.split("$")[0]: v for k, v in per_sec.items()},
           "unreferenced_bytes": total - covered, "total_bytes": total}
    print("static data: %d items, %.1f MB (.rdata %d KB, .data %d KB initialised, .bss %.1f MB)" % (
        len(items), total / 1e6, (RDATA[2] - RDATA[1]) // 1024, (DATA[2] - DATA[1]) // 1024, (BSS[2] - BSS[1]) / 1e6))
    for c in CLASSES:
        print("  %-22s %6d items  %10d bytes" % (c, by[c][0], by[c][1]))
    print("  %-22s %6s        %10d bytes" % ("before the first item", "", total - covered))
    if a.json:
        json.dump(dict(rep, item_list=[["%08x" % s, e - s, c] for s, e, c in items]), open(a.json, "w"), indent=1)
    if a.record:
        try:
            hist = json.load(open(HIST))
        except (OSError, ValueError):
            hist = {}
        hist[rep["when"]] = rep
        with open(HIST, "w") as f:
            json.dump(dict(sorted(hist.items())), f, indent=1)
            f.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
