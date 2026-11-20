#!/usr/bin/env python3
"""port64_ast.py: run tools/port/port64_ast (a clang libTooling pass) over every src/ TU at 64 and 32 bits, cached.

    port64_ast.py [--root DIR] [--only SUBSTR] [--jobs N]      -> work/port64/<root tag>/ast/{64,32}/<tu>.json

The 64-bit pass uses build/port/macos-arm64's compile command (the port's flags) with the 64-bit warnings on; the
32-bit pass is the same command for i686-linux-gnu with zig's glibc headers and -malign-double (MSVC's record rules,
as build/port/linux-i686), where MB_LAYOUT32_ASSERT is live. Every src/*.cpp of --root is a TU (the flags are the
same for all of them, plus port/tu/<path>.h when it exists), so an older checkout (a git worktree) works too.

Cache: a TU is re-run when its command or any file it read last time (path, mtime, size) changed, or the tool
binary changed. The tool is rebuilt when port64_ast.cpp is newer than the binary (Homebrew LLVM, ~20 s).
Used by tools/port/port64_worklist.py; importable (run(root, jobs) -> {tu: {64: path, 32: path}}).
"""
import argparse, glob, hashlib, json, os, shlex, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
LLVM = '/opt/homebrew/opt/llvm'
TOOL_SRC = os.path.join(HERE, 'port64_ast', 'port64_ast.cpp')
TOOL = os.path.join(ROOT, 'work', 'port64', 'bin', 'port64_ast')
WARN64 = ['-Wshorten-64-to-32', '-Wpointer-to-int-cast', '-Wint-to-pointer-cast', '-Wvoid-pointer-to-int-cast',
          '-Wint-to-void-pointer-cast', '-Wcast-align', '-Wcast-function-type-mismatch', '-Wformat',
          '-Wsizeof-pointer-memaccess', '-Wmicrosoft-cast', '-Wmicrosoft-template']
ZIG_INC = None


def build_tool():
    if os.path.exists(TOOL) and os.path.getmtime(TOOL) >= os.path.getmtime(TOOL_SRC):
        return
    os.makedirs(os.path.dirname(TOOL), exist_ok=True)
    sys.stderr.write('port64_ast: building the clang tool\n')
    subprocess.run([LLVM + '/bin/clang++', '-std=c++17', '-O1', '-I' + LLVM + '/include', '-fno-rtti', '-fno-exceptions',
                    TOOL_SRC, '-o', TOOL, '-L' + LLVM + '/lib', '-lclang-cpp', '-lLLVM', '-Wl,-rpath,' + LLVM + '/lib'],
                   check=True)


def zig_includes():
    global ZIG_INC
    if ZIG_INC is None:
        r = subprocess.run(['zig', 'c++', '-target', 'x86-linux-gnu', '-###', '-c', '-x', 'c++', os.devnull,
                            '-nostdinc++'], capture_output=True, text=True)
        a = shlex.split(r.stderr.strip().split('\n')[-1])
        inc, defs = [], []
        for i, x in enumerate(a):
            if x == '-isystem' and 'lib/zig/include' not in a[i + 1]:
                inc += ['-isystem', a[i + 1]]
            if x == '-D' and a[i + 1].startswith('__GLIBC'):
                defs.append('-D' + a[i + 1])
        ZIG_INC = inc + defs
    return ZIG_INC


def base_args(root):
    """The port's src/ flags (from build/port/macos-arm64/compile_commands.json of the MAIN tree), retargeted at root."""
    cc = json.load(open(os.path.join(ROOT, 'build', 'port', 'macos-arm64', 'compile_commands.json')))
    for e in cc:
        if e['file'].startswith(os.path.join(ROOT, 'src') + os.sep) and '-include' in e['command']:
            a = shlex.split(e['command'])[1:]
            break
    out, skip = [], False
    for x in a:
        if skip:
            skip = False
            continue
        if x in ('-o', '-c'):
            skip = True
            continue
        if x in ('-g', '-MD') or x.startswith('-O') or x.startswith('-W'):
            continue
        out.append(x.replace(ROOT, root))
    # drop per-TU preludes (port/tu/*.h): added per file below
    clean = []
    i = 0
    while i < len(out):
        if out[i] == '-include' and '/port/tu/' in out[i + 1]:
            i += 2
            continue
        clean.append(out[i])
        i += 1
    return clean


def tu_args(root, base, tu, bits):
    a = list(base)
    pre = os.path.join(root, 'port', 'tu', os.path.splitext(tu)[0][4:] + '.h')
    if os.path.exists(pre):
        a += ['-include', pre]
    rd = subprocess.run([LLVM + '/bin/clang', '-print-resource-dir'], capture_output=True, text=True).stdout.strip() \
        if not hasattr(tu_args, 'rd') else tu_args.rd
    tu_args.rd = rd
    if bits == 64:
        sdk = getattr(tu_args, 'sdk', None) or subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True,
                                                               text=True).stdout.strip()
        tu_args.sdk = sdk
        a = ['clang++'] + a + ['-isysroot', sdk, '-resource-dir', rd, '-Wno-everything'] + WARN64
    else:
        a = [x for x in a if x not in ('-arch', 'arm64', '-DMB_LP64=1')]
        a = ['clang++', '--target=i686-linux-gnu', '-nostdinc'] + a + ['-msse2', '-mfpmath=sse', '-malign-double',
                                                                       '-resource-dir', rd, '-isystem', rd + '/include'] \
            + zig_includes() + ['-Wno-everything']
    return a


def tag(root):
    return 'main' if os.path.realpath(root) == os.path.realpath(ROOT) else hashlib.sha1(
        os.path.realpath(root).encode()).hexdigest()[:10]


def stamp(paths):
    h = hashlib.sha1()
    for p in paths:
        try:
            st = os.stat(p)
            h.update(('%s %d %d\n' % (p, st.st_mtime_ns, st.st_size)).encode())
        except OSError:
            h.update(('%s missing\n' % p).encode())
    return h.hexdigest()


def run(root=ROOT, jobs=4, only=None, quiet=False):
    build_tool()
    root = os.path.realpath(root)
    base = base_args(root)
    tus = sorted(os.path.relpath(p, root) for p in glob.glob(os.path.join(root, 'src', '**', '*.cpp'), recursive=True))
    tus = [t for t in tus if '/_' not in t and (not only or only in t)]
    cdir = os.path.join(ROOT, 'work', 'port64', tag(root), 'ast')
    tool_st = stamp([TOOL])
    todo, out = [], {}
    for tu in tus:
        for bits in (64, 32):
            args = tu_args(root, base, tu, bits)
            js = os.path.join(cdir, str(bits), tu.replace('/', '__') + '.json')
            meta = js + '.meta'
            key = hashlib.sha1((tool_st + ' '.join(args)).encode()).hexdigest()
            out.setdefault(tu, {})[bits] = js
            try:
                m = json.load(open(meta))
                if m['key'] == key and m['stamp'] == stamp(m['files']) and os.path.exists(js):
                    continue
            except (OSError, ValueError, KeyError):
                pass
            todo.append((tu, bits, args, js, meta, key))
    t0 = time.time()

    def one(job):
        tu, bits, args, js, meta, key = job
        os.makedirs(os.path.dirname(js), exist_ok=True)
        r = subprocess.run([TOOL, '--root', root, '--mode', str(bits), '--out', js, os.path.join(root, tu), '--'] + args,
                           capture_output=True, text=True, cwd=root)
        try:
            d = json.load(open(js))
        except (OSError, ValueError):
            sys.stderr.write('port64_ast: %s (%d) failed: %s\n' % (tu, bits, r.stderr[-2000:]))
            return
        files = sorted(set(d['files']) | {os.path.join(root, tu)})
        json.dump({'key': key, 'files': files, 'stamp': stamp(files)}, open(meta, 'w'))

    if todo:
        if not quiet:
            sys.stderr.write('port64_ast: %d of %d TU passes to run (%d jobs)\n' % (len(todo), 2 * len(tus), jobs))
        # the 64-bit passes first (sites), the big TUs first
        todo.sort(key=lambda j: (-os.path.getsize(os.path.join(root, j[0])),))
        with ThreadPoolExecutor(jobs) as ex:
            list(ex.map(one, todo))
        if not quiet:
            sys.stderr.write('port64_ast: %.0f s\n' % (time.time() - t0))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default=ROOT)
    ap.add_argument('--only')
    ap.add_argument('--jobs', type=int, default=4)
    a = ap.parse_args()
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'relink'))
    import heavy
    run(a.root, heavy.pool_jobs(a.jobs), a.only)


if __name__ == '__main__':
    main()
