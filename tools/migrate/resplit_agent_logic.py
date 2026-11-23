#!/usr/bin/env python3
"""resplit_agent_logic.py: cut src/game/agent/agent_logic.cpp (the one object, 0x43a280..0x460160) into the 11 parts of
docs/source_tree.md section 3.1 again, this time with the object's shared context in one header.

Why batch B's split lost nine rows (docs/source_tree.md section 10): cl takes facts about a callee from its visible body, and the
old tool copied into each part only the helper definitions defined EARLIER in the file than the part (the original's order). The
visible bodies the rows needed can sit anywhere in the object (vector4::normalize 2,600 lines after pick_front). So here a helper
definition (a static / inline function, a type, a macro, a declaration, a visible body of a callee another object defines) that a
part other than its own mentions, directly or through another helper, goes to src/game/agent/agent_logic_shared.h whatever its
position; every part includes it after the prelude. Every `inline __declspec(noinline)` body (the "visible body" blocks) goes there
too: they are context of the whole object, not of one part. Nothing that emits code of its own (a function, an explicit
instantiation, a global) is moved: those stay in the part that owns their address.

RESULT (2026-11-22): the 11 parts compile and pick_front, pick_duel and fire_check stay byte-exact with the shared header, but
pick_threat, pick_reachable_goal, draw_glyph, refresh_carried, gui_manager::add and update_attack do not (docs/c2scope_evaluation.md,
"re-split attempt"): they need the bodies of real functions of other parts. The tree keeps the single file; this tool documents the test.

usage: resplit_agent_logic.py [--dump DIR]    --dump writes the parts and the header to DIR and changes nothing else
Without --dump it rewrites the tree: the parts, their sidecars (match/manifests), symbols/slices, link_inputs.json, tu_order.txt,
row_opts / equiv_verdicts / equiv_proofs keys, and drops agent_logic.* (git rm).
"""
import collections, csv, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import cxxchunk as C       # noqa: E402
import sidecars as SC      # noqa: E402

OLD = "src/game/agent/agent_logic.cpp"
OLD_TSV = "src/game/agent4/agent_logic.cpp"
SHARED = "src/game/agent/agent_logic_shared.h"
DEFINE_OWNER = {                       # a define that makes a header emit code: only the part that owns the code gets it
    "STD_VECTOR_INT_INSTANTIATED_HERE": "agent_attack_checks",
    "INT_HASH_LOWER_BOUND_INSTANTIATED_HERE": "agent_targeting",
}
EXTRA = [((0x40c2c0, 0x40c670), "agent_attack_checks"),       # the std::logic_error family
         ((0x547d80, 0x5484b0), "agent_static_collision")]    # _$E1 / _$E2: the g_phys initialiser
COPYABLE = ("helper_fn", "type", "static_data", "extern", "ns_helper", "pp:define", "decl")
BANNER = """//-----------------------------------------------------------------------------
// File: {name}  (0x{lo:x}..0x{hi:x})
//
// Desc: {desc}
//       One part of the agent logic object (the functions in address order; the cut points and the evidence for them are in
//       docs/source_tree.md sections 3.1 and 10). What the parts share is in game/agent/agent_logic_shared.h.
//-----------------------------------------------------------------------------
"""
HEADER_BANNER = """//-----------------------------------------------------------------------------
// File: agent_logic_shared.h
//
// Desc: what the parts of the agent logic object (agent_targeting.cpp .. agent_frame_update.cpp) share: the bodies of callees
//       another object defines that cl saw while compiling this object (declared `inline __declspec(noinline)` so the calls stay
//       calls; rows that call them keep their bytes only with the body visible, wherever in the object it was defined),
//       and the static / inline helpers, types and macros that more than one part uses. Include it after the includes of
//       the part's prelude; it emits no code of its own (a static helper is emitted by the parts that call it).
//-----------------------------------------------------------------------------
#pragma once
"""


def load_rows():
    with open(os.path.join(ROOT, "docs", "source_tree_moves.tsv"), newline="", encoding="utf-8") as f:
        rows = [r for r in csv.DictReader(f, delimiter="\t") if r["kind"] == "cpp-split" and r["old_path"] == OLD_TSV]
    return sorted(rows, key=lambda r: int(r["from_line"]))


def descriptions():
    out = {}
    for l in open(os.path.join(ROOT, "docs", "source_tree.md"), encoding="utf-8"):
        m = re.match(r"\|\s*`([\w.]+\.cpp)`\s*\|[^|]*\|[^|]*\|([^|]*)\|", l)
        if m:
            out[m.group(1)] = m.group(2).strip()
    return out


def directive_text(c):
    ls, cl = c.text.split("\n"), c.code.split("\n")
    i = 0
    while i < len(ls) - 1 and not cl[i].strip():
        i += 1
    return "\n".join(ls[i:])


def is_visible_body(c):
    """an `inline __declspec(noinline)` definition: a callee another object defines, whose body this object saw"""
    if c.kind != "helper_fn":
        return False
    head = re.split(r"[{;]", c.code, maxsplit=1)[0]
    return bool(re.search(r"\binline\b", head)) and "noinline" in head and not re.search(r"\bstatic\b", head)


def build():
    text = open(os.path.join(ROOT, OLD), "rb").read().decode("latin-1")
    lines = text.split("\n")
    rows = load_rows()
    desc = descriptions()
    P = 110
    chs = C.chunks(text)
    for c in chs:
        if c.kind == "ns":
            inner = c.code[c.code.find("{") + 1:c.code.rfind("}")]
            ic = C.chunks(inner)
            if {x.kind for x in ic} & {"def", "inst"}:
                c.kind = "ns_strong"
            else:
                c.kind = "ns_helper"
                m = re.match(r"\s*namespace\s+(\w+)", c.code)
                if m:
                    c.provides.add(m.group(1))
                for x in ic:
                    if not re.match(r"\s*using\b", x.code):
                        c.provides |= x.provides
    bounds = [(int(r["from_line"]), int(r["to_line"])) for r in rows]
    stems = [os.path.basename(r["new_path"])[:-4] for r in rows]

    def region_of(line):
        for i, (a, b) in enumerate(bounds):
            if a <= line <= b:
                return i
        return -1
    region = {id(c): (-1 if c.last <= P else region_of(c.last)) for c in chs}
    by_name = {}
    macro_texts = {}
    for c in chs:
        if c.kind == "pp:define":
            for n in c.provides:
                macro_texts.setdefault(n, set()).add(re.sub(r"\s+", " ", c.code).strip())
    for c in chs:
        if c.kind in COPYABLE:
            for n in c.provides:
                if n and n != "operator":
                    if c.kind == "pp:define" and len(macro_texts[n]) > 1:
                        continue          # a macro defined differently in different places (with #undef between): position-bound, stays put
                    by_name.setdefault(n, []).append(c)

    def mentions(code, self_chunk):
        out = []
        for n in C.idents(code):
            for c in by_name.get(n, ()):
                if c is not self_chunk and not (c.kind == "pp:define" and (c.provides & set(DEFINE_OWNER))):
                    out.append(c)
        return out
    chunk_by_id = {id(c): c for c in chs}
    # a name defined by more than one free helper (overloads, or the same name with another meaning later in the file) is
    # position-bound: the earlier definition is what the code before the next one saw, so it is never shared, only copied
    # into the later parts that mention it (the old rule); every other helper is shared
    free = collections.defaultdict(set)
    for c in chs:
        if c.kind in ("helper_fn", "decl", "type", "static_data", "extern") and not is_visible_body(c):
            for n in c.provides:
                free[n].add(id(c))
    ambiguous = {n for n, ids in free.items() if len(ids) > 1}
    movable = lambda c: c.kind in COPYABLE and not (c.provides & ambiguous and not is_visible_body(c))
    shared = set()                       # chunk ids that go to the header
    for c in chs:
        if is_visible_body(c) or (c.kind == "pp:include" and region[id(c)] >= 0):
            shared.add(id(c))
    users = {}                           # chunk id -> regions of the chunks that mention it
    for u in chs:
        if u.kind in ("pp:include", "pp:undef"):
            continue
        for d in mentions(u.code, u):
            if movable(d) and d.kind != "pp:include":
                users.setdefault(id(d), set()).add(region[id(u)])
    for c in chs:
        if movable(c) and c.kind in COPYABLE:
            u = users.get(id(c), set())
            if region[id(c)] < 0:
                if u:
                    shared.add(id(c))            # a prelude helper somebody uses
            elif u - {region[id(c)]}:
                shared.add(id(c))                # used outside its own part
    todo = [chunk_by_id[i] for i in shared]
    while todo:                          # a helper in the header pulls what it mentions
        c = todo.pop()
        for d in mentions(c.code, c):
            if d.kind in COPYABLE and id(d) not in shared and movable(d):
                shared.add(id(d)); todo.append(d)
    hdr = [c for c in chs if id(c) in shared and not (c.kind == "pp:define" and region[id(c)] == -1)]
    hdr.sort(key=lambda c: c.first)
    seen_def, uniq = set(), []
    for c in hdr:
        if c.kind == "pp:define":
            k = re.sub(r"\s+", " ", c.code).strip()
            if k in seen_def:
                continue
            seen_def.add(k)
        uniq.append(c)
    hdr = uniq
    moved = {id(c) for c in hdr}
    warns = []
    for c in hdr:
        if c.kind == "static_data":
            warns.append("static data in the header (one copy per part): %s" % c.text.strip()[:80])
    for n in sorted(ambiguous):
        warns.append("position-bound name (copied into later parts, not shared): %s" % n)

    pre = [c for c in chs if region[id(c)] == -1 and c.kind in ("pp:include", "pp:define", "pp:pragma")]
    parts = {}
    for pi, r in enumerate(rows):
        a, b = bounds[pi]
        name = os.path.basename(r["new_path"])
        # copies of the position-bound helpers of EARLIER regions this part mentions (closure)
        own = {n for c in chs if region[id(c)] == pi and c.kind in COPYABLE for n in c.provides}
        need = {}
        todo = [c for c in chs if region[id(c)] == pi]
        while todo:
            u = todo.pop()
            for d in mentions(u.code, u):
                if id(d) in moved or id(d) in need or d.kind not in COPYABLE or d.kind == "pp:include":
                    continue
                if not (d.provides & ambiguous):
                    continue
                if region[id(d)] >= pi or (d.provides & own):
                    continue
                need[id(d)] = d; todo.append(d)
        copies = sorted(need.values(), key=lambda c: c.first)
        pre_txt = []
        for c in pre:
            if c.kind == "pp:define" and (c.provides & set(DEFINE_OWNER)):
                if all(DEFINE_OWNER.get(n, stems[pi]) == stems[pi] for n in c.provides):
                    pre_txt.append(directive_text(c).strip("\n"))
                continue
            pre_txt.append(directive_text(c).strip("\n"))
        out = [BANNER.format(name=name, lo=int(r["addr_lo"], 16), hi=int(r["addr_hi"], 16),
                             desc=desc.get(name, "part of agent_logic"))]
        out.append("\n".join(pre_txt))
        out.append('\n#include "game/agent/agent_logic_shared.h"\n')
        if copies:
            out.append("// ---- helpers of earlier sections whose name the object defines more than once: the definition this part saw there")
            out.extend(c.text.strip("\n") for c in copies)
            out.append("")
        skip = set()
        for c in chs:
            if id(c) in moved and region[id(c)] == pi:
                skip.update(range(max(c.first, a), min(c.last, b) + 1))
        body = [lines[i - 1] for i in range(a, b + 1) if i not in skip]
        out.append("\n".join(body).rstrip("\n"))
        parts[r["new_path"]] = "\n".join(out).rstrip("\n") + "\n"
    header = HEADER_BANNER + "\n" + "\n".join(c.text.strip("\n") + "\n" for c in hdr)
    return rows, stems, parts, header, warns


def va_part(rows, va):
    for r in rows:
        if int(r["addr_lo"], 16) <= va < int(r["addr_hi"], 16):
            return r["new_path"]
    for (lo, hi), st in EXTRA:
        if lo <= va <= hi:
            return next(r["new_path"] for r in rows if os.path.basename(r["new_path"])[:-4] == st)
    return None


def sh(*a):
    return subprocess.run(a, cwd=ROOT, capture_output=True, text=True, check=True).stdout


def rewrite_tree(rows, stems, parts, header):
    new_paths = [r["new_path"] for r in rows]
    # sources
    for np, txt in parts.items():
        p = os.path.join(ROOT, np)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        open(p, "wb").write(txt.encode("latin-1"))
    open(os.path.join(ROOT, SHARED), "wb").write(header.encode("latin-1"))
    # sidecars, partitioned by address
    for kind in SC.KINDS:
        old = SC.sidecar_for(OLD, kind)
        if not os.path.exists(old):
            continue
        col = 2 if kind == "manifest" else 0
        buckets = {np: [] for np in new_paths}
        for line in open(old, "rb").read().decode("latin-1").split("\n"):
            if not line.strip():
                continue
            f = line.split()
            try:
                va = int(f[col], 16)
            except (ValueError, IndexError):
                buckets[new_paths[0]].append(line); continue
            dest = va_part(rows, va)
            if dest is None:
                raise SystemExit("row at 0x%x is in no part: %s" % (va, line[:70]))
            if kind == "manifest":
                line = line.replace("../" + OLD, "../" + dest, 1)
            buckets[dest].append(line)
        for np, ls in buckets.items():
            if not ls:
                continue
            sp = SC.sidecar_for(np, kind)
            os.makedirs(os.path.dirname(sp), exist_ok=True)
            open(sp, "wb").write(("\n".join(ls) + "\n").encode("latin-1"))
            sh("git", "add", "-f", os.path.relpath(sp, ROOT))
        sh("git", "rm", "-q", "-f", os.path.relpath(old, ROOT))
    sh("git", "rm", "-q", "-f", OLD)
    for np in new_paths + [SHARED]:
        sh("git", "add", "-f", np)
    # symbols/slices hints, by address (the lines that name no address stay with the first part)
    sp = os.path.join(ROOT, "symbols", "slices", "agent_logic.txt")
    if os.path.exists(sp):
        buckets = {}
        for l in open(sp, "rb").read().decode("latin-1").split("\n"):
            if not l.strip():
                continue
            m = re.match(r"([0-9a-fA-F]{8})\b", l)
            dest = va_part(rows, int(m.group(1), 16)) if m else None
            buckets.setdefault(os.path.basename(dest or new_paths[0])[:-4], []).append(l)
        for st, ls in buckets.items():
            fp = os.path.join(ROOT, "symbols", "slices", st + ".txt")
            open(fp, "wb").write(("\n".join(ls) + "\n").encode("latin-1"))
            sh("git", "add", "-f", os.path.relpath(fp, ROOT))
        sh("git", "rm", "-q", "-f", "symbols/slices/agent_logic.txt")
    # link_order, tu_order
    p = os.path.join(ROOT, "symbols", "link_inputs.json")
    t = open(p, encoding="utf-8").read()
    t = re.sub(r'^(\s*)"agent_logic",\n', lambda m: "".join('%s"%s",\n' % (m.group(1), s) for s in stems), t, count=1, flags=re.M)
    open(p, "w", encoding="utf-8").write(t)
    p = os.path.join(ROOT, "port", "link", "tu_order.txt")
    ls = open(p).read().split("\n")
    i = ls.index(OLD)
    ls[i:i + 1] = new_paths
    open(p, "w").write("\n".join(ls))
    # row keys "stem va"
    p = os.path.join(ROOT, "tools", "difftest", "row_opts.json")
    t = open(p, encoding="utf-8").read()
    def rep(m):
        np = va_part(rows, int(m.group(1), 16))
        return '"%s %s' % (os.path.basename(np)[:-4] if np else "agent_logic", m.group(1))
    open(p, "w", encoding="utf-8").write(re.sub(r'"agent_logic ([0-9a-fA-F]{6,8})', rep, t))
    # verdicts: module + file
    p = os.path.join(ROOT, "docs", "equiv_verdicts.json")
    raw = open(p, encoding="utf-8").read()
    d = json.loads(raw)
    for va, row in d.get("rows", {}).items():
        if row.get("module") == "agent_logic":
            np = va_part(rows, int(va, 16))
            if np:
                row["module"] = os.path.basename(np)[:-4]
                if row.get("file", "").endswith(".nonmatching.txt"):
                    row["file"] = os.path.relpath(SC.nonmatching_for(np), ROOT)
    json.dump(d, open(p, "w", encoding="utf-8"), indent=0 if raw.startswith("{\n\"") else 1, ensure_ascii=False)
    # equiv proofs name the source of the row by address
    pd = os.path.join(ROOT, "docs", "equiv_proofs")
    for fn in os.listdir(pd):
        fp = os.path.join(pd, fn)
        t = open(fp, encoding="utf-8").read()
        if OLD in t:
            np = va_part(rows, int(fn.split(".")[0], 16))
            if np:
                open(fp, "w", encoding="utf-8").write(t.replace(OLD, np))
    # port/link: drop the old file's blocks (link_tables.py regenerates them)
    for dp, _d, fns in os.walk(os.path.join(ROOT, "port", "link")):
        for fn in fns:
            fp = os.path.join(dp, fn)
            t = open(fp, "rb").read().decode("latin-1")
            if OLD not in t:
                continue
            if fn == "weaken.cmake":
                t = re.sub(r"mb_weaken\(%s\n(?:[ \t]+\S+\n?)*" % re.escape(OLD), "", t)
            elif fn == "emit_inline.cmake":
                t = re.sub(r"mb_emit_inline\(%s\)\n(?:#[^\n]*\n)*" % re.escape(OLD), "", t)
            else:
                t = t.replace(OLD, new_paths[0])
            open(fp, "wb").write(t.encode("latin-1"))


def main():
    rows, stems, parts, header, warns = build()
    for w in warns:
        print("WARNING", w)
    if "--dump" in sys.argv:
        d = sys.argv[sys.argv.index("--dump") + 1]
        os.makedirs(d, exist_ok=True)
        for np, txt in parts.items():
            open(os.path.join(d, os.path.basename(np)), "wb").write(txt.encode("latin-1"))
        open(os.path.join(d, os.path.basename(SHARED)), "wb").write(header.encode("latin-1"))
        print("dumped %d parts + header (%d lines) to %s" % (len(parts), header.count("\n"), d))
        return
    rewrite_tree(rows, stems, parts, header)
    print("wrote %d parts + %s" % (len(parts), SHARED))


if __name__ == "__main__":
    main()
