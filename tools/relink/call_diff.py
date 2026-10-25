#!/usr/bin/env python3
"""call_diff.py: compare what each decompiled function calls with what the original calls, to find rows that
compile but do something else.

usage: call_diff.py [--filter SUBSTR] [--va VA] [--all] [--top N] [--jobs N] [--json OUT]

A row that is not byte-exact can still be wrong in ways no byte diff points at: a record typed as padding (no
tstring constructors where the original's array new runs them), a made-up layout (r_int through a null field where
the original reads two floats), a stand-in helper (strip_tags(quest, 0) where the original strips the quest's
title), blocks in another order (load_data_files read skyboxes.txt's records from the ground-specs stream). All of
them change the calls. For every row of every src/ module this lists the original's calls in order (direct calls
by address, following jump stubs; imports by name; virtual calls by vtable offset) and ours (by the original
address resolve_map.json gives each symbol), then reports the calls only one side makes and whether the shared
ones come in another order.

Calls our compiler expands inline (or the original did) are the main noise: tstring members, small getters. The
score counts virtual-slot and import differences double, since inlining does not explain those. Byte-exact rows
(*.manifest.txt) are skipped unless --all. --va prints both call sequences of one row side by side.
Needs work/relink/standalone/resolve_map.json (tools/relink/build.py writes it).
"""
import argparse, collections, difflib, json, multiprocessing, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
RMAP = os.path.join(ROOT, "work", "relink", "standalone", "resolve_map.json")

_ctx = {}


def bare(name):
    """An import's function name without decoration: __imp__CreateFileA@28, KERNEL32.dll!CreateFileA -> CreateFileA."""
    name = name.split("!")[-1]
    if name.startswith("__imp_"):
        name = name[6:]
    name = name.lstrip("_")
    return re.sub(r"@\d+$", "", name)


def context():
    if not _ctx:
        import capstone
        import resolve as R
        rm = json.load(open(RMAP))
        name_va = {}
        for n, d in rm["names"].items():
            if len(d) == 1:
                name_va[n] = int(next(iter(d)), 16)
        for va, (n, _m) in rm["defined_at"].items():
            name_va.setdefault(n, int(va, 16))
        for va, (n, _m) in rm.get("static_at", {}).items():
            name_va.setdefault(n, int(va, 16))
        # the CRT functions by their library names (symbols/lib_names.txt), as the link binds them
        for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
            p = line.split()
            if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
                name_va.setdefault(p[1][9:], int(p[0], 16))
        # what the link binds a name to wins: glue.json's /alternatename aliases (?_atol@@YAJPBD@Z -> the CRT _atol)
        gj = os.path.join(os.path.dirname(RMAP), "glue", "glue.json")
        if os.path.exists(gj):
            for a_, t in json.load(open(gj))["aliases"].items():
                if t in name_va:
                    name_va[a_] = name_va[t]
        # a virtual method our code calls directly (the object's type is known) is the original's call through
        # that slot: vtable_slots names each method with the slot it fills
        slot_of = {}
        for n, uses in rm["vtable_slots"].items():
            ks = {int(re.search(r"\[(\d+)\]$", u[1])[1]) for u in uses}
            if len(ks) == 1:
                slot_of[n] = "v:+%#x" % (4 * ks.pop())
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        img = R.Image()
        # the same operation under two entry points: operator new / new[] and delete / delete[]
        same = {}
        for group in (("??2@YAPAXI@Z", "??_U@YAPAXI@Z"), ("??3@YAXPAX@Z", "??_V@YAXPAX@Z")):
            vas = [name_va[g] for g in group if g in name_va]
            for v in vas:
                same["f:%08x" % v] = "f:%08x" % vas[0]
        _ctx.update(img=img, rm=rm, name_va=name_va, md=md, slot_of=slot_of, same=same)
    return _ctx


def call_key(ins, target_of):
    """The key of one call instruction: f:<va> direct, imp:<name>, v:+<off> virtual, or None (register / unknown)."""
    from capstone import x86_const as X
    op = ins.operands[0]
    if op.type == X.X86_OP_IMM:
        return target_of(ins, None)
    if op.type == X.X86_OP_MEM:
        if op.mem.base == 0 and op.mem.index == 0:
            return target_of(ins, op.mem.disp & 0xFFFFFFFF)
        if op.mem.base != 0 and op.mem.index == 0:
            return "v:+%#x" % (op.mem.disp & 0xFFFFFFFF)
    return None


def orig_calls(va):
    c = context()
    img, md = c["img"], c["md"]
    lo, hi = img.func_extent(va)

    def target_of(ins, mem):
        if mem is not None:                              # call [iat]
            n = img.imports.get(mem)
            return "imp:" + bare(n) if n else "p:%08x" % mem
        t = ins.operands[0].imm & 0xFFFFFFFF
        for _ in range(4):
            b = img.read(t, 6)
            if b[:2] == b"\xff\x25":                     # import thunk: jmp [iat]
                n = img.imports.get(struct.unpack_from("<I", b, 2)[0])
                return "imp:" + bare(n) if n else "f:%08x" % t
            if b[0] != 0xE9 or t in img.start_set and False:
                break
            t = (t + 5 + struct.unpack_from("<i", b, 1)[0]) & 0xFFFFFFFF
        return "f:%08x" % t

    out, data = [], []
    for ins in md.disasm(img.read(lo, hi - lo), lo):
        if ins.mnemonic == "call":
            k = call_key(ins, target_of)
            if k:
                out.append(k)
        elif not ins.mnemonic.startswith("j"):
            for v in abs_operands(ins):
                if DATA_LO <= v < DATA_HI:
                    data.append("d:%08x" % v)
    return out, data


DATA_LO, DATA_HI = 0x549000, 0x8ee5b94        # the original's .rdata through .bss


def abs_operands(ins, offsets=False):
    """The 4-byte absolute values an instruction carries: a displacement without base or index register (a global),
    a displacement with an index only ([eax*4 + table]), and a mov / push / cmp / lea immediate."""
    from capstone import x86_const as X
    out = []
    if ins.disp_size == 4:
        for o in ins.operands:
            if o.type == X.X86_OP_MEM and o.mem.base == 0:
                out.append((o.mem.disp & 0xFFFFFFFF, ins.disp_offset))
    if ins.imm_size == 4 and ins.mnemonic in ("mov", "push", "cmp", "lea", "add", "sub"):
        for o in ins.operands:
            if o.type == X.X86_OP_IMM:
                out.append((o.imm & 0xFFFFFFFF, ins.imm_offset))
    return out if offsets else [v for v, _k in out]


def our_calls(coff, sym):
    c = context()
    md, name_va = c["md"], c["name_va"]
    sec, a, b = coff.func_extent(coff.by_name[sym])
    relocs = {off: coff.syms[si].name for off, si, _t in sec.relocs}

    def target_of(ins, mem):
        n = relocs.get(ins.address + (2 if mem is not None else 1))
        if n is None:
            return None
        if n.startswith("__imp_"):
            return "imp:" + bare(n)
        if n in c["slot_of"]:
            return c["slot_of"][n]
        va = name_va.get(n)
        return "f:%08x" % va if va else "n:" + n

    out, data = [], []
    for ins in md.disasm(sec.data[a:b], a):
        if ins.mnemonic == "call":
            k = call_key(ins, target_of)
            if k:
                out.append(k)
        elif not ins.mnemonic.startswith("j"):
            for addend, k in abs_operands(ins, offsets=True):
                n = relocs.get(ins.address + k)
                if n is None or n.startswith(("__imp_", "??_7", "??_C@", "__real@", "$")) or n in c["slot_of"]:
                    continue                             # (vtables, literals and floats: placed by the link)
                va = name_va.get(n)
                if va is not None and DATA_LO <= va + addend < DATA_HI:
                    data.append("d:%08x" % (va + addend))
    return out, data


def canon(keys):
    same = context()["same"]
    return [same.get(k, k) for k in keys]


# string constructors / destructors come and go with inlining on either side: a quarter weight each
LIGHT = None


def weight(k):
    global LIGHT
    if LIGHT is None:
        nv = context()["name_va"]
        LIGHT = {"f:%08x" % nv[n] for n in ("??0tstring@@QAE@XZ", "??1tstring@@QAE@XZ", "??0tstring@@QAE@PBD@Z",
                                           "??0tstring@@QAE@ABU0@@Z") if n in nv}
    if k in LIGHT:
        return 0.25
    if k.startswith("n:"):                     # a name the resolver never placed: unknown, not proven different
        return 0.5
    return 2 if k.startswith(("v:", "imp:")) else 1


def compare(o, u):
    """(missing, extra, order_ratio, score): calls only the original makes, only ours makes, and how alike the
    order of the shared ones is (1.0 = same)."""
    o, u = canon(o), canon(u)
    co, cu = collections.Counter(o), collections.Counter(u)
    missing, extra = co - cu, cu - co
    shared = set(co) & set(cu)
    ratio = difflib.SequenceMatcher(None, [k for k in o if k in shared], [k for k in u if k in shared],
                                    autojunk=False).ratio() if shared else 1.0
    score = sum(weight(k) * n for k, n in missing.items()) + sum(weight(k) * n for k, n in extra.items())
    if ratio < 0.9:
        score += (1 - ratio) * 10
    return missing, extra, ratio, round(score)


def one_module(job):
    src, flags, vas, exact = job
    os.chdir(ROOT)
    import slice as S
    import equiv as E
    from coff import Coff
    c = context()
    img = c["img"]
    o, _ = S.compile_obj(src, flags)
    if not o:
        return []
    coff = Coff(o)
    stem = os.path.basename(src)[:-4]
    rows = []
    for va in vas:
        if va in exact:
            continue
        try:
            lo, hi = img.func_extent(va)
            sym, _how = S.find_symbol(coff, stem, va, src, hi - lo)
            if isinstance(sym, list):
                sym = E.best_by_size(coff, sym, hi - lo) if sym else None
            if sym is None:
                continue
            (oc, od), (uc, ud) = orig_calls(va), our_calls(coff, sym)
        except Exception as e:                           # one bad row must not lose the module
            rows.append({"src": os.path.relpath(src, ROOT), "va": "%08x" % va, "error": str(e)})
            continue
        missing, extra, ratio, score = compare(oc, uc)
        # data: the globals and table bounds both sides address (a loop to &table[6] where the original runs to the
        # end of an 11-entry table). Only addresses our side names are comparable; literals and vtables are skipped.
        known = set(ud)
        dmiss = sorted(set(od) - known - set(ud)) if ud else []
        dextra = sorted(set(ud) - set(od))
        dmiss = [d for d in dmiss if any(abs(int(d[2:], 16) - int(u[2:], 16)) < 0x1000 for u in ud)]
        score += len(dmiss) + len(dextra)
        rows.append({"src": os.path.relpath(src, ROOT), "va": "%08x" % va, "sym": sym, "score": score,
                     "order": round(ratio, 2), "missing": dict(missing), "extra": dict(extra),
                     "data_missing": dmiss, "data_extra": dextra, "orig": oc, "ours": uc})
    return rows


def substitutions(r):
    """(Counter of (original call, our call) swapped one for one in place, the remaining missing, the remaining
    extra): a replace block of equal length in the aligned sequences is a call to something else at that point."""
    o, u = canon(r["orig"]), canon(r["ours"])
    subs = collections.Counter()
    sm = difflib.SequenceMatcher(None, o, u, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "replace" and i2 - i1 == j2 - j1:
            for k in range(i2 - i1):
                subs[(o[i1 + k], u[j1 + k])] += 1
    missing, extra = collections.Counter(r["missing"]), collections.Counter(r["extra"])
    for (k, j), n in list(subs.items()):
        n = min(n, missing[k], extra[j])
        if n <= 0:
            del subs[(k, j)]
            continue
        subs[(k, j)] = n
        missing[k] -= n
        extra[j] -= n
    return subs, +missing, +extra


def namer():
    c = context()
    rm = c["rm"]
    by_va = {int(k, 16): v[0] for k, v in rm["defined_at"].items()}
    for n, d in rm["names"].items():
        for va in d:
            by_va.setdefault(int(va, 16), n)

    def name(k):
        if k.startswith("f:"):
            return "%s %s" % (k[2:], by_va.get(int(k[2:], 16), "?")[:60])
        return k[:70]
    return name


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter")
    ap.add_argument("--va")
    ap.add_argument("--all", action="store_true", help="byte-exact rows too")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--min", type=int, default=1, help="smallest score listed")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("--json")
    a = ap.parse_args()
    os.chdir(ROOT)
    from resolve_map import module_rows
    exact = set()
    jobs = []
    want = int(a.va, 16) if a.va else None
    for src, flags, vas in module_rows():
        if a.filter and a.filter not in src:
            continue
        if want is not None:
            if want not in vas:
                continue
            vas = [want]
        man = src[:-4] + ".manifest.txt"
        if os.path.exists(man) and not a.all:
            for line in open(man):
                p = line.split()
                if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                    exact.add(int(p[2], 16))
        jobs.append((src, flags, vas, exact if not a.all else set()))
    if a.jobs > 1 and len(jobs) > 1:
        with multiprocessing.Pool(a.jobs) as pool:
            rows = [r for rs in pool.map(one_module, jobs, chunksize=1) for r in rs]
    else:
        rows = [r for j in jobs for r in one_module(j)]
    name = namer()
    if want is not None:
        for r in rows:
            if "error" in r:
                print(r["error"])
                continue
            print("%s %s  score %d, order %.2f" % (r["va"], r["sym"], r["score"], r["order"]))
            r["orig"], r["ours"] = canon(r["orig"]), canon(r["ours"])
            sm = difflib.SequenceMatcher(None, r["orig"], r["ours"], autojunk=False)
            for tag, i1, i2, j1, j2 in sm.get_opcodes():
                for k in range(max(i2 - i1, j2 - j1)):
                    lo = name(r["orig"][i1 + k]) if i1 + k < i2 else ""
                    ru = name(r["ours"][j1 + k]) if j1 + k < j2 else ""
                    print("%s %-72s | %s" % ("  " if tag == "equal" else "!!", lo, ru))
        return 0
    rows = [r for r in rows if "error" not in r]
    rows.sort(key=lambda r: -r["score"])
    if a.json:
        json.dump(rows, open(a.json, "w"), indent=1)
    flagged = [r for r in rows if r["score"] >= a.min]
    for r in flagged[:a.top]:
        print("%3d  %s  %-50s %s  order %.2f" % (r["score"], r["va"], r["sym"][:50], r["src"], r["order"]))
        subs, missing, extra = substitutions(r)
        for (k, j), n in sorted(subs.items()):
            print("       ~ %s  ->  %s%s" % (name(k), name(j), " x%d" % n if n > 1 else ""))
        for k, n in sorted(missing.items()):
            print("       - %s%s" % (name(k), " x%d" % n if n > 1 else ""))
        for k, n in sorted(extra.items()):
            print("       + %s%s" % (name(k), " x%d" % n if n > 1 else ""))
        for d in r.get("data_missing", []):
            print("       - data %s" % name("f:" + d[2:]).replace("?", "", 1))
        for d in r.get("data_extra", []):
            print("       + data %s" % name("f:" + d[2:]).replace("?", "", 1))
    print("%d rows compared, %d with differing calls (score >= %d)" % (len(rows), len(flagged), a.min))
    return 0


if __name__ == "__main__":
    sys.exit(main())
