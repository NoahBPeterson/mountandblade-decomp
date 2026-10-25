#!/usr/bin/env python3
"""build.py: link the standalone exe from our source.

usage: build.py [--out work/relink/standalone] [--keep-going]

Steps (each tool's docstring has the details):
  1. resolve_map.py   the original address of every symbol our modules reference
  2. trial_link.py    link our objects alone, to list what is unresolved
  3. glue.py          mb_data.obj (the original's data, labels, aliases) and the import libraries
  4. link             our objects + glue -> mb_standalone.exe, with a /MAP
  5. vtable fix-up    see below
  6. add_rsrc.py      the original's resources (dialogs, icons)
Writes OUT/mb_standalone.exe, OUT/link.log, OUT/mb_standalone.map and OUT/build.json (what each step did).

Vtable fix-up: a base class of our source often declares placeholder virtuals (MissionUi::d10) that the original's
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
    """{symbol: absolute address} from a link.exe /MAP file (the 'Publics by Value' table)."""
    out = {}
    for line in open(path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", line)
        if m:
            out.setdefault(m[1], int(m[2], 16))
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
    return report


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
    log = T.link(objs, out, os.path.join(out, "glue"), extra=["/MAP:" + T.winpath(map_path)], name="mb_standalone.exe")
    open(os.path.join(out, "link.log"), "w").write(log)
    unres = T.parse_unresolved(log)
    if unres or not os.path.exists(exe_path):
        sys.exit("link failed: %d unresolved (see %s)" % (len(unres), rel(os.path.join(out, "link.log"))))
    steps["link"] = "linked %d objects; %d duplicate definitions (/FORCE:MULTIPLE)" % (
        len(objs), len(set(re.findall(r"LNK4006: (.*?) already defined", log))))

    import resolve as R
    img = R.Image()
    lib_names = {}
    for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
        p = line.split()
        if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
            lib_names.setdefault(int(p[0], 16), p[1][9:])
    exe = Exe(exe_path)
    vt = fix_vtables(exe, read_map(map_path), json.load(open(rmap_path)), img, lib_names)
    exe.save()
    steps["vtables"] = "%d slots rewritten in %d vtables; %d skipped (match suspect), %d not in the map" % (
        vt["patched_slots"], vt["vtables"], len(vt["skipped"]), len(vt["missing"]))
    steps["rsrc"] = run(["tools/relink/add_rsrc.py", exe_path]).strip()
    json.dump(dict(steps, vtable_report=vt), open(os.path.join(out, "build.json"), "w"), indent=1)
    for k, v in steps.items():
        print("%-12s %s" % (k, v.splitlines()[-1] if v else ""))
    print("-> %s" % rel(exe_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
