#!/usr/bin/env python3
"""type_burndown.py: one history line per gate for the type-recovery plan (views, raw offsets, 64-bit worklist, link
traps, byte-exact rows) -> docs/type_burndown.json. Reads only generated files, so it costs nothing:

  docs/views_inventory.md      (tools/port/find_views.py)       views, view cast sites, 64-bit-wrong views
  docs/port64_worklist.tsv     (tools/port/port64_worklist.py)  W/V/L/C/I levels and every category
  docs/tickets/022-offset-sites.tsv or --offsets FILE (tools/port/offset_sites.py)  raw-offset sites
  port/link/{,macos-arm64/}tables.json (tools/port/link_tables.py)  trapped names (stubs that abort)

    tools/port/type_burndown.py [--rows "5003 / 5003"] [--offsets work/offset_sites_now.tsv] [--note TEXT] [--print]

work/commit_frozen.sh calls it after its gates (2026-11-20, the user: "Are we keeping counts anywhere and burning it
down?"). Each entry is keyed by time and carries the commit; the counts are whatever the lanes last committed.
"""
import argparse, collections, csv, datetime, json, os, re, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "docs", "type_burndown.json")


def rel(*p):
    return os.path.join(ROOT, *p)


def views():
    t = open(rel("docs", "views_inventory.md"), encoding="utf-8").read()
    m = re.search(r"^\| all \| (\d+) \| (\d+) \| (\d+) \| (\d+) \|", t, re.M)
    return {"views": int(m.group(1)), "views_64wrong": int(m.group(2)), "view_casts": int(m.group(4))} if m else {}


def worklist():
    p = rel("docs", "port64_worklist.tsv")
    if not os.path.exists(p):
        return {}
    rows = list(csv.DictReader(open(p, encoding="utf-8"), delimiter="\t"))
    lv = collections.Counter(r["level"] for r in rows)
    cat = collections.Counter(r["category"] for r in rows)
    d = {"w64_" + k: lv[k] for k in "WVLCI"}
    d["w64_classes_W"] = len({r["class"] for r in rows if r["level"] == "W"})
    d["raw_offsets"] = cat["raw_moved"] + cat["raw_pad"] + cat["raw_ok"]
    d["int_ptr"] = cat["int_ptr"]
    d["pad_ptr"] = cat["pad_ptr"]
    d["slot_calls"] = cat["vslot"] + cat["vtable_view"]
    return d


def offsets(path):
    p = path or rel("docs", "tickets", "022-offset-sites.tsv")
    if not os.path.exists(p):
        return {}
    n = sum(1 for _ in open(p, encoding="utf-8")) - 1
    return {"offset_sites": n, "offset_sites_at": datetime.datetime.fromtimestamp(os.path.getmtime(p)).isoformat(timespec="minutes")}


def traps():
    d = {}
    for key, p in (("traps_i686", ("port", "link", "tables.json")), ("traps_macos", ("port", "link", "macos-arm64", "tables.json"))):
        try:
            d[key] = len(json.load(open(rel(*p))).get("trapped", []))
        except (OSError, ValueError):
            pass
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rows", help='run_all.py\'s "N / M" byte-exact manifest rows')
    ap.add_argument("--offsets", help="a fresher offset_sites.py TSV than ticket 022's")
    ap.add_argument("--note")
    ap.add_argument("--print", action="store_true", help="print only, record nothing")
    a = ap.parse_args()
    e = {"commit": subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, capture_output=True, text=True).stdout.strip()}
    for f in (views, worklist, traps):
        e.update(f())
    e.update(offsets(a.offsets))
    if a.rows:
        m = re.match(r"\s*(\d+)\s*/\s*(\d+)", a.rows)
        if m:
            e["exact_rows"], e["manifest_rows"] = int(m.group(1)), int(m.group(2))
    if a.note:
        e["note"] = a.note
    line = " ".join("%s=%s" % (k, v) for k, v in e.items() if k not in ("commit", "offset_sites_at", "note"))
    print("burndown " + line)
    if a.print:
        return
    h = json.load(open(OUT)) if os.path.exists(OUT) else {"about": __doc__.split("\n\n")[0], "history": {}}
    h["history"][datetime.datetime.now().astimezone().isoformat(timespec="seconds")] = e
    tmp = OUT + ".tmp"
    json.dump(h, open(tmp, "w"), indent=1)
    os.replace(tmp, OUT)


if __name__ == "__main__":
    main()
