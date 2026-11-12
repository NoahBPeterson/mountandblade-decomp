"""Remote compile server: runs the VC7.1 cl.exe for Mac clients (tools/remote/rcl.py, called by tools/matching/cl.sh when
every local compile slot is busy). Runs on the Windows laptop under D:\\mb; see docs/remote_worker.md.

Protocol (TCP; every message is a 4-byte big-endian length plus UTF-8 JSON; a blob is an 8-byte length plus bytes):
  S -> C  {"nonce": hex}
  C -> S  {"mac": hmac_sha256(token, nonce + "C")}              (wrong mac: the connection is dropped, nothing runs)
  S -> C  {"mac": hmac_sha256(token, nonce + "S"), "slots": N, "busy": n}   (the client checks it: no source goes to an impostor)
  C -> S  {"op": "reserve"}  -> {"ok": true} or {"ok": false}   (takes one of the N compile slots for this connection)
  C -> S  {"op": "compile", "args": [...], "files": {"r/src/a.cpp": sha1, ...}, "outs": {"o0": "dir/x.obj"}}
  S -> C  {"need": [sha1, ...]}; C -> S one blob per needed sha1, in order
  S -> C  {"rc": int, "log": str, "stage": native stage dir, "repo": native repo dir, "outs": {"o0": size}} then the blobs
Inputs are content-addressed in D:\\mb\\rc\\store/<sha1[:2]>/<sha1>; each job gets its own stage directory holding only the
shipped files, so a header the client forgot to ship is a C1083, never a stale copy from D:\\mb\\repo. Args carry {S}/... (the stage
directory) and {R}/... (the repo checkout: toolchain and DX SDK only) placeholders. The token (a file outside the repo) is
never sent, only HMACs of a fresh nonce. cl.exe is the only program that is run; args starting with @ are refused.

Traced compiles (../vs2005-cpp tools/c2trace.py, via rcl.py's MB_RC_TC): a compile message may name "tc", an alternative
compiler directory D:\\mb\\rc\\tc\\<name> (installed by hand; it holds cl.exe with a patched c2.dll and c2trace.dll), and
"env", whose keys must start with C2TRACE_ and whose values take the same {S}/{R} placeholders (the hook list is a
shipped file, the trace log an out). An unknown tc or any other env key is refused.
"""
import argparse, hashlib, hmac, json, os, re, secrets, shutil, socket, socketserver, struct, subprocess, sys, threading, time

IS_WIN = sys.platform == "win32"
PH = re.compile(r"\{([SR])\}([^\s;]*)")


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
    n = struct.unpack(">I", recv_exact(sock, 4))[0]
    if n > 64 << 20:
        raise ConnectionError("message too large")
    return json.loads(recv_exact(sock, n))


def send_blob(sock, data):
    sock.sendall(struct.pack(">Q", len(data)) + data)


def recv_blob(sock):
    n = struct.unpack(">Q", recv_exact(sock, 8))[0]
    if n > 512 << 20:
        raise ConnectionError("blob too large")
    return recv_exact(sock, n)


def mac(token, nonce, who):
    return hmac.new(token, (nonce + who).encode(), hashlib.sha256).hexdigest()


class Server:
    def __init__(self, a):
        self.a = a
        self.token = open(a.token, "rb").read().strip()
        self.sem = threading.BoundedSemaphore(a.slots)
        self.busy = 0
        self.lock = threading.Lock()
        self.store = os.path.join(a.root, "store")
        self.jobs = os.path.join(a.root, "j")
        os.makedirs(self.store, exist_ok=True)
        shutil.rmtree(self.jobs, ignore_errors=True)
        os.makedirs(self.jobs, exist_ok=True)
        self.prune_store()

    def prune_store(self, days=14):
        cut = time.time() - days * 86400
        for d, _s, fs in os.walk(self.store):
            for f in fs:
                p = os.path.join(d, f)
                try:
                    if os.path.getmtime(p) < cut:
                        os.remove(p)
                except OSError:
                    pass

    # paths as the compiler sees them: native on Windows, wine's Z: view in the Mac test mode (--wine)
    def cpath(self, p):
        return "Z:" + p.replace("/", "\\") if self.a.wine else p

    def blob_path(self, sha):
        return os.path.join(self.store, sha[:2], sha)

    def expand(self, s, stage):
        def sub(m):
            base = stage if m.group(1) == "S" else self.a.repo
            rest = m.group(2).replace("\\", "/")
            return self.cpath(os.path.normpath(os.path.join(base, *rest.split("/"))) if rest else base)
        return PH.sub(sub, s)

    def env(self, stage):
        s, r = (lambda *p: self.cpath(os.path.join(stage, "r", *p))), (lambda *p: self.cpath(os.path.join(self.a.repo, *p)))
        tc = ("work", "toolchain")
        inc = [s("src", "include"), s("match", "include"), r(*tc, "vc71tk", "include"), r(*tc, "sal"),
               r(*tc, "vc9sp1", "WinSDK", "Include"), r("work", "ext", "dxsdk_jun10", "DXSDK", "Include")]
        lib = [r(*tc, "vc71tk", "lib"), r(*tc, "vc9sp1", "WinSDK", "Lib")]
        sep = ";"
        env = dict(os.environ, INCLUDE=sep.join(inc), LIB=sep.join(lib))
        if self.a.wine:
            env.update(WINEPREFIX=os.path.join(self.a.repo, "work", "toolchain", "wineprefix-mb"), WINEDEBUG="-all",
                       WINEPATH=self.cpath(os.path.join(self.a.repo, *tc, "vc71tk", "bin")))
        else:
            env["PATH"] = os.path.join(self.a.repo, *tc, "vc71tk", "bin") + os.pathsep + env.get("PATH", "")
        return env

    def tc_dir(self, tc):
        """The compiler directory for a job: the repo's vc71tk, or D:\\mb\\rc\\tc\\<tc> (None if it is not installed)."""
        if not tc:
            return os.path.join(self.a.repo, "work", "toolchain", "vc71tk", "bin")
        d = os.path.join(self.a.root, "tc", tc)
        return d if re.fullmatch(r"[A-Za-z0-9._-]+", tc) and os.path.isfile(os.path.join(d, "cl.exe")) else None

    def cl_cmd(self, tcdir=None):
        cl = os.path.join(tcdir or self.tc_dir(None), "cl.exe")
        return ["wine", cl] if self.a.wine else [cl]

    def handle(self, sock):
        sock.settimeout(60)
        nonce = secrets.token_hex(16)
        send(sock, {"nonce": nonce})
        m = recv(sock)
        if not hmac.compare_digest(str(m.get("mac", "")), mac(self.token, nonce, "C")):
            return
        send(sock, {"mac": mac(self.token, nonce, "S"), "slots": self.a.slots, "busy": self.busy})
        held = False
        try:
            while True:
                m = recv(sock)
                if m["op"] == "ping":
                    send(sock, {"slots": self.a.slots, "busy": self.busy})
                elif m["op"] == "reserve":
                    held = held or self.sem.acquire(blocking=False)
                    if held:
                        with self.lock:
                            self.busy += 1
                    send(sock, {"ok": held})
                    if not held:
                        return
                elif m["op"] == "compile":
                    if not held:
                        return
                    sock.settimeout(900)
                    self.compile(sock, m)
                    return
                else:
                    return
        finally:
            if held:
                with self.lock:
                    self.busy -= 1
                self.sem.release()

    def compile(self, sock, m):
        files, args, outs = m["files"], m["args"], m.get("outs", {})
        tcdir, xenv = self.tc_dir(m.get("tc")), m.get("env") or {}
        if tcdir is None or not isinstance(xenv, dict) or any(not str(k).startswith("C2TRACE_") for k in xenv) or \
                any(a.startswith("@") for a in args) or any(".." in k.split("/") or k.startswith("/") or ":" in k for k in files):
            send(sock, {"rc": 2, "log": "refused", "stage": "", "repo": "", "outs": {}})
            return
        need = [h for h in sorted(set(files.values())) if not os.path.exists(self.blob_path(h))]
        send(sock, {"need": need})
        for h in need:
            data = recv_blob(sock)
            if hashlib.sha1(data).hexdigest() != h:
                raise ConnectionError("bad blob")
            p = self.blob_path(h)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            tmp = "%s.%d.%d" % (p, os.getpid(), threading.get_ident())
            with open(tmp, "wb") as f:
                f.write(data)
            os.replace(tmp, p)
        stage = os.path.join(self.jobs, secrets.token_hex(6))
        try:
            for rel, h in files.items():
                dst = os.path.join(stage, *rel.split("/"))
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                src = self.blob_path(h)
                try:
                    os.link(src, dst)
                except OSError:
                    shutil.copyfile(src, dst)
                try:
                    os.utime(src)
                except OSError:
                    pass
            for rel in outs.values():
                os.makedirs(os.path.dirname(os.path.join(stage, "out", *rel.split("/"))), exist_ok=True)
            os.makedirs(os.path.join(stage, "out"), exist_ok=True)
            cmd = self.cl_cmd(tcdir) + [self.expand(a, stage) for a in args]
            env = self.env(stage)
            if m.get("tc"):                                  # the traced cl.exe finds its own DLLs first
                if self.a.wine:
                    env["WINEPATH"] = self.cpath(tcdir)
                else:
                    env["PATH"] = tcdir + os.pathsep + env.get("PATH", "")
            for k, v in xenv.items():
                env[str(k)] = self.expand(str(v), stage)
            kw = {}
            if IS_WIN:
                kw["creationflags"] = 0x00000040 | 0x08000000      # IDLE_PRIORITY_CLASS | CREATE_NO_WINDOW
            r = subprocess.run(cmd, cwd=stage, env=env, capture_output=True, stdin=subprocess.DEVNULL, **kw)
            log = (r.stdout + r.stderr).decode("latin-1")
            got = {}
            for k, rel in outs.items():
                p = os.path.join(stage, "out", *rel.split("/"))
                if os.path.isfile(p):
                    got[k] = open(p, "rb").read()
            send(sock, {"rc": r.returncode, "log": log, "stage": self.cpath(stage), "repo": self.cpath(self.a.repo),
                        "outs": {k: len(v) for k, v in got.items()}})
            for k in got:
                send_blob(sock, got[k])
        finally:
            shutil.rmtree(stage, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=r"D:\mb\rc" if IS_WIN else os.path.expanduser("~/.cache/mb-rc"))
    ap.add_argument("--repo", default=r"D:\mb\repo", help="checkout holding work/toolchain and work/ext (toolchain only)")
    ap.add_argument("--token", default=None)
    ap.add_argument("--bind", default="0.0.0.0", help="address to listen on (the Tailscale IP on the laptop)")
    ap.add_argument("--port", type=int, default=47813)
    ap.add_argument("--slots", type=int, default=36)
    ap.add_argument("--wine", action="store_true", help="test mode on the Mac: run cl.exe under wine with Z: paths")
    a = ap.parse_args()
    a.token = a.token or os.path.join(a.root, "token")
    srv = Server(a)

    class H(socketserver.BaseRequestHandler):
        def handle(self):
            try:
                srv.handle(self.request)
            except (OSError, ConnectionError, ValueError, KeyError):
                pass

    class T(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True
        request_queue_size = 64

    with T((a.bind, a.port), H) as s:
        print("compile server on %s:%d, %d slots, store %s" % (a.bind, a.port, a.slots, srv.store), flush=True)
        s.serve_forever()


if __name__ == "__main__":
    main()
