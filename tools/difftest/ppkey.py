"""ppkey.py: a cache key for "what the compiler sees" that needs no compiler run.

slice.compile_key hashes every header a TU can reach plus every header in its directory and in match/include, so one edit
to a shared header (rgl_math.h reaches 154 of 179 modules) misses for nearly every module even when the edit cannot change
the compiler's input (a comment, whitespace, a header the TU does not include after all). The exact answer is the
preprocessed text (`cl /EP`), but under wine that is a process start of the same order as the compile itself (measured
without the slot queue: /EP 17-32 s against 9-31 s for /c on the same sources), so for a miss it buys nothing.

This module computes a sound over-approximation of the preprocessed text in Python:

  * the include closure of the source: every `#include "x"` / `<x>` the files contain, whether or not an #if skips it,
    resolved the way cl does (the including file's directory, the source's directory, the /I flags in order, slice's
    match/include, then cl.sh's INCLUDE list); where several candidates could match, ALL of them are taken;
  * each file's content with comments removed and blank lines / indentation / trailing blanks dropped. Line structure is
    kept for a repo file that can let a line number reach the output: one that mentions __LINE__, __COUNTER__, __FILE__,
    __DATE__, __TIME__ or __TIMESTAMP__, or a macro whose body (transitively, over the #defines of the closure) does -- assert()
    and the debug-heap `new` macros do, so a file calling them still changes key when a line is added above the call.
    Files under work/ (the toolchain headers) never change, their structure is irrelevant;
  * a computed include (`#include MACRO`) makes the key unavailable (None): the caller then falls back to the coarse key
    alone. An include that resolves nowhere is tolerated (it sits in an #if-skipped region, or the compile fails and nothing
    is cached) and its name enters the key.

Anything that changes the preprocessed text changes some file of the closure, and the normalisation drops only text the
preprocessor itself drops, so equal keys imply equal preprocessed text. (The reverse need not hold: an edit inside an
#if-skipped region changes the key but not the text; that costs a recompile, never a stale object.)

Per-file results are cached in a table keyed by path, size and mtime (hdr_index.pkl), so the toolchain headers are lexed once.
"""
import atexit, hashlib, os, pickle, re, threading

_INCLUDE = re.compile(rb'^[ \t]*#[ \t]*(?:include|import)[ \t]*(.*?)[ \t]*$', re.M)
_LITERAL = re.compile(rb'^(?:"([^"\n]+)"|<([^>\n]+)>)')
_DEFINE = re.compile(rb'^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)(.*)$', re.M)
_IDENT = re.compile(rb"[A-Za-z_]\w*")
SEEDS = frozenset(["__LINE__", "__COUNTER__", "__FILE__", "__DATE__", "__TIME__", "__TIMESTAMP__"])
_WORK = os.sep + "work" + os.sep

_lock = threading.Lock()
_index = {}          # path -> (size, mtime_ns, norm digest, lined digest, [(kind, name)], computable, [(macro, body idents)], idents or None)
_loaded = {"path": None}
_dirty = [False]


def strip_comments(b):
    """Source bytes with // and /* */ comments removed (a block comment becomes one space and keeps its newlines so a
    line-sensitive key still sees the same line structure); string and character literals are left alone."""
    out, i, n = [], 0, len(b)
    start = 0
    while i < n:
        c = b[i]
        if c == 0x22 or c == 0x27:                       # " or '
            j = i + 1
            while j < n and b[j] != c and b[j] != 0x0A:
                j += 2 if b[j] == 0x5C else 1
            i = j + 1
        elif c == 0x2F and i + 1 < n and b[i + 1] == 0x2F:
            out.append(b[start:i])
            j = i
            while j < n and b[j] != 0x0A:
                if b[j] == 0x5C and j + 1 < n and b[j + 1] in (0x0A, 0x0D):      # a // comment continues over a backslash-newline
                    j += 2 if b[j + 1] == 0x0A else 3
                    continue
                j += 1
            start = i = j
        elif c == 0x2F and i + 1 < n and b[i + 1] == 0x2A:
            out.append(b[start:i])
            j = b.find(b"*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(b" " + b"\n" * b.count(b"\n", i, j))
            start = i = j
        else:
            i += 1
    out.append(b[start:])
    return b"".join(out)


def normalise(text, keep_lines):
    """text: comment-stripped bytes."""
    lines = text.replace(b"\r\n", b"\n").replace(b"\r", b"\n").split(b"\n")
    out, cont = [], False
    for ln in lines:
        s = ln.rstrip()
        if not cont:
            s = s.lstrip()
            if not s and not keep_lines:
                continue
        out.append(s)
        cont = s.endswith(b"\\")
    return b"\n".join(out)


def _index_path(idxdir):
    return os.path.join(idxdir, "hdr_index.pkl")


def load_index(idxdir):
    with _lock:
        if _loaded["path"] == idxdir:
            return
        _loaded["path"] = idxdir
        atexit.register(flush)
        try:
            _index.update(pickle.load(open(_index_path(idxdir), "rb")))
        except Exception:
            pass


def save_index(idxdir):
    """Merge the in-memory table into the on-disk one (atomic replace; a lost race only costs a re-lex)."""
    with _lock:
        if not _dirty[0]:
            return
        os.makedirs(idxdir, exist_ok=True)
        try:
            disk = pickle.load(open(_index_path(idxdir), "rb"))
        except Exception:
            disk = {}
        disk.update(_index)
        part = "%s.%d.%d.part" % (_index_path(idxdir), os.getpid(), threading.get_ident())
        pickle.dump(disk, open(part, "wb"), protocol=4)
        os.replace(part, _index_path(idxdir))
        _dirty[0] = False


def file_info(path):
    st = os.stat(path)
    ent = _index.get(path)
    if ent and ent[0] == st.st_size and ent[1] == st.st_mtime_ns:
        return ent[2:]
    raw = open(path, "rb").read()
    text = strip_comments(raw)
    incs, computable = [], True
    flat = text.replace(b"\r", b"")
    for m in _INCLUDE.finditer(flat):
        lit = _LITERAL.match(m.group(1))
        if lit:
            incs.append(("q", lit.group(1).decode("latin-1")) if lit.group(1) else ("a", lit.group(2).decode("latin-1")))
        else:
            computable = False
    # #define NAME body (continuation lines joined): which macros mention which identifiers
    defs = []
    joined = re.sub(rb"\\\n", b" ", flat)
    for m in _DEFINE.finditer(joined):
        body = {w.decode() for w in _IDENT.findall(m.group(2))}
        if body:
            defs.append((m.group(1).decode(), tuple(sorted(body))))
    idents = None if _WORK in path else frozenset(w.decode() for w in _IDENT.findall(_INCLUDE.sub(b"", flat)))
    info = (hashlib.md5(normalise(text, False)).hexdigest(), hashlib.md5(normalise(text, True)).hexdigest(), incs, computable,
            defs, idents)
    with _lock:
        _index[path] = (st.st_size, st.st_mtime_ns) + info
        _dirty[0] = True
    return info


def closure_key(src, include_dirs, root, extra=b""):
    """md5 over the normalised files of src's include closure, or None when it cannot be determined soundly.
    include_dirs: the search path in cl's order (the /I flags, then the INCLUDE environment)."""
    src = os.path.abspath(src)
    seen, order, todo, missing = set(), [], [src], []
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.add(f)
        try:
            info = file_info(f)
        except OSError:
            return None
        if not info[3]:
            return None
        order.append((f, info))
        for kind, name in info[2]:
            dirs = ([os.path.dirname(f), os.path.dirname(src)] if kind == "q" else []) + include_dirs
            found = False
            for d in dirs:
                c = os.path.normpath(os.path.join(d, name.replace("\\", "/")))
                if os.path.isfile(c):
                    found = True
                    if c not in seen:
                        todo.append(c)
            if not found:           # in an #if-skipped region (a live one would fail the compile): in the key, so a later-created file counts
                missing.append(os.path.relpath(f, root) + "|" + name)
    # macros that (transitively) let a line number reach the output
    users = {}
    for _f, info in order:
        for name, body in info[4]:
            for w in body:
                users.setdefault(w, []).append(name)
    tainted, work = set(SEEDS), list(SEEDS)
    while work:
        for name in users.get(work.pop(), ()):
            if name not in tainted:
                tainted.add(name)
                work.append(name)
    h = hashlib.md5(b"ppkey2\0" + extra + b"\0" + "\0".join(sorted(missing)).encode() + b"\0")
    # in path order (repo-relative, so worktrees agree): two headers that swap contents must not collide
    for f, info in sorted(order, key=lambda e: os.path.relpath(e[0], root)):
        lined = info[5] is not None and not tainted.isdisjoint(info[5])
        h.update(os.path.relpath(f, root).encode() + b"\0" + (info[1] if lined else info[0]).encode() +
                 (b"S" if f == src else b"H") + (b"L" if lined else b"N"))
    return h.hexdigest()


def flush():
    if _loaded["path"]:
        save_index(_loaded["path"])
