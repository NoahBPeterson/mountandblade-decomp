"""EQUIV_FRAME_OUP=1 (assumption O-up: a callee given a frame address reaches only the frame at or above the lowest
address the function takes). A spill below every taken address is no object: it may exist in one version only.
Mutants of the spill (wrong value, reloaded from the object, the object's contents) stay unproved."""
import os
import struct
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import angr_layer as AL  # noqa: E402
import func as FN  # noqa: E402
import regions as RG  # noqa: E402
from func import Func  # noqa: E402
from test_angr_layer import FakeImage  # noqa: E402

H = bytes.fromhex
pytestmark = pytest.mark.skipif(AL._angr_python() is None, reason="angr venv missing (see angr_layer.py)")
RVA = 0x1000


@pytest.fixture(autouse=True)
def env(monkeypatch):
    monkeypatch.setattr(FN, "_img", FakeImage())
    monkeypatch.setenv("EQUIV_FRAME_OUP", "1")
    RG._CACHE.clear()


def _call_g(at):
    return H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + at + 5)), at + 1


def _pair(reload="spill", store_x1=False):
    # retail keeps x in esi across the call (object at E-8); ours spills it below the object (object at E-8 too,
    # a bigger frame) and reloads it after the call
    pre_r = H("56" "8b742408" "83ec04" "893424" "8d0c24" "51")
    pre_o = H("8b442404" "83ec0c" "89442404") + (H("40") if store_x1 else b"") + H("890424" "8d4c2404" "51")
    cr, rr = _call_g(len(pre_r))
    co, ro = _call_g(len(pre_o))
    tail_r = H("83c404" "89f0" "83c404" "5e" "c3")
    tail_o = H("83c404") + H({"spill": "8b0424", "obj": "8b442404"}[reload]) + H("83c40c" "c3")
    a = Func(RVA, pre_r + cr + tail_r, {rr: ("abs", RVA + 0x5000)}, "retail", "retail", {"tables": None})
    b = Func(0, pre_o + co + tail_o, {ro: ("sym", "_g", 0x14, 0)}, "ours", "ours",
             {"symbol": "?f@@YAHH@Z", "consts": {}, "tables": None})
    return a, b


def _prove(a, b):
    r = RG.prove(a, b, "L1", None, 120, per_region=60, frame_model=True)
    print(r["status"], r["detail"][:300])
    return r["status"]


def test_spill_below_every_taken_address_proves():
    assert _prove(*_pair()) == "proved"


def test_without_oup_the_spill_difference_is_not_proved(monkeypatch):
    monkeypatch.delenv("EQUIV_FRAME_OUP")
    assert _prove(*_pair()) != "proved"


def test_wrong_spilled_value_not_proved():
    assert _prove(*_pair(store_x1=True)) != "proved"


def test_spill_reloaded_from_the_object_not_proved():
    # the object (passed to g) may have been written by g: reloading x from it is not x
    assert _prove(*_pair(reload="obj")) != "proved"


# the same, but ours' frame is 4 bytes smaller (retail pushes esi): every object sits 4 bytes closer to E in ours
# (one uniform shift), and the frame is read after the call (ours reloads its spill): the correlated havoc
def _pair_shift(reload="spill", store_x1=False, obj_x1=False):
    pre_r = H("56" "8b742408" "83ec08" "89742404" "8d4c2404" "51")
    pre_o = H("8b442404" "83ec08") + (H("40") if obj_x1 else b"") + H("89442404") + (H("40") if store_x1 else b"") + \
        H("890424" "8d4c2404" "51")
    cr, rr = _call_g(len(pre_r))
    co, ro = _call_g(len(pre_o))
    tail_r = H("83c404" "89f0" "83c408" "5e" "c3")
    tail_o = H("83c404") + H({"spill": "8b0424", "obj": "8b442404"}[reload]) + H("83c408" "c3")
    a = Func(RVA, pre_r + cr + tail_r, {rr: ("abs", RVA + 0x5000)}, "retail", "retail", {"tables": None})
    b = Func(0, pre_o + co + tail_o, {ro: ("sym", "_g", 0x14, 0)}, "ours", "ours",
             {"symbol": "?f@@YAHH@Z", "consts": {}, "tables": None})
    return a, b


def test_uniform_shift_with_frame_read_after_the_call_proves():
    assert _prove(*_pair_shift()) == "proved"


def test_uniform_shift_without_oup_not_proved(monkeypatch):
    monkeypatch.delenv("EQUIV_FRAME_OUP")
    assert _prove(*_pair_shift()) != "proved"


def test_uniform_shift_wrong_spill_not_proved():
    assert _prove(*_pair_shift(store_x1=True)) != "proved"


def test_uniform_shift_spill_reloaded_from_the_object_not_proved():
    assert _prove(*_pair_shift(reload="obj")) != "proved"


def test_uniform_shift_object_contents_differ_not_proved():
    assert _prove(*_pair_shift(obj_x1=True)) != "proved"


# a cdecl call whose cleanup is merged into the next call's: f(x); g(x+1); add esp,8. The second call's cleanup
# covers the first call's argument word too; the model takes the second call's OWN arguments (the pushes since the
# first returned). ours computes x+1 by lea. Mutants: x+2; a different first argument.
def _merged(bump="40", second="8d4001", first_arg="50"):
    def build(inc, lea):
        c = H("8b442404") + H(first_arg)                     # eax = x; push eax
        return c
    pre_r = H("8b442404" "50")
    cr1, rr1 = _call_g(len(pre_r))
    mid_r = H("8b442408" "40" "50")                           # eax = x (now at [esp+8]); inc eax; push eax
    cr2, rr2 = _call_g(len(pre_r) + len(cr1) + len(mid_r))
    tail = H("83c408" "31c0" "c3")
    a = Func(RVA, pre_r + cr1 + mid_r + cr2 + tail, {rr1: ("abs", RVA + 0x5000), rr2: ("abs", RVA + 0x5000)},
             "retail", "retail", {"tables": None})
    pre_o = H("8b442404") + H(first_arg)
    co1, ro1 = _call_g(len(pre_o))
    mid_o = H("8b442408") + H(second) + H("50")
    co2, ro2 = _call_g(len(pre_o) + len(co1) + len(mid_o))
    b = Func(0, pre_o + co1 + mid_o + co2 + tail, {ro1: ("sym", "_g", 0x14, 0), ro2: ("sym", "_g", 0x14, 0)}, "ours",
             "ours", {"symbol": "?f@@YAHH@Z", "consts": {}, "tables": None})
    return a, b


def test_merged_cleanup_calls_prove():
    assert _prove(*_merged()) == "proved"


def test_merged_cleanup_second_argument_mutant_not_proved():
    assert _prove(*_merged(second="8d4002")) != "proved"


def test_merged_cleanup_first_argument_mutant_not_proved():
    assert _prove(*_merged(first_arg="6a00")) != "proved"


# the two versions save DIFFERENT callee-saved registers (retail keeps the argument in ebx, ours in esi): at the join
# each version's saved word holds the OTHER version's untouched register (slot <-> register relations of F', and the
# pops at the return restore them). Mutants: ours restores a register it did not save (pop edi); ours does not restore.
def _saves(ours_pop="5e", ours_push="56"):
    pre_r = H("53" "8b5c2408" "85db" "740a" "6a00")
    cr, rr = _call_g(len(pre_r))
    tail_r = H("83c404" "89d8" "5b" "c3")
    a = Func(RVA, pre_r + cr + tail_r, {rr: ("abs", RVA + 0x5000)}, "retail", "retail", {"tables": None})
    pre_o = H(ours_push) + H("8b742408" "85f6" "740a" "6a00")
    co, ro = _call_g(len(pre_o))
    tail_o = H("83c404" "89f0") + H(ours_pop) + H("c3")
    b = Func(0, pre_o + co + tail_o, {ro: ("sym", "_g", 0x14, 0)}, "ours", "ours",
             {"symbol": "?f@@YAHH@Z", "consts": {}, "tables": None})
    return a, b


def test_different_callee_saved_registers_prove():
    assert _prove(*_saves()) == "proved"


def test_restoring_the_wrong_register_not_proved():
    assert _prove(*_saves(ours_pop="5f")) != "proved"          # pop edi: edi was never saved


def test_not_restoring_not_proved():
    assert _prove(*_saves(ours_pop="90")) != "proved"


# prove.py's own self-test (mutate.py mutants of ours, fast and full runs) under the frame model with the O-up flag:
# the pair is EQUIVALENT-O only after the unconditional layers failed, and every mutant must stay unproved.
def _prove_py(a, b, k=6):
    import prove as PV
    b.meta.update(src="", flags=[])
    return PV.run_funcs(a, b, "L1", k, True, lambda s: None, {"region": 120.0}, ["region"], match_shortcut=False,
                        frame_model=True)


def _selftest(pair):
    a, b = pair
    r = _prove_py(a, b)
    print(r["verdict"], r.get("detail", "")[:300], r.get("mutants"))
    assert r["verdict"] == "EQUIVALENT-O", r.get("detail")
    assert r["mutants"] and all(t[1] != "proved" for t in r["mutants"]), r["mutants"]
    return r


def test_selftest_spill_below_objects():
    _selftest(_pair())


def test_selftest_uniform_shift_read_after_call():
    _selftest(_pair_shift())


def test_selftest_different_callee_saved_registers():
    _selftest(_saves())
