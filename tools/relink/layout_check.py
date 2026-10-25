#!/usr/bin/env python3
"""layout_check.py: check every struct member's `// +0xNN` offset comment against the layout the compiler gives it.

usage: layout_check.py [--filter SUBSTR] [--jobs N] [-v]

A padding array of the wrong size moves every member after it while the comments keep the original's offsets:
LoadingMeshes had `char pad0[0xfb0]` before members commented +0x1210, so build_frames stored a pointer over the
mesh it had just built at +0xfb0. The comments are the evidence (they come from the original's code), so this
compiles one assertion per commented member, `offsetof(S, m) == 0xNN`, appended to a copy of each source (which
sees the source's own structs and every header it includes), and reports the members whose comment and layout
disagree, with the offset the compiler gave. Structs inside functions, templates and members it cannot name are
skipped. A header's structs are checked once, in the first source that includes it. Exits 1 when any disagree.
"""
import argparse, multiprocessing, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))

STRUCT_RE = re.compile(r"^(struct|class)\s+(?:__declspec\([^)]*\)\s*)?([A-Za-z_]\w*)\s*(?::[^{;]*)?\{")
NS_RE = re.compile(r"^namespace\s+([A-Za-z_]\w*)\s*\{")
# a data member with an offset comment: `type name[...]...;  // +0x1210 ...` (the first declarator is the one meant)
MEMBER_RE = re.compile(r"^\s+(?!return\b|typedef\b|using\b|friend\b|static\b|virtual\b|enum\b)"
                       r"[A-Za-z_][\w:<>,\s\*&]*?[\s\*&]([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*[,;].*//\s*\+\s*0x([0-9a-fA-F]+)")


def structs(path):
    """[(qualified struct name, member, offset, line)] of the namespace-level structs of one file."""
    out = []
    lines = open(path, errors="replace").read().split("\n")
    depth = 0
    ns = []                    # (name, depth at which it opened)
    cur = None                 # (qualified name, depth of its body)
    template = False
    for i, line in enumerate(lines):
        code = line.split("//")[0]
        s = line.strip()
        if cur is None:
            m = NS_RE.match(s)
            if m and depth == (ns[-1][1] + 1 if ns else 0):
                ns.append((m[1], depth))
            m = STRUCT_RE.match(s)
            if m and depth == (ns[-1][1] + 1 if ns else 0) and not template:
                cur = ("::".join([n for n, _d in ns] + [m[2]]), depth + 1)
        elif depth == cur[1]:
            m = MEMBER_RE.match(line)
            if m and "(" not in code:
                out.append((cur[0], m[1], int(m[2], 16), i + 1))
        template = s.startswith("template") and not s.rstrip().endswith((";", "}"))
        depth += code.count("{") - code.count("}")
        if cur is not None and depth < cur[1]:
            cur = None
        while ns and depth <= ns[-1][1]:
            ns.pop()
    return out


def quoted_headers(src):
    import slice as S
    return [f for f in S._quoted_includes(src) if f.endswith((".h", ".hpp")) and os.sep + "src" + os.sep in f]


def check_one(job):
    """Compile src with one assertion per (struct, member, offset) appended; -> [(entry, actual offset or None)]."""
    src, flags, entries = job
    os.chdir(ROOT)
    import slice as S
    if not entries:
        return []
    flags = S.resolve_include_flags(flags)
    tmp = os.path.join(os.path.dirname(src), "_layoutchk_%s" % os.path.basename(src))
    base = open(src, errors="replace").read()
    nbase = base.count("\n") + 1
    checks = []
    for k, (name, member, off, _f, _l) in enumerate(entries):
        checks.append("typedef char lc_%d_[(size_t)&reinterpret_cast<const volatile char&>(((%s*)0)->%s) == %#x ? 1 : -1];"
                      % (k, name, member, off))
    try:
        open(tmp, "w").write(base + "\n#include <stddef.h>\n" + "\n".join(checks) + "\n")
        r = subprocess.run([S.CL, "/nologo", "/c", "/Zs", *flags, S.INCLUDE, S.winpath(tmp)], capture_output=True,
                           text=True)
        out = r.stdout + r.stderr
        bad, unknown = set(), set()
        for m in re.finditer(r"_layoutchk_[^(]*\((\d+)\)\s*:\s*error (C\d+)", out):
            k = int(m[1]) - nbase - 2
            if 0 <= k < len(entries):
                (bad if m[2] == "C2118" else unknown).add(k)
        bad -= unknown
        if not bad:
            return [(e, None, "unchecked" if k in unknown else "ok") for k, e in enumerate(entries)]
        # the compiler's offsets of the disagreeing members: initializing a pointer to an array of offset + 1 chars
        # from an int fails with an error that spells the array type ("cannot convert from 'int' to 'char (*)[17]'")
        probes = ["/* probes */"]
        order = sorted(bad)
        for j, k in enumerate(order):
            name, member = entries[k][0], entries[k][1]
            probes.append("char (*lc_p_%d_)[(size_t)&reinterpret_cast<const volatile char&>(((%s*)0)->%s) + 1] = 1;"
                          % (j, name, member))
        open(tmp, "w").write(base + "\n#include <stddef.h>\n" + "\n".join(probes) + "\n")
        r = subprocess.run([S.CL, "/nologo", "/c", "/Zs", *flags, S.INCLUDE, S.winpath(tmp)], capture_output=True,
                           text=True)
        actual = {}
        for m in re.finditer(r"_layoutchk_[^(]*\((\d+)\)\s*:\s*error C\d+:.*\(\*\)\[(\d+)\]", r.stdout + r.stderr):
            j = int(m[1]) - nbase - 3
            if 0 <= j < len(order):
                actual[order[j]] = int(m[2]) - 1
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
    return [(e, actual.get(k), "bad" if k in bad else "unchecked" if k in unknown else "ok")
            for k, e in enumerate(entries)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("-v", action="store_true", help="also list the members that could not be checked")
    a = ap.parse_args()
    os.chdir(ROOT)
    import compile_all as C
    seen = set()
    jobs = []
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src or (a.filter and a.filter not in src):
            continue
        entries = []
        for f in [src] + quoted_headers(src):
            if f in seen:
                continue
            seen.add(f)
            entries += [(n, m, o, os.path.relpath(f, ROOT), l) for n, m, o, l in structs(f)]
        jobs.append((src, flags, entries))
    with multiprocessing.Pool(a.jobs) as pool:
        results = [r for rs in pool.map(check_one, jobs, chunksize=1) for r in rs]
    bad = [(e, act) for e, act, st in results if st == "bad"]
    unchecked = [e for e, _a, st in results if st == "unchecked"]
    for (name, member, off, f, line), act in sorted(bad, key=lambda x: (x[0][3], x[0][4])):
        print("%s:%d  %s::%s  comment +%#x, laid out at %s" % (f, line, name, member, off,
                                                              "+%#x" % act if act is not None else "?"))
    if a.v:
        for name, member, off, f, line in unchecked:
            print("unchecked %s:%d  %s::%s" % (f, line, name, member))
    print("%d commented members: %d agree, %d disagree, %d could not be checked"
          % (len(results), len(results) - len(bad) - len(unchecked), len(bad), len(unchecked)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
