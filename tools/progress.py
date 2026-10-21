#!/usr/bin/env python3
"""Print Mount & Blade decompilation progress as terminal tables. Standalone: run from any shell.

  .venv/bin/python tools/progress.py                 # coverage + per-batch tables
  .venv/bin/python tools/progress.py --plot [out.png]  # chart of progress over time (default work/progress.png;
                                                       # one point per committed docs/STATUS.md and one per day of
                                                       # --plot runs, kept in docs/progress_history.json; needs matplotlib)

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
        key = {"byte-exact": "exact", "total with compilable source": "comp"}.get(cells[0].lower())
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
    live = {"exact": 100.0 * len(exact) / G, "comp": 100.0 * len(done) / G,
            "exact_sz": 100.0 * nbytes(exact) / B, "comp_sz": 100.0 * nbytes(done) / B}
    # One saved point per day: each run overwrites today's entry, so a day keeps its latest figure.
    hist_path = W("docs", "progress_history.json")
    try:
        hist = json.load(open(hist_path))
    except (OSError, ValueError):
        hist = {}
    today = now.date().isoformat()
    hist[today] = dict(live, when=now.isoformat(timespec="seconds"))
    with open(hist_path, "w") as f:
        json.dump(dict(sorted(hist.items())), f, indent=1)
        f.write("\n")
    pts += [(datetime.datetime.fromisoformat(p["when"]), p) for d, p in hist.items() if d != today]
    pts.append((now, live))
    pts.sort(key=lambda t: t[0])

    # dataviz reference palette (validated light): categorical slots 1-2, surface and text tokens
    BLUE, ORANGE = "#2a78d6", "#eb6834"
    SURFACE, INK, MUTED, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e6e5e0"
    top = max([p[k] for _, p in pts for k in ("comp", "comp_sz") if p.get(k) is not None] + [0])
    ymax = 100 if top > 40 else max(5, round(top * 1.4 + 0.5))
    fig, axes = plt.subplots(1, 2, figsize=(12, 4.6), dpi=150, sharey=True, sharex=True)
    fig.patch.set_facecolor(SURFACE)
    for ax, (title, ke, kc) in zip(axes, (("Share of functions", "exact", "comp"),
                                          ("Share of code size", "exact_sz", "comp_sz"))):
        ax.set_facecolor(SURFACE)
        for key, color, label in ((kc, ORANGE, "Compilable (byte-exact + equivalent)"), (ke, BLUE, "Byte-exact")):
            xs = [t for t, p in pts if p.get(key) is not None]
            ys = [p[key] for t, p in pts if p.get(key) is not None]
            if not xs:
                continue
            ax.plot(xs, ys, color=color, lw=2, marker="o", ms=4, label=label, solid_capstyle="round")
            ax.annotate("%.1f%%" % ys[-1], (xs[-1], ys[-1]), xytext=(6, 0), textcoords="offset points",
                        va="center", fontsize=9, color=INK)
        ax.set_title(title, loc="left", fontsize=11, color=INK)
        ax.set_ylim(0, ymax)
        ax.yaxis.set_major_formatter(PercentFormatter(decimals=0))
        ax.grid(axis="y", color=GRID, lw=1)
        ax.set_axisbelow(True)
        for s in ("top", "right", "left"):
            ax.spines[s].set_visible(False)
        ax.spines["bottom"].set_color(GRID)
        ax.tick_params(colors=MUTED, labelsize=9, length=0)
        loc = mdates.AutoDateLocator(minticks=3, maxticks=8)   # hours while the history is short, days later
        ax.xaxis.set_major_locator(loc)
        ax.xaxis.set_major_formatter(mdates.ConciseDateFormatter(loc))
        ax.margins(x=0.08)
    axes[0].legend(loc="upper left", frameon=False, fontsize=9, labelcolor=INK)
    fig.suptitle("Mount & Blade 1.011 decompilation progress (%d game functions, %.2f MB; static CRT excluded)" % (G, B / 1e6),
                 x=0.01, ha="left", fontsize=12, color=INK)
    fig.text(0.01, 0.01, "One point per committed docs/STATUS.md, one per day of --plot runs (latest run that day), plus now. "
             "Equivalent = complete compilable source, not yet byte-identical.%s" %
             ("" if ymax == 100 else "\nThe y-axis is zoomed to %d%% while progress is low." % ymax), fontsize=8, color=MUTED)
    fig.tight_layout(rect=(0, 0.07, 1, 0.95))
    fig.savefig(path, facecolor=SURFACE)
    print("\nwrote %s (%d snapshots)" % (os.path.relpath(path, ROOT), len(pts)))


# Last on purpose: the plot path is the final line of output.
if "--plot" in sys.argv:
    i = sys.argv.index("--plot")
    plot_history(sys.argv[i + 1] if i + 1 < len(sys.argv) and not sys.argv[i + 1].startswith("--")
                 else W("work", "progress.png"))
