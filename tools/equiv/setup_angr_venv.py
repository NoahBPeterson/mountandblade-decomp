#!/usr/bin/env python3
"""Build (or check) the angr venv of tools/equiv from the lock file tools/equiv/angr-requirements.txt.

  python3.12 tools/equiv/setup_angr_venv.py                 # Mac: work/angr-venv
  py -3.12 tools\\equiv\\setup_angr_venv.py                    # Windows: work\\angr-venv
  ... --dest D:\\mb\\angr-venv --python D:\\mb\\py312\\python.exe   # another place / interpreter
  ... --check                                               # only compare an existing venv with the lock file
  ... --recreate                                            # delete the venv first

Standard library only. The interpreter is --python, else the one running this script when it is 3.12, else
`uv python find 3.12`, else python3.12 / `py -3.12` on PATH. The venv gets exactly the lock file's packages
(`pip install --no-deps -r`, so nothing else is resolved), then `pip check` and a version comparison of
`pip freeze` with the lock file (also run by --check). Exit 0 when the venv matches the lock, 1 otherwise.
angr_layer.py finds the venv at work/angr-venv; another location goes in $EQUIV_ANGR_PYTHON.
"""
import argparse, os, re, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LOCK = os.path.join(HERE, "angr-requirements.txt")
WIN = os.name == "nt"


def lock(path=LOCK):
    """-> ({normalised name: version} for this platform, python version wanted)"""
    want, py = {}, None
    for ln in open(path, encoding="utf-8"):
        ln = ln.strip()
        m = re.match(r"#\s*python==(\S+)", ln)
        if m:
            py = m.group(1)
        if not ln or ln.startswith("#"):
            continue
        spec, _, marker = ln.partition(";")
        if "Windows" in marker and not WIN:
            continue
        name, ver = spec.strip().split("==")
        want[norm(name)] = ver.strip()
    return want, py


def norm(name):
    return re.sub(r"[-_.]+", "-", name).lower()


def venv_python(dest):
    return os.path.join(dest, "Scripts", "python.exe") if WIN else os.path.join(dest, "bin", "python")


def find_python(explicit):
    if explicit:
        return explicit
    if sys.version_info[:2] == (3, 12):
        return sys.executable
    for cmd in (["uv", "python", "find", "3.12"],):
        try:
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode == 0 and r.stdout.strip():
                return r.stdout.strip().splitlines()[-1]
        except OSError:
            pass
    for c in ("python3.12",):
        p = shutil.which(c)
        if p:
            return p
    if WIN and shutil.which("py"):
        r = subprocess.run(["py", "-3.12", "-c", "import sys; print(sys.executable)"], capture_output=True, text=True)
        if r.returncode == 0:
            return r.stdout.strip()
    sys.exit("no Python 3.12 found: pass --python (uv python install 3.12 gets one)")


def check(dest):
    """-> list of differences between the venv and the lock file (empty: it matches)"""
    py = venv_python(dest)
    if not os.path.exists(py):
        return ["no venv at %s" % dest]
    want, pyver = lock()
    out = []
    have_py = subprocess.run([py, "-c", "import platform; print(platform.python_version())"], capture_output=True,
                             text=True).stdout.strip()
    if not have_py.startswith("3.12."):
        out.append("python %s, the lock wants 3.12.x" % have_py)
    elif pyver and have_py != pyver:
        print("note: python %s (the lock was frozen with %s; proofs record angr's versions, not python's)"
              % (have_py, pyver))
    fr = subprocess.run([py, "-m", "pip", "freeze", "--all"], capture_output=True, text=True).stdout
    have = {}
    for ln in fr.splitlines():
        if "==" in ln:
            n, v = ln.split("==", 1)
            have[norm(n)] = v.strip()
    for n, v in sorted(want.items()):
        if have.get(n) != v:
            out.append("%s: have %s, lock %s" % (n, have.get(n, "nothing"), v))
    for n in sorted(set(have) - set(want) - {"pip", "setuptools", "wheel"}):
        out.append("%s %s is not in the lock" % (n, have[n]))
    r = subprocess.run([py, "-m", "pip", "check"], capture_output=True, text=True)
    if r.returncode != 0:
        out.append("pip check: " + (r.stdout + r.stderr).strip()[-400:])
    r = subprocess.run([py, "-I", "-c", "import logging; logging.disable(50); import angr, z3, pyvex; from angr import claripy; print(angr.__version__)"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        out.append("import angr failed: " + r.stderr.strip()[-400:])
    elif r.stdout.strip() != want.get("angr"):
        out.append("angr reports version %s" % r.stdout.strip())
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--dest", default=os.path.join(ROOT, "work", "angr-venv"))
    ap.add_argument("--python", help="the Python 3.12 interpreter to build the venv from")
    ap.add_argument("--check", action="store_true", help="only compare an existing venv with the lock file")
    ap.add_argument("--recreate", action="store_true", help="delete an existing venv first")
    a = ap.parse_args()
    dest = os.path.abspath(a.dest)
    if not a.check:
        if a.recreate and os.path.exists(dest):
            shutil.rmtree(dest)
        if not os.path.exists(venv_python(dest)):
            base = find_python(a.python)
            print("creating %s from %s" % (dest, base))
            subprocess.run([base, "-m", "venv", dest], check=True)
        py = venv_python(dest)
        subprocess.run([py, "-m", "pip", "install", "-q", "--disable-pip-version-check", "--no-deps", "-r", LOCK],
                       check=True)
    bad = check(dest)
    if bad:
        print("%s does NOT match %s:" % (dest, os.path.relpath(LOCK, ROOT)))
        for b in bad:
            print("  " + b)
        return 1
    print("%s matches %s (%d packages)" % (dest, os.path.relpath(LOCK, ROOT), len(lock()[0])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
