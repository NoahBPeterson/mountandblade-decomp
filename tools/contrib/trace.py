#!/usr/bin/env python3
"""trace: ask the traced compiler why a near miss allocates registers or stack slots differently.

  .venv/bin/python tools/contrib/trace.py 0x<va> regs     which of our variables has the wrong register
  .venv/bin/python tools/contrib/trace.py 0x<va> slots    how cl packed the stack frame, against the original's slots

Use it when the diff (check.py) is a register swap or different [esp+N] offsets and the instructions otherwise agree.
It runs vs2005-cpp's c2trace.py (the tracer of cl 13.10's c2.dll: ../vs2005-cpp, docs/c2/tracer.md there) on the
row's own source and flags, with the original's disassembly beside it, and prints a short report:

  regs   per variable (the compiler's names: _this, s386 = local386, t591 = a temporary): our register, the
         original's register, and "ok" / "WANTS r" / "mixed". Then the swaps (A has what B wants and B has what A
         wants): usually an order question. Of two values that compete, the one coloured first gets the first free
         register, and priority grows with uses (more so in loops). Change which value is used more, or where it
         is first used.
  slots  per stack slot: offset, size, uses and members, in the order cl's packer visited them (size ascending, then
         uses descending), then the original's slot map. Frames under 128 bytes keep creation order; larger frames
         sort by 1000*uses/size. More or fewer uses of a variable, a separate variable instead of a reused one (or the
         reverse), or a different declaration order moves slots.

Takes 30 s to a few minutes (the traced compile runs on the laptop). Writes only work/match/trace/<lane>/trace_<va>* (lane: $MB_LANE, else the checkout's
directory name). Read-only
for the source.
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
V = os.path.join(os.path.dirname(ROOT), "vs2005-cpp")
sys.path[:0] = [os.path.join(ROOT, "tools", "contrib"), os.path.join(ROOT, "tools", "equiv")]
import task as T          # noqa: E402

LINES = {"regs": 70, "slots": 90}
TC = os.path.join(ROOT, "work", "toolchain")
INC = ["/I" + os.path.join(ROOT, "src", "include"), "/I" + os.path.join(ROOT, "match", "include"),
       "/I" + os.path.join(TC, "vc71tk", "include"), "/I" + os.path.join(TC, "sal"),
       "/I" + os.path.join(TC, "vc9sp1", "WinSDK", "Include"),
       "/I" + os.path.join(ROOT, "work", "ext", "dxsdk_jun10", "DXSDK", "Include")]


def orig_listing(va, out):
    """The original's disassembly as 'VA mnemonic operands' lines (c2trace's --asm-vs / --orig format)."""
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "matching", "disasm.py"), "0x%x" % va],
                       cwd=ROOT, capture_output=True, text=True)
    lines = []
    for l in r.stdout.splitlines():
        p = l.split()
        if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{6,8}:?", p[0]):
            lines.append(" ".join([p[0].rstrip(":"), p[2]] + p[3:]))
    if not lines:
        sys.exit("trace: no disassembly for 0x%x\n%s" % (va, r.stderr[-500:]))
    open(out, "w").write("\n".join(lines) + "\n")


def prepare(va):
    """(src, decorated symbol, c2trace --func substring, cl flags for c2trace, work/match/trace_<va> base) of a row;
    writes the original's listing to <base>.s."""
    hit = [r for r in T.rows() if r[0] == va]
    if not hit:
        sys.exit("trace: 0x%08x is not a near-miss row (no .nonmatching.txt lists it)" % va)
    src = hit[0][3]
    import func as F
    cflags = list(T.flags_for(src))                      # the row's flags, as check.py compiles it
    obj, _ = F.compile_src(src, cflags)
    sym = F.find_symbol(obj, src, va)
    # c2trace runs in vs2005-cpp: every include path absolute, plus the source's directory and the toolchain's
    flags = [("/I" + os.path.join(ROOT, f[2:])) if f.startswith("/I") and not os.path.isabs(f[2:]) else f
             for f in cflags] + ["/I" + os.path.dirname(src)] + INC
    # c2trace --func takes a substring of the decorated name: "name@Class" from "?name@Class@@..."
    fsub = sym.lstrip("?").split("@@")[0]
    # per lane: work/ is shared by every worktree (a symlink), so two lanes tracing one row must not share files
    lane = os.environ.get("MB_LANE") or os.path.basename(ROOT)
    base = os.path.join(ROOT, "work", "match", "trace", lane, "trace_%08x" % va)
    os.makedirs(os.path.dirname(base), exist_ok=True)
    orig_listing(va, base + ".s")
    return src, sym, fsub, flags, base


def main():
    if len(sys.argv) < 3 or sys.argv[2] not in LINES and sys.argv[2] not in ("c2", "icount") or sys.argv[2] == "c2" and len(sys.argv) < 4:
        sys.exit(__doc__.split("\n\n")[0] + "\nusage: trace.py 0x<va> regs|slots|icount [options]   (icount: which c2 pass made or removed each extra /"
                 " missing instruction, vs2005-cpp tools/icount.py)"
                 "\n       trace.py 0x<va> c2 SUBCOMMAND [options]   (any c2trace subcommand on the row: regs --why '*',"
                 " regs --export F, scratch --why, scratch --color, merges, il ...)")
    va, what, extra = int(sys.argv[1], 16), sys.argv[2], sys.argv[3:]
    src, sym, fsub, flags, base = prepare(va)
    env = dict(os.environ, C2TRACE_REMOTE=os.path.join(ROOT, "tools", "remote", "rcl.py"))
    if what == "regs":
        cmd = ["uv", "run", "tools/c2trace.py", "regmap", "--c2", "13.10", src, "--func", fsub,
               "--asm-vs", base + ".s", "--asm-func", sym] + extra + ["--"] + flags
    elif what == "slots":
        cmd = ["uv", "run", "tools/c2trace.py", "slots", "--c2", "13.10", src, "--func", fsub,
               "--orig", base + ".s"] + extra + ["--"] + flags
    elif what == "icount":                               # vs2005-cpp tools/icount.py: which c2 pass made / removed each
        cmd = ["uv", "run", "tools/icount.py", "--c2", "13.10", src, "--func", fsub,       # extra / missing instruction
               "--asm-vs", base + ".s", "--asm-func", sym] + extra + ["--"] + flags
        env["C2TRACE_FUNC"] = fsub                       # trace only this function (a big TU's log shrinks ~40x)
    else:                                                # raw passthrough: c2 SUBCOMMAND [options]
        what = extra[0]
        cmd = ["uv", "run", "tools/c2trace.py", what, "--c2", "13.10", src, "--func", fsub] + extra[1:] + ["--"] + flags
    print("trace %s 0x%08x  %s  (%s)" % (what, va, sym, os.path.relpath(src, ROOT)), flush=True)
    try:
        r = subprocess.run(cmd, cwd=V, env=env, capture_output=True, text=True, timeout=900)
    except subprocess.TimeoutExpired:
        sys.exit("trace: the traced compile took over 15 minutes; give up on tracing this row")
    out = (r.stdout + ("\n" + r.stderr if r.returncode else "")).replace(V + os.sep, "").replace(ROOT + os.sep, "")
    open(base + "_" + what + ".txt", "w").write(out)
    lines = out.splitlines()
    print("\n".join(lines[:LINES.get(what, 120)]))
    if len(lines) > LINES.get(what, 120):
        print("... %d more lines in %s_%s.txt (read with rg or sed -n)" % (
            len(lines) - LINES.get(what, 120), os.path.relpath(base, ROOT), what))
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
