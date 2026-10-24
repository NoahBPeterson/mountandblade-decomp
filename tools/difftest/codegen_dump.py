#!/usr/bin/env python3
"""codegen_dump.py: fingerprint the compiled code of every src module row, to tell which functions a
header-only change actually changed.

usage: codegen_dump.py --out FILE.json [--filter substr] [--jobs N=4]
       codegen_dump.py --diff BEFORE.json AFTER.json [--rows]

A row is every VA in src/**/<m>.manifest.txt and <m>.nonmatching.txt. Its fingerprint is the SHA-1 of the
function's bytes, its relocations (offset, type, target symbol name) and its symbol name, from one compile
of the module with the flags equiv.py would use. --diff prints the rows whose fingerprint changed, appeared
or vanished; rows with identical fingerprints compile to the same code against the same symbols, so their
run_all and equiv results cannot change (equiv reads only the function, its relocations and its symbol's
signature). Re-run equiv on the changed nonmatching rows only (--rows prints them as "<module> <va> <src>").
"""
import argparse, glob, hashlib, json, os, re, sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
os.environ.setdefault("DIFFTEST_NO_LEDGER", "1")
import slice as S          # noqa: E402
from coff import Coff      # noqa: E402

VA = re.compile(r"\b([0-9a-fA-F]{8})\b")


def modules(filt):
    out = []
    for man in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True)):
        base = man[:-len(".manifest.txt")]
        src = base + ".cpp"
        if filt and filt not in os.path.relpath(src, ROOT):
            continue
        vas = {}
        for line in open(man, encoding="utf-8", errors="replace"):
            p = line.split()
            if len(p) >= 3 and not line.startswith("#"):
                vas[p[2].lower()] = "exact"
        nm = base + ".nonmatching.txt"
        if os.path.exists(nm):
            for line in open(nm, encoding="utf-8", errors="replace"):
                p = line.split()
                if p and not line.startswith("#") and VA.fullmatch(p[0]):
                    vas.setdefault(p[0].lower(), "nonmatching")
        if os.path.exists(src) and vas:
            out.append((os.path.basename(base), src, vas))
    return out


def fingerprint(coff, s, h, seen):
    """Hash what equiv can execute or read from our object for symbol s: its bytes and relocations, and,
    recursively, everything it references that the object defines (out-of-line inline helpers, COMDAT
    copies, constants, vtables). External targets enter by name; local labels ($SG..., section symbols)
    by content only, so their numbering does not count as a change."""
    key = (s.secno, s.value)
    if key in seen:
        h.update(b"<seen>")
        return
    seen.add(key)
    sec = coff.section_of(s)
    if s.is_func and s.cls in (2, 3):
        _sec, a, b = coff.func_extent(s)
    elif s.cls == 3 and s.value == 0:
        a, b = 0, len(sec.data)       # section symbol: the whole section
    else:                              # a data label: up to the next label in its section
        nxt = [t.value for t in coff.syms.values() if t.secno == s.secno and t.value > s.value]
        a, b = s.value, min(nxt) if nxt else len(sec.data)
    h.update(bytes(sec.data[a:b]))
    for off, si, typ in sorted(r for r in sec.relocs if a <= r[0] < b):
        t = coff.syms.get(si)
        h.update(b"@%d/%d:" % (off - a, typ))
        if t is None:
            h.update(b"?%d" % si)
        elif not t.defined:
            h.update(t.name.encode())
        else:
            if t.cls == 2:
                h.update(t.name.encode())
            h.update(b"{")
            fingerprint(coff, t, h, seen)
            h.update(b"}")


def dump_module(item, img):
    sid, src, vas = item
    rel = os.path.relpath(src, ROOT)
    res = {}
    first = next(iter(vas))
    flags, _how = S.choose_flags(sid, int(first, 16))
    if not any(f.replace("\\", "/").rstrip("/").endswith("src/game/include") for f in flags if f.startswith("/I")):
        flags = list(flags) + ["/Isrc/game/include"]
    obj, log = S.compile_obj(src, flags)
    if obj is None:
        for va, kind in vas.items():
            res["%s %s" % (sid, va)] = {"src": rel, "kind": kind, "fp": "COMPILE-FAILED"}
        return res
    coff = Coff(obj)
    for va, kind in vas.items():
        lo, hi = img.func_extent(int(va, 16))
        sym, how = S.find_symbol(coff, sid, int(va, 16), src, hi - lo)
        if not isinstance(sym, str):
            fp = "NO-SYMBOL " + str(how)[:80]
        else:
            h = hashlib.sha1(sym.encode() + b"\0")
            fingerprint(coff, coff.by_name[sym], h, set())
            fp = h.hexdigest()
        res["%s %s" % (sid, va)] = {"src": rel, "kind": kind, "fp": fp}
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    ap.add_argument("--filter")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--diff", nargs=2)
    ap.add_argument("--rows", action="store_true")
    a = ap.parse_args()
    if a.diff:
        x, y = (json.load(open(p)) for p in a.diff)
        changed = sorted(k for k in set(x) | set(y) if x.get(k, {}).get("fp") != y.get(k, {}).get("fp"))
        for k in changed:
            r = y.get(k) or x.get(k)
            if a.rows:
                if r["kind"] == "nonmatching":
                    print(k, r["src"])
            else:
                print("%-8s %-12s %s  %s -> %s" % ("new" if k not in x else "gone" if k not in y else "changed", r["kind"], k,
                                                   x.get(k, {}).get("fp", "-")[:12], y.get(k, {}).get("fp", "-")[:12]))
        if not a.rows:
            print("%d of %d rows changed" % (len(changed), len(set(x) | set(y))), file=sys.stderr)
        return 0
    if not a.out:
        ap.error("--out or --diff")
    import equiv as E
    img = E.context()["img"]
    out = {}
    with ThreadPoolExecutor(a.jobs) as ex:
        for r in ex.map(lambda m: dump_module(m, img), modules(a.filter)):
            out.update(r)
    json.dump(out, open(a.out, "w"), indent=0, sort_keys=True)
    bad = sum(1 for v in out.values() if not v["fp"][0].isalnum() or v["fp"].startswith(("COMPILE", "NO-")))
    print("%d rows fingerprinted (%d without code: compile failure or no symbol)" % (len(out), bad))


if __name__ == "__main__":
    sys.exit(main())
