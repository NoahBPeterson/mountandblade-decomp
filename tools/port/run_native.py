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
     actions: shot <name> | click <fx> <fy> | key <code> | sleep <sec> | end (kill the game now: the run is over)
     <state> is the event and its leading arguments as one word or quoted: menu_ready, "screen map", "menu start_game_1",
     "mission_frame 100" (mission_frame / load_step: that count or more), start_done. A state counts when it is logged after the step
     before it was done. A wait that does not come within --state-timeout seconds ends the run (exit status 3).
  --user-dir DIR sets MB_USER_DIR (saves and config there, never in yours); a DIR that does not exist is created with
       copies of ~/Documents/Mount&Blade Savegames and ~/Library/Application Support/Mount&Blade.
  --start SPEC sets MB_START (port/mb_start.cpp: quick_battle[:N], new_game, tutorial, load[:N], map[:N], or steps such as
       main:custom,option:start_game_3#1): the game drives its own menus, no clicks.
The game's stdout/stderr go to <out>/<tag>.log. --lldb runs it under lldb in batch mode (bt on a crash; the log has it).
Audio: MBA_OUTPUT_GAIN (port/audio) is set to --gain so the game plays quietly.
Recipes: port/INTEGRATION_STATUS.md, "Test runs".
"""
import argparse, glob, json, os, signal, subprocess, sys, time

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
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
    import screen_lock
    screen_lock.hold('run_native.py ' + a.tag)
    src = os.path.join(ROOT, 'tools', 'port', 'cgev.swift')
    if not os.path.exists(CGEV) or os.path.getmtime(CGEV) < os.path.getmtime(src):
        os.makedirs(os.path.dirname(CGEV), exist_ok=True)
        subprocess.run(['swiftc', '-O', '-o', CGEV, src], check=True)
    os.makedirs(a.out, exist_ok=True)
    exe = os.path.join(ROOT, 'build', 'port', a.target, 'bin', 'mb')
    env = dict(os.environ, MBA_OUTPUT_GAIN=a.gain)
    if a.start:
        env['MB_START'] = a.start
    if a.user_dir:
        ud = os.path.abspath(a.user_dir)
        if not os.path.isdir(ud):            # port/platform/fs.cpp: Documents -> <dir>/Documents, AppData -> <dir>
            import shutil
            home = os.path.expanduser('~')
            for src, dst in (('Documents/Mount&Blade Savegames', 'Documents/Mount&Blade Savegames'),
                             ('Library/Application Support/Mount&Blade', 'Mount&Blade')):
                if os.path.isdir(os.path.join(home, src)):
                    shutil.copytree(os.path.join(home, src), os.path.join(ud, dst))
            os.makedirs(ud, exist_ok=True)
            print('new sandbox %s (copies of your saves and config)' % ud)
        env['MB_USER_DIR'] = ud
    cmd = [exe] + a.rest
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
        w = cgev('win', 'mb').split()
        if len(w) < 5:
            print('%.1f s: no window' % (time.time() - t0))
            return True
        wid, x, y, ww, wh = map(int, w)
        if st[0] == 'shot':
            f = os.path.join(a.out, '%s_%s.png' % (a.tag, st[1]))
            subprocess.run(['screencapture', '-x', '-o', '-l', str(wid), f])
            print('%.1f s: shot %s' % (time.time() - t0, f))
        elif st[0] == 'click':
            cgev('front', p.pid)
            cx, cy = x + float(st[1]) * ww, y + float(st[2]) * wh
            cgev('click', '%.0f' % cx, '%.0f' % cy)
            print('%.1f s: click %.0f %.0f' % (time.time() - t0, cx, cy))
        elif st[0] == 'key':
            cgev('front', p.pid)
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
            if p.returncode < 0:
                crash_report(t0, log)
    log.close()
    rd.close()
    return status


if __name__ == '__main__':
    sys.exit(main())
