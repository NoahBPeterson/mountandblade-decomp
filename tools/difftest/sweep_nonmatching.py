#!/usr/bin/env python3
"""sweep_nonmatching.py: re-run equiv on every nonmatching row (match/slices/*/nonmatching.txt and
src/**/*.nonmatching.txt) and tally every mismatch, not just the first (equiv's mismatch_sigs).

usage: sweep_nonmatching.py [--out DIR=work/sweep] [--workers N=6] [--filter substr] [--tally] [--extra "..."]
Each row runs `equiv.py <id> <va> [--src <module.cpp>] --fp-tol --max-mismatch 200 --json <out>/<id>_<va>.json`;
rows whose JSON exists are skipped (restartable). A VA listed both in a src module and in its old slice runs
once, from the module. --tally prints verdict counts and writes <out>/tally.json: every row with its recorded
reason, the new verdict and the mismatch signatures."""
import argparse, collections, glob, json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = os.path.join(ROOT, ".venv", "bin", "python")
EQ = os.path.join(ROOT, "tools", "difftest", "equiv.py")
ROW = re.compile(r"^([0-9a-fA-F]{8})\s+\S+\s+(.*)$")


def rows():
    out = {}
    for p in sorted(glob.glob(os.path.join(ROOT, "match", "slices", "*", "nonmatching.txt"))):
        sid = os.path.basename(os.path.dirname(p))
        for line in open(p, encoding="utf-8", errors="replace"):
            m = ROW.match(line.strip())
            if m:
                out[m[1].lower()] = {"id": sid, "va": m[1].lower(), "src": None, "reason": m[2][:2000],
                                     "file": os.path.relpath(p, ROOT)}
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True)):
        mod = os.path.basename(p)[:-len(".nonmatching.txt")]
        src = p[:-len(".nonmatching.txt")] + ".cpp"
        for line in open(p, encoding="utf-8", errors="replace"):
            m = ROW.match(line.strip())
            if m:
                out[m[1].lower()] = {"id": mod, "va": m[1].lower(), "src": os.path.relpath(src, ROOT),
                                     "reason": m[2][:2000], "file": os.path.relpath(p, ROOT)}
    return sorted(out.values(), key=lambda r: r["va"])


def run(r, out, extra):
    js = os.path.join(out, "%s_%s.json" % (r["id"], r["va"]))
    if os.path.exists(js):
        return
    cmd = [PY, EQ, r["id"], r["va"], "--fp-tol", "--max-mismatch", "200", "--json", js + ".part"] + extra
    if r["src"]:
        cmd += ["--src", os.path.join(ROOT, r["src"])]
    try:
        subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=900)
    except subprocess.TimeoutExpired:
        json.dump([{"verdict": "TIMEOUT"}], open(js + ".part", "w"))
    if os.path.exists(js + ".part"):
        os.replace(js + ".part", js)
    else:
        json.dump([{"verdict": "CRASH"}], open(js, "w"))


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
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rs = [r for r in rows() if not a.filter or a.filter in r["id"] or a.filter in r["va"]]
    if not a.tally:
        with ThreadPoolExecutor(a.workers) as ex:
            list(ex.map(lambda r: run(r, a.out, a.extra.split()), rs))
    tally(rs, a.out)


if __name__ == "__main__":
    sys.exit(main())
