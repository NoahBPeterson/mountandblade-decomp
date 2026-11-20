#!/usr/bin/env python3
"""layout32_msvc.py: does MSVC (cl 13.10, the original's compiler) lay out every src/ record as the i686 clang pass
of tools/port/port64_ast.py does?

    .venv/bin/python tools/port/layout32_msvc.py [--filter SUBSTR] [--jobs N]   -> work/port64/layout32_msvc.json

tools/port/port64_worklist.py judges every 64-bit site against the 32-bit layouts clang gives for i686 with
-malign-double (MSVC's rules, port/CMakeLists.txt). This checks that assumption against cl itself: for each TU it
appends, to a copy of the source, one compile-time assertion per record defined in the TU's own files (each record
once, in the first TU that has it) and per named member:

    typedef char l32_<k>[sizeof(Rec) == 0x508 ? 1 : -1];
    typedef char l32_<k>[(size_t)&reinterpret_cast<const volatile char&>(((Rec*)0)->member) == 0x58 ? 1 : -1];

and compiles it with cl /Zs (tools/matching/cl.sh: the laptop's compile server when it is up). The values are the
i686 pass's, so a failing assertion (C2118) is a record whose MSVC layout differs from clang's 32-bit one: the
worklist's 32-bit judgement of that record is then wrong (category layout32). Asserts on members the compiler
cannot name from namespace scope (private, nested in a function, template parameters) are counted as unchecked.
These are MB_LAYOUT32_ASSERT-style checks of every member, generated: nothing is written to src/ (the copies go
next to the source as _l32chk_<name>.cpp and are removed). Not per commit: run it after layout-changing merges.
"""
import argparse, glob, json, multiprocessing, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'difftest'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'matching'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
sys.path.insert(0, HERE)
import heavy, port64_ast

NAMEABLE = re.compile(r'^[A-Za-z_][\w:]*(<[\w:, *<>]*>)?$')


def checks_for(tu, d32, seen):
    """[(expr, expected, label)] for the records of one TU (records of headers once)."""
    out = []
    recs = {}
    for r in d32['records']:
        recs.setdefault(r['key'], r)
    for r in d32['records']:
        key = r['key']
        loc = (key, r['file'], r['line'])
        if loc in seen or key.startswith('anon@') or not NAMEABLE.match(key) or not r['file'].startswith('src/'):
            continue
        seen.add(loc)
        out.append(('sizeof(%s)' % key, r['size'], '%s sizeof' % key, r['file'], r['line']))

        def members(rr, base, depth=0):
            for f in rr['fields']:
                if f.get('bit') is not None or f['size'] == 0:
                    continue
                if not f['name']:
                    sub = recs.get(f.get('rec') or '')
                    if sub and depth < 4:
                        members(sub, base + f['off'], depth + 1)
                    continue
                if '&' in f['type']:
                    continue
                out.append(('(size_t)&reinterpret_cast<const volatile char&>(((%s*)0)->%s)' % (key, f['name']),
                            base + f['off'], '%s::%s' % (key, f['name']), r['file'], f['line']))
        members(r, 0)
    return out


def check_one(job):
    src, flags, entries = job
    os.chdir(ROOT)
    import slice as S
    if not entries:
        return []
    flags = S.resolve_include_flags(flags)
    tmp = os.path.join(os.path.dirname(src), '_l32chk_%s' % os.path.basename(src))
    base = open(src, errors='replace').read()
    nbase = base.count('\n') + 1
    lines = ['typedef char l32_%d_[(%s) == %#x ? 1 : -1];' % (k, e[0], e[1]) for k, e in enumerate(entries)]
    try:
        open(tmp, 'w').write(base + '\n#include <stddef.h>\n' + '\n'.join(lines) + '\n')
        r = subprocess.run([S.CL, '/nologo', '/c', '/Zs', *flags, S.INCLUDE, S.winpath(tmp)], capture_output=True,
                           text=True)
        out = r.stdout + r.stderr
        bad, unknown = set(), set()
        for m in re.finditer(r'_l32chk_[^(]*\((\d+)\)\s*:\s*error (C\d+)', out):
            k = int(m[1]) - nbase - 2
            if 0 <= k < len(entries):
                (bad if m[2] == 'C2118' else unknown).add(k)
        if not re.search(r'_l32chk_', out) and r.returncode not in (0,):
            # the copy did not compile for a reason of its own (before our lines)
            return [(e, None, 'skip') for e in entries]
        bad -= unknown
        actual = {}
        if bad:
            order = sorted(bad)
            probes = ['char (*l32p_%d_)[(%s) + 1] = 1;' % (j, entries[k][0]) for j, k in enumerate(order)]
            open(tmp, 'w').write(base + '\n#include <stddef.h>\n' + '\n'.join(probes) + '\n')
            r = subprocess.run([S.CL, '/nologo', '/c', '/Zs', *flags, S.INCLUDE, S.winpath(tmp)], capture_output=True,
                               text=True)
            for m in re.finditer(r'_l32chk_[^(]*\((\d+)\)\s*:\s*error C\d+:.*\(\*\)\[(\d+)\]', r.stdout + r.stderr):
                j = int(m[1]) - nbase - 2
                if 0 <= j < len(order):
                    actual[order[j]] = int(m[2]) - 1
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)
    return [(e, actual.get(k), 'bad' if k in bad else 'unchecked' if k in unknown else 'ok')
            for k, e in enumerate(entries)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--filter')
    ap.add_argument('--jobs', type=int, default=8)
    a = ap.parse_args()
    os.chdir(ROOT)
    import compile_all as C
    files = port64_ast.run(ROOT, 2, quiet=True)
    seen, jobs = set(), []
    for src, flags in C.sources():
        tu = os.path.relpath(src, ROOT)
        if not tu.startswith('src/') or tu not in files or (a.filter and a.filter not in tu):
            continue
        try:
            d32 = json.load(open(files[tu][32]))
        except (OSError, ValueError):
            continue
        jobs.append((src, flags, checks_for(tu, d32, seen)))
    with multiprocessing.Pool(heavy.pool_jobs(a.jobs)) as pool:
        res = [r for rs in pool.map(check_one, jobs, chunksize=1) for r in rs]
    st = {}
    for e, act, s in res:
        st[s] = st.get(s, 0) + 1
    bad = [dict(what=e[2], expr=e[0], clang=e[1], msvc=act, file=e[3], line=e[4]) for e, act, s in res if s == 'bad']
    out = os.path.join(ROOT, 'work', 'port64', 'layout32_msvc.json')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    json.dump(dict(stats=st, bad=bad), open(out, 'w'), indent=1)
    for b in bad:
        print('%s:%d  %s: i686 clang %#x, MSVC %s' % (b['file'], b['line'], b['what'], b['clang'],
                                                      '%#x' % b['msvc'] if b['msvc'] is not None else '?'))
    print('layout32_msvc: %s -> %s' % (', '.join('%s %d' % kv for kv in sorted(st.items())), out))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
