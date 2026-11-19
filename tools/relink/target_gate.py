#!/usr/bin/env python3
"""target_gate.py: byte-exact rows must call what the original calls.

usage: target_gate.py [--json CALL_DIFF_JSON] [--update] [--jobs N]

The byte check (chk.py, run_all.py) masks relocated fields on both sides, so it cannot see WHICH address a call, a
vtable store or an import goes to. A row can be "byte-exact" and call a same-shaped twin (vector<int>::_Insert_n at
0x410610 where the original calls vector<RecJ*>'s copy at 0x43e7b0: the original has no identical-code folding), a
destructor annotated with its deleting destructor's address, or store a vtable 4 bytes off. (Found 2026-11-19 after
the spore-decomp target audit: 45 byte-exact rows.)

Two checks over every *.manifest.txt row:
  targets  tools/relink/call_diff.py --all (its "ours" side resolves our symbols through
           work/relink/standalone/resolve_map.json, so run this after tools/relink/build.py): a call, virtual slot,
           import or stored code pointer whose original address the original's code at this row never uses.
           Calls our side names but resolve_map cannot place (n:...) are not compared.
  clones   one symbol listed at two VAs in the same .manifest.txt: a .cpp emits a symbol once, so one of the rows
           only matches the other's shape. File-local compiler names ($L..., _$E...) and `static` / `compiler-
           generated` placeholders are skipped.

Known findings are listed in tools/relink/target_gate_baseline.json; a finding not in it fails the gate (exit 1).
A baseline entry that no longer shows is reported as fixed; --update rewrites the baseline to the current findings
(only ever shrink it by hand-checked fixes; never add to it to make a gate pass).
"""
import argparse, collections, glob, json, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BASE = os.path.join(ROOT, "tools", "relink", "target_gate_baseline.json")
SKIP_SYMS = ("$L", "_$E", "static", "compiler-generated")


def manifest_rows():
    """{va: (manifest path, symbol)}, and the clone findings."""
    rows, clones = {}, []
    for f in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True)):
        by_sym = collections.defaultdict(list)
        for l in open(f, encoding="utf-8", errors="replace"):
            p = l.split()
            if len(p) < 3:
                continue
            try:
                va = int(p[2], 16)
            except ValueError:
                continue
            rows[va] = (os.path.relpath(f, ROOT), p[1])
            if not p[1].startswith(SKIP_SYMS):
                by_sym[p[1]].append(va)
        for sym, vas in by_sym.items():
            if len(set(vas)) > 1:
                clones.append("clone %s %s" % (sym, " ".join("%08x" % v for v in sorted(set(vas)))))
    return rows, clones


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", help="reuse a call_diff.py --all --json output instead of running it")
    ap.add_argument("--update", action="store_true", help="rewrite the baseline to the current findings")
    ap.add_argument("--jobs", type=int, default=3)
    a = ap.parse_args()
    rows, findings = manifest_rows()
    js = a.json
    if not js:
        js = os.path.join(tempfile.mkdtemp(), "call_diff.json")
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "relink", "call_diff.py"), "--all", "--top", "0",
                            "--jobs", str(a.jobs), "--json", js], cwd=ROOT, capture_output=True, text=True)
        if r.returncode or not os.path.exists(js):
            print("target_gate: call_diff.py failed\n" + (r.stdout + r.stderr)[-2000:])
            return 1
    for r in json.load(open(js)):
        if "error" in r or int(r["va"], 16) not in rows or not r.get("orig"):
            continue                                     # not byte-exact, or a row call_diff could not bind
        for k in sorted(r["extra"]):
            if not k.startswith("n:"):
                findings.append("target %s %s %s" % (r["va"], k, rows[int(r["va"], 16)][1]))
    base = set(json.load(open(BASE))) if os.path.exists(BASE) else set()
    now = set(findings)
    new, fixed = sorted(now - base), sorted(base - now)
    if a.update:
        json.dump(sorted(now), open(BASE, "w"), indent=0)
        open(BASE, "a").write("\n")
        print("target_gate: baseline rewritten, %d findings (%d fixed since the last baseline)" % (len(now), len(fixed)))
        return 0
    for f in fixed:
        print("fixed (drop from the baseline with --update): " + f)
    for f in new:
        print("NEW  " + f)
    print("target_gate: %d findings, %d known, %d new, %d fixed" % (len(now), len(now & base), len(new), len(fixed)))
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
