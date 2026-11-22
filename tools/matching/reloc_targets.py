#!/usr/bin/env python3
"""reloc_targets.py: do the relocations of each manifest row name the SAME targets as the original's operands?

usage: reloc_targets.py [--filter SUBSTR] [--manifest PATH] [--jobs N] [--json OUT] [--all-rows] [-v]

run_all.py masks every relocated field on both sides (the original's operand bytes and ours), so a row whose
instruction shapes agree is byte-exact even when it loads another global, or the same global at another field offset
(Recf0::frame_update 0x4d63b0 read 0xa73a08 where the original reads 0xa73a04, and the reverse, and stayed
byte-exact). This tool fills that hole: for every relocation of ours inside a row's range it computes the original
address our target stands for, and compares it with what the original's bytes at that spot encode:

  DIR32   the original's dword is the absolute address (mount&blade.exe has no .reloc: it is never rebased)
  REL32   the original's dword is a displacement: target = site + 4 + disp (calls, jumps)

Our target = (original address of the symbol) + addend (the dword our object stores at the site). The symbol's
original address comes from the equivalence harness resolver (tools/difftest/resolve.py), the same name evidence the
relink's resolve_map.json is built from, restricted to methods that do not look at the original's operands: a
binding found by alignment ("align": the N-th reference of ours paired with the original's N-th) is circular here
and counts as unresolved. A label or jump table inside the row's own section maps to the row's address + offset.

Classes (per relocation):
  ok               same target (also: through a `jmp rel32` / `jmp [IAT]` stub on either side)
  addend           same symbol, the original's target within 0x100 bytes of ours: another field offset (swapped
                   loads, a misdeclared struct member)
  target           a different target (another global, another callee)
  copy             a different callee whose code is the same as ours up to addresses: the original has the function
                   twice (one template instance per element type our source spells with one name). Not a code
                   difference in the row; the name binds the relink to the other copy.
  literal          a string literal / float constant / read-only block of ours (constant_diffs compares those by value)
  unresolved-*     no independent original address: local-static (function-local static / its guard), eh (EH
                   metadata), own-code / own-data (defined in our object, no name evidence), align-only (bound only by
                   position), extern (undefined, no name evidence), other-type (a relocation type other than
                   DIR32/REL32)
Rows: only byte-exact rows (run_all's comparison says MATCH) are checked unless --all-rows. The report prints counts
per class and every row with an addend/target mismatch; --json writes the full list (default
work/reloc_targets.json). Exit 0 iff no byte-exact row has an addend/target mismatch.

`gate_mismatches()` is the opt-in gate run_all.py --reloc-gate uses: a byte-exact row with an addend/target mismatch
fails as a byte difference.
"""
import argparse, glob, json, os, struct, sys, time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "relink"))

REL_DIR32, REL_REL32 = 0x06, 0x14
NEAR = 0x100
# resolver methods that do not depend on the original's operands at this row's reference sites
CIRCULAR = ("align", "model-")
# bindings that win over a manifest row of the same name: the row's own declaration annotation (names are not unique
# across match/slices: each slice spells its own E36 / tstring / Terrain::reset), imports and CRT helpers
STRONG = ("annotation", "import", "crt", "seh-chain", "builtin")      # (src/ rows: only for names no manifest lists)
LITERAL_PREFIXES = ("??_C@", "__real@", "__xmm@", "__mask@", "__int64@")
BAD = ("addend", "target")

_ctx = {}


def context():
    """Image, symbol database and resolver (no Machine: nothing is modelled), built once per process."""
    if not _ctx:
        import resolve as R
        img = R.Image()
        _ctx.update(R=R, img=img, res=R.Resolver(img, R.SymbolDB(img)), annots={}, coff={})
    return _ctx


def annotations(src):
    """The '// 0x...' declaration annotations the resolver reads for a source (as resolve_map.py collects them)."""
    c = context()
    if src not in c["annots"]:
        import slice as S
        c["annots"][src] = c["R"].source_annotations(
            S.annotation_sources(src))
    return c["annots"][src]


def coff_of(obj):
    c = context()
    if obj not in c["coff"]:
        from coff import Coff
        if len(c["coff"]) >= 16:                # a few objects at a time: rows mostly come grouped by object
            c["coff"].pop(next(iter(c["coff"])))
        c["coff"][obj] = Coff(obj)
    return c["coff"][obj]


def follow(img, va, depth=3):
    """A call target as the code it reaches: through `jmp rel32` stubs; a `jmp [IAT]` thunk -> ("imp", IAT slot)."""
    for _ in range(depth):
        if not img.in_text(va):
            return va
        b = img.read(va, 6)
        if b[:1] == b"\xeb":                     # jmp short: the WinLicense marker in front of a function (eb 10 'WL  ')
            va = (va + 2 + struct.unpack_from("<b", b, 1)[0]) & 0xFFFFFFFF
            continue
        if b[:1] == b"\xe9":
            va = (va + 5 + struct.unpack_from("<i", b, 1)[0]) & 0xFFFFFFFF
            continue
        if b[:2] == b"\xff\x25":
            return ("imp", struct.unpack_from("<I", b, 2)[0])
        return va
    return va


def manifest_vas():
    """{mangled name: VA} of every external symbol a src/ module's manifest row lists at exactly one address. The
    src/ modules link together, so their names are unique; a row is verified byte-exact at its VA, stronger evidence
    than a declaration annotation the resolver matches by identifier (`??4tstring@@` took the annotation of a vector's
    operator= in poly_mesh_edge_face_ops.h, `??0Skin@@QAE@ABU0@@Z` the default constructor's). For src/ rows this wins over the
    resolver; match/slices rows keep the resolver's own-source annotation first (each slice spells its own E36,
    tstring, Terrain::reset, so their names collide across slices)."""
    c = context()
    if "mvas" not in c:
        import run_all as RA
        seen = {}
        for src, sym, va, _flags in RA.read_rows(RA.manifests(), ""):
            if src.startswith("../src/") and sym.startswith(("?", "_")) and not sym.startswith(("_$E", "$L")) and not RA.ANON_NS.search(sym):
                seen.setdefault(sym, set()).add(va)
        c["mvas"] = {k: next(iter(v)) for k, v in seen.items() if len(v) == 1}
    return c["mvas"]


def same_shape(img, a, b):
    """Are the original's functions at a and b the same code up to addresses (a template or helper the original has
    twice, e.g. one copy per element type our source gives one name)? Equal extents, and every differing byte lies in
    a dword that is an image address on both sides or in the rel32 of a call/jmp."""
    if a == b or not (img.in_text(a) and img.in_text(b)):
        return False
    try:
        la, ha = img.func_extent(a)
        lb, hb = img.func_extent(b)
    except Exception:       # noqa: BLE001
        return False
    if ha - la != hb - lb or ha - la > 0x4000:
        return False
    x, y = img.read(la, ha - la), img.read(lb, hb - lb)
    lo, hi = img.base, img.base + img.size
    i, n = 0, len(x)
    while i < n:
        if x[i] == y[i]:
            i += 1
            continue
        for j in range(max(0, i - 3), i + 1):
            if j + 4 > n:
                continue
            u, v = struct.unpack_from("<I", x, j)[0], struct.unpack_from("<I", y, j)[0]
            rel = j >= 1 and (x[j - 1] in (0xE8, 0xE9) or (j >= 2 and x[j - 2] == 0x0F and 0x80 <= x[j - 1] <= 0x8F))
            if rel or (lo <= u < hi and lo <= v < hi):
                i = j + 4
                break
        else:
            return False
    return True


def classify_unresolved(s, coff, method):
    R = context()["R"]
    nm = s.name
    if method and method.startswith(CIRCULAR):
        return "unresolved-align-only"
    if nm.startswith(LITERAL_PREFIXES) or (method or "").startswith("rdata-literal"):
        return "literal"
    if nm.startswith(R.EH_PREFIXES):
        return "unresolved-eh"
    if R.LOCAL_STATIC_RE.search(nm) or nm.startswith("?$S"):
        return "unresolved-local-static"
    if s.defined:
        sec = coff.sections[s.secno - 1]
        if s.cls == 3 and s.value == 0 and s.naux and not sec.is_code and not sec.writable:
            return "literal"                    # a section symbol of a read-only block (a const table) + addend
        return "unresolved-own-code" if sec.is_code else "unresolved-own-data"
    return "unresolved-extern"


def row_relocs(obj, src, va, row):
    """[{at, type, sym, addend, ours, orig, class, method}] for every relocation of ours inside the row.
    row = run_all.compare_row's dict (name, off, n, mine, theirs)."""
    c = context()
    img, res = c["img"], c["res"]
    coff = coff_of(obj)
    fsym = coff.by_name[row["name"]]
    fsec = fsym.secno - 1
    off, n, mine, theirs = row["off"], row["n"], row["mine"], row["theirs"]
    r = res.resolve(coff, fsym, va, annotations(src))
    slice_row = os.sep + "src" + os.sep not in src
    out = []
    for p, si, typ in sorted(coff.sections[fsec].relocs):
        if not off <= p < off + n:
            continue
        s = coff.syms[si]
        m = {"at": p - off, "type": {REL_DIR32: "DIR32", REL_REL32: "REL32"}.get(typ, "0x%x" % typ), "sym": s.name}
        out.append(m)
        if typ not in (REL_DIR32, REL_REL32) or p - off + 4 > n:
            m["class"] = "unresolved-other-type"
            continue
        addend = struct.unpack_from("<i", mine, p - off)[0]
        disp = struct.unpack_from("<I", theirs, p - off)[0]
        orig = disp if typ == REL_DIR32 else (va + p - off + 4 + struct.unpack_from("<i", theirs, p - off)[0]) & 0xFFFFFFFF
        m.update(addend=addend, orig="%08x" % orig)
        base, method = None, r.method.get(s.name)
        if s.defined and s.secno - 1 == fsec:
            base, method = va - off + s.value, "own-section"     # a label / jump table / the row itself
        elif si in r.addr and (method or "").startswith(STRONG) and (slice_row or not s.name in manifest_vas()):
            base = r.addr[si]
        elif not slice_row and s.cls == 2 and s.name in manifest_vas():
            base, method = manifest_vas()[s.name], "manifest"
        elif si in r.addr and not (method or "").startswith(CIRCULAR) and not (method or "").startswith("rdata-literal"):
            base = r.addr[si]
        m["method"] = method
        if base is None:
            m["class"] = classify_unresolved(s, coff, method)
            continue
        ours = (base + addend) & 0xFFFFFFFF
        m["ours"] = "%08x" % ours
        if ours == orig:
            m["class"] = "ok"
        elif typ == REL_REL32 and follow(img, ours) == follow(img, orig):
            m["class"] = "ok"
        elif typ == REL_DIR32 and res.thunk_import(orig) is not None and res.thunk_import(orig) == img.imports.get(ours):
            m["class"] = "ok"
        elif addend == 0 and same_shape(img, follow(img, ours), follow(img, orig)):
            m["class"] = "copy"                 # another copy of the same code in the original: a naming collision
        elif abs(orig - ours) < NEAR:
            m["class"] = "addend"
        else:
            m["class"] = "target"
    return out


def gate_mismatches(obj, src, va, row):
    """The relocations of a byte-exact row whose resolvable target differs from the original's (run_all --reloc-gate)."""
    try:
        return [m for m in row_relocs(obj, os.path.normpath(os.path.join(ROOT, "match", src)), va, row)
                if m["class"] in BAD]
    except Exception as e:      # noqa: BLE001 - a resolver error must not pass silently as a mismatch-free row
        return [{"at": 0, "class": "error: %s" % e}]


def main():
    import run_all as RA
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter", default="")
    ap.add_argument("--manifest")
    ap.add_argument("--jobs", type=int, default=6, help="compile threads (the object cache makes most compiles free)")
    ap.add_argument("--json", default=os.path.join(ROOT, "work", "reloc_targets.json"))
    ap.add_argument("--all-rows", action="store_true", help="also check rows that are not byte-exact")
    ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    t0 = time.time()
    rows = RA.read_rows(RA.manifests(a.manifest), a.filter)
    keys = sorted(set((r[0], r[3]) for r in rows))
    objs = {}
    with ThreadPoolExecutor(a.jobs) as ex:
        for key, obj, log in ex.map(RA.compile_one, keys):
            objs[key] = obj
            if obj is None:
                print("COMPILE FAIL %s %s\n%s" % (key[0], " ".join(key[1]), log))
    func_vas = {}
    for src, sym, va, flags in rows:
        if not sym.startswith("$L"):
            func_vas.setdefault((src, flags), {})[sym] = va
    orig = RA.Original()
    coff_cache, const_cache = {}, {}
    counts, rows_out, bad_rows, checked, errors = {}, [], [], 0, []
    seen = set()
    for src, sym, va, flags in sorted(rows, key=lambda r: (objs.get((r[0], r[3])) or "", r[2])):
        obj = objs.get((src, flags))
        if not obj or (va, obj) in seen:
            continue
        seen.add((va, obj))
        status, row = RA.compare_row(orig, obj, sym, va, func_vas.get((src, flags)), coff_cache, const_cache)
        if row is None or (status != "MATCH" and not a.all_rows):
            continue
        checked += 1
        srcp = os.path.normpath(os.path.join(ROOT, "match", src))
        try:
            rel = row_relocs(obj, srcp, va, row)
        except Exception as e:      # noqa: BLE001
            errors.append("%08x %s: %s" % (va, src, e))
            continue
        for m in rel:
            counts[m["class"]] = counts.get(m["class"], 0) + 1
        bad = [m for m in rel if m["class"] in BAD]
        ent = {"va": "%08x" % va, "src": os.path.relpath(srcp, ROOT), "sym": sym, "byte_exact": status == "MATCH",
               "mismatches": bad, "copies": [m for m in rel if m["class"] == "copy"], "classes": {}}
        for m in rel:
            ent["classes"][m["class"]] = ent["classes"].get(m["class"], 0) + 1
        rows_out.append(ent)
        if bad:
            bad_rows.append(ent)
        if a.v:
            print("%08x %-40s %s" % (va, sym[:40], " ".join("%s=%d" % kv for kv in sorted(ent["classes"].items()))),
                  flush=True)
    json.dump({"rows": rows_out, "counts": counts, "errors": errors}, open(a.json, "w"), indent=1)
    print("reloc targets: %d rows checked (%s), %.0f s" % (checked, "all rows" if a.all_rows else "byte-exact rows",
                                                          time.time() - t0))
    for k in sorted(counts, key=lambda k: (k not in BAD, k)):
        print("  %-26s %6d relocations" % (k, counts[k]))
    for e in errors:
        print("ERROR %s" % e)
    print("\n%d rows with an addend/target mismatch%s" % (len(bad_rows), ":" if bad_rows else ""))
    for ent in bad_rows:
        print("%s %s  %s%s" % (ent["va"], ent["src"], ent["sym"], "" if ent["byte_exact"] else "  [not byte-exact]"))
        for m in ent["mismatches"]:
            print("    +%-5x %-5s %-7s ours %s  orig %s  %s%+d  [%s]" % (
                m["at"], m["type"], m["class"], m["ours"], m["orig"], m["sym"][:60], m["addend"], m["method"]))
    print("json: %s" % os.path.relpath(a.json, ROOT))
    return 0 if not [e for e in bad_rows if e["byte_exact"]] and not errors else 1


if __name__ == "__main__":
    sys.exit(main())
