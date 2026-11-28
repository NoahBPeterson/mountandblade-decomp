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
