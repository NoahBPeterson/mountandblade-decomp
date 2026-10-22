"""Names of functions from slice manifests and symbols (for reports)."""
import glob, os, re
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def names():
    nm = {}
    for f in glob.glob(os.path.join(ROOT, "symbols/slices/*.txt")):
        for line in open(f):
            p = line.split("#", 1)[0].split()
            if len(p) >= 2 and re.fullmatch(r"[0-9a-fA-F]{6,8}", p[0]):
                nm.setdefault(int(p[0], 16), p[1])
    for f in glob.glob(os.path.join(ROOT, "match/slices/*/manifest.txt")) + \
            glob.glob(os.path.join(ROOT, "src/*/*.manifest.txt")):
        for line in open(f):
            p = line.split()
            if len(p) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", p[2]):
                nm[int(p[2], 16)] = p[1]
    return nm
