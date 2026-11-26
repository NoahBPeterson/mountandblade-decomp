#!/usr/bin/env python3
"""insdiff.py: count the instructions that really differ between our compile of a row and the original.

    MB_RC_ALLOW_LOCAL=1 .venv/bin/python tools/matching/insdiff.py SRC.cpp 0x<va> [--frame] [--show] [--symbol=MANGLED]

chk.py's two figures (diff, aligned) are byte counts. On a big row, one 4-byte frame difference changes every
[esp+N] and swamps both, so an edit that fixes real instructions can look like no change. This compiles the row once
(chk.py --keep-obj), disassembles both sides (cmpdis.py), normalises each instruction and aligns the two sequences:

  relocations     call / jump targets, absolute addresses and pushed or loaded addresses become placeholders (always)
  --frame         also mask esp-relative offsets, the frame size in sub/add esp, and the frame constant moved for
                  __chkstk: use it when the frame size differs and you want the real instruction differences only

It prints `orig-only N  ours-only M`. N + M is the number of instructions that would still
have to change. --show prints the aligned unified diff of the normalised instructions (only the differing runs, 2
lines of context). The figure is a steering aid, as chk.py's aligned is; MATCH is still chk.py's call.

From lane W7-C2's helper (2026-11-25), which steered Party::tick 738 -> 163 aligned by it.
"""
import difflib, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = os.path.join(ROOT, ".venv", "bin", "python")


def norm(x, frame):
    x = re.sub(r'\[(?:<addr>|0x[0-9a-f]+|0)\]', '[A]', x)
    x = re.sub(r'\[(\w+)\*(\d)(?: \+ (?:<addr>|0x[0-9a-f]+|\d+))?\]', r'[\1*\2+A]', x)
    x = re.sub(r'^(j\w+|call) <.*>$', r'\1 T', x)
    x = re.sub(r'^push <addr>$', 'push A', x)
    x = re.sub(r'^mov (\w+), <addr>$', r'mov \1, A', x)
    # our object is unrelocated: a relocated immediate reads as its addend (0) where the original has the address
    x = re.sub(r'^push 0$', 'push A', x)
    x = re.sub(r'^mov (\w+), 0$', r'mov \1, A', x)
    if frame:
        x = re.sub(r'esp \+ 0x[0-9a-f]+', 'esp+N', x)
        x = re.sub(r'esp \+ (\w+)\*4 \+ 0x[0-9a-f]+', r'esp+\1*4+N', x)
        x = re.sub(r'^mov eax, 0x[0-9a-f]+$', 'mov eax, FRAME', x)
        x = re.sub(r'^(add|sub) esp, 0x[0-9a-f]{3,}$', r'\1 esp, FRAME', x)
    return x


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    # --symbol=MANGLED: the object symbol to compare (passed to chk.py), for twins the marker binds wrongly
    sym = [a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--symbol=")]
    if len(args) != 2:
        sys.exit(__doc__)
    src, va = args
    frame, show = "--frame" in sys.argv, "--show" in sys.argv
    env = dict(os.environ)
    with tempfile.TemporaryDirectory(dir=os.path.join(ROOT, "work", "match")) as d:
        obj = os.path.join(d, "row.obj")
        r = subprocess.run([PY, os.path.join(ROOT, "tools/matching/chk.py"), "--keep-obj", obj, src, va]
                           + (["--symbol=%s=%s" % (va, sym[0])] if sym else []),
                           cwd=ROOT, env=env, capture_output=True, text=True)
        row = [l for l in r.stdout.splitlines() if re.match(r'^[0-9a-f]{8} ', l)]
        m = re.search(r'(?:marker|manif|~size|other|given)\s+(\S+)', row[0]) if row else None
        if not m or not os.path.exists(obj):
            sys.exit("insdiff: chk.py gave no row / object:\n" + (r.stdout + r.stderr)[-1500:])
        size = re.search(r'\(size [^)]*\)', row[0])
        print(re.split(r"  (?:marker|manif|~size|other|given) ", row[0])[0] + ("  " + size.group(0) if size else ""))
        r2 = subprocess.run([PY, os.path.join(ROOT, "tools/matching/cmpdis.py"), obj, m.group(1),
                             va.lower().replace("0x", "")], cwd=ROOT, env=env, capture_output=True, text=True)
        if r2.returncode:
            sys.exit("insdiff: cmpdis failed:\n" + (r2.stdout + r2.stderr)[-1500:])
    a, b = [], []
    for l in r2.stdout.split("\n"):
        if l.startswith(("---", "+++", "@@")):
            continue
        if l.startswith("-"):
            a.append(norm(l[1:].strip(), frame))
        elif l.startswith("+"):
            b.append(norm(l[1:].strip(), frame))
        elif l.startswith(" "):
            x = norm(l[1:].strip(), frame)
            a.append(x); b.append(x)
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    same = sum(x.size for x in sm.get_matching_blocks())
    # cmpdis prints only the differing hunks (2 lines of context), so these are the instructions that differ, not
    # totals of the function
    print("orig-only %d  ours-only %d%s" % (len(a) - same, len(b) - same, "  [frame offsets masked]" if frame else ""))
    if show:
        print("\n".join(difflib.unified_diff(a, b, "orig", "ours", n=2, lineterm="")))


if __name__ == "__main__":
    main()
