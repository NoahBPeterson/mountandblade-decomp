#!/usr/bin/env python3
"""record_win.py: record a Newton 1.x call trace of the ORIGINAL game on the Windows laptop (see RECORDING.md).

    record_win.py setup              build NewtLg.dll + mb_physlog.exe, install them next to the game, start the game
                                     in the user's session (scheduled task mb_physlog -> win/physdrive.ps1)
    record_win.py do CMD [CMD ...]   send commands to physdrive.ps1 in order ("click 0.5 0.4", "key 13", "shot menu",
                                     "close", "quit"; "sleep S" waits here); shots land in work/physics_traces/shots/
    record_win.py finish NAME        end the game, fetch the trace as work/physics_traces/NAME.bin, remove everything
                                     this tool put on the laptop and check the install's Newton.dll is unchanged

Nothing in the install is modified: mb_physlog.exe is a copy of the unpacked original (game/mb_original.exe) whose one
import name "Newton.dll" reads "NewtLg.dll", and NewtLg.dll (the proxy) loads the install's own Newton.dll. Both files
are added next to it and deleted by `finish`. The connection is smoke.py's (MB_WORKER_*, ~/.config/mb-worker).
"""
import json, os, shutil, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "relink"))
import smoke  # noqa: E402  (win_cfg / win_ssh / win_scp)

GAME = smoke.WIN_GAME
WDIR = r"D:\mb\phys"
TASK = "mb_physlog"
WORK = os.path.join(ROOT, "work", "physics_traces")
STATE = os.path.join(WORK, "state.json")
EXE, DLL = "mb_physlog.exe", "NewtLg.dll"
NEWTON_SHA1 = "6e6f5caaf8b343d450b8ec43182c59c7cf5264de"   # game/Newton.dll (2006-05-24 build) on the Mac


def fwd(p):
    return p.replace("\\", "/")


def make_exe(dst):
    d = bytearray(open(os.path.join(ROOT, "game", "mb_original.exe"), "rb").read())
    i = d.find(b"Newton.dll\0")
    assert i > 0 and d.find(b"Newton.dll\0", i + 1) < 0, "expected one Newton.dll import name"
    d[i:i + 10] = b"NewtLg.dll"
    open(dst, "wb").write(d)


def setup(cfg):
    host = cfg[2]
    os.makedirs(os.path.join(WORK, "bin"), exist_ok=True)
    subprocess.run([os.path.join(ROOT, "port", "physics", "trace", "build_proxy.sh"), os.path.join(WORK, "bin")], check=True)
    make_exe(os.path.join(WORK, "bin", EXE))
    o = smoke.win_ssh(cfg, 'if exist "%s\\%s" (echo EXISTS) & if exist "%s\\%s" (echo EXISTS) & tasklist /fi "imagename eq %s" /nh'
                      % (GAME, EXE, GAME, DLL, EXE))
    if "EXISTS" in o or EXE in o:
        sys.exit("a previous recording is still installed or running: run `finish` first")
    run_cmd = os.path.join(WORK, "bin", "run.cmd")
    with open(run_cmd, "w", newline="") as f:
        f.write('@powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File %s\\physdrive.ps1 -GameDir "%s" '
                '-Exe %s -Dir %s -Trace %s\\trace.bin\r\n' % (WDIR, GAME, EXE, WDIR, WDIR))
    smoke.win_ssh(cfg, "(if exist %s rmdir /s /q %s) & mkdir %s\\cmd" % (WDIR, WDIR, WDIR))
    # one tar, one copy: the laptop's sshd refuses bursts of connections
    shutil.copy(os.path.join(ROOT, "port", "physics", "trace", "win", "physdrive.ps1"), os.path.join(WORK, "bin"))
    tar = os.path.join(WORK, "bin", "phys.tar")
    subprocess.run(["tar", "cf", tar, "-C", os.path.join(WORK, "bin"), EXE, DLL, "run.cmd", "physdrive.ps1"], check=True,
                   env=dict(os.environ, COPYFILE_DISABLE="1"))
    if not smoke.win_scp(cfg, tar, "%s:%s/phys.tar" % (host, fwd(WDIR))):
        sys.exit("could not copy %s" % tar)
    smoke.win_ssh(cfg, "cd /d %s && tar xf phys.tar && del phys.tar" % WDIR)
    user = host.split("@")[0]
    o = smoke.win_ssh(cfg, 'copy /y %s\\%s "%s\\%s" & copy /y %s\\%s "%s\\%s" & '
                      'schtasks /create /tn %s /tr "%s\\run.cmd" /sc once /st 00:00 /ru %s /it /f & schtasks /run /tn %s'
                      % (WDIR, EXE, GAME, EXE, WDIR, DLL, GAME, DLL, TASK, WDIR, user, TASK))
    print(o.strip())
    json.dump({"next": 1}, open(STATE, "w"))


def do(cfg, cmds):
    st = json.load(open(STATE))
    host = cfg[2]
    os.makedirs(os.path.join(WORK, "shots"), exist_ok=True)
    for c in cmds:
        if c.startswith("sleep "):
            time.sleep(float(c.split()[1]))
            continue
        n = st["next"]
        tmp = os.path.join(WORK, "bin", "cmd.txt")
        open(tmp, "w").write(c + "\n")
        # written under a temporary name and renamed, so the script never reads half a command
        if not smoke.win_scp(cfg, tmp, "%s:%s/cmd/%04d.tmp" % (host, fwd(WDIR), n)):
            sys.exit("could not send %r" % c)
        smoke.win_ssh(cfg, "move /y %s\\cmd\\%04d.tmp %s\\cmd\\%04d.txt >nul" % (WDIR, n, WDIR, n))
        st["next"] = n + 1
        json.dump(st, open(STATE, "w"))
        res = ""
        for _ in range(60):
            o = smoke.win_ssh(cfg, 'if exist %s\\cmd\\%04d.done (type %s\\cmd\\%04d.done) else (if exist %s\\out\\done (echo SCRIPT-ENDED))'
                              % (WDIR, n, WDIR, n, WDIR))
            if o.strip():
                res = o.strip()
                break
            time.sleep(1)
        print("%s -> %s" % (c, res or "no answer"))
        if c.startswith("shot ") and res.startswith("ok"):
            name = c.split()[1]
            smoke.win_scp(cfg, "%s:%s/out/%s.png" % (host, fwd(WDIR), name), os.path.join(WORK, "shots", name + ".png"))
        if "SCRIPT-ENDED" in res:
            print(smoke.win_ssh(cfg, "type %s\\out\\status.txt" % WDIR).strip())
            break


def finish(cfg, name):
    host = cfg[2]
    o = smoke.win_ssh(cfg, "if exist %s\\out\\done (echo DONE)" % WDIR)
    if "DONE" not in o:
        try:
            do(cfg, ["quit"])
        except SystemExit:
            pass
        for _ in range(20):
            if "DONE" in smoke.win_ssh(cfg, "if exist %s\\out\\done (echo DONE)" % WDIR):
                break
            time.sleep(2)
    print(smoke.win_ssh(cfg, "type %s\\out\\status.txt & dir %s\\trace.bin" % (WDIR, WDIR)).strip())
    out = os.path.join(WORK, name + ".bin")
    got = smoke.win_scp(cfg, "%s:%s/trace.bin" % (host, fwd(WDIR)), out)
    print("trace:", out if got else "NOT FETCHED", os.path.getsize(out) if got else "")
    smoke.win_ssh(cfg, 'for /f %%p in (%s\\out\\game.pid) do taskkill /pid %%p /fi "imagename eq %s" /f' % (WDIR, EXE))
    smoke.win_ssh(cfg, "schtasks /end /tn %s & schtasks /delete /tn %s /f" % (TASK, TASK))
    smoke.win_ssh(cfg, 'del /q "%s\\%s" "%s\\%s"' % (GAME, EXE, GAME, DLL))
    smoke.win_ssh(cfg, "rmdir /s /q %s" % WDIR)
    o = smoke.win_ssh(cfg, 'certutil -hashfile "%s\\Newton.dll" SHA1 & dir /b "%s" & tasklist /fi "imagename eq %s" /nh & schtasks /query /tn %s'
                      % (GAME, GAME, EXE, TASK))
    ok_hash = NEWTON_SHA1 in o.replace(" ", "").lower()
    left = [f for f in (EXE, DLL) if f in o]
    print("install Newton.dll sha1 %s; leftovers: %s" % ("unchanged" if ok_hash else "DIFFERS (check!)", left or "none"))


def main():
    cfg = smoke.win_cfg()
    if not cfg:
        sys.exit("laptop not configured (MB_WORKER_*)")
    a = sys.argv[1:]
    if a[:1] == ["setup"]:
        setup(cfg)
    elif a[:1] == ["do"]:
        do(cfg, a[1:])
    elif a[:1] == ["finish"] and len(a) == 2:
        finish(cfg, a[1])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
