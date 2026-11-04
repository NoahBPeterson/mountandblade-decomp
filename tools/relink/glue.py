#!/usr/bin/env python3
"""glue.py: generate what the standalone link needs besides our module objects.

usage: glue.py [--out work/relink/glue] [--log work/relink/link.log] [--map work/relink/resolve_map.json]
       (run tools/relink/trial_link.py and resolve_map.py first)

Reads the last trial link's unresolved symbols (work/relink/link.log) and the resolution map
(work/relink/resolve_map.json: the original address the equivalence harness gives every symbol a module references),
and writes:

  mb_data.obj     the original image's .rdata, initialised .data and .bss, as three sections of one object, with
                  a label for every unresolved data symbol at its original offset (so code that indexes from one
                  global into its neighbours, as the original does, still lands where it did). The image has no base
                  relocations, so pointers inside the data are found by value: a 4-aligned dword that is the
                  address of a function start, or of anything in the data sections, and is not part of a string.
                  Data pointers become section-relative relocations; code pointers point at the function our
                  source defines at that address, the library function at that address (symbols/lib_names.txt),
                  or else a trap (int 3, one per missing address, listed in glue.json).
  aliases         /alternatename directives (link 7.10 ignores one whose left side is a template name, ??$...:
                  such a call must name the definition in source) (in mb_data.obj's .drectve) for every unresolved symbol whose address
                  is a function our source defines under another name or signature, a CRT function, or an import.
  imports/*.lib   import libraries for the DLLs the game ships without one (Newton, FMOD 3, SkinMagic, d3dx9_31),
                  made with link /lib from the original's import names, decorated as the DLL exports them.
  glue.json       what was done, and every symbol left unresolved, with the reason.
"""
import argparse, bisect, collections, json, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
from winnative import WIN, winpath      # noqa: E402  (native link.exe on Windows, wine on the Mac)

TC = os.path.join(ROOT, "work", "toolchain")
SDK_LIBS = [os.path.join(TC, "vc71tk", "lib"), os.path.join(TC, "vc9sp1", "WinSDK", "Lib"),
            os.path.join(ROOT, "work", "ext", "dxsdk_jun10", "DXSDK", "Lib", "x86")]
OWN_IMPORTS = ("newton", "fmod", "skinmagic", "d3dx9_31")
DLL_FILES = {"newton": "Newton.dll", "fmod": "fmod.dll", "skinmagic": "SkinMagic.dll", "d3dx9_31": "d3dx9_31.dll"}
# argument bytes of the SkinMagic entry points the game calls (all stdcall; the DLL exports them undecorated)
SKINMAGIC_ARGS = {"InitSkinMagicLib": 16, "ExitSkinMagicLib": 0, "LoadSkinFile": 4, "SetSingleDialogSkin": 8}

RDATA = (".rdata$mb", 0x549000, 0x5a7d72)
# The original's .data is one section: initialised bytes, then .bss from 0x5d7000. The linker places our copies of
# the two apart, so the cut between them must not fall inside an object: the resource manager at 0x5d6fa0 is 0xe0
# bytes and straddles 0x5d7000 (its tail landed on DXUtil_Timer's statics). The copy of the initialised part
# therefore runs on to 0x5d7080, where the next object (the gui manager, g_gui) starts; those 0x80 bytes are zeros.
DATA = (".data$mb", 0x5a8000, 0x5d7080)
BSS = (".bss$mb", 0x5d7080, 0x8ee5b94)
TEXT_LO, TEXT_HI = 0x401000, 0x548578
IAT_LO, IAT_HI = 0x549000, 0x5494c8
# Original data our exe replaces and never reads: not copied (left zero). The CRT initializer / terminator tables are
# the .CRT$XCA..XCZ, XIA..XIZ, XPA..XPZ, XTA..XTZ ranges at the start of .data (0x5a8000..0x5a80f4: the game's
# static_init stubs, then libcmt's own entries). Our exe runs the tables the linker builds from our objects (static_init.cpp's
# .CRT$XCU) and libcmt's, whose __xc_a/__xi_a/... are libcmt symbols, so nothing of ours addresses the copy. build.py
# and glue.py refuse to link when a data label, a pointer in the remaining copy, a literal address or a vtable root
# reaches a dead range (check_dead_ranges), the way the trap gate does for traps.
DEAD_RANGES = (
    (0x5a8000, 0x5a80f4, "CRT initializer/terminator tables .CRT$XCA..XTZ"),
)


# Dead tables found by content, inside windows. Their records are interleaved with live data (the ctype table, month
# names, error strings), so a window is not zeroed whole; the dwords that are dead are:
#   - a seed: a dword that points at original code we do not define (what would become a trap) and lies in a window.
#     Those are the cleanup / catch funclets of the original's C++ EH maps and libcmt's locale / stdio code, whose
#     functions our build recompiles (cl emits its own maps; our __ehhandler thunks name ours) or takes from libcmt.lib;
#   - a referrer: a dword in a window that points at (or up to 20 bytes before) a dead dword, i.e. the try-block /
#     handler-array / unwind-map / FuncInfo pointer chain that leads to a seed. Only the original's own __ehhandler
#     immediates (code we do not link) reach the head of such a chain.
# The windows are the three groups of the trap classification (docs/standalone_traps.md).
DEAD_WINDOWS = (
    (0x59c6d8, 0x59cf24, "libcmt EH / locale funclet tables (.rdata)"),
    (0x5a0898, 0x5a60f4, "FuncInfo / unwind / try / catch maps of recompiled functions (.rdata)"),
    (0x5d12a8, 0x5d12ac, "libcmt static (.data)"),
    (0x5d5cf8, 0x5d6160, "libcmt statics (.data)"),
)
TYPE_INFO_VFTABLE = 0x59ca6c    # the original's type_info vftable (libcmt typinfo.obj); the glue binds libcmt's
CHAIN_REACH = 0x14           # a chain pointer may address a record up to 20 bytes before its dead dword


def in_window(va):
    return any(lo <= va < hi for lo, hi, _w in DEAD_WINDOWS)


def object_end(start, starts, runs=None):
    """End of the object that begins at `start`: the next referenced / labelled address (the way trap_report.py
    delimits objects), at least 4 bytes; inside a run of code pointers (a vtable's slots; `runs` is a sorted list of
    [start, bytes]) it ends where the run does."""
    i = bisect.bisect_right(starts, start)
    end = max(start + 4, starts[i]) if i < len(starts) else start + 4
    if runs:
        j = bisect.bisect_right(runs, [start, 1 << 32]) - 1
        if j >= 0 and start < runs[j][0] + runs[j][1]:
            end = min(end, runs[j][0] + runs[j][1])
    return end


def dead_in(lo, hi, dead_sorted):
    """The dead dwords inside [lo, hi)."""
    i = bisect.bisect_left(dead_sorted, lo)
    out = []
    while i < len(dead_sorted) and dead_sorted[i] < hi:
        out.append(dead_sorted[i])
        i += 1
    return out


def ranges_of(dead_sorted):
    out = []
    for w in dead_sorted:
        if out and out[-1][1] == w:
            out[-1][1] = w + 4
        else:
            out.append([w, w + 4])
    return out


# dwords in the copy that equal a dead-range address by accident: 0x5d3ef4 sits inside a table of 16-bit entries
# (80 00 4f 00 80 50 ...: bytes 0x5a 0x00 0x80 0x00 here), not a pointer
NOT_POINTERS = (0x5d3ef4,)


def in_dead_range(va):
    return next((r for r in DEAD_RANGES if r[0] <= va < r[1]), None)




# ------------------------------------------------------------------ archives

def archive_symbols(path):
    """Public symbol names of a COFF archive (first linker member)."""
    d = open(path, "rb").read()
    if d[:8] != b"!<arch>\n":
        return set()
    size = int(d[8 + 48:8 + 58].split()[0])
    body = d[68:68 + size]
    n = struct.unpack_from(">I", body, 0)[0]
    names = body[4 + 4 * n:].split(b"\0")[:n]
    return {x.decode("latin-1") for x in names}


def lib_index(libs):
    """symbol -> library file, over the SDK / runtime libraries."""
    idx = {}
    for d in SDK_LIBS:
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if f.lower().endswith(".lib") and (libs is None or f.lower() in libs):
                for s in archive_symbols(os.path.join(d, f)):
                    idx.setdefault(s, f)
    return idx


# ------------------------------------------------------------------ COFF writer

class Obj:
    def __init__(self):
        self.sections = []      # dict(name, flags, data (bytes or None), size, relocs [(off, symname)])
        self.syms = []          # (name, value, secno, type, cls)
        self.sym_index = {}

    def section(self, name, flags, data=None, size=None):
        self.sections.append({"name": name, "flags": flags, "data": data, "size": len(data) if data is not None else size,
                              "relocs": []})
        return len(self.sections)      # 1-based section number

    def add_sym(self, name, value, secno, typ=0, cls=2):
        if name in self.sym_index:
            return
        self.sym_index[name] = None
        self.syms.append((name, value, secno, typ, cls))

    def write(self, path):
        # symbol table: section symbols first (static, with aux), then the rest
        strtab = bytearray(b"\0\0\0\0")

        def sname(n):
            b = n.encode("latin-1")
            if len(b) <= 8:
                return b.ljust(8, b"\0")
            off = len(strtab)
            strtab.extend(b + b"\0")
            return b"\0\0\0\0" + struct.pack("<I", off)

        recs = []
        index = {}
        for i, s in enumerate(self.sections):
            index["$sec%d" % (i + 1)] = len(recs)
            recs.append(sname(s["name"]) + struct.pack("<IhHBB", 0, i + 1, 0, 3, 1))
            recs.append(struct.pack("<IHHIHB", s["size"], len(s["relocs"]), 0, 0, 0, 0) + b"\0" * 3)
        for name, value, secno, typ, cls in self.syms:
            index[name] = len(recs)
            recs.append(sname(name) + struct.pack("<IhHBB", value, secno, typ, cls, 0))
        nsec = len(self.sections)
        hdr_end = 20 + 40 * nsec
        pos = hdr_end
        layout = []
        for s in self.sections:
            raw = pos if s["data"] is not None and s["size"] else 0
            if raw:
                pos += s["size"]
            rel = pos if s["relocs"] else 0
            pos += 10 * len(s["relocs"])
            layout.append((raw, rel))
        symptr = pos
        out = bytearray(struct.pack("<HHIIIHH", 0x14C, nsec, 0, symptr, len(recs), 0, 0))
        for s, (raw, rel) in zip(self.sections, layout):
            nm = s["name"].encode()
            if len(nm) > 8:
                off = len(strtab)
                strtab.extend(nm + b"\0")
                nm = b"/%d" % off
            if len(s["relocs"]) > 0xFFFF:
                raise SystemExit("too many relocations in %s" % s["name"])
            out += nm.ljust(8, b"\0") + struct.pack("<IIIIIIHHI", 0, 0, s["size"], raw, rel, 0, len(s["relocs"]), 0,
                                                    s["flags"])
        for s in self.sections:
            if s["data"] is not None and s["size"]:
                out += s["data"]
            for off, symname in s["relocs"]:
                out += struct.pack("<IIH", off, index[symname], 0x06)      # IMAGE_REL_I386_DIR32
        assert len(out) == symptr
        for r in recs:
            out += r
        struct.pack_into("<I", strtab, 0, len(strtab))
        out += strtab
        open(path, "wb").write(out)


SCN_DATA_R = 0x40 | 0x40000000 | 0x00D00000            # initialised, read, align 4096
SCN_DATA_RW = SCN_DATA_R | 0x80000000
SCN_BSS = 0x80 | 0x40000000 | 0x80000000 | 0x00D00000
SCN_CODE = 0x20 | 0x20000000 | 0x40000000 | 0x00500000   # code, execute, read, align 16
SCN_INFO = 0x00100A00                                   # lnk_info | lnk_remove | align 1


# ------------------------------------------------------------------ main

def unresolved_symbols(log):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from trial_link import parse_unresolved
    return set(parse_unresolved(log))


def string_mask(b):
    """Byte positions inside a printable run of 4+ characters ended by NUL (C strings). A string starts on a 4-byte
    boundary or right after a NUL: the tail of a float before a vtable (0.01f = 0a d7 23 3c, then 0x417730 =
    30 77 41 00 reads "#<0wA") is not one."""
    mask = bytearray(len(b))
    for m in re.finditer(rb"[\x20-\x7e\t\r\n]{4,}(?=\0)", b):
        st = m.start()
        while st % 4 and b[st - 1] != 0:
            st += 1                                      # skip a non-string prefix up to the next boundary
        if m.end() - st < 4:
            continue                                     # (a pointer from a boundary is 3 bytes and its 00)
        mask[st:m.end()] = b"\1" * (m.end() - st)
    return mask


def call_conv(name):
    """The calling convention a mangled C++ function name encodes (cdecl / stdcall / fastcall / thiscall), or None
    for a C name or data."""
    m = re.match(r"\?[^@?]+(?:@[^@]+)*@@([A-Z])([A-Z])([A-Z])?", name)
    if not m:
        return None
    if m[1] in "YZSKC":                          # free function / static member: the code is the next letter
        cc = m[2]
    elif m[1] in "AEIMQU" and m[3]:              # instance member: a this-qualifier letter, then the code
        cc = m[3]
    else:
        return None
    return {"A": "cdecl", "E": "thiscall", "G": "stdcall", "I": "fastcall"}.get(cc)


def conv_mismatches(aliases):
    """Aliases whose two names pass arguments differently: a cdecl caller of a thiscall definition (or stdcall /
    cdecl) hands the callee its arguments in the wrong places (read_psys_rec -> psys_record::read wrote floats through
    a garbage record pointer). thiscall / fastcall pairs both pass the first argument in ecx (the fastcall stand-ins
    for deleting destructors and adjustors), and a function without parameters (XZ) cannot disagree."""
    out = {}
    for a, t in aliases.items():
        ca, ct = call_conv(a), call_conv(t)
        if ca and ct and ca != ct and {ca, ct} != {"thiscall", "fastcall"} and not (a.endswith("XZ") and t.endswith("XZ")):
            out[a] = "%s -> %s %s" % (ca, ct, t)
    return out


def section_of(va):
    for s in (RDATA, DATA, BSS):
        if s[1] <= va < s[2]:
            return s
    return None


def make_import_libs(img, out_dir, needed):
    """One import library per own-import DLL; returns {dll!name: decorated symbol}."""
    import pefile
    os.makedirs(out_dir, exist_ok=True)
    dx = archive_symbols(os.path.join(SDK_LIBS[2], "d3dx9.lib")) if os.path.isdir(SDK_LIBS[2]) else set()
    dx_dec = {}
    for s in dx:
        m = re.fullmatch(r"_(D3DX\w+)@(\d+)", s)
        if m:
            dx_dec[m[1]] = s[1:]
    by_dll = collections.defaultdict(list)
    for va, full in sorted(img.imports.items()):
        dll, name = full.split("!", 1)
        if dll in OWN_IMPORTS:
            by_dll[dll].append(name)
    decorated = {}
    env = dict(os.environ, WINEPREFIX=os.environ.get("CL_WINEPREFIX") or os.path.join(TC, "wineprefix-mb"), WINEDEBUG="-all")
    for dll, names in sorted(by_dll.items()):
        exports = {}
        if dll == "skinmagic":
            pe = pefile.PE(os.path.join(ROOT, "game", DLL_FILES[dll]))
            for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
                exports[e.ordinal] = e.name.decode() if e.name else None
        lines, patch = [], {}
        for n in names:
            if dll == "fmod":                          # exported decorated: _FSOUND_Init@12 imports "_FSOUND_Init@12"
                lines.append(n[1:])
                patch[n] = 1                           # IMPORT_NAME: the import name is the symbol name
                decorated[dll + "!" + n] = n
            elif dll == "skinmagic":
                k = int(n[3:])
                en = exports.get(k) or ("SkinMagic_ord%d" % k)
                if en in SKINMAGIC_ARGS:                  # stdcall: the symbol is _Name@N, imported by ordinal
                    en = "%s@%d" % (en, SKINMAGIC_ARGS[en])
                lines.append("%s @%d NONAME" % (en, k))
                decorated[dll + "!" + n] = "_" + en
            elif dll == "d3dx9_31":                    # stdcall, exported undecorated: _X@N imports "X"
                dn = dx_dec.get(n, n)
                lines.append(dn)
                decorated[dll + "!" + n] = "_" + dn
                patch["_" + dn] = 3                    # IMPORT_NAME_UNDECORATE (link /lib kept the @N)
            else:                                       # Newton: cdecl, exported undecorated
                lines.append(n)
                decorated[dll + "!" + n] = "_" + n
        dname = DLL_FILES[dll]
        deff = os.path.join(out_dir, dll + ".def")
        open(deff, "w").write("LIBRARY %s\nEXPORTS\n%s\n" % (dname, "\n".join("  " + l for l in lines)))
        lib = os.path.join(out_dir, dll + ".lib")
        subprocess.run((["wine"] if not WIN else []) + [os.path.join(TC, "vc71tk", "bin", "link.exe"), "/lib", "/nologo", "/machine:x86",
                        "/def:" + winpath(deff), "/out:" + winpath(lib)], env=env, stdin=subprocess.DEVNULL,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if patch:
            patch_import_names(lib, patch)
    return decorated


def patch_import_names(lib, patch):
    """Set the name type of the short import members whose symbol is in `patch` (symbol -> name type)."""
    d = bytearray(open(lib, "rb").read())
    pos = 8
    while pos + 60 <= len(d):
        size = int(d[pos + 48:pos + 58].split()[0])
        body = pos + 60
        if d[body:body + 4] == b"\0\0\xff\xff":
            nul = d.index(b"\0", body + 20)
            sym = d[body + 20:nul].decode("latin-1")
            if sym in patch:
                t = struct.unpack_from("<H", d, body + 18)[0]
                t = (t & ~0x1C) | (patch[sym] << 2)
                struct.pack_into("<H", d, body + 18, t)
        pos = body + size + (size & 1)
    open(lib, "wb").write(d)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "glue"))
    ap.add_argument("--log", default=os.path.join(ROOT, "work", "relink", "link.log"), help="a trial link's log (without --glue)")
    ap.add_argument("--map", default=os.path.join(ROOT, "work", "relink", "resolve_map.json"))
    a = ap.parse_args()
    os.chdir(ROOT)
    import resolve as R
    img = R.Image()
    db = R.SymbolDB(img)
    os.makedirs(a.out, exist_ok=True)
    unres = unresolved_symbols(open(a.log, errors="replace").read())
    rmap = json.load(open(a.map))
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    # data our modules define, by address (a name the resolver mapped to a data VA while defined in that object)
    our_data = {}
    for n, d in rmap["names"].items():
        for va, e in d.items():
            v = int(va, 16)
            if e["defined"] and section_of(v) and not n.startswith(("??_C@", "__real@", "??_7", "??_R", "?_")):
                our_data.setdefault(v, n)
    lib_names = {}
    for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
        p = line.split()
        if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
            lib_names.setdefault(int(p[0], 16), p[1][9:])
    runtime = lib_index({"libcmt.lib", "libcpmt.lib"})
    system = lib_index(None)
    imp_dec = make_import_libs(img, os.path.join(a.out, "imports"), unres)
    thunk_of = {v: k for k, v in img.thunks().items()}          # thunk VA -> import name
    iat_of = {va: full for va, full in img.imports.items()}     # IAT VA -> dll!name

    def import_symbol(full, data):
        dll, name = full.split("!", 1)
        dec = imp_dec.get(full)
        if dec is None:                                        # a system DLL: the SDK's decoration
            for c in ("_%s@" % name, "_" + name):
                hits = sorted(s for s in system if s == "__imp_" + c or (c.endswith("@") and s.startswith("__imp_" + c)))
                if hits:
                    dec = hits[0][6:]
                    break
        if dec is None:
            return None
        return ("__imp_" + dec) if data else dec

    obj = Obj()
    report = {"labels": 0, "aliases": {}, "unresolved": {}, "traps": {}, "conflicts": {}, "pointers": collections.Counter()}
    aliases = {}
    labels = {}       # name -> (section, offset)

    def code_target(va):
        for _ in range(4):       # a library jump stub (jmp rel32) stands for its target: 0x535712 -> _free
            b = img.read(va, 5)
            if va in defined_at or va in lib_names or b[0] != 0xE9:
                break
            va = (va + 5 + struct.unpack_from("<i", b, 1)[0]) & 0xFFFFFFFF
        if va in defined_at:
            return defined_at[va], "ours"
        n = lib_names.get(va)
        if n and n in runtime:
            return n, "crt"
        if va in thunk_of:
            full = next((f for f in img.imports.values() if f.split("!", 1)[1] == thunk_of[va]), None)
            s = import_symbol(full, False) if full else None
            if s:
                return s, "import"
        return None, None

    for s in sorted(unres):
        # a harness model (resolve.py's model-stub / model-data: an address in the emulator's stub region, 0x7f00xxxx)
        # is not an original address; a name only the models resolved falls through to its vtable slot or the symbols
        vas = {v: e for v, e in (rmap["names"].get(s) or {}).items()
               if not set(e.get("methods", ())) <= {"model-stub", "model-data"}}
        how = "map"
        if not vas:
            slots = {x[0] for x in rmap["vtable_slots"].get(s, [])}
            vas = {v: {} for v in slots}
            how = "vtable slot"
        if not vas:     # referenced only from code no row covers (a compiler-generated deleting destructor):
            cands = {v for v in db.mangled.get(s, ())}          # the symbol files' mangled-name mapping
            vas = {"%08x" % v: {} for v in cands} if len(cands) == 1 else {}
            how = "symbols"
        if not vas:
            report["unresolved"][s] = "no address (the resolver never saw it)"
            continue
        if len(vas) > 1:
            # one name, two original addresses: some declaration is wrong (the wrong one won by row count before
            # 2026-11-02 and bound renderer::set_view's matrix product to the scalar-times-vector operator). The glue
            # fails below rather than guess.
            report["conflicts"][s] = sorted(vas)
            continue
        va = int(next(iter(vas)), 16)
        if va in iat_of:
            t = import_symbol(iat_of[va], True)
            if t:
                if t != s:          # the import library defines the name the code uses: nothing to alias
                    aliases[s] = t
            else:
                report["unresolved"][s] = "import %s with no library symbol" % iat_of[va]
            continue
        if section_of(va):
            if va in our_data and our_data[va] != s:
                aliases[s] = our_data[va]
            else:
                sec = section_of(va)
                labels[s] = (sec, va - sec[1])
            continue
        if TEXT_LO <= va < TEXT_HI:
            t, kind = code_target(va)
            if t and t != s:
                aliases[s] = t
            else:
                report["unresolved"][s] = "code at %08x with no definition (%s)" % (va, how)
            continue
        if how == "vtable slot":
            # our vtable is longer than the original's (Rec428 0x595c68 has 9 slots, ours declares a 10th `vfunc`):
            # the dword past the original table is data, and the original never calls that slot. Anything that does
            # stops in the CRT's pure-call handler instead of jumping into data.
            aliases[s] = "__purecall"
            report["traps"][s] = "vtable slot past the original table (%08x is not code): __purecall" % va
            continue
        report["unresolved"][s] = "address %08x outside the image sections" % va

    # sections
    secno = {}
    for sec in (RDATA, DATA):
        data = bytearray(img.read(sec[1], sec[2] - sec[1]))
        if len(data) < sec[2] - sec[1]:
            data += b"\0" * (sec[2] - sec[1] - len(data))
        secno[sec[0]] = obj.section(sec[0], SCN_DATA_R if sec is RDATA else SCN_DATA_RW, data)
    secno[BSS[0]] = obj.section(BSS[0], SCN_BSS, None, BSS[2] - BSS[1])
    traps = {}
    trap_code = bytearray()
    for lo, hi, _why in DEAD_RANGES:
        sec = section_of(lo)
        obj.sections[secno[sec[0]] - 1]["data"][lo - sec[1]:hi - sec[1]] = b"\0" * (hi - lo)
    reach = []
    for n, (sec, off) in sorted(labels.items()):
        if in_dead_range(sec[1] + off):
            reach.append("label %s at %08x" % (n, sec[1] + off))
    for v, n in our_data.items():
        if in_dead_range(v):
            reach.append("definition %s at %08x" % (n, v))
    # dead tables found by content (DEAD_WINDOWS): seeds, then the chain of dwords that lead to them
    words = {}
    in_string, str_starts = set(), set()
    for sec in (RDATA, DATA):
        data = obj.sections[secno[sec[0]] - 1]["data"]
        mask = string_mask(bytes(data))
        for i, m in enumerate(mask):
            if m:
                in_string.add(sec[1] + i)
                if i == 0 or not mask[i - 1]:
                    str_starts.add(sec[1] + i)
        for off in range(0, len(data) - 3, 4):
            va_here = sec[1] + off
            if not (IAT_LO <= va_here < IAT_HI or any(mask[off:off + 4])):
                words[va_here] = struct.unpack_from("<I", data, off)[0]
    dead = {w for w, v in words.items() if in_window(w) and TEXT_LO <= v < TEXT_HI and v in img.start_set
            and code_target(v)[0] is None}
    nseeds = len(dead)
    # a pointer into a C string (weekday names at 0x5a1640, a message at 0x5a1654) is not a link in an EH chain
    # and the first dword of a record of these tables is a small count / index, -1 (an unwind entry's toState) or the
    # FuncInfo magic: a pointer to "Sun\0" (0x6e7553) or to the first word of a function-pointer table is not a link
    heads = dict(words)
    def head_ok(v):
        h = heads.get(v)
        return h is not None and (h < 0x100 or h == 0xffffffff or h == 0x19930520)
    refs_in = lambda v: v % 4 == 0 and v not in in_string and head_ok(v) and any(v + k in dead for k in range(0, CHAIN_REACH + 1, 4))
    funcinfos = [w for w, v in words.items() if v == 0x19930520 and in_window(w)]   # FuncInfo: magic, 6 more fields
    todo = True
    while todo:
        todo = False
        for fi in funcinfos:
            span = [fi + 4 * i for i in range(7)]
            if any(x in dead for x in span) and not all(x in dead for x in span):
                dead.update(x for x in span if x in words)       # a FuncInfo whose chain is dead is dead whole
                todo = True
        for w, v in words.items():
            if w not in dead and in_window(w) and w not in NOT_POINTERS and refs_in(v):
                dead.add(w)
                todo = True
    for w in dead:
        sec = section_of(w)
        struct.pack_into("<I", obj.sections[secno[sec[0]] - 1]["data"], w - sec[1], 0)
        words[w] = 0
    dead_sorted = sorted(dead)
    starts = set(v for w, v in words.items() if RDATA[1] <= v < DATA[2]) | set(sec[1] + off for sec, off in labels.values()) | set(our_data) | str_starts
    starts = sorted(starts)
    runs = []                   # maximal runs of code pointers: [start, bytes] (vtables and function-pointer tables)
    for w in sorted(heads):
        if TEXT_LO <= heads[w] < TEXT_HI:
            if runs and runs[-1][0] + runs[-1][1] == w:
                runs[-1][1] += 4
            else:
                runs.append([w, 4])
    report["dead_words"] = {"runs": runs, "seeds": nseeds, "dwords": len(dead), "ranges": [[lo, hi] for lo, hi in ranges_of(dead_sorted)],
                            "starts": starts}
    for sec in (RDATA, DATA):
        s = obj.sections[secno[sec[0]] - 1]
        data = s["data"]
        mask = string_mask(bytes(data))
        for off in range(0, len(data) - 3, 4):
            va_here = sec[1] + off
            if IAT_LO <= va_here < IAT_HI or any(mask[off:off + 4]):
                continue
            v = struct.unpack_from("<I", data, off)[0]
            tsec = section_of(v)
            if tsec and in_dead_range(v) and va_here not in NOT_POINTERS:
                reach.append("pointer at %08x to %08x" % (va_here, v))
            if v == TYPE_INFO_VFTABLE:
                # the pVFTable of a copied _TypeDescriptor (.?AVexception@@, .?AVbad_alloc@std@@ ...): libcmt's own
                # type_info vftable, not the copy of the original's (whose slot is a trapped destructor)
                struct.pack_into("<I", data, off, 0)
                s["relocs"].append((off, "??_7type_info@@6B@"))
                report["pointers"]["type_info_vftable"] += 1
            elif tsec:
                if v in our_data:
                    struct.pack_into("<I", data, off, 0)
                    s["relocs"].append((off, our_data[v]))
                    obj.add_sym(our_data[v], 0, 0)
                    report["pointers"]["data->ours"] += 1
                else:
                    struct.pack_into("<I", data, off, v - tsec[1])
                    s["relocs"].append((off, "$sec%d" % secno[tsec[0]]))
                    report["pointers"]["data"] += 1
            elif TEXT_LO <= v < TEXT_HI and v in img.start_set:
                t, kind = code_target(v)
                if t is None:
                    if v not in traps:
                        traps[v] = len(trap_code)
                        trap_code += b"\xcc" * 4
                    t, kind = "_mb_trap_%08x" % v, "trap"
                struct.pack_into("<I", data, off, 0)
                s["relocs"].append((off, t))
                report["pointers"]["code->" + kind] += 1
    tsec = obj.section(".text$mbtrap", SCN_CODE, bytes(trap_code or b"\xcc"))
    for v, o in traps.items():
        obj.add_sym("_mb_trap_%08x" % v, o, tsec, 0x20)
    for name, (sec, off) in sorted(labels.items()):
        obj.add_sym(name, off, secno[sec[0]])
    for sec, base in ((RDATA, "_mb_rdata_base"), (DATA, "_mb_data_base"), (BSS, "_mb_bss_base")):
        obj.add_sym(base, 0, secno[sec[0]])       # where each copied section lands (build.py's literal fix-up)
    # every relocation target that is not a section or a label is an external
    for s in obj.sections:
        for _off, t in s["relocs"]:
            if not t.startswith("$sec"):
                obj.add_sym(t, 0, 0, 0x20 if not t.startswith("?g_") else 0)
    drectve = " ".join("/alternatename:%s=%s" % kv for kv in sorted(aliases.items())) + " "
    obj.section(".drectve", SCN_INFO, drectve.encode("latin-1"))
    obj.write(os.path.join(a.out, "mb_data.obj"))
    report["labels"] = len(labels)
    report["aliases"] = aliases
    report["conv_mismatch"] = conv_mismatches(aliases)
    report["traps"] = {"%08x" % v: "int 3 at _mb_trap_%08x" % v for v in sorted(traps)}
    report["pointers"] = dict(report["pointers"])
    json.dump(report, open(os.path.join(a.out, "glue.json"), "w"), indent=1)
    print("unresolved in: %d; data labels %d, aliases %d, left unresolved %d, conflicts %d"
          % (len(unres), len(labels), len(aliases), len(report["unresolved"]), len(report["conflicts"])))
    print("pointers relocated: %s; traps for %d code addresses without a definition" % (report["pointers"], len(traps)))
    bad = 0
    for r in reach:
        print("DEAD RANGE REACHED: %s (glue.py DEAD_RANGES: the range is not copied)" % r)
        bad = 1
    for n, vs in sorted(report["conflicts"].items()):
        print("CONFLICT: %s resolves to %s (fix the declaration or annotation that gives the wrong address)" % (n, ", ".join(vs)))
        bad = 1
    for n in sorted(report["conv_mismatch"]):
        print("CONV MISMATCH: alias %s joins different calling conventions (glue.json conv_mismatch)" % n)
        bad = 1
    return bad


if __name__ == "__main__":
    sys.exit(main())
