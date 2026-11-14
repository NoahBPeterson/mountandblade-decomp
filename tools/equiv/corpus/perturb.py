#!/usr/bin/env python3
"""Same-compiler perturbation corpus: M&B byte-exact functions with locals, recompiled (cl 13.10) from copies of
their source changed in ways that keep the behaviour but move the frame layout, then proved against the original.

  uv run python tools/equiv/corpus/perturb.py gen   [--max-funcs 200] [--seed 1] [--jobs 12]
  uv run python tools/equiv/corpus/perturb.py prove [--jobs 5] [--remote] [--remote-only] [--frame-model]
                                                    [--tmo rename=30,angr=60,region=300] [--out F] [--resume]
  uv run python tools/equiv/corpus/perturb.py negatives [--sample 20] [--remote]
  uv run python tools/equiv/corpus/perturb.py summary

gen: the rows of the byte-exact manifests (src/**/*.manifest.txt, match/slices/*/manifest.txt) whose original
allocates a frame (sub esp, N >= 8), at most --max-funcs of them. Each source is taken from git HEAD (src/ is never
edited: the copy goes to work/equiv/perturb/src/<id>/), compiled unchanged first (the baseline: the function must
still be byte-exact there, else the source is skipped), then once per perturbation of one function:
  swap     two declarators of a run of declarations at the function's top level exchanged (scalar, pointer or
           array-of-scalar types only, initialisers without calls or side effects that name no local of the run;
           multi-declarator statements are split into one statement per declarator)
  pad      an unused volatile int (pad-v), or an unused volatile char array of 8/16/32 bytes with one store
           (pad-aN), inserted at the top of the body or after its first run of declarations
  split    a scalar local fully reassigned by a top-level statement becomes two locals: the statement declares a
           new one and every later use names it (not when the local's address is taken, it is a bare call argument
           (a reference parameter could keep its address), a reference is declared, or the body has labels)
Variants whose code equals the baseline's are dropped, and so are the ones whose frame layout is unchanged (same
frame size and the same sequence of esp-relative displacements); the rest are classed frame-size (the sub esp
differs), escaped (a frame address is computed: lea r, [esp+k], mov r, esp, push esp) or same-frame.
Behaviour-changing controls (bad-init: a swapped local's integer literal initialiser + 1) are generated alongside;
`negatives` proves a sample of them with the frame model, and the mutants of a sample of the frame model's new
proofs (prove.py's self-test), all of which must be rejected.

prove: each variant in its own process (prove-one: rename, angr, region, no mutants) against the original
function (the M&B mode: retail_func(va) vs the variant's object). With --frame-model only the region layer runs
(rename and angr do not use it), for the variants the run without it did not prove. Rows go to jsonl files under
work/equiv/perturb/; only aggregate counts are ever committed (docs/equiv_corpus.md).
"""
import argparse, collections, glob, hashlib, json, os, random, re, signal, subprocess, sys, threading, time
from concurrent.futures import ThreadPoolExecutor, as_completed

HERE = os.path.dirname(os.path.abspath(__file__))
EQ = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(EQ))
WORK = os.path.join(ROOT, "work", "equiv", "perturb")
sys.path.insert(0, EQ)

KEYWORDS = {"return", "delete", "goto", "throw", "case", "default", "new", "sizeof", "if", "else", "for", "while",
            "do", "switch", "break", "continue", "typedef", "using", "try", "catch", "static", "extern", "register",
            "operator", "this", "template", "typename", "class", "struct", "union", "enum", "__asm", "_asm",
            "asm", "namespace", "public", "private", "protected", "friend", "inline", "virtual"}
SCALARS = {"int", "char", "short", "long", "float", "double", "bool", "unsigned", "signed", "__int64", "__int32",
           "__int16", "__int8", "BYTE", "WORD", "DWORD", "UINT", "INT", "BOOL", "LONG", "ULONG", "USHORT", "UCHAR",
           "size_t", "uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t", "FLOAT", "HRESULT",
           "uint", "uchar", "ushort", "ulong"}
QUALS = {"const", "volatile", "unsigned", "signed", "struct"}


# ============================================================================ a small C++ token scanner
TOK = re.compile(r"""(?P<ws>\s+)|(?P<lc>//[^\n]*)|(?P<bc>/\*.*?\*/)|(?P<pp>^[ \t]*\#[^\n]*(?:\\\n[^\n]*)*)|
    (?P<str>L?"(?:\\.|[^"\\\n])*")|(?P<chr>L?'(?:\\.|[^'\\\n])*')|(?P<num>\.?[0-9](?:[0-9a-zA-Z_.]|[eE][+-])*)|
    (?P<id>[A-Za-z_$][A-Za-z0-9_$]*)|(?P<op>::|->\*?|\+\+|--|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^!=<>]=|\.\.\.|.)""",
                 re.S | re.M | re.X)


def tokens(text):
    out = []
    for m in TOK.finditer(text):
        k = m.lastgroup
        if k in ("ws", "lc", "bc", "pp"):
            continue
        out.append((k, m.group(), m.start(), m.end()))
    return out


def match_close(toks, i, o, c):
    d = 0
    for j in range(i, len(toks)):
        v = toks[j][1]
        if v == o:
            d += 1
        elif v == c:
            d -= 1
            if d == 0:
                return j
    return None


def qualname(sym):
    """decorated name -> ["Scope", ..., "name"] (out-of-class definitions and free functions), None otherwise"""
    m = re.match(r"\?\?([01])((?:[A-Za-z_][A-Za-z0-9_]*@)+)@", sym)
    if m:
        parts = m.group(2).rstrip("@").split("@")[::-1]
        cls = parts[-1]
        return parts + (["~" + cls] if m.group(1) == "1" else [cls])
    m = re.match(r"\?([A-Za-z_][A-Za-z0-9_]*)@((?:[A-Za-z_][A-Za-z0-9_]*@)*)@", sym)
    if not m:
        return None
    scopes = [s for s in m.group(2).split("@") if s][::-1]
    return scopes + [m.group(1)]


def find_body(toks, qual):
    """token indices (open brace, close brace) of the one definition of qual, or None"""
    n, found = len(qual), []
    for i in range(len(toks)):
        k = 0
        j = i
        ok = True
        for p, part in enumerate(qual):
            if p:
                if j < len(toks) and toks[j][1] == "::":
                    j += 1
                else:
                    ok = False
                    break
            if part.startswith("~"):
                if j < len(toks) and toks[j][1] == "~":
                    j += 1
                    part = part[1:]
                else:
                    ok = False
                    break
            if j < len(toks) and toks[j][0] == "id" and toks[j][1] == part:
                j += 1
            else:
                ok = False
                break
        if not ok or j >= len(toks) or toks[j][1] != "(":
            continue
        if i and toks[i - 1][1] in ("::", ".", "->", "~"):
            continue
        if n == 1 and (not i or toks[i - 1][0] not in ("id",) and toks[i - 1][1] not in ("*", "&")):
            continue
        cp = match_close(toks, j, "(", ")")
        if cp is None:
            continue
        q = cp + 1
        while q < len(toks) and toks[q][1] in ("const", "throw"):
            q += 1
            if q < len(toks) and toks[q][1] == "(":
                q = (match_close(toks, q, "(", ")") or q) + 1
        if q < len(toks) and toks[q][1] == ":":           # a constructor's initialiser list
            while q < len(toks) and toks[q][1] not in ("{", ";"):
                if toks[q][1] == "(":
                    q = match_close(toks, q, "(", ")") or q
                q += 1
        if q < len(toks) and toks[q][1] == "{":
            e = match_close(toks, q, "{", "}")
            if e is not None:
                found.append((q, e))
    return found[0] if len(found) == 1 else None


def segments(toks, b, e):
    """the top-level statements of the body (toks[b] = "{", toks[e] = "}"): [(first, last, kind)], last = the
    index of the closing ';' (simple) or '}' (compound); kind "simple" or "other\""""
    out, i = [], b + 1
    while i < e:
        s = i
        first = toks[i][1]
        compound = first in ("if", "for", "while", "switch", "do", "else", "try", "{", "__asm", "_asm") or (
            toks[i][0] == "id" and i + 1 < e and toks[i + 1][1] == ":" and toks[i][1] not in ("default",))
        depth, par, init = 0, 0, False
        while i < e:
            v = toks[i][1]
            if v == "(":
                par += 1
            elif v == ")":
                par -= 1
            elif v == "=" and depth == 0 and par == 0:
                init = True
            elif v == "{":
                if not init:
                    compound = True
                depth += 1
            elif v == "}":
                depth -= 1
                if depth == 0 and not init:
                    nx = toks[i + 1][1] if i + 1 < e else ""
                    if nx == "else" or nx == "catch" or (first == "do" and nx == "while"):
                        i += 1
                        continue
                    break
            elif v == ";" and depth == 0 and par == 0:
                break
            i += 1
        out.append((s, i, "other" if compound else "simple"))
        i += 1
    return out


def parse_decl(toks, s, t):
    """a simple declaration statement toks[s:t] (t = its ';') -> (type tokens, [declarator dict]) or None"""
    i = s
    ty = []
    while i < t and toks[i][1] in QUALS:
        ty.append(toks[i][1])
        i += 1
    if i >= t or toks[i][0] != "id" or toks[i][1] in KEYWORDS:
        return None
    ty.append(toks[i][1])
    i += 1
    while i + 1 < t and toks[i][1] == "::" and toks[i + 1][0] == "id":
        ty += ["::", toks[i + 1][1]]
        i += 2
    if i < t and toks[i][1] == "<":
        d = 0
        while i < t:
            ty.append(toks[i][1])
            d += toks[i][1] == "<"
            d -= toks[i][1] == ">"
            i += 1
            if d == 0:
                break
    while i < t and toks[i][0] == "id" and toks[i][1] in SCALARS and ty[-1] in SCALARS | QUALS:
        ty.append(toks[i][1])                           # unsigned int, long long
        i += 1
    decls = []
    while i < t:
        d = {"ptr": 0, "arr": "", "init": None, "ref": False}
        while i < t and toks[i][1] in ("*", "const"):
            d["ptr"] += toks[i][1] == "*"
            i += 1
        if i < t and toks[i][1] == "&":
            d["ref"] = True
            i += 1
        if i >= t or toks[i][0] != "id" or toks[i][1] in KEYWORDS:
            return None
        d["name"] = toks[i][1]
        i += 1
        arr = []
        while i < t and toks[i][1] == "[":
            c = match_close(toks, i, "[", "]")
            if c is None or c > t:
                return None
            arr.append(toks[i:c + 1])
            i = c + 1
        d["arr"] = arr
        if i < t and toks[i][1] == "(":
            return None                                 # a constructor call or a function declaration
        if i < t and toks[i][1] == "=":
            j, dd = i + 1, 0
            while j < t and not (toks[j][1] == "," and dd == 0):
                dd += toks[j][1] in "({["
                dd -= toks[j][1] in ")}]"
                j += 1
            d["init"] = toks[i + 1:j]
            i = j
        decls.append(d)
        if i < t and toks[i][1] == ",":
            i += 1
            continue
        if i != t:
            return None
    return (ty, decls) if decls else None


def side_effect_free(init):
    if init is None:
        return True
    for k, v, _a, _b in init:
        if v in ("(", "++", "--", "new", "delete", "=", ",") or v.endswith("=") and v not in ("==", "!=", "<=", ">="):
            return False
    return True


def swappable(ty, d):
    base = [x for x in ty if x not in QUALS]
    if "volatile" in ty or d["ref"]:
        return False
    return d["ptr"] > 0 or (len(base) >= 1 and all(x in SCALARS for x in base))


def text_of(src, toks, a, b):
    """source text of toks[a..b] inclusive"""
    return src[toks[a][2]:toks[b][3]]


def decl_text(src, ty, d):
    s = " ".join(ty) + " " + "*" * d["ptr"] + d["name"]
    for a in d["arr"]:
        s += src[a[0][2]:a[-1][3]]
    if d.get("init_text") is not None:
        s += " = " + d["init_text"]
    elif d["init"]:
        s += " = " + src[d["init"][0][2]:d["init"][-1][3]]
    return s + ";"


def perturbations(src, sym, rng):
    """[(kind, new source)] for the function sym of src, plus [("bad-init", ...)] controls"""
    qual = qualname(sym)
    if not qual:
        return []
    toks = tokens(src)
    fb = find_body(toks, qual)
    if fb is None:
        return []
    b, e = fb
    body = toks[b:e + 1]
    if any(v in ("goto", "__asm", "_asm", "setjmp", "longjmp", "alloca", "_alloca") for _k, v, _x, _y in body):
        return []
    segs = segments(toks, b, e)
    out = []
    # runs of consecutive top-level declarations
    decls = []
    for s, t, kind in segs:
        pd = parse_decl(toks, s, t) if kind == "simple" else None
        decls.append(pd)
    runs, cur = [], []
    for k, pd in enumerate(decls):
        if pd is not None:
            cur.append(k)
        else:
            if cur:
                runs.append(cur)
            cur = []
    if cur:
        runs.append(cur)
    has_ref = any(d["ref"] for pd in decls if pd for d in pd[1])
    all_locals = {d["name"]: (pd[0], d) for pd in decls if pd for d in pd[1]}

    def replace_run(run, items):
        """the source with the statements of run replaced by one statement per declarator (items: (ty, d))"""
        a = toks[segs[run[0]][0]][2]
        z = toks[segs[run[-1]][1]][3]
        return src[:a] + " ".join(decl_text(src, ty, d) for ty, d in items) + src[z:]

    for run in runs:
        items = [(decls[k][0], d) for k in run for d in decls[k][1]]
        names = {d["name"] for _ty, d in items}
        ok = all(swappable(ty, d) and side_effect_free(d["init"]) and
                 not ({v for _k, v, _a, _b in (d["init"] or [])} & names) for ty, d in items)
        if not ok or len(items) < 2:
            continue
        pairs = [(i, j) for i in range(len(items)) for j in range(i + 1, len(items))]
        rng.shuffle(pairs)
        for i, j in pairs[:4]:
            it = list(items)
            it[i], it[j] = it[j], it[i]
            out.append(("swap", replace_run(run, it), "%s<->%s" % (items[i][1]["name"], items[j][1]["name"])))
            # the behaviour-changing control: a swapped local's integer literal initialiser + 1
            for x in (i, j):
                ini = items[x][1]["init"]
                if ini and len(ini) == 1 and ini[0][0] == "num" and re.fullmatch(r"\d+", ini[0][1]):
                    it2 = list(it)
                    pos = it2.index(items[x])
                    it2[pos] = (items[x][0], dict(items[x][1], init_text=str(int(ini[0][1]) + 1)))
                    out.append(("bad-init", replace_run(run, it2), "%s=%d" % (items[x][1]["name"],
                                                                               int(ini[0][1]) + 1)))
                    break
    # pads
    at_top = toks[b][3]
    after_decls = toks[segs[runs[0][-1]][1]][3] if runs else at_top
    out.append(("pad-v", src[:at_top] + " volatile int __pert_v = 0; " + src[at_top:], "top"))
    for n, pos, where in ((16, at_top, "top"), (8, after_decls, "after-decls"), (32, after_decls, "after-decls")):
        out.append(("pad-a%d" % n, src[:pos] + " volatile char __pert_a[%d]; __pert_a[0] = 0; " % n + src[pos:],
                    where))
    # splits
    if not has_ref and not any(toks[k][0] == "id" and k + 1 < e and toks[k + 1][1] == ":" and
                               toks[k][1] not in ("default", "public", "private", "protected")
                               for k in range(b + 1, e)):
        seen_assign = collections.Counter()
        nsplit = 0
        for k, (s, t, kind) in enumerate(segs):
            if kind != "simple" or decls[k] is not None:
                continue
            if t - s < 3 or toks[s][0] != "id" or toks[s + 1][1] != "=":
                continue
            nm = toks[s][1]
            if nm not in all_locals:
                continue
            ty, d = all_locals[nm]
            if d["arr"] or d["ref"] or "const" in ty or not swappable(ty, d):
                continue
            seen_assign[nm] += 1
            declared_before = any(decls[q] and any(x["name"] == nm for x in decls[q][1]) for q in range(k))
            if not declared_before:
                continue
            if d["init"] is None and seen_assign[nm] < 2:
                continue                                 # its first value: nothing to split
            bad = False
            for q in range(b + 1, e):
                if toks[q][0] == "id" and toks[q][1] == nm:
                    if toks[q - 1][1] == "&" and toks[q - 2][1] in ("(", ",", "=", "return", "{", ";"):
                        bad = True
                    if toks[q - 1][1] in ("(", ",") and toks[q + 1][1] in (")", ","):
                        bad = True                       # a bare call argument (a reference parameter?)
            if bad:
                continue
            new = nm + "__s"
            rest_a = toks[t][3]
            rest = []
            for q in range(t + 1, e):
                if toks[q][0] == "id" and toks[q][1] == nm and toks[q - 1][1] not in (".", "->", "::"):
                    rest.append(q)
            if not rest:
                continue
            s2 = src[:toks[s][2]] + " ".join(ty) + " " + "*" * d["ptr"] + new + src[toks[s][3]:rest_a]
            last = rest_a
            pieces = []
            for q in rest:
                pieces.append(src[last:toks[q][2]] + new)
                last = toks[q][3]
            s2 += "".join(pieces) + src[last:]
            out.append(("split", s2, nm))
            nsplit += 1
            if nsplit >= 2:
                break
    return out


# ============================================================================ manifests, compile, classify
def manifest_rows():
    rows = []
    files = glob.glob(os.path.join(ROOT, "src", "**", "*.manifest.txt"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "match", "slices", "*", "manifest.txt"))
    for f in sorted(files):
        for ln in open(f):
            p = ln.split()
            if len(p) < 3 or ln.lstrip().startswith("#"):
                continue
            src = os.path.normpath(os.path.join(ROOT, "match", p[0]))
            if not src.startswith(os.path.join(ROOT, "src") + os.sep):
                continue
            rows.append({"src": os.path.relpath(src, ROOT), "symbol": p[1], "va": p[2], "flags": p[3:]})
    seen, out = set(), []
    for r in rows:
        k = (r["src"], r["symbol"])
        if k not in seen:
            seen.add(k)
            out.append(r)
    return out


def head_text(rel):
    r = subprocess.run(["git", "show", "HEAD:" + rel], cwd=ROOT, capture_output=True)
    return r.stdout.decode("latin-1") if r.returncode == 0 else None


def flags_for(row, src_abs):
    import func as FN
    fl = list(row["flags"]) or FN.default_flags(src_abs)
    if row["src"].startswith("src/game/") and "/Isrc/game/include" not in fl:
        fl.append("/Isrc/game/include")
    # the original's directory (its quoted includes), relative: slice.resolve_include_flags makes it a wine path
    # the laptop's compile server accepts
    return fl + ["/I" + os.path.dirname(row["src"])]


def compile_text(row, text, tag):
    import slice as S
    d = os.path.join(WORK, "src", tag)
    os.makedirs(d, exist_ok=True)
    p = os.path.join(d, os.path.basename(row["src"]))
    if not os.path.exists(p) or open(p, encoding="latin-1").read() != text:
        open(p, "w", encoding="latin-1").write(text)
    obj, log = S.compile_obj(p, flags_for(row, p))
    return obj, log


def esp_shape(code):
    """(frame size of the prologue, the sequence of esp-relative displacements, escaped?)"""
    import angr_layer as AL
    from capstone import x86_const as X
    disp, esc = [], False
    for i in AL.decode(code, 0):
        for o in i.operands:
            if o.type == X.X86_OP_MEM and o.mem.base and i.reg_name(o.mem.base) == "esp":
                disp.append(o.mem.disp)
        if i.mnemonic == "lea" and re.search(r"\[(esp|ebp)", i.op_str):
            esc = True
        if (i.mnemonic == "mov" and re.fullmatch(r"e\w\w, esp", i.op_str)) or \
                (i.mnemonic == "push" and i.op_str == "esp"):
            esc = True
    return AL.frame_alloc(code, 0), disp, esc


def masked(f):
    b = bytearray(f.code)
    for o in f.relocs:
        b[o:o + 4] = b"\0\0\0\0"
    return bytes(b)


def gen(a):
    import func as FN
    import angr_layer as AL
    rng = random.Random(a.seed)
    rows = manifest_rows()
    # functions whose original allocates a frame of at least 8 bytes
    cand = []
    for r in rows:
        try:
            rf = FN.retail_func(int(r["va"], 16))
        except BaseException:
            continue
        n = len(AL.decode(rf.code, rf.va))
        if not 8 <= n <= a.max_insns:
            continue
        sub = 0
        for i in AL.decode(rf.code, rf.va)[:12]:
            if i.mnemonic == "sub" and i.op_str.startswith("esp, "):
                try:
                    sub = int(i.op_str[5:], 0)
                except ValueError:
                    pass
                break
        if sub >= 8:
            r["n"] = n
            r["frame"] = sub
            cand.append(r)
    rng.shuffle(cand)
    print("%d manifest rows, %d with a frame of 8+ bytes and %d-%d instructions" % (
        len(rows), len(cand), 8, a.max_insns), flush=True)
    os.makedirs(WORK, exist_ok=True)
    out_p = os.path.join(WORK, "variants.jsonl")
    done = set()
    if a.resume and os.path.exists(out_p):
        done = {json.loads(l)["fid"] for l in open(out_p) if l.strip()}
    base_ok = {}
    lock = threading.Lock()
    stats = collections.Counter()

    def one(r):
        fid = hashlib.sha1(("%s|%s" % (r["src"], r["symbol"])).encode()).hexdigest()[:12]
        if fid in done:
            return []
        text = head_text(r["src"])
        if text is None:
            return [{"fid": fid, "skip": "no HEAD source"}]
        if re.search(r"namespace\s*\{", text):
            return [{"fid": fid, "skip": "anonymous namespace (compiles only on this Mac)"}]
        stag = hashlib.sha1(r["src"].encode()).hexdigest()[:10]
        with lock:
            bk = base_ok.get(r["src"])
        if bk is None:
            obj, log = compile_text(r, text, "base_" + stag)
            bk = obj
            with lock:
                base_ok[r["src"]] = bk
        if not bk:
            return [{"fid": fid, "skip": "baseline compile failed"}]
        try:
            base_f = FN.obj_func(bk, r["symbol"])
            rf = FN.retail_func(int(r["va"], 16))
            if not FN.is_byte_exact(rf, base_f):
                return [{"fid": fid, "skip": "baseline not byte-exact"}]
        except BaseException as ex:
            return [{"fid": fid, "skip": "baseline: %s" % str(ex)[:100]}]
        ps = perturbations(text, r["symbol"], random.Random(int(fid[:8], 16)))
        if not ps:
            return [{"fid": fid, "skip": "no perturbation (function not found or nothing to change)"}]
        bshape = esp_shape(base_f.code)
        res = []
        for k, (kind, new, what) in enumerate(ps):
            vtag = "%s_%02d" % (fid, k)
            obj, log = compile_text(r, new, vtag)
            row = {"fid": fid, "vid": vtag, "src": r["src"], "symbol": r["symbol"], "va": r["va"], "kind": kind,
                   "what": what, "n": r["n"], "frame": r["frame"]}
            if not obj:
                row["cls"] = "compile-failed"
                row["log"] = (log or "")[-300:]
                res.append(row)
                continue
            try:
                vf = FN.obj_func(obj, r["symbol"])
            except BaseException as ex:
                row["cls"] = "no-symbol"
                res.append(row)
                continue
            row["obj"] = os.path.relpath(obj, ROOT)
            if masked(vf) == masked(base_f) and sorted(vf.relocs.items()) == sorted(base_f.relocs.items()):
                row["cls"] = "same-code"
                res.append(row)
                continue
            vshape = esp_shape(vf.code)
            row["frame_v"] = vshape[0]
            row["n_v"] = len(AL.decode(vf.code, 0))
            if vshape[0] != bshape[0]:
                row["cls"] = "frame-size"
            elif vshape[1] == bshape[1]:
                row["cls"] = "same-layout"
            else:
                row["cls"] = "same-frame"
            row["esc"] = bool(vshape[2] or bshape[2])
            res.append(row)
        return res
    with open(out_p, "a") as out, ThreadPoolExecutor(a.jobs) as ex:
        futs = []
        # one source at a time per worker: the baseline compile of a source is shared
        by_src = collections.OrderedDict()
        for r in cand[:a.max_funcs]:
            by_src.setdefault(r["src"], []).append(r)
        def per_src(rs):
            o = []
            for r in rs:
                o += one(r)
            return o
        futs = [ex.submit(per_src, rs) for rs in by_src.values()]
        for k, f in enumerate(as_completed(futs), 1):
            try:
                rs = f.result()
            except BaseException as e:
                print("error: %s" % e, flush=True)
                continue
            with lock:
                for row in rs:
                    out.write(json.dumps(row) + "\n")
                    stats[row.get("cls") or row.get("skip")] += 1
                out.flush()
            if k % 10 == 0:
                print("sources %d/%d: %s" % (k, len(futs), dict(stats)), flush=True)
    print(dict(stats))


# ============================================================================ proving
def prove_one(a):
    """(child process) prove one variant against the original; prints one JSON line"""
    import func as FN
    import prove as PV
    if a.remote:
        import remote
        remote.enable()
    retail = FN.retail_func(int(a.va, 16))
    ours = FN.obj_func(os.path.join(ROOT, a.obj), a.symbol)
    ours.meta.update({"src": a.src or "", "flags": [], "ret64": FN.ret64(a.symbol)})
    tmo = parse_tmo(a.tmo)
    layers = tuple(a.layers.split(","))
    res = PV.run_funcs(retail, ours, None, a.mutants, True, lambda s: None, tmo, layers)
    res.pop("bundle", None)
    sys.stdout.write("\n" + json.dumps(res, default=str) + "\n")


def parse_tmo(s):
    return {k: float(v) for k, v in (x.split("=") for x in s.split(",") if x)}


def run_variant(v, tmo, layers, remote, remote_only, frame_model, mutants=0, extra_env=None):
    total = 60 + sum(tmo.values()) * (1 + mutants) + 120
    cmd = ["uv", "run", "--quiet", "python", os.path.abspath(__file__), "prove-one", v["obj"], v["symbol"], v["va"],
           "--src", v["src"], "--tmo", ",".join("%s=%g" % kv for kv in sorted(tmo.items())), "--layers",
           ",".join(layers), "--mutants", str(mutants)]
    if remote:
        cmd.append("--remote")
    env = dict(os.environ)
    if remote_only:
        env["EQUIV_ANGR_LOCAL"] = "/usr/bin/false"
    env["EQUIV_FRAME_MODEL"] = "1" if frame_model else "0"
    if "region" in tmo:
        env["EQUIV_REGION_TIMEOUT"] = str(min(120.0, max(30.0, tmo["region"] / 5)))
    env.update(extra_env or {})
    t = time.time()
    p = subprocess.Popen(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env,
                         start_new_session=True)
    try:
        out, _ = p.communicate(timeout=total)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL)
        p.communicate()
        return {"verdict": "UNKNOWN", "detail": "hard timeout after %.0fs" % total, "seconds": time.time() - t}
    txt = out.decode(errors="replace")
    k = txt.rfind("\n{")
    res = None
    if k >= 0:
        try:
            res = json.loads(txt[k + 1:])
        except ValueError:
            res = None
    if res is None:
        res = {"verdict": "ERROR", "detail": txt[-400:]}
    res["seconds"] = round(time.time() - t, 1)
    return res


def load_rows(p):
    if not os.path.exists(p):
        return {}
    return {r["vid"]: r for r in (json.loads(l) for l in open(p) if l.strip())}


def variants(kinds=None):
    out = []
    for r in (json.loads(l) for l in open(os.path.join(WORK, "variants.jsonl")) if l.strip()):
        if r.get("cls") in ("same-frame", "frame-size") and (kinds is None or r["kind"] in kinds):
            out.append(r)
    return out


def blocker(res):
    d = res.get("detail") or ""
    lay = res.get("layers") or {}
    reg = (lay.get("region") or ["", ""])[1] if lay.get("region") else ""
    s = reg or d
    m = re.search(r'"what":"([^"]*)"', s)
    if m:
        s = m.group(1)
    for pat, name in ((r"esp differs|esp at a call|call#\d+ esp|exit#\d+ esp|ret#\d+ esp", "frame: esp delta"),
                      (r"argument bytes|frame protection", "frame: call arguments"),
                      (r"call#\d+ memory", "frame: memory at a call (escaped or unprotected)"),
                      (r"exit#\d+ memory|frame -0x|frame slot", "frame: memory at a stop"),
                      (r"remapped frame", "frame: pointer into a remapped frame"),
                      (r"reads the stack below", "stack below esp"),
                      (r"call into the function", "recursive call"),
                      (r"timeout", "timeout"), (r"x87", "x87"), (r"paths", "paths"),
                      (r"call count|calls, ours", "call count"), (r"kill set|entry registers", "facts")):
        if re.search(pat, s):
            return name
    return "other: " + s[:60]


def prove_cmd(a):
    vs = [v for v in variants() if v["kind"] != "bad-init"]
    vs.sort(key=lambda v: (v["cls"] != "same-frame", v["kind"] != "swap", v["n"]))
    if a.limit:
        vs = vs[:a.limit]
    out_p = a.out or os.path.join(WORK, "results_fm.jsonl" if a.frame_model else "results.jsonl")
    seen = load_rows(out_p) if a.resume else {}
    tmo = parse_tmo(a.tmo)
    if a.frame_model:
        base = load_rows(os.path.join(WORK, "results.jsonl"))
        todo = [v for v in vs if v["vid"] in base and base[v["vid"]].get("verdict") not in ("EQUIVALENT", "MATCH")
                and v["vid"] not in seen]
        layers = ("region",)
        tmo = {k: v for k, v in tmo.items() if k == "region"}
    else:
        todo = [v for v in vs if v["vid"] not in seen]
        layers = ("rename", "angr", "region")
    print("%d variants, %d to run (%s)" % (len(vs), len(todo), ",".join(layers)), flush=True)
    lock = threading.Lock()
    with open(out_p, "a") as out, ThreadPoolExecutor(a.jobs) as ex:
        futs = {ex.submit(run_variant, v, tmo, layers, a.remote, a.remote_only, a.frame_model): v for v in todo}
        for k, f in enumerate(as_completed(futs), 1):
            v = futs[f]
            res = f.result()
            row = {"vid": v["vid"], "cls": v["cls"], "kind": v["kind"], "esc": v.get("esc"), "n": v["n"],
                   "verdict": res.get("verdict"), "layer": res.get("layer"), "level": res.get("level"),
                   "detail": (res.get("detail") or "")[:1200], "layers": res.get("layers") or {},
                   "seconds": res.get("seconds")}
            row["blocker"] = blocker(row) if row["verdict"] not in ("EQUIVALENT", "MATCH") else None
            with lock:
                out.write(json.dumps(row) + "\n")
                out.flush()
            if k % 10 == 0 or k == len(futs):
                print("%d/%d" % (k, len(futs)), flush=True)


def negatives(a):
    """task 3: behaviour-changing controls must not prove (frame model on); the mutants of a sample of the frame
    model's new proofs must all be rejected"""
    rng = random.Random(a.seed)
    out_p = os.path.join(WORK, "negatives.jsonl")
    seen = load_rows(out_p)
    allv = [json.loads(l) for l in open(os.path.join(WORK, "variants.jsonl")) if l.strip()]
    bad = [v for v in allv if v.get("kind") == "bad-init" and v.get("obj") and v.get("cls") not in ("same-code",)]
    rng.shuffle(bad)
    fm = load_rows(os.path.join(WORK, "results_fm.jsonl"))
    new = [r for r in fm.values() if r["verdict"] == "EQUIVALENT"]
    rng.shuffle(new)
    # and region proofs made without the flag, re-proved with it (every region then runs the frame model's
    # obligations: F' at stops, protected calls): their self-test exercises the model on proved functions too
    base = load_rows(os.path.join(WORK, "results.jsonl"))
    reg = [r for r in base.values() if r["verdict"] == "EQUIVALENT" and r.get("layer") == "region"]
    rng.shuffle(reg)
    new = new + reg[:max(0, a.sample - len(new))]
    vby = {v["vid"]: v for v in allv if "vid" in v}
    tmo = parse_tmo(a.tmo)
    jobs = []
    for v in bad[:a.sample]:
        if "bad:" + v["vid"] not in seen:
            jobs.append(("bad", v, ("rename", "angr", "region"), 0))
    for r in new[:a.sample]:
        if "mut:" + r["vid"] not in seen:
            jobs.append(("mut", vby[r["vid"]], ("region",), a.mutants))
    print("%d controls, %d new proofs' self-tests to run" % (sum(1 for j in jobs if j[0] == "bad"),
                                                            sum(1 for j in jobs if j[0] == "mut")), flush=True)
    lock = threading.Lock()
    with open(out_p, "a") as out, ThreadPoolExecutor(a.jobs) as ex:
        futs = {ex.submit(run_variant, v, tmo if kind == "bad" else {"region": tmo.get("region", 300)}, layers,
                          a.remote, a.remote_only, True, mut,
                          {"EQUIV_FULL_MUTANTS": "0"}): (kind, v) for kind, v, layers, mut in jobs}
        for f in as_completed(futs):
            kind, v = futs[f]
            res = f.result()
            row = {"vid": kind + ":" + v["vid"], "kind": kind, "verdict": res.get("verdict"),
                   "detail": (res.get("detail") or "")[:1500], "layers": res.get("layers") or {},
                   "mutants": res.get("mutants"), "seconds": res.get("seconds")}
            with lock:
                out.write(json.dumps(row) + "\n")
                out.flush()
            print(row["vid"], row["verdict"], flush=True)


def summary(a):
    vs = [json.loads(l) for l in open(os.path.join(WORK, "variants.jsonl")) if l.strip()]
    base = load_rows(os.path.join(WORK, "results.jsonl"))
    fm = load_rows(os.path.join(WORK, "results_fm.jsonl"))
    L = []
    gen_c = collections.Counter(v.get("cls") or "skipped" for v in vs if "vid" in v)
    L.append("functions: %d tried, %d skipped (%s)" % (
        len({v["fid"] for v in vs}), sum(1 for v in vs if "skip" in v),
        dict(collections.Counter(v["skip"].split(":")[0] for v in vs if "skip" in v))))
    L.append("variants generated: %s" % dict(gen_c))
    kinds = collections.Counter((v["kind"], v.get("cls")) for v in vs if "vid" in v)
    L.append("by kind: %s" % {"%s/%s" % k: n for k, n in sorted(kinds.items())})
    rows = [v for v in vs if v.get("cls") in ("same-frame", "frame-size") and v["kind"] != "bad-init"]
    groups = collections.OrderedDict()
    for v in rows:
        g = v["cls"] + (" escaped" if v.get("esc") else "")
        groups.setdefault(g, []).append(v)
    L.append("%-22s %6s %10s %10s %6s" % ("class", "n", "flag off", "flag on", "new"))
    tot = [0, 0, 0]
    for g, lst in sorted(groups.items()):
        ran = [v for v in lst if v["vid"] in base]
        off = sum(1 for v in ran if base[v["vid"]]["verdict"] == "EQUIVALENT")
        on = off + sum(1 for v in ran if v["vid"] in fm and fm[v["vid"]]["verdict"] == "EQUIVALENT")
        L.append("%-22s %6d %10d %10d %6d" % (g, len(ran), off, on, on - off))
        tot = [tot[0] + len(ran), tot[1] + off, tot[2] + on]
    L.append("%-22s %6d %10d %10d %6d" % ("all", tot[0], tot[1], tot[2], tot[2] - tot[1]))
    lay = collections.Counter(base[v["vid"]].get("layer") for v in rows if v["vid"] in base and
                              base[v["vid"]]["verdict"] == "EQUIVALENT")
    L.append("flag off, proved by layer: %s" % dict(lay))
    for g in sorted(groups):
        bl = collections.Counter()
        for v in groups[g]:
            r = fm.get(v["vid"]) or base.get(v["vid"])
            if r and r["verdict"] not in ("EQUIVALENT", "MATCH"):
                bl[blocker(r)] += 1
        L.append("blockers with the flag on, %s: %s" % (g, dict(bl.most_common(8))))
    kk = collections.Counter()
    for v in rows:
        if v["vid"] in base:
            on = base[v["vid"]]["verdict"] == "EQUIVALENT" or (v["vid"] in fm and fm[v["vid"]]["verdict"] == "EQUIVALENT")
            kk[(v["kind"], on)] += 1
    L.append("by perturbation (proved with the flag on / run): %s" % {
        k: "%d/%d" % (kk[(k, True)], kk[(k, True)] + kk[(k, False)]) for k in sorted({k for k, _ in kk})})
    neg = load_rows(os.path.join(WORK, "negatives.jsonl"))
    if neg:
        nb = [r for r in neg.values() if r["kind"] == "bad"]
        nm = [r for r in neg.values() if r["kind"] == "mut"]
        L.append("negatives: behaviour-changing controls proved %d of %d" % (
            sum(1 for r in nb if r["verdict"] == "EQUIVALENT"), len(nb)))
        mt = [m for r in nm for m in (r.get("mutants") or [])]
        L.append("negatives: self-tests of %d new proofs: %d mutants, %d proved; verdicts %s" % (
            len(nm), len(mt), sum(1 for m in mt if m[1] == "proved"),
            dict(collections.Counter(r["verdict"] for r in nm))))
    print("\n".join(L))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    g = sub.add_parser("gen")
    g.add_argument("--max-funcs", type=int, default=200)
    g.add_argument("--max-insns", type=int, default=400)
    g.add_argument("--seed", type=int, default=1)
    g.add_argument("--jobs", type=int, default=12)
    g.add_argument("--resume", action="store_true")
    p = sub.add_parser("prove")
    p.add_argument("--jobs", type=int, default=5)
    p.add_argument("--remote", action="store_true")
    p.add_argument("--remote-only", action="store_true")
    p.add_argument("--frame-model", action="store_true")
    p.add_argument("--tmo", default="rename=30,angr=60,region=300")
    p.add_argument("--out")
    p.add_argument("--resume", action="store_true")
    p.add_argument("--limit", type=int)
    n = sub.add_parser("negatives")
    n.add_argument("--sample", type=int, default=20)
    n.add_argument("--mutants", type=int, default=6)
    n.add_argument("--seed", type=int, default=1)
    n.add_argument("--jobs", type=int, default=5)
    n.add_argument("--remote", action="store_true")
    n.add_argument("--remote-only", action="store_true")
    n.add_argument("--tmo", default="rename=30,angr=60,region=300")
    sub.add_parser("summary")
    o = sub.add_parser("prove-one")
    o.add_argument("obj")
    o.add_argument("symbol")
    o.add_argument("va")
    o.add_argument("--src")
    o.add_argument("--tmo", default="rename=30,angr=60,region=300")
    o.add_argument("--layers", default="rename,angr,region")
    o.add_argument("--mutants", type=int, default=0)
    o.add_argument("--remote", action="store_true")
    a = ap.parse_args()
    if getattr(a, "jobs", 0) and a.cmd in ("prove", "negatives") and a.jobs > 6:
        sys.exit("--jobs at most 6 (the laptop's angr slots)")
    {"gen": gen, "prove": prove_cmd, "negatives": negatives, "summary": summary, "prove-one": prove_one}[a.cmd](a)


if __name__ == "__main__":
    sys.exit(main())
