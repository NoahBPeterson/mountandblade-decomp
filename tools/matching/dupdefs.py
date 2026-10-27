#!/usr/bin/env python3
"""dupdefs.py: find model records defined more than once with a different size.

usage: dupdefs.py [--min 2] [--only NAME] [--sizes]

Why: a "size only" modelling struct (the same name, a different member list) is folded by the linker with the real
one - every std::vector<T>/_Uninit_copy<T>/_Ufill<T>/allocator<T>::construct body is a COMDAT, so whichever body
wins runs on both definitions' data. Two crashes on Start-New-Game came from exactly that (RowRec 0x1c8 vs 0x20,
item_modifier 0x30 vs 0x50); this audit lists the remaining candidates.

For every `struct`/`class`/`union` definition under src/ it records the file, the line, the body and an inferred
size (when every member is a plain 1/2/4/8-byte item, an array of them, or a known builtin). A name defined twice
with different bodies is reported, sorted: size mismatch first (the dangerous ones), then differing bodies whose
size could not be inferred, then identical bodies (harmless duplicates).

--sizes asks the real compiler (tools/matching/cl.sh) for sizeof() of each definition that lives in a *header*:
one probe TU per header, so what is reported is the compiler's answer, not the inference.
"""
import argparse, os, re, subprocess, sys, collections

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "src")
CL = os.path.join(ROOT, "tools", "matching", "cl.sh")

BUILTIN = {"char": 1, "signed char": 1, "unsigned char": 1, "bool": 1, "__int8": 1, "short": 2, "unsigned short": 2,
           "__int16": 2, "int": 4, "unsigned": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "__int32": 4,
           "float": 4, "__int64": 8, "double": 8, "void*": 4, "char*": 4}
DECL = re.compile(r'\b(struct|class|union)\s+([A-Za-z_]\w*)\s*(?:final\s*)?(?::[^;{]*)?\{')


def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', ' ', s, flags=re.S)
    s = re.sub(r'//[^\n]*', ' ', s)
    return s


def match_brace(text, open_idx):
    depth = 0
    for i in range(open_idx, len(text)):
        c = text[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
    return -1


def members(body):
    """Split a definition body into member declarations (top level, ';'-terminated)."""
    out, depth, cur = [], 0, []
    for c in body:
        if c in '{([':
            depth += 1
        elif c in '})]':
            depth -= 1
        if c == ';' and depth == 0:
            out.append(''.join(cur).strip())
            cur = []
        else:
            cur.append(c)
    return [m for m in out if m]


def infer_size(body, known):
    """Size in bytes when every member is plain data; None otherwise."""
    total, align = 0, 1
    for m in members(body):
        m = m.strip()
        if not m or m.startswith(('typedef', 'using', 'static', 'friend')):
            continue
        if '(' in m:                                   # a member function
            continue
        if re.search(r'\b(virtual|template)\b', m):
            return None
        m = re.sub(r'\b(public|private|protected)\s*:', '', m).strip()
        if not m:
            continue
        arr = re.search(r'\[\s*(0[xX][0-9a-fA-F]+|\d+)\s*\]', m)
        count = int(arr.group(1), 0) if arr else 1
        base = re.sub(r'\[.*?\]', ' ', m)
        base = re.sub(r'\b[A-Za-z_]\w*\s*(?=$)', '', base).strip()      # drop the member name
        base = base.replace('const', '').replace('volatile', '').strip()
        ptr = base.replace(' ', '')
        if ptr.endswith('*') or base == 'void*':
            size, a = 4, 4
        elif base in BUILTIN:
            size, a = BUILTIN[base], BUILTIN[base]
        elif base in known:
            size, a = known[base], 4
        else:
            return None
        if align < a:
            align = a
        total = (total + a - 1) // a * a
        total += size * count
    return (total + align - 1) // align * align if align else total


def scan():
    defs = collections.defaultdict(list)
    for dirpath, _, files in os.walk(SRC):
        for f in files:
            if not f.endswith(('.h', '.hpp', '.cpp')):
                continue
            path = os.path.join(dirpath, f)
            raw = open(path, errors='replace').read()
            text = strip_comments(raw)
            for m in DECL.finditer(text):
                name = m.group(2)
                ob = m.end() - 1
                cb = match_brace(text, ob)
                if cb < 0:
                    continue
                body = text[ob + 1:cb]
                line = raw.count('\n', 0, m.start()) + 1
                defs[name].append({"file": os.path.relpath(path, ROOT), "line": line,
                                   "body": re.sub(r'\s+', ' ', body).strip(),
                                   "members": members(body)})
    return defs


def canon(body):
    return re.sub(r'\s+', '', body)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", default="")
    ap.add_argument("--sizes", action="store_true", help="ask cl for sizeof() of every header definition")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()

    defs = scan()
    known = {}
    for name, ds in defs.items():
        bodies = {canon(d["body"]) for d in ds}
        if len(bodies) == 1:
            sz = infer_size(ds[0]["body"], known)
            if sz is not None:
                known[name] = sz
    for name, ds in defs.items():
        sz = infer_size(ds[0]["body"], known)
        if sz is not None and len({canon(d["body"]) for d in ds}) == 1:
            known[name] = sz

    if a.sizes:
        # one probe TU per header: `int sz_<Type> = sizeof(<Type>);`, compiled with /FAs, then read back from the
        # listing (`?sz_<Type>@@3HA DD 0...H`). The compiler's answer, not the inference: it accounts for vptrs,
        # padding, base classes and the members this tool does not model.
        import tempfile
        by_header = collections.defaultdict(set)
        for name, ds in defs.items():
            if len(ds) > 1 and (a.only == "" or a.only.lower() in name.lower()):
                for d in ds:
                    if d["file"].endswith(('.h', '.hpp')):
                        by_header[d["file"]].add(name)
        for header, names in sorted(by_header.items()):
            names = sorted(names)
            tmpdir = tempfile.mkdtemp()
            src = '#include "Z:%s"\n' % os.path.join(ROOT, header).replace("/", "\\")
            src += "".join('int sz_%s = sizeof(%s);\n' % (n, n) for n in names)
            open(os.path.join(tmpdir, "p.cpp"), "w").write(src)
            subprocess.run([CL, "/nologo", "/c", "/FAs",
                            "/FaZ:" + os.path.join(tmpdir, "p.asm").replace("/", "\\"),
                            "Z:" + os.path.join(tmpdir, "p.cpp").replace("/", "\\"),
                            "/FoZ:" + os.path.join(tmpdir, "p.obj").replace("/", "\\")],
                           cwd=ROOT, capture_output=True)
            asm = os.path.join(tmpdir, "p.asm")
            if not os.path.exists(asm):
                continue
            text = open(asm, errors="replace").read()
            for n in names:
                m = re.search(r'^\?sz_%s@@3HA\s+DD\s+([0-9a-fA-F]+)H' % re.escape(n), text, re.M)
                if m:
                    for d in defs[n]:
                        if d["file"] == header:
                            d["cl_size"] = int(m.group(1), 16)

    usage = collections.defaultdict(list)          # where the name is used as a *value* type (fold-sensitive)
    for dirpath, _, files in os.walk(SRC):
        for f in files:
            if not f.endswith(('.h', '.hpp', '.cpp')):
                continue
            path = os.path.join(dirpath, f)
            text = strip_comments(open(path, errors='replace').read())
            for name in defs:
                if re.search(r'vector\s*<\s*(?:\w+\s*,\s*\w+\s*,\s*)?%s\b' % re.escape(name), text) \
                        or re.search(r'sizeof\s*\(\s*%s\s*\)' % re.escape(name), text):
                    usage[name].append(os.path.relpath(path, ROOT))

    rows = []
    for name, ds in defs.items():
        if a.only and a.only.lower() not in name.lower():
            continue
        if len(ds) < 2:
            continue
        bodies = {canon(d["body"]) for d in ds}
        if len(bodies) == 1 and not a.verbose:
            continue
        if sum(1 for x in (infer_size(d["body"], known) for d in ds) if x) < 2 and not a.verbose:
            continue
        sizes = [d.get("cl_size") for d in ds]
        inferred = [infer_size(d["body"], known) for d in ds]
        s = sizes if all(x is not None for x in sizes) else inferred
        data = [x for x in s if x]                      # function-only views (size 0) are opaque handles, harmless
        bad = len(set(data)) > 1
        rows.append((0 if bad else 1, name, ds, s))
    rows.sort(key=lambda r: (r[0], r[1]))
    for rank, name, ds, s in rows:
        print("%s %s   sizes=%s" % ("SIZE!!" if rank == 0 else "body  ", name, s))
        for d in ds:
            print("    %s:%d  %s" % (d["file"], d["line"], d["body"][:150]))
        if rank == 0 and usage.get(name):
            print("    used as a value type in: %s" % ", ".join(sorted(set(usage[name]))[:6]))
    print("\n%d names defined more than once with differing bodies (%d with a size mismatch)"
          % (len(rows), sum(1 for r in rows if r[0] == 0)))


if __name__ == "__main__":
    main()
