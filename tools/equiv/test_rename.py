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
