#!/usr/bin/env python3
"""Build the cross-compiler equivalence corpus: every /O2 unit of a byte-exact decomp compiled by two compilers.

Runs on the Windows laptop (native cl.exe, no wine):
    D:\\mb\\repo\\.venv\\Scripts\\python build_pairs.py --root D:\\mb\\fnv --out D:\\mb\\fnv_out [--jobs 8] [--only substr]

--root holds the decomp's src/, include/ and build/units.json (the FNV layout: {"units": [{"source", "flags",
"functions": [{"address", "symbol", "size"}]}]}). Each unit with /O2 in its flags is compiled twice, with the same flags,
by each compiler in COMPILERS, into <out>/<compiler>/<unit stem>.obj, with the compiler's output in a .log beside it.
/GS is dropped (flags_for): cl 13.10 and cl 14.00 lay out the security cookie differently (14.00 xors it with the frame pointer),
and that difference is not what the corpus is for. A pair of objects from the same source is equivalent by construction
(barring undefined behaviour and compiler bugs), but their register allocation, scheduling and frame layout differ:
that is the test set for tools/equiv (the frame model). <out>/index.json lists every unit: its flags, each compiler's
exit code and the unit's function rows.

The decomp's own source is not part of this repository (fnv_decomp grants no licence); neither are the objects. Only
this script is.
"""
import argparse, json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

TC = r"D:\mb\repo\work\toolchain"
COMPILERS = {
    # name: (cl.exe, INCLUDE entries, extra PATH entries)
    "vc71": (TC + r"\vc71tk\bin\cl.exe", [r"D:\mb\corpus\vc71shim", TC + r"\vc71tk\include", TC + r"\sal", TC + r"\vc9sp1\WinSDK\Include"],
             [TC + r"\vc71tk\bin"]),
    "vc8": (r"D:\mb\vc8\VC\bin\cl.exe", [r"D:\mb\vc8\VC\include", r"D:\mb\vc8\SDK\Include"], [r"D:\mb\vc8\VC\bin"]),
}
IDLE = 0x00000040          # IDLE_PRIORITY_CLASS


def flags_for(unit, name="vc8"):
    """The unit's flags without the security cookie. cl 13.10 has no /GS- (the cookie is off unless /GS is given)
    and no __declspec(noalias), __restrict or override (hints and checks, so dropping them keeps the behaviour); its
    <crtdefs.h>, new in VS2005, is corpus/vc71shim/crtdefs.h, installed beside the script."""
    if name == "vc71":
        return [f for f in unit["flags"] if f not in ("/GS", "/GS-")] + ["/Dnoalias=", "/D__restrict=", "/Doverride="]
    return ["/GS-" if f == "/GS" else f for f in unit["flags"]]


def stem(unit):
    return unit["source"].replace("/", "_").replace("\\", "_").rsplit(".", 1)[0]


THISCALL_PTR = re.compile(r"__thiscall\s*\*|\(\s*__thiscall")


def run_cl(name, unit, root, extra, out_args, log):
    cl, inc, path = COMPILERS[name]
    env = dict(os.environ, INCLUDE=";".join(inc), PATH=";".join(path) + ";" + os.environ.get("PATH", ""))
    cmd = [cl, "/nologo"] + flags_for(unit, name) + extra + ["/I" + os.path.join(root, "include"),
                                                          os.path.join(root, unit["source"])] + out_args
    with open(log, "w") as lf:
        return subprocess.run(cmd, cwd=root, env=env, stdout=lf, stderr=subprocess.STDOUT, creationflags=IDLE).returncode


def build(job):
    """-> (compiler, source, exit code, thiscall stripped). cl 13.10 rejects an explicit __thiscall (C4234). On a
    member function it is redundant (members are thiscall anyway), so when the unit's whole preprocessed text (cl 14's
    /EP) has no __thiscall pointer type, cl 13.10 is rerun with /D__thiscall=. A pointer type keeps the unit out: there
    the keyword is what makes the call pass this in ecx. A free __thiscall function changes its decorated name with
    the keyword gone, so index_pairs.py never pairs it."""
    root, out, name, unit = job
    d = os.path.join(out, name)
    obj, log = os.path.join(d, stem(unit) + ".obj"), os.path.join(d, stem(unit) + ".log")
    stripped = obj + ".stripped"
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(os.path.join(root, unit["source"])):
        return name, unit["source"], 0, os.path.exists(stripped)
    r = run_cl(name, unit, root, ["/c"], ["/Fo" + obj], log)
    st = False
    if r and name == "vc71" and "C4234" in open(log, errors="replace").read():
        pp = os.path.join(d, stem(unit) + ".i")
        ok = run_cl("vc8", unit, root, ["/EP"], [], pp) == 0 and not THISCALL_PTR.search(open(pp, errors="replace").read())
        os.remove(pp)
        if ok:
            r = run_cl(name, unit, root, ["/c", "/D__thiscall="], ["/Fo" + obj], log)
            st = r == 0
    if r and os.path.exists(obj):
        os.remove(obj)
    if st:
        open(stripped, "w").close()
    elif os.path.exists(stripped):
        os.remove(stripped)
    return name, unit["source"], r, st


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--only", default="")
    a = ap.parse_args()
    units = [u for u in json.load(open(os.path.join(a.root, "build", "units.json")))["units"]
             if "/O2" in u["flags"] and a.only in u["source"]]
    for n in COMPILERS:
        os.makedirs(os.path.join(a.out, n), exist_ok=True)
    jobs = [(a.root, a.out, n, u) for u in units for n in COMPILERS]
    rc, strip = {}, {}
    with ThreadPoolExecutor(a.jobs) as ex:
        for i, (n, src, code, st) in enumerate(ex.map(build, jobs)):
            rc.setdefault(src, {})[n] = code
            strip[src] = strip.get(src, False) or st
            if (i + 1) % 100 == 0:
                print("%d/%d" % (i + 1, len(jobs)), flush=True)
    index = [{"source": u["source"], "stem": stem(u), "flags": {n: flags_for(u, n) for n in COMPILERS}, "rc": rc[u["source"]],
              "thiscall_stripped": strip[u["source"]], "functions": u["functions"]} for u in units]
    json.dump(index, open(os.path.join(a.out, "index.json"), "w"), indent=0)
    ok = sum(all(v == 0 for v in e["rc"].values()) for e in index)
    print("%d units, %d built by both compilers (%d with __thiscall stripped); per compiler: %s" % (
        len(index), ok, sum(e["thiscall_stripped"] for e in index), {n: sum(e["rc"][n] == 0 for e in index) for n in COMPILERS}))


if __name__ == "__main__":
    sys.exit(main())
