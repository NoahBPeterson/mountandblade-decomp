#!/usr/bin/env python3
"""Variant search: mechanically rewrite one function's source, compile every variant, keep what scores closer.

usage: vsearch.py <file.cpp> <va-hex> [--rounds N] [--jobs J] [--apply] [--keep-unsafe]
       vsearch.py --batch [--max-aligned 64] [--limit N] [--jobs J] [--apply]   (rows from every *.nonmatching.txt)

No model in the loop. The levers a byte-matching lane tries by hand, applied to the text of the function after its
`// @ 0x<va>` marker:
  cmp-swap      a == b  ->  b == a   (also !=, and a < b -> b > a etc.)
  comm-swap     a + b   ->  b + a    (also *, &, |, ^; simple operands only)
  incdec        i++;    ->  ++i;  i += 1;   (statement level only)
  zero-test     if (x)  ->  if (x != 0);   if (!x) -> if (x == 0)   (and back)
  stmt-swap     two adjacent one-line statements that share no identifier (decl order moves stack slots)
  decl-hoist    a declaration without initialiser moved to the top of the function body
Every variant is a whole-TU compile (cl.sh: the laptop's compile server first) scored like chk.py: the aligned figure
(the .nonmatching.txt column), positional diff as the tie-break. Greedy hill climb: each round tries every single
rewrite of the current best and takes the best strict improvement, until none improves or it MATCHes.

Safety: every rewrite except stmt-swap and decl-hoist is meaning-preserving by construction. stmt-swap is guarded by
the shared-identifier check, which cannot see aliasing through pointers, so a result that uses it and is not a MATCH is
only reported (marked UNVERIFIED), never applied, unless --keep-unsafe. A MATCH is correct by definition.
--apply writes the best variant back to the source only when it is strictly better; re-run chk.py / run_all after.
Variant files live in work/vsearch/ (never in src/); the source's own directory is passed as /I.
"""
import argparse, concurrent.futures as cf, threading, traceback, difflib, glob, hashlib, os, re, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from cmpobj import parse_coff, REL_SIZES, coff_constants, constant_diffs   # noqa: E402
from run_all import Original, winpath, CL, DEFAULT_FLAGS, INCLUDE, ROOT, resolve_flags   # noqa: E402
from card import load_funcs, bounds   # noqa: E402
import chk   # noqa: E402

WORK = os.path.join(ROOT, "work", "vsearch")
KEYWORDS = set("""if else for while do return break continue switch case default goto sizeof int unsigned signed char short
long float double void const volatile static struct class union enum typedef new delete this true false bool operator
inline __forceinline register auto extern template typename public private protected virtual friend""".split())

# ---------------------------------------------------------------------------------------------------------- source
def code_mask(text):
    """A copy of text with comments, string and char literals blanked (same length), so rewrites never touch them."""
    out, i, n = list(text), 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i); j = n if j < 0 else j
            for k in range(i, j): out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2); j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n": out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)): out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def function_span(text, va):
    """(body_start, body_end) of the function after the `// @ 0x<va>` marker: the offsets of its { and its matching }."""
    m = re.search(r"//.*@\s*0x0*%x\b.*" % va, text, re.I)
    if not m:
        return None
    mask = code_mask(text)
    i = mask.find("{", m.end())
    semi = mask.find(";", m.end())
    while semi != -1 and semi < i and "(" not in mask[m.end():semi]:   # skip declarations between marker and body
        semi = mask.find(";", semi + 1)
    if i < 0:
        return None
    depth = 0
    for j in range(i, len(mask)):
        if mask[j] == "{": depth += 1
        elif mask[j] == "}":
            depth -= 1
            if depth == 0:
                return i, j + 1
    return None


# -------------------------------------------------------------------------------------------------------- rewrites
OPND = r"(?:(?<![\w.>])-?\d[\w.]*|[A-Za-z_]\w*(?:(?:\.|->)[A-Za-z_]\w*|\[[^\[\]\n]*\])*)"
CMP_FLIP = {"==": "==", "!=": "!=", "<": ">", ">": "<", "<=": ">=", ">=": "<="}


def _sub_each(body, mask, pattern, repl, tag, safe=True):
    for m in re.finditer(pattern, mask):
        new = repl(m, body)
        if new is not None and new != body[m.start():m.end()]:
            yield ("%s @%d: %s -> %s" % (tag, m.start(), body[m.start():m.end()].strip(), new.strip()),
                   body[:m.start()] + new + body[m.end():], safe)


def rw_cmp_swap(body, mask):
    pat = r"(?<![\w\]\)])(%s)\s*(==|!=|<=|>=|<(?![<=])|>(?![>=]))\s*(%s)(?![\w\[(.]|->)" % (OPND, OPND)
    def repl(m, b):
        a, op, c = b[m.start(1):m.end(1)], m.group(2), b[m.start(3):m.end(3)]
        if op in "<>" and (re.match(r"[A-Za-z_]", c) and c in KEYWORDS):
            return None
        # both operands must stand alone: the comparison is bounded by ( , ; ? : && || = or return on the left and
        # by ) , ; ] ? : && || on the right. (`-1 < n - 2` once became `n > -1 - 2`.)
        pre = mask[:m.start()].rstrip()
        if not (pre.endswith(("(", ",", ";", "{", "}", "?", ":", "&&", "||", "return")) or
                (pre.endswith("=") and not pre.endswith(("==", "!=", "<=", ">=")))):
            return None
        post = mask[m.end():].lstrip()
        if not post.startswith((")", ",", ";", "]", "?", ":", "&&", "||")):
            return None
        return "%s %s %s" % (c, CMP_FLIP[op], a)
    yield from _sub_each(body, mask, pat, repl, "cmp-swap")


def rw_comm_swap(body, mask):
    pat = r"(?<![\w\]\)*/%+\-.>])(" + OPND + r")\s*([+*&|^])(?![=+&|])\s*(" + OPND + r")(?![\w\[(.]|->|\s*[*/%])"
    def repl(m, b):
        pre = mask[:m.start()].rstrip()
        if pre.endswith(("(", ",", "=", "return")) or pre[-1:] in ("(", ",", "=", ";", "{", "}"):
            nxt = mask[m.end():].lstrip()[:1]
            if nxt in (")", ",", ";", "]"):
                a, op, c = b[m.start(1):m.end(1)], m.group(2), b[m.start(3):m.end(3)]
                if op in "&*" and pre[-1:] in ("(", ",", "=", ";", "{", "}") and re.match(r"[A-Za-z_]\w*$", a) \
                        and pre[-1:] != "=" and pre[-1:] != "(":
                    return None   # `T * x` / `T & x` declaration, not a product
                return "%s %s %s" % (c, op, a)
        return None
    yield from _sub_each(body, mask, pat, repl, "comm-swap")


def rw_incdec(body, mask):
    for m in re.finditer(r"(?m)^([ \t]*)(?:(\+\+|--)\s*(%s)|(%s)\s*(\+\+|--))\s*;" % (OPND, OPND), mask):
        ind = m.group(1)
        v = m.group(3) or m.group(4)
        op = m.group(2) or m.group(5)
        vb = body[m.start(3):m.end(3)] if m.group(3) else body[m.start(4):m.end(4)]
        forms = ["%s%s%s;" % (ind, op, vb), "%s%s%s;" % (ind, vb, op), "%s%s %s= 1;" % (ind, vb, op[0])]
        cur = body[m.start():m.end()]
        for f in forms:
            if f.replace(" ", "") != cur.replace(" ", ""):
                yield ("incdec @%d: %s -> %s" % (m.start(), cur.strip(), f.strip()), body[:m.start()] + f + body[m.end():], True)
    for m in re.finditer(r"(?m)^([ \t]*)(%s)\s*([+-])=\s*1\s*;" % OPND, mask):
        ind, vb, op = m.group(1), body[m.start(2):m.end(2)], m.group(3) * 2
        for f in ("%s%s%s;" % (ind, vb, op), "%s%s%s;" % (ind, op, vb)):
            yield ("incdec @%d: %s -> %s" % (m.start(), body[m.start():m.end()].strip(), f.strip()),
                   body[:m.start()] + f + body[m.end():], True)


def rw_zero_test(body, mask):
    # if (x) -> if (x != 0); if (!x) -> if (x == 0); and the reverse
    for m in re.finditer(r"\b(if|while)\s*\(\s*(!?)\s*(%s)\s*\)" % OPND, mask):
        kw, neg, x = m.group(1), m.group(2), body[m.start(3):m.end(3)]
        if x in KEYWORDS or re.match(r"-?\d", x):
            continue
        new = "%s (%s %s 0)" % (kw, x, "==" if neg else "!=")
        yield ("zero-test @%d: %s -> %s" % (m.start(), body[m.start():m.end()], new), body[:m.start()] + new + body[m.end():], True)
    for m in re.finditer(r"\b(if|while)\s*\(\s*(%s)\s*(==|!=)\s*0\s*\)" % OPND, mask):
        kw, x, op = m.group(1), body[m.start(2):m.end(2)], m.group(3)
        new = "%s (%s%s)" % (kw, "!" if op == "==" else "", x)
        yield ("zero-test @%d: %s -> %s" % (m.start(), body[m.start():m.end()], new), body[:m.start()] + new + body[m.end():], True)


SIMPLE_STMT = re.compile(r"^(?![ \t]*(?:if|else|for|while|do|return|break|continue|goto|case|default|switch|throw|delete)\b)[ \t]*[^{};\n]*;[ \t]*(?://[^\n]*)?$")


def idents(s):
    return {w for w in re.findall(r"[A-Za-z_]\w*", s) if w not in KEYWORDS}


def rw_stmt_swap(body, mask):
    lines = body.split("\n")
    mlines = mask.split("\n")
    offs, o = [], 0
    for l in lines:
        offs.append(o); o += len(l) + 1
    for i in range(len(lines) - 1):
        a, b = mlines[i], mlines[i + 1]
        if not (SIMPLE_STMT.match(a) and SIMPLE_STMT.match(b)):
            continue
        if len(a) - len(a.lstrip()) != len(b) - len(b.lstrip()) or ":" in a.split("//")[0].replace("::", ""):
            continue
        ia, ib = idents(a), idents(b)
        if "(" in a.split("//")[0] and "(" in b.split("//")[0]:
            continue   # two calls (or casts): their order may be visible; not worth the risk
        # the only shared identifiers allowed are type names (a declaration pair `int a; int b;`)
        shared = ia & ib
        shared = {w for w in shared if not re.search(r"\b%s\b\s*[*&]*\s*[A-Za-z_]\w*\s*[;=\[,]" % re.escape(w), a)
                  or not re.search(r"\b%s\b\s*[*&]*\s*[A-Za-z_]\w*\s*[;=\[,]" % re.escape(w), b)}
        if shared:
            continue
        new = lines[:i] + [lines[i + 1], lines[i]] + lines[i + 2:]
        yield ("stmt-swap @%d: [%s] <-> [%s]" % (offs[i], lines[i].strip(), lines[i + 1].strip()), "\n".join(new), False)
    # stmt-move: one statement moved 2..4 lines up past statements it is independent of (same block, same indent)
    for i in range(len(lines)):
        if not SIMPLE_STMT.match(mlines[i]):
            continue
        ind = len(mlines[i]) - len(mlines[i].lstrip())
        for k in range(1, 5):
            j = i - k
            if j < 0 or not SIMPLE_STMT.match(mlines[j]) or len(mlines[j]) - len(mlines[j].lstrip()) != ind:
                break
            if not _independent(mlines[j], mlines[i]):
                break
            if k >= 2:
                new = lines[:j] + [lines[i]] + lines[j:i] + lines[i + 1:]
                yield ("stmt-move @%d: [%s] up %d" % (offs[i], lines[i].strip(), k), "\n".join(new), False)


def _independent(a, b):
    """No shared identifier (a declaration pair may share its type name) and not two calls."""
    if ":" in a.split("//")[0].replace("::", "") or ":" in b.split("//")[0].replace("::", ""):
        return False
    if "(" in a.split("//")[0] and "(" in b.split("//")[0]:
        return False
    decl = r"\b%s\b\s*[*&]*\s*[A-Za-z_]\w*\s*[;=\[,(]"
    shared = {w for w in idents(a) & idents(b)
              if not re.search(decl % re.escape(w), a) or not re.search(decl % re.escape(w), b)}
    return not shared


DECL = re.compile(r"^([ \t]*)((?:unsigned\s+|signed\s+|const\s+)*[A-Za-z_][\w:]*(?:\s*[*&]+\s*|\s+)[A-Za-z_]\w*(?:\s*\[[^\]]*\])?"
                  r"(?:\s*,\s*[*&]*\s*[A-Za-z_]\w*)*)\s*;[ \t]*(?://[^\n]*)?$")


def rw_decl_hoist(body, mask):
    lines, mlines = body.split("\n"), mask.split("\n")
    if len(lines) < 3:
        return
    top = 1
    while top < len(lines) and (not mlines[top].strip() or DECL.match(mlines[top])):
        top += 1
    for i in range(top + 1, len(lines)):
        m = DECL.match(mlines[i])
        if not m or m.group(2).split()[0] in KEYWORDS - {"int", "unsigned", "signed", "char", "short", "long", "float",
                                                           "double", "bool", "const"}:
            continue
        if mlines[i].strip().startswith(("return", "delete", "goto")):
            continue
        name = re.findall(r"[A-Za-z_]\w*", m.group(2).split("[")[0])[-1]
        if re.search(r"\b%s\b" % re.escape(name), "\n".join(mlines[:i])):   # an earlier use: shadowing / scope change
            continue
        ind = re.match(r"[ \t]*", lines[1]).group(0) or "    "
        new = lines[:top] + [ind + lines[i].strip()] + lines[top:i] + lines[i + 1:]
        yield ("decl-hoist @%d: %s" % (i, lines[i].strip()), "\n".join(new), False)


def _close(mask, i, o, c):
    """Offset just past the bracket matching the one at mask[i]."""
    d = 0
    for j in range(i, len(mask)):
        if mask[j] == o: d += 1
        elif mask[j] == c:
            d -= 1
            if d == 0:
                return j + 1
    return -1


def _stmt_end(mask, i):
    """End offset of the statement starting at or after i: a {block}, an if/else chain, or up to its ;."""
    while i < len(mask) and mask[i].isspace():
        i += 1
    if mask.startswith("{", i):
        return _close(mask, i, "{", "}")
    m = re.match(r"(if|while|for|switch)\s*\(", mask[i:])
    if m:
        e = _close(mask, i + m.end() - 1, "(", ")")
        if e < 0:
            return -1
        e = _stmt_end(mask, e)
        if m.group(1) == "if" and e > 0:
            k = re.match(r"\s*else\b", mask[e:])
            if k:
                e = _stmt_end(mask, e + k.end())
        return e
    if re.match(r"do\b", mask[i:]):
        return -1
    e = mask.find(";", i)
    return -1 if e < 0 else e + 1


def _top_split(mask, a, b, op):
    """Offset of the first top-level `op` in mask[a:b], or -1."""
    d = 0
    for j in range(a, b - 1):
        c = mask[j]
        if c in "([": d += 1
        elif c in ")]": d -= 1
        elif d == 0 and mask.startswith(op, j):
            return j
    return -1


def rw_if_shapes(body, mask):
    """if (A && B) S        ->  if (A) { if (B) S }        (no else: same meaning)
       if (c) X else Y       ->  if (!(c)) Y else X        (block order)"""
    for m in re.finditer(r"\bif\s*\(", mask):
        if re.search(r"\belse\s*$", mask[:m.start()]):
            continue                                   # else-if: leave the chain shape alone
        cp = m.end() - 1
        ce = _close(mask, cp, "(", ")")
        if ce < 0:
            continue
        se = _stmt_end(mask, ce)
        if se < 0:
            continue
        k = re.match(r"\s*else\b", mask[se:])
        cond = body[cp + 1:ce - 1]
        if not k:
            j = _top_split(mask, cp + 1, ce - 1, "&&")
            if j > 0 and _top_split(mask, cp + 1, ce - 1, "||") < 0:
                a_, b_ = body[cp + 1:j].strip(), body[j + 2:ce - 1].strip()
                new = "if (%s) { if (%s)%s }" % (a_, b_, body[ce:se])
                yield ("if-split @%d: if (%s && ...)" % (m.start(), a_[:40]), body[:m.start()] + new + body[se:], True)
        else:
            ee = _stmt_end(mask, se + k.end())
            if ee < 0:
                continue
            x, y = body[ce:se].strip(), body[se + k.end():ee].strip()
            if not x.startswith("{"): x = "{ " + x + " }"
            if not y.startswith("{"): y = "{ " + y + " }"
            new = "if (!(%s)) %s else %s" % (cond.strip(), y, x)
            yield ("if-flip @%d: if (%s)" % (m.start(), cond.strip()[:50]), body[:m.start()] + new + body[ee:], True)


REWRITES = [rw_cmp_swap, rw_comm_swap, rw_incdec, rw_zero_test, rw_if_shapes, rw_stmt_swap, rw_decl_hoist]


def variants(body):
    mask = code_mask(body)
    seen = set()
    for rw in REWRITES:
        for desc, new, safe in rw(body, mask):
            if new not in seen and new != body:
                seen.add(new)
                yield desc, new, safe


# ---------------------------------------------------------------------------------------------------------- scoring
class Scorer:
    def __init__(self, src, va, flags):
        self.src, self.va, self.flags = os.path.abspath(src), va, flags
        self.orig = Original()
        fstarts, _ = load_funcs()
        em = set()
        for l in open(os.path.join(ROOT, "symbols", "extent_merge.txt")):
            p = l.split("#", 1)[0].split()
            if p:
                em.add(int(p[0], 16))
        self.n0 = len(bounds(va, [a for a in fstarts if a not in em]))
        self.theirs0, self.omask0 = self.orig.bytes_and_mask(va, self.n0)
        self.sym = chk.manifest_syms(self.src).get(va)
        tag = "%s_%08x" % (os.path.basename(src).rsplit(".", 1)[0], va)
        self.dir = os.path.join(WORK, tag)
        os.makedirs(self.dir, exist_ok=True)
        self.cache = {}

    def score(self, text):
        """(aligned, positional, symbol) for this whole-file text, or None when it does not compile / has no symbol."""
        h = hashlib.md5(text.encode()).hexdigest()[:12]
        if h in self.cache:
            return self.cache[h]
        base = os.path.basename(self.src)
        cpp = os.path.join(self.dir, "v_%s_%d_%d_%s" % (h, os.getpid(), threading.get_ident(), base))
        obj = cpp.rsplit(".", 1)[0] + ".obj"
        open(cpp, "w").write(text)
        try:
            r = subprocess.run([CL, "/nologo", "/c", *self.flags, INCLUDE, "/I" + winpath(os.path.dirname(self.src)),
                                "/Fo" + winpath(obj), winpath(cpp)], capture_output=True, text=True)
            if r.returncode or not os.path.exists(obj):
                self.cache[h] = None
                return None
            res = self._score_obj(obj, cpp)
        finally:
            for p in (cpp, obj):
                try: os.remove(p)
                except OSError: pass
        self.cache[h] = res
        return res

    def _score_obj(self, obj, cpp):
        secs, syms = parse_coff(obj)
        starts = {}
        for si, off in syms.values():
            starts.setdefault(si, set()).add(off)
        names = [n for n in syms if not n.startswith(chk.SKIP)]
        want = {self.sym} if self.sym in names else set(chk.marker_syms(cpp, self.va, names))
        if not want:
            return None
        consts = coff_constants(obj)
        best = None
        for name in want:
            si, off = syms[name]
            if not (0 <= si < len(secs)) or secs[si] is None:
                continue
            sec = secs[si]
            nxt = sorted(o for o in starts[si] if o > off)
            mine = sec["data"][off:nxt[0] if nxt else len(sec["data"])]
            if nxt:
                mine = mine.rstrip(b"\xcc")
            n = len(mine)
            rmask = {v + k - off for v, _s, t in sec["relocs"] for k in range(REL_SIZES.get(t, 4)) if off <= v + k < off + n}
            theirs, omask = self.orig.bytes_and_mask(self.va, n)
            d = sum(1 for i in range(n) if i not in rmask and i not in omask and theirs[i] != mine[i]) + abs(n - self.n0)
            d += len(constant_diffs(consts, sec["relocs"], off, theirs, self.orig.read, mine))
            al = 0 if d == 0 else chk.aligned_diff(self.theirs0, self.omask0, mine, rmask)
            if best is None or (al, d) < best[:2]:
                best = (al, d, name)
        return best


# ----------------------------------------------------------------------------------------------------------- search
def search(src, va, rounds=8, jobs=24, keep_unsafe=False, log=print):
    text = open(src).read()
    flags = row_flags(src, va)
    sc = Scorer(src, va, flags)
    span = function_span(text, va)
    if not span:
        log("  no function body after the `// @ 0x%x` marker" % va)
        return None
    t0 = time.time()
    base = sc.score(text)
    if base is None:
        log("  the source does not compile or the symbol is missing")
        return None
    log("  start: aligned %d, diff %d  (%s)" % (base[0], base[1], base[2][:70]))
    cur_text, cur, applied, tried = text, base, [], 0
    with cf.ThreadPoolExecutor(jobs) as ex:
        for rnd in range(rounds):
            if cur[1] == 0:
                break
            a, b = function_span(cur_text, va)
            body = cur_text[a:b]
            cands = [(d, cur_text[:a] + nb + cur_text[b:], s) for d, nb, s in variants(body)]
            if not cands:
                break
            futs = {ex.submit(sc.score, t): (d, t, s) for d, t, s in cands}
            tried += len(cands)
            results = []
            for f in cf.as_completed(futs):
                r = f.result()
                if r is not None:
                    results.append((r[:2], futs[f]))
            results.sort(key=lambda x: x[0])
            if not results or results[0][0] >= cur[:2]:
                log("  round %d: %d variants, none closer" % (rnd + 1, len(cands)))
                break
            (al, d), (desc, t, s) = results[0]
            log("  round %d: %d variants, best aligned %d diff %d  <- %s%s" % (rnd + 1, len(cands), al, d, desc[:110],
                                                                             "" if s else "  [unsafe]"))
            cur_text, cur = t, (al, d, cur[2])
            applied.append((desc, s))
    unsafe = any(not s for _, s in applied)
    return dict(src=src, va=va, start=base[:2], end=cur[:2], applied=applied, text=cur_text, orig_text=text,
                match=cur[1] == 0, unsafe=unsafe and cur[1] != 0, tried=tried, secs=time.time() - t0)


def row_flags(src, va):
    mf = src.rsplit(".", 1)[0] + ".manifest.txt"
    if os.path.exists(mf):
        for l in open(mf):
            p = l.split("#", 1)[0].split()
            if len(p) >= 4 and os.path.basename(p[0]) == os.path.basename(src):
                return resolve_flags(p[3:])
    return list(DEFAULT_FLAGS)


def report(r, apply=False, keep_unsafe=False, log=print):
    if r is None:
        return
    gain = r["start"][0] - r["end"][0]
    tag = "MATCH" if r["match"] else ("UNVERIFIED (stmt-swap/decl-hoist: run equiv)" if r["unsafe"] else "")
    log("  result: aligned %d -> %d, diff %d -> %d, %d variants in %.0f s  %s" %
        (r["start"][0], r["end"][0], r["start"][1], r["end"][1], r["tried"], r["secs"], tag))
    if not r["applied"]:
        return
    patch = "".join(difflib.unified_diff(r["orig_text"].splitlines(True), r["text"].splitlines(True),
                                         os.path.relpath(r["src"], ROOT), os.path.relpath(r["src"], ROOT)))
    pf = os.path.join(WORK, "%s_%08x.patch" % (os.path.basename(r["src"]).rsplit(".", 1)[0], r["va"]))
    open(pf, "w").write(patch)
    log("  patch: %s" % os.path.relpath(pf, ROOT))
    if apply and (r["match"] or not r["unsafe"] or keep_unsafe) and (r["end"] < r["start"]):
        if open(r["src"]).read() != r["orig_text"]:
            log("  NOT applied: the source changed during the search")
            return
        open(r["src"], "w").write(r["text"])
        log("  applied to %s" % os.path.relpath(r["src"], ROOT))


def nonmatching_rows(max_aligned):
    rows = []
    for nm in glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True):
        src = nm[:-len(".nonmatching.txt")] + ".cpp"
        if not os.path.exists(src):
            continue
        text = open(src).read()
        for l in open(nm):
            p = l.split(None, 2)
            if len(p) < 2:
                continue
            try:
                va, al = int(p[0], 16), int(p[1])
            except ValueError:
                continue
            if 0 < al <= max_aligned and re.search(r"//.*@\s*0x0*%x\b" % va, text, re.I):
                rows.append((al, src, va))
    return sorted(rows, key=lambda r: (r[0], r[1], r[2]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?"); ap.add_argument("va", nargs="?")
    ap.add_argument("--batch", action="store_true"); ap.add_argument("--max-aligned", type=int, default=64)
    ap.add_argument("--limit", type=int, default=0); ap.add_argument("--rounds", type=int, default=8)
    ap.add_argument("--jobs", type=int, default=24); ap.add_argument("--apply", action="store_true")
    ap.add_argument("--keep-unsafe", action="store_true")
    ap.add_argument("--shard", default="", help="K/N: only every N-th row starting at K (run N shards side by side)")
    a = ap.parse_args()
    if a.batch:
        rows = nonmatching_rows(a.max_aligned)
        if a.limit:
            rows = rows[:a.limit]
        if a.shard:
            k, n = map(int, a.shard.split("/"))
            rows = rows[k::n]
        tot = dict(rows=0, closer=0, match=0, saved=0)
        for al, src, va in rows:
            print("%s %08x  (aligned %d)" % (os.path.relpath(src, ROOT), va, al), flush=True)
            try:
                r = search(src, va, a.rounds, a.jobs, a.keep_unsafe, log=lambda s: print(s, flush=True))
            except Exception:
                print("  ERROR\n" + traceback.format_exc(), flush=True)
                r = None
            report(r, a.apply, a.keep_unsafe, log=lambda s: print(s, flush=True))
            tot["rows"] += 1
            if r and r["end"] < r["start"]:
                tot["closer"] += 1; tot["saved"] += r["start"][0] - r["end"][0]
            if r and r["match"]:
                tot["match"] += 1
        print("== %(rows)d rows: %(closer)d closer, %(match)d MATCH, %(saved)d aligned bytes saved" % tot)
        return 0
    if not (a.src and a.va):
        ap.error("src and va, or --batch")
    r = search(a.src, int(a.va, 16), a.rounds, a.jobs, a.keep_unsafe)
    report(r, a.apply, a.keep_unsafe)
    return 0 if r and r["match"] else 1


if __name__ == "__main__":
    sys.exit(main())
