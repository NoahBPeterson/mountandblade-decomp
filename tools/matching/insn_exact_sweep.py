#!/usr/bin/env python3
"""insn_exact_sweep.py: run insn_exact.py over every non-byte-exact row of the migrated modules and record the verdicts.

usage: insn_exact_sweep.py [--jobs 4] [--filter SUBSTR] [--out docs/insn_exact.json]

Rows come from src/**/<module>.nonmatching.txt (their source is <module>.cpp next to it). Byte-exact rows
(manifest.txt) are instruction-exact by definition and are not re-checked. A function whose row says it is a
switch with a jump table is also compared case by case (--cases) when insn_exact.py finds a dispatch; the case
summary is recorded next to the function verdict. tools/progress.py reads the output file.
"""
import argparse, datetime, glob, json, os, re, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOL = os.path.join(ROOT, "tools", "matching", "insn_exact.py")
PY = sys.executable


def rows():
    out = {}
    for nm in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True)):
        src = nm[:-len(".nonmatching.txt")] + ".cpp"
        if not os.path.exists(src):
            continue
        for l in open(nm, encoding="utf-8", errors="replace"):
            m = re.match(r"^([0-9a-fA-F]{8})\s", l)
            if m:
                out.setdefault(os.path.relpath(src, ROOT), []).append(int(m.group(1), 16))
    return out


def run(src, vas, cases_names):
    fd, js = tempfile.mkstemp(suffix=".json")
    os.close(fd)
    try:
        r = subprocess.run([PY, TOOL, src, ",".join("%x" % v for v in vas), "--json", js], cwd=ROOT,
                           capture_output=True, text=True, timeout=1800)
        res = json.load(open(js)) if os.path.getsize(js) else {}
        if not res:
            tail = (r.stdout + r.stderr).strip().splitlines()[-1:] or ["no output"]
            res = {"%08x" % v: {"verdict": "ERROR", "why": tail[0][:300]} for v in vas}
        # switch functions: the case-by-case view (only rows insn_exact.py could not call exact as a whole)
        for v in vas:
            row = res.get("%08x" % v, {})
            if row.get("verdict") == "DIFF" and row.get("insns", 0) > 2000:
                args = [PY, TOOL, src, "%x" % v, "--cases", "--json", js]
                if cases_names:
                    args += ["--names", cases_names]
                subprocess.run(args, cwd=ROOT, capture_output=True, text=True, timeout=3600)
                cr = json.load(open(js)).get("%08x" % v) if os.path.getsize(js) else None
                if cr and "cases" in cr:
                    vs = [c["verdict"] for c in cr["cases"].values()]
                    ins = sum(c["insns"] for c in cr["cases"].values()) or 1
                    ex = sum(c["insns"] for c in cr["cases"].values() if c["verdict"] == "INSN-EXACT")
                    row["cases"] = {"total": len(vs), "insn_exact": vs.count("INSN-EXACT"),
                                    "insn_share": round(ex / ins, 4), "prologue": cr.get("prologue")}
        return src, res
    except subprocess.TimeoutExpired:
        return src, {"%08x" % v: {"verdict": "ERROR", "why": "timeout"} for v in vas}
    finally:
        os.remove(js)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--filter")
    ap.add_argument("--out", default=os.path.join(ROOT, "docs", "insn_exact.json"))
    ap.add_argument("--names", default=os.path.join(ROOT, "work", "match", "scratch_si_tools", "jt_names.json"))
    a = ap.parse_args()
    todo = {s: v for s, v in rows().items() if not a.filter or a.filter in s}
    names = a.names if os.path.exists(a.names) else None
    prev = {}
    if a.filter and os.path.exists(a.out):
        prev = json.load(open(a.out)).get("rows", {})
    out = dict(prev)
    n = 0
    with ThreadPoolExecutor(a.jobs) as ex:
        for src, res in ex.map(lambda kv: run(kv[0], kv[1], names), todo.items()):
            n += 1
            for va, row in res.items():
                row["src"] = src
                out[va] = row
            c = sum(1 for r in res.values() if r.get("verdict") == "INSN-EXACT")
            print("[%d/%d] %-60s %d/%d insn-exact" % (n, len(todo), src, c, len(res)), flush=True)
    counts = {}
    for r in out.values():
        counts[r.get("verdict")] = counts.get(r.get("verdict"), 0) + 1
    json.dump({"when": datetime.datetime.now().astimezone().isoformat(timespec="seconds"), "counts": counts,
               "rows": dict(sorted(out.items()))}, open(a.out, "w"), indent=1)
    print("rows:", len(out), counts)


if __name__ == "__main__":
    main()
