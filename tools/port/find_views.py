#!/usr/bin/env python3
"""tools/port/find_views.py: find the "views" in src/ (docs/views_inventory.md, ticket 006).

A view is a type written during decompilation to name a few fields of another object's memory: a struct with
`char padNN[..]` filler and a few `// +0x..` fields, reached by casting a pointer of a different type
(`(GameMapView*)game`, `*(vec_i4*)&x`, `slot_as<T>(i)`, `#define g_game_s29840 (*(GameData_s29840**)&g_game)`),
a `{lead, first, last, end}` stand-in for std::vector, a fake vtable (`WVt`), a per-function `*_s2c2b0` record,
or an X-suffixed twin (`PartyX : Party`) that only adds the names some caller uses.

Text only (no compiler; fast, re-runnable while src/ is being edited). Signals per record definition:

  (a) layout  pad arrays (`char padNN[..]`, `unkNN`, `gapNN`), `// +0x..` offset comments, named fields, pointer-holding
              fields (T*, tstring, std:: containers, records that hold pointers), 32-bit offsets from the comments or
              from summing 32-bit sizes (pads evaluated when their size is a constant expression)
  (b) casts   C-style `(T*)e`, `(T&)e`, `*(T*)&e`, `reinterpret_cast<T*>`, `static_cast<T*>` and the template helpers
              that wrap a reinterpret_cast (`slot_as<T>(i)`), plus #define macros that contain such a cast (every use
              of the macro is a site). The operand's static type is guessed: `this` -> the enclosing class; locals /
              parameters by the last declaration above the site in the function; members of the enclosing class;
              globals by their file-scope / extern declaration; `->m` / `.m` chains through parsed members;
              `(char*)p + N` gives p's type and the offset. Unresolved operands are "?".
  (c) names   `*_s[0-9a-f]{5,6}` per-slice records, `*View`, `*Vt` / `*Vtbl`, `*_pub` / `*Pub`, an X-suffixed twin of a
              defined record, `c[0-9a-f]{6}` / `s[0-9a-f]{6}` address names, `*_view`, `*Shim`
  (d) use     constructor declared or defined, `new T`, by-value declarations (locals, members, arrays, globals),
              `sizeof(T)`: a type that is only ever reached through a cast is a view candidate

Each cast site gets its enclosing function, the function's VA (the `// @ 0x..` marker above it, or a `// 0x..` comment on
the header line), and the VA's row status: exact (a *.manifest.txt row: the manifests list the byte-exact rows),
nonmatching (a *.nonmatching.txt row), norow (a VA in neither), or "-" (no VA: inline helpers, headers).

A record is reported as a view when it is cast to from another record or from raw memory and is never constructed, or
when its name says so and it has a layout (pads / offset comments) or casts; see is_view(). Records with pads that are
not views are listed as incomplete real types.

Known false positives / limits (the doc repeats them):
  - types with the same name in several TUs are separate records here (keyed by file:line); casts bind to the nearest
    definition (same file, then same directory, then any)
  - operand typing is lexical: overloaded operators, templates, macros with arguments and method-call results stay "?"
  - a cast from void* / char* to a real record (pool access, allocator results) is typed access, not a view: such
    records only count as views when they are never constructed and have pads
  - base-class casts written C-style (`(Base*)derived`) look like views: casts to a record that is a declared base of the
    source are dropped, but bases spelled through typedefs are not seen
  - row status needs a VA marker; header inline functions inherit "-" even when they are inlined into exact rows

    tools/port/find_views.py                       # -> work/views/views.json + summary on stdout
    tools/port/find_views.py --out x.json --top 40
    tools/port/find_views.py --name GameData_s29130   # one record's details
"""
import argparse, bisect, collections, json, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SRC = os.path.join(ROOT, 'src')
PATH_DOCS = ['port/INTEGRATION_STATUS.md', 'docs/layout64_inventory.md']

KEYWORDS = set('''if for while switch return sizeof else do case new delete throw catch operator typedef using
 goto break continue default static_cast reinterpret_cast const_cast dynamic_cast decltype alignof __declspec
 template typename struct class union enum namespace public private protected virtual inline __forceinline const
 volatile static extern friend explicit unsigned signed'''.split())
PRIM = set('''void char bool short int long float double unsigned signed __int64 __int8 __int16 __int32 wchar_t
 BYTE WORD DWORD UINT LONG ULONG INT BOOL size_t ptrdiff_t intptr_t uintptr_t uint8_t uint16_t uint32_t int32_t
 int64_t uint64_t int16_t int8_t HANDLE HWND LPVOID PVOID HRESULT'''.split())
RAW = set('void char int unsigned BYTE DWORD UINT uint8_t uint32_t int32_t intptr_t uintptr_t'.split())
PADNAME = re.compile(r'^_*(pad|unk|gap|filler|raw|spare|rest|tail|res|reserved|skip|hole|blank)\w*$|^_*(pad|unk|u|f|b|x)_?[0-9a-fA-F]+(_[0-9a-fA-F]+)?$', re.I)
SIZE32 = {'char': 1, 'bool': 1, 'BYTE': 1, 'uint8_t': 1, 'int8_t': 1, '__int8': 1, 'short': 2, 'WORD': 2,
          'uint16_t': 2, 'int16_t': 2, '__int16': 2, 'wchar_t': 2, 'int': 4, 'unsigned': 4, 'long': 4, 'float': 4,
          'DWORD': 4, 'UINT': 4, 'LONG': 4, 'ULONG': 4, 'INT': 4, 'BOOL': 4, 'uint32_t': 4, 'int32_t': 4,
          '__int32': 4, 'size_t': 4, 'ptrdiff_t': 4, 'intptr_t': 4, 'uintptr_t': 4, 'HRESULT': 4,
          'double': 8, '__int64': 8, 'int64_t': 8, 'uint64_t': 8, 'tstring': 0x1c}
PTRY_TYPES = re.compile(r'\b(tstring|std::|string\b|vector\b|list\b|map\b|hash_map|deque\b|rgl::vec\b|engvec|ptr_vec|PVec|IntVec|HANDLE|HWND)')
VIEWNAME = [
    ('slice', re.compile(r'_s[0-9a-f]{5,6}$|^s[0-9a-f]{6}_')),
    ('view', re.compile(r'(View|_view|Vw)$')),
    ('vtable', re.compile(r'(Vt|Vtbl|VTable|_vt|_vtbl|Vtab)$')),
    ('pub', re.compile(r'(_pub|Pub|Shim|_shim|Overlay|_ov)$')),
    ('addr', re.compile(r'^[cs][0-9a-f]{6,7}$|^[A-Za-z]+_[0-9a-f]{6}$')),
]
VECFIELD = {'first', 'last', 'end', 'begin', '_Myfirst', '_Mylast', '_Myend', 'finish', 'cap', 'end_of_storage', 'b', 'e', 'f', 'l'}
CODEGEN = re.compile(r'cl 13\.10|byte-exact|codegen|load-bearing|swaps? two|keeps? .{0,40}(exact|bytes)|run_all DIFF|exact only with|the row(\'s)? bytes', re.I)


def strip(text):
    """blank comments and string / char literals (same length, newlines kept) so braces and casts parse."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', text[i:j])); i = j
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == '\\': j += 1
                if j < n and text[j] == '\n': break
                j += 1
            out.append(c + ' ' * (j - i - 1) + (c if j < n else '')); i = j + 1
        else:
            out.append(c); i += 1
    return ''.join(out)


def match_close(s, i, o='{', c='}'):
    d = 0
    for j in range(i, len(s)):
        ch = s[j]
        if ch == o: d += 1
        elif ch == c:
            d -= 1
            if d == 0: return j
    return len(s) - 1


def eval_size(expr):
    e = expr.strip()
    if not re.fullmatch(r'[0-9a-fA-FxX+\-*/() \t]+', e): return None
    try:
        return int(eval(e, {}, {}))
    except Exception:
        return None


class File:
    def __init__(self, path):
        self.path = path
        self.rel = os.path.relpath(path, ROOT)
        self.text = open(path, errors='replace').read()
        self.code = strip(self.text)
        self.lines = self.text.split('\n')
        self.nl = [m.start() for m in re.finditer('\n', self.text)]

    def line(self, pos):
        return bisect.bisect_left(self.nl, pos) + 1


REC_RE = re.compile(r'(?<![\w:])(enum\s+)?(struct|class|union)\s+(?:__declspec\([^)]*\)\s*|alignas\([^)]*\)\s*|MB_\w+\s+)*'
                    r'([A-Za-z_]\w*)\s*(?:final\s*)?(:\s*[^{;()]*?)?\{')
FUNC_HDR = re.compile(r'([~\w:]+(?:<[^<>;{}]*>)?(?:::[~\w]+)?|operator\s*\S+?)\s*\($')


class Record:
    def __init__(self, f, name, kind, bases, start, body_open, body_close, outer):
        self.f, self.name, self.kind = f, name, kind
        self.bases = [re.sub(r'\b(public|private|protected|virtual)\b', '', b).strip() for b in (bases or '').lstrip(':').split(',') if b.strip()]
        self.start, self.open, self.close = start, body_open, body_close
        self.line = f.line(start)
        self.end_line = f.line(body_close)
        self.outer = outer
        self.qual = (outer.qual + '::' if outer else '') + name
        self.fields, self.methods = [], []
        self.pads = 0; self.pad_bytes = 0; self.offcomments = 0
        self.ctor = False; self.virtual = False; self.fnptrs = 0
        self.size32 = None; self.hints = set(); self.codegen = ''
        l0 = f.lines[self.line - 1] if self.line - 1 < len(f.lines) else ''
        lu = f.lines[self.line - 2] if self.line >= 2 else ''
        self.headcmt = (l0.split('//', 1)[1] if '//' in l0 else '') + ' ' + (lu.split('//', 1)[1] if lu.strip().startswith('//') else '')
        self.parse()

    def parse(self):
        f = self.f
        body = f.code[self.open + 1:self.close]
        flat, depth, buf = [], 0, []      # statements at depth 0 of the body with their start offsets
        stmt_start = 0
        i = 0
        while i < len(body):
            ch = body[i]
            if ch == '{':
                j = match_close(body, i)
                head = ''.join(buf)
                buf.append('{}')
                i = j + 1
                if re.search(r'\)\s*(const\s*)?(throw\s*\([^)]*\)\s*)?(:[^;]*)?$', head.replace('\n', ' ')) or re.search(r'\b(struct|class|union|enum)\b', head):
                    flat.append((stmt_start, ''.join(buf))); buf = []; stmt_start = i
                continue
            if ch == ';':
                flat.append((stmt_start, ''.join(buf))); buf = []; stmt_start = i + 1
            elif ch == ':' and re.fullmatch(r'\s*(public|private|protected)\s*', ''.join(buf) or 'x'):
                buf = []; stmt_start = i + 1
            else:
                buf.append(ch)
            i += 1
        off = 0
        for st, s in flat:
            s1 = ' '.join(s.split())
            if not s1: continue
            s1 = re.sub(r'^(public|private|protected)\s*:\s*', '', s1)
            pos = self.open + 1 + st + (len(s) - len(s.lstrip()))
            ln = f.line(pos)
            src_line = f.lines[ln - 1] if ln - 1 < len(f.lines) else ''
            # comments of the statement's lines
            last_ln = f.line(self.open + 1 + st + len(s))
            cmt = ' '.join(l.split('//', 1)[1] for l in f.lines[ln - 1:last_ln] if '//' in l)
            for h in re.findall(r'\b([A-Z]\w+|rgl::\w+)::(~?\w+)', cmt):
                if h[0] != self.name: self.hints.add(h[0])
            if re.search(r'\b(struct|class|union|enum|typedef|using|friend|static_assert|template)\b', s1.split('{')[0]) and not re.search(r'\b(struct|class|union)\s+\w+\s*[*&]*\s*\w+\s*(\[|$)', s1):
                if 'typedef' not in s1 and 'using' not in s1 and '{}' in s1:
                    continue
                if not re.search(r'\b(struct|class|union)\s+[\w:]+\s*[*&]+', s1): continue
            fnptr = re.search(r'\(\s*(?:__\w+\s+)?\*\s*(\w+)\s*\)\s*\(', s1)
            if '(' in s1 and not fnptr:
                m = re.search(r'([~\w]+)\s*\(', s1)
                nm = m.group(1) if m else ''
                is_virtual = 'virtual' in s1
                self.virtual |= is_virtual
                if nm == self.name: self.ctor = True
                self.methods.append({'name': nm, 'line': ln, 'virtual': is_virtual, 'sig': s1[:160]})
                continue
            if fnptr:
                self.fnptrs += 1
                self.fields.append({'name': fnptr.group(1), 'type': 'fnptr', 'ptr': True, 'line': ln, 'off': self.comment_off(src_line), 'size': 4, 'pad': False})
                off = None if off is None else off + 4
                continue
            s1 = re.sub(r'\s*=[^,]*$', '', s1)
            s1 = re.sub(r'\b(mutable|static|volatile|__declspec\([^)]*\))\s*', '', s1)
            if s1.startswith('static ') or 'static' in s.split():
                continue
            m = re.match(r'((?:const\s+)?(?:unsigned\s+|signed\s+)?(?:struct\s+|class\s+|union\s+|enum\s+)?(?:[\w:]+(?:<.*>)?)(?:\s+(?:long|int|char|short))*)\s*(.*)$', s1)
            if not m: continue
            typ, decls = m.group(1).strip(), m.group(2)
            if not decls:      # anonymous union/struct member or bare type
                if '{}' in s1: off = None
                continue
            for d in self.split_decls(decls):
                dm = re.match(r'([*&\s]*)(?:const\s+)?(\w+)\s*((?:\[[^\]]*\])*)\s*(?::\s*\d+)?$', d.strip())
                if not dm: continue
                stars, nm, arr = dm.group(1).replace(' ', ''), dm.group(2), dm.group(3)
                cnt = 1
                for a in re.findall(r'\[([^\]]*)\]', arr):
                    v = eval_size(a)
                    cnt = None if (v is None or cnt is None) else cnt * v
                base = re.sub(r'^(const|struct|class|union|enum)\s+', '', typ)
                base = re.sub(r'^(unsigned|signed)\s+(int|char|short|long)$', r'\2', base)
                if base.startswith('unsigned') and base != 'unsigned': base = base.split()[-1]
                is_ptr = bool(stars) or bool(PTRY_TYPES.search(typ))
                esize = 4 if stars else SIZE32.get(base)
                size = None if (esize is None or cnt is None) else esize * cnt
                is_pad = bool(PADNAME.match(nm)) and base in ('char', 'BYTE', 'uint8_t', 'int', 'unsigned', 'DWORD', 'byte') and bool(arr) or \
                         (bool(re.match(r'^_*(pad|gap|unk|filler)', nm, re.I)) and not stars)
                if is_pad:
                    self.pads += 1
                    if size: self.pad_bytes += size
                fo = self.comment_off(src_line)
                if fo is not None: self.offcomments += 1
                if fo is None and off is not None: fo = off
                ptr_cmt = bool(re.search(r'\b(ptr|pointer)\b|->|\w\*', src_line.split('//', 1)[1])) if '//' in src_line else False
                self.fields.append({'name': nm, 'type': typ + stars + arr, 'ptr': is_ptr, 'int_ptr': (not stars and base in RAW and ptr_cmt and not arr), 'line': ln, 'off': fo, 'size': size, 'pad': is_pad, 'base': base})
                off = None if (fo is None or size is None) else fo + size
        if off is not None and not self.bases and not self.virtual: self.size32 = off
        # comments in and above the definition: real-type hints, codegen notes
        lo = max(0, self.line - 8)
        above = '\n'.join(f.lines[lo:self.line - 1])
        inside = '\n'.join(l.split('//', 1)[1] for l in f.lines[self.line - 1:self.end_line] if '//' in l)
        for h in re.findall(r'\b([A-Z]\w+|rgl::\w+)::(~?\w+)', above + '\n' + inside):
            if h[0] != self.name: self.hints.add(h[0])
        for h in re.findall(r'(?:view of|views?|overlay of|as seen|reads? the|is the|of the)\s+(?:the\s+)?`?((?:rgl::)?[A-Z][A-Za-z0-9_]{2,})', above + '\n' + inside):
            if h != self.name: self.hints.add(h)
        # the codegen note must be the definition's own: its lines or the comment block right above it
        blk = []
        for l in reversed(f.lines[:self.line - 1]):
            if l.strip().startswith('//'): blk.insert(0, l)
            else: break
        own = '\n'.join(blk) + '\n' + '\n'.join(f.lines[self.line - 1:self.end_line])
        m = CODEGEN.search(own)
        above, inside = own, ''
        if m:
            seg = (above + '\n' + inside)
            k = m.start()
            self.codegen = ' '.join(seg[max(0, k - 80):k + 100].split())

    @staticmethod
    def comment_off(line):
        if '//' not in line: return None
        m = re.search(r'//\s*(?:\[|\()?\+\s*(0x[0-9a-fA-F]+|\d+)\b', line) or re.search(r'//\s*(0x[0-9a-fA-F]{1,5})\b(?!\s*\()', line)
        if not m: return None
        v = m.group(1)
        if len(v) > 7: return None      # an address, not an offset
        return int(v, 16) if v.lower().startswith('0x') else int(v)

    @staticmethod
    def split_decls(s):
        out, d, cur = [], 0, ''
        for ch in s:
            if ch in '[(<': d += 1
            elif ch in '])>': d -= 1
            if ch == ',' and d == 0:
                out.append(cur); cur = ''
            else:
                cur += ch
        out.append(cur)
        return out

    def key(self):
        return '%s:%d' % (self.f.rel, self.line)


class Func:
    __slots__ = ('name', 'cls', 'start', 'end', 'line', 'va', 'f')


def find_funcs(f, recs):
    """function bodies: a '{' after ')' (optionally const / an init list) whose '(' follows a name that is not a
    control keyword. Outermost only."""
    code = f.code
    out = []
    i = 0
    rec_spans = sorted((r.open, r.close, r) for r in recs)
    pat = re.compile(r'\)\s*(?:const\s*)?(?:throw\s*\([^)]*\)\s*)?(?:override\s*)?(?::(?:[^;{}]|\{[^{}]*\})*?)?\{')
    while True:
        m = pat.search(code, i)
        if not m: break
        brace = m.end() - 1
        # the ')' position
        rp = m.start()
        # find matching '('
        d, j = 0, rp
        while j >= 0:
            if code[j] == ')': d += 1
            elif code[j] == '(':
                d -= 1
                if d == 0: break
            j -= 1
        pre = code[max(0, j - 200):j].rstrip()
        nm = re.search(r'((?:[\w~]+(?:<[^<>;{}()]*>)?::)*[~\w]+|operator\s*[^\s(]+)$', pre)
        name = nm.group(1) if nm else ''
        last = name.split('::')[-1]
        # init list: `X::X(..) : a(1), b(2) {` -> the ':' segment is in m; name is the function if the ':' came after rp
        if not name or last in KEYWORDS or last in ('if', 'for', 'while', 'switch', 'catch', 'sizeof', '__try', '__except'):
            i = brace + 1 if (last in ('switch',) or not name) else m.start() + 1
            if last in ('if', 'for', 'while', 'switch', 'catch'): i = m.start() + 1
            continue
        end = match_close(code, brace)
        # if this brace is inside a record but not inside its methods it is an inline method: fine
        fn = Func()
        fn.f = f
        fn.name, fn.start, fn.end = name, brace, end
        fn.line = f.line(j)
        cls = None
        if '::' in name:
            cls = name.rsplit('::', 1)[0]
        else:
            for a, b, r in rec_spans:
                if a < brace < b: cls = r.qual
        fn.cls = cls
        fn.va = None
        for k in range(fn.line - 1, max(-1, fn.line - 5), -1):
            l = f.lines[k] if k < len(f.lines) else ''
            mm = re.search(r'//\s*@\s*0x([0-9a-fA-F]{6,8})', l)
            if mm: fn.va = int(mm.group(1), 16); break
            if k < fn.line - 1 and l.strip() and not l.strip().startswith(('//', '__declspec', 'template', 'static', 'inline')): break
        if fn.va is None:
            for l in f.lines[fn.line - 1:f.line(brace)]:
                mm = re.search(r'//.*?\b0x(4[0-9a-f]{5}|5[0-9a-f]{5})\b', l)
                if mm: fn.va = int(mm.group(1), 16); break
        out.append(fn)
        i = end + 1
    return out


def load_rows():
    rows, nonm = {}, set()
    for dp, dn, fn in os.walk(SRC):
        for x in fn:
            p = os.path.join(dp, x)
            if x.endswith('.manifest.txt'):
                for l in open(p, errors='replace'):
                    a = l.split()
                    if len(a) >= 3 and re.fullmatch(r'[0-9a-fA-F]{6,8}', a[2]):
                        rows[int(a[2], 16)] = a[1]
            elif x.endswith('.nonmatching.txt'):
                for l in open(p, errors='replace'):
                    a = l.split()
                    if a and re.fullmatch(r'[0-9a-fA-F]{6,8}', a[0]):
                        nonm.add(int(a[0], 16))
    return rows, nonm


CAST_C = re.compile(r'\(\s*(?:const\s+|volatile\s+|struct\s+|class\s+|union\s+)*((?:\w+::)*[A-Za-z_]\w*)\s*(?:const\s*)?(\*+|&)\s*(?:const\s*)?(\*?)\s*\)\s*(?=[\w(&*])')
CAST_CPP = re.compile(r'\b(reinterpret_cast|static_cast)\s*<\s*(?:const\s+|struct\s+|class\s+)*((?:\w+::)*\w+)\s*(?:const\s*)?([*&]+)\s*>\s*\(')
TMPL_HELPER = re.compile(r'template\s*<\s*(?:class|typename)\s+(\w+)\s*>\s*(?:static\s+)?(?:inline\s+|__forceinline\s+)*[\w:<>*& ]*?\b(\w+)\s*\([^)]*\)\s*(?:const\s*)?\{[^{}]*reinterpret_cast\s*<\s*(?:const\s+)?\1\b')


def operand(code, i):
    """the postfix expression starting at i; returns (text, end)."""
    n = len(code)
    j = i
    while j < n and code[j] in ' \t': j += 1
    st = j
    pre = ''
    while j < n and code[j] in '&*':
        pre += code[j]; j += 1
        while j < n and code[j] in ' \t': j += 1
    if j < n and code[j] == '(':
        e = match_close(code, j, '(', ')')
        j = e + 1
    else:
        m = re.match(r'(?:\w+::)*~?\w+', code[j:])
        if not m: return code[st:j], j
        j += m.end()
    while j < n:
        k = j
        while k < n and code[k] in ' \t': k += 1
        if code.startswith('->', k) or (k < n and code[k] == '.' and not code[k + 1:k + 2].isdigit()):
            k += 2 if code[k] == '-' else 1
            m = re.match(r'\s*~?\w+', code[k:])
            if not m: break
            j = k + m.end()
        elif k < n and code[k] in '[(':
            e = match_close(code, k, code[k], ']' if code[k] == '[' else ')')
            j = e + 1
        else:
            break
    return code[st:j], j


class Index:
    def __init__(self):
        self.files = []
        for dp, dn, fn in os.walk(SRC):
            dn.sort()
            for x in sorted(fn):
                if x.endswith(('.cpp', '.h', '.hpp', '.inl', '.c')):
                    self.files.append(File(os.path.join(dp, x)))
        self.by_rel = {f.rel: f for f in self.files}
        self.outctors = set()
        for f in self.files:
            for m in re.finditer(r'\b(\w+)::(\w+)\s*\(', f.code):
                if m.group(1) == m.group(2): self.outctors.add(m.group(1))
        self.recs = []
        self.by_name = collections.defaultdict(list)
        self.file_recs = {}
        for f in self.files:
            rs = []
            stack = []
            for m in REC_RE.finditer(f.code):
                if m.group(1): continue     # enum class
                pre = f.code[max(0, m.start() - 30):m.start()]
                if re.search(r'template\s*<[^>]*$', pre) or re.search(r'\btypedef\s*$', pre) and False:
                    pass
                o = m.end() - 1
                c = match_close(f.code, o)
                while stack and stack[-1].close < o: stack.pop()
                r = Record(f, m.group(3), m.group(2), m.group(4), m.start(), o, c, stack[-1] if stack else None)
                stack.append(r)
                rs.append(r)
            self.file_recs[f.rel] = rs
            for r in rs:
                self.recs.append(r)
                self.by_name[r.name].append(r)
                if r.qual != r.name: self.by_name[r.qual].append(r)
        # namespace-qualified names (rgl::mesh) by the enclosing namespace text
        for r in self.recs:
            ns = re.findall(r'\bnamespace\s+(\w+)\s*\{', r.f.code[:r.start])
            if ns and r.outer is None:
                q = ns[-1] + '::' + r.name
                # only when still inside it
                k = r.f.code.rfind('namespace ' + ns[-1], 0, r.start)
                b = r.f.code.find('{', k)
                if match_close(r.f.code, b) > r.start:
                    self.by_name[q].append(r)
                    r.ns = ns[-1]
        self.funcs = {f.rel: find_funcs(f, self.file_recs[f.rel]) for f in self.files}
        self.func_starts = {k: [fn.start for fn in v] for k, v in self.funcs.items()}
        self.globals = {}
        gre = re.compile(r'^[ \t]*(?:extern\s+)?(?:static\s+)?(?:const\s+)?(?:struct\s+|class\s+)?((?:\w+::)*\w+)\s*(?:<[^;]*>)?\s*([*&]*)\s*(?:const\s+)?(\w+)\s*(\[[^\]]*\])?\s*(?:;|=|\()', re.M)
        for f in self.files:
            for m in gre.finditer(f.code):
                t, nm = m.group(1), m.group(3)
                if t in KEYWORDS or t in ('return', 'typedef') or nm in KEYWORDS: continue
                if m.group(0).rstrip().endswith('(') : continue
                if f.code[m.start():m.start() + 1] in ' \t' and not m.group(0).lstrip().startswith('extern'):
                    continue    # indented: not file scope (good enough)
                if len(nm) < 3 and not nm.startswith('g'): continue
                self.globals.setdefault(nm, (t, m.group(2) + ('[]' if m.group(4) else '')))
        self.helpers = {}
        for f in self.files:
            for m in TMPL_HELPER.finditer(f.code):
                self.helpers[m.group(2)] = f.rel
        self.rows, self.nonm = load_rows()

    def lookup(self, name, near=None):
        rs = self.by_name.get(name) or self.by_name.get(name.split('::')[-1]) or []
        if not rs: return None
        if near:
            for r in rs:
                if r.f is near: return r
            d = os.path.dirname(near.rel)
            for r in rs:
                if os.path.dirname(r.f.rel) == d: return r
            # included headers
            incs = re.findall(r'#include\s+"([^"]+)"', near.text)
            for inc in incs:
                p = os.path.normpath(os.path.join(os.path.dirname(near.rel), inc))
                for r in rs:
                    if r.f.rel == p: return r
        return rs[0]

    def func_at(self, f, pos):
        fs = self.funcs[f.rel]
        k = bisect.bisect_right(self.func_starts[f.rel], pos) - 1
        if k >= 0 and fs[k].start <= pos <= fs[k].end: return fs[k]
        return None

    def member_type(self, rec, name, depth=0):
        if rec is None or depth > 4: return None
        for fl in rec.fields:
            if fl['name'] == name: return fl['type']
        for b in rec.bases:
            t = self.member_type(self.lookup(re.sub(r'<.*', '', b), rec.f), name, depth + 1)
            if t: return t
        return None

    def type_of(self, f, pos, fn, expr, depth=0):
        """(type name, offset or None) of a postfix / paren expression, lexically."""
        e = expr.strip()
        if depth > 4 or not e: return ('?', None)
        while e.startswith('(') and match_close(e, 0, '(', ')') == len(e) - 1:
            inner = e[1:-1].strip()
            # (char*)X + N
            m = re.match(r'\(\s*(?:const\s+)?(?:unsigned\s+)?(char|BYTE|uint8_t)\s*\*\s*\)\s*(.+?)\s*\+\s*(0x[0-9a-fA-F]+|\d+)\s*$', inner)
            if m:
                t, _ = self.type_of(f, pos, fn, m.group(2), depth + 1)
                return (t, int(m.group(3), 0))
            m = re.match(r'\(\s*(?:const\s+|struct\s+)?((?:\w+::)*\w+)\s*[*&]+\s*\)\s*(.+)$', inner)
            if m and m.group(1) not in KEYWORDS:
                return (m.group(1), None)
            e = inner
        neg = 0
        while e and e[0] in '&*':
            e = e[1:].strip()
        m = re.match(r'((?:\w+::)*\w+)', e)
        if not m: return ('?', None)
        root = m.group(1)
        rest = e[m.end():]
        t = None
        if root == 'this':
            t = fn.cls if fn and fn.cls else '?'
        else:
            if fn:
                body = f.code[max(0, fn.start - 600):pos]
                # declarations: `Type* name`, `Type name`, `Type& name` (last one wins)
                for dm in re.finditer(r'(?<![\w.>])((?:\w+::)*\w+)(?:<[^;(){}]*>)?\s*(?:const\s*)?([*&]*)\s*(?:const\s*)?\b' + re.escape(root) + r'\b\s*(?=[;=,)\[(:])', body):
                    tt = dm.group(1)
                    if tt in KEYWORDS and tt not in ('unsigned', 'signed') or tt == root: continue
                    t = tt
                if t is None and fn.cls:
                    mt = self.member_type(self.lookup(fn.cls.split('::')[-1], f), root)
                    if mt: t = mt
            if t is None and root in self.globals:
                t = self.globals[root][0]
            if t is None:
                rr = self.lookup(root, f)
                if rr is not None and rest.startswith('::'): t = None
        if t is None: return ('?', None)
        t = re.sub(r'^(const|struct|class)\s+', '', t)
        t = re.sub(r'[*&\s]|\[.*\]', '', t)
        # member chain
        for mm in re.finditer(r'\s*(->|\.)\s*(\w+)|\s*(\[)', rest):
            if mm.group(3): continue
            rec = self.lookup(t, f)
            mt = self.member_type(rec, mm.group(2)) if rec else None
            if not mt: return ('?', None)
            t = re.sub(r'^(const|struct|class|union)\s+', '', mt)
            t = re.sub(r'[*&\s]|\[.*\]', '', t)
            t = re.sub(r'<.*', '', t) if not t.startswith('std::') else t
        if re.search(r'\)\s*$', rest) and not rest.strip().startswith('['):
            return ('?', None)
        return (t, None)

    def casts(self):
        sites = []
        macros = {}
        accessors = {}
        for f in self.files:
            code = f.code
            hits = []
            for m in CAST_C.finditer(code):
                t = m.group(1)
                if t in KEYWORDS or t in PRIM or t.startswith('std::'): continue
                # a parameter list `f(Foo*)` or `sizeof(Foo*)`: the char after ')' must start an expression
                after = code[m.end():m.end() + 12]
                if re.match(r'(const|override|noexcept|throw|volatile)\b', after): continue
                before = code[max(0, m.start() - 40):m.start()].rstrip()
                if re.search(r'(\bsizeof|\boperator\s*\S*|\w|\])\s*$', before) and not re.search(r'\b(return|case|else|throw|delete)\s*$', before):
                    continue
                hits.append((m.start(), m.end(), t, m.group(2) + m.group(3), 'c'))
            for m in CAST_CPP.finditer(code):
                t = m.group(2)
                if t in PRIM or t.startswith('std::'): continue
                hits.append((m.start(), m.end() - 1, t, m.group(3), m.group(1)))
            for h in self.helpers:
                for m in re.finditer(r'\b' + h + r'\s*<\s*(?:const\s+)?((?:\w+::)*\w+)\s*>\s*\(', code):
                    t = m.group(1)
                    if t in PRIM: continue
                    hits.append((m.start(), m.end() - 1, t, '*', h))
            for st, en, t, ref, how in hits:
                expr, _ = operand(code, en)
                ln = f.line(st)
                fn = self.func_at(f, st)
                lt = f.code[f.text.rfind('\n', 0, st) + 1:st]
                macro = None
                mm = re.match(r'\s*#\s*define\s+(\w+)(\()?', lt)
                if mm: macro = (mm.group(1), bool(mm.group(2)))
                if how in self.helpers:
                    src, off = ('slot', None)
                elif macro and macro[1]:
                    src, off = ('?', None)      # a macro parameter: typed at each use
                else:
                    src, off = self.type_of(f, st, fn, expr)
                rec = {'file': f.rel, 'line': ln, 'target': t, 'ref': ref, 'how': how, 'expr': ' '.join(expr.split())[:80],
                       'src': src, 'off': off, 'func': fn.name if fn else None, 'cls': fn.cls if fn else None,
                       'va': fn.va if fn else None, 'macro': macro[0] if macro else None}
                if fn and not macro and '::' not in fn.name and re.fullmatch(r'\{\s*return\s*[^;]*;\s*\}', code[fn.start:fn.end + 1]) and fn.cls is None:
                    hdr = code[code.rfind('\n', 0, fn.start) + 1:fn.start]
                    if re.search(r'\b' + re.escape(fn.name) + r'\s*\(\s*\)', hdr):
                        # a zero-argument accessor `T* mission() { return (T*)g_mission; }`: every call is a site
                        accessors[fn.name] = (t, src, rec)
                if macro:
                    # the param index the cast applies to
                    pidx = None
                    if macro[1]:
                        pm = re.match(r'\s*#\s*define\s+\w+\(([^)]*)\)', f.code[f.text.rfind('\n', 0, st) + 1:])
                        params = [x.strip() for x in pm.group(1).split(',')] if pm else []
                        e0 = re.sub(r'[()\s&*]', '', expr)
                        pidx = params.index(e0) if e0 in params else None
                    macros[macro[0]] = (t, src, macro[1], f.rel, pidx, rec)
                else:
                    sites.append(rec)
        # macro uses
        used = set()
        for f in self.files:
            for name, (t, src, fnlike, deff, pidx, drec) in macros.items():
                pat = re.compile(r'\b' + re.escape(name) + (r'\s*\(' if fnlike else r'\b'))
                for m in pat.finditer(f.code):
                    lt = f.code[f.text.rfind('\n', 0, m.start()) + 1:m.start()]
                    if re.match(r'\s*#\s*(define|undef|ifdef|ifndef)', lt): continue
                    fn = self.func_at(f, m.start())
                    expr, src1 = name, src
                    if fnlike and pidx is not None:
                        e = match_close(f.code, m.end() - 1, '(', ')')
                        args = Record.split_decls(f.code[m.end():e])
                        if pidx < len(args):
                            expr = ' '.join(args[pidx].split())[:80]
                            src1, _ = self.type_of(f, m.start(), fn, '(' + args[pidx] + ')')
                    used.add(name)
                    sites.append({'file': f.rel, 'line': f.line(m.start()), 'target': t, 'ref': '*', 'how': 'macro:' + name, 'expr': expr,
                                  'src': src1, 'off': drec['off'] if not fnlike else None, 'func': fn.name if fn else None, 'cls': fn.cls if fn else None,
                                  'va': fn.va if fn else None, 'macro': None})
        for name, v in macros.items():
            if name not in used: sites.append(v[5])
        for name, (t, src, drec) in accessors.items():
            pat = re.compile(r'(?<![\w.>:])' + re.escape(name) + r'\s*\(\s*\)')
            for f in self.files:
                for m in pat.finditer(f.code):
                    fn = self.func_at(f, m.start())
                    if fn and fn.name == name: continue
                    lt = f.code[f.text.rfind('\n', 0, m.start()) + 1:m.start()]
                    if re.search(r'\b(inline|__forceinline)\b', lt): continue
                    sites.append(dict(drec, file=f.rel, line=f.line(m.start()), how='accessor:' + name, expr=name + '()',
                                      func=fn.name if fn else None, cls=fn.cls if fn else None, va=fn.va if fn else None))
        return sites

    def uses(self, name):
        """by-value declarations, new: evidence the type is really constructed (one pass over src/, cached)."""
        if not hasattr(self, '_uses'):
            self._uses = collections.defaultdict(list)
            pat = re.compile(r'\bnew\s+(?:\(\w+\)\s*)?(?:\w+::)*(\w+)\b|(?<![\w:*&(<,.>])\s*\b(?:\w+::)*(\w+)\b[ \t]+(?!const\b)(\w+)\s*(?=\[|;|=|\(|\{|,)')
            for f in self.files:
                for m in pat.finditer(f.code):
                    t = m.group(1) or m.group(2)
                    if m.group(3) in KEYWORDS or t in KEYWORDS: continue
                    pre = f.code[max(0, m.start() - 12):m.start() + 1]
                    if re.search(r'(struct|class|union|typedef|friend|enum|return|goto|case)\s*$', pre): continue
                    if m.group(2):
                        # `T name(` at file or class scope is a function declaration, not an object
                        tail = f.code[m.end():m.end() + 1]
                        if tail == '(':
                            q = f.code[m.end():m.end() + 80]
                            if re.match(r'\(\s*(\)|(const\s+)?(\w+::)*\w+\s*[*&]*\s*\w*\s*[,)])', q) and self.func_at(f, m.start()) is None: continue
                    self._uses[t].append((f.rel, m.start()))
        hits = self._uses.get(name, [])
        return len(hits), ['%s:%d' % (r, self.by_rel[r].line(p)) for r, p in hits[:4]]


def row_status(ix, va):
    if va is None: return '-'
    if va in ix.nonm: return 'nonmatching'      # *.nonmatching.txt rows are not in the manifests
    return 'exact' if va in ix.rows else 'norow'


def path_terms():
    words = set()
    for p in PATH_DOCS:
        try: t = open(os.path.join(ROOT, p)).read()
        except OSError: continue
        words |= set(re.findall(r'\b(?:\w+::)*\w+\b', t))
        words |= set(re.findall(r'\b0x[0-9a-f]{6}\b', t))
    return words


# the native port's run path (port/INTEGRATION_STATUS.md "Where it gets to"): startup, load_module_files, the new-game
# sequence up to the first game menu. A function is on the path when the original's direct call graph
# (work/migrate/facts.json: calls + tail calls, no virtual calls) reaches it from these.
PATH_SEEDS = {0x4d3ec0: 'GameApp::GameApp', 0x4f63c0: 'GameApp::OneTimeSceneInit', 0x4c1940: 'GameApp::RestoreDeviceObjects',
              0x50d5e0: 'load_module_files', 0x5236f0: 'load_game_data', 0x51f1a0: 'start_new_game', 0x520b20: 'Game::build_map',
              0x5213d0: 'Game::build_map_trees', 0x520d60: 'Game::build_scene', 0x4ceb90: 'ItemA::s_last',
              0x48f020: 'FieldPanel::relayout'}


def path_reach():
    p = os.path.join(ROOT, 'work', 'migrate', 'facts.json')
    try: d = json.load(open(p))
    except (OSError, ValueError): return None
    g = {f['va']: set(f.get('calls', [])) | set(f.get('tails', [])) for f in d['funcs']}
    st = list(PATH_SEEDS); seen = set(st)
    while st:
        x = st.pop()
        for y in g.get(x, ()):
            if y not in seen: seen.add(y); st.append(y)
    return seen


INTISH = {'int', 'unsigned', 'unsignedint', 'DWORD', 'UINT', 'long', 'unsignedlong', 'BOOL', 'LONG', 'ULONG', 'int32_t', 'uint32_t', 'size_t', 'intptr_t', 'uintptr_t'}


def norm_type(t):
    t = re.sub(r'\b(const|volatile|struct|class|union|enum)\b|\b(rgl|std)::', '', t)
    t = re.sub(r'\s+', '', t)
    return re.sub(r'\[.*\]$', '[]', t)


GENERIC = {'iterator', 'const_iterator', 'T', 'U', 'value_type', 'reference', 'pointer', 'define', 'auto', 'Elem', 'E', 'V'}


def is_raw(t):
    return t in RAW or t in ('slot', 'char', 'void')


def classify(ix, r, info, recmap):
    """the triage class guess and its reason."""
    nm = r.name
    names = {f['name'] for f in r.fields}
    ptrfields = [f for f in r.fields if f['ptr'] and f['type'] != 'fnptr']
    vecshape = len(names & VECFIELD) >= 2 and sum(1 for f in r.fields if f['name'] in VECFIELD and '*' in f['type']) >= 2 and len(r.fields) <= 6
    stdbase = any(b.startswith('std::') for b in r.bases)
    if r.codegen:
        return 'F', 'comment ties it to cl 13.10 codegen of an exact row: ' + r.codegen[:90]
    if re.search(r'(Vt|Vtbl|VTable|_vt|_vtbl|Vtab)$', nm) or (r.virtual and not [f for f in r.fields if not f['pad']] and not r.ctor and info['casts'] and all(m['virtual'] or m['name'].startswith('~') for m in r.methods)):
        return 'E', 'virtual-slot view: calls through it stand for the real virtual interface'
    if r.fnptrs and r.fnptrs >= len(r.fields) - r.pads and r.fnptrs >= 2:
        return 'E', 'function-pointer table view'
    if vecshape or stdbase or re.search(r'^(raw)?vec|Vec\d*$|_vec$|Vector', nm) and len(names & VECFIELD) >= 1:
        return 'D', 'vector-shaped (%s)' % ('std:: base' if stdbase else '/'.join(sorted(names & VECFIELD)))
    real = info['real']
    if info['srcs'] and set(info['srcs']) <= {'slot', '?'} and 'slot' in info['srcs']:
        return 'A', 'a screen in g_windows.arr[n] (slot_as<T>(n)): fold into that slot\'s screen class (head_rec family, game/screen_records.h)'
    if not real:
        return 'C', 'overlays raw memory / no real record found' + (' (sources: %s)' % ','.join(info['srcs'][:3]) if info['srcs'] else '')
    rr = recmap.get(real)
    if info.get('conflicts'):
        return 'B', 'members of %s at the same offsets disagree: %s' % (real, '; '.join(info['conflicts'][:2]))
    if rr is not None and not info['fields_named'] and r.methods:
        return 'A', 'method twin: declare the %d method(s) on %s' % (len(r.methods), real)
    return 'A', 'fold into %s (%s)' % (real, 'pads cover the fields' if info.get('fits') else 'real type has no offsets there: check')


# hand triage (docs/views_inventory.md): name -> (class, reason[, file substring]); class '-' = not a view
OVERRIDES = {
    'MissionX': ('B', 'the script\'s Mission (157 mission() calls): EntryPoint[64] at +0x670 vs Mission\'s MarkRec[64], teams / mode / cam_frame to reconcile with game/mission.h, then mission() returns Mission*'),
    'GameX': ('A', 'method twin of GameData (91 game() calls): declare the script\'s names on GameData (include/game_data.h), game() returns GameData*'),
    'WVt': ('E', 'the widget vtable seen as 51 numbered slots over MissionUi / ContainerBase: give MissionUi the virtuals (d03 rebuild, d13 set value, d25 set_xy, d31 install mesh, kind, d50 add child) and call them directly'),
    'PsOps': ('E', 'second numbering of the widget vtable (PsFill chain, slots +0xa4..+0xc8) over MissionUi: same fix as WVt, one virtual interface'),
    'PsOpsW': ('E', 'PsOps with slot +0xa4 returning WRet: the same virtual under another signature; reconcile with WVt / PsOps first'),
    'ElemVt': ('F', 'declaration-only polymorphic class kept because cl 13.10 swaps two loads in Recf0::frame_update (0x4d63b0) without it; test removing it with run_all on video_page.cpp'),
    'contact_ops': ('F', 'D-shaped (VC7.1 vector<Contact> members written out) and load-bearing: the stock <vector> does not give 0x427450 / 0x427540 / 0x427590; the 64-bit build can use std::vector<Contact> under a non-MSVC branch'),
    'IntStdVec': ('D', 'std::vector<int>::_Insert_n made public over IntVec; keep the specialization declaration in int_vec_insert.h (its placement is codegen-relevant, see the header comment), make IntVec a std::vector<int>'),
    'lod_mesh': ('B', 'rgl::lod_mesh (rgl/mesh.h) is itself a view: its comment says the 0xe0-byte object is lod_group / lod_mesh2 (ctor 0x41c2a0); reconcile lists / m8c / mask with rgl/scene/scene.h lod_mesh2 and resmgr.h lod_group'),
    'PropEnt': ('A', 'a 32-bit-offset view of rgl::entity (docs/layout64_inventory.md: pad4[0x108 - 4], Vec of int words): mission_3 / terrain_a PlacedObj should read rgl::entity lods / meshes'),
    'PresentationRec': ('-', 'the real record (include/prsnt_rec.h, integration lane): reached through void* tables, not a view'),
    'Blob16': ('A', '16-byte block copy of Sel::rec[].a / .c (int[4]): give those members a 4-int struct type and assign it; no 64-bit effect'),
    'CamSlot': ('A', 'Mission +0x6748..+0x67f4 (CAMSLOT macro, 61 uses, all exact rows): mission.h has f6748 / frame_a / frame_b / blend_*; name them as CamSlot names them and make CamSlot a Mission member'),
    'GameData_s261e0': ('B', 'one of the per-slice GameData views (s261e0): +0x1a090 int map_editor vs GameData\'s member there; reconcile then use GameData'),
    'Scene_s2cf60': ('A', 'Mission with only +0x6698 nav named (g_scene_s2cf60, 88 sites in the map editor): Mission::nav exists; the sub-view gui_manager_s2cf60 is the editor\'s nav mesh (graph_obj)'),
}


def prio(i):
    return (-(i['wrong64'] == 'yes' and bool(i['on_path'])), -(i['wrong64'] == 'yes'), -i['casts'], i['name'])


def write_md(path, out):
    views = [i for i in out if i['view']]
    inc = sorted((i for i in out if not i['view'] and i['pads'] > 0), key=lambda i: (-i['pads'], i['name']))
    esc = lambda x: str(x).replace('|', '/').replace('\n', ' ')
    L = ['<!-- generated by tools/port/find_views.py --md docs/views_inventory.md: edit the tool (OVERRIDES), not this part -->', '']
    cc = collections.Counter(i['class'] for i in views)
    L.append('| class | views | 64-bit wrong (yes) | on the port path | cast sites |')
    L.append('|---|---:|---:|---:|---:|')
    for k in 'ABCDEF':
        vs = [i for i in views if i['class'] == k]
        L.append('| %s | %d | %d | %d | %d |' % (k, len(vs), sum(i['wrong64'] == 'yes' for i in vs), sum(bool(i['on_path']) for i in vs), sum(i['casts'] for i in vs)))
    L.append('| all | %d | %d | %d | %d |' % (len(views), sum(i['wrong64'] == 'yes' for i in views), sum(bool(i['on_path']) for i in views), sum(i['casts'] for i in views)))
    L.append('')
    titles = {'A': 'A. Fold into the real type', 'B': 'B. Real type exists but conflicts', 'C': 'C. Real type not yet defined',
              'D': 'D. Std container stand-ins', 'E': 'E. Fake vtable / slot views', 'F': 'F. Byte-match blocked'}
    for k in 'ABCDEF':
        vs = sorted((i for i in views if i['class'] == k), key=prio)
        L.append('### %s (%d)' % (titles[k], len(vs)))
        L.append('')
        L.append('| view | file:line | overlays | fields named | casts (files) | 64? | path | rows e/n/- | conf | notes |')
        L.append('|---|---|---|---|---|---|---|---|---|---|')
        for i in vs:
            fl = [re.sub(r'^\+0x[0-9a-f]+ ', '', x) for x in i['fields_named']]
            fields = ', '.join(fl[:4]) + (' +%d' % (len(fl) - 4) if len(fl) > 4 else '') if fl else ('%d methods' % i['methods'] if i['methods'] else '-')
            files = sorted({os.path.basename(f) for f in i['cast_files']})
            cf = '%d (%s%s)' % (i['casts'], ', '.join(files[:2]), ' +%d' % (len(files) - 2) if len(files) > 2 else '') if i['casts'] else '0'
            rs = i['row_status']
            rows = '%d/%d/%d' % (rs.get('exact', 0), rs.get('nonmatching', 0), rs.get('-', 0) + rs.get('norow', 0)) if rs else '-'
            real = i['real'] or '?'
            if i.get('real_def'): real += ' (%s)' % i['real_def'].replace('src/', '')
            onp = ','.join(i['on_path']) or '-'
            note = ('**' if i.get('curated') else '') + i['reason'] + ('**' if i.get('curated') else '')
            L.append('| %s | %s:%d | %s | %s | %s | %s | %s | %s | %s | %s |' % (esc(i['name']), i['file'].replace('src/', ''), i['line'], esc(real), esc(fields), esc(cf), i['wrong64'], onp, rows, i['confidence'], esc(note)))
        L.append('')
    L.append('### Incomplete real types (%d records with pads that are not views)' % len(inc))
    L.append('')
    L.append('Name (file:line) pads/pad bytes, by pad count. These are the classes the views above fold into, or records reached')
    L.append('only directly; their pads are the members still to name.')
    L.append('')
    by_dir = collections.defaultdict(list)
    for i in inc:
        by_dir[os.path.dirname(i['file']).replace('src/', '')].append(i)
    for d in sorted(by_dir, key=lambda d: -sum(i['pads'] for i in by_dir[d])):
        items = by_dir[d]
        L.append('- `%s` (%d): %s' % (d, len(items), ', '.join('%s (%s:%d) %d/%s' % (i['name'], os.path.basename(i['file']), i['line'], i['pads'], hex(i['pad_bytes']) if i['pad_bytes'] else '?') for i in items)))
    L.append('')
    L.append('<!-- end generated -->')
    gen = '\n'.join(L)
    try:
        doc = open(path).read()
    except OSError:
        doc = ''
    a, b = doc.find('<!-- generated by tools/port/find_views.py'), doc.find('<!-- end generated -->')
    doc = (doc[:a] + gen + doc[b + len('<!-- end generated -->'):]) if a >= 0 and b > a else (doc + ('\n' if doc else '') + gen + '\n')
    open(path, 'w').write(doc)
    print('wrote', os.path.relpath(path, ROOT))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--out', default=os.path.join(ROOT, 'work', 'views', 'views.json'))
    ap.add_argument('--top', type=int, default=30)
    ap.add_argument('--name')
    ap.add_argument('--md', help='rewrite the generated part of this markdown file (docs/views_inventory.md)')
    a = ap.parse_args()
    ix = Index()
    sites = ix.casts()
    # bind each site to the target record nearest the site
    by_target = collections.defaultdict(list)
    for s in sites:
        rec = ix.lookup(s['target'], ix.by_rel[s['file']])
        if rec is None: continue
        s['rec'] = rec.key()
        by_target[rec.key()].append(s)
    # `this` wrappers: methods of R that cast this to another record
    wraps = collections.defaultdict(list)
    for s in sites:
        if s['expr'] == 'this' and s['cls'] and s.get('rec'):
            rr = ix.lookup(s['cls'].split('::')[-1])
            if rr and rr.key() != s['rec']: wraps[rr.key()].append(s)
    terms = path_terms()
    reach = path_reach() or set()
    recmap = {}
    for r in ix.recs:
        # the fullest definition of a name stands for it (a TU-local stub of the same name loses)
        o = recmap.get(r.name)
        if o is None or (len(r.fields) + len(r.methods), r.f.rel.startswith('src/include/')) > (len(o.fields) + len(o.methods), o.f.rel.startswith('src/include/')):
            recmap[r.name] = r
    refs = collections.Counter()
    for f in ix.files: refs.update(re.findall(r'[A-Za-z_]\w*', f.code))
    def name_sig(r):
        sig = [lab for lab, p in VIEWNAME if p.search(r.name)]
        b0 = re.sub(r'X$', '', r.name)
        if r.name.endswith('X') and b0 != r.name and b0 in recmap: sig.append('twin')
        for b in r.bases:
            b1 = re.sub(r'<.*', '', b).split('::')[-1]
            if r.name != b1 and r.name.startswith(b1) and not [x for x in r.fields if not x['pad']]: sig.append('twin')
        return sig
    # pre-pass: names that look like views (by name, or cast to far more than they are otherwise named)
    likely = set()
    for r in ix.recs:
        n = len(by_target.get(r.key(), []))
        if name_sig(r) or (n and refs[r.name] <= 3 * n + 6): likely.add(r.name)
    out = []
    for r in ix.recs:
        k = r.key()
        cs = by_target.get(k, [])
        # drop casts from a derived type to a declared base and from the type itself
        def derived_from(t, base, d=0):
            if d > 5: return False
            x = ix.lookup(t, r.f)
            if x is None: return False
            for b in x.bases:
                b0 = re.sub(r'<.*', '', b).split('::')[-1]
                if b0 == base or derived_from(b0, base, d + 1): return True
            return False
        cs = [s for s in cs if s['src'] != r.name and not derived_from(s['src'], r.name) and not derived_from(r.name, s['src'].split('::')[-1])]
        # a cast from a view / twin back to this record is the view's evidence, not this record's
        cs_in = [s for s in cs if s['src'].split('::')[-1] not in likely or s['src'] == '?']
        srcs = collections.Counter(s['src'] for s in cs)
        named = [f for f in r.fields if not f['pad']]
        nm_sig = name_sig(r)
        base0 = re.sub(r'X$', '', r.name)
        if 'addr' in nm_sig and not (cs or r.pads): nm_sig.remove('addr')
        rec_srcs = [t for t in srcs if t != '?' and not is_raw(t)]
        raw_srcs = [t for t in srcs if is_raw(t)]
        cast_other = any(s['src'] != '?' for s in cs_in) or any(s['how'] in ix.helpers or s['how'].startswith('macro') for s in cs_in)
        # a record named far more often than it is cast to is a real type that is also reached through casts
        rare = refs[r.name] <= 3 * len(cs_in) + 6
        nused, uwhere = ix.uses(r.name) if (cs or nm_sig or wraps.get(k)) else (0, [])
        if r.ctor: nused += 1
        hasctor_out = r.name in ix.outctors
        constructed = nused > 0 or hasctor_out
        layout = r.pads > 0 or r.offcomments > 0
        view = False; why = []
        if cast_other and rare and not constructed and (layout or nm_sig or rec_srcs):
            view = True; why.append('cast-to, never constructed')
        if nm_sig and (layout or cs or wraps.get(k) or r.bases):
            view = True; why.append('name:' + ','.join(nm_sig))
        vnames = {f['name'] for f in r.fields if '*' in f['type'] and f['name'] in VECFIELD}
        if len(vnames) >= 2 and len(r.fields) <= 6 and refs[r.name] > 1 and not r.f.rel.startswith('src/port'):
            view = True; why.append('vector-shaped {alloc, first, last, end}')
        if 'vtable' in nm_sig and r.virtual:
            view = True; why.append('vtable-named polymorphic declaration')
        if wraps.get(k) and not r.fields and not constructed:
            view = True; why.append('this-wrapper')
        if r.f.rel.startswith('src/port') or '/port/' in r.f.rel: view = False
        # a record annotated with its own vtable address is an original class (it has an image vtable), not a view
        if view and not nm_sig and re.search(r'\bvtable 0x[0-9a-f]{6}', r.headcmt + ' ' + r.f.lines[r.line - 1]):
            view = False; why.append('own vtable: real class')
        # real type evidence
        real = None
        cand = collections.Counter()
        for t, c in srcs.items():
            if t != '?' and not is_raw(t) and t != r.name and t not in GENERIC: cand[t.split('::')[-1] if t.split('::')[-1] in recmap else t] += c * 2
        for h in r.hints:
            hh = h.split('::')[-1]
            if hh in recmap and hh != r.name: cand[hh] += 1
        for h in re.findall(r'\b((?:rgl::)?[A-Z][A-Za-z]*[0-9]*[A-Za-z0-9_]*)\b', r.headcmt):
            hh = h.split('::')[-1]
            if hh in recmap and hh != r.name and hh in recmap: cand[hh] += 2
        for b in r.bases:
            b0 = re.sub(r'<.*', '', b).split('::')[-1]
            if b0 in recmap: cand[b0] += 3
        if 'twin' in nm_sig and base0 in recmap: cand[base0] += 3
        for s in wraps.get(k, []): cand[s['target']] += 2
        if cand:
            real = max(cand, key=lambda t: (t.split('::')[-1] not in likely, cand[t], refs[t.split('::')[-1]]))
        rr = (recmap.get(real.split('::')[-1]) or ix.lookup(real, r.f)) if real else None
        if rr is not None and rr.key() == k: real, rr = None, None
        conflicts, fits = [], True
        base_off = collections.Counter(s['off'] for s in cs if s['off'] is not None and s['src'] == real)
        base_off = base_off.most_common(1)[0][0] if base_off else 0
        if rr is not None:
            roffs = {f['off']: f for f in rr.fields if f['off'] is not None}
            for f in named:
                if f['off'] is None: continue
                o = f['off'] + base_off
                g = roffs.get(o)
                if g and not g['pad'] and g['name'] != f['name'] and not re.match(r'^[a-z]{1,2}_?[0-9a-f]{1,5}$', g['name']):
                    t1, t2 = norm_type(f['type']), norm_type(g['type'])
                    if t1 != t2 and not ({t1, t2} <= INTISH) and not (t1.endswith('*') and t2.endswith('*') and ('void*' in (t1, t2))) \
                            and not ((t1 in INTISH and t2.endswith('*')) or (t2 in INTISH and t1.endswith('*'))):
                        conflicts.append('+0x%x %s %s vs %s %s' % (o, f['type'], f['name'], g['type'], g['name']))
                if not g: fits = fits and any(x['pad'] and x['off'] is not None and x['size'] and x['off'] <= o < x['off'] + x['size'] for x in rr.fields)
        # 64-bit
        wrong = 'no'
        after_pad = False; seen_pad = False
        for f in r.fields:
            if f['pad']: seen_pad = True
            elif seen_pad and (f['ptr'] or f.get('int_ptr')): after_pad = True
        rr_ptr = bool(rr and any(f['ptr'] for f in rr.fields))
        if any(f.get('int_ptr') for f in r.fields): wrong = 'yes'
        elif r.pads and (after_pad or (rr_ptr and named)): wrong = 'yes'
        elif r.pads and named: wrong = 'likely' if (rr is None or not rr.fields or rr.pads) else 'no'
        elif not r.pads and named and r.offcomments and any(f['ptr'] for f in named): wrong = 'likely'
        statuses = collections.Counter(row_status(ix, s['va']) for s in cs)
        on_path = []
        if r.name in terms and len(r.name) >= 5: on_path.append('doc')     # named in the port docs (short names are words)
        nreach = sum(1 for s in cs + wraps.get(k, []) if s['va'] in reach)
        if nreach: on_path.append('reach:%d' % nreach)
        info = {
            'name': r.name, 'qual': r.qual, 'kind': r.kind, 'file': r.f.rel, 'line': r.line, 'bases': r.bases,
            'pads': r.pads, 'pad_bytes': r.pad_bytes, 'offcomments': r.offcomments, 'size32': r.size32,
            'fields_named': [('+0x%x ' % f['off'] if f['off'] is not None else '') + f['name'] for f in named],
            'ptr_fields': [f['name'] for f in named if f['ptr']], 'int_ptr_fields': [f['name'] for f in named if f.get('int_ptr')],
            'refs': refs[r.name], 'methods': len(r.methods), 'virtual': r.virtual, 'ctor': r.ctor or hasctor_out, 'constructed': nused, 'constructed_at': uwhere,
            'name_signals': nm_sig, 'hints': sorted(r.hints), 'codegen': r.codegen,
            'casts': len(cs), 'srcs': [t for t, _ in srcs.most_common()], 'cast_files': sorted({s['file'] for s in cs}),
            'cast_funcs': sorted({'%s%s' % (s['func'] or '?', '@0x%x' % s['va'] if s['va'] else '') for s in cs})[:40],
            'row_status': dict(statuses), 'exact_sites': statuses.get('exact', 0),
            'wrapper_casts': len(wraps.get(k, [])), 'wrap_targets': sorted({s['target'] for s in wraps.get(k, [])}),
            'real': real, 'real_def': rr.key() if rr is not None else None, 'conflicts': conflicts, 'fits': fits,
            'wrong64': wrong, 'on_path': on_path, 'view': view, 'why': why,
        }
        if view:
            info['class'], info['reason'] = classify(ix, r, info, recmap)
            # confidence that this is a view and not a real record reached through casts
            core = r.f.rel.startswith(('src/include/', 'src/rgl/')) and not r.f.rel.startswith('src/rgl/xmlcfg/')
            if nm_sig or (rec_srcs and r.pads) or wraps.get(k):
                conf = 'high'
            elif core or refs[r.name] > 2 * len(cs) + 4 or (not rec_srcs and not r.pads):
                conf = 'low'
            else:
                conf = 'medium'
            info['confidence'] = conf
            ov = OVERRIDES.get(r.name)
            if ov and (len(ov) < 3 or ov[2] in r.f.rel):
                info['class'], info['reason'] = ov[0], ov[1]
                info['curated'] = True
                if ov[0] == '-': info['view'] = False
        out.append(info)
    if a.md:
        write_md(a.md, out)
        return
    if a.name:
        for i in out:
            if i['name'] == a.name:
                print(json.dumps(i, indent=1))
                for s in sites:
                    if s['target'] == a.name: print('  %s:%d %s %s <- %s (%s) [%s %s]' % (s['file'], s['line'], s['how'], s['target'] + s['ref'], s['expr'], s['src'], s['func'], row_status(ix, s['va'])))
        return
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    json.dump({'records': out, 'sites': sites}, open(a.out, 'w'), indent=0, default=list)
    views = [i for i in out if i['view']]
    inc = [i for i in out if not i['view'] and i['pads'] > 0]
    print('records %d, cast sites %d, views %d, incomplete real types (pads, not views) %d' % (len(out), len(sites), len(views), len(inc)))
    cc = collections.Counter(i['class'] for i in views)
    print('by class: ' + ', '.join('%s %d' % (k, cc[k]) for k in 'ABCDEF'))
    for k in 'ABCDEF':
        vs = sorted((i for i in views if i['class'] == k), key=prio)
        print('== %s (%d)' % (k, len(vs)))
        for i in vs[:a.top]:
            print('  %-28s %s:%d casts %d wrong64 %s path %s real %s' % (i['name'], i['file'], i['line'], i['casts'], i['wrong64'], ','.join(i['on_path']) or '-', i['real']))
    print('wrote', os.path.relpath(a.out, ROOT))


if __name__ == '__main__':
    main()
