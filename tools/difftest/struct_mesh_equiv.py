"""Structured-input driver for equiv.py, written for slice s004d1410 (Mesh::build_first_pass 0x4d1410 /
build_second_pass 0x4d1880). Stock equiv hands the function random pointers, so its std::vector members
(vertices +0x1c, faces +0x2c, edges +0x3c; records of 0x18 / 0x64 / 0x34 bytes) come out empty and the
mesh loops never run (18-23% insn coverage). This builds a consistent random mesh in the input pool for most
inputs (PSTRUCT, default 0.9) and leaves the rest to the stock generator: 96-98% coverage, 0 mismatches.

The record layouts are this class's; a driver for another class copies the pattern (wrap InputGen.fresh, write
the object graph into inp.pool_over, point ecx at it). Generalising it is queued after migration.

usage: .venv/bin/python tools/difftest/struct_mesh_equiv.py <equiv.py args...>
env: MESHMODE=mixed|tri|two, PSTRUCT=0.9, HIST=1 (print every raw mismatch signature)
The equiv verdict cache is disabled here: the key does not see the patched input generator."""
import sys, os, random, struct
os.environ["DIFFTEST_NO_CACHE"] = "1"
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import equiv as E
import machine as M

SL = M.SLOT
B = M.POOL_BASE
VSLOT, FSLOT, ESLOT = 8, 9, 10
MODE = os.environ.get("MESHMODE", "mixed")
P_STRUCT = float(os.environ.get("PSTRUCT", "0.9"))

def f2u(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]

def build(rng, inp):
    over = {}
    def w(a, v):
        over[a & ~3] = v & 0xFFFFFFFF
    nV = rng.randint(4, 9)
    # faces
    nF = rng.randint(1, 4)
    faces = []
    for _ in range(nF):
        n = rng.choice((3, 4, 4, 4)) if MODE != "tri" else 3
        vs = rng.sample(range(nV), n)
        faces.append(vs)
    # edges from faces
    edges = []      # [v0, v1, [faces]]
    eid = {}
    fe = []
    for fi, vs in enumerate(faces):
        ids = []
        for j in range(len(vs)):
            a, b = vs[j], vs[(j + 1) % len(vs)]
            key = (a, b) if (a, b) in eid else ((b, a) if (b, a) in eid else None)
            if key is None:
                eid[(a, b)] = len(edges)
                edges.append([a, b, []])
                key = (a, b)
            e = eid[key]
            edges[e][2].append(fi)
            ids.append(e)
        fe.append(ids)
    # extra loose edges
    for _ in range(rng.choice((0, 0, 1, 2, 3))):
        a, b = rng.sample(range(nV), 2)
        edges.append([a, b, []])
    # flagged verts
    r = rng.random()
    if MODE == "two" or r < 0.7:
        flagged = set(rng.sample(range(nV), 2))
    else:
        flagged = set(i for i in range(nV) if rng.random() < 0.4)
    # vertices
    va = B + VSLOT * SL
    for i in range(nV):
        o = va + 0x18 * i
        for k in range(3):
            w(o + 4 * k, f2u(rng.uniform(-5, 5)))
        w(o + 0x10, (1 if i in flagged else 0) | (rng.getrandbits(24) << 8 if rng.random() < 0.3 else 0))
    fa = B + FSLOT * SL
    for i, vs in enumerate(faces):
        o = fa + 0x64 * i
        n = len(vs)
        for k in range(4):
            w(o + 0x28 + 4 * k, vs[k] if k < n else (-100000 & 0xFFFFFFFF))
            w(o + 0x38 + 4 * k, fe[i][k] if k < n else (-100000 & 0xFFFFFFFF))
        w(o + 0x48, n)
        # normal (+0x18..0x20)
        nv = [rng.uniform(-1, 1) for _ in range(3)]
        for k in range(3):
            w(o + 0x18 + 4 * k, f2u(nv[k]))
        w(o + 0x24, f2u(rng.uniform(-1, 1)))
        w(o + 0x00, 0); w(o + 0x5c, 0)
        w(o + 0x60, 0)
    ea = B + ESLOT * SL
    for i, (a, b, fl) in enumerate(edges):
        o = ea + 0x34 * i
        w(o, a); w(o + 4, b)
        w(o + 8, fl[0] if len(fl) > 0 else -10000)
        w(o + 12, fl[1] if len(fl) > 1 else -10000)
        w(o + 0x10, len(fl) if rng.random() < 0.9 else rng.randint(0, 2))
        w(o + 0x14, rng.choice((0, 0, 1)))
        w(o + 0x18, f2u(1.0)); w(o + 0x1c, f2u(1.0))
        w(o + 0x20, rng.choice((0, 1, 1, 1)))
    # this
    th = B + 1 * SL
    def vec(off, base, n, stride):
        w(th + off, base)
        w(th + off + 4, base + n * stride)
        w(th + off + 8, base + 0x4000)
    vec(0x1c, va, nV, 0x18)
    vec(0x2c, fa, len(faces), 0x64)
    vec(0x3c, ea, len(edges), 0x34)
    w(th + 0x7c, rng.getrandbits(32))
    inp.pool_over.update(over)
    inp.touched = sorted(set(a & ~0xFFF for a in over))
    inp.regs["ecx"] = th
    inp.origin = "struct"

orig_fresh = E.InputGen.fresh
def fresh(self, rng):
    inp = orig_fresh(self, rng)
    if rng.random() < P_STRUCT:
        build(rng, inp)
    return inp
E.InputGen.fresh = fresh

import collections, atexit
HIST = collections.Counter()
_orig_cmp = E.Comparer.compare
def _cmp(self, a, b, t):
    d = _orig_cmp(self, a, b, t)
    if d:
        HIST[E.mismatch_sig(d)] += 1
    return d
E.Comparer.compare = _cmp
def _dump():
    if os.environ.get("HIST"):
        for k, v in HIST.most_common(30):
            print("  raw-diff", v, k)
atexit.register(_dump)

if __name__ == "__main__":
    sys.exit(E.main(sys.argv[1:]))
