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
  il           the IL at the entry of chosen passes: --at PASS[,PASS..], or --diff A,B to diff one pass's IL
               against another's (what the passes from A up to B did); --plain / --grep RE / --count RE
  tsweep       compile every alternative of a template (`{{a<|>b<|>c}}` choices: the separator is <|>, not |; see doc tsweep) and rank by bdiff
  schedwant    which DAG changes give the original's instruction order in a scheduling region
  helpersweep  inline-helper rewrites that fix a colouring order (--want 'A>B')
  effectmine   mutations mined for a wanted effect (mine; --want drop:N ...)
  --func NAME  use this decorated name instead of chk.py's marker (for rows chk.py binds only "~size")
  symkeys      symbol keys / sort sites: c2.py symkeys 0x<va> SUB [args] (SUB = keys, sites, events, whatif, levers, compare, export).
               Slow on big rows: `symkeys 0x<va> export --out work/match/X.json` once (background), then
               `symkeys 0x<va> sites --from work/match/X.json` etc. run in seconds. Site lines are function-relative.
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
         "icount": ["tools/icount.py"], "sched": ["tools/c2sched.py"], "il": ["tools/c2trace.py", "il"],
         "tsweep": ["tools/tsweep.py"],
         "helpersweep": ["tools/helpersweep.py"], "effectmine": ["tools/effectmine.py", "mine"],
         "schedwant": ["tools/schedwant.py"], "symkeys": ["tools/symkeys.py"],
         "slotwant": ["tools/slotwant.py"],   # frame layout inverse (docs/c2/slotwant.md); gets --image/--va below
         "realize": ["tools/realize.py"],
         "inline": ["tools/c2inline.py"],
         "x87round": ["tools/x87round.py"]}   # rounding points (float32/64 stores) paired by dataflow (docs/c2/x87round.md)     # per-site inliner decisions (docs/c2/inline.md): --func = the row (substring), --callee C     # register-allocation inverse (docs/c2/realize.md): --from scratch|slots, --from-rasim auto, edit EDIT..
ASM = {"regmap", "icount", "tsweep", "helpersweep", "effectmine", "schedwant", "realize", "x87round"}      # take --asm-vs
SLOTS, MAXJOBS = 8, 4
if os.path.exists(os.path.join(ROOT, "work", "locks", "laptop_off")):
    # every traced compile runs under wine on the 10-core Mac: 2026-11-25 three effectmine runs at --jobs 4 each
    # (12 traced cl's plus other lanes) pushed the load to 32 and one compile from 40 s to 8 min
    SLOTS, MAXJOBS = 3, 2
DOC_ALIAS = {"x87round": "x87round", "inline": "inline", "realize": "realize", "slotwant": "slotwant", "tsweep": "tracer", "schedwant": "sched", "helpersweep": "tracer", "regs": "regalloc", "regmap": "regalloc"}   # tsweep's manual is a section of tracer.md ("tsweep")
LOCAL_OK = {"icount"}                                       # tools with --allow-local (compile under wine when the server declines)


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
        name = DOC_ALIAS.get(name, name)
        p = os.path.join(VS, "docs", "c2", name + ".md")
        if not os.path.isfile(p):
            die("no doc %s; have: %s" % (name, " ".join(sorted(f[:-3] for f in os.listdir(os.path.join(VS, "docs", "c2"))))))
        sys.stdout.write(open(p).read()); return
    tool = a[0]
    if tool in TOOLS and len(a) >= 2 and a[1] in ("-h", "--help"):
        # the tool's own help (c2.py TOOL -h parsed -h as the VA and crashed, lane N1-A 2026-11-26)
        sys.exit(subprocess.call([os.path.join(VS, ".venv", "bin", "python")] + TOOLS[tool] + ["-h"], cwd=VS))
    if tool not in TOOLS or len(a) < 2:
        die("usage: c2.py {%s|doc} 0x<va> [--src work/match/F.cpp] [args]" % "|".join(TOOLS))
    va = int(a[1], 16)
    rest = a[2:]
    src = None
    if "--src" in rest:
        i = rest.index("--src"); src = rest[i + 1]; del rest[i:i + 2]
    func = None
    if "--func" in rest:
        i = rest.index("--func"); func = rest[i + 1]; del rest[i:i + 2]
    # the laptop is off (work/locks/laptop_off) or the caller allowed local compiles: let the tool use wine
    if tool in LOCAL_OK and "--allow-local" not in rest and (os.environ.get("MB_RC_ALLOW_LOCAL") == "1"
                                                             or os.path.exists(os.path.join(ROOT, "work", "locks", "laptop_off"))):
        rest.append("--allow-local")
    if tool not in LOCAL_OK and "--allow-local" in rest:   # only icount has the flag; the others would reject it
        rest.remove("--allow-local")
    if any(x.startswith("--c2") or x == "--" for x in rest):
        die("--c2 and the cl flags are filled in by the wrapper; leave them out")
    for opt in ("--orig", "--export", "--keep", "--from", "--out"):     # the tools run in ../vs2005-cpp: a relative path would resolve there
        if opt in rest:
            i = rest.index(opt) + 1
            if tool == "realize" and opt == "--from":      # realize's --from is a mode (slots / scratch), not a file
                continue
            rest[i] = os.path.abspath(rest[i])
    if tool == "realize" and "--from-rasim" in rest:        # an ANSWERS file (not auto / -) resolves from here
        i = rest.index("--from-rasim") + 1
        if i < len(rest) and rest[i] not in ("auto", "-"):
            rest[i] = os.path.abspath(rest[i])
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
    if not func:
        if not d:
            die("chk.py did not give the decorated name (pass --func <decorated name> if chk.py bound it ~size):\n" + c[-800:])
        func = d.group(1)
    if src:
        rel = os.path.normpath(os.path.relpath(os.path.abspath(src), ROOT))
        if not rel.startswith("work/match/") or not os.path.isfile(os.path.join(ROOT, rel)):
            die("--src must be an existing file under work/match/")
        src = rel
    else:
        src = module
    # the original's size: chk.py's second column ("(size ours vs orig)" is printed only when the two differ)
    size = re.search(r"^\s*[0-9a-f]{8}\s+(\d+)\s", c, re.M)
    orig = os.path.join(ROOT, "work", "match", "c2_orig_%08x.s" % va)
    def orig_ok():
        # a cached listing is reused only if it covers exactly this function: one written when chk.py's size was not
        # read (the 4000-byte fallback) ran thousands of bytes past the end, and every tool that diffs against it
        # scored the extra code (0x4b9110: tsweep said 886/889 where chk.py says 2/8)
        try:
            addrs = [int(l.split()[0], 16) for l in open(orig) if l.strip()]
        except (OSError, ValueError):
            return False
        return bool(addrs) and addrs[0] == va and (not size or addrs[-1] < va + int(size.group(1)))
    if tool in ASM and not orig_ok():
        if not size:
            die("chk.py printed no size for 0x%08x, so the original's listing would be guessed:\n%s" % (va, c[-800:]))
        n = int(size.group(1))
        lines = run([PY, "tools/matching/disasm.py", "0x%x" % va, str(n)]).splitlines()
        with open(orig, "w") as fh:
            for ln in lines:
                mm = re.match(r"^\s*(?:0x)?([0-9a-fA-F]{6,8}):?\s+[0-9a-fA-F]+\s+(\S.*)$", ln)
                if mm:
                    fh.write("%08x %s\n" % (int(mm.group(1), 16), re.sub(r"\s+;.*$", "", mm.group(2)).strip()))
    inc = ["/I" + os.path.join(ROOT, os.path.dirname(module)), "/I" + os.path.join(ROOT, "match", "include"),
           "/I" + os.path.join(ROOT, "src"), "/I" + os.path.join(ROOT, "work/ext/dxsdk_jun10/DXSDK/Include")]
    sub = []
    if tool == "symkeys":                     # symkeys.py SUB <src> ...: the subcommand comes before the source
        if not rest or rest[0].startswith("-"):
            die("symkeys needs a subcommand: keys, sites, events, whatif, levers, compare, export")
        sub = [rest.pop(0)]
    if tool == "realize":                     # realize.py takes its mode before the source: edit/measure/pairs or --from X / --from-rasim X
        if rest and rest[0] in ("edit", "measure", "pairs"):
            sub = [rest.pop(0)]
        elif rest and rest[0] in ("--from", "--from-rasim") and len(rest) > 1:
            sub = [rest.pop(0), rest.pop(0)]
        elif "--from" in rest or "--from-rasim" in rest:
            k = rest.index("--from" if "--from" in rest else "--from-rasim"); sub = rest[k:k + 2]; del rest[k:k + 2]
        else:
            die("realize needs a mode: --from scratch|slots, --from-rasim auto, or edit EDIT..")
    # symkeys --from EXPORT reads a saved trace (no compile, seconds): it must not get the source, which it would take
    # as an edit, nor the cl flags. Export once per source edit (c2.py symkeys 0x<va> export --out X.json, in the
    # background: the trace takes minutes on a large row), then run sites / levers / whatif / compare with --from.
    fromx = tool == "symkeys" and "--from" in rest
    cmd = [os.path.join(VS, ".venv", "bin", "python")] + TOOLS[tool] + sub + ([] if fromx else [os.path.join(ROOT, src)])
    cmd += ["--func", func] + ([] if tool in ("inline", "x87round") else ["--c2", "13.10.3052" if tool == "tsweep" else "13.10"])
    if tool == "slotwant" and "--image" not in rest and "--asm-vs" not in rest:
        # the original's slots are read from the image (callee `ret N` for the push depth), as slotwant.md says for M&B
        cmd += ["--image", os.path.join(ROOT, "work", "mb.analysis.bin"), "--va", "0x%x" % va]
    if tool in ASM:
        cmd += ["--asm-vs", orig] + (["--asm-func", func] if tool in ("regmap", "tsweep", "schedwant", "realize", "x87round") else [])
    cmd += rest + ([] if fromx else ["--"] + flags + inc)

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
    env = dict(os.environ)
    if tool in ("inline", "x87round"):
        env.setdefault("C2_BUILD", "13.10.3077")   # c2inline takes the build from the environment, not --c2
    if os.path.exists(os.path.join(ROOT, "work", "locks", "laptop_off")):
        env["C2S_OFF"] = "1"              # the laptop is off: every traced compile goes to wine at once (no server timeout)
    sys.exit(subprocess.run(cmd, cwd=VS, env=env).returncode)


if __name__ == "__main__":
    main()
