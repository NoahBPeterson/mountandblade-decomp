#!/usr/bin/env python3
"""Quick verdict for a function that is not byte-exact: is our source WRONG (fix the logic), or does it only compile
differently (a codegen lever, or prove it with tools/equiv/prove.py)? Seconds, never records anything.

  uv run python tools/equiv/quick.py VA [--src SRC] [--symbol SYM] [--flags "..."] [--angr-timeout 15]
                                        [--emu-timeout 10] [--no-emu] [--no-replay] [--remote] [--json OUT]
  tools/matching/chk.py SRC VA --quick     # the same check after chk's byte score, for each target that does not MATCH

Steps (ported in spirit from spore-decomp tools/equiv/quick.py, commit f6bc57f27):
  0. the call count: when the original and ours make a different NUMBER of calls, it says so first, naming the
     callees that are extra or missing (static, from the code; ours' targets paired with the original's addresses
     as the angr layer pairs relocations).
  1. register renaming (tools/equiv/rename.py, milliseconds): EQUIVALENT when it proves.
  2. angr whole-function (tools/equiv/angr_layer.py through cexmodel.py), capped at --angr-timeout seconds, WITHOUT
     prove.py's mutant self-test:
       proved      "very likely equivalent: run tools/equiv/prove.py SRC VA --record" (that runs the self-test
                   and records the proof)
       refuted     a counterexample the layer trusts: DIFFERS, what differs in words ("the return value (eax)
                   differs", "call #2 goes to a different function", "esi is not restored", ...) and the input
                   values that show it
       untrusted   "LIKELY DIFFERS (unverified: <the guess it rests on>)"
     Every counterexample is replayed concretely (tools/equiv/cexreplay.py: both versions under unicorn on the
     model's input); a replay that shows the difference adds both values, the instruction that produced each
     (original +offset / ours +offset), and our source line (a /FAsc listing of our object), and is saved for
     `uv run python tools/equiv/cexreplay.py work/equiv/cex/<va>.json`.
  3. only when angr did not decide: the emulator test tools/difftest/equiv.py for --emu-timeout seconds (random
     inputs through both builds): a mismatch is a real difference with a replayable input; none is only a hint.
Nothing is written to docs/ or manifests; work/equiv/cex/ keeps the counterexamples, work/equiv/lst/ the listings.
--remote (or EQUIV_REMOTE=1) runs the angr step on the Windows laptop (tools/equiv/remote.py), locally when the
laptop declines.
"""
import argparse, json, os, re, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
CEX_DIR = os.path.join(ROOT, "work", "equiv", "cex")
LST_DIR = os.path.join(ROOT, "work", "equiv", "lst")
PRESERVED = ("ebx", "esi", "edi", "ebp")


def ordinal(n):
    return "%d%s" % (n, "th" if 10 <= n % 100 <= 20 else {1: "st", 2: "nd", 3: "rd"}.get(n % 10, "th"))


# ------------------------------------------------------------------------------------------ names and lines
_NAMES = {}


def name_of(va):
    """the symbols file's name for an original address (demangled when possible), or the hex address"""
    if not _NAMES:
        try:
            import func as FN
            import resolve as R
            db = R.SymbolDB(FN.image())
            for nm, vs in db.mangled.items():
                for v in vs:
                    _NAMES.setdefault(v, nm)
            for nm, v in db.lib.items():
                _NAMES.setdefault(v, nm)
        except Exception:
            _NAMES[None] = None
    nm = _NAMES.get(va)
    return "%s (0x%08x)" % (demangle(nm), va) if nm else "0x%08x" % va


def demangle(nm):
    try:
        import slice as S
        d = S.demangle_qual(nm)
        return d or nm
    except Exception:
        return nm


def listing_lines(src, flags, symbol):
    """{offset in our function: (source line number, source text)} from a /FAsc listing of src (one extra compile,
    cached in work/equiv/lst by source+flags mtime), or {} when it cannot be made"""
    import hashlib
    sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
    try:
        from run_all import CL, INCLUDE, winpath
        import slice as S
    except Exception:
        return {}
    os.makedirs(LST_DIR, exist_ok=True)
    srcp = os.path.join(ROOT, src)
    fl = S.resolve_include_flags(list(flags))
    key = hashlib.sha1(("%s|%s|%s" % (srcp, " ".join(fl), os.path.getmtime(srcp))).encode()).hexdigest()[:12]
    cod = os.path.join(LST_DIR, "%s_%s.cod" % (os.path.basename(src).rsplit(".", 1)[0], key))
    if not os.path.exists(cod):
        tmp = cod + ".%d" % os.getpid()
        obj = tmp + ".obj"
        r = subprocess.run([CL, "/nologo", "/c", *fl, INCLUDE, "/Fo" + winpath(obj), "/FAsc", "/Fa" + winpath(tmp),
                            winpath(srcp)], capture_output=True, text=True, cwd=ROOT)
        for p in (obj,):
            try:
                os.remove(p)
            except OSError:
                pass
        if r.returncode or not os.path.exists(tmp):
            return {}
        os.replace(tmp, cod)
    out, inside, line, first = {}, False, None, None
    lines = open(cod, encoding="latin-1").read().splitlines()
    text = {}
    try:
        text = dict(enumerate(open(srcp, encoding="latin-1").read().splitlines(), 1))
    except OSError:
        pass
    for ln in lines:
        if not inside:
            if ln.startswith(symbol + " PROC"):
                inside = True
            continue
        if ln.startswith(symbol + " ENDP"):
            break
        m = re.match(r"; (\d+)\s+:", ln)
        if m:
            line = int(m.group(1))
            continue
        m = re.match(r"\s+([0-9a-f]{5,8})\t", ln)
        if m and line is not None:
            off = int(m.group(1), 16)
            if first is None:
                first = off
            out[off - first] = (line, text.get(line, "").strip())
    return out


def src_line(lines, off, src):
    """'src.cpp:123: text' for our instruction at off (the nearest listing offset at or before it)"""
    if not lines or off is None:
        return None
    ks = [k for k in lines if k <= off]
    if not ks:
        return None
    ln, txt = lines[max(ks)]
    return "%s:%d: %s" % (src, ln, txt[:100])


def marker_line(src, va):
    """the line of the function's `// @ 0x<va>` marker in our source"""
    try:
        for n, ln in enumerate(open(os.path.join(ROOT, src), encoding="latin-1"), 1):
            if re.search(r"//\s*@\s*0x0*%x\b" % va, ln, re.I):
                return n
    except OSError:
        pass
    return None


# ------------------------------------------------------------------------------------------ the call count
def call_list(code, base):
    """[(offset, target address or None, operand text)] of the call instructions (and jumps out of the function)"""
    import func as FN
    from capstone import x86_const as X
    out = []
    end = base + len(code)
    for i in FN.MD.disasm(code, base):
        if i.mnemonic == "call" or (i.mnemonic == "jmp" and i.operands and i.operands[0].type == X.X86_OP_IMM and
                                    not base <= (i.operands[0].imm & 0xffffffff) < end):
            o = i.operands[0]
            t = o.imm & 0xffffffff if o.type == X.X86_OP_IMM else None
            out.append((i.address - base, t, i.op_str))
    return out


def call_count_note(retail, ours, job):
    """a plain sentence when the two versions make a different number of calls (else None)"""
    rc = call_list(retail.code, retail.va)
    pc = bytes.fromhex(job["sides"]["ours"]["code"]) if job else ours.code
    oc = call_list(pc, retail.va if job else 0)
    if len(rc) == len(oc):
        return None
    from collections import Counter

    def key(t, txt):
        return ("d", t) if t is not None else ("i", re.sub(r"0x[0-9a-f]+", "#", txt))
    ca = Counter(key(t, s) for _o, t, s in rc)
    cb = Counter(key(t, s) for _o, t, s in oc)
    oname = {}
    for off, t, s in oc:
        tok = ours.relocs.get(off + 1) if s and t is not None else None
        if tok:
            oname[("d", t)] = demangle(tok[1])

    def nm(k):
        if k[0] == "i":
            return "an indirect call (%s)" % k[1]
        if k[1] in _names_or_load():
            return name_of(k[1])
        return oname.get(k) or name_of(k[1])
    missing = [(nm(k), n) for k, n in (ca - cb).items()]
    extra = [(nm(k), n) for k, n in (cb - ca).items()]
    parts = ["the original makes %d calls, ours %d" % (len(rc), len(oc))]
    if missing:
        parts.append("missing in ours: " + ", ".join("%s%s" % (a, " x%d" % n if n > 1 else "") for a, n in missing))
    if extra:
        parts.append("extra in ours: " + ", ".join("%s%s" % (a, " x%d" % n if n > 1 else "") for a, n in extra))
    if not missing and not extra:
        parts.append("the same callees, a different number of times")
    return "; ".join(parts)


def _names_or_load():
    if not _NAMES:
        name_of(0)
    return _NAMES


# ------------------------------------------------------------------------------------------ words
def what_words(token, job=None):
    """one difference token of the angr layer ('call#1 target', 'ret#0 esi', ...) -> words"""
    m = re.match(r"(call|ret|diverge|exit)#(\d+)\s+(.*)", token.strip())
    if not m:
        return token.strip()
    kind, k, obs = m.group(1), int(m.group(2)), m.group(3)
    if kind == "call":
        n = "call #%d" % (k + 1)
        if obs == "target":
            return "%s goes to a different function" % n
        if obs == "esp":
            return "%s sees a different stack pointer (other arguments pushed, or a different frame)" % n
        if obs == "memory":
            return "memory differs when %s is made (a stack argument, or a store before the call)" % n
        if obs.startswith("st-slot") or obs in ("ftop", "fptag"):
            return "the x87 stack differs at %s" % n
        if obs == "d":
            return "the direction flag differs at %s" % n
        return "%s gets a different %s (an argument register%s)" % (n, obs, ", `this`" if obs == "ecx" else "")
    if kind == "ret":
        if obs == "eax":
            return "the return value (eax) differs"
        if obs == "edx":
            return "the upper half of the 64-bit return value (edx) differs"
        if obs in PRESERVED:
            return "%s is not restored (callers rely on it)" % obs
        if obs == "esp":
            return "it returns with a different stack pointer (ret N / stack imbalance)"
        if obs == "memory":
            return "the memory it leaves behind differs (a store writes a different value, or one is missing/extra)"
        if obs == "return target":
            return "it tail-calls a different function"
        if obs.startswith("st-slot") or obs in ("ftop", "fptag"):
            return "the x87 result differs (st0 / the x87 stack at return)"
        return "%s differs at the return" % obs
    if kind == "diverge":
        return "memory differs on a path that never returns"
    return token.strip()


def explain(cex):
    what = cex.get("what", "")
    if what.startswith("event sequence"):
        return ["a different sequence of calls and returns on one path (%s)" % what[len("event sequence "):]]
    out = []
    for t in what.split(","):
        if t.strip():
            w = what_words(t)
            if "memory" in w and cex.get("addr"):
                w += " at %s" % cex["addr"]
            if w not in out:
                out.append(w)
    return out


def input_words(cex, job):
    """the counterexample's input in words: registers (ecx as `this` for member functions), stack arguments"""
    regs = {k[3:]: int(v, 16) for k, v in cex.items() if k.startswith("in_")}
    out = []
    for r in ("ecx", "eax", "edx", "ebx", "esi", "edi", "ebp"):
        v = regs.get(r)
        if v:
            out.append("%s=0x%x" % (r, v))
    esp = regs.get("esp")
    model = cex.get("model") or {}
    argw = (job or {}).get("argw") or 0
    if esp is not None and model.get("arr", {}).get("M0") and argw:
        ent, dflt = model["arr"]["M0"]
        mem = {a & 0xffffffff: b for a, b in ent}
        args = []
        for k in range(min(argw // 4, 6)):
            a = esp + 4 + 4 * k
            v = sum((mem.get((a + j) & 0xffffffff, dflt) & 0xff) << (8 * j) for j in range(4))
            args.append("0x%x" % v)
        out.append("stack arguments " + ", ".join(args))
    if esp is not None:
        out.append("esp=0x%x" % esp)
    return ", ".join(out)


def _store_ordinal(code, base, off):
    """'the 3rd store' for the store instruction at off (stores counted in address order)"""
    import func as FN
    from capstone import x86_const as X
    n = 0
    for i in FN.MD.disasm(code, base):
        if i.operands and i.operands[0].type == X.X86_OP_MEM and i.mnemonic not in ("cmp", "test", "push") \
                and not i.mnemonic.startswith(("fld", "fcom", "fild", "fucom")):
            n += 1
            if i.address - base == off:
                return "the %s store" % ordinal(n)
    return "a store"


def _ins_text(code, base, off):
    import func as FN
    i = next(FN.MD.disasm(code[off:off + 16], base + off, 1), None)
    return "%s %s" % (i.mnemonic, i.op_str) if i else "?"


def describe_replay(r, saved):
    """lines in words for a replay result (cexreplay.replay) of a saved counterexample"""
    d = r.get("diff")
    job = saved["job"]
    base = job["sides"]["retail"]["base"]
    codes = {s: bytes.fromhex(job["sides"][s]["code"]) for s in ("retail", "ours")}
    lines = saved.get("lines") or {}
    lines = {int(k): tuple(v) for k, v in lines.items()}
    src = saved.get("src")
    out = []
    if not d:
        out.append("replay: %s (%s)" % (r["status"], r.get("why", "")))
        return out
    val = lambda v: ("0x%x" % v) if isinstance(v, int) else str(v)
    if d["kind"] == "sequence":
        out.append("on this input the original makes the events %s, ours %s" % (d["orig"], d["ours"]))
    elif d["kind"] == "target":
        out.append("%s: the original goes to %s, ours to %s" % (
            d["what"].replace("call#%d" % d["event"], "call #%d" % (d["event"] + 1)),
            name_of(d["orig"]) if isinstance(d["orig"], int) else d["orig"],
            name_of(d["ours"]) if isinstance(d["ours"], int) else d["ours"]))
    elif d["kind"] == "mem":
        out.append("%s: byte at 0x%x is %s in the original, %s in ours" % (
            what_words(d["what"]).split(" (")[0], d["addr"], val(d["orig"]), val(d["ours"])))
    else:
        out.append("%s: original %s, ours %s" % (what_words(d["what"]), val(d["orig"]), val(d["ours"])))
    prod = d.get("producer") or {}
    for side, nm in (("retail", "original"), ("ours", "ours")):
        p = prod.get(side)
        if isinstance(p, int):
            off = p - base
            s = "  %-8s +0x%x  %s" % (nm, off, _ins_text(codes[side], base, off))
            if d["kind"] == "mem":
                s += "  (%s)" % _store_ordinal(codes[side], base, off)
            if side == "ours":
                sl = src_line(lines, off, src)
                if sl:
                    s += "  <- " + sl
            out.append(s)
        elif isinstance(p, str):
            out.append("  %-8s the value comes from %s (the callee's result)" % (nm, p.replace("call#", "call #")))
        elif d["kind"] == "mem":
            out.append("  %-8s does not write that byte" % nm)
    br = d.get("branches") or {}
    if d["kind"] == "sequence" and br:
        for side, nm in (("retail", "original"), ("ours", "ours")):
            if br.get(side):
                pc, tk = br[side][-1]
                s = "  %-8s last branch +0x%x %s (%s)" % (nm, pc - base, _ins_text(codes[side], base, pc - base),
                                                          "taken" if tk else "not taken")
                if side == "ours":
                    sl = src_line(lines, pc - base, src)
                    if sl:
                        s += "  <- " + sl
                out.append(s)
    return out


# ------------------------------------------------------------------------------------------ the check
def our_side(va, src, symbol, flags, obj):
    import func as FN
    if obj:
        f = FN.obj_func(obj, symbol)
        f.meta.update({"src": os.path.relpath(os.path.abspath(src), ROOT), "flags": list(flags or []),
                       "ret64": FN.ret64(symbol)})
        return f
    return FN.our_func(src, va, symbol, flags)


def locate(va):
    """(src, symbol) of va from docs/insn_exact.json"""
    try:
        r = json.load(open(os.path.join(ROOT, "docs", "insn_exact.json")))["rows"].get("%08x" % va)
    except (OSError, ValueError):
        r = None
    if not r or not r.get("src"):
        raise SystemExit("0x%08x: no source in docs/insn_exact.json; pass --src" % va)
    return r["src"], r.get("symbol")


def run(va, src=None, symbol=None, flags=None, obj=None, angr_timeout=15.0, emu_timeout=10, emu=True,
        replay=True, log=print, save=True):
    """Print the quick verdict for va. -> dict: verdict 'match' | 'equivalent' | 'likely-equivalent' | 'differs' |
    'likely-differs' | 'undecided' | 'skipped', plus details"""
    import func as FN
    import facts
    t0 = time.time()
    res = {"va": "%08x" % va, "verdict": "undecided"}
    if src is None:
        src, sym0 = locate(va)
        symbol = symbol or sym0
    src = os.path.relpath(os.path.abspath(os.path.join(ROOT, src) if not os.path.isabs(src) else src), ROOT)
    res["src"] = src
    try:
        retail = FN.retail_func(va)
        ours = our_side(va, os.path.join(ROOT, src), symbol, flags, obj)
    except SystemExit as e:
        log("quick check: skipped (%s)" % str(e).strip().splitlines()[0][:200])
        return dict(res, verdict="skipped", why=str(e)[:300])
    flags = ours.meta.get("flags") or flags or FN.default_flags(src)
    res["symbol"] = ours.meta.get("symbol")
    try:
        import prove
        asm = prove.asm_transcribed(src, va)
    except Exception:
        asm = None
    if asm:
        log("quick check: skipped (an assembly transcription: write it as C++ first)")
        return dict(res, verdict="skipped", why="assembly transcription")
    if FN.is_byte_exact(retail, ours):
        log("quick check: byte-exact (MATCH)")
        return dict(res, verdict="match")
    same, why, _fa = facts.same(retail, ours)
    level = "L1" if same else "L2"
    res["level"] = level
    job = None
    try:
        import angr_layer as AL
        job = AL.prepare(retail, ours, level, None)
    except Exception:
        job = None
    note = call_count_note(retail, ours, job)
    if note:
        log("quick check: CALL COUNT: " + note)
        res["calls"] = note
    elif not same:
        log("quick check: ours uses registers differently from the original (%s): checked at L2 (every register)"
            % why)
    # 1. renaming
    import rename
    r = rename.prove(retail, ours, level)
    res["rename_s"] = r["seconds"]
    if r["status"] == "proved":
        log("quick check: EQUIVALENT by register renaming at %s (the bytes differ only in register names, frame "
            "slots and the order of independent instructions). Record it: uv run python tools/equiv/prove.py %s "
            "0x%08x --record" % (level, src, va))
        res.update(verdict="equivalent", layer="rename", seconds=round(time.time() - t0, 2))
        return res
    # 2. angr
    verdict = "undecided"
    if angr_timeout and angr_timeout > 0:
        import cexmodel
        r, job2 = cexmodel.prove(retail, ours, level, angr_timeout)
        job = job2 or job
        res["angr"] = {"status": r["status"], "seconds": r["seconds"], "detail": re.sub(r"\s*\|.*$", "",
                                                                                         r["detail"])[:400]}
        if r["status"] == "proved":
            log("quick check: very likely EQUIVALENT: angr proves it at %s in %.0fs (the mutant self-test is not run "
                "here). Record it: uv run python tools/equiv/prove.py %s 0x%08x --record" % (level, r["seconds"],
                                                                                            src, va))
            res.update(verdict="likely-equivalent", layer="angr", seconds=round(time.time() - t0, 2))
            return res
        cex = r.get("cex")
        if cex:
            diffs = explain(cex)
            ins = input_words(cex, job)
            nt = re.search(r"not trusted \((.*?)\): difference", r["detail"])
            if r["status"] == "refuted":
                log("quick check: DIFFERS from the original (angr, %s, a counterexample it trusts): the source "
                    "computes something else; fix the logic, not the codegen." % level)
                verdict = "differs"
            else:
                log("quick check: LIKELY DIFFERS (unverified: %s)" % (nt.group(1) if nt else "untrusted"))
                verdict = "likely-differs"
            res["unverified"] = nt.group(1) if nt else None
            for x in diffs:
                log("  - " + x)
            if ins:
                log("  inputs that show it: " + ins)
            res.update(differences=diffs, inputs=ins)
            if replay and job:
                rp = do_replay(va, src, ours, job, cex, flags, log, save)
                res["replay"] = rp
        else:
            reason = re.sub(r"\s*\|.*$", "", r["detail"]).strip()
            log("quick check: angr could not decide in %.0fs: %s" % (r["seconds"], reason[:220]))
    if verdict in ("differs",) or not emu:
        res.update(verdict=verdict, seconds=round(time.time() - t0, 2))
        log("  (quick check took %.0fs)" % (time.time() - t0))
        return res
    # 3. the emulator, a quick refuter for what angr left open
    e = emulator(va, src, emu_timeout, log)
    res["emu"] = e
    if e.get("verdict", "").startswith("FAIL"):
        verdict = "differs"
    res.update(verdict=verdict, seconds=round(time.time() - t0, 2))
    log("  (quick check took %.0fs)" % (time.time() - t0))
    return res


def do_replay(va, src, ours, job, cex, flags, log, save=True):
    import cexreplay
    try:
        r = cexreplay.replay(job, cex)
    except Exception as e:
        log("  concrete replay: failed (%s: %s)" % (type(e).__name__, str(e)[:160]))
        return {"status": "error", "why": str(e)[:300]}
    saved = {"va": "%08x" % va, "src": src, "symbol": ours.meta.get("symbol"), "job": job, "cex": cex}
    st = r["status"]
    out = {"status": st, "why": r.get("why"), "diff": r.get("diff")}
    if st in ("confirmed", "likely"):
        lines = listing_lines(src, flags, ours.meta.get("symbol") or "") if r.get("diff") else {}
        saved["lines"] = {str(k): list(v) for k, v in lines.items()}
        if st == "confirmed":
            log("  concrete replay CONFIRMS it (both versions under unicorn on the model's input):")
        else:
            log("  concrete replay shows it, but %s:" % r["why"])
        desc = describe_replay(r, saved)
        for ln in desc:
            log("    " + ln)
        out["words"] = desc
        ml = marker_line(src, va)
        if ml:
            out["marker"] = "%s:%d" % (src, ml)
    elif st == "not-reproduced":
        log("  concrete replay: not reproduced on the model's input (the model's callee/memory choices may differ "
            "from the replay's; the difference stays unconfirmed)")
    else:
        log("  concrete replay: not possible (%s)" % (r.get("why") or "")[:200])
    if save:
        os.makedirs(CEX_DIR, exist_ok=True)
        p = os.path.join(CEX_DIR, "%08x.json" % va)
        with open(p, "w") as f:
            json.dump(saved, f)
        out["file"] = os.path.relpath(p, ROOT)
        log("  replay it: uv run python tools/equiv/cexreplay.py %s" % out["file"])
    return out


def emulator(va, src, seconds, log):
    import importlib.util
    if importlib.util.find_spec("unicorn") is None or importlib.util.find_spec("numpy") is None:
        log("quick check: the emulator test needs unicorn and numpy; skipped")
        return {"verdict": "skipped"}
    import tempfile
    mod = os.path.basename(src).rsplit(".", 1)[0]
    env = dict(os.environ, PYTHONHASHSEED="0", DIFFTEST_NO_LEDGER="1",
               DIFFTEST_FAIL_DIR=os.path.join(ROOT, "work", "equiv", "quick_fail"))
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, "r.json")
        try:
            subprocess.run([sys.executable, os.path.join(ROOT, "tools", "difftest", "equiv.py"), mod, "%08x" % va,
                            "--src", os.path.join(ROOT, src), "--time-limit", str(int(seconds)), "--json", out],
                           capture_output=True, text=True, timeout=seconds + 180, env=env, cwd=ROOT)
            res = json.load(open(out))[0]
        except (OSError, ValueError, IndexError, subprocess.TimeoutExpired) as e:
            log("quick check: the emulator test gave no result (%s)" % type(e).__name__)
            return {"verdict": "none"}
    v, reason = res.get("verdict", ""), (res.get("reason") or "")[:240]
    e = {"verdict": v, "reason": reason}
    fm = res.get("first_mismatch")
    if v.startswith("FAIL"):
        log("quick check: DIFFERS (emulator): a concrete input gives a different result: %s" % reason)
        if fm:
            log("    original: %s\n    ours:     %s" % (str(fm.get("orig"))[:160], str(fm.get("ours"))[:160]))
            if fm.get("input_file"):
                e["replay"] = "uv run python tools/difftest/equiv.py %s %08x --src %s --replay %s -v" % (
                    mod, va, src, fm["input_file"])
                log("    replay: " + e["replay"])
    elif v.startswith(("PASS", "WEAK")) or "time limit" in reason:
        m = re.search(r"after (\d+) inputs", reason)
        v = v if v.startswith(("PASS", "WEAK")) else "%s inputs" % (m.group(1) if m else "some")
        log("quick check: the emulator found no mismatch in %ds of random inputs (%s); a hint, not a proof"
            % (seconds, v))
    else:
        log("quick check: the emulator could not run it (%s %s)" % (v, reason[:120]))
    return e


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("va")
    ap.add_argument("--src")
    ap.add_argument("--symbol")
    ap.add_argument("--flags")
    ap.add_argument("--angr-timeout", type=float, default=15.0, help="seconds for the angr step (0: skip it)")
    ap.add_argument("--emu-timeout", type=int, default=10)
    ap.add_argument("--no-emu", action="store_true", help="skip the emulator step")
    ap.add_argument("--no-replay", action="store_true", help="do not replay angr's counterexample concretely")
    ap.add_argument("--remote", action="store_true", help="angr on the Windows laptop (tools/equiv/remote.py)")
    ap.add_argument("--json", help="write the result here")
    a = ap.parse_args()
    if a.remote or os.environ.get("EQUIV_REMOTE") == "1":
        import remote
        remote.enable()
    va = int(a.va, 16)
    r = run(va, a.src, a.symbol, a.flags.split() if a.flags else None, angr_timeout=a.angr_timeout,
            emu_timeout=a.emu_timeout, emu=not a.no_emu, replay=not a.no_replay)
    if a.json:
        with open(a.json, "w") as f:
            json.dump(r, f, indent=1, default=str)
    return 0 if r["verdict"] in ("match", "equivalent", "likely-equivalent") else 1


if __name__ == "__main__":
    sys.exit(main())
