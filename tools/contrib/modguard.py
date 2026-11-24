#!/usr/bin/env python3
"""modguard: re-verify every manifest row of a source file (the whole-module guard).

  .venv/bin/python tools/contrib/modguard.py src/game/ui/widgets/drop_down_list.cpp [--cpp VARIANT.cpp]

chk.py scores one function, so a variant can improve its target while another row of the same file stops emitting
(a bodiless specialization turns the explicit instantiation into a declaration: 0x49ad20, codegen.md 17.1). This
lists the file's rows from the manifests and runs run_all.py --filter on each distinct file name. With --cpp the
variant is copied over the file for the run and the original text is always restored. Exit 1 if any row fails.
"""
import argparse, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = os.path.join(ROOT, ".venv", "bin", "python")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("--cpp")
    a = ap.parse_args()
    src = os.path.join(ROOT, a.src)
    if not os.path.isfile(src):
        sys.exit("modguard: %s not found" % a.src)
    keep = None
    if a.cpp:
        keep = open(src, "rb").read()
        shutil.copyfile(a.cpp, src)
    try:
        env = dict(os.environ, MB_RC_ALLOW_LOCAL="1")
        r = subprocess.run([PY, "tools/matching/run_all.py", "--filter", os.path.basename(a.src)], cwd=ROOT,
                           capture_output=True, text=True, env=env)
    finally:
        if keep is not None:
            open(src, "wb").write(keep)
    out = r.stdout + r.stderr
    bad = [l for l in out.splitlines() if re.search(r"FAIL|MISMATCH|not found|missing|error", l, re.I)]
    print("\n".join(out.splitlines()[-6:]))
    if bad or r.returncode:
        print("modguard: %d problem line(s)" % len(bad))
        print("\n".join(bad[:15]))
        sys.exit(1)
    print("modguard: all rows of %s verified" % a.src)


if __name__ == "__main__":
    main()
