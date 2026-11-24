#!/usr/bin/env python3
"""tools/port/link_tables.py: generate the portable link's tables (port/link/) from the MSVC flow's ownership data.
Ticket 004 milestone 2. Never edit port/link/* by hand: re-run this.

The MSVC link (tools/relink/build.py) resolves by original address what the clang link has to resolve by name:

  owners.tsv + weaken.cmake   a strong symbol several TUs define (a TU keeps its own copy of a helper next to the row's
                    real definition). build.py links the row's OWNER first, so /FORCE:MULTIPLE keeps the owner's copy.
                    The owner here is the TU whose manifest row (match/manifests/**/*.manifest.txt) or resolve_map defined_at row
                    names the symbol; with no row, the first defining TU in symbols/link_inputs.json's link_order (the
                    copy /FORCE:MULTIPLE keeps). Every other copy is weakened (llvm-objcopy --weaken-symbol, applied by
                    port/CMakeLists.txt through mb_weaken(); COFF has no --weaken-symbol, so there tools/port/coff.py
                    does the equivalent surgery: local copy, references rebound to the owner).
  aliases.ld        a name src/ declares and calls but defines under another name at the same original address: glue.py's
                    /alternatename aliases (link_inputs.json "aliases"; resolve_map "names" -> "defined_at" when a
                    work/relink build exists). PROVIDE(alias = target) in a linker script (-Wl,-T: PROVIDE only binds
                    a name nothing defines; the default layout stays).
  emit_inline.cmake a name src/ calls in one TU and defines only as an inline (or implicit) function in another: cl
                    emitted the out-of-line COMDAT (the MSVC map names the object), clang inlined every use and emitted
                    nothing. mb_emit_inline() compiles that TU with -fno-inline so the out-of-line copies exist.
  stl_instances.cpp an STL member src/ declares as an explicit specialization and defines nowhere (the MSVC link folds it
                    onto another instance): explicitly instantiated from port/stl in a port TU that never sees the
                    specialization's declaration.
  traps.cpp         what is left: names with no definition anywhere for this ABI (Itanium `delete[]` calls the complete
                    destructor directly where MSVC goes through the vtable's vector deleting destructor, a constructor
                    the source never located). Each is a weak symbol that reports its name and aborts (port_link_trap).
                    These are the run blockers this table tracks; the count is burndown.py's "trapped". It must stay 0:
                    the script exits 1 when any name is trapped (--allow-traps only for the first pass below).

usage: tools/port/link_tables.py [--target linux-i686] [--resolve-map work/relink/<build>/resolve_map.json] [--check]
                                [--allow-traps]
       Two passes reach the fixpoint after an emit_inline change: link_tables.py --allow-traps; cmake --build;
       link_tables.py.
"""
import argparse, collections, glob, json, os, re, shlex, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, os.path.join(ROOT, "tools", "common"))
import sidecars as SC          # noqa: E402  (where the sidecar files live)
sys.path.insert(0, os.path.join(ROOT, "tools", "port"))
import coff                    # noqa: E402  (COFF COMDAT detection; the weaken surgery mb_weaken() runs on Windows)
LLVM = '/opt/homebrew/opt/llvm/bin/'
OUT = os.path.join(ROOT, 'port', 'link')   # linux-i686; another target writes port/link/<target>/ (main())
MACHO = False
LP64 = False
COFF = False


def tool(name):
    p = os.path.join(LLVM, name)
    return p if os.path.exists(p) else name


def run_filter(name, names):
    names = list(names)
    if not names:
        return {}
    out = subprocess.run([tool(name)], input='\n'.join(names) + '\n', capture_output=True, text=True).stdout.splitlines()
    if name == 'llvm-undname':                  # echoes each input line, then the result, then a blank line
        res, i, want = {}, 0, set(names)
        while i < len(out):
            if out[i] in want and i + 1 < len(out):
                res[out[i]] = out[i + 1]
                i += 2
            else:
                i += 1
        return res
    return dict(zip(names, out))


def split_top(s):
    parts, depth, cur = [], 0, ''
    for ch in s:
        if ch in '<([':
            depth += 1
        elif ch in '>)]':
            depth -= 1
        if ch == ' ' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return [p for p in parts if p]


CC_RE = re.compile(r'\b__(cdecl|stdcall|fastcall|thiscall)\b')


def key(d):
    """One spelling for an MSVC (llvm-undname) or Itanium (llvm-cxxfilt) demangling: qualified name + parameters."""
    s = re.sub(r'\b(public|private|protected): ', '', d)
    s = re.sub(r'\b(virtual|static) ', '', s)
    s = CC_RE.sub('', s)
    s = re.sub(r'\b(struct|class|union|enum) ', '', s)
    s = s.replace('__int64', 'long long').replace('(void)', '()')
    s = re.sub(r'\bfalse\b', '0', re.sub(r'\btrue\b', '1', s))   # bool template arguments: undname prints 0 / 1
    if LP64:                                   # size_t / ptrdiff_t: `unsigned int` / `int` for cl, `unsigned long` / `long` here
        s = re.sub(r'\bunsigned long\b(?! long)', 'unsigned int', s)
        s = re.sub(r'(?<!unsigned )(?<!long )\blong\b(?! long)', 'int', s)
    s = re.sub(r'\s+', ' ', s).strip()
    parts = split_top(s)
    core = None
    for i, p in enumerate(parts):
        if '(' in p and not p.startswith('('):
            core = ' '.join(parts[i:])
            break
    if core is None:
        core = parts[-1] if parts else s
    return core.replace(' ', '')


def port_abi(d):
    """(convention, stack-or-register words) the clang i686 build gives the function an MSVC demangling describes.
    Members are cdecl with `this` as the first stack argument on a non-Windows target (src/ writes no __thiscall on them);
    explicit __stdcall / __fastcall / __cdecl keep their meaning. Argument words are counted roughly (one per parameter)."""
    m = CC_RE.search(d)
    cc = m.group(1) if m else 'data'
    k = key(d)
    i = k.find('(')
    params = k[i + 1:k.rfind(')')] if i >= 0 else ''
    n = len([p for p in split_top(params.replace(',', ', ')) if p]) if params else 0
    member = cc == 'thiscall'
    return ('cdecl' if member else cc, n + (1 if member else 0))


def variant(n):
    """Itanium constructor/destructor variant (C1/C2/D0/D1/D2) of a mangled name, '' for other names."""
    m = re.search(r'(C[123]|D[012])E', n)
    return m.group(1) if m else ''


def objects(build):
    with open(os.path.join(build, 'compile_commands.json')) as f:
        cc = json.load(f)
    res = {}
    src_prefix = os.path.join(ROOT, 'src') + os.sep
    for e in cc:
        if not e['file'].startswith(src_prefix):   # only the reconstruction's TUs, not work/port_deps/src/ (Jolt)
            continue
        args = e.get('arguments') or shlex.split(e['command'])
        o = os.path.join(e['directory'], args[args.index('-o') + 1])
        if os.path.exists(o):
            res[os.path.relpath(e['file'], ROOT)] = o
    return res


def symbols(objs):
    out = subprocess.run([tool('llvm-nm'), '-A', '-P', '-g'] + list(objs.values()), capture_output=True, text=True).stdout
    back = {o: s for s, o in objs.items()}
    weak_macho = set()                      # (object, name): llvm-nm -P prints a Mach-O weak definition as 'T'; -m says "weak external"
    if MACHO:
        mout = subprocess.run([tool('llvm-nm'), '-A', '-m', '-g'] + list(objs.values()), capture_output=True, text=True).stdout
        for line in mout.splitlines():
            m = re.match(r'^(.*?\.o): .*\bweak external\b.* (\S+)$', line)
            if m and '(undefined)' not in line:
                weak_macho.add((m.group(1), m.group(2)))
    # COFF: llvm-nm prints a COMDAT definition as plain T/B/D, but the linker folds COMDATs by itself (the section's
    # aux section definition has a nonzero Selection), so a COMDAT copy is weak, never a duplicate needing a weakened owner.
    comdat = {o: coff.comdat_names(o) for o in objs.values()} if COFF else {}
    strong, defined, undef = collections.defaultdict(set), collections.defaultdict(set), collections.defaultdict(set)
    comdat_def = collections.defaultdict(set)   # COFF: name -> TUs whose definition of it is COMDAT
    for line in out.splitlines():
        m = re.match(r'^(.*): (\S+) (\w)', line)      # .o on ELF/Mach-O, .obj on COFF
        if not m or m.group(1) not in back:
            continue
        o, n, t = m.groups()
        if (o, n) in weak_macho:
            t = 'W'
        if t != 'U' and n in comdat.get(o, ()):
            t = 'W'
            comdat_def[n].add(back[o])
        if MACHO and n.startswith('_'):
            n = n[1:]                       # Mach-O C-level names carry a leading underscore
        src = back[o]
        if t == 'U':
            undef[n].add(src)
        else:
            defined[n].add(src)
            if t not in 'WwVv':                 # every other global definition is strong (Mach-O __const data prints as 'S')
                strong[n].add(src)
    return strong, defined, undef, comdat_def


def manifest_owners():
    """MSVC name -> src path, from every manifest row (the byte-exact rows name their symbol)."""
    own = {}
    for mf in sorted(SC.all_manifests()):
        src = os.path.relpath(SC.source_for(mf), ROOT)
        for line in open(mf, errors='replace'):
            f = line.split()
            if len(f) >= 3 and not line.startswith('#'):
                own.setdefault(f[1], src)
    return own


def work_dirs():
    """work/ of this checkout, then of the main checkout (a git worktree has no work/ of its own)."""
    dirs = [os.path.join(ROOT, 'work')]
    common = subprocess.run(['git', '-C', ROOT, 'rev-parse', '--path-format=absolute', '--git-common-dir'],
                            capture_output=True, text=True).stdout.strip()
    if common:
        dirs.append(os.path.join(os.path.dirname(common), 'work'))
    return dirs


def default_resolve_map():
    """The newest unpatched MSVC build's resolve_map.json (the faithful flow's ownership is the one src/ alone defines)."""
    for w in work_dirs():
        maps = [m for m in glob.glob(os.path.join(w, 'relink', '*', 'resolve_map.json'))
                if os.path.basename(os.path.dirname(m)) in ('standalone', 'faithful')]
        if maps:
            return max(maps, key=os.path.getmtime)
    return None


def read_map(path):
    """MSVC map: symbol -> module stem of the object whose copy the link kept."""
    got = {}
    if path and os.path.exists(path):
        for line in open(path, errors='replace'):
            m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+(?:f\s+)?(?:i\s+)?(\S+)\.obj', line)
            if m:
                got.setdefault(m[1], m[2].split(':')[-1].rsplit('_', 1)[0])
    return got


# ---- STL members: explicit instantiation of port/stl's primary template in a port TU ------------------------------
# Only for element types a port TU can name without src/ headers (pointers to forward-declared structs, built-in types).
STL_MEMBER = re.compile(r'^(?:\S+ )?std::(vector|list)<(.+?), std::allocator<.+?>>::(\w+)\((.*)\)( const)?$')
PTR_ELEM = re.compile(r'^(?:(\w+(?:::\w+)*)\s*\*|int|unsigned int|char|float)$')
STL_RET = {'_Buy': 'bool', '_Tidy': 'void', 'push_back': 'void', 'resize': 'void'}


# the allocator fill of a pointer vector's push_back (src/ declares the specialization; 0x40f8a0 in the original)
STL_FILL = re.compile(r'^void std::(_Uninitialized_fill_n)<((\w+(?:::\w+)*)\*\*), (unsigned int|unsigned long), \3\*, std::allocator<\3\*>>\((.*)\)$')


def stl_instance(d):
    m = STL_FILL.match(d)
    if m:
        return m
    m = STL_MEMBER.match(d)
    if m and PTR_ELEM.match(m.group(2).strip()) and (m.group(3) in STL_RET or m.group(3) in ('_Buynode', 'capacity')):
        if m.group(3) == 'resize' and not COFF:
            # ELF/Mach-O (LP64) spell size_type `unsigned long`, which the aliases already bind to another
            # instance; only COFF's LLP64 `unsigned long long` needs an explicit instantiation here.
            return None
        return m
    return None


def render_stl(hdr, insts):
    fwd, lines = set(), []
    for n, m in insts:
        if m.re is STL_FILL:
            fwd.add(m.group(3))
            lines.append('template void std::%s<%s, %s, %s*, std::allocator<%s*> >(%s);   // %s'
                         % (m.group(1), m.group(2), m.group(4), m.group(3), m.group(3), m.group(5), n))
            continue
        cont, elem, member, params, const = m.group(1), m.group(2).strip(), m.group(3), m.group(4), bool(m.group(5))
        pm = re.match(r'^(\w+(?:::\w+)*)\s*\*$', elem)
        if pm and pm.group(1) not in ('char', 'int', 'float', 'double', 'short', 'long', 'bool', 'void'):
            fwd.add(pm.group(1))   # a built-in pointee (std::vector<char*>) needs no forward declaration
        sig = 'std::%s<%s >' % (cont, elem)
        if member == '_Buynode':
            lines.append('template %s::_Nodeptr %s::_Buynode();' % (sig, sig))
        elif member == '_Buy':          # size_type, not the 32-bit demangling's `unsigned int` (64-bit targets)
            lines.append('template bool %s::_Buy(%s::size_type);' % (sig, sig))
        elif member == 'capacity':
            lines.append('template %s::size_type %s::capacity() const;' % (sig, sig))
        elif member == 'resize':        # a pointer vector resized to N copies of a value; size_type, not the demangled width
            lines.append('template void %s::resize(%s::size_type, %s);' % (sig, sig, elem))
        else:
            lines.append('template %s %s::%s(%s)%s;' % (STL_RET[member], sig, member, params, ' const' if const else ''))
        lines[-1] += '   // ' + n
    out = ['// port/link/stl_instances.cpp: ' + hdr,
           '// STL members src/ declares as explicit specializations and defines nowhere (the MSVC link takes them from another',
           '// instance at the same original address). This TU does not see those declarations, so each explicit instantiation',
           '// below emits port/stl\'s own implementation under the same name.',
           '#include <vector>', '#include <list>', '']
    for f in sorted(fwd):
        parts = f.split('::')
        out.append(''.join('namespace %s { ' % p for p in parts[:-1]) + 'struct %s;' % parts[-1] + ' }' * (len(parts) - 1))
    return '\n'.join(out + [''] + lines) + '\n'


def render_traps(hdr, traps, dem):
    if COFF:
        # COFF has no linker scripts: aliases.ld's PROVIDE lines become /alternatename (port/cmake/native_link.cmake),
        # binding each name to its stub only while nothing defines it -- the same "whatever later defines it wins" as
        # the weak placeholders the ELF / Mach-O targets use.
        out = ['// port/link/traps.cpp: ' + hdr,
               '// Names src/ calls that have no definition for this ABI (see tools/port/link_tables.py). Each binds to a stub',
               '// here only while nothing defines it (aliases.ld -> /alternatename). Reaching one calls port_link_trap(name)',
               '// (port/runtime/link_trap.cpp), which reports it and aborts. tables.json "trapped" lists them; burndown.py',
               '// counts them as the link\'s open names.',
               'extern "C" void port_link_trap(const char* name) __attribute__((noreturn));']
        for i, n in enumerate(traps):
            s = dem.get(n, n).replace('\\', '\\\\').replace('"', '\\"')
            out += ['', 'extern "C" void mb_trap_%d()' % i, '{', '    port_link_trap("%s");' % s, '}']
        return '\n'.join(out) + '\n'
    out = ['// port/link/traps.cpp: ' + hdr,
           '// Names src/ calls that have no definition for this ABI (see tools/port/link_tables.py). Each is a weak symbol,',
           '// so whatever later defines it wins. Reaching one calls port_link_trap(name) (port/runtime/link_trap.cpp), which',
           '// reports it and aborts. tables.json "trapped" lists them; burndown.py counts them as the link\'s open names.',
           'extern "C" void port_link_trap(const char*);',
           'asm(']
    for i, n in enumerate(traps):
        out += ['    // %s' % dem.get(n, n),
                '    "    .text\\n"',
                '    "    .weak %s\\n"' % n, '    "    .type %s,@function\\n"' % n, '    "%s:\\n"' % n,
                '    "    pushl $.Lmb_trap_name%d\\n"' % i, '    "    call port_link_trap\\n"']
    out.append('    "    .section .rodata.str1.1,\\"aMS\\",@progbits,1\\n"')
    for i, n in enumerate(traps):
        s = dem.get(n, n).replace('\\', '\\\\').replace('"', '\\\\\\"')
        out.append('    ".Lmb_trap_name%d: .asciz \\"%s\\"\\n"' % (i, s))
    out.append('    "    .text\\n");')
    return '\n'.join(out) + '\n'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target', default='linux-i686')
    ap.add_argument('--resolve-map', help='a work/relink/<build>/resolve_map.json (default: the newest standalone/faithful '
                                          'build; its mb_standalone.map is read too)')
    ap.add_argument('--check', action='store_true', help='exit 1 if the generated files would change')
    ap.add_argument('--allow-traps', action='store_true',
                    help='exit 0 although names are trapped (only for the first pass of an emit_inline change, whose '
                         'pending names the rebuild defines); without it any trapped name fails the run')
    a = ap.parse_args()
    build = os.path.join(ROOT, 'build', 'port', a.target)
    global OUT, MACHO, LP64, COFF
    if a.target != 'linux-i686':
        OUT = os.path.join(ROOT, 'port', 'link', a.target)
    MACHO = a.target.startswith('macos')
    LP64 = a.target != 'linux-i686'
    COFF = a.target.startswith('windows')
    objs = objects(build)
    if not objs:
        sys.exit('no objects under %s: build port/ first' % build)
    strong, defined, undef, comdat_def = symbols(objs)
    inp = json.load(open(os.path.join(ROOT, 'symbols', 'link_inputs.json')))
    stem_rank = {s: i for i, s in enumerate(inp['link_order'])}
    stem = lambda p: os.path.splitext(os.path.basename(p))[0]
    by_stem = collections.defaultdict(list)
    for s in objs:
        by_stem[stem(s)].append(s)
    tu_of_stem = lambda st: by_stem[st][0] if len(by_stem.get(st, [])) == 1 else None

    rmap_path = a.resolve_map or default_resolve_map()
    rmap = json.load(open(rmap_path)) if rmap_path and os.path.exists(rmap_path) else {}
    msvc_map = read_map(os.path.join(os.path.dirname(rmap_path), 'mb_standalone.map') if rmap else None)
    owners = manifest_owners()                          # msvc name -> src path
    for va, (sym, st) in sorted(rmap.get('defined_at', {}).items()):
        if sym not in owners and tu_of_stem(st):
            owners[sym] = tu_of_stem(st)
    msvc_to_target = dict(inp['aliases'])               # alias msvc name -> target msvc name
    def_at = rmap.get('defined_at', {})
    for m, vas in sorted(rmap.get('names', {}).items()):
        if m not in msvc_to_target and len(vas) == 1:
            va = next(iter(vas))
            if va in def_at and def_at[va][0] != m:
                msvc_to_target[m] = def_at[va][0]
    msvc_names = set(owners) | set(msvc_to_target) | set(msvc_to_target.values()) | set(msvc_map)
    mdem = run_filter('llvm-undname', sorted(n for n in msvc_names if n.startswith('?')))
    mkey = collections.defaultdict(set)                 # key -> msvc names
    for n, d in mdem.items():
        mkey[key(d)].add(n)

    all_names = sorted(set(defined) | set(undef))
    idem = run_filter('llvm-cxxfilt', all_names)
    ikey = collections.defaultdict(set)                 # key -> itanium names any object defines or references
    for n in all_names:
        ikey[key(idem.get(n, n))].add(n)

    # ---- duplicates ---------------------------------------------------------------------------------------------
    dups = sorted(n for n, s in strong.items() if len(s) > 1)
    owner_rows, weaken = [], collections.defaultdict(set)
    for n in dups:
        tus = sorted(strong[n])
        k = key(idem.get(n, n))
        cands = {owners[m] for m in mkey.get(k, ()) if m in owners}
        own = [t for t in tus if t in cands]
        if len(own) == 1:
            o, basis = own[0], 'row owner'
        else:
            o = min(tus, key=lambda t: stem_rank.get(stem(t), 1 << 20))
            basis = 'first in link_order' + (' (rows in %d TUs)' % len(own) if own else ' (no row names it)')
        for t in tus:
            if t != o:
                weaken[t].add(n)
        owner_rows.append((n, o, [t for t in tus if t != o], basis, idem.get(n, n)))
    # COFF (zig's lld-link): unlike ELF/Mach-O, it does NOT fold a COMDAT definition against a plain strong
    # definition in another object -- it reports a duplicate (probed work/winprobe2/cwprobe: a -fno-inline inline
    # copy of a function next to the src TU that defines it out of line). src/'s inline helpers are emitted as
    # COMDAT copies in every TU -fno-inline touches; where a TU also defines one strongly (the owner above), the
    # COMDAT copies must be made local too, or the link fails. Weakening keeps the owner's strong copy and rebinds
    # references, exactly as for a strong duplicate.
    if COFF:
        for n in sorted(comdat_def):
            if n not in strong:                 # no strong definition: the linker folds the COMDATs among themselves
                continue
            for t in sorted(comdat_def[n]):
                if t not in strong[n]:          # a COMDAT copy in a TU that also defines it strongly is already weakened above
                    weaken[t].add(n)

    # ---- unresolved src/ names ----------------------------------------------------------------------------------
    # A TU already in emit_inline.cmake defines its inline functions weakly only BECAUSE of -fno-inline: those weak copies do
    # not count as definitions here, or the next run would drop the TU and the names would be undefined again.
    emit_now, emit_listed = set(), set()          # the TUs, and the (demangled) names they are there for
    ep = os.path.join(OUT, 'emit_inline.cmake')
    if os.path.exists(ep):
        for line in open(ep):
            m = re.match(r'^mb_emit_inline\((\S+)\)', line)
            if m:
                emit_now.add(m.group(1))
            elif line.startswith('#   '):
                emit_listed.add(line[4:].strip())
    # A weak definition in such a TU still counts when a manifest row names that TU as the symbol's owner and the TU is
    # not in emit_inline.cmake for that name: an explicit instantiation (`template class std::vector<X>;`, `template T
    # f<X>(...)`) is weak in ELF too, and it is the real one whatever -fno-inline does.
    def row_owned(n, tus):
        return (idem.get(n, n) not in emit_listed
                and any(owners.get(m) in tus for m in mkey.get(key(idem.get(n, n)), ())))
    counted = {n for n, tus in defined.items()
               if (tus - emit_now) or (strong.get(n, set()) & emit_now) or row_owned(n, tus & emit_now)}
    # COFF: lld-link folds a lone COMDAT (no plain strong competitor) to one copy, so a COMDAT definition resolves
    # its references. src/'s -fno-inline TUs also emit incidental COMDAT copies of inline members (a vector's
    # operator[] next to the one name that TU is in emit_inline.cmake for); those are real definitions, not the
    # pending emit_inline names, so they count. The pending ones stay unresolved until the rebuild defines them.
    if COFF:
        counted |= {n for n in comdat_def if n not in strong and idem.get(n, n) not in emit_listed}
    unres = sorted(set(undef) - counted)
    runtime = re.compile(r'__cxxabiv1|typeinfo for std::|vtable for std::|std::(exception|bad_alloc|nothrow|terminate|bad_exception)'
                         r'|operator (new|delete)')
    src_unres = [n for n in unres if n.startswith('_Z') and not runtime.search(idem.get(n, n))]
    insts = [(n, stl_instance(idem.get(n, n))) for n in src_unres if stl_instance(idem.get(n, n))]
    inst_names = {n for n, _ in insts}
    emit = collections.defaultdict(set)                 # TU -> names it must emit out of line

    def emitter(n):
        """The TU whose inline definition of n cl emitted (MSVC map), if any."""
        for m in sorted(mkey.get(key(idem.get(n, n)), ())):
            tu = tu_of_stem(msvc_map.get(m, ''))
            if tu:
                return tu
        return None

    def available(n):
        return n in defined or n in inst_names

    alias_rows, left = [], []
    for n in src_unres:
        if n in inst_names:
            continue
        d = idem.get(n, n)
        target = None
        for m in sorted(mkey.get(key(d), ())):
            t = msvc_to_target.get(m)
            if not (t and t in mdem):
                continue
            hits = sorted(ikey.get(key(mdem[t]), ()))
            if len(hits) > 1:
                hits = [h for h in hits if variant(h) == variant(n)] or hits
            if len(hits) == 1:
                target = (m, t, hits[0])
                break
        if target and not available(target[2]):
            tu = emitter(target[2])
            if tu:
                emit[tu].add(target[2])
            else:
                target = None
        if target:
            alias_rows.append((n, target))
            continue
        tu = emitter(n)
        if tu:
            emit[tu].add(n)
        else:
            left.append(n)
    # an emit_inline entry only counts once the rebuilt objects define the name; until then it is trapped too
    pending = sorted(x for t in emit for x in emit[t] if x not in defined)
    traps = sorted(set(left) | ({n for n, (m, t, it) in alias_rows if it in pending}) | (set(pending) & set(src_unres)))

    # ---- write --------------------------------------------------------------------------------------------------
    os.makedirs(OUT, exist_ok=True)
    rel_rmap = None
    if rmap:
        rel_rmap = 'work/' + os.path.relpath(rmap_path, os.path.dirname(os.path.dirname(os.path.dirname(rmap_path))))
    hdr = ('generated by tools/port/link_tables.py from %s (do not edit; re-run the script)'
           % ', '.join(['the port objects', 'the match/manifests sidecars', 'symbols/link_inputs.json'] +
                       ([rel_rmap, rel_rmap.replace('resolve_map.json', 'mb_standalone.map')] if rel_rmap else [])))
    files = {}
    lines = ['# port/link/owners.tsv: ' + hdr,
             '# one row per strong symbol defined in more than one TU: symbol, owner TU (keeps its copy), weakened TUs, '
             'basis, demangled']
    lines += ['\t'.join([n, o, ','.join(w), b, d]) for n, o, w, b, d in owner_rows]
    files['owners.tsv'] = '\n'.join(lines) + '\n'

    lines = ['# port/link/weaken.cmake: ' + hdr,
             '# mb_weaken(<src TU> <symbol>...): ' + (
                 'the TU\'s object goes through tools/port/coff.py weaken, which makes its copy local and rebinds the'
                 ' references to the owner (COFF has no --weaken-symbol)' if COFF else
                 'the TU\'s object is linked through llvm-objcopy --weaken-symbol for each name'),
             '# (owners.tsv says who keeps the strong copy and why)']
    for t in sorted(weaken):
        lines.append('mb_weaken(%s\n    %s)' % (t, '\n    '.join(('_' if MACHO else '') + n for n in sorted(weaken[t]))))   # object-file spelling
    files['weaken.cmake'] = '\n'.join(lines) + '\n'

    lines = ['# port/link/emit_inline.cmake: ' + hdr,
             '# mb_emit_inline(<src TU>): compiled with -fno-inline, so the inline / implicit functions other TUs call',
             '# (cl emitted them out of line in this object, per the MSVC map) exist out of line here too']
    for t in sorted(emit):
        lines += ['mb_emit_inline(%s)' % t] + ['#   %s' % idem.get(x, x) for x in sorted(emit[t])]
    files['emit_inline.cmake'] = '\n'.join(lines) + '\n'

    lines = ['/* port/link/aliases.ld: ' + hdr + ' */',
             '/* src/ declares and calls these names; the MSVC link binds each to the function at the same original address',
             '   under another name (glue.py /alternatename). PROVIDE binds only a name nothing defines.',
             '   ABI marks a pair whose convention or argument count differs on the clang i686 build (members are cdecl',
             '   there, `this` first on the stack): it links, but needs a thunk before that path runs. */']
    abi_bad = []
    for n, (m, t, it) in alias_rows:
        a1, a2 = port_abi(mdem[m]), port_abi(mdem[t])
        if a1 != a2:
            abi_bad.append(n)
        lines.append('/* %s  ->  %s%s */' % (idem.get(n, n), idem.get(it, it),
                                             '  ABI: %s/%d vs %s/%d' % (a1 + a2) if a1 != a2 else ''))
        lines.append('PROVIDE(%s = %s);' % (n, it))
    if COFF and traps:
        lines.append('/* trapped names (nothing defines them for this ABI): each binds to its stub in traps.cpp */')
        for i, n in enumerate(traps):
            lines.append('PROVIDE(%s = mb_trap_%d);' % (n, i))
    files['aliases.ld'] = '\n'.join(lines) + '\n'
    files['stl_instances.cpp'] = render_stl(hdr, insts)
    files['traps.cpp'] = render_traps(hdr, traps, idem)

    rep = {'_note': 'summary of the generated tables; ' + hdr,
           'duplicates': len(dups), 'weakened_tus': len(weaken),
           'owner_basis': dict(collections.Counter(r[3].split(' (')[0] for r in owner_rows)),
           'src_unresolved': len(src_unres), 'aliased': len(alias_rows), 'aliases_abi_mismatch': [idem.get(n, n) for n in abi_bad],
           'stl_instantiated': len(insts), 'emit_inline_tus': len(emit),
           'emitted_inline': sum(len(v) for v in emit.values()) - len(pending),
           'trapped': [idem.get(n, n) for n in traps]}
    files['tables.json'] = json.dumps(rep, indent=1) + '\n'
    changed = []
    for fn, text in files.items():
        p = os.path.join(OUT, fn)
        old = open(p).read() if os.path.exists(p) else None
        if old != text:
            changed.append(fn)
            if not a.check:
                open(p, 'w').write(text)
    print('duplicates %d (owner basis: %s) -> %d TUs weakened'
          % (len(dups), ', '.join('%s %d' % kv for kv in rep['owner_basis'].items()), len(weaken)))
    print('src/ unresolved %d: %d aliased (%d ABI mismatch), %d STL members instantiated, %d emitted by %d -fno-inline TUs'
          '%s, %d trapped'
          % (len(src_unres), len(alias_rows), len(abi_bad), len(insts), rep['emitted_inline'], len(emit),
             ' (%d pending a rebuild)' % len(pending) if pending else '', len(traps)))
    for d in rep['trapped']:
        print('   trapped:', d)
    print(('would change: ' if a.check else 'wrote: ') + (', '.join(changed) or 'nothing'))
    if a.check and changed:
        sys.exit(1)
    if traps and not a.allow_traps:
        # zero traps is the invariant (2026-11-20): a trapped name is a function src/ calls and the port does not define
        # (a stub by another name). Define it for real (an out-of-line definition, an explicit instantiation, an inline
        # body in the header) instead of letting traps.cpp abort at run time.
        sys.exit('FAIL: %d trapped name(s)%s: give each a real definition (see the list above)'
                 % (len(traps), ' (%d pending a rebuild: build port/ and re-run)' % len(pending) if pending else ''))


if __name__ == '__main__':
    main()
