#!/usr/bin/env python3
"""Recompile every manifested source and verify each listed function byte-for-byte.

usage: run_all.py [--filter substr] [-v] [--manifest path] [--jobs N] [--no-reloc-gate]

Manifests: match/manifest.txt, match/slices/*/manifest.txt, match/clones.txt, match/synth/*.manifest, match/manifests/**/*.manifest.txt.
Each distinct (source, flags) pair is compiled once, in parallel, into its own object file; every
function is then compared in-process (relocations masked on both sides). Only one run_all may run
at a time (work/run_all.lock). Results are written to work/verify_results.json.
With --manifest, a run uses its own object directory, lock and results file (work/match/m/<key>/,
work/run_all_m/<key>.lock, work/run_all_m/<key>.json, key = hash of the manifest path), so runs on different
manifests never wait for each other (fix pass 6 F26: mig7 agents waited 20-40 minutes on the shared lock).
Reloc gate (default on for linked src/ rows): a byte-exact src/ row also fails when one of its relocations names
another target than the original's operand (tools/matching/reloc_targets.py: another global/callee, or the same symbol
at another field offset). match/ and match/slices rows are report-only: their mismatches are printed as RELOC lines
and do not change the row's status. --no-reloc-gate turns the check off (results then go to
<results>_no_reloc_gate.json, not the default results file).
Exit 0 iff every listed function matches.
"""
import argparse, bisect, glob, hashlib, json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pefile
from cmpobj import parse_coff, REL_SIZES, coff_constants, constant_diffs
import imgcache
import asm_audit
from winnative import CL, winpath, lock as lock_file    # cl.sh under wine on the Mac, cl.exe on Windows
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "difftest"))
import slice as S                                       # the shared object cache (compile_obj)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import sidecars as SC          # noqa: E402  (where the sidecar files live)
IMAGE = os.path.join(ROOT, "work", "mb.analysis.bin")
OUT = os.path.join(ROOT, "work", "match")
DEFAULT_FLAGS = ["/O2", "/MT", "/Gy", "/TP"]  # cl 13.10; static CRT; /EHsc only where the original has EH frames
INCLUDE = "/I" + winpath(os.path.join(ROOT, "match", "include"))


def resolve_flags(flags):
    """Make relative /I paths independent of the working directory. Manifests were written with
    paths relative to work/match (e.g. /Iscratch_s00880170_inc, local EAWebKit include links; see
    THIRD_PARTY.md); cl resolved them against cwd, so they only worked when run from there. Try
    work/match first, then the repo root; absolute and Z: paths are left alone."""
    out = []
    for f in flags:
        if f[:2] in ("/I", "-I") and len(f) > 2 and not f[2:].startswith(("Z:", "/", "\\")) and ":" not in f[2:4]:
            rel = f[2:]
            for base in (OUT, ROOT):
                cand = os.path.join(base, rel)
                if os.path.isdir(cand):
                    f = "/I" + winpath(cand)
                    break
        out.append(f)
    return out


OBJ_DIR = OUT        # main() points this at a per-manifest directory for --manifest runs


def obj_path(src, flags):
    return os.path.join(OBJ_DIR, src.replace("/", "_").rsplit(".", 1)[0] + "_" +
                        "".join(f.strip("/") for f in flags).replace(":", "").replace("/", "_") + ".obj")


def manifests(only=None):
    if only:
        return [only]
    m = [os.path.join(ROOT, "match", "manifest.txt")]
    m += sorted(glob.glob(os.path.join(ROOT, "match", "slices", "*", "manifest.txt")))
    if os.path.exists(os.path.join(ROOT, "match", "clones.txt")):
        m.append(os.path.join(ROOT, "match", "clones.txt"))
    m += sorted(glob.glob(os.path.join(ROOT, "match", "synth", "*.manifest")))
    m += sorted(SC.all_manifests())   # the decompiled modules
    return m


def read_rows(paths, filt):
    rows = []
    for m in paths:
        for line in open(m):
            p = line.split("#", 1)[0].split()
            if len(p) >= 3 and filt in " ".join(p):
                rows.append((p[0], p[1], int(p[2], 16), tuple(p[3:]) or tuple(DEFAULT_FLAGS)))
    return rows


def compile_one(key):
    # The difftest object cache (slice.compile_obj: keyed by the source, every header it reaches and the flags, shared
    # by every worktree): an unchanged source costs no compiler run. Without it every run recompiled all ~500 sources.
    src, flags = key
    obj, log = S.compile_obj(os.path.join(ROOT, "match", src), list(flags))
    return key, obj, log[-2000:]


class Original:
    def __init__(self):
        pe = pefile.PE(IMAGE, fast_load=True)
        self.pe, self.base = pe, pe.OPTIONAL_HEADER.ImageBase
        self.relocs = imgcache.relocs(IMAGE)  # sorted HIGHLOW reloc RVAs, cached (was ~1.6 s per run)

    def read(self, va, n):
        return self.pe.get_data(va - self.base, n)

    def bytes_and_mask(self, va, n):
        rva = va - self.base
        data = self.pe.get_data(rva, n)
        mask = set()
        i = bisect.bisect_left(self.relocs, rva - 3)
        while i < len(self.relocs) and self.relocs[i] < rva + n:
            for k in range(4):
                j = self.relocs[i] + k - rva
                if 0 <= j < n:
                    mask.add(j)
            i += 1
        return data, mask


ANON_NS = re.compile(r"\?A0x[0-9a-f]{8}@")   # an anonymous namespace: cl names it by a hash of the file's inputs


def resolve(syms, sym, va=None, func_vas=None):
    if sym.startswith("$L") and func_vas is not None:
        # A compiler label inside a function (the tail after an EH block): its number is counted over the whole
        # translation unit, so any header change renumbers it. Find it by place instead: the label whose enclosing
        # function (the symbol before it in its section) is a manifest row at va - (label offset - function offset).
        by_sec = {}
        for s, (si, off) in syms.items():
            by_sec.setdefault(si, []).append((off, s))
        c = []
        for s, (si, off) in syms.items():
            if not s.startswith("$L"):
                continue
            fn = [(o, n) for o, n in by_sec[si] if o <= off and not n.startswith("$L")]
            if fn:
                foff, fname = max(fn)
                if func_vas.get(fname) == va - (off - foff):
                    c.append(s)
        return c[0] if len(c) == 1 else None
    if sym in syms:
        return sym
    if ANON_NS.search(sym):
        pat = re.compile(ANON_NS.pattern.join(re.escape(p) for p in ANON_NS.split(sym)) + "$")
        c = [s for s in syms if pat.match(s)]
        return c[0] if len(c) == 1 else None
    c = [s for s in syms if sym in s and not s.startswith(("__ehhandler", "__unwindfunclet", "__catch", "__tryend"))]
    return c[0] if len(c) == 1 else None


def compare_row(orig, obj, sym, va, func_vas, coff_cache, const_cache):
    """One manifest row's comparison: -> (status, row) where row = {"name", "si", "off", "n", "mine", "theirs"}
    (None when the symbol is not found). Relocated fields are masked on both sides; constant_diffs compares
    the values of the float/string constants they reference."""
    if obj not in coff_cache:
        secs_, syms_ = parse_coff(obj)
        starts, fstarts = {}, {}
        for n_, (si_, off_) in syms_.items():
            starts.setdefault(si_, set()).add(off_)
            if not n_.startswith("$L"):
                fstarts.setdefault(si_, set()).add(off_)
        coff_cache[obj] = (secs_, syms_, {k: sorted(v) for k, v in starts.items()},
                           {k: sorted(v) for k, v in fstarts.items()})
    secs, syms, starts, fstarts = coff_cache[obj]
    name = resolve(syms, sym, va, func_vas)
    if name is None:
        return "FAIL symbol %s not found/ambiguous" % sym, None
    si, off = syms[name]
    sec = secs[si]
    # a $L row (a catch block or cold tail cl emits after its function) runs over the further $L labels
    # inside it to the next real symbol: the original lists the whole tail as one function
    nxt = [o for o in (fstarts if sym.startswith("$L") else starts).get(si, []) if o > off]
    mine = sec["data"][off:nxt[0] if nxt else len(sec["data"])]
    if nxt:  # strip int3 padding between functions in a shared section
        mine = mine.rstrip(b"\xcc")
    n = len(mine)
    theirs, mask_o = orig.bytes_and_mask(va, n)
    mask = set(mask_o)
    for v, _s, t in sec["relocs"]:
        for k in range(REL_SIZES.get(t, 4)):
            if off <= v + k < off + n:
                mask.add(v + k - off)
    diffs = [i for i in range(n) if i not in mask and theirs[i] != mine[i]]
    if obj not in const_cache:
        const_cache[obj] = coff_constants(obj)
    diffs += constant_diffs(const_cache[obj], sec["relocs"], off, theirs, orig.read, mine)
    status = "MATCH" if not diffs else "DIFF %d bytes" % len(diffs)
    return status, {"name": name, "si": si, "off": off, "n": n, "mine": mine, "theirs": theirs}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter", default=""); ap.add_argument("-v", action="store_true")
    ap.add_argument("--manifest"); ap.add_argument("--jobs", type=int, default=6)
    # Default on: a byte-exact src/ row whose relocations name another target than the original's
    # (tools/matching/reloc_targets.py: a resolvable target that differs, or a field offset that differs) fails;
    # match/ and match/slices rows only report theirs. --reloc-gate is the old opt-in spelling, kept as a no-op.
    ap.add_argument("--no-reloc-gate", dest="reloc_gate", action="store_false")
    ap.add_argument("--reloc-gate", dest="reloc_gate", action="store_true", default=True)
    a = ap.parse_args()
    global OBJ_DIR
    os.makedirs(OUT, exist_ok=True)
    # One run_all at a time per object directory: the lock serialises it and the results file. Wait for it (module
    # manifests take seconds), say so first so a caller can tell a queue from a slow compile.
    lock_path, res_path = os.path.join(ROOT, "work", "run_all.lock"), os.path.join(ROOT, "work", "verify_results.json")
    if a.manifest:
        key = hashlib.sha1(os.path.abspath(a.manifest).encode()).hexdigest()[:16]
        OBJ_DIR = os.path.join(OUT, "m", key)
        os.makedirs(OBJ_DIR, exist_ok=True)
        os.makedirs(os.path.join(ROOT, "work", "run_all_m"), exist_ok=True)
        lock_path = os.path.join(ROOT, "work", "run_all_m", key + ".lock")
        res_path = os.path.join(ROOT, "work", "run_all_m", key + ".json")
    if not a.reloc_gate:    # an ungated run's verdicts must not replace the default results other tools read
        res_path = res_path[:-5] + "_no_reloc_gate.json"
    lock = open(lock_path, "w")
    try:
        lock_file(lock, wait=False)
    except BlockingIOError:
        print("waiting for another run_all.py to finish (%s)" % os.path.relpath(lock_path, ROOT), flush=True)
        lock_file(lock)

    rows = read_rows(manifests(a.manifest), a.filter)
    keys = sorted(set((r[0], r[3]) for r in rows))
    objs = {}
    with ThreadPoolExecutor(a.jobs) as ex:
        for i, (key, obj, log) in enumerate(ex.map(compile_one, keys)):
            objs[key] = obj
            if obj is None:
                print("COMPILE FAIL %s %s\n%s" % (key[0], " ".join(key[1]), log))
            if a.v and i % 25 == 0:
                print("compiled %d/%d" % (i + 1, len(keys)), flush=True)

    orig = Original()
    coff_cache, const_cache = {}, {}
    results, ok, seen_va, ok_va = {}, 0, set(), set()
    reloc_report = []      # match/ and match/slices rows with reloc target mismatches (not gated)
    func_vas = {}       # (source, flags) -> {function symbol: original address}, to place the $L label rows
    for src, sym, va, flags in rows:
        if not sym.startswith("$L"):
            func_vas.setdefault((src, flags), {})[sym] = va
    for src, sym, va, flags in rows:
        obj = objs.get((src, flags))
        status = "FAIL no object"
        if obj:
            status, row = compare_row(orig, obj, sym, va, func_vas.get((src, flags)), coff_cache, const_cache)
            if row and a.reloc_gate and status == "MATCH":
                import reloc_targets as RT
                bad = RT.gate_mismatches(obj, src, va, row)
                if bad:
                    what = ", ".join("+%x %s" % (m["at"], m["class"]) for m in bad[:4])
                    if src.startswith("../src/"):
                        status = "DIFF %d bytes (reloc targets: %s)" % (4 * len(bad), what)
                    else:           # match/ and match/slices rows: report only
                        reloc_report.append("RELOC  %-40s %08x  %d reloc target mismatches (report only): %s"
                                            % (sym[:40], va, len(bad), what))
        # docs/matching.md "No assembly transcription": a naked/_emit/mostly-__asm body is not decompiled, so it
        # never counts as byte-exact (2026-10-18 audit: 18 such manifest rows had been counted).
        asm = asm_audit.transcribed(os.path.join(ROOT, "match", src)).get("%08x" % va)
        if asm:
            status = "ASM transcription (%s), not decompiled: move the row to partial.txt" % asm
        results["%08x" % va] = {"src": src, "sym": sym, "status": status}
        seen_va.add(va)
        if status == "MATCH":
            ok += 1
            ok_va.add(va)
            if a.v:
                print("MATCH  %-40s %08x" % (sym[:40], va))
        else:
            print("%-6s %-40s %08x  %s" % (status.split()[0], sym[:40], va, status))
    for line in reloc_report:
        print(line)
    json.dump(results, open(res_path, "w"))
    # Rows, not functions: a function can be listed by more than one manifest (the old match/slices copy and its
    # migrated src/ module both list it), so the row count over-counts; the distinct-address line is the function count.
    print("\n%d / %d manifest rows byte-exact (%d sources compiled)" % (ok, len(rows), len(keys)))
    print("%d / %d distinct addresses byte-exact (%d rows list an address another row already lists)"
          % (len(ok_va), len(seen_va), len(rows) - len(seen_va)))
    return 0 if ok == len(rows) else 1


if __name__ == "__main__":
    sys.exit(main())
