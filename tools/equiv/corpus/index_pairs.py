#!/usr/bin/env python3
"""Index the cross-compiler corpus that build_pairs.py built: one row per function both compilers emitted.

usage: index_pairs.py <dir with index.json, vc71/, vc8/> [--out pairs.json]

Each function of a unit's units.json rows is looked up by its decorated name in both objects and extracted with
tools/equiv/func.obj_func, the loader the prover uses for our side. A row records:
  size_a/size_b, n_a/n_b      code bytes and instructions (a = vc71, b = vc8)
  ident                       same bytes outside relocated fields, and the same relocation targets
  frame_a/frame_b             the bytes the prologue reserves (sub esp, imm; 0 when there is none; enter is rare)
  ebp_a/ebp_b                 an ebp frame (push ebp; mov ebp, esp)
  esc_a/esc_b                 the function takes the address of its own frame (lea r, [esp+d] or [ebp-d]): a frame
                              object escapes to a callee, the case the per-object frame model is for
  cls                         identical | same-frame | frame-size | missing (one side has no such symbol) |
                              dyn-init (a dynamic initialiser: cl 13.10 names them otherwise; not paired)
The objects and the source stay out of the repository (fnv_decomp has no licence); only the index's shape is ours.
"""
import argparse, collections, json, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import func as F                                  # noqa: E402
from capstone import x86 as X                     # noqa: E402
from coff import Coff                             # noqa: E402  (tools/matching via func's sys.path)


def shape(f):
    ins = list(F.MD.disasm(f.code, 0))
    frame, ebp, esc = 0, False, False
    for k, i in enumerate(ins[:6]):
        if i.mnemonic == "push" and i.op_str == "ebp" and k + 1 < len(ins) and ins[k + 1].op_str == "ebp, esp":
            ebp = True
        if i.mnemonic == "sub" and i.op_str.startswith("esp, ") and not frame:
            frame = int(i.op_str.split(", ")[1], 0)
    for i in ins:
        if i.mnemonic == "lea" and len(i.operands) == 2 and i.operands[1].type == X.X86_OP_MEM:
            m = i.operands[1].mem
            if m.index == 0 and (m.base == X.X86_REG_ESP or (ebp and m.base == X.X86_REG_EBP and m.disp < 0)):
                esc = True
    return len(ins), frame, ebp, esc


def same(a, b):
    if len(a.code) != len(b.code) or set(a.relocs) != set(b.relocs):
        return False
    mask = {o + k for o in a.relocs for k in range(4)}
    if any(a.code[i] != b.code[i] for i in range(len(a.code)) if i not in mask):
        return False
    return all(a.relocs[o][1] == b.relocs[o][1] and a.relocs[o][3] == b.relocs[o][3] for o in a.relocs)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--out")
    a = ap.parse_args()
    index = json.load(open(os.path.join(a.dir, "index.json")))
    rows, cnt = [], collections.Counter()
    for u in index:
        if any(v != 0 for v in u["rc"].values()):
            cnt["unit-failed"] += 1
            continue
        objs = {n: os.path.join(a.dir, n, u["stem"] + ".obj") for n in ("vc71", "vc8")}
        names = {n: Coff(p).by_name for n, p in objs.items()}
        for fn in u["functions"]:
            sym = fn["symbol"]
            r = {"source": u["source"], "symbol": sym, "address": fn["address"], "size_retail": fn["size"]}
            # cl 13.10's headers declare __m128 a struct (U), cl 14's a union (T): the same function otherwise
            syms = {"vc8": sym, "vc71": sym if sym in names["vc71"] else sym.replace("T__m128@@", "U__m128@@")}
            if sym.startswith(("??__E", "??__F")):
                r["cls"] = "dyn-init"          # cl 13.10 names a dynamic initialiser/atexit destructor otherwise
            elif not all(syms[n] in names[n] for n in objs):
                r["cls"] = "missing"
            else:
                try:
                    fa, fb = (F.obj_func(objs[n], syms[n]) for n in ("vc71", "vc8"))
                except SystemExit as e:
                    r["cls"], r["why"] = "missing", str(e)[:200]
                    rows.append(r)
                    cnt["missing"] += 1
                    continue
                (na, fra, ea, sa), (nb, frb, eb, sb) = shape(fa), shape(fb)
                r.update(size_a=len(fa.code), size_b=len(fb.code), n_a=na, n_b=nb, frame_a=fra, frame_b=frb,
                         ebp_a=ea, ebp_b=eb, esc_a=sa, esc_b=sb)
                r["cls"] = "identical" if same(fa, fb) else ("same-frame" if fra == frb else "frame-size")
            cnt[r["cls"]] += 1
            rows.append(r)
    out = a.out or os.path.join(a.dir, "pairs.json")
    json.dump(rows, open(out, "w"), indent=0)
    print(dict(cnt))
    big = [r for r in rows if r["cls"] in ("same-frame", "frame-size")]
    for lo, hi in ((0, 100), (100, 1000), (1000, 4000), (4000, 1 << 30)):
        s = [r for r in big if lo <= r["n_b"] < hi]
        print("%5d-%-6s insns: %5d differing pairs, %5d with a frame-size difference, %5d with an escaping frame address"
              % (lo, hi if hi < 1 << 30 else "", len(s), sum(r["cls"] == "frame-size" for r in s),
                 sum(r["esc_a"] or r["esc_b"] for r in s)))


if __name__ == "__main__":
    sys.exit(main())
