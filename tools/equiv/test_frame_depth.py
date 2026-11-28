"""The frame model's static stack depth (regions.py): the `ret N` of an INDIRECT call deduced from its pushed
arguments (assumption C, checked by check_pops: every ret at depth 0), the cdecl inference from the compiler's cleanup,
`jmp` thunks, and an ebp frame's `mov esp, ebp`. Pure static tests (no angr)."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import regions as RG  # noqa: E402

H = bytes.fromhex


class _Bij:
    inv = {}
    m = {}


def _side(code):
    s = RG.Side(code, 0x1000, {}, {})
    s.call_target = lambda i: None
    return s


def _depths(code):
    s = _side(code)
    pops = RG._pops(s, _Bij, "retail", indirect=True)
    P = s.provenance(pops, lea_nop=True)
    return s, pops, P


# sub esp,0x10; push esi; mov esi,[esp+0x1c]; lea eax,[esp+8]; push eax; lea eax,[esp+0x14]; push eax;
# mov eax,[esi]; call [eax+8]; pop esi; add esp,0x10; ret 4
def _vcall(extra_push=False):
    c = H("83ec10" "56" "8b74241c" "8d442408" "50" "8d442414" "50")
    c += H("8b06" "ff5008")
    if extra_push:
        c += H("50")                                   # a push after the call that nothing pops
    c += H("5e") + H("83c410" "c2")
    c += H("0400")
    return c


def test_indirect_call_pops_only_its_arguments():
    s, pops, P = _depths(_vcall())
    call = [i for i in s.ins if s.kind[i.address] == "call"][0]
    assert pops(call) == 8                             # the two pushes after `push esi` (a prologue save)
    assert s.assumed_pops == {call.address: 8}
    assert RG.check_pops(s, P) is None


def test_prologue_save_is_not_an_argument():
    # push esi (saved, popped at the end) directly before the argument pushes must not be counted
    s, pops, P = _depths(_vcall())
    assert 8 in s.assumed_pops.values() and 12 not in s.assumed_pops.values()


def test_unbalanced_stack_after_indirect_call_refused():
    # one more push that nothing pops: ret runs at depth != 0 under the assumed cleanup
    s, pops, P = _depths(_vcall(extra_push=True))
    assert RG.check_pops(s, P) is not None


def test_cdecl_cleanup_after_indirect_call_is_not_assumed():
    # call [eax+8]; add esp,8: the cleanup after the call is a cdecl callee's: unknown here (no assumption)
    c = H("8b0424" "50" "50" "ff5008" "83c408" "c3")
    s, pops, P = _depths(c)
    call = [i for i in s.ins if s.kind[i.address] == "call"][0]
    assert pops(call) is None and not s.assumed_pops


def test_ebp_frame_mov_esp_ebp_restores_depth():
    # push ebp; mov ebp,esp; and esp,-8; sub esp,0x10; mov esp,ebp; pop ebp; ret
    c = H("55" "89e5" "83e4f8" "83ec10" "89ec" "5d" "c3")
    s, pops, P = _depths(c)
    ret = [i for i in s.ins if i.mnemonic == "ret"][0]
    assert P[ret.address][2] == 0
