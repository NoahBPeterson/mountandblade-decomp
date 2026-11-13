#!/usr/bin/env python3
"""The angr layer's whole-function check with the counterexample's MODEL kept, for a concrete replay
(tools/equiv/cexreplay.py) and for messages in words (tools/equiv/quick.py).

angr_layer.py is used as it is: this module's worker (run in the angr venv, `cexmodel.py --worker`) imports it,
wraps Worker.cex so a counterexample also carries the Z3 model it came from, and then runs angr_layer.worker_main().
The verdict, detail and hints are angr_layer's own; only the counterexample dict gains a "model" entry:

    {"bv": {"in_eax": "0x..", "c0_eax": "0x..", "ra0": .., "rn0": .., "x_probe": .., ...},   every bit-vector constant
     "arr": {"M0": [[[addr, byte], ...], default], "M1": ...}}                                each memory epoch

M<k> is memory after the k-th call's havoc (M0: at entry); c<k>_<reg> are the k-th callee's outputs (shared by both
versions, as in the model). The parent side, prove(), mirrors angr_layer.prove() (prepare, child process with the
angr interpreter, killed at the timeout) and returns the prepared job too: the replay needs the patched code of ours
and the callee facts. The child's interpreter is angr_layer._angr_python() ($EQUIV_ANGR_PYTHON: the laptop through
tools/equiv/remote.py when that is enabled).
"""
import json, os, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)


# ------------------------------------------------------------------ worker side (angr venv)
def _arr_entries(z3, m, v):
    """a model's array value -> [[addr, byte], ...], default byte (or None when its form is not understood)"""
    try:
        if z3.is_as_array(v):
            fi = m[z3.get_as_array_func(v)]
            lst = fi.as_list()
            return [[e[0].as_long(), e[1].as_long()] for e in lst[:-1]], lst[-1].as_long()
        ent = []
        while z3.is_store(v):
            ent.append([v.arg(1).as_long(), v.arg(2).as_long()])
            v = v.arg(0)
        if z3.is_K(v) or z3.is_const_array(v):
            seen, out = set(), []
            for a, b in ent:                  # the outermost store wins
                if a not in seen:
                    seen.add(a)
                    out.append([a, b])
            return out, v.arg(0).as_long()
        if z3.is_quantifier(v) and v.is_lambda():
            body, out = v.body(), []
            while z3.is_app_of(body, z3.Z3_OP_ITE):
                c, t, body = body.arg(0), body.arg(1), body.arg(2)
                if not z3.is_eq(c):
                    return None
                k = c.arg(1) if z3.is_bv_value(c.arg(1)) else c.arg(0)
                out.append([k.as_long(), t.as_long()])
            return out, (body.as_long() if z3.is_bv_value(body) else 0)
    except Exception:
        return None
    return None


def dump_model(z3, m):
    bv, arr = {}, {}
    for d in m.decls():
        if d.arity() != 0:
            continue
        v = m[d]
        nm = d.name()
        try:
            if z3.is_bv_value(v):
                bv[nm] = hex(v.as_long())
            elif d.range().kind() == z3.Z3_ARRAY_SORT:
                e = _arr_entries(z3, m, v)
                if e is not None:
                    arr[nm] = [e[0], e[1]]
        except Exception:
            pass
    return {"bv": bv, "arr": arr}


def worker():
    import angr_layer as AL
    orig = AL.Worker.cex

    def cex(self, q, what, diffs, regions):
        out = orig(self, q, what, diffs, regions)
        try:
            out["model"] = dump_model(self.z3, q.model())
        except Exception as e:
            out["model_error"] = "%s: %s" % (type(e).__name__, e)
        return out
    AL.Worker.cex = cex
    return AL.worker_main()


# ------------------------------------------------------------------ parent side (any interpreter)
def prove(retail, ours, level, timeout, hints=None):
    """angr_layer.prove() with the model kept -> (result, job or None). result["cex"]: the counterexample dict
    (with its "model") when the detail carries one."""
    import angr_layer as AL
    t0 = time.time()
    res = lambda st, d: ({"status": st, "seconds": round(time.time() - t0, 2), "detail": d, "hints": {}}, None)
    if level not in ("L1", "L2"):
        return res("unknown", "level must be L1 or L2")
    py = AL._angr_python()
    if not py:
        return res("unknown", "angr is not installed (tools/equiv/setup_angr_venv.py)")
    try:
        job = AL.prepare(retail, ours, level, hints)
    except AL.HintError as e:
        return res("unknown", str(e))
    except Exception as e:
        return res("unknown", "prepare failed: %s: %s" % (type(e).__name__, e))
    if job["x87"] and not job["x87_norm_same"]:
        return ({"status": "unknown", "seconds": round(time.time() - t0, 2), "hints": {},
                 "detail": "x87 instruction sequences differ (not even the same arithmetic up to stack shuffling): "
                           "the 64-bit x87 model cannot prove it"}, job)
    job["timeout"] = max(1.0, timeout - (time.time() - t0) - 1.0)
    kw = {"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP} if os.name == "nt" else {"start_new_session": True}
    env = dict(os.environ, PYTHONHASHSEED="0")
    p = subprocess.Popen([py, "-I", os.path.abspath(__file__), "--worker"], stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, **kw)
    try:
        out, err = p.communicate(json.dumps(job).encode(), timeout=max(1.0, timeout - (time.time() - t0)))
    except subprocess.TimeoutExpired:
        AL._kill(p)
        return ({"status": "unknown", "seconds": round(time.time() - t0, 2), "hints": {},
                 "detail": "timeout after %.0fs" % timeout}, job)
    except BaseException:
        AL._kill(p)
        raise
    try:
        r = json.loads(out.decode().strip().splitlines()[-1])
    except Exception:
        return ({"status": "unknown", "seconds": round(time.time() - t0, 2), "hints": {},
                 "detail": "worker failed: %s" % err.decode(errors="replace")[-600:]}, job)
    r["seconds"] = round(time.time() - t0, 2)
    c = parse_cex(r.get("detail") or "")
    if c is not None:
        r["cex"] = c
        # the detail line keeps angr_layer's form, without the (large) model
        r["detail"] = r["detail"].replace(json.dumps(c, separators=(",", ":")), json.dumps(
            {k: v for k, v in c.items() if k != "model"}, separators=(",", ":")))
    if job["notes"]:
        r["detail"] = r.get("detail", "") + " [bijection: %s]" % "; ".join(job["notes"])
    return r, job


def parse_cex(detail):
    """the counterexample dict of an angr detail line ('difference: {...} | summary'), or None"""
    k = detail.find("difference: {")
    if k < 0:
        return None
    s = detail[k + len("difference: "):]
    try:
        obj, _end = json.JSONDecoder().raw_decode(s)
        return obj
    except ValueError:
        return None


if __name__ == "__main__":
    if "--worker" in sys.argv:
        sys.exit(worker())
    sys.exit(__doc__)
