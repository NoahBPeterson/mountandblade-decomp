"""prove.py --record / --recheck on a real row (needs work/mb.analysis.bin and a compile, normally cached):
the row gets the build hash and the bundle, a recheck with the same build is the hinted check of the recorded
layer, a changed build hash makes it prove again from scratch. Writes only to a temporary directory.

  uv run pytest tools/equiv/test_prove_record.py -q
"""
import json
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import prove  # noqa: E402

VA = 0x005094a0                     # a rename-layer proof in docs/equiv_proofs.json (its source is read from there)
pytestmark = pytest.mark.skipif(not os.path.exists(os.path.join(ROOT, "work", "mb.analysis.bin")),
                                reason="needs work/mb.analysis.bin")


@pytest.fixture
def tmp_record(tmp_path, monkeypatch):
    monkeypatch.setattr(prove, "PROOFS", str(tmp_path / "equiv_proofs.json"))
    monkeypatch.setattr(prove, "BUNDLES", str(tmp_path / "equiv_proofs"))
    monkeypatch.setattr(prove, "BIG_BUNDLES", str(tmp_path / "big"))
    return tmp_path


def _src():
    row = json.load(open(os.path.join(ROOT, "docs", "equiv_proofs.json")))["rows"].get("%08x" % VA)
    if not row:
        pytest.skip("0x%08x is no longer a recorded proof" % VA)
    return row["src"]


def test_record_and_recheck(tmp_record):
    src = _src()
    res = prove.run(os.path.join(ROOT, src), VA, layers=["rename"], log=lambda s: None)
    assert res["verdict"] == "EQUIVALENT"
    prove.record(VA, res, src, quiet=True)
    doc = json.load(open(prove.PROOFS))
    row = doc["rows"]["%08x" % VA]
    assert row["build"] == res["build"] and len(row["build"]) == 64
    assert {"src", "symbol", "layer", "level", "detail", "build", "bundle"} <= set(row)
    b = prove.find_bundle(VA, None)
    assert b["build"] == row["build"] and b["layer"] == "rename"
    r2, how = prove.recheck(VA, row, log=lambda s: None)
    assert r2["verdict"] == "EQUIVALENT" and how.startswith("hinted recheck")
    b["build"] = "0" * 64                                   # as if our build had changed since the proof
    open(prove.bundle_path(VA, True), "w").write(json.dumps(b))
    r3, how = prove.recheck(VA, row, log=lambda s: None)
    assert r3["verdict"] == "EQUIVALENT" and how.startswith("re-proved")
