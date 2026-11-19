"""Synthetic pairs for rename_core.py (the spirit of spore-decomp work/scratch/equiv_rename/tests.py, plus the frame
model): what must be accepted, and the mutations it must reject.

  uv run pytest tools/equiv -q
"""
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rename_core as RC  # noqa: E402
import callconv  # noqa: E402
import mutate  # noqa: E402
from func import Func  # noqa: E402

H = bytes.fromhex


@pytest.fixture(autouse=True)
def no_image_hooks():
    """Synthetic code: no original image behind it (rename.py would install facts.py's hooks)."""
    saved = (RC.STD_SAFE, RC.ENTRY_REGS, RC.CALLEE_POP, RC.TOKEN_OK, RC.FRAME_SLOTS)
    RC.STD_SAFE = RC.ENTRY_REGS = RC.CALLEE_POP = RC.TOKEN_OK = None
    RC.FRAME_SLOTS = True
    yield
    RC.STD_SAFE, RC.ENTRY_REGS, RC.CALLEE_POP, RC.TOKEN_OK, RC.FRAME_SLOTS = saved


def chk(a, b, level=1, ra=None, rb=None):
    ok, why = RC.check(a, 0x1000, ra or {}, b, 0, rb or {}, level=level)
    return ok, why


def calltok(code, name, base=0):
    """the relocation of the (only, or every) `call rel32` in code, naming the callee"""
    out, k = {}, code.find(H("e800000000"))
    while k >= 0:
        out[k + 1] = ("sym", name, 0x14, 0)
        k = code.find(H("e800000000"), k + 1)
    return out


def accept(*a, **kw):
    ok, why = chk(*a, **kw)
    assert ok, why
    return why


def reject(*a, **kw):
    ok, why = chk(*a, **kw)
    assert not ok, "accepted: %s" % (why,)
    return why


# --- registers and scheduling (spore's rule) ---------------------------------------------------------------------
A = H("8b4c2404" "8b542408" "8d0411" "c3")          # mov ecx,[esp+4]; mov edx,[esp+8]; lea eax,[ecx+edx]; ret
Bren = H("8b542404" "8b4c2408" "8d040a" "c3")       # the same with ecx <-> edx


def test_pure_rename_accepted():
    accept(A, Bren)


def test_rename_visible_at_level2():
    reject(A, Bren, level=2)                         # ecx/edx differ at ret


A2 = H("8b442404" "8bc8" "83c101" "8bd0" "83c202" "8d0411" "c3")   # two chains from eax, then lea eax,[ecx+edx]
B2 = H("8b442404" "8bd0" "83c202" "8bc8" "83c101" "8d0411" "c3")   # the chains in the other order


def test_independent_reorder_accepted():
    accept(A2, B2)
    accept(A2, B2, level=2)


def test_swapped_dependent_instructions_rejected():
    Bsw = H("8b442404" "83c101" "8bc8" "8bd0" "83c202" "8d0411" "c3")   # add ecx,1 before mov ecx,eax
    reject(A2, Bsw)


def test_changed_immediate_rejected():
    Bimm = H("8b442404" "8bc8" "83c101" "8bd0" "83c203" "8d0411" "c3")  # add edx,3
    reject(A2, Bimm)


def test_constants_exchanged_between_chains_rejected():
    # with lea eax,[ecx+edx] the exchange is a commutation (accepted); with a scale it changes the value
    Bx = H("8b442404" "8bd0" "83c201" "8bc8" "83c102" "8d0411" "c3")
    accept(A2, Bx)
    A3 = H("8b442404" "8bc8" "83c101" "8bd0" "83c202" "8d0451" "c3")
    B3 = H("8b442404" "8bd0" "83c201" "8bc8" "83c102" "8d0451" "c3")
    reject(A3, B3)


def test_two_stores_swapped_rejected():
    Ast = H("8b4c2404" "8b542408" "c70101000000" "c70202000000" "c3")   # [ecx]=1; [edx]=2 (may alias)
    Bst = H("8b4c2404" "8b542408" "c70202000000" "c70101000000" "c3")
    reject(Ast, Bst)


def test_flag_producer_changed_rejected():
    Ainc2 = H("83c001" "41" "7401" "40" "c3")        # je reads ZF of inc ecx
    Binc2 = H("41" "83c001" "7401" "40" "c3")        # ... of add eax,1
    reject(Ainc2, Binc2)
    Ainc = H("83c001" "41" "7201" "40" "c3")         # jb reads CF of the add in both (inc keeps CF)
    Binc = H("41" "83c001" "7201" "40" "c3")
    accept(Ainc, Binc)


# --- frame slots ---------------------------------------------------------------------------------------------------
# return a1 - a2 through two locals
SA = H("83ec08" "8b44240c" "890424" "8b4c2410" "894c2404" "8b0424" "2b442404" "83c408" "c3")
SB = H("83ec08" "8b44240c" "89442404" "8b4c2410" "890c24" "8b442404" "2b0424" "83c408" "c3")      # locals swapped
SBIG = H("83ec0c" "8b442410" "89442404" "8b4c2414" "894c2408" "8b442404" "2b442408" "83c40c" "c3")  # bigger frame
SALIAS = H("83ec08" "8b44240c" "890424" "8b4c2410" "890c24" "8b0424" "2b442404" "83c408" "c3")    # both into [esp]
SREV = H("83ec08" "8b44240c" "890424" "8b4c2410" "894c2404" "8b442404" "2b0424" "83c408" "c3")    # a2 - a1


def test_slot_rename_accepted():
    why = accept(SA, SB)
    assert why["mode"] == "slots" and why["slot_renamed"] > 0


def test_slot_rename_needs_the_frame_model():
    RC.FRAME_SLOTS = False                            # frame operands by offset only (the spore rule)
    reject(SA, SB)


def test_bigger_frame_accepted():
    accept(SA, SBIG)                                  # sub esp, 8 vs 0xc: the depth check covers the immediate


def test_slot_aliasing_change_rejected():
    reject(SA, SALIAS)


def test_slot_values_swapped_rejected():
    reject(SA, SREV)


# a double argument copied to a local, then its HIGH dword returned
QA = H("83ec08" "dd44240c" "dd1c24" "8b442404" "83c408" "c3")
QLOW = H("83ec08" "dd44240c" "dd1c24" "8b0424" "83c408" "c3")                 # the low dword instead
QBIG = H("83ec0c" "dd442410" "dd5c2404" "8b442408" "83c40c" "c3")             # the slot moved, high dword kept


def test_overlap_respected():
    accept(QA, QBIG)
    reject(QA, QLOW)


def test_callee_saved_register_choice_accepted():
    Acs = H("53" "8b5c2408" "8bc3" "5b" "c3")         # push ebx; mov ebx,[esp+8]; mov eax,ebx; pop ebx; ret
    Bcs = H("56" "8b742408" "8bc6" "5e" "c3")         # the same with esi
    accept(Acs, Bcs)
    accept(Acs, Bcs, level=2)
    Bnot = H("56" "8b742408" "8bc6" "59" "c3")        # esi saved but popped into ecx: esi not restored
    reject(Acs, Bnot)


def test_flags_of_frame_adjustment_must_be_dead():
    Af = H("83ec08" "7200" "83c408" "c3")             # jb reads the flags of sub esp, 8
    Bf = H("83ec0c" "7200" "83c40c" "c3")
    reject(Af, Bf)


# a local passed to a cdecl f(int): the argument is read positionally above esp at the call
F1 = "?f@@YAXH@Z"
CA = H("83ec08" "894c2404" "ff742404" "e800000000" "83c404" "83c408" "c3")
CB = H("83ec0c" "890c24" "ff3424" "e800000000" "83c404" "83c40c" "c3")          # another slot, another frame size
CBAD = H("83ec0c" "890c24" "ff742408" "e800000000" "83c404" "83c40c" "c3")      # pushes an unwritten slot


def test_call_argument_from_renamed_slot_accepted():
    accept(CA, CB, ra=calltok(CA, F1), rb=calltok(CB, F1))


def test_call_argument_from_wrong_slot_rejected():
    reject(CA, CBAD, ra=calltok(CA, F1), rb=calltok(CBAD, F1))


def test_unknown_argument_bytes_needs_same_depth():
    why = reject(CA, CB, ra=calltok(CA, "_unknown_c_function"), rb=calltok(CB, "_unknown_c_function"))
    assert "unknown argument bytes" in why


def test_stack_argument_changed_rejected():
    Ap = H("ff742404" "ff742408" "e800000000" "83c408" "c3")   # push a1; push a1 (depth moved)
    Bp = H("ff742408" "ff742408" "e800000000" "83c408" "c3")   # push a2; push a1
    reject(Ap, Bp, ra=calltok(Ap, "?g@@YAXHH@Z"), rb=calltok(Bp, "?g@@YAXHH@Z"))
    accept(Ap, Ap, ra=calltok(Ap, "?g@@YAXHH@Z"), rb=calltok(Ap, "?g@@YAXHH@Z"))


def test_register_argument_of_thiscall_rejected():
    At = H("33c9" "33d2" "e800000000" "c3")             # xor ecx,ecx; xor edx,edx; call C::f (thiscall)
    Bt = H("33d2" "33c9" "e800000000" "c3")
    accept(At, Bt, ra=calltok(At, "?f@C@@QAEXXZ"), rb=calltok(Bt, "?f@C@@QAEXXZ"))   # order is free
    Bt2 = H("33c0" "33d2" "e800000000" "c3")            # ecx not set
    reject(At, Bt2, ra=calltok(At, "?f@C@@QAEXXZ"), rb=calltok(Bt2, "?f@C@@QAEXXZ"))


def test_call_target_bijection():
    two = H("e800000000" "e800000000" "c3")
    ff = calltok(two, "?f@@YAXXZ")
    gg = calltok(two, "?g@@YAXXZ")
    fg = dict(ff)
    fg[6] = ("sym", "?g@@YAXXZ", 0x14, 0)
    accept(two, two, ra=ff, rb=gg)                     # a consistent renaming of symbols (as the byte check trusts)
    reject(two, two, ra=ff, rb=fg)                     # one callee in A, two in B


# escaping frames: one translation of the whole frame
EA = H("83ec08" "8d0424" "50" "e800000000" "83c404" "8b0424" "83c408" "c3")          # f(&local); return local
EB = H("83ec0c" "8d0424" "50" "e800000000" "83c404" "8b0424" "83c40c" "c3")          # everything 4 deeper
EBAD = H("83ec0c" "8d0424" "50" "e800000000" "83c404" "8b442404" "83c40c" "c3")      # reads another slot
P1 = "?f@@YAXPAH@Z"


def test_escaping_frame_uniform_shift_accepted():
    why = accept(EA, EB, ra=calltok(EA, P1), rb=calltok(EB, P1))
    assert why["mode"] == "delta" and why["delta"] == {False: -4}


def test_escaping_frame_other_slot_rejected():
    reject(EA, EBAD, ra=calltok(EA, P1), rb=calltok(EBAD, P1))


def test_stack_depth_must_be_zero_at_ret():
    reject(H("50" "c3"), H("50" "c3"))                 # push eax; ret: depth 4 at ret


# ebp frame with an aligned area (push ebp; mov ebp, esp; and esp, -8): [ebp+8] is the first argument
EBA = H("55" "8bec" "83e4f8" "83ec08" "8b4508" "890424" "8b0424" "8be5" "5d" "c3")
EBB = H("55" "8bec" "83e4f8" "83ec10" "8b4508" "89442408" "8b442408" "8be5" "5d" "c3")
EBX = H("55" "8bec" "83e4f8" "83ec10" "8b4508" "89442408" "8b442404" "8be5" "5d" "c3")   # reads an unwritten slot


def test_ebp_frame_aligned_slots():
    accept(EBA, EBB)
    reject(EBA, EBX)


def test_lea_esp_is_a_no_op_and_wl_marker():
    wl = H("eb10") + b"WL  " + H("01000000") + H("00000000") + b"WL  "
    wl2 = H("eb10") + b"WL  " + H("02000000") + H("00000000") + b"WL  "
    accept(H("8d2424") + wl + SA, H("8d2424") + wl + SB)
    reject(wl + SA, wl2 + SA)                          # another marker id


# --- value flow: instructions that only move values (LAYERS.md "rename: value flow") ------------------------------
# return a1 + 1; ours copies through ecx (one instruction more)
VC_A = H("8b442404" "83c001" "c3")                       # mov eax,[esp+4]; add eax,1
VC_B = H("8b4c2404" "89c8" "83c001" "c3")                # mov ecx,[esp+4]; mov eax,ecx; add eax,1
VC_BBAD = H("8b4c2404" "89c8" "83c101" "c3")             # ... add ecx,1: returns a1
# a1 kept across a call: the original in esi (saved), ours spilled to a slot that push ecx allocated
F0 = "?f@@YAXXZ"
SP_A = H("56" "8b742408" "e800000000" "89f0" "5e" "c3")
SP_B = H("51" "8b442408" "890424" "e800000000" "8b0424" "59" "c3")
SP_BCLOB = H("51" "8b442408" "890424" "e800000000" "890c24" "8b0424" "59" "c3")   # slot overwritten before reload
SP_BNOSPILL = H("51" "8b442408" "e800000000" "8b0424" "59" "c3")                  # reload without the spill
SP_BREG = H("51" "8b442408" "890424" "e800000000" "59" "c3")                      # eax used across the call
# the spill slot left below esp before the call (add esp, 4) and read after re-allocating it: the callee's frame
# overwrote it (its return address goes exactly there)
SP_BSTALE = H("8b442404" "50" "83c404" "e800000000" "83ec04" "8b0424" "83c404" "c3")
# a1 passed to g(int) and returned: ours pushes the argument from eax and keeps a1 in a slot
G1 = "?g@@YAXH@Z"
AR_A = H("56" "8b742408" "56" "e800000000" "83c404" "89f0" "5e" "c3")
AR_B = H("51" "8b442408" "890424" "50" "e800000000" "83c404" "8b0424" "59" "c3")
AR_BARG = H("8b442404" "50" "e800000000" "8b0424" "83c404" "c3")   # reads its argument slot back: the callee owns it
# a loop: acc += i for i from a1 to 99; ours keeps i in ecx and a copy in edx
LP_A = H("8b442404" "31d2" "01c2" "40" "83f864" "7cf8" "89d0" "c3")
LP_BOK = H("8b4c2404" "31c0" "89ca" "01d0" "41" "89ca" "83f964" "7cf6" "c3")
LP_BBAD = H("8b4c2404" "31c0" "89ca" "01d0" "89ca" "41" "83f964" "7cf6" "c3")   # the copy before the increment
# a1*a1 + a1 through a spill: the original reloads once and copies, ours reloads twice; another frame shape
RL_A = H("83ec04" "8b442408" "890424" "8b0424" "89c1" "0fafc8" "01c8" "83c404" "c3")
RL_B = H("83ec04" "8b442408" "890424" "8b0c24" "8b0424" "0fafc8" "01c8" "83c404" "c3")
RL_BBAD = H("83ec04" "8b442408" "890424" "8b0c24" "c7042405000000" "8b0424" "0fafc8" "01c8" "83c404" "c3")
FR_B = H("51" "51" "8b44240c" "89442404" "8b442404" "89c1" "0fafc8" "01c8" "83c408" "c3")
# a1 ? a2 : ecx at the join; ours merges the value in a frame slot
PH_A = H("8b442404" "85c0" "7404" "8b4c2408" "89c8" "c3")
PH_B = H("51" "8b442408" "85c0" "7407" "8b44240c" "890424" "8b0424" "59" "c3")
# escaped frame (delta mode): moving a value through the frame is not transparent there
P1E = "?f@@YAXPAH@Z"
ES_A = H("83ec08" "8d0424" "50" "e800000000" "83c404" "8b0424" "83c408" "c3")
ES_B = H("83ec08" "8d0424" "50" "e800000000" "83c404" "8b0c24" "894c2404" "8b442404" "83c408" "c3")
# xor eax, eax whose flags are read stays an instruction to pair
XF_A = H("31c0" "7401" "40" "c3")
XF_B = H("b800000000" "7401" "40" "c3")


def test_value_copy_accepted():
    why = accept(VC_A, VC_B)
    assert why["transparent"] == [0, 1]
    reject(VC_A, VC_BBAD)


def test_spill_reload_across_call_accepted():
    accept(SP_A, SP_B, ra=calltok(SP_A, F0), rb=calltok(SP_B, F0))
    reject(SP_A, SP_B, level=2, ra=calltok(SP_A, F0), rb=calltok(SP_B, F0))     # ecx differs at the ret


@pytest.mark.parametrize("bad", [SP_BCLOB, SP_BNOSPILL, SP_BREG, SP_BSTALE])
def test_spill_reload_negatives(bad):
    reject(SP_A, bad, ra=calltok(SP_A, F0), rb=calltok(bad, F0))


def test_spill_kept_apart_from_call_arguments():
    accept(AR_A, AR_B, ra=calltok(AR_A, G1), rb=calltok(AR_B, G1))
    reject(AR_A, AR_BARG, ra=calltok(AR_A, G1), rb=calltok(AR_BARG, G1))


def test_loop_copy_placement():
    accept(LP_A, LP_BOK)
    why = reject(LP_A, LP_BBAD)                      # a copy of the previous iteration's value: the merges differ
    assert "merged" in why


def test_extra_reload_of_unchanged_slot():
    accept(RL_A, RL_B)
    accept(RL_A, FR_B)                               # push ecx; push ecx vs sub esp, 4
    reject(RL_A, RL_BBAD)                            # the slot overwritten between the reloads


def test_value_merged_in_frame_slot():
    accept(PH_A, PH_B)


def test_escaped_frame_moves_not_transparent():
    accept(ES_A, ES_A, ra=calltok(ES_A, P1E), rb=calltok(ES_A, P1E))
    reject(ES_A, ES_B, ra=calltok(ES_A, P1E), rb=calltok(ES_B, P1E))


def test_read_flags_of_zeroing_stay_paired():
    accept(XF_A, XF_A)
    reject(XF_A, XF_B)


# --- CFG map: blocks paired along the successors, not by layout (LAYERS.md "rename: CFG map") ---------------------
CF_A = H("8b442404" "85c0" "7406" "b801000000" "c3" "b802000000" "c3")      # a1 == 0 ? 2 : 1
CF_BINV = H("8b442404" "85c0" "7506" "b802000000" "c3" "b801000000" "c3")   # jne, the bodies swapped
CF_BBAD = H("8b442404" "85c0" "7506" "b801000000" "c3" "b802000000" "c3")   # jne, the bodies not swapped
CF_BFWD = H("8b442404" "85c0" "7402" "eb06" "b802000000" "c3" "b801000000" "c3")   # the fall-through via a jmp block
CF_BCOND = H("8b442404" "85c0" "7c06" "b801000000" "c3" "b802000000" "c3")  # jl: another condition
# a diamond's arm moved after the join (jne to it, jmp back)
CJ_A = H("8b4c2404" "31c0" "85c9" "7405" "b805000000" "c3")
CJ_B = H("8b4c2404" "31c0" "85c9" "7501" "c3" "b805000000" "ebf8")
CJ_BBAD = H("8b4c2404" "31c0" "85c9" "7501" "c3" "b806000000" "ebf8")
CJ_BNOP = H("8b4c2404" "31c0" "85c9" "7501" "c3" "ebfd")      # the arm only jumps back: no mov eax, 5 at all


def test_cfg_inverted_condition_and_layout():
    why = accept(CF_A, CF_BINV)
    assert why["inverted"] == 1
    accept(CF_A, CF_BFWD)
    accept(CJ_A, CJ_B)
    for bad in (CF_BBAD, CF_BCOND):
        reject(CF_A, bad)
    reject(CJ_A, CJ_BBAD)
    reject(CJ_A, CJ_BNOP)          # A's one-instruction arm (a transparent mov) is no forwarder


def test_cfg_unreached_blocks_still_compared():
    """a block no edge reaches (EH code entered through an address in data) is paired in layout order and compared"""
    U = H("8b442404" "c3" "83c001" "cc")                       # ret; then add eax, 1; int3 nobody jumps to
    accept(U, U)
    reject(U, H("8b442404" "c3" "83c002" "cc"))
    reject(U, H("8b442404" "c3"))


@pytest.mark.parametrize("a,b", [(CF_A, CF_BINV), (CF_A, CF_BFWD), (CJ_A, CJ_B)])
def test_cfg_map_mutants_rejected(a, b):
    for desc, m in mutate.mutants(Func(0, b, {}, "t", "ours"), 40, seed=1):
        ok, why = RC.check(a, 0x1000, {}, m.code, 0, m.relocs, level=1)
        assert not ok, "mutant accepted: %s" % desc


def test_image_address_immediates():
    """an original field holding an image address against our plain constant: equal only with the same value
    (paired: add eax, imm; moved: mov eax, imm / push imm through a slot)"""
    tA = {1: ("abs", 0x500000)}
    for code_a, code_b, code_bad in ((H("0500005000c3"), H("0500005000c3"), H("0504005000c3")),
                                     (H("b800005000c3"), H("b800005000c3"), H("b804005000c3")),
                                     (H("6800005000" "58" "c3"), H("6800005000" "58" "c3"), H("6804005000" "58" "c3"))):
        accept(code_a, code_b, ra=tA)
        reject(code_a, code_bad, ra=tA)
    sym = {1: ("sym", "?g@@3HA", 6, 0)}
    accept(H("b800005000c3"), H("b800000000c3"), ra=tA, rb=sym)
    reject(H("b800005000c3"), H("b800000000c3"), ra=tA, rb={})          # a symbol against a plain 0


def test_no_transparency_restores_one_to_one():
    RC.TRANSPARENT = False
    try:
        assert "instruction count" in reject(VC_A, VC_B)
    finally:
        RC.TRANSPARENT = True


@pytest.mark.parametrize("a,b,name", [(VC_A, VC_B, None), (SP_A, SP_B, F0), (AR_A, AR_B, G1), (LP_A, LP_BOK, None),
                                      (PH_A, PH_B, None)])
def test_value_flow_mutants_rejected(a, b, name):
    ra = calltok(a, name) if name else {}
    rb = calltok(b, name) if name else {}
    accept(a, b, ra=ra, rb=rb)
    ms = mutate.mutants(Func(0, b, rb, "t", "ours"), 40, seed=1)
    assert ms
    for desc, m in ms:
        ok, why = RC.check(a, 0x1000, ra, m.code, 0, m.relocs, level=1)
        assert not ok, "mutant accepted: %s" % desc


# --- the mutation self-test bites ----------------------------------------------------------------------------------
@pytest.mark.parametrize("code,rel", [(SA, {}), (A2, {}), (QA, {}), (CA, "f")])
def test_every_mutant_rejected(code, rel):
    rel = calltok(code, F1, 0) if rel == "f" else rel
    f = Func(0, code, rel, "t", "ours")
    accept(code, code, ra=rel, rb=rel)
    ms = mutate.mutants(f, 20, seed=1)
    assert ms
    for desc, m in ms:
        ok, why = RC.check(code, 0x1000, rel, m.code, 0, m.relocs, level=1)
        assert not ok, "mutant accepted: %s" % desc


# --- callconv ------------------------------------------------------------------------------------------------------
@pytest.mark.parametrize("name,conv,args,pops", [
    ("?f@@YAXHH@Z", "cdecl", 8, 0),
    ("?f@C@@QAEXH@Z", "thiscall", 4, 4),
    ("??0C@@QAE@XZ", "thiscall", 0, 0),
    ("?f@@YGXN@Z", "stdcall", 8, 8),
    ("?g@@YAXPAUv@@0@Z", "cdecl", 8, 0),
    ("?h@@YA?AUv@@H@Z", "cdecl", 8, 0),
    ("?f@?$vec@H@std@@QAEXABH@Z", "thiscall", 4, 4),
    ("?m@C@@QAEXPAV?$vector@HV?$allocator@H@std@@@std@@_J@Z", "thiscall", 12, 12),
    ("_malloc", "cdecl", 4, 0),
    ("_f@12", "stdcall", 12, 12),
    ("??_GC@@UAEPAXI@Z", "thiscall", 4, 4),
])
def test_callconv_parse(name, conv, args, pops):
    p = callconv.parse(name)
    assert p == {"conv": conv, "args": args, "pops": pops}


@pytest.mark.parametrize("name", ["?k@@YAXUv@@@Z", "?x@@YAXHZZ", "_unknown_c_function", "??_C@_03ABC@abc?$AA@"])
def test_callconv_unknown(name):
    assert callconv.parse(name) is None


def test_commutative_address_and_or_minus_one():
    La = H("8b442404" "8b542408" "8d3c10" "8bc7" "c3")      # eax=a1; edx=a2; lea edi,[eax+edx]; mov eax,edi
    Lb = H("8b442404" "8b542408" "8d3c02" "8bc7" "c3")      # lea edi,[edx+eax]
    Lc = H("8b442404" "8b542408" "8d3c50" "8bc7" "c3")      # lea edi,[eax+edx*2]: another address
    accept(La, Lb)
    reject(La, Lc)
    accept(H("83c8ff" "c3"), H("83c8ff" "c3"))
    Oa = H("8b4c2404" "83c8ff" "03c1" "c3")                 # ecx=a1; eax=-1 (or); eax+=ecx
    Ob = H("8b4c2404" "83c8fe" "03c1" "c3")                 # or eax,-2 keeps bits of eax
    reject(Oa, Ob)


def test_crt_64bit_helper_interfaces():
    """__allshl & co. take edx:eax and cl; their plain C names used to read as 'no register arguments', so a moved
    shift-count load proved (frame-model self-test, perturbation variant e4e84cd35253_01)."""
    import angr_layer as AL
    import rename_core as RC
    for nm in ("__allshl", "__allshr", "__aullshr"):
        assert set(AL.name_facts(nm)["reads"]) == {"eax", "ecx", "edx"}
        assert set(RC.conv_reads(("sym", nm))) == {"eax", "ecx", "edx"}
    for nm in ("__allmul", "__alldiv", "__allrem", "__aulldiv", "__aullrem"):
        assert AL.name_facts(nm)["W"] == 16
    assert AL.name_facts("__aulldvrm").get("kills") == ["ebx"]
