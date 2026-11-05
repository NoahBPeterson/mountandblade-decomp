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
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import heavy                                    # machine-wide worker-pool gate (heavy.py)
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
    import time
    t_start = time.time()
    src, flags, vas = job
    os.chdir(ROOT)
    import slice as S
    import resolve as R
    import equiv as E
    from coff import Coff
    t0 = time.time()
    first = not E._ctx
    ctx = E.context()
    t_ctx = time.time() - t0
    img = ctx["img"]
    stem = os.path.basename(src)[:-4]
    t0 = time.time()
    cl0, key0 = S.TIMING["cl_s"], S.TIMING["key_s"]
    o, _ = S.compile_obj(src, flags)
    t_compile = time.time() - t0
    d_cl, d_key = S.TIMING["cl_s"] - cl0, S.TIMING["key_s"] - key0
    t_resolve0 = time.time()
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
            "vtables": ours, "statics": statics,
            "timing": {"ctx": t_ctx, "compile": t_compile, "resolve": time.time() - t_resolve0, "total": time.time() - t_start,
                       "cl_s": d_cl, "key_s": d_key, "first": first, "rows": len(vas)}}


# ---- per-module result cache ------------------------------------------------------------------------------------
# A module's resolver result is a function of: its object (obj_digest: sections, relocations, symbols, not the debug records: an unchanged module that
# recompiled to the same code keeps its result), its rows, the text of its source and of the headers whose annotations the
# resolver reads for it, and the resolver's environment: the resolver/harness code, the original image, symbols/, every
# manifest (SymbolDB reads them all), and the annotation universe of all of src/ (the resolver's deep-symbols fallback;
# line numbers excluded, they only reach the method text after the "(" that this tool drops).
CACHE_DIR = os.environ.get("MB_RESOLVE_CACHE") or os.path.join(ROOT, "work", "relink", "resolve_cache")


def _md5_files(h, paths):
    for f in sorted(paths):
        h.update(os.path.relpath(f, ROOT).encode() + b"\0")
        try:
            h.update(open(f, "rb").read())
        except OSError:
            h.update(b"<missing>")


def resolver_env():
    """Digest of everything the resolver reads that is not the module's own object or sources."""
    import hashlib
    import resolve as R
    h = hashlib.md5(("resolve_map cache v1 %s\0" % (sys.version.split()[0],)).encode())
    code = glob.glob(os.path.join(ROOT, "tools", "difftest", "*.py")) + glob.glob(os.path.join(ROOT, "tools", "matching", "*.py")) + \
        [os.path.abspath(__file__)]
    _md5_files(h, code)
    data = glob.glob(os.path.join(ROOT, "symbols", "**", "*"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "match", "slices", "*", "manifest.txt")) + \
        glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True) + \
        [os.path.join(ROOT, "work", "mb.analysis.bin"), os.path.join(ROOT, "work", "oss", "crt_matches.csv")]
    _md5_files(h, [f for f in data if os.path.isfile(f)])
    files = glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)
    ann = sorted(repr((i, sc, va, pt)) for i, sc, va, _loc, pt in R.source_annotations(files))
    h.update("\n".join(ann).encode())
    return h.hexdigest()


def obj_parts(path):
    """[(label, bytes)] of what an object means: its sections (name, flags, bytes, relocations) and symbols. Not the
    header's timestamp, not .debug$S/.debug$T (they record the output path, which names the temp file and the machine
    that compiled), not the .file records (the source path; its length shifts the physical symbol indices, so
    relocations are listed with logical symbol numbers)."""
    import struct
    d = open(path, "rb").read()
    _m, nsec, _ts, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", d, 0)
    logical, i, n = {}, 0, 0
    while i < nsym:
        logical[i] = n
        n += 1
        i += 1 + d[symptr + 18 * i + 17]
    strtab = symptr + 18 * nsym

    def sec_name(o):
        nm = d[o:o + 8]
        if nm[:1] == b"/" and nm[1:].rstrip(b"\0").isdigit():            # a long section name lives in the string table
            at = strtab + int(nm[1:].rstrip(b"\0"))
            return d[at:d.index(b"\0", at)]
        return nm.rstrip(b"\0")
    out = [("header", d[:2] + struct.pack("<HI", nsec, n))]
    skipped = set()
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        name = sec_name(o)
        _vs, _va, size, rawptr, relptr, _lnp, nrel, _nln, flags = struct.unpack_from("<IIIIIIHHI", d, o + 8)
        if name.startswith((b".debug$S", b".debug$T")):
            skipped.add(i + 1)
            out.append(("section %d %r" % (i, name), b""))
            continue
        out.append(("section %d %r" % (i, name), name + struct.pack("<I", flags) + (d[rawptr:rawptr + size] if rawptr and not flags & 0x80 else b"")))
        out.append(("relocs %d" % i, b"".join(struct.pack("<IIH", *(lambda r: (r[0], logical.get(r[1], r[1]), r[2]))(struct.unpack_from("<IIH", d, relptr + 10 * r)))
                                              for r in range(nrel))))
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        naux = d[o + 17]
        # C_FILE: the aux records spell the source path; a .debug$S/.debug$T section symbol's aux carries that section's size
        # and checksum
        skip = d[o + 16] == 0x67 or struct.unpack_from("<h", d, o + 12)[0] in skipped
        rec = d[o:o + 17] if skip else d[o:o + 18 * (1 + naux)]
        if d[o + 16] == 105 and naux:                 # a weak external: its aux names the default symbol by physical index
            rec = d[o:o + 18] + struct.pack("<I", logical.get(struct.unpack_from("<I", d, o + 18)[0], 0)) + d[o + 22:o + 36]
        out.append(("symbol %d" % logical[i], rec))
        i += 1 + naux
    out.append(("strtab", d[symptr + 18 * nsym:]))
    return out


def obj_digest(path):
    import hashlib
    h = hashlib.md5()
    for _label, b in obj_parts(path):
        h.update(b)
    return h.hexdigest()


def module_key(env, src, vas, obj):
    import hashlib
    import slice as S
    h = hashlib.md5(env.encode())
    h.update(os.path.relpath(src, ROOT).encode() + repr(vas).encode() + obj_digest(obj).encode())
    _md5_files(h, [src] + glob.glob(os.path.join(os.path.dirname(src), "*.h")) + S.sibling_headers(src))
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=int(os.environ.get("MB_JOBS") or max(1, (os.cpu_count() or 2) - 2)))
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    a = ap.parse_args()
    os.chdir(ROOT)
    import time
    t_wall = time.time()
    jobs = module_rows()
    import pickle
    import slice as S
    t0 = time.time()
    S.ensure_objs([(src, flags) for src, flags, _vas in jobs], jobs=a.jobs)        # misses: preprocess-keyed, batched compiles
    t_compile = time.time() - t0
    t0 = time.time()
    use_cache = os.environ.get("MB_RESOLVE_CACHE_OFF") != "1"
    env = resolver_env() if use_cache else None
    keys, results, todo = [], [None] * len(jobs), []
    for i, (src, flags, vas) in enumerate(jobs):
        key = None
        if use_cache:
            o, _ = S.compile_obj(src, flags)
            if o:
                key = module_key(env, src, vas, o)
                try:
                    results[i] = pickle.load(open(os.path.join(CACHE_DIR, key + ".pkl"), "rb"))
                    continue
                except (OSError, EOFError, pickle.UnpicklingError):
                    pass
        keys.append(key)
        todo.append(i)
    t_keys = time.time() - t0
    t0 = time.time()
    if todo:
        with multiprocessing.Pool(heavy.pool_jobs(min(a.jobs, len(todo)))) as pool:
            got = pool.map(one_module, [jobs[i] for i in todo], chunksize=1)
        os.makedirs(CACHE_DIR, exist_ok=True)
        for i, key, r in zip(todo, keys, got):
            results[i] = r
            if key and "error" not in r:
                part = os.path.join(CACHE_DIR, "%s.%d.part" % (key, os.getpid()))
                pickle.dump({k: v for k, v in r.items() if k != "timing"}, open(part, "wb"))
                os.replace(part, os.path.join(CACHE_DIR, key + ".pkl"))
    t_pool = time.time() - t0
    if os.environ.get("MB_TIMING"):
        ts = [r["timing"] for r in results if "timing" in r]
        print("TIMING resolve_map: wall %.1fs = ensure_objs %.1fs + result-cache lookup %.1fs + resolving pool %.1fs; "
              "%d of %d modules resolved (%d from the result cache); pool workers' summed time: compile_obj %.1fs (cl %.1fs, "
              "key hashing %.1fs), resolver context %.1fs (%d workers built it), resolving rows %.1fs; this process: %s" % (
                  time.time() - t_wall, t_compile, t_keys, t_pool, len(ts), len(results), len(results) - len(ts),
                  sum(t["compile"] for t in ts), sum(t["cl_s"] for t in ts), sum(t["key_s"] for t in ts),
                  sum(t["ctx"] for t in ts), sum(1 for t in ts if t["first"]), sum(t["resolve"] for t in ts),
                  {k: round(v, 1) for k, v in S.TIMING.items()}), file=sys.stderr)
        json.dump(ts, open(os.environ["MB_TIMING"], "w"))
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
