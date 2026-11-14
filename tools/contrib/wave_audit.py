#!/usr/bin/env python3
"""wave_audit: check what a cheap-model wave really left in src/ before anything is gated or committed.

  .venv/bin/python tools/contrib/wave_audit.py nm2a [nm2b ...] [--allow-vas FILE]

For the batches' modules (work/batches/<batch>.json), against HEAD:
  WIPED      a .cpp that lost more than half its lines (a session's `write` once replaced terrain_render.cpp with a
             placeholder); restore it from git or work/opencode/src_backup/
  SCOPE      a diff hunk inside a function that is neither one of the batch's rows nor in --allow-vas (another
             session's or my own patches), or above the first function (shared helpers, includes)
  FIGURE     a row whose recorded aligned figure differs from what it compiles to now (check.py records "closer"
             mid-attempt and nothing resets it when the session puts the code back)
  NOTE       a .nonmatching.txt line that is not a row (a note written on its own line)
  LOST       a row of the module's .nonmatching.txt at HEAD that is now in no .nonmatching.txt and no manifest (a
             session folded it into another row's note)
  ORIGDATA   an identifier the edits newly use that is declared extern with an original address (`// 0x...`) and
             defined nowhere in src/: the standalone relink refuses it (commit_frozen.sh: ORIGINAL DATA REFERENCED);
             wave-4 spelled the constant 0.0005f at 0x590d7c as `g_scroll`. Textual, so check each by hand
Then the wave's own report lines (MATCH / closer / no) per session, and its cost.
Exit 1 if anything was flagged.
"""
import argparse, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = sys.executable


def git(*a):
    return subprocess.run(["git"] + list(a), cwd=ROOT, capture_output=True, text=True, errors="replace").stdout


def rows_of(text):
    out = {}
    for l in text.split("\n"):
        p = l.split(None, 2)
        if len(p) >= 2 and re.fullmatch(r"[0-9a-fA-F]{8}", p[0]) and p[1].isdigit():
            out[p[0].lower()] = int(p[1])
    return out


def measure(src, va):
    out = subprocess.run([PY, os.path.join(ROOT, "tools", "matching", "chk.py"), src, va], cwd=ROOT,
                         capture_output=True, text=True, errors="replace").stdout
    line = next((l for l in out.splitlines() if l.startswith(va)), "")
    if line.rstrip().endswith("MATCH"):
        return 0
    m = re.search(r"aligned (\d+)", line)
    return int(m.group(1)) if m else None


def all_rows():
    """Every address listed by any .nonmatching.txt or .manifest.txt in src/ now."""
    out = set()
    for d, _, fs in os.walk(os.path.join(ROOT, "src")):
        for f in fs:
            if f.endswith(".nonmatching.txt"):
                out.update(rows_of(open(os.path.join(d, f), errors="replace").read()))
            elif f.endswith(".manifest.txt"):
                for l in open(os.path.join(d, f), errors="replace"):
                    p = l.split("#", 1)[0].split()
                    if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                        out.add(p[2].lower())
    return out


def orig_data(cpp):
    """Identifiers on the edit's added lines, absent from HEAD's file, declared `extern` with an original address and
    defined nowhere in src/ (a definition: the name declared at line start without extern)."""
    head = git("show", "HEAD:" + cpp)
    added = "\n".join(l[1:] for l in git("diff", "-U0", "--", cpp).split("\n") if l.startswith("+") and not l.startswith("+++"))
    new = {w for w in re.findall(r"\b[A-Za-z_]\w*\b", added) if not re.search(r"\b%s\b" % w, head)}
    out = []
    for w in sorted(new):
        decl = subprocess.run(["rg", "-n", "--glob", "*.{h,cpp}", r"^\s*extern\b[^;(]*\b%s\b[^;(]*;.*//\s*0x[0-9a-fA-F]+" % w,
                               "src"], cwd=ROOT, capture_output=True, text=True).stdout
        if not decl:
            continue
        defn = subprocess.run(["rg", "-n", "--glob", "*.cpp", r"^(?!\s*extern\b)[\w:<>,\s\*&]*\b%s\b\s*(=|;|\[|\()" % w, "src",
                               "--pcre2"], cwd=ROOT, capture_output=True, text=True).stdout
        if not any("(" not in l.split(w, 1)[1][:2] for l in defn.splitlines()):
            out.append((w, decl.splitlines()[0][:160]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("batches", nargs="+")
    ap.add_argument("--allow-vas", help="file of VAs (one per line) whose edits are expected (e.g. vsearch patches)")
    a = ap.parse_args()
    allow = set()
    if a.allow_vas:
        allow = {int(v, 16) for v in open(a.allow_vas).read().split()}
    mods, waves = {}, {}
    for b in a.batches:
        for x in json.load(open(os.path.join(ROOT, "work", "batches", b + ".json"))):
            mods.setdefault(x["module"], set()).update(int(v, 16) for v in x["vas"])
        wf = os.path.join(ROOT, "work", "opencode", "wave_%s.json" % b)
        if os.path.exists(wf):
            waves[b] = json.load(open(wf))
    flagged = 0
    listed = all_rows()
    for mod, vas in sorted(mods.items()):
        cpp, nm = mod + ".cpp", mod + ".nonmatching.txt"
        head = git("show", "HEAD:" + cpp)
        cur = open(os.path.join(ROOT, cpp), errors="replace").read() if os.path.exists(os.path.join(ROOT, cpp)) else ""
        if head and len(cur.split("\n")) < len(head.split("\n")) / 2:
            print("WIPED   %s: %d lines, %d at HEAD" % (cpp, len(cur.split("\n")), len(head.split("\n"))))
            flagged += 1
            continue
        lines = cur.split("\n")
        marks = [(i + 1, int(m.group(1), 16)) for i, l in enumerate(lines)
                 for m in [re.search(r"//.*@\s*0x0*([0-9a-fA-F]+)\b", l)] if m]
        bad = set()
        for m in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))?", git("diff", "-U0", "--", cpp), re.M):
            s, n = int(m.group(1)), int(m.group(2) or 1)
            for ln in range(s, s + max(n, 1)):
                prev = [va for l, va in marks if l <= ln]
                va = prev[-1] if prev else None
                if va is None or (va not in vas and va not in allow):
                    bad.add(va)
        for w, decl in orig_data(cpp):
            print("ORIGDATA %s: the edit uses %s, declared at an original address and defined nowhere (%s)" % (cpp, w, decl))
            flagged += 1
        if bad:
            print("SCOPE   %s: edits in %s" % (cpp, ", ".join("0x%x" % v if v else "file top" for v in sorted(bad, key=lambda v: v or 0))))
            flagged += 1
        if os.path.exists(os.path.join(ROOT, nm)):
            ntext = open(os.path.join(ROOT, nm), errors="replace").read()
            for l in ntext.split("\n"):
                if l.strip() and not re.match(r"[0-9a-fA-F]{8}\s", l) and not l.startswith("#"):
                    print("NOTE    %s: line without a row address: %s" % (nm, l[:80]))
                    flagged += 1
            old, new = rows_of(git("show", "HEAD:" + nm)), rows_of(ntext)
            for va in sorted(set(old) - listed):
                print("LOST    %s 0x%s: listed at HEAD, now in no .nonmatching.txt and no manifest; restore its line" % (nm, va))
                flagged += 1
            for va, fig in sorted(new.items()):
                if old.get(va) != fig:
                    real = measure(cpp, va)
                    if real != fig:
                        print("FIGURE  %s 0x%s: recorded %s, compiles to %s (HEAD %s)" % (nm, va, fig, real, old.get(va)))
                        flagged += 1
    for b, w in waves.items():
        cost = sum(s.get("usage", {}).get("cost", 0) for s in w.values())
        res = [l for s in w.values() for l in (s.get("report") or "").splitlines() if re.match(r"[0-9a-f]{8} ", l)]
        kinds = {}
        for l in res:
            k = l.split()[1].rstrip(":")
            kinds[k] = kinds.get(k, 0) + 1
        states = {}
        for s in w.values():
            states[s.get("state")] = states.get(s.get("state"), 0) + 1
        print("wave %s: %s, sessions %s, $%.2f" % (b, kinds, states, cost))
    print("%d flagged" % flagged)
    return 1 if flagged else 0


if __name__ == "__main__":
    sys.exit(main())
