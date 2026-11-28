"""Self-tests for equiv.py --snapshots (real game states as inputs; docs/snapshot_testing.md).

Run in its own process (the flag moves our compiled sections above the captured game heap, which machine.py reads at
import):  .venv/bin/python -m pytest tools/difftest/test_equiv_snapshots.py
Needs the captures in work/capture_2026-11-01 and work/capture_text (tools/hybrid/capture.py; 2.5 GB, not in git); the tests skip without them.

(a) the capture store reads exactly what a cumulative replay of the files holds;
(b) a snapshot input reproduces snapreplay's SAME on matched rows: every seed valid, no mismatch;
(c) a deliberately broken build (one constant of EmitTable::setup 0x4424c0) is a FAIL found by a snapshot input;
(d) mutants of a snapshot are ordinary inputs (pickle, replay, steering edits) and reach edges the seeds do not;
(e) captured memory the capture cannot give (harness regions, the null page) is not served;
(f) every session directory is read (one SnapStore per session, a snapshot's identity is {dir, seq}, a truncated file is skipped);
(g) branch-directed edits: related values come from live siblings in the same state and reach branches no seed does.
"""
import os, sys

os.environ.setdefault("DIFFTEST_OBJ_BASE", "0x38000000")
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
os.environ.setdefault("DIFFTEST_NO_LEDGER", "1")
os.environ.setdefault("DIFFTEST_NO_CACHE", "1")
os.environ.setdefault("DIFFTEST_FAIL_DIR", os.path.join(ROOT, "work", "difftest", "test_fail_inputs"))
os.environ.setdefault("DIFFTEST_REGRESS_DIR", os.path.join(ROOT, "work", "difftest", "test_regress"))
import pickle, random, struct  # noqa: E402
import pytest  # noqa: E402
import equiv  # noqa: E402
import machine as M  # noqa: E402
import snapreplay as SR  # noqa: E402

CAP = os.path.join(ROOT, "work", "capture_2026-11-01")   # the session that holds the matched rows below (work/capture is
#                                                        re-captured by other lanes: its function set changes)
CAP_TEXT = os.path.join(ROOT, "work", "capture_text")    # a second session that also captured Font::draw_glyph
pytestmark = [pytest.mark.skipif(M.OBJ_BASE < 0x38000000, reason="machine imported without DIFFTEST_OBJ_BASE: run this file alone"),
              pytest.mark.skipif(not os.path.isdir(CAP), reason="no captures in work/capture_2026-11-01")]
AGENT = "agent_logic"
AGENT_SRC = os.path.join(ROOT, "src", "game", "agent", "agent_logic.cpp")
GLYPH = 0x44f5d0            # Font::draw_glyph: byte-exact, 4 snapshots in CAP, 3 in CAP_TEXT
HIT_DAMAGE = 0x45c220        # a matched agent_logic row with 4 snapshots in CAP (the store / mutation tests use it)
EMIT_SETUP = 0x4424c0       # EmitTable::setup: byte-exact, 4 snapshots (writes the captured psys object)
MUT_DIR = os.path.join(ROOT, "work", "difftest", "mut")


def run_row(va, *extra, src=AGENT_SRC):
    o = equiv.parse_args([AGENT, "%x" % va, "--src", src, "--no-regress", "--snapshots", CAP, *extra])
    return equiv.test_function(AGENT, va, o)


def seeds_only(va, **kw):
    return run_row(va, "--inputs", "4", "--snap-share", "0", "--no-steer", "--plateau", "0", "--time-limit", "120")


# ---------------------------------------------------------------------- (a) the store

def test_walk_records_and_parse_agree():
    seq, va, path = SR.read_snapshots(CAP)[2]
    pva, ctx, pages = SR.parse(path)
    b = open(path, "rb").read()
    s2, v2, c2, recs = SR.walk_records(b)
    assert (s2, v2) == (seq, va) == (seq, pva) and c2 == ctx
    assert [(p, None if o is None else b[o:o + 0x1000]) for p, o in recs] == pages


def test_store_pages_equal_a_cumulative_replay():
    store = SR.SnapStore(CAP)
    seq = store.seqs(HIT_DAMAGE)[1]
    mem = {}
    for sq, _va, path in SR.read_snapshots(CAP):
        if sq > seq:
            break
        for p, d in SR.parse(path)[2]:
            mem[p] = d
    assert mem
    bad = [p for p, d in mem.items() if store.page(seq, p) != d]
    assert not bad, "%d of %d pages differ, e.g. %08x" % (len(bad), len(mem), bad[0])
    absent = next(p for p in range(0x7ff00000, 0x7ffff000, 0x1000) if p not in store.pages)
    assert not store.has(seq, absent)
    with pytest.raises(KeyError):
        store.page(seq, absent)
    # a page's newest version at or before seq: a later snapshot's version is not visible earlier
    later = store.seqs(HIT_DAMAGE)[-1]
    assert later > seq and store.ctx(later) != store.ctx(seq)


# ---------------------------------------------------------------------- (b) matched rows: snapshot inputs agree

@pytest.mark.parametrize("va", [GLYPH, HIT_DAMAGE])
def test_snapshot_inputs_reproduce_snapreplay_same(va):
    import argparse
    # snapreplay: the whole captured state, both builds, every snapshot of the row
    rp = SR.Replayer(argparse.Namespace(budget=20000000))
    want = set(SR.read_snapshots(CAP) and SR.SnapStore(CAP).seqs(va))
    last = max(want)
    verdicts = []
    for seq, v, path in SR.read_snapshots(CAP):
        if seq > last:
            break
        _v, ctx, pages = SR.parse(path)
        rp.apply(pages)
        if seq in want:
            r, why = rp.runner(va)
            assert r is not None, why
            verdicts.append(rp.compare(rp.run_side(r, r.orig, ctx), rp.run_side(r, r.ours, ctx))[0])
    assert verdicts == ["SAME"] * len(want), verdicts
    del rp
    # equiv.py: the same states as inputs, from the harness's own machine
    r = seeds_only(va)
    sn = r["snapshots"]
    assert sn["seeds"] == len(want) and sn["seeds_valid"] == len(want), sn
    assert r["mismatches"] == 0 and r["valid"] == len(want), r["reason"]
    assert r["edges"]["orig"]["covered"] >= 1 and r["edges"]["ours"]["covered"] >= 1


# ---------------------------------------------------------------------- (c) a broken build

def broken_source(old, new, tag):
    text = open(AGENT_SRC).read()
    assert text.count(old) == 1, old
    os.makedirs(MUT_DIR, exist_ok=True)
    path = os.path.join(MUT_DIR, "agent_logic_snap_%s.cpp" % tag)
    open(path, "w").write(text.replace(old, new, 1))
    return path


def test_broken_build_fails_from_a_snapshot_input():
    ok = run_row(EMIT_SETUP, "--inputs", "4", "--snap-share", "0", "--no-steer", "--plateau", "0", "--time-limit", "120")
    assert ok["verdict"] != "FAIL" and ok["mismatches"] == 0, ok["reason"]
    src = broken_source("vector4 g(0.0f, 0.0f, -9.8f);\n    e->accel = g;", "vector4 g(0.0f, 0.0f, -9.7f);\n    e->accel = g;",
                        "accel")
    bad = run_row(EMIT_SETUP, "--inputs", "4", "--snap-share", "0", "--no-steer", "--plateau", "0", "--time-limit", "120",
                  src=src)
    assert bad["verdict"] == "FAIL", bad["reason"]
    assert bad["first_mismatch"]["origin"] == "snapshot", bad["first_mismatch"]
    assert bad["snapshots"]["seeds_valid"] == bad["snapshots"]["seeds"]


# ---------------------------------------------------------------------- (d) mutants are ordinary inputs

@pytest.fixture()
def runner():
    o = equiv.parse_args([AGENT, "%x" % HIT_DAMAGE, "--src", AGENT_SRC, "--no-regress", "--snapshots", CAP, "--no-steer"])
    t = equiv.prepare(AGENT, HIT_DAMAGE, None, lambda *a, **k: None, AGENT_SRC)
    assert not t.verdict, t.reason
    r = equiv.Runner(t, o)
    assert r.snapin is not None, r.snap_why
    r.snapin.seeds(r)
    yield r
    r.close()


def test_seeds_carry_the_captured_call(runner):
    si = runner.snapin
    assert [b.snap["seq"] for b in si.bases] == si.seqs and len(si.seqs) == 4
    for b in si.bases:
        assert b.origin == "snapshot" and b.snap_cands and len(b.args) == len(runner.t.stack_kinds)
        assert b.regs["ecx"] >= 0x10000 and si.mappable(b.regs["ecx"] & ~0xFFF)       # `this` is a captured heap object
        kinds = {k for _a, _s, k in b.snap_cands}
        assert kinds <= {"flag", "int", "float", "ptr"}


def test_mutants_are_deterministic_edits_of_one_state(runner):
    si = runner.snapin
    p = si.bases[0]
    m1 = si.mutant(random.Random(5), p)
    m2 = si.mutant(random.Random(5), p)
    assert m1.snap == p.snap and m1.origin == "mutant"
    assert (m1.args, m1.regs, m1.overrides) == (m2.args, m2.regs, m2.overrides)
    assert any((x.args, x.regs, x.overrides) != (p.args, p.regs, p.overrides)
               for x in (si.mutant(random.Random(k), p) for k in range(20)))
    assert p.overrides == {}                                              # the parent is never edited in place
    for k in range(30):
        m = si.mutant(random.Random(k), p)
        for a4 in m.overrides:
            assert a4 & 3 == 0 and (si.mappable(a4 & ~0xFFF) or runner.img.in_image(a4))
    # the generator hands out snapshot mutants for fresh inputs, and mutate() of a snapshot input stays a snapshot input
    g = runner.gen
    assert g.snap is si
    assert g.mutate(p, random.Random(1), [p]).snap == p.snap


def test_snapshot_input_survives_pickle_and_reruns_identically(runner):
    si = runner.snapin
    inp = si.mutant(random.Random(3), si.bases[1])
    inp2 = equiv.Input()
    inp2.__dict__.update(pickle.loads(pickle.dumps(dict(inp.__dict__, pagegen=inp.pagegen))))
    a = runner.run_side(runner.orig, inp)
    b = runner.run_side(runner.orig, inp2)
    assert a["status"] == b["status"] and a["edges"] == b["edges"] and a["eax"] == b["eax"] and a["esp"] == b["esp"]
    assert a["mem"].get("extra") == b["mem"].get("extra") and a["mem"].keys() == b["mem"].keys()
    assert runner.snap_init == {} and runner.m.snap_fetch is None            # nothing stays mapped after a run


def test_steering_edit_of_captured_memory_is_an_override_the_run_sees(runner):
    import steer as ST
    si = runner.snapin
    st = ST.Steer(runner, runner.gen, runner.opts)
    base = si.bases[0]
    a4 = next(a for a, _s, k in base.snap_cands if k in ("int", "flag") and not runner.img.in_image(a))
    assert st.region(a4) == "snap" and st.dword(base, a4, "snap") == si.value(base, a4)
    new = st.set_loc(base, (a4, 4), 0x5a5a, "int")
    assert new is not None and new.overrides[a4] == 0x5a5a and base.overrides == {}
    assert st.dword(new, a4, "snap") == 0x5a5a and si.value(new, a4) == 0x5a5a
    # the page the run maps carries the edit, the other build sees the same bytes
    runner.snap_cur = {"seq": new.snap["seq"], "over": {a4 & ~0xFFF: {a4: 0x5a5a}}}
    try:
        assert runner._snap_fetch(a4, 4)
        assert struct.unpack("<I", runner.m.read(a4, 4))[0] == 0x5a5a
    finally:
        runner._snap_unmap()
        runner.snap_init, runner.snap_cur = {}, None
    # a byte-sized edit changes only its byte
    b1 = st.set_loc(base, (a4, 1), 0x7f, "int")
    assert b1.overrides[a4] & 0xFF == 0x7f and (b1.overrides[a4] >> 8) == (si.value(base, a4) >> 8)


def test_mutants_reach_edges_the_seeds_do_not():
    seeds = seeds_only(HIT_DAMAGE)
    more = run_row(HIT_DAMAGE, "--inputs", "40", "--snap-share", "1", "--no-steer", "--plateau", "0", "--time-limit", "120")
    assert more["mismatches"] == 0, more["reason"]
    assert more["snapshots"]["mutants_valid"] >= 10
    assert more["edges"]["orig"]["covered"] > seeds["edges"]["orig"]["covered"], (more["edges"]["orig"]["covered"],
                                                                                  seeds["edges"]["orig"]["covered"])
    assert more["edges"]["ours"]["covered"] > seeds["edges"]["ours"]["covered"]


def test_saved_snapshot_inputs_are_not_replayed_without_the_flag(tmp_path, monkeypatch):
    monkeypatch.setattr(equiv, "REGRESS_DIR", str(tmp_path))
    o_snap = equiv.parse_args([AGENT, "x", "--snapshots", CAP])
    o_plain = equiv.parse_args([AGENT, "x"])
    d = equiv.regress_dir(AGENT, HIT_DAMAGE, o_plain)
    os.makedirs(d)
    inp = equiv.Input()
    inp.__dict__.update(seed=1, profile="mixed", args=[], pad=[], pool_over={}, regs={}, xmm=[], overrides={}, touched=[],
                        snap={"dir": CAP, "seq": 431}, pagegen=2)
    pickle.dump(dict(inp.__dict__), open(os.path.join(d, "a.pkl"), "wb"))
    assert equiv.regress_inputs(AGENT, HIT_DAMAGE, o_plain) == []
    assert len(equiv.regress_inputs(AGENT, HIT_DAMAGE, o_snap)) == 1


# ---------------------------------------------------------------------- (e) what the capture cannot give

def test_harness_regions_and_the_null_page_are_not_served(runner):
    si = runner.snapin
    runner.snap_cur = {"seq": si.seqs[0], "over": {}}
    try:
        for addr in (0, 0x8000, M.OBJ_BASE, M.POOL_BASE + 0x1000, M.DYN_BASE, M.STACK_TOP - 0x1000, runner.img.base + 0x1000):
            assert not runner._snap_fetch(addr, 4), "%08x" % addr
        assert runner.snap_init == {}
        # a page of the captured heap is served, and a straddling access maps both pages
        a = runner.snapin.bases[0].regs["ecx"]
        assert runner._snap_fetch(a, 4)
        edge = (a & ~0xFFF) + 0xFFE
        runner._snap_unmap()
        runner.snap_init = {}
        if si.mappable((edge + 0x1000) & ~0xFFF):
            assert runner._snap_fetch(edge, 4) and (edge & ~0xFFF) in runner.snap_init and ((edge + 0x1000) & ~0xFFF) in runner.snap_init
    finally:
        runner._snap_unmap()
        runner.snap_init, runner.snap_cur = {}, None


# ---------------------------------------------------------------------- (f) every session directory

def run_sessions(va, spec, *extra, src=AGENT_SRC):
    o = equiv.parse_args([AGENT, "%x" % va, "--src", src, "--no-regress", "--snapshots", spec, *extra])
    return o, equiv.test_function(AGENT, va, o)


def test_session_dirs_lists_every_session_directory():
    allv = SR.session_dirs()
    assert CAP in allv and CAP_TEXT in allv
    assert all(glob_has_caps(d) for d in allv) and len(allv) == len(set(allv))
    assert SR.session_dirs("all") == allv == SR.session_dirs(None)
    sess = os.path.join(ROOT, "work", "capture_sessions")
    if os.path.isdir(sess):                         # the dated sessions are found (each holds its own snapshots)
        assert all(d in allv for d in (os.path.join(sess, x) for x in sorted(os.listdir(sess))) if glob_has_caps(d))
    assert SR.session_dirs(CAP) == [CAP]
    assert SR.session_dirs(CAP + os.pathsep + CAP_TEXT) == [CAP, CAP_TEXT]
    assert SR.session_dirs("/no/such/dir") == []


def glob_has_caps(d):
    return any(f.startswith("cap_") and f.endswith(".bin") for f in os.listdir(d))


def test_truncated_capture_file_is_skipped_not_fatal(tmp_path):
    files = SR.read_snapshots(CAP_TEXT)[:3]
    for _s, _v, f in files[:2]:
        os.symlink(f, str(tmp_path / os.path.basename(f)))
    cut = str(tmp_path / os.path.basename(files[2][2]))                 # the game was killed while writing this one
    data = open(files[2][2], "rb").read()
    open(cut, "wb").write(data[:len(data) // 2])
    st = SR.SnapStore(str(tmp_path))
    assert sorted(st.vas) == [files[0][0], files[1][0]]                 # only the complete files are snapshots
    assert os.path.exists(cut) and os.path.getsize(cut) == len(data) // 2


def test_multi_session_seeds_carry_their_session():
    n_text, n_cap = len(SR.SnapStore(CAP_TEXT).seqs(GLYPH)), len(SR.SnapStore(CAP).seqs(GLYPH))
    assert n_text and n_cap
    o, r = run_sessions(GLYPH, CAP + os.pathsep + CAP_TEXT, "--inputs", str(n_text + n_cap), "--snap-share", "0", "--no-steer",
                        "--plateau", "0", "--time-limit", "120")
    sn = r["snapshots"]
    assert sn["seeds"] == n_text + n_cap and sn["seeds_valid"] == sn["seeds"], sn
    assert sn["sessions"] == ["capture_2026-11-01", "capture_text"]
    assert sorted(sn["seed_ids"]) == sorted(["capture_2026-11-01:%d" % q for q in SR.SnapStore(CAP).seqs(GLYPH)] +
                                            ["capture_text:%d" % q for q in SR.SnapStore(CAP_TEXT).seqs(GLYPH)])
    assert r["mismatches"] == 0, r["reason"]
    # the cache key covers every session that holds the row (a new session re-runs it)
    assert [x[0] for x in o.snap_sig] == ["capture_2026-11-01", "capture_text"]


def test_multi_session_state_is_read_from_its_own_store():
    t = equiv.prepare(AGENT, GLYPH, None, lambda *a, **k: None, AGENT_SRC)
    o = equiv.parse_args([AGENT, "%x" % GLYPH, "--src", AGENT_SRC, "--no-regress", "--snapshots", CAP + os.pathsep + CAP_TEXT])
    r = equiv.Runner(t, o)
    try:
        si = r.snapin
        si.seeds(r)
        assert len(si.bases) == len(si.ids) and len(si.stores) == 2
        for b in si.bases:
            st = si.stores[b.snap["dir"]]
            assert b.snap["seq"] in st.seqs(GLYPH)
            ecx = b.regs["ecx"]
            want = st.page(b.snap["seq"], ecx & ~0xFFF)
            want = bytes(0x1000) if want is None else want
            assert si.read(b.snap, ecx, 4) == want[ecx & 0xFFF:(ecx & 0xFFF) + 4]
        # a mutant keeps its session; two states of different sessions are different identities
        m = si.mutant(random.Random(1), si.bases[-1])
        assert m.snap == si.bases[-1].snap and si.bases[0].snap != si.bases[-1].snap
        assert si.bases[0].snap["dir"] != si.bases[-1].snap["dir"]
    finally:
        r.close()


# ---------------------------------------------------------------------- (g) branch-directed, state-consistent edits

def test_related_values_come_from_live_siblings_in_the_same_state(runner):
    si = runner.snapin
    found = 0
    for b in si.bases:
        for a4, _s, kind in b.snap_cands:
            c = si.container(b, a4)
            if c is None:
                continue
            base, vt = c
            assert base < a4 < base + si.OBJ_SPAN and si.value(b, base) == vt
            sibs = si.siblings(b, vt, base)
            assert base not in sibs and all(si.value(b, q) == vt for q in sibs)
            rel = si.related(b, a4, random.Random(2))
            cur = si.value(b, a4)
            assert cur not in rel
            allowed = {si.value(b, q + a4 - base) for q in sibs} | {cur - 1}       # the same field of a sibling; a count - 1
            if si.mappable(cur & ~0xFFF) and cur & 3 == 0:                          # a pointer: objects of the pointee's class
                vt2 = si.value(b, cur)
                allowed |= set(si.siblings(b, vt2, cur))
            assert set(rel) <= allowed, (hex(a4), rel, allowed)
            assert si.related(b, a4, random.Random(2)) == rel             # deterministic
            found += bool(rel)
    assert found, "no snapshot field of the row has a related value: pick a row whose state holds several live objects"
    si.use_related = False
    try:
        assert si.related(si.bases[0], si.bases[0].snap_cands[0][0]) == []
    finally:
        si.use_related = True


DIRECTED_ROW = 0x4465e0     # Agent::... (agent_logic): 4 snapshots in CAP; a related-value edit reaches branches no seed does


@pytest.fixture()
def directed_runner():
    o = equiv.parse_args([AGENT, "%x" % DIRECTED_ROW, "--src", AGENT_SRC, "--no-regress", "--snapshots", CAP, "--no-steer"])
    t = equiv.prepare(AGENT, DIRECTED_ROW, None, lambda *a, **k: None, AGENT_SRC)
    assert not t.verdict, t.reason
    r = equiv.Runner(t, o)
    assert r.snapin is not None, r.snap_why
    r.snapin.seeds(r)
    yield r
    r.close()


def test_a_branch_reached_only_through_a_related_value_edit(directed_runner):
    r = directed_runner
    si = r.snapin
    seed_edges = set()
    for b in si.bases:
        seed_edges |= set(r.run_side(r.orig, b)["edges"])
    won = {}
    for b in si.bases:
        for a4, size, _k in b.snap_cands:
            if size != 4:
                continue
            for v in si.related(b, a4, random.Random(1))[:4]:
                c = b.copy()
                c.overrides[a4] = v
                new = set(r.run_side(r.orig, c)["edges"]) - seed_edges
                for e in new:
                    won.setdefault(e, (b, a4, v, c))
    assert won, "no related-value edit reaches an edge the seeds miss"
    b, a4, v, c = next(iter(won.values()))
    # the value is the one a live record of the same class holds in the same state (or a count one lower), nothing invented
    cont = si.container(b, a4)
    cur = si.value(b, a4)
    held = {si.value(b, q + a4 - cont[0]) for q in si.siblings(b, cont[1], cont[0])} if cont else set()
    assert v in held or v == cur - 1 or v in set(si.siblings(b, si.value(b, cur), cur)), (hex(a4), v)
    # the edit is an ordinary input: both builds run it from the same state, and they agree
    ra, rb = r.run_side(r.orig, c), r.run_side(r.ours, c)
    assert ra["status"] == rb["status"]
    # without related values the same single edit is not offered (A/B switch), so these edges need the directed draw
    si.use_related = False
    try:
        assert si.related(b, a4, random.Random(1)) == []
    finally:
        si.use_related = True


def test_steering_offers_related_values_for_a_snapshot_field(directed_runner):
    import steer as ST
    r = directed_runner
    si = r.snapin
    st = ST.Steer(r, r.gen, r.opts)
    b = next(b for b in si.bases if any(si.related(b, a4) for a4, s, _k in b.snap_cands if s == 4))
    a4 = next(a4 for a4, s, _k in b.snap_cands if s == 4 and si.related(b, a4))
    out = st._snap_directed(b, [(a4, 4)], "int", random.Random(3))
    assert out and all(c.snap == b.snap and c.overrides.get(a4) in si.related(b, a4) for c in out)
    assert b.overrides == {}
    assert st._snap_directed(b, [("arg", 0)], "int", random.Random(3)) == []                 # only captured memory
    assert st._snap_directed(b, [(a4, 4)], "float", random.Random(3)) == []
    out2 = st._snap_directed(b, [(a4, 4)], "int", random.Random(3))
    assert [c.overrides for c in out] == [c.overrides for c in out2]                         # deterministic
