#!/usr/bin/env python3
"""tools/port/burndown.py: compile every src/ TU with the PORTABLE configuration (port/CMakeLists.txt, clang) and record
how many compile, the top error kinds and the worst files in docs/port_burndown.json (with history). Tickets 002 / 004.

It configures port/ with CMake (compile_commands.json is the one source of truth for flags), then runs each TU's
compile command itself, in parallel, so one failing TU never stops the count.

    tools/port/burndown.py                       # configure + compile all + append a history entry
    tools/port/burndown.py --no-record           # print only
    tools/port/burndown.py --only src/engine/render/particle_system.cpp --show 40     # one TU, print its first 40 diagnostics
    tools/port/burndown.py --note "windows.h shim"
    tools/port/burndown.py --target linux-x86_64    # the 64-bit count (ticket 006's baseline)

Every run also takes a link inventory of the objects that compiled (llvm-nm, no linker run): strong symbols defined in
more than one TU, and the undefined externals no object defines, grouped by who has to provide them (platform shims,
Newton, FMOD, the C/C++ runtime, port runtime, or src/ itself): the raw inventory of src/ alone.

Then it links the `mb` executable (cmake --build --target mb, at most 4 jobs) on linux-i686 and records whether it
linked, the linker's undefined / duplicate symbol counts, and from port/link/tables.json (tools/port/link_tables.py)
how the src/ names were resolved and how many are still only trapped (weak placeholders that abort when reached).

The compiles run where the target's toolchain lives: zig cross (port/cmake/zig-c++) for linux-i686 from the Mac,
native clang for the macOS targets, and linux-x86_64 inside the OrbStack amd64 machine mb64 (run this script there).
Nothing touches the MSVC flow.
"""
import argparse, collections, datetime, glob, json, os, re, shlex, shutil, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
TOOLCHAINS = {'linux-i686': 'port/cmake/linux-i686.cmake', 'linux-x86_64': 'port/cmake/linux-x86_64-native.cmake',
              'macos-arm64': 'port/cmake/macos-arm64.cmake', 'macos-x86_64': 'port/cmake/macos-x86_64.cmake'}
ZIG_LIBC = {'linux-i686': 'x86-linux-gnu', 'linux-x86_64': 'x86_64-linux-gnu'}


def uses_zig(target):
    """zig cross (linux-i686) vs native clang (the macOS targets; linux-x86_64 inside the OrbStack amd64 machine,
    where this script then runs too — the Mac home is mounted at identical paths)."""
    with open(os.path.join(ROOT, TOOLCHAINS[target])) as f:
        return 'zig-cc' in f.read()


def compiler_version(target, build):
    if uses_zig(target):
        return subprocess.run(['zig', 'cc', '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    m = re.search(r'^CMAKE_CXX_COMPILER:FILEPATH=(.+)$', open(os.path.join(build, 'CMakeCache.txt')).read(), re.M)
    cxx = m.group(1) if m else None
    if not cxx:   # a toolchain set() without CACHE keeps the compiler out of the cache; the compile commands have it
        with open(os.path.join(build, 'compile_commands.json')) as f:
            entries = json.load(f)
        cxx = shlex.split(entries[0]['command'])[0] if entries else None
    return subprocess.run([cxx, '--version'], capture_output=True, text=True).stdout.splitlines()[0] if cxx else ''


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


def object_of(entry):
    args = entry.get('arguments') or shlex.split(entry['command'])
    return os.path.join(entry['directory'], args[args.index('-o') + 1]) if '-o' in args else None


def header_tokens(dirs):
    toks = set()
    for d in dirs:
        for root, _, files in os.walk(d):
            for f in files:
                with open(os.path.join(root, f), errors='ignore') as fh:
                    toks.update(re.findall(r'\b[A-Za-z_]\w*\b', fh.read()))
    return toks


def link_inventory(target, objs):
    """Duplicate strong definitions and unresolved externals across the objects, by provider."""
    nm = shutil.which('llvm-nm') or '/opt/homebrew/opt/llvm/bin/llvm-nm'
    filt = shutil.which('llvm-cxxfilt') or '/opt/homebrew/opt/llvm/bin/llvm-cxxfilt'
    if not os.path.exists(nm) or not objs:
        return None
    out = subprocess.run([nm, '-A', '-P', '-g'] + objs, capture_output=True, text=True).stdout
    strong, defined, undef = collections.defaultdict(set), set(), set()
    for line in out.splitlines():
        m = re.match(r'^(.*?\.o): (\S+) (\w)', line)
        if not m:
            continue
        obj, name, t = m.groups()
        if t == 'U':
            undef.add(name)
        else:
            defined.add(name)
            if t in 'TDBR':
                strong[name].add(obj)
    dups = sorted(n for n, o in strong.items() if len(o) > 1)
    unres = sorted(undef - defined)
    dem = dict(zip(unres, subprocess.run([filt], input='\n'.join(unres), capture_output=True, text=True).stdout.splitlines()))
    zig_lib = re.search(r'\.lib_dir = "([^"]+)"', subprocess.run(['zig', 'env'], capture_output=True, text=True).stdout) if uses_zig(target) else None
    libc_dirs = []
    if zig_lib:
        inc = os.path.join(zig_lib.group(1), 'libc', 'include')
        libc_dirs = [os.path.join(inc, d) for d in ('generic-glibc', ZIG_LIBC[target], 'any-linux-any')]
    elif target.startswith('macos'):
        sdk = subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True, text=True).stdout.strip()
        if sdk:
            libc_dirs = [os.path.join(sdk, 'usr', 'include')]
    elif target.startswith('linux'):
        libc_dirs = ['/usr/include']
    libc, shims = header_tokens(libc_dirs), header_tokens([os.path.join(ROOT, 'port', 'include')])
    groups = collections.defaultdict(list)
    for n in unres:
        d = dem.get(n, n)
        if n.startswith('_Z'):
            if re.search(r'__cxxabiv1|typeinfo for std::|std::(exception|bad_alloc|nothrow|terminate)|operator (new|delete)', d):
                g = 'c++ runtime'
            elif 'std::' in d:
                g = 'src: STL instances defined only in the MSVC link'
            else:
                g = 'src: declared, defined in no TU'
        elif n.startswith('Newton'):
            g = 'Newton (ticket 011)'
        elif n.startswith(('FSOUND', 'FMUSIC')):
            g = 'FMOD (ticket 010)'
        elif n.startswith('mb_') or n == '_beginthread':
            g = 'port runtime'
        elif n.startswith('__') or n in ('_GLOBAL_OFFSET_TABLE_', '_Unwind_Resume', 'bcmp'):
            g = 'compiler runtime'
        elif n in shims:
            g = 'platform shims (Win32/D3D9/D3DX/DirectInput/COM/Winsock: 007-009)'
        elif n in libc:
            g = 'libc/libm'
        else:
            g = 'other'
        groups[g].append(d)
    return {'objects': len(objs), 'duplicate_strong': len(dups), 'duplicates_sample': dups[:12],
            'unresolved': len(unres), 'by_provider': {k: len(v) for k, v in sorted(groups.items(), key=lambda kv: -len(kv[1]))},
            'src_unresolved': sorted(groups.get('src: declared, defined in no TU', []) +
                                     groups.get('src: STL instances defined only in the MSVC link', []))}


def compile_one(entry, error_limit):
    args = entry.get('arguments') or shlex.split(entry['command'])
    args = [a for a in args if not a.startswith('-ferror-limit')] + ['-ferror-limit=%d' % error_limit, '-fno-color-diagnostics']
    t0 = time.time()
    r = subprocess.run(args, cwd=entry['directory'], capture_output=True, text=True)
    errs = [(m.group('file'), int(m.group('line')), m.group('msg')) for m in ERR_RE.finditer(r.stderr)]
    if r.returncode != 0 and not errs:
        errs = [(entry['file'], 0, (r.stderr.strip().splitlines() or ['compiler failed'])[-1][:200])]
    return entry['file'], r.returncode == 0, errs, r.stderr, time.time() - t0


def link_exe(target, build, jobs):
    """Link `mb` through CMake (it also builds the port TUs and the weakened copies). linux-i686 only: the link tables
    (port/link) describe the 32-bit objects."""
    if target == 'linux-i686':                   # compile check only: the native executable needs 64-bit SDL3 / Jolt
        return {'attempted': False}
    t0 = time.time()
    r = subprocess.run(['cmake', '--build', build, '--target', 'mb_native', '-j', str(min(jobs, 4))], capture_output=True, text=True)
    log = r.stdout + r.stderr
    exe = os.path.join(build, 'bin', 'mb')
    undef = sorted(set(re.findall(r'undefined symbol: (.+)', log)))
    dup = sorted(set(re.findall(r'duplicate symbol: (.+)', log)))
    res = {'attempted': True, 'links': r.returncode == 0 and os.path.exists(exe), 'undefined': len(undef),
           'duplicates': len(dup), 'other_errors': len(re.findall(r'error: ', log)) - len(undef) - len(dup),
           'seconds': round(time.time() - t0, 1), 'undefined_sample': undef[:10], 'duplicates_sample': dup[:10]}
    if res['links']:
        res['exe_bytes'] = os.path.getsize(exe)
    try:
        tdir = os.path.join(ROOT, 'port', 'link', target)
        tables = json.load(open(os.path.join(tdir if os.path.isdir(tdir) else os.path.join(ROOT, 'port', 'link'), 'tables.json')))
        res['tables'] = {k: (len(v) if isinstance(v, list) else v) for k, v in tables.items()
                         if k not in ('_note', 'owner_basis')}
        res['trapped'] = tables.get('trapped', [])
    except (OSError, ValueError):
        pass
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target', default='linux-i686', choices=sorted(TOOLCHAINS))
    ap.add_argument('--jobs', type=int, default=min(4, max(2, (os.cpu_count() or 4) // 2)),
                    help='parallel compiles (default and the link step: at most 4, the 16 GB machine rule)')
    ap.add_argument('--no-link', action='store_true', help='skip linking mb')
    ap.add_argument('--error-limit', type=int, default=100)
    ap.add_argument('--only', action='append', help='compile only TUs whose path contains this (repeatable); never records')
    ap.add_argument('--show', type=int, default=0, help='print the first N diagnostics of each failing TU')
    ap.add_argument('--idents', type=int, default=0, help='also print the N most frequent quoted names per error kind')
    ap.add_argument('--no-record', action='store_true')
    ap.add_argument('--note', default='')
    ap.add_argument('--out', default=os.path.join(ROOT, 'docs', 'port_burndown.json'))
    a = ap.parse_args()

    build = os.path.join(ROOT, 'build', 'port', a.target)
    entries = [e for e in configure(a.target, build) if e['file'].startswith(os.path.join(ROOT, 'src') + os.sep)]
    if a.only:
        entries = [e for e in entries if any(o in e['file'] for o in a.only)]
    t0 = time.time()
    with ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(lambda e: compile_one(e, a.error_limit), entries))
    wall = time.time() - t0
    objs = [o for (e, r) in zip(entries, results) if r[1] for o in [object_of(e)] if o and os.path.exists(o)]
    link = link_inventory(a.target, objs) if not a.only else None
    exe = link_exe(a.target, build, a.jobs) if not (a.only or a.no_link) else None

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
    if link:
        print('-- link inventory (%d objects): %d strong symbols defined in more than one TU, %d unresolved externals'
              % (link['objects'], link['duplicate_strong'], link['unresolved']))
        for k, v in link['by_provider'].items():
            print('%6d  %s' % (v, k))
    if exe and exe.get('attempted'):
        print('-- link of mb: %s (%.0f s): %d undefined, %d duplicate symbols%s'
              % ('LINKS' if exe['links'] else 'FAILS', exe['seconds'], exe['undefined'], exe['duplicates'],
                 ', %d other errors' % exe['other_errors'] if exe['other_errors'] > 0 else ''))
        for s in exe['undefined_sample'] + exe['duplicates_sample']:
            print('        ', s[:150])
        t = exe.get('tables')
        if t:
            print('   port/link: %d duplicates owned (%d TUs weakened); src/ names: %d aliased (%d ABI-mismatched), '
                  '%d STL instances, %d emitted by -fno-inline TUs, %d trapped'
                  % (t['duplicates'], t['weakened_tus'], t['aliased'], t['aliases_abi_mismatch'], t['stl_instantiated'],
                     t['emitted_inline'], t['trapped']))

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
        'compiler': compiler_version(a.target, build),
        'tus': len(results), 'compile': ok, 'fail': len(results) - ok, 'errors': total_errs, 'error_limit': a.error_limit,
        'categories': dict(cats.most_common()), 'top_kinds': kinds.most_common(12), 'top_first_kinds': first_kinds.most_common(8),
        'worst': worst[:10], 'link': {k: v for k, v in (link or {}).items() if k != 'src_unresolved'},
        'exe': {k: v for k, v in (exe or {}).items() if k != 'trapped'}, 'note': a.note})
    latest = doc.get('latest', {})
    if 'files' in latest:                       # the first format kept one target only
        latest = {latest.get('target', 'linux-i686'): {'files': latest['files']}}
    latest[a.target] = {'files': per_file, 'src_unresolved': (link or {}).get('src_unresolved', []),
                        'trapped': (exe or {}).get('trapped', [])}
    doc['latest'] = latest
    with open(a.out, 'w') as f:
        json.dump(doc, f, indent=1)
        f.write('\n')
    print('recorded ->', rel(a.out))


if __name__ == '__main__':
    main()
