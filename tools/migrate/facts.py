#!/usr/bin/env python3
"""Per-function facts for TU inference (cached in work/migrate/facts.json).

usage: facts.py [--rebuild]

For every .text function of the game (0x401000..0x535440 and the post-CRT area 0x547100..end of .text):
  start, size (Ghidra), end (next start), fill bytes after the code (int3 / nop / other), alignment,
  direct call/jmp targets, absolute references into .rdata / .data (.bss included), and immediates that
  point at function starts (callbacks). Also the .CRT$XC* initializer table and the post-CRT EH stubs.
"""
import bisect, csv, json, os, struct, sys
import capstone, pefile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
W = lambda *p: os.path.join(ROOT, *p)
IMAGE = W("work/mb.analysis.bin")
OUT = W("work/migrate/facts.json")

CRT_LO, CRT_HI = 0x535440, 0x547100


def sections(p):
    base = p.OPTIONAL_HEADER.ImageBase
    out = {}
    for s in p.sections:
        out[s.Name.rstrip(b"\0").decode()] = (base + s.VirtualAddress, base + s.VirtualAddress + s.Misc_VirtualSize)
    return out


_FULL = {"al": "eax", "ah": "eax", "ax": "eax", "eax": "eax", "dl": "edx", "dh": "edx", "dx": "edx",
         "edx": "edx", "bl": "ebx", "bh": "ebx", "bx": "ebx", "ebx": "ebx", "si": "esi", "esi": "esi",
         "di": "edi", "edi": "edi", "cl": "ecx", "ch": "ecx", "cx": "ecx", "ecx": "ecx"}


def regargs(md, code, a):
    """Registers read before written in the entry block (custom-convention arguments; ECX = this)."""
    written, first = set(), set()
    R, I = capstone.x86.X86_OP_REG, capstone.x86.X86_OP_IMM
    for ins in md.disasm(code[:200], a):
        ops = ins.operands
        if ins.mnemonic == "push" and ops and ops[0].type == R:
            continue  # callee-saved register spill
        if len(ops) == 2 and ops[0].type == R:
            if ins.mnemonic in ("xor", "sub") and ops[1].type == R and ops[0].reg == ops[1].reg:
                written.add(_FULL.get(md.reg_name(ops[0].reg))); continue
            if ins.mnemonic == "mov" and ops[1].type == R and ops[0].reg == ops[1].reg:
                continue  # 2-byte nop
            if ops[1].type == I and ((ins.mnemonic == "or" and ops[1].imm & 0xffffffff == 0xffffffff) or
                                     (ins.mnemonic == "and" and ops[1].imm == 0)):
                written.add(_FULL.get(md.reg_name(ops[0].reg))); continue
        try:
            rr, ww = ins.regs_access()
        except capstone.CsError:
            break
        for r in rr:
            x = _FULL.get(md.reg_name(r))
            if x and x not in written:
                first.add(x)
        for r in ww:
            x = _FULL.get(md.reg_name(r))
            if x:
                written.add(x)
        if ins.mnemonic.startswith("j") or ins.mnemonic in ("call", "ret"):
            break
    return sorted(first)


def build():
    p = pefile.PE(IMAGE, fast_load=True)
    base = p.OPTIONAL_HEADER.ImageBase
    sec = sections(p)
    tlo, thi = sec[".text"]
    text = p.get_data(tlo - base, thi - tlo)
    rlo, rhi = sec[".rdata"]
    dlo, dhi = sec[".data"]
    dsec = next(s for s in p.sections if s.Name.startswith(b".data"))
    dinit_hi = base + dsec.VirtualAddress + dsec.SizeOfRawData
    rows = list(csv.DictReader(open(W("work/index/functions.csv"))))
    starts = sorted(int(r["address"], 16) for r in rows if r["block"] == ".text")
    size = {int(r["address"], 16): int(r["size_bytes"]) for r in rows if r["block"] == ".text"}
    # lib names (CRT / Dinkumware byte-exact)
    lib = {}
    for line in open(W("symbols/lib_names.txt")):
        q = line.split("#", 1)[0].split()
        if len(q) >= 2:
            lib.setdefault(int(q[0], 16), q[1])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    sset = set(starts)
    funcs = []
    for i, a in enumerate(starts):
        nxt = starts[i + 1] if i + 1 < len(starts) else thi
        if CRT_LO <= a < CRT_HI:
            continue
        sz = max(1, min(size[a], nxt - a))
        code = text[a - tlo:a + sz - tlo]
        tail = text[a + sz - tlo:nxt - tlo]
        calls, tails, jccs, rrefs, drefs, frefs = set(), set(), set(), set(), set(), set()
        for ins in md.disasm(code, a):
            mn = ins.mnemonic
            vals = []
            for op in ins.operands:
                if op.type == capstone.x86.X86_OP_IMM:
                    vals.append(("imm", op.imm & 0xffffffff))
                elif op.type == capstone.x86.X86_OP_MEM:
                    if op.mem.disp:
                        vals.append(("mem", op.mem.disp & 0xffffffff))
            for kind, v in vals:
                if (mn == "call" or mn.startswith("j")) and kind == "imm":
                    if tlo <= v < thi and not (a <= v < a + sz):
                        (calls if mn == "call" else tails if mn == "jmp" else jccs).add(v)
                    continue
                if rlo <= v < rhi:
                    rrefs.add(v)
                elif dlo <= v < dhi:
                    # an immediate in .data's huge virtual range may be a plain constant: keep only
                    # memory operands and immediates inside the initialized part of .data
                    # (.bss immediates: 4-aligned and not a round constant such as 0x00cf0000)
                    if kind == "mem" or v < dinit_hi or (v % 4 == 0 and v & 0xffff and v != 0xffffff):
                        drefs.add(v)
                elif tlo <= v < thi and v in sset and v != a:
                    frefs.add(v)
        fill = {"cc": tail.count(0xCC), "90": tail.count(0x90)}
        fill["other"] = len(tail) - fill["cc"] - fill["90"]
        funcs.append({"va": a, "size": sz, "end": nxt, "fill": fill, "calls": sorted(calls), "tails": sorted(tails), "jcc": sorted(jccs),
                      "regargs": regargs(md, code, a),
                      "rdata": sorted(rrefs), "data": sorted(drefs), "fptr": sorted(frefs),
                      "lib": lib.get(a)})
    # initializer tables: find _cinit's ranges by scanning .data start (VC7.1 merges .CRT into .data)
    def dw(va):
        if tlo <= va < thi:
            return struct.unpack_from("<I", text, va - tlo)[0]
        return struct.unpack_from("<I", p.get_data(va - base, 4))[0]
    # rdata function-pointer tables (vtables etc.): runs of >=1 dwords pointing at function starts
    rdata = p.get_data(rlo - base, rhi - rlo)
    ptrs = {}
    for o in range(0, len(rdata) - 3, 4):
        v = struct.unpack_from("<I", rdata, o)[0]
        if v in sset:
            ptrs[rlo + o] = v
    data_init = p.get_data(dlo - base, min(dhi - dlo, 0x2f000))
    dptrs = {}
    for o in range(0, len(data_init) - 3, 4):
        v = struct.unpack_from("<I", data_init, o)[0]
        if v in sset:
            dptrs[dlo + o] = v
    # rdata->rdata/data pointers (string tables etc.)
    rr = {}
    for o in range(0, len(rdata) - 3, 4):
        v = struct.unpack_from("<I", rdata, o)[0]
        if rlo <= v < rhi or dlo <= v < dhi:
            rr[rlo + o] = v
    out = {"sections": sec, "funcs": funcs, "rdata_fptrs": ptrs, "data_fptrs": dptrs, "rdata_ptrs": rr}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(out, open(OUT, "w"))
    return out


def load(rebuild=False):
    if not rebuild and os.path.exists(OUT) and os.path.getmtime(OUT) > os.path.getmtime(W("work/index/functions.csv")):
        d = json.load(open(OUT))
        d["rdata_fptrs"] = {int(k): v for k, v in d["rdata_fptrs"].items()}
        d["data_fptrs"] = {int(k): v for k, v in d["data_fptrs"].items()}
        d["rdata_ptrs"] = {int(k): v for k, v in d["rdata_ptrs"].items()}
        return d
    build()
    return load()


if __name__ == "__main__":
    d = load("--rebuild" in sys.argv)
    print(len(d["funcs"]), "functions;", len(d["rdata_fptrs"]), "rdata fn ptrs;", len(d["rdata_ptrs"]), "rdata data ptrs")
