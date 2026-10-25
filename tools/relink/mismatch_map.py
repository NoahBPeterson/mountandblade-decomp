#!/usr/bin/env python3
"""mismatch_map.py: for every symbol the trial link could not resolve although our source defines a function at
the address it stands for, say who declares it how, and how the definition looks.

usage: mismatch_map.py [--log work/relink/link.log] [--out work/relink/mismatch.json] [--by-module]

Reads the last trial link (tools/relink/trial_link.py) and, per unresolved symbol:
  caller   the module(s) whose object references it (from the LNK2019 lines)
  va       the original address, from the address annotations of the caller module's declarations
  defined  the decorated name our source defines at that va (from every module's manifest / nonmatching rows)
  kind     signature   same qualified name, other parameter / return / const / calling-convention decoration
           name        another qualified name (class or function name differ)
           unknown     no definition found at the va
Fix a row by making the caller's declaration match the definition (include the canonical header that declares it,
or correct the local declaration); the trial link then resolves it.
"""
import argparse, collections, glob, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
import slice as S          # noqa: E402
from coff import Coff      # noqa: E402

DECL = re.compile(r'\b([A-Za-z_~][\w:~]*)\s*(?:\([^;{]*\)|\[[^\]]*\])?\s*(?:const\s*)?;\s*//.*?\b0x([0-9a-fA-F]{6,7})\b')


def module_of_obj(objname):
    return objname.rsplit("_", 1)[0]          # <stem>_<hash12>.obj


def defined_symbols():
    """va -> (decorated name, module stem) for every module row whose symbol the object defines."""
    import compile_all as C
    import equiv as E
    img = E.context()["img"]
    out = {}
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src:
            continue
        stem = os.path.basename(src)[:-4]
        o, _ = S.compile_obj(src, flags)
        if not o:
            continue
        coff = Coff(o)
        vas = set()
        for f in (src[:-4] + ".manifest.txt", src[:-4] + ".nonmatching.txt"):
            if not os.path.exists(f):
                continue
            for line in open(f, errors="replace"):
                p = line.split()
                if line.startswith("#"):
                    continue
                if f.endswith(".manifest.txt") and len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                    vas.add(int(p[2], 16))
                elif p and re.fullmatch(r"[0-9a-fA-F]{8}", p[0]):
                    vas.add(int(p[0], 16))
        for va in vas:
            lo, hi = img.func_extent(va)
            sym, _how = S.find_symbol(coff, stem, va, src, hi - lo)
            if isinstance(sym, str):
                out.setdefault(va, (sym, stem))
    return out


def module_annotations():
    """module stem -> {identifier: va}, from the module's .cpp and every header in its directory and src/ headers it
    names (approximate: the identifier's last component)."""
    per_file = {}
    for p in glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True):
        if p.endswith((".h", ".cpp")):
            d = {}
            for line in open(p, errors="replace"):
                m = DECL.search(line)
                if m:
                    d.setdefault(m[1].split("::")[-1], int(m[2], 16))
            per_file[p] = d
    glob_ann = {}
    for d in per_file.values():
        for k, v in d.items():
            glob_ann.setdefault(k, set()).add(v)
    return per_file, glob_ann


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default=os.path.join(ROOT, "work", "relink", "link.log"))
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "mismatch.json"))
    ap.add_argument("--by-module", action="store_true")
    a = ap.parse_args()
    os.chdir(ROOT)
    log = open(a.log, errors="replace").read()
    refs = collections.defaultdict(set)
    pretty = {}
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from trial_link import parse_unresolved
    for sym, (pr, objs) in parse_unresolved(log).items():
        refs[sym] |= {module_of_obj(o) for o in objs}
        pretty[sym] = pr
    defs = defined_symbols()
    by_name = {}
    for va, (sym, stem) in defs.items():
        by_name.setdefault(sym, va)
    per_file, glob_ann = module_annotations()
    srcs = {os.path.basename(p)[:-4]: p for p in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)}
    rows = []
    for sym, callers in sorted(refs.items()):
        if "Newton" in sym[:16] or "FSOUND_" in sym or "SkinMagic" in sym:
            continue
        q = S.demangle_qual(sym) if sym.startswith("?") else sym.lstrip("_").split("@")[0]
        base = (q or sym).split("::")[-1]
        va = None
        for c in sorted(callers):                       # the caller's own files first
            src = srcs.get(c)
            if src:
                for f in [src] + glob.glob(os.path.join(os.path.dirname(src), "*.h")):
                    va = per_file.get(f, {}).get(base)
                    if va:
                        break
            if va:
                break
        if va is None and len(glob_ann.get(base, ())) == 1:
            va = next(iter(glob_ann[base]))
        if va is None or va not in defs:
            continue                                    # data, CRT, or no definition: other trial-link categories
        dsym, dmod = defs[va]
        dq = S.demangle_qual(dsym) if dsym.startswith("?") else dsym.lstrip("_").split("@")[0]
        kind = "signature" if dq and q and dq == q else "name"
        rows.append({"symbol": sym, "pretty": pretty.get(sym, sym), "callers": sorted(callers), "va": "%08x" % va,
                     "defined": dsym, "defined_in": dmod, "kind": kind})
    json.dump(rows, open(a.out, "w"), indent=1)
    c = collections.Counter(r["kind"] for r in rows)
    print("%d declarations that miss a definition our source has: %s" % (len(rows), dict(c)))
    mods = collections.Counter(m for r in rows for m in r["callers"])
    print("caller modules: %d; most: %s" % (len(mods), ", ".join("%s %d" % kv for kv in mods.most_common(8))))
    if a.by_module:
        for m, n in mods.most_common():
            print("%5d %s" % (n, m))
    return 0


if __name__ == "__main__":
    sys.exit(main())
