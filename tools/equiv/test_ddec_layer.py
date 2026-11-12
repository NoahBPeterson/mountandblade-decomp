"""Tests of the DDEC layer (tools/equiv/ddec_layer.py) on synthetic pairs.

Ported in spirit from spore-decomp tools/equiv/test_ddec_layer.py (commit 82de4ab6c), whose cases are spore
functions (Raster::Initialize and its mutants); these are hand-assembled pairs with the same expectations: a loop
proved with learned invariants, rechecked from its hints, a wrong hint and mutants not proved, L1 vs L2 at the
return, and x87 refused. Needs Docker with the image mb-stoke-ddec:98d8a0f-1 (tools/equiv/ddec/Dockerfile) and
unicorn; skipped otherwise. Slow (STOKE runs under amd64 emulation on an arm64 Mac): about a minute per case.

  uv run pytest tools/equiv/test_ddec_layer.py -q
"""
import copy
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from func import Func  # noqa: E402

try:
    import ddec_layer as DL  # noqa: E402
    HAVE = DL._image_ok()
except ImportError:
    DL, HAVE = None, False
pytestmark = pytest.mark.skipif(not HAVE, reason="Docker or the STOKE image is missing (ddec_layer.py)")
H = bytes.fromhex
TMO = 600


def R(code):
    return Func(0x1000, code, {}, "retail", "retail", {})


def O(code, ret64=False):
    return Func(0, code, {}, "ours", "ours", {"ret64": ret64})


# mov ecx,[esp+4]; xor eax,eax; L: add eax,ecx; dec ecx; jnz L; ret     (the sum n + (n-1) + ... + 1, n > 0)
SUM = H("8b4c2404" "31c0" "01c8" "49" "75fb" "c3")
SUM_REN = H("8b542404" "31c0" "01d0" "4a" "75fb" "c3")          # the same with edx
SUM_BAD = H("8b542404" "31c0" "8d440201" "4a" "75f9" "c3")       # lea eax,[edx+eax+1]: one more per step

ADD = H("8b4c2404" "8b542408" "8d0411" "c3")
ADD_REN = H("8b542404" "8b4c2408" "8d040a" "c3")


def prove(a, b, level="L1", hints=None):
    r = DL.prove(a, b, level, hints, TMO)
    print(r["status"], r["seconds"], r["detail"][:300])
    return r


@pytest.fixture(scope="module")
def sum_proof():
    return prove(R(SUM), O(SUM_REN))


def test_loop_proved(sum_proof):
    assert sum_proof["status"] == "proved"


def test_loop_hints_recheck(sum_proof):
    assert sum_proof["status"] == "proved"
    assert prove(R(SUM), O(SUM_REN), hints=sum_proof["hints"])["status"] == "proved"


def test_loop_bad_hints(sum_proof):
    h = copy.deepcopy(sum_proof["hints"])
    changed = False
    for s in h["segs"]:
        for c in s["cuts"]:
            for a, cs in c["inv"].items():
                for k, x in enumerate(cs):
                    if not changed and x.endswith(" = 0x0"):
                        cs[k], changed = x[:-1] + "1", True
    if not changed:
        pytest.skip("no equality invariant to change")
    assert prove(R(SUM), O(SUM_REN), hints=h)["status"] != "proved"


def test_loop_mutant_not_proved():
    r = prove(R(SUM), O(SUM_BAD))
    assert r["status"] != "proved" and "no instruction" not in r["detail"]


def test_return_L1_proved_L2_not():
    # ecx/edx are swapped at the return: invisible at L1, visible at L2
    assert prove(R(ADD), O(ADD_REN))["status"] == "proved"
    assert prove(R(ADD), O(ADD_REN), "L2")["status"] != "proved"


def test_x87_unsupported():
    a = H("d9442404" "d84c2408" "c3")
    r = prove(R(a), O(H("8bcc" "d94104" "d84908" "c3")))
    assert r["status"] == "unknown" and "unsupported" in r["detail"]
