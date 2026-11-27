"""Tests for tools/equiv/cexreplay.py's classification helpers (frame-only differences, rel32 stubs).

  uv run pytest tools/equiv/test_cexreplay.py -q
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cexreplay as CR  # noqa: E402


class Fake:
    def __init__(self, esp0, calls):
        self.esp0 = esp0
        self.events = [{"kind": "call", "esp": esp0 - d} for d in calls]


def test_frame_delta_constant_is_frame_only():
    A, B = Fake(0x1000, [0x20, 0x30, 0x24]), Fake(0x1000, [0x28, 0x38, 0x2c])
    assert CR.frame_deltas(A, B) == {8}


def test_frame_delta_varying_is_not_frame_only():
    # a call that sees a different depth than the others: an extra push on one path, not a frame size
    A, B = Fake(0x1000, [0x20, 0x30]), Fake(0x1000, [0x28, 0x3c])
    assert len(CR.frame_deltas(A, B)) == 2


def test_frame_delta_wraps():
    A, B = Fake(0x10, [0x20]), Fake(0x10, [0x24])
    assert CR.frame_deltas(A, B) == {4}
    A, B = Fake(0xfffffff0, [0x20]), Fake(0xfffffff0, [0x24])
    assert CR.frame_deltas(A, B) == {4}
