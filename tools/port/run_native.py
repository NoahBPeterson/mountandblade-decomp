#!/usr/bin/env python3
"""run_native.py: run the native port (build/port/<target>/bin/mb) on game/ for a bounded time, with a timed script of
screenshots and input, then kill it. Every run ends with the game gone (SIGTERM, then SIGKILL).

usage: run_native.py [--time 60] [--out work/port_shots] [--tag run] [--gain 0.15] [--lldb] [--] [game args...]
       steps (any number, in time order):  --at <sec> shot <name> | --at <sec> click <fx> <fy> | --at <sec> key <kVK code>
       (click coordinates are fractions of the window's content: 0.5 0.5 is the centre)
The game's stdout/stderr go to <out>/<tag>.log. --lldb runs it under lldb in batch mode (bt on a crash; the log has it).
Audio: MBA_OUTPUT_GAIN (port/audio) is set to --gain so the game plays quietly.
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--time', type=float, default=60)
    ap.add_argument('--out', default=os.path.join(ROOT, 'work', 'port_shots'))
    ap.add_argument('--tag', default='run')
    ap.add_argument('--gain', default='0.15')
    ap.add_argument('--target', default='macos-arm64')
    ap.add_argument('--lldb', action='store_true')
    ap.add_argument('--at', nargs='+', action='append', default=[])
    ap.add_argument('rest', nargs='*')
    a = ap.parse_args()
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
    cmd = [exe] + a.rest
    if a.lldb:
        cmd = ['lldb', '--batch', '-o', 'run', '-k', 'bt 40', '-k', 'register read', '-k', 'quit 1', '--'] + cmd
    log = open(os.path.join(a.out, a.tag + '.log'), 'w')
    p = subprocess.Popen(cmd, cwd=os.path.join(ROOT, 'game'), stdout=log, stderr=subprocess.STDOUT, env=env,
                         start_new_session=True)
    steps = sorted((float(s[0]), s[1:]) for s in a.at)
    t0 = time.time()
    try:
        for t, st in steps:
            while time.time() - t0 < t:
                if p.poll() is not None:
                    break
                time.sleep(0.2)
            if p.poll() is not None:
                print('exited %s at %.1f s' % (p.returncode, time.time() - t0))
                break
            w = cgev('win', 'mb').split()
            if len(w) < 5:
                print('%.1f s: no window' % (time.time() - t0))
                continue
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
        while p.poll() is None and time.time() - t0 < a.time:
            time.sleep(0.2)
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


if __name__ == '__main__':
    main()
