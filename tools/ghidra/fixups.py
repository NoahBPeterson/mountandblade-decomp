#!/usr/bin/env python3
"""Post-analysis fixups for the mb Ghidra project (idempotent; run after tools/ghidra_index.py new).

1. Jump tables Ghidra left unresolved (its limit is 1024 entries; the script interpreter at
   0x49fa10 has 2356): for every `jmp [idx*4 + table]` in .text, read the bound from the
   preceding `cmp idx, N; ja` (through an optional `movzx idx, byte [r + bytetable]` second
   level), add COMPUTED_JUMP references to every target, define the table as data, and fix up
   the owning function's body.
2. Functions Ghidra never created: targets of direct calls (E8) and of data pointers that land
   outside every function body on a decodable instruction (the image has no .reloc, so Spore's
   relocation-driven discovery finds nothing here).
Repeats 2 until nothing new is created.

usage: fixups.py <binary> <project_dir> <project_name>
"""
import struct, sys
import capstone
import pyghidra

pyghidra.start()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True


def find_switches(code, base):
    """-> [(jmp_va, table_va, n_entries)] from raw .text bytes."""
    out = []
    i = code.find(b"\xff\x24")
    while i != -1:
        modrm_sib = code[i + 2]
        if (modrm_sib & 7) == 5 and (modrm_sib >> 6) == 2:  # SIB: scale 4, no base, disp32
            idx_reg = (modrm_sib >> 3) & 7
            table = struct.unpack_from("<I", code, i + 3)[0]
            jva = base + i
            # decode the 48 bytes before the jmp at every alignment; keep a decode that ends on it
            n = None
            for back in range(48, 0, -1):
                ins = list(md.disasm(code[i - back:i + 7], jva - back))
                if not ins or ins[-1].address != jva:
                    continue
                cmp_n, byte_table = None, None
                for k in range(len(ins) - 1, -1, -1):
                    x = ins[k]
                    if x.mnemonic == "movzx" and "byte ptr" in x.op_str and byte_table is None:
                        op = x.operands[1]
                        if op.mem.disp >= 0x401000:
                            byte_table = op.mem.disp
                    if x.mnemonic == "cmp" and k + 1 < len(ins) and ins[k + 1].mnemonic == "ja" \
                            and x.operands[1].type == capstone.x86.X86_OP_IMM:
                        cmp_n = x.operands[1].imm
                        break
                if cmp_n is None:
                    continue
                if byte_table is not None:
                    bt_off = byte_table - base
                    n = max(code[bt_off:bt_off + cmp_n + 1]) + 1
                else:
                    n = cmp_n + 1
                break
            if n:
                out.append((jva, table, n))
        i = code.find(b"\xff\x24", i + 1)
    return out


def main(binary, pdir, name):
    with pyghidra.open_program(binary, project_location=pdir, project_name=name, analyze=False) as flat:
        import jpype
        from ghidra.app.cmd.disassemble import DisassembleCommand
        from ghidra.app.cmd.function import CreateFunctionCmd
        from ghidra.program.model.symbol import RefType, SourceType
        from ghidra.program.model.data import DWordDataType, ArrayDataType
        from ghidra.program.model.address import AddressSet
        p = flat.getCurrentProgram(); mon = flat.getMonitor()
        mem, listing, fm, rm = p.getMemory(), p.getListing(), p.getFunctionManager(), p.getReferenceManager()
        A = flat.toAddr
        text = mem.getBlock(".text")
        lo = text.getStart().getOffset()
        buf = jpype.JByte[int(text.getSize())]
        text.getBytes(text.getStart(), buf)
        code = bytes(b & 0xFF for b in buf)
        u32 = lambda va: struct.unpack_from("<I", code, va - lo)[0] if lo <= va < lo + len(code) - 3 else \
            mem.getInt(A(va)) & 0xFFFFFFFF

        tx = p.startTransaction("mb fixups")
        try:
            # 1. switches
            fixed = 0
            for jva, table, n in find_switches(code, lo):
                jaddr = A(jva)
                targets = [u32(table + 4 * k) for k in range(n)]
                k = next((k for k, t in enumerate(targets) if not lo <= t < lo + len(code)), n)
                if k < n:  # bound over-read (padding or the next table follows): keep the code prefix
                    print("switch %x: table %x truncated %d -> %d entries" % (jva, table, n, k))
                    n, targets = k, targets[:k]
                have = set(r.getToAddress().getOffset() for r in rm.getReferencesFrom(jaddr)
                           if r.getReferenceType().isComputed())
                if not n or set(targets) <= have:
                    continue
                if listing.getInstructionAt(jaddr) is None:
                    DisassembleCommand(jaddr, None, True).applyTo(p, mon)
                for t in sorted(set(targets)):
                    ta = A(t)
                    if listing.getInstructionAt(ta) is None:
                        DisassembleCommand(ta, None, True).applyTo(p, mon)
                    rm.addMemoryReference(jaddr, ta, RefType.COMPUTED_JUMP, SourceType.ANALYSIS, 0)
                ta = A(table)
                if listing.getDefinedDataAt(ta) is None and listing.getInstructionContaining(ta) is None:
                    try:
                        listing.clearCodeUnits(ta, A(table + 4 * n - 1), False)
                        listing.createData(ta, ArrayDataType(DWordDataType.dataType, n, 4))
                    except Exception as e:
                        print("table %x: %s" % (table, e))
                f = fm.getFunctionContaining(jaddr)
                if f is not None:
                    CreateFunctionCmd.fixupFunctionBody(p, f, mon)
                fixed += 1
                print("switch %x: table %x, %d entries (had %d targets), owner %s" % (
                    jva, table, n, len(have), f.getEntryPoint() if f else None))
            print("switches fixed:", fixed)

            # 2. missing functions
            data_ptrs = set()
            for blk in mem.getBlocks():
                if blk.getName() not in (".rdata", ".data") or not blk.isInitialized():
                    continue
                b = jpype.JByte[int(blk.getSize())]
                blk.getBytes(blk.getStart(), b)
                raw = bytes(x & 0xFF for x in b)
                for k in range(0, len(raw) - 3, 4):
                    v = struct.unpack_from("<I", raw, k)[0]
                    if lo <= v < lo + len(code):
                        data_ptrs.add(v)
            for rnd in range(5):
                cands = set()
                for k in range(len(code) - 5):
                    if code[k] == 0xE8:
                        t = lo + k + 5 + struct.unpack_from("<i", code, k + 1)[0]
                        if lo <= t < lo + len(code):
                            src = listing.getInstructionAt(A(lo + k))
                            if src is not None:  # only real call instructions
                                cands.add(t)
                cands |= data_ptrs
                made = 0
                for t in sorted(cands):
                    ta = A(t)
                    if fm.getFunctionAt(ta) is not None or fm.getFunctionContaining(ta) is not None:
                        continue
                    if listing.getDefinedDataContaining(ta) is not None:
                        continue
                    if listing.getInstructionAt(ta) is None:
                        if listing.getInstructionContaining(ta) is not None:
                            continue
                        DisassembleCommand(ta, None, True).applyTo(p, mon)
                        if listing.getInstructionAt(ta) is None:
                            continue
                    if CreateFunctionCmd(ta).applyTo(p, mon):
                        made += 1
                print("round %d: created %d functions" % (rnd, made))
                if not made:
                    break
        finally:
            p.endTransaction(tx, True)
        print("functions now:", fm.getFunctionCount())


if __name__ == "__main__":
    main(*sys.argv[1:4])
