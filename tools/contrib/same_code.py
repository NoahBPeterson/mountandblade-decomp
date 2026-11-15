#!/usr/bin/env python3
"""same_code: does a source-only change (a rename, a real type for a view, a cleaner spelling) compile to the same code?

  .venv/bin/python tools/contrib/same_code.py [--base REV] [--jobs N] [FILE.cpp ...]

Compiles every given .cpp twice, at REV (default HEAD, in a temporary git worktree) and in the working tree, with the
flags check.py uses, and compares every function (each COMDAT section starting a decorated symbol: its bytes and its
relocations' offsets and types). With no files: the .cpp files changed against REV, plus every src/ .cpp that
includes a changed header, directly or through other headers (matched by file name, so a same-named header elsewhere
adds extra files, never fewer). --list prints that file list and stops. Prints one line per file and the functions that differ or exist
on one side only; exit status 0 only when everything is identical.

The gate for idiomatization work: a rename or a real type that cl lays out the same way must change nothing. (It
does not prove a renamed field means the same field: a local that shadows the new member name compiles to different
code, which this catches, but two members swapped consistently would not. Read the diff too.)
"""
import argparse, concurrent.futures as cf, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path[:0] = [os.path.join(ROOT, "tools", "matching"), os.path.join(ROOT, "tools", "contrib"),
                os.path.join(ROOT, "tools", "equiv")]
import run_all as R       # noqa: E402
import func as F          # noqa: E402
import task as T          # noqa: E402


def git(*a):
    return subprocess.run(["git", "-C", ROOT] + list(a), capture_output=True, text=True, check=True).stdout


def changed_files(base):
    names = set(git("diff", "--name-only", base, "--", "src").split()) | \
        set(git("ls-files", "--others", "--exclude-standard", "src").split())
    cpps = {n for n in names if n.endswith(".cpp")}
    hdrs = {os.path.basename(n) for n in names if n.endswith((".h", ".hpp", ".inl"))}
    seen = set()
    while hdrs - seen:                                   # transitive: headers that include a changed header, ...
        new = hdrs - seen
        seen |= new
        pat = r'#\s*include\s*[<"]([^>"]*/)?(%s)[>"]' % "|".join(re.escape(h) for h in sorted(new))
        out = subprocess.run(["rg", "-l", "-g", "*.cpp", "-g", "*.h", "-g", "*.hpp", "-g", "*.inl", pat, "src"],
                             cwd=ROOT, capture_output=True, text=True).stdout.split()
        cpps |= {o for o in out if o.endswith(".cpp")}
        hdrs |= {os.path.basename(o) for o in out if not o.endswith(".cpp")}
    return sorted(c for c in cpps if os.path.exists(os.path.join(ROOT, c)))


def funcs(root, rel, tmpdir):
    src = os.path.join(root, rel)
    if not os.path.exists(src):
        return None
    fl = list(T.flags_for(src))       # (src/game/include does not exist; an absolute /I to it sent compiles to local wine)
    obj, _ = F.compile_src(src, fl)
    if obj is None:
        raise RuntimeError("compile failed: %s" % src)
    if isinstance(obj, (bytes, bytearray)):
        p = os.path.join(tmpdir, re.sub(r"\W", "_", root + rel) + ".obj")
        open(p, "wb").write(obj)
        obj = p
    secs, syms = R.parse_coff(obj)
    return {n: (bytes(secs[si]["data"]), tuple(sorted((v, t) for v, _s, t in secs[si]["relocs"])))
            for n, (si, off) in syms.items() if off == 0 and n.startswith("?") and secs[si]["data"]}


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("files", nargs="*")
    ap.add_argument("--base", default="HEAD")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    files = [os.path.relpath(os.path.abspath(f), ROOT) for f in a.files] or changed_files(a.base)
    if a.list:
        print("\n".join(files))
        return 0
    if not files:
        print("same_code: nothing changed against %s" % a.base)
        return 0
    tmp = tempfile.mkdtemp(prefix="same_code_")
    wt = os.path.join(tmp, "base")
    git("worktree", "add", "-q", "--detach", wt, a.base)
    bad = 0
    try:
        def one(rel):
            try:
                return rel, funcs(wt, rel, tmp), funcs(ROOT, rel, tmp), None
            except BaseException as e:                   # a compile failure on either side (compile_src exits) is a failure
                return rel, None, None, str(e)
        with cf.ThreadPoolExecutor(a.jobs) as ex:
            for rel, old, new, err in ex.map(one, files):
                if err:
                    print("%-60s ERROR %s" % (rel, err)); bad += 1; continue
                if old is None:
                    print("%-60s new file (no base version)" % rel); continue
                diff = sorted(n for n in old if n in new and old[n] != new[n])
                only = sorted(set(old) ^ set(new))
                ok = not diff and not only
                bad += not ok
                print("%-60s %4d functions  %s" % (rel, len(old), "identical" if ok else
                                                   "%d differ, %d on one side only" % (len(diff), len(only))))
                for n in diff[:20]:
                    print("    differs: %s" % n)
                for n in only[:20]:
                    print("    only %s: %s" % ("base" if n in old else "ours", n))
    finally:
        git("worktree", "remove", "--force", wt)
        shutil.rmtree(tmp, ignore_errors=True)
    print("same_code: %s (%d files)" % ("IDENTICAL" if not bad else "%d files differ" % bad, len(files)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
