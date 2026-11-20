#!/usr/bin/env python3
"""Compare the MSVC objects of src/ TUs before and after a source edit, symbol by symbol.

The layout64 lane (ticket 006, docs/layout64_inventory.md) changes types so src/ is right at 64 bits while the MSVC 7.1
build must not move. An identical object means every row of the TU, byte-exact or nonmatching, is unchanged, so this is
the regression check for such edits: cheaper than check.py per row and it also covers data and unlisted functions.

    objcmp.py cmp [--base REV] [--rename OLD=NEW] [--rename-re 'PAT=>REP'] [-v] TU.cpp ...
    objcmp.py cmp --changed [--base REV]          every TU that sees a file changed since REV (src/ edits in the tree)
    objcmp.py includers HEADER ...                the src/ TUs that reach HEADER through #include "..."

Base side: a detached git worktree of REV (default HEAD) under --base-tree (default work/objcmp_base, created and moved
to REV on demand; its work/ and .venv are symlinks to this repo's, so it uses the same toolchain and object cache).
Both sides compile through tools/difftest/slice.compile_obj of their own tree (cl.sh under wine, or the compile
server unless MB_RC=0; objects cached by source + headers + flags, so an unchanged TU costs nothing on the second run).
Compiles run one at a time. Flag sets per TU: those of its manifest rows plus explicit `flags /..` notes of its
nonmatching rows (the defaults /O2 /MT /Gy /TP when it has none).

Per symbol: every defined symbol (function, data, string literal, static) is compared by its bytes (up to the next
symbol in its section) and its relocations by target name; a COMDAT section's whole content belongs to its symbol.
Local label numbers ($L123, $SG456) are normalised; .debug / .sxdata / .drectve sections are ignored. --rename maps a
base-side mangled name to its new spelling (for a deliberate signature change), --rename-re does the same by regex.
Exit 1 when any symbol differs or a compile fails.
"""
import argparse, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
os.environ.setdefault("MB_PP", "0")          # the second cache level costs GBs per miss and buys nothing here
import slice as S  # noqa: E402
from coff import Coff  # noqa: E402

IGNORED = (".debug", ".sxdata", ".drectve")
LABEL = re.compile(r"\$(SG|L|T|S|M|E)?\d+")


# ---------------------------------------------------------------- includers

INC = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def include_graph():
    """{file: [files it includes]} over src/ and match/include (quoted includes, resolved like slice._quoted_includes)."""
    g = {}
    for top in ("src", os.path.join("match", "include")):
        for d, _sub, files in os.walk(os.path.join(ROOT, top)):
            for f in files:
                if not f.endswith((".cpp", ".c", ".h", ".hpp", ".inl")):
                    continue
                p = os.path.join(d, f)
                out = []
                for name in INC.findall(open(p, "rb").read().decode("latin-1")):
                    for base in (d, os.path.join(ROOT, "match", "include")):
                        c = os.path.normpath(os.path.join(base, name))
                        if os.path.isfile(c):
                            out.append(c)
                            break
                g[p] = out
    return g


def includers(headers, graph=None):
    graph = graph or include_graph()
    want = {os.path.abspath(h) for h in headers}
    rev = {}
    for f, incs in graph.items():
        for i in incs:
            rev.setdefault(i, set()).add(f)
    seen, todo = set(want), list(want)
    while todo:
        for f in rev.get(todo.pop(), ()):
            if f not in seen:
                seen.add(f)
                todo.append(f)
    return sorted(f for f in seen if f.startswith(os.path.join(ROOT, "src") + os.sep) and f.endswith((".cpp", ".c")))


# ---------------------------------------------------------------- flags

def flag_sets(src):
    base = src.rsplit(".", 1)[0]
    sets = set()
    man = base + ".manifest.txt"
    if os.path.exists(man):
        for line in open(man, errors="replace"):
            p = line.split("#", 1)[0].split()
            if len(p) >= 4:
                sets.add(tuple(t for t in p[3:] if t.startswith("/")))
    nm = base + ".nonmatching.txt"
    if os.path.exists(nm):
        for line in open(nm, errors="replace"):
            for m in S.EXPLICIT_RE.finditer(line):
                f = S.flags_from_reason(m.group(1))
                if f:
                    sets.add(tuple(f))
    sets.discard(())
    return sorted(sets) or [tuple(S.DEFAULT_FLAGS)]


# ---------------------------------------------------------------- base tree

def git(*a, cwd=ROOT):
    return subprocess.run(["git", *a], cwd=cwd, capture_output=True, text=True, check=True).stdout.strip()


def base_tree(path, rev):
    sha = git("rev-parse", rev)
    if not os.path.exists(os.path.join(path, ".git")):
        git("worktree", "add", "--detach", path, sha)
    elif git("rev-parse", "HEAD", cwd=path) != sha:
        git("checkout", "--detach", "-q", sha, cwd=path)
    for link in ("work", ".venv"):
        dst = os.path.join(path, link)
        if not os.path.lexists(dst) and os.path.exists(os.path.join(ROOT, link)):
            os.symlink(os.path.join(ROOT, link), dst)
    return sha


def compile_in(tree, src, flags):
    """compile_obj of `tree`'s own tools (its cl.sh puts its own src/include first) -> (obj, log)."""
    if tree == ROOT:
        return S.compile_obj(src, list(flags))
    code = ("import sys,json; sys.path.insert(0, %r); import slice; o, l = slice.compile_obj(%r, %r); "
            "print(json.dumps([o, l]))" % (os.path.join(tree, "tools", "difftest"), src, list(flags)))
    r = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, env=dict(os.environ))
    if r.returncode:
        return None, r.stderr[-2000:]
    import json
    return json.loads(r.stdout.strip().splitlines()[-1])


# ---------------------------------------------------------------- per-symbol digest

def symbols(obj, rename=None, rename_re=()):
    """{normalised symbol name: (bytes, relocs)} for every defined symbol of a COFF object."""
    c = Coff(obj)

    def norm(n):
        n = (rename or {}).get(n, n)
        for a, b in rename_re:
            n = re.sub(a, b, n)
        return LABEL.sub(lambda m: "$%s#" % (m.group(1) or ""), n)

    per_sec = {}
    for s in c.syms.values():
        if s.secno > 0 and s.cls in (2, 3, 6) and not (s.cls == 3 and s.value == 0 and s.naux and s.name.startswith(".")):
            per_sec.setdefault(s.secno, []).append(s)
    out = {}
    for secno, syms in per_sec.items():
        sec = c.sections[secno - 1]
        if sec.name.startswith(IGNORED) or sec.discard:
            continue
        syms.sort(key=lambda s: (s.value, s.cls != 2))
        if sec.flags & 0x1000:          # a COMDAT section belongs whole to its symbol (switch-table labels inside it are covered)
            syms = [s for s in syms if s.value == 0][:1]
        else:                           # code labels ($L123, class 6) are positions inside a function, not symbols
            syms = [s for s in syms if s.cls != 6 and not re.match(r"\$L\d+$", s.name)]
        starts = sorted({s.value for s in syms})
        for s in syms:
            if sec.flags & 0x1000:
                lo, hi = 0, sec.size
            else:
                lo = s.value
                hi = next((v for v in starts if v > lo), sec.size)
            rel = tuple((a - lo, norm(c.syms[i].name) if c.syms[i].name else "?", t)
                        for a, i, t in sec.relocs if lo <= a < hi)
            key = norm(s.name)
            if s.cls != 2:
                key = "%s@%s" % (key, sec.name)
            while key in out:
                key += "'"
            out[key] = (sec.data[lo:hi], rel)
    return out


def describe(a, b):
    if a[0] != b[0]:
        if len(a[0]) != len(b[0]):
            return "code/data size %d -> %d" % (len(a[0]), len(b[0]))
        k = next(i for i in range(len(a[0])) if a[0][i] != b[0][i])
        return "bytes differ from +%#x" % k
    ra, rb = set(a[1]), set(b[1])
    d = sorted(ra ^ rb)[:2]
    return "relocations differ (%s)" % ", ".join("+%#x %s" % (o, n) for o, n, _t in d)


# ---------------------------------------------------------------- main

def changed_tus(rev):
    files = [os.path.join(ROOT, f) for f in git("diff", "--name-only", rev, "--", "src", "match/include").splitlines()]
    files = [f for f in files if os.path.exists(f)]
    return includers(files)


def cmd_cmp(a):
    rename = dict(kv.split("=", 1) for kv in a.rename)
    rename_re = [kv.split("=>", 1) for kv in a.rename_re]
    tus = [os.path.abspath(t) for t in a.tus]
    if a.changed:
        tus = sorted(set(tus) | set(changed_tus(a.base)))
    if not tus:
        print("no TUs")
        return 0
    tree = os.path.abspath(a.base_tree)
    sha = base_tree(tree, a.base)
    print("base %s (%s), %d TU(s)" % (sha[:10], tree, len(tus)))
    bad = 0
    for src in tus:
        rel = os.path.relpath(src, ROOT)
        for fl in flag_sets(src):
            fs = " ".join(fl)
            bsrc = os.path.join(tree, rel)
            if not os.path.exists(bsrc):
                print("NEW      %s [%s]: no base version" % (rel, fs))
                continue
            ob, lb = compile_in(tree, bsrc, fl)
            on, ln = compile_in(ROOT, src, fl)
            if not ob or not on:
                bad += 1
                print("COMPILE FAIL %s [%s] (%s)\n%s" % (rel, fs, "base" if not ob else "tree", (lb if not ob else ln)[-1500:]))
                continue
            old, new = symbols(ob, rename, rename_re), symbols(on)
            names = sorted(set(old) | set(new))
            diff = []
            for n in names:
                if n not in new:
                    diff.append((n, "only in base"))
                elif n not in old:
                    diff.append((n, "only in tree"))
                elif old[n] != new[n]:
                    diff.append((n, describe(old[n], new[n])))
                elif a.verbose:
                    print("  identical %s" % n)
            print("%s %s [%s]: %d symbols, %d differ" % ("SAME    " if not diff else "DIFFERS ", rel, fs, len(names), len(diff)))
            for n, why in diff:
                print("  differs  %s: %s" % (n, why))
            bad += bool(diff)
    return 1 if bad else 0


def main():
    sys.stdout.reconfigure(line_buffering=True)          # progress shows in a log file while a long run compiles
    ap =argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("cmp")
    c.add_argument("tus", nargs="*")
    c.add_argument("--base", default="HEAD")
    c.add_argument("--base-tree", default=os.path.join(ROOT, "work", "objcmp_base"))
    c.add_argument("--changed", action="store_true", help="add every TU that sees a src/ file changed since --base")
    c.add_argument("--rename", action="append", default=[], metavar="OLD=NEW")
    c.add_argument("--rename-re", action="append", default=[], metavar="PAT=>REP")
    c.add_argument("-v", "--verbose", action="store_true", help="also list identical symbols")
    i = sub.add_parser("includers")
    i.add_argument("headers", nargs="+")
    a = ap.parse_args()
    if a.cmd == "includers":
        for f in includers(a.headers):
            print(os.path.relpath(f, ROOT))
        return 0
    return cmd_cmp(a)


if __name__ == "__main__":
    sys.exit(main())
