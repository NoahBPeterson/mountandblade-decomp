#!/usr/bin/env python3
"""sweep_nonmatching.py: re-run equiv on every nonmatching row (match/slices/*/nonmatching.txt and
src/**/*.nonmatching.txt) and tally every mismatch, not just the first (equiv's mismatch_sigs).

usage: sweep_nonmatching.py [--out DIR=work/sweep] [--workers N=6] [--filter substr] [--tally] [--extra "..."]
Each row runs `equiv.py <id> <va> [--src <module.cpp>] --fp-tol --max-mismatch 200 --json <out>/<id>_<va>.json`;
rows whose JSON exists are skipped (restartable). One row per VA: a VA listed both in a src module and in its
old slice runs from the module (and a VA two slices both claim runs from the one whose source/symbol table
records it); every dropped claim is named once on stderr. A slice claim for a VA a src module's manifest binds (the
slice was migrated into the module and left in place) is no row: the slice's source is no longer built. --tally prints verdict counts and writes
<out>/tally.json: every row with its recorded reason, the new verdict and the mismatch signatures."""
import argparse, collections, glob, json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = os.path.join(ROOT, ".venv", "Scripts", "python.exe") if sys.platform == "win32" else os.path.join(ROOT, ".venv", "bin", "python")
EQ = os.path.join(ROOT, "tools", "difftest", "equiv.py")
ROW = re.compile(r"^([0-9a-fA-F]{8})\s+\S+\s+(.*)$")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import slice as SL                                # noqa: E402  (VA->symbol tables, manifests, source markers)
import threading                                  # noqa: E402
import fn_hash                                    # noqa: E402  (per-function fingerprints: skip rows whose code is unchanged)

_VA_COMMENT = re.compile(r"//.*@\s*0x0*([0-9a-fA-F]{3,8})\b")
_TABLES, _MANIFESTS, _COMMENTS = {}, {}, {}


def _table_vas(sid):
    """VAs `symbols/slices/<sid>.txt` names a mangled symbol for (written from the compiled TU)."""
    if sid not in _TABLES:
        out, p = set(), os.path.join(ROOT, "symbols", "slices", sid + ".txt")
        try:
            for line in open(p, encoding="utf-8", errors="replace"):
                q = line.split("#", 1)[0].split(None, 1)
                try:
                    out.add(int(q[0], 16))
                except (IndexError, ValueError):
                    pass
        except OSError:
            pass
        _TABLES[sid] = out
    return _TABLES[sid]


def _manifest_vas(sid):
    """VAs the slice/module manifest binds to a symbol."""
    if sid not in _MANIFESTS:
        _MANIFESTS[sid] = {r[2] for r in SL.manifest_rows(sid)}
    return _MANIFESTS[sid]


def _comment_vas(src):
    """VAs src marks with a '// ... @ 0x<va>' comment: find_symbol's marker binding."""
    if src not in _COMMENTS:
        out = set()
        try:
            out = {int(m[1], 16) for m in _VA_COMMENT.finditer(open(src, encoding="utf-8", errors="replace").read())}
        except OSError:
            pass
        _COMMENTS[src] = out
    return _COMMENTS[src]


def _defines(c, va):
    """2 if the module's own symbol table/manifest binds va, 1 if its source carries the VA marker, else 0."""
    v = int(va, 16)
    if v in _table_vas(c["id"]) or v in _manifest_vas(c["id"]):
        return 2
    src = os.path.join(ROOT, c["src"]) if c["src"] else SL.source_path(c["id"])
    return 1 if v in _comment_vas(src) else 0


def _claims():
    out = []
    for p in sorted(glob.glob(os.path.join(ROOT, "match", "slices", "*", "nonmatching.txt"))):
        sid = os.path.basename(os.path.dirname(p))
        for line in open(p, encoding="utf-8", errors="replace"):
            m = ROW.match(line.strip())
            if m:
                out.append({"id": sid, "va": m[1].lower(), "src": None, "reason": m[2][:2000],
                            "file": os.path.relpath(p, ROOT)})
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True)):
        mod = os.path.basename(p)[:-len(".nonmatching.txt")]
        src = p[:-len(".nonmatching.txt")] + ".cpp"
        for line in open(p, encoding="utf-8", errors="replace"):
            m = ROW.match(line.strip())
            if m:
                out.append({"id": mod, "va": m[1].lower(), "src": os.path.relpath(src, ROOT),
                            "reason": m[2][:2000], "file": os.path.relpath(p, ROOT)})
    return out


def _module_manifest_vas():
    """VA -> (module, module source) for every function a src module's manifest binds (its compiled object)."""
    out = {}
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True)):
        mod = os.path.basename(p)[:-len(".manifest.txt")]
        src = os.path.relpath(p[:-len(".manifest.txt")] + ".cpp", ROOT)
        for line in open(p, encoding="utf-8", errors="replace"):
            q = line.split("#", 1)[0].split()
            if len(q) > 2:
                try:
                    out.setdefault(int(q[2], 16), (mod, src))
                except ValueError:
                    pass
    return out


def rows():
    """One row per VA, from the module whose compiled object defines it: a src module beats its old slice,
    and within a kind the module whose manifest/symbol table/VA marker binds the VA wins; remaining ties go
    to the lowest module name. Dropped duplicate claims are named on stderr."""
    by_va = collections.defaultdict(list)
    live = _module_manifest_vas()
    for c in _claims():
        if not c["src"] and int(c["va"], 16) in live:
            # a slice claim for a function a module now compiles (the slices stay in place after a migration,
            # src/rgl/mesh2/MIGRATION.md) and does not list as nonmatching: the slice's source is dead code and
            # the module's function is exact, so there is no row (verdicts.py counts module rows only)
            continue
        by_va[c["va"]].append(c)
    out, dropped = [], []
    for va, cs in by_va.items():
        # (module before slice, strongest definition evidence, lowest name) -- total and deterministic
        ranked = sorted(cs, key=lambda c: (0 if c["src"] else 1, -_defines(c, va), c["id"], c["file"]))
        out.append(ranked[0])
        dropped += ["%s:%s" % (va, c["id"]) for c in ranked[1:]]
    if dropped:
        dropped.sort()
        print("sweep: kept one row per VA (%d rows); dropped %d duplicate claim(s): %s"
              % (len(out), len(dropped), ", ".join(dropped)), file=sys.stderr)
    return sorted(out, key=lambda r: r["va"])


_ROW_OPTS = None


def row_opts(r):
    """Extra equiv.py arguments for one row from tools/difftest/row_opts.json ("<id> <va>": {"args": [...], "why":
    "..."}): facts about a row the harness cannot infer, such as image functions one build inlines and the other
    calls (--transparent), each with its reason."""
    global _ROW_OPTS
    if _ROW_OPTS is None:
        try:
            _ROW_OPTS = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "row_opts.json")))
        except (OSError, ValueError):
            _ROW_OPTS = {}
    return list((_ROW_OPTS.get("%s %s" % (r["id"], r["va"])) or {}).get("args", []))


FN_CACHE = os.path.join(ROOT, "work", "equiv_fn_cache.json")
_FC, _FC_LOCK = None, threading.Lock()
SKIPPED = []


def fn_cache():
    """{va: {"key": fingerprint+options, "json": path of the result it produced, "verdict": ...}} (work/equiv_fn_cache.json)."""
    global _FC
    if _FC is None:
        try:
            _FC = json.load(open(FN_CACHE))
        except (OSError, ValueError):
            _FC = {}
    return _FC


def save_fn_cache():
    if _FC is not None:
        tmp = FN_CACHE + ".tmp"
        json.dump(_FC, open(tmp, "w"), indent=0, sort_keys=True)
        os.replace(tmp, FN_CACHE)


def cached_obj(src, flags):
    """The object slice.compile_obj would return for src+flags if it is already cached, else None. Never compiles: a
    row with no cached object just runs (equiv compiles it)."""
    try:
        flags = SL.resolve_include_flags(flags)
        obj = os.path.join(SL.OBJDIR, "%s_%s.obj" % (SL._stem(src), SL.compile_key(src, flags)[:12]))
    except Exception:
        return None
    return obj if os.path.exists(obj) else None


def row_key(r, symbol, flags, extra):
    """The row's fingerprint: our compiled function (fn_hash) + the equiv options that shape the verdict. None when the
    object or symbol cannot be found (then the row simply runs)."""
    if not r["src"] or not symbol:
        return None
    obj = cached_obj(os.path.join(ROOT, r["src"]), (flags or "").split())
    sha = obj and fn_hash.fn_sha(obj, symbol)
    if not sha:
        return None
    return sha + "|" + " ".join(row_opts(r) + list(extra))


def run(r, out, extra, force=False):
    """Run one row into out/<id>_<va>.json. An existing result is skipped (restartable) unless force: then it stays in
    place until the new result replaces it, so a re-run in progress never leaves the row without a result.
    Unless force, a row whose compiled function and options are unchanged since its last recorded run is not re-run:
    that result is copied in (fn_cache, keyed by tools/difftest/fn_hash.py)."""
    js = os.path.join(out, "%s_%s.json" % (r["id"], r["va"]))
    if os.path.exists(js) and not force:
        return
    prev = fn_cache().get(r["va"])
    if prev and not force and os.path.exists(prev.get("json", "")):
        k = row_key(r, prev.get("symbol"), prev.get("flags"), extra)
        if k and k == prev.get("key"):
            if os.path.abspath(prev["json"]) != os.path.abspath(js):
                import shutil
                shutil.copyfile(prev["json"], js)
            SKIPPED.append(r["va"])
            return
    cmd = [PY, EQ, r["id"], r["va"], "--fp-tol", "--max-mismatch", "200", "--json", js + ".part"] + extra + row_opts(r)
    if r["src"]:
        cmd += ["--src", os.path.join(ROOT, r["src"])]
    # the hard cap follows a --time-limit given to equiv (the last one wins, as in argparse), with room to write the JSON
    tl = [cmd[i + 1] for i, a in enumerate(cmd[:-1]) if a == "--time-limit"]
    try:
        subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=max(900, float(tl[-1]) + 180 if tl else 0))
    except subprocess.TimeoutExpired:
        json.dump([{"verdict": "TIMEOUT"}], open(js + ".part", "w"))
    if os.path.exists(js + ".part"):
        os.replace(js + ".part", js)
    else:
        json.dump([{"verdict": "CRASH"}], open(js, "w"))
    record(r, js, extra)


def record(r, js, extra):
    """Remember the fingerprint the result in js was produced from (nothing for a crash or a timeout)."""
    try:
        d = json.load(open(js))
        d = d[0] if isinstance(d, list) and d else d
    except (OSError, ValueError):
        return
    if d.get("verdict") in (None, "CRASH", "TIMEOUT") or not d.get("symbol"):
        return
    k = row_key(r, d["symbol"], d.get("flags"), extra)
    if k:
        with _FC_LOCK:
            fn_cache()[r["va"]] = {"key": k, "json": os.path.abspath(js), "verdict": d["verdict"],
                                   "symbol": d["symbol"], "flags": d.get("flags")}


def tally(rs, out):
    res, c = [], collections.Counter()
    for r in rs:
        js = os.path.join(out, "%s_%s.json" % (r["id"], r["va"]))
        if not os.path.exists(js):
            continue
        d = json.load(open(js))
        d = d[0] if isinstance(d, list) and d else d
        v = d.get("verdict", "?")
        c[v] += 1
        res.append(dict(r, verdict=v, why=d.get("reason", "")[:300], sigs=d.get("mismatch_sigs", {}),
                        fp_tol=d.get("fp_tol")))
    json.dump(res, open(os.path.join(out, "tally.json"), "w"), indent=1)
    print("%d rows run of %d: %s" % (len(res), len(rs), dict(c.most_common())))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "sweep"))
    ap.add_argument("--workers", type=int, default=6)
    ap.add_argument("--filter")
    ap.add_argument("--tally", action="store_true")
    ap.add_argument("--extra", default="")
    ap.add_argument("--force", action="store_true", help="re-run every row, even one whose compiled function is unchanged "
                    "since its last run (after a harness change)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rs = [r for r in rows() if not a.filter or a.filter in r["id"] or a.filter in r["va"]]
    if not a.tally:
        with ThreadPoolExecutor(a.workers) as ex:
            list(ex.map(lambda r: run(r, a.out, a.extra.split(), force=a.force), rs))
        save_fn_cache()
        if SKIPPED:
            print("%d rows not re-run: their compiled function and options are unchanged since the recorded run"
                  % len(SKIPPED))
    tally(rs, a.out)


if __name__ == "__main__":
    sys.exit(main())
