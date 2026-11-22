#!/usr/bin/env python3
"""eqremote.py: run an `equiv.py` row (or its shards) on the Windows laptop and fetch the result JSON.

Library (used by tools/difftest/sweep_nonmatching.py):
    run_remote(equiv_args, json_path, shards=1, timeout=900)  -> True when json_path now holds the laptop's result,
                                                                 False when the caller must run locally (the reason is
                                                                 appended to work/locks/local_equiv.log)
CLI (a row by hand; shards run in parallel on the laptop and are merged here with merge_shards.py):
    tools/remote/eqremote.py [--shards N] [--timeout S] ROW_ARGS... --json OUT.json
    e.g. eqremote.py --shards 4 facegen_win 004f1460 --src src/game/editor/terrain_page/terrain_page_init.cpp --fp-tol --json /tmp/x.json
    (exit 3 = declined, nothing ran: run equiv.py locally)

How: the current tree (git ls-files -co --exclude-standard) is tarred, keyed by a content hash, and unpacked once into
D:\\mb\\eq\\s\\<hash> on the laptop, whose `work`, `game` and `.venv` are junctions to D:\\mb\\repo's (the unpacked image,
toolchain, regress inputs and venv live there, see docs/remote_worker.md). The row then runs detached at Idle priority
through startjob.ps1 (-NoQueue: a laptop at its 11-job cap declines instead of queueing), the JSON is copied back.
The tree is pinned at the first call of a process: edits made later are not seen by that process's remote rows.
Off with EQUIV_REMOTE=0 or without the connection settings ($MB_WORKER_SSH/SCP/HOST, else ~/.config/mb-worker).
A laptop that fails a connection is skipped for 60 s (work/locks/eq_job.down). Standard library only.
"""
import hashlib, io, json, os, shlex, subprocess, sys, tarfile, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LOCKS = os.path.join(ROOT, "work", "locks")
LOG = os.path.join(LOCKS, "local_equiv.log")
DOWN = os.path.join(LOCKS, "eq_job.down")
EQ = "D:\\mb\\eq"
_pin, _lock = None, threading.Lock()


def _cfg():
    env = dict(os.environ)
    if not env.get("MB_WORKER_SSH"):
        try:
            for line in open(os.path.expanduser("~/.config/mb-worker")):
                if "=" in line and not line.startswith("#"):
                    k, v = line.rstrip("\n").split("=", 1)
                    env.setdefault(k.strip(), v)
        except OSError:
            pass
    return env if all(env.get(k) for k in ("MB_WORKER_SSH", "MB_WORKER_SCP", "MB_WORKER_HOST")) else None


def enabled():
    # work/locks/laptop_off: the user switched the laptop off; rows run here (rcl.py and equiv/remote.py honour it too)
    return (os.environ.get("EQUIV_REMOTE") != "0" and not os.path.exists(os.path.join(LOCKS, "laptop_off"))
            and _cfg() is not None)


def log_local(why, args):
    try:
        os.makedirs(LOCKS, exist_ok=True)
        with open(LOG, "a") as f:
            f.write("%s %s: %s\n" % (time.strftime("%F %T"), why, " ".join(args)[:300]))
    except OSError:
        pass


def _run(cmd, timeout=120, tries=3):
    """cmd (list) with retries: the laptop's sshd refuses bursts of connections for a while."""
    last = None
    for i in range(tries):
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", timeout=timeout)
        except subprocess.TimeoutExpired:
            last = "timeout"
            continue
        if r.returncode != 255 and not (r.returncode and "Permission denied" in r.stderr):
            return r
        last = (r.stderr or "").strip()[-200:]
        time.sleep(3 * (i + 1))
    raise OSError("ssh/scp failed: %s" % last)


def ssh(cmd, timeout=120, tries=3):
    e = _cfg()
    return _run(shlex.split(e["MB_WORKER_SSH"]) + [cmd], timeout, tries)


def scp_put(local, remote):
    e = _cfg()
    r = _run(shlex.split(e["MB_WORKER_SCP"]) + [local, "%s:%s" % (e["MB_WORKER_HOST"], remote)], 300)
    if r.returncode:
        raise OSError("scp put: " + r.stderr[-200:])


def scp_get(remote, local):
    e = _cfg()
    r = _run(shlex.split(e["MB_WORKER_SCP"]) + ["%s:%s" % (e["MB_WORKER_HOST"], remote), local], 300)
    if r.returncode:
        raise OSError("scp get: " + r.stderr[-200:])


def _files():
    out = subprocess.check_output(["git", "-C", ROOT, "ls-files", "-co", "--exclude-standard", "-z"])
    fs = sorted(set(p for p in out.decode("utf-8", "surrogateescape").split("\0") if p))
    return [p for p in fs if os.path.isfile(os.path.join(ROOT, p))]


def snapshot():
    """Make sure the laptop has this tree unpacked; returns its hash. Pinned for the life of the process."""
    global _pin
    with _lock:
        if _pin:
            return _pin
        fs = _files()
        h = hashlib.sha1()
        for p in fs:
            h.update(p.encode("utf-8", "surrogateescape") + b"\0")
            h.update(hashlib.sha1(open(os.path.join(ROOT, p), "rb").read()).digest())
        key = h.hexdigest()[:16]
        d = "%s\\s\\%s" % (EQ, key)
        r = ssh('if exist %s\\ok (echo HAVE) else (echo NEED)' % d)
        if "HAVE" not in r.stdout:
            with tempfile.TemporaryDirectory() as td:
                tgz = os.path.join(td, key + ".tgz")
                with tarfile.open(tgz, "w:gz", compresslevel=3) as t:
                    for p in fs:
                        t.add(os.path.join(ROOT, p), arcname=p, recursive=False)
                ssh("if not exist %s\\in mkdir %s\\in" % (EQ, EQ))
                scp_put(tgz, "%s\\in\\%s.tgz" % (EQ.replace("\\", "/"), key))
            if not os.path.exists(os.path.join(HERE, "startjob.ps1")):
                raise OSError("startjob.ps1 missing")
            scp_put(os.path.join(HERE, "startjob.ps1"), "D:/mb/startjob.ps1")
            r = ssh('(rmdir /s /q {d} 2>nul) & mkdir {d} && tar -xzf {eq}\\in\\{k}.tgz -C {d} && '
                    'mklink /J {d}\\work D:\\mb\\repo\\work && mklink /J {d}\\game D:\\mb\\repo\\game && '
                    'mklink /J {d}\\.venv D:\\mb\\repo\\.venv && echo ok> {d}\\ok && del {eq}\\in\\{k}.tgz && echo UNPACKED'
                    .format(d=d, eq=EQ, k=key), timeout=600)
            if "UNPACKED" not in r.stdout:
                raise OSError("unpack failed: %s %s" % (r.stdout[-200:], r.stderr[-200:]))
        ssh("if not exist {eq}\\out mkdir {eq}\\out & forfiles /p {eq}\\out /d -3 /c \"cmd /c del @path\" 2>nul".format(eq=EQ))
        _pin = key
        return key


def prune(days=3):
    """Remove snapshots older than `days` (junctions unlinked first, so D:\\mb\\repo is never entered)."""
    ssh('for /d %d in ({eq}\\s\\*) do @(forfiles /p "%d" /m ok /d -{n} /c "cmd /c echo old" >nul 2>&1 && '
        '(rmdir "%d\\work" & rmdir "%d\\game" & rmdir "%d\\.venv" & rmdir /s /q "%d"))'.format(eq=EQ, n=days))


def _rel(a):
    return os.path.relpath(a, ROOT).replace("/", "\\") if os.path.isabs(a) and a.startswith(ROOT + os.sep) else a


def run_remote(args, json_path, shards=1, timeout=900):
    """args: equiv.py arguments after the script (no --json/--shard; paths under the repo may be absolute)."""
    if not enabled():
        log_local("remote off" if os.environ.get("EQUIV_REMOTE") == "0" else "remote not configured", args)
        return False
    try:
        if time.time() - os.path.getmtime(DOWN) < 60:
            log_local("laptop marked down", args)
            return False
    except OSError:
        pass
    rargs = [_rel(a) for a in args]
    if any((" " in a or '"' in a or "&" in a or "%" in a or "|" in a or "^" in a) for a in rargs):
        log_local("arguments not shippable", args)
        return False
    ident = hashlib.sha1((" ".join(rargs) + str(os.getpid()) + str(time.time())).encode()).hexdigest()[:10]
    started = []
    try:
        key = snapshot()
        for k in range(shards):
            name = "eq_%s_%d" % (ident, k)
            sh = ["--shard", "%d/%d" % (k, shards)] if shards > 1 else []
            cmd = ("(cd /d {eq}\\s\\{key}&& set DIFFTEST_NO_LEDGER=1&& .venv\\Scripts\\python tools\\difftest\\equiv.py {a} {sh} "
                   "--json {eq}\\out\\{n}.json & echo done> {eq}\\out\\{n}.done)").format(
                eq=EQ, key=key, a=" ".join(rargs), sh=" ".join(sh), n=name)
            r = ssh('powershell -NoProfile -ExecutionPolicy Bypass -File D:\\mb\\startjob.ps1 -Name %s -NoQueue -Cmd "%s"' % (name, cmd))
            if r.returncode == 3 or "declined" in r.stdout:
                _kill(started)
                log_local("laptop at its job cap", args)
                return False
            if "started" not in r.stdout:
                raise OSError("start failed: %s %s" % (r.stdout[-200:], r.stderr[-200:]))
            started.append(name)
        t0 = time.time()
        for name in started:
            left = max(30, timeout - (time.time() - t0))
            while True:
                try:
                    r = ssh('powershell -NoProfile -Command "$t=[DateTime]::Now.AddSeconds(%d); while(-not (Test-Path %s\\out\\%s.done) -and '
                            '[DateTime]::Now -lt $t){Start-Sleep 3}; if(Test-Path %s\\out\\%s.done){\'FIN\'}"'
                            % (left, EQ, name, EQ, name), timeout=left + 60, tries=5)
                except OSError:
                    raise
                if "FIN" in r.stdout:
                    break
                _kill(started)
                log_local("timeout on laptop", args)
                return False
        parts = []
        with tempfile.TemporaryDirectory() as td:
            for name in started:
                p = os.path.join(td, name + ".json")
                try:
                    scp_get("%s/out/%s.json" % (EQ.replace("\\", "/"), name), p)
                except OSError:
                    if os.path.exists(p):
                        os.remove(p)
                if not os.path.exists(p):
                    t = ssh("powershell -NoProfile -Command \"gc D:\\mb\\jobs\\%s.log -Tail 8\"" % name).stdout
                    log_local("laptop produced no JSON (%s)" % t.replace("\n", " | ")[-200:], args)
                    return False
                parts.append(p)
            if len(parts) == 1:
                out = open(parts[0], "rb").read()
            else:
                m = os.path.join(td, "merged.json")
                r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "difftest", "merge_shards.py"), m] + parts,
                                   capture_output=True, text=True)
                if not os.path.exists(m):
                    log_local("merge_shards failed: %s" % r.stderr[-200:], args)
                    return False
                out = open(m, "rb").read()
        os.makedirs(os.path.dirname(os.path.abspath(json_path)), exist_ok=True)
        open(json_path, "wb").write(out)
        return True
    except (OSError, subprocess.SubprocessError) as e:
        try:
            os.makedirs(LOCKS, exist_ok=True)
            open(DOWN, "w").write(str(e))
        except OSError:
            pass
        _kill(started)
        log_local("laptop unreachable/failed: %s" % e, args)
        return False


def _kill(names):
    for n in names:
        try:
            ssh('powershell -NoProfile -Command "if(Test-Path D:\\mb\\jobs\\{n}.pid){{taskkill /pid (gc D:\\mb\\jobs\\{n}.pid) /t /f}}"'.format(n=n), timeout=30, tries=1)
        except OSError:
            pass


def main(argv):
    shards, timeout, rest = 1, 3600, []
    i = 0
    while i < len(argv):
        if argv[i] == "--shards":
            shards = int(argv[i + 1]); i += 2
        elif argv[i] == "--timeout":
            timeout = float(argv[i + 1]); i += 2
        else:
            rest.append(argv[i]); i += 1
    if "--json" not in rest:
        sys.exit("need --json OUT.json")
    j = rest.index("--json")
    out = rest[j + 1]
    args = rest[:j] + rest[j + 2:]
    sys.exit(0 if run_remote(args, out, shards, timeout) else 3)


if __name__ == "__main__":
    main(sys.argv[1:])
