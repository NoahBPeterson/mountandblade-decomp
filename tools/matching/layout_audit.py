#!/usr/bin/env python3
"""layout_audit.py: check the `// +0xNN` field offsets (and `// 0xNN bytes` sizes) that every struct in src/
claims against what the compiler actually lays out.

Each `struct`/`class` body in src/**/*.h and src/**/*.cpp is parsed (nested braces, lambdas, comments and
strings are handled), the claimed offsets are read from the member lines' trailing comments and the claimed
size from the struct's own trailing comment. The file is then copied to work/layout_audit/ with, after each
audited struct, one compile-time assertion per claim:

    typedef char chk_<NAME>_<FIELD>[offsetof(<NAME>, <FIELD>) == 0xNN ? 1 : -1];
    typedef char chk_<NAME>_size[sizeof(<NAME>) == 0xNN ? 1 : -1];

and the copy is compiled with the source's own flags (manifest, else /O2 /MT /Gy /TP). A negative array size
(C2118) on one of those lines is a MISMATCH. On a mismatch a second pass compiles probe classes
(`struct la_probe_i : la_show<offsetof(NAME, FIELD)> {};` where `la_show` is declared but never defined) so the
compiler's "with [ N=8 ]" note prints the actual offset/size; the report shows both values.

The two live bugs this exists for:

  * struct MapEdge { char pad0c[0xc]; int a; int b; char pad18[0x24 - 0x18]; };
    pad18 is 0x24-0x18 = 0x0c bytes, so the struct is 0x20 bytes -- not the documented 0x24. The size check
    (`sizeof(MapEdge) == 0x24`) fails even though every field offset is right.
  * struct OrderEntry { char pad0[0xc]; char* name; /* +0xc */ char flag1c; /* claimed +0x1c */ ... };
    `pad0` is 0x0c and `name` is 4 bytes, so flag1c sits at +0x10, not +0x1c: `offsetof(OrderEntry, flag1c)`
    fails, and everything after it in the record is 12 bytes early (obj at +0x59b4 instead of +0x59c0,
    Party::model read through a pointer stored into the wrong member -> null deref crash). Field checks catch
    the drift at the first wrong member; later fields usually fail too, which is the drift's fingerprint.

A file whose copy does not compile for a reason of its own (a missing include, an include-order conflict) is
reported `SKIP (compile error)` and not audited: its offsets would be garbage. Files whose every claimed field
is off by one and the same amount are listed separately as a "uniform shift" (the usual cause is a record whose
offsets are documented against the containing object, e.g. `struct CamSlot { void* rec; // +0x6748 }` with
`#define CAMSLOT (*(CamSlot*)((char*)g_mission + 0x6748))`; a wrong *leading* pad looks the same). A partial
shift -- the first fields right, later ones off -- is the OrderEntry drift and is reported as a plain mismatch.

usage: layout_audit.py [--filter SUBSTR] [--json out.json] [--jobs N] [--show N]
Exit status is non-zero when mismatches exist.
"""
import argparse
import bisect
import glob
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))

import compile_all as CA                            # noqa: E402  (manifest flags / defaults)
import slice as S                                   # noqa: E402  (cl.sh path, winpath)

OUTDIR = os.path.join(ROOT, "work", "layout_audit")
OBJDIR = os.path.join(OUTDIR, "obj")
CL = S.CL


# ------------------------------------------------------------------ lexical scanning

DEF_RE = re.compile(r"\b(struct|class)\s+(?:__declspec\s*\([^)]*\)\s*)?(\w+)\s*"
                    r"(?:final\s*)?(?::[^{;{}]*)?\{")
# a member claim: the offset is the first thing in the comment. `// 0x04 ...` (no plus), `// +8, +0xc`
# (decimal), `// (ib[0].ib at +0x350 ...)` (prose) and `// 5 (+0x14)` (a vtable slot) are not claims.
CLAIM_RE = re.compile(r"^(?://+|/\*)[\s*]*(\+0x[0-9a-fA-F]+)")
SIZE_RE = re.compile(r"\b0x([0-9a-fA-F]{1,8})\s*(?:-\s*)?bytes?\b")
# words that mean the "0xNN bytes" in a struct comment is *not* the struct's size (e.g. "starts 0x10 bytes
# early", "the layer ... 0x10 bytes early", "the first 0x138b8 bytes"); reused for a handful of prose
# comments in the tree.
NOT_A_SIZE_RE = re.compile(r"\b(early|later|before|after|apart|starts?|slides?|gap|offset|behind|ahead|first)\b",
                           re.I)
FIELD_END = set("[,;=]")
FNPTR_RE = re.compile(r"\(\s*(?:__\w+\s+)?\*\s*(?:\w+\s*::\s*)*(\w+)\s*\)")


class Comment:
    __slots__ = ("start", "end", "text")

    def __init__(self, start, end, text):
        self.start, self.end, self.text = start, end, text


def mask(src):
    """Mask comments and string/char literals (keeping newlines) so braces can be matched, and return the
    comments found: [(start, end, text)]."""
    out = list(src)
    comments = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i)
            if j < 0:
                j = n
            comments.append(Comment(i, j, src[i:j]))
            for k in range(i, j):
                out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            comments.append(Comment(i, j, src[i:j]))
            for k in range(i, j):
                if src[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            q = c
            j = i + 1
            while j < n:
                if src[j] == "\\":
                    j += 2
                    continue
                if src[j] == q:
                    j += 1
                    break
                j += 1
            for k in range(i, min(j, n)):
                if src[k] != "\n":
                    out[k] = " "
            i = j
        else:
            i += 1
    return "".join(out), comments


def line_starts(text):
    starts = [0]
    i = text.find("\n")
    while i >= 0:
        starts.append(i + 1)
        i = text.find("\n", i + 1)
    return starts


class Struct:
    __slots__ = ("name", "kw", "decl", "open", "close", "end")

    def __init__(self, name, kw, decl, open_, close, end):
        self.name, self.kw, self.decl = name, kw, decl
        self.open, self.close, self.end = open_, close, end


def find_structs(masked):
    """All `struct|class NAME { ... }` definitions (including nested ones), with the position just past the
    terminating ';'."""
    out = []
    for m in DEF_RE.finditer(masked):
        open_ = m.end() - 1
        depth = 0
        j = open_
        while j < len(masked):
            if masked[j] == "{":
                depth += 1
            elif masked[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        if j >= len(masked):
            continue                                    # unbalanced (a macro or #if trick): ignore
        close = j
        semi = masked.find(";", close + 1)
        if semi < 0 or semi - close > 300 or "{" in masked[close + 1:semi]:
            semi = close                                # no clean terminator; insert right after '}'
        out.append(Struct(m.group(2), m.group(1), m.start(), open_, close, semi + 1))
    return out


def field_name(code):
    """The declared field name in one member declaration line (text before its trailing comment).
    `float dirx, diry;` -> dirx (the claim describes the first declarator), `unsigned f74;` -> f74,
    `char pad0c[0xc];` -> pad0c. None when the line is not a simple declaration."""
    code = code.strip()
    if not code:
        return None
    # split into top-level statements (`;` inside a method body or a nested union does not split), and take the
    # first one that declares storage: on `int clock0; __int64 t0; // +0x08 / +0x10` the claim belongs to
    # clock0, on `void f() { x; } // ...` there is none.
    segs = []
    depth = 0
    cur = []
    for ch in code:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        elif ch == ";" and depth <= 0:
            segs.append("".join(cur))
            cur = []
            continue
        cur.append(ch)
    segs.append("".join(cur))
    for seg in segs:
        name = _decl_name(seg)
        if name:
            return name
    return None


def _decl_name(seg):
    seg = seg.strip()
    if not seg:
        return None
    if "}" in seg:
        seg = seg[seg.rfind("}") + 1:].strip()          # `} named;` after a union/struct body
        if not seg:
            return None
    fp = FNPTR_RE.search(seg)
    if fp:
        return fp.group(1)                              # a function *pointer* is a plain data member
    if "(" in seg:
        return None                                     # a method: the `+0xNN` in its comment is a vtable slot
    if seg.count(")") > seg.count("("):
        return None                                     # the tail of a declaration continued from an earlier line
    if "{" in seg:
        return None
    for m in re.finditer(r"[A-Za-z_]\w*", seg):
        k = m.end()
        while k < len(seg) and seg[k] in " \t":
            k += 1
        if k < len(seg):
            if seg[k] not in FIELD_END:
                continue
            if seg[k] == ":" and seg[k:k + 2] == "::":
                continue
            if seg[k] == ":":
                return None                             # a bitfield: offsetof is impossible
        if m.group(0) in TYPED_NAMES:
            continue
        return m.group(0)
    return None


TYPED_NAMES = {"int", "char", "short", "long", "float", "double", "unsigned", "signed", "bool", "void",
               "const", "volatile", "struct", "union", "enum", "class", "static", "extern", "register",
               "wchar_t", "__int64", "__int32", "__int16", "auto", "mutable"}


def size_claim(comment):
    """The struct size in a struct's trailing comment (`0x20 bytes`, `0x204-byte`, `// 0x5ac8 bytes, ...`)."""
    if not comment:
        return None
    for m in SIZE_RE.finditer(comment):
        before = comment[max(0, m.start() - 28):m.start()]
        if NOT_A_SIZE_RE.search(before):
            continue
        after = comment[m.end():m.end() + 16]
        if after.lstrip().startswith("+"):
            continue                                    # "0x24 bytes + extras": the base size, not the size
        if re.match(r"\s*(elements?|records?|rows?|entries?|items?)\b", after):
            continue                                    # "0x60-byte elements": a element size, not the struct
        if re.search(r"\b(records?|elements?|rows?|entries?|items?)\s+of\s*$", comment[max(0, m.start() - 30):m.start()]):
            continue                                    # "trigger records of 0x78 bytes": the element size
        return int(m.group(1), 16)
    return None


def has_data_members(masked, sd):
    """True when the body declares storage: a top-level statement with no `(` (a member function declaration
    has one) or a function pointer. Method-only views (`struct BattleRec { void detach(); };`) have none, and a
    "0x98-byte object" in their comment describes the object they are a view of, not their own sizeof."""
    depth = 0
    stmt = []
    for ch in masked[sd.open + 1:sd.close]:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        elif ch == ";" and depth <= 0:
            s = "".join(stmt)
            stmt = []
            if "(" not in s or FNPTR_RE.search(s):
                return True
            continue
        stmt.append(ch)
    return False


def struct_claims(masked, comments_by_line, starts, sd, nested):
    """[(field, claimed_offset)] + claimed size for one struct."""
    def ln(off):
        return bisect.bisect_right(starts, off)         # 1-based

    l0, l1 = ln(sd.open), ln(sd.close)
    # blank the nested struct bodies so their member lines are not attributed to this struct
    blank = set()
    for other in nested:
        for k in range(max(other.open + 1, sd.open + 1), min(other.close, sd.close)):
            blank.add(k - (sd.open + 1))
    body = list(masked[sd.open + 1:sd.close])
    for k in blank:
        if 0 <= k < len(body) and body[k] != "\n":
            body[k] = " "
    body = "".join(body)
    base = sd.open + 1

    fields = []
    for line in range(l0, l1 + 1):
        ls = starts[line - 1]
        for c in comments_by_line.get(line, ()):
            m = CLAIM_RE.match(c.text)
            if not m:
                continue
            lo = max(c.start, base)
            if lo >= min(c.end, sd.close):
                continue
            # only this line's code, so a claim on a later line is not read here
            cstart = lo - base
            lstart = max(ls - base, 0)
            prefix = body[lstart:cstart]
            if not prefix.strip():
                continue
            name = field_name(prefix)
            if not name:
                continue
            fields.append((name, int(m.group(1)[3:], 16)))
            break

    # dedupe (a field claimed twice on one struct: keep the first)
    seen, uniq = set(), []
    for f, o in fields:
        if f not in seen:
            seen.add(f)
            uniq.append((f, o))

    # the claimed size lives in the comment on the struct's own `struct NAME {` line (`{` included: the comment
    # comes after the brace), possibly on the line before an opening brace on its own line.
    size = None
    decl_lines = {ln(sd.decl), l0}
    if has_data_members(masked, sd):
        for line in sorted(decl_lines):
            for c in comments_by_line.get(line, ()):
                if c.start < sd.decl or ln(c.start) not in decl_lines:
                    continue
                size = size_claim(c.text)
                if size is not None:
                    break
            if size is not None:
                break
    if size is not None and uniq and max(o for _f, o in uniq) >= size:
        size = None     # the struct's own claims put fields outside the "size": that note describes a view
    return uniq, size


# ------------------------------------------------------------------ generated copies

HDR = "#include <stddef.h>\ntemplate<int N> struct la_show;\n"


def typecheck_name(used, *bits):
    base = "chk_" + "_".join(bits)
    name, n = base, 2
    while name in used:
        name = "%s_%d" % (base, n)
        n += 1
    used.add(name)
    return name


def is_template(masked, sd):
    """True when the definition is a template (`template<class T>` on its line or the one before), which cannot
    be named in offsetof/sizeof without its arguments."""
    pre = masked[max(0, sd.decl - 400):sd.decl]
    t = pre.rfind("template")
    return t >= 0 and ";" not in pre[t:] and "}" not in pre[t:]


def build(text, masked, structs, comments_by_line, starts, probe=False, only=()):
    """Generated source text + {line: check} where check is (kind, struct, field, claimed).
    `text` is copied into the generated file (comments and strings intact); `masked` is the same text with
    comments/literals blanked, used to find the members. probe=False: the chk_ typedefs. probe=True:
    `la_show<...>` probe classes (for the failed checks only, given by `only`)."""
    used = set()
    inserts = []
    audited = 0
    nfields = 0
    probe_n = 0                                         # probe names are file-wide (a nested reuse collides)
    for sd in structs:
        others = [o for o in structs if o is not sd and sd.open < o.open and o.close < sd.close]
        claims, size = struct_claims(masked, comments_by_line, starts, sd, others)
        if not claims and size is None:
            continue
        if is_template(masked, sd) or any(o.open < sd.open and sd.close < o.close and is_template(masked, o)
                                          for o in structs):
            continue
        lines, checks = [], []
        if probe:
            for field, off in claims:
                if (sd.name, field, "field") not in only:
                    continue
                lines.append("struct la_probe_%d : la_show<offsetof(%s, %s)> {};"
                             % (probe_n, sd.name, field))
                probe_n += 1
                checks.append(("field", sd.name, field, off))
            if size is not None and (sd.name, "size", "size") in only:
                lines.append("struct la_probe_%d : la_show<sizeof(%s)> {};" % (probe_n, sd.name))
                probe_n += 1
                checks.append(("size", sd.name, "size", size))
        else:
            audited += 1
            for field, off in claims:
                name = typecheck_name(used, sd.name, field)
                lines.append("typedef char %s[offsetof(%s, %s) == 0x%x ? 1 : -1];"
                             % (name, sd.name, field, off))
                checks.append(("field", sd.name, field, off))
                nfields += 1
            if size is not None:
                name = typecheck_name(used, sd.name, "size")
                lines.append("typedef char %s[sizeof(%s) == 0x%x ? 1 : -1];" % (name, sd.name, size))
                checks.append(("size", sd.name, "size", size))
        if not lines:
            continue
        block = "// layout_audit: %s\n" % sd.name + "\n".join(lines) + "\n"
        # a `class`'s members are private by default; make them reachable for the checks (scoped to this one
        # definition so a `template<class T>` in an included header is untouched)
        if sd.kw == "class":
            inserts.append((sd.decl, "#define class struct\n", [], 0))
            inserts.append((sd.end, "#undef class\n", [], 0))
        inserts.append((sd.end, block, checks, 1))
    if probe and not inserts:
        return None, {}, 0, 0

    inserts.sort(key=lambda t: t[0])
    parts = []
    ln = 1
    last = 0
    line_map = {}
    for pos, block, checks, lead in inserts:
        seg = text[last:pos]
        parts.append(seg)
        ln += seg.count("\n")
        parts.append("\n")
        ln += 1
        for off, chk in enumerate(checks):
            line_map[ln + lead + off] = chk
        parts.append(block)
        ln += block.count("\n")
        last = pos
    parts.append(text[last:])
    gen = "".join(parts)
    shift = HDR.count("\n")
    line_map = {k + shift: v for k, v in line_map.items()}
    return HDR + gen, line_map, audited, nfields


# ------------------------------------------------------------------ compile

ERR_RE = re.compile(r"^(.*)\((\d+)\)\s*:\s*(?:fatal error|error)\s+(C\d+)\s*:\s*(.*)$")


def compile_copy(src, gen_path, obj_path, flags, text=""):
    # The checks read members from outside the type, so access has to be lifted: `private:`/`protected:` are
    # rewritten globally (they never appear as anything but access labels, so the STL is safe); a class's
    # *implicit* private default is handled in the generated copy by a `#define class struct` scoped to the
    # audited class itself (a file-wide define would rewrite `template<class T>` in <vector> and friends).
    access = ["/Dprivate=public", "/Dprotected=public"]
    args = [CL, "/nologo", "/c"] + list(flags) + access + [
        "/I" + S.winpath(os.path.dirname(src)),
        "/Fo" + S.winpath(obj_path), S.winpath(gen_path)]
    r = subprocess.run(args, capture_output=True, text=True)
    return r.returncode, (r.stdout + r.stderr)


def parse_errors(log):
    return [(int(m.group(2)), m.group(3), m.group(4).strip())
            for m in (ERR_RE.match(l) for l in log.splitlines()) if m]


def parse_probe_values(log):
    """{line: actual value} from `with [ N=8 ]` notes under each C2504/C2079 error."""
    vals = {}
    lines = log.splitlines()
    cur = None
    for l in lines:
        m = ERR_RE.match(l)
        if m:
            cur = int(m.group(2))
            continue
        if cur is not None:
            v = re.search(r"\bN=(-?\d+)\b", l)
            if v:
                vals.setdefault(cur, int(v.group(1)))
    return vals


# ------------------------------------------------------------------ sources

def sources():
    return sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True) +
                  glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))


def flags_for(src):
    man = src[:-4] + ".manifest.txt" if src.endswith(".cpp") else src + ".manifest.txt"
    return CA.manifest_flags(man) or list(CA.DEFAULT)


def sanitize(rel):
    return rel.replace("/", "_").replace(".", "_")


def audit_file(src, jobs_unused=None):
    rel = os.path.relpath(src, ROOT)
    text = open(src, encoding="utf-8", errors="replace").read()
    masked, comments = mask(text)
    starts = line_starts(text)
    cbl = {}
    for c in comments:
        l0 = bisect.bisect_right(starts, c.start)
        l1 = bisect.bisect_right(starts, c.end)
        for line in range(l0, l1 + 1):
            cbl.setdefault(line, []).append(c)
    structs = find_structs(masked)
    gen, line_map, audited, nfields = build(text, masked, structs, cbl, starts)
    res = {"file": rel, "status": "ok", "structs": audited, "fields": nfields,
           "mismatches": [], "check_errors": [], "skipped": None}
    if not line_map:
        res["status"] = "empty"
        return res
    flags = flags_for(src)
    os.makedirs(OBJDIR, exist_ok=True)
    stem = sanitize(rel)
    gen_path = os.path.join(OUTDIR, stem + ".cpp")
    obj = os.path.join(OBJDIR, stem + ".obj")
    os.makedirs(OUTDIR, exist_ok=True)
    open(gen_path, "w", encoding="utf-8").write(gen)
    rc, log = compile_copy(src, gen_path, obj, flags, text)
    errs = parse_errors(log)
    failed = []
    unrelated = []
    for line, code, msg in errs:
        chk = line_map.get(line)
        if chk is None:
            unrelated.append("C%s: %s" % (code[1:], msg))
        elif code == "C2118":
            failed.append(chk)
        else:
            res["check_errors"].append({"struct": chk[1], "field": chk[2], "error": code,
                                        "message": msg})
    if unrelated:
        # the copy does not compile for reasons of its own (a missing include, an include-order conflict, the
        # TU not being self-contained): every offsetof/sizeof it yields is suspect, so skip it entirely
        res["status"] = "SKIP (compile error)"
        res["skipped"] = unrelated[:3]
        res["check_errors"] = []
        return res
    seen = set()
    uniq_failed = []
    for chk in failed:
        if chk not in seen:
            seen.add(chk)
            uniq_failed.append(chk)
    if uniq_failed:
        only = {(c[1], c[2], c[0]) for c in uniq_failed}
        pgen, pmap, _a, _f = build(text, masked, structs, cbl, starts, probe=True, only=only)
        actual = {}
        if pgen is not None:
            pgen_path = os.path.join(OUTDIR, stem + ".probe.cpp")
            pobj = os.path.join(OBJDIR, stem + ".probe.obj")
            open(pgen_path, "w", encoding="utf-8").write(pgen)
            _rc, plog = compile_copy(src, pgen_path, pobj, flags, text)
            for line, val in parse_probe_values(plog).items():
                if line in pmap:
                    kind, name, field, _claimed = pmap[line]
                    actual[(kind, name, field)] = val
        for kind, name, field, claimed in uniq_failed:
            res["mismatches"].append({"struct": name, "field": field, "kind": kind,
                                      "claimed": claimed,
                                      "actual": actual.get((kind, name, field)),
                                      "actual_hex": ("0x%x" % actual[(kind, name, field)]
                                                     if (kind, name, field) in actual else None)})
    if res["mismatches"]:
        mark_uniform_shifts(res, masked, structs, cbl, starts)
    elif rc:
        res["status"] = "SKIP (compile error)"
        diag = [l.strip() for l in log.splitlines() if l.strip()][:3]
        res["skipped"] = diag or ["cl exit %d, no diagnostic" % rc]
    return res


def mark_uniform_shifts(res, masked, structs, cbl, starts):
    """Tag mismatch sets in which *every* claimed field of a struct is off by one and the same amount.

    That is the signature of offsets documented against a containing record rather than against the struct
    (`struct CamSlot { void* rec; // +0x6748 ... }` with `#define CAMSLOT (*(CamSlot*)((char*)g_mission +
    0x6748))`, a nested `struct Ring { int count; // +0xb0b8 }`, or a derived record whose claims include its
    base). It is also what a wrong leading pad looks like, so these are reported separately, not dropped:
    a *partial* shift (the first fields right, later ones off -- the OrderEntry shape) stays a plain mismatch.
    """
    claims = {}
    for sd in structs:
        others = [o for o in structs if o is not sd and sd.open < o.open and o.close < sd.close]
        cl, _size = struct_claims(masked, cbl, starts, sd, others)
        if cl:
            claims.setdefault(sd.name, cl)
    by_struct = {}
    for m in res["mismatches"]:
        if m["kind"] == "field" and m["actual"] is not None:
            by_struct.setdefault(m["struct"], []).append(m)
    for name, ms in by_struct.items():
        cl = claims.get(name)
        if not cl or {f for f, _o in cl} != {m["field"] for m in ms}:
            continue
        deltas = {m["claimed"] - m["actual"] for m in ms}
        if len(deltas) != 1:
            continue
        shift = deltas.pop()
        if not shift:
            continue
        for m in ms:
            m["uniform_shift"] = shift


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter", help="only paths containing SUBSTR")
    ap.add_argument("--json", help="write the full result table here")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--show", type=int, default=15, help="mismatch rows to print (default 15)")
    a = ap.parse_args()

    todo = [p for p in sources() if not a.filter or a.filter in os.path.relpath(p, ROOT)]
    os.makedirs(OUTDIR, exist_ok=True)
    results = []
    with ThreadPoolExecutor(a.jobs) as ex:
        for res in ex.map(audit_file, todo):
            results.append(res)

    structs = sum(r["structs"] for r in results)
    fields = sum(r["fields"] for r in results)
    mm = [{"file": r["file"], **x} for r in results for x in r["mismatches"]]
    drift = [m for m in mm if not m.get("uniform_shift")]
    uniform = [m for m in mm if m.get("uniform_shift")]
    skips = [r for r in results if r["status"].startswith("SKIP")]
    cerr = [r for r in results if r["check_errors"]]

    print("%d structs audited, %d fields checked, %d mismatches"
          % (structs, fields, len(mm)))
    if skips:
        print("%d files skipped (unrelated compile error), %d files with unusable checks, "
              "%d mismatches in uniform-shift structs (absolute offsets / missing leading pad)"
              % (len(skips), len(cerr), len(uniform)))
    if drift:
        print("\n%-52s %-26s %-24s %10s %10s" % ("file", "struct", "field", "claimed", "actual"))
        for m in drift[:a.show]:
            print("%-52s %-26s %-24s %10s %10s"
                  % (m["file"][:52], m["struct"][:26], str(m["field"])[:24],
                     "0x%x" % m["claimed"], m["actual_hex"] or "?"))
        if len(drift) > a.show:
            print("... %d more (see --json)" % (len(drift) - a.show))
    if uniform:
        by = {}
        for m in uniform:
            by.setdefault((m["file"], m["struct"]), [0, m["uniform_shift"]])[0] += 1
        print("\nuniform shift (every claimed field off by the same amount):")
        for (f, s), (n, shift) in list(by.items())[:10]:
            print("  %-50s %-24s %2d fields  shift %+#x" % (f[:50], s[:24], n, shift))
        if len(by) > 10:
            print("  ... %d more (see --json)" % (len(by) - 10))
    if skips:
        print("\nskipped files:")
        for r in skips[:10]:
            print("  %s: %s" % (r["file"], "; ".join(r["skipped"] or [])[:150]))
        if len(skips) > 10:
            print("  ... %d more" % (len(skips) - 10))
    if cerr:
        print("\nunusable checks (field could not be offsetof'd):")
        for r in cerr[:10]:
            for e in r["check_errors"][:2]:
                print("  %s %s::%s: %s %s" % (r["file"], e["struct"], e["field"],
                                              e["error"], e["message"])[:180])
    if a.json:
        json.dump({"structs": structs, "fields": fields, "mismatches": mm,
                   "drift": len(drift), "uniform_shift": len(uniform),
                   "skipped_files": [r["file"] for r in skips], "files": results},
                  open(a.json, "w"), indent=1)
    return 1 if mm else 0


if __name__ == "__main__":
    sys.exit(main())
