"""An indirect call through a vtable read from the null page after the input's failed `new` (0x501a20, inventory_window)
is undefined behaviour in both builds; their soup targets differ with the frame layout (ff5db87d vs ff5dc87d), so the
input is discarded as call-null-vtable, never compared as a FAIL. Needs the saved failing input under work/difftest."""
import os
import pickle
import pytest

import equiv

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = "src/game/ui/screens/inventory/inventory_window.cpp"
PKL = os.path.join(ROOT, "work", "difftest", "fail_inputs", "inventory_window_00501a20.pkl")


@pytest.mark.skipif(not os.path.exists(PKL), reason="the saved failing input is not present")
def test_failed_new_null_vtable_flags_both_builds_with_different_targets():
    opts = equiv.parse_args(["inventory_window", "501a20", "--src", os.path.join(ROOT, SRC)])
    t = equiv.prepare("inventory_window", 0x501a20, opts.flags, print, opts.src)
    assert not t.verdict, t.reason
    runner = equiv.Runner(t, opts)
    runner.budget = opts.budget
    inp = equiv.Input()
    inp.__dict__.update(pickle.load(open(PKL, "rb")))
    inp.__dict__.setdefault("pagegen", 1)
    a = runner.run_side(runner.orig, inp)
    b = runner.run_side(runner.ours, inp)
    runner.close()
    assert a["wildcall"] and b["wildcall"], (a["wildcall"], b["wildcall"])
    assert [w[0] for w in a["wildcall"]] != [w[0] for w in b["wildcall"]]          # the soup targets differ: the input is discarded
