"""Executable spec for the per-object frame model (tools/equiv/LAYERS.md, "The per-object frame model").

The region interface R requires esp equal and all memory at and above the cut's esp equal, so frame LAYOUT
differences (a local in a different slot, a different frame size, push vs mov argument passing) fail a region even
between equivalent functions. The frame model extends R with a per-object frame correspondence F; this file is the
battery it must satisfy.

The "must NOT prove" cases are plain asserts: they hold today (a region never refutes, and a real difference keeps
it unknown) and guard against a frame model that proves too much. The "must prove" cases are xfail until the model
lands (LAYERS.md "Status"): flip `FRAME_MODEL = True` when regions.py carries F through R, the cut key, the angr
region worker's entry state / exit comparison and the hints.

  uv run pytest tools/equiv/test_frame.py -q          # not-proved asserts pass; proves are xfail
  FRAME_MODEL=1 uv run pytest tools/equiv/test_frame.py -q
"""
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
FRAME_MODEL = os.environ.get("FRAME_MODEL") == "1"
proves = pytest.mark.xfail(not FRAME_MODEL, reason="per-object frame model not yet implemented (LAYERS.md)",
                           strict=False)
RVA = 0x1000


@pytest.fixture(autouse=True)
def fake_image(monkeypatch):
    monkeypatch.setattr(FN, "_img", FakeImage())
    RG._CACHE.clear()


def R(code, relocs=None, tables=None):
    return Func(RVA, code, relocs or {}, "retail", "retail", {"tables": tables})


def O(code, relocs=None, tables=None, symbol="?f@@YAHH@Z"):
    return Func(0, code, relocs or {}, "ours", "ours", {"symbol": symbol, "consts": {}, "tables": tables})


def prove(a, b, level="L1"):
    r = RG.prove(a, b, level, None, 120, per_region=60)
    print(r["status"], r["detail"][:300])
    return r


# --------------------------------------------------------------------------- two locals swapped between slots
# int f(int x){ int a=x+1, b=x+2; g(&a,&b); return a-b; }  retail puts a at [esp+0], b at [esp+4]; ours the other
# way round. Nothing is read from the frame after the call except a and b, which correspond across the swap.
def _swap(swap_ours, bad=False):
    # retail: sub esp,8; mov eax,[esp+0xc]; lea ecx,[eax+1]; mov [esp],ecx; lea edx,[eax+2]; mov [esp+4],edx;
    #         push esp+4-rel... (kept simple: call g(); here modelled as mov eax,[esp]; sub eax,[esp+4]); add esp,8; ret
    r = H("83ec08" "8b44240c" "8d4801" "890c24" "8d5002" "89542404" "8b0424" "2b442404" "83c408" "c3")
    if not swap_ours:
        return R(r), O(r)
    # ours: a at [esp+4], b at [esp]
    o = H("83ec08" "8b44240c" "8d4801" "894c2404" "8d5002" "891424" "8b442404" "2b0424" "83c408" "c3")
    if bad:                                                   # return b-a instead of a-b (a real difference)
        o = H("83ec08" "8b44240c" "8d4801" "894c2404" "8d5002" "891424" "8b0424" "2b442404" "83c408" "c3")
    return R(r), O(o)


@proves
def test_two_locals_swapped_between_slots_proves():
    a, b = _swap(swap_ours=True)
    assert prove(a, b)["status"] == "proved"


def test_two_locals_swapped_wrong_result_not_proved():
    a, b = _swap(swap_ours=True, bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- a slot reused for a different value
# A dead slot: ours stores a different value into [esp+4] after its last read, where retail leaves its old value.
# The slot is dead (never read again), so F omits it and the pair proves. When the slot is still LIVE (read at the
# return) the differing store is a real difference and the region stays unknown.
def _reuse(dead=True):
    r = H("83ec08" "8b44240c" "890424" "89442404" "8b0424" "83c408" "c3")   # [esp]=x; [esp+4]=x; ret [esp]
    if dead:
        # ours stores a junk value into the dead [esp+4] after everything; returns [esp]
        o = H("83ec08" "8b44240c" "890424" "89442404" "8b0424" "c7442404ff000000" "83c408" "c3")
        return R(r), O(o)
    # ours returns [esp+4] where retail returns [esp]: the slot is live and ours wrote a different value there
    o = H("83ec08" "8b44240c" "890424" "89442404" "c7442404ff000000" "8b442404" "83c408" "c3")
    return R(r), O(o)


@proves
def test_reused_dead_slot_proves():
    a, b = _reuse(dead=True)
    assert prove(a, b)["status"] == "proved"


def test_reused_live_slot_not_proved():
    a, b = _reuse(dead=False)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- an escaped object at different offsets
# g(&obj): retail passes &[esp+0x10], ours &[esp+0x20]; the object's four words are filled equally. With equal
# contents it proves; with one differing field it does not.
def _escape(bad=False):
    def body(off, junk):
        s = H("83ec40")                                        # sub esp,0x40
        for k in range(4):
            v = 0x11 * (k + 1) + (0x100 if (junk and k == 2) else 0)
            s += H("c74424%02x" % (off + 4 * k)) + struct.pack("<I", v)   # mov [esp+off+4k], imm
        s += H("8d4424%02x" % off) + H("50")                   # lea eax,[esp+off]; push eax
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c444" "c3")
        return s, e8
    r, er = body(0x10, False)
    o, eo = body(0x20, bad)
    return R(r, {er + 1: ("abs", RVA + 0x5000)}), O(o, {eo + 1: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAXXZ")


@proves
def test_escaped_object_different_offset_equal_contents_proves():
    a, b = _escape(bad=False)
    assert prove(a, b)["status"] == "proved"


def test_escaped_object_differing_field_not_proved():
    a, b = _escape(bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- different frame sizes
# retail sub esp,0x20, ours sub esp,0x24; the same local is at the same offset from each esp; esp differs at the
# cut by 4 but the outgoing/返回 behaviour is identical.
def _frame_size(bad=False):
    r = H("83ec20" "8b442424" "890424" "8b0424" "83c420" "c3")
    o = H("83ec24" "8b442428" "890424" "8b0424" "83c424" "c3")
    if bad:
        o = H("83ec24" "8b442428" "890424" "8b442404" "83c424" "c3")   # returns a different slot
    return R(r), O(o)


@proves
def test_frame_size_differs_proves():
    a, b = _frame_size(bad=False)
    assert prove(a, b)["status"] == "proved"


def test_frame_size_differs_wrong_slot_not_proved():
    a, b = _frame_size(bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- push vs mov argument passing
# g(x): retail pushes x, ours writes it to [esp+k] after sub esp. The outgoing argument value corresponds.
def _push_mov(bad=False):
    r = H("8b442404" "50" "e8") + b"\x00\x00\x00\x00" + H("83c404" "c3")
    o = H("83ec04" "8b442408") + (H("890424") if not bad else H("c7042407000000")) + \
        H("e8") + b"\x00\x00\x00\x00" + H("83c404" "c3")
    er, eo = r.index(b"\xe8"), o.index(b"\xe8")
    r = r[:er + 1] + struct.pack("<i", (RVA + 0x5000) - (RVA + er + 5)) + r[er + 5:]
    rel_r = {er + 1: ("abs", RVA + 0x5000)}
    rel_o = {eo + 1: ("sym", "_g", 0x14, 0)}
    return R(r, rel_r), O(o, rel_o, symbol="?f@@YAXH@Z")


@proves
def test_push_vs_mov_argument_proves():
    a, b = _push_mov(bad=False)
    assert prove(a, b)["status"] == "proved"


def test_push_vs_mov_wrong_argument_not_proved():
    a, b = _push_mov(bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- a pointer to the wrong object
# ours passes &obj2 where retail passes &obj1 (both filled, but the callee reads the one pointed at): not proved.
def test_pointer_to_wrong_object_not_proved():
    def body(point_off):
        s = H("83ec40")
        for k in range(8):
            s += H("c74424%02x" % (4 * k)) + struct.pack("<I", 0x11 * (k + 1))
        s += H("8d4424%02x" % point_off) + H("50")
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c444" "c3")
        return s, e8
    r, er = body(0x00)
    o, eo = body(0x20)                                         # a different object entirely
    assert prove(R(r, {er + 1: ("abs", RVA + 0x5000)}),
                 O(o, {eo + 1: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAXXZ"))["status"] != "proved"
