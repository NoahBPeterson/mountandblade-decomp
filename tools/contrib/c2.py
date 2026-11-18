#!/usr/bin/env python3
"""c2: run the c2 glass-box tools of ../vs2005-cpp (cl 13.10's own register allocator, scheduler, IL) on one row.

  .venv/bin/python tools/contrib/c2.py TOOL 0x<va> [--src work/match/FILE.cpp] [TOOL ARGS...]

TOOL (docs: ../vs2005-cpp/docs/c2/<name>.md; `c2.py doc NAME` prints one):
  regs         register-allocation report: which variable got which register, and why (c2trace.py regs; --why a,b)
  regmap       which register each of our variables SHOULD have, read off the original's code
  scratch      scratch-register picks -> listing offsets (--why)
  slots        stack-slot packing (--what EDIT.., --search)
  icount       why our function has more/fewer instructions: each extra/missing one traced to the c2 pass
  sched        what the list scheduler did (instruction order)
  tsweep       compile every alternative of a template (`{{a|b|c}}` choices, see doc tsweep) and rank by bdiff
  helpersweep  inline-helper rewrites that fix a colouring order (--want 'A>B')
  effectmine   mutations mined for a wanted effect (mine; --want drop:N ...)
  doc NAME     print ../vs2005-cpp/docs/c2/NAME.md (e.g. tracer, regalloc, icount, tsweep, effectmine, x87)

The wrapper fills in what every tool needs: the source (the row's module, or --src: a copy/template you wrote under
work/match/, whose relative #includes resolve as if it sat in the module's directory), the decorated name
(--func), --c2 13.10, the original's listing (--asm-vs, written to work/match/c2_orig_<va>.s), and the module's cl
flags and include path. Traced compiles run on the laptop's compile server. At most 8 c2.py runs go at once
across all sessions (it waits for a slot) and --jobs is capped at 4.
"""
import fcntl, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VS = os.path.join(os.path.dirname(ROOT), "vs2005-cpp")
PY = os.path.join(ROOT, ".venv", "bin", "python")
TOOLS = {"regs": ["tools/c2trace.py", "regs"], "regmap": ["tools/c2trace.py", "regmap"],
         "scratch": ["tools/c2trace.py", "scratch"], "slots": ["tools/c2trace.py", "slots"],
         "icount": ["tools/icount.py"], "sched": ["tools/c2sched.py"], "tsweep": ["tools/tsweep.py"],
         "helpersweep": ["tools/helpersweep.py"], "effectmine": ["tools/effectmine.py", "mine"]}
ASM = {"regmap", "icount", "tsweep", "helpersweep", "effectmine"}      # take --asm-vs
SLOTS, MAXJOBS = 8, 4


def die(msg):
    sys.exit("c2.py: " + msg)


def run(args):
    return subprocess.run(args, cwd=ROOT, capture_output=True, text=True).stdout


def main():
    a = sys.argv[1:]
    if not a or a[0] in ("-h", "--help"):
        print(__doc__); return
    if a[0] == "doc":
        name = re.sub(r"[^\w-]", "", a[1] if len(a) > 1 else "")
        p = os.path.join(VS, "docs", "c2", name + ".md")
        if not os.path.isfile(p):
            die("no doc %s; have: %s" % (name, " ".join(sorted(f[:-3] for f in os.listdir(os.path.join(VS, "docs", "c2"))))))
        sys.stdout.write(open(p).read()); return
    tool = a[0]
    if tool not in TOOLS or len(a) < 2:
        die("usage: c2.py {%s|doc} 0x<va> [--src work/match/F.cpp] [args]" % "|".join(TOOLS))
    va = int(a[1], 16)
    rest = a[2:]
    src = None
    if "--src" in rest:
        i = rest.index("--src"); src = rest[i + 1]; del rest[i:i + 2]
    if any(x.startswith("--c2") or x == "--" for x in rest):
        die("--c2 and the cl flags are filled in by the wrapper; leave them out")
    if "--jobs" in rest:
        i = rest.index("--jobs"); rest[i + 1] = str(min(MAXJOBS, int(rest[i + 1])))
    elif tool in ("tsweep", "helpersweep", "effectmine"):
        rest += ["--jobs", str(MAXJOBS)]

    t = run([PY, "tools/contrib/task.py", "0x%08x" % va])
    m = re.search(r"^source\s+(\S+\.cpp)", t, re.M); f = re.search(r"^flags\s+(.*)$", t, re.M)
    if not m:
        die("task.py knows no source for 0x%08x" % va)
    module = m.group(1)
    flags = (f.group(1).split() if f else ["/O2", "/MT", "/Gy", "/TP"])
    c = run([PY, "tools/matching/chk.py", module, "0x%x" % va])
    d = re.search(r"marker (\S+)", c)
    if not d:
        die("chk.py did not give the decorated name:\n" + c[-800:])
    func = d.group(1)
    if src:
        rel = os.path.normpath(os.path.relpath(os.path.abspath(src), ROOT))
        if not rel.startswith("work/match/") or not os.path.isfile(os.path.join(ROOT, rel)):
            die("--src must be an existing file under work/match/")
        src = rel
    else:
        src = module
    size = re.search(r"\(size \d+ vs (\d+)\)", c)
    orig = os.path.join(ROOT, "work", "match", "c2_orig_%08x.s" % va)
    if tool in ASM and not os.path.exists(orig):
        n = int(size.group(1)) if size else 4000
        lines = run([PY, "tools/matching/disasm.py", "0x%x" % va, str(n)]).splitlines()
        with open(orig, "w") as fh:
            for ln in lines:
                mm = re.match(r"^\s*(?:0x)?([0-9a-fA-F]{6,8}):?\s+[0-9a-fA-F]+\s+(\S.*)$", ln)
                if mm:
                    fh.write("%08x %s\n" % (int(mm.group(1), 16), re.sub(r"\s+;.*$", "", mm.group(2)).strip()))
    inc = ["/I" + os.path.join(ROOT, os.path.dirname(module)), "/I" + os.path.join(ROOT, "match", "include"),
           "/I" + os.path.join(ROOT, "src", "include"), "/I" + os.path.join(ROOT, "work/ext/dxsdk_jun10/DXSDK/Include")]
    cmd = [os.path.join(VS, ".venv", "bin", "python")] + TOOLS[tool] + [os.path.join(ROOT, src)]
    cmd += ["--func", func, "--c2", "13.10.3052" if tool == "tsweep" else "13.10"]
    if tool in ASM:
        cmd += ["--asm-vs", orig] + (["--asm-func", func] if tool in ("regmap", "tsweep") else [])
    cmd += rest + ["--"] + flags + inc

    os.makedirs(os.path.join(ROOT, "work", "match", ".c2slots"), exist_ok=True)
    lock = None
    while lock is None:
        for k in range(SLOTS):
            fh = open(os.path.join(ROOT, "work", "match", ".c2slots", str(k)), "w")
            try:
                fcntl.flock(fh, fcntl.LOCK_EX | fcntl.LOCK_NB); lock = fh; break
            except OSError:
                fh.close()
        else:
            time.sleep(2)
    sys.stdout.flush()
    sys.exit(subprocess.run(cmd, cwd=VS).returncode)


if __name__ == "__main__":
    main()
