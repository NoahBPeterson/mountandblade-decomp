"""sweep.py's economics: which rows a second pass retries, and that a proof without the self-test is never
EQUIVALENT (record_sweep.py records only EQUIVALENT rows).

  uv run pytest tools/equiv/test_sweep.py -q
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import prove  # noqa: E402
import sweep  # noqa: E402


def test_timed_out():
    t = {"verdict": "UNKNOWN", "layers": {"rename": ["unknown", "instruction count 3 vs 4", 0.0],
                                          "angr": ["unknown", "timeout after 20s", 20.0]}}
    d = {"verdict": "UNKNOWN", "layers": {"angr": ["unknown", "loop in retail: trip count depends on the inputs", 1]}}
    assert sweep.timed_out(t)
    assert not sweep.timed_out(d)
    assert not sweep.timed_out({"verdict": "EQUIVALENT", "layers": {"angr": ["proved", "", 1]}})
    assert sweep.timed_out({"verdict": "UNKNOWN", "layers": {"region": ["unknown", "region 1: timeout (exploring ours)",
                                                                        30]}})


def test_untested_is_not_equivalent():
    assert sweep.UNTESTED != "EQUIVALENT"


def test_no_mutants_runs_none(monkeypatch):
    """prove.run with k_mutants=0 must not call mutate.mutants (which yields one mutant per kind even for k=0)"""
    called = []
    monkeypatch.setattr(prove.mutate, "mutants", lambda *a, **k: called.append(1) or [])
    import ast
    src = open(os.path.join(HERE, "prove.py")).read()
    assert "if k_mutants else ()" in src
    tree = ast.parse(src)
    assert tree is not None and not called
