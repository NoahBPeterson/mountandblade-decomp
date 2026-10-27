#!/usr/bin/env python3
"""reach_audit.py: list vtable slots the original has and our build does not.

A class whose reconstructed source declares fewer virtuals than the original emits a short vtable. Every slot
past its end is unreachable: the relink cannot patch it, and a call through it reads whatever .rdata follows.
The loss is silent - the functions the missing slots point at simply never run.

FaceGen is the worked example: the original's table at 0x595c68 has nine entries and slot +0x08 is
FaceGen::init (0x4f49d0). Our source declared one virtual, so nothing ever ran init, the face-generator widgets
stayed zero, and the first slider read crashed (0x44a09d, reading 0x3f800000 = 1.0f).

usage: reach_audit.py [--strict] [--top N]
--strict exits 1 when any slot is lost, so the relink can gate on it.
"""
import argparse, json, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ORIG = os.path.join(ROOT, 'work', 'mb.analysis.bin')
RESOLVE = os.path.join(ROOT, 'work', 'relink', 'standalone', 'resolve_map.json')
INDEX = os.path.join(ROOT, 'work', 'index', 'functions.csv')


def image(data):
    e = struct.unpack_from('<I', data, 0x3c)[0]
    n = struct.unpack_from('<H', data, e + 6)[0]
    opt = struct.unpack_from('<H', data, e + 20)[0]
    base = struct.unpack_from('<I', data, e + 24 + 28)[0]
    secs = []
    for i in range(n):
        q = e + 24 + opt + i * 40
        vsize, va, rawsize, raw = struct.unpack_from('<IIII', data, q + 8)
        secs.append((base + va, max(vsize, rawsize), raw))
    return secs


def read_u32(secs, data, va):
    for va0, size, raw in secs:
        if va0 <= va < va0 + size:
            off = raw + va - va0
            if off + 4 <= len(data):
                return struct.unpack_from('<I', data, off)[0]
            return None
    return None


# Tables whose real length the "leading run of function starts" over-measures because the next table's entries follow
# immediately. Measured against the game (the only oracle): 0x5947d8 declared with 8 slots makes the engine call entry 7
# (an unnamed function that faults inside TiXmlBase::StringEqual), 7 slots runs; every 0x5947d8 alias (head_rec,
# head_cam, head_obj2, ItemD_vt, the Rec* records) is therefore seven slots.
VERIFIED_SLOTS = {0x5947D8: 7}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--strict', action='store_true')
    ap.add_argument('--top', type=int, default=30)
    a = ap.parse_args()

    data = open(ORIG, 'rb').read()
    secs = image(data)
    starts = set()
    with open(INDEX) as f:
        next(f)
        for line in f:
            p = line.split(',')
            if p:
                starts.add(int(p[0], 16))

    rm = json.load(open(RESOLVE))
    orig_name = {}
    for name, ent in rm['names'].items():
        for va, meta in ent.items():
            if meta.get('defined') and int(va, 16) not in orig_name:
                orig_name[int(va, 16)] = name
    ours = {pair[0] for pair in rm['defined_at'].values()}

    # A table's leading run of function starts does not stop at its end: the next table in .rdata continues it.
    # Bound every run by the distance to the next known table so the count is the table's own slots.
    # Every vtable start we know: the map's, plus every ??_7 name in symbols/ (a table's run must stop there; without
    # the bound the run walks into the next table - 0x5947d8 measured 9 that way, while entry 7 is a CRT compare, and
    # declaring 9 made the engine call it: crash in TiXmlBase::StringEqual).
    known = {int(e['orig'][0], 16) for e in rm['vtables'].values() if len(e['orig']) == 1}
    import glob
    import re as _re
    for f in glob.glob(os.path.join(ROOT, 'symbols', '**', '*.txt'), recursive=True):
        try:
            for line in open(f, errors='replace'):
                m = _re.match(r'\s*(?:0x)?([0-9a-fA-F]{6,8})\s+\?\?_7', line)
                if m:
                    known.add(int(m.group(1), 16))
        except OSError:
            pass
    known = sorted(known)
    nxt = {}
    for i, va in enumerate(known):
        nxt[va] = known[i + 1] if i + 1 < len(known) else None

    lost = []
    for vt, e in rm['vtables'].items():
        if len(e['orig']) != 1:
            continue
        ova, have = int(e['orig'][0], 16), e['slots']
        limit = 512
        n2 = nxt.get(ova)
        if n2 is not None and (n2 - ova) // 4 < limit:
            limit = max((n2 - ova) // 4, 0)
        k = 0
        while k < limit:
            t = read_u32(secs, data, ova + 4 * k)
            if t is None or t not in starts:
                break
            k += 1
        if ova in VERIFIED_SLOTS:
            k = min(k, VERIFIED_SLOTS[ova])
        if k <= have:
            continue
        missing = []
        for s in range(have, k):
            t = read_u32(secs, data, ova + 4 * s)
            name = orig_name.get(t)
            if name is None:
                missing.append('%d: %08x (unnamed)' % (s, t))
            elif name in ours:
                missing.append('%d: %s' % (s, name))
        if missing:
            lost.append((k - have, vt, '%08x' % ova, have, k, missing))

    lost.sort(reverse=True)
    total = sum(n for n, _, _, _, _, _ in lost)
    print('%d vtable(s) shorter than the original, %d slot(s) unreachable' % (len(lost), total))
    for n, vt, ova, have, k, missing in lost[:a.top]:
        print('  %-42s orig %s  ours %d/%d' % (vt[:42], ova, have, k))
        for m in missing:
            print('      lost slot %s' % m)
    return 1 if (a.strict and lost) else 0


if __name__ == '__main__':
    sys.exit(main())
