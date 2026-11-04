#!/usr/bin/env python3
"""data_coverage.py: how much of the original's static data (.rdata, .data, .bss) our source defines.

usage: data_coverage.py [--record] [--map work/relink/resolve_map.json] [--json OUT] [--from-build DIR [--commit SHA] [--record-data]]
       (run tools/relink/resolve_map.py first; --record --from-build also writes work/relink/latest_build.json (the dir's absolute path); --record appends a point to docs/data_history.json, which
        tools/progress.py prints. --from-build DIR reads DIR/resolve_map.json and DIR/build.json (a tools/relink/build.py
        output); --record then appends the build's crutch counts to docs/standalone_history.json instead (which
        tools/progress.py --plot-standalone draws), and data_history.json only with --record-data)

Function coverage says nothing about globals: a module that declares `extern int g_x; // 0x5e2464` compiles and
can match byte-exact while nothing in src/ defines g_x. The standalone link then takes that storage from the
original image (tools/relink/glue.py). This tool measures the split.

Items: the original image has no symbols, so data items are recovered from their uses. Every address in
.rdata/.data/.bss that an instruction of the original uses (a memory operand's displacement, or a 4-aligned
immediate), or that our resolution map names, starts an item; an item extends to the next start, so storage that
pointer tables in the data reach (runs of 4+ pointers: the pages of a pool) starts its own items.
An address that game code uses but that our resolution map does not name (a field offset into a global, such as
label+4) is taken as part of the item before it. Sizes are therefore approximate: an item absorbs any padding and
any unreferenced data up to the next start.

Classes, per item:
  defined in source       our source defines a global at this address (a named definition the resolver mapped)
  generated from source   our objects define it as compiler output: string literals, floating-point constants,
                          vtables and RTTI
  borrowed from image     game code uses it, our source only declares it (extern) or names it nowhere
  reached through tables  no instruction uses it; a pointer table in the data does (the pages of a pool)
  library data            used only by the static CRT/C++ library (linked from libcmt/libcpmt, not decompiled)
Not an item: data the glue does not copy (glue.py DEAD_RANGES: the original's CRT tables, strings only the original's
own pointer tables reached, the dead activation library); it is reported as "not copied (dead)".
The import address table is excluded.
"""
import argparse, bisect, collections, contextlib, datetime, io, json, os, re, runpy, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

HIST = os.path.join(ROOT, "docs", "data_history.json")
SA_HIST = os.path.join(ROOT, "docs", "standalone_history.json")
# crutch -> (build.json step, pattern): the counts tools/relink/build.py prints, one per way the standalone link
# still borrows from the original image (data_definitions is the one that should go up: source defining the
# original's addresses by name)
CRUTCHES = (("data_labels", "glue", r"data labels (\d+)"), ("aliases", "glue", r"aliases (\d+)"),
            ("source_aliases", "source_aliases", r"(\d+) /alternatename directives"),
            ("traps", "traps", r"(\d+) trapped code addresses"),
            ("redirects", "glue", r"'code->trap': (\d+)"),
            ("vtable_slots_rewritten", "vtables", r"(\d+) slots rewritten"),
            ("vptr_stores_moved", "vtable_gate", r"(\d+) vptr stores moved"),
            ("vtbl_externs", "vtable_gate", r"(\d+) vtbl_\* externs"),
            ("placeholder_virtuals", "vtable_gate", r"(\d+) placeholder virtuals"),
            ("forwarding_stubs", "vtable_gate", r"(\d+) slots bound by forwarding stubs"),
            ("data_definitions", "data", r"(\d+) data definitions at original addresses"))
CLASSES = ("defined in source", "generated from source", "borrowed from image", "reached through tables", "library data")
GLUE_MODULES = ("mb_data",)          # the glue's object: the original's bytes laid out under labels, never a definition
GENERATED = ("??_C@", "__real@", "__int64@", "__xmm@", "__mask@", "??_7", "??_R", "$SG")


def game_functions():
    argv, sys.argv = sys.argv, [os.path.join(ROOT, "tools", "status.py"), "--no-write"]
    with contextlib.redirect_stdout(io.StringIO()):
        st = runpy.run_path(os.path.join(ROOT, "tools", "status.py"))
    sys.argv = argv
    return st["starts"], st["game"]


def link_map_symbols(map_path):
    """{mangled name: (size, module)} for the data symbols of a link map (tools/relink/build.py --out DIR/mb_standalone.map).
    The module is the object file's stem without its content hash (mesh_start_2dbc0c289283.obj -> mesh_start), i.e. the
    source module that defines the symbol; the size is the distance to the next symbol of the same segment (None for the
    last one), so it includes alignment padding but never the next object."""
    row = re.compile(r"\s*(\w{4}):(\w{8})\s+(\S+)\s+[0-9a-f]{8}\s+(?:f\s+)?(?:i\s+)?(\S+)\.obj")
    ents = collections.defaultdict(list)
    for line in open(map_path, errors="replace"):
        m = row.match(line)
        if m and m.group(1) != "0001":
            ents[m.group(1)].append((int(m.group(2), 16), m.group(3), re.sub(r"_[0-9a-f]{12}$", "", m.group(4))))
    out = {}
    for seg, lst in ents.items():
        offs = sorted({o for o, _n, _m in lst})
        for o, n, mod in lst:
            i = bisect.bisect_right(offs, o)
            sz = max(offs[i] - o, out.get(n, (0, 0))[0] or 0) if i < len(offs) else out.get(n, (None, 0))[0]
            out[n] = (sz, mod)
    return out


def data_items(img, rmap, starts, game, first_user=None, syms=None, users=None):
    """[[start, end, class]] items of the static data (see the module docstring), plus the instruction uses
    {address: {'game'|'lib'}} and the names {address: [defined, [names]]}. gen_data.py reuses this.
    first_user, when given, is filled with {address: the lowest-addressed game function that uses it}; users, with
    {address: set of the game functions that use it}. syms ({name: (size, module)} from link_map_symbols): an item
    that our source defines takes the defined symbol's whole extent, absorbing the items inside it."""
    import capstone
    from glue import RDATA, DATA, BSS, IAT_LO, IAT_HI, section_of
    lo_all, hi_all = RDATA[1], BSS[2]

    def in_data(v):
        return lo_all <= v < hi_all and not IAT_LO <= v < IAT_HI and section_of(v) is not None

    # uses by instructions: address -> {"game", "lib"}. A memory operand's displacement always counts; a bare
    # immediate (push / mov reg, imm) only when 4-aligned, since most unaligned ones are constants, not addresses
    uses = collections.defaultdict(set)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    tok = re.compile(r"\[[^\]]*\]|0x[0-9a-f]+")
    t0, tn = img.section(".text")
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else t0 + tn
        owner = "game" if s in game else "lib"
        for ins in md.disasm(img.read(s, e - s), s):
            for t in tok.findall(ins.op_str):
                mem = t.startswith("[")
                for h in re.findall(r"0x([0-9a-f]+)", t):
                    v = int(h, 16)
                    # library immediates into .bss are constants (strlen's 0x01010100 masks), not addresses
                    if in_data(v) and (mem or (v % 4 == 0 and (owner == "game" or section_of(v) is not BSS))):
                        uses[v].add(owner)
                        if first_user is not None and owner == "game":
                            first_user.setdefault(v, s)
                        if users is not None and owner == "game":
                            users.setdefault(v, set()).add(s)

    # pointer tables: runs of 4+ consecutive dwords that all point into the data (a page table of a pool, a list of
    # strings); a lone dword that happens to hold such a value is more likely an integer
    tabled = set()
    for sec in (RDATA, DATA):
        data = img.read(sec[1], sec[2] - sec[1])
        run = []
        for off in range(0, len(data) - 3, 4):
            v = struct.unpack_from("<I", data, off)[0]
            if in_data(v) and v % 4 == 0 and not IAT_LO <= sec[1] + off < IAT_HI:
                run.append(v)
                continue
            if len(run) >= 4:
                tabled.update(run)
            run = []
        if len(run) >= 4:
            tabled.update(run)

    named = collections.defaultdict(lambda: [False, []])      # va -> [defined anywhere, names]
    for n, d in rmap["names"].items():
        for va, e in d.items():
            v = int(va, 16)
            if in_data(v):
                # defined: a row of the defining module references it (the resolver), or the link map shows one of our
                # objects defining it (a definition no row of its own module names: g_strtable in mesh_2.cpp)
                named[v][0] |= e["defined"] or bool(syms and n in syms and syms[n][1] not in GLUE_MODULES)
                named[v][1].append(n)

    def classify(v):
        u = uses.get(v, set())
        if "game" in u or v in named:
            if v in named and named[v][0]:
                defs = named[v][1]
                return "generated from source" if all(n.startswith(GENERATED) for n in defs) else "defined in source"
            return "borrowed from image"
        if u:
            return "library data"
        return "reached through tables"

    bounds = sorted(set(uses) | set(named) | tabled)
    items = []                       # [start, end, class]
    for i, v in enumerate(bounds):
        sec = section_of(v)
        nxt = bounds[i + 1] if i + 1 < len(bounds) and bounds[i + 1] < sec[2] else sec[2]
        cls = classify(v)
        unnamed_game = "game" in uses.get(v, set()) and v not in named
        if unnamed_game and items and items[-1][1] == v and section_of(items[-1][0]) is sec:
            items[-1][1] = nxt       # a field of the item before
            continue
        items.append([v, nxt, cls])
    if syms:
        items = _absorb(items, named, syms, section_of)
    return items, uses, named


def drop_dead(items):
    """The items without the ones inside a range the glue does not copy (glue.py DEAD_RANGES)."""
    from glue import DEAD_RANGES
    return [it for it in items if not any(lo <= it[0] and it[1] <= hi for lo, hi, _why in DEAD_RANGES)]


def _absorb(items, named, syms, section_of):
    """An item our source defines spans its symbol's whole size (the link map's), not just up to the next use: the
    items that start inside it are part of it (one that sticks out past the end keeps its tail). [start, end, class]."""
    out, cover = [], 0
    for s, e, c in items:
        if s < cover:
            if e <= cover:
                continue
            s = cover
        if c == "defined in source":
            ext = max([syms[n][0] or 0 for n in (named[s][1] if s in named else []) if n in syms] or [0])
            if ext > e - s:
                e = min(s + ext, section_of(s)[2])
                cover = max(cover, e)
        out.append([s, e, c])
    return out


def item_owners(items, first_user, named, rmap, syms=None, users=None):
    """[(function VA or None, module or None, {functions})] per item: the lowest-addressed game function that uses any
    address of the item; the module that defines the item (a named item: the object file the link map puts its symbol
    in, syms from link_map_symbols; without one, the module of the first row that references its name); and, when
    users is given, the set of game functions that use the item (progress.py picks the module of most of them for the
    items our source does not define)."""
    addrs = sorted(first_user)
    out = []
    for s, e, _c in items:
        i, j = bisect.bisect_left(addrs, s), bisect.bisect_left(addrs, e)
        fn = min((first_user[a] for a in addrs[i:j]), default=None)
        fns = set()
        for a in addrs[i:j]:
            fns |= (users or {}).get(a, set())
        mod = None
        names = named[s][1] if s in named else []
        for n in names:
            if syms and n in syms:
                mod = syms[n][1]
                break
        for n in names if mod is None else ():
            rows = rmap["names"][n].get("%08x" % s, {}).get("rows")
            if rows:
                mod = rows[0].split()[0]
                break
        out.append((fn, mod, fns))
    return out


_TYPE_OF = re.compile(r"^\?[^@]+@(?:[^@]+@)*@3[UV]([A-Za-z_]\w*)@@[A-Z]")
_HDRS = None


def type_of_symbol(name):
    """The struct/class a mangled data symbol is an object of (?g_terrain@@3UTerrain@@A -> Terrain), else None."""
    m = _TYPE_OF.match(name)
    return m.group(1) if m else None


def struct_members(typename, size):
    """[(offset, size, name)] of a struct/class defined in src/ headers whose members carry `// +0x...` offset comments
    (the member's size is the distance to the next annotated one, the last runs to `size`), or None when no definition has
    any. Of several definitions of one name the one with most annotations wins; members inside a nested union or struct
    are skipped except the first annotated one (a union's first member), and an offset seen twice keeps the first."""
    global _HDRS
    if _HDRS is None:
        import glob
        _HDRS = {}
        for f in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
            _HDRS[f] = open(f, errors="replace").read().split("\n")
    best = None
    head = re.compile(r"^\s*(?:struct|class)\s+%s\b[^;{]*\{" % re.escape(typename))
    for f, lines in _HDRS.items():
        for i, ln in enumerate(lines):
            if not head.match(ln):
                continue
            stack, mem, done = [i], [], set()          # open blocks (by the line that opened them); done: blocks that gave a member
            for j in range(i + 1, len(lines)):
                code, _, cmt = lines[j].partition("//")
                om = re.search(r"\+0x([0-9a-f]+)", cmt)
                nm = re.search(r"(\w+)\s*(?:\[[^\]]*\]\s*)*[;,=]", code)
                if om and nm and (len(stack) == 1 or stack[-1] not in done):
                    done.add(stack[-1])
                    mem.append((int(om.group(1), 16), nm.group(1)))
                for ch in code:
                    if ch == "{":
                        stack.append(j)
                    elif ch == "}" and stack:
                        stack.pop()
                if not stack:
                    break
            if best is None or len(mem) > len(best):
                best = mem
    if not best:
        return None
    out, seen = [], set()
    for off, nm in best:
        if off not in seen and off < size:
            seen.add(off)
            out.append((off, nm))
    out.sort()
    if not out:
        return None
    if out[0][0] > 0:
        out.insert(0, (0, "(start)"))
    return [(o, (out[k + 1][0] if k + 1 < len(out) else size) - o, n) for k, (o, n) in enumerate(out)]


def crutch_counts(build_dir):
    """The crutch counts of one tools/relink/build.py output (its build.json step lines), None where a line is absent."""
    steps = json.load(open(os.path.join(build_dir, "build.json")))
    found = {k: re.search(pat, str(steps.get(step, ""))) for k, step, pat in CRUTCHES}
    counts = {k: (int(m.group(1)) if m else None) for k, m in found.items()}
    if counts["redirects"] is None and "pointers relocated" in str(steps.get("glue", "")):
        counts["redirects"] = 0               # the glue prints the code->trap key only when it is non-zero
    return counts


def MAP_OF(resolve_map):
    """The link map that sits beside a build's resolve_map.json."""
    return os.path.join(os.path.dirname(os.path.abspath(resolve_map)), "mb_standalone.map")


LATEST = os.path.join(ROOT, "work", "relink", "latest_build.json")


def _record(path, rep):
    try:
        hist = json.load(open(path))
    except (OSError, ValueError):
        hist = {}
    hist[rep["when"]] = rep
    with open(path, "w") as f:
        json.dump(dict(sorted(hist.items())), f, indent=1)
        f.write("\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    ap.add_argument("--no-extents", action="store_true", help="do not widen defined items to their link-map size")
    ap.add_argument("--json")
    ap.add_argument("--from-build", metavar="DIR")
    ap.add_argument("--record-data", action="store_true", help="with --from-build: also append to docs/data_history.json")
    ap.add_argument("--label", help="with --record: what landed in this build, shown on the chart's x axis (e.g. 'terrain lane')")
    ap.add_argument("--commit", help="commit to record with --from-build (default: the last one on main before build.json was written)")
    a = ap.parse_args()
    os.chdir(ROOT)
    if a.from_build:
        a.map = os.path.join(a.from_build, "resolve_map.json")
    import capstone
    import resolve as R
    from glue import RDATA, DATA, BSS, IAT_LO, IAT_HI, section_of
    img = R.Image()
    starts, game = game_functions()
    rmap = json.load(open(a.map))
    syms = link_map_symbols(MAP_OF(a.map)) if not a.no_extents and os.path.exists(MAP_OF(a.map)) else None
    items, uses, named = data_items(img, rmap, starts, game, syms=syms)
    live = drop_dead(items)
    dead = sum(e - s for s, e, _c in items) - sum(e - s for s, e, _c in live)
    items = live
    by = {c: [0, 0] for c in CLASSES}
    per_sec = {s[0]: {c: 0 for c in CLASSES} for s in (RDATA, DATA, BSS)}
    for s, e, c in items:
        by[c][0] += 1
        by[c][1] += e - s
        per_sec[section_of(s)[0]][c] += e - s
    total = sum(s[2] - s[1] for s in (RDATA, DATA, BSS)) - (IAT_HI - IAT_LO)
    covered = sum(b for _n, b in by.values())
    when = datetime.datetime.now().astimezone()
    commit = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip()
    if a.from_build:                 # a build output is dated by its build.json, not by this run
        mt = os.path.getmtime(os.path.join(a.from_build, "build.json"))
        when = datetime.datetime.fromtimestamp(mt).astimezone()
        commit = a.commit or subprocess.run(["git", "-C", ROOT, "log", "-1", "--format=%h", "--before=" + when.isoformat(), "main"],
                                            capture_output=True, text=True).stdout.strip()
    rep = {"when": when.isoformat(timespec="seconds"), "commit": commit, "label": a.label,
           "items": {c: by[c][0] for c in CLASSES}, "bytes": {c: by[c][1] for c in CLASSES},
           "section_bytes": {k.split("$")[0]: v for k, v in per_sec.items()},
           "unreferenced_bytes": total - covered - dead, "dead_bytes": dead, "total_bytes": total}
    print("static data: %d items, %.1f MB (.rdata %d KB, .data %d KB initialised, .bss %.1f MB)" % (
        len(items), total / 1e6, (RDATA[2] - RDATA[1]) // 1024, (DATA[2] - DATA[1]) // 1024, (BSS[2] - BSS[1]) / 1e6))
    for c in CLASSES:
        print("  %-22s %6d items  %10d bytes" % (c, by[c][0], by[c][1]))
    print("  %-22s %6s        %10d bytes" % ("not copied (dead)", "", dead))
    print("  %-22s %6s        %10d bytes" % ("before the first item", "", total - covered - dead))
    if a.json:
        json.dump(dict(rep, item_list=[["%08x" % s, e - s, c] for s, e, c in items]), open(a.json, "w"), indent=1)
    if a.record or a.record_data:
        if not a.from_build or a.record_data:
            _record(HIST, rep)
        if a.from_build and a.record:
            os.makedirs(os.path.dirname(LATEST), exist_ok=True)       # the history keeps only the basename; progress.py --treemap-data wants the dir
            json.dump({"build": os.path.abspath(a.from_build), "when": rep["when"], "commit": commit}, open(LATEST, "w"), indent=1)
            _record(SA_HIST, dict(crutch_counts(a.from_build), when=rep["when"], commit=commit,
                                  build=os.path.basename(a.from_build.rstrip("/")), label=a.label, items=rep["items"],
                                  bytes=rep["bytes"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
