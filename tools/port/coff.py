#!/usr/bin/env python3
"""tools/port/coff.py: the COFF (Windows x86_64) half of the portable link's weaken step.

llvm-objcopy has no --weaken-symbol for COFF ("option is not supported for COFF"), so tools/port/link_tables.py's
weaken.cmake entries go through this script instead. It gives a symbol exactly what --weaken-symbol gives on ELF/Mach-O:
the object keeps its own copy (here: static, so it cannot collide) while every reference to the name resolves to the
definition another object links -- the row owner, as MSVC's /FORCE:MULTIPLE kept it. Renaming definitions and references
alike (llvm-objcopy --redefine-sym) would NOT do: the object would call its own copy (a function-local static or a
duplicated const table would silently split from the shared one; probed 2026-11-23, `1 2 1 1`).

The surgery, per weakened symbol:
  * its defined External symbol record becomes Static (local); its aux records stay where they are;
  * one undefined External record with the same name is appended to the symbol table (the string table is last, so only
    its file offset moves; every name field in the appended record is copied verbatim from the original);
  * relocations against the old record are retargeted to the appended one -- except in .debug$*, .pdata, .xdata,
    .gxdata, which describe the local body itself and must keep pointing at it.

Also the COMDAT half of link_tables.py's duplicate detection: llvm-nm -P prints a COMDAT definition as plain T/B/D,
but the linker folds COMDATs by itself (the section symbol's aux section-definition Selection is nonzero), so those are
weak here, not strong.

usage: coff.py weaken <in.obj> <out.obj> <symbol>...
       coff.py comdat <obj>...          # one "path<TAB>symbol" line per COMDAT global definition
"""
import struct
import sys


class Coff:
    def __init__(self, path):
        self.path = path
        self.data = bytearray(open(path, 'rb').read())
        d = self.data
        self.num_sections = struct.unpack_from('<H', d, 2)[0]
        self.sym_ptr = struct.unpack_from('<I', d, 8)[0]
        self.num_syms = struct.unpack_from('<I', d, 12)[0]
        self.sec_ptr = 20 + struct.unpack_from('<H', d, 16)[0]
        self.str_ptr = self.sym_ptr + 18 * self.num_syms
        self.str_size = struct.unpack_from('<I', d, self.str_ptr)[0] if self.str_ptr + 4 <= len(d) else 4

    def name(self, off):
        raw = self.data[off:off + 8]
        if raw[:4] == b'\0\0\0\0':
            so = struct.unpack_from('<I', raw, 4)[0]
            if so == 0:
                return ''
            end = self.data.index(b'\0', self.str_ptr + so)
            return self.data[self.str_ptr + so:end].decode('latin-1')
        return raw.rstrip(b'\0').decode('latin-1')

    def section_name(self, off):
        raw = self.data[off:off + 8]
        if raw[:1] == b'/':                      # long name: "/<decimal offset into the string table>"
            n = int(raw[1:].rstrip(b'\0'))
            end = self.data.index(b'\0', self.str_ptr + n)
            return self.data[self.str_ptr + n:end].decode('latin-1')
        return raw.rstrip(b'\0').decode('latin-1')

    def sections(self):
        """[(index, name, characteristics, ptr_relocations, nrelocations)] (1-based index, as relocations use it)"""
        res = []
        for i in range(self.num_sections):
            off = self.sec_ptr + 40 * i
            ptr = struct.unpack_from('<I', self.data, off + 24)[0]
            n = struct.unpack_from('<H', self.data, off + 32)[0]
            ch = struct.unpack_from('<I', self.data, off + 36)[0]
            res.append((i + 1, self.section_name(off), ch, ptr, n))
        return res

    def symbols(self):
        """[(record offset, index, name, value, section, type, storage class, aux count)] in symbol table order.
        index counts every record, auxiliaries included, because relocations address that index."""
        res, i = [], 0
        while i < self.num_syms:
            off = self.sym_ptr + 18 * i
            value, section, typ, storage, naux = struct.unpack_from('<IhHBB', self.data, off + 8)
            res.append((off, i, self.name(off), value, section, typ, storage, naux))
            i += 1 + naux
        return res

    def comdat_sections(self):
        """{section index: selection} for COMDAT sections (the section symbol's aux section definition)."""
        sel = {}
        for off, _, _, _, section, _, storage, naux in self.symbols():
            if storage == 3 and naux >= 1:                        # IMAGE_SYM_CLASS_STATIC section symbol
                s = self.data[off + 18 + 14]                      # AuxSectionDef.Selection
                if s:
                    sel[section] = s
        return sel


def comdat_names(path):
    """Global definitions the linker folds as COMDAT (weak): nm prints them as ordinary T/B/D."""
    o = Coff(path)
    comdat = o.comdat_sections()
    return {nm for _, _, nm, _, section, _, storage, _ in o.symbols()
            if storage == 2 and section in comdat}


# relocations the local body's own metadata uses: they must keep addressing the (now static) definition
KEEP_LOCAL = ('.debug', '.pdata', '.xdata', '.gxdata')


def weaken(inpath, outpath, names):
    o = Coff(inpath)
    d = o.data
    wanted = set(names)
    syms = o.symbols()
    defs = [s for s in syms if s[2] in wanted and s[6] == 2 and s[4] > 0]
    missing = wanted - {s[2] for s in defs}
    if missing:
        sys.exit('coff.py: %s does not define (as a global) %s' % (inpath, ', '.join(sorted(missing))))

    added = bytearray()
    retarget = {}
    for off, index, _, _, _, typ, _, _ in defs:
        name_field = bytes(d[off:off + 8])            # inline name or string-table offset: copied as is
        retarget[index] = o.num_syms + len(added) // 18
        added += name_field + struct.pack('<IhHBB', 0, 0, typ, 2, 0)   # undefined External, same name
        d[off + 16] = 3                                                 # IMAGE_SYM_CLASS_STATIC: local copy

    for index, name, ch, ptr, n in o.sections():
        if not ptr or not n or name.startswith(KEEP_LOCAL):
            continue
        start = 0
        if n == 0xFFFF and (ch & 0x01000000):         # IMAGE_SCN_LNK_NRELOC_OVFL: true count in the first entry
            n, start = struct.unpack_from('<I', d, ptr)[0] - 1, 1
        for k in range(n):
            roff = ptr + 10 * (start + k)
            sym = struct.unpack_from('<I', d, roff + 4)[0]
            if sym in retarget:
                struct.pack_into('<I', d, roff + 4, retarget[sym])

    struct.pack_into('<I', d, 12, o.num_syms + len(added) // 18)
    open(outpath, 'wb').write(bytes(d[:o.str_ptr]) + bytes(added) + bytes(d[o.str_ptr:]))


def main():
    if len(sys.argv) < 2:
        sys.exit('usage: coff.py weaken <in.obj> <out.obj> <symbol>... | coff.py comdat <obj>...')
    cmd = sys.argv[1]
    if cmd == 'comdat':
        for path in sys.argv[2:]:
            for nm in sorted(comdat_names(path)):
                print('%s\t%s' % (path, nm))
    elif cmd == 'weaken' and len(sys.argv) >= 5:
        weaken(sys.argv[2], sys.argv[3], sys.argv[4:])
    else:
        sys.exit('usage: coff.py weaken <in.obj> <out.obj> <symbol>... | coff.py comdat <obj>...')


if __name__ == '__main__':
    main()
