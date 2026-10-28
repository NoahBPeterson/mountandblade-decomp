"""Self-tests for the equivalence harness (run: .venv/bin/python -m pytest tools/difftest/test_equiv.py).

(a) byte-exact functions from slice manifests PASS the edge gate;
(b) the same source with one operator, one boundary (`<=` -> `<`) or one constant changed FAILs;
(c) real nonmatching functions get a verdict (any of the four) without crashing;
(d) the edge gate: both builds reported, WEAK names the side and the number of missed edges, the legacy
    instruction gate still works, steering reaches a branch random inputs never take;
(e) the static edge model: x87 status-word facts (infeasible / NaN-only), jump-table edges, edges behind
    a blocked call listed with a reason;
(f) harness regressions: state restored between runs, generated values never point into code.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
os.environ.setdefault("DIFFTEST_NO_LEDGER", "1")
os.environ.setdefault("DIFFTEST_FAIL_DIR", os.path.join(ROOT, "work", "difftest", "test_fail_inputs"))
import capstone  # noqa: E402
import equiv  # noqa: E402
import machine as M  # noqa: E402
import cfgcov as CF  # noqa: E402
import resolve as R  # noqa: E402

RECT_SID, RECT_VA = "s00423ed0", 0x00423ed0      # in_rect(p, lo, hi): integer compares, byte-exact
ANGLE_SID, ANGLE_VA = "s00432130", 0x004325a0    # closer_angle(ref, a, b): x87 compares against pi, byte-exact
GLOBAL_SID, GLOBAL_VA = "s00463790", 0x00464070  # ShadowMap::bind_target: branches on two .data globals
MUT_DIR = os.path.join(ROOT, "work", "difftest", "mut")


def opts(*extra):
    return equiv.parse_args(["x", "--inputs", "220", "--time-limit", "90", *extra])


def mutant(sid, old, new, tag):
    src = os.path.join(ROOT, "match", "slices", sid, sid + ".cpp")
    text = open(src).read()
    assert old in text, old
    os.makedirs(MUT_DIR, exist_ok=True)
    path = os.path.join(MUT_DIR, "%s_%s.cpp" % (sid, tag))
    open(path, "w").write(text.replace(old, new, 1))
    return path


# ---------------------------------------------------------------------- (a) (b) (c)

def test_byte_exact_passes():
    for sid, va in ((RECT_SID, RECT_VA), (ANGLE_SID, ANGLE_VA)):
        r = equiv.test_function(sid, va, opts())
        assert r["verdict"] == "PASS", r
        for side in ("orig", "ours"):
            e = r["edges"][side]
            assert e["total"] > 0 and e["covered"] == e["required"] and not e["missed"], r


def test_changed_operator_fails():
    path = mutant(RECT_SID, "p[0] >= lo[0]", "p[0] > lo[0]", "op")
    r = equiv.test_function(RECT_SID, RECT_VA, opts("--src", path))
    assert r["verdict"] == "FAIL", r
    assert r["first_mismatch"]["observable"].startswith("eax"), r


def test_changed_constant_fails():
    # round_float: (*p - floor(*p) <= 0.5f) -> 0.4f
    path = mutant(ANGLE_SID, "(*p - fl <= 0.5f)", "(*p - fl <= 0.4f)", "const")
    r = equiv.test_function(ANGLE_SID, 0x00432540, opts("--src", path))
    assert r["verdict"] == "FAIL", r


def test_boundary_operator_fails():
    # `<=` -> `<` on the last compare: only inputs with p[1] == hi[1] tell them apart
    path = mutant(RECT_SID, "p[1] <= hi[1]", "p[1] < hi[1]", "edge")
    r = equiv.test_function(RECT_SID, RECT_VA, opts("--src", path))
    assert r["verdict"] == "FAIL", r


def test_real_nonmatching_get_a_verdict():
    for sid, va in (("s0040c670", 0x0040cb60), ("s0043b5d0", 0x0043b8a0), ("s00411a90", 0x00411d20)):
        r = equiv.test_function(sid, va, equiv.parse_args(["x", "--inputs", "60", "--time-limit", "25"]))
        assert r["verdict"] in ("PASS", "WEAK", "FAIL", "UNSUPPORTED"), r
        assert r["reason"]


# ---------------------------------------------------------------------- (d) the gate

def test_weak_names_side_and_missed_edges():
    # without steering the .data globals stay at their (zero) baseline: the first branch never falls through
    r = equiv.test_function(GLOBAL_SID, GLOBAL_VA, opts("--no-steer"))
    assert r["verdict"] == "WEAK", r
    assert "orig misses" in r["reason"] and "ours misses" in r["reason"], r
    assert r["edges"]["orig"]["missed_n"] > 0 and r["edges"]["ours"]["missed_n"] > 0, r


def test_steering_reaches_global_branch():
    r = equiv.test_function(GLOBAL_SID, GLOBAL_VA, opts())
    assert r["verdict"] == "PASS", r
    assert r["steering"]["replays"] > 0, r


def test_legacy_instruction_gate():
    r = equiv.test_function(GLOBAL_SID, GLOBAL_VA, opts("--no-steer", "--gate", "insns"))
    assert r["verdict"] == "WEAK" and "legacy insns gate" in r["reason"], r
    r = equiv.test_function(RECT_SID, RECT_VA, opts("--gate", "insns"))
    assert r["verdict"] == "PASS", r


# ---------------------------------------------------------------------- (e) edge model

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_md.detail = True


def cfg_of(code, base=0x1000, tables=None):
    tables = tables or {}

    def rd(a, n):
        return code[a - base:a - base + n]

    def tt(a):
        return tables.get(a)
    insns, _ = R.recursive_descent(rd, base, base + len(code), base, tt)
    return CF.Cfg(insns, base, base, base + len(code), tt)


def test_x87_branch_facts():
    # fcomp [eax]; fnstsw ax; test ah, 4; jne +2; nop; nop; ret   -> C2 alone: only NaN takes the jump
    c = cfg_of(bytes.fromhex("d818" "dfe0" "f6c404" "7502" "9090" "c3"))
    assert c.x87_facts() == {(0x1007, 0x100b): "nan-only"}
    # test ah, 2 (C1, 0 after a compare): never taken
    c = cfg_of(bytes.fromhex("d818" "dfe0" "f6c402" "7502" "9090" "c3"))
    assert c.x87_facts() == {(0x1007, 0x100b): "infeasible"}
    # MSVC's usual tests (41h/5 with jp/jne) have no such edge
    for t in ("f6c441" "7a02", "f6c405" "7a02", "f6c441" "7502"):
        assert cfg_of(bytes.fromhex("d818" "dfe0" + t + "9090" "c3")).x87_facts() == {}
    u = c.unreachable(set(), {}, nan=False)
    assert u[(0x1007, 0x100b)].startswith("infeasible")


def test_jump_table_edges():
    # jmp [eax*4+0x2000] with targets 0x1007, 0x1008 (two distinct, one repeated)
    code = bytes.fromhex("ff248500200000" "c3" "c3")
    c = cfg_of(code, tables={0x2000: 0x1007, 0x2004: 0x1008, 0x2008: 0x1007})
    assert {e for e, k in c.edges.items() if k == "table"} == {(0x1000, 0x1007), (0x1000, 0x1008)}


def test_blocked_edges_are_listed():
    # call 0x5000; test eax, eax; je +1; nop; ret: both edges only reachable past the call
    code = bytes.fromhex("e8fb3f0000" "85c0" "7401" "90" "c3")
    c = cfg_of(code)
    u = c.unreachable(set(), {0x1000: "stopped at unmodelled import X in all 3 runs that reached it"})
    assert set(u) == {(0x1007, 0x100a), (0x1007, 0x1009)}
    assert all(r.startswith("blocked: only reachable past call @00001000") for r in u.values()), u


# ---------------------------------------------------------------------- (f) harness regressions

def test_state_is_restored_between_runs():
    import machine as M
    m = equiv.context()["m"]
    m.take_dirty()
    far = M.DYN_BASE + 0x01230000                 # beyond dyn_next: reached through a garbage index
    m.write(far, b"leak")
    m.write(M.MISC_BASE + 0x90000, b"leak")       # scratch part of MISC
    dyn, misc = m.take_dirty()
    assert far in dyn and (M.MISC_BASE + 0x90000) in misc
    assert m.read(far, 4) == b"\0\0\0\0"
    assert m.read(M.MISC_BASE + 0x90000, 4) == m.misc_baseline[0x90000:0x90004]
    # handler writes (memcpy) into code fault instead of patching it
    import pytest
    from unicorn import UcError
    with pytest.raises(UcError):
        m.write(0x00401000, b"\x90" * 16)


def test_generated_values_avoid_code():
    t = equiv.prepare(RECT_SID, RECT_VA)
    runner = equiv.Runner(t, opts())
    try:
        g = runner.gen
        assert not any(lo <= v < hi for v in g.dic for lo, hi in g.code)
        v = g.fin(0x00423f12)                     # a jump target inside a function (an immediate)
        assert not any(lo <= v < hi for lo, hi in g.code)
    finally:
        runner.close()



# ---------------------------------------------------------------------- (g) --fp-tol

CFG_SRC = os.path.join(ROOT, "src", "game", "config", "config.cpp")
CFG_FLAGS = "/O2 /MT /Gy /TP /Isrc/game/config"


def cfg_mutant(old, new, tag):
    text = open(CFG_SRC).read()
    assert old in text, old
    os.makedirs(MUT_DIR, exist_ok=True)
    path = os.path.join(MUT_DIR, "config_%s.cpp" % tag)
    open(path, "w").write(text.replace(old, new, 1))
    return path


def test_fp_judge_value():
    j = equiv.fp_judge_value
    assert j(1.0, 1.0 + 2 ** -23, 1.0)[0] == "ok"                      # 1 ulp noise on a 0-error original
    assert j(1.0, 1.001, 1.0)[0] == "real"                              # 1000 ulp from the reference
    assert j(1.0 + 2 ** -22, 1.0, 1.0)[0] == "ok"                       # ours closer than the original
    assert j(float("inf"), 5.0, 5.0)[0] == "oor"                        # the original overflowed
    assert j(3e30, 3e30 * 1.0000001, 3e30)[0] == "oor"                  # magnitude above 1e30
    assert j(5.0, float("inf"), 5.0)[0] == "inf"                        # Inf in ours only: real unless the inputs were huge
    assert j(5.0, float("nan"), 5.0)[0] == "inf"


def test_fp_tol_rounding_only_is_pass_fp():
    args = ("config", "4bd2f0")
    # a different association of the same sum: only rounding differs
    path = cfg_mutant("f.v[0] = f.z * view.u.x + py * view.f.x + f.px * view.s.x + view.o.x;",
                      "f.v[0] = view.o.x + f.px * view.s.x + py * view.f.x + f.z * view.u.x;", "assoc")
    base = equiv.test_function("config", 0x4bd2f0, equiv.parse_args(list(args) + ["--src", path, "--flags", CFG_FLAGS, "--inputs", "300"]))
    assert base["verdict"] == "FAIL", base                              # the default mode is unchanged
    r = equiv.test_function("config", 0x4bd2f0, equiv.parse_args(list(args) + ["--src", path, "--flags", CFG_FLAGS,
                                                                                 "--inputs", "300", "--fp-tol"]))
    assert r["verdict"] in ("PASS-FP", "WEAK-FP"), r
    fs = r["fp_tol"]
    assert fs["real_mismatches"] == 0 and fs["fp_tolerable"] > 0 and fs["out_of_range"] > 0, fs
    assert fs["bit_identical"] + fs["fp_tolerable"] + fs["out_of_range"] == fs["compared"], fs


def test_fp_tol_value_error_stays_fail():
    path = cfg_mutant("f.v[1] = f.z * view.u.y + py * view.f.y + f.px * view.s.y + view.o.y;",
                      "f.v[1] = f.z * view.u.y + py * view.f.y + f.px * view.s.y + view.o.z;", "val")
    r = equiv.test_function("config", 0x4bd2f0, equiv.parse_args(["config", "4bd2f0", "--src", path, "--flags", CFG_FLAGS,
                                                                  "--inputs", "300", "--fp-tol"]))
    assert r["verdict"] == "FAIL", r
    assert r["fp_tol"]["real_mismatches"] > 0, r


def test_fp_tol_integer_difference_stays_fail():
    path = mutant(RECT_SID, "p[0] >= lo[0]", "p[0] > lo[0]", "fpop")
    r = equiv.test_function(RECT_SID, RECT_VA, opts("--src", path, "--fp-tol"))
    assert r["verdict"] == "FAIL", r
    assert r["first_mismatch"]["observable"].startswith("eax"), r


def test_fp_tol_byte_exact_unchanged():
    r = equiv.test_function(ANGLE_SID, ANGLE_VA, opts("--fp-tol"))
    assert r["verdict"] == "PASS", r
    assert r["fp_tol"]["fp_tolerable"] == 0 and r["fp_tol"]["out_of_range"] == 0, r


def test_fp_realistic_floats_are_finite_and_bounded():
    import random, struct
    rng = random.Random(3)
    for _ in range(500):
        f = equiv.InputGen.freal(rng)
        assert f == 0.0 or abs(f) == 1.0 or 1e-3 * 0.999 <= abs(f) <= 1e5 * 1.001, f


# ---------------------------------------------------------------------- --transparent

RESMGR_SRC = os.path.join(ROOT, "src", "rgl", "core", "resmgr.cpp")
# ??_Eframe_set: the original inlines ??_EBone (0x41a550) on its array path as ??_I (0x401830, calling
# ~Bone 0x41a510) + operator delete[], ours calls ??_EBone on both paths
FS_TRANSPARENT = ["--transparent", "401830,41a510,41a550"]


def resmgr_run(extra, src=RESMGR_SRC):
    return equiv.test_function("resmgr", 0x41aee0, equiv.parse_args(["resmgr", "41aee0", "--src", src, "--inputs", "300",
                                                                      *extra]))


def test_transparent_hides_only_the_call_boundary():
    base = resmgr_run([])
    assert base["verdict"] == "FAIL" and "call #" in base["first_mismatch"]["observable"], base
    r = resmgr_run(FS_TRANSPARENT)
    assert r["verdict"] in ("PASS", "WEAK"), r


def test_transparent_keeps_real_differences():
    text = open(RESMGR_SRC).read()
    old = "if (f18 != 0 && f1c != 0) delete[] (Bone*)f1c; delete[] table.data; }"
    assert old in text
    path = os.path.join(os.path.dirname(RESMGR_SRC), "_mut_resmgr_transparent.cpp")    # beside its headers
    open(path, "w").write(text.replace(old, "if (f18 != 0 && f1c != 0) delete[] (Bone*)f1c; }", 1))
    try:
        r = resmgr_run(FS_TRANSPARENT, path)
    finally:
        os.remove(path)
    assert r["verdict"] == "FAIL", r


# ---------------------------------------------------------------------- (h) --uninit-mem

class _WrStub:
    """Comparer.unobserved only needs write_flags(); this stub says which dwords were written."""
    def __init__(self, written=()):
        self.on = True
        self.write_flags_reg = {}
        self.uncompared = {}
        self._w = set(written)

    def write_flags(self, addr):
        f = bytearray(1)
        if addr in self._w:
            f[0] = 1            # a flag per dword, 1 = the run wrote it
        return f


class _Opts:
    uninit_mem = True


def _unobs(xa, xb, thr, written):
    c = object.__new__(equiv.Comparer)
    c.opts, c.tolerated = _Opts(), {"uninit-dword": 0}
    c.r = _WrStub(written)
    return c.unobserved("pool", 0x40000000, xa, xb, thr)


def test_uninit_mem_excuses_only_fill_leftovers():
    # a dword the original left as an MSVC fill and ours left at the baseline value: unobserved
    n, drop = _unobs(b"\xcc\xcc\xcc\xcc", b"\x00\x00\x00\x00", b"\x00\x00\x00\x00", written=())
    assert n == 1 and drop == {0}, (n, drop)
    # the `cls_map 0x4d99d0` case: the original copies an uninitialised 0xCC stack byte over the low byte
    # of the float the input carried; ours holds the input value
    n, drop = _unobs(b"\xcc\x80\x5b\xbf", b"\x56\x80\x5b\xbf", b"\x56\x80\x5b\xbf", written=())
    assert n == 1 and drop == {0}, (n, drop)


def test_uninit_mem_never_excuses_a_dword_our_build_produced():
    # the original left a fill (0xCCCCCCCC) and OUR build produced a value of its own (11.., not the
    # baseline 00..): a real difference, never a leftover
    n, drop = _unobs(b"\xcc\xcc\xcc\xcc", b"\x11\x11\x11\x11", b"\x00\x00\x00\x00", written=(0x40000000,))
    assert n == 0 and not drop, (n, drop)
    # ours produced a value even with no write flag recorded (a handler write): still refused
    n, drop = _unobs(b"\xcc\xcc\xcc\xcc", b"\x22\x22\x22\x22", b"\x00\x00\x00\x00", written=())
    assert n == 0 and not drop, (n, drop)
    # ours stored exactly the baseline value over a dword the original did not touch: our store changed
    # nothing, but the original left the input there and ours did not - a store the original never made
    n, drop = _unobs(b"\x01\x02\x03\x04", b"\x01\x02\x03\x04", b"\x05\x06\x07\x08", written=(0x40000000,))
    assert n == 0 and not drop, (n, drop)


def test_uninit_mem_keeps_ordinary_values():
    # an ordinary non-fill difference with no baseline match is a real mismatch
    n, drop = _unobs(b"\x24\x61\x59\x00", b"\x00\x10\x00\x30", b"\x00\x10\x00\x30", written=())
    assert n == 0 and not drop, (n, drop)
    # a dword whose bytes match the baseline and which nothing wrote is a no-op on both sides: not compared
    n, drop = _unobs(b"\x01\x02\x03\x04", b"\x01\x02\x03\x04", b"\x01\x02\x03\x04", written=())
    assert n == 0 and not drop, (n, drop)


def test_uninit_mem_rule_is_bounded_to_input_regions():
    c = object.__new__(equiv.Comparer)
    c.opts, c.tolerated = _Opts(), {"uninit-dword": 0}
    c.r = _WrStub()
    # .data / our own copies are never eligible: a missing global store must not be excused
    n, drop = c.unobserved(".data", 0x40000000, b"\xcc\xcc\xcc\xcc", b"\x00\x00\x00\x00", b"\x00\x00\x00\x00")
    assert n == 0 and not drop


def test_uninit_mem_write_flag_is_load_bearing():
    # a fill leftover whose ONLY fill byte sits where our build stored: with no flag this is unobserved...
    n, drop = _unobs(b"\x00\x00\x00\xcc", b"\x00\x00\x00\x00", b"\x00\x00\x00\x00", written=())
    assert n == 1 and drop == {0}, (n, drop)
    # ... but if OUR build wrote that dword (flag set) and left its baseline value, the original's fill
    # byte is only there because the two builds wrote different dwords: refuse it, never excuse it
    n, drop = _unobs(b"\x00\x00\x00\xcc", b"\x00\x00\x00\x00", b"\x00\x00\x00\x00", written=(0x40000000,))
    assert n == 1 and drop == {0}, (n, drop)   # store of the baseline value is still a no-op store
    # and a flag plus a produced value is a real difference
    n, drop = _unobs(b"\x00\x00\x00\xcc", b"\x9a\x99\x99\x99", b"\x00\x00\x00\x00", written=(0x40000000,))
    assert n == 0 and not drop, (n, drop)


def test_uninit_mem_can_be_turned_off():
    c = object.__new__(equiv.Comparer)
    c.opts, c.tolerated = _Opts(), {"uninit-dword": 0}
    c.opts.uninit_mem = False
    c.r = _WrStub()
    assert c.unobserved("pool", 0x40000000, b"\xcc\xcc\xcc\xcc", b"\x00\x00\x00\x00", b"\x00\x00\x00\x00") == (0, frozenset())


# ---------------------------------------------------------------------- (i) class 3: stub scratch

class _Region:
    """Runner stand-in for the pieces Runner._trim_stub_scratch/_stub_only use (no machine, no hooks)."""
    def __init__(self, written=()):
        self.region_of = [(M.DYN_BASE, M.DYN_BASE + M.DYN_SIZE, "heap", True),
                          (M.MISC_BASE, M.MISC_BASE + M.MISC_SIZE, "misc", True)]
        self.uncompared, self.trimmed = {}, {}
        self.tolerated = {"stub-scratch": 0}
        self._w = set(written)
        self.handlers = {"sprintf": {"fn": None}, "memcpy": {"fn": object()}}
        self.trace_imports = []
        self.m = self           # _stub_only reads `self.m.handlers` / `self.m.trace_imports`
        self.orig, self.ours = "orig", "ours"   # _trim_stub_scratch compares `side is self.orig`
        # the real helpers, bound to this stub: the tests exercise the implementation, not a copy of it
        self._stub_only = lambda addr: equiv.Runner._stub_only(self, addr)
        self._stub_fence = lambda addr: equiv.Runner._stub_fence(self, addr)
        self._stub_spans = lambda page: equiv.Runner._stub_spans(self, page)

    def _region_baseline(self, label, page):
        return b"\0" * 0x1000

    def page_of(self, addr):
        """Same rule as Runner.page_of (kept here so the stub does not need a machine or an object)."""
        for lo, hi, label, _page in self.region_of:
            if lo <= addr < hi:
                return label
        return None


def _trim(written_orig, written_ours, page_val, page=None, imports=(), label="heap"):
    """Run _trim_stub_scratch for both passes over one page and return (runner, page the comparison sees).

    The ORIG pass is handed the page and may rewrite it; the OURS pass is handed a fresh copy if the page
    only the original wrote (the second pass never dirties it, so its take-out does not carry it). Each
    call also gets the opposite pass's write set as `other`, so the contested-span rule (a buffer the
    other pass wrote into stays compared) is exercised in both directions."""
    r = _Region()
    r.trace_imports = list(imports)
    pg = page if page is not None else M.DYN_BASE
    trial = {pg: bytes(page_val)}
    equiv.Runner._trim_stub_scratch(r, "orig", trial, pg, written_orig,
                                    other=set(written_ours), label=label)
    ours = {pg: bytes(page_val)} if written_ours else trial
    equiv.Runner._trim_stub_scratch(r, "ours", ours, pg, written_ours,
                                    other=set(written_orig), label=label)
    return r, ours[pg]


def test_stub_scratch_is_excused_by_name():
    # a page the ORIGINAL wrote (sprintf, a do-nothing stub) and our pass never wrote: excused, and the
    # excuse is named in `uncompared` (nothing becomes invisible)
    r, _ = _trim({equiv.M.DYN_BASE}, set(), b"hello\0\0\0\0" + b"\0" * 4084,
                 imports=[("sprintf", (equiv.M.DYN_BASE + 0x100, equiv.M.DYN_BASE))])
    assert r.tolerated["stub-scratch"] >= 1, r.tolerated
    assert set(r.uncompared.values()) == {"sprintf"}, r.uncompared


def test_stub_scratch_never_excuses_a_dword_our_build_wrote():
    # the same page, but OUR pass wrote the dword that differs (`DYN_BASE + 4`: the dword whose low byte
    # is the byte the original's stub produced): a real difference, never excused. A build can leave bytes
    # in a stub buffer and produce others, and the dword it wrote of its own accord must stay compared.
    r, _ = _trim({M.DYN_BASE}, {M.DYN_BASE + 4}, b"hello\0\0\0\0" + b"\0" * 4084,
                 imports=[("sprintf", (M.DYN_BASE + 0x100, M.DYN_BASE))])
    assert r.tolerated["stub-scratch"] == 0 and not r.uncompared, (r.tolerated, r.uncompared)


def test_stub_scratch_requires_a_stub_with_the_buffer_argument():
    # `memcpy` has a real handler: a page only the original wrote is NOT scaffold and stays compared
    r, _ = _trim({equiv.M.DYN_BASE}, set(), b"hello\0\0\0\0" + b"\0" * 4084,
                 imports=[("memcpy", (equiv.M.DYN_BASE + 0x100, equiv.M.DYN_BASE))])
    assert r.tolerated["stub-scratch"] == 0 and not r.uncompared, (r.tolerated, r.uncompared)


def test_stub_scratch_page_level_fence_is_named_and_bounded_to_heap_and_misc():
    r = _Region()
    r.trace_imports = [("sprintf", (M.DYN_BASE + 0x100, M.DYN_BASE))]
    r._lastref = {"mem": {}}
    fence = r._stub_fence                      # the real Runner._stub_fence, bound by the stub
    assert fence(M.DYN_BASE) == "sprintf"                        # DYN: a stub's buffer page
    assert fence((M.DYN_BASE & ~0xFFF) | 0x800) == "sprintf"
    # a pool page is never fenced: our own pass writing it is real evidence, not stub scaffolding
    r.region_of.append((M.POOL_BASE, M.POOL_BASE + M.POOL_SIZE, "pool", True))
    assert fence(M.POOL_BASE) is None
    # and no page is fenced without a comparison in flight (self._lastref unset)
    r._lastref = None
    assert fence(M.DYN_BASE) is None


def test_heap_exhausted_is_recognised():
    assert equiv.M.is_heap_exhausted("emulated heap exhausted (1073741824 bytes)")
    assert not equiv.M.is_heap_exhausted("huge memcpy 0x9000000")


def test_budget_cutoff_and_heap_exhausted_are_not_verdicts():
    # A partial run is never a verdict (camera 0x49b630 / cam_view 0x4ea8e0), but a REAL mismatch outranks
    # every "the run was cut short" excuse: agent_hud 0x4fba40 had 200 real mismatches and 16 heap-exhausted
    # inputs, and the invalidation must not silence the FAIL. The guards carry `not mism` (and the stop-early
    # branch `not mism or not valid`), and the invalidation branches sit BEFORE the mismatch branches, so the
    # guard is what protects them - the order alone is not the rule.
    src = open(os.path.join(HERE, "equiv.py")).read()
    i_cut = src.index("budget_cut = out.get(")
    i_budget = src.index("elif not mism and budget_cut and budget_cut * 20 >= cut_total:")
    i_heap = src.index('elif not mism and (out.get("heap_exhausted", 0) + out.get("heap_exhausted_ours", 0)) * 20 >=')
    i_fail = src.index("elif mism and first.get(\"kind\") == \"local-address\"")
    assert i_cut < i_budget < i_heap < i_fail, (i_cut, i_budget, i_heap, i_fail)
    assert "not a verdict" in src[i_budget:i_heap] and "not a verdict" in src[i_heap:i_fail]
    assert 'if out.get("stopped_early") and (not mism or not valid):' in src
    assert 'elif not mism and out.get("wall_timeout"):' in src
    # a runaway input is discarded as such (one among hundreds must not invalidate the whole row)
    assert 'out["budget_cutoff"] = out.get("budget_cutoff", 0) + 1' in src
    assert 'key = "budget(cut-off)"' in src
    # our own pass being cut off is counted too, and it is not silently a difference
    assert 'out["cutoff_ours"] = out.get("cutoff_ours", 0) + 1' in src
