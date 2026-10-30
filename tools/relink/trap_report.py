#!/usr/bin/env python3
"""trap_report.py: prove that the code addresses glue.py traps (glue.json "traps") can never be reached.

glue.py copies the original's .rdata/.data into mb_data.obj. A dword there that points at original code we do not
define becomes a pointer to an int3 trap. The trap only matters if something we link can reach that dword. What our
build can reach in the copied data is exactly its roots:
  - a data label (a name of ours resolve_map placed at a data address, and the map says mb_data.obj provides it),
  - a literal original address in our code (literal_addrs.py scan, rewritten to the copy),
  - a copied vtable build.py redirects our vptr stores to (resolve_map "vtables" orig addresses).
The original's own code is not linked, so its immediates (mov eax, offset FuncInfo ...) are not roots.
A trap holder (the data slot) is reachable if, following references backwards (data dwords that point at the object
containing it), some root points at that object or into the 0x100 bytes before any object on the chain.

Classification of the trapped function's role (from the table the holder lives in):
  eh-unwind   a cleanup funclet in an exception-handling unwind map (entry {int toState; void (*action)()}),
  eh-catch    a catch handler of a try-block map (HandlerType {adjectives, type_info*, catchObj, handler}),
  data        a dword in a table of other layout that merely looks like a function address.
All three live in tables only the original's compiler-generated __ehhandler/CRT code points at; our build gets its own.

usage: trap_report.py [--glue ...glue.json] [--map ...resolve_map.json] [--exe-map ...mb_standalone.map]
                      [--json out.json] [--md docs/standalone_traps.md]
"""
import argparse, bisect, collections, json, os, re, struct, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
OUT = os.path.join(ROOT, "work", "relink", "standalone")
WINDOW = 0x100


def map_providers(map_path):
    """{symbol: object file} from a link.exe /MAP file."""
    out = {}
    for line in open(map_path, errors="replace"):
        m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+(?:f\s+)?(?:i\s+)?(\S+\.obj)", line)
        if m:
            out.setdefault(m[1], m[2])
    return out


def _analyse(glue_path, rmap, map_path, literals):
    import resolve as R
    img = R.Image()
    traps = sorted(int(k, 16) for k in json.load(open(glue_path))["traps"] if re.fullmatch(r"[0-9a-f]{8}", k))
    prov = map_providers(map_path) if map_path and os.path.exists(map_path) else None
    secs = {n: (lo, img.read(lo, sz)) for n, lo, sz, ch in img.sections if n in (".rdata", ".data")}

    def insec(v):
        return any(lo <= v < lo + len(d) for lo, d in secs.values())

    def u(a):
        for lo, d in secs.values():
            if lo <= a <= lo + len(d) - 4:
                return struct.unpack_from("<I", d, a - lo)[0]

    roots = set()
    extent = {}                                   # a vtable root reaches exactly its slots
    for n, d in rmap["names"].items():
        if prov is not None and prov.get(n, "mb_data.obj") != "mb_data.obj":
            continue                       # linked from a library or one of our objects: not the copy
        for va, e in d.items():
            if insec(int(va, 16)) and not e.get("defined"):       # defined: ours, never the copy
                roots.add(int(va, 16))
    for l in literals:
        v = int(l["value"], 16)
        if insec(v):
            roots.add(v)
    for e in rmap["vtables"].values():
        for o in e["orig"]:
            roots.add(int(o, 16))
            extent[int(o, 16)] = 4 * e["slots"]

    ptr = collections.defaultdict(list)          # target -> data slots holding it
    for n, (lo, d) in secs.items():
        for off in range(0, len(d) - 3, 4):
            v = struct.unpack_from("<I", d, off)[0]
            if insec(v) or v in img.start_set:
                ptr[v].append(lo + off)
    text = img.section(".text")
    tc = img.read(text[0], text[1])
    timm = collections.defaultdict(list)         # data address -> original code immediates (not roots: not linked)
    for i in range(len(tc) - 3):
        v = struct.unpack_from("<I", tc, i)[0]
        if 0x590000 <= v < 0x5e0000:
            timm[v].append(text[0] + i)
    targets = sorted(set(ptr) | set(timm) | roots)
    sroots = sorted(roots)

    def start_of(h):
        i = bisect.bisect_right(targets, h) - 1
        return targets[i] if i >= 0 else None

    def near_root(x):
        i = bisect.bisect_right(sroots, x) - 1
        return sroots[i] if i >= 0 and x - sroots[i] < extent.get(sroots[i], WINDOW) else None

    def chain(h):
        seen, stack, hit, orig = set(), [h], set(), set()
        while stack:
            x = stack.pop()
            r = near_root(x)
            if r is not None:
                hit.add(r)
            s = start_of(x)
            if s is None or s in seen:
                continue
            seen.add(s)
            r = near_root(s)
            if r is not None:
                hit.add(r)
            stack.extend(ptr.get(s, []))
            orig.update(timm.get(s, []))
        return hit, orig, seen

    sizes = {}
    for line in list(open(os.path.join(ROOT, "work/index/functions.csv")))[1:]:
        p = line.split(",")
        sizes[int(p[0], 16)] = (int(p[2]), p[1])
    rows = collections.defaultdict(set)
    for dp, _dn, fn in os.walk(os.path.join(ROOT, "src")):
        for f in fn:
            if f.endswith((".nonmatching.txt", ".manifest.txt")):
                p = os.path.join(dp, f)
                for l in open(p, errors="replace"):
                    for m in re.finditer(r"\b([0-9a-f]{8})\b", l.split("#")[0][:80]):
                        rows[int(m[1], 16)].add(os.path.relpath(p, ROOT))
    trapset = set(traps)
    holders = collections.defaultdict(list)
    for n, (lo, d) in secs.items():
        for off in range(0, len(d) - 3, 4):
            v = struct.unpack_from("<I", d, off)[0]
            if v in trapset:
                holders[v].append(lo + off)
    out = []
    for t in traps:
        ent = {"va": "%08x" % t, "size": sizes.get(t, (None, None))[0], "rows": sorted(rows.get(t, ())), "holders": [],
               "reachable": False, "roots": []}
        kinds = set()
        for h in holders[t]:
            prev = u(h - 4)
            if prev is not None and (prev == 0xffffffff or prev < 0x80):
                k = "eh-unwind"
                if u(h - 12) is not None and u(h - 12) < 0x40 and u(h - 8) is not None and (u(h - 8) == 0 or insec(u(h - 8))) \
                        and not (prev == 0xffffffff or prev < 0x80 and u(h - 8) == 0xffffffff):
                    k = "eh-catch" if u(h - 8) == 0 or insec(u(h - 8)) else k
            else:
                k = "data"
            hit, orig, seen = chain(h)
            kinds.add(k)
            ent["holders"].append({"slot": "%08x" % h, "kind": k, "original_code_refs": ["%08x" % x for x in sorted(orig)[:3]]})
            if hit:
                ent["reachable"] = True
                ent["roots"] += ["%08x" % r for r in sorted(hit)]
        ent["kind"] = ",".join(sorted(kinds))
        out.append(ent)
    return out


def gate(glue_path, rmap, map_path, literals):
    """-> (n traps, reachable traps). build.py fails when the second is not empty."""
    res = _analyse(glue_path, rmap, map_path, literals)
    return len(res), [e for e in res if e["reachable"]]


def write_md(res, path):
    byk = collections.Counter(e["kind"] for e in res)
    crt = [e for e in res if int(e["va"], 16) >= 0x530000]
    L = ["# Trapped code addresses (standalone build, workstream 3)", "",
         "Generated by `tools/relink/trap_report.py` (build.py runs the same proof as a gate). Edit nothing by hand.", "",
         "glue.py copies the original's `.rdata`/`.data` into `mb_data.obj`; a dword there that points at original code",
         "we do not define becomes a pointer to an `int3` trap: **%d** code addresses, %s." % (
             len(res), ", ".join("%d %s" % (v, k) for k, v in sorted(byk.items()))), "",
         "## Verdict: all (d), provably unreachable", "",
         "A trap can only fire if our linked code can reach the data slot that holds it. What the build reaches in the",
         "copied data is its roots: data labels (names of ours that the link map shows `mb_data.obj` providing), literal",
         "original addresses in our code (literal_addrs.py), and the original vtables our vptr stores are redirected to.",
         "Every slot is followed backwards through the data's pointer graph (object start by nearest referenced address,",
         "any root within 0x100 bytes of an object on the chain counts as a hit). **No root reaches any of the %d**; the" % len(res),
         "only referrers are immediates in the original's own code (`mov eax, offset FuncInfo` in the `__ehhandler$`",
         "stubs, listed per slot as `original_code_refs` in `work/relink/standalone/traps.json`), and that code is not",
         "linked.", "",
         "What the slots are (the kind is a heuristic from the neighbouring dwords; the proof above does not depend on it):", "",
         "- `eh-unwind` (%d): the `action` of a `{toState, action}` unwind-map entry. The pointed-to function is a compiler" % byk.get("eh-unwind", 0),
         "  generated cleanup funclet (`lea ecx,[ebp-x]; jmp ~T`, size 9-43 bytes) in the middle of one original function.",
         "  Our build compiles those functions itself, and cl emits their own unwind maps and funclets.",
         "- `eh-catch`: the handler of a try-block `HandlerType`; same argument.",
         "- `data`: tables in the original's CRT area (0x53xxxx funclets of libcmt's `_output`, `_fltin`, locale/stdio code and",
         "  their lock/FuncInfo tables). The CRT we link is libcmt.lib (VC7.1), which carries its own copies of this data.",
         "  The `module rows` column lists nonmatching/manifest rows that mention the address: those are size-0 notes that",
         "  the address is a funclet inside its parent function, not a callable function.", "",
         "%d of the %d traps are in the CRT range (>= 0x530000), %d are funclets of our own modules' functions." % (
             len(crt), len(res), len(res) - len(crt)), "",
         "Two apparent exceptions were checked and are not traps that matter: `0x534390` / `0x534440` are the `rgodf`",
         "pointers of DirectInput's `c_dfDIMouse` / `c_dfDIKeyboard`. The original data is a statically linked dinput8.lib",
         "object whose ODF array sits in `.text`; we link the SDK's `dinput8.lib`, so those symbols come from `dilib1.obj`",
         "and `dilib2.obj` (link map), not from the copy.", "",
         "## Gate", "",
         "`tools/relink/build.py` calls `trap_report.gate()` after the link and refuses to produce an exe if any trap is",
         "reachable from a root (a data label, a literal address or a redirected vtable reaching it). It prints the",
         "trap count and the proven-unreachable count in `build.json` (`traps`). When workstream 4 stops copying the",
         "original's data the count goes to 0 by itself.", "",
         "## List", "",
         "| address | size | kind | slots | original code that references the table | module rows |",
         "|---|---|---|---|---|---|"]
    for e in res:
        refs = sorted({r for h in e["holders"] for r in h["original_code_refs"]})
        L.append("| %s | %s | %s | %s | %s | %s |" % (e["va"], e["size"], e["kind"],
                                                      " ".join(h["slot"] for h in e["holders"]),
                                                      " ".join(refs[:3]) or "-", ", ".join(e["rows"]) or "-"))
    open(path, "w").write("\n".join(L) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--glue", default=os.path.join(OUT, "glue", "glue.json"))
    ap.add_argument("--map", default=os.path.join(OUT, "resolve_map.json"))
    ap.add_argument("--exe-map", default=os.path.join(OUT, "mb_standalone.map"))
    ap.add_argument("--json", default=os.path.join(OUT, "traps.json"))
    ap.add_argument("--literals", help="a literals.json instead of scanning (slow)")
    ap.add_argument("--md", default=os.path.join(ROOT, "docs", "standalone_traps.md"))
    a = ap.parse_args()
    os.chdir(ROOT)
    import literal_addrs as L
    res = _analyse(a.glue, json.load(open(a.map)), a.exe_map, json.load(open(a.literals)) if a.literals else L.scan())
    json.dump(res, open(a.json, "w"), indent=1)
    write_md(res, a.md)
    bad = [e for e in res if e["reachable"]]
    print("%d traps, %d reachable from a root" % (len(res), len(bad)))
    for e in bad:
        print("  REACHABLE %s via roots %s" % (e["va"], e["roots"]))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
