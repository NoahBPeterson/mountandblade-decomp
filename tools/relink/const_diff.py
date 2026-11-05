#!/usr/bin/env python3
"""const_diff.py: compare the numbers each non-byte-exact row uses with the ones the original function uses.

usage: const_diff.py [--filter SUBSTR] [--map work/relink/standalone/resolve_map.json] [--json OUT]

Masked relocations and "equivalent" verdicts do not see a wrong constant: the conversation view at x 0.500085 where the
original stores 0.008 drew it behind the parchment. For each row this counts three kinds of number on each side and
lists what only one side has:
  float   float immediates stored to memory and float / double constants read from memory (by value)
  int     immediates of mov / cmp / test / and / or / push (not stack adjustments or addresses)
  field   displacements of [reg + disp] other than esp / ebp (structure offsets)
A different register allocation does not change these; inlining does, so a row whose callee is inlined on one side lists
noise. Rows with few differences come first: a single pair of near values is the shape to look at. Runs in one process.
"""
import argparse, collections, json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))

INT_OPS = ("mov", "cmp", "test", "and", "or", "push")
MEM_ABS = re.compile(r"(dword|qword) ptr (?:\w+:)?\[(0x[0-9a-f]+|\d+)\]")
MEM_REG = re.compile(r"\[((?:e[a-ds][xpi]|e[sd]i)[^\]]*?) ([+-]) (0x[0-9a-f]+)\]")
IMM = re.compile(r"(?:^|, )(0x[0-9a-f]+|\d+)$")


def as_float(u):
    return struct.unpack("<f", struct.pack("<I", u))[0]


def floaty(u):
    """Does this dword read as an ordinary float rather than an integer or a pointer?"""
    if u in (0, 0x80000000) or u < 0x10000 or 0x400000 <= u < 0x20000000:
        return False
    v = abs(as_float(u))
    return 1e-5 <= v <= 1e7 and (u & 0xff) in (0, 0x0a, 0x3d, 0xcd, 0xcc, 0x6f, 0x71, 0xd7, 0xae, 0x8f) or \
        (1e-5 <= v <= 1e7 and bin(u & 0xfffff).count("1") not in (0, 1))


def rounded(x):
    return float("%.5g" % x)


def scan(md, code, base, read_abs, relocs=None):
    """-> {"float": Counter, "int": Counter, "field": Counter}. relocs: {offset of the dword: value to read as float, or None}"""
    out = {"float": collections.Counter(), "int": collections.Counter(), "field": collections.Counter()}
    for i in md.disasm(code, base):
        ops = i.op_str
        if i.mnemonic == "call" or i.mnemonic.startswith("j") or i.mnemonic == "lea" and "esp" in ops:
            continue
        rel = None
        if relocs is not None:
            for off in range(i.address - base, i.address - base + i.size - 3):
                if off in relocs:
                    rel = (off, relocs[off])
        m = MEM_ABS.search(ops)
        if m and relocs is None:
            v = read_abs(int(m[2], 16), m[1])
            if v is not None:
                out["float"][rounded(v)] += 1
        if rel is not None and rel[1] is not None and m:
            out["float"][rounded(rel[1])] += 1
        for r in MEM_REG.findall(ops):
            if r[0].split()[0] not in ("esp", "ebp"):
                d = int(r[2], 16)
                if r[1] == "+" and 4 <= d < 0x100000:
                    out["field"]["%#x" % d] += 1
        imm = IMM.search(ops)
        if imm and i.mnemonic in INT_OPS and rel is None and not ops.startswith(("esp", "ebp")):
            u = int(imm[1], 0) & 0xffffffff
            if relocs is None and 0x400000 <= u < 0x9000000:
                continue                                # an address of the original
            if floaty(u) and (i.mnemonic == "push" or "ptr [" in ops):
                out["float"][rounded(as_float(u))] += 1
            elif u >= 2 and u < 0x20000000:
                out["int"]["%#x" % u] += 1
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter")
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "standalone", "resolve_map.json"))
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
    addr_of = {}
    if os.path.exists(a.map):
        for n, d in json.load(open(a.map))["names"].items():
            if d:
                addr_of[n] = int(next(iter(d)), 16)

    def read_abs(va, kind):
        if not img.in_image(va) or img.in_text(va):
            return None
        try:
            b = img.read(va, 8 if kind == "qword" else 4)
        except Exception:
            return None
        if len(b) < (8 if kind == "qword" else 4):
            return None
        if kind == "qword":
            v = struct.unpack("<d", b)[0]
            return v if 1e-9 < abs(v) < 1e12 else None
        u = struct.unpack("<I", b)[0]
        return as_float(u) if floaty(u) else None

    rows, n = [], 0
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
                sec, x, y = coff.func_extent(coff.by_name[sym])
                relocs = {}
                for off, si, _t in sec.relocs:
                    if not x <= off < y:
                        continue
                    nm = coff.syms[si].name
                    val = None
                    if nm.startswith("__real@"):
                        hx = nm.split("@", 1)[1]
                        val = as_float(int(hx, 16)) if len(hx) == 8 else struct.unpack("<d", int(hx, 16).to_bytes(8, "little"))[0]
                    elif nm in addr_of and not nm.startswith("??_C@"):
                        val = read_abs(addr_of[nm], "dword")
                    relocs[off - x] = val
                theirs = scan(md, img.read(lo, hi - lo), lo, read_abs)
                ours = scan(md, bytes(sec.data[x:y]), 0, read_abs, relocs)
            except Exception as ex:
                print("%08x skipped: %r" % (va, ex), file=sys.stderr)
                continue
            n += 1
            d = {k: {"orig": dict(theirs[k] - ours[k]), "ours": dict(ours[k] - theirs[k])} for k in theirs}
            if d["float"]["orig"] or d["float"]["ours"] or d["int"]["orig"] or d["int"]["ours"]:
                rows.append({"va": "%08x" % va, "sym": sym, "src": os.path.relpath(src, ROOT), "size": hi - lo, "diff": d})

    def weight(r):
        return sum(sum(r["diff"][k][s].values()) for k in ("float", "int") for s in ("orig", "ours"))
    rows.sort(key=lambda r: (not (r["diff"]["float"]["orig"] and r["diff"]["float"]["ours"]), weight(r)))
    for r in rows:
        print("%s  %s  %s  (%d bytes)" % (r["va"], r["sym"][:60], r["src"], r["size"]))
        for k in ("float", "int", "field"):
            e = r["diff"][k]
            if e["orig"] or e["ours"]:
                print("    %-5s orig %s   ours %s" % (k, sorted(e["orig"].items(), key=str)[:14], sorted(e["ours"].items(), key=str)[:14]))
    print("%d rows compared, %d with differing numbers" % (n, len(rows)))
    if a.json:
        json.dump(rows, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
