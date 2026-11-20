#!/usr/bin/env python3
"""view_layout.py: compare two class definitions of one object (a "view" and the real class) at 32 and 64 bits.

The original's objects are often declared by several classes (party/game.h Game, include/game_data.h GameData,
mission5/game_2.h GameMapView, ...). At 32 bits they agree by construction (MB_LAYOUT32_ASSERT); at 64 bits every pointer,
tstring and vector widens, so a member at the same 32-bit offset can land at different 64-bit offsets in the two classes:
a reader through one class misses what the other wrote. This tool lays both classes out with clang
(-fdump-record-layouts, -fsyntax-only, fast) for the 32-bit target (build/port/linux-i686) and the 64-bit one
(build/port/macos-arm64), flattens the members, and for every named member of B finds the member of A at the same 32-bit
offset and compares their 64-bit offsets.

    view_layout.py dump TU.cpp Record                 the flattened members: off32 off64 path type
    view_layout.py cmp TU_A.cpp:RecA TU_B.cpp:RecB [--all]
        A is the class the object is built as (its constructor's), B a view; prints B's members whose 64-bit offset
        differs from A's member at the same 32-bit offset ("pad" when A has only padding there: A does not know the
        member, so its 64-bit position is undefined). --all prints matches too.
Record is the name as clang prints it after `struct` / `class` (namespaces included: rgl::mesh).
"""
import argparse, json, os, re, shlex, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
TARGETS = {32: 'linux-i686', 64: 'macos-arm64'}
LINE = re.compile(r'^\s*(\d+)(?::[\d-]+)? \| (\s*)(.*)$')


def command(target, tu):
    cc = json.load(open(os.path.join(ROOT, 'build', 'port', target, 'compile_commands.json')))
    tu = os.path.abspath(tu)
    for e in cc:
        if os.path.abspath(os.path.join(e['directory'], e['file'])) == tu:
            args = e.get('arguments') or shlex.split(e['command'])
            break
    else:
        sys.exit('%s: not in %s compile_commands.json' % (tu, target))
    out, skip = [], False
    for a in args:
        if skip:
            skip = False
            continue
        if a in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
            continue
        if a in ('-c', '-g', '-MD', '-MMD') or a.startswith('-O'):
            continue
        out.append(a)
    # zig c++ (the linux targets) cannot -fsyntax-only (it renames an output that is never written): compile at -O0
    # and its cache would skip the compile (no layout dump) on a second run: a nonce define makes every run a miss
    tail = ['-fsyntax-only'] if 'zig' not in out[0] else ['-O0', '-c', '-o', os.devnull, '-DMB_VIEW_LAYOUT_NONCE=%d' % time.time_ns()]
    return out + tail + ['-Xclang', '-fdump-record-layouts-complete'], e['directory']


_cache = {}


def layouts(bits, tu):
    key = (bits, tu)
    if key not in _cache:
        cmd, cwd = command(TARGETS[bits], tu)
        r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
        if r.returncode:
            sys.exit('%d-bit compile of %s failed:\n%s' % (bits, tu, r.stderr[-3000:]))
        _cache[key] = r.stdout
    return _cache[key]


def record(bits, tu, name):
    """[(offset, depth, path, type)] of the members of `name`, flattened (bases and nested records included)."""
    txt = layouts(bits, tu)
    blocks = txt.split('*** Dumping AST Record Layout')
    head = re.compile(r'^\s*0 \| (?:struct|class|union) %s$' % re.escape(name))
    for b in blocks:
        lines = b.strip('\n').split('\n')
        if lines and head.match(lines[0]):
            break
    else:
        sys.exit('%s not laid out in %s (%d-bit)' % (name, tu, bits))
    out, stack = [], []
    for l in lines[1:]:
        m = LINE.match(l)
        if not m:
            continue
        off, ind, rest = int(m.group(1)), len(m.group(2)) // 2, m.group(3)
        base = rest.endswith('(base)') or rest.endswith('(base) (empty)') or rest.endswith('(primary base)')
        mem = re.match(r'(.*?) (\w+)(?: \(empty\))?$', rest)
        typ, nm = (rest, '<base>') if base or not mem else (mem.group(1), mem.group(2))
        if base:
            typ = re.sub(r' \((primary )?base\)( \(empty\))?$', '', rest)
            nm = '<%s>' % typ.split()[-1]
        del stack[ind - 1:]
        stack.append(nm)
        out.append((off, ind, '.'.join(stack), typ))
    if not out:
        sys.exit('%s: empty layout' % name)
    size = re.search(r'sizeof=(\d+)', b)
    return out, int(size.group(1)) if size else None


def flat(tu, name):
    """members with both offsets: [(off32, off64, path, type, leaf)]; the two layouts list the same members in order."""
    a, s32 = record(32, tu, name)
    b, s64 = record(64, tu, name)
    if [x[2] for x in a] != [x[2] for x in b]:
        sys.exit('%s: member lists differ between the 32- and 64-bit layouts (target-dependent members?)' % name)
    res = []
    for i, (x, y) in enumerate(zip(a, b)):
        leaf = i + 1 == len(a) or a[i + 1][1] <= x[1]
        res.append((x[0], y[0], x[2], x[3], leaf))
    return res, s32, s64


PAD = re.compile(r'(^|\.)(pad|unk|_pad|gap|res)[0-9a-fA-F_]*$|(^|\.)pad\w*$')


def cover(mems, o32):
    """the deepest leaf of mems whose 32-bit range holds o32 (ranges end at the next leaf's offset)."""
    leaves = [m for m in mems if m[4]]
    best = None
    for i, m in enumerate(leaves):
        if m[0] <= o32:
            best = (m, leaves[i + 1][0] if i + 1 < len(leaves) else None)
        else:
            break
    return best


def main():
    ap = argparse.ArgumentParser()
    sp = ap.add_subparsers(dest='cmd', required=True)
    d = sp.add_parser('dump')
    d.add_argument('tu')
    d.add_argument('rec')
    c = sp.add_parser('cmp')
    c.add_argument('a')
    c.add_argument('b')
    c.add_argument('--all', action='store_true')
    a = ap.parse_args()
    if a.cmd == 'dump':
        mems, s32, s64 = flat(a.tu, a.rec)
        for o32, o64, p, t, leaf in mems:
            print('%#8x %#8x  %s  %s' % (o32, o64, p, t))
        print('sizeof %#x / %#x' % (s32, s64))
        return
    split = lambda x: re.match(r'(.*?\.(?:cpp|c|cc)):(.*)$', x).groups()       # the record may hold '::'
    (ta, ra), (tb, rb) = split(a.a), split(a.b)
    ma, sa32, sa64 = flat(ta, ra)
    mb, sb32, sb64 = flat(tb, rb)
    bad = 0
    for o32, o64, p, t, leaf in mb:
        if not leaf or PAD.search(p) or p.endswith('>'):
            continue
        cv = cover(ma, o32)
        if not cv:
            continue
        m, _end = cv
        exp = m[1] + (o32 - m[0])
        apad = bool(PAD.search(m[2]))
        tag = 'ok' if exp == o64 and m[0] == o32 else ('pad' if apad else ('MOVED' if exp != o64 else 'inside'))
        if exp == o64 and tag != 'pad' and not a.all:
            continue
        if tag != 'ok':
            bad += 1
        print('%-6s 32:%#7x  B64:%#7x  A64:%#7x  B %s (%s)  <->  A %s' % (tag, o32, o64, exp, p, t, m[2]))
    print('sizeof A %#x/%#x  B %#x/%#x; %d member(s) of B not at A\'s 64-bit position' % (sa32, sa64, sb32, sb64, bad))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
