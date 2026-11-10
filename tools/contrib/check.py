#!/usr/bin/env python3
"""check: does one function compile to the original's bytes? Ported from spore-decomp.

  .venv/bin/python tools/contrib/check.py 0x004e3eb0 [--dry-run] [--no-diff]

Compiles the function's source file (the laptop compile server first, see tools/matching/cl.sh) and compares the
function with the original (tools/matching/chk.py):
  MATCH      records it: adds the function's .manifest.txt line and removes its .nonmatching.txt line.
  closer     the aligned figure (bytes of the instructions that still differ) went down: records the new figure in
             the .nonmatching.txt row. Closer is progress.
  further    the figure went up: says so, records nothing (undo the edit or keep working).
Without a match it prints the instruction diff (original | yours, tools/matching/insn_align.py --show).

Every run also re-checks the other byte-exact functions of the same source file (run_all.py --manifest): an edit that
breaks one of them is reported as BROKE and nothing is recorded. --dry-run prints the bookkeeping instead of doing it.
Exit 0 on MATCH, 1 otherwise.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import task as T   # noqa: E402

PY = sys.executable


def run(args):
    return subprocess.run([PY] + args, cwd=ROOT, capture_output=True, text=True, errors="replace")


def module_ok(src):
    mf = src[:-4] + ".manifest.txt"
    if not os.path.exists(mf):
        return True, "no manifest"
    r = run([os.path.join(ROOT, "tools", "matching", "run_all.py"), "--manifest", mf])
    out = (r.stdout + r.stderr).strip().splitlines()
    m = re.search(r"(\d+) / (\d+) functions byte-exact", "\n".join(out))
    if not m:
        return False, "\n".join(out[-15:])
    if m.group(1) != m.group(2):
        bad = [l for l in out if "MATCH" not in l and re.match(r"\s*\S+\s+[0-9a-f]{8}", l)]
        return False, "%s\n%s" % (m.group(0), "\n".join(bad[:10]))
    return True, m.group(0)


def set_row(nm, va, new_line=None):
    """Replace (or with new_line None, delete) the row of va in a .nonmatching.txt."""
    lines = open(nm, encoding="utf-8", errors="replace").read().split("\n")
    out = []
    for l in lines:
        p = l.split(None, 1)
        if p and re.fullmatch(r"[0-9a-fA-F]{8}", p[0]) and int(p[0], 16) == va:
            if new_line is not None:
                out.append(new_line(l))
            continue
        out.append(l)
    open(nm, "w", encoding="utf-8").write("\n".join(out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("va")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-diff", action="store_true")
    a = ap.parse_args()
    va = int(a.va, 16)
    hit = [r for r in T.rows() if r[0] == va]
    if not hit:
        mf, mrow = T.manifest_row(va)
        print("0x%08x is byte-exact already (%s)" % (va, os.path.relpath(mf, ROOT)) if mrow else
              "no .nonmatching.txt lists 0x%08x" % va)
        return 1
    _va, old, note, src, nm = hit[0]
    r = run([os.path.join(ROOT, "tools", "matching", "chk.py"), src, "%x" % va])
    out = r.stdout + r.stderr
    if "COMPILE FAIL" in out:
        print("COMPILE FAILED (%s)\n%s" % (os.path.relpath(src, ROOT), out[-3000:]))
        return 1
    line = next((l for l in out.splitlines() if l.startswith("%08x" % va)), "")
    match = line.rstrip().endswith("MATCH")
    m = re.search(r"aligned (\d+)", line)
    new = 0 if match else (int(m.group(1)) if m else None)
    ok, why = module_ok(src)
    if not ok:
        print("BROKE  your edit makes other functions of %s stop matching; nothing recorded. Undo that part:\n%s"
              % (os.path.relpath(src, ROOT), why))
        return 1
    if match:
        rowl = [l for l in out.splitlines() if re.match(r"\S+\s+\S+\s+%08x\s" % va, l)]
        mf = src[:-4] + ".manifest.txt"
        print("MATCH  0x%08x  %s" % (va, line.split()[-2] if len(line.split()) > 2 else ""))
        if not rowl:
            print("  (chk.py printed no manifest row: the manifest may already name it)")
            return 0
        if a.dry_run:
            print("would add to %s:\n  %s\nand remove its line from %s" % (os.path.relpath(mf, ROOT), rowl[0],
                                                                         os.path.relpath(nm, ROOT)))
            return 0
        t = open(mf).read() if os.path.exists(mf) else ""
        open(mf, "a").write(("" if not t or t.endswith("\n") else "\n") + rowl[0] + "\n")
        set_row(nm, va, None)
        print("recorded: added to %s, removed from %s" % (os.path.relpath(mf, ROOT), os.path.relpath(nm, ROOT)))
        return 0
    if new is None:
        print("NO MATCH  0x%08x: chk.py gave no figure:\n%s" % (va, out[-1500:]))
        return 1
    verdict = "closer" if new < old else "further" if new > old else "same"
    print("NO MATCH  0x%08x  aligned %d (recorded %d): %s" % (va, new, old, verdict))
    if verdict == "closer" and not a.dry_run:
        set_row(nm, va, lambda l: re.sub(r"^(\S+)([ \t]+)\d+", lambda mm: "%s%s%d" % (mm.group(1), mm.group(2), new), l, 1))
        print("recorded the new figure in %s" % os.path.relpath(nm, ROOT))
    elif verdict == "further":
        print("further from a match than before: undo the change or keep going; nothing recorded")
    if not a.no_diff:
        d = run([os.path.join(ROOT, "tools", "matching", "insn_align.py"), src, "%x" % va, "--show"])
        print("\ninstruction diff (orig | ours), relocations masked:\n" + "\n".join((d.stdout or d.stderr).splitlines()[:80]))
    return 1


if __name__ == "__main__":
    sys.exit(main())
