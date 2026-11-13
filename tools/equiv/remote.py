#!/usr/bin/env python3
"""Run the angr layer's solver on the Windows laptop (tools/remote/angr_server.py), falling back to the local angr
venv when the laptop declines. Standard library only.

How it plugs in: angr_layer.prove(), regions.py's worker pool and cexmodel.prove() start their solver as
`$EQUIV_ANGR_PYTHON -I <script> --worker|--serve` and talk JSON over its stdin/stdout. enable() points
$EQUIV_ANGR_PYTHON at this file, which then acts as that interpreter: it reserves a solver slot on the laptop, ships
the script and angr_layer.py (content-addressed: the laptop runs exactly our files), and relays stdin/stdout. The job
is prepared on the Mac as always (image, object, symbols), so the laptop only solves; its venv is the same lock file
(tools/equiv/angr-requirements.txt), and both sides run with PYTHONHASHSEED=0. When the laptop is busy (all slots
taken), unreachable or not configured, this process becomes the local interpreter instead (exec), so callers see no
difference; their timeouts kill it as before, and the server kills the remote solver when the connection drops.

  prove.py / sweep.py / quick.py --remote          or   EQUIV_REMOTE=1 in the environment
  EQUIV_REMOTE_DEBUG=1                              one stderr line per solver: where it ran and why (always
                                                    appended to work/equiv/remote.log too)

Configuration (nothing secret in the repo): the endpoint is ~/.config/mb-angr-server (host:port) or $MB_EQ_ENDPOINT,
else the compile server's host (~/.config/mb-compile-server) at port 47814; the token is the compile server's
(~/.config/mb-compile-token or $MB_RC_TOKEN). A failed connection marks the laptop down for 30 s
(work/locks/eq.down), so a sweep does not pay the connect timeout per solver.
"""
import hashlib, hmac, json, os, socket, struct, sys, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PORT = 47814
CONNECT_TIMEOUT = 2.0


def local_python():
    p = os.environ.get("EQUIV_ANGR_LOCAL")
    if p:
        return p
    for c in (os.path.join(ROOT, "work", "angr-venv", "bin", "python"),
              os.path.join(ROOT, "work", "angr-venv", "Scripts", "python.exe")):
        if os.path.exists(c):
            return c
    return None


def enable():
    """route this process's (and its children's) angr solvers through the laptop. Returns False when not
    configured (then nothing changes)."""
    if os.environ.get("EQUIV_REMOTE") == "0" or not endpoint() or not token():
        return False
    cur = os.environ.get("EQUIV_ANGR_PYTHON")
    me = os.path.abspath(__file__)
    if cur and os.path.abspath(cur) != me:
        os.environ.setdefault("EQUIV_ANGR_LOCAL", cur)
    os.environ["EQUIV_ANGR_PYTHON"] = me
    os.environ["EQUIV_REMOTE"] = "1"
    return True


def _cfg(name):
    try:
        return open(os.path.expanduser("~/.config/" + name)).read().strip()
    except OSError:
        return None


def endpoint():
    ep = os.environ.get("MB_EQ_ENDPOINT") or _cfg("mb-angr-server")
    if ep:
        return ep
    rc = os.environ.get("MB_RC_ENDPOINT") or _cfg("mb-compile-server")
    return "%s:%d" % (rc.rsplit(":", 1)[0], PORT) if rc else None


def token():
    p = os.environ.get("MB_RC_TOKEN") or os.path.expanduser("~/.config/mb-compile-token")
    try:
        return open(p, "rb").read().strip()
    except OSError:
        return None


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
    return json.loads(recv_exact(sock, struct.unpack(">I", recv_exact(sock, 4))[0]))


def mac(tok, nonce, who):
    return hmac.new(tok, (nonce + who).encode(), hashlib.sha256).hexdigest()


LOG = os.path.join(ROOT, "work", "equiv", "remote.log")


def debug(msg):
    """where a solver ran: stderr with EQUIV_REMOTE_DEBUG=1, and one line in work/equiv/remote.log (the callers
    swallow their solver's stderr)"""
    if os.environ.get("EQUIV_REMOTE_DEBUG"):
        sys.stderr.write("angr remote: %s\n" % msg)
        sys.stderr.flush()
    try:
        with open(LOG, "a") as f:
            f.write("%s %d %s\n" % (time.strftime("%Y-%m-%dT%H:%M:%S"), os.getpid(), msg))
    except OSError:
        pass


def go_local(argv, why, data=None):
    """become the local angr interpreter (exec), or run it with the stdin already read (data)"""
    py = local_python()
    debug("local (%s)" % why)
    if not py:
        sys.stdout.write(json.dumps({"status": "unknown", "detail": "angr: the laptop declined (%s) and no local "
                                     "angr venv" % why, "hints": {}}) + "\n")
        return 0
    if data is None:
        os.execv(py, [py] + argv)
    import subprocess
    r = subprocess.run([py] + argv, input=data, stdout=subprocess.PIPE)
    sys.stdout.buffer.write(r.stdout)
    sys.stdout.flush()
    return r.returncode


def connect():
    """-> an authenticated socket holding a solver slot, or (None, why)"""
    ep, tok = endpoint(), token()
    if not ep or not tok:
        return None, "not configured"
    down = os.path.join(ROOT, "work", "locks", "eq.down")
    try:
        if time.time() - os.path.getmtime(down) < 30:
            return None, "marked down"
    except OSError:
        pass
    host, port = ep.rsplit(":", 1)
    try:
        sock = socket.create_connection((host, int(port)), timeout=CONNECT_TIMEOUT)
        sock.settimeout(15)
        nonce = recv(sock)["nonce"]
        send(sock, {"mac": mac(tok, nonce, "C")})
        if not hmac.compare_digest(recv(sock).get("mac", ""), mac(tok, nonce, "S")):
            raise ConnectionError("server failed authentication")
        send(sock, {"op": "reserve"})
        if not recv(sock).get("ok"):
            sock.close()
            return None, "busy (no free slot)"
        return sock, None
    except (OSError, ConnectionError, ValueError, KeyError) as e:
        try:
            os.makedirs(os.path.dirname(down), exist_ok=True)
            open(down, "w").close()
            os.utime(down)
        except OSError:
            pass
        return None, "unreachable (%s)" % type(e).__name__


def shim(argv):
    """argv as given to an interpreter: ['-I', script, '--worker'|'--serve']"""
    args = [a for a in argv if a != "-I"]
    if len(args) != 2 or args[1] not in ("--worker", "--serve"):
        return go_local(argv, "not a solver command")
    script, mode = os.path.abspath(args[0]), args[1]
    sock, why = connect()
    if sock is None:
        return go_local(argv, why)
    files = {}
    for p in {script, os.path.join(HERE, "angr_layer.py")}:
        data = open(p, "rb").read()
        files[os.path.basename(p)] = (hashlib.sha1(data).hexdigest(), data)
    worker = mode == "--worker"
    stdin_data = sys.stdin.buffer.read() if worker else None
    got_output = False
    try:
        sock.settimeout(60)
        send(sock, {"op": "run", "script": os.path.basename(script), "args": [mode],
                    "files": {k: v[0] for k, v in files.items()}})
        need = recv(sock)
        if "exit" in need:
            raise ConnectionError("refused: %s" % need.get("err"))
        by_sha = {v[0]: v[1] for v in files.values()}
        for h in need["need"]:
            sock.sendall(struct.pack(">Q", len(by_sha[h])) + by_sha[h])
        wl = threading.Lock()
        if worker:
            send(sock, {"in": stdin_data.decode("utf-8", "surrogateescape")}, wl)
            send(sock, {"eof": 1}, wl)
        else:
            def pump():
                try:
                    while True:
                        c = os.read(0, 1 << 16)
                        if not c:
                            send(sock, {"eof": 1}, wl)
                            return
                        send(sock, {"in": c.decode("utf-8", "surrogateescape")}, wl)
                except (OSError, ConnectionError):
                    return
            threading.Thread(target=pump, daemon=True).start()
        sock.settimeout(None)
        debug("laptop %s (%s %s)" % (endpoint(), os.path.basename(script), mode))
        while True:
            m = recv(sock)
            if "out" in m:
                got_output = True
                sys.stdout.buffer.write(m["out"].encode("utf-8", "surrogateescape"))
                sys.stdout.flush()
            elif "exit" in m:
                if m.get("err"):
                    sys.stderr.write(m["err"])
                return m["exit"]
    except (OSError, ConnectionError, ValueError, KeyError) as e:
        if worker and not got_output:
            return go_local(argv, "lost the laptop before a result (%s)" % e, stdin_data)
        sys.stderr.write("angr remote: connection lost: %s\n" % e)
        return 1
    finally:
        sock.close()


if __name__ == "__main__":
    sys.exit(shim(sys.argv[1:]))
