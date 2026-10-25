#!/usr/bin/env python3
"""trial_link.py: link every src/ module into an exe and measure what is still missing for a standalone build.

usage: trial_link.py [--record] [--list CATEGORY] [--json OUT] [--glue [DIR]] [--dir OUT_DIR]

Compiles each src/**/*.cpp with its manifest flags (cached, as compile_all), links all objects with link.exe 7.10
(the original's linker version) against the VC7.1 static runtime (libcmt, libcpmt), the Windows SDK 6.0A import
libraries and the DirectX SDK, and classifies every unresolved symbol:

  name mismatch    a call to a function our source defines, under another name or signature (each module declared
                   its callees its own way; the equivalence harness resolves by address and never noticed)
  global data      an extern with an address annotation and no definition: storage the original image provides
  no annotation    a function or global declared somewhere, defined nowhere, without an address annotation
  missing body     a function whose address has no definition in src/
  third-party      Newton / FMOD 3 / SkinMagic imports (those DLLs ship without import libraries)
  CRT              C runtime functions declared under the wrong name or linkage
  other            anything else

Duplicate definitions are counted too (linked with /FORCE:MULTIPLE so that unresolved symbols are all listed).
--record appends a point to docs/relink_history.json (tools/progress.py prints the latest). Classification is by
name and annotation, so counts are close but not exact (an overloaded name can land in the wrong bucket).
"""
import argparse, collections, csv, datetime, glob, json, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
import slice as S          # noqa: E402

TC = os.path.join(ROOT, "work", "toolchain")
LIBS = ["libcmt.lib", "libcpmt.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "shell32.lib",
        "shfolder.lib", "ole32.lib", "oleaut32.lib", "winmm.lib", "ws2_32.lib", "d3d9.lib", "d3dx9.lib",
        "dinput8.lib", "dxguid.lib"]
CRT_LO, CRT_HI = 0x535440, 0x549000          # static CRT / C++ library code in the original
CATS = ("name mismatch", "global data", "no annotation", "missing body", "third-party", "CRT", "other")
HIST = os.path.join(ROOT, "docs", "relink_history.json")


def winpath(p):
    return "Z:" + p.replace("/", "\\")


def objects():
    import compile_all as C
    objs, fails = [], []
    for src, flags in C.sources():
        if os.sep + "src" + os.sep not in src:
            continue
        o, _log = S.compile_obj(src, flags)
        (objs if o else fails).append(o or os.path.relpath(src, ROOT))
    return objs, fails


def link(objs, out_dir, glue=None, extra=(), name="trial.exe"):
    libs = LIBS
    if glue:        # tools/relink/glue.py: the original's data, aliases, and import libraries for the shipped DLLs
        objs = objs + [os.path.join(glue, "mb_data.obj")]
        own = sorted(glob.glob(os.path.join(glue, "imports", "*.lib")))
        libs = [l for l in LIBS if l != "d3dx9.lib"] + [winpath(l) for l in own]
    rsp = os.path.join(out_dir, "objs.rsp")
    open(rsp, "w").write("\n".join('"%s"' % winpath(o) for o in objs) + "\n")
    env = dict(os.environ, WINEPREFIX=os.path.join(TC, "wineprefix-mb"), WINEDEBUG="-all",
               LIB=";".join(winpath(p) for p in (os.path.join(TC, "vc71tk", "lib"), os.path.join(TC, "vc9sp1", "WinSDK", "Lib"),
                                                   os.path.join(ROOT, "work", "ext", "dxsdk_jun10", "DXSDK", "Lib", "x86"))))
    cmd = ["wine", os.path.join(TC, "vc71tk", "bin", "link.exe"), "/NOLOGO", "/OUT:" + winpath(os.path.join(out_dir, name)),
           "/SUBSYSTEM:WINDOWS", "/MACHINE:X86", "/FORCE:MULTIPLE", "/OPT:REF", "/OPT:ICF"] + list(extra) + ["@" + winpath(rsp)] + libs
    with tempfile.TemporaryFile("w+") as f:      # never the caller's pipes: wine services inherit stdio
        subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=f, stderr=subprocess.STDOUT, env=env)
        f.seek(0)
        return f.read()


UNRES = re.compile(r'^(\S+?\.obj) : error LNK20(?:19|01): unresolved external symbol (?:"(.*?)" \((\S+)\)|(\S+))', re.M)


def parse_unresolved(log):
    """{decorated name: (readable name, {referencing object})} from LNK2019 (from code) and LNK2001 (from data,
    e.g. a vtable slot) lines."""
    out = {}
    for m in UNRES.finditer(log):
        sym = m[3] or m[4]
        out.setdefault(sym, (m[2] or sym, set()))[1].add(os.path.basename(m[1]))
    return out


def annotations():
    ann, defined = collections.defaultdict(set), set()
    decl = re.compile(r'\b([A-Za-z_~][\w:~]*)\s*(?:\([^;{]*\)|\[[^\]]*\])?\s*(?:const\s*)?;\s*//.*?\b0x([0-9a-fA-F]{6,7})\b')
    mark = re.compile(r'//\s*@\s*0x([0-9a-fA-F]{6})\b')
    for p in glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True):
        if not p.endswith((".h", ".cpp")):
            continue
        for line in open(p, errors="replace"):
            m = mark.search(line)
            if m:
                defined.add(int(m[1], 16))
            m = decl.search(line)
            if m:
                ann[m[1].split("::")[-1]].add(int(m[2], 16))
    return ann, defined


def classify(mangled, pretty, ann, defined):
    n = mangled
    if "Newton" in n[:16] or "FSOUND_" in n or "FMUSIC_" in n or "SkinMagic" in n:
        return "third-party"
    q = S.demangle_qual(n) if n.startswith("?") else n.lstrip("_").split("@")[0]
    base = (q or n).split("::")[-1]
    isdata = bool(re.match(r'^\?[^@]+(@[^@]+)*@@[0-9]', n)) or (not n.startswith("?") and "(" not in pretty
                                                                 and not re.search(r'@\d+$', n))
    addrs = ann.get(base)
    if not addrs:
        return "no annotation"
    a = min(addrs)
    if CRT_LO <= a < CRT_HI:
        return "CRT"
    if isdata or a >= CRT_HI:
        return "global data"
    return "name mismatch" if a in defined else "missing body"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--record", action="store_true")
    ap.add_argument("--list", metavar="CATEGORY")
    ap.add_argument("--json")
    ap.add_argument("--glue", nargs="?", const=os.path.join(ROOT, "work", "relink", "glue"),
                    help="also link tools/relink/glue.py's output (default work/relink/glue)")
    ap.add_argument("--dir", help="output directory for trial.exe and link.log (default work/relink); worktrees share "
                                  "work/, so parallel runs need their own")
    a = ap.parse_args()
    os.chdir(ROOT)
    out_dir = os.path.abspath(a.dir) if a.dir else os.path.join(ROOT, "work", "relink")
    os.makedirs(out_dir, exist_ok=True)
    objs, fails = objects()
    log = link(objs, out_dir, a.glue)
    open(os.path.join(out_dir, "link.log"), "w").write(log)
    syms = {k: '"%s"' % v[0] if v[0] != k else k for k, v in parse_unresolved(log).items()}
    dups = sorted(set(re.findall(r'LNK4006: (.*?) already defined', log)))
    ann, defined = annotations()
    by = collections.defaultdict(list)
    for m, p in syms.items():
        by[classify(m, p, ann, defined)].append(p)
    linked = not syms and os.path.exists(os.path.join(out_dir, "trial.exe")) and "LNK1120" not in log
    print("trial link of %d objects (%d failed to compile): %s" % (len(objs), len(fails), "LINKS" if linked else
                                                                    "%d unresolved symbols, %d duplicate definitions" % (len(syms), len(dups))))
    for c in CATS:
        if by[c]:
            print("  %5d  %-14s e.g. %s" % (len(by[c]), c, by[c][0][:90]))
    if dups:
        print("  %5d  %-14s e.g. %s" % (len(dups), "duplicates", dups[0][:90]))
    if a.list:
        for p in sorted(by.get(a.list, dups if a.list == "duplicates" else [])):
            print(p)
    rep = {"when": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
           "commit": subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip(),
           "objects": len(objs), "compile_failures": len(fails), "unresolved": len(syms), "duplicates": len(dups),
           "linked": linked, "by_category": {c: len(by[c]) for c in CATS},
           "glue": bool(a.glue), "counts_lnk2001": True}
    if a.json:
        json.dump(dict(rep, symbols={c: sorted(by[c]) for c in CATS}, duplicate_symbols=dups), open(a.json, "w"), indent=1)
    if a.record:
        try:
            hist = json.load(open(HIST))
        except (OSError, ValueError):
            hist = {}
        hist[rep["when"]] = rep
        with open(HIST, "w") as f:
            json.dump(dict(sorted(hist.items())), f, indent=1)
            f.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
