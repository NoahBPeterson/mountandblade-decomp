#!/usr/bin/env python3
"""tools/port/burndown.py: compile every src/ TU with the PORTABLE configuration (port/CMakeLists.txt, clang) and record
how many compile, the top error kinds and the worst files in docs/port_burndown.json (with history). Tickets 002 / 004.

It configures port/ with CMake (compile_commands.json is the one source of truth for flags), then runs each TU's
compile command itself, in parallel, so one failing TU never stops the count.

    tools/port/burndown.py                       # configure + compile all + append a history entry
    tools/port/burndown.py --no-record           # print only
    tools/port/burndown.py --only src/rgl/psys.cpp --show 40     # one TU, print its first 40 diagnostics
    tools/port/burndown.py --note "windows.h shim"

The compiles run locally (clang via zig, port/cmake/zig-c++); nothing touches the MSVC flow.
"""
import argparse, collections, datetime, json, os, re, shlex, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
TOOLCHAINS = {'linux-i686': 'port/cmake/linux-i686.cmake'}
ERR_RE = re.compile(r'^(?P<file>[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?:fatal )?error: (?P<msg>.*)$', re.M)


def git(*args):
    try:
        return subprocess.run(['git', '-C', ROOT] + list(args), capture_output=True, text=True).stdout.strip()
    except OSError:
        return ''


def configure(target, build):
    cmd = ['cmake', '-S', os.path.join(ROOT, 'port'), '-B', build, '-G', 'Ninja',
           '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_BUILD_TYPE=Release',
           '-DCMAKE_TOOLCHAIN_FILE=' + os.path.join(ROOT, TOOLCHAINS[target])]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit('cmake configure failed:\n' + r.stdout[-3000:] + r.stderr[-3000:])
    with open(os.path.join(build, 'compile_commands.json')) as f:
        return json.load(f)


def kind_of(msg):
    """Normalise one error message into a kind: identifiers and numbers become placeholders."""
    m = re.match(r"'([^']+)' file not found", msg)
    if m:
        return "header not found: " + m.group(1)
    k = re.sub(r"'[^']*'", "'X'", msg)
    k = re.sub(r'\b0x[0-9a-fA-F]+\b|\b\d+\b', 'N', k)
    return k[:110]


def category(msg):
    m = re.match(r"'([^']+)' file not found", msg)
    if m:
        h = m.group(1).lower()
        if h in ('d3d9.h', 'd3dx9.h', 'dinput.h') or h.startswith('d3d'):
            return 'header:d3d'
        if h in ('windows.h', 'tchar.h', 'shlobj.h', 'mmsystem.h', 'winsock2.h', 'windowsx.h', 'shellapi.h', 'richedit.h',
                 'commctrl.h', 'oleauto.h'):
            return 'header:windows'
        if h in ('vector', 'list', 'deque', 'hash_map', 'xmemory', 'algorithm', 'memory'):
            return 'header:stl'
        return 'header:crt'
    if re.search(r'_Myfirst|_Mylast|_Myend|_Insert_n|_Uninit|_Nonscalar|_Scalar_ptr|_Buy\b|_Tidy|_Xlen|_Destroy|_Buynode|_Growmap|_Ptr_cat|_Copy_opt', msg):
        return 'stl-internal'
    if re.search(r'size_chk|size_check|negative size|static_assert|static assertion', msg):
        return 'layout-assert'
    if re.search(r'__asm|__declspec|naked|__thiscall|__stdcall|__fastcall|__cdecl|calling convention|alternatename', msg):
        return 'msvc-syntax'
    return 'other'


def compile_one(entry, error_limit):
    args = entry.get('arguments') or shlex.split(entry['command'])
    args = [a for a in args if not a.startswith('-ferror-limit')] + ['-ferror-limit=%d' % error_limit, '-fno-color-diagnostics']
    t0 = time.time()
    r = subprocess.run(args, cwd=entry['directory'], capture_output=True, text=True)
    errs = [(m.group('file'), int(m.group('line')), m.group('msg')) for m in ERR_RE.finditer(r.stderr)]
    if r.returncode != 0 and not errs:
        errs = [(entry['file'], 0, (r.stderr.strip().splitlines() or ['compiler failed'])[-1][:200])]
    return entry['file'], r.returncode == 0, errs, r.stderr, time.time() - t0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target', default='linux-i686', choices=sorted(TOOLCHAINS))
    ap.add_argument('--jobs', type=int, default=max(2, (os.cpu_count() or 4) // 2))
    ap.add_argument('--error-limit', type=int, default=100)
    ap.add_argument('--only', action='append', help='compile only TUs whose path contains this (repeatable); never records')
    ap.add_argument('--show', type=int, default=0, help='print the first N diagnostics of each failing TU')
    ap.add_argument('--idents', type=int, default=0, help='also print the N most frequent quoted names per error kind')
    ap.add_argument('--no-record', action='store_true')
    ap.add_argument('--note', default='')
    ap.add_argument('--out', default=os.path.join(ROOT, 'docs', 'port_burndown.json'))
    a = ap.parse_args()

    build = os.path.join(ROOT, 'build', 'port', a.target)
    entries = [e for e in configure(a.target, build) if '/src/' in e['file']]
    if a.only:
        entries = [e for e in entries if any(o in e['file'] for o in a.only)]
    t0 = time.time()
    with ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(lambda e: compile_one(e, a.error_limit), entries))
    wall = time.time() - t0

    rel = lambda p: os.path.relpath(p, ROOT)
    ok = sum(1 for r in results if r[1])
    kinds, cats, first_kinds = collections.Counter(), collections.Counter(), collections.Counter()
    idents = collections.defaultdict(collections.Counter)   # kind -> first quoted name -> count
    per_file = {}
    for f, good, errs, stderr, dt in sorted(results):
        per_file[rel(f)] = {'ok': good, 'errors': len(errs), 'first': (rel(errs[0][0]) + ':%d: ' % errs[0][1] + errs[0][2]) if errs else ''}
        for (_, _, msg) in errs:
            kinds[kind_of(msg)] += 1
            cats[category(msg)] += 1
            q = re.search(r"'([^']*)'", msg)
            if q:
                idents[kind_of(msg)][q.group(1)] += 1
        if errs:
            first_kinds[kind_of(errs[0][2])] += 1
        if a.show and not good:
            print('==', rel(f))
            lines = [l for l in stderr.splitlines() if ' error: ' in l]
            print('\n'.join(lines[:a.show]))
    total_errs = sum(v['errors'] for v in per_file.values())
    worst = sorted(((k, v['errors']) for k, v in per_file.items() if not v['ok']), key=lambda kv: -kv[1])[:15]

    print('target %s  TUs %d  compile %d  fail %d  errors %d (capped %d/TU)  %.0f s wall, %d jobs'
          % (a.target, len(results), ok, len(results) - ok, total_errs, a.error_limit, wall, a.jobs))
    print('-- categories:', ', '.join('%s %d' % kv for kv in cats.most_common()))
    print('-- top error kinds (all errors):')
    for k, v in kinds.most_common(20):
        print('%6d  %s' % (v, k))
    print('-- top FIRST error kinds (what blocks each TU):')
    for k, v in first_kinds.most_common(12):
        print('%6d  %s' % (v, k))
    if a.idents:
        for k, _ in kinds.most_common(8):
            if idents[k]:
                print('-- names in "%s":' % k[:60], ', '.join('%s %d' % kv for kv in idents[k].most_common(a.idents)))
    print('-- worst files:')
    for k, v in worst:
        print('%6d  %s' % (v, k))

    if a.no_record or a.only:
        return
    try:
        with open(a.out) as f:
            doc = json.load(f)
    except (OSError, ValueError):
        doc = {'about': 'Ticket 004/002 burn-down: src/ TUs compiled with the portable configuration (port/, clang). '
                        'Written by tools/port/burndown.py; history is append-only.', 'history': []}
    doc['history'].append({
        'date': datetime.datetime.now().strftime('%Y-%m-%d %H:%M'), 'commit': git('rev-parse', '--short', 'HEAD'),
        'dirty': bool(git('status', '--porcelain', '--', 'port', 'src', 'tools/port')), 'target': a.target,
        'compiler': subprocess.run(['zig', 'cc', '--version'], capture_output=True, text=True).stdout.splitlines()[0]
                    if a.target == 'linux-i686' else '',
        'tus': len(results), 'compile': ok, 'fail': len(results) - ok, 'errors': total_errs, 'error_limit': a.error_limit,
        'categories': dict(cats.most_common()), 'top_kinds': kinds.most_common(12), 'top_first_kinds': first_kinds.most_common(8),
        'worst': worst[:10], 'note': a.note})
    doc['latest'] = {'target': a.target, 'files': per_file}
    with open(a.out, 'w') as f:
        json.dump(doc, f, indent=1)
        f.write('\n')
    print('recorded ->', rel(a.out))


if __name__ == '__main__':
    main()
