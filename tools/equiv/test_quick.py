"""Tests of the quick check's parts (tools/equiv/quick.py, cexmodel.py, cexreplay.py) on hand-assembled pairs, as
test_angr_layer.py does (no game image: the image hooks are replaced by a fake), plus one real row when the game
image is there. The angr parts need work/angr-venv (or $EQUIV_ANGR_PYTHON); the replay needs unicorn.

  uv run pytest tools/equiv/test_quick.py -q
"""
import json
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import angr_layer as AL  # noqa: E402
import cexmodel  # noqa: E402
import cexreplay  # noqa: E402
import func as FN  # noqa: E402
import quick  # noqa: E402
from func import Func  # noqa: E402

H = bytes.fromhex
RVA = 0x1000
TMO = 60
needs_angr = pytest.mark.skipif(AL._angr_python() is None, reason="angr venv missing")


class FakeImage:
    base, size = 0x400000, 0x100000

    def read(self, va, n):
        raise ValueError("unmapped")

    def in_text(self, va):
        return False

    def func_extent(self, va):
        raise ValueError("no index")


@pytest.fixture
def fake_image(monkeypatch):
    monkeypatch.setattr(FN, "_img", FakeImage())


def R(code):
    return Func(RVA, code, {}, "retail", "retail", {})


def O(code, symbol=""):
    return Func(0, code, {}, "ours", "ours", {"symbol": symbol, "consts": {}})


INT2 = "?add@@YAHHH@Z"
ADD = H("8b4c2404" "8b542408" "8d0411" "c3")                    # return a + b
ADD1 = H("8b542404" "8b4c2408" "8d440a01" "c3")                  # return a + b + 1
# this->x = a; return  (thiscall, one argument): mov eax,[esp+4]; mov [ecx+8],eax; ret 4
STORE = H("8b442404" "894108" "c20400")
STORE_OFF = H("8b442404" "89410c" "c20400")                      # stores into this+0xc instead
STORE_SYM = "?set@A@@QAEXH@Z"


# ------------------------------------------------------------------------------------------ words (no angr)
def test_what_words():
    assert quick.what_words("ret#0 eax") == "the return value (eax) differs"
    assert quick.what_words("call#1 target") == "call #2 goes to a different function"
    assert quick.what_words("ret#2 esi") == "esi is not restored (callers rely on it)"
    assert "call #1" in quick.what_words("call#0 ecx") and "`this`" in quick.what_words("call#0 ecx")
    assert quick.explain({"what": "ret#0 eax, ret#0 esi"}) == ["the return value (eax) differs",
                                                               "esi is not restored (callers rely on it)"]


def test_parse_cex():
    d = 'counterexample in the model, not trusted (x): difference: {"what":"ret#0 eax","in_ecx":"0x1"} | L1: ...'
    assert cexmodel.parse_cex(d) == {"what": "ret#0 eax", "in_ecx": "0x1"}
    assert cexmodel.parse_cex("timeout after 15s") is None


def test_ordinal():
    assert [quick.ordinal(n) for n in (1, 2, 3, 4, 11, 12, 13, 21, 22)] == \
        ["1st", "2nd", "3rd", "4th", "11th", "12th", "13th", "21st", "22nd"]


def test_call_count_note(fake_image):
    # the original calls a function twice, ours once
    a = H("e8 00000000 e8 00000000 c3".replace(" ", ""))
    b = H("e8 00000000 c3".replace(" ", ""))
    ra, ob = R(a), O(b)
    note = quick.call_count_note(ra, ob, None)
    assert note and note.startswith("the original makes 2 calls, ours 1")
    assert quick.call_count_note(ra, O(a), None) is None


# ------------------------------------------------------------------------------------------ angr + replay
@needs_angr
def test_refuted_with_model_and_replay_confirms(fake_image):
    r, job = cexmodel.prove(R(ADD), O(ADD1, symbol=INT2), "L1", TMO)
    assert r["status"] == "refuted"
    c = r["cex"]
    assert c["what"].startswith("ret#0 eax") and "model" in c and "in_esp" in c["model"]["bv"]
    assert '"model"' not in r["detail"]                     # the detail line stays small
    rp = cexreplay.replay(job, c)
    assert rp["status"] == "confirmed", rp
    d = rp["diff"]
    assert d["kind"] == "reg" and d["reg"] == "eax" and (d["ours"] - d["orig"]) & 0xffffffff == 1
    assert d["producer"]["retail"] == RVA + 8 and d["producer"]["ours"] == RVA + 8     # the two lea
    words = quick.describe_replay(rp, {"job": job, "cex": c, "src": "x.cpp", "lines": {"8": [12, "return a+b;"]}})
    assert any("x.cpp:12" in w for w in words) and words[0].startswith("the return value (eax) differs")


@needs_angr
def test_replay_memory_difference(fake_image):
    r, job = cexmodel.prove(R(STORE), O(STORE_OFF, symbol=STORE_SYM), "L1", TMO)
    assert r["status"] == "refuted" and "memory" in r["cex"]["what"]
    rp = cexreplay.replay(job, r["cex"])
    assert rp["status"] == "confirmed", rp
    assert rp["diff"]["kind"] == "mem"


@needs_angr
def test_equivalent_pair_proves_without_cex(fake_image):
    ADD_REN = H("8b542404" "8b4c2408" "8d040a" "c3")
    r, job = cexmodel.prove(R(ADD), O(ADD_REN, symbol=INT2), "L1", TMO)
    assert r["status"] == "proved" and "cex" not in r


def test_replay_not_reproduced_when_equal(fake_image):
    """a model that does not separate the versions: the replay must say so, never confirm"""
    pytest.importorskip("unicorn")
    job = {"level": "L1", "facts": {}, "ret64": False, "void_eax_dead": False, "argw": 8,
           "sides": {"retail": {"code": ADD.hex(), "base": RVA},
                     "ours": {"code": H("8b542404" "8b4c2408" "8d040a" "c3").hex(), "base": RVA}}}
    cex = {"what": "ret#0 eax", "model": {"bv": {"in_esp": "0x100000"}, "arr": {"M0": [[[0x100004, 7]], 1]}}}
    rp = cexreplay.replay(job, cex)
    assert rp["status"] == "not-reproduced"


# ------------------------------------------------------------------------------------------ a real row
@pytest.mark.skipif(not os.path.exists(os.path.join(ROOT, "work", "mb.analysis.bin")), reason="needs the game image")
@needs_angr
def test_real_row_likely_equivalent():
    """0x43dcb0 (set_goal_point) is an angr proof in docs/equiv_proofs.json: the quick check says so, in seconds"""
    row = json.load(open(os.path.join(ROOT, "docs", "equiv_proofs.json")))["rows"].get("0043dcb0")
    if not row or row["layer"] != "angr":
        pytest.skip("no longer an angr proof")
    out = []
    r = quick.run(0x43dcb0, row["src"], row["symbol"], log=out.append, emu=False)
    assert r["verdict"] in ("likely-equivalent", "equivalent", "match"), out
