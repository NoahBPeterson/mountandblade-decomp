#!/usr/bin/env python3
"""build.py: link the standalone exe from our source.

usage: build.py [--out work/relink/standalone] [--keep-going]

Steps (each tool's docstring has the details):
  1. resolve_map.py   the original address of every symbol our modules reference
  2. trial_link.py    link our objects alone, to list what is unresolved
  3. glue.py          mb_data.obj (the original's data, labels, aliases) and the import libraries
  4. link             our objects + glue -> mb_standalone.exe, with a /MAP and /OPT:NOICF (every function keeps
                      its own address, so the fix-ups below find the one they look for)
  5. vtable fix-up    see below
  6. literal gate     the build fails when our source writes an original address as a number (literal_addrs.py)
  7. add_rsrc.py      the original's resources (dialogs, icons)
Writes OUT/mb_standalone.exe, OUT/link.log, OUT/mb_standalone.map and OUT/build.json (what each step did).

Vtable fix-up (and redirect: matched vtables' references in our code are moved to the copy of the original's
vtable, which has every slot): a base class of our source often declares placeholder virtuals (MissionUi::d10) that the original's
derived classes override and ours do not, so our compiled vtable of the derived class points at the base's
placeholder, which the glue can alias to only one function. For each of our vtables that resolve_map matched to an
original vtable, every slot is rewritten to our definition of the function in the same slot of the original (by
address); slots whose original function we do not define keep what the compiler put there. A vtable where fewer
than half of the comparable slots already agree is left alone and reported: its match is suspect.
"""
import argparse, bisect, json, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
PY = sys.executable


def run(args):
    r = subprocess.run([PY] + args, cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        sys.exit("%s failed:\n%s%s" % (" ".join(args), r.stdout[-2000:], r.stderr[-2000:]))
    return r.stdout


def read_map(path):
    """{symbol: absolute address} from a link.exe /MAP file (publics, then statics). A name listed at two addresses
    (a static of the same name in two objects) maps to None."""
    out = {}
    for line in open(path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            v = int(m[2], 16)
            out[m[1]] = v if out.get(m[1], v) == v else None
    return out


class Exe:
    def __init__(self, path):
        self.path = path
        self.d = bytearray(open(path, "rb").read())
        pe = struct.unpack_from("<I", self.d, 0x3C)[0]
        nsec, = struct.unpack_from("<H", self.d, pe + 6)
        optsz, = struct.unpack_from("<H", self.d, pe + 20)
        self.base, = struct.unpack_from("<I", self.d, pe + 24 + 28)
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + optsz + 40 * i
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", self.d, o + 8)
            self.secs.append((self.base + va, min(vsize, rsize), rptr))

    def off(self, va):
        for a, n, p in self.secs:
            if a <= va < a + n:
                return p + va - a
        return None

    def u32(self, va):
        o = self.off(va)
        return None if o is None else struct.unpack_from("<I", self.d, o)[0]

    def read(self, va, n):
        o = self.off(va)
        return b"" if o is None else bytes(self.d[o:o + n])

    def put(self, va, v):
        struct.pack_into("<I", self.d, self.off(va), v)

    def save(self):
        open(self.path, "wb").write(self.d)


def slot_equivalent(icf_map, want_name, have_names):
    """A slot whose function is not the one the original's table names, but is the same code: the linker with
    /OPT:ICF (as the original was linked) folds the two to one address. `icf_map` is the map of that trial link."""
    w = icf_map.get(want_name)
    return w is not None and any(icf_map.get(n) == w for n in have_names)


def jmp_target(exe, va, hops=2):
    """Where `jmp rel32` stubs at va lead (None if va is not a stub)."""
    t = None
    for _ in range(hops):
        b = exe.read(va, 5)
        if len(b) < 5 or b[0] != 0xE9:
            break
        va = (va + 5 + struct.unpack_from("<i", b, 1)[0]) & 0xFFFFFFFF
        t = va
    return t


def is_deleting_dtor(name):
    return name.startswith(("??_G", "??_E")) or "head_rec_delete" in name or "destroy_" in name or name.startswith("?destroy@")


def vtable_starts(img):
    """Sorted addresses in .rdata that some dword in the image names (code immediates at any alignment, data
    pointers) and whose first dword is a function start: the original's vtables. Tables sit back to back, so a table
    ends at the next start. (The image carries no base relocations, so the pointers are found by value.)"""
    import numpy as np
    from glue import RDATA, TEXT_LO, TEXT_HI, DATA
    out = set()
    for lo, hi in ((TEXT_LO, TEXT_HI), (RDATA[1], RDATA[2]), (DATA[1], DATA[2])):
        b = np.frombuffer(img.read(lo, hi - lo), dtype=np.uint8)
        if len(b) < 8:
            continue
        w = (b[:-3].astype(np.uint32) | (b[1:-2].astype(np.uint32) << 8) | (b[2:-1].astype(np.uint32) << 16)
             | (b[3:].astype(np.uint32) << 24))
        w = w[(w >= RDATA[1]) & (w < RDATA[2]) & (w % 4 == 0)]
        for v in np.unique(w):
            v = int(v)
            if img.u32(v) in img.start_set:
                out.add(v)
    return sorted(out)


def fix_vtables(exe, symmap, rmap, img, lib_names, icf_map):
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    report = {"patched_slots": 0, "vtables": 0, "skipped": {}, "missing": [], "detail": {}}
    by_addr = {}
    for n_, a_ in symmap.items():
        if isinstance(a_, int):
            by_addr.setdefault(a_, []).append(n_)
    redirect = {}
    vt_starts = vtable_starts(img)
    _sym_addrs = sorted({v for v in symmap.values() if isinstance(v, int)})
    from glue import RDATA
    for vt, e in rmap["vtables"].items():
        if len(e["orig"]) != 1 or vt not in symmap:
            report["missing"].append(vt)
            continue
        ova, ours = int(e["orig"][0], 16), symmap[vt]
        # How many slots the original's table really has: the leading run of function starts. Our own compiled
        # vtable can be shorter than that (a record class whose base's virtuals our source never declared), and a
        # call through a slot past its end then reads whatever .rdata follows (crash 0x3ea66666 = the float 0.325f).
        n_orig = 0
        while n_orig < 512 and ova + 4 * n_orig < (1 << 32):
            if img.u32(ova + 4 * n_orig) not in img.start_set:
                break
            n_orig += 1
        # (tables sit back to back, so that run can swallow the next table: n_true stops at the next pointed-to start;
        # a table something points into the middle of is cut short there, hence the rewrite below keeps n_orig)
        _j = bisect.bisect_right(vt_starts, ova)
        n_true = min(n_orig, (vt_starts[_j] - ova) // 4) if _j < len(vt_starts) else n_orig
        # Our compiled table's length: the distance to the next symbol in the map (MSVC emits vtables side by side),
        # so a source that now declares every virtual of the original gets those slots patched too, and a slot we
        # never wrote is never written past our table's end.
        _sa = _sym_addrs
        _i = bisect.bisect_right(_sa, ours)
        n_ours = (_sa[_i] - ours) // 4 if _i < len(_sa) else 0
        limit = min(max(e["slots"], n_orig), n_ours) if n_ours > 0 else e["slots"]
        plan, agree, comparable = [], 0, 0
        if e["slots"] != n_true:
            report.setdefault("shape", {})[vt] = "ours %d slots, original %d" % (e["slots"], n_true)
        for k in range(limit):
            t = img.u32(ova + 4 * k)
            if t not in img.start_set:
                continue
            name = defined_at.get(t) or lib_names.get(t)
            if name is None or name not in symmap:
                continue
            want, have = symmap[name], exe.u32(ours + 4 * k)
            if want != have and slot_equivalent(icf_map, name, by_addr.get(have) or []):
                comparable += 1          # (not `agree`: the match-suspect test below counts exact slots only)
                continue
            if want != have:
                # a slot bound in source by a forwarding stub (`void C::s() { ((Owner*)this)->f(); }` compiles to
                # `jmp f`): the compiler's vtable points at the stub, the stub at the function the original's slot names
                tgt = jmp_target(exe, have)
                if tgt is not None and (tgt == want or slot_equivalent(icf_map, name, by_addr.get(tgt) or [])):
                    report.setdefault("thunk_slots", []).append([vt, k, name])
                    comparable += 1
                    continue
            if want != have and k == 0 and is_deleting_dtor(name) and any(is_deleting_dtor(n) for n in by_addr.get(have) or []):
                # slot 0 on both sides is a deleting destructor: ours the compiler's, the original's a function the
                # row owner wrote by hand or a different class's folded copy. Not rewritten (the class's own is right
                # for it); listed so the class gets its real destructor chain.
                report.setdefault("dtor_slots", []).append(vt)
                comparable += 1
                continue
            if have == symmap.get("__purecall") and want != have:
                plan.append((k, want))       # a class one source declares abstract: no evidence against the match
                continue
            comparable += 1
            agree += want == have
            if want != have:
                plan.append((k, want))
                if k < n_true:
                    report["detail"].setdefault(vt, []).append(
                        [k, "%08x" % t, name, "%08x" % have, (by_addr.get(have) or ["?"])[:3]])
        if comparable and agree * 2 < comparable:
            report["skipped"][vt] = "%d of %d slots agree" % (agree, comparable)
            # The match is suspect, so do not rewrite slots - but the original's table is a real one: point our vptr
            # stores at the copy of it in the glue, so every slot our code (or the original's) calls runs the
            # original's function relocated to our definition. Without this a source that declares the table's
            # whole length would run it unpatched (the record tables' slot 7/8 read as a no-op instead of always1).
            if n_orig >= 2 and RDATA[1] <= ova < RDATA[2]:
                redirect[ours] = ova
                report.setdefault("redirected_short", {})[vt] = "our %d slots, original %d" % (e["slots"], n_orig)
            continue
        for k, want in plan:
            exe.put(ours + 4 * k, want)
        report["patched_slots"] += len(plan)
        report["vtables"] += 1
        redirect[ours] = ova
        # Our source may declare fewer virtuals than the class really has, so our compiled table is short and a
        # call through a later slot reads the .rdata that follows (a string, or a float: 0x3ea66666 = 0.325f,
        # 0x3cb923a2 = 0.009f). Point the vptr stores at the glue's copy of the original table, which has every
        # slot relocated to our functions.
        if n_orig > e["slots"] and n_orig >= 2 and RDATA[1] <= ova < RDATA[2]:
            report.setdefault("redirected_short", {})[vt] = "our %d slots, original %d" % (e["slots"], n_orig)
    # Then point the code at the original's vtables: a class our source declares with fewer virtuals than the
    # original has a short compiled vtable (a call through slot 7 of the 0x4c3290 record read a string literal past
    # its end). The copy of the original's vtable in the glue's .rdata has every slot, relocated to our functions.
    base = symmap.get("_mb_rdata_base")
    t0, tn, tp = exe.secs[0]
    text = bytes(exe.d[tp:tp + tn])
    moved = 0
    for ours, ova in redirect.items():
        new = struct.pack("<I", base + ova - RDATA[1])
        pat = struct.pack("<I", ours)
        i = text.find(pat)
        while i >= 0:
            exe.d[tp + i:tp + i + 4] = new
            moved += 1
            i = text.find(pat, i + 4)
    report["vptr_stores_redirected"] = moved
    return report


def read_map_by_object(path):
    """{(symbol, module stem): absolute address}: tells apart statics of the same name in different objects."""
    out = {}
    for line in open(path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)\.obj", line)
        if m:
            out[(m[1], m[3].split(":")[-1].rsplit("_", 1)[0])] = int(m[2], 16)
    return out


def fix_literals(exe, symmap, rmap, img, lib_names, by_obj={}):
    """Rewrite the original addresses our source writes as literal numbers (tools/relink/literal_addrs.py): a data
    address to the same place in the copied data (or to our own definition of that global), a code address to our
    function at that address. Each site is checked first: the linked bytes must still hold the original value."""
    import literal_addrs as L
    from glue import RDATA, DATA, BSS
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    static_at = {int(k, 16): tuple(v) for k, v in rmap.get("static_at", {}).items()}
    our_data = {}
    for n, d in rmap["names"].items():
        for va, e in d.items():
            if e["defined"] and not n.startswith(("??_C@", "__real@", "??_7", "??_R", "?_")):
                our_data.setdefault(int(va, 16), n)
    bases = {sec: symmap.get(name) for sec, name in ((RDATA, "_mb_rdata_base"), (DATA, "_mb_data_base"),
                                                    (BSS, "_mb_bss_base"))}
    rep = {"patched": 0, "missing_function": [], "no_target": [], "changed": []}
    for st in L.scan():
        v = int(st["value"], 16)
        f = symmap.get(st["func"])
        if f is None:
            rep["missing_function"].append(st["func"])       # folded or dropped: none of its code runs
            continue
        at = f + st["offset"]
        if exe.u32(at) != v:
            rep["changed"].append("%s+%#x" % (st["func"], st["offset"]))
            continue
        new = None
        if v in our_data and our_data[v] in symmap:
            new = symmap[our_data[v]]
        else:
            for sec, b in bases.items():
                if sec[1] <= v < sec[2] and b is not None:
                    new = b + v - sec[1]
            if new is None and v in img.start_set:
                name = defined_at.get(v) or lib_names.get(v)
                new = symmap.get(name) if name else None
                if new is None and v in static_at:              # a static: the row owner's copy
                    new = by_obj.get(static_at[v])
        if new is None:
            rep["no_target"].append("%08x in %s" % (v, st["func"]))
            continue
        exe.put(at, new)
        rep["patched"] += 1
    return rep


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "standalone"))
    ap.add_argument("--strict-vtables", action="store_true", help="fail when a vtable slot differs from the original's "
                    "(would be rewritten), a vptr store is moved to the original's table, a vtbl_* extern is still "
                    "referenced, or a placeholder virtual is bound by the glue instead of defined in source")
    ap.add_argument("--icf", action="store_true", help="link with /OPT:ICF like the original (identical functions fold)")
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    rel = lambda p: os.path.relpath(p, ROOT)
    steps = {}
    rmap_path = os.path.join(out, "resolve_map.json")
    steps["resolve_map"] = run(["tools/relink/resolve_map.py", "--out", rmap_path]).strip()
    steps["pre_link"] = run(["tools/relink/trial_link.py", "--dir", os.path.join(out, "pre")]).splitlines()[0]
    steps["glue"] = run(["tools/relink/glue.py", "--log", os.path.join(out, "pre", "link.log"), "--map", rmap_path,
                         "--out", os.path.join(out, "glue")]).strip()

    import trial_link as T
    objs, fails = T.objects()
    if fails:
        sys.exit("compile failures: %s" % fails)
    exe_path = os.path.join(out, "mb_standalone.exe")
    map_path = os.path.join(out, "mb_standalone.map")
    for p in (exe_path, map_path):
        if os.path.exists(p):
            os.remove(p)
    rmap = json.load(open(rmap_path))
    owner = {sym: stem for sym, stem in rmap["defined_at"].values()}
    stem_of = lambda o: os.path.basename(o).rsplit("_", 1)[0]
    # a function our code reaches only through a literal address has no reference /OPT:REF can see: keep it
    import literal_addrs as L
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    lits = L.scan()
    # literal gate: our source must name every original address it uses (an extern with its `// 0x...` annotation, a
    # function it declares); a number written as an address points into our own image, and no post-link rewrite fixes it
    text_lits = L.text_literals()
    if lits or text_lits or L.FAILED:
        sys.exit("literal gate: %d literal image addresses in the compiled objects, %d numeric address casts in the "
                 "source, %d sources that did not compile (tools/relink/literal_addrs.py lists them): %s" % (
                     len(lits), len(text_lits), len(L.FAILED),
                     ", ".join(sorted({s["src"] for s in lits} | {p for p, _i, _v in text_lits} | set(L.FAILED))[:12])))
    keep = sorted({defined_at[int(st["value"], 16)] for st in lits if int(st["value"], 16) in defined_at})
    # the original's stack (reserve = commit = 0x3e8480): mesh::build_skin alone has a 1 MB frame
    link_extra = ["/MAP:" + T.winpath(map_path), "/STACK:0x3e8480,0x3e8480"] + ["/INCLUDE:" + k for k in keep]
    steps["keep"] = "%d functions reached only through literal addresses kept (/INCLUDE)" % len(keep)
    log = T.link(objs, out, os.path.join(out, "glue"), extra=link_extra, name="mb_standalone.exe", icf=a.icf)
    # A symbol two objects define (a view header's inline copy, or an out-of-line forwarder, next to the row's own
    # definition): /FORCE:MULTIPLE keeps the first in link order, and a forwarder whose callee aliases back to the
    # same address recurses forever. Link the row owners first, then check that every duplicate came from its owner.
    def chosen_defs():
        got = {}
        for line in open(map_path, errors="replace"):
            m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+(?:f\s+)?(?:i\s+)?(\S+)\.obj", line)
            if m:
                got.setdefault(m[1], m[2].split(":")[-1].rsplit("_", 1)[0])
        return got

    dups = set()
    for _round in range(12):
        dups |= set(re.findall(r"LNK4006: .*? \((\S+)\) already defined", log))
        got = chosen_defs()
        # a symbol missing from the map was folded or dropped (/OPT:ICF, /OPT:REF): no copy of it is called
        wrong = {d: (got[d], owner[d]) for d in dups if d in owner and d in got and got[d] != owner[d]}
        if not wrong:
            break
        stems = [stem_of(o) for o in objs]
        for d, (took, own) in sorted(wrong.items()):       # move the owner just ahead of the module that won
            oi, ti = stems.index(own), stems.index(took)
            if oi > ti:
                objs.insert(ti, objs.pop(oi))
                stems.insert(ti, stems.pop(oi))
        log = T.link(objs, out, os.path.join(out, "glue"), extra=link_extra, name="mb_standalone.exe", icf=a.icf)
    else:
        sys.exit("duplicate definitions not taken from the row's module: %s" % wrong)
    open(os.path.join(out, "link.log"), "w").write(log)
    unres = T.parse_unresolved(log)
    if unres or not os.path.exists(exe_path):
        sys.exit("link failed: %d unresolved (see %s)" % (len(unres), rel(os.path.join(out, "link.log"))))
    steps["link"] = "linked %d objects; %d duplicate definitions, each taken from its row's module" % (len(objs), len(dups))

    # trap gate: glue.py points the original's data at int3 traps for code we do not define. That is only acceptable
    # where nothing we link can reach the slot (tools/relink/trap_report.py has the proof; docs/standalone_traps.md)
    import trap_report as TR
    ntraps, reach = TR.gate(os.path.join(out, "glue", "glue.json"), rmap, map_path, lits)
    if reach:
        sys.exit("trap gate: %d of %d trapped code addresses are reachable from our code (%s); define them (see "
                 "docs/standalone_traps.md, trap_report.py)" % (len(reach), ntraps, ", ".join(e["va"] for e in reach[:12])))
    steps["traps"] = "%d trapped code addresses, all proven unreachable (no data label, literal or vtable reaches them)" % ntraps

    import resolve as R
    img = R.Image()
    lib_names = {}
    for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
        p = line.split()
        if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
            lib_names.setdefault(int(p[0], 16), p[1][9:])
    exe = Exe(exe_path)
    # the oracle for "same code": the same objects linked with /OPT:ICF (not shipped: only its map is read)
    icf_dir = os.path.join(out, "icf")
    os.makedirs(icf_dir, exist_ok=True)
    icf_map_path = os.path.join(icf_dir, "icf.map")
    T.link(objs, icf_dir, os.path.join(out, "glue"), extra=["/MAP:" + T.winpath(icf_map_path), "/STACK:0x3e8480,0x3e8480"]
           + ["/INCLUDE:" + k for k in keep], name="icf.exe", icf=True)
    vt = fix_vtables(exe, read_map(map_path), rmap, img, lib_names, read_map(icf_map_path))
    lit = fix_literals(exe, read_map(map_path), rmap, img, lib_names, read_map_by_object(map_path))
    exe.save()
    steps["literals"] = "%d literal image addresses rewritten; %d in dropped functions, %d without a target, %d not " \
                        "where expected" % (lit["patched"], len(lit["missing_function"]), len(lit["no_target"]),
                                            len(lit["changed"]))
    steps["vtables"] = "%d slots rewritten in %d vtables, %d vtable references moved to the original's; %d skipped " \
                       "(match suspect), %d not in the map" % (vt["patched_slots"], vt["vtables"],
                                                             vt["vptr_stores_redirected"], len(vt["skipped"]), len(vt["missing"]))
    vt["vtbl_externs"] = sorted(n for n in rmap["names"] if n.startswith("?vtbl_"))
    gj = json.load(open(os.path.join(out, "glue", "glue.json")))
    vt["glue_bound_slots"] = sorted(n for n in gj["aliases"] if n in rmap["vtable_slots"])
    vt["slots_differing"] = max(vt["patched_slots"], sum(len(v) for v in vt["detail"].values()))
    steps["vtable_gate"] = ("%d slots differ from the original's table (same code per ICF counts as equal), %d vtables " \
                            "skipped as suspect, %d vptr stores moved to the original's table, %d vtbl_* externs " \
                            "referenced, %d placeholder virtuals bound by the glue" % (
                                vt["slots_differing"], len(vt["skipped"]), vt["vptr_stores_redirected"],
                                len(vt["vtbl_externs"]), len(vt["glue_bound_slots"]))) + \
                            "; %d deleting-destructor slots not the original's function; %d slots bound by forwarding stubs in source" % (
                                len(vt.get("dtor_slots", [])), len(vt.get("thunk_slots", [])))
    steps["rsrc"] = run(["tools/relink/add_rsrc.py", exe_path]).strip()
    json.dump(dict(steps, vtable_report=vt, literal_report=lit), open(os.path.join(out, "build.json"), "w"), indent=1)
    for k, v in steps.items():
        print("%-12s %s" % (k, v.splitlines()[-1] if v else ""))
    if a.strict_vtables and (vt["slots_differing"] or vt["skipped"] or vt["vptr_stores_redirected"]
                             or vt["vtbl_externs"] or vt["glue_bound_slots"] or vt.get("dtor_slots")):
        sys.exit("vtable gate: " + steps["vtable_gate"])
    print("-> %s" % rel(exe_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
