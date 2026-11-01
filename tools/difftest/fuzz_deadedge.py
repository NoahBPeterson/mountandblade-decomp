"""Differential fuzz of deadedge.Exec against unicorn: random straight-line sequences of the instructions the prover
models are run symbolically (z3) and concretely (unicorn) from the same random state; registers, the data window and
(for the instructions whose flags are fully defined) the flags must agree. usage: fuzz_deadedge.py [n] [seed]"""
import os, random, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import capstone
import unicorn
import unicorn.x86_const as U
import z3
import deadedge as DE

CODE, DATA, STACK = 0x10000, 0x20000, 0x30000
REGS = [("eax", U.UC_X86_REG_EAX), ("ecx", U.UC_X86_REG_ECX), ("edx", U.UC_X86_REG_EDX), ("ebx", U.UC_X86_REG_EBX),
        ("esp", U.UC_X86_REG_ESP), ("ebp", U.UC_X86_REG_EBP), ("esi", U.UC_X86_REG_ESI), ("edi", U.UC_X86_REG_EDI)]
FLAGBITS = {"cf": 0, "pf": 2, "zf": 6, "sf": 7, "of": 11}
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True
SAFE_REGS = [0, 1, 2, 3, 5, 6, 7]       # not esp (modrm 4)


def rr():
    return random.choice(SAFE_REGS)


def gen():
    """-> (bytes, flags_defined)"""
    k = random.randrange(14)
    r1, r2 = rr(), rr()
    if k == 0:       # reg,reg ALU (flags defined)
        op = random.choice([0x01, 0x03, 0x09, 0x0b, 0x21, 0x23, 0x29, 0x2b, 0x31, 0x33, 0x39, 0x3b, 0x85, 0x87, 0x89, 0x8b,
                            0x00, 0x02, 0x08, 0x0a, 0x20, 0x22, 0x28, 0x2a, 0x30, 0x32, 0x38, 0x3a, 0x84, 0x86, 0x88, 0x8a])
        return bytes([op, 0xC0 | r1 << 3 | r2]), op not in (0x87, 0x86, 0x89, 0x8b, 0x88, 0x8a)
    if k == 1:       # imm ALU
        op = random.choice([0, 1, 4, 5, 6, 7])
        if random.random() < .5:
            return bytes([0x83, 0xC0 | op << 3 | r1, random.randrange(256)]), True
        return bytes([0x81, 0xC0 | op << 3 | r1]) + random.getrandbits(32).to_bytes(4, "little"), True
    if k == 2:       # shifts by imm / 1
        op = random.choice([4, 5, 7])
        c = random.randrange(1, 32)
        if random.random() < .3:
            return bytes([0xD1, 0xC0 | op << 3 | r1]), True
        return bytes([0xC1, 0xC0 | op << 3 | r1, c]), "shift"
    if k == 3:       # shift by cl
        return bytes([0xD3, 0xC0 | random.choice([4, 5, 7]) << 3 | r1]), False
    if k == 4:       # inc/dec/neg/not
        c = random.randrange(4)
        if c == 0:
            return bytes([0x40 + r1]), True
        if c == 1:
            return bytes([0x48 + r1]), True
        return bytes([0xF7, 0xC0 | (3 if c == 2 else 2) << 3 | r1]), c == 2
    if k == 5:       # movzx/movsx
        return bytes([0x0F, random.choice([0xB6, 0xB7, 0xBE, 0xBF]), 0xC0 | r1 << 3 | r2]), False
    if k == 6:       # lea
        return bytes([0x8D, 0x44 | r1 << 3, random.choice([0x00, 0x40, 0x80, 0xC0]) | r2 << 3 | rr(), random.randrange(256)]), False
    if k == 7:       # setcc / cmovcc
        cc = random.randrange(16)
        if random.random() < .5:
            return bytes([0x0F, 0x90 | cc, 0xC0 | r2]), False
        return bytes([0x0F, 0x40 | cc, 0xC0 | r1 << 3 | r2]), False
    if k == 8:       # memory through ebx (= DATA)
        op = random.choice([0x01, 0x03, 0x29, 0x2b, 0x39, 0x3b, 0x89, 0x8b, 0x31, 0x33, 0x21, 0x09, 0x00, 0x02, 0x88, 0x8a])
        reg = random.choice([0, 1, 2, 6, 7, 5])
        return bytes([op, 0x43 | reg << 3, random.randrange(0, 56)]), op in (0x01, 0x03, 0x29, 0x2b, 0x39, 0x3b, 0x31, 0x33, 0x21, 0x09, 0x00, 0x02)
    if k == 9:       # push/pop
        return bytes([random.choice([0x50 + r1, 0x58 + r1])]), False
    if k == 10:      # imul
        return bytes([0x0F, 0xAF, 0xC0 | r1 << 3 | r2]), False
    if k == 11:      # cdq/cwde/cbw/cwd
        return random.choice([b"\x99", b"\x98", b"\x66\x98", b"\x66\x99"]), False
    if k == 12:      # mov imm / mov mem imm
        if random.random() < .5:
            return bytes([0xB8 + r1]) + random.getrandbits(32).to_bytes(4, "little"), False
        return bytes([0xC7, 0x43, random.randrange(0, 56)]) + random.getrandbits(32).to_bytes(4, "little"), False
    # 16-bit ALU
    op = random.choice([0x01, 0x03, 0x29, 0x2b, 0x31, 0x39])
    return bytes([0x66, op, 0xC0 | r1 << 3 | r2]), True


def one(seed):
    random.seed(seed)
    seq = [gen() for _ in range(random.randrange(1, 5))]
    code = b"".join(b for b, _ in seq)
    defined = all(d is True for _, d in seq)
    last_shift = seq[-1][1] == "shift" and all(d in (True, "shift") for _, d in seq)
    init = DE.fresh_state("in")
    st = init.copy()
    ex = DE.Exec(md, 0x1000, lambda t: False)
    n = 0
    for ins in md.disasm(code, CODE):
        ex.step(ins, st)
        st = ex.s
        n += 1
    assert n == len(seq), (code.hex(), n, len(seq))
    uc = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    uc.mem_map(CODE, 0x1000)
    uc.mem_map(DATA, 0x1000)
    uc.mem_map(STACK, 0x1000)
    uc.mem_write(CODE, code + b"\xf4")
    vals = {nm: random.choice([random.getrandbits(32), random.randrange(8), 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, random.getrandbits(8)])
            for nm, _ in REGS}
    vals["ebx"] = DATA
    vals["esp"] = STACK + 0x800
    mem = bytes(random.getrandbits(8) for _ in range(0x100))
    uc.mem_write(DATA, mem)
    uc.mem_write(STACK + 0x700, bytes(random.getrandbits(8) for _ in range(0x200)))
    stack0 = bytes(uc.mem_read(STACK + 0x700, 0x200))
    for nm, r in REGS:
        uc.reg_write(r, vals[nm])
    fl0 = random.getrandbits(32) & 0x8D5
    uc.reg_write(U.UC_X86_REG_EFLAGS, 0x202 | fl0)
    try:
        uc.emu_start(CODE, CODE + len(code))
    except unicorn.UcError:
        return None
    s = z3.Solver()
    for nm, _ in REGS:
        s.add(init.r[nm] == vals[nm])
    for f, b in FLAGBITS.items():
        s.add(init.f[f] == bool(fl0 >> b & 1))
    for i in range(0x100):
        s.add(z3.Select(init.mem, DATA + i) == mem[i])
    for i in range(0x200):
        s.add(z3.Select(init.mem, STACK + 0x700 + i) == stack0[i])
    assert s.check() == z3.sat
    bad = []

    def feasible(expr):        # the real outcome must be one the model allows (fresh symbols may take any value)
        s.push()
        s.add(expr)
        r = s.check()
        s.pop()
        return r == z3.sat

    for nm, r in REGS:
        real = uc.reg_read(r)
        if not feasible(st.r[nm] == real):
            bad.append((nm, hex(real)))
    for i in range(0x100):
        real = uc.mem_read(DATA + i, 1)[0]
        if not feasible(z3.Select(st.mem, DATA + i) == real):
            bad.append(("mem+%x" % i, real))
    for i in range(0x200):
        real = uc.mem_read(STACK + 0x700 + i, 1)[0]
        if not feasible(z3.Select(st.mem, STACK + 0x700 + i) == real):
            bad.append(("stk+%x" % i, real))
    if defined or last_shift:
        ef = uc.reg_read(U.UC_X86_REG_EFLAGS)
        for f, b in FLAGBITS.items():
            if last_shift and f == "of" and not defined:
                continue
            if not feasible(st.f[f] == bool(ef >> b & 1)):
                bad.append((f, bool(ef >> b & 1)))
    # all outputs jointly (a model that allows each one alone but not together would still be unsound)
    allx = [st.r[nm] == uc.reg_read(r) for nm, r in REGS] + [st.f[f] == bool(uc.reg_read(U.UC_X86_REG_EFLAGS) >> b & 1)
                                                            for f, b in FLAGBITS.items() if defined]
    if not bad and not feasible(z3.And(*allx)):
        bad.append(("joint", 0))
    return (code.hex(), bad) if bad else True


def main(n=2000, seed0=0):
    fails, ran = [], 0
    for sd in range(seed0, seed0 + n):
        r = one(sd)
        if r is None:
            continue
        ran += 1
        if r is not True:
            fails.append((sd, r))
    return ran, fails


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
    ran, fails = main(n, int(sys.argv[2]) if len(sys.argv) > 2 else 0)
    print("ran %d, %d mismatches" % (ran, len(fails)))
    for f in fails[:30]:
        print(f)
    sys.exit(1 if fails else 0)
