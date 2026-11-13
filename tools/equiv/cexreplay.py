#!/usr/bin/env python3
"""Concrete replay of an angr counterexample (tools/equiv/cexmodel.py) under unicorn: does the difference the model
found really happen when both versions run on that input?

  uv run python tools/equiv/cexreplay.py work/equiv/cex/<va>.json        # a file quick.py saved; prints the replay

Both versions run at the original's VA (ours as angr_layer.prepare patched it: every relocated field holds the
original address it is paired with) on one concrete environment taken from the model:
  - registers: the model's in_<reg>; DF = 0; x87 empty, control word 0x027F (53-bit precision, round to nearest:
    the MSVC CRT's setting and the angr layer's 64-bit model);
  - memory: every byte not yet written is read from the model's memory array of the current epoch (M0 at entry,
    M<k+1> after the call that is event #k), lazily, on both sides alike;
  - a call is not executed: it is an event (target, esp, registers, memory); the callee's effect is the model's
    (eax/ecx/edx from c<k>_<reg>, every GPR at L2; flags; esp += 4 + ret N; its x87 pops and float result) and it
    rewrites memory outside the caller's frame with M<k+1> (the same values for both versions, so a callee given
    the same inputs behaves the same). The caller's frame [esp at the call + 4 + W, entry esp + 4 + own W) is left
    alone (a C++ callee cannot see it).
That environment is one possible real execution (a deterministic callee, memory with the model's values), so a
difference seen here is a real difference of the two functions on it; one not seen here is NOT confirmed (the
concrete callee/memory choices may differ from the model's).

Observables are the angr layer's (LAYERS.md, L1/L2): the sequence of events; at each call its target, esp, the
argument registers the callee reads (eax/ecx/edx when unknown: a difference there is marked "assumed"), the x87
depth, and memory at and above esp outside the caller's frame (the argument window is compared); at a return eax
(not for a void function whose original writes eax), edx for 64-bit returns, ebx/esi/edi/ebp, esp afterwards, the
return target, the x87 depth and st0; memory at and above the final esp. For each difference it names the
instruction that last wrote the differing value on each side (the producer) and the last conditional branches.
"""
import json, os, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

GPR = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
SAVED = ["ebx", "esi", "edi", "ebp"]
STUB = 0xF0000000                       # callee stubs (x87 effects) and their data; never model memory
BUDGET = 400000
FAM = {}
for _f, _p in (("eax", "al ah ax"), ("ecx", "cl ch cx"), ("edx", "dl dh dx"), ("ebx", "bl bh bx"),
               ("esi", "si sil"), ("edi", "di dil"), ("ebp", "bp bpl"), ("esp", "sp spl")):
    FAM[_f] = _f
    for _x in _p.split():
        FAM[_x] = _f


class Unsupported(Exception):
    pass


def _md():
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    return md


class Model:
    def __init__(self, model):
        self.bv = {k: int(v, 16) for k, v in (model.get("bv") or {}).items()}
        self.arr = {}
        for k, (ent, dflt) in (model.get("arr") or {}).items():
            self.arr[k] = ({a & 0xffffffff: b for a, b in ent}, dflt)

    def byte(self, epoch, addr):
        ent, d = self.arr.get("M%s" % epoch, ({}, 0))
        return ent.get(addr & 0xffffffff, d) & 0xff

    def val(self, name, default=0):
        return self.bv.get(name, default)


class Side:
    """one version running on the model's environment"""

    def __init__(self, name, code, base, job, model):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, \
            UC_HOOK_MEM_UNMAPPED
        import unicorn.x86_const as U
        self.U = U
        self.name, self.code, self.base, self.end = name, code, base, base + len(code)
        self.job, self.m = job, model
        self.md = _md()
        self.ins = {i.address: i for i in self.md.disasm(code, base)}
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.pages = set()
        lo, hi = base & ~0xfff, (self.end + 0xfff) & ~0xfff
        self.uc.mem_map(lo, hi - lo)
        self.pages |= set(range(lo, hi, 0x1000))
        self.uc.mem_write(base, code)
        self.code_lo, self.code_hi = lo, hi
        self.uc.mem_map(STUB, 0x10000)
        self.stub_at = STUB
        self.epoch = "0"
        self.havocs = []                # (epoch, protected lo, hi) per callee
        self.valid = set()              # bytes whose concrete content is current (written, or read from the model)
        self.writes = {}                # addr -> pc of the last store by this version (since the last havoc there)
        self.last = {}                  # register family -> pc (or "call#k") of its last writer
        self.branches = []              # (pc, taken)
        self.events = []
        self.steps = 0
        self.pending_br = None
        self.done = None
        self.err = None
        self.esp0 = model.val("in_esp")
        self.alias = None
        self.taint = {"esp"}            # registers holding an address computed from esp (frame addresses)
        self.tmem = set()               # memory bytes holding one
        self.uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._unmapped)
        self.uc.hook_add(UC_HOOK_MEM_READ, self._read)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self._write)
        self.uc.hook_add(UC_HOOK_CODE, self._code, begin=base, end=self.end - 1)
        R = {"eax": U.UC_X86_REG_EAX, "ecx": U.UC_X86_REG_ECX, "edx": U.UC_X86_REG_EDX, "ebx": U.UC_X86_REG_EBX,
             "esp": U.UC_X86_REG_ESP, "ebp": U.UC_X86_REG_EBP, "esi": U.UC_X86_REG_ESI, "edi": U.UC_X86_REG_EDI}
        self.R = R
        for r in GPR:
            self.uc.reg_write(R[r], model.val("in_" + r))
        fl = model.val("in_flags") & 0x8d5
        self.uc.reg_write(U.UC_X86_REG_EFLAGS, fl | 0x2)
        self.uc.reg_write(U.UC_X86_REG_FPCW, 0x027F)

    # ---------------------------------------------------------------- memory
    def _unmapped(self, uc, access, addr, size, value, ud):
        for p in range(addr & ~0xfff, (addr + max(size, 1) + 0xfff) & ~0xfff, 0x1000):
            if p not in self.pages and not STUB <= p < STUB + 0x10000:
                uc.mem_map(p, 0x1000)
                self.pages.add(p)
        return True

    def _in_code(self, a):
        return self.code_lo <= a < self.code_hi or STUB <= a < STUB + 0x10000

    def materialize(self, addr, size):
        for k in range(size):
            a = (addr + k) & 0xffffffff
            if a in self.valid or self._in_code(a):
                continue
            if (a & ~0xfff) not in self.pages:
                self.uc.mem_map(a & ~0xfff, 0x1000)
                self.pages.add(a & ~0xfff)
            self.uc.mem_write(a, bytes([self.m.byte(self.epoch_of(a), a)]))
            self.valid.add(a)

    def epoch_of(self, a):
        """the memory epoch a byte nobody has touched comes from: the latest callee havoc that did not protect it"""
        for ep, lo, hi in reversed(self.havocs):
            if not ((a - lo) & 0xffffffff) < ((hi - lo) & 0xffffffff):
                return ep
        return "0"

    def _read(self, uc, access, addr, size, value, ud):
        self.alias_check(uc, addr)
        self.materialize(addr, size)

    def alias_check(self, uc, addr):
        """a C++ input pointer never points into the function's own stack: below its entry esp (1 MB) or at its
        return address and parameters. An access there through any base register but esp (and ebp after
        `mov ebp, esp`) means the model's input is not a realistic one (the angr layer's trust rule)"""
        if self.alias or not ((addr - (self.esp0 - 0x100000)) & 0xffffffff) < 0x100000 + 4 + (
                self.job.get("argw") or 0):
            return
        pc = uc.reg_read(self.U.UC_X86_REG_EIP)
        i = self.ins.get(pc)
        if i is None or i.mnemonic in ("push", "pop", "call", "ret", "retn", "leave", "enter") or \
                i.mnemonic.startswith(("rep", "movs", "stos")):
            return
        from capstone import x86_const as X
        for o in i.operands:
            if o.type == X.X86_OP_MEM:
                b = FAM.get(i.reg_name(o.mem.base)) if o.mem.base else None
                x = FAM.get(i.reg_name(o.mem.index)) if o.mem.index else None
                if b in self.taint or x in self.taint or (b is None and x is None):
                    return                      # an address computed from esp (a frame address of its own)
                self.alias = "+%x %s %s reaches the function's own stack (%#x)" % (
                    pc - self.base, i.mnemonic, i.op_str, addr)
                return

    def _write(self, uc, access, addr, size, value, ud):
        self.alias_check(uc, addr)
        pc = uc.reg_read(self.U.UC_X86_REG_EIP)
        for k in range(size):
            a = (addr + k) & 0xffffffff
            if self._in_code(a):
                if STUB <= a < STUB + 0x10000:
                    continue
                self.err = "writes into its own code at %#x" % a
                uc.emu_stop()
                return
            self.valid.add(a)
            self.writes[a] = pc

    def read(self, addr, size):
        self.materialize(addr, size)
        return bytes(self.uc.mem_read(addr & 0xffffffff, size))

    def u32(self, addr):
        return struct.unpack("<I", self.read(addr, 4))[0]

    def reg(self, r):
        return self.uc.reg_read(self.R[r])

    # ---------------------------------------------------------------- x87
    def x87_top(self):
        return (self.uc.reg_read(self.U.UC_X86_REG_FPSW) >> 11) & 7

    def x87_depth(self):
        return (8 - self.x87_top()) & 7

    def st(self, i):
        phys = (self.x87_top() + i) & 7
        return tuple(self.uc.reg_read(self.U.UC_X86_REG_FP0 + phys))

    # ---------------------------------------------------------------- execution
    def _code(self, uc, addr, size, ud):
        try:
            self._step(uc, addr)
        except Unsupported as e:
            self.err = str(e)
            uc.emu_stop()

    def _step(self, uc, addr):
        self.steps += 1
        if self.steps > BUDGET:
            self.err = "more than %d instructions (a loop the model bounded?)" % BUDGET
            uc.emu_stop()
            return
        if self.pending_br is not None:
            pc, fall = self.pending_br
            self.branches.append((pc, addr != fall))
            self.pending_br = None
        i = self.ins.get(addr)
        if i is None:
            raise Unsupported("%s runs into the middle of an instruction at %#x" % (self.name, addr))
        m = i.mnemonic
        from capstone import x86_const as X
        tgt = i.operands[0].imm & 0xffffffff if (i.operands and i.operands[0].type == X.X86_OP_IMM and
                                                  (m == "call" or m.startswith("j"))) else None
        if m == "call":
            if tgt is not None and self.base <= tgt < self.end:
                raise Unsupported("a call into the function itself")
            return self.call(i, tgt, tail=False)
        if m == "jmp" and (tgt is None or not self.base <= tgt < self.end):
            if tgt is None:
                raise Unsupported("indirect jump at +%x (switch table?)" % (addr - self.base))
            return self.call(i, tgt, tail=True)
        if m in ("ret", "retn"):
            n = int(i.op_str, 16) if i.op_str else 0
            esp = self.reg("esp")
            self.event("ret", i, target=self.u32(esp), esp_after=(esp + 4 + n) & 0xffffffff)
            self.done = "ret"
            uc.emu_stop()
            return
        if m in ("int3", "ud2", "hlt", "int"):
            self.events.append({"kind": "trap", "pc": addr})
            self.done = "trap"
            uc.emu_stop()
            return
        if m.startswith("j") or m.startswith("loop"):
            if tgt is not None and m != "jmp":
                if not self.base <= tgt < self.end:
                    raise Unsupported("conditional tail jump")
                self.pending_br = (addr, addr + i.size)
        try:
            _rd, wr = i.regs_access()
            for r in wr:
                f = FAM.get(i.reg_name(r))
                if f:
                    self.last[f] = addr
        except Exception:
            pass
        self.track_taint(i)
        if m.startswith("f"):
            self.last["x87"] = addr

    def mem_snap(self):
        """the memory as this event sees it: every touched byte's content, the writers, and the epoch the others
        come from"""
        vals = {}
        for a in self.valid:
            vals[a] = self.uc.mem_read(a, 1)[0]
        return {"vals": vals, "writes": dict(self.writes), "nhavoc": len(self.havocs)}

    def mem_at(self, snap, a):
        if a in snap["vals"]:
            return snap["vals"][a]
        for ep, lo, hi in reversed(self.havocs[:snap["nhavoc"]]):
            if not ((a - lo) & 0xffffffff) < ((hi - lo) & 0xffffffff):
                return self.m.byte(ep, a)
        return self.m.byte("0", a)

    def ea(self, i, o):
        a = o.mem.disp
        if o.mem.base:
            a += self.reg(FAM[i.reg_name(o.mem.base)])
        if o.mem.index:
            a += self.reg(FAM[i.reg_name(o.mem.index)]) * o.mem.scale
        return a & 0xffffffff

    def track_taint(self, i):
        """frame addresses (computed from esp) through registers and memory, before i executes (its inputs)"""
        from capstone import x86_const as X
        try:
            rd, wr = i.regs_access()
        except Exception:
            return
        rd = {FAM.get(i.reg_name(r)) for r in rd} - {None}
        wr = {FAM.get(i.reg_name(r)) for r in wr} - {None, "esp"}
        m = i.mnemonic
        mems = [o for o in i.operands if o.type == X.X86_OP_MEM]
        if m == "push" and i.operands:
            o = i.operands[0]
            t = (o.type == X.X86_OP_REG and FAM.get(i.reg_name(o.reg)) in self.taint)
            esp = self.reg("esp")
            for k in range(4):
                (self.tmem.add if t else self.tmem.discard)((esp - 4 + k) & 0xffffffff)
            return
        if m == "pop":
            esp = self.reg("esp")
            t = any(((esp + k) & 0xffffffff) in self.tmem for k in range(4))
            for r in wr:
                (self.taint.add if t else self.taint.discard)(r)
            return
        if mems and m != "lea":
            a = self.ea(i, mems[0])
            if i.operands[0].type == X.X86_OP_MEM:          # a store
                src = [FAM.get(i.reg_name(o.reg)) for o in i.operands[1:] if o.type == X.X86_OP_REG]
                t = bool(src) and src[0] in self.taint and m == "mov"
                for k in range(mems[0].size or 4):
                    (self.tmem.add if t else self.tmem.discard)((a + k) & 0xffffffff)
                return
            t = m == "mov" and any(((a + k) & 0xffffffff) in self.tmem for k in range(4))
            for r in wr:
                (self.taint.add if t else self.taint.discard)(r)
            return
        if m in ("xor", "sub") and len(i.operands) == 2 and i.operands[0].type == i.operands[1].type == X.X86_OP_REG \
                and i.operands[0].reg == i.operands[1].reg:
            t = False
        else:
            t = bool((rd - {"eflags"}) & self.taint)
        for r in wr:
            (self.taint.add if t else self.taint.discard)(r)

    def snapshot(self):
        return {r: self.reg(r) for r in GPR}

    def event(self, kind, i, **kw):
        ev = dict(kind=kind, pc=i.address, regs=self.snapshot(), last=dict(self.last), depth=self.x87_depth(),
                  st0=self.st(0) if self.x87_depth() else None, nbr=len(self.branches), mem=self.mem_snap(), **kw)
        self.events.append(ev)
        return ev

    def call(self, i, tgt, tail):
        k = len(self.events)
        from capstone import x86_const as X
        if tgt is None:
            op = i.operands[0]
            if op.type == X.X86_OP_REG:
                tgt = self.reg(FAM[i.reg_name(op.reg)])
            elif op.type == X.X86_OP_MEM and op.mem.segment == 0:
                a = op.mem.disp & 0xffffffff
                if op.mem.base:
                    a += self.reg(FAM[i.reg_name(op.mem.base)])
                if op.mem.index:
                    a += self.reg(FAM[i.reg_name(op.mem.index)]) * op.mem.scale
                tgt = self.u32(a & 0xffffffff)
            else:
                raise Unsupported("call operand at +%x" % (i.address - self.base))
        f = dict(self.job["facts"].get("%x" % tgt, {})) if i.operands[0].type == X.X86_OP_IMM else {}
        if i.operands[0].type == X.X86_OP_MEM and i.operands[0].mem.base == 0 and i.operands[0].mem.index == 0:
            f = dict(self.job["facts"].get("m%x" % (i.operands[0].mem.disp & 0xffffffff), {}))
        esp0 = self.reg("esp")
        esp_ev = esp0 if tail else (esp0 - 4) & 0xffffffff
        if not tail:
            self.uc.mem_write(esp_ev, struct.pack("<I", self.m.val("ra%d" % k)))
            for b in range(4):
                self.valid.add(esp_ev + b)
                self.writes.pop(esp_ev + b, None)
            self.uc.reg_write(self.R["esp"], esp_ev)
        retn = f.get("retn")
        if retn is None:
            retn = self.m.val("rn%d" % k, None)
            if retn is None:
                raise Unsupported("call #%d: the callee's ret N is unknown" % k)
        W = f.get("W")
        reads = f.get("reads")
        guess = reads is None or f.get("guess") or (i.operands[0].type != X.X86_OP_IMM and "reads" not in f)
        if reads is None:
            reads = ["eax", "ecx", "edx"] if i.operands[0].type == X.X86_OP_IMM or "reads" in f else ["ecx", "edx"]
        self.event("call", i, target=tgt, esp=esp_ev, W=W, reads=list(reads), guess=bool(guess),
                   name=f.get("name", ""), tail=tail)
        if f.get("noreturn"):
            self.done = "noreturn"
            self.uc.emu_stop()
            return
        # the callee's effects (the model's c<k>_* values)
        L2 = self.job["level"] == "L2"
        kills = [r for r in GPR if r != "esp"] if L2 else ["eax", "ecx", "edx"]
        if f.get("chkstk"):
            kills = ["eax"]
        for r in kills:
            self.uc.reg_write(self.R[r], self.m.val("c%d_%s" % (k, r)))
            self.last[r] = "call#%d" % k
            self.taint.discard(r)
        self.uc.reg_write(self.U.UC_X86_REG_EFLAGS, (self.m.val("c%d_flags" % k) & 0x8d5) | 0x2)
        # memory: outside the caller's frame, the callee leaves M<k+1>
        prot_lo = (esp_ev + 4 + (W or 0)) & 0xffffffff
        prot_hi = (self.esp0 + 4 + (self.job.get("argw") or 0)) & 0xffffffff
        self.epoch = str(k + 1)
        self.havocs.append((self.epoch, prot_lo, prot_hi))
        for a in list(self.valid):
            if not (prot_lo <= a < prot_hi) and not self._in_code(a):
                self.valid.discard(a)
                self.writes.pop(a, None)
        ret_to = None if tail else i.address + i.size
        if f.get("chkstk"):
            self.uc.reg_write(self.R["esp"], (esp0 - self.events[-1]["regs"]["eax"]) & 0xffffffff)
        else:
            self.uc.reg_write(self.R["esp"], (esp_ev + 4 + retn) & 0xffffffff)
        fret = bool(f.get("fret"))
        fd = f.get("fdelta", -1 if fret else 0)
        pops = fd + (1 if fret else 0)
        stub = b""
        if pops < 0 or pops > 2:
            raise Unsupported("call #%d: x87 effect %s not modelled here" % (k, fd))
        stub += b"\xdd\xd8" * pops                                   # fstp st(0)
        if fret:
            top_after = (self.x87_top() + pops - 1) & 7
            v = (self.m.val("c%d_fpreg" % k) >> (64 * top_after)) & ((1 << 64) - 1)
            data = self.stub_at + 0x800
            self.uc.mem_write(data, struct.pack("<Q", v))
            stub += b"\xdd\x05" + struct.pack("<I", data)              # fld qword ptr [data]
            self.last["x87"] = "call#%d" % k
        if tail:
            if stub:
                raise Unsupported("tail call with an x87 effect")
            # the callee returned to our caller
            esp = self.reg("esp")
            self.events.append(dict(kind="ret", pc=i.address, regs=self.snapshot(), last=dict(self.last),
                                    depth=self.x87_depth(), st0=self.st(0) if self.x87_depth() else None,
                                    target=None, esp_after=esp, nbr=len(self.branches), mem=self.mem_snap()))
            self.done = "ret"
            self.uc.emu_stop()
            return
        if stub:
            at = self.stub_at
            self.stub_at += 0x20
            stub += b"\xe9" + struct.pack("<i", ret_to - (at + len(stub) + 5))
            self.uc.mem_write(at, stub)
            self.uc.reg_write(self.U.UC_X86_REG_EIP, at)
        else:
            self.uc.reg_write(self.U.UC_X86_REG_EIP, ret_to)

    def run(self):
        from unicorn import UcError
        pc = self.base
        try:
            self.uc.emu_start(pc, 0xFFFFFFF0, count=BUDGET * 4)
        except UcError as e:
            if not self.done and not self.err:
                self.err = "emulation stopped: %s at %#x" % (e, self.uc.reg_read(self.U.UC_X86_REG_EIP))
        if not self.done and not self.err:
            self.err = "did not reach a return"
        return self


# -------------------------------------------------------------------- comparison
def compare(job, A, B):
    """-> the first difference {what, event, kind, orig, ours, addr?, note?} or None"""
    level = job["level"]
    L2 = level == "L2"
    ka, kb = [e["kind"] for e in A.events], [e["kind"] for e in B.events]
    n = min(len(ka), len(kb))
    for k in range(n):
        x, y = A.events[k], B.events[k]
        tag = "%s#%d" % (x["kind"], k)
        if x["kind"] != y["kind"]:
            return {"what": "event sequence", "event": k, "kind": "sequence", "orig": ka, "ours": kb}
        if x["kind"] == "trap":
            continue
        if x["kind"] == "call":
            if x["target"] != y["target"]:
                return {"what": tag + " target", "event": k, "kind": "target", "orig": x["target"],
                        "ours": y["target"]}
            if (x["esp"] - A.esp0) & 0xffffffff != (y["esp"] - B.esp0) & 0xffffffff:
                return {"what": tag + " esp", "event": k, "kind": "esp", "orig": x["esp"], "ours": y["esp"]}
            regs = [r for r in GPR if r != "esp"] if L2 else sorted(set(x["reads"]) | set(y["reads"]))
            for r in regs:
                if x["regs"][r] != y["regs"][r]:
                    return {"what": "%s %s" % (tag, r), "event": k, "kind": "reg", "reg": r, "orig": x["regs"][r],
                            "ours": y["regs"][r], "assumed": bool(x["guess"]) and not L2}
            if x["depth"] != y["depth"]:
                return {"what": tag + " x87 depth", "event": k, "kind": "x87", "orig": x["depth"],
                        "ours": y["depth"]}
            d = mem_diff(job, A, B, x["esp"], x, y, frame=(x["esp"], (A.esp0 + 4 + (job.get("argw") or 0))),
                         argw=x["W"])
            if d:
                d.update(what=tag + " memory", event=k)
                return d
        elif x["kind"] == "ret":
            if (x["target"] is None) != (y["target"] is None):
                return {"what": "tail call vs return at " + tag, "event": k, "kind": "sequence", "orig": ka,
                        "ours": kb}
            if x["target"] != y["target"]:
                return {"what": tag + " return target", "event": k, "kind": "target", "orig": x["target"],
                        "ours": y["target"]}
            if (x["esp_after"] - A.esp0) & 0xffffffff != (y["esp_after"] - B.esp0) & 0xffffffff:
                return {"what": tag + " esp", "event": k, "kind": "esp", "orig": x["esp_after"],
                        "ours": y["esp_after"]}
            if L2:
                regs = [r for r in GPR if r != "esp"]
            else:
                regs = ([] if job.get("void_eax_dead") else ["eax"]) + SAVED + (["edx"] if job.get("ret64") else [])
            for r in regs:
                if x["regs"][r] != y["regs"][r]:
                    return {"what": "%s %s" % (tag, r), "event": k, "kind": "reg", "reg": r, "orig": x["regs"][r],
                            "ours": y["regs"][r]}
            if x["depth"] != y["depth"]:
                return {"what": tag + " x87 depth", "event": k, "kind": "x87", "orig": x["depth"],
                        "ours": y["depth"]}
            if x["st0"] != y["st0"]:
                return {"what": tag + " st0", "event": k, "kind": "x87", "orig": _f80(x["st0"]),
                        "ours": _f80(y["st0"])}
            d = mem_diff(job, A, B, x["esp_after"], x, y, frame=None, argw=None)
            if d:
                d.update(what=tag + " memory", event=k)
                return d
    if len(ka) != len(kb):
        return {"what": "event sequence", "event": n, "kind": "sequence", "orig": ka, "ours": kb}
    return None


def _f80(t):
    if not t:
        return None
    mant, ex = t
    if ex & 0x7fff == 0 and mant == 0:
        return 0.0
    sign = -1 if ex & 0x8000 else 1
    try:
        return sign * mant * 2.0 ** ((ex & 0x7fff) - 16383 - 63)
    except OverflowError:
        return float("inf") * sign


def mem_diff(job, A, B, lo, x, y, frame, argw):
    """memory at and above lo that differs between the versions at events x (original) and y (ours), among the
    bytes either wrote; L1 calls skip the caller's frame above the argument window"""
    L2 = job["level"] == "L2"
    sa, sb = x["mem"], y["mem"]
    cand = sorted(a for a in set(sa["writes"]) | set(sb["writes"]) if a >= lo)        # unsigned, as the model's UGE
    for a in cand:
        in_frame = frame is not None and ((a - frame[0]) & 0xffffffff) < ((frame[1] - frame[0]) & 0xffffffff)
        in_args = frame is not None and isinstance(argw, int) and \
            ((a - frame[0] - 4) & 0xffffffff) < argw
        if in_frame and not in_args and not L2 and isinstance(argw, int):
            continue                     # the caller's private frame (a C++ callee cannot see it)
        va, vb = A.mem_at(sa, a), B.mem_at(sb, a)
        if va != vb:
            return {"kind": "mem", "addr": a, "orig": va, "ours": vb,
                    "producer_mem": {"retail": sa["writes"].get(a), "ours": sb["writes"].get(a)},
                    "frame": bool(in_frame and not in_args)}
    return None


def replay(job, cex):
    """-> {"status": "confirmed"|"not-reproduced"|"unsupported", "why", "diff", "sides": {...}}"""
    model = cex.get("model")
    if not model:
        return {"status": "unsupported", "why": "the counterexample carries no model (%s)" %
                cex.get("model_error", "angr_layer's own worker")}
    try:
        import unicorn  # noqa: F401
    except ImportError:
        return {"status": "unsupported", "why": "unicorn is not installed in this environment"}
    m = Model(model)
    base = job["sides"]["retail"]["base"]
    A = Side("retail", bytes.fromhex(job["sides"]["retail"]["code"]), base, job, m).run()
    B = Side("ours", bytes.fromhex(job["sides"]["ours"]["code"]), base, job, m).run()
    out = {"sides": {}}
    for nm, S in (("retail", A), ("ours", B)):
        out["sides"][nm] = {"events": [{k: v for k, v in e.items() if k in ("kind", "pc", "target", "name")}
                                       for e in S.events], "steps": S.steps, "error": S.err,
                            "branches": S.branches[-6:]}
    d = compare(job, A, B)
    if d is not None and d["event"] < min(len(A.events), len(B.events)) + (1 if d["kind"] == "sequence" else 0):
        k = d["event"]
        ea = A.events[k] if k < len(A.events) else None
        eb = B.events[k] if k < len(B.events) else None
        d["at"] = {"retail": ea and ea["pc"], "ours": eb and eb["pc"]}
        if d["kind"] == "reg":
            d["producer"] = {"retail": ea["last"].get(d["reg"]), "ours": eb["last"].get(d["reg"])}
        elif d["kind"] == "x87":
            d["producer"] = {"retail": ea["last"].get("x87"), "ours": eb["last"].get("x87")}
        elif d["kind"] == "mem":
            d["producer"] = d.pop("producer_mem")
        elif d["kind"] in ("target", "sequence", "esp"):
            d["producer"] = {"retail": ea and ea["pc"], "ours": eb and eb["pc"]}
        d["branches"] = {"retail": A.branches[:ea["nbr"]][-3:] if ea else A.branches[-3:],
                         "ours": B.branches[:eb["nbr"]][-3:] if eb else B.branches[-3:]}
        weak = []
        if d.get("assumed"):
            weak.append("the register is read only if the unknown callee's convention reads it")
        if d.get("frame"):
            weak.append("the memory difference is in the caller's own frame")
        for S in (A, B):
            if S.alias:
                weak.append("unrealistic input: in the %s, %s" % ("original" if S is A else "ours", S.alias))
        if A.err or B.err:
            # a difference before an error is still a difference, but say what stopped
            out["note"] = "; ".join(x for x in (A.err and "original: " + A.err, B.err and "ours: " + B.err) if x)
        out.update(status="likely" if weak else "confirmed", why="; ".join(weak), diff=d)
        return out
    if A.err or B.err:
        out.update(status="unsupported", why="; ".join(x for x in (A.err and "original: " + A.err,
                                                                   B.err and "ours: " + B.err) if x))
        return out
    out.update(status="not-reproduced", why="both versions show the same observables on the model's input")
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    saved = json.load(open(sys.argv[1]))
    r = replay(saved["job"], saved["cex"])
    print(json.dumps({k: v for k, v in r.items()}, indent=1, default=str))
    try:
        import quick
        for ln in quick.describe_replay(r, saved):
            print(ln)
    except Exception as e:
        print("(no description: %s)" % e)
    return 0 if r["status"] == "confirmed" else 1


if __name__ == "__main__":
    sys.exit(main())
