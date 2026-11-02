#!/usr/bin/env python3
"""vtable_audit.py: the work list for "our compiled vtables need no fix-up".

usage: vtable_audit.py [--out work/relink/standalone] [--json FILE] [--vt REGEX] [--slots] [--module STEM]

For every original vtable that resolve_map matched to one of ours, prints the original's slot list (original function
-> our symbol for it) beside the slots our compiler emitted, classifies each differing slot and each table, and groups
the tables by the module (object) that defines our vtable. The counts are the fan-out work list for the build.py
vtable fix-up (build.json's vtable_report: slots rewritten, vptr stores redirected, skipped, thunk_slots,
glue_bound_slots, vtbl_externs, dtor_slots): when every table is "ok" the fix-up has nothing left to do.

Our compiled table is read from OUT/mb_standalone.prefix.exe (build.py saves the linked exe before its fix-up) with
OUT/mb_standalone.map; without it the ICF trial link OUT/icf/icf.exe + icf.map is used (never fixed up, but identical
functions are folded to one address there, so a slot lists every name at its address). A slot whose function is not the
original's but is the same code (the two fold to one address in OUT/icf/icf.map, as in the original's own link) is equal.

Slot classes (one per differing slot):
  stub         our slot is a bare `jmp` to the function the original's slot names (a forwarding stub in source)
  placeholder  our slot is a placeholder virtual (s00 / d10 / vfuncN / slotN ...) or __purecall
  foreign      our slot is a real function, but one the original's slot is not (a base default, another class's method,
               a free function: the override is missing, or the class has the wrong base)
  undefined    the original's slot function is one no row of ours defines (nothing to bind to)
  dtor         slot 0, both sides deleting destructors that are different functions
  beyond       the original has the slot, our table ends before it (short table: base slots not declared)
Table classes (a table may have several): short, long, suspect (fewer than half the comparable slots agree: build.py
skips the rewrite), hand (our slots are free functions: a hand-rolled table), plus one per slot class above.
"""
import argparse, bisect, collections, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
from build import Exe, read_map, read_map_by_object, jmp_target, is_deleting_dtor, vtable_starts, slot_equivalent   # noqa: E402

PLACEHOLDER = re.compile(r"^\?(?:[sdfv]\d{1,3}|vfunc\d*|slot\d+|unk\w*|virt\w*|pure\d*)@")


def module_of(sym, by_obj):
    for (n, stem), _a in by_obj.items():
        if n == sym:
            return stem
    return "?"


def audit(out, only=None):
    import resolve as R
    rmap = json.load(open(os.path.join(out, "resolve_map.json")))
    pre = os.path.join(out, "mb_standalone.prefix.exe")
    if os.path.exists(pre):
        exe, mp, src = Exe(pre), os.path.join(out, "mb_standalone.map"), "prefix"
    else:
        exe, mp, src = Exe(os.path.join(out, "icf", "icf.exe")), os.path.join(out, "icf", "icf.map"), "icf"
    symmap = read_map(mp)
    icf_path = os.path.join(out, "icf", "icf.map")
    icf_map = read_map(icf_path) if os.path.exists(icf_path) else {}      # the oracle for "same code" (identical functions folded)
    by_addr = collections.defaultdict(list)
    for n, a in symmap.items():
        if isinstance(a, int):
            by_addr[a].append(n)
    sym_addrs = sorted(by_addr)
    img = R.Image()
    lib_names = {}
    for line in open(os.path.join(ROOT, "symbols", "lib_names.txt")):
        p = line.split()
        if p and len(p[0]) == 8 and p[1].startswith("lib_crt::"):
            lib_names.setdefault(int(p[0], 16), p[1][9:])
    defined_at = {int(k, 16): v[0] for k, v in rmap["defined_at"].items()}
    by_obj = read_map_by_object(os.path.join(out, "mb_standalone.map")) if os.path.exists(
        os.path.join(out, "mb_standalone.map")) else {}
    mod_of = {n: stem for (n, stem) in by_obj}
    vt_starts = vtable_starts(img)
    bj = os.path.join(out, "build.json")
    skipped = set(json.load(open(bj))["vtable_report"]["skipped"]) if os.path.exists(bj) else set()
    glue_bound = set(json.load(open(bj))["vtable_report"].get("glue_bound_slots", [])) if os.path.exists(bj) else set()
    purecall = symmap.get("__purecall")
    tables = []
    for vt, e in sorted(rmap["vtables"].items()):
        if only and not only.search(vt):
            continue
        if len(e["orig"]) != 1 or vt not in symmap:
            tables.append({"vt": vt, "orig": e["orig"], "module": "?", "classes": ["unmatched"], "slots": []})
            continue
        ova, ours = int(e["orig"][0], 16), symmap[vt]
        n_orig = 0
        while n_orig < 512 and img.u32(ova + 4 * n_orig) in img.start_set:
            n_orig += 1
        j = bisect.bisect_right(vt_starts, ova)
        n_true = min(n_orig, (vt_starts[j] - ova) // 4) if j < len(vt_starts) else n_orig
        i = bisect.bisect_right(sym_addrs, ours)
        n_ours = min(e["slots"], (sym_addrs[i] - ours) // 4) if i < len(sym_addrs) else e["slots"]
        rows, classes, agree, comparable = [], set(), 0, 0
        if n_ours < n_true:
            classes.add("short")
        elif n_ours > n_true:
            classes.add("long")
        for k in range(max(n_true, n_ours)):
            t = img.u32(ova + 4 * k) if k < n_true else None
            have = exe.u32(ours + 4 * k) if k < n_ours else None
            want = (defined_at.get(t) or lib_names.get(t)) if t is not None else None
            hn = by_addr.get(have, []) if have is not None else []
            row = {"k": k, "orig": "%08x" % t if t is not None else None, "want": want,
                   "ours": "%08x" % have if have is not None else None, "have": hn[:4], "cls": "ok"}
            if t is None:
                row["cls"] = "long"
            elif have is None:
                row["cls"] = "beyond"
            elif want is None:
                row["cls"] = "undefined"
                comparable += 1
            else:
                w = symmap.get(want)
                comparable += 1
                if w == have or want in hn or slot_equivalent(icf_map, want, hn):
                    agree += 1
                elif k == 0 and is_deleting_dtor(want) and any(is_deleting_dtor(n) for n in hn):
                    row["cls"] = "dtor"
                else:
                    tg = jmp_target(exe, have)
                    if tg is not None and (tg == w or want in by_addr.get(tg, []) or slot_equivalent(icf_map, want, by_addr.get(tg, []))):
                        row["cls"] = "stub"
                        agree += 1
                    elif have == purecall or any(PLACEHOLDER.match(n) for n in hn):
                        row["cls"] = "placeholder"
                    else:
                        row["cls"] = "foreign"
            if row["cls"] != "ok":
                classes.add(row["cls"])
            rows.append(row)
        if hn_free(rows):
            classes.add("hand")
        if classes and (vt in skipped or (comparable and agree * 2 < comparable)):
            classes.add("suspect")      # (build.py skips a table with under half of its slots exactly equal; a table with no differing slot is fine)
        if vt in glue_bound:
            classes.add("glue")
        tables.append({"vt": vt, "orig": "%08x" % ova, "module": mod_of.get(vt, "?"), "ours_slots": n_ours,
                       "orig_slots": n_true, "agree": agree, "comparable": comparable,
                       "classes": sorted(classes) or ["ok"], "slots": rows})
    return tables, src


def hn_free(rows):
    """Most differing slots point at free functions (?name@@YA...): a hand-rolled table."""
    bad = [r for r in rows if r["cls"] not in ("ok", "long", "beyond")]
    return bool(bad) and all(any("@@YA" in n or "@@YI" in n for n in r["have"]) for r in bad)


def summarize(tables):
    mods = collections.OrderedDict()
    for t in tables:
        m = mods.setdefault(t["module"], {"tables": 0, "ok": 0, "slots_bad": collections.Counter(),
                                          "table_classes": collections.Counter()})
        m["tables"] += 1
        m["ok"] += t["classes"] == ["ok"]
        for c in t["classes"]:
            m["table_classes"][c] += 1
        for r in t["slots"]:
            if r["cls"] != "ok":
                m["slots_bad"][r["cls"]] += 1
    return mods


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "relink", "standalone"))
    ap.add_argument("--json")
    ap.add_argument("--vt", help="regex over the vtable symbol")
    ap.add_argument("--module", help="only tables defined by this module stem (substring)")
    ap.add_argument("--slots", action="store_true", help="print every slot of the listed tables, not just the summary")
    a = ap.parse_args()
    tables, src = audit(a.out, re.compile(a.vt) if a.vt else None)
    if a.module:
        tables = [t for t in tables if a.module in t["module"]]
    mods = summarize(tables)
    if a.json:
        json.dump({"source": src, "tables": tables,
                   "modules": {m: {"tables": v["tables"], "ok": v["ok"], "slots_bad": dict(v["slots_bad"]),
                                   "table_classes": dict(v["table_classes"])} for m, v in mods.items()}},
                  open(a.json, "w"), indent=1)
    print("vtable audit (our slots from %s): %d original vtables matched, %d fully equal" % (
        src, len(tables), sum(t["classes"] == ["ok"] for t in tables)))
    tot_slots, tot_tab = collections.Counter(), collections.Counter()
    for t in tables:
        for c in t["classes"]:
            tot_tab[c] += 1
        for r in t["slots"]:
            if r["cls"] != "ok":
                tot_slots[r["cls"]] += 1
    print("tables by class: " + ", ".join("%s %d" % kv for kv in sorted(tot_tab.items())))
    print("differing slots by class: " + ", ".join("%s %d" % kv for kv in sorted(tot_slots.items())))
    print("\n%-28s %6s %4s  %s" % ("module", "tables", "ok", "differing slots | table classes"))
    for m, v in sorted(mods.items(), key=lambda kv: -(kv[1]["tables"] - kv[1]["ok"])):
        print("%-28s %6d %4d  %s | %s" % (m[:28], v["tables"], v["ok"],
                                           " ".join("%s=%d" % kv for kv in sorted(v["slots_bad"].items())),
                                           " ".join("%s=%d" % kv for kv in sorted(v["table_classes"].items()) if kv[0] != "ok")))
    if a.slots or a.vt or a.module:
        for t in tables:
            if t["classes"] == ["ok"]:
                continue
            print("\n%s  orig %s  [%s]  %s  ours %s / orig %s slots" % (
                t["vt"], t["orig"], t["module"], ",".join(t["classes"]), t.get("ours_slots"), t.get("orig_slots")))
            for r in t["slots"]:
                if r["cls"] != "ok":
                    print("   %2d %-11s want %s %s | ours %s %s" % (r["k"], r["cls"], r["orig"], r["want"], r["ours"],
                                                                 ",".join(r["have"][:2])))


if __name__ == "__main__":
    main()
