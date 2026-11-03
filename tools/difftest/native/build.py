#!/usr/bin/env python3
"""Build the equivalence harness's native tracing library (tools/difftest/native/eqhooks.c) into
work/difftest/native/ (gitignored). Portable C99; no unicorn headers or libraries are needed (the unicorn entry points
are handed in at run time).

  python tools/difftest/native/build.py          # build if missing or older than the source
  python tools/difftest/native/build.py --force  # rebuild

Compiler: $CC if set, else cc / clang / gcc on POSIX; on Windows clang, then MSVC cl.exe (from a developer prompt).
equiv.py (tools/difftest/eqnative.py) calls build() on demand and silently uses the pure-Python hooks when there is no
compiler. EQUIV_NATIVE=0 forces the Python hooks; EQUIV_NATIVE=1 makes a missing library an error.
"""
import os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SRC = os.path.join(HERE, "eqhooks.c")
OUTDIR = os.environ.get("EQUIV_NATIVE_DIR") or os.path.join(ROOT, "work", "difftest", "native")
LIB = os.path.join(OUTDIR, "eqhooks.dll" if sys.platform == "win32" else "libeqhooks.dylib" if sys.platform == "darwin"
                   else "libeqhooks.so")


def _fresh():
    return os.path.exists(LIB) and os.path.getmtime(LIB) >= os.path.getmtime(SRC)


def build(force=False, quiet=True):
    """-> the library path, or None when there is no compiler / the build failed."""
    if not force and _fresh():
        return LIB
    os.makedirs(OUTDIR, exist_ok=True)
    tmp = LIB + ".%d.tmp" % os.getpid()
    cands = []
    if os.environ.get("CC"):
        cands.append(os.environ["CC"])
    if sys.platform == "win32":
        cands += ["clang", "cl"]
    else:
        cands += ["cc", "clang", "gcc"]
    for cc in cands:
        exe = shutil.which(cc.split()[0])
        if not exe:
            continue
        if os.path.basename(exe).lower().startswith("cl") and sys.platform == "win32":
            cmd = [exe, "/nologo", "/O2", "/LD", SRC, "/Fe:" + tmp, "/Fo:" + os.path.join(OUTDIR, "eqhooks.obj")]
        else:
            cmd = cc.split() + ["-O2", "-std=c99", "-fPIC", "-fvisibility=hidden", "-shared", SRC, "-o", tmp]
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        except (OSError, subprocess.SubprocessError):
            continue
        if r.returncode == 0 and os.path.exists(tmp):
            os.replace(tmp, LIB)
            return LIB
        if not quiet:
            sys.stderr.write(r.stdout + r.stderr)
    try:
        os.unlink(tmp)
    except OSError:
        pass
    return None


if __name__ == "__main__":
    p = build(force="--force" in sys.argv, quiet=False)
    print(p or "no usable C compiler (the pure-Python hooks are used)")
    sys.exit(0 if p else 1)
