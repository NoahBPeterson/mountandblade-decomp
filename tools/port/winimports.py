#!/usr/bin/env python3
"""tools/port/winimports.py: partial DATA-import archives for the Windows x86_64 port link.

The problem. SDL3 is compiled against the real mingw-w64 headers, so its objects call the Win32 API through
dllimport: they reference __imp_X. zig synthesizes its import libraries from its own .def files, and the member
that supplies __imp_X ALSO defines a bare X thunk. mb_platform defines its own strong pseudo-Win32 X (the port's
contract: registry.ini, pseudo CreateProcess, SDL-backed windows ...), and zig's COFF linker has no first-wins:
bare X from the import thunk and bare X from mb_platform are a duplicate-symbol error (~110 per test exe).

The fix. For each import library, emit a PARTIAL archive exporting exactly the symbols mb_platform defines, each
marked DATA. A DATA short import exposes ONLY __imp_X (no bare thunk -- probed work/winprobe2/improbe). Linked as
explicit items ahead of the -l flags:
  * a bare X call binds to mb_platform's strong definition (the pseudo behavior the tests assert);
  * SDL3's __imp_X call binds to the real DLL through the partial archive;
  * zig's synthesized member for X never loads, because __imp_X is already satisfied -- so its bare thunk never
    collides. Symbols mb_platform does NOT define are left to zig's library as usual (bare thunk + __imp).
The intersection with mb_platform's actual definitions is what keeps this safe: covering a symbol the platform
does not define would strand its bare thunk (nothing else would supply it).

llvm-dlltool also emits vestigial import-descriptor / __NULL_IMPORT_DESCRIPTOR / null-thunk members. Short imports
never reference them (the linker synthesizes the import directory itself), and leaving them in risks a duplicate
__NULL_IMPORT_DESCRIPTOR against zig's own library. Keep only the short-import members (Sig1=0x0000, Sig2=0xFFFF).

usage: winimports.py --platform <libmb_platform.a> --outdir <dir> [--libs a,b,c]
                     [--mingw <zig libc/mingw dir>] [--dlltool P] [--ar P] [--nm P]
Writes <outdir>/<name>.a per library and prints "<name>: <N> symbols" lines. The mingw dir defaults to the one
`zig env` reports (lib_dir/libc/mingw); the llvm tools default to /opt/homebrew/opt/llvm/bin, then PATH.
"""
import argparse
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

# the Win32 import libraries the port link pulls (uuid is a static GUID lib, pthread/m are not import libs).
DEFAULT_LIBS = ("kernel32 user32 gdi32 winmm imm32 ole32 oleaut32 version advapi32 setupapi shell32 "
                "dinput8 usp10 rpcrt4 ws2_32 winspool comdlg32").split()

# func.def.in for __x86_64__: these expand to their argument, every other F* macro expands to nothing.
KEEP_MACROS = {"F64", "F_X64", "F_X86_ANY", "F_X86_NATIVE", "F_NON_I386", "F_NON_ARM64"}


def find_tool(flag, name):
    if flag:
        return flag
    p = "/opt/homebrew/opt/llvm/bin/" + name
    if os.path.exists(p):
        return p
    w = shutil.which(name)
    if not w:
        sys.exit("winimports.py: cannot find %s" % name)
    return w


def mingw_dir(flag):
    if flag:
        return flag
    try:
        out = subprocess.run(["zig", "env"], capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit("winimports.py: `zig env` failed (%s); pass --mingw" % e)
    m = re.search(r'\.lib_dir\s*=\s*"([^"]+)"', out)
    if not m:
        sys.exit("winimports.py: no lib_dir in `zig env` output; pass --mingw")
    return os.path.join(m.group(1), "libc", "mingw")


def platform_symbols(nm, archive):
    out = subprocess.run([nm, "--defined-only", "--extern-only", archive],
                         capture_output=True, text=True, check=True).stdout
    syms = set()
    for line in out.splitlines():
        m = re.match(r'^[0-9a-fA-F]+ [A-Za-z] (\S+)$', line)   # "<addr> <type> <name>"; skips member headers
        if m:
            syms.add(m.group(1))
    return syms


def def_path(mingw, name):
    for rel in ("lib-common/%s.def.in" % name, "lib-common/%s.def" % name, "lib64/%s.def" % name):
        p = os.path.join(mingw, rel)
        if os.path.exists(p):
            return p
    sys.exit("winimports.py: no def file for %s under %s" % (name, mingw))


def parse_def(path):
    """(dll_name, [export names]) with the F* architecture macros expanded for x86_64."""
    text = open(path, encoding="latin-1").read()

    def expand(m):
        return m.group(2) if m.group(1) in KEEP_MACROS else ""
    text = re.sub(r'\b(F[A-Z0-9_]+)\(([^)]*)\)', expand, text)

    dll, exports = None, []
    for line in text.splitlines():
        line = line.split(";", 1)[0].strip()          # ';' comments (commented-out exports)
        if not line or line.startswith("#"):          # '#include "func.def.in"'
            continue
        head = line.split(None, 1)[0].upper()
        if head == "LIBRARY":
            dll = line.split(None, 1)[1].strip().strip('"')
            continue
        if head in ("EXPORTS", "HEAPSIZE", "STACKSIZE", "SECTIONS", "DESCRIPTION", "VERSION", "ATTRIBUTES"):
            continue
        exports.append(line.split()[0])               # Name [DATA] [@ord] [== internal] [PRIVATE]: name is first
    if not dll:
        sys.exit("winimports.py: %s has no LIBRARY line" % path)
    return dll, exports


def short_import_members(archive):
    """The per-symbol short-format import bodies (Sig1=0x0000, Sig2=0xFFFF); drops index/descriptor/NULL members."""
    data = open(archive, "rb").read()
    off, kept = 8, []
    while off + 60 <= len(data):
        size = int(data[off + 48:off + 58].decode("ascii").strip())
        body = data[off + 60:off + 60 + size]
        if size >= 4 and body[0:2] == b"\x00\x00" and body[2:4] == b"\xff\xff":
            kept.append(body)
        off += 60 + size + (size & 1)
    return kept


def build_one(name, mingw, plat, dlltool, ar, outdir, tmp):
    dll, exports = parse_def(def_path(mingw, name))
    common = sorted(set(exports) & plat)
    defp = os.path.join(tmp, name + ".def")
    with open(defp, "w") as f:
        f.write('LIBRARY "%s"\nEXPORTS\n' % dll)
        for s in common:
            f.write("%s DATA\n" % s)                  # DATA: expose only __imp_X, never a bare thunk
    full = os.path.join(tmp, name + ".full.lib")
    subprocess.run([dlltool, "-m", "i386:x86-64", "-d", defp, "-l", full], check=True)
    members = short_import_members(full)
    out = os.path.join(outdir, name + ".a")
    if members:
        paths = []
        for i, body in enumerate(members):
            p = os.path.join(tmp, "%s.%d.obj" % (name, i))
            open(p, "wb").write(body)
            paths.append(p)
        if os.path.exists(out):
            os.remove(out)
        subprocess.run([ar, "rc", out] + paths, check=True)
    else:
        open(out, "wb").write(b"!<arch>\n")           # empty archive: nothing the platform owns in this DLL
    return len(common)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--platform", required=True)
    ap.add_argument("--outdir", required=True)
    ap.add_argument("--libs", default=",".join(DEFAULT_LIBS))
    ap.add_argument("--mingw")
    ap.add_argument("--dlltool")
    ap.add_argument("--ar")
    ap.add_argument("--nm")
    a = ap.parse_args()
    dlltool = find_tool(a.dlltool, "llvm-dlltool")
    ar = find_tool(a.ar, "llvm-ar")
    nm = find_tool(a.nm, "llvm-nm")
    mingw = mingw_dir(a.mingw)
    plat = platform_symbols(nm, a.platform)
    os.makedirs(a.outdir, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="winimports.") as tmp:
        for name in a.libs.split(","):
            name = name.strip()
            if not name:
                continue
            n = build_one(name, mingw, plat, dlltool, ar, a.outdir, tmp)
            print("%s: %d symbols" % (name, n))


if __name__ == "__main__":
    main()