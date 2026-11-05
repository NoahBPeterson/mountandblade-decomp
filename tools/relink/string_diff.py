#!/usr/bin/env python3
"""string_diff.py: compare the string literals each non-byte-exact row uses with the ones the original function uses.

usage: string_diff.py [--filter SUBSTR] [--json OUT]

call_diff compares calls, not their arguments: label_refresh(leave, "inv_b_leave_h") where the original passes "inv_b_leave" put
the inventory's Return button at (0,0), and no gate saw it. Literals are the arguments a typo or a slip changes most, and config
keys, resource names and script names all arrive this way. For each row this lists the literals only one side references, counted
once per reference. A literal the original builds by a shared object, or that an inlined callee on one side brings in, shows up as
noise; a one-for-one pair of near-identical strings is the shape to look at first. Runs in one process.
"""
import argparse, collections, difflib, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))


def text_at(read, va):
    d = read(va, 256).split(b"\0")[0]
    return d.decode("latin1") if len(d) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in d) else None


def orig_strings(img, md, va):
    lo, hi = img.func_extent(va)
    out = []
    for i in md.disasm(img.read(lo, hi - lo), lo):
        if i.mnemonic == "call" or i.mnemonic.startswith("j"):
            continue
        for m in re.findall(r"0x[0-9a-f]{6,8}\b", i.op_str):
            v = int(m, 16)
            if img.in_image(v) and not img.in_text(v):
                t = text_at(img.read, v)
                if t is not None:
                    out.append(t)
    return out


def our_strings(coff, sym):
    sec, a, b = coff.func_extent(coff.by_name[sym])
    out = []
    for off, si, _t in sec.relocs:
        if not a <= off < b:
            continue
        s = coff.syms[si]
        if not s.name.startswith("??_C@") or s.secno <= 0:
            continue
        data = coff.sections[s.secno - 1].data[s.value:]
        out.append(data.split(b"\0")[0].decode("latin1"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter")
    ap.add_argument("--json")
    a = ap.parse_args()
    os.chdir(ROOT)
    import capstone
    import resolve as R
    import slice as S
    import equiv as E
    from coff import Coff
    from resolve_map import module_rows
    img = R.Image()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    rows = []
    n = 0
    for src, flags, vas in module_rows():
        if a.filter and a.filter not in src:
            continue
        exact = set()
        man = src[:-4] + ".manifest.txt"
        if os.path.exists(man):
            for line in open(man):
                p = line.split()
                if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                    exact.add(int(p[2], 16))
        vas = [v for v in vas if v not in exact]
        if not vas:
            continue
        o, _ = S.compile_obj(src, flags)
        if not o:
            continue
        coff = Coff(o)
        stem = os.path.basename(src)[:-4]
        for va in vas:
            try:
                lo, hi = img.func_extent(va)
                sym, _how = S.find_symbol(coff, stem, va, src, hi - lo)
                if isinstance(sym, list):
                    sym = E.best_by_size(coff, sym, hi - lo) if sym else None
                if sym is None:
                    continue
                co, cu = collections.Counter(orig_strings(img, md, va)), collections.Counter(our_strings(coff, sym))
            except Exception:
                continue
            n += 1
            miss, extra = co - cu, cu - co
            if miss or extra:
                near = []
                for m in miss:
                    c = difflib.get_close_matches(m, list(extra), 1, 0.7)
                    if c:
                        near.append((m, c[0]))
                rows.append({"va": "%08x" % va, "sym": sym, "src": os.path.relpath(src, ROOT), "missing": dict(miss), "extra": dict(extra), "near": near})
    rows.sort(key=lambda r: (-len(r["near"]), -(sum(r["missing"].values()) + sum(r["extra"].values()))))
    for r in rows:
        print("%s  %s  %s" % (r["va"], r["sym"][:50], r["src"]))
        for m, u in r["near"]:
            print("    NEAR  %r  ours %r" % (m, u))
        for k, v in r["missing"].items():
            print("    -  %r%s" % (k, " x%d" % v if v > 1 else ""))
        for k, v in r["extra"].items():
            print("    +  %r%s" % (k, " x%d" % v if v > 1 else ""))
    print("%d rows compared, %d with differing literals, %d with a near pair" % (n, len(rows), sum(1 for r in rows if r["near"])))
    if a.json:
        json.dump(rows, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
