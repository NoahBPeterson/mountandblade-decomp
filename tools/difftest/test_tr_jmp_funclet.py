"""A `jmp` inside a --transparent std function to a label Ghidra split off as a function (vector<tstring>::_Insert_n 0x43e310
-> its epilogue funclet 0x43e575) is not a tail call (the stack is not balanced back to the entry state), so it must not be
recorded as a depth-1 call: its "arguments" were uninitialised stack words that differ with the caller's frame (list_modes
0x4cf100: [0,0,0] vs [10,0,0]). Needs the saved input and the original image under work/."""
import os
import pickle
import pytest

import equiv

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = "src/game/config/launcher_config_dialog.cpp"
PKL = os.path.join(ROOT, "work", "difftest", "fail_inputs", "launcher_config_dialog_004cf100.pkl")
TRANSPARENT = "40c0c0,40c140,40c360,40c450,40c540,40c580,41ef30,42d360,436c90,438a10,438c30,438f00,43a4d0,43b2b0,43e2a0,43e310," \
              "43e7b0,4beb30,4c6f50,4cc680,524cd0,5354ae,535506"


@pytest.mark.skipif(not os.path.exists(PKL), reason="the saved input is not present")
def test_epilogue_funclet_jump_is_not_a_recorded_call():
    opts = equiv.parse_args(["launcher_config_dialog", "4cf100", "--src", os.path.join(ROOT, SRC), "--transparent", TRANSPARENT])
    t = equiv.prepare("launcher_config_dialog", 0x4cf100, opts.flags, print, opts.src)
    assert not t.verdict, t.reason
    runner = equiv.Runner(t, opts)
    runner.budget = opts.budget
    inp = equiv.Input()
    inp.__dict__.update(pickle.load(open(PKL, "rb")))
    inp.__dict__.setdefault("pagegen", 1)
    a = runner.run_side(runner.orig, inp)
    b = runner.run_side(runner.ours, inp)
    first = equiv.Comparer(runner, opts).compare(a, b, t)
    runner.close()
    assert first is None, first      # before the fix: call #16 (depth 1) 0043e575 args [0,0,0] vs [10,0,0]
