#!/usr/bin/env python3
"""resolve_map.py: the original address of every symbol our module objects reference, as the equivalence harness
resolves it, for the relink glue (tools/relink/glue.py).

usage: resolve_map.py [--jobs N] [--out work/relink/resolve_map.json]

For every row of every src/ module (manifest and nonmatching), compiles the module (cached), finds the row's
symbol and runs the harness resolver (tools/difftest/resolve.py) on it. Records, per referenced symbol name, the
addresses it resolved to (with the method and the rows that saw it), and per VA the symbol our source defines
there. A name that resolved to two addresses is a conflict: glue.py refuses to alias it.

Vtables: our compiled vtables reference virtual functions no function body names, so their slots are mapped by
position: our ??_7X@@6B@ resolved to the original vtable at V, so slot i stands for the dword at V + 4i.
"vtables" lists each of our vtables with its original address and its slot count (for build.py's slot fix-up).
"""
import argparse, collections, glob, json, multiprocessing, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))

ROW_RE = re.compile(r"[0-9a-fA-F]{8}")


def module_rows():
    """[(src, flags, [va...])] for every src/ module."""
    import compile_all as C
    out = []
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src:
            continue
        vas = []
        for f in (src[:-4] + ".manifest.txt", src[:-4] + ".nonmatching.txt"):
            if not os.path.exists(f):
                continue
            for line in open(f, errors="replace"):
                if line.startswith("#"):
                    continue
                p = line.split()
                if f.endswith(".manifest.txt") and len(p) >= 3 and ROW_RE.fullmatch(p[2]):
                    vas.append(int(p[2], 16))
                elif not f.endswith(".manifest.txt") and p and ROW_RE.fullmatch(p[0]):
                    vas.append(int(p[0], 16))
        out.append((src, flags, sorted(set(vas))))
    return out


def vtable_stores(coff, fsym, img, lo, hi):
    """[(our ??_7 name, original vtable VA)] by order of appearance: the vtables our function references, in
    relocation order, against the original function's operands that address a vtable (a .rdata table whose first
    dword is a function start). Used only when both sides name the same number of distinct vtables."""
    import capstone
    sec, a, b = coff.func_extent(fsym)
    ours = []
    for off, si, _typ in sorted(sec.relocs):
        n = coff.syms[si].name
        if a <= off < b and n.startswith("??_7") and n not in ours:
            ours.append(n)
    if not ours:
        return []
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    orig = []
    for ins in md.disasm(img.read(lo, hi - lo), lo):
        for tok in ins.op_str.replace(",", " ").replace("]", " ").replace("[", " ").split():
            if tok.startswith("0x"):
                v = int(tok, 16)
                if 0x549000 <= v < 0x5a8000 and img.u32(v) in img.start_set and v not in orig:
                    orig.append(v)
    return list(zip(ours, orig)) if len(ours) == len(orig) else []


def one_module(job):
    src, flags, vas = job
    os.chdir(ROOT)
    import slice as S
    import resolve as R
    import equiv as E
    from coff import Coff
    ctx = E.context()
    img = ctx["img"]
    stem = os.path.basename(src)[:-4]
    o, _ = S.compile_obj(src, flags)
    if not o:
        return {"src": src, "error": "compile failed"}
    coff = Coff(o)
    annots = R.source_annotations([src] + glob.glob(os.path.join(os.path.dirname(src), "*.h")) + S.sibling_headers(src))
    refs, defined, vtables, errors, statics = [], [], [], [], []
    for va in vas:
        try:
            lo, hi = img.func_extent(va)
            sym, _how = S.find_symbol(coff, stem, va, src, hi - lo)
            if isinstance(sym, list):         # several candidates: the closest in size among the external functions
                ext = [n for n in sym if coff.by_name[n].cls == 2]
                sym = E.best_by_size(coff, ext, hi - lo) if ext else None
            if sym is None or coff.by_name[sym].cls != 2:     # a static has no name another object can bind to
                errors.append("%08x: %s" % (va, "static symbol %s" % sym if sym else "symbol not found"))
                if sym:
                    statics.append((va, sym))
                continue
            if False:
                errors.append("%08x: symbol not found" % va)
                continue
            defined.append((va, sym))
            res = ctx["res"].resolve(coff, coff.by_name[sym], va, annots)
        except Exception as e:      # one bad row must not lose the module
            errors.append("%08x: %s" % (va, e))
            continue
        for si, a in res.addr.items():
            s = coff.syms[si]
            refs.append((s.name, a, res.method.get(s.name, "?"), s.defined, "%08x" % va))
            if s.name.startswith("??_7") and a:
                vtables.append((s.name, a))
        vtables += vtable_stores(coff, coff.by_name[sym], img, lo, hi)
    # vtable slots: relocations of our vtable sections, by position
    slots = []
    seen = set()
    for vt, a in vtables:
        if vt in seen:
            continue
        seen.add(vt)
        vs = coff.by_name.get(vt)
        if vs is None or not vs.defined:
            continue
        sec = coff.section_of(vs)
        for off, si, _typ in sec.relocs:
            if off >= vs.value and (off - vs.value) % 4 == 0:
                k = (off - vs.value) // 4
                if k > 512:
                    continue
                target = img.read(a + 4 * k, 4)
                slots.append((coff.syms[si].name, int.from_bytes(target, "little"), vt, k))
    ours = {}
    for vt, a in vtables:
        vs = coff.by_name.get(vt)
        if vs is not None and vs.defined:
            ours[vt] = (a, coff.section_of(vs).size // 4)
    return {"src": os.path.relpath(src, ROOT), "refs": refs, "defined": defined, "slots": slots, "errors": errors,
            "vtables": ours, "statics": statics}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    a = ap.parse_args()
    os.chdir(ROOT)
    jobs = module_rows()
    with multiprocessing.Pool(a.jobs) as pool:
        results = pool.map(one_module, jobs, chunksize=1)
    names = collections.defaultdict(lambda: collections.defaultdict(lambda: {"methods": set(), "rows": set(), "defined": False}))
    defs = {}
    statics = {}
    slots = collections.defaultdict(set)
    vtmap = collections.defaultdict(lambda: {"orig": set(), "slots": 0})
    errors = []
    for r in results:
        if "error" in r:
            errors.append("%s: %s" % (r["src"], r["error"]))
            continue
        stem = os.path.basename(r["src"])[:-4]
        for va, sym in r["defined"]:
            defs.setdefault("%08x" % va, [sym, stem])
        for va, sym in r["statics"]:
            statics.setdefault("%08x" % va, [sym, stem])
        for name, va, how, isdef, row in r["refs"]:
            e = names[name]["%08x" % va]
            e["methods"].add(how.split("(")[0])
            e["rows"].add("%s %s" % (stem, row))
            e["defined"] |= bool(isdef)
        for name, va, vt, k in r["slots"]:
            slots[name].add(("%08x" % va, "%s[%d]" % (vt, k)))
        errors += ["%s %s" % (r["src"], e) for e in r["errors"]]
        for vt, (va, n) in r["vtables"].items():
            vtmap[vt]["orig"].add("%08x" % va)
            vtmap[vt]["slots"] = max(vtmap[vt]["slots"], n)
    out = {"names": {n: {va: {"methods": sorted(e["methods"]), "rows": sorted(e["rows"])[:6], "nrows": len(e["rows"]),
                               "defined": e["defined"]} for va, e in d.items()} for n, d in sorted(names.items())},
           "defined_at": dict(sorted(defs.items())),
           "static_at": dict(sorted(statics.items())),
           "vtable_slots": {n: sorted(v) for n, v in sorted(slots.items())},
           "vtables": {n: {"orig": sorted(v["orig"]), "slots": v["slots"]} for n, v in sorted(vtmap.items())},
           "errors": errors}
    json.dump(out, open(a.out, "w"), indent=1)
    conflicts = sum(1 for d in out["names"].values() if len(d) > 1)
    print("%d modules, %d referenced names (%d resolve to more than one address), %d defined rows, %d vtable-slot names, "
          "%d errors -> %s" % (len(results), len(out["names"]), conflicts, len(defs), len(slots), len(errors),
                                os.path.relpath(a.out, ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
