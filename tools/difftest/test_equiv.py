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

