#!/usr/bin/env python3
"""Print Mount & Blade decompilation progress as terminal tables. Standalone: run from any shell.

  .venv/bin/python tools/progress.py                 # coverage + per-batch tables
  .venv/bin/python tools/progress.py --plot [out.png]     # progress over time (default work/progress.png)
  .venv/bin/python tools/progress.py --treemap [out.png]  # one box per module -> function, area = instructions,
                                                          # colour = verdict (default work/treemap.png)

Coverage is computed by tools/status.py itself (run via runpy), so the numbers match docs/STATUS.md.
"""
import contextlib, datetime, glob, io, json, os, re, runpy, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = lambda *p: os.path.join(ROOT, *p)

_argv, sys.argv = sys.argv, [W("tools", "status.py"), "--no-write"]
with contextlib.redirect_stdout(io.StringIO()):
    S = runpy.run_path(W("tools", "status.py"))
sys.argv = _argv
game, size, G, B = S["game"], S["size"], S["G"], S["B"]
starts, lib = S["starts"], S["lib"]          # census order; the CRT/STL functions the project excludes
exact, equiv, partial = S["exact"] & game, S["equiv"] & game, S["partial"] & game
# "Equivalent" counts only the STRONG pass rows: tools/difftest/verdicts.py replayed the original and our function on
# the same random inputs and found no mismatch. weak / untestable-on-this-harness / FAIL rows are failures here.
try:
    _verd = {k.lower().lstrip("0x").rjust(8, "0"): v.get("verdict")
             for k, v in json.load(open(W("docs", "equiv_verdicts.json")))["rows"].items()}
except (OSError, ValueError, KeyError):
    _verd = {}
_equiv_all = equiv
equiv = {a for a in _equiv_all if _verd.get("%08x" % a) in ("PASS", "PASS-FP")}   # PASS-FP = PASS within fp tolerance
equiv_fail = _equiv_all - equiv
done = exact | equiv
# "Instruction-exact": the original's instructions in the original's order, up to a consistent renaming of registers
# and stack slots, with every call/global/literal operand resolved to the same original address
# (tools/matching/insn_exact.py; tools/matching/insn_exact_sweep.py writes docs/insn_exact.json). Its own tier: not
# counted in `done`, and these rows also appear under their equivalence verdict below.
try:
    _ie = json.load(open(W("docs", "insn_exact.json")))
except (OSError, ValueError):
    _ie = {}
insn_exact = {int(a, 16) for a, r in _ie.get("rows", {}).items() if r.get("verdict") == "INSN-EXACT"} & game - exact
migrated = S["migrated"] & done


def table(headers, rows):
    w = [max(len(str(r[i])) for r in [headers] + rows) for i in range(len(headers))]
    fmt = lambda r: "  ".join((str(c).ljust(w[i]) if i == 0 else str(c).rjust(w[i])) for i, c in enumerate(r))
    return "\n".join([fmt(headers), "  ".join("-" * x for x in w)] + [fmt(r) for r in rows])


try:
    import csv as _csv
    INS = {int(r["address"], 16): int(r["instructions"] or 0)
           for r in _csv.DictReader(open(W("work", "index", "functions.csv"))) if r.get("block") == ".text"}
except (OSError, ValueError, KeyError):
    INS = {}
nbytes = lambda s: sum(size[a] for a in s)
ninsn = lambda s: sum(INS.get(a, 0) for a in s)
pc = lambda n, d: "%.2f%%" % (100.0 * n / d) if d else "-"
TI = sum(INS.get(a, 0) for a in game) or 1
row = lambda label, s: [label, len(s), pc(len(s), G), "%d" % ninsn(s), pc(ninsn(s), TI),
                        "%.1f KB" % (nbytes(s) / 1024), pc(nbytes(s), B)]
print("Mount & Blade 1.011 decompilation: %d game functions, %.2f MB (static CRT/C++ library excluded)\n" % (G, B / 1e6))
print(table(["Tier", "Functions", "Share", "Insns", "Share (insns)", "Code size", "Share (size)"],
            [row("Byte-exact", exact), row("Equivalent (STRONG pass)", equiv),
             row("Not equivalent: weak / untestable / FAIL", equiv_fail),
             row("Total counted as done", done),
             row("Partial (not counted)", partial), row("No source yet", game - done - partial - equiv_fail)]))
print()
print(table(["Instruction-exact (not counted above)", "Functions", "Share", "Insns", "Share (insns)", "Code size",
             "Share (size)"],
            [row("Same instructions, renamed registers/slots", insn_exact),
             row("  of which: equivalent (STRONG pass)", insn_exact & equiv),
             row("  of which: weak / untestable / FAIL", insn_exact & equiv_fail)]))
_sw = [(int(a, 16), r) for a, r in _ie.get("rows", {}).items() if r.get("cases")]
if _sw:
    print()
    print("Switch functions compared case by case (instruction-exact cases; sweep of %s):"
          % _ie.get("when", "?")[:16].replace("T", " "))
    print(table(["Function", "Cases", "Insn-exact", "Share (insns)", "Prologue"],
                [["%08x %s" % (a, (r.get("symbol") or "")[:40]), r["cases"]["total"], r["cases"]["insn_exact"],
                  "%.1f%%" % (100.0 * r["cases"]["insn_share"]), r["cases"].get("prologue") or "-"]
                 for a, r in sorted(_sw)]))
print()
print(table(["Source tree", "Functions", "Share", "Insns", "Share (insns)", "Code size", "Share (size)"],
            [row("Migrated into src/", migrated), row("Compilable, still only in match/slices", done - migrated)]))

# Equivalence evidence for the equivalent-only rows (tools/difftest/verdicts.py, from the latest sweep)
EV_CATS = (("pass", "PASS: matched, every branch outcome taken"), ("weak", "WEAK: matched, some branch outcomes never taken"),
           ("fail", "FAIL: a mismatch (see the row's note)"), ("inputs", "Untestable: random inputs invalid"),
           ("harness", "Untestable: harness limitation"),
           ("comdat", "Byte-exact via COMDAT (no callable symbol, not executed)"),
           ("comdatdiff", "COMDAT differs (symbol-less fragment of a non-matching parent)"),
           ("funclet", "Symbol-less code block: not yet COMDAT-checked"),
           ("not_run", "Not run"))
try:
    ev = json.load(open(W("docs", "equiv_verdicts.json")))
except (OSError, ValueError):
    ev = None
if ev:
    s = ev["summary"]
    # instruction weight per category: the summary has row counts and bytes only, so group the file's own rows
    icat = {c: 0 for c, _ in EV_CATS}
    for a, r in (ev.get("rows") or {}).items():
        try:
            icat[r.get("category", "not_run")] += INS.get(int(a, 16), 0)
        except ValueError:
            pass
    tot, tsz = sum(s[c] for c, _ in EV_CATS), sum(s[c + "_sz"] for c, _ in EV_CATS)
    ti = sum(icat.values()) or 1
    print()
    print("Equivalence evidence for the %d equivalent-only module rows (sweep of %s, commit %s):" % (tot, ev["when"][:16].replace("T", " "), ev["commit"]))
    print(table(["Verdict", "Rows", "Share", "Insns", "Share (insns)", "Code size", "Share (size)"],
                [[lab, s[c], pc(s[c], tot), icat[c], pc(icat[c], ti), "%.1f KB" % (s[c + "_sz"] / 1024),
                  pc(s[c + "_sz"], tsz)] for c, lab in EV_CATS if s[c]]))

# Standalone build: the latest trial link of every src/ module (tools/relink/trial_link.py --record)
try:
    rl = sorted(json.load(open(W("docs", "relink_history.json"))).values(), key=lambda p: p["when"])[-1]
except (OSError, ValueError, IndexError):
    rl = None
if rl:
    print()
    print("Standalone build (trial link of %d objects, %s, commit %s): %s" % (
        rl["objects"], rl["when"][:16].replace("T", " "), rl["commit"],
        "LINKS" if rl["linked"] else "%d unresolved symbols, %d duplicate definitions" % (rl["unresolved"], rl["duplicates"])))
    if not rl["linked"]:
        print(table(["Unresolved, by cause", "Symbols"], [[k, v] for k, v in rl["by_category"].items() if v]))

# Static data: what our source defines of the original's .rdata/.data/.bss (tools/relink/data_coverage.py --record)
try:
    dc = sorted(json.load(open(W("docs", "data_history.json"))).values(), key=lambda p: p["when"])[-1]
except (OSError, ValueError, IndexError):
    dc = None
if dc:
    it, sb = dc["items"], dc["section_bytes"]
    gcls = [c for c in it if c != "library data"]
    gitems = sum(it[c] for c in gcls)
    init = lambda c: sb[".rdata"][c] + sb[".data"][c]
    ginit = sum(init(c) for c in gcls)
    gbss = sum(sb[".bss"][c] for c in gcls)
    src = it["defined in source"] + it["generated from source"]
    print()
    print("Static data (items recovered from their uses, %s, commit %s): %d of %d game data items come from our source "
          "(%s); the rest the standalone link takes from the original image.\nNamed globals: %d defined in source, %d "
          "borrowed (string literals, constants and vtables are the compiler's)" % (
              dc["when"][:16].replace("T", " "), dc["commit"], src, gitems, pc(src, gitems),
              it["defined in source"], it["borrowed from image"]))
    print(table(["Class", "Items", "Share", ".rdata+.data", "Share (bytes)", ".bss"],
                [[c, it[c], pc(it[c], gitems) if c in gcls else "-", "%.1f KB" % (init(c) / 1024),
                  pc(init(c), ginit) if c in gcls else "-", "%.1f MB" % (sb[".bss"][c] / 1e6)] for c in it]))

rows = []
for bf in sorted(glob.glob(W("work", "batches", "*.json")), key=os.path.getmtime):
    sl = json.load(open(bf))
    vas = {int(v, 16) for s in sl for v in s["vas"]} & game
    started = sum(1 for s in sl if os.path.isdir(W("match", "slices", s["id"])))
    e, q, p = vas & exact, vas & equiv, vas & partial
    rows.append([os.path.basename(bf)[:-5], "%d/%d" % (started, len(sl)), len(vas), len(e), len(q), len(p),
                 len(vas - e - q - p), pc(len(e), len(vas)), pc(nbytes(e), nbytes(vas))])
if rows:
    print()
    print(table(["Batch", "Slices started", "Functions", "Exact", "Equiv", "Partial", "Open", "Exact %", "Exact % (size)"], rows))


def record_verdicts():
    """Make the sweep behind docs/equiv_verdicts.json a point in docs/equiv_history.json (verdicts.py writes it too)."""
    try:
        hist = json.load(open(W("docs", "equiv_history.json")))
    except (OSError, ValueError):
        hist = {}
    try:
        v = json.load(open(W("docs", "equiv_verdicts.json")))
        when, summ = v["when"], v["summary"]
    except (OSError, ValueError, KeyError):
        return
    if when in hist:
        print("verdicts: the treemap uses the sweep at %s" % when)
        return
    hist[when] = dict(summ, when=when)
    with open(W("docs", "equiv_history.json"), "w") as f:
        json.dump(dict(sorted(hist.items())), f, indent=1)
        f.write("\n")
    print("verdicts: recorded the sweep at %s (%d PASS, %d WEAK, %d FAIL)" % (when, summ.get("pass", 0), summ.get("weak", 0), summ.get("fail", 0)))


def parse_status_md(md):
    """Shares from one committed docs/STATUS.md (tools/status.py format) -> dict or None."""
    out = {}
    for line in md.splitlines():
        cells = [c.strip().strip("*").strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 5 or not cells[2].endswith("%"):
            continue
        key = {"byte-exact": "exact", "total with compilable source": "comp", "migrated into src/": "mig"}.get(cells[0].lower())
        if key:
            out[key] = float(cells[2].rstrip("%"))
            out[key + "_sz"] = float(cells[4].rstrip("%"))
    return out if "exact" in out and "comp" in out else None


def plot_history(path):
    """Progress over time: one point per committed docs/STATUS.md and one per --plot run (kept in
    docs/progress_history.json), plus the equivalence-evidence stacked bands per recorded sweep.
    The per-module picture is --treemap."""
    import datetime
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.dates as mdates          # noqa: F401  (kept: mdates is used by older saved points)
    import matplotlib.pyplot as plt
    from matplotlib.ticker import PercentFormatter
    pts = []
    log = subprocess.run(["git", "-C", ROOT, "log", "--format=%H %cI", "--", "docs/STATUS.md"],
                         capture_output=True, text=True).stdout.split("\n")
    for line in filter(str.strip, log):
        sha, when = line.split()
        p = parse_status_md(subprocess.run(["git", "-C", ROOT, "show", sha + ":docs/STATUS.md"],
                                           capture_output=True, text=True).stdout)
        if p:
            pts.append((datetime.datetime.fromisoformat(when), p))
    now = datetime.datetime.now().astimezone()
    live = {"exact": 100.0 * len(exact) / G, "comp": 100.0 * len(done) / G, "mig": 100.0 * len(migrated) / G,
            "exact_sz": 100.0 * nbytes(exact) / B, "comp_sz": 100.0 * nbytes(done) / B,
            "mig_sz": 100.0 * nbytes(migrated) / B}
    # One saved point per --plot run, keyed by its timestamp (older files were keyed by date, one entry per day;
    # re-keying them by their 'when' keeps those points).
    hist_path = W("docs", "progress_history.json")
    try:
        hist = {p["when"]: p for p in json.load(open(hist_path)).values()}
    except (OSError, ValueError, KeyError, TypeError):
        hist = {}
    stamp = now.isoformat(timespec="seconds")
    hist[stamp] = dict(live, when=stamp)
    with open(hist_path, "w") as f:
        json.dump(dict(sorted(hist.items())), f, indent=1)
        f.write("\n")
    # The saved runs are measured live; a committed STATUS.md can be stale (3e37956 carried a snapshot from hours
    # earlier and drew a dip to 32% migrated). Keep commit points only from before the first saved run.
    first = min(datetime.datetime.fromisoformat(p["when"]) for p in hist.values())
    pts = [t for t in pts if t[0] < first]
    pts += [(datetime.datetime.fromisoformat(p["when"]), p) for p in hist.values()]   # includes this run
    pts.sort(key=lambda t: t[0])

    BLUE, ORANGE, AQUA = "#2a78d6", "#eb6834", "#1baf7a"
    SURFACE, INK, MUTED, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e6e5e0"
    top = max([p[k] for _, p in pts for k in ("comp", "comp_sz") if p.get(k) is not None] + [0])
    ymax = 100 if top > 40 else max(5, round(top * 1.4 + 0.5))
    record_verdicts()
    # one point per recorded sweep (record_verdicts returns nothing: evh was always None, so the bands never drew)
    try:
        evh = sorted(json.load(open(W("docs", "equiv_history.json"))).values(), key=lambda p: p["when"])
    except (OSError, ValueError, KeyError):
        evh = []

    def style(ax, title, times):
        ax.set_facecolor(SURFACE)
        ax.set_title(title, loc="left", fontsize=11, color=INK)
        ax.yaxis.set_major_formatter(PercentFormatter(decimals=0))
        ax.grid(axis="y", color=GRID, lw=1)
        ax.set_axisbelow(True)
        for sp in ("top", "right", "left"):
            ax.spines[sp].set_visible(False)
        ax.spines["bottom"].set_color(GRID)
        ax.tick_params(colors=MUTED, labelsize=9, length=0)
        byday = {}
        for t in times:
            byday.setdefault(t.astimezone(now.tzinfo).date(), []).append(t)
        days = sorted(byday)[::max(1, -(-len(byday) // 8))]
        ax.set_xticks([min(byday[d]) + (max(byday[d]) - min(byday[d])) / 2 for d in days])
        ax.set_xticklabels([d.strftime("%b %-d") for d in days])
        ax.margins(x=0.08)

    nrows = 1 + bool(evh)
    fig, grid = plt.subplots(nrows, 2, figsize=(12, 4.4 * nrows + 1.6), dpi=150, squeeze=False)
    axes = grid[0]
    fig.patch.set_facecolor(SURFACE)
    for ax, (title, ke, kc, km) in zip(axes, (("Share of functions", "exact", "comp", "mig"),
                                              ("Share of code size", "exact_sz", "comp_sz", "mig_sz"))):
        ax.set_facecolor(SURFACE)
        for key, color, label in ((kc, ORANGE, "Compilable (byte-exact + equivalent)"), (ke, BLUE, "Byte-exact"),
                                  (km, AQUA, "Migrated into src/ modules")):
            val = (lambda p, key=key: p.get(key, 0.0)) if key == km else (lambda p, key=key: p.get(key))
            xs = [t for t, p in pts if val(p) is not None]
            ys = [val(p) for t, p in pts if val(p) is not None]
            if not xs:
                continue
            ax.plot(xs, ys, color=color, lw=2, marker="o", ms=4, label=label, solid_capstyle="round")
            ax.annotate("%.1f%%" % ys[-1], (xs[-1], ys[-1]), xytext=(6, 0), textcoords="offset points",
                        va="center", fontsize=9, color=INK)
        ax.set_ylim(0, ymax)
        style(ax, title, [t for t, _ in pts])
    axes[1].tick_params(labelleft=False)
    axes[0].legend(loc="upper left", frameon=False, fontsize=9, labelcolor=INK)
    if evh:
        # Every game function, 100%: byte-exact at the bottom, then equivalent (PASS), then the remaining verdicts of
        # the non-byte-exact rows, then code with no source yet. The byte-exact share at each sweep is the --plot
        # point at or before it; the verdict counts are that sweep's (same categories and colours as --treemap).
        TC = dict((k, c) for k, _l, c in TREE_CATS)
        LAYERS = (("exact", "Byte-exact", TC["byte-exact"]),
                  ("pass", "Equivalent: PASS (matched, every branch taken)", TC["equivalent"]),
                  ("comdat", "Byte-exact via COMDAT (symbol-less fragment)", "#7FB8DA"),
                  ("weak", "WEAK: matched, some branch never taken", TC["weak"]),
                  ("fail", "FAIL: a mismatch", TC["fail"]),
                  ("inputs", "Untestable: inputs invalid", TC["inputs"]),
                  ("harness", "Untestable: harness limitation", TC["harness"]),
                  ("comdatdiff", "COMDAT differs", TC["comdatdiff"]),
                  ("funclet", "Fragment not yet COMDAT-checked", TC["funclet"]),
                  ("not_run", "Not run (time limit)", TC["not_run"]),
                  ("nosource", "No source yet", "#E4E2DA"))
        xs = [datetime.datetime.fromisoformat(p["when"]) for p in evh]
        saved = sorted((datetime.datetime.fromisoformat(p["when"]), p) for p in hist.values())

        def exact_at(t, key):
            prior = [p for w, p in saved if w <= t and p.get(key) is not None]
            return prior[-1][key] if prior else (saved[0][1].get(key, 0.0) if saved else 0.0)
        for ax, suffix, total, title in ((grid[1][0], "", G, "All game functions, by evidence"),
                                         (grid[1][1], "_sz", B, "... by code size")):
            ys = {k: [] for k, _l, _c in LAYERS}
            for p, t in zip(evh, xs):
                e = exact_at(t, "exact" + suffix)
                ys["exact"].append(e)
                used = e
                for k, _l, _c in LAYERS[1:-1]:
                    v = 100.0 * p.get(k + suffix, 0) / total
                    ys[k].append(v)
                    used += v
                ys["nosource"].append(max(0.0, 100.0 - used))
            keys = [k for k, _l, _c in LAYERS if any(v > 0 for v in ys[k])]
            ax.stackplot(xs, [ys[k] for k in keys], colors=[c for k, _l, c in LAYERS if k in keys],
                         edgecolor=SURFACE, linewidth=1.0, labels=[l for k, l, _c in LAYERS if k in keys])
            base = 0.0
            for k in keys:
                y = ys[k][-1]
                if y >= 3:
                    ax.annotate("%.1f%%" % y, (xs[-1], base + y / 2), xytext=(6, 0),
                                textcoords="offset points", va="center", fontsize=9, color=INK)
                base += y
            ax.set_ylim(0, 100)
            style(ax, title, xs)
        grid[1][1].tick_params(labelleft=False)
        h, l = grid[1][0].get_legend_handles_labels()
        grid[1][0].legend(h, l, loc="upper center", ncol=4, frameon=False, fontsize=8, labelcolor=INK,
                          bbox_to_anchor=(1.07, -0.09))
    fig.suptitle("Mount & Blade 1.011 decompilation progress (%d game functions, %.2f MB; static CRT excluded)" % (G, B / 1e6),
                 x=0.01, ha="left", fontsize=12, color=INK)
    fig.text(0.01, 0.01, "One point per committed docs/STATUS.md and one per --plot run. "
             "Equivalent = complete compilable source whose equivalence sweep PASSed (no mismatch on the same random\n"
             "inputs); weak / untestable / FAIL rows are failures here. Migrated = moved from match/slices "
             "into one .cpp per original translation unit (0%% before the first migration).%s" %
             ("" if ymax == 100 else "\nThe y-axis is zoomed to %d%% while progress is low." % ymax) +
             ("\nVerdicts: one point per full equivalence sweep (tools/difftest/verdicts.py); a sweep tests the original and "
              "our function\non the same random inputs under emulation. The first point also counted pre-migration slice rows."
              if evh else "") + "\nPer-module detail (area = instructions, colour = verdict): tools/progress.py --treemap",
             fontsize=8, color=MUTED)
    h_in = fig.get_figheight()
    fig.subplots_adjust(left=0.06, right=0.93, top=1 - 0.75 / h_in, bottom=(1.15 + 0.35 * nrows) / h_in,
                        wspace=0.14, hspace=0.42)
    fig.savefig(path, facecolor=SURFACE)
    print("\nwrote %s (%d snapshots)" % (os.path.relpath(path, ROOT), len(pts)))


# ---------------------------------------------------------------- treemap

# Cell colours are the verdicts themselves: exact / equivalent / weak / FAIL plus the two "no evidence" kinds.
# Okabe-Ito (colour-blind safe): one hue per verdict, exactly one grey (comdat) and white (no source). The four
# "untested/unknown" kinds used to be four shades of grey, which is what made the treemap unreadable.
TREE_CATS = (("byte-exact", "Byte-exact", "#0072B2"),                    # blue
             ("equivalent", "Equivalent: STRONG pass", "#009E73"),        # bluish green
             ("weak", "Weak: some branch never taken", "#E69F00"),        # orange
             ("fail", "FAIL: a mismatch", "#D55E00"),                     # vermillion
             ("inputs", "Untestable: random inputs invalid", "#CC79A7"),  # reddish purple
             ("harness", "Untestable: harness limitation", "#56B4E9"),    # sky blue
             ("comdat", "Byte-exact via COMDAT (no callable symbol)", "#999999"),  # the only grey
             ("comdatdiff", "COMDAT differs (symbol-less fragment)", "#000000"),   # black
             ("funclet", "Symbol-less, not COMDAT-checked", "#999999"),
             ("not_run", "Not run: hit the time limit", "#F0E442"),       # yellow
             ("excluded", "Byte-exact library code: the real VC7.1 CRT/STL (excluded from the totals)", "#8FB1C8"),
             ("nosource", "No source yet", "#FFFFFF"))
# a TIMEOUT row (verdicts.py category not_run) has source: it was "No source yet" here until 2026-10-29
_VCAT = {"inputs": "inputs", "harness": "harness", "funclet": "funclet", "comdat": "comdat",
         "comdatdiff": "comdatdiff", "not_run": "not_run"}


def _verdict_data():
    """VA -> (verdict, category) from docs/equiv_verdicts.json (tools/difftest/verdicts.py)."""
    try:
        doc = json.load(open(W("docs", "equiv_verdicts.json")))
    except (OSError, ValueError):
        return {}
    out = {}
    for a, r in (doc.get("rows") or {}).items():
        try:
            out[int(a, 16)] = (r.get("verdict"), r.get("category"))
        except ValueError:
            pass
    return out


def _files_by_function():
    """VA -> source/manifest file that claims it (first claim wins), from the src module and slice manifests."""
    out = {}
    for pat, col in (("src/**/*.manifest.txt", 2), ("src/**/*.nonmatching.txt", 0), ("src/**/*.partial.txt", 0),
                     ("match/manifest.txt", 2), ("match/slices/*/manifest.txt", 2),
                     ("match/slices/*/nonmatching.txt", 0), ("match/slices/*/partial.txt", 0)):
        for f in glob.glob(W(pat), recursive=True):
            rel = os.path.relpath(f, ROOT).replace(os.sep, "/")
            for line in open(f):
                p = line.split("#", 1)[0].split()
                if len(p) > col:
                    try:
                        out.setdefault(int(p[col], 16), rel)
                    except ValueError:
                        pass
    return out


def _tree_rows():
    """(group, file, va, instructions, category) for every in-scope function. The import thunks / post-CRT stubs
    after 0x5342a6 are neither game code nor library code (6-byte jmp stubs, no source by design), so they are not
    drawn; `game` is the tier table's denominator and `lib` is the excluded CRT/STL the link takes from VC7.1."""
    files, verd = _files_by_function(), _verdict_data()
    rows = []
    for a in sorted(set(game) | set(lib)):
        rel = files.get(a)
        if a in lib:
            group, fname = "3rd party", "lib_crt"
        elif rel and rel.startswith("src/rgl/"):
            group, fname = "Engine (rgl)", rel[4:]
        elif rel and (rel.startswith("src/tinyxml/") or rel.startswith("src/dxframework/")):
            group, fname = "3rd party", rel[4:]
        elif rel and rel.startswith("src/game/"):
            group, fname = "Game", rel[4:]
        elif rel and rel.startswith("match/slices/"):
            group, fname = "Game", "slices/" + rel.split("/")[2]
        else:
            group, fname = "Game", "(no source)"
        if a in lib:                     # the CRT/STL the project links from the real VC7.1 libraries, not decompiled
            cat = "excluded"
        elif a in exact:
            cat = "byte-exact"
        elif a in equiv:
            cat = "equivalent"
        else:
            v, c = verd.get(a, (None, None))
            cat = ("equivalent" if v in ("PASS", "PASS-FP") else "fail" if v in ("FAIL", "FAIL-FP") else
                   "weak" if v in ("WEAK", "WEAK-FP") else _VCAT.get(c, "nosource"))
        rows.append((group, fname, a, INS.get(a, 0) or 1, cat))
    return rows


def _worst(row, side):
    s = sum(v for _, v in row)
    mx = max(v for _, v in row)
    mn = min(v for _, v in row)
    if s <= 0 or mn <= 0:
        return float("inf")
    return max(side * side * mx / (s * s), (s * s) / (side * side * mn))


def squarify(items, x, y, w, h):
    """Squarified treemap: [(key, weight)] -> [(key, (x, y, w, h))], areas proportional to weight."""
    items = sorted(((k, float(v)) for k, v in items if v > 0), key=lambda kv: -kv[1])
    tot = sum(v for _, v in items)
    if not tot or w <= 0 or h <= 0:
        return []
    items = [(k, v * w * h / tot) for k, v in items]
    out, i = [], 0
    while i < len(items):
        side = min(w, h)
        row, j = [items[i]], i + 1
        while j < len(items) and _worst(row, side) >= _worst(row + [items[j]], side):
            row.append(items[j])
            j += 1
        rsum = sum(v for _, v in row)
        if w >= h:                      # shorter side is the height: the row is a column of full height
            rw = rsum / h
            cy = y
            for k, v in row:
                rh = v / rw if rw else 0.0
                out.append((k, (x, cy, rw, rh)))
                cy += rh
            x, w = x + rw, w - rw
        else:                           # shorter side is the width: the row is a band of full width
            rh = rsum / w
            cx = x
            for k, v in row:
                rw = v / rh if rh else 0.0
                out.append((k, (cx, y, rw, rh)))
                cx += rw
            y, h = y + rh, h - rh
        i = j
    return out


def _symbol_names():
    """VA -> readable name, from the per-VA symbol tables in symbols/slices (mangled C++ is demangled crudely)."""
    out = {}
    for f in glob.glob(W("symbols", "slices", "*.txt")):
        for line in open(f):
            p = line.split("#", 1)[0].split()
            if len(p) > 1:
                try:
                    a = int(p[0], 16)
                except ValueError:
                    continue
                nm = p[1]
                if nm.startswith("?"):
                    nm = "::".join(x for x in reversed(nm[1:].split("@@")[0].split("@")) if x)
                out.setdefault(a, nm.replace("@", "").replace("$", ""))
    return out


def _tree_cells(rows, names):
    """(group, file) -> [(va, weight, category, label)]."""
    out = {}
    for g, f, a, n, cat in rows:
        nm = names.get(a, "0x%08x" % a)
        if len(nm) > 46:
            nm = nm[:45] + "…"
        out.setdefault((g, f), []).append((a, n, cat, nm))
    return out


def treemap(path):
    """Treemap of every census function: group -> source file (module) -> function, area = instructions,
    colour = verdict. Every file box is drawn; its label is measured and only kept where it fits (horizontal first,
    then rotated), so nothing overlaps and nothing is clipped."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch, Rectangle
    record_verdicts()
    INK, MUTED, SURFACE = "#0b0b0b", "#52514e", "#fcfcfb"
    rows = _tree_rows()
    cells = _tree_cells(rows, _symbol_names())
    colour = dict((k, c) for k, _l, c in TREE_CATS)
    label = dict((k, l) for k, l, _c in TREE_CATS)
    W_, H_ = 141.0, 100.0
    counts = {k: 0 for k, _l, _c in TREE_CATS}
    insn = {k: 0 for k, _l, _c in TREE_CATS}
    for _g, _f, _a, n, cat in rows:            # count everything, including cells too small to draw
        counts[cat] += 1
        insn[cat] += n
    tot_i, tot_n = sum(insn.values()) or 1, sum(counts.values()) or 1
    gi = {}
    for g, _f, _a, n, _c in rows:
        gi[g] = gi.get(g, 0) + n

    fig, ax = plt.subplots(figsize=(16.0, 11.2), dpi=150)
    fig.patch.set_facecolor(SURFACE)
    ax.set_facecolor(SURFACE)
    ax.set_xlim(0, W_)
    ax.set_ylim(0, H_)
    ax.set_xticks([])
    ax.set_yticks([])
    for sp in ax.spines.values():
        sp.set_visible(False)
    ax.set_position([0.012, 0.13, 0.976, 0.72])

    FS, FSF = 6.4, 5.2
    # average glyph width in data units, measured on a string shaped like the labels ("agent_combat", "mission_ui")
    cal = ax.text(0, 0, ("agent_combat_0123456789_" * 4), fontsize=FS, color=INK)
    fig.canvas.draw()
    bb = cal.get_window_extent(fig.canvas.get_renderer())
    (cx0, cy0), (cx1, cy1) = ax.transData.inverted().transform([[bb.x0, bb.y0], [bb.x1, bb.y1]])
    char_w, line_h = (cx1 - cx0) / len(cal.get_text()), abs(cy1 - cy0)
    cal.remove()

    def clip(txt, width):
        n = int(max(1.0, (width - 0.35) / char_w))
        return txt if len(txt) <= n else (txt[:n - 1] + "…" if n > 2 else txt[:1])

    labels = []        # (Text, rect, name) - measured after each draw pass; never truncated, dropped only as a last resort
    for g, (gx, gy, gw, gh) in squarify([(g, gi[g]) for g in gi], 0, 0, W_, H_):
        gx, gy, gw, gh = gx + 0.5, gy + 0.5, gw - 1.0, gh - 1.0
        if gw <= 2 or gh <= 2:
            continue
        ax.add_patch(Rectangle((gx, gy), gw, gh, fill=False, lw=1.5, ec=INK, zorder=5))
        ax.text(gx + 0.4, gy + gh - 0.15, g, ha="left", va="top", fontsize=10.5, color=INK, weight="bold", zorder=7,
                bbox=dict(boxstyle="square,pad=0.1", fc=SURFACE, ec="none"))
        strip = line_h + 0.75                    # keep the group caption clear of the file boxes
        files = files_all = {f: v for (gg, f), v in cells.items() if gg == g}
        fx0, fy0, fw0, fh0 = gx + 0.35, gy + 0.35, gw - 0.7, gh - strip - 0.35
        for f, (fx, fy, fw, fh) in squarify([(f, sum(c[1] for c in v)) for f, v in files.items()],
                                            fx0, fy0, fw0, fh0):
            fx, fy, fw, fh = fx + 0.1, fy + 0.1, fw - 0.2, fh - 0.2
            if fw <= 0 or fh <= 0:
                continue
            ax.add_patch(Rectangle((fx, fy), fw, fh, fill=False, lw=0.5, ec=MUTED, zorder=4))
            for a, n, cat, fnm in files.get(f, []):
                for _k, (x_, y_, w_, h_) in squarify([(cat, n)], fx, fy, fw, fh):
                    ax.add_patch(Rectangle((x_, y_), w_, h_, fc=colour[cat], ec="none", lw=0, antialiased=False, zorder=2))
            nm = f.rsplit("/", 1)[-1].replace(".nonmatching.txt", "").replace(".manifest.txt", "").replace(".txt", "")
            t = ax.text(fx + 0.25, fy + fh - 0.2, nm, ha="left", va="top", fontsize=FS, color=INK, zorder=6,
                        bbox=dict(boxstyle="square,pad=0.1", fc=SURFACE, ec="none", alpha=0.85))
            labels.append((t, (fx, fy, fw, fh), nm))

    # Fit the labels, horizontally first, then rotated; drop the ones that still do not fit. Two draw passes measure
    # real glyph extents, so no label can spill into its neighbour.
    def keep(t, r, rot):
        t.set_rotation(rot)
        bb = t.get_window_extent(fig.canvas.get_renderer())
        (x0, y0), (x1, y1) = ax.transData.inverted().transform([[bb.x0, bb.y0], [bb.x1, bb.y1]])
        w, h = abs(x1 - x0), abs(y1 - y0)
        fx, fy, fw, fh = r
        if w <= fw - 0.5 and h <= fh - 0.35:
            return True
        return False

    def bbox_data(t):
        bb = t.get_window_extent(fig.canvas.get_renderer())
        (x0, y0), (x1, y1) = ax.transData.inverted().transform([[bb.x0, bb.y0], [bb.x1, bb.y1]])
        return min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)

    def place(t, r, rot):
        """Rotate the label and put its *measured* box inside the cell; False when it cannot fit at this size."""
        t.set_rotation(rot)
        t.set_ha("left")
        t.set_va("top" if rot == 0 else "bottom")
        x0, y0, x1, y1 = bbox_data(t)
        fx, fy, fw, fh = r
        if x1 - x0 > fw - 0.4 or y1 - y0 > fh - 0.3:
            return False
        px, py = t.get_position()
        t.set_position((px + (fx + 0.2 - x0), py + (fy + fh - 0.2 - y1)))     # exact containment, no clipping
        return True

    placed, pending = 0, labels
    for size in (FS, FS - 0.9, FS - 1.7, 4.5):   # shrink before giving up: a smaller word beats an unlabelled box
        for t, r, _nm in pending:
            t.set_fontsize(size)
            t.set_position((r[0] + 0.2, r[1] + r[3] - 0.2))
            t.set_rotation(0)
        fig.canvas.draw()
        rot, still = [], []
        for t, r, nm in pending:
            if place(t, r, 0):
                placed += 1
                continue
            t.set_position((r[0] + 0.2, r[1] + 0.25))
            rot.append((t, r, nm))
        fig.canvas.draw()
        for t, r, nm in rot:
            if place(t, r, 90):
                placed += 1
                continue
            t.set_rotation(0)
            still.append((t, r, nm))
        pending = still
    # last resort: abbreviate the name at the smallest size (a clipped word is better than a blank box)
    for t, r, _nm in pending:
        t.set_text(_nm[:9] + "…" if len(_nm) > 10 else _nm)
        t.set_fontsize(4.5)
        t.set_position((r[0] + 0.2, r[1] + r[3] - 0.15))
    fig.canvas.draw()
    for t, r, _nm in pending:
        if place(t, r, 0):
            placed += 1
        else:
            t.remove()                           # nothing fits: the colour still carries the verdict
    labels = placed
    handles = [Patch(fc=colour[k], ec="#d8d6cf", lw=0.5,
                     label="%s - %d functions, %.1f%% of the instructions" % (label[k], counts[k], 100.0 * insn[k] / tot_i))
               for k, _l, _c in TREE_CATS if counts[k]]
    fig.legend(handles=handles, loc="lower left", bbox_to_anchor=(0.012, 0.004), frameon=False, fontsize=8.6,
               labelcolor=INK, ncol=2, handlelength=1.1, handleheight=0.9)
    fig.suptitle("Mount & Blade 1.011: %d functions (%d game/engine/3rd-party, %.2f MB, + %d CRT/STL) - area = "
                 "instructions, colour = verdict" % (tot_n, tot_n - counts["excluded"], B / 1e6, counts["excluded"]),
                 x=0.012, ha="left", fontsize=12, color=INK)
    fig.text(0.012, 0.895, "Group -> file (module) -> function; area = Ghidra instruction count.  Game %.0f%%, engine "
             "(rgl) %.0f%%, 3rd party %.0f%% of the instructions.\n"
             "Every module keeps its own box (colour needs no space); the name is drawn only where it is measured to "
             "fit - rotated when the box is narrow, abbreviated at the smallest size, omitted when even that is too big.\n"
             "Import thunks (6-byte jmp stubs at/after 0x5342a6) are not drawn: no source by design.\n"
             "Colours are verdicts: byte-exact / equivalence PASS from tools/status.py and tools/difftest/verdicts.py, "
             "WEAK / FAIL / untestable from the last sweep. The light blue is the real VC7.1 CRT/STL: byte-exact, but "
             "excluded from the totals." % tuple(100.0 * gi.get(g, 0) / tot_i for g in ("Game", "Engine (rgl)", "3rd party")),
             fontsize=8.2, color=MUTED, va="top")
    fig.savefig(path, facecolor=SURFACE)
    print("\nwrote %s (%d functions, %d module labels; %s)" % (os.path.relpath(path, ROOT), tot_n, labels,
          ", ".join("%s %d" % (k, v) for k, v in counts.items() if v)))


# Last on purpose: the chart paths are the final lines of output.
def _out(name, default):
    i = sys.argv.index(name)
    return sys.argv[i + 1] if i + 1 < len(sys.argv) and not sys.argv[i + 1].startswith("--") else W("work", default)


if "--treemap" in sys.argv:
    treemap(_out("--treemap", "treemap.png"))
if "--plot" in sys.argv:
    plot_history(_out("--plot", "progress.png"))
