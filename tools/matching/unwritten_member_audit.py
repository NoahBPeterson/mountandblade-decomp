#!/usr/bin/env python3
"""unwritten_member_audit.py: find struct members the reconstructed source reads but never writes.

A member declaration carrying a `// +0xNN` comment is a layout record copied from the original.  If the
original's initialiser was not reconstructed, the member is read with whatever the allocation left behind
(CharSheet::label[0x2a] held 2; CharSheetWidget::skill_index held 0xd7e9c).  This audit pairs every such
declaration with the tree-wide write sites and reports the members that have reads and no writes.

Attribution:
  * `this->field` resolves through the enclosing member function (out-of-line `Owner::fn` or an inline body).
  * `X->field` / `X.field` resolve X through a `(Owner*)expr` cast in the receiver, a declaration in the
    same file, then through the tree's `extern T* name` globals.  A write whose receiver resolves is
    attributed to that owner only, so OrderEntry's `e->label =` cannot hide CharSheet::label, and
    note_window's `this->links =` cannot hide Agent::links.
  * Bare `field =` / `field[i] =` count only inside a member function of the owner whose field it is
    (MapPanel::update's local tstring label is not a write to CharSheet::label); bare `field[...]` reads
    count the same way (cs_troop.cpp reads `label[i]` / `btn[i]` with no receiver to resolve).
  * Writes through the documented layout (`*(int*)((char*)this + 0xNN) =`) count for the enclosing owner.
  * A write whose receiver cannot be resolved falls back to the bare field name (conservative: it can only
    hide a row, never invent one).  An unresolved *read* is counted only when the field name belongs to a
    single owner, so it can never spread one struct's read over another's layout.

Excluded: extern globals, `const` members, members initialised at their declaration, padding arrays,
anonymous aggregate members whose offsets the comment does not pin down, and anything listed in
unwritten_member_allowlist.txt (one `Struct::field` per line).  Members of a struct that never appears as
a type in any .cpp (only opened through char*/void* views) are reported in a separate unverifiable bucket.

Usage: python3 tools/matching/unwritten_member_audit.py [--json out.json] [--filter SUBSTR]
Exit status 1 when (filtered) findings remain, else 0.
"""
import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

TOOL = Path(__file__).resolve()
ROOT = TOOL.parents[2]
SRC = ROOT / "src"
ALLOWLIST = TOOL.with_name("unwritten_member_allowlist.txt")
SKIP_DIRS = {"tinyxml"}

KEYWORDS = {
    "return", "if", "else", "while", "for", "switch", "case", "default", "do", "sizeof", "new", "delete",
    "throw", "const", "static", "mutable", "unsigned", "signed", "struct", "class", "union", "enum", "void",
    "int", "char", "float", "double", "bool", "long", "short", "auto", "register", "volatile", "extern",
    "inline", "typedef", "using", "namespace", "template", "typename", "operator", "virtual", "explicit",
    "friend", "public", "private", "protected", "this", "true", "false", "nullptr",
}
ASSIGN_OPS = ("<<=", ">>=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "++", "--", "=")
ACCESS = re.compile(r"(->|\.)\s*([A-Za-z_]\w*)")
WORD = re.compile(r"[A-Za-z_]\w*")
STRUCT_HEAD = re.compile(r"\b(struct|class|union)\b\s*([A-Za-z_]\w*)?\s*(?::[^;{}]*)?$")
FN_HEAD = re.compile(r"([A-Za-z_]\w*)\s*::\s*~?[A-Za-z_]\w*\s*\(")
OFFSET_WRITE = re.compile(
    r"\*\s*\(\s*[A-Za-z_][\w\s:<>]*\*?\s*\)\s*\([^()]*?\b0x([0-9a-fA-F]+)\b[^()]*\)\s*"
    r"(?:[-+*/%&|^]?=|\+\+|--)"
)
MEM_WRITE = re.compile(
    r"\b(?:memcpy|memset|memmove|CopyMemory|strcpy|strncpy|strcat)\s*\(\s*&?\s*"
    r"([^,;()]*?(?:->|\.)\s*[A-Za-z_]\w*)"
)


# ---- lexing -------------------------------------------------------------------------------------------
def strip_lines(text):
    """Code lines with comments and string/char literal bodies blanked (line numbers preserved)."""
    out = []
    in_block = False
    for line in text.splitlines():
        buf = []
        i, n = 0, len(line)
        while i < n:
            c = line[i]
            if in_block:
                j = line.find("*/", i)
                if j < 0:
                    i = n
                else:
                    in_block = False
                    i = j + 2
                continue
            if c == "/" and i + 1 < n and line[i + 1] == "/":
                break
            if c == "/" and i + 1 < n and line[i + 1] == "*":
                in_block = True
                i += 2
                continue
            if c in "\"'":
                quote = c
                i += 1
                while i < n:
                    if line[i] == "\\":
                        i += 2
                        continue
                    if line[i] == quote:
                        i += 1
                        break
                    i += 1
                buf.append(" ")
                continue
            buf.append(c)
            i += 1
        out.append("".join(buf))
    return out


def split_top(text, seps):
    parts, cur, depth = [], [], 0
    for ch in text:
        if ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth = max(0, depth - 1)
        if ch in seps and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return [p for p in parts if p.strip()]


# ---- declarations -------------------------------------------------------------------------------------
def _seg_names(seg):
    seg = seg.strip()
    if not seg or "(" in seg:
        return []
    parts = split_top(seg, ",")
    names = []
    m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$", parts[0])
    if m:
        names.append(m.group(1))
    for p in parts[1:]:
        m = re.match(r"\s*([A-Za-z_]\w*)", p)
        if m:
            names.append(m.group(1))
    return names


def decl_names(code):
    """(names, shared_offset) for one comment-offset declaration line."""
    body = code.strip()
    if body.endswith(";"):
        body = body[:-1].strip()
    if not body or "(" in body:
        return [], False
    agg = re.match(r"^(struct|union|class)\b\s*(?:[A-Za-z_]\w*\s*)?(?::[^{]*)?\{", body)
    if agg:
        open_at = body.find("{")
        close_at = body.rfind("}")
        if close_at < open_at:
            return [], False
        tail = body[close_at + 1:].strip()
        if tail:
            # `struct Pair { ... } direct[4]` -> the declarator after the body
            return _seg_names(tail)[:1], False
        inner = body[open_at + 1:close_at]
        names = []
        for seg in split_top(inner, ";"):
            names.extend(_seg_names(seg))
        # an anonymous union's members share the offset; an anonymous struct's later members do not.
        return names, agg.group(1) == "union"
    return _seg_names(body), False


def block_kind(head):
    """Classify the construct a `{` opens from the text since the previous ;{}."""
    if ")" in head:
        return None, None
    m = STRUCT_HEAD.search(head)
    if m:
        return m.group(1), m.group(2)
    return None, None


def fn_owner(head):
    """Owner of an out-of-line member definition whose body starts here."""
    if not re.search(r"\)\s*(?::[^{]*)?$", head):
        return None
    m = FN_HEAD.search(head)
    return m.group(1) if m else None


def parse_file(rel, raw, code, members, externs, spans):
    stack = []          # {kind, name, fn_owner}
    head = ""
    ctx = [None] * (len(code) + 2)
    decl_lines = set()
    lines = code
    for ln in range(1, len(lines) + 1):
        line = lines[ln - 1]
        owner = None
        for blk in reversed(stack):
            if blk["kind"] in ("struct", "class", "union") and blk["name"]:
                owner = blk["name"]
                break
            if blk["fn_owner"]:
                owner = blk["fn_owner"]
                break
        ctx[ln] = owner
        # member declaration?
        if owner:
            code_part = line.strip()
            is_decl_ctx = True
            for blk in reversed(stack):
                if blk["kind"] == "code":
                    is_decl_ctx = False
                    break
                if blk["kind"] in ("struct", "class", "union") and blk["name"]:
                    break
            if (is_decl_ctx and ";" in code_part and "+0x" in raw[ln - 1]
                    and not code_part.startswith(("typedef", "using", "#"))):
                comment = raw[ln - 1].split("//", 1)[1] if "//" in raw[ln - 1] else ""
                offs = [int(x, 16) for x in re.findall(r"\+0x([0-9a-fA-F]+)", comment)]
                names, shared = decl_names(code_part)
                if names and offs:
                    if not shared and len(names) > 1:
                        names = names[:1]  # the comment pins one offset, the rest are guesses
                    if not shared and len(offs) == 1:
                        offs = offs[:1]
                    for k, name in enumerate(names):
                        off = offs[k] if shared and len(offs) >= len(names) else offs[0]
                        members.append({"owner": owner, "name": name, "offset": off,
                                        "file": rel, "line": ln, "decl": code_part})
                    decl_lines.add(ln)
        # advance the block stack
        i = 0
        while i < len(line):
            ch = line[i]
            if ch == "{":
                kind, name = block_kind(head)
                if kind:
                    stack.append({"kind": kind, "name": name, "fn_owner": None, "start": ln})
                else:
                    stack.append({"kind": "code", "name": None, "fn_owner": fn_owner(head),
                                  "start": ln})
                head = ""
            elif ch == "}":
                if stack:
                    blk = stack.pop()
                    if blk["kind"] in ("struct", "class", "union") and blk["name"]:
                        spans.setdefault((rel, blk["name"]), []).append((blk["start"], ln))
                    elif blk["fn_owner"]:
                        spans.setdefault((rel, blk["fn_owner"]), []).append((blk["start"], ln))
                head = ""
            elif ch == ";":
                head = ""
            else:
                head += ch
            i += 1
    # extern globals (engine/glue writes these) with a layout comment
    for ln, line in enumerate(raw, 1):
        if re.match(r"\s*extern\b", line) and ";" in line and re.search(r"//.*0x[0-9a-fA-F]+", line):
            m = re.match(r"\s*extern\s+(?:struct\s+|class\s+)?([A-Za-z_]\w*)\s*(\*+)?\s*([A-Za-z_]\w*)", line)
            if m:
                externs.append({"name": m.group(3), "type": m.group(1) if m.group(2) else None,
                                "file": rel, "line": ln})
    return ctx, decl_lines


# ---- receiver resolution ------------------------------------------------------------------------------
DECL_TYPE = re.compile(
    r"\b(?:struct\s+|class\s+|union\s+)?([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(\*+|&)?\s*([A-Za-z_]\w*)\b"
)


def decl_index(path, code_by_file, known_types, cache):
    """{var: [(line, type), ...]} for every `Type* var` / `Type var` in the file."""
    if path in cache:
        return cache[path]
    index = defaultdict(list)
    for ln, line in enumerate(code_by_file[path], 1):
        for m in DECL_TYPE.finditer(line):
            t = m.group(1)
            if t in KEYWORDS:
                continue
            short = t.rsplit("::", 1)[-1]
            if short in known_types:
                index[m.group(3)].append((ln, short))
    cache[path] = index
    return index


def resolve_type(var, path, line_no, known_types, globals_by_name, code_by_file, index_cache):
    """Type of `var` at `line_no`: the nearest declaration at or before the site, else a global."""
    index = decl_index(path, code_by_file, known_types, index_cache)
    sites = index.get(var)
    if sites:
        best = None
        for tick, t in sites:      # file order; keep the last declaration before the use
            if tick <= line_no:
                best = t
            else:
                break
        if best:
            return best
        return sites[0][1]         # use precedes any declaration in this file: best effort
    return globals_by_name.get(var)


CAST_RECV = re.compile(
    r"\(\s*\(\s*(?:const\s+)?(?:struct\s+|class\s+|::)*([A-Za-z_]\w*(?:::\w+)*)\s*\*+\s*\)[^()]*\)$"
)


def receiver_type(line, start, line_no, owner_ctx, path, known_types, globals_by_name, code_by_file,
                  index_cache):
    """Type the `->`/`.` at `start` is applied to, or None when it cannot be pinned down."""
    pre = line[:start].rstrip()
    if re.search(r"\bthis$", pre):
        return owner_ctx
    m = CAST_RECV.search(pre)
    if m:
        t = m.group(1).rsplit("::", 1)[-1]
        return t if t in known_types else None
    m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*$", pre)
    if not m:
        return None
    var = m.group(1)
    if var in KEYWORDS:
        return None
    return resolve_type(var, path, line_no, known_types, globals_by_name, code_by_file, index_cache)


def tail_after(line, pos):
    rest = line[pos:].lstrip()
    while rest.startswith("["):
        depth = 0
        for k, ch in enumerate(rest):
            if ch == "[":
                depth += 1
            elif ch == "]":
                depth -= 1
                if depth == 0:
                    rest = rest[k + 1:].lstrip()
                    break
        else:
            break
    return rest


def is_write_tail(rest):
    if rest.startswith("=="):
        return False
    for op in ASSIGN_OPS:
        if rest.startswith(op):
            return True
    return False


# ---- audit --------------------------------------------------------------------------------------------
def load_allowlist(path):
    out = set()
    if path.exists():
        for line in path.read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                out.add(line)
    return out


def collect():
    files = sorted(p for p in SRC.rglob("*") if p.suffix in (".h", ".cpp")
                   and not (SKIP_DIRS & set(p.parts)))
    raw_by_file, code_by_file = {}, {}
    members, externs, spans = [], [], {}
    ctx_by_file, decl_by_file = {}, {}
    for p in files:
        rel = p.relative_to(ROOT).as_posix()
        raw = p.read_text(errors="replace").splitlines()
        code = strip_lines(p.read_text(errors="replace"))
        raw_by_file[p], code_by_file[p] = raw, code
        ctx, decl_lines = parse_file(rel, raw, code, members, externs, spans)
        ctx_by_file[p], decl_by_file[p] = ctx, decl_lines
    return files, code_by_file, members, externs, spans, ctx_by_file, decl_by_file


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--json", metavar="OUT", help="write the full result as JSON")
    ap.add_argument("--filter", metavar="SUBSTR", default=None,
                    help="only rows whose Struct::field, declaration or first read contains SUBSTR")
    args = ap.parse_args(argv)

    files, code_by_file, members, externs, spans, ctx_by_file, decl_by_file = collect()
    rel_of = {p: p.relative_to(ROOT).as_posix() for p in files}

    # dedupe declarations
    seen, decls, dupes = set(), [], 0
    for m in members:
        key = (m["owner"], m["name"])
        if key in seen:
            dupes += 1
            continue
        if m["name"].startswith("pad") or re.search(r"\bconst\b", m["decl"]):
            continue
        if "=" in m["decl"]:
            continue
        seen.add(key)
        decls.append(m)
    by_owner = defaultdict(dict)
    for m in decls:
        by_owner[m["owner"]][m["name"]] = m
    known_types = set(by_owner)

    globals_by_name = {}
    for g in externs:
        if g["type"]:
            globals_by_name.setdefault(g["name"], g["type"])

    # typed usage of each owner in .cpp code (a struct only seen through char*/void* views is unverifiable)
    def_lines = defaultdict(set)
    for (rel, owner), ranges in spans.items():
        for lo, hi in ranges:
            def_lines[(rel, owner)].update(range(lo, hi + 1))
    used_in_cpp = Counter()
    cpps = [p for p in files if p.suffix == ".cpp"]
    for p in cpps:
        rel = rel_of[p]
        for ln, line in enumerate(code_by_file[p], 1):
            for word in WORD.findall(line):
                if word in known_types and ln not in def_lines.get((rel, word), ()):
                    used_in_cpp[word] += 1

    writes = defaultdict(list)        # (owner, name)
    writes_name = defaultdict(list)   # name, unresolved receiver
    offset_writes = defaultdict(list)
    reads = defaultdict(list)         # (owner, name)
    bare_reads = 0
    bare_writes = 0
    index_cache = {}
    owners_by_field = defaultdict(set)
    for d in decls:
        owners_by_field[d["name"]].add(d["owner"])

    def resolve(site_line, pos, line_no, owner_ctx, path):
        return receiver_type(site_line, pos, line_no, owner_ctx, path, known_types,
                             globals_by_name, code_by_file, index_cache)

    for p in files:
        rel = rel_of[p]
        ctx = ctx_by_file[p]
        decl_lines = decl_by_file[p]
        for ln, line in enumerate(code_by_file[p], 1):
            if ln in decl_lines:
                continue
            owner_ctx = ctx[ln]
            # `->field` / `.field`
            for m in ACCESS.finditer(line):
                name = m.group(2)
                if name[0].isdigit():
                    continue
                rest = line[m.end():]
                if rest.lstrip().startswith(("(", "::")):
                    continue
                rtype = resolve(line, m.start(), ln, owner_ctx, p)
                write = is_write_tail(tail_after(line, m.end()))
                if rtype:
                    if name in by_owner.get(rtype, {}):
                        (writes if write else reads)[(rtype, name)].append((rel, ln))
                    # else: the receiver's layout is known and has no such member -> not an audited member
                elif name in owners_by_field:
                    if write:
                        writes_name[name].append((rel, ln))
                    elif len(owners_by_field[name]) == 1:
                        # one owner only: the field name alone identifies it
                        reads[(next(iter(owners_by_field[name])), name)].append((rel, ln))
            # prefix ++/-- on an access chain
            for m in re.finditer(r"(\+\+|--)\s*([A-Za-z_]\w*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)+)", line):
                chain = m.group(2)
                last = re.findall(r"[A-Za-z_]\w*", chain)[-1]
                rtype = resolve(chain, chain.rfind("->") if "->" in chain else chain.rfind("."),
                                ln, owner_ctx, p)
                if rtype:
                    if last in by_owner.get(rtype, {}):
                        writes[(rtype, last)].append((rel, ln))
                elif last in owners_by_field:
                    writes_name[last].append((rel, ln))
            # memcpy/memset into a member's address
            for m in MEM_WRITE.finditer(line):
                expr = m.group(1)
                am = list(ACCESS.finditer(expr))
                if not am:
                    continue
                last = am[-1]
                name = last.group(2)
                rtype = resolve(expr, last.start(), ln, owner_ctx, p)
                if rtype:
                    if name in by_owner.get(rtype, {}):
                        writes[(rtype, name)].append((rel, ln))
                elif name in owners_by_field:
                    writes_name[name].append((rel, ln))
            # writes through the documented layout
            if owner_ctx:
                for m in OFFSET_WRITE.finditer(line):
                    offset_writes[(owner_ctx, int(m.group(1), 16))].append((rel, ln))
            # bare `field =` / `field[i] =` inside a member function of the owner
            if owner_ctx:
                fields = by_owner.get(owner_ctx, {})
                if fields:
                    for m in re.finditer(r"([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)=", line):
                        name = m.group(1)
                        if name not in fields:
                            continue
                        before = line[:m.start()].rstrip()
                        if before.endswith(("->", ".", "::")) or before.endswith("*"):
                            continue
                        if not (before == "" or before[-1] in ";{}"):
                            continue
                        if m.end() < len(line) and line[m.end()] == "=":
                            continue
                        writes[(owner_ctx, name)].append((rel, ln))
                        bare_writes += 1
            # bare `field[...]` reads in a member function of the owner
            if owner_ctx:
                fields = by_owner.get(owner_ctx, {})
                for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\[", line):
                    name = m.group(1)
                    if name not in fields:
                        continue
                    rest = tail_after(line, m.end() - 1)
                    if is_write_tail(rest):
                        continue
                    reads[(owner_ctx, name)].append((rel, ln))
                    bare_reads += 1

    allow = load_allowlist(ALLOWLIST)

    def allowlisted(owner, name):
        return f"{owner}::{name}" in allow or f"{owner}::*" in allow

    findings, unverifiable = [], []
    for m in decls:
        sites = reads.get((m["owner"], m["name"]), [])
        if not sites:
            continue
        if (m["owner"], m["name"]) in writes or m["name"] in writes_name:
            continue
        if (m["owner"], m["offset"]) in offset_writes:
            continue
        if allowlisted(m["owner"], m["name"]):
            continue
        row = {
            "struct": m["owner"],
            "field": m["name"],
            "offset": "0x%x" % m["offset"],
            "label": "%s::%s (+0x%x)" % (m["owner"], m["name"], m["offset"]),
            "decl": "%s:%d" % (m["file"], m["line"]),
            "reads": len(sites),
            "first_read": "%s:%d" % min(sites),
        }
        if used_in_cpp.get(m["owner"], 0) == 0:
            unverifiable.append(row)
        else:
            findings.append(row)

    def rank(rows):
        return sorted(rows, key=lambda r: (-r["reads"], r["struct"], r["field"]))

    findings, unverifiable = rank(findings), rank(unverifiable)

    def keep(row):
        if not args.filter:
            return True
        needle = args.filter.lower()
        return any(needle in row[k].lower() for k in ("label", "decl", "first_read"))

    findings_shown = [r for r in findings if keep(r)]
    unverifiable_shown = [r for r in unverifiable if keep(r)]

    summary = {
        "declarations": len(members),
        "members": len(decls),
        "duplicate_declarations": dupes,
        "owners": len(by_owner),
        "extern_globals": len(externs),
        "findings": len(findings_shown),
        "unverifiable": len(unverifiable_shown),
        "allowlisted": len(allow),
        "bare_writes": bare_writes,
        "bare_reads": bare_reads,
    }
    print("unwritten_member_audit: %d offset declarations -> %d members in %d owners "
          "(%d duplicate declarations, %d extern globals)"
          % (summary["declarations"], summary["members"], summary["owners"],
             summary["duplicate_declarations"], summary["extern_globals"]))
    print("reads: %d access, %d bare subscript; writes: %d named fields, %d unresolved-receiver fields, "
          "%d offset sites; %d bare assignments"
          % (sum(len(v) for (o, n), v in reads.items()),
             summary["bare_reads"],
             sum(len(v) for v in writes.values()),
             sum(len(v) for v in writes_name.values()),
             len(offset_writes), summary["bare_writes"]))
    print("allowlisted: %d" % summary["allowlisted"])

    def show(rows, title):
        print("\n%s: %d rows" % (title, len(rows)))
        if not rows:
            return
        print("  %6s  %-42s %-34s %s" % ("reads", "member", "declaration", "first read"))
        for r in rows:
            print("  %6d  %-42s %-34s %s" % (r["reads"], r["label"], r["decl"], r["first_read"]))

    show(findings_shown, "READ BUT NEVER WRITTEN")
    show(unverifiable_shown, "UNVERIFIABLE (owner never appears as a type in any .cpp; char*/void* view)")

    if args.json:
        out = Path(args.json)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(
            {"summary": summary, "findings": findings_shown, "unverifiable": unverifiable_shown},
            indent=1) + "\n")
    return 1 if findings_shown else 0


if __name__ == "__main__":
    sys.exit(main())
