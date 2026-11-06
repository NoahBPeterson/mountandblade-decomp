"""Slice bookkeeping for the equivalence harness: rows, flags, compile, symbol lookup.

Read-only with respect to match/: objects are compiled into work/difftest/obj/.
"""
import glob, hashlib, os, re, subprocess, sys, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))

from coff import Coff  # noqa: E402
from winnative import CL, WIN, winpath  # noqa: E402  (cl.sh under wine on the Mac, cl.exe on Windows; wine / native paths)

DEFAULT_FLAGS = ["/O2", "/MT", "/Gy", "/TP"]  # cl 13.10; static CRT; /EHsc only where the original has EH frames
INCLUDE = "/I" + winpath(os.path.join(ROOT, "match", "include"))
OBJDIR = os.environ.get("MB_OBJDIR") or os.path.join(ROOT, "work", "difftest", "obj")
# per-process timing of the compile layer (MB_TIMING reports it): key hashing, compiler runs, cache hits
TIMING = {"key_s": 0.0, "cl_s": 0.0, "cl_runs": 0, "hits": 0, "misses": 0, "pp_s": 0.0, "pp_runs": 0, "pp_hits": 0}

FLAG_RE = re.compile(r"(?<![\w/])(/(?:O[12dxyst]-?|Ob\d|Oi-?|Oy-?|M[DT]d?|Gy-?|EH\w+|TP|TC|arch:\w+|fp:\w+|GS-?|GR-?|Gz|Gr|Gd|Zp\d+|GF|Gs\d*|J|vc71))(?![\w:])")


# an explicit note: 'flags' (or 'Flags:') immediately followed by the flag list
EXPLICIT_RE = re.compile(r"\bflags?\s*[:=]?\s*((?:/[\w:+\-]+(?![\w:+\-])[\s,]*)+)", re.I)


def slice_dir(sid):
    return os.path.join(ROOT, "match", "slices", sid)


def read_rows(path):
    """Rows of a nonmatching/partial file: [(va, diff, reason)]."""
    rows = []
    if not os.path.exists(path):
        return rows
    for line in open(path, encoding="utf-8", errors="replace"):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        p = line.split(None, 2)
        try:
            va = int(p[0], 16)
        except ValueError:
            continue
        rows.append((va, p[1] if len(p) > 1 else "", p[2].strip() if len(p) > 2 else ""))
    return rows


def manifest_rows(sid):
    """[(src, symbol, va, flags-or-None)] from the slice manifest."""
    out = []
    path = os.path.join(slice_dir(sid), "manifest.txt")
    if not os.path.exists(path):
        # a migrated module (src/<area>/<stem>.cpp): its rows are in src/<area>/<stem>.manifest.txt, and the mangled
        # names there are the only binding for compiler-generated functions and same-size template instances
        hits = glob.glob(os.path.join(ROOT, "src", "**", sid + ".manifest.txt"), recursive=True)
        if len(hits) != 1:
            return out
        path = hits[0]
    for line in open(path):
        p = line.split("#", 1)[0].split()
        if len(p) >= 3:
            try:
                out.append((p[0], p[1], int(p[2], 16), p[3:] or None))
            except ValueError:
                pass
    return out


def flags_from_reason(reason):
    """Flags recorded in a nonmatching reason, merged onto the defaults when only extras are given.
    Returns None when the reason names no compiler flag."""
    if not reason:
        return None
    toks = []
    for m in FLAG_RE.finditer(reason):
        pre = reason[max(0, m.start() - 4):m.start()].lower()
        if pre.endswith("no ") or pre.endswith("not "):
            continue
        if m.group(1) not in toks:
            toks.append(m.group(1))
    if not toks:
        return None
    if not any(t.startswith(("/O1", "/O2", "/Od", "/Ox")) for t in toks):
        return _merge(toks, reason)
    if ("/MD" in toks or "/MT" in toks) and "/Gy" in toks:
        flags = _dedupe(toks)
    else:
        flags = _merge(toks, reason)
    if re.search(r"\bno (/EHsc|EH)\b", reason) and "/EHsc" in flags:
        flags.remove("/EHsc")
    return flags


def _family(f):
    for p in ("/O1", "/O2", "/Od", "/Ox"):
        if f.startswith(p):
            return "/O"
    for p in ("/fp:", "/arch:", "/M", "/Ob", "/GS", "/Oy", "/EH"):
        if f.startswith(p):
            return p
    return f


def _dedupe(toks):
    seen, out = set(), []
    for t in toks:
        if _family(t) not in seen:
            seen.add(_family(t))
            out.append(t)
    return out


def _merge(toks, reason):
    toks = _dedupe(toks)
    flags = list(DEFAULT_FLAGS)
    for t in toks:
        if t.startswith(("/O1", "/O2", "/Od", "/Ox")):
            flags = [f for f in flags if not f.startswith(("/O1", "/O2", "/Od", "/Ox"))]
            flags.insert(0, t)
        elif t not in flags:
            flags.append(t)
    return flags


def flags_from_source(src):
    """Flags named in a 'Flags: ...' / 'flags /O2 ...' comment near the top of the source."""
    if not os.path.exists(src):
        return None
    head = open(src, encoding="utf-8", errors="replace").read().split("\n")[:80]
    for line in head:
        if "//" not in line:
            continue
        c = line.split("//", 1)[1]
        m = re.search(r"\bflags?\b|\b(?:compiled?|built|build) with\b", c, re.I)
        if m and FLAG_RE.search(c):
            f = flags_from_reason(c[m.start():])
            if f:
                return f
    return None


def choose_flags(sid, va, override=None):
    """(flags, provenance), first found wins:
    1. --flags (override);
    2. an explicit 'flags /O2 ...' note in this VA's nonmatching reason ("reason");
    3. the flags column of the slice's manifest.txt: the row for this VA, else the rows' flags when
       they all agree ("manifest(va)" / "manifest(slice)"); with no flagged rows, a
       '# ... flags: /Od ...' note in the manifest ("manifest(note)");
    4. a '// Flags: ...' / '// compile with ...' comment near the top of the source ("source comment");
    5. the most common flags of disagreeing manifest rows ("manifest(majority)");
    6. a complete flag list (one naming /O1, /O2, /Od or /Ox) anywhere in the reason ("reason(loose)");
    7. the defaults.
    Lone loose tokens in a reason ("needs /fp:fast") are never merged onto the defaults: they are
    often stale or describe an experiment, and compiling with them gave wrong /fp modes."""
    if override:
        return override.split(), "override"
    for rva, _d, reason in read_rows(os.path.join(slice_dir(sid), "nonmatching.txt")):
        if rva == va:
            for m in EXPLICIT_RE.finditer(reason):
                f = flags_from_reason(m.group(1))
                if f:
                    return f, "reason"
            break
    rows = manifest_rows(sid)
    for _src, _sym, mva, fl in rows:
        if mva == va and fl:
            return fl, "manifest(va)"
    fls = {tuple(r[3]) for r in rows if r[3]}
    if len(fls) == 1:
        return list(fls.pop()), "manifest(slice)"
    if not fls:
        f = manifest_note_flags(sid)
        if f:
            return f, "manifest(note)"
    f2 = flags_from_source(source_path(sid))
    if f2:
        return f2, "source comment"
    if fls:
        allf = [tuple(r[3]) for r in rows if r[3]]
        return list(max(fls, key=lambda f: (allf.count(f), f))), "manifest(majority)"
    for rva, _d, reason in read_rows(os.path.join(slice_dir(sid), "nonmatching.txt")):
        if rva == va:
            toks = [m.group(1) for m in FLAG_RE.finditer(reason)]
            if any(t.startswith(("/O1", "/O2", "/Od", "/Ox")) for t in toks):
                f = flags_from_reason(reason)
                if f:
                    return f, "reason(loose)"
            break
    return list(DEFAULT_FLAGS), "default"


def manifest_note_flags(sid):
    """Flags in a '# ... flags: /Od /Ob1 ...' comment of the slice manifest."""
    path = os.path.join(slice_dir(sid), "manifest.txt")
    if not os.path.exists(path):
        return None
    for line in open(path, encoding="utf-8", errors="replace"):
        if "#" in line:
            for m in EXPLICIT_RE.finditer(line.split("#", 1)[1]):
                f = flags_from_reason(m.group(1))
                if f and any(t.startswith(("/O1", "/O2", "/Od", "/Ox")) for t in f):
                    return f
    return None


def source_path(sid):
    return os.path.join(slice_dir(sid), sid + ".cpp")


def sibling_headers(src):
    """Headers the slice source includes from another slice's directory by relative path
    ('#include "../s00908c00/s00908c00.h"'): their declarations and annotations count too."""
    out = []
    try:
        text = open(src, encoding="utf-8", errors="replace").read()
    except OSError:
        return out
    for m in re.finditer(r'^\s*#\s*include\s+"(\.\.?/[^"]+)"', text, re.M):
        p = os.path.normpath(os.path.join(os.path.dirname(src), m.group(1)))
        if os.path.exists(p) and p not in out:
            out.append(p)
    return out


def resolve_include_flags(flags):
    """Relative /I paths in manifests are relative to work/match (see run_all.resolve_flags); cl would
    resolve them against cwd. Try work/match, then the repo root; absolute and Z: paths are kept."""
    out = []
    for f in flags:
        if f[:2] in ("/I", "-I") and len(f) > 2 and not f[2:].startswith(("Z:", "/", "\\")) and ":" not in f[2:4]:
            for base in (os.path.join(ROOT, "work", "match"), ROOT):
                cand = os.path.join(base, f[2:])
                if os.path.isdir(cand):
                    f = "/I" + winpath(cand)
                    break
        out.append(f)
    return out


_INC = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def _quoted_includes(src):
    """src plus every file it reaches through #include "..." (resolved next to the including file, then in
    match/include), so an edit to any header a TU uses (src/include, ../../rgl/..., ...) invalidates its object."""
    seen, todo = [], [os.path.abspath(src)]
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.append(f)
        try:
            text = open(f, "rb").read().decode("latin-1")
        except OSError:
            continue
        for name in _INC.findall(text):
            for d in (os.path.dirname(f), os.path.join(ROOT, "match", "include")):
                c = os.path.normpath(os.path.join(d, name))
                if os.path.isfile(c):
                    todo.append(c)
                    break
    return sorted(seen)


def compile_key(src, flags):
    """The cache key of compiling src with flags (flags already through resolve_include_flags): source, every header
    it can reach, the flags. compile_obj names its objects by it; other per-source compiler probes
    (relink/data_size_audit.py) cache their results under it, so they invalidate exactly when the object would."""
    h = hashlib.md5()
    # paths enter the key relative to the repo, so git worktrees of the repo share compiled objects. No source
    # uses __FILE__, but cl mangles anonymous-namespace names with a hash of the source path (?A0x101b409b), so a
    # TU that reaches an anonymous namespace keys on its absolute path (run_all looks such symbols up by name).
    anon = False
    for f in _quoted_includes(src):
        h.update(os.path.relpath(f, ROOT).encode())
        text = open(f, "rb").read()
        h.update(text)
        anon = anon or re.search(rb"namespace\s*\{", text) is not None
    if anon:
        h.update(os.path.abspath(src).encode())
    for hdr in sorted(os.listdir(os.path.dirname(src))):
        if hdr.endswith((".h", ".hpp", ".inl")):
            h.update(open(os.path.join(os.path.dirname(src), hdr), "rb").read())
    h.update(" ".join(flags).replace(winpath(ROOT), "<ROOT>").encode())
    # the shared slice headers (match/include, on cl.sh's include path): an edit there must invalidate too
    for f in sorted(glob.glob(os.path.join(ROOT, "match", "include", "*.h"))):
        h.update(open(f, "rb").read())
    if os.path.abspath(src).startswith(os.path.join(ROOT, "src", "game") + os.sep):
        # generated class headers under src/game/include may also be reached through an /I path
        for d, _sub, files in sorted(os.walk(os.path.join(ROOT, "src", "game", "include"))):
            for f in sorted(files):
                if f.endswith((".h", ".hpp", ".inl")):
                    h.update(open(os.path.join(d, f), "rb").read())
    return h.hexdigest()


# Second-level cache (cache scheme "pp1"): compile_key is deliberately coarse (every header the TU can reach, every header
# in its directory and in match/include), so one shared-header edit misses for nearly every module even when the edit
# (a comment, a member nobody reaches) cannot change what the compiler sees. On a coarse miss the source is run through
# `cl /EP` (same flags and include path), and the object is keyed by the preprocessed TEXT: a module whose
# preprocessed output is unchanged reuses its object. The coarse name is hard-linked to the object afterwards, so the
# next lookup is again one stat. Things the text does not carry stay in the key: the flags, the source's repo-relative
# path (cl records the source name), the absolute path when the TU reaches an anonymous namespace (cl hashes it into
# the mangled names); debug-info flags bypass the level (a /Z7 object embeds the header paths).
PPDIR = os.environ.get("MB_PPDIR") or (OBJDIR.rstrip("/\\") + "_pp")         # objects named by preprocessed-text key
PPMAP = os.path.join(PPDIR, "key")                                           # coarse compile_key -> preprocessed-text key
PP_ENABLED = os.environ.get("MB_PP", "1") != "0"
DEBUG_FLAGS = ("/Z7", "/Zi", "/ZI", "/Zd", "-Z7", "-Zi", "-ZI", "-Zd")


def _pp_eligible(flags):
    return PP_ENABLED and not any(f in DEBUG_FLAGS for f in flags)


def _norm_flags(flags):
    return " ".join(flags).replace(winpath(ROOT), "<ROOT>")


def search_path(flags):
    """cl's include search order for a compile through compile_obj: the /I flags, slice's own /I, then cl.sh's INCLUDE."""
    from winnative import include_dirs
    out = []
    for f in list(flags) + [INCLUDE]:
        if f[:2] in ("/I", "-I") and len(f) > 2:
            p = f[2:]
            if p[:2].upper() == "Z:":
                p = p[2:]
            out.append(p.replace("\\", "/") if not WIN else p)
    return out + include_dirs()


# ppkey (py) costs 1.3-2.3 GB and up to 30 s per miss. That paid for itself against 9-31 s under wine, but not against the
# compile server's 2 s, so it is off by default when the server is configured.
_RC_ON = os.environ.get("MB_RC") != "0" and (os.environ.get("MB_RC_ENDPOINT") or os.path.exists(os.path.expanduser("~/.config/mb-compile-server")))
PP_MODE = os.environ.get("MB_PP_MODE", "off" if _RC_ON else "py")      # py: ppkey.py (no compiler run) | cl: cl /EP (exact text, one wine run) | off


def anon_path_matters(src):
    """True when the TU reaches an anonymous namespace: cl mangles it with a hash of the source path, so the absolute path
    stays in the key (the same test compile_key applies to the quoted-include closure)."""
    for f in _quoted_includes(src):
        try:
            if re.search(rb"namespace\s*\{", open(f, "rb").read()):
                return True
        except OSError:
            pass
    return False


def pp_key(src, flags, coarse=None):
    """Key of what the compiler sees (see the block comment above): None when not eligible or not determinable (the
    caller then compiles under the coarse key only). `flags` already through resolve_include_flags."""
    if not _pp_eligible(flags) or PP_MODE == "off":
        return None
    if PP_MODE == "py":
        import ppkey
        ppkey.load_index(os.path.join(PPDIR, "idx"))
        extra = (_norm_flags(flags) + "\0" + os.path.relpath(src, ROOT)).encode()
        k = ppkey.closure_key(src, search_path(flags), ROOT, extra)
        if k and anon_path_matters(src):
            k = hashlib.md5((k + os.path.abspath(src)).encode()).hexdigest()
        return k
    coarse = coarse or compile_key(src, flags)
    mp = os.path.join(PPMAP, coarse + ".txt")
    try:
        k = open(mp).read().strip()
        if len(k) == 32:
            return k
    except OSError:
        pass
    t0 = time.time()
    r = subprocess.run([CL, "/nologo", "/EP", *flags, INCLUDE, winpath(src)], capture_output=True)
    TIMING["pp_s"] += time.time() - t0
    TIMING["pp_runs"] += 1
    if r.returncode or not r.stdout:
        return None
    return pp_key_from_text(src, flags, r.stdout, coarse)


def pp_key_from_text(src, flags, text, coarse):
    h = hashlib.md5(b"pp1\0")
    h.update(text)
    h.update(b"\0" + _norm_flags(flags).encode() + b"\0" + os.path.relpath(src, ROOT).encode())
    if re.search(rb"namespace\s*\{", text):
        h.update(b"\0" + os.path.abspath(src).encode())
    k = h.hexdigest()
    _write_map(coarse, k)
    return k


def _stem(src):
    return os.path.basename(src).rsplit(".", 1)[0]


def _link_coarse(ppobj, obj):
    """Give the coarse name the same object (hard link; a copy where links fail)."""
    if os.path.exists(obj):
        return
    tmp = "%s.%d.%d.lnk" % (obj, os.getpid(), threading.get_ident())
    try:
        os.link(ppobj, tmp)
    except OSError:
        import shutil
        shutil.copyfile(ppobj, tmp)
    try:
        os.replace(tmp, obj)
    except OSError:
        if os.path.exists(tmp):
            os.remove(tmp)


def _write_map(coarse, k):
    os.makedirs(PPMAP, exist_ok=True)
    mp = os.path.join(PPMAP, coarse + ".txt")
    tmp = "%s.%d.%d.part" % (mp, os.getpid(), threading.get_ident())
    open(tmp, "w").write(k)
    os.replace(tmp, mp)


def register(src, flags, coarse, obj):
    """An object found under its coarse name (compiled by this or an older tool) is also what the compiler makes of
    the current preprocessed-text key: file it there once, so that after a header edit that leaves the text alone the
    module finds it. A marker per coarse key (PPMAP) makes the next lookup a single stat."""
    if not _pp_eligible(flags) or PP_MODE == "off" or os.path.exists(os.path.join(PPMAP, coarse + ".txt")):
        return
    ppk = pp_key(src, flags, coarse)
    if not ppk:
        return
    os.makedirs(PPDIR, exist_ok=True)
    _link_coarse(obj, pp_obj_path(src, ppk))          # (links the pp name to the coarse object)
    _write_map(coarse, ppk)


def pp_obj_path(src, ppk):
    return os.path.join(PPDIR, "%s_%s.obj" % (_stem(src), ppk[:16]))


def compile_obj(src, flags):
    """Compile src with flags into work/difftest/obj (cached by source+headers+flags, then by preprocessed text). -> (obj, log)."""
    flags = resolve_include_flags(flags)
    os.makedirs(OBJDIR, exist_ok=True)
    t0 = time.time()
    coarse = compile_key(src, flags)
    obj = os.path.join(OBJDIR, "%s_%s.obj" % (_stem(src), coarse[:12]))
    TIMING["key_s"] += time.time() - t0
    if os.path.exists(obj):
        TIMING["hits"] += 1
        register(src, flags, coarse, obj)
        return obj, ""
    ppk = pp_key(src, flags, coarse)
    ppobj = None
    if ppk:
        os.makedirs(PPDIR, exist_ok=True)
        ppobj = pp_obj_path(src, ppk)
        if os.path.exists(ppobj):
            TIMING["pp_hits"] += 1
            _link_coarse(ppobj, obj)
            return obj, ""
    TIMING["misses"] += 1
    target = ppobj or obj
    # Unique temp per thread: six sweep workers compile the same module at once, and a shared "<obj>.part.obj"
    # made one of them fail with FileNotFoundError on the rename (mesh_start, slot_api, mission_ctx rows).
    tmp = "%s.%d.%d.part.obj" % (target, os.getpid(), threading.get_ident())
    t0 = time.time()
    r = subprocess.run([CL, "/nologo", "/c", *flags, INCLUDE, "/Fo" + winpath(tmp), winpath(src)],
                       capture_output=True, text=True)
    TIMING["cl_s"] += time.time() - t0
    TIMING["cl_runs"] += 1
    if r.returncode or not os.path.exists(tmp):
        if os.path.exists(target):
            if ppobj:
                _link_coarse(ppobj, obj)
            return obj if ppobj else target, r.stdout[-500:]      # another worker finished the identical object first
        return None, (r.stdout + r.stderr)[-3000:]
    try:
        os.replace(tmp, target)
    except OSError:
        if not os.path.exists(target):
            raise
    if ppobj:
        _link_coarse(ppobj, obj)
    return obj, r.stdout[-500:]


# Sources per cl call in ensure_objs. Default 1: measured under wine (cl.exe started directly, no slot queue, same sources)
# a call with 3 sources took 87.6 s against 9.1 + 8.7 + 31.0 s for three calls, so batching only removes parallelism; and
# rcl.py (the laptop) refuses the /Fo<dir>\ form, so a batch is forced onto the Mac. MB_CL_BATCH=N turns it on.
CL_BATCH = int(os.environ.get("MB_CL_BATCH") or 1)


def ensure_objs(items, jobs=None, batch=None):
    """Warm the object cache for many (src, flags) at once, so the per-module compile_obj calls that follow are hits.
    Same keys and the same objects as compile_obj; the difference is orchestration: the coarse keys are hashed up
    front, the misses are preprocessed (cl /EP) concurrently, the ones whose preprocessed text is unseen are compiled
    in batches of `batch` sources per cl call (one wine process start per batch; the number of calls never drops
    below `jobs` so the compile slots stay busy). A source the batch did not produce is left to compile_obj, which
    reports the compiler's diagnostics. -> number of objects compiled."""
    from concurrent.futures import ThreadPoolExecutor
    jobs = jobs or os.cpu_count() or 4
    batch = batch or CL_BATCH
    os.makedirs(OBJDIR, exist_ok=True)
    os.makedirs(PPDIR, exist_ok=True)

    def coarse_of(item):
        src, flags = item
        flags = resolve_include_flags(flags)
        c = compile_key(src, flags)
        o = os.path.join(OBJDIR, "%s_%s.obj" % (_stem(src), c[:12]))
        if os.path.exists(o):
            register(src, flags, c, o)
        return src, flags, c, o

    with ThreadPoolExecutor(jobs) as ex:
        keyed = list(ex.map(coarse_of, items))
        miss = [k for k in keyed if not os.path.exists(k[3])]
        TIMING["hits"] += len(keyed) - len(miss)
        pps = list(ex.map(lambda k: pp_key(k[0], k[1], k[2]), miss))
    todo = {}                                   # object to build -> (src, flags, coarse obj or None)
    for (src, flags, _c, obj), ppk in zip(miss, pps):
        if ppk:
            target = pp_obj_path(src, ppk)
            if os.path.exists(target):
                TIMING["pp_hits"] += 1
                _link_coarse(target, obj)
                continue
            todo.setdefault(target, (src, flags, obj))
        else:
            todo.setdefault(obj, (src, flags, None))
    if not todo:
        return 0
    by_flags = {}
    for target, (src, flags, coarse_obj) in todo.items():
        by_flags.setdefault(tuple(flags), []).append((target, src, coarse_obj))
    calls = []
    for flags, group in by_flags.items():
        size = max(1, min(batch, -(-len(group) // jobs)))
        cur, stems = [], set()
        for ent in group:
            if len(cur) >= size or _stem(ent[1]) in stems:
                calls.append((flags, cur))
                cur, stems = [], set()
            cur.append(ent)
            stems.add(_stem(ent[1]))
        if cur:
            calls.append((flags, cur))

    def run_call(call):
        flags, ents = call
        d = os.path.join(PPDIR, ".batch.%d.%d" % (os.getpid(), threading.get_ident()))
        os.makedirs(d, exist_ok=True)
        t0 = time.time()
        subprocess.run([CL, "/nologo", "/c", *flags, INCLUDE, "/Fo" + winpath(d) + "\\", *[winpath(e[1]) for e in ents]],
                       capture_output=True, text=True)
        TIMING["cl_s"] += time.time() - t0
        TIMING["cl_runs"] += 1
        done = 0
        for target, src, coarse_obj in ents:
            got = os.path.join(d, _stem(src) + ".obj")
            if os.path.exists(got):
                os.replace(got, target)
                if coarse_obj and target != coarse_obj:
                    _link_coarse(target, coarse_obj)
                TIMING["misses"] += 1
                done += 1
        for f in os.listdir(d):
            os.remove(os.path.join(d, f))
        os.rmdir(d)
        return done

    with ThreadPoolExecutor(jobs) as ex:
        return sum(ex.map(run_call, calls))


# ---------------------------------------------------------------- symbol lookup

def demangle_qual(name):
    """Qualified name of an MSVC-mangled symbol ('?f@B@A@@...' -> 'A::B::f'); extern "C" '_f' -> 'f'.
    Returns None for names it does not understand (templates are kept raw)."""
    if name.startswith("?"):
        body = name[1:]
        special = None
        if body.startswith("?0"):
            special, body = "ctor", body[2:]
        elif body.startswith("?1"):
            special, body = "dtor", body[2:]
        elif body.startswith("?"):
            special, body = "op" + body[1], body[2:]
        if body.startswith("?$"):
            return None
        end = body.find("@@")
        if end < 0:
            return None
        parts = body[:end].split("@")
        if any(not p or p[0].isdigit() or p.startswith("?") for p in parts):
            return None
        if special:
            cls = parts[0]
            q = parts[::-1]
            if special == "ctor":
                return "::".join(q + [cls])
            if special == "dtor":
                return "::".join(q + ["~" + cls])
            return "::".join(q + ["operator" + special[2:]])
        return "::".join(parts[::-1])
    if name.startswith("@") and "@" in name[1:]:
        return name[1:name.index("@", 1)]
    if name.startswith("_"):
        return name[1:].split("@")[0]
    return name


def _def_name_after(lines, i):
    """Name of the function the marker on line i labels: the first definition ('(' ... '{' before any ';'), or an
    explicit template instantiation ('template void V<int>::f(...);'), which IS a definition. A file-local
    'static inline' / 'static __forceinline' helper defined between the marker and the free function it labels is
    skipped when such a definition follows before the next '// ... @ 0x' marker (and before any class body)."""
    buf = ""
    first = None
    depth = 0
    tpl = False

    def name_of(head):
        m = re.search(r"((?:[A-Za-z_]\w*\s*::\s*)*~?(?:operator\s*\S+|[A-Za-z_]\w*))\s*$", head)
        return re.sub(r"\s+", "", m.group(1)) if m else None

    for j in range(i + 1, min(i + 80, len(lines))):
        if first is not None and re.search(r"//.*@\s*0x[0-9a-fA-F]{5,8}\b", lines[j]):
            break
        raw = lines[j].split("//")[0]
        if depth:                       # inside the skipped helper's body
            depth += raw.count("{") - raw.count("}")
            continue
        l = raw.strip()
        if l.startswith("template"):
            # the instantiation/specialisation on this line is the definition the marker labels; without this the
            # scan ran on to the next ordinary definition (0x445ea0, the vector<E56> _Insert_n COMDAT, was bound to
            # ir_max_amount twelve lines below, so the row compared two unrelated functions)
            tpl = True
            l = re.sub(r"^template\s*(<[^<>]*>)?\s*", "", l).strip()
        if not l or l.startswith(("#", "/*", "*")):
            continue
        # `__declspec(noinline) char __fastcall check_register(...)`: the attribute's parenthesis is not the
        # parameter list (every validator of the interpreter was "our symbol not found: __declspec")
        l = re.sub(r"__declspec\s*\([^)]*\)", " ", l)
        buf += " " + l
        if "{" in buf or ";" in buf:
            semi, brace = buf.find(";"), buf.find("{")
            if brace >= 0 and (semi < 0 or brace < semi) and "(" in buf[:brace]:
                head = buf[:buf.index("(")]
                nm = name_of(head)
                if nm:
                    helper = re.search(r"\bstatic\b", head) and re.search(r"\b(?:inline|__inline|__forceinline)\b", head)
                    if not helper:
                        return nm if first is None or "::" not in nm else first
                    if first is None:
                        first = nm              # a file-local inline helper: look further
                    depth = buf.count("{") - buf.count("}")
                    buf, tpl = "", False
                    continue
            if tpl and semi >= 0 and "(" in buf and brace < 0:
                nm = name_of(buf[:buf.index("(")])
                if nm:
                    return nm
            if brace >= 0 and (semi < 0 or brace < semi) and "(" not in buf[:brace]:
                if first is not None:
                    return first   # past a helper into a class body: the helper was the target
                buf = ""   # class/namespace opening: keep looking
                continue
            buf, tpl = "", False
    return first


def marker_names(src, va):
    """Candidate qualified names for the function at va: the definition right after a
    '// ... @ 0x<va>' marker, and names written next to the address in comments."""
    lines = open(src, encoding="utf-8", errors="replace").read().split("\n")
    marker = re.compile(r"//.*@\s*0x0*%x\b" % va, re.I)
    named = re.compile(r"//\s*(?:@\s*)?(?:0x)?0*%x\b[\s:,-]*((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)" % va, re.I)
    out = []
    for i, line in enumerate(lines):
        if marker.search(line):
            nm = _def_name_after(lines, i)
            if nm and nm not in out:
                out.append(nm)
        m = named.search(line)
        if m and m.group(1) not in out and m.group(1) not in ("complete", "flags", "the"):
            out.append(m.group(1))
    return out


def slice_symbol_name(sid, va):
    path = os.path.join(ROOT, "symbols", "slices", sid + ".txt")
    if os.path.exists(path):
        for line in open(path, encoding="utf-8", errors="replace"):
            p = line.split("#", 1)[0].strip().split(None, 1)
            if len(p) >= 2:
                try:
                    if int(p[0], 16) == va:
                        return p[1].strip()
                except ValueError:
                    pass
    return None


def base_ident(mangled):
    """Leading identifier of a symbol: '?f@C@@..'->'f', '??$f@..'->'f', '??0C@@'->'C', '_f'->'f'."""
    m = mangled
    if m.startswith("??$"):
        m = m[3:]
    elif m.startswith(("??0", "??1")):
        m = m[3:]
    elif m.startswith("?"):
        m = m[1:]
    elif m.startswith("@"):
        m = m[1:]
    elif m.startswith("_"):
        m = m[1:]
    return m.split("@", 1)[0]


def _clean_name(nm):
    nm = re.sub(r"<[^<>]*(<[^<>]*>[^<>]*)*>", "", nm)
    nm = nm.replace("`anonymous namespace'::", "").replace("anon::", "")
    return [p.lstrip("_") or p for p in nm.split("::") if p]


def _name_matches(mangled, nm):
    parts = _clean_name(nm)
    if not parts:
        return False
    last = parts[-1]
    b = base_ident(mangled).lstrip("_")
    if last.startswith("~"):
        if not mangled.startswith("??1") or b != last[1:]:
            return False
    elif len(parts) >= 2 and last == parts[-2]:      # constructor
        if not mangled.startswith("??0") or b != last:
            return False
    elif b != last or mangled.startswith(("??0", "??1")):
        return False
    if len(parts) >= 2 and not last.startswith("~") and last != parts[-2]:
        cls = parts[-2]
        if ("@" + cls + "@") not in mangled and ("?$" + cls + "@") not in mangled:
            return False
    return True


def find_symbol(coff, sid, va, src, orig_size=None):
    """-> (symbol name, how) | (list of candidates, 'ambiguous:...') | (None, reason).
    orig_size (bytes of the original function) rejects candidates of implausible size."""
    funcs = {n: f for n, f in coff.functions().items()
             if not n.startswith(("__ehhandler", "__unwindfunclet", "__catch", "__tryend", "$"))}
    for _src, sym, mva, _fl in manifest_rows(sid):
        if mva == va:
            if sym in funcs:
                return sym, "manifest"
            c = [n for n in funcs if sym in n]
            if len(c) == 1:
                return c[0], "manifest"
    names = marker_names(src, va)
    sn = slice_symbol_name(sid, va)
    if sn and sn in funcs:
        # symbols/slices/<sid>.txt holds the exact decorated name (rewrite.py writes it from the compiled module TU): an
        # overload set (GetProperty(key) const / GetProperty(key, Variant const **) const / ...) must not fall back to the
        # loose name match below
        return sn, "symbols/slices/%s.txt" % sid
    if sn:
        names.insert(0, sn)

    def plausible(f):
        if not orig_size:
            return True
        _sec, a, b = coff.func_extent(funcs[f])
        return orig_size / 5 <= (b - a) <= orig_size * 5

    # Size only breaks ties between same-name candidates: a unique name match is accepted at any size
    # (a thin wrapper around a non-inlined shared helper is complete source, just much smaller).
    allc = []
    for nm in names:
        named = [f for f in funcs if _name_matches(f, nm)]
        if len(named) == 1:
            return named[0], "name %s" % nm
        cands = [f for f in named if plausible(f)]
        if len(cands) == 1:
            return cands[0], "name %s" % nm
        for c in cands:
            if c not in allc:
                allc.append(c)
    if allc:
        return allc, "ambiguous: %s" % names
    return None, "no marker/name for 0x%08x matched an object symbol (names tried: %s)" % (va, names or "-")
