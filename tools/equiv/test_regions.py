"""Tests of the region mode of the angr layer (tools/equiv/regions.py) on synthetic pairs: a switch dispatch with
one case body that differs, a loop whose registers are renamed (lock-step under a register correspondence), and
what must not prove: a wrong case body, a wrong table, the mutants of a proof, wrong hints. No game image is needed
(the image hooks are replaced by a fake, as in test_angr_layer.py). Needs work/angr-venv; skipped otherwise.

  uv run pytest tools/equiv/test_regions.py -q
"""
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import angr_layer as AL  # noqa: E402
import func as FN  # noqa: E402
import mutate  # noqa: E402
import regions as RG  # noqa: E402
from func import Func  # noqa: E402
from test_angr_layer import FakeImage  # noqa: E402

H = bytes.fromhex
pytestmark = pytest.mark.skipif(AL._angr_python() is None, reason="angr venv missing (see angr_layer.py)")
RVA = 0x1000
TMO = 120
INT1 = "?f@@YAHH@Z"                     # int f(int): eax is the return value
TABLE = 0x405000


@pytest.fixture(autouse=True)
def fake_image(monkeypatch):
    img = FakeImage()
    monkeypatch.setattr(FN, "_img", img)
    RG._CACHE.clear()
    return img


def R(code, relocs=None, tables=None):
    return Func(RVA, code, relocs or {}, "retail", "retail", {"tables": tables})


def O(code, relocs=None, tables=None, symbol=INT1):
    return Func(0, code, relocs or {}, "ours", "ours", {"symbol": symbol, "consts": {}, "tables": tables})


def prove(a, b, level="L1", hints=None):
    r = RG.prove(a, b, level, hints, TMO, per_region=60)
    print(r["status"], r["detail"][:400])
    return r


# switch: mov eax,[esp+4]; cmp eax,1; ja default; jmp [eax*4+T]; case0: mov eax,10; ret; case1: ...; default: xor eax,
# eax; ret. Case 1 of the original doubles its second argument with add, ours with lea from another register.
HEAD = H("8b442404" "83f801")
DISPATCH_R = H("ff2485") + TABLE.to_bytes(4, "little")
DISPATCH_O = H("ff2485" "00000000")
CASE0 = H("b80a000000" "c3")
CASE1_R = H("8b442408" "01c0" "c3")                 # mov eax,[esp+8]; add eax,eax; ret
CASE1_O = H("8b4c2408" "8d0409" "c3")               # mov ecx,[esp+8]; lea eax,[ecx+ecx]; ret
CASE1_BAD = H("8b4c2408" "8d440901" "c3")           # lea eax,[ecx+ecx+1]
DEFAULT = H("31c0" "c3")


def switch_pair(case1_o=CASE1_O, swap=False):
    r = HEAD + H("77") + bytes([0x1d - 0x09]) + DISPATCH_R + CASE0 + CASE1_R + DEFAULT
    c1 = 0x16
    dflt = c1 + len(case1_o)
    o = HEAD + H("77") + bytes([dflt - 0x09]) + DISPATCH_O + CASE0 + case1_o + DEFAULT
    rt = {RVA + 9: [RVA + 0x10, RVA + 0x16]}
    ot = {9: [0x16, 0x10] if swap else [0x10, 0x16]}
    return (R(r, {12: ("abs", TABLE)}, rt),
            O(o, {12: ("sym", "$L1#3+40", 6, 0)}, ot))


def test_switch_case_proved():
    a, b = switch_pair()
    r = prove(a, b)
    assert r["status"] == "proved"
    assert "1 switch tables" in r["detail"] and r["covered"]
    regs = r["info"]["regions"]
    assert any(k.startswith("16,16,") for k in regs)          # the differing case got an angr region


def test_switch_wrong_case_not_proved():
    a, b = switch_pair(CASE1_BAD)
    r = prove(a, b)
    assert r["status"] == "unknown"                           # a region never refutes


def test_switch_wrong_table_not_proved():
    a, b = switch_pair(swap=True)
    assert prove(a, b)["status"] == "unknown"


def test_switch_mutants_rejected():
    a, b = switch_pair()
    r = prove(a, b)
    assert r["status"] == "proved"
    ms = list(mutate.mutants(b, 6, seed=1, covered=r["covered"]))
    assert ms
    for desc, m in ms:
        assert prove(a, m)["status"] != "proved", desc


def test_switch_hints_recheck():
    a, b = switch_pair()
    r = prove(a, b)
    assert r["status"] == "proved" and r["hints"]["mode"] == "region"
    RG._CACHE.clear()
    assert prove(a, b, hints=r["hints"])["status"] == "proved"
    bad = dict(r["hints"], regions={})
    RG._CACHE.clear()
    assert prove(a, b, hints=bad)["status"] == "unknown"


# a loop whose counter and pointer registers are swapped (n in ecx / edx, p in edx / ecx): the whole-function angr
# layer cannot bound it (the trip count is an input); lock-step under the correspondence ecx<->edx goes around it
LOOP_R = H("8b4c2404" "8b542408" "31c0" "85c9" "7e08" "0302" "83c204" "49" "75f8" "c3")
LOOP_O = H("8b542404" "8b4c2408" "31c0" "85d2" "7e08" "0301" "83c104" "4a" "75f8" "c3")
LOOP_BAD = H("8b542404" "8b4c2408" "31c0" "85d2" "7e08" "0301" "83c108" "4a" "75f8" "c3")   # add ecx, 8


def test_renamed_loop_proved():
    r = prove(R(LOOP_R), O(LOOP_O, symbol="?f@@YAHHPAH@Z"))
    assert r["status"] == "proved"


def test_renamed_loop_wrong_stride_not_proved():
    assert prove(R(LOOP_R), O(LOOP_BAD, symbol="?f@@YAHHPAH@Z"))["status"] == "unknown"


def test_renamed_loop_edx_compared_for_unknown_return():
    # no decorated name: the return may be edx:eax, and edx differs (p + 4n against 0)
    assert prove(R(LOOP_R), O(LOOP_O, symbol=""))["status"] == "unknown"


def test_lockstep_register_correspondence():
    # mov eax,[esp+4] / mov ecx,[esp+4] is a renaming; add eax,1 / add ecx,1 follows it; ret needs eax itself
    ra = R(H("8b442404" "83c001" "c3"))
    ob = O(H("8b4c2404" "83c101" "c3"))
    R_, O_, D = RG.decompose(ra, ob)
    assert D.diverge                                          # ret: ours' eax is not the original's eax
    r = prove(ra, ob)
    assert r["status"] == "unknown"


def test_fold_only_for_calls():
    bij = RG.Bijection(lambda k, v, b: True)
    assert bij.check(("sym", "_a", 0x14, 0), 0x2000, [])[0]
    bij.commit([(("sym", "_a", 0), 0x2000, False, 1, 1)])
    ok, fold = bij.check(("sym", "_b", 0x14, 0), 0x2000, [])
    assert ok and fold                                        # a second callee folded onto 0x2000
    assert not bij.check(("sym", "_g", 0x06, 0), 0x2000, [])[0]     # data is never folded
    assert not RG.Bijection().check(("sym", "_b", 0x14, 0), 0x2000,
                                    [(("sym", "_a", 0), 0x2000, False, 1, 1)])[0]


# a chain of independent if/else diamonds; ours uses ecx where the original uses edx and lays out each diamond the
# other way round (jne to the then-arm, the else-arm first). 2^13 paths through the stretch: one angr region over it
# explodes; cut at the joins, each region has two paths.
def _diamond_r(k, d_delta=0):
    bit, c, d = (1 << k), k + 1, 2 * k + 3
    return (H("8b542404" "f7c2") + bit.to_bytes(4, "little") + H("7407" "05") + c.to_bytes(4, "little") +
            H("eb05" "2d") + d.to_bytes(4, "little"))


def _diamond_o(k, d_delta=0):
    bit, c, d = (1 << k), k + 1, 2 * k + 3 + d_delta
    return (H("8b4c2404" "f7c1") + bit.to_bytes(4, "little") + H("7507" "2d") + d.to_bytes(4, "little") +
            H("eb05" "05") + c.to_bytes(4, "little"))


CHAIN_N = 13


def chain_pair(bad=None):
    r = H("31c0") + b"".join(_diamond_r(k) for k in range(CHAIN_N)) + H("c3")
    o = H("31c0") + b"".join(_diamond_o(k, 1 if k == bad else 0) for k in range(CHAIN_N)) + H("c3")
    return R(r), O(o, symbol="?f@@YAHH@Z")


def test_diamond_chain_fine_cuts():
    import time
    a, b = chain_pair()
    t0 = time.time()
    r = prove(a, b)
    assert r["status"] == "proved"
    assert r["info"]["regions"] and len(r["info"]["regions"]) >= CHAIN_N
    assert all(v[0] == "proved" for v in r["info"]["regions"].values())
    assert time.time() - t0 < 120
    # every region a diamond: two paths each (none over the whole stretch)
    assert all("retail 2 paths" in v[1] for v in r["info"]["regions"].values()), r["info"]["regions"]


def test_diamond_chain_wrong_value_not_proved():
    a, b = chain_pair(bad=7)
    assert prove(a, b)["status"] == "unknown"


def test_diamond_chain_mutants_rejected():
    a, b = chain_pair()
    r = prove(a, b)
    assert r["status"] == "proved"
    ms = list(mutate.mutants(b, 6, seed=3, covered=r["covered"]))
    assert ms
    for desc, m in ms:
        assert prove(a, m)["status"] != "proved", desc


def test_diamond_chain_hints_recheck():
    a, b = chain_pair()
    r = prove(a, b)
    assert r["status"] == "proved"
    RG._CACHE.clear()
    assert prove(a, b, hints=r["hints"])["status"] == "proved"


# a bool in al over an upper part that differs (cl loads al into a register whose upper bytes hold garbage), live
# across a join: the region before the join can relate only the low byte of eax ("eax:8"), which is all the code
# after it reads (test al, al); then eax is written in full again
AL_R = H("8b442408" "8a442404" "8b54240c" "f7c201000000" "7405" "83c203" "eb03" "83ea07"
         "84c0" "7403" "89d0" "c3" "31c0" "c3")
AL_O = H("8b44240c" "8a442404" "8b4c240c" "f7c101000000" "7505" "83e907" "eb03" "83c103"
         "84c0" "7403" "89c8" "c3" "31c0" "c3")
AL_BAD = H("8b44240c" "8a442408" "8b4c240c" "f7c101000000" "7505" "83e907" "eb03" "83c103"
           "84c0" "7403" "89c8" "c3" "31c0" "c3")                # mov al, [esp+8]


def test_partial_register_relation_across_join():
    r = prove(R(AL_R), O(AL_O, symbol="?f@@YAHHHH@Z"))
    assert r["status"] == "proved"
    assert RG.LAST["D"].S and any("eax:8" in (v or "") for k in RG.LAST["D"].S for v in k[2])


def test_partial_register_relation_wrong_byte_not_proved():
    assert prove(R(AL_R), O(AL_BAD, symbol="?f@@YAHHHH@Z"))["status"] == "unknown"


def test_partial_writes():
    pi = RG.pi_drop(RG.PI0, "eax", "eax")
    assert RG.pi_get(RG._apply_writes(pi, {}, set(), {"eax"}, {"eax": [(0, 8)]}), "eax") == "eax:8"
    assert RG.pi_get(RG._apply_writes(pi, {}, set(), {"eax"}, {"eax": [(8, 16)]}), "eax") is None
    p8 = RG.pi_set(pi, "eax", "eax:8")
    assert RG.pi_get(RG._apply_writes(p8, {}, set(), {"eax"}, {"eax": [(8, 16)]}), "eax") == "eax:16"
    assert RG.pi_get(RG._apply_writes(RG.PI0, {}, set(), {"eax"}, {"eax": [(0, 8)]}), "eax") == "eax"


# code moved across a join: the original loads edx before the diamond, ours after it. The finest region stops at
# the joins, where edx is unrelated (ours has not loaded it yet), and the region after the join fails; the
# backtrack reruns the region before it coarser (to the return), which proves.
MOVED_R = H("31c0" "8b542408" "8b4c2404" "f7c101000000" "7405" "83c003" "eb03" "83e807" "01d0" "c3")
MOVED_O = H("31c0" "8b4c2404" "f7c101000000" "7505" "83e807" "eb03" "83c003" "8b542408" "01d0" "c3")
MOVED_BAD = H("31c0" "8b4c2404" "f7c101000000" "7505" "83e807" "eb03" "83c003" "8b54240c" "01d0" "c3")


def test_backtrack_to_coarser_cut():
    r = prove(R(MOVED_R), O(MOVED_O, symbol="?f@@YAHHH@Z"))
    assert r["status"] == "proved"
    assert any(v[3] == "none" for v in r["info"]["regions"].values())
    RG._CACHE.clear()
    assert prove(R(MOVED_R), O(MOVED_O, symbol="?f@@YAHHH@Z"), hints=r["hints"])["status"] == "proved"


def test_backtrack_wrong_value_not_proved():
    assert prove(R(MOVED_R), O(MOVED_BAD, symbol="?f@@YAHHH@Z"))["status"] == "unknown"
