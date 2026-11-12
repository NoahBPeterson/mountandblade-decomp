#!/usr/bin/env python3
"""Client of the Windows laptop's compile server (tools/remote/compile_server.py): rcl.py CL-ARGS...

tools/matching/cl.sh runs this when every local compile slot is busy. It takes the same arguments cl.sh got (wine "Z:\\..."
paths), ships the source and every header it reaches that the server does not already hold (content-addressed), has the
laptop's native cl.exe compile it, and writes the outputs (/Fo, /Fa) to the paths asked for. Prints cl's output (paths
mapped back to the Mac's) on stdout, like cl.sh. Exit status:
  0 or 2   cl.exe's own (the compile happened on the laptop)
  75       the laptop is busy or unreachable right now: wait for a local slot, ask again later
  76       this command cannot / should not run remotely (not configured, unsupported flags, a header was missing):
           compile locally
Traced compiles (../vs2005-cpp tools/c2trace.py): MB_RC_TC names a compiler directory installed on the laptop
(D:\\mb\\rc\\tc\\<name>: cl.exe with the patched c2.dll and c2trace.dll), MB_RC_TRACE_CFG is the hook list to ship and
MB_RC_TRACE_LOG where the trace log is written here. The log and the other outputs are then written whatever cl's exit
status was (a trace can end the compile on purpose).
Pure standard library. Off unless ~/.config/mb-compile-token and ~/.config/mb-compile-server (host:port) exist, or
MB_RC_TOKEN / MB_RC_ENDPOINT are set; MB_RC=0 turns it off. Nothing is read from or written to the laptop outside D:\\mb\\rc.
"""
import hashlib, hmac, json, os, posixpath, re, socket, struct, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LOCAL, BUSY = 76, 75
INC = re.compile(rb'^[ \t]*#[ \t]*(?:include|import)[ \t]*([<"])([^>"\r\n]+)[>"]', re.M)
ANON = re.compile(rb"namespace\s*\{")
TOOLDIRS = tuple(os.path.join(ROOT, "work", d) + os.sep for d in ("toolchain", "ext"))
SYSDIRS = [os.path.join(ROOT, "src", "include"), os.path.join(ROOT, "match", "include")]   # cl.sh's INCLUDE, repo part
CONNECT_TIMEOUT = 1.0


def cfg(name, envname, default=None):
    if os.environ.get(envname):
        return os.environ[envname]
    try:
        return open(os.path.expanduser("~/.config/" + name)).read().strip()
    except OSError:
        return default


def send(sock, obj):
    b = json.dumps(obj).encode()
    sock.sendall(struct.pack(">I", len(b)) + b)


def recv_exact(sock, n):
    buf = bytearray()
    while len(buf) < n:
        c = sock.recv(min(1 << 20, n - len(buf)))
        if not c:
            raise ConnectionError("closed")
        buf += c
    return bytes(buf)


def recv(sock):
    return json.loads(recv_exact(sock, struct.unpack(">I", recv_exact(sock, 4))[0]))


def send_blob(sock, data):
    sock.sendall(struct.pack(">Q", len(data)) + data)


def recv_blob(sock):
    return recv_exact(sock, struct.unpack(">Q", recv_exact(sock, 8))[0])


def mac(token, nonce, who):
    return hmac.new(token, (nonce + who).encode(), hashlib.sha256).hexdigest()


def unwine(p):
    """'Z:\\a\\b' -> '/a/b'; a relative path resolves against the cwd, as wine would (None for any other drive or root)."""
    if p[:2].upper() == "Z:":
        return os.path.normpath(p[2:].replace("\\", "/"))
    if p and p[0] not in "/\\" and p[1:2] != ":":
        return os.path.abspath(p.replace("\\", "/"))
    return None


def place(path):
    """Unix path -> (placeholder path the server expands, stage-relative key or None for toolchain files)."""
    path = os.path.normpath(path)
    if path.startswith(TOOLDIRS) or path in [t.rstrip(os.sep) for t in TOOLDIRS]:
        return "{R}/" + os.path.relpath(path, ROOT).replace(os.sep, "/"), None
    if path == ROOT or path.startswith(ROOT + os.sep):
        key = "r/" + os.path.relpath(path, ROOT).replace(os.sep, "/")
    else:
        key = "x/" + path.lstrip("/")
    return "{S}/" + key, key


class Unsupported(Exception):
    pass


def parse(argv):
    """-> (args for the server, sources, include dirs, outs {id: (server rel path, local dest)}, forced includes)."""
    args, srcs, idirs, outs, fis = [], [], [], {}, []
    for a in argv:
        if a.startswith("@"):
            raise Unsupported
        if a[:2] in ("/I", "-I") and len(a) > 2:
            p = unwine(a[2:])
            if p is None:
                raise Unsupported
            idirs.append(p)
            args.append("/I" + place(p)[0])
        elif a[:3] in ("/Fo", "/Fa", "/fo"):
            dest = unwine(a[3:])
            if dest is None or a[-1] in "\\/" or os.path.isdir(dest):
                raise Unsupported
            oid = "o%d" % len(outs)
            rel = "%s/%s" % (oid, os.path.basename(dest))
            outs[oid] = (rel, dest)
            args.append(a[:3] + "{S}/out/" + rel)
        elif a[:3] == "/FI" and len(a) > 3:
            fis.append(a[3:])
            args.append(a)
        elif a[:2] in ("/F", "-F") and a[2:3] in ("d", "e", "p", "m", "r", "x", "R", "D") and a[:3] != "/FD":
            raise Unsupported                       # /Fd /Fe /Fp /Fm /FR: other outputs
        elif a[:2] in ("/T", "-T") and a[2:3] in ("c", "p") and len(a) > 3:
            p = unwine(a[3:])
            if p is None:
                raise Unsupported
            srcs.append(p)
            args.append(a[:3] + place(p)[0])
        elif a[:1] in "/-":
            if a[:2] in ("/E", "/P", "/Y") or a in ("/Zi", "/ZI"):
                raise Unsupported                   # preprocess-to-file / PCH / PDB: the outputs are not shipped back
            args.append(a)
        else:
            p = unwine(a)
            if p is None:
                raise Unsupported
            srcs.append(p)
            args.append(place(p)[0])
    if not srcs:
        raise Unsupported
    if not outs and "/Zs" not in argv:
        raise Unsupported                           # /c without /Fo drops the object in the server's cwd
    if any(a.startswith("/FA") for a in argv) and not any(a.startswith("/Fa") for a in argv) and "/Zs" not in argv:
        raise Unsupported
    return args, srcs, idirs, outs, fis


def scan(srcs, idirs, fis):
    """Every file under the repo (or elsewhere on the Mac, but not the toolchain) that compiling srcs can include:
    {stage key: path}. Follows quoted includes next to the includer (then its includers), then /I dirs, then cl.sh's
    repo INCLUDE dirs; a header found nowhere there is the toolchain's (the server's INCLUDE has it)."""
    found, seen = {}, set()
    todo = [(s, []) for s in srcs]
    for fi in fis:
        for d in idirs + SYSDIRS:
            c = os.path.join(d, fi)
            if os.path.isfile(c):
                todo.append((c, []))
                break
    while todo:
        f, chain = todo.pop()
        f = os.path.normpath(f)
        if f in seen:
            continue
        seen.add(f)
        key = place(f)[1]
        if key is None:
            continue
        found[key] = f
        try:
            text = open(f, "rb").read()
        except OSError:
            continue
        if ANON.search(text):
            raise Unsupported       # mangled names of an anonymous namespace hash the source path: only the Mac's path is right
        here = chain + [os.path.dirname(f)]
        for kind, name in INC.findall(text):
            name = name.decode("latin-1").strip().replace("\\", "/")
            dirs = (list(reversed(here)) if kind == b'"' else []) + idirs + SYSDIRS
            for d in dirs:
                c = os.path.normpath(os.path.join(d, name))
                if os.path.isfile(c):
                    todo.append((c, here))
                    break
    return found


def sha1_of(p, memo={}):
    if p not in memo:
        memo[p] = hashlib.sha1(open(p, "rb").read()).hexdigest()
    return memo[p]


def unmap_log(log, stage, repo):
    def rep(text, native, local):
        return re.sub(re.escape(native), lambda m: local, text, flags=re.I)
    w = lambda p: "Z:" + p.replace("/", "\\")
    sep = "\\" if "\\" in stage else "/"
    log = rep(log, stage + sep + "r", w(ROOT))
    log = rep(log, stage + sep + "x", "Z:")
    log = rep(log, repo + sep + "work", w(ROOT + "/work"))
    return log


def main(argv):
    if os.environ.get("MB_RC") == "0":
        return LOCAL
    token_path = os.environ.get("MB_RC_TOKEN") or os.path.expanduser("~/.config/mb-compile-token")
    ep = cfg("mb-compile-server", "MB_RC_ENDPOINT")
    if not ep or not os.path.isfile(token_path):
        return LOCAL
    down = os.path.join(ROOT, "work", "locks", "rcl.down")
    traced = bool(os.environ.get("MB_RC_TC"))          # a traced compile waits for the laptop: its caller retries on 75,
    try:                                                # so it neither trusts nor writes the 30 s "down" marker
        if not traced and time.time() - os.path.getmtime(down) < 30:
            return BUSY
    except OSError:
        pass
    try:
        args, srcs, idirs, outs, fis = parse(argv)
    except Unsupported:
        return LOCAL
    tc, tcfg, tlog = (os.environ.get(k) for k in ("MB_RC_TC", "MB_RC_TRACE_CFG", "MB_RC_TRACE_LOG"))
    extra = {}
    if tc:
        if not (tcfg and tlog and os.path.isfile(tcfg)):
            return LOCAL
        outs["oT"] = ("oT/c2trace.log", tlog)
        cfg_ph, cfg_key = place(os.path.abspath(tcfg))
        extra = {"tc": tc, "env": {"C2TRACE_CFG": cfg_ph, "C2TRACE_LOG": "{S}/out/oT/c2trace.log"}}
    token = open(token_path, "rb").read().strip()
    host, port = ep.rsplit(":", 1)
    try:
        sock = socket.create_connection((host, int(port)), timeout=5.0 if traced else CONNECT_TIMEOUT)
        sock.settimeout(10)
        nonce = json.loads(recv_exact(sock, struct.unpack(">I", recv_exact(sock, 4))[0]))["nonce"]
        send(sock, {"mac": mac(token, nonce, "C")})
        if not hmac.compare_digest(recv(sock).get("mac", ""), mac(token, nonce, "S")):
            raise ConnectionError("server failed authentication")
        send(sock, {"op": "reserve"})
        ok = recv(sock)["ok"]
    except (OSError, ConnectionError, ValueError, KeyError):
        if traced:
            return BUSY
        try:
            os.makedirs(os.path.dirname(down), exist_ok=True)
            open(down, "w").close()
            os.utime(down)
        except OSError:
            pass
        return BUSY
    if not ok:
        return BUSY
    try:
        try:
            files = scan(srcs, idirs, fis)
        except Unsupported:
            return LOCAL
        if tc:
            files[cfg_key] = os.path.abspath(tcfg)
        shas = {k: sha1_of(p) for k, p in files.items()}
        send(sock, dict({"op": "compile", "args": args, "files": shas, "outs": {k: v[0] for k, v in outs.items()}}, **extra))
        sock.settimeout(300)                            # a stalled job falls back to a local slot (cl.sh) instead of waiting 20 min
        need = recv(sock)["need"]
        by_sha = {}
        for k, h in shas.items():
            by_sha.setdefault(h, files[k])
        for h in need:
            send_blob(sock, open(by_sha[h], "rb").read())
        res = recv(sock)
        got = {}
        for k in res["outs"]:
            got[k] = recv_blob(sock)
    except (OSError, ConnectionError, ValueError, KeyError):
        return BUSY
    finally:
        sock.close()
    log = unmap_log(res["log"], res["stage"], res["repo"])
    if "C1083" in log or res["log"] == "refused":      # a header we did not ship (or a refused job): the local compiler decides
        if os.environ.get("MB_RC_DEBUG"):
            sys.stderr.write("rcl: falling back to a local compile: %s\n" % log[-600:])
        return LOCAL
    return _finish(res, log, outs, got, always=bool(tc))


def _finish(res, log, outs, got, always=False):
    if always:                                          # a traced compile: keep what came back, even after a failure
        if "oT" not in got:
            return LOCAL
        outs = {k: v for k, v in outs.items() if k in got}
    if res["rc"] == 0 or always:
        for k, (_rel, dest) in outs.items():
            if k not in got:
                return LOCAL
        for k, (_rel, dest) in outs.items():
            tmp = "%s.rcl.%d" % (dest, os.getpid())
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(tmp, "wb") as f:
                f.write(got[k])
            os.replace(tmp, dest)
    sys.stdout.write(log)
    sys.stdout.flush()
    return res["rc"]


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
