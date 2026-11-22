#!/usr/bin/env python3
"""frame_solve: why cl packs a near-miss row's stack frame differently from the original, and what source would move it.

  .venv/bin/python tools/contrib/frame_solve.py 0x<va>                   diagnose (traced compile, then the analysis)
  .venv/bin/python tools/contrib/frame_solve.py 0x<va> --cached          the analysis again on the last kept trace
  .venv/bin/python tools/contrib/frame_solve.py 0x<va> --what EDIT ...   packer what-if on the kept trace (no compile;
                                                                          EDIT as c2slots: uses:NAME=N size: pos: conf+: conf-: add:)
  .venv/bin/python tools/contrib/frame_solve.py 0x<va> search EDITS.txt [--combine K] [--jobs 3] [--apply]
  .venv/bin/python tools/contrib/frame_solve.py 0x<va> score FILE.cpp ...  score whole-file variants

Diagnose runs vs2005-cpp's traced cl 13.10 (c2trace.py slots, docs/c2/slots.md there: the packer's rule and its replay
model) on the row's source, decodes the traced object, aligns it with the original instruction by instruction and
compares every frame operand by its address relative to the return-address slot (the same coordinates on both sides;
no shift is guessed). It prints:
  frame    both frame sizes, which packing rule each side is under (creation order at T <= 128, the sort by
           1000*uses/size above; param homes dropped at T >= 112 in an ESP frame), and where the bytes differ.
  colours  our colours in final order and where the original puts each one's members; the original's slots that
           take members of several of our colours.
  slots    the original's slot map with our elements in each slot, and the slots none of ours reach (an extra
           variable, temporary or a register spill of the original: the stored register's origin is shown, `this`
           for ecx at entry), our slots the original never touches (a value the original keeps in a register).
  elements per element out of place: ours / the original's offset, and the packer's reason (its tries from the
           trace: newest colour first, `size`, `conf X`, the colour not made yet when it was visited), as levers:
           a use count (the packer counts operands before the late flow merges), a separate or a shared local, the
           lifetime overlap with a named element (scope), the visiting order (size, then uses, then first to reach
           the count: the order of first uses).
  edits    the model's best single what-if edits (c2slots search, scored against the original's map) in words.
--pairs lists the aligned instructions whose frame operands differ; --full lifts the 60-slot cap of big frames.

Push depth is exact on both sides (the original: each callee's `ret N` in work/mb.analysis.bin; ours: the calling
convention of each call's relocation symbol), so call arguments do not show up as slots. Validated by hand on 0x43f970
(param-home undo at T >= 112), 0x43fd10 (alt spilled in ours, a register in the original), 0x440070 (the original
spills `this`), 0x44c210 (shape_res/e visiting order follows e's reloads): docs in the rows' .nonmatching.txt notes.
Many frame differences are register-allocation ones (an extra spill on either side changes the element set); use
trace.py regs for those.

search compiles source variants and ranks them by slot agreement first (frame size equal, then the share of aligned
frame operands at the original's address) and check.py's aligned bytes second; scoring is the compile/score loop of
tools/matching/vsearch.py (its Scorer), with the slot score added. EDITS.txt holds candidate edits of the row's source:

  @@ name of the edit
  <<<
  text to replace (exact, unique in the file)
  ===
  replacement
  >>>
  (more <<< === >>> hunks of the same edit)

Every edit is tried alone; --combine K then tries the combinations (up to K at once) of the edits that did not make
the score worse. --apply writes the best variant to the source only if it is strictly closer in bytes or MATCH.
Variant files live in work/vsearch/; the trace files in work/match/trace_<va>* and work/match/fs_<va>/.
"""
import argparse, collections, concurrent.futures as cf, itertools, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import sidecars as SC          # noqa: E402  (where the sidecar files live)
V = os.path.join(os.path.dirname(ROOT), "vs2005-cpp")
sys.path[:0] = [os.path.join(ROOT, "tools", "contrib"), os.path.join(ROOT, "tools", "matching")]
sys.path.append(os.path.join(V, "tools"))        # last: vs2005-cpp's coff.py / card.py must not shadow ours
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
import coff               # noqa: E402,F401  (ours first: slice.py needs it; vs2005-cpp has its own coff.py)
import capstone           # noqa: E402
import trace as TR        # noqa: E402
import c2slots as CS      # noqa: E402
import regmap             # noqa: E402
import tsweep             # noqa: E402

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
FULL = False


# ------------------------------------------------------------------------------------------------ the traced compile
def paths(va):
    d = os.path.join(ROOT, "work", "match", "fs_%08x" % va)
    return d, os.path.join(d, "snap.json"), os.path.join(d, "meta.json")


def run_trace(va, src_override=None):
    """Trace the row's source (or src_override, a variant of it); returns the meta dict."""
    src, sym, fsub, flags, base = TR.prepare(va)
    d, snap, meta = paths(va)
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        if f.endswith((".obj", ".asm", ".log", ".cfg")):
            os.remove(os.path.join(d, f))
    env = dict(os.environ, C2TRACE_REMOTE=os.path.join(ROOT, "tools", "remote", "rcl.py"))
    cmd = ["uv", "run", "tools/c2trace.py", "slots", "--c2", "13.10", src_override or src, "--func", fsub, "--quiet",
           "--keep", d, "--export", snap, "--"] + flags
    r = subprocess.run(cmd, cwd=V, env=env, capture_output=True, text=True, timeout=1800)
    if r.returncode or not os.path.exists(snap):
        sys.exit("frame_solve: the traced compile failed\n" + (r.stdout + r.stderr)[-3000:])
    print(r.stdout.strip().replace(V + os.sep, ""))
    obj = [f for f in os.listdir(d) if f.endswith(".obj")]
    asm = [f for f in os.listdir(d) if f.endswith(".asm")]
    m = dict(va=va, src=src, sym=sym, fsub=fsub, orig=base + ".s", obj=os.path.join(d, obj[0]),
             asm=os.path.join(d, asm[0]))
    json.dump(m, open(meta, "w"), indent=1)
    return m


# ------------------------------------------------------------------------------------------------ instructions
def obj_function(obj, sym):
    """(bytes, relocated offsets) of sym in a COFF object."""
    from cmpobj import parse_coff, REL_SIZES
    secs, syms = parse_coff(obj)
    si, off = syms[sym]
    sec = secs[si]
    nxt = sorted(o for s2, o in syms.values() if s2 == si and o > off)
    data = sec["data"][off:nxt[0] if nxt else len(sec["data"])]
    if nxt:
        data = data.rstrip(b"\xcc")
    rm = {v + k - off for v, _s, t in sec["relocs"] for k in range(REL_SIZES.get(t, 4)) if off <= v + k < off + len(data)}
    return data, rm


def decode(data, base=0):
    return [(i.address, i.mnemonic, i.op_str) for i in MD.disasm(bytes(data), base)]


def obj_call_syms(obj, sym):
    """{function offset of a call instruction: the symbol its rel32 relocation names} for sym in obj."""
    import struct
    from cmpobj import parse_coff
    d = open(obj, "rb").read()
    _m, nsec, _t, symptr, nsym, optsz, _c = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + nsym * 18
    names, i = {}, 0
    while i < nsym:
        o = symptr + 18 * i
        raw = d[o:o + 8]
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            names[i] = d[strtab + off: d.index(b"\0", strtab + off)].decode(errors="replace")
        else:
            names[i] = raw.split(b"\0")[0].decode(errors="replace")
        i += 1 + d[o + 17]
    secs, syms = parse_coff(obj)
    si, off = syms[sym]
    return {v - off - 1: names.get(k, "") for v, k, t in secs[si]["relocs"] if t == 0x14 and v > off}


def callee_pops(name):
    """True when a call to this decorated name leaves the argument cleanup to the callee (thiscall/stdcall/fastcall)."""
    if name.startswith("?"):
        m = re.search(r"@@([A-Z])([A-Z])([A-Z])?", name)
        if not m:
            return True
        a, b, c = m.group(1), m.group(2), m.group(3)
        conv = b if a in "YZCDKLST" else c          # global / static: <scope><conv>; member: <scope><this cv><conv>
        return conv != "A"                          # A cdecl; E thiscall, G stdcall, I fastcall
    if name.startswith("@") or re.search(r"@\d+$", name):
        return True                                 # __fastcall @f@N, __stdcall _f@N
    return False


def frame_keys(ins, callees=None, image_pop=None):
    """{index: [frame keys]} with exact call popping: `callees` (our object: {offset: symbol}) or `image_pop` (the
    original: the callee's `ret N`). Calls are re-spelled so c2slots.orig_slots pops what the callee pops."""
    if callees is not None:
        # a callee the project maps to an original VA pops what its original pops (`ret N`; this also covers
        # by-value struct arguments built with `sub esp, N`, which "pushes since the last call" misses)
        ins2, pops = [], {}
        sv = sym_vas()
        for k, (a, mn, ops) in enumerate(ins):
            if mn == "call" and a in callees:
                nm_ = callees[a]
                if nm_ in sv:
                    pops[0xf0000000 + k] = orig_pop()(sv[nm_])
                    ops = "0x%x" % (0xf0000000 + k)
                elif callee_pops(nm_):
                    ops = "cs%d" % k
                else:
                    pops[0xf0000000 + k] = 0
                    ops = "0x%x" % (0xf0000000 + k)
            ins2.append((a, mn, ops))
        sl, info = CS.orig_slots(ins2, lambda t: pops.get(t, 0))
    else:
        sl, info = CS.orig_slots(ins, image_pop)
    # not frame slots: cl's alignment filler `lea esp, [esp]`; ebp operands when ebp is a general register (no
    # `mov ebp, esp` in the prologue); stores into outgoing arguments below the frame (`fstp [esp]` after `push ecx`)
    ebp_frame = any(mn == "mov" and ops == "ebp, esp" for _, mn, ops in ins[:12])
    alloc = frame_alloc(ins) or 0
    low = -(alloc + 4 * len(saved_regs(ins)) + 4)
    out = {}
    for k, keys in sl:
        mn, ops = ins[k][1], ins[k][2]
        if mn == "lea" and ops.startswith("esp,"):
            continue
        if not ebp_frame and "esp" not in ops:
            continue
        keys = [x for x in keys if ebp_frame or x > low]
        if keys:
            out[k] = keys
    return out, info


_IMAGE_POP = None
_SYM_VAS = None


def sym_vas():
    """{decorated symbol: original VA} from every match/manifests/**/*.manifest.txt row."""
    global _SYM_VAS
    if _SYM_VAS is None:
        import glob
        _SYM_VAS = {}
        for mf in SC.all_manifests():
            for l in open(mf, errors="replace"):
                p = l.split("#", 1)[0].split()
                if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                    _SYM_VAS.setdefault(p[1], int(p[2], 16))
    return _SYM_VAS


def orig_pop():
    global _IMAGE_POP
    if _IMAGE_POP is None:
        _IMAGE_POP = CS.image_callee_pop(os.path.join(ROOT, "work", "mb.analysis.bin"))
    return _IMAGE_POP


def frame_alloc(ins):
    """The frame the prologue allocates (sub esp / __chkstk / push ecx), bytes."""
    if ins and ins[0][1] == "mov":
        m = re.match(r"eax, (0x[0-9a-f]+|\d+)$", ins[0][2])
        if m and len(ins) > 1 and ins[1][1] == "call":
            return CS._num(m.group(1))
    for a, mn, ops in ins[:24]:
        if mn == "sub" and ops.startswith("esp, "):
            try:
                return CS._num(ops[5:])
            except ValueError:
                return None
        if mn == "push" and ops == "ecx" and a == ins[0][0]:
            return 4
        if mn in ("call", "ret") or mn.startswith("j"):
            break
    return 0


def saved_regs(ins):
    m0 = re.match(r"eax, (0x[0-9a-f]+|\d+)$", ins[0][2]) if ins and ins[0][1] == "mov" else None
    pro = CS._prologue(ins, m0)
    return [ops for a, mn, ops in ins[:32] if a in pro and mn == "push"]


# ------------------------------------------------------------------------------------------------ the comparison
def compare(m, s):
    """Per element: our raw key, the original's raw keys (votes); per original key: refs; plus alignment info."""
    data, _ = obj_function(m["obj"], m["sym"])
    ours = decode(data)
    orig = CS.read_orig(m["orig"])
    okeys, oinfo = frame_keys(orig, image_pop=orig_pop())
    ukeys, uinfo = frame_keys(ours, callees=obj_call_syms(m["obj"], m["sym"]))
    lst = tsweep.listing_offsets(m["asm"], s["func"])
    name2i = {L["lname"]: i for i, L in s["listing"].items()}
    pairs = regmap.align([regmap.norm((mn, ops)) for _, mn, ops in ours], [regmap.norm((mn, ops)) for _, mn, ops in orig])
    votes = collections.defaultdict(collections.Counter)        # element -> orig key - our key (per operand)
    mine = collections.defaultdict(collections.Counter)         # element -> our raw key (base + member offset)
    agree = total = 0
    unexplained = collections.Counter()                           # orig raw keys in aligned insns with no element of ours
    seen_orig = collections.Counter()
    for i, j in pairs:
        uk, ok = ukeys.get(i, []), okeys.get(j, [])
        if not uk and not ok:
            continue
        if len(uk) == len(ok):
            total += len(uk)
            agree += sum(1 for a, b in zip(uk, ok) if a == b)
        txt = lst.get(ours[i][0], "")
        ns = [x.group(1) for x in CS.OPSYM.finditer(txt)]
        if len(ns) == len(uk) == len(ok):
            for n, a, b in zip(ns, uk, ok):
                e = name2i.get(n)
                if e is not None:
                    votes[e][b - a] += 1
                    mine[e][a] += 1
    # every frame key the original touches (all instructions, aligned or not)
    for j, ks in okeys.items():
        for k in ks:
            seen_orig[k] += 1
    our_all = collections.Counter(k for ks in ukeys.values() for k in ks)
    return dict(ours=ours, orig=orig, okeys=okeys, ukeys=ukeys, pairs=pairs, votes=votes, mine=mine, agree=agree,
                total=total, oinfo=oinfo, uinfo=uinfo, orig_refs=seen_orig, our_refs=our_all,
                ofr=frame_alloc(orig), ufr=frame_alloc(ours), osaved=saved_regs(orig), usaved=saved_regs(ours))


def show_pairs(m, s, out=sys.stdout, only_bad=True):
    """Every aligned instruction pair with frame operands: ours (listing text, key) against the original's."""
    c = compare(m, s)
    lst = tsweep.listing_offsets(m["asm"], s["func"])
    for i, j in c["pairs"]:
        uk, ok = c["ukeys"].get(i, []), c["okeys"].get(j, [])
        if not uk and not ok or (only_bad and uk == ok):
            continue
        out.write("%5x %-44s %-14s | %08x %-40s %s\n" % (c["ours"][i][0], lst.get(c["ours"][i][0], "?")[:44], uk,
                                                         c["orig"][j][0], (c["orig"][j][1] + " " + c["orig"][j][2])[:40], ok))


def slot_score(ours, orig, okeys=None, callees=None):
    """(frame sizes equal, operands at the original's address, aligned frame operands) for a variant."""
    if okeys is None:
        okeys, _ = frame_keys(orig, image_pop=orig_pop())
    ukeys, _ = frame_keys(ours, callees=callees or {})
    pairs = regmap.align([regmap.norm((mn, ops)) for _, mn, ops in ours], [regmap.norm((mn, ops)) for _, mn, ops in orig])
    agree = total = 0
    for i, j in pairs:
        uk, ok = ukeys.get(i, []), okeys.get(j, [])
        if uk and len(uk) == len(ok):
            total += len(uk)
            agree += sum(1 for a, b in zip(uk, ok) if a == b)
    return frame_alloc(ours) == frame_alloc(orig), agree, total


# ------------------------------------------------------------------------------------------------ diagnosis
def diagnose(m, out=sys.stdout):
    w = out.write
    s = CS.load(paths(m["va"])[1])
    E = s["elems"]
    res = CS.model_of(s).run(CS.base_order(s), record=True)
    bad = CS.check(s, res)
    c = compare(m, s)
    nm = lambda i: CS.ename(s, i)
    ln = lambda i: CS.eline(s, i)
    # our raw key per element (most common) and the packer offset -> raw shift (locals / params)
    # the packer offset -> address shift (locals / params): the most common (our key - packer offset) over every
    # operand; a struct member's operand carries its member offset, base members dominate
    sh, shp = collections.Counter(), collections.Counter()
    for e, v in c["mine"].items():
        if res["offs"].get(e) is None:
            continue
        for a_, n_ in v.items():
            (shp if E[e]["param"] else sh)[a_ - res["offs"][e]] += n_
    SH = sh.most_common(1)[0][0] if sh else 0
    SHP = shp.most_common(1)[0][0] if shp else SH
    raw = lambda e: (res["offs"][e] + (SHP if E[e]["param"] else SH)) if res["offs"].get(e) is not None else None
    orig_of = {}
    for e, v in c["votes"].items():
        if raw(e) is not None:
            d, n = v.most_common(1)[0]
            orig_of[e] = (raw(e) + d, n)
    w("# frame_solve 0x%08x  %s  (%s)\n" % (m["va"], s["func"], os.path.relpath(m["src"], ROOT)))
    w("# model: %s\n" % ("exact" if not bad else "%d differences (%s): the what-ifs are weaker evidence" % (len(bad), "; ".join(bad[:3]))))
    # ---- frame
    T, ofr, ufr = res["T"], c["ofr"], c["ufr"]
    ebp = bool(s["fn"].get("ebp"))
    rule = lambda t: "sorted by 1000*uses/size (T > 128)" if t > 128 else "creation order (T <= 128)"
    w("\n## frame\n")
    w("ours: alloc %s (packer T=%d, %d colours, %s%s), saved %s\n" % (
        hex(ufr) if ufr is not None else "?", T, len(res["cols"]), rule(T),
        ", param homes dropped (undo, T >= 112)" if res["undo"] else "", ",".join(c["usaved"]) or "-"))
    gs = s["fn"].get("gs")
    extra = (ufr or 0) - T                                        # non-packer bytes (EH node, cookie, alignment)
    oT = (ofr or 0) - extra
    w("orig: alloc %s (packer T ~%d, %s), saved %s\n" % (hex(ofr) if ofr is not None else "?", oT,
                                                          rule(oT) if not gs else "sorted (GS)", ",".join(c["osaved"]) or "-"))
    if ofr is not None and ufr is not None and ofr != ufr:
        w("difference: %+d bytes (orig - ours)%s\n" % (ofr - ufr,
          "; the two frames are under DIFFERENT packing rules (the 128-byte line): fix the size first, the order follows"
          if (oT > 128) != (T > 128) and not gs else ""))
        if not ebp and (oT >= 112) != (T >= 112) and s["params"]:
            w("  and on different sides of the 112-byte param-home undo: %s\n" % (
                "the original keeps locals in parameter homes (its T < 112), ours drops them" if oT < 112 else
                "ours keeps locals in parameter homes, the original does not"))
    w("frame operands: %d of %d aligned operands at the original's address (%d of %d instructions aligned)\n" % (
        c["agree"], c["total"], len(c["pairs"]), len(c["orig"])))
    # ---- colour map: our colours (final order) -> the original's addresses of their members
    our_cols0 = {}
    for k0, C in enumerate(res["cols"]):
        for x0 in C["members"]:
            our_cols0[x0] = k0
    w("\n## colour map (ours in final order; colour 0 nearest esp): where the original puts each colour's members\n")
    w("%4s %8s %5s %5s %6s  %s\n" % ("col", "ours", "size", "uses", "key", "original address: members (votes) ..."))
    rev = collections.defaultdict(collections.Counter)
    for k0, C in enumerate(res["cols"]):
        at = collections.defaultdict(list)
        for x0 in C["members"]:
            if x0 in orig_of:
                at[orig_of[x0][0]].append(x0)
                rev[orig_of[x0][0]][k0] += 1
        key0 = 1000 * C["uses"] // C["size"] if C["size"] else 0
        r0 = raw(C["members"][0]) if C["members"] else None
        desc = "  ".join("%d%s: %s" % (a0, "" if a0 == r0 else "(%+d)" % (a0 - r0) if r0 is not None else "",
                                       ",".join(nm(x0) for x0 in sorted(v0, key=lambda q: -E[q]["uses"])[:4]) +
                                       ("+%d" % (len(v0) - 4) if len(v0) > 4 else ""))
                         for a0, v0 in sorted(at.items(), key=lambda t: -len(t[1])))
        w("%4s %8s %5d %5d %6d  %s\n" % (("p" if C.get("param") else "") + str(k0), r0, C["size"], C["uses"], key0,
                                         desc or "(no aligned evidence)"))
    merged = {a0: cs for a0, cs in rev.items() if len(cs) > 1}
    if merged:
        w("original slots holding members of several of our colours (the original packs them together):\n")
        for a0 in sorted(merged):
            w("  %d: our colours %s\n" % (a0, ", ".join("%d(%d)" % (k0, n0) for k0, n0 in sorted(merged[a0].items()))))
    # ---- the original's slots
    by_okey = collections.defaultdict(list)
    for e, (k, n) in orig_of.items():
        by_okey[k].append(e)
    our_cols = {}                                                 # element -> our final colour index
    for k, C in enumerate(res["cols"]):
        for x in C["members"]:
            our_cols[x] = k
    w("\n## the original's slots (address from the return-address slot; ours in the same coordinates)\n")
    w("%7s %5s  %s\n" % ("orig", "refs", "our elements there (our address, colour#) "))
    okeys_all = sorted(set(c["orig_refs"]) | set(by_okey))
    if len(okeys_all) > 60 and not FULL:
        okeys_all = [k for k in okeys_all if by_okey.get(k) or c["orig_refs"].get(k, 0) >= 3][:60]
        w("(the 60 most informative of %d addresses; --full lists all)\n" % len(set(c["orig_refs"]) | set(by_okey)))
    for k in okeys_all:
        if k > 0 and not by_okey.get(k):
            continue
        es = sorted(by_okey.get(k, []), key=lambda e: -E[e]["uses"])
        parts = []
        for e in es[:8]:
            r = raw(e)
            parts.append("%s%s %s c%s" % (nm(e), "" if r == k else "@%s" % r, "ok" if r == k else "<-", our_cols.get(e, "p")))
        inside = [e for e, (b_, n_) in orig_of.items() if b_ < k < b_ + E[e]["size"]]
        if not parts and inside:
            parts = ["(inside %s)" % ", ".join(sorted({nm(e) for e in inside})[:3])]
        if not parts and k <= 0:
            parts = ["(none of ours: %s)" % what_fills(c["orig"], c["okeys"], k)]
        w("%7d %5d  %s%s\n" % (k, c["orig_refs"].get(k, 0), ", ".join(parts) if parts else
                                "(none of ours: an extra variable, temporary or spill of the original)",
                                " +%d" % (len(es) - 8) if len(es) > 8 else ""))
    our_only = sorted(k for k in c["our_refs"] if k not in c["orig_refs"] and k <= 0)
    if our_only:
        w("ours only (addresses the original never touches): %s\n" % ", ".join(str(k) for k in our_only[:20]))
    # ---- elements out of place, with the packer's reasons
    w("\n## elements out of place\n")
    made = {C["created"]: C for C in res["created"]}
    fin = {C["created"]: k for k, C in enumerate(res["cols"])}     # creation number -> final colour index
    cname = lambda cn: "c%s (made #%s)" % (fin.get(cn, "?"), cn)
    vis = {i: k for k, i in enumerate(CS.base_order(s))}
    colour_at = {}                                                # orig key -> our colours whose members sit there in orig
    for e, (k, n) in orig_of.items():
        colour_at.setdefault(k, collections.Counter())[our_cols.get(e)] += 1
    wrong = [e for e in orig_of if raw(e) != orig_of[e][0]]
    wrong.sort(key=lambda e: (-E[e]["uses"] * (1 if not E[e]["param"] else 0), e))
    if not wrong:
        w("none: every element seen in both is at the original's address\n")
    levers = []
    for e in wrong:
        k, n = orig_of[e]
        x = E[e]
        tot = sum(c["votes"][e].values())
        w("%-24s %s size %d uses %d (listing %s) vis %d: ours %s, orig %d (%d/%d votes)\n" % (
            nm(e), ln(e), x["size"], x["uses"], (s["listing"].get(e) or {}).get("lrefs", "-"), vis.get(e, -1), raw(e), k, n, tot))
        mates = [o for o in by_okey.get(k, []) if o != e]
        ours_at = raw(e)
        if ours_at is not None and ours_at <= 0 and not any(
                b_ <= ours_at < b_ + 4 or (k2 <= ours_at < k2 + 4) for k2 in c["orig_refs"] for b_ in [k2]):
            w("    the original never touches our address %d: it has no slot for this value (kept in a register there:"
              " a register-allocation question, trace.py regs), or the slot is elsewhere%s\n" % (
                  ours_at, "; the one aligned operand is weak evidence for %d" % k if tot <= 1 else ""))
            levers.append((e, "%s: register in the original (trace.py regs)" % nm(e)))
            if tot <= 1:
                continue
        elif tot <= 1:
            w("    (weak: one aligned operand; the alignment may pair it with another variable's access)\n")
        if x["param"]:
            w("    a parameter: its home moves only with the push depth / frame; check the votes\n")
            continue
        if k > 0 and not x["param"]:
            pr = [p for p in E if E[p]["param"] and raw(p) == k]
            w("    the original keeps it in the home of parameter %s: in ours %s\n" % (
                ", ".join(nm(p) for p in pr) or "(a parameter slot)",
                "the undo dropped param homes (T=%d >= 112): shrink the frame below 112 first" % T if res["undo"] else
                "it conflicts with that parameter (the parameter is live longer in ours, or it is used after)"))
            levers.append((e, "frame below 112 bytes / shorter parameter lifetime"))
            continue
        lg = res["log"].get(e) or {}
        tries = {cc: r for cc, r in lg.get("tries", [])}
        tgt = [cc for cc in colour_at.get(k, {}) if cc is not None and cc != our_cols.get(e)]
        if mates:
            w("    the original shares the slot with: %s\n" % ", ".join("%s(c%s)" % (nm(o), our_cols.get(o, "p")) for o in mates[:6]))
        # the colours (by creation number) of the original's slot mates in ours
        tcreated = sorted({res["cols"][cc]["created"] for cc in tgt})
        for cn in tcreated:
            C = made.get(cn)
            r = tries.get(cn)
            creator = C["members"][0] if C else None
            if r is None:
                if C is not None and creator is not None and vis.get(creator, 0) > vis.get(e, 0):
                    why = "colour %s did not exist yet: it is made by %s (vis %d, size %d, uses %d), visited after this one" % (
                        cname(cn), nm(creator), vis[creator], E[creator]["size"], E[creator]["uses"])
                    if E[creator]["size"] == x["size"]:
                        lever = "make %s visit before %s: more uses of %s or fewer of %s (same size: uses descending)" % (
                            nm(creator), nm(e), nm(creator), nm(e))
                    elif E[creator]["size"] > 2 * x["size"] and not any(
                            x["size"] < E[o]["size"] < E[creator]["size"] for o in mates):
                        # x is visited first and opens a colour of its size; the larger creator can join it only once
                        # a member of at least half its size has grown it: none of ours is there in the original
                        lever = ("not reachable with these elements: %s (size %d) is visited first and %s (size %d) cannot "
                                 "join a %d-byte colour; the original has a %d..%d-byte element in this slot first (a struct "
                                 "temporary or local we do not have), or %s is a member of a larger local there" % (
                                     nm(e), x["size"], nm(creator), E[creator]["size"], x["size"],
                                     (E[creator]["size"] + 1) // 2, E[creator]["size"], nm(e)))
                    else:
                        lever = "sizes differ (%d vs %d): visiting is size ascending, so %s cannot open it first" % (
                            E[creator]["size"], x["size"], nm(creator))
                else:
                    why = "colour %s was not tried (a newer colour accepted it first: %s)" % (cname(cn), cname(lg.get("res")))
                    lever = "%s must not fit the newer colour %s: overlap its lifetime with a member there, or open the target slot later" % (
                        nm(e), cname(lg.get("res")))
            elif r == "size":
                why = "colour %s refused it for size (%d > 2 x %d)" % (cname(cn), x["size"], C["size"] if C else -1)
                lever = "size of %s or of the slot's first member" % nm(e)
            elif isinstance(r, (list, tuple)) and r[0] == "conf":
                why = "colour %s refused it: lifetime conflict with %s %s" % (cname(cn), nm(r[1]), ln(r[1]))
                lever = "end %s's live range before %s is defined (or start %s later): block scope, a separate local per use, no use after" % (
                    nm(r[1]), nm(e), nm(e))
            else:
                why, lever = "colour %d: %s" % (cn, r), "?"
            w("    why: %s\n    lever: %s\n" % (why, lever))
            levers.append((e, lever))
        if not tcreated:
            if lg.get("new"):
                w("    in ours it opened colour %s; the original's slot holds none of our colours (an element of the original we do not have, or the sort put colours elsewhere)\n" % cname(lg.get("res")))
            else:
                w("    in ours it joined colour %s (%s)\n" % (cname(lg.get("res")), ", ".join(nm(o) for o in made[lg["res"]]["members"][:4])
                                                             if lg.get("res") in made else "?"))
            if T > 128 or oT > 128:
                key = 1000 * x["uses"] // x["size"]
                w("    sorted frame: its colour's key is 1000*uses/size = %d (uses %d)\n" % (key, x["uses"]))
    # ---- colour order (sorted frames): which key would place each colour where the original has it
    if T > 128 or oT > 128:
        w("\n## colour order (sorted: key 1000*uses/size descending, colour 0 nearest esp)\n")
        place = []
        for k2, C in enumerate(res["cols"]):
            if C.get("param"):
                continue
            ks = collections.Counter(orig_of[x][0] for x in C["members"] if x in orig_of)
            if ks:
                place.append((ks.most_common(1)[0][0], k2, C))
        place.sort(key=lambda t: t[0] if not ebp else -t[0])
        ours_order = [k2 for _, k2, _ in place]
        inorder = lis(ours_order)            # the colours already in the original's relative order
        for pos, (ok_, k2, C) in enumerate(place):
            key = 1000 * C["uses"] // C["size"] if C["size"] else 0
            flag = "" if pos in inorder else "  <- out of order"
            mem = ", ".join(nm(x) for x in C["members"][:4])
            w("  orig %6d  our colour %2d  key %6d (uses %3d / size %4d)  %s%s\n" % (ok_, k2, key, C["uses"], C["size"], mem, flag))
            if flag:
                lo = place[pos - 1][2] if pos > 0 else None
                hi = place[pos + 1][2] if pos + 1 < len(place) else None
                # keys must be descending along the orig order
                kmax = 1000 * lo["uses"] // lo["size"] if lo else 10 ** 9
                kmin = 1000 * hi["uses"] // hi["size"] if hi else 0
                umin = next((u for u in range(1, 4000) if kmin <= 1000 * u // C["size"] <= kmax), None)
                umax = next((u for u in range(4000, 0, -1) if kmin <= 1000 * u // C["size"] <= kmax), None)
                if umin:
                    w("      to sit here its key must be in [%d, %d]: uses %d..%d (now %d)\n" % (kmin, kmax, umin, umax, C["uses"]))
    # ---- the model's best single edits against the original's map (in packer coordinates)
    target = {e: k - (SHP if (E[e]["param"] or k > 0) else SH) for e, (k, n) in orig_of.items()}
    om = {e: (target[e], n, sum(c["votes"][e].values())) for e, (k, n) in orig_of.items()}
    buf = __import__("io").StringIO()
    rows = CS.search(s, om, top=12, out=buf)
    w("\n## what-if: the best single packer edits (model, no compile)\n")
    if not rows:
        w("  no single edit puts more elements at the original's address\n")
    for sc, ed in rows[:12]:
        w("  %3d at orig  %-34s %s\n" % (sc, " ".join(ed), lever_text(s, ed)))
    w("\nlevers (source): %s\n" % ("; ".join(sorted({l for _, l in levers}))[:3000] if levers else "-"))
    return dict(agree=c["agree"], total=c["total"], ofr=ofr, ufr=ufr, wrong=len(wrong), seen=len(orig_of))


def lis(seq):
    """Positions of one longest strictly increasing subsequence of seq."""
    import bisect
    tails, idx, prev = [], [], [-1] * len(seq)
    for i, v in enumerate(seq):
        k = bisect.bisect_left(tails, v)
        if k == len(tails):
            tails.append(v); idx.append(i)
        else:
            tails[k] = v; idx[k] = i
        prev[i] = idx[k - 1] if k else -1
    out, i = set(), idx[-1] if idx else -1
    while i >= 0:
        out.add(i)
        i = prev[i]
    return out


def what_fills(ins, okeys, k):
    """A short description of an original slot none of our elements reach: its accesses, and for a store of a register
    where that register's value came from (a spill: `this` when it is ecx at entry)."""
    acc = [j for j in sorted(okeys) if k in okeys[j]]
    txt = "; ".join("%s %s" % (ins[j][1], ins[j][2]) for j in acc[:3]) + (" ..." if len(acc) > 3 else "")
    # its address handed to a call (`lea r, [slot]` then `call X` within a few instructions): a temporary that an
    # inlined helper or a by-value return builds there
    names = {v: n for n, v in sym_vas().items()}
    took = collections.Counter()
    for j in acc:
        if ins[j][1] != "lea":
            continue
        for j2 in range(j + 1, min(j + 8, len(ins))):
            if ins[j2][1] == "call":
                t = ins[j2][2]
                took[names.get(int(t, 16), t) if t.startswith("0x") else t] += 1
                break
    if took:
        txt = "its address goes to %s; %s" % (", ".join("%s x%d" % (n[:60], c) for n, c in took.most_common(3)), txt)
    for j in acc:
        mn, ops = ins[j][1], ins[j][2]
        m = re.match(r"dword ptr \[[^\]]+\], (e[a-z]{2})$", ops) if mn == "mov" else None
        if not m:
            continue
        r = m.group(1)
        for j2 in range(j - 1, -1, -1):
            mn2, ops2 = ins[j2][1], ins[j2][2]
            if ops2.startswith(r + ","):
                src = ops2.split(",", 1)[1].strip()
                origin = "this" if (mn2 == "mov" and src == "ecx" and j2 < 24) else "%s %s" % (mn2, ops2)
                return "a spill of %s (%s); %s" % (r, origin, txt)
        return "a spill of %s (live from entry); %s" % (r, txt)
    return "an extra variable, temporary or spill of the original: " + txt


def lever_text(s, ed):
    out = []
    for x in ed:
        kind, _, arg = x.partition(":")
        names = [CS.ename(s, int(t)) for t in re.findall(r"#(\d+)", arg)]
        if kind == "uses":
            u = int(arg.split("=")[1].rstrip("^"))
            i = int(re.search(r"#(\d+)", arg).group(1))
            d = u - s["elems"][i]["uses"]
            out.append("%s %+d uses (re-read it once more / cache it once less%s)" % (names[0], d, ", reaching the count first" if "^" in arg else ""))
        elif kind == "pos":
            out.append("%s visited at %s (its first uses earlier/later among same-size, same-use elements)" % (names[0], arg.split("=")[1]))
        elif kind == "conf-":
            out.append("%s and %s not live together (narrow a scope, separate local)" % tuple(names[:2]))
        elif kind == "conf+":
            out.append("%s live across %s (wider scope, a use later)" % tuple(names[:2]))
    return "; ".join(out)


# ------------------------------------------------------------------------------------------------ search
def parse_edits(path):
    edits, cur = [], None
    lines = open(path).read().split("\n")
    k = 0
    while k < len(lines):
        l = lines[k]
        if l.startswith("@@"):
            cur = [l[2:].strip(), []]
            edits.append(cur)
        elif l == "<<<":
            a, b, k = [], [], k + 1
            while lines[k] != "===":
                a.append(lines[k]); k += 1
            k += 1
            while lines[k] != ">>>":
                b.append(lines[k]); k += 1
            cur[1].append(("\n".join(a), "\n".join(b)))
        k += 1
    return [(n, h) for n, h in edits if h]


def apply(text, hunks, va=None):
    """text with every hunk replaced once: inside the row's function (from its `// @ 0x<va>` marker to the end of its
    body) when the hunk's text is there exactly once, else in the whole file when it is unique there."""
    import vsearch
    for a, b in hunks:
        span = vsearch.function_span(text, va) if va is not None else None
        if span:
            m = re.search(r"//.*@\s*0x0*%x\b" % va, text, re.I)
            lo, hi = (m.start() if m else span[0]), span[1] + 1
            if text.count(a, lo, hi) == 1:
                k = text.index(a, lo, hi)
                text = text[:k] + b + text[k + len(a):]
                continue
        if text.count(a) != 1:
            return None
        text = text.replace(a, b)
    return text


class SlotScorer:
    """vsearch.Scorer (compile on cl.sh, aligned/positional bytes) plus the slot score of the compiled object."""

    def __init__(self, src, va):
        import vsearch
        flags = vsearch.row_flags(src, va)
        if os.path.relpath(src, ROOT).replace(os.sep, "/").startswith("src/game/") and \
                not any(f.endswith("src/game/include") for f in flags):
            flags = flags + ["/I" + vsearch.winpath(os.path.join(ROOT, "src", "game", "include"))]
        self.vs = vsearch.Scorer(src, va, flags)
        base = os.path.join(ROOT, "work", "match", "trace_%08x.s" % va)
        if not os.path.exists(base):
            TR.orig_listing(va, base)
        self.orig = CS.read_orig(base)
        self.okeys, _ = frame_keys(self.orig, image_pop=orig_pop())
        self.slot = {}
        sc, me = self.vs, self

        def _score_obj(obj, cpp, _orig=sc._score_obj):
            r = _orig(obj, cpp)
            if r is not None:
                try:
                    data, _ = obj_function(obj, r[2])
                    me.slot[cpp] = slot_score(decode(data), me.orig, me.okeys, obj_call_syms(obj, r[2]))
                except Exception as ex:          # noqa: BLE001
                    me.slot[cpp] = (False, 0, 1, str(ex))
            return r
        sc._score_obj = _score_obj

    def why_not(self, text):
        """The compiler's first error line for a variant that does not compile."""
        import vsearch
        cpp = os.path.join(self.vs.dir, "err_%d_%s" % (os.getpid(), os.path.basename(self.vs.src)))
        open(cpp, "w").write(text)
        try:
            r = subprocess.run([vsearch.CL, "/nologo", "/c", *self.vs.flags, vsearch.INCLUDE, "/I" + vsearch.winpath(
                os.path.dirname(self.vs.src)), "/Fo" + vsearch.winpath(cpp[:-4] + ".obj"), vsearch.winpath(cpp)],
                capture_output=True, text=True)
            errs = [l.strip() for l in (r.stdout + r.stderr).splitlines() if " error " in l]
            return (errs[0].split(": ", 1)[-1] if errs else "no object / no symbol")[:150]
        finally:
            for q in (cpp, cpp[:-4] + ".obj"):
                try: os.remove(q)
                except OSError: pass

    def score(self, text):
        import hashlib
        r = self.vs.score(text)
        if r is None:
            return None
        h = hashlib.md5(text.encode()).hexdigest()[:12]
        sl = next((v for k, v in self.slot.items() if "v_%s_" % h in k), None)
        if sl is None:
            return None
        return dict(frame=sl[0], agree=sl[1], total=sl[2], aligned=r[0], diff=r[1])


def rank(r):
    return (not r["frame"], -(r["agree"] / max(1, r["total"])), r["aligned"], r["diff"])


def fmt(r):
    return "frame %s  slots %d/%d (%.1f%%)  aligned %d  diff %d" % ("=" if r["frame"] else "!=", r["agree"], r["total"],
                                                                     100.0 * r["agree"] / max(1, r["total"]), r["aligned"], r["diff"])


def search(va, edits_path, combine, jobs, do_apply, files=()):
    hit = [r for r in TR.T.rows() if r[0] == va]
    if not hit:
        sys.exit("frame_solve: 0x%08x is not a near-miss row" % va)
    src = hit[0][3]
    text = open(src).read()
    sc = SlotScorer(src, va)
    base = sc.score(text)
    if base is None:
        sys.exit("frame_solve: the source does not compile")
    print("base:    %s" % fmt(base), flush=True)
    cands = []
    if edits_path:
        for name, hunks in parse_edits(edits_path):
            t = apply(text, hunks, va)
            if t is None:
                print("  skip %r: a hunk is not found exactly once" % name)
                continue
            cands.append(((name,), t, hunks))
    for f in files:
        cands.append(((os.path.basename(f),), open(f).read(), None))
    results = []

    def run(batch):
        out = []
        with cf.ThreadPoolExecutor(jobs) as ex:
            futs = {ex.submit(sc.score, t): (n, t, h) for n, t, h in batch}
            for fu in cf.as_completed(futs):
                n, t, h = futs[fu]
                r = fu.result()
                print("  %-60s %s" % (" + ".join(n)[:60], fmt(r) if r else "does not compile: " + sc.why_not(t)), flush=True)
                if r:
                    out.append((r, n, t, h))
        return out
    results += run(cands)
    if combine > 1 and edits_path:
        keep = [(n, h) for r, n, t, h in results if h and rank(r) <= rank(base)]
        combos = []
        for kk in range(2, combine + 1):
            for grp in itertools.combinations(keep, kk):
                hunks = [x for _, hs in grp for x in hs]
                t = apply(text, hunks, va)
                if t is not None:
                    combos.append((tuple(n[0] for n, _ in grp), t, hunks))
        if combos:
            print("combinations: %d" % len(combos), flush=True)
            results += run(combos[:400])
    results.sort(key=lambda x: rank(x[0]))
    print("\nbest first:")
    print("  %-60s %s" % ("(base)", fmt(base)))
    for r, n, t, h in results[:15]:
        print("  %-60s %s" % (" + ".join(n)[:60], fmt(r)))
    if do_apply and results:
        bytes_best = min(results, key=lambda x: (x[0]["aligned"], x[0]["diff"]))
        if (bytes_best[0]["aligned"], bytes_best[0]["diff"]) < (base["aligned"], base["diff"]):
            open(src, "w").write(bytes_best[2])
            print("applied (closer in bytes): %s" % " + ".join(bytes_best[1]))
        else:
            print("not applied: nothing is closer in bytes (slot-better variants are listed; judge them by the proof)")


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("va")
    ap.add_argument("mode", nargs="?", default="diag", choices=["diag", "search", "score"])
    ap.add_argument("rest", nargs="*")
    ap.add_argument("--cached", action="store_true")
    ap.add_argument("--src", help="trace this variant of the row's source instead")
    ap.add_argument("--what", nargs="+")
    ap.add_argument("--pairs", action="store_true", help="list the aligned frame operands at different addresses")
    ap.add_argument("--full", action="store_true", help="every slot and element, not the most informative ones")
    ap.add_argument("--combine", type=int, default=1)
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--apply", action="store_true")
    a = ap.parse_args()
    va = int(a.va, 16)
    global FULL
    FULL = a.full
    if a.mode == "search":
        return search(va, a.rest[0] if a.rest else None, a.combine, a.jobs, a.apply)
    if a.mode == "score":
        return search(va, None, 1, a.jobs, False, files=a.rest)
    d, snap, meta = paths(va)
    if a.what:
        CS.what(CS.load(snap), a.what)
        return 0
    m = json.load(open(meta)) if a.cached and os.path.exists(meta) else run_trace(va, a.src)
    if a.pairs:
        show_pairs(m, CS.load(snap))
        return 0
    diagnose(m)
    return 0


if __name__ == "__main__":
    sys.exit(main())
