#!/usr/bin/env python3
"""regress: did this row ever match, and which commit broke it? (the history probe)

  .venv/bin/python tools/contrib/regress.py 0x004d7120 [--depth 25] [--all]

Most rows that the subagent waves of 2026-11-24 fixed had been byte-exact before a cleanup lane rewrote them. This
tool finds that commit. It takes the row's source file from `task.py`, walks the last --depth commits that touched
that file (newest first), checks out each version of the file into work/match/regress/ (the working tree is never
touched), measures the function with chk.py, and stops at the first MATCH (--all keeps going and prints every
version's score). It then prints the commit, its message, the lane named in the row's sidecar note, and the diff of
the function's source between the matching version and the current one, which is the edit to try undoing.

The measured copy is placed next to the original file's directory under work/match/regress/ with the same relative
#includes, so the commit's own headers are NOT used: only the .cpp text of the old version is swapped in. A version
that no longer compiles against today's headers scores "compile fail" and is skipped (the lane that broke the row
usually changed a header too; read that commit's diff by hand in that case).
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = os.path.join(ROOT, ".venv", "bin", "python")


def sh(args, **kw):
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True, **kw)


def source_of(va):
    """The row's source file: task.py's `source` line, else the manifest row (a row that already matches has none)."""
    sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
    import sidecars as SC
    for mf in SC.all_manifests():
        for l in open(mf, encoding="utf-8", errors="replace"):
            p = l.split("#", 1)[0].split()
            if len(p) >= 3:
                try:
                    if int(p[2], 16) == va:
                        return os.path.normpath(os.path.join(ROOT, "match", p[0])).replace(ROOT + os.sep, ""), ""
                except ValueError:
                    pass
    t = sh([PY, "tools/contrib/task.py", "0x%08x" % va]).stdout
    m = re.search(r"^source\s+(\S+\.cpp)", t, re.M)
    return (m.group(1) if m else None), t


def sidecar_note(va):
    sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
    import sidecars as SC
    for nm in SC.all_nonmatching():
        for l in open(nm, encoding="utf-8", errors="replace"):
            if l.startswith("%08x" % va):
                return l.rstrip("\n")
    return ""


def score(path, va):
    env = dict(os.environ, MB_RC_ALLOW_LOCAL="1")
    r = subprocess.run([PY, "tools/matching/chk.py", path, "0x%x" % va], cwd=ROOT, capture_output=True, text=True, env=env)
    out = r.stdout + r.stderr
    m = re.search(r"^%08x\s+\d+\s+(\d+) diff(?:\s+aligned (\d+))?" % va, out, re.M)
    if re.search(r"^%08x .* MATCH\s*$" % va, out, re.M):
        return "MATCH", 0
    if m:
        return "diff", int(m.group(2) or m.group(1))
    return "compile fail", None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("va")
    ap.add_argument("--depth", type=int, default=25)
    ap.add_argument("--all", action="store_true")
    a = ap.parse_args()
    va = int(a.va, 16)
    src, task = source_of(va)
    if not src or not src.endswith(".cpp"):
        sys.exit("regress: task.py gave no source file for 0x%08x" % va)
    if not os.path.isfile(os.path.join(ROOT, src)):
        sys.exit("regress: %s not found" % src)
    print("row 0x%08x  file %s" % (va, src))
    note = sidecar_note(va)
    if note:
        print("sidecar: " + note[:300])
    log = sh(["git", "log", "--format=%H %h %ad %s", "--date=short", "-n", str(a.depth), "--", src]).stdout.splitlines()
    if not log:
        sys.exit("regress: no history for " + src)
    work = os.path.join(ROOT, "work", "match", "regress")
    os.makedirs(work, exist_ok=True)
    cur = open(os.path.join(ROOT, src), encoding="utf-8", errors="replace").read()
    hit = None
    print("%-9s %-11s %s" % ("commit", "score", "subject"))
    for ln in log:
        full, short, date, subj = ln.split(" ", 3)
        old = sh(["git", "show", "%s:%s" % (full, src)]).stdout
        if not old:
            continue
        if old == cur:
            verdict, n = score(src, va)
            print("%-9s %-11s %s (= working tree)" % (short, "%s %s" % (verdict, "" if n is None else n), subj[:70]))
            if verdict == "MATCH":
                print("already MATCH in the current text: nothing to undo"); return
            continue
        dst = os.path.join(os.path.dirname(os.path.join(ROOT, src)), ".regress_%s.cpp" % short)
        open(dst, "w", encoding="utf-8").write(old)
        try:
            verdict, n = score(os.path.relpath(dst, ROOT), va)
        finally:
            os.remove(dst)
        print("%-9s %-11s %s %s" % (short, "%s %s" % (verdict, "" if n is None else n), date, subj[:70]))
        if verdict == "MATCH" and not hit:
            hit = (full, short, subj, old)
            if not a.all:
                break
    if not hit:
        print("no MATCH in the last %d versions of %s" % (len(log), src)); return
    full, short, subj, old = hit
    print("\nfirst MATCH at %s: %s" % (short, subj))
    nxt = sh(["git", "log", "--format=%h %ad %s", "--date=short", "--ancestry-path", "%s..HEAD" % full, "--", src]).stdout.splitlines()
    if nxt:
        print("the next commit to touch the file (the likely culprit): " + nxt[-1])
    # the function's text: from the "// @ 0x<va>" marker (or the symbol's first line) to the next top-level "// @" marker
    def region(text):
        m = re.search(r"^//\s*@\s*0x%x\b" % va, text, re.M | re.I)
        if not m:
            return None
        n = re.search(r"^//\s*@\s*0x[0-9a-f]+", text[m.end():], re.M | re.I)
        return text[m.start(): m.end() + n.start()] if n else text[m.start():]
    ro, rc = region(old), region(cur)
    import difflib
    if ro and rc:
        d = list(difflib.unified_diff(ro.splitlines(), rc.splitlines(), "matching version", "current", lineterm="", n=2))
        print("function text, matching -> current (undo these lines):")
        print("\n".join(d) if d else "(function text identical: the difference is elsewhere in the file, e.g. includes/specializations)")
    d2 = list(difflib.unified_diff(old.splitlines(), cur.splitlines(), "matching version", "current", lineterm="", n=0))
    print("\nwhole-file diff: %d changed lines (git diff %s -- %s to see it)" % (
        sum(1 for x in d2 if x[:1] in "+-" and x[:3] not in ("+++", "---")), short, src))


if __name__ == "__main__":
    main()
