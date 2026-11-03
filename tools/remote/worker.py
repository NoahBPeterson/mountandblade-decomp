#!/usr/bin/env python3
"""worker.py: sync a commit to the Windows worker and run / fetch jobs there (see docs/remote_worker.md).

usage: worker.py sync [REV]               bundle REV (default HEAD), copy it, check it out (detached) in D:\\mb\\repo
       worker.py run NAME "COMMAND"       start COMMAND (cmd.exe line, cwd D:\\mb\\repo) at Idle priority via startjob.ps1,
                                          which waits while the cap of 11 heavy jobs is reached
       worker.py status                   the jobs' pids and which are alive
       worker.py tail NAME [N]            last N lines of the job's log
       worker.py get REMOTE LOCAL         copy a file back (REMOTE like D:/mb/repo/work/x.json)
       worker.py ssh "COMMAND"            any cmd.exe command

The connection is not in the repo: MB_WORKER_SSH is the ssh command up to the command to run (e.g. "ssh user@host", or
a wrapper script), MB_WORKER_SCP the scp command (same host, e.g. "scp -q"), MB_WORKER_HOST the "user@host" scp prefix.
"""
import os, shlex, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REMOTE = "D:/mb"


def env(name):
    v = os.environ.get(name)
    if not v:
        sys.exit("set %s (see docs/remote_worker.md)" % name)
    return shlex.split(v)


def ssh(cmd, check=True):
    return subprocess.run(env("MB_WORKER_SSH") + [cmd], check=check)


def scp_put(local, remote):
    subprocess.run(env("MB_WORKER_SCP") + [local, "%s:%s" % (os.environ["MB_WORKER_HOST"], remote)], check=True)


def scp_get(remote, local):
    subprocess.run(env("MB_WORKER_SCP") + ["%s:%s" % (os.environ["MB_WORKER_HOST"], remote), local], check=True)


def main(argv):
    if not argv:
        sys.exit(__doc__)
    c, a = argv[0], argv[1:]
    if c == "sync":
        rev = a[0] if a else "HEAD"
        sha = subprocess.check_output(["git", "-C", ROOT, "rev-parse", rev], text=True).strip()
        with tempfile.TemporaryDirectory() as d:
            b = os.path.join(d, "sync.bundle")
            subprocess.run(["git", "-C", ROOT, "bundle", "create", b, rev], check=True, stderr=subprocess.DEVNULL)
            scp_put(b, REMOTE + "/sync.bundle")
        ssh("cd /d D:\\mb\\repo && git fetch D:\\mb\\sync.bundle && git checkout --detach -f %s && git log --oneline -1" % sha)
        scp_put(os.path.join(ROOT, "tools", "remote", "startjob.ps1"), REMOTE + "/startjob.ps1")
    elif c == "run":
        name, cmd = a
        ssh('powershell -NoProfile -ExecutionPolicy Bypass -File D:\\mb\\startjob.ps1 -Name %s -Cmd "%s"' % (name, cmd.replace('"', '\\"')))
    elif c == "status":
        ssh('powershell -NoProfile -Command "foreach ($f in gci D:\\mb\\jobs\\*.pid) { $p=[int](gc $f.FullName); '
            '\\"$($f.BaseName) $p alive=$([bool](Get-Process -Id $p -ea 0))\\" }; '
           '"')
    elif c == "tail":
        ssh('powershell -NoProfile -Command "gc D:\\mb\\jobs\\%s.log -Tail %s"' % (a[0], a[1] if len(a) > 1 else 20))
    elif c == "get":
        scp_get(a[0], a[1])
    elif c == "ssh":
        ssh(a[0])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
