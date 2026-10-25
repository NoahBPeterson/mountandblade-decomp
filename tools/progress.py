#!/usr/bin/env python3
"""Print Mount & Blade decompilation progress as terminal tables. Standalone: run from any shell.

  .venv/bin/python tools/progress.py                 # coverage + per-batch tables
  .venv/bin/python tools/progress.py --plot [out.png]  # chart of progress over time (default work/progress.png;
                                                       # one point per committed docs/STATUS.md and one per
                                                       # --plot run, kept in docs/progress_history.json; needs matplotlib)

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
exact, equiv, partial = S["exact"] & game, S["equiv"] & game, S["partial"] & game
done = exact | equiv
migrated = S["migrated"] & done


def table(headers, rows):
    w = [max(len(str(r[i])) for r in [headers] + rows) for i in range(len(headers))]
    fmt = lambda r: "  ".join((str(c).ljust(w[i]) if i == 0 else str(c).rjust(w[i])) for i, c in enumerate(r))
    return "\n".join([fmt(headers), "  ".join("-" * x for x in w)] + [fmt(r) for r in rows])


nbytes = lambda s: sum(size[a] for a in s)
pc = lambda n, d: "%.2f%%" % (100.0 * n / d) if d else "-"
row = lambda label, s: [label, len(s), pc(len(s), G), "%.1f KB" % (nbytes(s) / 1024), pc(nbytes(s), B)]
print("Mount & Blade 1.011 decompilation: %d game functions, %.2f MB (static CRT/C++ library excluded)\n" % (G, B / 1e6))
print(table(["Tier", "Functions", "Share", "Code size", "Share (size)"],
            [row("Byte-exact", exact), row("Equivalent, not exact", equiv), row("Total with compilable source", done),
             row("Partial (not counted)", partial), row("No source yet", game - done - partial)]))
print()
print(table(["Source tree", "Functions", "Share", "Code size", "Share (size)"],
            [row("Migrated into src/", migrated), row("Compilable, still only in match/slices", done - migrated)]))

# Equivalence evidence for the equivalent-only rows (tools/difftest/verdicts.py, from the latest sweep)
EV_CATS = (("pass", "PASS: matched, every branch outcome taken"), ("weak", "WEAK: matched, some branch outcomes never taken"),
           ("fail", "FAIL: a mismatch (see the row's note)"), ("inputs", "Untestable: random inputs invalid"),
           ("harness", "Untestable: harness limitation"), ("funclet", "Funclet / cold tail (tested with parent)"),
           ("not_run", "Not run"))
try:
    ev = json.load(open(W("docs", "equiv_verdicts.json")))
except (OSError, ValueError):
    ev = None
if ev:
    s = ev["summary"]
    tot, tsz = sum(s[c] for c, _ in EV_CATS), sum(s[c + "_sz"] for c, _ in EV_CATS)
    print()
    print("Equivalence evidence for the %d equivalent-only module rows (sweep of %s, commit %s):" % (tot, ev["when"][:16].replace("T", " "), ev["commit"]))
    print(table(["Verdict", "Rows", "Share", "Code size", "Share (size)"],
                [[lab, s[c], pc(s[c], tot), "%.1f KB" % (s[c + "_sz"] / 1024), pc(s[c + "_sz"], tsz)] for c, lab in EV_CATS if s[c]]))

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


def parse_status_md(md):
    """Shares from one committed docs/STATUS.md (tools/status.py format) -> dict or None."""
    out = {}
    for line in md.splitlines():
        cells = [c.strip().strip("*").strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 5 or not cells[2].endswith("%"):
            continue
        key = {"byte-exact": "exact", "total with compilable source": "comp",
               "migrated into src/": "mig"}.get(cells[0].lower())
        if key:
            out[key] = float(cells[2].rstrip("%"))
            out[key + "_sz"] = float(cells[4].rstrip("%"))
    return out if "exact" in out and "comp" in out else None


def plot_history(path):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.dates as mdates
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

    # dataviz reference palette (validated light): categorical slots 1-3, surface and text tokens
    BLUE, ORANGE, AQUA = "#2a78d6", "#eb6834", "#1baf7a"
    SURFACE, INK, MUTED, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e6e5e0"
    top = max([p[k] for _, p in pts for k in ("comp", "comp_sz") if p.get(k) is not None] + [0])
    ymax = 100 if top > 40 else max(5, round(top * 1.4 + 0.5))
    try:
        evh = sorted(json.load(open(W("docs", "equiv_history.json"))).values(), key=lambda p: p["when"])
    except (OSError, ValueError):
        evh = []

    def style(ax, title, times):
        ax.set_facecolor(SURFACE)
        ax.set_title(title, loc="left", fontsize=11, color=INK)
        ax.yaxis.set_major_formatter(PercentFormatter(decimals=0))
        ax.grid(axis="y", color=GRID, lw=1)
        ax.set_axisbelow(True)
        for s in ("top", "right", "left"):
            ax.spines[s].set_visible(False)
        ax.spines["bottom"].set_color(GRID)
        ax.tick_params(colors=MUTED, labelsize=9, length=0)
        # Calendar dates only, never hours: one label per local calendar day, under the middle of that day's points
        # (every Nth day once the history passes 8 days). Day locators would put ticks at UTC midnights, and a day
        # whose midnight lies outside the plotted range would get no label at all.
        byday = {}
        for t in times:
            byday.setdefault(t.astimezone(now.tzinfo).date(), []).append(t)
        days = sorted(byday)[::max(1, -(-len(byday) // 8))]
        ax.set_xticks([min(byday[d]) + (max(byday[d]) - min(byday[d])) / 2 for d in days])
        ax.set_xticklabels([d.strftime("%b %-d") for d in days])
        ax.margins(x=0.08)

    fig, grid = plt.subplots(2 if evh else 1, 2, figsize=(12, 9.4 if evh else 4.6), dpi=150, squeeze=False)
    axes = grid[0]
    fig.patch.set_facecolor(SURFACE)
    for ax, (title, ke, kc, km) in zip(axes, (("Share of functions", "exact", "comp", "mig"),
                                              ("Share of code size", "exact_sz", "comp_sz", "mig_sz"))):
        ax.set_facecolor(SURFACE)
        for key, color, label in ((kc, ORANGE, "Compilable (byte-exact + equivalent)"), (ke, BLUE, "Byte-exact"),
                                  (km, AQUA, "Migrated into src/ modules")):
            # snapshots from before the migrated row existed had nothing in src/: their migrated share is 0
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
        # Equivalence evidence of the equivalent-only rows, stacked bottom-up from strongest to none. Verdicts are
        # states, so they wear the reserved status colors (good / warning / critical) and grays for "no evidence".
        EVC = {"pass": "#0ca30c", "weak": "#fab219", "fail": "#d03b3b", "inputs": "#898781", "harness": "#b4b2ab",
               "funclet": "#d9d7d0", "not_run": "#efeee9"}
        xs = [datetime.datetime.fromisoformat(p["when"]) for p in evh]
        for ax, suffix, title in ((grid[1][0], "", "Equivalent-only rows by test verdict (share of rows)"),
                                  (grid[1][1], "_sz", "... by code size")):
            keys = [c for c, _ in EV_CATS if any(p.get(c + "_sz") for p in evh) or c == "funclet" and any(p.get(c) for p in evh)]
            tot = [sum(p.get(c + suffix, 0) for c, _ in EV_CATS) or 1 for p in evh]
            ys = [[100.0 * p.get(c + suffix, 0) / t for p, t in zip(evh, tot)] for c in keys]
            ax.stackplot(xs, ys, colors=[EVC[c] for c in keys], edgecolor=SURFACE, linewidth=1.5,
                         labels=[dict(EV_CATS)[c] for c in keys])
            base = 0.0
            for c, y in zip(keys, ys):          # direct labels at the latest point for bands >= 4 %
                if y[-1] >= 4:
                    ax.annotate("%.0f%%" % y[-1], (xs[-1], base + y[-1] / 2), xytext=(6, 0), textcoords="offset points",
                                va="center", fontsize=9, color=INK)
                base += y[-1]
            ax.set_ylim(0, 100)
            style(ax, title, xs)
        grid[1][1].tick_params(labelleft=False)
        h, l = grid[1][0].get_legend_handles_labels()
        fig.legend(h, l, loc="lower center", ncol=3, frameon=False, fontsize=8.5, labelcolor=INK,
                   bbox_to_anchor=(0.5, 0.075))
    fig.suptitle("Mount & Blade 1.011 decompilation progress (%d game functions, %.2f MB; static CRT excluded)" % (G, B / 1e6),
                 x=0.01, ha="left", fontsize=12, color=INK)
    fig.text(0.01, 0.01, "One point per committed docs/STATUS.md and one per --plot run. "
             "Equivalent = complete compilable source, not yet byte-identical.\nMigrated = moved from match/slices "
             "into one .cpp per original translation unit (0%% before the first migration).%s" %
             ("" if ymax == 100 else "\nThe y-axis is zoomed to %d%% while progress is low." % ymax) +
             ("\nVerdicts: one point per full equivalence sweep (tools/difftest/verdicts.py); a sweep tests the original and "
              "our function\non the same random inputs under emulation. The first point also counted pre-migration slice rows."
              if evh else ""), fontsize=8, color=MUTED)
    fig.tight_layout(rect=(0, 0.15 if evh else 0.09, 1, 0.95))
    fig.savefig(path, facecolor=SURFACE)
    print("\nwrote %s (%d snapshots)" % (os.path.relpath(path, ROOT), len(pts)))


# Last on purpose: the plot path is the final line of output.
if "--plot" in sys.argv:
    i = sys.argv.index("--plot")
    plot_history(sys.argv[i + 1] if i + 1 < len(sys.argv) and not sys.argv[i + 1].startswith("--")
                 else W("work", "progress.png"))
