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
    r = RG.prove(a, b, level, None, 120, per_region=60, frame_model=FRAME_MODEL)
    print(r["status"], r["detail"][:300])
    return r


def _call_g(at):
    """an e8 call to g at RVA+0x5000; returns (bytes, reloc-offset)"""
    return H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + at + 5)), at + 1


# --------------------------------------------------------------------------- two locals swapped between slots
# void f(int x){ int a=x+1, b=x+2; g(&a,&b); }  retail puts a at [esp], b at [esp+4]; ours the other way round.
# Both pass &a,&b to g (the slots are live at the call, nothing is read after), so a<->a and b<->b correspond
# across the swap (objects by content, the two pointers by the pointer relation). The bad version builds a = x+9,
# so a's contents no longer match: the object is unmatched and the pair is not proved. merged: one `add esp,0x10`
# cleans the arguments and the frame, so the code's evidence allows a callee with four argument words, which would
# read a and b by value at swapped positions: not provable (stage 1 proved it by matching words by content).
def _swap(swap_ours, bad=False, merged=False):
    a_r = H("8d4801")                                                        # lea ecx,[eax+1]  ; a = x+1
    a_o = H("8d4809") if bad else H("8d4801")                                # ours a = x+9 (bad) else x+1
    pre_r = (H("83ec08" "8b44240c") + a_r + H("890c24") + H("8d5002" "89542404") +    # a@[esp], b@[esp+4]
             H("8d0c24" "8d542404" "52" "51"))                              # &a=ecx, &b=edx; push &b; push &a
    pre_o = (H("83ec08" "8b44240c") + a_o + H("894c2404") + H("8d5002" "891424") +    # a@[esp+4], b@[esp]
             H("8d4c2404" "8d1424" "52" "51"))                              # &a=ecx, &b=edx; push &b; push &a
    call_r, rr = _call_g(len(pre_r))
    call_o, ro = _call_g(len(pre_o))
    tail = H("83c410" "31c0" "c3") if merged else H("83c408" "83c408" "31c0" "c3")    # cleanup; eax=0; ret
    return (R(pre_r + call_r + tail, {rr: ("abs", RVA + 0x5000)}),
            O(pre_o + call_o + tail, {ro: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAHH@Z"))


@proves
def test_two_locals_swapped_between_slots_proves():
    a, b = _swap(swap_ours=True)
    assert prove(a, b)["status"] == "proved"


def test_two_locals_swapped_merged_cleanup_not_proved():
    a, b = _swap(swap_ours=True, merged=True)
    assert prove(a, b)["status"] != "proved"


def test_two_locals_swapped_wrong_result_not_proved():
    a, b = _swap(swap_ours=True, bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- a slot reused for a different value
# Both fill [esp] and [esp+4] with x, load [esp] into eax, then a branch joins before the frame is freed. ours
# stores 0xff into the dead [esp+4] where retail stores x; the differing store sizes make the two versions
# diverge, so an angr region runs to the join. At the join [esp+4] is DEAD (only eax is used), so the frame model
# excludes it and the pair proves. In the LIVE case [esp+4] is re-loaded after the join, so the junk is observed:
# not proved.  (A non-call stop: at a call the callee could read [esp+4] as an argument, so it could not be
# excluded.)
def _reuse(dead=True):
    #           sub esp,8; eax=[esp+0xc]; [esp]=eax; [esp+4]=<x|0xff>; eax=[esp]; test eax,eax; je +2; (mov eax,eax)
    #           JOIN: [reload [esp+4] if live]; add esp,8; ret
    def build(junk, live):
        s = H("83ec08" "8b44240c" "890424")                                  # sub esp,8; eax=x; [esp]=x
        s += H("c7442404ff000000") if junk else H("89442404")               # [esp+4] = 0xff (junk) or x
        s += H("8b0424" "85c0" "7402" "89c0")                                # eax=[esp]; test; je JOIN; mov eax,eax
        if live:
            s += H("8b442404")                                               # JOIN: eax=[esp+4]  (reuses the slot)
        s += H("83c408" "c3")
        return s
    return R(build(False, not dead)), O(build(True, not dead), symbol="?f@@YAHH@Z")


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
def _escape(bad=False, merged=False):
    """merged: one `add esp,0x44` cleans the argument and the frame (the code then allows a callee reading 17
    argument words, which would see the objects at different positions: not provable)"""
    def body(off, junk):
        s = H("83ec40")                                        # sub esp,0x40
        for k in range(4):
            v = 0x11 * (k + 1) + (0x100 if (junk and k == 2) else 0)
            s += H("c74424%02x" % (off + 4 * k)) + struct.pack("<I", v)   # mov [esp+off+4k], imm
        s += H("8d4424%02x" % off) + H("50")                   # lea eax,[esp+off]; push eax
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + \
            (H("83c444" "c3") if merged else H("83c404" "83c440" "c3"))
        return s, e8
    r, er = body(0x10, False)
    o, eo = body(0x20, bad)
    return R(r, {er + 1: ("abs", RVA + 0x5000)}), O(o, {eo + 1: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAXXZ")


@proves
def test_escaped_object_different_offset_equal_contents_proves():
    a, b = _escape(bad=False)
    assert prove(a, b)["status"] == "proved"


def test_escaped_object_merged_cleanup_not_proved():
    a, b = _escape(bad=False, merged=True)
    assert prove(a, b)["status"] != "proved"


def test_escaped_object_differing_field_not_proved():
    a, b = _escape(bad=True)
    assert prove(a, b)["status"] != "proved"


# --------------------------------------------------------------------------- different frame sizes
# retail sub esp,0x20, ours sub esp,0x24; the local is at the same offset from each esp. The frame is deallocated
# before the ret, so this proves without the frame model. A frame-size difference LIVE at a cut needs the esp
# delta g (esp_ours = esp_retail + g), which the current implementation does not model (LAYERS.md "Status"):
# keeping esp equal at cuts, it only handles slot reassignment at an equal frame size. So this is a baseline case,
# not yet an esp-delta test.
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
# g(x): retail pushes x, ours sub esp,4 then mov [esp],x. Both leave esp and the stacked argument identical, so
# this is equivalent by construction (it proves with or without the frame model); it documents that push/mov
# argument passing is accepted, not an esp-delta test.
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
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404" "83c440" "c3")
        return s, e8
    r, er = body(0x00)
    o, eo = body(0x20)                                         # a different object entirely
    assert prove(R(r, {er + 1: ("abs", RVA + 0x5000)}),
                 O(o, {eo + 1: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAXXZ"))["status"] != "proved"


# --------------------------------------------------------------------------- an escaped object matched word by word
# g(&obj) reads obj's two words. retail: obj = (1, 2) at [esp+0x10], another live word 3 at [esp+0x30]; ours: obj =
# (1, 3) at [esp+0x20], the other word 2 at [esp+0x30]. Every frame word has an equal-content partner on the other
# side and the pointers' targets (the first words, 1 and 1) are matched, but g reads (1, 2) in retail and (1, 3) in
# ours: NOT equivalent. The pointer relation must require the pointed object's whole extent to correspond with ONE
# shift, not just its first word. This was a KNOWN FALSE PROOF of stage 1's dead-after subset (a0217a4e; the flag is
# off by default and never recorded); stage 2's window obligation (_frame_window_diffs: every word the callee can
# reach relative to the pointer corresponds under the pointer's one shift) rejects it.
def _escape_split(bad=True):
    def body(obj, other, vals, extra):
        s = H("83ec40")                                                       # sub esp,0x40
        s += H("c74424%02x" % obj) + struct.pack("<I", vals[0])
        s += H("c74424%02x" % (obj + 4)) + struct.pack("<I", vals[1])
        s += H("c74424%02x" % other) + struct.pack("<I", extra)
        s += H("8d4424%02x" % obj) + H("50")                                  # lea eax,[esp+obj]; push eax
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404" "83c440" "c3")
        return s, e8
    r, er = body(0x10, 0x30, (1, 2), 3)
    o, eo = body(0x20, 0x30, (1, 3) if bad else (1, 2), 2 if bad else 3)
    return R(r, {er + 1: ("abs", RVA + 0x5000)}), O(o, {eo + 1: ("sym", "_g", 0x14, 0)}, symbol="?f@@YAXXZ")


def test_escaped_object_fields_matched_elsewhere_not_proved():
    a, b = _escape_split(bad=True)
    assert prove(a, b)["status"] != "proved"


# =========================================================================== stage 1: F carried past the cut
# (LAYERS.md "The per-object frame model", items 1-3: F in the cut key, F' from the region's exit, the next region's
# entry state remapping ours' frame through F, call havoc that keeps F where no frame address escaped). These pairs
# READ the reassigned slots after the cut, so the dead-after subset cannot prove them.
stage1 = proves                       # implemented (regions.py F in the cut key, angr_layer frame_base / F')


def _swap_live(bad=False, call=False):
    """int f(int x){ int a = x+1, b = x+2; if (x) {} [g(x);] return a - b; }: retail a@[esp] b@[esp+4], ours the
    other way round; a loop head after the branch's join is a region stop before the slots are read, so F must pair
    a<->a and b<->b there and carry it into the next region. bad: ours returns b - a. call: g(x) by value between the join and
    the reads (no frame address escapes: the callee cannot touch the slots, F survives the call)."""
    def build(a_off, b_off, reads):
        s = H("83ec08" "8b44240c" "8d4801")                                  # sub esp,8; eax=x; ecx=x+1
        s += H("890c24") if a_off == 0 else H("894c2404")                    # a
        s += H("8d5002")                                                     # edx=x+2
        s += H("891424") if b_off == 0 else H("89542404")                    # b
        s += H("85c0" "7402" "89c0")                                         # test; je JOIN; mov eax,eax
        s += H("b903000000" "49" "75fd")                 # JOIN: ecx=3; L: dec ecx; jnz L (a loop head: a stop)
        rel = {}
        if call:
            s += H("50")                                                     # JOIN: push x
            e8 = len(s)
            s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404")
            rel = {e8 + 1: True}
        first, second = reads
        s += (H("8b0424") if first == 0 else H("8b442404"))                  # eax = [first]
        s += (H("2b0424") if second == 0 else H("2b442404"))                 # eax -= [second]
        return s + H("83c408" "c3"), rel
    r, rr = build(0, 4, (0, 4))
    o, ro = build(4, 0, (0, 4) if bad else (4, 0))
    return (R(r, {k: ("abs", RVA + 0x5000) for k in rr}),
            O(o, {k: ("sym", "_g", 0x14, 0) for k in ro}, symbol="?f@@YAHH@Z"))


@stage1
def test_slots_swapped_read_after_join_proves():
    a, b = _swap_live()
    assert prove(a, b)["status"] == "proved"


def test_slots_swapped_read_after_join_wrong_slot_not_proved():
    a, b = _swap_live(bad=True)
    assert prove(a, b)["status"] != "proved"


@stage1
def test_slots_swapped_live_across_non_escaping_call_proves():
    a, b = _swap_live(call=True)
    assert prove(a, b)["status"] == "proved"


def test_slots_swapped_across_call_wrong_slot_not_proved():
    a, b = _swap_live(bad=True, call=True)
    assert prove(a, b)["status"] != "proved"


def _swap_escape(wrong):
    """the slots of _swap_live, but g(&a) (an escape: the callee may write a) between the join and the reads; ours
    passes &b when wrong. F cannot survive an escaping call (stage 1): the frame is compared at the same address,
    so even the right version is not proved; the wrong one must never be."""
    def build(a_off, b_off, pass_off):
        s = H("83ec08" "8b44240c" "8d4801")
        s += H("890c24") if a_off == 0 else H("894c2404")
        s += H("8d5002")
        s += H("891424") if b_off == 0 else H("89542404")
        s += H("85c0" "7402" "89c0")
        s += H("b903000000" "49" "75fd")
        s += H("8d0c24" "51") if pass_off == 0 else H("8d4c2404" "51")       # lea ecx,[esp+pass]; push ecx
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404")
        s += H("8b0424" "2b442404") if a_off == 0 else H("8b442404" "2b0424")
        return s + H("83c408" "c3"), e8 + 1
    r, er = build(0, 4, 0)
    o, eo = build(4, 0, 0 if wrong else 4)
    return (R(r, {er: ("abs", RVA + 0x5000)}),
            O(o, {eo: ("sym", "?g@@YAXPAH@Z", 0x14, 0)}, symbol="?f@@YAHH@Z"))


def test_slots_swapped_escaping_call_wrong_object_not_proved():
    a, b = _swap_escape(wrong=True)
    assert prove(a, b)["status"] != "proved"


def test_slots_swapped_call_without_cleanup_not_proved():
    """the arguments of a call to a cdecl callee are known only from the cleanup right after the call: without it
    the callee may own any frame byte above its return address as a parameter, and F does not survive the call
    (equivalent, but beyond what the model may assume)"""
    a, b = _swap_live(call=True)
    k = b.code.index(H("83c404" "8b"))                     # ours' add esp,4 -> lea esp,[esp+4] (no cleanup idiom)
    code = b.code[:k] + H("8d642404") + b.code[k + 3:]
    b = O(code, dict(b.relocs), symbol="?f@@YAHH@Z")
    assert prove(a, b)["status"] != "proved"


# =========================================================================== stage 2: the esp delta g
# (LAYERS.md "Stage 2"): the frame sizes differ while the frame is live at a cut, so ours' esp = the original's + g
# there (g from the two static depths), with F relative to the entry esp.
stage2 = proves


def _fsize_live(bad=False, call=False, arg_bad=False, read_bad=False):
    """int f(int x){ int a = x+1, b = x+2; if (x) {} [g(x);] return a; }: retail's frame 0x20 (a@[esp+8],
    b@[esp+0xc]), ours' 0x24 (a@[esp+0x14], b@[esp+4]); a loop head after the join is a stop while both frames are
    allocated, so the cut has esp_ours = esp_orig - 4. bad: ours returns b. call: g(x) by value after the loop (no
    escape: F survives it at the two esp); arg_bad: ours passes x+1 instead. read_bad: ours reads a slot it never
    wrote (garbage) where the original reads a."""
    def build(fs, a_off, b_off, ret_off, arg_reg="eax"):
        s = H("83ec%02x" % fs) + H("8b4424%02x" % (fs + 4))               # sub esp,fs; eax=x
        s += H("8d4801") + H("894c24%02x" % a_off)                         # ecx=x+1; [esp+a]=ecx
        s += H("8d5002") + H("895424%02x" % b_off)                         # edx=x+2; [esp+b]=edx
        s += H("85c0" "7402" "89c0")                                       # test; je JOIN; mov eax,eax
        s += H("b903000000" "49" "75fd")                                   # ecx=3; L: dec ecx; jnz L (a stop)
        rel = {}
        if call:
            s += H("50") if arg_reg == "eax" else H("40" "50")             # push x (or inc eax; push x+1)
            e8 = len(s)
            s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404")
            rel = {e8 + 1: True}
        s += H("8b4424%02x" % ret_off)                                     # eax = [esp+ret]
        return s + H("83c4%02x" % fs) + H("c3"), rel
    r, rr = build(0x20, 0x08, 0x0c, 0x08)
    o, ro = build(0x24, 0x14, 0x04, 0x04 if bad else (0x18 if read_bad else 0x14),
                  "inc" if arg_bad else "eax")
    return (R(r, {k: ("abs", RVA + 0x5000) for k in rr}),
            O(o, {k: ("sym", "_g", 0x14, 0) for k in ro}, symbol="?f@@YAHH@Z"))


@stage2
def test_frame_size_differs_live_at_a_cut_proves():
    a, b = _fsize_live()
    assert prove(a, b)["status"] == "proved"


def test_frame_size_differs_live_at_a_cut_wrong_slot_not_proved():
    a, b = _fsize_live(bad=True)
    assert prove(a, b)["status"] != "proved"


def test_frame_size_differs_live_at_a_cut_garbage_slot_not_proved():
    a, b = _fsize_live(read_bad=True)
    assert prove(a, b)["status"] != "proved"


@stage2
def test_frame_size_differs_across_non_escaping_call_proves():
    a, b = _fsize_live(call=True)
    assert prove(a, b)["status"] == "proved"


def test_frame_size_differs_across_call_wrong_slot_not_proved():
    a, b = _fsize_live(call=True, bad=True)
    assert prove(a, b)["status"] != "proved"


def test_frame_size_differs_call_wrong_argument_not_proved():
    a, b = _fsize_live(call=True, arg_bad=True)
    assert prove(a, b)["status"] != "proved"


def _fsize_esc(wrong=False, sized=True):
    """the frames of _fsize_live, g(&a) after the loop (an escape) and a read of a after it: retail passes
    &[esp+8], ours &[esp+0x14] (wrong: &[esp+4], b). Not proved in stage 1 (an escaping call compares the frame at
    the same address); stage 2's pointer relation proves the right one, never the wrong one."""
    def build(fs, a_off, b_off, pass_off):
        s = H("83ec%02x" % fs) + H("8b4424%02x" % (fs + 4))
        s += H("8d4801") + H("894c24%02x" % a_off)
        s += H("8d5002") + H("895424%02x" % b_off)
        s += H("85c0" "7402" "89c0")
        s += H("b903000000" "49" "75fd")
        s += H("8d4c24%02x" % pass_off) + H("51")                           # lea ecx,[esp+pass]; push ecx
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404")
        s += H("8b4424%02x" % a_off)                                       # eax = a (the callee may change it)
        return s + H("83c4%02x" % fs) + H("c3"), e8 + 1
    r, er = build(0x20, 0x08, 0x0c, 0x08)
    o, eo = build(0x24 if sized else 0x20, 0x14 if sized else 0x0c, 0x04 if sized else 0x08,
                  (0x04 if sized else 0x08) if wrong else (0x14 if sized else 0x0c))
    return (R(r, {er: ("abs", RVA + 0x5000)}),
            O(o, {eo: ("sym", "?g@@YAXPAH@Z", 0x14, 0)}, symbol="?f@@YAHH@Z"))


def test_frame_size_differs_escaping_call_wrong_object_not_proved():
    a, b = _fsize_esc(wrong=True)
    assert prove(a, b)["status"] != "proved"


def test_escaped_object_moved_frame_read_after_not_proved():
    """g(&a) with a at different offsets and a read of a after the call: the callee's havoc writes each version's
    object at its own addresses, so a shifted escape is related only when nothing reads the frame afterwards"""
    a, b = _fsize_esc(wrong=False)
    assert prove(a, b)["status"] != "proved"


def _esc_same(bad=None):
    """int f(int x){ int a = x+1, b = x+2; volatile int pad; if (x) {} g(&a); return a; }: retail's frame 0x20
    (a@[esp+8], b@[esp+0xc]); ours' 0x24 with a pad word at its bottom ([esp] = 0), a@[esp+0xc], b@[esp+0x10]: the
    same offsets from the entry esp, esp 4 lower. The escaped object is at the same address on both sides (shift 0)
    while the esp differ, and a is read after the call. bad: "arg" ours passes &b; "b" ours stores x+3 in b (the
    callee could read it: the object's extent is unknown); "one" ours also writes a word retail never writes inside
    the window; "after" ours reads b after the call."""
    def build(fs, a_off, b_off, pad, pass_off, extra=None, ret_off=None):
        s = H("83ec%02x" % fs) + H("8b4424%02x" % (fs + 4))
        if pad:
            s += H("c7042400000000")                                      # mov dword [esp], 0
        s += H("8d4801") + H("894c24%02x" % a_off)
        s += H("8d50%02x" % (3 if bad == "b" and pad else 2)) + H("895424%02x" % b_off)
        if extra is not None:
            s += H("c74424%02x" % extra) + struct.pack("<I", 7)
        s += H("85c0" "7402" "89c0")
        s += H("b903000000" "49" "75fd")                                   # a loop head: a stop (a cut)
        s += H("8d4c24%02x" % pass_off) + H("51")
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404")
        s += H("8b4424%02x" % (a_off if ret_off is None else ret_off))
        return s + H("83c4%02x" % fs) + H("c3"), e8 + 1
    r, er = build(0x20, 0x08, 0x0c, False, 0x08)
    o, eo = build(0x24, 0x0c, 0x10, True, 0x10 if bad == "arg" else 0x0c, extra=0x18 if bad == "one" else None,
                  ret_off=0x10 if bad == "after" else None)
    return (R(r, {er: ("abs", RVA + 0x5000)}),
            O(o, {eo: ("sym", "?g@@YAXPAH@Z", 0x14, 0)}, symbol="?f@@YAHH@Z"))


@stage2
def test_escaped_object_same_offset_esp_delta_proves():
    a, b = _esc_same()
    assert prove(a, b)["status"] == "proved"


@pytest.mark.parametrize("bad", ["arg", "b", "one", "after"])
def test_escaped_object_same_offset_esp_delta_negatives_not_proved(bad):
    a, b = _esc_same(bad)
    assert prove(a, b)["status"] != "proved"


def _fp_reg(bad=False):
    """a frame address in a register live across a stop: lea esi,[a]; a loop head; return *esi. retail's frame 0x20
    (a@[esp+8], b@[esp+0xc]), ours' 0x24 (a@[esp+0x14], b@[esp+4]): esi differs by a constant at the stop (pi
    "esi@c"). bad: ours points esi at b."""
    def build(fs, a_off, b_off, p_off):
        s = H("56") + H("83ec%02x" % fs) + H("8b4424%02x" % (fs + 8))     # push esi; sub esp,fs; eax=x
        s += H("8d4801") + H("894c24%02x" % a_off)
        s += H("8d5002") + H("895424%02x" % b_off)
        s += H("8d7424%02x" % p_off)                                       # lea esi,[esp+p]
        s += H("b903000000" "49" "75fd")                                   # a loop head: a stop
        s += H("8b06")                                                     # eax = [esi]
        return s + H("83c4%02x" % fs) + H("5e" "c3")
    return R(build(0x20, 0x08, 0x0c, 0x08)), O(build(0x24, 0x14, 0x04, 0x04 if bad else 0x14), symbol="?f@@YAHH@Z")


@stage2
def test_frame_pointer_register_across_a_stop_proves():
    a, b = _fp_reg()
    assert prove(a, b)["status"] == "proved"


def test_frame_pointer_register_across_a_stop_wrong_object_not_proved():
    a, b = _fp_reg(bad=True)
    assert prove(a, b)["status"] != "proved"


def _chkstk(bad=False):
    """a large frame allocated with __chkstk (mov eax, N; call __chkstk), N 0x1004 against 0x1008: the call's eax
    differs by design; x stored in a slot at the same entry offset, a stop, then read back. bad: ours reads another
    slot. (The callee is named __chkstk_synthetic: chkstk by its prefix, absent from the symbols file, which
    places the real __chkstk elsewhere than this synthetic image's callee.)"""
    def build(n, slot, read):
        s = H("b8") + struct.pack("<I", n)                                # mov eax, n
        e8 = len(s)
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5))  # call __chkstk
        s += H("8b8c24") + struct.pack("<I", n + 4)                         # mov ecx, [esp+n+4]  (x)
        s += H("898c24") + struct.pack("<I", slot)                          # mov [esp+slot], ecx
        s += H("b903000000" "49" "75fd")                                    # a loop head: a stop
        s += H("8b8424") + struct.pack("<I", read)                          # mov eax, [esp+read]
        s += H("81c4") + struct.pack("<I", n) + H("c3")
        return s, e8 + 1
    r, er = build(0x1004, 0x1000, 0x1000)
    o, eo = build(0x1008, 0x1004, 0x1000 if bad else 0x1004)
    return (R(r, {er: ("abs", RVA + 0x5000)}), O(o, {eo: ("sym", "__chkstk_synthetic", 0x14, 0)}, symbol="?f@@YAHH@Z"))


@stage2
def test_chkstk_frame_size_differs_proves():
    a, b = _chkstk()
    assert prove(a, b)["status"] == "proved"


def test_chkstk_frame_size_differs_wrong_slot_not_proved():
    a, b = _chkstk(bad=True)
    assert prove(a, b)["status"] != "proved"


def test_pushed_local_passed_by_address_not_proved():
    """cl allocates a 4-byte local with `push ecx`: the bytes pushed right before a call are not excluded from the
    escaped objects when the call receives their address (ours stores x+1 there instead of x)"""
    def build(inc):
        s = H("8b442404" "51") + (H("40") if inc else b"") + H("890424" "8d0c24" "51")
        e8 = len(s)                      # eax=x; push ecx; [inc eax]; [esp]=eax; lea ecx,[esp]; push ecx
        s += H("e8") + struct.pack("<i", (RVA + 0x5000) - (RVA + e8 + 5)) + H("83c404" "58" "c3")
        return s, e8 + 1
    r, er = build(False)
    o, eo = build(True)
    assert prove(R(r, {er: ("abs", RVA + 0x5000)}),
                 O(o, {eo: ("sym", "?g@@YAXPAH@Z", 0x14, 0)}, symbol="?f@@YAHH@Z"))["status"] != "proved"


def test_frame_liveness_byte_offsets():
    """frame_liveness labels the byte at E - off as off (byte k of an access at [esp+disp] is off = dep - disp - k).
    A dword store at [esp+2] writes [esp+2, esp+6), so of a later dword read of [esp] the bytes [esp], [esp+1] are
    live before the store (labelled by the access base instead, the store killed the wrong bytes)"""
    code = H("83ec10" "89442402" "8b0424" "83c410" "c3")       # sub esp,0x10; mov [esp+2],eax; mov eax,[esp]
    side = RG.Side(code, RVA, {}, {})
    side.call_target = lambda i: None
    prov = side.provenance(lambda i: 0)
    live, _dead = side.frame_liveness(prov, lambda i: 0)
    st = live[RVA + 3]
    assert st != "all" and {0x0f, 0x10} <= set(st) and not {0x0d, 0x0e} & set(st)


def test_frame_liveness_pop_reads_the_stack():
    """pop reads [esp]: a value pushed before a stop and popped after it is live at the stop"""
    code = H("50" "85c0" "7400" "59" "c3")          # push eax; test; je +0; pop ecx; ret
    side = RG.Side(code, RVA, {}, {})
    side.call_target = lambda i: None
    prov = side.provenance(lambda i: 0)
    live, _dead = side.frame_liveness(prov, lambda i: 0)
    pop = RVA + 5
    assert live[pop] != "all" and set(range(1, 5)) <= set(live[pop])


# The self-test must still reject every mutant of a frame-model proof in which every frame slot matters (a mutant
# that moves a value to a different slot, changes an object field, or drops a store must not prove under the
# correspondence). The reuse-DEAD case is deliberately excluded: a mutant of a store into a dead slot is an
# equivalent mutant (the slot is never read), which proves correctly -- the same dead-code/self-test tension
# mutate.mutants already handles with `covered` -- so prove.py would not record such a proof anyway.
import mutate  # noqa: E402


@pytest.mark.skipif(not FRAME_MODEL, reason="frame model off")
# (_swap_live without the call has an equivalent mutant: its `mov eax, eax` filler is dead after the join; so do
# _esc_same and _fsize_live, whose b is never read)
@pytest.mark.parametrize("mk", [lambda: _escape(bad=False), lambda: _swap(True), lambda: _swap_live(call=True)])
def test_frame_mutants_rejected(mk):
    a, b = mk()
    r = prove(a, b)
    assert r["status"] == "proved"
    ms = list(mutate.mutants(b, 8, seed=5, covered=r.get("covered")))
    assert ms
    for desc, m in ms:
        rm = RG.prove(a, m, "L1", None, 120, per_region=60, frame_model=True)
        assert rm["status"] != "proved", desc


def test_whole_function_layer_with_a_call():
    """the whole-function angr layer runs do_call too (the frame model's call bookkeeping must not assume region
    mode): an identical pair with a call runs without an error"""
    a, b = _push_mov(bad=False)
    r = AL.prove(a, b, "L1", None, 120)
    assert "error" not in r["detail"], r["detail"]
