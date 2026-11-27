#!/usr/bin/env python3
"""Project status: game functions per fidelity tier, by count and by code size. Writes docs/STATUS.md.

Denominator: Ghidra functions in .text below the import json, thunks (0x5342a6), minus the CRT/C++ library
functions identified byte-exact in symbols/lib_names.txt (linked from the real libcmt/libcpmt, not
decompiled). Plus the game's dynamic initializers and the atexit destructors they register (0x5479a0..0x54856e,
after the CRT: the constructors of the game's file-scope globals; src/game/static_init/). The EH handlers
and unwind funclets are compiler-generated with their parent functions and are not counted separately.

Tiers (from match/manifest.txt, match/slices/*/{manifest,nonmatching,partial}.txt and the migrated modules'
match/manifests/**/<module>.{manifest,nonmatching,partial}.txt):
  byte-exact  - listed in a manifest (run_all.py verifies them; results in work/match/... json)
  equivalent  - complete source, not byte-exact (nonmatching.txt)
  partial     - incomplete source (partial.txt), not counted as decompiled
Migrated: byte-exact or equivalent functions whose source now lives in a src/ module (one .cpp per original
translation unit) instead of a match/slices staging slice. A subset of the compilable total.
usage: status.py [--no-write]
"""
import csv, glob, json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import sidecars as SC  # noqa: E402  (where the sidecar files live)
W = lambda *p: os.path.join(ROOT, *p)
GAME_END = 0x5342A6
INIT_LO, INIT_HI = 0x5479A0, 0x54856E    # the game's dynamic initializers and their atexit destructors

not_func = set()
for line in open(W("symbols/not_functions.txt")):
    p = line.split("#", 1)[0].split()
    if p:
        not_func.add(int(p[0], 16))
starts = sorted(a for a in (int(r["address"], 16) for r in csv.DictReader(open(W("work/index/functions.csv")))
                            if r["block"] == ".text") if a not in not_func)
size = {a: (starts[i + 1] if i + 1 < len(starts) else a + 16) - a for i, a in enumerate(starts)}
lib = set()
for line in open(W("symbols/lib_names.txt")):
    p = line.split("#", 1)[0].split()
    if p:
        try:
            lib.add(int(p[0], 16))
        except ValueError:
            pass
game = {a for a in starts if (a < GAME_END or INIT_LO <= a < INIT_HI) and a not in lib}


def rows(pattern, col):
    """the rows of the files matching a glob under the repository root, or of a list of files (SC.all_*())"""
    out = set()
    for f in (pattern if isinstance(pattern, list) else glob.glob(W(pattern), recursive=True)):
        for line in open(f):
            p = line.split("#", 1)[0].split()
            if len(p) > col:
                try:
                    out.add(int(p[col], 16))
                except ValueError:
                    pass
    return out


src_exact = rows(SC.all_manifests(), 2)
src_equiv = rows(SC.all_nonmatching(), 0) - src_exact
# A src module beats its old slice (as in sweep_nonmatching.rows): a function the module lists as nonmatching is not
# byte-exact in the build we ship, even when the old slice's manifest still lists the slice source as exact (69 rows
# were counted exact that way until 2026-11-26).
exact = ((rows("match/manifest.txt", 2) | rows("match/slices/*/manifest.txt", 2)) - src_equiv) | src_exact
equiv = (rows("match/slices/*/nonmatching.txt", 0) | src_equiv) - exact
partial = (rows("match/slices/*/partial.txt", 0) | rows(SC.all_partials(), 0)) - exact - equiv
migrated = src_exact | src_equiv
G, B = len(game), sum(size[a] for a in game)
pct = lambda n, d: "%.2f%%" % (100.0 * n / d) if d else "-"


def line(label, s):
    s = s & game
    b = sum(size[a] for a in s)
    return "| %s | %d | %s | %.3f MB | %s |" % (label, len(s), pct(len(s), G), b / 1e6, pct(b, B))


done = exact | equiv
# The Equivalent tier split by its equivalence evidence (docs/equiv_verdicts.json, tools/difftest/verdicts.py; user 2026-11-27):
# "equivalent" only means complete source that is not byte-exact, so show how much of it is proven, tested or neither
EVID = [("proven", ("proven",), "static proof (tools/equiv/prove.py)"),
        ("proven_o", ("proven_o",), "static proof under the frame model (PROVEN-O)"),
        ("pass", ("pass", "comdat"), "tested: no mismatch, every branch run (PASS)"),
        ("weak", ("weak",), "tested: no mismatch, some branches never run (WEAK)"),
        ("fail", ("fail",), "FAIL: a mismatch on some input"),
        ("unsupported", ("inputs", "harness", "funclet", "comdatdiff", "notcode"), "not testable yet (UNSUPPORTED, funclet)"),
        ("none", ("not_run",), "no verdict")]
try:
    _vr = json.load(open(W("docs/equiv_verdicts.json"))).get("rows", {})
except (OSError, ValueError):
    _vr = {}
_cat = {int(k, 16): v.get("category", "not_run") for k, v in _vr.items()}
evid_lines = []
_seen = set()
for key, cats, label in EVID:
    s = {a for a in equiv if _cat.get(a, "not_run") in cats}
    _seen |= s
    evid_lines.append(line("&nbsp;&nbsp;&nbsp;&nbsp;↳ " + label, s))
assert not (equiv - _seen), "an equivalent row has an unknown verdict category"
out = ["# Status", "", "Generated by `tools/status.py`. Denominator: %d game functions (%.2f MB); %d CRT/C++ library "
       "functions excluded (linked from the real VC7.1 libraries)." % (G, B / 1e6, len(lib & set(starts))), "",
       "| Tier | Functions | Share | Code size | Share (size) |", "|---|---:|---:|---:|---:|",
       line("Byte-exact", exact), line("Equivalent, not exact", equiv)] + evid_lines + [line("Partial (not counted)", partial),
       line("**Total with compilable source**", done), line("No source yet", game - done - partial), "",
       "| Source tree | Functions | Share | Code size | Share (size) |", "|---|---:|---:|---:|---:|",
       line("Migrated into src/", migrated), line("Compilable, still only in match/slices", done - migrated)]
print("\n".join(out))
if "--no-write" not in sys.argv:
    open(W("docs/STATUS.md"), "w").write("\n".join(out) + "\n")
