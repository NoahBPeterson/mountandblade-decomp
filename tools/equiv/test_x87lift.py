"""Tests of the x87 lifter (tools/equiv/x87lift.py) and of DDEC on lifted x87 code (ddec_layer.py), on
hand-assembled pairs.

The lifter tests are pure and fast. The DDEC tests need Docker with the image mb-stoke-ddec:98d8a0f-2
(tools/equiv/ddec/Dockerfile, with x87/4-x87-lifter.patch) and are skipped otherwise; about a minute per case.

  uv run pytest tools/equiv/test_x87lift.py -q
"""
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import x87lift as XL  # noqa: E402
import mutate  # noqa: E402
from func import Func  # noqa: E402

H = bytes.fromhex
TMO = 600

# fld dword [esp+4]; fmul dword [esp+8]; ret            st0 = a * b
MUL = H("d9442404" "d84c2408" "c3")
MUL_COMM = H("d9442408" "d84c2404" "c3")              # b * a
# fld a; fsub dword [esp+8] / fsubr dword [esp+8]       a - b / b - a
SUB = H("d9442404" "d8642408" "c3")
SUBR = H("d9442404" "d86c2408" "c3")
# fld a; fld b; fsubrp st(1)                            st1 := st0 - st1 = b - a
FX_A = H("d9442404" "d9442408" "dee1" "c3")
# fld a; fld b; fxch st(1); fsubp st(1)                 st1 := st1 - st0 = b - a
FX_B = H("d9442404" "d9442408" "d9c9" "dee9" "c3")
FX_B_NOFXCH = H("d9442404" "d9442408" "9090" "dee9" "c3")     # the fxch dropped: a - b
# mov eax,[esp+4]; fld dword [esp+8]; fstp dword [eax] / fstp qword [eax]; ret
ST_F = H("8b442404" "d9442408" "d918" "c3")
ST_D = H("8b442404" "d9442408" "dd18" "c3")
# a * b spilled to a float and reloaded before adding c, vs kept in the register
SPILL_F = H("d9442404" "d84c2408" "d95c2404" "d9442404" "d844240c" "c3")
NOSPILL = H("d9442404" "d84c2408" "d844240c" "c3")
# (a < b) ? 1 : 0 through fcomp/fnstsw/test ah,5/jp, and the same with the operands swapped (b > a: test ah,0x41)
LT = H("d9442404" "d85c2408" "dfe0" "f6c405" "7a06" "b801000000" "c3" "31c0" "c3")
GT_SWAPPED = H("d9442408" "d85c2404" "dfe0" "f6c441" "7506" "b801000000" "c3" "31c0" "c3")
LT_SWAPPED_WRONG = H("d9442408" "d85c2404" "dfe0" "f6c405" "7a06" "b801000000" "c3" "31c0" "c3")
# float loop: mov ecx,[esp+4]; fldz; L: fadd dword [esp+8]; dec ecx; jnz L; ret        (n * x, summed)
FLOOP = H("8b4c2404" "d9ee" "d8442408" "49" "75f9" "c3")
FLOOP_REN = H("8b542404" "d9ee" "d9442408" "dec1" "4a" "75f7" "c3")   # edx; fld x; faddp st(1)
FLOOP_BAD = H("8b542404" "d9ee" "d9442408" "dee9" "4a" "75f7" "c3")   # fsubp st(1): acc - x


def R(code):
    return Func(0x1000, code, {}, "retail", "retail", {})


def O(code, symbol=""):
    return Func(0, code, {}, "ours", "ours", {"symbol": symbol})


# ------------------------------------------------------------------------------------------- the lifter alone
def test_register_forms_by_bytes():
    # DE E9 is fsubp st(1) (st1 := st1 - st0), DE E1 fsubrp st(1) (st1 := st0 - st1), whatever a tool calls them
    # FX_B: after fxch st(0) = a lives in x0, st(1) = b in x1; fsubp st(1): x1 := x1 - x0 = b - a
    assert XL.lift(FX_B, 0).ops[10] == [("arith", "sub", 1, 0)]
    # FX_A: st(0) = b in x1, st(1) = a in x0; fsubrp st(1): x1 := x1 - x0 = st0 - st1, renamed to position 0
    assert XL.lift(FX_A, 0).ops[8] == [("arith", "sub", 1, 0)]


def test_fxch_is_a_renaming():
    L = XL.lift(FX_B, 0)
    assert 8 not in L.ops                                           # fxch: no code
    assert L.pre[12][0] == ("move", 0, 1)                           # identity restored before the ret
    assert L.ret_depth == 1


def test_depth_mismatch_at_join_rejected():
    # fld a; test ecx,ecx; je L; fld b; L: ret
    code = H("d9442404" "85c9" "7404" "d9442408" "c3")
    with pytest.raises(XL.Reject, match="paths disagree"):
        XL.lift(code, 0)


def test_return_depth_two_rejected():
    with pytest.raises(XL.Reject):
        XL.lift(H("d9442404" "d9442408" "c3"), 0)


def test_underflow_rejected():
    with pytest.raises(XL.Reject, match="underflow"):
        XL.lift(H("ddd8" "c3"), 0)                                  # fstp st(0) on an empty stack


def test_unsupported_rejected():
    for code in (H("d9442404" "d9fe" "c3"),                         # fsin
                 H("d96c2404" "c3"),                                # fldcw
                 H("db6c2404" "c3"),                                # fld tbyte
                 H("d9442404" "d9442408" "dee9" "9e" "c3")):        # sahf
        with pytest.raises(XL.Reject):
            XL.lift(code, 0)


def test_fnstsw_needs_a_compare():
    with pytest.raises(XL.Reject, match="without a compare"):
        XL.lift(H("d9442404" "dfe0" "ddd8" "c3"), 0)
    # data movement between the compare and fnstsw keeps the condition codes (cl: fcomp m; fstp st(0); fnstsw)
    XL.lift(H("d9442404" "d9442408" "d85c2404" "ddd8" "dfe0" "f6c405" "c3"), 0)


def test_fnstsw_merged_tail():
    # two compares jump to one fnstsw ax; test ah,5 tail
    code = H("85c9" "740a" "d9442404" "d85c2408" "eb08" "d9442408" "d85c2404" "dfe0" "f6c405" "c3")
    L = XL.lift(code, 0)
    assert ("fnstsw", 0) in L.ops[0x16]


def test_flags_live_across_fcom_rejected():
    # cmp eax,ecx; fld a; fcomp b; jne ... : the lowering of fcomp writes EFLAGS, the x87 instruction does not
    code = H("39c8" "d9442404" "d85c2408" "7501" "90" "dfe0" "c3")
    with pytest.raises(XL.Reject, match="live across"):
        XL.lift(code, 0)


def test_al_read_after_fnstsw_rejected():
    code = H("d9442404" "d85c2408" "dfe0" "a801" "c3")              # test al,1
    with pytest.raises(XL.Reject, match="al is read"):
        XL.lift(code, 0)


def test_call_depths():
    # call f; fstp dword [esp+4]; ret : f returns a float (depth 1 after the call)
    L = XL.lift(H("e800000000" "d95c2404" "c3"), 0x1000)
    assert L.roots[5] == 1
    # fld a; call __ftol2-like; ret : the callee takes st(0), the continuation has depth 0
    L = XL.lift(H("d9442404" "e800000000" "c3"), 0x1000)
    assert L.depth[4] == 1 and L.roots[9] == 0


def test_x87_mutants():
    kinds = {d.split(" at ")[0] for d, _ in mutate.candidates(O(FX_B))}
    assert "x87 dropped fxch" in kinds and "x87 swapped sub/subr" in kinds
    kinds = {d.split(" at ")[0] for d, _ in mutate.candidates(O(H("d9442404" "d8742408" "d9e8" "dec9" "c3")))}
    assert "x87 swapped div/divr" in kinds and "x87 changed constant" in kinds and "x87 float/double load" in kinds


# ------------------------------------------------------------------------------------------- DDEC on lifted code
try:
    import ddec_layer as DL  # noqa: E402
    HAVE = DL._image_ok()
except ImportError:
    DL, HAVE = None, False
ddec = pytest.mark.skipif(not HAVE, reason="Docker or the STOKE image mb-stoke-ddec:98d8a0f-2 is missing")


def prove(a, b, level="L1"):
    r = DL.prove(a, b, level, None, TMO)
    print(r["status"], r["seconds"], r["detail"][:300])
    return r


@ddec
def test_ddec_commuted_fmul_proved():
    assert prove(R(MUL), O(MUL_COMM))["status"] == "proved"


@ddec
def test_ddec_fsub_vs_fsubr_not_proved():
    assert prove(R(SUB), O(SUBR))["status"] != "proved"


@ddec
def test_ddec_fxch_only_difference_proved():
    assert prove(R(FX_A), O(FX_B))["status"] == "proved"


@ddec
def test_ddec_dropped_fxch_not_proved():
    assert prove(R(FX_A), O(FX_B_NOFXCH))["status"] != "proved"


@ddec
def test_ddec_float_vs_double_store_not_proved():
    assert prove(R(ST_F), O(ST_D))["status"] != "proved"


@ddec
def test_ddec_float_spill_not_proved():
    assert prove(R(SPILL_F), O(NOSPILL))["status"] != "proved"


@ddec
def test_ddec_swapped_compare_proved():
    assert prove(R(LT), O(GT_SWAPPED))["status"] == "proved"
    assert prove(R(LT), O(LT_SWAPPED_WRONG))["status"] != "proved"


@ddec
def test_ddec_stack_depth_mismatch_unknown():
    r = prove(R(H("d9442404" "c3")), O(H("d9442404" "d9442408" "c3")))
    assert r["status"] == "unknown" and "x87" in r["detail"]


@ddec
def test_ddec_float_loop_proved():
    assert prove(R(FLOOP), O(FLOOP_REN))["status"] == "proved"
    assert prove(R(FLOOP), O(FLOOP_BAD))["status"] != "proved"
