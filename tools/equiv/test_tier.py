"""The PROVEN-O tier (tools/equiv/LAYERS.md "Status"): a frame-model proof is recorded with its own tier and every
reader keeps it apart from the unconditional proofs. No angr, no compile (prove.py's own frame-model run is in
test_frame.py, test_prove_frame_model_*).

  uv run pytest tools/equiv/test_tier.py -q
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
import func as FN  # noqa: E402
import prove  # noqa: E402
import sweep  # noqa: E402
import verdicts  # noqa: E402


class NoImage:
    def func_extent(self, va):
        raise ValueError("no index")


def _bundle(fm):
    b = {"va": "00401000", "level": "L1", "build": "ab" * 32, "src": "src/x.cpp", "symbol": "?f@@YAXXZ",
         "flags": "", "layer": "region", "hints": {}, "tools": {}}
    if fm:
        b.update(frame_model=True, assumes=list(prove.FRAME_ASSUMES), tier="PROVEN-O")
    return b


def test_record_tier(tmp_path, monkeypatch):
    monkeypatch.setattr(prove, "PROOFS", str(tmp_path / "equiv_proofs.json"))
    monkeypatch.setattr(prove, "BUNDLES", str(tmp_path / "b"))
    monkeypatch.setattr(prove, "BIG_BUNDLES", str(tmp_path / "big"))
    monkeypatch.setattr(FN, "_img", NoImage())
    doc = {"rows": {}}
    prove.record(0x401000, {"verdict": "EQUIVALENT-O", "bundle": _bundle(True)}, "src/x.cpp", doc, quiet=True)
    prove.record(0x401010, {"verdict": "EQUIVALENT", "bundle": _bundle(False)}, "src/x.cpp", doc, quiet=True)
    o, p = doc["rows"]["00401000"], doc["rows"]["00401010"]
    assert o["tier"] == "PROVEN-O" and o["assumes"] == ["O", "T", "I"]
    assert "tier" not in p and "assumes" not in p
    # a row proved again without the frame model loses its tier (the row is rewritten whole)
    prove.record(0x401000, {"verdict": "EQUIVALENT", "bundle": _bundle(False)}, "src/x.cpp", doc, quiet=True)
    assert "tier" not in doc["rows"]["00401000"]


def test_record_refuses_unmarked_o():
    try:
        prove.record(0x401000, {"verdict": "EQUIVALENT-O", "bundle": _bundle(False)}, "src/x.cpp", {"rows": {}},
                     quiet=True)
    except SystemExit:
        return
    raise AssertionError("an EQUIVALENT-O result without a frame-model bundle was recorded")


def test_verdicts_category_and_apply(tmp_path, monkeypatch):
    assert verdicts.category({"verdict": "PROVEN-O"}) == "proven_o"
    assert verdicts.category({"verdict": "PROVEN"}) == "proven"
    assert "proven_o" in verdicts.CATS
    proofs = tmp_path / "p.json"
    proofs.write_text(json.dumps({"rows": {
        "00401000": {"layer": "region", "level": "L1", "tier": "PROVEN-O", "assumes": ["O", "T", "I"]},
        "00401010": {"layer": "angr", "level": "L1"},
        "00401020": {"layer": "region", "level": "L1", "tier": "PROVEN-O"}}}))
    monkeypatch.setattr(verdicts, "PROOFS", str(proofs))
    rows = {"00401000": {"verdict": "WEAK", "category": "weak", "size": 4},
            "00401010": {"verdict": "PASS", "category": "pass", "size": 4},
            "00401020": {"verdict": "FAIL", "category": "fail", "size": 4},
            "00401030": {"verdict": "PROVEN-O", "category": "proven_o", "tested": "WEAK", "proof": "x", "size": 4}}
    assert verdicts.apply_proofs(rows) == 2
    assert rows["00401000"]["verdict"] == "PROVEN-O" and rows["00401000"]["category"] == "proven_o"
    assert rows["00401000"]["tested"] == "WEAK" and "assuming O,T,I" in rows["00401000"]["proof"]
    assert rows["00401010"]["category"] == "proven"
    assert rows["00401020"]["category"] == "fail"                   # a FAIL beside a proof stays FAIL
    assert rows["00401030"]["category"] == "weak"                   # a withdrawn PROVEN-O proof
    s = verdicts.summarize((r["category"], r["size"]) for r in rows.values())
    assert s["proven_o"] == 1 and s["proven"] == 1


def test_sweep_summary_counts_o_apart():
    rows = [{"va": "00401000", "verdict": "EQUIVALENT-O", "level": "L1", "layer": "region",
             "layers": {"region": ["unknown", "x", 1], "region-O": ["proved", "y", 1]}},
            {"va": "00401010", "verdict": "EQUIVALENT", "level": "L1", "layer": "angr", "layers": {}}]
    out = sweep.summary(rows)
    assert "EQUIVALENT-O (tier PROVEN-O" in out and "EQUIVALENT by layer: angr 1" in out
    assert "layer region-O: proved 1" in out
