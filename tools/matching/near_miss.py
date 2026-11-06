#!/usr/bin/env python3
"""near_miss.py: rank every non-byte-exact row of the src modules by how few instructions stand between it and a match.

usage: near_miss.py [--jobs 2] [--filter SUBSTR] [--out work/near_miss.json]

One compile per module (cached by slice.compile_obj), then each row is aligned the way insn_align.py does it:
identical / changed / only in the original / only in ours. "edits" is the sum of the last three. Each row also gets
a kind, from what the difference survives:
  stack  identical once stack-slot numbers are masked
  regs   identical once registers and stack-slot numbers are masked (register allocation)
  order  the same instructions once masked, in a different order (x87 and scheduling often land here)
  other  different instructions
Rows without a symbol in our object (funclets, COMDAT fragments) are recorded as nosym."""
import argparse, collections, difflib, glob, json, os, re, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
os.chdir(ROOT)
import capstone, coff, equiv as E, slice as S   # noqa: E402

MAX_INSNS = 8000       # difflib is quadratic; a larger row is ranked as unaligned
IMG_LO, IMG_HI = 0x400000, 0x10000000
REG = re.compile(r"\b(e[abcd]x|e[sd]i|ebp|[abcd][xlh]|[sd]il?|bpl?)\b")


def norm(i, relocs, ours, mask_stack=False, mask_regs=False):
    t = i.mnemonic + " " + i.op_str
    if ours and any(i.address <= r < i.address + i.size for r in relocs):
        t = re.sub(r"\[(0x[0-9a-f]+|0)\]", "[G]", t)
        t = re.sub(r"(, |^\S+ )(0x[0-9a-f]+|0)$", r"\1IMM", t)
    if not ours:
        t = re.sub(r"0x[0-9a-f]+", lambda m: "IMM" if IMG_LO <= int(m.group(0), 16) < IMG_HI else m.group(0), t)
        t = t.replace("[IMM]", "[G]").replace(" + IMM]", "]")
    t = re.sub(r"\[0x[0-9a-f]+\]|\[0\]", "[G]", t)
    if i.mnemonic.startswith("j") or i.mnemonic == "call":
        t = i.mnemonic
    if mask_stack:
        t = re.sub(r"\[esp( \+ 0x[0-9a-f]+)?\]", "[esp+S]", t)
    if mask_regs:
        t = REG.sub("R", t)
    return t


def edits(a, b):
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    same = changed = only_o = only_m = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            same += i2 - i1
        elif tag == "replace":
            changed += max(i2 - i1, j2 - j1)
        elif tag == "delete":
            only_o += i2 - i1
        else:
            only_m += j2 - j1
    return same, changed, only_o, only_m


def align(img, c, sym, va):
    s = c.by_name[sym]
    sec = c.sections[s.secno - 1]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    lo, hi = img.func_extent(va)
    O = list(md.disasm(img.read(lo, max(hi - lo, 1)), 0))
    M = list(md.disasm(bytes(sec.data[s.value:s.value + s.size] if getattr(s, "size", 0) else sec.data[s.value:]), 0))
    relocs = {off - s.value for off, _si, _t in sec.relocs}
    out = {"orig": len(O), "ours": len(M), "bytes": hi - lo}
    if len(O) > MAX_INSNS or len(M) > MAX_INSNS:
        out.update(kind="unaligned", edits=abs(len(O) - len(M)) + len(O))
        return out

    def seq(ours, **kw):
        return [norm(i, relocs, ours, **kw) for i in (M if ours else O)]

    same, ch, oo, om = edits(seq(False), seq(True))
    out.update(same=same, changed=ch, only_orig=oo, only_ours=om, edits=ch + oo + om)
    a2, b2 = seq(False, mask_stack=True), seq(True, mask_stack=True)
    a3, b3 = seq(False, mask_stack=True, mask_regs=True), seq(True, mask_stack=True, mask_regs=True)
    if a2 == b2:
        out["kind"] = "stack"
    elif a3 == b3:
        out["kind"] = "regs"
    elif collections.Counter(a3) == collections.Counter(b3):
        out["kind"] = "order"
    else:
        out["kind"] = "other"
    return out


def rows():
    out = {}
    for nm in sorted(glob.glob("src/**/*.nonmatching.txt", recursive=True)):
        src = nm[:-len(".nonmatching.txt")] + ".cpp"
        if os.path.exists(src):
            for line in open(nm, encoding="utf-8", errors="replace"):
                m = re.match(r"^([0-9a-fA-F]{8})\s", line)
                if m:
                    out.setdefault(src, []).append(int(m.group(1), 16))
    return out


def flags_for(src, sid):
    for _s, _sym, _va, fl in S.manifest_rows(sid):
        if fl:
            fl = fl.split() if isinstance(fl, str) else list(fl)
            break
    else:
        fl = S.flags_from_source(src) or list(S.DEFAULT_FLAGS)
    if src.startswith("src/game/") and "/Isrc/game/include" not in fl:
        fl = list(fl) + ["/Isrc/game/include"]
    return fl


def module(src, vas, img):
    sid = os.path.basename(src).rsplit(".", 1)[0]
    res = {}
    obj, log = S.compile_obj(src, flags_for(src, sid))
    if not obj:
        return {"%08x" % v: {"src": src, "kind": "nocompile", "why": (log or "")[-200:]} for v in vas}
    c = coff.Coff(obj)
    for va in vas:
        r = {"src": src}
        try:
            sym, _how = S.find_symbol(c, sid, va, src)
            sym = sym[0] if isinstance(sym, list) and sym else sym
            if not sym:
                r["kind"] = "nosym"
            else:
                r["symbol"] = sym
                r.update(align(img, c, sym, va))
        except Exception as e:                     # one bad row must not lose the module
            r.update(kind="error", why=repr(e)[:200])
        res["%08x" % va] = r
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--filter")
    ap.add_argument("--out", default="work/near_miss.json")
    a = ap.parse_args()
    img = E.context()["img"]
    todo = {s: v for s, v in rows().items() if not a.filter or a.filter in s}
    out = {}
    with ThreadPoolExecutor(a.jobs) as ex:
        for i, res in enumerate(ex.map(lambda kv: module(kv[0], kv[1], img), todo.items())):
            out.update(res)
            print("%d/%d modules, %d rows" % (i + 1, len(todo), len(out)), flush=True)
    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    json.dump(out, open(a.out, "w"), indent=0, sort_keys=True)
    print(collections.Counter(r["kind"] for r in out.values()))
    ranked = sorted((r for r in out.values() if "edits" in r), key=lambda r: r["edits"])
    for n in (1, 2, 5, 10, 25, 50, 100):
        print("edits <= %3d: %4d rows" % (n, sum(r["edits"] <= n for r in ranked)))


if __name__ == "__main__":
    main()
