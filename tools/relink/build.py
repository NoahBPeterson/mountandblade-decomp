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
  6. literal fix-up   original addresses our source writes as numbers (literal_addrs.py), moved to their new place
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
import argparse, json, os, re, struct, subprocess, sys

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

    def put(self, va, v):
        struct.pack_into("<I", self.d, self.off(va), v)

    def save(self):
        open(self.path, "wb").write(self.d)


def fix_vtables(exe, symmap, rmap, img, lib_names):
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    report = {"patched_slots": 0, "vtables": 0, "skipped": {}, "missing": []}
    redirect = {}
    for vt, e in rmap["vtables"].items():
        if len(e["orig"]) != 1 or vt not in symmap:
            report["missing"].append(vt)
            continue
        ova, ours = int(e["orig"][0], 16), symmap[vt]
        plan, agree, comparable = [], 0, 0
        for k in range(e["slots"]):
            t = img.u32(ova + 4 * k)
            name = defined_at.get(t) or lib_names.get(t)
            if t not in img.start_set or name is None or name not in symmap:
                continue
            want, have = symmap[name], exe.u32(ours + 4 * k)
            if have == symmap.get("__purecall") and want != have:
                plan.append((k, want))       # a class one source declares abstract: no evidence against the match
                continue
            comparable += 1
            agree += want == have
            if want != have:
                plan.append((k, want))
        if comparable and agree * 2 < comparable:
            report["skipped"][vt] = "%d of %d slots agree" % (agree, comparable)
            continue
        for k, want in plan:
            exe.put(ours + 4 * k, want)
        report["patched_slots"] += len(plan)
        report["vtables"] += 1
        redirect[ours] = ova
    # Then point the code at the original's vtables: a class our source declares with fewer virtuals than the
    # original has a short compiled vtable (a call through slot 7 of the 0x4c3290 record read a string literal past
    # its end). The copy of the original's vtable in the glue's .rdata has every slot, relocated to our functions.
    from glue import RDATA
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
    keep = sorted({defined_at[int(st["value"], 16)] for st in L.scan() if int(st["value"], 16) in defined_at})
    # the original's stack (reserve = commit = 0x3e8480): mesh::build_skin alone has a 1 MB frame
    link_extra = ["/MAP:" + T.winpath(map_path), "/STACK:0x3e8480,0x3e8480"] + ["/INCLUDE:" + k for k in keep]
    steps["keep"] = "%d functions reached only through literal addresses kept (/INCLUDE)" % len(keep)
    log = T.link(objs, out, os.path.join(out, "glue"), extra=link_extra, name="mb_standalone.exe", icf=False)
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
        log = T.link(objs, out, os.path.join(out, "glue"), extra=link_extra, name="mb_standalone.exe", icf=False)
    else:
        sys.exit("duplicate definitions not taken from the row's module: %s" % wrong)
    open(os.path.join(out, "link.log"), "w").write(log)
    unres = T.parse_unresolved(log)
    if unres or not os.path.exists(exe_path):
        sys.exit("link failed: %d unresolved (see %s)" % (len(unres), rel(os.path.join(out, "link.log"))))
    steps["link"] = "linked %d objects; %d duplicate definitions, each taken from its row's module" % (len(objs), len(dups))

    import resolve as R
    img = R.Image()
    lib_names = {}
    for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
        p = line.split()
        if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
            lib_names.setdefault(int(p[0], 16), p[1][9:])
    exe = Exe(exe_path)
    vt = fix_vtables(exe, read_map(map_path), rmap, img, lib_names)
    lit = fix_literals(exe, read_map(map_path), rmap, img, lib_names, read_map_by_object(map_path))
    exe.save()
    steps["literals"] = "%d literal image addresses rewritten; %d in dropped functions, %d without a target, %d not " \
                        "where expected" % (lit["patched"], len(lit["missing_function"]), len(lit["no_target"]),
                                            len(lit["changed"]))
    steps["vtables"] = "%d slots rewritten in %d vtables, %d vtable references moved to the original's; %d skipped " \
                       "(match suspect), %d not in the map" % (vt["patched_slots"], vt["vtables"],
                                                             vt["vptr_stores_redirected"], len(vt["skipped"]), len(vt["missing"]))
    steps["rsrc"] = run(["tools/relink/add_rsrc.py", exe_path]).strip()
    json.dump(dict(steps, vtable_report=vt, literal_report=lit), open(os.path.join(out, "build.json"), "w"), indent=1)
    for k, v in steps.items():
        print("%-12s %s" % (k, v.splitlines()[-1] if v else ""))
    print("-> %s" % rel(exe_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
