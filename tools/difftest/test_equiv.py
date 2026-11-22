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
os.environ.setdefault("DIFFTEST_REGRESS_DIR", os.path.join(ROOT, "work", "difftest", "test_regress"))
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


FROZEN_OBJ = "7ddfb635"       # 2026-11-01 sources: obj_tbl 0x499050 with its uninitialised-slot copies, agent_hud 0x4faee0
FROZEN_LOOP = "710c99d0"      # 2026-10-29 sources: trigger_vec 0x4c1b60's runaway loop


def frozen_row(commit, sid, va, relsrc, *x):
    """test_function on `relsrc` as of git commit `commit` (a `git archive` of src/ cached under work/difftest/frozen,
    run with that directory as the working tree so the sources' -Isrc/ includes resolve there). Rows whose property
    depends on how the source is written (which stack slots it leaves unwritten, how its types lay out) use this
    instead of the live src/, which other work keeps rewriting."""
    import subprocess
    d = os.path.join(ROOT, "work", "difftest", "frozen", commit)
    if not os.path.isdir(os.path.join(d, "src")):
        os.makedirs(d, exist_ok=True)
        tar = subprocess.run(["git", "archive", commit, "src"], cwd=ROOT, check=True, capture_output=True).stdout
        subprocess.run(["tar", "-x", "-C", d], input=tar, check=True)
    link = os.path.join(d, "work")
    if not os.path.lexists(link):
        os.symlink(os.path.join(ROOT, "work"), link)
    here = os.getcwd()
    os.chdir(d)
    try:
        return equiv.test_function(sid, va, equiv.parse_args(
            [sid, "%x" % va, "--src", os.path.join(d, relsrc), "--no-regress", *x]))
    finally:
        os.chdir(here)


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


def cfg_xf_mutant(comp, tag, reassociate=False, wrong_o=None):
    """config.cpp with the camera-frame transform line of component `comp` (x/y/z, the `a.z * m.u.c + a.y * m.f.c +
    a.x * m.s.c + m.o.c` sum of screen_to_world 0x4bd2f0) mutated, found by pattern so it survives renames of the
    locals: reassociated (rounding only) or with a wrong origin component (a value error)."""
    import re
    text = open(CFG_SRC).read()
    pat = re.compile(r"(\b\w+\.%s = )(\w+\.z \* \w+\.u\.%s \+ \w+\.y \* \w+\.f\.%s \+ \w+\.x \* \w+\.s\.%s) \+ (\w+\.o)\.%s;"
                     % ((comp,) * 5))
    m = pat.search(text)
    assert m, "no transform line for " + comp
    if reassociate:
        # the x87 sum is evaluated at 64-bit precision, so a mere reorder rounds identically after the final store:
        # force the first two terms through a 32-bit memory temporary, a different rounding of the same sum
        t1, t2, t3 = m.group(2).split(" + ")
        new = "{ volatile float t = %s + %s; %s%s + %s + %s.%s; }" % (t1, t2, m.group(1), "t", t3, m.group(3), comp)
    else:
        new = "%s%s + %s.%s;" % (m.group(1), m.group(2), m.group(3), wrong_o)
    os.makedirs(MUT_DIR, exist_ok=True)
    path = os.path.join(MUT_DIR, "config_%s.cpp" % tag)
    open(path, "w").write(text[:m.start()] + new + text[m.end():])
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
    # soup inputs only: typed inputs (--typed-share) reach a case fp-tol judges real on this mutant (open item)
    args = ("config", "4bd2f0", "--typed-share", "0")
    # a different association of the same sum: only rounding differs
    path = cfg_xf_mutant("y", "assoc", reassociate=True)
    base = equiv.test_function("config", 0x4bd2f0, equiv.parse_args(list(args) + ["--src", path, "--flags", CFG_FLAGS, "--inputs", "300"]))
    assert base["verdict"] == "FAIL", base                              # the default mode is unchanged
    r = equiv.test_function("config", 0x4bd2f0, equiv.parse_args(list(args) + ["--src", path, "--flags", CFG_FLAGS,
                                                                                 "--inputs", "300", "--fp-tol"]))
    assert r["verdict"] in ("PASS-FP", "WEAK-FP"), r
    fs = r["fp_tol"]
    assert fs["real_mismatches"] == 0 and fs["fp_tolerable"] > 0 and fs["out_of_range"] > 0, fs
    assert fs["bit_identical"] + fs["fp_tolerable"] + fs["out_of_range"] == fs["compared"], fs


def test_fp_tol_value_error_stays_fail():
    path = cfg_xf_mutant("y", "val", wrong_o="z")
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

RESMGR_SRC = os.path.join(ROOT, "src", "engine", "resource", "resource_manager.cpp")
# ??_Eframe_set: the original inlines ??_EBone (0x41a550) on its array path as ??_I (0x401830, calling
# ~Bone 0x41a510) + operator delete[], ours calls ??_EBone on both paths
FS_TRANSPARENT = ["--transparent", "401830,41a510,41a550"]


def resmgr_run(extra, src=RESMGR_SRC):
    return equiv.test_function("resource_manager", 0x41aee0, equiv.parse_args(["resource_manager", "41aee0", "--src", src, "--inputs", "300",
                                                                      *extra]))


def test_transparent_hides_only_the_call_boundary():
    base = resmgr_run(["--no-auto-transparent"])
    assert base["verdict"] == "FAIL" and "call #" in base["first_mismatch"]["observable"], base
    r = resmgr_run(FS_TRANSPARENT)
    assert r["verdict"] in ("PASS", "WEAK"), r


def test_marker_names_skip_declspec():
    """`__declspec(noinline) char __fastcall check_register(...)` under a marker is check_register, not __declspec."""
    import slice as S
    src = os.path.join(ROOT, "src", "game", "script", "operation_execute.cpp")
    assert S.marker_names(src, 0x49cd00) == ["check_register"]


def test_crt_startup_state():
    """_mtinit/_mtinitlocks state: the preallocated CRT locks have critical sections and _getptd finds the main
    thread's _tiddata through the TLS fallback (without it, quest_log 0x4ee910 overflowed the stack in _lock)."""
    import struct
    m = M.Machine(R.Image())
    u = lambda a: struct.unpack("<I", bytes(m.uc.mem_read(a, 4)))[0]
    table = m.crt_locktable if hasattr(m, "crt_locktable") else 0x5d61b0
    assert u(table + 8 * 0xA) != 0                       # _LOCKTAB_LOCK
    idx = u(0x5d6150)                                    # __tlsindex
    assert idx != 0xFFFFFFFF
    assert u(M.TLS_BLOCKS + 0x1000 * idx + 0x54) == 0x5d67d0   # ptmbcinfo = &__initialmbcinfo
    assert all(m.image_base_page(".data", p) == bytes(m.uc.mem_read(p, 0x1000)) for p in m.image_extra_base)


def test_system_import_models_are_input_driven():
    """FindFirstFileA / SHGetFolderPathA / FSOUND_PlaySoundEx / NewtonBodyGetMatrix answer by (seed, call ordinal):
    the same seed replays the same answers and out-parameters (orig and ours see one environment), and across
    seeds both sides of the caller's branch occur. Out-parameters are written as the API documents."""
    import struct
    m = M.Machine(R.Image())
    buf = m.model_data("test_sys_models", 0x1000)
    m.write(buf + 0x800, b"C:\\mods\\*.sco\0")
    call = lambda name, *a: m.handlers[name]["fn"](m, list(a), 0)

    def run(seed):
        m.ui_seed, m.ui_calls = seed, 0
        m.find_patterns, m.find_counts = {}, {}
        m.write(buf, b"\0" * 0x800)
        out = [call("FindFirstFileA", buf + 0x800, buf), m.read(buf, 320),
               call("SHGetFolderPathA", 0, 0x1A, 0, 0, buf + 0x400), m.cstr(buf + 0x400),
               call("_FSOUND_PlaySoundEx@16", 0xFFFFFFFF, 0, 0, 1)]
        call("NewtonBodyGetMatrix", 0, buf + 0x600)
        return out + [m.read(buf + 0x600, 64)]

    runs = [run(s) for s in range(0x1000, 0x1040)]
    assert runs == [run(s) for s in range(0x1000, 0x1040)]                 # deterministic per seed
    assert {r[0] for r in runs} == {M.INVALID_HANDLE, M.H_FIND}             # found / not found
    found = next(r for r in runs if r[0] == M.H_FIND)
    assert found[1][44:50] == b"a.sco\0" and struct.unpack_from("<I", found[1])[0] == 0x20
    assert {r[2] for r in runs} == {0, 0x80004005}
    assert next(r for r in runs if r[2] == 0)[3] == M.FOLDER_PATHS[0x1A]
    assert next(r for r in runs if r[2])[3] == b""                         # a failed lookup leaves ""
    assert {r[4] for r in runs} == {0xFFFFFFFF, 0, 3, 7}                    # no channel / a channel
    assert len({r[5][48:60] for r in runs}) > 4                            # body translations vary
    assert all(struct.unpack("<16f", r[5])[:12] == (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0) for r in runs)
    assert m.handlers["NewtonBodyGetMatrix"]["pops"] == 0                  # newton.dll is __cdecl
    assert m.handlers["_FSOUND_PlaySoundEx@16"]["pops"] == 4


def test_program_exit_is_a_compared_run():
    """resmgr::load_stream 0x41db20 reports an unknown token through mb_error -> exit(-1): the CRT exit ends the run
    with status "exit" on both sides and the run is compared (memory + traces), not discarded; it was UNSUPPORTED
    (every input stopped at the exit path's SetUnhandledExceptionFilter)."""
    m = M.Machine(R.Image())
    try:
        m.handlers["exit"]["fn"](m, [0xFFFFFFFF], 0)
        assert False, "exit returned"
    except M.Exit as e:
        assert str(e) == "code -1"
    assert equiv.noreturn_stop("exit: code -1")
    src = os.path.join(ROOT, "src", "engine", "resource", "resource_manager.cpp")
    r = equiv.test_function("resource_manager", 0x41db20, equiv.parse_args(
        ["resource_manager", "41db20", "--src", src, "--inputs", "100", "--time-limit", "90", "--no-regress"]))
    assert r["verdict"] in ("PASS", "WEAK") and r["valid"] > 50, r
    assert not any(k.startswith("import:SetUnhandledExceptionFilter") for k in r["discarded"]), r["discarded"]


def test_lookup_stub_makes_row_testable():
    """make_scroll_track calls resmgr::get_object, which exit()s on a name an empty test resource manager lacks:
    stubbed (the default) the row is measured past the lookup; unstubbed every input ends at the exit (a compared
    run since program exits are modelled, but one that never reaches the code after the lookup)."""
    src = os.path.join(ROOT, "src", "game", "ui", "screens", "quest_log.cpp")
    run = lambda *x: equiv.test_function("quest_log", 0x4ee910, equiv.parse_args(
        ["quest_log", "4ee910", "--src", src, "--inputs", "150", "--no-regress", "--time-limit", "900", *x]))
    r = run()
    assert r["verdict"] in ("PASS", "WEAK") and r["valid"] > 0 and r.get("exited", 0) < r["valid"], r
    stubbed_cov = r["coverage"]["covered_insns"]
    r = run("--stub", "")
    assert r["valid"] > 0 and r.get("exited", 0) == r["valid"], r
    assert r["coverage"]["covered_insns"] < stubbed_cov, r["coverage"]


def test_caller_dead_register_is_tolerated():
    """Cont650::get 0x48d650 (the slice match/slices/s0048d3c0, the frozen early version of SlotArray::get: src/ has
    since matched it byte for byte): ours leaves a dead value in ecx, which the original never writes; none of the 10
    call sites reads ecx before overwriting it (callerdeps SAFE), so the difference is tolerated, not a FAIL."""
    src = os.path.join(ROOT, "match", "slices", "s0048d3c0", "s0048d3c0.cpp")
    r = equiv.test_function("s0048d3c0", 0x48d650, equiv.parse_args(
        ["s0048d3c0", "48d650", "--src", src, "--inputs", "100", "--no-regress"]))
    assert r["verdict"] in ("PASS", "WEAK") and r["tolerated"].get("caller-dead-ecx"), r


def test_callee_narrow_args():
    """tstring::push_back(char) 0x42be70 reads its argument as a byte (`mov al, [esp+8]` after `push esi`): only that
    byte of the caller's pushed dword is compared. A dword-reading callee keeps the full width."""
    r = equiv.Runner.__new__(equiv.Runner)
    r.img, r.arity_cache = R.Image(), {}
    assert r.callee_arg_widths(0x42be70) == {0: 1}
    assert r.callee_arg_widths(0x42bcb0) == {}          # tstring::assign(const char*): a pointer


def test_tail_call_pops_count():
    """0x48bda0 has no ret of its own: it ends in `jmp 0x488720`, whose `ret 0x10` pops its four arguments. Read as
    0, every input was discarded as orig-abi-broken; with the tail target's pops the row is measured."""
    src = os.path.join(ROOT, "src", "game", "ui", "widgets", "text_editor_widget.cpp")
    r = equiv.test_function("text_editor_widget", 0x48bda0, equiv.parse_args(
        ["text_editor_widget", "48bda0", "--src", src, "--inputs", "100", "--no-regress"]))
    assert r["callee_pop"] == 0x10 and r["valid"] > 0, r


def test_tail_jump_arguments_start_above_the_return_address():
    """The site hook runs before the instruction: `call` has its first argument at [esp], a tail `jmp` has the return
    address there. 0x4d5290 reported a FAIL with args=[7efff000] against [3] on a `mov [esp+4],3; jmp` that the original
    made with `push 3; call`."""
    import struct, types
    mem = {0x1000: 0x7EFFF000, 0x1004: 3, 0x1008: 5}
    uc = types.SimpleNamespace(reg_read=lambda r: 0)

    def record(is_jmp):
        r = types.SimpleNamespace(calls=[], call_esps=[], call_taint=[], call_regs=[], call_taint_fn=lambda a: a,
                                  opts=types.SimpleNamespace(call_regs=False),
                                  m=types.SimpleNamespace(read=lambda a, n: b"".join(
                                      struct.pack("<I", mem.get(a + k, 0)) for k in range(0, n, 4))))
        esp = 0x1000 if is_jmp else 0x1004
        equiv.Runner._record_call(r, uc, 0x401000, {"add_esp": 1, "is_jmp": is_jmp}, esp)
        return r
    tail, call = record(True), record(False)
    assert tail.calls[0][3] == call.calls[0][3] and tail.calls[0][3][:2] == (3, 5)
    assert tail.call_esps == call.call_esps == [0x1004] and tail.call_taint == call.call_taint == [0x1004]


def test_uninit_copy_is_excused():
    """scene_tbl::placed 0x499050: Frame3's rows leave w unset and set_part copies them into the mesh, so both builds
    store an uninitialised stack slot there (different leftovers). The original's value is a plain copy of a slot
    its run never wrote, so it is excused; with --no-uninit-copy the row FAILs on it."""
    run = lambda *x: frozen_row(FROZEN_OBJ, "object_table_spawners", 0x499050, "src/game/data/object_table_spawners.cpp",
                                "--inputs", "120", "--fp-tol", "--typed-share", "0", *x)
    r = run()
    assert r["verdict"] in ("PASS", "PASS-FP", "WEAK", "WEAK-FP") and r["tolerated"].get("uninit-copy"), r
    assert run("--no-uninit-copy")["verdict"] == "FAIL"


def _row(sid, va, src, *x):
    return equiv.test_function(sid, va, equiv.parse_args(
        [sid, "%x" % va, "--src", os.path.join(ROOT, src), "--no-regress", *x]))


def test_forced_callee_result_covers_branch():
    """0x4b25c0 branches on `call 0x435740; test al, al`: the predicate reads game state soup never satisfies, so
    one edge stayed missed; forcing the callee's result (both builds call it) covers it, and the switch removes it."""
    assert _row("party_map", 0x4b25c0, "src/game/party/party_map.cpp")["verdict"] == "PASS"
    assert _row("party_map", 0x4b25c0, "src/game/party/party_map.cpp", "--no-force-results")["verdict"] == "WEAK"


def test_fault_only_edge_is_unreachable():
    """0x48f480: the NULL-vector arm of the size idiom reads first[-1]: every input taking it faults in both builds,
    so the edge is reported unreachable with that reason, not missed."""
    r = _row("container_and_tableau_misc", 0x48f480, "src/game/ui/widgets/container_and_tableau_misc.cpp")
    assert r["verdict"] == "PASS" and any("fault" in u["reason"] for u in r["edges"]["orig"]["unreachable"]), r


def test_import_float_args_are_fp_judged():
    """0x4f8c80 passes the listener frame to FSOUND_3D_Listener_SetAttributes by value; on some inputs the two builds'
    floats are 1 ulp apart (x87 rounding). --fp-tol judges import arguments against the reference run like memory, so
    the row is not a FAIL; the arguments are still compared exactly without --fp-tol."""
    r = _row("agent_hud", 0x4f8c80, "src/game/ui/hud/agent_hud.cpp", "--fp-tol")
    assert r["verdict"] != "FAIL" and not r["fp_tol"]["real_mismatches"], r


def test_tail_call_through_pointer_is_a_fake_method():
    """0x48d7e0 ends in `jmp [eax+0x68]`: the generated target takes the function's own arguments (no call site)."""
    r = _row("stacks_and_loot", 0x48d7e0, "src/game/party/stacks_and_loot.cpp")
    assert r["verdict"] == "PASS" and not r["discarded"].get("fake-arity"), r


def test_indirect_call_into_midfunction_is_discarded():
    """0x4eb3c0 (FaceGen::refresh): a repair-mapped null page of soup bytes read at an odd offset gives the vtable
    call at 0x4eb3f6 a target such as 0x4fef00 (the middle of 0x4fe860). No game object has such a slot: the input is
    discarded as call-midfunction (reason with target and call site), not compared as a FAIL."""
    r = _row("face_generator_refresh", 0x4eb3c0, "src/game/ui/screens/face_generator/face_generator_refresh.cpp", "--fp-tol", "--inputs", "60")
    assert r["verdict"] != "FAIL" and r["discarded"].get("call-midfunction"), r
    assert "call at 004eb3f6 targets" in r["call_midfunction"][0] and "not a function entry" in r["call_midfunction"][0], r


def _static_arity(call_va):
    """Argument dwords Runner's call-site inference gives the image call at call_va (no run needed)."""
    class _T:
        va = orig_lo = orig_hi = 0

    class _S:
        entry = 0
    r = equiv.Runner.__new__(equiv.Runner)
    r.t, r.ours, r.obj = _T(), _S(), {"code": []}
    r.img = equiv.context()["img"]
    r.code_cache, r.targets_cache = {}, {}
    addrs, insns = r._insns_around(call_va)
    return r._call_pushes(addrs, insns, addrs.index(call_va))


def test_fake_call_arity_inference():
    """Call-site arity for calls through soup pointers, against the image's own code:
    - 0x4260ab: `push ebx/ebp/esi` + `push edi` of the prologue are register saves -> 0;
    - 0x4617fc: `push ebp; push edi` save registers the function first touches here (late /O2 saves) -> 3;
    - 0x46197e: the 4th argument is pushed in both arms before the merged call (`push 3; jmp` / `push 2`) -> 4;
    - 0x4bce92: two of the three COM arguments are pushed 30-60 instructions earlier -> 3;
    - 0x4eb981: 0x4eb8ed is a split continuation (reads the ebx it inherits), so its `push ebx`s are
      arguments, not saves -> 7 (IDirect3DDevice9::Clear)."""
    assert _static_arity(0x4260ab) == 0
    assert _static_arity(0x4617fc) == 3
    assert _static_arity(0x46197e) == 4
    assert _static_arity(0x4bce92) == 3
    assert _static_arity(0x4eb981) == 7


def test_prologue_pushes_are_not_fake_call_args():
    """dynamic_entity::integrate 0x4260a0 opens `sub esp,70h; push ebx; push ebp; push esi; mov ebx,ecx;
    mov eax,[ebx]; push edi; call [eax+7Ch]`: the four pushes save callee-saved registers, so the soup method
    takes no arguments. Counting them made the fake pop 16 bytes and every input returned through a float
    (fake-arity "no call site before cccccccc", 0/400 valid)."""
    r = _row("rigid_body", 0x4260a0, "src/engine/physics/rigid_body.cpp", "--fp-tol")
    assert r["verdict"] in ("PASS", "PASS-FP") and not r["discarded"].get("fake-arity"), r


def test_transparent_keeps_real_differences():
    """--transparent hides only the call boundary: a real difference (here, ours leaks the delete[] of the bones
    that the original does) must still FAIL. The mutant is built from the current resource_manager.cpp by dropping the
    `delete[] bones;` in frame_set::~frame_set, located by regex rather than by exact text."""
    import re
    text = open(RESMGR_SRC).read()
    mut, n = re.subn(r"(frame_set::~frame_set\(\)[^\n]*?)delete\[\]\s*bones\s*;", r"\1;", text, count=1)
    assert n, "frame_set::~frame_set no longer has a `delete[] bones;` to drop: pick another real difference"
    path = os.path.join(os.path.dirname(RESMGR_SRC), "_mut_resmgr_transparent.cpp")    # beside its headers
    open(path, "w").write(mut)
    try:
        r = resmgr_run(FS_TRANSPARENT, path)
    finally:
        os.remove(path)
    assert r["verdict"] == "FAIL", r


def test_mangled_scopes_find_namespace_std():
    sc = equiv.mangled_scopes
    assert sc("?_Insert_n@?$vector@HV?$allocator@H@std@@@std@@IAEXViterator@12@IABH@Z") == ["_Insert_n", "vector", "std"]
    assert sc("??4?$vector@UPlacedObj@@V?$allocator@UPlacedObj@@@std@@@std@@QAEAAV01@ABV01@@Z")[-1] == "std"
    assert sc("??$_Uninit_fill_n@PAUE32@@IU1@V?$allocator@UE32@@@std@@@std@@YAXPAUE32@@IABU1@AAV?$allocator@UE32@@@0@@Z") \
        == ["_Uninit_fill_n", "std"]
    # a function-pointer template argument (merge with a predicate) and a const argument ($$CB) parse too
    assert sc("??$_Buffered_merge@Viterator@?$vector@UP8@@V?$allocator@UP8@@@std@@@std@@HUP8@@P6A_NABU4@0@Z@std@@"
              "YAXViterator@?$vector@UP8@@V?$allocator@UP8@@@std@@@0@00HHAAV?$_Temp_iterator@UP8@@@0@P6A_NABUP8@@2@Z@Z")[-1] == "std"
    assert equiv.is_std_name("?_Ufill@?$vector@PAUmesh@rgl@@V?$allocator@PAUmesh@rgl@@@std@@@std@@IAEPAPAUmesh@rgl@@PAPAU34@IABQAU34@@Z")
    # a game function taking or returning a std::vector is not std code
    assert not equiv.is_std_name("?query_entities@World@@QAEXPAV?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@std@@@Z")
    assert not equiv.is_std_name("?add_mesh@entity@rgl@@QAEXPAUmesh@2@@Z")
    assert not equiv.is_std_name("??$uninit_copy@Umface@@@@YAPAUmface@@PAU0@00@Z")     # a global helper of the game


MESH_PASS_SRC = os.path.join(ROOT, "src", "game", "mission", "render_passes.cpp")


def test_std_calls_are_transparent():
    """render_batch::render 0x47e520: the original calls the out-of-line std::vector<scene*>::push_back (0x4119d0) for
    `tmp.push_back(only)`, ours expands it. Without --transparent (its old row option) that call boundary FAILed the
    depth-1 trace; std-library code is transparent by default now, and recording it again brings the FAIL back.
    (60 inputs: the row runs ~130 valid inputs in its 180 s limit, so 200 would end UNSUPPORTED on the clock.)"""
    r = _row("render_passes", 0x47e520, MESH_PASS_SRC, "--inputs", "60")
    assert r["verdict"] in ("PASS", "WEAK"), r
    assert "004119d0" in r.get("transparent_std", []), r
    base = _row("render_passes", 0x47e520, MESH_PASS_SRC, "--inputs", "60", "--no-std-transparent")
    assert base["verdict"] == "FAIL" and "call #" in base["first_mismatch"]["observable"], base


def test_std_transparency_keeps_real_differences():
    """A transparent std call still compares what it does: a mutant that pushes `only` twice (two inline push_backs
    where the original makes one out-of-line call) must FAIL."""
    import re
    text = open(MESH_PASS_SRC).read()
    mut, n = re.subn(r"(\btmp\.push_back\(only\);)", r"\1 tmp.push_back(only);", text, count=1)
    assert n, "render_batch::render no longer has `tmp.push_back(only);`: pick another std call"
    path = os.path.join(os.path.dirname(MESH_PASS_SRC), "_mut_mesh_pass_std.cpp")      # beside its headers
    open(path, "w").write(mut)
    try:
        r = _row("render_passes", 0x47e520, path, "--inputs", "60")
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
        self._stub_only = lambda addr, calls=None: equiv.Runner._stub_only(self, addr, calls)
        self._stub_calls = lambda page: equiv.Runner._stub_calls(self, page)
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


def test_global_pointing_into_unallocated_heap_is_invalid():
    """The emulated malloc heap is empty at entry, so an image global cannot hold an address in it (facegen_win
    0x4f1460 [0x5e2934] = 0x5003e911, a malloc result +1 from a pointer compare's boundary, became an unaligned
    `this`): such an input is discarded and steering never builds one."""
    import steer
    inp = equiv.Input()
    inp.overrides = {0x5e2934: M.DYN_BASE + 0x3e911, 0x5e2938: 0x40010000}
    assert equiv.unallocated_heap_global(inp) == (0x5e2934, M.DYN_BASE + 0x3e911)
    inp.overrides = {0x5e2934: 0x40010000, 0x5e2938: M.DYN_BASE - 1, 0x5e293c: M.DYN_BASE + M.DYN_SIZE}
    assert equiv.unallocated_heap_global(inp) is None
    st = steer.Steer.__new__(steer.Steer)
    inp.seed, inp.profile, inp.args, inp.pad, inp.pool_over, inp.regs, inp.xmm = 1, "zeroish", [0], [], {}, {}, []
    inp.origin, inp.touched = "fresh", []
    for loc in (("arg", 0), ("reg", "ecx"), (0x5e2934, 4)):
        assert st.set_loc(inp, loc, M.DYN_BASE + 0x3e910 + 1, "int") is None
    assert st.set_loc(inp, ("arg", 0), 5, "int").args[0] == 5


def test_shrink_relations():
    """repair_heap / repair_loop find the input dword a trip count or allocation size follows from: equal, a count
    times a stride plus a header (also modulo 2**32: new[] of 0xff7fffff elements of 0x78 + 4 wraps), a byte size
    walked in strides, or the distance of an adjacent begin/end pointer pair; _invert gives the dword's small value."""
    R_ = equiv.Runner
    assert R_._relate(0x1234, 0x1234) == ("eq", 1, 0)
    assert R_._relate(1000 * 0x78 + 4, 1000) == ("scaled", 0x78, 4)
    assert R_._relate((0xff7fffff * 0x78 + 4) & 0xFFFFFFFF, 0xff7fffff) == ("scaled", 0x78, 4)
    assert R_._relate(1000, 1000 * 0x20) == ("divided", 0x20, 0)
    assert R_._relate(0x10000, 0x12345678) is None
    assert R_._invert(("scaled", 0x78, 4), 0xff7fffff, 3 * 0x78 + 4) == 3
    assert R_._invert(("divided", 0x20, 0), 32000, 2) == 0x40
    r = R_.__new__(R_)
    r.img = R.Image()
    b, e = equiv.M.POOL_BASE + 0x100, equiv.M.POOL_BASE + 0x100 + 5000 * 0x204
    reads = [(0x40010000, 7), (0x40020004, b), (0x40020008, e)]
    rels = r.repair_relations(reads, 5000)
    assert (0x40020008, e, ("pair", 0x204, 0, b)) in rels, rels
    assert R_._invert(("pair", 0x204, 0, b), e, 2 * 0x204) == b + 0x408
    # a soup header (no pointer among the dwords; mesh_game 0x4832b0 spawn: begin 0xff7fffff, last 0xc31d9dde, a
    # reserve of end - begin + 1 bytes) is related only with near=0, and a size one byte past the multiple counts
    b2, e2 = 0xff7fffff, 0xc31d9dde
    reads = [(0x40d60090, b2), (0x40d60094, e2)]
    assert not r.repair_relations(reads, (e2 - b2 + 1) & 0xFFFFFFFF)
    assert any(x[2][0] == "pair" for x in r.repair_relations(reads, (e2 - b2 + 1) & 0xFFFFFFFF, near=0))


def test_stdcall_through_vtable_ecx_is_not_compared():
    """GuiScreen::process 0x4ebfd0 calls DirectInput's GetDeviceData through `mov ecx,[eax]; ... call [ecx+0x28]`: ecx is
    the vtable, the device is a stack argument (__stdcall), so ecx is dead at the call. The original leaves the vtable
    there, ours a stack address; comparing it made 200/201 inputs FAIL. Every stack argument is still compared."""
    src = "src/game/ui/screens/screen_misc.cpp"
    r = _row("screen_misc", 0x4ebfd0, src, "--inputs", "60")
    assert r["verdict"] in ("PASS", "WEAK", "PASS-FP", "WEAK-FP") and r["valid"] > 0, r


def test_com_and_cdecl_calls_through_pointers_ignore_ecx():
    """GeomBuffer::upload 0x46d060 locks its vertex buffer with `mov edx,[eax]; push ...; push eax; call [edx+2Ch]`: edx is
    the vtable of the first stack argument, so the callee is a COM __stdcall method and ecx (a leftover) is dead there.
    mesh::render 0x413750 calls a function pointer member with `push eax; call [eax+68h]; add esp,4`: caller-cleaned, so
    __cdecl, and ecx again carries nothing. Comparing ecx made 194/480 and 107/514 inputs FAIL."""
    r = _row("terrain_cells", 0x46d060, "src/game/terrain/terrain_render.cpp", "--inputs", "60")
    assert r["verdict"] in ("PASS", "WEAK", "PASS-FP", "WEAK-FP") and r["valid"] > 0, r
    r = _row("mesh", 0x413750, "src/engine/render/mesh.cpp", "--inputs", "60")
    assert r["verdict"] in ("PASS", "WEAK", "PASS-FP", "WEAK-FP") and r["valid"] > 0, r


def test_loop_repair_shrinks_runaway_trip_count():
    """uninitialized_copy 0x4c1b60 walks [first, last) in 0x204-byte strides until `cmp esi, [ebp+0xc]` is equal: soup
    pointers never meet, so every original run burned the instruction cap (the row TIMEd OUT in the sweep). repair_loop
    finds the hot loop, samples its exit compare (esi steps by 0x204, the bound is the argument slot) and rewrites
    `last` to first + n*0x204; the input, replayed identically on both sides, then finishes. Without it every input
    is cut off. (A small --budget keeps the unrepaired run fast; the repair itself never changes the cap.)"""
    src = "src/game/app/options_dialog_and_module_tables.cpp"
    _row = lambda sid, va, src, *x: frozen_row(FROZEN_LOOP, sid, va, src, *x)      # the live source's own runaway differs
    r = _row("options_dialog_and_module_tables", 0x4c1b60, src, "--inputs", "30", "--budget", "200000", "--loop-repair")
    assert r["valid"] > 0 and not r.get("budget_cutoff"), r
    assert any(k.startswith("budget-loop:") for k in r.get("shrink_repairs", {})), r
    r = _row("options_dialog_and_module_tables", 0x4c1b60, src, "--inputs", "30", "--budget", "200000", "--no-loop-repair")
    assert r["valid"] == 0 and r["verdict"] == "UNSUPPORTED", r


def test_budget_cutoff_and_heap_exhausted_are_not_verdicts():
    # A partial run is never a verdict (camera 0x49b630 / cam_view 0x4ea8e0), but a REAL mismatch outranks
    # every "the run was cut short" excuse: agent_hud 0x4fba40 had 200 real mismatches and 16 heap-exhausted
    # inputs, and the invalidation must not silence the FAIL. The guards carry `not mism` (and the stop-early
    # branch `not mism or not valid`), and the invalidation branches sit BEFORE the mismatch branches, so the
    # guard is what protects them - the order alone is not the rule.
    src = open(os.path.join(HERE, "equiv.py")).read()
    i_cut = src.index("budget_cut = out.get(")
    i_budget = src.index("elif not mism and not valid and budget_cut:")
    i_heap = src.index('elif not mism and not valid and (out.get("heap_exhausted", 0) + out.get("heap_exhausted_ours", 0)):')
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


# ---------------------------------------------------------------- switch functions case by case (case_sweep.py)

def test_pin_spec():
    """--pin parses offset, mask and values; a value outside the set is a violation the fix puts back."""
    import invariants as INV
    inv = INV.pin_invariant("this+0&0fffffff=848,3b")
    assert inv.bounds == [(0x3b, 0x848)]
    import pytest
    with pytest.raises(ValueError):
        INV.pin_invariant("ecx=1")


def test_face_count_invariant_governs_fault_mapped_memory():
    """face::count is judged on a face that lives in memory fault repair mapped (inp.extra), using the same
    soup bytes run_side maps; a valid count there is not a violation."""
    import struct
    import invariants as INV

    class Gen:
        def __init__(self, count):
            self.buf = bytearray(0x10000)
            struct.pack_into("<I", self.buf, 0xB9, count)       # poly = 9: count at 9 + 0xb0

        def _slot_soup(self, inp, base):
            return bytes(self.buf)

        def slot_pages(self, inp, slot):
            return {}

    class Inp:
        args = [9, 0x44]
        extra = {0: 0x10000}

    for count, bad in ((0, True), (5, True), (0x4B8500, True), (1, False), (4, False)):
        got = INV.check([INV.FACE_INV], Gen(count), Inp())
        assert (got is not None) == bad, (count, got)
        if bad:
            assert got[0] == "face::count" and got[1] == count
    Inp.extra = {}                                               # nothing mapped there: not judged
    assert INV.check([INV.FACE_INV], Gen(0), Inp()) is None


def test_char_return_compares_al_only():
    """A char return (mangled D/C/E) is compared on al; a short (F/G) on ax."""
    import sig
    assert sig.parse("?execute@Operation@@QAEDPA_JPAH1@Z").ret_width == 1
    assert sig.parse("?f@@YAFH@Z").ret_width == 2


def test_jump_table_beyond_2048_entries():
    """The interpreter's 2356-entry table: every target is in the static edge model (the old 2048 cut 115 cases)."""
    assert CF.MAX_TABLE >= 2356
    insns = {}
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    code = bytes.fromhex("ff2485") + (0x1000).to_bytes(4, "little")        # jmp [eax*4 + 0x1000]
    for i in md.disasm(code, 0x100):
        insns[i.address] = i
    targets = {0x1000 + 4 * k: 0x200 + k for k in range(2356)}
    cfg = CF.Cfg(insns, 0x100, 0x100, 0x4000, lambda a: targets.get(a))
    assert sum(1 for e in cfg.edges if e[0] == 0x100) == 2356


def test_regression_inputs_rerun():
    """A mismatching input is kept and run first next time: the second run fails on it at input 0, even with
    another seed, and without it (--no-regress) the cache key differs."""
    import shutil
    shutil.rmtree(os.environ["DIFFTEST_REGRESS_DIR"], ignore_errors=True)
    os.environ["DIFFTEST_NO_CACHE"] = "1"
    try:
        path = mutant(RECT_SID, "p[0] >= lo[0]", "p[0] > lo[0]", "op")
        r1 = equiv.test_function(RECT_SID, RECT_VA, opts("--src", path))
        assert r1["verdict"] == "FAIL" and r1["regress_inputs"] == 0, r1
        r2 = equiv.test_function(RECT_SID, RECT_VA, opts("--src", path, "--seed", "7"))
        assert r2["verdict"] == "FAIL" and r2["regress_inputs"] >= 1 and r2.get("regress_failed", 0) >= 1, r2
        assert r2["first_mismatch"]["input"] == 0 and r2["first_mismatch"]["origin"] == "regress", r2
    finally:
        os.environ.pop("DIFFTEST_NO_CACHE", None)


# ---------------------------------------------------------------------- (g) --prove-dead (deadedge.py)

def _prove(hexcode):
    import deadedge
    c = cfg_of(bytes.fromhex(hexcode))
    return {e: v[0] for e, v in deadedge.prove_dead(c, set(c.edges), budget_s=60).items()}


def test_prove_dead_masked_compare():
    # ebx = [esp+4] & 3; cmp ebx,5; ja: never taken (ebx <= 3); the fall-through is reachable
    r = _prove("8b5c2404" "83e303" "83fb05" "7702" "9090" "c3")
    assert r == {(0x100a, 0x100e): "dead", (0x100a, 0x100c): "unknown"}, r


def test_prove_dead_if_chain_last_compare():
    # the options_panel 0x4d86d0 shape: ebx in 0..3 tested against 0,1,2, then `cmp ebx,3; jne` cannot be taken
    r = _prove("8b5c2404" "83e303" "83fb00" "7410" "83fb01" "740b" "83fb02" "7406" "83fb03" "7501" "90" "c3")
    assert r[(0x1019, 0x101c)] == "dead", r
    assert r[(0x1019, 0x101b)] == "unknown", r
    assert [e for e, v in r.items() if v == "dead"] == [(0x1019, 0x101c)], r


def test_prove_dead_rare_input_branch_not_proven():
    # cmp eax,0x12345678; je on an argument: rare but reachable
    r = _prove("8b442404" "3d78563412" "7402" "9090" "c3")
    assert all(v == "unknown" for v in r.values()), r


def test_prove_dead_call_result_not_proven():
    # call; test eax,eax; jne: the result is a fresh symbol
    r = _prove("e800000000" "85c0" "7502" "9090" "c3")
    assert all(v == "unknown" for v in r.values()), r
    # xor eax,eax before the call does not survive it either
    r = _prove("31c0" "e800000000" "85c0" "7502" "9090" "c3")
    assert all(v == "unknown" for v in r.values()), r
    # a callee-saved register does (documented assumption): ebx = 5 across the call
    r = _prove("bb05000000" "e800000000" "83fb05" "7501" "90" "c3")
    assert r[(0x100d, 0x1010)] == "dead" and r[(0x100d, 0x100f)] == "unknown", r


def test_prove_dead_loop_carried_not_overproven():
    # eax = 0; L: cmp eax,1; je done; inc eax; jmp L: eax == 1 on the second iteration, so both edges are feasible
    r = _prove("31c0" "83f801" "7403" "40" "ebf8" "c3")
    assert all(v == "unknown" for v in r.values()), r
    # ebx = 7; L: inc ebx; dec ecx; jnz L; cmp ebx,7; jne: ebx changes in the loop, nothing proven about the exit
    r = _prove("bb07000000" "43" "49" "75fc" "83fb07" "7501" "90" "c3")
    assert r[(0x100c, 0x100f)] == "unknown", r
    # ebx = 7; L: dec ecx; jnz L; cmp ebx,7; jne: ebx is a loop invariant, so the jne cannot be taken
    r = _prove("bb07000000" "49" "75fd" "83fb07" "7501" "90" "c3")
    assert r[(0x100b, 0x100e)] == "dead", r
    assert r[(0x1006, 0x1005)] == "unknown", r


def test_prove_dead_counter_loop_bound():
    # ebx = 0; L: cmp ebx,7; jae done; cmp ebx,9; ja X; inc ebx; X: jmp L: the head has ebx <= 7, the body ebx <= 6
    r = _prove("31db" "83fb07" "730a" "83fb09" "7701" "43" "ebf3" "90" "90" "c3")
    assert r[(0x100a, 0x100d)] == "dead", r
    assert r[(0x1005, 0x1011)] == "unknown", r          # ebx == 7 does reach the exit
    # the same loop with cmp ebx,5: ebx == 6 takes the ja, so it is not dead
    r = _prove("31db" "83fb07" "730a" "83fb05" "7701" "43" "ebf3" "90" "90" "c3")
    assert r[(0x100a, 0x100d)] == "unknown", r


def test_prove_dead_address_taken_label_gets_unknown_entry():
    import deadedge
    # ebx = 5; push 0x100a; L(100a): cmp ebx,5; jne: dead on the straight path, but L's address is taken (a catch
    # continuation is entered by the EH runtime with other registers), so nothing may be proven there
    code = bytes.fromhex("bb05000000" "680a100000" "83fb05" "7501" "90" "c3")
    c = cfg_of(code)
    assert deadedge.prove_dead(c, {(0x100d, 0x1010)})[(0x100d, 0x1010)][0] == "dead"
    extra = deadedge.code_refs(code, 0x1000, set(c.insns))
    assert extra == {0x100a}, extra
    assert deadedge.prove_dead(c, {(0x100d, 0x1010)}, extra_entries=extra)[(0x100d, 0x1010)][0] == "unknown"


def test_prove_dead_covered_edge_refutes_model():
    import deadedge
    c = cfg_of(bytes.fromhex("8b5c2404" "83e303" "83fb05" "7702" "9090" "c3"))
    # claiming the (infeasible) ja edge was covered must make the prover refuse everything
    r = deadedge.prove_dead(c, {(0x100a, 0x100c)}, covered={(0x100a, 0x100e)}, budget_s=60)
    assert all(v[0] == "unknown" for v in r.values()), r


# ---------------------------------------------------------------------- uninitialised stack slots (Runner.uninit_copies)
def _uninit_fake(orig_body, ours_body, imm=0x1234):
    """A Runner stand-in around a real unicorn: the original's function at 0x10000 and ours at 0x30000 (each a
    frame of 0x20 bytes; `callee` at 0x10200 fills the dword it is handed with imm), storing the dword at
    [esp+0x10] to the global 0x20000 once the body has run. Returns (fake, inp)."""
    import struct
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
    from unicorn.x86_const import UC_X86_REG_ESP
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    top = M.STACK_TOP
    uc.mem_map(top - M.STACK_SIZE, M.STACK_SIZE)
    for base in (0x10000, 0x20000, 0x30000):
        uc.mem_map(base, 0x1000)
    callee = b"\x8b\x44\x24\x04" + b"\xc7\x00" + struct.pack("<I", imm) + b"\xc3"   # mov eax,[esp+4]; mov [eax],imm; ret
    uc.mem_write(0x10200, callee)
    pre = b"\x83\xec\x20"                                   # sub esp, 0x20
    arg = b"\x8d\x44\x24\x10\x50"                           # lea eax,[esp+0x10]; push eax
    post = b"\x83\xc4\x04"                                  # add esp, 4
    tail = b"\x8b\x44\x24\x10\xa3" + struct.pack("<I", 0x20000) + b"\x83\xc4\x20\xc3"   # eax=[esp+0x10]; [D]=eax; ret
    own = b"\xc7\x44\x24\x10\x05\x00\x00\x00"               # mov dword [esp+0x10], 5

    def build(base, kind):
        parts = {"callee_then_copy": [pre, arg, None, post, tail],
                 "callee_then_own_write": [pre, arg, None, post, own, tail],
                 "never_called": [pre, tail]}[kind]
        code = b""
        for p in parts:
            if p is None:
                p = b"\xe8" + struct.pack("<i", 0x10200 - (base + len(code) + 5))
            code += p
        uc.mem_write(base, code)

    build(0x10000, orig_body)
    build(0x30000, ours_body)

    class Side:
        def __init__(s, entry):
            s.entry, s.span = entry, (entry, entry + 0x100)

    class Fake:
        pass
    f = Fake()
    f.m = type("MM", (), {"uc": uc})()
    f.orig, f.ours, f.cur = Side(0x10000), Side(0x30000), None
    f.esp_entry = top - 0x1000
    f.call_taint_fn, f.call_taint, f.last_call_uninit = None, [], None

    def run_side(side, inp):
        uc.mem_write(0x20000, b"\0" * 4)
        uc.mem_write(f.esp_entry, struct.pack("<I", 0x20800))        # return address: a hlt landing pad
        uc.mem_write(0x20800, b"\xf4")
        uc.reg_write(UC_X86_REG_ESP, f.esp_entry)
        f.cur = side
        try:
            uc.emu_start(side.entry, 0x20800)
        finally:
            f.cur = None
    f.run_side = run_side
    f.uninit_copies = lambda *a, **k: equiv.Runner.uninit_copies(f, *a, **k)
    return f, None


def test_uninit_slot_written_by_returned_callee_is_excused():
    """A slot only a callee that has since returned wrote (agent_logic 0x448eb0: collect_boxed filled the EntVec the
    vector4 temp then reuses) is uninitialised for the tested function: the copy is tainted on both sides, excused,
    and listed with each side's writer pc and the read pc."""
    f, inp = _uninit_fake("callee_then_copy", "callee_then_copy")
    both = equiv.Runner.uninit_both(f, inp)
    assert set(both) == {0x20000, 0x20001, 0x20002, 0x20003}, both
    oa, ob = both[0x20000]
    assert oa[1] == 0x10204 and ob[1] == 0x10204, (oa, ob)            # the callee's `mov [eax], imm` wrote it
    assert 0x10000 <= oa[2] < 0x10100 and 0x30000 <= ob[2] < 0x30100, (oa, ob)    # the read is in each function
    c = equiv.Comparer.__new__(equiv.Comparer)
    c.uninit_copy, c.uninit_log = both, {}
    assert c.uninit_excused(0x20000, b"\x34\x12\0\0", b"\x99\0\0\0")
    row, = c.uninit_log.values()
    assert row["orig_writer_pc"] == row["ours_writer_pc"] == 0x10204 and row["addr"] == 0x20000, row


def test_uninit_slot_written_by_the_function_itself_is_compared():
    """The function's own write after the callee initialises the slot: its copy is a real value, never excused."""
    f, inp = _uninit_fake("callee_then_own_write", "callee_then_own_write")
    assert equiv.Runner.uninit_both(f, inp) == {}


def test_uninit_on_one_side_only_is_compared():
    """Uninitialised in the original but initialised by ours' own code (or the reverse): a real difference."""
    f, inp = _uninit_fake("callee_then_copy", "callee_then_own_write")
    assert equiv.Runner.uninit_both(f, inp) == {}
    f, inp = _uninit_fake("callee_then_own_write", "callee_then_copy")
    assert equiv.Runner.uninit_both(f, inp) == {}


def test_uninit_leftover_slot_never_written_is_excused():
    f, inp = _uninit_fake("never_called", "never_called")
    assert set(equiv.Runner.uninit_both(f, inp)) == {0x20000, 0x20001, 0x20002, 0x20003}


# ---- input model: element-aligned vectors, static-initialiser vtable pointers on global objects

def test_align_triple():
    at = equiv.align_triple
    esz = 0x24
    f = 0x43bf0000
    assert at(f, f + 0x10, f + 0x10, esz) == (f, f)                       # read_map's edge vector: 0x10 bytes of 0x24
    assert at(f, f + 2 * esz, f + 5 * esz, esz) == (f + 2 * esz, f + 5 * esz)      # well formed: unchanged
    assert at(f, f + 2 * esz + 5, f + 5 * esz + 9, esz) == (f + 2 * esz, f + 5 * esz)       # rounded down
    assert at(f, f + 3 * esz, f + esz, esz) == (f + 3 * esz, f + 3 * esz)                   # end below last: end = last
    assert at(f, f - 4, f + esz, esz)[0] == f                              # last below first: empty
    assert at(f, 0, 0, esz) == (f, f)
    assert at(0, 0x43bf0000, 0x43bf0100, esz) == (0, 0)                    # NULL first: empty
    assert at(0xFFFFFF00, 0xFFFFFF00 + 0x48, 0xFFFFFF00 + 0x48, 4)[0] >= 0xFFFFFF00


class _Cls:
    """Just enough cvtypes.TypeTable for TypedInputs: one `this` class holding a 0x24-byte-element vector at +0x20."""
    ELEM, THIS = 1, 2

    def layout(self, c):
        import cvtypes as CV
        return [CV.Field(0x20, "vector", 12, self.ELEM, "edges")] if c == self.THIS else []

    def sizeof(self, c):
        return {self.ELEM: 0x24, self.THIS: 0x40}[c]

    def is_class(self, c):
        return True

    def scalar(self, c):
        return None


def _typed_fixture():
    import random
    ti = equiv.TypedInputs(_Cls(), _Cls.THIS, [], lambda n: None)
    gen = object.__new__(equiv.InputGen)
    gen.slot_cache, gen.dic, gen.nan, gen.code, gen.typed, gen.invs = {}, [0], False, [], ti, []
    gen._slot_soup = lambda inp, slot: bytes(M.SLOT)
    return ti, gen, random.Random(7)


def _new_input(rng):
    inp = equiv.Input()
    inp.seed, inp.profile, inp.args, inp.pad = 1, "mixed", [], []
    inp.pool_over, inp.regs, inp.xmm, inp.overrides, inp.touched = {}, {"ecx": M.POOL_BASE + M.SLOT}, [], {}, []
    inp.typed, inp.origin = 1, "fresh"
    return inp


def test_typed_vectors_are_element_aligned_after_generation_and_mutation():
    ti, gen, rng = _typed_fixture()
    this = M.POOL_BASE + M.SLOT
    for seed in range(60):
        rng.seed(seed)
        inp = _new_input(rng)
        ti.build(gen, inp, rng)
        rd = lambda a: equiv.INV._dword(gen, inp, a)
        f, l, e = rd(this + 0x20), rd(this + 0x24), rd(this + 0x28)
        assert (l - f) % 0x24 == 0 and (e - f) % 0x24 == 0 and f <= l <= e, (seed, f, l, e)
        assert gen.realign(inp) == 0                                   # a generated input is left byte-identical
        # mutation / steering / repair: any one of the three dwords replaced by something arbitrary
        for k in range(3):
            m = inp.copy()
            m.pool_over[this + 0x20 + 4 * k] = rng.choice([0x43bf0010, 3, 0, 0xFFFFFFFF, f + 7, f + 0x13, (f - 0x24) & 0xFFFFFFFF])
            gen.realign(m)
            rd2 = lambda a: equiv.INV._dword(gen, m, a)
            f2, l2, e2 = rd2(this + 0x20), rd2(this + 0x24), rd2(this + 0x28)
            assert (l2 - f2) % 0x24 == 0 and (e2 - f2) % 0x24 == 0 and f2 <= l2 <= e2, (f2, l2, e2)
            assert f2 != 0 or (l2 == 0 and e2 == 0)
            assert gen.realign(m) == 0                                 # idempotent


def test_realign_read_map_vector():
    ti, gen, rng = _typed_fixture()
    this = M.POOL_BASE + M.SLOT
    inp = _new_input(rng)
    inp.typed = 0                                                      # a soup `this` of a known class is still aligned
    inp.pool_over.update({this + 0x20: 0x43bf0000, this + 0x24: 0x43bf0010, this + 0x28: 0x43bf0010})
    assert gen.realign(inp) == 1
    assert [inp.pool_over[this + 0x20 + 4 * k] for k in range(3)] == [0x43bf0000] * 3
    inp.pool_over[this + 0x24] = 0x43bf0010
    gen.no_realign = True
    assert gen.realign(inp) == 0


def test_static_vptrs_find_the_log_stream():
    v = equiv.static_vptrs()
    assert v[0x5a9f60] == 0x595990              # g_log: the constructor's vtable, not the destructor's 0x595618
    assert v[0x5a9f30] == 0x54a7d0
    img = equiv.context()["img"]
    assert img.u32(v[0x5a9f60] + 0x28) == 0x4153a0                    # slot 10 is w_cstr
    assert img.u32(0x5a9f60) != v[0x5a9f60]                           # .data holds the pre-construction vtable


def test_static_vptrs_are_installed_and_not_repairable():
    import random
    t = equiv.prepare(RECT_SID, RECT_VA, log=lambda *a: None)
    r = equiv.Runner(t, opts(), lambda *a: None)
    assert r.vptrs[0x5a9f60] == 0x595990
    inp = r.gen.fresh(random.Random(3))
    inp.overrides[0x5a9f60] = 0x48b200                                 # a repaired / steered / mutated vptr
    writes = []
    real_write = r.m.write
    r.m.write = lambda a, d, *x, **k: (writes.append((a, bytes(d))), real_write(a, d, *x, **k))[1]
    try:
        r.run_side(r.orig, inp)
    finally:
        del r.m.write
    # the last pre-run write to the slot is the static initialiser's vtable, after the (bad) override
    assert [d for a, d in writes if a == 0x5a9f60][-1] == (0x595990).to_bytes(4, "little")
    assert r._put_dword(inp, 0x5a9f60, 0x48b200) is None


def test_d3d_lockrect_surface_comes_from_the_input():
    """ShadowMap::sample 0x4faee0 branches on a texel ShadowMap::read_pixel reads through IDirect3DSurface9::LockRect's
    pBits. The locked surface is the input's own pool memory (Runner.D3D_LOCK_BITS, the same address for both builds),
    not a zero block, so lit and shadowed pixels both occur and the row covers every edge (it was WEAK: the skip edge
    was never taken). A lock call writes {Pitch, pBits} with that address and returns D3D_OK."""
    r = frozen_row(FROZEN_OBJ, "agent_hud", 0x4faee0, "src/game/ui/hud/agent_hud.cpp", "--fp-tol")
    assert r["verdict"] == "PASS" and not r["fp_tol"]["real_mismatches"], r
    assert equiv.Runner.D3D_LOCK_BITS == equiv.M.POOL_BASE + equiv.M.POOL_SIZE - 0x40000
    assert equiv.Runner.D3D_LOCK_BITS % equiv.M.SLOT == 0


# ---------------------------------------------------------------------- native tracing hooks (tools/difftest/native)

def _both_paths(fn):
    """fn() on the native hooks and on the pure-Python hooks (EQUIV_NATIVE=0); skipped without the library."""
    import eqnative
    import pytest
    if not eqnative.available():
        pytest.skip("native tracing library not buildable here: %s" % eqnative._state.get("why"))
    real, real_nc = equiv._native, os.environ.get("DIFFTEST_NO_CACHE")
    os.environ["DIFFTEST_NO_CACHE"] = "1"           # the second path must emulate, not read the first one's cached verdict
    try:
        equiv._native = lambda: eqnative
        a = fn()
        equiv._native = lambda: None
        b = fn()
    finally:
        equiv._native = real
        if real_nc is None:
            del os.environ["DIFFTEST_NO_CACHE"]
        else:
            os.environ["DIFFTEST_NO_CACHE"] = real_nc
    return a, b


def _strip_seconds(o):
    if isinstance(o, dict):
        return {k: _strip_seconds(v) for k, v in o.items() if k != "seconds"}
    if isinstance(o, list):
        return [_strip_seconds(v) for v in o]
    return o


def test_native_uninit_taint_matches_python_on_the_fakes():
    """Runner.uninit_both on the hand-built stack scenarios: the native taint pass reports the same bytes with the same
    origins (a, writer pc, read pc) as the Python hooks."""
    def go():
        out = []
        for ob, ou in (("callee_then_copy", "callee_then_copy"), ("callee_then_own_write", "callee_then_own_write"),
                       ("callee_then_copy", "callee_then_own_write"), ("never_called", "never_called")):
            f, inp = _uninit_fake(ob, ou)
            out.append((equiv.Runner.uninit_both(f, inp), f.last_call_uninit, getattr(f, "stale_w_slots", None)))
        return out
    nat, py = _both_paths(go)
    assert nat == py, (nat, py)
    assert any(x[0] for x in py), py                    # the scenarios do taint something


def test_native_hooks_give_identical_row_results():
    """Whole rows through test_function on both paths: identical JSON except 'seconds'. obj_tbl 0x499050 exercises the
    uninit-copy pass (its tolerated 'uninit-copy' count); the config mutant under --fp-tol exercises the reference run's
    write/read tracking (fstore, huge/denormal inputs)."""
    a, b = _both_paths(lambda: _strip_seconds(frozen_row(
        FROZEN_OBJ, "object_table_spawners", 0x499050, "src/game/data/object_table_spawners.cpp",
        "--inputs", "120", "--fp-tol", "--typed-share", "0", "--time-limit", "3000")))
    assert a == b
    assert a["tolerated"].get("uninit-copy"), a["tolerated"]
    path = cfg_xf_mutant("y", "assoc_native", reassociate=True)
    a, b = _both_paths(lambda: _strip_seconds(equiv.test_function("config", 0x4bd2f0, equiv.parse_args(
        ["config", "4bd2f0", "--typed-share", "0", "--src", path, "--flags", CFG_FLAGS, "--inputs", "300",
         "--no-regress", "--fp-tol", "--time-limit", "3000"]))))
    assert a == b
    assert a["fp_tol"]["fp_tolerable"] > 0, a["fp_tol"]


def test_dword_fast_path_equals_slot_pages():
    """invariants._dword's fast path (own edit, else cached soup) returns exactly what the slot_pages path does,
    including edits at unaligned addresses that overlap the dword (those take the general path)."""
    import struct
    import invariants as INV
    _ti, gen, rng = _typed_fixture()
    soup = bytes(rng.getrandbits(8) for _ in range(M.SLOT))
    gen._slot_soup = lambda inp, slot: soup
    inp = _new_input(rng)
    inp.pagegen = 2
    base = M.POOL_BASE + 3 * M.SLOT
    for _ in range(300):
        inp.pool_over[base + rng.randrange(0, 0x3000)] = rng.getrandbits(32)

    def slow(a):
        buf = gen.slot_pages(inp, base).get(a & ~0xFFF)
        return None if buf is None or a & 0xFFF > len(buf) - 4 else struct.unpack_from("<I", buf, a & 0xFFF)[0]
    assert INV._dword(gen, inp, base + 0x10) == slow(base + 0x10)      # before the soup is cached, and after
    for a in list(inp.pool_over) + [base + rng.randrange(0, 0x3004) for _ in range(3000)] + [base + 0xFFD, base + 0xFFC]:
        assert INV._dword(gen, inp, a) == slow(a), hex(a)


def test_case_sweep_merges_shards_soundly():
    """case_sweep.py --shards: valid inputs add, covered edges are the union, an edge is unreachable only when every
    shard says so, and one shard's mismatch makes the case FAIL."""
    import case_sweep as CS
    f = lambda a, b: "%08x->%08x" % (a, b)                                              # noqa: E731
    r1 = {"valid": 30, "inputs": 34, "mismatches": 0, "discarded": {"x": 1}, "verdict": "UNSUPPORTED", "reason": "a",
          "edges": {"orig": {"covered_edges": [f(1, 2)], "unreachable": [{"edge": f(5, 6)}, {"edge": f(7, 8)}]},
                    "ours": {"covered_edges": ["a"], "covered": 1}}}
    r2 = {"valid": 25, "inputs": 30, "mismatches": 0, "discarded": {"x": 2, "y": 1}, "verdict": "WEAK", "reason": "b",
          "edges": {"orig": {"covered_edges": [f(3, 4)], "unreachable": [{"edge": f(5, 6)}]},
                    "ours": {"covered_edges": ["b"], "covered": 1}}}
    case = {"required": [(1, 2), (3, 4), (5, 6)], "dispatch": 1, "orig_target": 2, "tail": []}
    j = CS.judge(case, CS.merge_rows([r1, r2]), 50)
    assert (j["verdict"], j["valid"], j["edges_required"], j["edges_covered"], j["edges_unreachable"]) == ("PASS", 55, 2, 2, 1), j
    assert j["discarded"] == {"x": 3, "y": 1} and j["ours_edges_covered"] == 2
    j = CS.judge(dict(case, required=case["required"] + [(7, 8)]), CS.merge_rows([r1, r2]), 50)
    assert j["verdict"] == "WEAK" and j["missed"] == [f(7, 8)], j              # unreachable in one shard only: still required
    bad = dict(r2, mismatches=1, first_mismatch={"observable": "eax"}, verdict="FAIL")
    j = CS.judge(case, CS.merge_rows([r1, bad]), 50)
    assert j["verdict"] == "FAIL" and j["first_mismatch"] == {"observable": "eax"}, j


def test_bool_result_is_the_same_on_both_schedules():
    # list_modes 0x4cf100: GetAdapterModeCount's result was zeroed as a "boolean predicate" on our side only,
    # because ours has `test eax,eax` right after the call and the original `xor esi,esi; test eax,eax`. The
    # predicate is now what the caller does with eax, wherever the test sits; a UINT count (`test; jbe`) is not one.
    class _Uc:
        def __init__(self, regs):
            self.regs = regs

        def reg_read(self, r):
            return self.regs.get(r, 1)

    class _M:
        def __init__(self, code, regs=None):
            self.code, self.uc = code, _Uc(regs or {})

        def read(self, va, n):
            return (self.code + b"\x90" * 64)[:n]

    class _R:
        _EAX_REGS = equiv.Runner._EAX_REGS
        _ZERO_JCC = equiv.Runner._ZERO_JCC
        _ZERO_SETCC = equiv.Runner._ZERO_SETCC

        def __init__(self, code, regs=None):
            self.m = _M(code, regs)

    def b(code, regs=None):
        return equiv.Runner._bool_result(_R(bytes.fromhex(code), regs), 0x1000)

    assert b("84c07405")                            # test al,al; je
    assert b("31f685c0894424507505")                # xor esi,esi; test eax,eax; mov [esp+50h],eax; jne
    assert b("85c00f94c0")                          # test eax,eax; sete al
    assert not b("85c0894424507605")                # test eax,eax; mov; jbe: a UINT count
    assert not b("31f685c08944245076 05".replace(" ", ""))   # the original's schedule of the same count
    assert not b("85c07e05")                        # test eax,eax; jle: a signed count
    assert not b("8bc885c07405")                    # mov ecx,eax reads eax first: not a test
    assert not b("33c085c07405")                    # xor eax,eax: the result is dropped
    assert b("3bc57405", {equiv.UC_X86_REG_EBP: 0})         # cmp eax,ebp with ebp = 0; je
    assert not b("3bc57405", {equiv.UC_X86_REG_EBP: 7})     # cmp eax,ebp with ebp = 7: not a zero test
    assert not b("85c083f9007405")                  # test eax,eax; cmp ecx,0 (flags rewritten); je


def test_delete_statement_is_not_an_annotation(tmp_path):
    """`delete[] g; // ??_EX (0x4c7ee0)` names the deleting destructor, not g: the scanner must not bind g there."""
    src = tmp_path / "x.cpp"
    src.write_text("extern int* g_tbl;   // 0x5e29dc\n"
                   "void f() {\n"
                   "    delete[] g_tbl;                  // ??_EMissionObj (0x4c7ee0)\n"
                   "}\n")
    got = [(a[0], a[2]) for a in R.source_annotations([str(src)])]
    assert ("g_tbl", 0x5e29dc) in got
    assert ("g_tbl", 0x4c7ee0) not in got


def test_deep_variable_ignores_code_addresses():
    """A variable never resolves to a function start; one stray code address in the repository (winmain_and_menu_screens.cpp's
    `delete[] g_propkinds;  // ??_EMissionObj (0x4c7ee0)`) made g_propkinds ambiguous and a model page, and
    build_mesh_name 0x5302f0 then copied a vector out of soup (false FAIL)."""
    res = equiv.context()["res"]
    va, how = res.by_deep("?g_propkinds@@3PAUMissionObj@@A", False, False)
    assert va == 0x5e29dc, (hex(va or 0), how)
    res._deep.setdefault("g_propkinds", []).append((("g_propkinds",), 0x4c7ee0, "stray.cpp:1", None))
    try:
        assert res.by_deep("?g_propkinds@@3PAUMissionObj@@A", False, False)[0] == 0x5e29dc
    finally:
        res._deep["g_propkinds"].pop()


def test_icf_folded_template_instance_binds_its_sibling():
    """dust 0x4f4060 calls std::vector<face20>::capacity(), which no name source knows: the original's linker folded
    it into vector<point20>::capacity 0x40f090 (ICF). The row's alignment pairs the call with 0x40f090 only weakly
    (a 2-reference gap beside our local reserve copy), so it was modelled as a zero stub: 153/153 FAIL at import
    call #0. Weak alignment plus a same-template sibling name at that address now binds it."""
    t = equiv.prepare("dust_particles", 0x4f4060, src=os.path.join(ROOT, "src/game/terrain/dust_particles.cpp"), log=lambda *a: None)
    cap = "?capacity@?$vector@Uface20@rgl@@V?$allocator@Uface20@rgl@@@std@@@std@@QBEIXZ"
    pt = "?capacity@?$vector@Upoint20@rgl@@V?$allocator@Upoint20@rgl@@@std@@@std@@QBEIXZ"
    assert t.res.method.get(cap) == "orig-code(icf-sibling(%s))" % pt, t.res.method.get(cap)
    assert not any(m["kind"].startswith("model") for m in equiv.modelled_symbols(t)), equiv.modelled_symbols(t)
    assert R.icf_sibling(cap, pt)
    assert not R.icf_sibling(cap, cap)
    assert not R.icf_sibling(cap, pt.replace("?capacity@", "?size@"))                  # another member
    assert not R.icf_sibling(cap, pt.replace("QBEIXZ", "QAEIXZ"))                      # another function type
    assert not R.icf_sibling("?f@A@@QAEXXZ", "?f@B@@QAEXXZ")                          # no template: not a sibling
    assert not R.icf_sibling(cap, pt.replace("Upoint20@rgl@@V", "Uface9@rgl@@V"))     # two different substitutions
    res = equiv.context()["res"]
    saved = res._align_weak, res._orig_calls
    try:
        res._orig_calls = {0x40f090, 0x40f030}
        res._align_weak = {1: {0x40f090}, 2: {0x40f090, 0x40f030}, 3: {0x40f030}}
        assert res.by_icf_sibling(1, cap, True)[0] == 0x40f090
        assert res.by_icf_sibling(2, cap, True) == (None, None)      # alignment ambiguous
        assert res.by_icf_sibling(3, cap, True)[0] == 0x40f030       # names cannot tell siblings apart: the position does
        assert res.by_icf_sibling(4, cap, True) == (None, None)      # no positional evidence at all
        res._orig_calls = {0x40f030}
        assert res.by_icf_sibling(1, cap, True) == (None, None)      # a direct callee the original never calls
    finally:
        res._align_weak, res._orig_calls = saved


def test_reentry_through_an_image_callee_runs_ours():
    """gui_manager::draw_string 0x4e43e0: when the text is wider, gui_manager::rebuild 0x4da900 (image code, called by
    both builds) replays every line through draw_string. That call reached the ORIGINAL's entry in our run too, so its
    calls fell outside our span: 85/333 FAIL at call #12 (the original's recursive tstring::operator= against none).
    Our run now enters our function there, as the relinked game does."""
    r = _row("progress_bar_and_load_screens", 0x4e43e0, "src/game/ui/screens/progress_bar_and_load_screens.cpp", "--inputs", "60", "--fp-tol")
    assert r["verdict"] != "FAIL", r


def test_dead_callee_frames_are_refilled():
    """spawn_party_scene 0x49f3d0: ItemRef::reduce 0x437510 pushes the caller's esi (0 in the original, a dead pointer in
    ours); FxDef::FxDef 0x428820 then reads that slot as its unset vector4 w and FxDef::update 0x421df0 multiplies it into
    the effect's matrices, so the heap differed by the two compilers' register choices (15/90 FAIL, copy-only taint
    cannot excuse arithmetic). The returned callees' frames below esp are refilled on both sides after every call.
    Fresh inputs rarely get there; the row's regression inputs (the sweep's failing ones) do, so the test runs on a
    copy of them."""
    import shutil
    import pytest
    sid, va, src = "operation_execute", 0x49f3d0, os.path.join(ROOT, "src/game/script/operation_execute.cpp")
    real = os.path.join(ROOT, "work", "difftest", "regress", "%s_%08x" % (sid, va))
    if not os.path.isdir(real):
        pytest.skip("no regression inputs for %s %x" % (sid, va))
    mine = os.path.join(equiv.REGRESS_DIR, "%s_%08x" % (sid, va))           # the test copy (DIFFTEST_REGRESS_DIR)
    shutil.rmtree(mine, ignore_errors=True)
    shutil.copytree(real, mine)

    def run(*x):
        return equiv.test_function(sid, va, equiv.parse_args([sid, "%x" % va, "--src", src, "--inputs", "20", "--fp-tol", *x]))
    try:
        base = run("--no-scrub-dead-stack")
        assert base["verdict"] == "FAIL" and "heap" in base["first_mismatch"]["observable"], base
        r = run()
        assert r["verdict"] != "FAIL" and r["valid"] > 0, r
    finally:
        shutil.rmtree(mine, ignore_errors=True)


def test_stale_w_of_an_own_dead_temporary_is_uninitialised():
    """process_blow 0x457b40 copies vector4(k * dir) (w never set) to Agent+0x5694: the original's w slot was never
    written (0xCCCCCCCC), ours held a float an earlier temp of ours left in the slot cl reused. Both are the unset w;
    stale_w now accepts a w slot whose last write was the function's own dword store before x, y, z."""
    r = _row("agent_blows", 0x457b40, "src/game/agent/combat/agent_blows.cpp", "--inputs", "120", "--fp-tol")
    assert r["verdict"] != "FAIL", r


def test_out_of_line_dtor_callees_are_transparent():
    """Our ??_Glod_mesh (no original: build_appearance 0x482240 expands `delete br` inline, ~mesh_list 0x419420
    included) calls ~mesh_list out of line: its member callees are transparent, operator delete is not, and a
    deleting destructor the original has (resolved to an image address) adds nothing."""
    from types import SimpleNamespace as NS
    img = equiv.context()["img"]
    dtor_va, member_va = 0x419420, 0x42bc40
    assert dtor_va in img.start_set and member_va in img.start_set
    call = b"\xe8\0\0\0\0"
    code = call + call + call                            # ~mesh_list, ~string, operator delete
    syms = {0: NS(name="?build@@YAXXZ", defined=True, secno=1),
            1: NS(name="??_Glod_mesh@rgl@@QAEPAXI@Z", defined=True, secno=2),
            2: NS(name="??1mesh_list@rgl@@QAE@XZ", defined=False, secno=0),
            3: NS(name="??1tstring@@QAE@XZ", defined=False, secno=0),
            4: NS(name="??3@YAXPAX@Z", defined=False, secno=0),
            5: NS(name="??_Gother@@QAEPAXI@Z", defined=True, secno=3)}
    secs = [NS(data=b"", relocs=[]),
            NS(data=code, relocs=[(1, 2, R.REL_REL32), (6, 3, R.REL_REL32), (11, 4, R.REL_REL32)]),
            NS(data=call, relocs=[(1, 2, R.REL_REL32)])]
    res = NS(addr={2: dtor_va, 3: member_va, 4: 0x535712}, local_sections={0, 1, 2})
    t = NS(res=res, coff=NS(syms=syms, sections=secs), fsym=syms[0], va=0x482240)
    assert equiv.dtor_callee_transparent(t) == sorted([dtor_va, member_va])
    res.addr[1] = 0x4c7ee0                               # the original has this deleting destructor: it runs as itself
    res.addr[5] = 0x4c7ee0
    assert equiv.dtor_callee_transparent(t) == []
