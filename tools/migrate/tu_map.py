#!/usr/bin/env python3
"""Infer the original translation units (.obj files) of mount&blade.exe's game code from link order.

usage: tu_map.py [--rebuild-facts]

Inputs: work/index/functions.csv, work/mb.analysis.bin (via facts.py -> work/migrate/facts.json),
        match/slices/*/{manifest,nonmatching,partial}.txt, work/slices.json, symbols/slices/*.txt,
        src/*/*.manifest.txt.
Outputs (work/migrate/):
  tu_map.csv          one row per inferred TU
  tu_functions.csv    one row per function (game code, import thunks, post-CRT EH stubs / dynamic
                      initializers / atexit thunks) with its TU
  tu_boundaries.csv   every boundary and every unresolved candidate gap with its evidence
  tu_validation.txt   the checks against known TUs (DirectX SDK framework, TinyXML)

Model and signal measurements: docs/tu_map.md.
"""
import bisect, collections, csv, glob, json, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import facts as factsmod
from mbnames import names as load_names

ROOT = os.path.dirname(os.path.dirname(HERE))
W = lambda *p: os.path.join(ROOT, *p)
OUT = W("work/migrate")

GAME_LO, GAME_HI = 0x401000, 0x5342a6        # game code; import thunks + dinput8.lib data follow
LIB_HI = 0x535440                             # static CRT 0x535440..0x547100 (not mapped)
POST_LO = 0x547100                            # .text$x EH stubs, ??__E initializers, atexit thunks
IAT_HI = 0x5494c8                             # .rdata starts with the IAT
XC_LO, XC_HI = 0x5a8000, 0x5a80c4             # .CRT$XCA..XCZ (VC7.1 merges .CRT into .data)
DINIT_HI = 0x5d7000                           # end of initialized .data (raw size); .bss after

STRONG, MEDIUM, WEAK = 100.0, 10.0, 3.0

# known TUs (validation): (name, first va, end va)
KNOWN = {
    "dxframework": [("d3dapp", 0x401000, 0x4035d0), ("d3denumeration", 0x4035d0, 0x404500),
                    ("d3dfont", 0x404500, 0x4055e0), ("d3dsettings", 0x4055e0, 0x407250),
                    ("d3dutil", 0x407250, 0x407660), ("dxutil", 0x407660, 0x407c10)],
    # TinyXML 2.5 (source known; boundaries from the class/function split of its files, not from a map)
    "tinyxml": [("tinystr", 0x42d820, 0x42d9a0), ("tinyxml", 0x42d9a0, 0x42f720),
                ("tinyxmlparser", 0x42f720, 0x431400)],
}


# ----------------------------------------------------------------------------------------------- inputs
def slice_status():
    """va -> exact / nonmatching / partial; plus va -> slice id."""
    st = {}
    for path in glob.glob(W("match/slices/*/manifest.txt")) + glob.glob(W("src/*/*.manifest.txt")):
        for line in open(path):
            q = line.split()
            if len(q) >= 3 and re.fullmatch(r"[0-9a-fA-F]{8}", q[2]):
                st[int(q[2], 16)] = "exact"
    for kind in ("nonmatching", "partial"):
        for path in glob.glob(W("match/slices/*/%s.txt" % kind)):
            for line in open(path):
                q = line.split()
                if q and re.fullmatch(r"(0x)?[0-9a-fA-F]{6,8}", q[0]):
                    st.setdefault(int(q[0], 16), kind)
    inslice = {}
    try:
        for s in json.load(open(W("work/slices.json"))):
            for f in s["functions"]:
                inslice[int(f["va"], 16)] = s["id"]
    except OSError:
        pass
    return st, inslice


def scc_ids(n, edges):
    """Tarjan (iterative) over nodes 0..n-1: node -> component id."""
    index, low, on, stack, comp = {}, {}, set(), [], {}
    c = 0
    for root in range(n):
        if root in index:
            continue
        index[root] = low[root] = c; c += 1
        stack.append(root); on.add(root)
        work = [(root, iter(edges.get(root, ())))]
        while work:
            v, it = work[-1]
            for w in it:
                if w not in index:
                    index[w] = low[w] = c; c += 1
                    stack.append(w); on.add(w)
                    work.append((w, iter(edges.get(w, ()))))
                    break
                if w in on:
                    low[v] = min(low[v], index[w])
            else:
                work.pop()
                if work:
                    low[work[-1][0]] = min(low[work[-1][0]], low[v])
                if low[v] == index[v]:
                    while True:
                        w = stack.pop(); on.discard(w); comp[w] = v
                        if w == v:
                            break
    return comp


# ----------------------------------------------------------------------------------------- names
GENERIC_CLASS = re.compile(
    r"^(rgl|std|tstring|string|vector4|matrix|quaternion|pos|Viterator|iterator|allocator|"
    r"[A-Z]?vec\w*|Vec\w*|alloc\w*|iter\w*|PA\w*|U[A-Z]\w*|DU\w*|E\d+|Rec[A-Z0-9]?\w{0,2}|"
    r"VecPair|ObjVec|RecList|logic_error|length_error|out_of_range|_Container_base|Ctx|Mgr|Block)$")


def classes_of(name):
    """class qualifier of a mangled or 'A::b' name, or a C prefix ('DXUtil_Timer' -> 'DXUtil')."""
    if not name:
        return None
    if name.startswith("?"):
        m2 = re.match(r"\?\?(?:_[0-9A-Z]|[0-9A-Z])([A-Za-z_][A-Za-z0-9_]*)@", name)
        if m2:
            return m2.group(1)
        m = re.match(r"\?([^@?$][^@]*)@([^@?$][^@]*)@", name)
        if m:
            return m.group(2)
        m = re.match(r"\?([A-Z][A-Za-z0-9]+)_[A-Za-z]", name)   # global C-style function
        return m.group(1) if m else None
    if "::" in name:
        return name.split("::")[0]
    m = re.match(r"([A-Z][A-Za-z0-9]+)_[A-Za-z]", name)
    return m.group(1) if m else None


def substantive(cls, freq):
    return bool(cls) and freq.get(cls, 0) >= 2 and not GENERIC_CLASS.match(cls) and "$" not in cls


def snake(s):
    s = re.sub(r"^C(?=[A-Z][A-Z0-9])", "", s)      # CD3DFont -> D3DFont
    s = re.sub(r"(?<=[a-z])([A-Z])", r"_\1", s)
    return re.sub(r"[^A-Za-z0-9_]+", "_", s).lower().strip("_")


# ---------------------------------------------------------------------------------------- analysis
class Analysis:
    def __init__(self, d, nm):
        self.d, self.nm = d, nm
        self.F = [f for f in d["funcs"] if GAME_LO <= f["va"] < GAME_HI]
        self.N = len(self.F)
        self.pos = {f["va"]: i for i, f in enumerate(self.F)}
        self.vas = [f["va"] for f in self.F]
        self.ends = [f["end"] for f in self.F]
        self.callers = collections.defaultdict(set)
        edges = collections.defaultdict(list)
        for i, f in enumerate(self.F):
            for t in f["calls"] + f["tails"]:
                j = self.pos.get(t)
                if j is not None and j != i:
                    self.callers[j].add(i)
                    edges[i].append(j)
        self.scc = scc_ids(self.N, edges)
        self.glue = [0.0] * (self.N + 1)       # glue[g]: weight against a boundary in gap g (g-1 | g)
        self.glue_src = collections.defaultdict(collections.Counter)
        self.forced = []
        self.log = {}
        freq = collections.Counter()
        self.cls = []
        for va in self.vas:
            c = classes_of(nm.get(va))
            self.cls.append(c)
            if c:
                freq[c] += 1
        self.freq = freq

    def add_glue(self, i, j, w, src):
        i, j = min(i, j), max(i, j)
        for g in range(i + 1, j + 1):
            self.glue[g] += w
            self.glue_src[g][src] += 1

    # placement regime: B = bottom-up (callee before caller), T = caller first
    def regimes(self, near=12, smooth=25):
        lab = []
        for i in range(self.N):
            cs = [c for c in self.callers[i] if abs(c - i) <= near]
            before, after = any(c < i for c in cs), any(c > i for c in cs)
            lab.append(1 if after and not before else -1 if before and not after else 0)
        pre = [0]
        for x in lab:
            pre.append(pre[-1] + x)
        self.regime = []
        for i in range(self.N):
            s = pre[min(self.N, i + smooth + 1)] - pre[max(0, i - smooth)]
            self.regime.append("B" if s > 0 else "T" if s < 0 else "?")

    # -------------------------------------------------------------------------------------- glue
    def items(self, lo, hi, key="data"):
        m = collections.defaultdict(set)
        for i, f in enumerate(self.F):
            for r in f[key]:
                if lo <= r < hi:
                    m[r].add(i)
        return m

    def glue_inversions(self, m, w, src, span=0x6000):
        its = [r for r in sorted(m) if len(m[r]) == 1]
        p = [next(iter(m[r])) for r in its]
        n = 0
        for k in range(len(p) - 1):
            a, b = p[k], p[k + 1]
            # elements of one array referenced at different offsets are not two variables
            if its[k + 1] - its[k] < 0x10:
                continue
            if b < a and self.vas[a] - self.vas[b] <= span:
                self.add_glue(b, a, w, src); n += 1
        return n

    def glue_shared(self, m, w, src, span):
        n = 0
        for r, ps in m.items():
            if len(ps) >= 2:
                lo, hi = min(ps), max(ps)
                if self.vas[hi] - self.vas[lo] <= span:
                    self.add_glue(lo, hi, w, src); n += 1
        return n

    def glue_regargs(self, span=0x8000):
        n = 0
        for i, f in enumerate(self.F):
            if any(r in ("eax", "edx", "ebx", "esi", "edi") for r in f["regargs"]) and self.callers[i]:
                ps = self.callers[i] | {i}
                if self.vas[max(ps)] - self.vas[min(ps)] <= span:
                    self.add_glue(min(ps), max(ps), STRONG, "regconv"); n += 1
        return n

    def glue_helpers(self, maxpos=24, maxcallers=4):
        """few-caller helper next to its callers in the regime's direction (BU: helper first)."""
        n = 0
        for i in range(self.N):
            cs = self.callers[i]
            if not cs or len(cs) > maxcallers:
                continue
            if self.regime[i] == "B" and all(0 < c - i <= maxpos for c in cs):
                self.add_glue(i, max(cs), WEAK, "helper"); n += 1
            elif self.regime[i] == "T" and all(0 < i - c <= maxpos for c in cs):
                self.add_glue(min(cs), i, WEAK, "helper"); n += 1
        return n

    def glue_vtables(self, span=0x6000):
        """slots that belong to one vtable only (the class's own overrides) live in one TU."""
        fp = self.d["rdata_fptrs"]
        refd = set()
        for f in self.d["funcs"]:
            refd.update(f["rdata"])
        tables, cur = [], None
        for a in sorted(fp):
            if cur is not None and a == cur[-1] + 4 and a not in refd:
                cur.append(a)
            else:
                cur = [a]; tables.append(cur)
        tables = [t for t in tables if t[0] in refd and len(t) >= 2]
        owners = collections.Counter()
        for t in tables:
            for a in set(fp[x] for x in t):
                owners[a] += 1
        n = 0
        self.vtables = []
        for t in tables:
            own = sorted(self.pos[fp[x]] for x in t if fp[x] in self.pos and owners[fp[x]] == 1)
            self.vtables.append((t[0], len(t), [self.vas[p] for p in own]))
            if len(own) >= 2 and self.vas[own[-1]] - self.vas[own[0]] <= span:
                self.add_glue(own[0], own[-1], WEAK, "vtable"); n += 1
        return n

    def glue_strings(self, span=0x3000):
        """a string literal (pooled by /GF) used by several nearby functions."""
        pe_data = self.d.get("_rdata_bytes")
        m = self.items(IAT_HI, XC_LO, "rdata")
        n = 0
        for r, ps in m.items():
            if len(ps) >= 2 and self.is_string(r):
                lo, hi = min(ps), max(ps)
                if self.vas[hi] - self.vas[lo] <= span:
                    self.add_glue(lo, hi, WEAK, "string"); n += 1
        return n

    def is_string(self, va):
        b = self.rdata[va - self.rlo: va - self.rlo + 64]
        s = b.split(b"\0")[0]
        return len(s) >= 4 and all(32 <= c < 127 or c in (9, 10, 13) for c in s)

    def glue_nearcalls(self, maxpos=6):
        n = 0
        for j, cs in self.callers.items():
            for i in cs:
                if abs(i - j) <= maxpos and (self.regime[j] != "B" or j < i):
                    self.add_glue(i, j, WEAK, "nearcall"); n += 1
        return n

    # --------------------------------------------------------------------------------- boundaries
    def class_run(self, g, cls, reach=8):
        """named functions of class cls among the reach functions left (g<0: right) of gap |g|."""
        if g > 0:
            rng = range(max(0, g - reach), g)
        else:
            rng = range(-g, min(self.N, -g + reach))
        ks = [k for k in rng if self.cls[k] == cls]
        # a run made only of small functions may be inline header code emitted in a neighbour's TU
        return len(ks) if any(self.F[k]["size"] >= 0x50 for k in ks) else 0

    def class_change(self, g, reach=6):
        """substantive class of the nearest named function on each side of gap g (within reach)."""
        left = right = None
        for k in range(g - 1, max(-1, g - 1 - reach), -1):
            if substantive(self.cls[k], self.freq):
                left = self.cls[k]; break
        for k in range(g, min(self.N, g + reach)):
            if substantive(self.cls[k], self.freq):
                right = self.cls[k]; break
        return left, right

    def is_deleting_dtor(self, i):
        """compiler-generated scalar/vector deleting destructor (??_G / ??_E): emitted with the vtable,
        before the destructor it calls, so its forward call says nothing about TUs."""
        f = self.F[i]
        if f["size"] > 0xc0 or not (set(f["calls"]) & {0x535712, 0x53570d}):
            return False
        code = self.pe.get_data(f["va"] - 0x400000, f["size"])
        return b"\xf6\x44\x24\x08\x01" in code or b"\xf6\x44\x24\x08\x02" in code

    def forced_intervals(self):
        for i, f in enumerate(self.F):
            if self.regime[i] != "B" or self.is_deleting_dtor(i):
                continue
            for t in f["calls"] + f["tails"]:
                j = self.pos.get(t)
                if j is None or j <= i or self.scc[i] == self.scc[j]:
                    continue
                self.forced.append((i + 1, j, i, j))

    def place_forced(self):
        cover = [0.0] * (self.N + 1)
        for a, b, _, _ in self.forced:
            for g in range(a, b + 1):
                cover[g] += 1.0 / (b - a + 1)
        self.cover = cover
        chosen, out, conflicts = [], {}, []
        for a, b, ci, ti in sorted(self.forced, key=lambda x: (x[1] - x[0], x[0])):
            k = bisect.bisect_left(chosen, a)
            if k < len(chosen) and chosen[k] <= b:
                continue
            gaps = [g for g in range(a, b + 1) if self.glue[g] < MEDIUM]
            if not gaps:
                gaps = [g for g in range(a, b + 1) if self.glue[g] < STRONG]
                if not gaps:
                    conflicts.append((self.vas[ci], self.vas[ti]))
                    continue

            def key(g):
                l, r = self.class_change(g)
                cc = 1 if (l and r and l != r) else 0
                return (self.glue[g], -cc, -cover[g], -g)
            best = min(gaps, key=key)
            bisect.insort(chosen, best)
            out[best] = {"why": "fwd-call %x->%x" % (self.vas[ci], self.vas[ti]),
                         "interval": (a, b)}
        self.conflicts = conflicts
        return out

    def place_class_changes(self, bounds):
        """transitions between runs of two different substantive classes, with a free gap between."""
        named = [(i, self.cls[i]) for i in range(self.N) if substantive(self.cls[i], self.freq)]
        out = {}
        for (i, a), (j, b) in zip(named, named[1:]):
            if a == b or j - i > 12:
                continue
            gaps = [g for g in range(i + 1, j + 1) if self.glue[g] < MEDIUM]
            if not gaps or any(i < x <= j for x in bounds):
                continue
            # both classes must form a run (>= 2 named functions) on their side of the transition
            if self.class_run(i + 1, a) < 2 or self.class_run(-j, b) < 2:
                continue
            best = min(gaps, key=lambda g: (self.glue[g], -g))
            out[best] = {"why": "class %s|%s" % (a, b), "interval": (i + 1, j)}
        return out

    def place_regime_switches(self, bounds):
        out = {}
        for g in range(1, self.N):
            r0, r1 = self.regime[g - 1], self.regime[g]
            if r0 != r1 and "?" not in (r0, r1):
                lo, hi = max(1, g - 3), min(self.N - 1, g + 3)
                if any(lo <= x <= hi for x in list(bounds) + list(out)):
                    continue
                best = min(range(lo, hi + 1), key=lambda x: (self.glue[x], abs(x - g)))
                if self.glue[best] < MEDIUM:
                    out[best] = {"why": "regime %s>%s" % (r0, r1), "interval": (lo, hi)}
        return out

    def forced_near(self, g, width=20):
        return any(a <= g <= b and b - a <= width for a, b, _, _ in self.forced)

    def windows(self, bounds):
        bs = sorted(bounds)
        for k, g in enumerate(bs):
            a, b = bounds[g]["interval"]
            prevb = bs[k - 1] if k else 0
            nextb = bs[k + 1] if k + 1 < len(bs) else self.N
            lo = hi = g
            while lo - 1 >= max(a, prevb + 1) and self.glue[lo - 1] < MEDIUM:
                lo -= 1
            while hi + 1 <= min(b, nextb - 1) and self.glue[hi + 1] < MEDIUM:
                hi += 1
            bounds[g]["win"] = (lo, hi)
            free = [x for x in range(lo, hi + 1) if self.glue[x] == 0]
            why = bounds[g]["why"]
            l, r = self.class_change(g)
            cc = bool(l and r and l != r)
            if why.startswith("fwd") and (hi == lo or (cc and len(free) <= 1)):
                conf = "high"
            elif why.startswith("fwd") and hi - lo <= 3:
                conf = "medium"
            elif why.startswith("class") and cc and self.forced_near(g):
                conf = "medium"   # class change inside a forward-call interval
            else:
                conf = "low"
            bounds[g]["conf"] = conf
            bounds[g]["classes"] = (l, r)
        return bs


# ------------------------------------------------------------------------------------------- run
def analyse(rebuild=False):
    d = factsmod.load(rebuild)
    nm = load_names()
    A = Analysis(d, nm)
    import pefile
    pe = pefile.PE(factsmod.IMAGE, fast_load=True)
    A.rlo, rhi = d["sections"][".rdata"]
    A.rdata = pe.get_data(A.rlo - 0x400000, rhi - A.rlo)
    A.pe = pe
    A.regimes()
    st = A.log
    bss = A.items(DINIT_HI, 1 << 32)
    dat = A.items(XC_HI, DINIT_HI)
    rdt = A.items(IAT_HI, XC_LO, "rdata")
    st["glue: .bss order inversions"] = A.glue_inversions(bss, STRONG, "bss-order")
    st["glue: .data order inversions"] = A.glue_inversions(dat, STRONG, "data-order")
    st["glue: .rdata order inversions"] = A.glue_inversions(rdt, MEDIUM, "rdata-order")
    st["glue: shared .bss statics"] = A.glue_shared(bss, WEAK, "bss-static", 0x4000)
    st["glue: shared .data statics"] = A.glue_shared(dat, WEAK, "data-static", 0x4000)
    st["glue: register-convention callees"] = A.glue_regargs()
    st["glue: vtables"] = A.glue_vtables()
    st["glue: shared strings"] = A.glue_strings()
    st["glue: few-caller helpers"] = A.glue_helpers()
    st["glue: near calls"] = A.glue_nearcalls()
    A.forced_intervals()
    st["forward-call intervals (BU callers)"] = len(A.forced)
    bounds = A.place_forced()
    st["fwd-call boundaries"] = len(bounds)
    cc = A.place_class_changes(bounds)
    bounds.update(cc)
    st["class-change boundaries"] = len(cc)
    rs = A.place_regime_switches(bounds)
    bounds.update(rs)
    st["regime-switch boundaries"] = len(rs)
    st["forced intervals with no free gap (glue conflicts)"] = len(A.conflicts)
    A.bounds = bounds
    A.bs = A.windows(bounds)
    A.free = [g for g in range(1, A.N) if A.glue[g] == 0 and g not in bounds]
    return A


def tu_rows(A):
    """TUs from boundaries; module names from classes / C prefixes / strings."""
    starts = [0] + A.bs
    ends = A.bs + [A.N]
    rows = []
    for k, (s, e) in enumerate(zip(starts, ends)):
        rows.append({"idx": k, "s": s, "e": e})
    return rows


def strings_of(A, i, limit=3):
    out = []
    for r in A.F[i]["rdata"]:
        if r >= IAT_HI and A.is_string(r):
            s = A.rdata[r - A.rlo: r - A.rlo + 64].split(b"\0")[0].decode("latin-1")
            out.append(s)
    return out[:limit]


def module_name(A, s, e, used):
    c = collections.Counter()
    for i in range(s, e):
        k = A.cls[i]
        if k and not GENERIC_CLASS.match(k) and "$" not in k:
            c[k] += 1
    if c:
        top = [k for k, _ in c.most_common(2)]
        name = snake(top[0])
        why = "classes " + ",".join("%s:%d" % kv for kv in c.most_common(4))
    else:
        # strings: file names / subsystem words
        words = collections.Counter()
        for i in range(s, e):
            for t in strings_of(A, i, 6):
                for w in re.findall(r"[A-Za-z][a-z]{3,}", t):
                    words[w.lower()] += 1
        if words:
            name = "unk_" + words.most_common(1)[0][0]
            why = "strings " + ",".join(w for w, _ in words.most_common(4))
        else:
            name = "unk"
            why = "no names"
    base = name
    n = 2
    while name in used:
        name = "%s_%d" % (base, n); n += 1
    used.add(name)
    return name, why


SECTIONS = {"rdata": (IAT_HI, XC_LO, "rdata"), "data": (XC_HI, DINIT_HI, "data"),
            "bss": (DINIT_HI, 1 << 32, "data")}


def lis_nondecreasing(vals):
    """indices of a longest nondecreasing subsequence."""
    tails, tails_i, prev = [], [], [-1] * len(vals)
    for i, v in enumerate(vals):
        k = bisect.bisect_right(tails, v)
        if k == len(tails):
            tails.append(v); tails_i.append(i)
        else:
            tails[k] = v; tails_i[k] = i
        prev[i] = tails_i[k - 1] if k else -1
    out, i = [], tails_i[-1] if tails_i else -1
    while i >= 0:
        out.append(i); i = prev[i]
    return out[::-1]


def layout_points(A, tu_of_pos):
    """per section: (addr, tu) of items referenced from exactly one TU, and the subset that is in
    TU order (longest nondecreasing run). The share in order measures how well the TU sequence agrees
    with the data layout (each object's contributions are contiguous and in link order)."""
    res = {}
    for sec, (lo, hi, key) in SECTIONS.items():
        m = A.items(lo, hi, key)
        pts = []
        for r in sorted(m):
            ts = set(tu_of_pos[p] for p in m[r])
            if len(ts) == 1:
                pts.append((r, ts.pop()))
        keep = lis_nondecreasing([t for _, t in pts])
        res[sec] = (pts, [pts[i] for i in keep])
    return res


def tu_range_for_addr(points, a):
    """TU range [lo, hi] that may own address a, from the in-order points around it."""
    addrs = [x for x, _ in points]
    k = bisect.bisect_right(addrs, a)
    lo = points[k - 1][1] if k else 0
    hi = points[k][1] if k < len(points) else 1 << 30
    if k and points[k - 1][0] == a:
        hi = lo
    return lo, hi


def post_crt(A, d, tu_of_va, layout):
    """assign the post-CRT area: EH stubs (referenced by their function's prologue), dynamic
    initializers (.CRT$XCU order = object order; the global each one writes lies in its TU's .data/.bss
    contribution), atexit thunks (pushed by an initializer)."""
    pe = A.pe
    post = [f for f in d["funcs"] if f["va"] >= POST_LO]
    refby = collections.defaultdict(set)
    for f in d["funcs"]:
        for t in f["fptr"] + f["calls"] + f["tails"]:
            refby[t].add(f["va"])
    xcu = []
    tbl = pe.get_data(XC_LO - 0x400000, XC_HI - XC_LO)
    for o in range(0, len(tbl), 4):
        v = struct.unpack_from("<I", tbl, o)[0]
        if v >= POST_LO:
            xcu.append(v)
    users = collections.defaultdict(collections.Counter)    # data address -> TU -> refs
    for f in A.F:
        for r in f["data"]:
            users[r][tu_of_va[f["va"]]] += 1
    fa = {f["va"]: f for f in post}
    out = {}
    for f in post:
        callers = [c for c in refby.get(f["va"], ()) if c in tu_of_va]
        if f["size"] == 10 and callers and f["va"] < min(xcu or [1 << 32]):
            out[f["va"]] = ["eh-stub", tu_of_va[min(callers)], "__ehhandler of %x" % min(callers)]
        else:
            out[f["va"]] = ["post-crt", None, ""]
    # initializers: candidate TU range from the global's address, narrowed to TUs that use it
    last = 0
    for v in xcu:
        f = fa.get(v)
        if f is None:
            continue
        gl = [r for r in f["data"] if r >= XC_HI]
        lo, hi, how = 0, 1 << 30, "no global"
        for r in gl[-1:]:                       # the last data reference is the constructed/stored global
            sec = "data" if r < DINIT_HI else "bss"
            lo, hi = tu_range_for_addr(layout[sec][1], r)
            how = "global %08x in %s layout between TU %d and %s" % (r, sec, lo, hi if hi < 1 << 30 else "end")
        # .bss/.data do not follow text order well (externs, see docs/tu_map.md), so the layout range is
        # only reported; the TU is the main user of the global, else the TU of the constructor called
        cand = collections.Counter()
        for r in gl:
            cand.update(users.get(r, {}))
        if cand:
            tu = cand.most_common(1)[0][0]
            how += "; main user TU %d" % tu
        else:
            ctor = [tu_of_va[c] for c in f["calls"] if c in tu_of_va]
            tu = ctor[0] if ctor else None
            how += "; ctor's TU" if ctor else ""
        if tu is not None and tu < last:
            how += "; OUT OF .CRT$XCU ORDER (prev TU %d)" % last
        out[v] = ["initializer", tu, how]
        if tu is not None:
            last = max(last, tu)
    for f in post:
        if out[f["va"]][0] == "initializer":
            for t in f["fptr"]:
                if t in out and out[t][0] == "post-crt":
                    out[t] = ["atexit-thunk", out[f["va"]][1], "registered by initializer %x" % f["va"]]
    for f in post:
        if out[f["va"]][0] == "post-crt" and out[f["va"]][1] is None:
            callers = [c for c in refby.get(f["va"], ()) if c in tu_of_va]
            if callers:
                out[f["va"]][1] = tu_of_va[min(callers)]
                out[f["va"]][2] = "referenced by %x" % min(callers)
    return out, xcu


def validate(A, tu_of_pos):
    lines = []
    for setname, tus in KNOWN.items():
        truth = [s for _, s, _ in tus] + [tus[-1][2]]
        tpos = [A.pos[v] for v in truth if v in A.pos]
        lo, hi = tpos[0], tpos[-1]
        found = [g for g in A.bs if lo <= g <= hi]
        hits = [g for g in found if g in tpos]
        lines.append("== %s: %d known boundaries (incl. both ends), found %d boundaries in range" %
                     (setname, len(tpos), len(found)))
        for v in truth:
            g = A.pos.get(v)
            b = A.bounds.get(g)
            if g == 0:
                lines.append("  %08x  START   (first game function)" % v)
            elif b:
                w = b["win"]
                lines.append("  %08x  FOUND   %-6s %-40s window %08x..%08x" %
                             (v, b["conf"], b["why"][:40], A.vas[w[0]], A.vas[w[1]]))
            else:
                near = [x for x in found if abs(x - g) <= 3]
                lines.append("  %08x  MISSED  glue=%.0f %s%s" % (v, A.glue[g], dict(A.glue_src[g]),
                             ("  nearest boundary %08x" % A.vas[near[0]]) if near else ""))
        for g in found:
            if g not in tpos:
                b = A.bounds[g]
                lines.append("  %08x  EXTRA   %-6s %s" % (A.vas[g], b["conf"], b["why"]))
        free = [g for g in A.free if lo <= g <= hi]
        lines.append("  free (zero-glue, no evidence) gaps in range: %d; true boundaries among them: %d" %
                     (len(free), len([g for g in free if g in tpos])))
    return lines


def main():
    A = analyse("--rebuild-facts" in sys.argv)
    d = A.d
    status, inslice = slice_status()
    rows = tu_rows(A)
    used = set()
    tu_of_pos = [0] * A.N
    for r in rows:
        for i in range(r["s"], r["e"]):
            tu_of_pos[i] = r["idx"]
    tu_of_va = {A.vas[i]: tu_of_pos[i] for i in range(A.N)}
    layout = layout_points(A, tu_of_pos)
    post, xcu = post_crt(A, d, tu_of_va, layout)
    # data ranges per TU (unique-referenced items only)
    def data_range(s, e, lo, hi, key):
        own = []
        m = A.items(lo, hi, key)
        return m
    rmap = A.items(IAT_HI, XC_LO, "rdata")
    dmap = A.items(XC_HI, 1 << 32)
    rown = collections.defaultdict(list)
    downl = collections.defaultdict(list)
    for r, ps in rmap.items():
        if len(ps) == 1:
            rown[tu_of_pos[next(iter(ps))]].append(r)
    for r, ps in dmap.items():
        ts = set(tu_of_pos[p] for p in ps)
        if len(ts) == 1:
            downl[ts.pop()].append(r)
    nposts = collections.Counter(v[1] for v in post.values())
    os.makedirs(OUT, exist_ok=True)
    out_rows = []
    for r in rows:
        s, e = r["s"], r["e"]
        name, why = module_name(A, s, e, used)
        r["module"] = name
        cnt = collections.Counter()
        for i in range(s, e):
            va = A.vas[i]
            if A.F[i]["lib"]:
                cnt["lib"] += 1
            elif va in status:
                cnt[status[va]] += 1
            elif va in inslice:
                cnt["missing"] += 1
            else:
                cnt["noslice"] += 1
        bl = A.bounds.get(s)
        br = A.bounds.get(e)
        def bconf(b, edge):
            if edge in (0, A.N):
                return "high"
            return b["conf"] if b else "low"
        confs = [bconf(bl, s), bconf(br, e)]
        order = {"high": 2, "medium": 1, "low": 0}
        conf = min(confs, key=lambda c: order[c])
        regs = collections.Counter(A.regime[i] for i in range(s, e))
        ev = []
        if bl:
            ev.append("start: %s [%s]" % (bl["why"], bl["conf"]))
        if br:
            ev.append("end: %s [%s]" % (br["why"], br["conf"]))
        ev.append(why)
        free = [g for g in A.free if s < g < e]
        if free:
            ev.append("%d possible inner split(s)" % len(free))
        code = A.ends[e - 1] - A.vas[s]
        ro = [x for x, t in layout["rdata"][1] if t == r["idx"]]
        do = [x for x, t in layout["data"][1] if t == r["idx"]]
        bo = [x for x, t in layout["bss"][1] if t == r["idx"]]
        n_ex = cnt["exact"] + cnt["lib"]
        out_rows.append({
            "tu": r["idx"], "start_va": "%08x" % A.vas[s], "end_va": "%08x" % A.ends[e - 1],
            "n_functions": e - s, "code_bytes": code, "module": name, "confidence": conf,
            "start_window": ("%08x..%08x" % (A.vas[bl["win"][0]], A.vas[bl["win"][1]])) if bl else "",
            "regime": "".join("%s%d" % kv for kv in sorted(regs.items())),
            "rdata": ("%08x..%08x" % (ro[0], ro[-1])) if ro else "",
            "data": ("%08x..%08x" % (do[0], do[-1])) if do else "",
            "bss": ("%08x..%08x" % (bo[0], bo[-1])) if bo else "",
            "post_crt": nposts.get(r["idx"], 0),
            "exact": cnt["exact"], "nonmatching": cnt["nonmatching"], "partial": cnt["partial"],
            "missing": cnt["missing"], "noslice": cnt["noslice"], "lib": cnt["lib"],
            "exact_share": "%.2f" % (n_ex / (e - s)),
            "evidence": "; ".join(ev)})
    # library rows (import thunks, dinput8 data)
    lib_rows = []
    libf = [f for f in d["funcs"] if GAME_HI <= f["va"] < LIB_HI]
    groups = collections.OrderedDict()
    for f in libf:
        nm = A.nm.get(f["va"]) or ""
        b = pe_name = None
        row = next((x for x in csv.DictReader(open(W("work/index/functions.csv"))) if int(x["address"], 16) == f["va"]), None)
        n = row["name"] if row else ""
        key = ("fmod import lib (fmodvc.lib)" if n.startswith("_FSOUND") else
               "d3d9.lib" if n.startswith("Direct3D") else
               "d3dx9.lib" if n.startswith("D3DX") else "dinput8.lib (data formats)")
        groups.setdefault(key, []).append(f)
    with open(os.path.join(OUT, "tu_map.csv"), "w", newline="") as fh:
        cols = list(out_rows[0].keys())
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        for r in out_rows:
            w.writerow(r)
        k = len(out_rows)
        for key, fs in groups.items():
            w.writerow({"tu": k, "start_va": "%08x" % fs[0]["va"], "end_va": "%08x" % fs[-1]["end"],
                        "n_functions": len(fs), "code_bytes": fs[-1]["end"] - fs[0]["va"],
                        "module": "lib:" + key, "confidence": "high", "evidence": "import thunks / library data"})
            k += 1
    # per-function file
    with open(os.path.join(OUT, "tu_functions.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["va", "end", "tu", "module", "kind", "status", "regime", "name", "note"])
        for i, f in enumerate(A.F):
            va = f["va"]
            kind = "lib-comdat" if f["lib"] else "code"
            stt = "lib" if f["lib"] else status.get(va, "missing" if va in inslice else "noslice")
            w.writerow(["%08x" % va, "%08x" % f["end"], tu_of_pos[i], rows[tu_of_pos[i]]["module"], kind, stt,
                        A.regime[i], f["lib"] or A.nm.get(va, ""), ""])
        for f in d["funcs"]:
            va = f["va"]
            if va >= POST_LO:
                kind, tu, how = post[va]
                w.writerow(["%08x" % va, "%08x" % f["end"], "" if tu is None else tu,
                            "" if tu is None else rows[tu]["module"], kind, status.get(va, "post-crt"),
                            "", A.nm.get(va, ""), how])
    # boundaries file
    with open(os.path.join(OUT, "tu_boundaries.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["gap_va", "kind", "confidence", "window", "glue", "glue_sources", "classes", "why"])
        for g in sorted(set(A.bs) | set(A.free)):
            b = A.bounds.get(g)
            if b:
                w.writerow(["%08x" % A.vas[g], "boundary", b["conf"],
                            "%08x..%08x" % (A.vas[b["win"][0]], A.vas[b["win"][1]]), A.glue[g],
                            dict(A.glue_src[g]), "%s|%s" % b["classes"], b["why"]])
            else:
                w.writerow(["%08x" % A.vas[g], "free-gap", "", "", 0, "", "%s|%s" % A.class_change(g), ""])
    val = validate(A, tu_of_pos)
    val.append("== data layout vs TU order (items referenced from one TU only; share in TU order)")
    for sec, (pts, keep) in layout.items():
        val.append("  %-6s %5d items, %5d in order (%.1f%%)" % (sec, len(pts), len(keep),
                                                          100.0 * len(keep) / max(1, len(pts))))
    xs = [post[v][1] for v in xcu if v in post]
    val.append("  .CRT$XCU initializers: %d, TU sequence %s" % (len(xs), " ".join(map(str, xs))))
    with open(os.path.join(OUT, "tu_validation.txt"), "w") as fh:
        fh.write("\n".join(val) + "\n")
    # summary
    confc = collections.Counter(r["confidence"] for r in out_rows)
    print("TUs (game code): %d; confidence %s" % (len(out_rows), dict(confc)))
    for k, v in A.log.items():
        print("  %-55s %s" % (k, v))
    print("  free zero-glue gaps without boundary evidence           %d" % len(A.free))
    print("\n".join(val))
    A.rows, A.out_rows, A.post, A.xcu = rows, out_rows, post, xcu
    return A


if __name__ == "__main__":
    main()
