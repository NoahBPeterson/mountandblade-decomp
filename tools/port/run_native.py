#!/usr/bin/env python3
"""run_native.py: run the native port (build/port/<target>/bin/mb) on game/ for a bounded time, with a script of
screenshots and input, then kill it. Every run ends with the game gone (SIGTERM, then SIGKILL).

usage: run_native.py [--time 60] [--out work/port_shots] [--tag run] [--gain 0.15] [--lldb] [--start SPEC]
                     [--state-timeout 120] [steps...] [--] [game args...]
  timed steps (any number, run at their time from launch):
       --at <sec> shot <name> | --at <sec> click <fx> <fy> | --at <sec> key <kVK code>
       (click coordinates are fractions of the window's content: 0.5 0.5 is the centre)
  state steps (run in command-line order, each after the one before it; port/mb_start.cpp writes the states):
       --wait <state>              until the game logs that state (MBSTATE lines in the log)
       --on <state> <action...>    --wait <state>, then the action
       --do <action...>            the action, right after the step before it
     actions: shot <name> | click <fx> <fy> | sweep <fx> <fy> <dx> <n> | scroll <lines> [n] | key <code> [hold ms] | sleep <sec> | end (kill the game now: the run is over)
     <state> is the event and its leading arguments as one word or quoted: menu_ready, "screen map", "menu start_game_1",
     "mission_frame 100" (mission_frame / load_step: that count or more), start_done. A state counts when it is logged after the step
     before it was done. A wait that does not come within --state-timeout seconds ends the run (exit status 3).
  --user-dir DIR sets MB_USER_DIR (saves and config there, never in yours); a DIR that does not exist is created with
       copies of ~/Documents/Mount&Blade Savegames and ~/Library/Application Support/Mount&Blade.
  --start SPEC sets MB_START (port/mb_start.cpp: quick_battle[:N], new_game, tutorial, load[:N], map[:N], or steps such as
       main:custom,option:start_game_3#1): the game drives its own menus, no clicks.
The game's stdout/stderr go to <out>/<tag>.log. --lldb runs it under lldb in batch mode (bt on a crash; the log has it).
Audio: MBA_OUTPUT_GAIN (port/audio) is set to --gain so the game plays quietly.
Linux (the OrbStack amd64 machine mb64): run under Xvfb with lavapipe; a shot captures the X root window whole
(ImageMagick import — the game is the display's only window). click/key/sweep/scroll are macOS-only; --start
drives the menus without them.
Windows x86_64 (any host with wine on PATH; this Mac): the PE binary runs through wine; cwd game/ is Z:<abs> in the
default prefix (~/.wine). Wine draws the game window on the Mac desktop, so the macOS shot/click/key actions (cgev)
apply to it; the window's owner process is wine (cgev matches by title "mb"). MB_START drives the menus like elsewhere.
Recipes: port/INTEGRATION_STATUS.md, "Test runs".
"""
import argparse, glob, json, os, shutil, signal, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CGEV = os.path.join(ROOT, 'work', 'tools', 'cgev')


def cgev(*a):
    return subprocess.run([CGEV] + [str(x) for x in a], capture_output=True, text=True).stdout.strip()


def crash_report(since, out):
    """The faulting thread of the newest macOS crash report (~/Library/Logs/DiagnosticReports/mb-*.ips) written after since."""
    for _ in range(30):
        reps = [r for r in glob.glob(os.path.expanduser('~/Library/Logs/DiagnosticReports/mb-*.ips')) if os.path.getmtime(r) >= since]
        if reps:
            break
        time.sleep(0.5)
    else:
        return
    txt = open(max(reps, key=os.path.getmtime)).read()
    body = json.loads(txt[txt.index('\n') + 1:])
    imgs, th = body['usedImages'], body['threads'][body['faultingThread']]
    lines = ['%s' % body.get('exception', {}).get('subtype', body.get('exception'))]
    for f in th['frames'][:30]:
        lines.append('  %s %s+%s' % (imgs[f['imageIndex']].get('name'), f.get('symbol', hex(f['imageOffset'])), f.get('symbolLocation', '')))
    st = th.get('threadState', {})
    lines.append('  regs: ' + ' '.join('x%d=%x' % (i, r['value']) for i, r in enumerate(st.get('x', [])) if i < 29))
    print('\n'.join(lines))
    out.write('\n---- crash report ----\n' + '\n'.join(lines) + '\n')


COUNTERS = ('mission_frame', 'load_step')   # their number matches that count or more


class Ordered(argparse.Action):
    """--wait / --on / --do keep their command-line order in one list."""
    def __call__(self, parser, ns, values, option_string=None):
        ns.seq.append((option_string, values))


def state_match(want, line_ev):
    """want: the state words; line_ev: an MBSTATE line's words after the time. A counter's number matches >=."""
    if len(line_ev) < len(want):
        return False
    for k, (w, h) in enumerate(zip(want, line_ev)):
        if w == h:
            continue
        if k == len(want) - 1 and want[0] in COUNTERS and w.isdigit() and h.isdigit() and int(h) >= int(w):
            continue
        return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--time', type=float, default=60)
    ap.add_argument('--out', default=os.path.join(ROOT, 'work', 'port_shots'))
    ap.add_argument('--tag', default='run')
    ap.add_argument('--gain', default='0.15')
    ap.add_argument('--target', default='macos-arm64')
    ap.add_argument('--lldb', action='store_true')
    ap.add_argument('--start', help='MB_START for port/mb_start.cpp')
    ap.add_argument('--state-timeout', type=float, default=120)
    ap.add_argument('--user-dir', help='MB_USER_DIR sandbox (saves, config); a new one is seeded with copies of yours')
    ap.add_argument('--at', nargs='+', action='append', default=[])
    ap.add_argument('--wait', nargs=1, action=Ordered, dest='seq')
    ap.add_argument('--on', nargs='+', action=Ordered, dest='seq')
    ap.add_argument('--do', nargs='+', action=Ordered, dest='seq')
    ap.add_argument('rest', nargs='*')
    ap.set_defaults(seq=[])
    a = ap.parse_args()
    seq = []                                       # (state words or None, action list or None)
    for opt, v in a.seq:
        if opt == '--wait':
            seq.append((v[0].split(), None))
        elif opt == '--on':
            if len(v) < 2:
                ap.error('--on <state> <action...>')
            seq.append((v[0].split(), v[1:]))
        else:
            seq.append((None, v))
    if sys.platform == 'darwin':
        sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
        import screen_lock
        screen_lock.hold('run_native.py ' + a.tag)
    # A stopped run (SIGTERM / SIGHUP: an agent's task stop, a closed terminal) still ends its game before the process
    # exits and the screen lock goes. Python's default SIGTERM exit skips the `finally` below, and the game runs in its
    # own session (start_new_session), so it lived on with the lock free: 2026-11-21 a gate smoke that got the lock at
    # that moment read the orphan's "Finished All" in game/rgl_log.txt as its own (launcher still up: 76.8 DIFFERS), twice.
    def stop(sig, frame):
        raise SystemExit(128 + sig)
    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGHUP, stop)
    if sys.platform == 'darwin':
        src = os.path.join(ROOT, 'tools', 'port', 'cgev.swift')
        if not os.path.exists(CGEV) or os.path.getmtime(CGEV) < os.path.getmtime(src):
            os.makedirs(os.path.dirname(CGEV), exist_ok=True)
            subprocess.run(['swiftc', '-O', '-o', CGEV, src], check=True)
    os.makedirs(a.out, exist_ok=True)
    wine = a.target.startswith('windows')           # cross-built PE binaries: the binary director is Wine
    exe = os.path.join(ROOT, 'build', 'port', a.target, 'bin', 'mb' + ('.exe' if wine else ''))
    env = dict(os.environ, MBA_OUTPUT_GAIN=a.gain)
    if a.start:
        env['MB_START'] = a.start
    if a.user_dir:
        ud = os.path.abspath(a.user_dir)
        if not os.path.isdir(ud):            # port/platform/fs.cpp: Documents -> <dir>/Documents, AppData -> <dir>
            home = os.path.expanduser('~')
            for src, dst in (('Documents/Mount&Blade Savegames', 'Documents/Mount&Blade Savegames'),
                             ('Library/Application Support/Mount&Blade', 'Mount&Blade')):
                if os.path.isdir(os.path.join(home, src)):
                    shutil.copytree(os.path.join(home, src), os.path.join(ud, dst))
            os.makedirs(ud, exist_ok=True)
            print('new sandbox %s (copies of your saves and config)' % ud)
        env['MB_USER_DIR'] = ud
    cmd = [exe] + a.rest
    if wine:
        # Wine turns cwd game/ into Z:\<abs path>; the exe's own argv[0] handling and the game's relative paths work
        # from there. Wine draws the window on the Mac desktop like a native app, so the cgev shots and input (below,
        # macOS branch) apply to it too; WINEDEBUG=-all keeps the log to the game's own output.
        # MB_WINE wins; then PATH; then the known prefixes. NB GCenx .app bundles put a launcher at Contents/MacOS/wine
        # and the real loader at Contents/Resources/wine/bin/wine - the 11.21 tarball's launcher is a compiled AppKit
        # "document opener" that launches each argument separately, dropping the program's argv (the game directory
        # never reaches the exe) and spawning explorer.exe windows for the stray items, so anything with arguments must
        # run the Resources loader (swapped in below). (AGENT=1 / MANPATH=... after the program name in process listings
        # was a pgrep artifact of wine's ps-title rewrite, not program arguments.)
        wcmd = (os.environ.get('MB_WINE') or shutil.which('wine')
                or shutil.which('wine', path='/opt/homebrew/bin:/opt/local/bin:/usr/local/bin')
                or shutil.which('wine', path=os.path.expanduser('~/wine1121/Wine Devel.app/Contents/Resources/wine/bin'))
                or shutil.which('wine', path=os.path.expanduser('~/wine1121/Wine Devel.app/Contents/MacOS')))
        if wcmd is None:
            print('wine not found (MB_WINE, PATH, or known install paths): the %s run gate needs it' % a.target)
            return 3
        d = os.path.dirname(os.path.abspath(wcmd))
        if os.path.basename(d) == 'MacOS':      # a bundle launcher: swap in the real loader it wraps
            real = os.path.join(os.path.dirname(d), 'Resources', 'wine', 'bin', 'wine')
            if os.path.isfile(real):
                wcmd = real
        env['WINEDEBUG'] = 'fixme-all,err-unimpl'          # not -all: an unimplemented call is the first crash clue
        cmd = [wcmd] + cmd
    if a.lldb:
        cmd = ['lldb', '--batch', '-o', 'run', '-k', 'bt 40', '-k', 'register read', '-k', 'quit 1', '--'] + cmd
    logpath = os.path.join(a.out, a.tag + '.log')
    log = open(logpath, 'w')
    p = subprocess.Popen(cmd, cwd=os.path.join(ROOT, 'game'), stdout=log, stderr=subprocess.STDOUT, env=env,
                         start_new_session=True)
    timed = sorted((float(s[0]), s[1:]) for s in a.at)
    t0 = time.time()
    rd = open(logpath, 'rb')
    part = b''
    states = []                                   # MBSTATE lines seen: (game time, words)
    seen = 0                                      # states[:seen] were before the current step's start
    step = 0
    since = t0                                    # when the current ordered step became current
    status = 0

    def read_states():
        nonlocal part
        part += rd.read()
        *lines, part = part.split(b'\n')
        for ln in lines:
            w = ln.decode('utf-8', 'replace').split()
            if len(w) >= 3 and w[0] == 'MBSTATE':
                states.append((float(w[1]), w[2:]))
                if w[2] in ('menu_ready', 'start_done', 'start_failed', 'screen', 'mission_frame', 'first_frame'):
                    if w[2] != 'mission_frame' or w[3] in ('1', '100', '1000'):
                        print('%.1f s: %s (game %s s)' % (time.time() - t0, ' '.join(w[2:]), w[1]), flush=True)

    def act(st):
        """one action; False when the run is over (end)"""
        if st[0] == 'end':
            print('%.1f s: end' % (time.time() - t0))
            return False
        if st[0] == 'sleep':
            end = time.time() + float(st[1])
            while time.time() < end and p.poll() is None:
                time.sleep(0.05)
            return True
        if sys.platform != 'darwin':
            if st[0] == 'shot':
                # Xvfb: the game is the display's only window; capture the root window whole.
                f = os.path.join(a.out, '%s_%s.png' % (a.tag, st[1]))
                subprocess.run(['import', '-window', 'root', f], check=True)
                print('%.1f s: shot %s (X root window)' % (time.time() - t0, f))
            else:
                print('%.1f s: %s: input actions are macOS-only (MB_START drives the menus)' % (time.time() - t0, st[0]))
            return True
        # cgev matches the owner process name, not the title: a native run's windows belong to mb, a Wine run's to
        # wine (its "mb"-owned helpers are 54x54 cursor stubs that would otherwise win nothing but exist).
        w = cgev('win', 'wine' if wine else 'mb').split()
        if len(w) < 5:
            print('%.1f s: no window' % (time.time() - t0))
            return True
        wid, x, y, ww, wh = map(int, w)
        if st[0] == 'shot':
            f = os.path.join(a.out, '%s_%s.png' % (a.tag, st[1]))
            subprocess.run(['screencapture', '-x', '-o', '-l', str(wid), f])
            print('%.1f s: shot %s (window %d %d %d %d)' % (time.time() - t0, f, x, y, ww, wh))
        elif st[0] == 'click':
            cgev('front', p.pid)
            cx, cy = x + float(st[1]) * ww, y + float(st[2]) * wh
            cgev('click', '%.0f' % cx, '%.0f' % cy)
            print('%.1f s: click %.0f %.0f' % (time.time() - t0, cx, cy))
        elif st[0] == 'sweep':       # sweep <fx> <fy> <dx> <n>: n mouse moves from a window point, dx screen points each
            cgev('front', p.pid)
            cgev('sweep', '%.0f' % (x + float(st[1]) * ww), '%.0f' % (y + float(st[2]) * wh), st[3], st[4])
            print('%.1f s: sweep %s' % (time.time() - t0, ' '.join(st[1:])))
        elif st[0] == 'scroll':      # scroll <lines> [n]: mouse wheel (positive: up), n events
            cgev('front', p.pid)
            cgev('scroll', *st[1:3])
            print('%.1f s: scroll %s' % (time.time() - t0, ' '.join(st[1:])))
        elif st[0] == 'key':
            cgev('front', p.pid)
            if len(st) > 2:          # a held key: cgev runs on while the steps after this one continue (shots while it is down)
                subprocess.Popen([CGEV, 'key'] + st[1:3], stdout=subprocess.DEVNULL)
            else:
                cgev('key', st[1])
            print('%.1f s: key %s' % (time.time() - t0, st[1]))
        else:
            print('unknown action %s' % ' '.join(st))
        return True

    try:
        going = True
        deadline = max([a.time] + [t for t, _ in timed])     # (as before: every --at step runs, then on to --time)
        while going and p.poll() is None and time.time() - t0 < deadline:
            read_states()
            while timed and time.time() - t0 >= timed[0][0]:
                going = act(timed.pop(0)[1]) and going
            if step < len(seq):
                want, action = seq[step]
                hit = want is None
                if not hit:
                    for k in range(seen, len(states)):
                        if state_match(want, states[k][1]):
                            hit, seen = True, k + 1
                            print('%.1f s: state %s (game %.1f s)' % (time.time() - t0, ' '.join(states[k][1]), states[k][0]))
                            break
                    if any(s[1][0] == 'start_failed' for s in states[seen:]):
                        print('MB_START failed: %s' % ' '.join(next(s[1] for s in states[seen:] if s[1][0] == 'start_failed')))
                if hit:
                    step += 1
                    since = time.time()
                    if action:
                        going = act(action) and going
                    continue
                if time.time() - since > a.state_timeout:
                    print('%.1f s: no "%s" within %.0f s: giving up' % (time.time() - t0, ' '.join(want), a.state_timeout))
                    status = 3
                    break
            time.sleep(0.05)
        if p.poll() is not None:
            print('exited %s at %.1f s' % (p.returncode, time.time() - t0))
    finally:
        signal.signal(signal.SIGTERM, signal.SIG_IGN)      # a second stop must not cut the kill below short
        signal.signal(signal.SIGHUP, signal.SIG_IGN)
        if p.poll() is None:
            os.killpg(p.pid, signal.SIGTERM)
            try:
                p.wait(5)
            except subprocess.TimeoutExpired:
                os.killpg(p.pid, signal.SIGKILL)
                p.wait()
            print('killed at %.1f s' % (time.time() - t0))
        else:
            print('exit code %s after %.1f s' % (p.returncode, time.time() - t0))
            if p.returncode < 0 and sys.platform == 'darwin':
                crash_report(t0, log)
    log.close()
    rd.close()
    return status


if __name__ == '__main__':
    sys.exit(main())
