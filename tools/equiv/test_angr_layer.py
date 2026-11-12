"""Tests of the angr equivalence layer (tools/equiv/angr_layer.py) on synthetic pairs: what it must prove, what it
must refute or leave unknown, and its hints.

Ported in spirit from spore-decomp tools/equiv/test_angr_layer.py (commit 82de4ab6c), whose cases are spore
functions (an /LTCG build, Raster::Initialize and its hand-made mutants); M&B's cases are hand-assembled pairs that
exercise the same rules plus M&B's additions (x87 square root / uninterpreted transcendental functions, the
literal-constant check, the early stop for input-dependent loops). No game image is needed: the image hooks are
replaced by a fake. Needs work/angr-venv (or $EQUIV_ANGR_PYTHON); skipped otherwise.

  uv run pytest tools/equiv/test_angr_layer.py -q
"""
import os
import struct
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import angr_layer as AL  # noqa: E402
import func as FN  # noqa: E402
import mutate  # noqa: E402
from func import Func  # noqa: E402

H = bytes.fromhex
pytestmark = pytest.mark.skipif(AL._angr_python() is None, reason="angr venv missing (see angr_layer.py)")
RVA = 0x1000
TMO = 90


class FakeImage:
    """func.image() stand-in: a few data bytes, no code (scan_callee finds nothing, as for an unknown callee)."""
    base, size = 0x400000, 0x100000

    def __init__(self, data=None):
        self.data = data or {}

    def read(self, va, n):
        for a, b in self.data.items():
            if a <= va and va + n <= a + len(b):
                return b[va - a:va - a + n]
        raise ValueError("unmapped")

    def in_text(self, va):
        return False

    def func_extent(self, va):
        raise ValueError("no index")


@pytest.fixture(autouse=True)
def fake_image(monkeypatch):
    img = FakeImage()
    monkeypatch.setattr(FN, "_img", img)
    return img


def R(code, relocs=None):
    return Func(RVA, code, relocs or {}, "retail", "retail", {})


def O(code, relocs=None, consts=None, symbol=""):
    return Func(0, code, relocs or {}, "ours", "ours", {"symbol": symbol, "consts": consts or {}})


def prove(a, b, level="L1", hints=None):
    r = AL.prove(a, b, level, hints, TMO)
    print(r["status"], r["detail"][:300])
    return r


# mov ecx,[esp+4]; mov edx,[esp+8]; lea eax,[ecx+edx]; ret      and the same with ecx <-> edx
ADD = H("8b4c2404" "8b542408" "8d0411" "c3")
ADD_REN = H("8b542404" "8b4c2408" "8d040a" "c3")


INT2 = "?add@@YAHHH@Z"                              # int add(int, int): eax only (an unknown name means edx:eax)


def test_renamed_registers_L1_proved():
    assert prove(R(ADD), O(ADD_REN, symbol=INT2))["status"] == "proved"


def test_unknown_return_type_compares_edx():
    r = prove(R(ADD), O(ADD_REN))
    assert r["status"] == "refuted" and "edx" in r["detail"]


def test_renamed_registers_L2_refuted():
    # at L2 ecx and edx are observable at the return, and they differ
    r = prove(R(ADD), O(ADD_REN), "L2")
    assert r["status"] == "refuted" and "ecx" in r["detail"]


def test_changed_immediate_refuted():
    r = prove(R(ADD), O(H("8b542404" "8b4c2408" "8d440a01" "c3"), symbol=INT2))      # lea eax,[edx+ecx+1]
    assert r["status"] == "refuted"


def test_every_mutant_rejected():
    ours = O(ADD_REN, symbol=INT2)
    ms = list(mutate.mutants(ours, 6, seed=1))
    assert ms
    for desc, m in ms:
        assert prove(R(ADD), m)["status"] != "proved", desc


def test_different_instructions_same_value():
    # shl eax,1 / add eax,eax / lea eax,[eax+eax]
    a = H("8b442404" "d1e0" "c3")
    b = H("8b442404" "8d0400" "c3")
    assert prove(R(a), O(b))["status"] == "proved"


# x87 -------------------------------------------------------------------------------------------------------------
def test_x87_same_sequence_proved():
    a = H("d9442404" "d84c2408" "c3")               # fld [esp+4]; fmul [esp+8]
    b = H("8bcc" "d94104" "d84908" "c3")            # mov ecx,esp; fld [ecx+4]; fmul [ecx+8]
    assert prove(R(a), O(b))["status"] == "proved"


def test_x87_wrong_operand_refuted():
    a = H("d9442404" "d84c2408" "c3")
    b = H("d9442404" "d84c2404" "c3")               # fmul [esp+4]: x*x
    assert prove(R(a), O(b))["status"] == "refuted"


def test_x87_fsqrt_supported():
    # angr 10.0.6 has no Iop_SqrtF64; install_x87_ops gives it the IEEE square root
    a = H("d9442404" "d9fa" "c3")
    b = H("8bcc" "d94104" "d9fa" "c3")
    assert prove(R(a), O(b))["status"] == "proved"


def test_x87_fsin_uninterpreted():
    a = H("d9442404" "d9fe" "c3")                   # fsin
    b = H("8bcc" "d94104" "d9fe" "c3")
    assert prove(R(a), O(b))["status"] == "proved"
    c = H("d9442408" "d9fe" "c3")                   # fsin of the other argument: the model's counterexample
    r = prove(R(a), O(c))                           # rests on an uninterpreted function, so it is not trusted
    assert r["status"] == "unknown" and "uninterpreted" in r["detail"]


def test_x87_different_sequences_unknown():
    a = H("d9442404" "dcc0" "c3")                   # fld [esp+4]; fadd st(0),st(0)
    b = H("d9442404" "d9c0" "dec1" "c3")            # fld [esp+4]; fld st(0); faddp st(1)
    r = prove(R(a), O(b))
    assert r["status"] == "unknown" and "x87" in r["detail"]


# relocations and literal constants --------------------------------------------------------------------------------
def test_call_through_bijection(fake_image):
    a = H("8b442404" "50" "e8") + struct.pack("<i", 0x2000 - (RVA + 10)) + H("83c404" "c3")
    b = H("ff742404" "e800000000" "59" "c3")
    r = prove(R(a, {6: ("abs", 0x2000)}), O(b, {5: ("sym", "_f", 0x14, 0)}))
    assert r["status"] == "proved"
    bad = H("ff742408" "e800000000" "59" "c3")      # pushes the wrong argument
    assert prove(R(a, {6: ("abs", 0x2000)}), O(bad, {5: ("sym", "_f", 0x14, 0)}))["status"] != "proved"


def _const_pair(fake_image, retail_value):
    fake_image.data[0x405000] = struct.pack("<f", retail_value)
    a = H("d905") + struct.pack("<I", 0x405000) + H("c3")
    b = H("d90500000000" "c3")
    FN.set_consts(O(b, consts={"__real@3f800000": struct.pack("<f", 1.0)}))
    return (R(a, {2: ("abs", 0x405000)}),
            O(b, {2: ("sym", "__real@3f800000", 6, 0)}, consts={"__real@3f800000": struct.pack("<f", 1.0)}))


def test_literal_constant_same_bytes_proved(fake_image):
    assert prove(*_const_pair(fake_image, 1.0))["status"] == "proved"


def test_literal_constant_other_bytes_rejected(fake_image):
    # memory is uninterpreted: without the constant check both would read the retail address and "prove" it
    # (the pairing by content drops the wrong pair: our 1.0 is left unpaired, so it cannot be proved)
    r = prove(*_const_pair(fake_image, 2.0))
    assert r["status"] == "unknown"
    retail, ours = _const_pair(fake_image, 2.0)
    with pytest.raises(AL.HintError):           # a pairing that hints force onto it is rejected outright
        AL.prepare(retail, ours, "L1", {"bij": [[2, 2]]})


# loops and hints ---------------------------------------------------------------------------------------------------
COUNT4 = H("31c0" "40" "83f804" "7cfa" "c3")        # xor eax,eax; L: inc eax; cmp eax,4; jl L; ret
CONST4 = H("b804000000" "c3")


def test_constant_loop_proved_and_hints():
    r = prove(R(COUNT4), O(CONST4))
    assert r["status"] == "proved"
    assert "loops" in r["hints"]
    r2 = prove(R(COUNT4), O(CONST4), hints=r["hints"])
    assert r2["status"] == "proved"
    bad = {k: v for k, v in r["hints"].items()}
    bad["loops"] = {"retail": {h: [1, why] for h, (n, why) in r["hints"]["loops"]["retail"].items()}}
    assert prove(R(COUNT4), O(CONST4), hints=bad)["status"] == "unknown"


def test_constant_loop_wrong_count_refuted():
    assert prove(R(COUNT4), O(H("b805000000" "c3")))["status"] == "refuted"


def test_input_dependent_loop_with_store_unknown():
    a = H("8b4c2404" "8b442408" "c60000" "40" "49" "75f9" "c3")
    b = H("8b542404" "8b442408" "c60000" "40" "4a" "75f9" "c3")
    r = prove(R(a), O(b))
    assert r["status"] == "unknown" and "loop" in r["detail"]
    assert r["seconds"] < 60


def test_wrong_hint_bijection_unknown(fake_image):
    a = H("8b442404" "50" "e8") + struct.pack("<i", 0x2000 - (RVA + 10)) + H("83c404" "c3")
    b = H("ff742404" "e800000000" "59" "c3")
    ra, ob = R(a, {6: ("abs", 0x2000)}), O(b, {5: ("sym", "_f", 0x14, 0)})
    r = prove(ra, ob)
    assert r["status"] == "proved" and r["hints"]["bij"] == [[6, 5]]
    assert prove(ra, ob, hints=dict(r["hints"], bij=[[6, 6]]))["status"] == "unknown"


def test_void_return_eax_not_compared():
    # void f(int *p) { *p = 1; } with the pointer in eax vs ecx: eax at the return is dead for a void function
    # whose original writes eax (here: no index, so facts.py counts every volatile register as written)
    a = H("8b442404" "c70001000000" "c3")
    b = H("8b4c2404" "c70101000000" "c3")
    assert prove(R(a), O(b, symbol="?f@@YAXPAH@Z"))["status"] == "proved"
    assert prove(R(a), O(b, symbol="?f@@YAHPAH@Z"))["status"] == "refuted"      # int f(int *): eax differs


def test_constants_paired_by_content(fake_image):
    # retail: fld [1.0]; fadd [2.0]   ours: fld [__real 2.0]; fadd [__real 1.0]: the shape alignment pairs the two
    # loads with each other's constant; pairing by content (the bytes chk.py compares) repairs it
    fake_image.data[0x405000] = struct.pack("<f", 1.0)
    fake_image.data[0x405010] = struct.pack("<f", 2.0)
    a = H("d905") + struct.pack("<I", 0x405000) + H("d805") + struct.pack("<I", 0x405010) + H("c3")
    b = H("d90500000000" "d80500000000" "c3")
    consts = {"__real@3f800000": struct.pack("<f", 1.0), "__real@40000000": struct.pack("<f", 2.0)}
    r = prove(R(a, {2: ("abs", 0x405000), 8: ("abs", 0x405010)}),
              O(b, {2: ("sym", "__real@40000000", 6, 0), 8: ("sym", "__real@3f800000", 6, 0)}, consts=consts))
    assert r["status"] == "proved"


def test_mutants_outside_covered_code_left_out():
    # retail calls a noreturn function: what follows in ours is never executed, and mutating it gives an
    # equivalent mutant; the proof reports the instructions it covered and the self-test keeps to them
    a = H("6a01" "e8") + struct.pack("<i", 0x2000 - (RVA + 7)) + H("c3")
    b = H("6a01" "e800000000" "b805000000" "c3")
    ra, ob = R(a, {3: ("abs", 0x2000)}), O(b, {3: ("sym", "_exit", 0x14, 0)})
    orig = AL.scan_callee
    AL.scan_callee = lambda va, budget=4000: {"noreturn": True}
    try:
        r = prove(ra, ob)
        assert r["status"] == "proved"
        assert 7 not in r["covered"] and 0 in r["covered"]
        assert all("+0x7" not in d for d, _ in mutate.mutants(ob, 6, seed=1, covered=r["covered"]))
    finally:
        AL.scan_callee = orig
