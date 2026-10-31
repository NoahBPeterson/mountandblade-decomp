#!/usr/bin/env python3
"""data_size_audit.py: find data definitions that are smaller than the original object they stand for.

usage: data_size_audit.py [--resolve-map JSON] [--filter SUBSTR] [--jobs N] [-v]

Nothing else notices an undersized definition. `char* g_object_names[1];` (4 bytes) defined the original's ground-type
table (11 x 0x20 bytes at 0x8ed4df8) while terrain.h declared `extern GroundType g_ground_types[11];` for the same
address: MSVC mangles an array like a pointer, so the linker never complained, and in the standalone exe the table
overlapped every global linked after it.

For each original data address our objects define (the resolver's `defined` entries):
  our size      the compiled size: the distance from the symbol to the next symbol of its section in the .obj that
                defines it (or to the section's end)
  claimed       the largest sizeof any declaration of that address in our source gives (`extern T x[N]; // 0xADDR`,
                a definition, a struct type), measured by the compiler: each annotated declaration is probed with
                `char (*p)[sizeof(x) + 1] = 1;`, whose error message spells the size
  accessed      (suspect only) the highest offset the original's code addresses past the symbol before the next
                address any declaration, name or annotation of ours puts a label on
UNDERSIZED = our size < claimed: a definite error (exit 1, and build.py's gate). SUSPECT = our size < accessed:
listed for judgement (the next original object may simply not have a label of ours); never fails the build.
"""
import argparse, collections, json, multiprocessing, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

LO, HI = 0x549000, 0x8ee5b94          # the original's .rdata through .bss
GENERATED = ("??_C@", "__real@", "__int64@", "__xmm@", "__mask@", "??_7", "??_R", "$SG", "??_8", "??_9")
# `extern T name[N]...; // 0xADDR`, `T name[N] = ...; // 0xADDR`, `static T Cls::name[N] = // 0xADDR`
DECL = re.compile(r"^\s*(?!return\b|typedef\b|using\b|//|#)(?:[A-Za-z_][\w:<>,\*&\s]*?[\s\*&])?"
                  r"((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)\s*(?:=[^;/]*)?;?\s*//\s*0x([0-9a-fA-F]{6,8})\b")


# ---- the compiled size of each definition ---------------------------------------------------------------------

def coff_symbols(path):
    """{name: [(section index, offset, section size, is_code)]} of the symbols an object defines, with every
    defined symbol (static ones too) listed so a neighbour bounds a size."""
    d = open(path, "rb").read()
    machine, nsec, _ts, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        size, _rawptr = struct.unpack_from("<II", d, o + 16)
        flags = struct.unpack_from("<I", d, o + 36)[0]
        secs.append((size, bool(flags & 0x20), bool(flags & 0x200)))     # (size, code, lnk_remove: debug / directives)
    strtab = symptr + 18 * nsym
    syms, i = [], 0
    while i < nsym:
        o = symptr + 18 * i
        nm = d[o:o + 8]
        if nm[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", nm, 4)[0]
            name = d[strtab + off:d.index(b"\0", strtab + off)].decode("latin1")
        else:
            name = nm.rstrip(b"\0").decode("latin1")
        value, sec, _typ, cls, naux = struct.unpack_from("<IhHBB", d, o + 8)
        if sec > 0 and cls in (2, 3) and not secs[sec - 1][1] and not secs[sec - 1][2]:
            syms.append((name, sec, value, cls))
        i += 1 + naux
    by_sec = collections.defaultdict(list)
    for name, sec, value, _c in syms:
        by_sec[sec].append(value)
    out = collections.defaultdict(list)
    for name, sec, value, cls in syms:
        if cls != 2:
            continue
        nxt = [v for v in by_sec[sec] if v > value]
        end = min(nxt) if nxt else secs[sec - 1][0]
        out[name].append(end - value)
    return out


def our_sizes(objs):
    got = {}
    for o in objs:
        for name, sizes in coff_symbols(o).items():
            got.setdefault(name, []).extend((s, os.path.basename(o)) for s in sizes)
    return got


# ---- the size every declaration claims --------------------------------------------------------------------------

def quoted_headers(src):
    import slice as S
    return [f for f in S._quoted_includes(src) if f.endswith((".h", ".hpp")) and os.sep + "src" + os.sep in f]


def decls(path):
    """[(identifier, address, line)] of the annotated data declarations / definitions of one file."""
    out = []
    for n, line in enumerate(open(path, errors="replace").read().split("\n"), 1):
        if "//" not in line:
            continue
        m = DECL.match(line)
        if m:
            a = int(m[3], 16)
            if LO <= a < HI:
                out.append((m[1], a, n))
    return out


def probe_one(job):
    """Compile src with one size probe per declaration appended -> [(name, addr, file, line, size)]."""
    src, flags, entries = job
    os.chdir(ROOT)
    import slice as S
    if not entries:
        return []
    flags = S.resolve_include_flags(flags)
    tmp = os.path.join(os.path.dirname(src), "_dataaudit_%s" % os.path.basename(src))
    base = open(src, errors="replace").read()
    if not base.endswith("\n"):
        base += "\n"
    first = base.count("\n") + 2
    probes = ["/* probes */"] + ["char (*da_%d_)[sizeof(%s) + 1] = 1;" % (k, e[0]) for k, e in enumerate(entries)]
    try:
        open(tmp, "w").write(base + "\n".join(probes) + "\n")
        r = subprocess.run([S.CL, "/nologo", "/c", "/Zs", *flags, S.INCLUDE, S.winpath(tmp)], capture_output=True, text=True)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
    out = []
    for m in re.finditer(r"_dataaudit_[^(]*\((\d+)\)\s*:\s*error C\d+:.*\(\*\)\[(\d+)\]", r.stdout + r.stderr):
        k = int(m[1]) - first
        if 0 <= k < len(entries):
            name, addr, f, line = entries[k]
            out.append((name, addr, f, line, int(m[2]) - 1))
    return out


def claimed_sizes(jobs_n):
    import compile_all as C
    seen, jobs = set(), []
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src or os.path.basename(src).startswith("_dataaudit_"):
            continue                                 # (a concurrent or killed audit's probe copies are not sources)
        entries = []
        for f in [src] + quoted_headers(src):
            if f in seen:
                continue
            seen.add(f)
            entries += [(n, a, os.path.relpath(f, ROOT), l) for n, a, l in decls(f)]
        jobs.append((src, flags, entries))
    with multiprocessing.Pool(jobs_n) as pool:
        res = [r for rs in pool.map(probe_one, jobs, chunksize=1) for r in rs]
    claims = collections.defaultdict(list)           # address -> [(size, name, file:line)]
    for name, addr, f, line, size in res:
        claims[addr].append((size, name, "%s:%d" % (f, line)))
    labels = {a for src, flags, es in jobs for _n, a, _f, _l in es}
    return claims, labels


# ---- what the original's code addresses --------------------------------------------------------------------------

def accessed_addresses():
    """Every address in the original's data that an instruction of the game or library names (memory displacement)."""
    import capstone
    import resolve as R
    import data_coverage as D
    img = R.Image()
    starts, _game = D.game_functions()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    tok = re.compile(r"\[[^\]]*\]")
    t0, tn = img.section(".text")
    uses = set()
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else t0 + tn
        for ins in md.disasm(img.read(s, e - s), s):
            for t in tok.findall(ins.op_str):
                for h in re.findall(r"0x([0-9a-f]+)", t):
                    v = int(h, 16)
                    if LO <= v < HI:
                        uses.add(v)
    return uses


STATS = {}


def audit(rmap_path, jobs_n, suspects=True):
    """-> (undersized, suspect) lists of dicts."""
    import trial_link as T
    objs, fails = T.objects()
    if fails:
        sys.exit("data size audit: %d sources did not compile: %s" % (len(fails), ", ".join(fails[:6])))
    ours = our_sizes(objs)
    rmap = json.load(open(rmap_path))
    claims, ann_labels = claimed_sizes(jobs_n)
    defined = collections.defaultdict(list)          # va -> [mangled names we define there]
    labelled = set(ann_labels)
    for n, d in rmap["names"].items():
        for va, e in d.items():
            v = int(va, 16)
            if LO <= v < HI:
                labelled.add(v)
                if not n.startswith(GENERATED) and n in ours:
                    defined[v].append(n)
    labels = sorted(labelled)
    STATS.update(defined=sum(len(v) for v in defined.values()), sized=sum(1 for v in defined if v in claims), claims=len(claims))
    uses = sorted(accessed_addresses()) if suspects else []
    import bisect
    under, suspect = [], []
    for va, names in sorted(defined.items()):
        for n in names:
            size = min(s for s, _o in ours[n])
            where = ours[n][0][1]
            cl = max(claims.get(va, [(0, "", "")]))
            if cl[0] > size:
                under.append(dict(va=va, name=n, size=size, claimed=cl[0], by=cl[1], at=cl[2], obj=where))
                continue
            i = bisect.bisect_right(labels, va)
            nxt = labels[i] if i < len(labels) else HI
            j = bisect.bisect_left(uses, va + size)
            if suspects and j < len(uses) and uses[j] < nxt:
                top = max(u for u in uses[j:] if u < nxt)
                suspect.append(dict(va=va, name=n, size=size, accessed=top - va + 4, gap=nxt - va, obj=where))
    return under, suspect


def duplicates(rmap_path, map_path):
    """Original data addresses that end up with more than one storage location in the linked exe: several of our
    names (different types or namespaces, an `extern` the resolver also counts as a definition elsewhere) bound to one
    address by annotations, each defined by a module. The glue aliases the original's address to one of them; the
    others are filled or read by code that never sees the first. -> [(va, {our address: [names]})]"""
    pub = {}
    for line in open(map_path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            pub.setdefault(m[1], int(m[2], 16))
    byva = collections.defaultdict(lambda: collections.defaultdict(list))
    for n, d in json.load(open(rmap_path))["names"].items():
        for va, e in d.items():
            v = int(va, 16)
            if LO <= v < HI and not n.startswith(GENERATED) and n in pub:
                byva[v][pub[n]].append(n)
    return [(v, dict(at)) for v, at in sorted(byva.items()) if len(at) > 1]


def gate(rmap_path, map_path, jobs_n=None):
    """build.py's gate -> (undersized, duplicates, number of definitions checked)."""
    under, _s = audit(rmap_path, jobs_n or max(1, (os.cpu_count() or 2) - 2), suspects=False)
    return under, duplicates(rmap_path, map_path), STATS.get("defined", 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--resolve-map", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("--map", help="the linked exe's map: also report original addresses with 2+ storage locations")
    ap.add_argument("-v", action="store_true", help="list the suspects (the original's code reaches past our storage)")
    a = ap.parse_args()
    os.chdir(ROOT)
    under, suspect = audit(a.resolve_map, a.jobs, suspects=a.v)
    for e in under:
        print("UNDERSIZED %08x %s: we define %d bytes (%s), %s claims %d at %s" % (
            e["va"], e["name"], e["size"], e["obj"], e["by"], e["claimed"], e["at"]))
    for e in suspect:
        print("suspect    %08x %s: we define %d bytes (%s), the original addresses +%#x, next label +%#x" % (
            e["va"], e["name"], e["size"], e["obj"], e["accessed"], e["gap"]))
    dups = duplicates(a.resolve_map, a.map) if a.map else []
    for va, at in dups:
        print("DUPLICATE  %08x: %s" % (va, "; ".join("%#x %s" % (x, ", ".join(ns)) for x, ns in sorted(at.items()))))
    print("%(defined)d data definitions at original addresses, %(sized)d with a measured declared size (%(claims)d addresses probed)" % STATS)
    print("%d undersized definitions, %d suspects, %d addresses with several storage locations" % (len(under), len(suspect), len(dups)))
    return 1 if under or dups else 0


if __name__ == "__main__":
    sys.exit(main())
