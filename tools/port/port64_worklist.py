#!/usr/bin/env python3
"""port64_worklist.py: the ranked list of every place in src/ whose types are wrong at 64 bits (or lie about the
real type) -> docs/port64_worklist.md + docs/port64_worklist.tsv.

    .venv/bin/python tools/port/port64_worklist.py            # regenerate both files (~1 min cold, ~15 s cached)
    tools/port/port64_worklist.py --root WORKTREE --out DIR   # another checkout (validation at old commits)
    tools/port/port64_worklist.py --check NAME ...            # print the findings whose class / text matches NAME

Sources, all mechanical:
  1. tools/port/port64_ast.py: one clang pass per src/ TU at 64 bits (macos-arm64 flags) and one at 32 bits (i686,
     MSVC record rules). It gives every record's layout at both widths and the sites: raw byte offsets off a record
     pointer (`(char*)p + 0x58`, the MI / FLD / GD / WD_FLD macros, `char*` aliases, `at<T>(off)` helpers), records
     reinterpreted as other records, member addresses reinterpreted (`(SoaBlock*)&t->note_times`,
     `*(T**)&x->pad[4]`), scalar views indexed (`((int*)p)[5]`), pointer <-> integer casts with the member /
     parameter / local holding the integer, constant-size allocations and mem* calls, calls through computed
     function pointers; and the clang warnings of the 64-bit pass (-Wpointer-to-int-cast, -Wint-to-pointer-cast,
     -Wshorten-64-to-32, -Wcast-align, -Wcast-function-type-mismatch, -Wformat ...), plus the 32-bit pass's
     errors (a failing MB_LAYOUT32_ASSERT).
  2. tools/port/find_views.py (text): the view records and the real type each one overlays.
Every site is judged against the layouts: a raw offset is "wrong" when the byte it names at 32 bits is somewhere
else at 64 bits; a view is "wrong" when one of its members lands on a different member of the real record at 64
bits; and so on (see judge_*). Findings are grouped by the real class they are about, ranked: classes with
64-bit-wrong findings first, then by finding count.
"""
import argparse, collections, glob, json, os, re, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import port64_ast

# the keyword is followed by nothing, a plural s, an offset / index or an `_...` tail: `resolved`, `result`, `rawptr` are no pads;
# nor is `padding`, a real attribute (Font::padding, the font file's "padding"), nor `rest_period` / `rest_*` (GameData's resting members)
PAD = re.compile(r'^(?!padding$|rest_)(?:_*(pad|unk|unknown|gap|raw|res|rsv|reserved|filler|spare|tail|rest|unused|junk|dummy|skip|blob|'
                 r'bytes|hole|opaque|space)(?:s|_\w*|[0-9a-f]+\w*)?$|_*[a-z]?pad\w*$)', re.I)
OFFNAME = re.compile(r'^(f|v|w|m|u|p|i|b|n|c|d|s|lvl|x|fld|field|off|o|r)_?[0-9a-f]{1,6}$', re.I)
SLOTNAME = re.compile(r'^(v|s|slot|vf|vfn|fn|f|m)_?(0x)?[0-9a-f]{1,3}$', re.I)
COMMENT_OFF = re.compile(r'(?://|/\*)\s*(?:@\s*)?\+\s*(0x[0-9a-fA-F]+)\b|(?://|/\*)\s*(0x[0-9a-fA-F]{1,5})\b(?!\s*[:(])')

# category -> (level, description). Level W: wrong at 64 bits; V: a view of another object with pointers (find_views
# says 64-bit wrong; fold it); L: likely / unproven; C: the 32-bit layout disagrees with the original's offsets;
# I: correct at 64 bits but the type lies (idiom debt).
CATS = collections.OrderedDict([
    ('int_ptr', ('W', 'a pointer squeezed through a 32-bit integer (member / parameter / local / cast)')),
    ('raw_moved', ('W', 'raw byte offset / scalar index whose bytes are elsewhere at 64 bits')),
    ('view_moved', ('W', 'record reinterpreted as another whose members land elsewhere at 64 bits')),
    ('vslot', ('W', 'vtable view: MSVC slot numbers that are different Itanium slots, or a call through a computed slot')),
    ('alloc32', ('W', 'constant allocation / mem* size = the 32-bit sizeof (smaller than the 64-bit one)')),
    ('sret', ('W', 'function-pointer cast between a by-value record return and a hidden result pointer (arm64 x8)')),
    ('odr64', ('W', 'same-named records whose 64-bit layout or by-value ABI differs between TUs')),
    ('view64', ('V', 'view record (find_views) over a real type with pointers: fold into the real class')),
    ('raw_pad', ('L', 'raw offset into a pad / an unknown offset / a record not known to the tool')),
    ('lp64_long', ('W', 'long / unsigned long (8 bytes on macOS, 4 in MSVC) in arithmetic or a cast to a pointer / 32-bit int: wraps and overflows differently')),
    ('long_decl', ('I', 'a long / unsigned long member (8 bytes at LP64): plain storage')),
    ('raw_untyped', ('L', 'raw byte offset read through a char* / int the code never typed as a class: the record behind it is unknown here')),
    ('view_raw', ('L', 'record reached from raw memory (void* / int / char*): real type unknown here')),
    ('vtable_view', ('L', 'record of slot-numbered virtuals (v04, s00): a vtable view')),
    ('pad_ptr', ('L', 'real class with a pad array before pointer-holding members (021)')),
    ('fncast', ('L', 'call through a function-pointer type cast (struct returns break on arm64)')),
    ('format', ('L', 'printf-style format that does not fit the 64-bit argument')),
    ('layout32', ('C', 'offset comment / MB_LAYOUT32_ASSERT disagrees with the 32-bit layout')),
    ('raw_ok', ('I', 'raw offset / scalar index that happens to be right at 64 bits')),
    ('view_ok', ('I', 'record reinterpreted as a layout-compatible one (a view that holds at 64 bits)')),
    ('shorten', ('I', '-Wshorten-64-to-32 (size_t / ptrdiff_t into int; benign unless it is a pointer)')),
    ('cast_align', ('I', '-Wcast-align (char* to a more-aligned type)')),
])
BUILTIN = {'void', 'int', 'unsigned int', 'float', 'double', 'bool', 'char', 'unsigned char', 'short', 'long',
           'unsigned long', 'long long', 'unsigned long long', 'unsigned short', 'signed char', 'DWORD', 'HRESULT', 'BOOL',
           'UINT', 'LONG', '__int64', 'unsigned __int64'}
LEVEL_ORDER = {'W': 0, 'V': 1, 'L': 2, 'C': 3, 'I': 4}


# ------------------------------------------------------------------------------------------------ layouts

class Layouts:
    """Record layouts at 32 and 64 bits; per TU so same-named records of different TUs stay apart."""

    def __init__(self):
        self.defs = {}            # (key, file, line, bits) -> record
        self.tu_keys = {}         # tu -> {bits: {key: (file, line)}}
        self.by_key = collections.defaultdict(set)   # key -> {(file, line)}
        self.abi = collections.defaultdict(lambda: collections.defaultdict(list))   # key -> sig -> [(tu, file, line)]

    def add(self, tu, bits, records):
        m = self.tu_keys.setdefault(tu, {}).setdefault(bits, {})
        for r in records:
            k = (r['key'], r['file'], r['line'], bits)
            if k not in self.defs:
                self.defs[k] = r
            if bits == 64 and r['file'].startswith('src/') and (r['fields'] or r.get('bases')):
                sig = (r['size'], tuple((x['name'], x['off']) for x in r['fields']), r.get('regs'))
                self.abi[r['key']][sig].append((tu, r['file'], r['line']))
            m.setdefault(r['key'], (r['file'], r['line']))
            self.by_key[r['key']].add((r['file'], r['line']))

    def get(self, key, tu, bits):
        loc = self.tu_keys.get(tu, {}).get(bits, {}).get(key)
        if loc is None:
            locs = self.by_key.get(key)
            if not locs:
                return None
            loc = sorted(locs)[0]
        r = self.defs.get((key, loc[0], loc[1], bits))
        return self.get_full(key, tu, bits, r) if self.full else r

    full = False

    def get_full(self, key, tu, bits, r=None):
        """get(), but a member-less method holder (`struct FireAgent { void ray(); }` in one TU) yields the same-named
        record that has members in another TU (the one the holder's out-of-line body reads through): its layout is
        what the cast's target really is."""
        if r is None:
            self.full, f0 = False, self.full
            try:
                r = self.get(key, tu, bits)
            finally:
                self.full = f0
        if r is not None and not r['fields'] and not r.get('bases') and not r.get('ownvptr'):
            for loc in sorted(self.by_key.get(key, ())):
                o = self.defs.get((key, loc[0], loc[1], bits))
                if o is not None and (o['fields'] or o.get('bases')):
                    return o
        return r

    def pair(self, key, tu):
        return self.get(key, tu, 32), self.get(key, tu, 64)


def is_pad(f):
    return bool(PAD.match(f['name'])) and (f.get('n') or f['cat'] == 'I')


class Leaf:
    __slots__ = ('pos64', 'path', 'cat', 'pad', 'type', 'start32', 'size32', 'exact', 'offname', 'rest32')

    def __init__(self, **k):
        for s in self.__slots__:
            setattr(self, s, k.get(s))


def field_pairs(r32, r64):
    """[(f32, f64)] matched by name (anonymous members by order among the anonymous)."""
    out = []
    an64 = [f for f in r64['fields'] if not f['name']]
    by = {f['name']: f for f in r64['fields'] if f['name']}
    ai = 0
    for f in r32['fields']:
        if f['name']:
            g = by.get(f['name'])
        else:
            g = an64[ai] if ai < len(an64) else None
            ai += 1
        if g is not None:
            out.append((f, g))
    return out


def map_off(L, key, tu, n, depth=0, path=''):
    """Where the byte at 32-bit offset n of record key is at 64 bits: a Leaf, or None if the record is unknown."""
    r32, r64 = L.pair(key, tu)
    if r32 is None or r64 is None or depth > 30:
        return None
    if n < 0 or n >= max(r32['size'], 1):
        return Leaf(pos64=None, path=path + '<past the end %#x>' % r32['size'], cat='?', pad=False, type='',
                    start32=n, size32=0, exact=False)
    # vptr
    if r32.get('ownvptr') and n < 4:
        return Leaf(pos64=0 if n == 0 else None, path=path + '<vptr>', cat='P', pad=False, type='vptr', start32=0,
                    size32=4, exact=n == 0)
    b64 = {b['key']: b for b in r64.get('bases', [])}
    for b in r32.get('bases', []):
        if b['size'] and b['off'] <= n < b['off'] + b['size'] and b['key'] in b64:
            sub = map_off(L, b['key'], tu, n - b['off'], depth + 1, path + b['key'] + '::')
            if sub is not None and sub.pos64 is not None:
                sub.pos64 += b64[b['key']]['off']
            return sub
    cands = []
    for f, g in field_pairs(r32, r64):
        sz = f['size'] or 0
        if f['off'] <= n < f['off'] + max(sz, 1):
            cands.append((f, g))
    if not cands:
        # a hole (alignment / tail padding): position relative to the last member before it
        prev = [(f, g) for f, g in field_pairs(r32, r64) if f['off'] + (f['size'] or 0) <= n]
        if prev:
            f, g = max(prev, key=lambda x: x[0]['off'])
            e32, e64 = f['off'] + f['size'], g['off'] + g['size']
            return Leaf(pos64=e64 + (n - e32), path=path + '<gap after %s>' % f['name'], cat='I', pad=True,
                        type='', start32=e32, size32=0, exact=False)
        return Leaf(pos64=n, path=path + '<gap>', cat='I', pad=True, type='', start32=0, size32=0, exact=False)
    # prefer a named, non-pad member starting at n
    cands.sort(key=lambda fg: (fg[0]['off'] != n, is_pad(fg[0]), not fg[0]['name']))
    f, g = cands[0]
    name = f['name'] or '(anon)'
    rel = n - f['off']
    cnt = f.get('n')
    if f.get('rec') and not cnt:
        sub = map_off(L, f['rec'], tu, rel, depth + 1, path + name + '.')
        if sub is None:
            return Leaf(pos64=g['off'] if rel == 0 else (g['off'] + rel if not f.get('hasptr') else None),
                        path=path + name, cat='R', pad=False, type=f['type'], start32=f['off'], size32=f['size'],
                        exact=rel == 0)
        if sub.pos64 is not None:
            sub.pos64 += g['off']
        return sub
    if cnt:
        e32, e64 = f['esz'], g.get('esz', f['esz'])
        idx, r = divmod(rel, e32) if e32 else (0, rel)
        if f.get('rec'):
            sub = map_off(L, f['rec'], tu, r, depth + 1, path + '%s[%d].' % (name, idx))
            if sub is not None and sub.pos64 is not None:
                sub.pos64 += g['off'] + idx * e64
            return sub
        pos = g['off'] + idx * e64 + (r if e32 == e64 or r == 0 else 0)
        if e32 != e64 and r:
            pos = None
        return Leaf(pos64=pos, path=path + '%s[%d]' % (name, idx) + ('+%d' % r if r else ''), cat=f['cat'],
                    pad=is_pad(f), type=f['type'], start32=f['off'] + idx * e32, size32=e32, exact=r == 0,
                    offname=bool(OFFNAME.match(f['name'])), rest32=f['off'] + f['size'] - n)
    pos = g['off'] + rel if (rel == 0 or f['size'] == g['size']) else None
    return Leaf(pos64=pos, path=path + name + ('+%d' % rel if rel else ''), cat=f['cat'], pad=is_pad(f),
                type=f['type'], start32=f['off'], size32=f['size'], exact=rel == 0,
                offname=bool(OFFNAME.match(f['name'])), rest32=f['off'] + f['size'] - n)


def leaves(L, key, tu, depth=0, base32=0, base64=0, path='', out=None, limit=400):
    """The named scalar / pointer members of a record, flattened: [(off32, off64, path, cat, size32, pad)]."""
    if out is None:
        out = []
    r32, r64 = L.pair(key, tu)
    if r32 is None or r64 is None or depth > 12 or len(out) > limit:
        return out
    if r32.get('ownvptr'):
        out.append((base32, base64, path + '<vptr>', 'P', 4, False))
    b64 = {b['key']: b for b in r64.get('bases', [])}
    for b in r32.get('bases', []):
        if b['key'] in b64:
            leaves(L, b['key'], tu, depth + 1, base32 + b['off'], base64 + b64[b['key']]['off'], path + b['key'] + '::',
                   out, limit)
    for f, g in field_pairs(r32, r64):
        name = f['name'] or '(anon)'
        if f.get('rec') and not f.get('n'):
            n0 = len(out)
            leaves(L, f['rec'], tu, depth + 1, base32 + f['off'], base64 + g['off'], path + name + '.', out, limit)
            if len(out) == n0:
                out.append((base32 + f['off'], base64 + g['off'], path + name, 'R', f['size'], False))
        elif f.get('rec') and f.get('n'):
            k = min(f['n'], 4)
            for i in range(k):
                leaves(L, f['rec'], tu, depth + 1, base32 + f['off'] + i * f['esz'], base64 + g['off'] + i * g['esz'],
                       path + '%s[%d].' % (name, i), out, limit)
        else:
            out.append((base32 + f['off'], base64 + g['off'], path + name, f['cat'], f['size'], is_pad(f)))
    return out


# ------------------------------------------------------------------------------------------------ judging

def lv(cat):
    return CATS[cat][0]


def empty(r):
    return r is not None and not r['fields'] and not r.get('bases') and not r.get('ownvptr')


def judge_raw(L, s, tu, alias=None):
    """rawoff / scalar_index -> (category, detail, suggestion, class)."""
    key = s['rec']
    r32, r64 = L.pair(key, tu)
    if empty(r32):
        # a member-less method holder (`struct c49c2c0 { void f(); }` called on a cast pointer): judge against the
        # record the code casts to it, when there is one
        real = (alias or {}).get(key)
        if not real:
            return 'raw_pad', 'offset %#x into %s, a record without members (the real object is not known here)' % (
                s.get('off') or 0, key), 'type `this` as the real class'
        s = dict(s, rec=real)
        cat, det, sug = judge_raw(L, s, tu)
        return cat, det + ' (through %s)' % key, sug
    if s['kind'] == 'scalar_index':
        if s.get('idx') is None:
            return 'raw_pad', 'variable index into %s as %s[]' % (key, s['elem']), 'name the member(s) of %s' % key
        narrow = s['ecat'] == 'P' or (re.search(r'\blong\b', s['elem']) and 'long long' not in s['elem'])
        n32 = s['idx'] * (4 if narrow else s['esz'])
        n64 = s['idx'] * s['esz']
        acc_ptr = s['ecat'] == 'P'
    else:
        if s.get('sizeof'):
            if not s.get('const'):
                return 'raw_ok', 'offset built from sizeof / offsetof (%s)' % key, 'index a typed array of %s' % key
        if s.get('strides'):
            k = s['strides'][0]
            if r32 and r64 and not s.get('sizeof'):
                if k == r32['size'] and r64['size'] != k:
                    return ('raw_moved', 'stride %#x = sizeof32(%s); 64-bit sizeof %#x' % (k, key, r64['size']),
                            'index %s[] (sizeof)' % key)
            # element stride of an inner array: judge the constant part against an element we cannot name here
            if not s.get('const'):
                if k in (r32 or {}).get('_strides', ()):
                    pass
                return 'raw_pad', 'computed offset (stride %#x + %#x) into %s' % (k, s.get('off') or 0, key), \
                    'typed array member of %s' % key
        if not s.get('const') and not s.get('strides'):
            return 'raw_pad', 'non-constant offset into %s' % key, 'name the member of %s' % key
        n32 = n64 = s.get('off') or 0
        acc = s.get('access') or ''
        acc_ptr = acc.count('*') >= 2 or acc.endswith('*&') or (acc.endswith('*') and acc[:-1].strip().endswith('*'))
        acc_ptr = acc_ptr or acc in ('void **',)
    if r32 is None or r64 is None:
        return 'raw_pad', 'offset %#x into %s (layout unknown to the tool)' % (n32, key), 'type the pointer'
    lf = map_off(L, key, tu, n32)
    if lf is None:
        return 'raw_pad', 'offset %#x into %s (layout unknown)' % (n32, key), 'type the pointer'
    what = '%s+%#x = %s' % (key, n32, lf.path)
    if lf.path.startswith('<past the end'):
        return 'raw_pad', '%s+%#x: past the end of %s (%#x bytes): the object is a larger (derived?) class' % (
            key, n32, key, r32['size']), 'type the pointer as the real class'

    sug = ('%s::%s' % (key, lf.path)) if lf.exact and not lf.pad and not lf.offname else \
        'name the member of %s at +%#x%s' % (key, n32, (' (%s)' % (s.get('access') or s.get('elem') or '')).rstrip())
    if lf.pos64 is None:
        return 'raw_moved', '%s: inside a member that widens at 64 bits' % what, sug
    if lf.pos64 != n64:
        return 'raw_moved', '%s: at +%#x at 64 bits, the code reads +%#x' % (what, lf.pos64, n64), sug
    if acc_ptr and lf.cat != 'P':
        room = lf.rest32 if lf.rest32 is not None else 0
        if lf.pad and room >= 8:
            return 'raw_pad', '%s: a pointer read from a pad (8 bytes at 64 bits)' % what, sug
        return 'raw_moved', '%s: a pointer read from a %d-byte %s' % (what, lf.size32 or 4, lf.type or 'slot'), sug
    if lf.pad:
        return 'raw_pad', '%s: inside a pad (same position at 64 bits)' % what, sug
    return 'raw_ok', '%s (same at 64 bits)' % what, sug


def compare_view(L, ukey, rkey, tu, base32=0):
    L.full = True
    try:
        return _compare_view(L, ukey, rkey, tu, base32)
    finally:
        L.full = False


def _compare_view(L, ukey, rkey, tu, base32=0):
    """Members of view ukey whose 64-bit offset differs from what is at the same 32-bit offset of rkey (+base32)."""
    r32 = L.get_full(rkey, tu, 32)
    u32 = L.get_full(ukey, tu, 32)
    if r32 is None or u32 is None:
        return None
    if empty(r32):
        return None
    if not base32 and u32['size'] > r32['size'] and u32['fields'] and u32['fields'][0]['off'] == 0 and \
            u32['fields'][0].get('rec') == rkey:
        return []      # container_of: the pointer is the first member of the larger record
    b = map_off(L, rkey, tu, base32) if base32 else Leaf(pos64=0)
    if b is None or b.pos64 is None:
        return [('<base>', base32, None, None)]
    bad = []
    for o32, o64, path, cat, sz, pad in leaves(L, ukey, tu):
        if pad:
            continue
        lf = map_off(L, rkey, tu, base32 + o32)
        if lf is None or lf.path.startswith('<past the end'):
            continue
        p = None if lf.pos64 is None else lf.pos64 - b.pos64
        if p != o64:
            bad.append((path, o32, o64, p, lf.path))
        elif cat == 'P' and lf.cat not in ('P', 'R', '?') and not lf.pad:
            bad.append((path, o32, o64, p, lf.path + ' (not a pointer)'))
    return bad


VIEWNAME = re.compile(r'_s[0-9a-f]{5,6}$|View\b|Vt$|Vtbl$|_vt$|^Rec[0-9a-fx]+$|^[A-Z][0-9a-f]{2,3}$|^c[0-9a-f]{6}::|'
                      r'^[A-Z]\w*[A-Z]$|_pub$|Shim$|_f[0-9a-f]+$')


def viewish(L, key, tu, views):
    """How much a record looks like a decompilation view (higher = more): find_views' verdict, its name, its pads."""
    r = L.get(key, tu, 32)
    v = 4 if key in views else 0
    v += 2 if VIEWNAME.search(key.split('<')[0]) else 0
    if r is not None:
        v += min(3, sum(1 for f in r['fields'] if is_pad(f)))
        v -= 2 if r.get('ownvptr') else 0
    return v


def slots_differ(L, ukey, rkey, tu):
    u, r = L.get(ukey, tu, 64), L.get(rkey, tu, 64)
    if not u or not r or not u.get('slots') or not r.get('slots'):
        return None
    rm = {s[0]: s for s in r['slots']}
    out = []
    for k, ita, name, dtor in u['slots']:
        if k in rm and bool(rm[k][3]) != bool(dtor):
            out.append('%s() slot %d is %s in the view, %s in %s (MSVC: one destructor slot, Itanium: two; every virtual '
                       'after it shifts)' % (name, k, 'a destructor' if dtor else 'not a destructor',
                                              'a destructor' if rm[k][3] else rm[k][2] + '()', rkey))
        elif k in rm and rm[k][1] != ita:
            out.append('%s() slot %d -> Itanium %d, %s has %s() there (Itanium %d)' % (name, k, ita, rkey, rm[k][2],
                                                                                       rm[k][1]))
    return out


def ptr_view_slots(L, ukey, rkey, tu):
    """A view's pointer member to a polymorphic record laid over a real member pointing at another polymorphic record
    (`EmitTarget* target` over Party::model): the slot problems of calling the view's virtuals on the real object."""
    u, r = L.get_full(ukey, tu, 32), L.get_full(rkey, tu, 32)
    if not u or not r:
        return []
    out = []
    for f in u['fields']:
        if not f.get('prec'):
            continue
        for g in r['fields']:
            if g['off'] == f['off'] and g.get('prec') and g['prec'] != f['prec']:
                sd = slots_differ(L, f['prec'], g['prec'], tu)
                if sd:
                    out.append((f['name'], f['prec'], g['name'], g['prec'], sd[0]))
    return out


# ------------------------------------------------------------------------------------------------ main

def src_line(root, f, line, cache={}):
    p = os.path.join(root, f)
    if p not in cache:
        try:
            cache[p] = open(p, errors='replace').read().split('\n')
        except OSError:
            cache[p] = []
    ls = cache[p]
    return ls[line - 1] if 0 < line <= len(ls) else ''


def short(t, n=110):
    t = ' '.join((t or '').split())
    return t if len(t) <= n else t[:n - 3] + '...'


def build(root, jobs, views_json=None, quiet=False):
    t0 = time.time()
    files = port64_ast.run(root, jobs, quiet=quiet)
    L = Layouts()
    sites, diags32, diags64 = {}, {}, {}
    for tu, d in files.items():
        for bits in (64, 32):
            try:
                j = json.load(open(d[bits]))
            except (OSError, ValueError):
                sys.stderr.write('port64_worklist: no %d-bit pass for %s\n' % (bits, tu))
                continue
            L.add(tu, bits, j['records'])
            for x in j['diags']:
                (diags64 if bits == 64 else diags32).setdefault((x['file'], x['line'], x['col'], x['flag']), (tu, x))
            if bits == 64:
                for s in j['sites']:
                    sites.setdefault((s['file'], s['line'], s['col'], s['kind'], s.get('text'), s.get('to'), s.get('rec')), (tu, s))
    t1 = time.time()
    F = []   # findings: dict(cls, cat, file, line, func, detail, sug, text)

    def add(cls, cat, f, line, func, detail, sug, text='', view=''):
        F.append(dict(cls=cls or '?', cat=cat, file=f, line=line, func=func or '', detail=detail, sug=sug,
                      text=short(text), view=view))

    views = set()
    vj = None
    if views_json:
        try:
            vj = json.load(open(views_json))
        except (OSError, ValueError):
            vj = None
    for v in (vj or {}).get('records', []):
        if v.get('view'):
            views.add(v['qual'])
    alias = collections.defaultdict(collections.Counter)
    for (_f, _l, _c, kind, *_t), (tu, s) in sites.items():
        if kind == 'recast' and s.get('from'):
            t32, f32 = L.get(s['to'], tu, 32), L.get(s['from'], tu, 32)
            if empty(t32) and f32 is not None and not empty(f32):
                alias[s['to']][s['from']] += 1
    alias = {k: c.most_common(1)[0][0] for k, c in alias.items()}
    p2i_lines = set()
    poly = {}
    for (f, line, col, kind, *_t), (tu, s) in sorted(sites.items(), key=lambda kv: kv[0][:4] + (str(kv[0][4:]),)):
        k = s['kind']
        if k in ('rawoff', 'scalar_index'):
            cat, det, sug = judge_raw(L, s, tu, alias)
            add(alias.get(s['rec'], s['rec']) if empty(L.get(s['rec'], tu, 32)) else s['rec'], cat, f, line, s['func'], det + (' [%s]' % s['macro'] if s.get('macro') else ''), sug,
                s.get('text'))
        elif k in ('long_arith', 'long_cast') and re.match(
                r'(LOWORD|HIWORD|LOBYTE|HIBYTE|MAKE\w*|\w*HRESULT\w*|S_\w+|E_\w+|D3D\w+|FAILED|SUCCEEDED|\w*ToPtr|PtrTo\w+|'
                r'\w*ToLong|_\w+|[A-Z][A-Z0-9_]{2,})$', s.get('macro') or ''):
            pass       # a Win32 / D3D macro of the platform shim: the shim's own width, not the code's
        elif k == 'long_arith':
            add('fn ' + (s['func'] or '?'), 'lp64_long', f, line, s['func'], '%s arithmetic (%s) on a %s' % (
                s['op'], s['text'][:60], s['type']), 'use the original\'s 32-bit type (int32_t / uint32_t)', s.get('text'))
        elif k == 'long_cast' and s.get('widen'):
            add('fn ' + (s['func'] or '?'), 'long_decl', f, line, s['func'], 'cast %s -> %s (widening)' % (s['from'], s['to']),
                'a fixed-width type', s.get('text'))
        elif k == 'long_cast':
            add('fn ' + (s['func'] or '?'), 'lp64_long', f, line, s['func'], 'cast %s -> %s (%s)' % (
                s['from'], s['to'], 'a pointer' if s.get('ptr') else 'a 32-bit int'),
                'use the original\'s 32-bit type / intptr_t', s.get('text'))
        elif k == 'long_decl':
            add(s['text'].split('::')[0], 'long_decl', f, line, s['func'], 'member %s is a %s' % (s['text'], s['type']),
                'a fixed-width type', s.get('text'))
        elif k == 'rawoff_untyped':
            h = s.get('holder')
            if h == 'field':
                src = '%s::%s (%s)' % (s['hrec'], s['hname'], s['htype'])
            elif h:
                src = '%s %s (%s)' % (h, s['hname'], s.get('htype', ''))
            else:
                src = 'an untyped pointer'
            add('fn ' + (s['func'] or '?'), 'raw_untyped', f, line, s['func'], '%s + %#x read as %s: the record behind it '
                'is not named' % (src, s['off'], s['access']), 'type the pointer as the real class and read the member',
                s.get('text'))
        elif k in ('p2i', 'i2p'):
            if s['width'] >= 64:
                continue
            if s.get('enum'):
                add('fn ' + s['func'], 'raw_ok', f, line, s['func'], 'an enum value stored in a %s (fits)' % s['to'],
                    'keep (or intptr_t)', s.get('text'))
                continue
            p2i_lines.add((f, line))
            h = s.get('holder')
            ptr_t = s['from'] if k == 'p2i' else s['to']
            if h == 'field':
                cls, det = s['hrec'], '%s::%s (%s) holds a %s' % (s['hrec'], s['hname'], s['htype'], ptr_t)
                sug = 'make %s::%s a %s' % (s['hrec'], s['hname'], ptr_t)
            elif h in ('param', 'local', 'global', 'return'):
                fn = s['hname'].split('#')[0]
                cls = s.get('fromrec') or ('fn ' + (s['func'] or fn))
                det = '%s %s (%s) holds a %s' % (h, s['hname'], s.get('htype', ''), ptr_t)
                sug = 'make it a %s (or intptr_t)' % ptr_t
            else:
                cls = s.get('fromrec') or ('fn ' + s['func'] if s['func'] else '?')
                det = '(%s) of a %s' % (s['to'], s['from'])
                sug = 'keep the pointer (%s) / intptr_t' % ptr_t
            add(cls, 'int_ptr', f, line, s['func'], det, sug, s.get('text'))
        elif k == 'fieldpun':
            hr, hn, ht = s['hrec'], s['hname'], s['htype']
            r32, r64 = L.pair(hr, tu)
            to = s['to']
            if s['tcat'] == 'P':
                # a pointer read out of the member's bytes: wrong when the member (element) is not a pointer
                fld = None
                if r32:
                    fld = next((x for x in r32['fields'] if x['name'] == hn), None)
                if fld and (fld['cat'] == 'P' or (fld.get('esz') or fld['size']) >= 8):
                    cat = 'raw_ok'      # a pointer, or 8-byte words: holds a pointer at 64 bits too
                elif fld and is_pad(fld) and (fld['size'] - (s.get('idx') or 0) * s.get('esz', 1)) >= 8:
                    cat = 'raw_pad'
                else:
                    cat = 'int_ptr'
                det = '%s::%s (%s) read as %s' % (hr, hn, ht, to)
                sug = 'make %s::%s a %s' % (hr, hn, to)
            else:
                cat = 'raw_ok' if s.get('tsize', 0) <= 4 or 'long long' in ht or 'double' in ht else 'raw_pad'
                det = '%s::%s (%s) read as %s' % (hr, hn, ht, to)
                sug = 'name / type the member %s::%s' % (hr, hn)
            add(hr, cat, f, line, s['func'], det, sug, s.get('text'))
        elif k in ('recast', 'fieldcast'):
            to = s['to']
            if k == 'fieldcast':
                rk, base = s['hrec'], s.get('hoff')
                r32 = L.get(rk, tu, 32)
                fld = next((x for x in (r32 or {}).get('fields', []) if x['name'] == s['hname']), None)
                base32 = fld['off'] if fld else None
                if fld and s.get('idx') and fld.get('esz'):
                    base32 += s['idx'] * fld['esz']
                if base32 is None:
                    add(rk, 'raw_pad', f, line, s['func'], '%s viewed over %s::%s' % (to, rk, s['hname']),
                        'make %s::%s a %s' % (rk, s['hname'], to), s.get('text'), view=to)
                    continue
                bad = compare_view(L, to, rk, tu, base32)
                what = '%s over %s::%s (+%#x)' % (to, rk, s['hname'], base32)
                sug = 'make %s::%s a %s (or name its members)' % (rk, s['hname'], to)
            else:
                rk = s.get('from')
                if not rk and s.get('fromtype'):
                    # the source's class is incomplete in this TU (a forward declaration): it is defined in another
                    ft = re.sub(r'\b(const|volatile|struct|class)\b', '', s['fromtype']).replace('*', '').strip()
                    if ft not in BUILTIN and ft != to and not empty(L.get(ft, tu, 32)) and L.get(ft, tu, 32) is not None:
                        rk = ft
                if rk and viewish(L, rk, tu, views) > viewish(L, to, tu, views):
                    rk, to = to, rk       # the source is the view: judge it over the real target
                if not rk:
                    h = s.get('holder')
                    det = '%s from %s%s' % (to, s.get('fromtype', '?'), (' (%s %s)' % (h, s.get('hname'))) if h else '')
                    add(to, 'view_raw', f, line, s['func'], det, 'type the source as %s (or the real class)' % to,
                        s.get('text'), view=to)
                    continue
                bad = compare_view(L, to, rk, tu, 0)
                what = '%s over %s' % (to, rk)
                sug = 'use %s\'s members (fold %s into %s)' % (rk, to, rk)
                sd = None if s.get('qcall') else slots_differ(L, to, rk, tu)   # a qualified call names the function: no slot
                if sd:
                    add(rk, 'vslot', f, line, s['func'], '%s: %s' % (what, sd[0]), 'call %s\'s own virtual' % rk,
                        s.get('text'), view=to)
                    continue
            for fn_, fp_, gn_, gp_, why in ptr_view_slots(L, to, rk, tu) + ptr_view_slots(L, rk, to, tu):
                add(rk, 'vslot', f, line, s['func'], '%s over %s: pointer %s (%s*) is %s (%s*): %s' % (
                    to, rk, fn_, fp_, gn_, gp_, why), 'call %s\'s own virtual' % gp_, s.get('text'), view=fp_)
            if s.get('passive') and bad:
                add(rk, 'view_ok', f, line, s['func'], what + ' (the pointer only goes into a std container: not dereferenced as %s)' % to, sug,
                    s.get('text'), view=to)
                continue
            if bad is None:
                add(rk, 'view_raw', f, line, s['func'], what + ' (layout unknown)', sug, s.get('text'), view=to)
            elif bad:
                p, o32, o64, pr, rp = bad[0]
                det = '%s: %d member(s) disagree at 64 bits, e.g. %s::%s (+%#x at 32) is at +%s, %s\'s %s at +%s' % (
                    what, len(bad), to, p, o32, '%#x' % o64 if o64 is not None else '?', rk, rp,
                    '%#x' % pr if pr is not None else '? (inside a widened member)')
                add(rk, 'view_moved', f, line, s['func'], det, sug, s.get('text'), view=to)
            else:
                add(rk, 'view_ok', f, line, s['func'], what + ' (agrees at 64 bits)', sug, s.get('text'), view=to)
        elif k in ('alloc', 'memsize'):
            rk = s.get('rec')
            n = s['size']
            r32, r64 = L.pair(rk, tu) if rk else (None, None)
            if not rk or r64 is None:
                if k == 'alloc':
                    add(rk or ('fn ' + s['func']), 'raw_pad', f, line, s['func'],
                        '%s(%#x) of an untyped block' % (s['fn'], n), 'new T / sizeof(T)', s.get('text'))
                continue
            if k == 'alloc' and n < r64['size']:
                add(rk, 'alloc32', f, line, s['func'], '%s(%#x) for %s: sizeof %#x at 32 bits, %#x at 64' % (
                    s['fn'], n, rk, r32['size'] if r32 else 0, r64['size']), 'new %s / sizeof(%s)' % (rk, rk),
                    s.get('text'))
            elif r32 and n == r32['size'] and n != r64['size']:
                add(rk, 'alloc32', f, line, s['func'], '%s(%#x) = sizeof32(%s); 64-bit sizeof %#x' % (
                    s['fn'], n, rk, r64['size']), 'sizeof(%s)' % rk, s.get('text'))
            else:
                add(rk, 'raw_ok', f, line, s['func'], '%s(%#x) for %s (fits at 64 bits)' % (s['fn'], n, rk),
                    'sizeof(%s)' % rk, s.get('text'))
        elif k == 'vslot':
            add('fn ' + s['func'], 'vslot', f, line, s['func'], 'call through a computed function pointer (slot %s)' % (
                s.get('slot'),), 'call the real class\'s virtual', s.get('text'))

    # warnings of the 64-bit pass
    wmap = {'pointer-to-int-cast': 'int_ptr', 'int-to-pointer-cast': 'int_ptr', 'void-pointer-to-int-cast': 'int_ptr',
            'int-to-void-pointer-cast': 'int_ptr', 'shorten-64-to-32': 'shorten', 'cast-align': 'cast_align',
            'cast-function-type-mismatch': 'fncast', 'format': 'format', 'format-extra-args': 'format',
            'microsoft-cast': 'int_ptr', 'sizeof-pointer-memaccess': 'alloc32'}
    for (f, line, col, flag), (tu, x) in sorted(diags64.items()):
        cat = wmap.get(x['flag'])
        if not cat or not f.startswith('src/'):
            continue
        if cat == 'int_ptr' and (f, line) in p2i_lines:
            continue
        if cat == 'format' and not re.search(r'pointer|long|\*', x['msg']):
            continue
        if cat == 'fncast':
            m2 = re.search(r"cast from '(.+?)\s*\(\*\)\(.*?\)' to '(.+?)\s*\(\*\)\(", x['msg'])
            byval = lambda t: not t.rstrip().endswith('*') and t.strip() not in BUILTIN
            if m2 and byval(m2.group(1)) != byval(m2.group(2)):
                cat = 'sret'
        m = re.search(r"'([\w:<>]+)\s*\*'", x['msg'])
        cls = m.group(1) if m and cat in ('cast_align',) else 'file ' + f
        add(cls, cat, f, line, '', '-W%s: %s' % (x['flag'], x['msg']) + (' [%s]' % x['macro'] if x['macro'] else ''),
            '', src_line(root, f, line))
    for (f, line, col, flag), (tu, x) in sorted(diags32.items()):
        if x['level'] == 'error' and f.startswith('src/'):
            add('file ' + f, 'layout32', f, line, '', '32-bit pass: ' + x['msg'], 'fix the layout (32-bit build)',
                src_line(root, f, line))

    # record-level checks (each definition once)
    seen = set()
    by_key64 = collections.defaultdict(dict)
    for (key, file, line, bits), r in L.defs.items():
        if not file.startswith('src/'):
            continue
        if bits == 64:
            by_key64[key][(file, line)] = r
        if bits != 32 or (key, file, line) in seen:
            continue
        seen.add((key, file, line))
        r64 = L.defs.get((key, file, line, 64))
        if r64 is None:
            continue
        # offset comments vs the 32-bit layout (layout_check.py's rule, here for i686 / MSVC record rules)
        deltas = []
        first_on_line = {}
        for fd in r['fields']:
            if fd['line'] not in first_on_line or fd['off'] < first_on_line[fd['line']]['off']:
                first_on_line[fd['line']] = fd
        for fd in first_on_line.values():
            txt = src_line(root, file, fd['line'])
            m = COMMENT_OFF.search(txt)
            if not m or not fd['name']:
                continue
            v = int(m.group(1) or m.group(2), 16)
            if not m.group(1) and (v > r['size'] + 0x10 or v >= 0x400000):
                continue
            deltas.append((v - fd['off'], fd, v))
        if deltas:
            cnt = collections.Counter(d for d, _f, _v in deltas)
            d0 = 0 if cnt.get(0, 0) * 2 >= len(deltas) else cnt.most_common(1)[0][0]
            for d, fd, v in deltas:
                if d != d0:
                    add(key, 'layout32', file, fd['line'], '', '%s::%s comment +%#x, laid out at +%#x (32-bit)%s' % (
                        key, fd['name'], v, fd['off'], ' (most comments are +%#x off)' % d0 if d0 else ''),
                        'fix the member before it / the comment', src_line(root, file, fd['line']))
        # pad before pointer-holding members; vptr stand-in; vtable views
        fl = r['fields']
        pads = [x for x in fl if is_pad(x)]
        if pads and not key.startswith('anon@'):
            first = min(p['off'] for p in pads)
            ptr_after = [x for x in fl if x['off'] > first and (x['cat'] == 'P' or x.get('hasptr'))]
            if fl and fl[0]['off'] == 0 and is_pad(fl[0]) and fl[0]['size'] == 4 and not r.get('poly'):
                add(key, 'pad_ptr', file, fl[0]['line'], '', '%s starts with %s[4]: a vptr stand-in?' % (key, fl[0]['name']),
                    'derive from the polymorphic base (8-byte vptr at 64 bits)', src_line(root, file, fl[0]['line']))
            elif ptr_after:
                add(key, 'pad_ptr', file, line, '', '%s: %d pad(s), %d pointer-holding member(s) after the first pad '
                    '(%s)' % (key, len(pads), len(ptr_after), ptr_after[0]['name']), 'name the pad\'s members',
                    src_line(root, file, line))
        sl = r64.get('slots') or []
        named = [s for s in sl if SLOTNAME.match(s[2])]
        if len(named) >= 3:
            add(key, 'vtable_view', file, line, '', '%s: %d slot-numbered virtuals (%s ...)' % (
                key, len(named), ', '.join(s[2] for s in named[:3])), 'call the real class\'s virtuals',
                src_line(root, file, line))
    # same-named records whose 64-bit layout or by-value ABI differs between TUs (another definition, or the same
    # one under different #ifdefs: hnode's copy constructor only for cl made it trivial in one TU, not in another)
    for key, sigs in L.abi.items():
        if len(sigs) < 2 or key.startswith('anon@'):
            continue
        lay = {(sz, fl) for sz, fl, _r in sigs}
        ex = [v[0] for v in sigs.values()]
        if len(lay) > 1:
            det = '%s laid out %d ways at 64 bits (%s)' % (key, len(lay), ', '.join(
                '%s:%d size %#x' % (f, l, sz) for (sz, _fl, _r), (_t, f, l) in list(zip(sigs, ex))[:4]))
        else:
            det = '%s passed by value %s (%s)' % (key, 'in registers in some TUs and by address in others', ', '.join(
                '%s: %s' % (t, 'regs' if r else 'address') for (_s, _f, r), (t, _a, _b) in list(zip(sigs, ex))[:4]))
        add(key, 'odr64', ex[0][1], ex[0][2], '', det, 'one definition, the same in every TU (shared header)')

    # tools/port/layout32_msvc.py's last result (cl's own layout of each record vs the i686 pass): where they differ
    # (Itanium tail-padding reuse, member-function-pointer sizes, SDK shims) the 32-bit judgement above is off
    lm = os.path.join(ROOT, 'work', 'port64', 'layout32_msvc.json')
    if os.path.realpath(root) == os.path.realpath(ROOT) and os.path.exists(lm):
        try:
            for b in json.load(open(lm))['bad']:
                add(b['what'].split('::')[0].replace(' sizeof', ''), 'layout32', b['file'], b['line'], '',
                    '%s: MSVC %s, i686 clang %#x (layout32_msvc.py)' % (
                        b['what'], '%#x' % b['msvc'] if b['msvc'] is not None else '?', b['clang']),
                    'the i686 model differs from cl here: check the offsets by hand', src_line(root, b['file'], b['line']))
        except (OSError, ValueError, KeyError):
            pass

    # find_views: the view records and their real types
    if True:
        if vj:
            covered = {x['view'] for x in F if x['view'] and lv(x['cat']) == 'W'}
            for v in vj['records']:
                if not v.get('view'):
                    continue
                name = v['qual']
                if name in covered or v['name'] in covered:
                    continue
                real = v.get('real') or name
                if v.get('wrong64') == 'yes':
                    add(real, 'view64', v['file'], v['line'], '', 'view %s (%d cast sites, find_views: %s)' % (
                        name, v.get('casts', 0), '; '.join(v.get('why') or [])[:120]), 'fold %s into %s' % (name, real),
                        view=name)
    t2 = time.time()
    if not quiet:
        sys.stderr.write('port64_worklist: load %.1f s, judge %.1f s, %d findings\n' % (t1 - t0, t2 - t1, len(F)))
    return F


def rank(F):
    groups = collections.defaultdict(list)
    for x in F:
        groups[x['cls']].append(x)
    out = []
    for cls, xs in groups.items():
        c = collections.Counter(lv(x['cat']) for x in xs)
        out.append((cls, xs, c))
    out.sort(key=lambda g: (min(LEVEL_ORDER[k] for k in g[2]), -g[2]['W'], -g[2]['V'], -g[2]['L'], -len(g[1]), g[0]))
    return out


def cls_kind(cls):
    return 'fn' if cls.startswith('fn ') else 'file' if cls.startswith('file ') else 'record'


def write(F, out_md, out_tsv, root):
    groups = rank(F)
    head = subprocess.run(['git', '-C', root, 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()
    dirty = subprocess.run(['git', '-C', root, 'status', '--porcelain', 'src'], capture_output=True, text=True).stdout
    if dirty.strip():
        head += ' + %d uncommitted src/ file(s)' % len(dirty.strip().split('\n'))
    with open(out_tsv, 'w') as t:
        t.write('rank\tclass\tlevel\tcategory\tfile\tline\tfunc\tdetail\tsuggestion\tview\ttext\n')
        for i, (cls, xs, c) in enumerate(groups, 1):
            for x in sorted(xs, key=lambda x: (LEVEL_ORDER[lv(x['cat'])], list(CATS).index(x['cat']), x['file'], x['line'])):
                t.write('\t'.join(str(v).replace('\t', ' ') for v in (
                    i, cls, lv(x['cat']), x['cat'], x['file'], x['line'], x['func'], x['detail'], x['sug'], x['view'],
                    x['text'])) + '\n')
    tot = collections.Counter(x['cat'] for x in F)
    ncls = collections.Counter()
    for cls, xs, c in groups:
        for cat in {x['cat'] for x in xs}:
            ncls[cat] += 1
    lvl = collections.Counter(lv(x['cat']) for x in F)
    wcls = sum(1 for g in groups if g[2]['W'])
    L = []
    w = L.append
    w('# 64-bit worklist: every place in src/ whose types are wrong at 64 bits, ranked')
    w('')
    w('Generated by `.venv/bin/python tools/port/port64_worklist.py` (writes this file and docs/port64_worklist.tsv;')
    w('~1 min cold, seconds when cached: run it per commit). Measured at %s. Do not edit by hand.' % head)
    w('')
    w('Sources: one clang pass per src/ TU at 64 bits (macos-arm64 flags, the warnings below on) and one at 32 bits')
    w('(i686, MSVC record rules) through tools/port/port64_ast (libTooling), plus tools/port/find_views.py\'s view')
    w('records. Every site is judged against both layouts: a raw offset is wrong when the byte it names at 32 bits')
    w('is elsewhere at 64; a view is wrong when one of its members lands on another member of the real record;')
    w('a pointer through a 32-bit integer is always wrong (every macOS arm64 address is above 4 GB). The real')
    w('class is the record the bytes belong to (the cast source, the member\'s owner); `fn X` groups a function\'s')
    w('locals / parameters, `file X` a file\'s warnings. Full rows (file:line, function, suggestion) in the TSV;')
    w('`--check NAME` prints one class.')
    w('')
    w('## Summary')
    w('')
    w('**%d findings in %d classes; %d classes have 64-bit-wrong findings (W %d, V %d, L %d, C %d, I %d).**' % (
        len(F), len(groups), wcls, lvl['W'], lvl['V'], lvl['L'], lvl['C'], lvl['I']))
    w('')
    w('| category | level | findings | classes | meaning |')
    w('|---|---|---:|---:|---|')
    for cat, (l, desc) in CATS.items():
        w('| %s | %s | %d | %d | %s |' % (cat, l, tot[cat], ncls[cat], desc))
    w('')
    w('Levels: **W** wrong at 64 bits (proven from the layouts), **V** a view over a real type with pointers')
    w('(find_views: fold it), **L** likely / unproven, **C** the 32-bit layout disagrees with the original\'s')
    w('offsets, **I** right at 64 bits but the type lies.')
    w('')
    w('## Ranked classes')
    w('')
    w('Ordered: classes with W findings first (most W first), then V, L, C, I. Counts per level; the first')
    w('W / V finding and its suggestion.')
    w('')
    w('| # | class | W | V | L | C | I | first finding | suggestion |')
    w('|---:|---|---:|---:|---:|---:|---:|---|---|')
    for i, (cls, xs, c) in enumerate(groups, 1):
        if i > 400:
            break
        top = sorted(xs, key=lambda x: (LEVEL_ORDER[lv(x['cat'])], x['file'], x['line']))[0]
        w('| %d | %s | %d | %d | %d | %d | %d | %s:%d %s | %s |' % (
            i, cls.replace('|', '\\|'), c['W'], c['V'], c['L'], c['C'], c['I'], top['file'], top['line'],
            short(top['detail'], 90).replace('|', '\\|'), short(top['sug'], 60).replace('|', '\\|')))
    if len(groups) > 400:
        w('')
        w('(%d more classes in the TSV.)' % (len(groups) - 400))
    w('')
    w('## Top 40 in detail (W findings)')
    w('')
    for i, (cls, xs, c) in enumerate(groups[:40], 1):
        ws = [x for x in xs if lv(x['cat']) == 'W']
        if not ws:
            break
        w('### %d. %s (W %d)' % (i, cls, len(ws)))
        w('')
        cc = collections.Counter(x['cat'] for x in ws)
        w(', '.join('%s %d' % kv for kv in cc.most_common()))
        w('')
        for x in sorted(ws, key=lambda x: (x['file'], x['line']))[:12]:
            w('- `%s:%d` %s%s: %s -> %s' % (x['file'], x['line'], x['cat'], (' (' + x['func'] + ')') if x['func'] else '',
                                          short(x['detail'], 140), short(x['sug'], 80)))
        if len(ws) > 12:
            w('- ... %d more in the TSV' % (len(ws) - 12))
        w('')
    open(out_md, 'w').write('\n'.join(L) + '\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default=ROOT)
    ap.add_argument('--out', help='directory for port64_worklist.md / .tsv (default docs/)')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--no-views', action='store_true', help='skip find_views.py')
    ap.add_argument('--check', nargs='*', help='print the findings whose class / text matches')
    ap.add_argument('--json', help='also dump the findings as JSON')
    a = ap.parse_args()
    root = os.path.realpath(a.root)
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
    import heavy
    jobs = heavy.pool_jobs(a.jobs)
    vj = None
    if not a.no_views:
        vj = os.path.join(ROOT, 'work', 'port64', port64_ast.tag(root), 'views.json')
        os.makedirs(os.path.dirname(vj), exist_ok=True)
        subprocess.run([sys.executable, os.path.join(HERE, 'find_views.py'), '--out', vj], cwd=root, capture_output=True,
                       env=dict(os.environ, FIND_VIEWS_ROOT=root))
    F = build(root, jobs, vj)
    if a.json:
        json.dump(F, open(a.json, 'w'), indent=0)
    if a.check is not None:
        pats = [p.lower() for p in a.check]
        for x in F:
            blob = ('%s %s %s %s %s' % (x['cls'], x['view'], x['detail'], x['text'], x['file'])).lower()
            if any(p in blob for p in pats):
                print('%s %-10s %s:%d [%s] %s | %s -> %s' % (lv(x['cat']), x['cat'], x['file'], x['line'], x['cls'],
                                                           x['func'], x['detail'], x['sug']))
        return
    out = a.out or os.path.join(ROOT, 'docs')
    os.makedirs(out, exist_ok=True)
    write(F, os.path.join(out, 'port64_worklist.md'), os.path.join(out, 'port64_worklist.tsv'), root)
    c = collections.Counter(lv(x['cat']) for x in F)
    print('port64_worklist: %d findings (W %d, V %d, L %d, C %d, I %d) -> %s' % (
        len(F), c['W'], c['V'], c['L'], c['C'], c['I'], os.path.join(out, 'port64_worklist.md')))


if __name__ == '__main__':
    main()
