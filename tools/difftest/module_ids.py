#!/usr/bin/env python3
"""module_ids.py: map a module id from before the source-tree moves (terrain_b, mesh_poly_ray, ...) to the
current module, by docs/source_tree_moves.tsv and the function's VA.

A module id is the stem of its .cpp. The 2026-11-22 restructure renamed and split many of them, but notes, docs and
habits still use the old stems. An equiv result saved as <old id>_<va>.json is then never picked up by
verdicts.py, which looks for <current id>_<va>.json (0x46e590's 2026-11-25 result, saved as terrain_b_*).

  current(old_id, va) -> (new_id, new_src) or None when old_id is not an old stem (a slice id, a current module)
  former(new_id, va)  -> the old stems that map to new_id for this VA
"""
import csv, functools, os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MOVES = os.path.join(ROOT, "docs", "source_tree_moves.tsv")


def _stem(p):
    return os.path.splitext(os.path.basename(p))[0]


@functools.lru_cache(maxsize=1)
def _moves():
    out = []
    try:
        with open(MOVES, newline="") as f:
            for r in csv.DictReader(f, delimiter="\t"):
                if not r.get("kind", "").startswith("cpp") or not r.get("old_path") or not r.get("new_path"):
                    continue
                try:
                    lo, hi = int(r["addr_lo"], 16), int(r["addr_hi"], 16)
                except (TypeError, ValueError):
                    lo = hi = None
                out.append((_stem(r["old_path"]), _stem(r["new_path"]), r["new_path"], lo, hi))
    except OSError:
        pass
    return out


def _pick(rows, va):
    """the row whose address range holds va; else the only row; else None (ambiguous)"""
    hit = [r for r in rows if r[3] is not None and r[3] <= va < r[4]]
    if len(hit) == 1:
        return hit[0]
    return rows[0] if len(rows) == 1 else None


def current(old_id, va):
    va = int(va, 16) if isinstance(va, str) else va
    seen, cur, src = set(), old_id, None
    while cur not in seen:              # follow chains of moves (renamed twice)
        seen.add(cur)
        r = _pick([m for m in _moves() if m[0] == cur and m[1] != cur], va)
        if not r:
            break
        cur, src = r[1], r[2]
    if src is None or not os.path.exists(os.path.join(ROOT, src)):
        return None
    return cur, src


def former(new_id, va):
    va = int(va, 16) if isinstance(va, str) else va
    olds = {m[0] for m in _moves() if m[0] != new_id}
    return sorted(o for o in olds if (current(o, va) or (None,))[0] == new_id)


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        sys.exit("usage: module_ids.py OLD_ID 0xVA")
    print(current(sys.argv[1], sys.argv[2]))
