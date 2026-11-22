"""restructure_split.py: batch B of tools/migrate/restructure.py, the three splits of docs/source_tree.md section 3.

agent_logic.cpp -> 11 files, script_interpreter.cpp -> 3, mission_5.cpp -> 3 (party_2.cpp stays whole, section 8).
The cut lines are the TSV's (docs/source_tree_moves.tsv); each part is

    banner / the prelude's #includes and #defines / helper definitions the part uses / the part's own lines

The helpers are the file's top-level static / inline / forceinline functions, types, enums, typedefs, macros and
externs (cxxchunk.py finds them) that the part's code mentions, taken from the prelude and from the sections before it
(declaration before use, as in the one-file original); a helper another helper needs comes along. Nothing that emits
code (a function, an explicit instantiation, a global) is copied: those stay in the part that owns their address.
A static that is a variable (shared state) is never duplicated silently: the tool says so.

The sidecars are partitioned by address (column 3 of a .manifest.txt row, column 1 of a .nonmatching.txt row), with the
rows outside the file's ranges placed by EXTRA below. Idempotent: a part that exists is not rewritten unless --force;
the old file and its sidecars are `git rm`ed only after every part exists.
"""
import json, os, re, sys

import cxxchunk as C

# defines that make a header emit code in THIS TU: only the part that owns the code gets them (name -> part stem);
# every other prelude #define goes to every part
DEFINE_OWNER = {
    "STD_VECTOR_INT_INSTANTIATED_HERE": "agent_attack_checks",       # vector<int>::resize 0x43f310, operator= 0x43f0a0
    "INT_HASH_LOWER_BOUND_INSTANTIATED_HERE": "agent_targeting",     # the glyph table lookup 0x43b210
}   # (the *_OUT_OF_LINE defines apply to the whole old object: every part gets them; the definitions behind them stay put)
# data / function definitions in the old file's prelude (they emit something): the part that takes them
PRELUDE_DEF_OWNER = {"src/game/script/script_interpreter.cpp": "operation_execute"}
# file statics of the old object that two parts use: the owner keeps the static (the original's cl saw a static there, which is
# what schedules Operation::execute), the others reach it through two accessors the owner defines after its last function
SHARED_STATICS = {"g_script_error": ("operation_execute", "int")}
ACCESSOR_TEXT = """
// ---- the file static g_script_error (0x5e259c) is shared with dialog_block_runner.cpp, which was one object with this file:
// its code reaches the variable through these two functions (no address of the static is taken here)
int script_error_get() { return g_script_error; }
void script_error_set(int v) { g_script_error = v; }
"""
ACCESSOR_DECL = """
// g_script_error (0x5e259c) is a file static of operation_execute.cpp (one object with this file in the original)
int script_error_get();
void script_error_set(int v);
"""
# sidecar rows whose address lies outside every part's range -> part stem
EXTRA = {
    "src/game/agent4/agent_logic.cpp": [((0x40c2c0, 0x40c670), "agent_attack_checks"),       # the std::logic_error family
                                         ((0x547d80, 0x5484b0), "agent_static_collision")],  # _$E1 / _$E2: the g_phys initialiser
    "src/game/script/script_interpreter.cpp": [((0x4904a0, 0x4904a0), "operation_execute"),       # trigger_item_mesh (line 2580)
                                                ((0x40e420, 0x40e420), "operation_execute")],     # vector4::operator+= COMDAT
}
BANNER = """//-----------------------------------------------------------------------------
// File: {name}  (0x{lo:x}..0x{hi:x})
//
// Desc: {desc}
//       One part of what was {old} (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------
"""


def directive_text(c):
    """a preprocessor chunk without the comment / blank lines in front of it"""
    ls = c.text.split("\n")
    cl = c.code.split("\n")
    i = 0
    while i < len(ls) - 1 and not cl[i].strip():
        i += 1
    return "\n".join(ls[i:])


def descriptions(R):
    """{new file name: what it holds} from the tables of docs/source_tree.md section 2"""
    out = {}
    for l in open(os.path.join(R.ROOT, "docs", "source_tree.md"), encoding="utf-8"):
        m = re.match(r"\|\s*`([\w.]+\.cpp)`\s*\|[^|]*\|[^|]*\|([^|]*)\|", l)
        if m:
            out[m.group(1)] = m.group(2).strip()
    return out


def part_rows(R, old_cpp):
    return [r for r in R.load_rows() if r["kind"] == "cpp-split" and r["old_path"] == old_cpp]


def prelude_end(r):
    m = re.match(r"(\d+)-(\d+)", r["prelude_lines"].strip())
    return int(m.group(2))


DESC = {}


def build_parts(R, old_cpp, force=False):
    rows = sorted(part_rows(R, old_cpp), key=lambda r: int(r["from_line"]))
    text = open(os.path.join(R.ROOT, old_cpp), "rb").read().decode("latin-1")
    lines = text.split("\n")
    P = prelude_end(rows[0])
    chs = C.chunks(text)
    for c in chs:
        if c.kind == "ns":
            inner = c.code[c.code.find("{") + 1:c.code.rfind("}")]
            ic = C.chunks(inner)
            kinds = {x.kind for x in ic}
            if kinds & {"def", "inst"}:
                c.kind = "ns_strong"
            else:
                c.kind = "ns_helper"
                nsname = re.match(r"\s*namespace\s+(\w+)", c.code)
                if nsname:
                    c.provides.add(nsname.group(1))
                for x in ic:
                    if not re.match(r"\s*using\b", x.code):
                        c.provides |= x.provides
    bounds = [(int(r["from_line"]), int(r["to_line"])) for r in rows]
    stems = [os.path.basename(r["new_path"])[:-4] for r in rows]

    def part_of(line):
        for i, (a, b) in enumerate(bounds):
            if a <= line <= b:
                return i
        return -1

    # which chunk lives in which region: -1 = prelude, i = part i
    region = {}
    for c in chs:
        mid = c.last
        region[id(c)] = -1 if mid <= P else part_of(mid)
    # provided names -> candidate chunks (copyable kinds); includes are always kept, defines by owner
    copyable = ("helper_fn", "type", "static_data", "extern", "ns_helper", "pp:define", "decl")
    by_name = {}
    for c in chs:
        if c.kind in copyable:
            for n in c.provides:
                if n and n != "operator":
                    by_name.setdefault(n, []).append(c)
    results, warns = {}, []
    use_count = {}
    for pi, r in enumerate(rows):
        a, b = bounds[pi]
        body = [c for c in chs if region[id(c)] == pi]
        own = {n for c in body if c.kind in copyable for n in c.provides}
        need_ids = set()
        why = {}
        def mentions(code, exclude_chunk=None):
            toks = C.idents(code)
            out = []
            for n in toks:
                for c in by_name.get(n, ()):
                    if c is exclude_chunk or id(c) in need_ids:
                        continue
                    ri = region[id(c)]
                    if ri == pi or ri > pi:
                        continue          # own section (already there) or later than this part (not visible in the original)
                    if n in own and c.kind != "pp:define":
                        continue
                    if n in SHARED_STATICS and SHARED_STATICS[n][0] != stems[pi]:
                        continue          # reached through the owner's accessors
                    if c.kind == "pp:define" and c.provides & {d for d, o in DEFINE_OWNER.items()}:
                        continue
                    out.append(c)
                    why.setdefault(id(c), (n, exclude_chunk.first if exclude_chunk is not None else -1))
            return out
        todo = [x for c in body for x in mentions(c.code, c)]
        # also the part's own helper chunks can pull earlier helpers; and the pieces the copies need
        while todo:
            c = todo.pop()
            if id(c) in need_ids:
                continue
            need_ids.add(id(c))
            todo.extend(mentions(c.code, c))
        # an #include in the middle of the old file applies to everything after it: the later parts start with it
        mid_inc = [c for c in chs if c.kind == "pp:include" and 0 <= region[id(c)] < pi]
        carried = [c for c in chs if id(c) in need_ids or c in mid_inc]
        carried.sort(key=lambda c: c.first)
        if os.environ.get("SPLIT_WHY") == stems[pi]:
            for c in carried:
                print("  carry %s [%d-%d] %s because %s in chunk@%s" % (c.kind, c.first, c.last, sorted(c.provides)[:3], *why.get(id(c), ("(include)", 0))))
        for c in carried:
            if c.kind == "pp:include":
                continue
            if c.kind == "static_data":
                use_count.setdefault(id(c), []).append(stems[pi])
        pre = [c for c in chs if region[id(c)] == -1 and c.kind.startswith("pp:") and c.kind in ("pp:include", "pp:define", "pp:pragma")]
        pre_txt = []
        first_code = next((c.first for c in chs if c.kind.startswith("pp:")), 1)
        for c in pre:
            if c.kind == "pp:define" and (c.provides & set(DEFINE_OWNER)):
                if all(DEFINE_OWNER.get(n, stems[pi]) == stems[pi] for n in c.provides):
                    pre_txt.append(directive_text(c).strip("\n"))
                continue
            if c.kind == "pp:define" and id(c) in need_ids:
                continue           # emitted with the carried helpers, below
            pre_txt.append(directive_text(c).strip("\n"))
        # defines in the prelude are all kept (cheap, and a header may read them); strip duplicate carried ones
        carried = [c for c in carried if not (c.kind == "pp:define" and region[id(c)] == -1)]
        out = [BANNER.format(name=os.path.basename(r["new_path"]), lo=int(r["addr_lo"], 16), hi=int(r["addr_hi"], 16),
                             desc=DESC.get(os.path.basename(r["new_path"]), "part of " + os.path.basename(old_cpp)),
                             old=old_cpp[len("src/"):])]
        out.append("\n".join(pre_txt))
        owner = PRELUDE_DEF_OWNER.get(old_cpp)
        pdefs = [c for c in chs if region[id(c)] == -1 and c.kind in ("def", "inst", "ns_strong")]
        if pdefs and owner is None:
            warns.append("PRELUDE DEFINITIONS with no owner part: %s" % [c.first for c in pdefs])
        if owner == stems[pi] and pdefs:
            out.append("\n// ---- definitions of the old file's prelude (the data this module owns)")
            for c in pdefs:
                out.append(c.text.strip("\n"))
        if carried:
            out.append("\n// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static\n"
                       "// or inline helper, a type or a macro: none of them emits code or data of its own)")
            for c in carried:
                out.append(c.text.strip("\n"))
        out.append("")
        out.append("\n".join(lines[a - 1:b]).rstrip("\n"))
        txt = "\n".join(out).rstrip("\n") + "\n"
        for var, (owner, ty) in SHARED_STATICS.items():
            if owner == stems[pi]:
                txt += ACCESSOR_TEXT
            elif re.search(r"\b%s\b" % var, txt):
                txt = re.sub(r"\b%s\s*=\s*([^;]+);" % var, r"script_error_set(\1);", txt)
                txt = re.sub(r"\b%s\b" % var, "script_error_get()", txt)
                i = txt.index("\n// ----") if "\n// ----" in txt else txt.index("\n\n", txt.rindex("#include"))
                txt = txt[:i] + "\n" + ACCESSOR_DECL + txt[i:]
        results[r["new_path"]] = txt
    for cid, ps in use_count.items():
        if len(ps) > 1:
            c = next(c for c in chs if id(c) == cid)
            warns.append("SHARED STATIC DATA %s copied into %s: %s" % (sorted(c.provides), ps, c.text.strip()[:80]))
    return rows, stems, results, warns


def split_sidecar(R, old_cpp, ext, rows, stems):
    old = old_cpp[:-4] + ext
    path = os.path.join(R.ROOT, old)
    if not os.path.exists(path):
        return {}
    col = 2 if ext == ".manifest.txt" else 0
    rng = [(int(r["addr_lo"], 16), int(r["addr_hi"], 16), r["new_path"]) for r in rows]
    extra = [(lo, hi, next(r["new_path"] for r in rows if os.path.basename(r["new_path"])[:-4] == st))
             for (lo, hi), st in EXTRA.get(old_cpp, ())]
    out = {r["new_path"][:-4] + ext: [] for r in rows}
    for line in open(path, "rb").read().decode("latin-1").split("\n"):
        if not line.strip():
            continue
        f = line.split()
        va = int(f[col], 16)
        dest = next((n for lo, hi, n in rng if lo <= va < hi), None) or next((n for lo, hi, n in extra if lo <= va <= hi), None)
        if dest is None:
            raise SystemExit("%s: row at 0x%x is in no part: %s" % (old, va, line[:70]))
        if ext == ".manifest.txt":
            line = line.replace("../" + old_cpp, "../" + dest, 1)
        out[dest[:-4] + ext].append(line)
    return out


def run(R, check=False, dump=None):
    DESC.update(descriptions(R))
    state = R.load_state()
    for old_cpp in R.SPLIT_FILES:
        if not os.path.exists(os.path.join(R.ROOT, old_cpp)):
            print("%s: already split" % old_cpp)
            continue
        rows, stems, parts, warns = build_parts(R, old_cpp)
        print("%s -> %d parts" % (old_cpp, len(rows)))
        for w in warns:
            print("  WARNING " + w)
        if dump:
            os.makedirs(dump, exist_ok=True)
            for np, txt in parts.items():
                open(os.path.join(dump, os.path.basename(np)), "wb").write(txt.encode("latin-1"))
            continue
        if check:
            continue
        for np, txt in parts.items():
            p = os.path.join(R.ROOT, np)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            open(p, "wb").write(txt.encode("latin-1"))
        for ext in (".manifest.txt", ".nonmatching.txt", ".partial.txt"):
            sc = split_sidecar(R, old_cpp, ext, rows, stems)
            for np, ls in sc.items():
                with open(os.path.join(R.ROOT, np), "wb") as f:
                    f.write(("\n".join(ls) + ("\n" if ls else "")).encode("latin-1"))
            old = old_cpp[:-4] + ext
            if os.path.exists(os.path.join(R.ROOT, old)):
                R.sh("git", "rm", "-q", "-f", old)
        R.sh("git", "rm", "-q", "-f", old_cpp)
        for np in parts:
            R.sh("git", "add", "-f", np)
            for ext in (".manifest.txt", ".nonmatching.txt"):
                if os.path.exists(os.path.join(R.ROOT, np[:-4] + ext)):
                    R.sh("git", "add", "-f", np[:-4] + ext)
        fan_out(R, old_cpp, rows, stems)
    R.prune_empty_dirs()


def fan_out(R, old_cpp, rows, stems):
    """References to the old file elsewhere: tu_order.txt, the link order, row keys, equiv verdicts."""
    ostem = os.path.basename(old_cpp)[:-4]
    new_paths = [r["new_path"] for r in sorted(rows, key=lambda r: int(r["addr_lo"], 16))]
    new_stems = [os.path.basename(p)[:-4] for p in new_paths]
    def stem_for_va(va):
        for r in rows:
            if int(r["addr_lo"], 16) <= va < int(r["addr_hi"], 16):
                return os.path.basename(r["new_path"])[:-4], r["new_path"]
        for (lo, hi), st in EXTRA.get(old_cpp, ()):
            if lo <= va <= hi:
                return st, next(r["new_path"] for r in rows if os.path.basename(r["new_path"])[:-4] == st)
        return None, None
    ap = R.ANNOT
    if os.path.exists(ap):                    # the parts read the annotations of the old file's directory
        ann = json.load(open(ap))
        if old_cpp in ann:
            for np in new_paths:
                ann[np] = ann[old_cpp]
            del ann[old_cpp]
            json.dump(ann, open(ap, "w"), indent=0, sort_keys=True)
    fan_out_more(R, old_cpp, rows, stem_for_va, new_paths)
    # tu_order.txt: the old line becomes the parts' lines, address order
    p = os.path.join(R.ROOT, "port", "link", "tu_order.txt")
    ls = open(p).read().split("\n")
    if old_cpp in ls:
        i = ls.index(old_cpp)
        ls[i:i + 1] = new_paths
        open(p, "w").write("\n".join(ls))
    # symbols/link_inputs.json link_order
    p = os.path.join(R.ROOT, "symbols", "link_inputs.json")
    t = open(p, encoding="utf-8").read()
    pat = re.compile(r'^(\s*)"%s",?\n' % re.escape(ostem), re.M)
    m = pat.search(t)
    if m:
        ind = m.group(1)
        t = t[:m.start()] + "".join('%s"%s",\n' % (ind, s) for s in new_stems) + t[m.end():]
        open(p, "w", encoding="utf-8").write(t)
    # row keys "stem va" (tools/difftest/row_opts.json, test_equiv.py ...)
    for rel in ("tools/difftest/row_opts.json",):
        p = os.path.join(R.ROOT, rel)
        t = open(p, encoding="utf-8").read()
        def rep(m):
            st, _ = stem_for_va(int(m.group(2), 16))
            return '"%s %s' % (st or ostem, m.group(2))
        t2 = re.sub(r'"(%s) ([0-9a-fA-F]{6,8})' % re.escape(ostem), rep, t)
        if t2 != t:
            open(p, "w", encoding="utf-8").write(t2)
    # docs/equiv_verdicts.json: module + file per row
    p = os.path.join(R.ROOT, "docs", "equiv_verdicts.json")
    d = json.load(open(p, encoding="utf-8"))
    raw = open(p, encoding="utf-8").read()
    changed = False
    for va, row in d.get("rows", {}).items():
        if row.get("module") == ostem:
            st, np = stem_for_va(int(va, 16))
            if st:
                row["module"] = st
                if row.get("file", "").startswith(old_cpp[:-4]):
                    row["file"] = np[:-4] + row["file"][len(old_cpp[:-4]):]
                changed = True
    if changed:
        json.dump(d, open(p, "w", encoding="utf-8"), indent=0 if raw.startswith("{\n\"") else 1, ensure_ascii=False)


def fan_out_more(R, old_cpp, rows, stem_for_va, new_paths):
    ostem = os.path.basename(old_cpp)[:-4]
    root = R.ROOT
    # port/link tables: weaken / emit_inline blocks are repeated for every part (a weakened symbol a part does not define
    # is harmless); owners.tsv and tables.json name the first part (link_tables.py regenerates all of them at the fixpoint)
    for dp, _dn, fns in os.walk(os.path.join(root, "port", "link")):
        for fn in fns:
            p = os.path.join(dp, fn)
            t = open(p, "rb").read().decode("latin-1")
            if old_cpp not in t:
                continue
            if fn in ("weaken.cmake", "emit_inline.cmake"):
                out, ls, i = [], t.split("\n"), 0
                while i < len(ls):
                    l = ls[i]
                    m = re.match(r"(mb_weaken|mb_emit_inline)\(" + re.escape(old_cpp) + r"(\)?)(.*)$", l)
                    if not m:
                        out.append(l); i += 1; continue
                    block = [l]
                    if fn == "weaken.cmake":
                        while not block[-1].rstrip().endswith(")"):
                            i += 1; block.append(ls[i])
                    else:
                        while i + 1 < len(ls) and ls[i + 1].startswith("#"):
                            i += 1; block.append(ls[i])
                    for np in new_paths:
                        out.extend(b.replace(old_cpp, np) for b in block)
                    i += 1
                t = "\n".join(out)
            else:
                t = t.replace(old_cpp, new_paths[0])
            open(p, "wb").write(t.encode("latin-1"))
    # symbols/slices/<stem>.txt: address -> name hints, partitioned by address
    sp = os.path.join(root, "symbols", "slices", ostem + ".txt")
    if os.path.exists(sp):
        buckets = {}
        for l in open(sp, "rb").read().decode("latin-1").split("\n"):
            if not l.strip():
                continue
            m = re.match(r"([0-9a-fA-F]{8})\b", l)
            st, _ = stem_for_va(int(m.group(1), 16)) if m else (None, None)
            buckets.setdefault(st or os.path.basename(new_paths[0])[:-4], []).append(l)
        for st, ls in buckets.items():
            with open(os.path.join(root, "symbols", "slices", st + ".txt"), "wb") as f:
                f.write(("\n".join(ls) + "\n").encode("latin-1"))
            R.sh("git", "add", "symbols/slices/%s.txt" % st)
        R.sh("git", "rm", "-q", "-f", "symbols/slices/%s.txt" % ostem)
    # a line that names the old file and an address of it: the part that holds the address
    forms = [old_cpp, old_cpp[len("src/"):], "/".join(old_cpp.split("/")[-2:])]
    rx = re.compile(r"(?<![\w/.\-])(" + "|".join(re.escape(f) for f in forms) + r")(?![\w\-])")
    row_rx = re.compile(r"([\"'])" + re.escape(ostem) + r"\1(\s*,\s*)(0x[0-9a-fA-F]+|[\"'][0-9a-fA-F]{6,8}[\"'])")
    for rel in R.tracked():
        if rel in R.EXCLUDE_TEXT or rel.startswith(("work/", "src/")) or rel.startswith("tools/migrate/restructure"):
            continue
        p = os.path.join(root, rel)
        if os.path.islink(p) or not os.path.isfile(p):
            continue
        b = open(p, "rb").read()
        if not R.is_text(b):
            continue
        t = b.decode("latin-1")
        if ostem not in t:
            continue
        out, ch = [], False
        for l in t.split("\n"):
            if ostem in l:
                addr = re.search(r"\b(?:0x)?([0-9a-fA-F]{6,8})\b", l[l.find(ostem):]) or re.search(r"\b0x([0-9a-fA-F]{6,8})\b", l)
                m = row_rx.search(l)
                va = int(re.sub(r"[^0-9a-fA-Fx]", "", m.group(3)), 16) if m else (int(addr.group(1), 16) if addr else None)
                st, np = stem_for_va(va) if va and 0x400000 <= va < 0x600000 else (None, None)
                if st:
                    l2 = row_rx.sub(lambda mm: mm.group(1) + st + mm.group(1) + mm.group(2) + mm.group(3), l)
                    l2 = rx.sub(lambda mm: np if mm.group(1) == old_cpp else np[len("src/"):] if mm.group(1) == old_cpp[len("src/"):] else "/".join(np.split("/")[-2:]), l2)
                    if l2 != l:
                        ch = True; l = l2
            out.append(l)
        if ch:
            open(p, "wb").write("\n".join(out).encode("latin-1"))
