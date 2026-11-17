#!/usr/bin/env python3
"""tools/port/offset_sites.py: list every raw-offset member access in src/ (ticket 022) from clang's AST.

Each src/ TU is parsed with the portable linux-i686 configuration (port/, the same flags as tools/port/burndown.py; the
i686 record layouts match the MSVC 32-bit ones closely enough, port/ asserts most of them), twice: once with
-ast-dump=json for the expressions, once with -fdump-record-layouts-complete for the field offsets. A site is one of

  A  (char*)X + 0xNN          a byte offset off a record pointer (cast kind BitCast), however it is then dereferenced
  B  ((T*)X)[k]               a scalar-pointer view of a record, indexed
  C  ((U*)X)->f               a record pointer reinterpreted as another, unrelated record (BitCast, not a base cast)
  D  *(T*)&X->field           a field's address reinterpreted as another type (`*(Pawn**)&g_mission->f65e4`)

and gets a class from the layout of X's record at that offset:

  E  a named field exists at that offset: replace the access directly
  N  only a pad or an offset-named field (fNN, pad_NN, unkNN) covers it: name or add the field first
  V  a hand-rolled view of a VC7.1 std::vector (a 4-byte allocator pad then three element pointers, or a cast to
     std::vector* over a pad): becomes a real std::vector member
  U  the record is unknown (void*, char*, int*) or the offset is not a constant

    tools/port/offset_sites.py                    # all TUs -> docs/tickets/022-offset-sites.tsv
    tools/port/offset_sites.py --only gui_edit    # some TUs, print only
    tools/port/offset_sites.py --out x.tsv

Nothing is compiled for MSVC; the port build directory build/port/linux-i686 is configured if missing.
"""
import argparse, json, os, re, shlex, subprocess, sys, threading
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SRC = os.path.join(ROOT, 'src') + os.sep
BUILD = os.path.join(ROOT, 'build', 'port', 'linux-i686')
PADLIKE = re.compile(r'^(_*(pad|unk|gap|raw|res|reserved|filler|field|f|u|x|b|p|r|v|w|o|off|at|d|n|m|s|i|c|h|q)_?)?'
                     r'(0x)?[0-9a-f]+(_[0-9a-f]+)?$|^_*(pad|unk|gap|raw|tail|rest|spare|filler)\w*$', re.I)


def cc1_line(entry):
    a = shlex.split(entry['command'])
    a.insert(1, '-###')
    r = subprocess.run(a, capture_output=True, cwd=entry['directory'], text=True)
    line = [l for l in r.stderr.splitlines() if '"-cc1"' in l][0]
    cc1 = shlex.split(line)
    i = cc1.index('-o')
    del cc1[i:i + 2]
    return cc1


def run_cc1(cc1, action, extra, cwd):
    c = list(cc1)
    c[c.index('-emit-obj')] = action
    return subprocess.run(c + extra, capture_output=True, cwd=cwd).stdout


# ---------------------------------------------------------------- record layouts
LAY_RE = re.compile(r'^\s*(\d+)(?::[0-9-]+)? \|( *)(.*)$')


def parse_layouts(text):
    """{record name: [(offset, depth, type, name)]}; depth 1 = a direct member or base."""
    out, cur, name = {}, None, None
    for line in text.decode('utf-8', 'replace').splitlines():
        if line.startswith('*** Dumping AST Record Layout'):
            cur, name = [], None
            continue
        if cur is None:
            continue
        m = LAY_RE.match(line)
        if not m:
            if '[sizeof=' in line and name is not None:
                sz = re.search(r'sizeof=(\d+)', line)
                out.setdefault(name, (int(sz.group(1)), cur))
                cur = None
            continue
        off, ind, rest = int(m.group(1)), len(m.group(2)), m.group(3).strip()
        if name is None:
            name = re.sub(r'^(struct|class|union) ', '', rest)
            continue
        depth = (ind - 1) // 2 + 1
        if rest.endswith('(primary base)') or rest.endswith('(base)') or rest.endswith('(virtual base)'):
            cur.append((off, depth, re.sub(r' \((primary |virtual )?base\)$', '', rest), '<base>'))
        elif rest.startswith('(') and rest.endswith(')'):
            cur.append((off, depth, rest, '<vptr>'))
        else:
            mm = re.match(r'^(.*?)\s*([A-Za-z_]\w*)$', rest)
            cur.append((off, depth, mm.group(1) if mm else rest, mm.group(2) if mm else '?'))
    return out


def arr_len(t):
    m = re.search(r'\[(\d+)\]', t)
    return int(m.group(1)) if m else None


def field_offset(lay, rec, name):
    """byte offset of member `name` of `rec` (a direct member, or one of a base's), else None."""
    if rec not in lay:
        return None
    hits = sorted((dep, o) for (o, dep, t, n) in lay[rec][1] if n == name)   # the shallowest: not a sub-object's same-named field
    return hits[0][1] if hits else None


def classify_site(lay, s):
    kind, rec, off, target = s['kind'], s['rec'], s['off'], s['target']
    if kind == 'C':
        if s.get('method'):
            return 'X', '', 'method called through a reinterpreted pointer (%s): a class view, not a field access' % s['member']
        off = field_offset(lay, s['to'], s['member'])
        s['off'] = off
        if off is None:
            return 'U', '', 'member %s of %s not found' % (s['member'], s['to'])
        if s['to'] in lay and rec in lay and lay[s['to']][0] > lay[rec][0]:
            pass
    if kind == 'D':
        off = field_offset(lay, rec, s['member'])
        s['off'] = off
        if off is None:
            return 'U', '', 'field %s not found' % s['member']
        if PADLIKE.match(s['member']):
            return 'N', s['member'], 'offset-named field `%s %s` viewed as %s: name and type it' % (s.get('ftype', ''), s['member'], target)
        if s['member'] in ('buf', '_Bx', '_Buf'):
            return 'X', s['member'], 'string small buffer (not a member access)'
        ft = s.get('ftype', '')
        if target.endswith('**') and re.match(r'^(const )?(unsigned( int)?|int|long|unsigned long)$', ft):
            return 'N', s['member'], 'a pointer kept in an `%s` field: retype it (64-bit: the pointer does not fit)' % ft
        return 'X', s['member'], 'typed view (%s) of the named field `%s %s`: portable; ticket 019 cleanup' % (target, ft, s['member'])
    if rec is None:
        return 'X', '', 'untyped pointer arithmetic (element stride / array cookie), not a member access'
    if off is None:
        return 'U', '', 'computed offset (array stride or index)'
    if off < 0:
        return 'X', '', 'negative offset (array cookie / container-of)'
    return classify(lay, rec, off, target)


def classify(lay, rec, off, target):
    """(class, field path, note) for an access at byte `off` of record `rec`."""
    if rec not in lay:
        return 'U', '', 'record has no layout (incomplete?)'
    size, rows = lay[rec]
    tvec = 'vector<' in (target or '')
    # the chain of fields that start at exactly `off`, outermost first
    path, chain = [], []
    for i, (o, dep, t, n) in enumerate(rows):
        if o == off and n not in ('<vptr>',):
            chain.append((dep, t, n, i))
    if not chain:
        # inside something: the deepest row starting before off whose extent (known only for arrays) covers it
        best = None
        for i, (o, dep, t, n) in enumerate(rows):
            if o < off and n not in ('<vptr>', '<base>'):
                best = (o, dep, t, n, i)
        if best is None:
            return 'U', '', 'offset 0x%x before any field' % off
        o, dep, t, n, i = best
        if '[' in t and PADLIKE.match(n):
            return ('V' if tvec else 'N'), n + '+0x%x' % (off - o), 'pad: add the field'
        if '[' in t:
            return 'E', n + '[+0x%x]' % (off - o), 'inside an array'
        if off >= size:
            return 'U', '', 'offset 0x%x past sizeof 0x%x' % (off, size)
        return 'N', n + '+0x%x' % (off - o), 'inside a field (wrong record type?)'
    names = [n for (_, _, n, _) in chain if n != '<base>']
    good = [n for n in names if not PADLIKE.match(n)]
    # VC7.1 vector: a 4-byte allocator pad followed by three pointers of one type
    dep, t, n, i = chain[-1]
    if PADLIKE.match(n) and t.startswith('char[4]') and i + 3 < len(rows):
        p = rows[i + 1:i + 4]
        if all(r[2].endswith('*') for r in p) and len({r[2] for r in p}) == 1:
            return 'V', '%s,%s/%s/%s' % (n, p[0][3], p[1][3], p[2][3]), 'hand-rolled vector (allocator pad + 3 pointers)'
    if any(n in ('_Myfirst', '_Mylast', '_Myend') for n in names):
        return 'V', '.'.join(names), 'reads vector internals'
    named = [n for n in names if n != '?']
    if named and PADLIKE.match(named[0]) and not tvec:
        t0 = [t for (_, t, n, _) in chain if n == named[0]][0]
        if t0.startswith('char[') or t0.startswith('unsigned char['):
            return 'N', '.'.join(names), 'pad: add the field'
        return 'N', '.'.join(names), 'offset-named member exists (%s): use it now, name it (ticket 021)' % t0
    if good:
        ftype = [t for (_, t, n, _) in chain if n == good[-1]][0]
        if tvec and 'vector' not in ftype:
            return 'V', '.'.join(names), 'std::vector view over a non-vector field'
        return 'E', '.'.join(names), ftype
    if tvec:
        return 'V', '.'.join(names), 'std::vector view over a pad'
    return 'N', '.'.join(names) or chain[-1][2], 'pad: add the field'


# ---------------------------------------------------------------- AST walk
def strip(n):
    while n.get('kind') in ('ParenExpr', 'ImplicitCastExpr', 'ExprWithCleanups', 'MaterializeTemporaryExpr'):
        if not n.get('inner'):
            break
        if n.get('kind') == 'ImplicitCastExpr' and n.get('castKind') not in ('LValueToRValue', 'NoOp', 'ArrayToPointerDecay', 'UncheckedDerivedToBase', 'DerivedToBase'):
            break
        n = n['inner'][0]
    return n


def qt(n):
    t = n.get('type', {})
    return t.get('desugaredQualType') or t.get('qualType') or ''


def rec_of_ptr(t):
    """'Foo *' -> 'Foo' if it looks like a record pointer, else None."""
    t = re.sub(r'\b(const|volatile|struct|class|union)\b', '', t).strip()
    if not t.endswith('*') or t.endswith('**'):
        return None
    base = t[:-1].strip()
    if base in ('char', 'unsigned char', 'signed char', 'void', 'int', 'unsigned int', 'short', 'unsigned short', 'float',
                'double', 'long', 'unsigned long', 'bool', 'wchar_t', 'long long', 'unsigned long long', 'char16_t') or '(' in base:
        return None
    return base


def const_eval(n):
    n = strip(n)
    k = n.get('kind')
    if k == 'IntegerLiteral':
        return int(n['value'])
    if k == 'BinaryOperator' and n.get('opcode') in ('+', '-', '*'):
        a, b = const_eval(n['inner'][0]), const_eval(n['inner'][1])
        if a is None or b is None:
            return None
        return a + b if n['opcode'] == '+' else a - b if n['opcode'] == '-' else a * b
    if k in ('CStyleCastExpr', 'ImplicitCastExpr') and n.get('inner'):
        return const_eval(n['inner'][0])
    return None


SCALAR_SIZES = {'char': 1, 'unsigned char': 1, 'signed char': 1, 'bool': 1, 'short': 2, 'unsigned short': 2, 'int': 4,
                'unsigned int': 4, 'float': 4, 'long': 4, 'unsigned long': 4, 'double': 8, 'long long': 8, 'unsigned long long': 8}


class Walker:
    def __init__(self, tu):
        self.tu, self.file, self.sites, self.func = tu, None, [], []
        self.alias = {}   # VarDecl id of a `char* e = (char*)rec [+ k]` -> (record, k)

    def loc(self, d):
        if not isinstance(d, dict):
            return None
        if 'spellingLoc' in d:
            s = self.loc(d['spellingLoc'])
            e = self.loc(d['expansionLoc'])
            return e if (e and e[0].startswith(SRC)) else s   # a macro's use site, not its #define
        if 'file' in d:
            self.file = d['file']
        if 'offset' in d and self.file:
            return (self.file, d['offset'])
        return None

    def walk(self, n, parents):
        here = None
        for k, v in n.items():
            if k == 'loc':
                self.loc(v)
            elif k == 'range':
                b = self.loc(v.get('begin', {}))
                self.loc(v.get('end', {}))
                here = b
        kind = n.get('kind')
        isfn = kind in ('FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl')
        if isfn:
            self.func.append(n.get('name', '?'))
        if here and here[0].startswith(SRC) and kind:
            self.check(n, here, parents)
        if n.get('isImplicit') and kind in ('CXXRecordDecl',):
            pass
        for c in n.get('inner', []):
            if isinstance(c, dict):
                parents.append(n)
                self.walk(c, parents)
                parents.pop()
        if isfn:
            self.func.pop()

    def add(self, here, kind, rec, off, target, expr, parents):
        self.sites.append(dict(file=os.path.relpath(os.path.realpath(here[0]), ROOT), offset=here[1], kind=kind, rec=rec,
                               off=off, target=target, func=self.func[-1] if self.func else '', expr=expr))

    def access_type(self, parents):
        """the type the byte pointer is finally cast to (the nearest enclosing cast), for the record."""
        for p in reversed(parents[-4:]):
            if p.get('kind') in ('CStyleCastExpr', 'CXXReinterpretCastExpr', 'CXXStaticCastExpr'):
                return qt(p)
        return ''

    def byte_view(self, l):
        """(record or None, base offset, source type) if l is a byte pointer onto a record, else None."""
        if l.get('kind') in ('CStyleCastExpr', 'CXXReinterpretCastExpr') and re.match(r'^(const |volatile )*(unsigned |signed )?char \*', qt(l)) \
                and l.get('castKind') == 'BitCast':
            src = strip(l['inner'][0])
            st = qt(src)
            if rec_of_ptr(st) is None and '*' not in st:
                return None
            return rec_of_ptr(st), 0, st
        if l.get('kind') == 'DeclRefExpr' and l.get('referencedDecl', {}).get('id') in self.alias:
            rec, k = self.alias[l['referencedDecl']['id']]
            return rec, k, rec + ' * (via a char* local)'
        return None

    def check(self, n, here, parents):
        k = n.get('kind')
        if k == 'VarDecl' and n.get('inner') and re.match(r'^(const |volatile )*(unsigned |signed )?char \*', qt(n)):
            init = strip(n['inner'][-1])
            v = self.byte_view(init)
            if v and v[0]:
                self.alias[n['id']] = (v[0], v[1])
            elif init.get('kind') == 'BinaryOperator' and init.get('opcode') == '+':
                v = self.byte_view(strip(init['inner'][0]))
                off = const_eval(init['inner'][1])
                if v and v[0] and off is not None:
                    self.alias[n['id']] = (v[0], v[1] + off)
            return
        if k == 'BinaryOperator' and n.get('opcode') in ('+', '-') and len(n.get('inner', [])) == 2:
            v = self.byte_view(strip(n['inner'][0]))
            if v:
                rec, base, st = v
                off = const_eval(n['inner'][1])
                if off is not None and n['opcode'] == '-':
                    off = -off
                if off is not None:
                    off += base
                self.add(here, 'A', rec, off, self.access_type(parents), st, parents)
        elif k == 'ArraySubscriptExpr':
            b = strip(n['inner'][0])
            if b.get('kind') in ('CStyleCastExpr', 'CXXReinterpretCastExpr') and b.get('castKind') == 'BitCast':
                src = strip(b['inner'][0])
                rec = rec_of_ptr(qt(src))
                if rec is None:
                    return
                el = re.sub(r'\s*\*$', '', re.sub(r'\b(const|volatile)\b', '', qt(b)).strip())
                idx = const_eval(n['inner'][1])
                sz = SCALAR_SIZES.get(el, 4 if el.endswith('*') else None)
                off = idx * sz if (idx is not None and sz) else None
                self.add(here, 'B', rec, off, qt(b), qt(src), parents)
        elif k == 'MemberExpr' and n.get('isArrow'):
            b = strip(n['inner'][0])
            if b.get('kind') in ('CStyleCastExpr', 'CXXReinterpretCastExpr') and b.get('castKind') == 'BitCast':
                src = strip(b['inner'][0])
                srec = rec_of_ptr(qt(src))
                if srec is None:
                    return
                if src.get('kind') == 'UnaryOperator' and src.get('opcode') == '&':
                    return   # *(T*)&x->f: reported as D at the cast
                self.add(here, 'C', srec, None, qt(b) + ' ->' + n.get('name', '?'), qt(src), parents)
                self.sites[-1].update(to=rec_of_ptr(qt(b)), member=n.get('name', '?'),
                                      method=parents and parents[-1].get('kind') == 'CXXMemberCallExpr' or n.get('name', '?').startswith('~'))
        elif k in ('CStyleCastExpr', 'CXXReinterpretCastExpr') and n.get('castKind') == 'BitCast':
            src = strip(n['inner'][0])
            if src.get('kind') == 'UnaryOperator' and src.get('opcode') == '&':
                m = strip(src['inner'][0])
                if m.get('kind') == 'MemberExpr':
                    base = strip(m['inner'][0])
                    arrow = m.get('isArrow')
                    while base.get('kind') == 'MemberExpr' and not base.get('name') and base.get('inner'):
                        arrow = base.get('isArrow')   # a field of an anonymous union / struct: the record that holds it
                        base = strip(base['inner'][0])
                    rec = rec_of_ptr(qt(base)) if arrow else re.sub(r'\b(const|struct|class)\b', '', qt(base)).strip()
                    t = qt(n)
                    if 'char *' in t and parents and parents[-1].get('kind') in ('BinaryOperator',):
                        return   # (char*)&x->f + k: the A pattern on a field, counted there if at all
                    self.add(here, 'D', rec, None, t, 'field ' + m.get('name', '?'), parents)
                    self.sites[-1].update(member=m.get('name', '?'), ftype=qt(m))


def scan(entry):
    tu = entry['file']
    cwd = entry['directory']
    try:
        cc1 = cc1_line(entry)
    except Exception as e:
        return tu, [], {}, 'no cc1: %s' % e
    lay = parse_layouts(run_cc1(cc1, '-fsyntax-only', ['-fdump-record-layouts-complete'], cwd))
    raw = run_cc1(cc1, '-ast-dump=json', [], cwd)
    try:
        ast = json.loads(raw)
    except ValueError:
        return tu, [], lay, 'ast dump failed'
    del raw
    w = Walker(tu)
    w.walk(ast, [])
    return tu, w.sites, lay, ''


def line_of(path, off, cache={}):
    if path not in cache:
        with open(os.path.join(ROOT, path), 'rb') as f:
            data = f.read()
        nl = [i for i, c in enumerate(data) if c == 10]
        cache[path] = (data, nl)
    data, nl = cache[path]
    import bisect
    ln = bisect.bisect_right(nl, off - 1) + 1
    s = nl[ln - 2] + 1 if ln > 1 else 0
    e = nl[ln - 1] if ln - 1 < len(nl) else len(data)
    return ln, off - s + 1, data[s:e].decode('utf-8', 'replace').strip()


def marker_va(path, ln, cache={}):
    """the nearest `// @ 0x<va>` marker at or above the line (the enclosing function's VA, by the repo convention)."""
    if path not in cache:
        with open(os.path.join(ROOT, path), encoding='utf-8', errors='replace') as f:
            cache[path] = f.read().splitlines()
    lines = cache[path]
    for i in range(ln - 1, -1, -1):
        m = re.search(r'//\s*@\s*(0x[0-9a-fA-F]{6,8})', lines[i])
        if m:
            return m.group(1).lower()
    return ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', action='append', default=[])
    ap.add_argument('--out', default=os.path.join(ROOT, 'docs', 'tickets', '022-offset-sites.tsv'))
    ap.add_argument('--jobs', type=int, default=3)
    a = ap.parse_args()
    cc = os.path.join(BUILD, 'compile_commands.json')
    if not os.path.exists(cc):
        subprocess.run(['cmake', '-S', os.path.join(ROOT, 'port'), '-B', BUILD, '-G', 'Ninja', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                        '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TOOLCHAIN_FILE=' + os.path.join(ROOT, 'port/cmake/linux-i686.cmake')],
                       check=True, capture_output=True)
    entries = [e for e in json.load(open(cc)) if os.path.realpath(e['file']).startswith(SRC)]
    if a.only:
        entries = [e for e in entries if any(o in e['file'] for o in a.only)]
    seen, rows, errs, glay = {}, [], [], {}
    pending = []
    with ThreadPoolExecutor(a.jobs) as ex:
        for tu, sites, lay, err in ex.map(scan, entries):
            if err:
                errs.append((tu, err))
            for k, v in lay.items():
                glay.setdefault(k, v)
            for s in sites:
                key = (s['file'], s['offset'], s['kind'])
                if key in seen:
                    continue
                seen[key] = s
                pending.append((s, lay))
    for s, lay in pending:
        import collections
        merged = collections.ChainMap(lay, glay)
        cls, field, note = classify_site(merged, s)
        s.update(cls=cls, field=field, note=note)
    for s in seen.values():
        ln, col, text = line_of(s['file'], s['offset'])
        s.update(line=ln, col=col, text=text, va=marker_va(s['file'], ln))
        rows.append(s)
    rows.sort(key=lambda s: (s['file'], s['line'], s['col']))
    hdr = ['file', 'line', 'col', 'va', 'func', 'kind', 'record', 'offset', 'access', 'class', 'field', 'note', 'status', 'source']
    out = ['\t'.join(hdr)]
    for s in rows:
        out.append('\t'.join(str(x).replace('\t', ' ') for x in [
            s['file'], s['line'], s['col'], s['va'], s['func'], s['kind'], s['rec'] or '-',
            ('0x%x' % s['off']) if s['off'] is not None else '?', s['target'], s['cls'], s['field'], s['note'], 'open',
            s['text'][:200]]))
    from collections import Counter
    c = Counter(s['cls'] for s in rows)
    print('sites %d  ' % len(rows) + '  '.join('%s %d' % kv for kv in sorted(c.items())))
    print('by kind: ' + '  '.join('%s %d' % kv for kv in sorted(Counter(s['kind'] for s in rows).items())))
    for tu, e in errs:
        print('error', tu, e)
    if not a.only:
        with open(a.out, 'w') as f:
            f.write('\n'.join(out) + '\n')
        print('wrote', os.path.relpath(a.out, ROOT))
    else:
        print('\n'.join(out))


if __name__ == '__main__':
    sys.setrecursionlimit(200000)
    threading.stack_size(512 * 1024 * 1024)
    t = threading.Thread(target=main)
    t.start()
    t.join()
