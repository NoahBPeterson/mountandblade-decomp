#!/usr/bin/env python3
"""Breadth-first reach analysis from the crash sites.

usage: reach.py [--seeds docs/crashes.json] [--dir up|down|both] [--levels N] [--list N] [--json out]

Seeds are the functions in docs/crashes.json (their entry addresses, not the faulting instruction). --dir up
follows callers (who can reach the crash), down follows callees (what the crashed code needs), both is the union.
Each level prints how many of its functions are byte-exact / STRONG-equivalent (equivalence sweep PASS) / weak or
worse, so the cost of getting a crash path in-game is visible per hop.
"""
import argparse, bisect, collections, contextlib, glob, io, json, os, runpy, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = lambda *p: os.path.join(ROOT, *p)
sys.path.insert(0, W("tools", "matching"))
import capstone
from card import load_funcs, pe


def status_sets():
    argv, sys.argv = sys.argv, [W("tools", "status.py"), "--no-write"]
    with contextlib.redirect_stdout(io.StringIO()):
        S = runpy.run_path(W("tools", "status.py"))
    sys.argv = argv
    game = S["game"]
    return game, S["exact"] & game, S["equiv"] & game, S["partial"] & game


def verdicts():
    try:
        rows = json.load(open(W("docs", "equiv_verdicts.json")))["rows"]
    except (OSError, ValueError, KeyError):
        return {}
    return {int(k, 16): v.get("verdict", "?") for k, v in rows.items()}


def names():
    """original address -> symbol, from the relink resolver (authoritative), then the symbols/ tables."""
    out = {}
    try:
        for a, v in json.load(open(W("work", "relink", "standalone", "resolve_map.json")))["defined_at"].items():
            if isinstance(v, (list, tuple)) and v:
                out[int(a, 16)] = v[0]
    except (OSError, ValueError, KeyError):
        pass
    for f in glob.glob(W("symbols", "*.txt")) + glob.glob(W("symbols", "slices", "*.txt")):
        for line in open(f):
            p = line.split("#", 1)[0].split()
            if len(p) >= 2:
                try:
                    out.setdefault(int(p[0], 16), p[1])
                except ValueError:
                    pass
    return out


def graph():
    p = pe()
    base = p.OPTIONAL_HEADER.ImageBase
    text = next(s for s in p.sections if s.Name.rstrip(b"\0") == b".text")
    lo, hi = base + text.VirtualAddress, base + text.VirtualAddress + text.Misc_VirtualSize
    corpus = p.get_data(text.VirtualAddress, text.Misc_VirtualSize)
    starts = sorted(s for s in load_funcs()[0] if lo <= s < hi)
    where = {va: i for i, va in enumerate(starts)}
    data_secs = [(base + s.VirtualAddress, base + s.VirtualAddress + s.Misc_VirtualSize)
                 for s in p.sections if s.Name.rstrip(b"\0") in (b".rdata", b".data")]

    def slots_at(va, n=64):
        if not any(a <= va < b for a, b in data_secs):
            return []
        raw = p.get_data(va - base, n)
        out = []
        for k in range(0, len(raw) - 3, 4):
            j = where.get(int.from_bytes(raw[k:k + 4], "little"))
            if j is not None:
                out.append(j)
        return out

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True          # operands are empty without it, so every edge would be missed
    fwd = [set() for _ in starts]
    for i, va in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else hi
        try:
            insns = md.disasm(corpus[va - lo:end - lo], va)
            for ins in insns:
                for op in ins.operands:
                    if op.type != capstone.x86.X86_OP_IMM:
                        continue
                    t = op.imm & 0xFFFFFFFF
                    j = where.get(t)
                    if j is not None:
                        if j != i:
                            fwd[i].add(j)
                    else:
                        for j in slots_at(t):
                            if j != i:
                                fwd[i].add(j)
        except capstone.CsError:
            pass
    rev = [set() for _ in starts]
    for i, ts in enumerate(fwd):
        for j in ts:
            rev[j].add(i)
    return starts, fwd, rev


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", default=W("docs", "crashes.json"))
    ap.add_argument("--dir", choices=("up", "down", "both"), default="up")
    ap.add_argument("--levels", type=int, default=8)
    ap.add_argument("--list", type=int, default=0)
    ap.add_argument("--json")
    a = ap.parse_args()

    seeds = sorted({int(c.get("fn_addr") or c["addr"], 16)
                    for c in json.load(open(a.seeds))["crashes"]})
    starts, fwd, rev = graph()
    where = {va: i for i, va in enumerate(starts)}
    game, exact, equiv, partial = status_sets()      # game functions only: the CRT/C++ library is excluded
    vd = verdicts()
    nm = names()

    def state(i):
        va = starts[i]
        if va in exact:
            return "exact"
        v = vd.get(va)
        if va in equiv and v == "PASS":
            return "strong"
        if va in equiv:
            return "weak" if v == "WEAK" else ("fail" if v == "FAIL" else "untested")
        return "partial" if va in partial else "none"

    def idx_of(va):
        i = where.get(va)
        if i is not None:
            return i
        i = bisect.bisect_right(starts, va) - 1
        return i if 0 <= i < len(starts) else None

    adj = {"up": rev, "down": fwd, "both": [r | f for r, f in zip(rev, fwd)]}[a.dir]
    seen, q, skipped = {}, collections.deque(), set()
    for s in seeds:
        i = idx_of(s)
        if i is not None and i not in seen:
            seen[i] = 0
            q.append(i)
    order = list(q)
    while q:
        i = q.popleft()
        if seen[i] >= a.levels:
            continue
        for j in sorted(adj[i]):
            if starts[j] not in game:
                skipped.add(starts[j])
                continue
            if j not in seen:
                seen[j] = seen[i] + 1
                order.append(j)
                q.append(j)

    by_level = collections.defaultdict(list)
    for i, d in seen.items():
        by_level[d].append(i)

    label = lambda va: nm.get(va, "0x%08x" % va)
    print("seeds (%d): %s" % (len(seeds), ", ".join(label(s) for s in seeds)))
    print("direction: %s, %d game functions (%d CRT/library neighbours skipped)\n" % (a.dir, len(seen), len(skipped)))
    hdr = "%-6s %6s %7s %7s %7s %7s %6s %6s %7s" % ("level", "funcs", "exact", "exact%", "strong", "weak", "fail", "none", "done%")
    print(hdr)
    print("-" * len(hdr))
    rows = []
    for d in sorted(by_level):
        fs = by_level[d]
        c = collections.Counter(state(i) for i in fs)
        done = c["exact"] + c["strong"]
        rows.append({"level": d, "funcs": len(fs), **c})
        print("%-6d %6d %7d %6.1f%% %7d %7d %6d %6d %6.1f%%" %
              (d, len(fs), c["exact"], 100.0 * c["exact"] / len(fs), c["strong"],
               c["weak"] + c["untested"], c["fail"], c["partial"] + c["none"], 100.0 * done / len(fs)))
    tot = collections.Counter(state(i) for i in seen)
    n = len(seen)
    done = tot["exact"] + tot["strong"]
    print("-" * len(hdr))
    print("%-6s %6d %7d %6.1f%% %7d %7d %6d %6d %6.1f%%" %
          ("total", n, tot["exact"], 100.0 * tot["exact"] / n, tot["strong"],
           tot["weak"] + tot["untested"], tot["fail"], tot["partial"] + tot["none"], 100.0 * done / n))

    if a.list:
        print("\nnot yet byte-exact, shallowest first:")
        shown = 0
        for d in sorted(by_level):
            for i in sorted((i for i in by_level[d] if state(i) != "exact"), key=lambda i: starts[i]):
                if shown >= a.list:
                    break
                print("  L%d  %-8s %s" % (d, state(i), label(starts[i])))
                shown += 1
            if shown >= a.list:
                break
    if a.json:
        json.dump({"seeds": ["%08x" % s for s in seeds], "direction": a.dir, "levels": rows,
                   "total": tot, "functions": [{"va": "%08x" % starts[i], "name": nm.get(starts[i], ""),
                                                "level": seen[i], "state": state(i)} for i in order]},
                  open(a.json, "w"), indent=1)
        print("\nwrote", os.path.relpath(a.json, ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
