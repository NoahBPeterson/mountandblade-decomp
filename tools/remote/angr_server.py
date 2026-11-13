"""Remote angr server: runs the angr layer's solver processes (tools/equiv/angr_layer.py --worker / --serve, or
tools/equiv/cexmodel.py --worker) for Mac clients (tools/equiv/remote.py). Runs on the Windows laptop under D:\\mb\\eq
with the pinned venv D:\\mb\\angr-venv (tools/equiv/setup_angr_venv.py); see docs/remote_worker.md.

Why only the solver: angr_layer.prove() prepares a self-contained JSON job on the Mac (the original's code from
work/mb.analysis.bin, our object's code patched with the relocation pairing, callee facts from the image and the
symbols file) and hands it to a child process on stdin; the child needs nothing but the job and the Python files.
So a proof made here reads exactly the job bytes the Mac would have solved, with the same pinned package versions,
and returns the same verdict.

Protocol (TCP; framing and authentication as tools/remote/compile_server.py: 4-byte length + JSON, HMACs of a fresh
nonce with the shared token, never the token itself):
  S -> C  {"nonce": hex};  C -> S {"mac": ...};  S -> C {"mac": ..., "slots": N, "busy": n}
  C -> S  {"op": "reserve"} -> {"ok": bool}            (one of N solver slots for this connection; busy: false)
  C -> S  {"op": "run", "script": "angr_layer.py", "args": ["--worker"], "files": {"angr_layer.py": sha1, ...}}
  S -> C  {"need": [sha1, ...]};  C -> S one blob per needed sha1 (8-byte length + bytes)
  then, both ways at once until the child exits:
  C -> S  {"in": text} ... {"eof": 1}                  (the child's stdin)
  S -> C  {"out": text} ...  then {"exit": rc, "err": last 2000 characters of stderr}
The files are content-addressed in D:\\mb\\eq\\store; each run gets a fresh stage directory with only them (so a
client's angr_layer.py is the one that runs, whatever the laptop's checkout holds). Only the venv's python.exe is
started, with the script from the stage and the arguments --worker / --serve; anything else is refused. The child
runs at Idle priority with PYTHONHASHSEED=0 and is killed (by its own PID, which this server started) when the
connection drops, when the client's stdin ends and the child outlives --max-seconds, or at --max-seconds.
"""
import argparse, hashlib, hmac, json, os, re, secrets, shutil, socket, socketserver, struct, subprocess, sys, \
    threading, time

IS_WIN = sys.platform == "win32"
SCRIPTS = {"angr_layer.py", "cexmodel.py"}
MODES = {"--worker", "--serve"}


def send(sock, obj, lock=None):
    b = json.dumps(obj).encode()
    data = struct.pack(">I", len(b)) + b
    if lock:
        with lock:
            sock.sendall(data)
    else:
        sock.sendall(data)


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
    if n > 256 << 20:
        raise ConnectionError("message too large")
    return json.loads(recv_exact(sock, n))


def recv_blob(sock):
    n = struct.unpack(">Q", recv_exact(sock, 8))[0]
    if n > 64 << 20:
        raise ConnectionError("blob too large")
    return recv_exact(sock, n)


def mac(token, nonce, who):
    return hmac.new(token, (nonce + who).encode(), hashlib.sha256).hexdigest()


def kill(p):
    if p.poll() is not None:
        return
    try:
        if IS_WIN:
            subprocess.run(["taskkill", "/pid", str(p.pid), "/t", "/f"], capture_output=True)
        else:
            p.kill()
    except OSError:
        pass
    try:
        p.wait(10)
    except Exception:
        pass


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
        self.last_activity = time.time()

    def blob_path(self, sha):
        return os.path.join(self.store, sha[:2], sha)

    def handle(self, sock):
        sock.settimeout(60)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
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
                self.last_activity = time.time()
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
                elif m["op"] == "run" and held:
                    self.run(sock, m)
                    return
                else:
                    return
        finally:
            if held:
                with self.lock:
                    self.busy -= 1
                self.sem.release()
            self.last_activity = time.time()

    def run(self, sock, m):
        files, script, args = m.get("files") or {}, m.get("script"), m.get("args") or []
        if script not in SCRIPTS or script not in files or not set(args) <= MODES or len(args) != 1 or \
                any(not re.fullmatch(r"[A-Za-z0-9_]+\.py", k) for k in files) or \
                any(not re.fullmatch(r"[0-9a-f]{40}", h) for h in files.values()):
            send(sock, {"exit": 2, "err": "refused"})
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
        os.makedirs(stage)
        try:
            for name, h in files.items():
                shutil.copyfile(self.blob_path(h), os.path.join(stage, name))
            env = dict(os.environ, PYTHONHASHSEED="0", PYTHONUTF8="1", EQUIV_ANGR_PYTHON="")
            kw = {"creationflags": 0x00000040 | 0x08000000} if IS_WIN else {}   # IDLE_PRIORITY_CLASS | NO_WINDOW
            errf = open(os.path.join(stage, "stderr.txt"), "w+b")
            p = subprocess.Popen([self.a.python, "-I", os.path.join(stage, script)] + args, cwd=stage, env=env,
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errf, **kw)
            wl = threading.Lock()
            deadline = time.time() + self.a.max_seconds
            state = {"eof": False, "gone": False}

            def pump_in():
                try:
                    sock.settimeout(None)
                    while True:
                        msg = recv(sock)
                        if "in" in msg:
                            p.stdin.write(msg["in"].encode("utf-8", "surrogateescape"))
                            p.stdin.flush()
                        elif msg.get("eof") and not state["eof"]:
                            state["eof"] = True
                            p.stdin.close()     # and keep reading: a closed connection must stop the solver
                except (OSError, ConnectionError, ValueError):
                    state["gone"] = True
                    kill(p)                    # the client went away (killed at its own timeout): stop the solver

            def watchdog():
                while p.poll() is None:
                    if time.time() > deadline:
                        kill(p)
                        return
                    time.sleep(1)
            threading.Thread(target=pump_in, daemon=True).start()
            threading.Thread(target=watchdog, daemon=True).start()
            try:
                for ln in iter(p.stdout.readline, b""):
                    send(sock, {"out": ln.decode("utf-8", "surrogateescape")}, wl)
            except (OSError, ConnectionError):
                kill(p)
                return
            rc = p.wait()
            errf.seek(0)
            err = errf.read().decode(errors="replace")[-2000:]
            errf.close()
            if not state["gone"]:
                send(sock, {"exit": rc, "err": err}, wl)
        finally:
            shutil.rmtree(stage, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=r"D:\mb\eq" if IS_WIN else os.path.expanduser("~/.cache/mb-eq"))
    ap.add_argument("--python", default=r"D:\mb\angr-venv\Scripts\python.exe" if IS_WIN else
                    os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
                                 "work", "angr-venv", "bin", "python"))
    ap.add_argument("--token", default=r"D:\mb\rc\token" if IS_WIN else os.path.expanduser("~/.config/mb-compile-token"))
    ap.add_argument("--bind", default="0.0.0.0", help="address to listen on (the Tailscale IP on the laptop)")
    ap.add_argument("--port", type=int, default=47814)
    ap.add_argument("--slots", type=int, default=6, help="solver processes at once (12 logical CPUs, shared with the "
                    "compile server and another project's game)")
    ap.add_argument("--max-seconds", type=float, default=3600, help="a child is killed after this long")
    ap.add_argument("--idle-exit", type=float, default=0, help="exit after this many seconds without a connection "
                    "(0: never)")
    a = ap.parse_args()
    srv = Server(a)

    class H(socketserver.BaseRequestHandler):
        def handle(self):
            try:
                srv.handle(self.request)
            except (OSError, ConnectionError, ValueError, KeyError) as e:
                sys.stderr.write("%s handler: %r\n" % (time.strftime("%H:%M:%S"), e))
                sys.stderr.flush()

    class T(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True
        request_queue_size = 64

    with T((a.bind, a.port), H) as s:
        print("angr server on %s:%d, %d slots, python %s" % (a.bind, a.port, a.slots, a.python), flush=True)
        if a.idle_exit:
            def idle():
                while True:
                    time.sleep(10)
                    if srv.busy == 0 and time.time() - srv.last_activity > a.idle_exit:
                        print("idle for %.0f s: exiting" % a.idle_exit, flush=True)
                        s.shutdown()
                        return
            threading.Thread(target=idle, daemon=True).start()
        s.serve_forever()


if __name__ == "__main__":
    main()
