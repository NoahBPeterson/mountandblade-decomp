#!/usr/bin/env python3
"""sig_vs_original.py: signature drift against the original, with zero alias noise.

Every function we define is placed at the original's address by the relink (work/relink/standalone/
resolve_map.json: `names` maps our mangled name to the original VAs, `defined_at` maps a VA to the
original's mangled name). MSVC encodes a signature canonically in the mangled name - typedefs are
expanded, so `mat3*` vs `rgl::mat3*` and `tstring` vs `rgl::string` produce the SAME encoding - which
makes this pair the one oracle that does not drown in alias spellings:

    ours  ?f@C@@QAEXXZ        (public, thiscall, void(void))
    orig  ?f@C@@UAEXXZ        (public *virtual*, thiscall)      -> VIRTUAL

Rows are bucketed so only the actionable classes come first:

  VIRTUAL     the original's function is virtual and ours is not (or the reverse): the vtable shape
              differs, which is exactly the `set_track`/`enable` family of crashes.
  CONVENTION  cdecl vs thiscall vs stdcall vs fastcall: the stack is cleaned by the wrong side.
  ARITY       a different number of stack parameters (`add_order_arrow(int,int)` vs `ret 4`).
  RETURN      same params, different return encoding (struct-by-value vs out-parameter, `void` vs
              `int`): usually ABI-compatible, listed behind --return.
  OTHER       a parameter type of the same shape but another name (a struct renamed): benign alias.

usage: sig_vs_original.py [--return] [--other] [--limit N] [--json OUT]
"""
import argparse, json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RMAP = os.path.join(ROOT, "work", "relink", "standalone", "resolve_map.json")

VIRTUAL_ACCESS = set("UEM")          # public/protected/private virtual
CONVENTIONS = {"A": "cdecl", "E": "thiscall", "G": "stdcall", "I": "fastcall"}
SIMPLE = set("HIJKLMNDEFG")          # one-character type codes (M/N/D/E also used as codes)


def split_mangled(name):
    i = name.find("@@")
    if i < 0:
        return name, ""
    return name[:i + 2], name[i + 2:]


def parse_encoding(enc):
    """(access_is_virtual, convention, param_count) from 'QAEXXZ' / 'UAE@XZ' / 'YAXH@Z'; None on doubt."""
    if not enc:
        return None
    p = 0
    if enc[0] == "Y":                      # free function: Y <conv> <ret> <params>
        if len(enc) < 2:
            return None
        conv = enc[1]
        p = 2
    else:                                  # member: <access+virtual> <storage> <conv> <ret> <params>
        if len(enc) < 3:
            return None
        conv = enc[2]
        p = 3
    virtual = enc[0] in VIRTUAL_ACCESS
    if conv not in CONVENTIONS:
        return None

    def skip_qualified(s, i):
        # '?AUvector4@rgl@@' / '?AV?$vector@...@std@@' -> index past the closing '@'
        if i >= len(s) or s[i] != "?":
            return i
        i += 1
        if i < len(s) and s[i] in "AVU":
            i += 1
        # walk to the last '@' of the qualified name (the terminator is '@' not followed by '?')
        depth = 0
        while i < len(s):
            c = s[i]
            if c == "?":
                depth += 1
                i += 1
                if i < len(s) and s[i] in "AVU":
                    i += 1
                continue
            if c == "@":
                if depth > 0:
                    i += 1
                    depth -= 1
                    continue
                i += 1
                if i < len(s) and s[i] == "@":
                    i += 1
                return i
            i += 1
        return i

    def one_type(s, i):
        # returns (ok, next_index); counts as exactly one stack parameter unless it is 'X' (void)
        if i >= len(s):
            return (False, i)
        c = s[i]
        if c == "X":
            return (True, i + 1)
        if c == "_" and s[i:i + 2] == "_N":
            return (True, i + 2)
        if c in "PQAB":                    # pointer / const pointer / reference / const reference
            i += 1
            if i < len(s) and s[i] == "?":
                j = skip_qualified(s, i)
                return (True, j)
            if i < len(s):
                return (True, i + 1)
            return (False, i)
        if c == "?":
            return (True, skip_qualified(s, i))
        if c in SIMPLE or c in "UVW":
            return (True, i + 1)
        return (False, i)

    # return type (a constructor/destructor has no return: the '@' marker stands there instead)
    if p < len(enc) and enc[p] == "@":
        p += 1
    ret_start = p
    ok, i = one_type(enc, p)
    if not ok:
        return None
    params_start = i
    count = 0
    while i < len(enc) and enc[i] != "Z":
        ok, i = one_type(enc, i)
        if not ok:
            return None
        count += 1
    if i >= len(enc):
        return None
    if count == 1 and enc[params_start] == "X":   # a single 'X' means "no parameters"
        count = 0
    return (virtual, CONVENTIONS[conv], count)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--return", dest="show_return", action="store_true",
                    help="also list RETURN rows (struct-by-value vs out-parameter)")
    ap.add_argument("--other", dest="show_other", action="store_true", help="also list OTHER (renamed struct) rows")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json")
    a = ap.parse_args()

    m = json.load(open(RMAP))
    buckets = {"VIRTUAL": [], "CONVENTION": [], "ARITY": [], "RETURN": [], "OTHER": [], "RAW-DIFF": []}
    skipped = 0
    mismatched_names = 0
    for ours, per_va in m.get("names", {}).items():
        if not isinstance(per_va, dict):
            continue
        for va in per_va:
            ent = m.get("defined_at", {}).get(va)
            if not ent:
                continue
            orig = ent[0] if isinstance(ent, (list, tuple)) else None
            if not orig:
                continue
            op, oe = split_mangled(ours)
            np_, ne = split_mangled(orig)
            if "@std@@" in ours or "@std@@" in orig:
                continue
            if op != np_:
                mismatched_names += 1       # a different function of ours sits at that VA: a placement question
                continue
            po, pn = parse_encoding(oe), parse_encoding(ne)
            if oe == ne:
                continue
            if po is None or pn is None:
                skipped += 1
                kind = "RAW-DIFF"
            elif po[0] != pn[0]:
                kind = "VIRTUAL"
            elif po[1] != pn[1]:
                kind = "CONVENTION"
            elif po[2] != pn[2]:
                kind = "ARITY"
            elif oe.split("@")[0][2:] != ne.split("@")[0][2:]:
                kind = "RETURN"
            else:
                kind = "OTHER"
            buckets[kind].append({"va": va, "ours": ours, "orig": orig,
                                  "parsed_ours": po, "parsed_orig": pn})
    total = sum(len(v) for v in buckets.values())
    print("sig_vs_original: %d signature rows (%d unparsed encodings, %d VAs where a different function of ours "
          "sits at the original's address)" % (total, skipped, mismatched_names))
    for kind in ("VIRTUAL", "CONVENTION", "ARITY", "RETURN", "OTHER", "RAW-DIFF"):
        rows = buckets[kind]
        if not rows:
            continue
        if kind == "RETURN" and not a.show_return:
            print("  %-10s %d rows (--return to list)" % (kind, len(rows)))
            continue
        if kind in ("OTHER", "RAW-DIFF") and not a.show_other:
            print("  %-10s %d rows (--other to list)" % (kind, len(rows)))
            continue
        print("  %-10s %d rows" % (kind, len(rows)))
        for r in rows[: a.limit]:
            print("    %s  ours %-44s %s  |  orig %s" %
                  (r["va"], r["ours"], r["parsed_ours"], r["orig"]))
        if len(rows) > a.limit:
            print("    ... %d more" % (len(rows) - a.limit))
    if a.json:
        json.dump(buckets, open(a.json, "w"), indent=1)
    return 0 if not (buckets["VIRTUAL"] or buckets["CONVENTION"] or buckets["ARITY"]) else 1


if __name__ == "__main__":
    sys.exit(main())
