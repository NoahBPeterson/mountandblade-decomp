"""ctypes front end of the native tracing library (tools/difftest/native/eqhooks.c, built by native/build.py).

Layout of the native acceleration of tools/difftest/equiv.py:
  native/eqhooks.c   the C code: native unicorn callbacks for Runner.uninit_copies (taint pass) and FpTol.ref_run
                     (write/read tracking), registered with uc_hook_add on the same uc_engine
  native/build.py    compiles it into work/difftest/native/ (gitignored) on demand
  eqnative.py        this file: loads the library, wraps the two runs, selects the path
Python keeps every decision (capstone decoding, verdicts); equiv.py keeps the pure-Python hooks as the reference and
the fallback. EQUIV_NATIVE=0 forces the Python hooks, EQUIV_NATIVE=1 makes a missing library an error, unset = use
the library when it loads (building it first if a compiler exists).
"""
import ctypes, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
MAXCLR = 64
NREG = 1024
K_SKIP, K_ERR, K_NOP, K_MOV, K_MOVS, K_PUSH, K_POP, K_PENDMEM, K_LEA, K_FLD, K_FADD, K_FST, K_GEN = range(13)
S_NONE, S_MEM, S_REG = range(3)


class Desc(ctypes.Structure):
    _fields_ = [("kind", ctypes.c_int32), ("src", ctypes.c_int32), ("dst_reg", ctypes.c_int32), ("src_reg", ctypes.c_int32),
                ("ea_base", ctypes.c_int32), ("ea_index", ctypes.c_int32), ("ea_scale", ctypes.c_int32),
                ("ea_disp", ctypes.c_int64), ("esize", ctypes.c_int32), ("fpop", ctypes.c_int32),
                ("fclear", ctypes.c_int32), ("nclr", ctypes.c_int32), ("clr", ctypes.c_int32 * MAXCLR)]


_DECODE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_uint64, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int, ctypes.POINTER(Desc))
_CLASSIFY = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_uint64, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int)

_state = {}


def _load():
    """-> (CDLL, unicorn function pointers) or None."""
    if "lib" in _state:
        return _state["lib"]
    _state["lib"] = None
    mode = os.environ.get("EQUIV_NATIVE", "")
    if mode == "0":
        return None
    try:
        sys.path.insert(0, os.path.join(HERE, "native"))
        try:
            import build
        finally:
            sys.path.pop(0)
        path = build.build()
        if path is None:
            raise OSError("no C compiler / build failed")
        lib = ctypes.CDLL(path)
        from unicorn.unicorn_py3 import unicorn as _u
        uclib = _u.uclib
        ptr = lambda n: ctypes.cast(getattr(uclib, n), ctypes.c_void_p).value
        fns = {n: ptr("uc_" + n) for n in ("reg_read", "mem_read", "hook_add", "hook_del")}
        vp, u64, c_int = ctypes.c_void_p, ctypes.c_uint64, ctypes.c_int
        lib.eq_icache_new.restype = vp
        lib.eq_icache_free.argtypes = [vp]
        lib.eq_desc_size.restype = c_int
        if lib.eq_desc_size() != ctypes.sizeof(Desc):
            raise OSError("Desc layout mismatch")
        lib.eq_taint_new.restype = vp
        lib.eq_taint_new.argtypes = [vp] * 7 + [u64] * 5
        for n, res, args in (("start", c_int, [vp]), ("stop", None, [vp]), ("args", ctypes.c_uint32, [vp, u64]),
                             ("steps", u64, [vp]), ("nstale", u64, [vp]), ("stale", None, [vp, vp]),
                             ("count", u64, [vp]), ("dump", None, [vp, vp, vp, vp, vp]), ("free", None, [vp])):
            f = getattr(lib, "eq_taint_" + n)
            f.restype, f.argtypes = res, args
        lib.eq_pcache_new.restype = vp
        lib.eq_pcache_free.argtypes = [vp]
        lib.eq_ref_start.restype = c_int
        lib.eq_ref_start.argtypes = [vp] * 7
        for n, res, args in (("stop", None, [vp]), ("flags", c_int, [vp]), ("nstore", u64, [vp]), ("dump", None, [vp, vp, vp])):
            f = getattr(lib, "eq_ref_" + n)
            f.restype, f.argtypes = res, args
        _state["lib"] = (lib, fns)
    except Exception as e:                      # noqa: BLE001  (no compiler, no library, an unexpected unicorn build)
        if mode == "1":
            raise
        _state["why"] = repr(e)
    return _state["lib"]


def available():
    return _load() is not None


class _Handle:
    def __init__(self, lib, p, free):
        self.lib, self.p, self._free = lib, p, free

    def __del__(self):
        try:
            if self.p:
                self._free(self.p)
                self.p = None
        except Exception:                       # noqa: BLE001  (interpreter shutdown)
            pass


def icache():
    """A decoded-instruction cache for TaintRun, kept by the caller across runs."""
    lib, _ = _load()
    return _Handle(lib, lib.eq_icache_new(), lib.eq_icache_free)


class TaintRun:
    """The native hooks of Runner.uninit_copies. describe(address, raw) -> (mn, ops, wr) as the Python hook decodes it,
    or None for an undecodable instruction (the Python hook skips those); fill(dec, Desc) sets the descriptor."""

    def __init__(self, uc, ic, describe, fill, lo_s, entry, span_lo, span_hi, max_steps):
        self.lib, fns = _load()
        self.uc, self.errors = uc, []
        self._describe, self._fill = describe, fill

        def decode(addr, rawp, n, dp):
            try:
                dec = self._describe(addr, ctypes.string_at(rawp, n))
                if dec is None:
                    return 0
                self._fill(dec, dp[0])
                return 1
            except BaseException as e:          # noqa: BLE001  (a bug in the descriptor builder must not pass silently)
                self.errors.append(e)
                return 0
        self._cb = _DECODE(decode)
        self._ic = ic
        self.p = self.lib.eq_taint_new(uc._uch.value, fns["reg_read"], fns["mem_read"], fns["hook_add"], fns["hook_del"],
                                       ic.p, ctypes.cast(self._cb, ctypes.c_void_p).value,
                                       lo_s, entry, span_lo, span_hi, max_steps)
        if not self.p:
            raise MemoryError("eq_taint_new")
        self.started = False

    def start(self):
        e = self.lib.eq_taint_start(self.p)
        if e:
            self.lib.eq_taint_stop(self.p)
            raise OSError("uc_hook_add failed: %d" % e)
        self.started = True

    def stop(self):
        if self.started:
            self.lib.eq_taint_stop(self.p)
            self.started = False

    def args(self, esp):
        m = self.lib.eq_taint_args(self.p, esp)
        return frozenset(k for k in range(8) if m >> k & 1)

    def steps(self):
        return self.lib.eq_taint_steps(self.p)

    def stale(self):
        n = self.lib.eq_taint_nstale(self.p)
        buf = (ctypes.c_uint64 * max(n, 1))()
        self.lib.eq_taint_stale(self.p, ctypes.addressof(buf))
        return list(buf[:n])

    def origins(self):
        """{byte address: (a, writer pc or None, read pc)} of the tainted bytes outside the entry frame, in the Python
        dict's insertion order."""
        n = self.lib.eq_taint_count(self.p)
        a = (ctypes.c_uint64 * max(n, 1))()
        oa = (ctypes.c_uint32 * max(n, 1))()
        ow = (ctypes.c_int64 * max(n, 1))()
        orr = (ctypes.c_uint32 * max(n, 1))()
        self.lib.eq_taint_dump(self.p, *(ctypes.addressof(x) for x in (a, oa, ow, orr)))
        return {x: (y, w if w >= 0 else None, r) for x, y, w, r in zip(a[:n], oa[:n], ow[:n], orr[:n])}

    def close(self):
        self.stop()
        if self.p:
            self.lib.eq_taint_free(self.p)
            self.p = None


class RefTrack:
    """The native hooks of FpTol.ref_run. classify(pc, raw) -> bool (is the instruction at pc a float store)."""

    def __init__(self, classify):
        self.lib, self.fns = _load()
        self.errors = []

        def cls(pc, rawp, n):
            try:
                return 1 if classify(pc, ctypes.string_at(rawp, n)) else 0
            except BaseException as e:          # noqa: BLE001
                self.errors.append(e)
                return 0
        self._cb = _CLASSIFY(cls)
        self._h = _Handle(self.lib, self.lib.eq_pcache_new(), self.lib.eq_pcache_free)
        self.started = False

    def start(self, uc):
        f = self.fns
        e = self.lib.eq_ref_start(self._h.p, uc._uch.value, f["reg_read"], f["mem_read"], f["hook_add"], f["hook_del"],
                                  ctypes.cast(self._cb, ctypes.c_void_p).value)
        if e:
            self.lib.eq_ref_stop(self._h.p)
            raise OSError("uc_hook_add failed: %d" % e)
        self.started = True

    def stop(self):
        if self.started:
            self.lib.eq_ref_stop(self._h.p)
            self.started = False

    def flags(self):
        f = self.lib.eq_ref_flags(self._h.p)
        return bool(f & 1), bool(f & 2)

    def fstore(self):
        n = self.lib.eq_ref_nstore(self._h.p)
        a = (ctypes.c_uint64 * max(n, 1))()
        v = (ctypes.c_uint8 * max(n, 1))()
        self.lib.eq_ref_dump(self._h.p, ctypes.addressof(a), ctypes.addressof(v))
        return {x: bool(y) for x, y in zip(a[:n], v[:n])}
