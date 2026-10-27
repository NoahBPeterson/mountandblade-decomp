"""crash ledger: join docs/crashes.json to the per-function match status.

usage: crashes.py [--write]

Reads docs/crashes.json (one row per runtime fault) and docs/equiv_verdicts.json (the per-address verdict:
PASS / WEAK / FAIL / UNSUPPORTED produced by tools/difftest/equiv.py), and prints a table; --write rewrites
docs/crashes.md. An address with no verdict row is byte-exact or not yet judged - docs/STATUS.md owns that
split, so the column says "no verdict" instead of guessing.
"""
import argparse, json, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
J = os.path.join(ROOT, "docs", "crashes.json")
V = os.path.join(ROOT, "docs", "equiv_verdicts.json")
MD = os.path.join(ROOT, "docs", "crashes.md")


def verdicts():
    try:
        rows = json.load(open(V))["rows"]
    except (OSError, KeyError, ValueError):
        return {}
    return {k.lower().lstrip("0x").rjust(8, "0"): v.get("verdict", "?") for k, v in rows.items()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    a = ap.parse_args()
    data = json.load(open(J))
    vd = verdicts()
    rows = []
    for c in data["crashes"]:
        # the verdict belongs to the containing function, not to the faulting instruction
        key = c.get("fn_addr") or c["addr"]
        addr = key.lower().lstrip("0x").rjust(8, "0")
        st = vd.get(addr, "no verdict")
        rows.append((c["id"], c["addr"], c["fn"], st, c["state"], c["symptom"], c["cause"], c["fix"], c.get("where", "")))
    out = ["# Crash ledger", "", data.get("note", ""), "",
           "`status` is the function's equivalence verdict from `docs/equiv_verdicts.json` (PASS / WEAK / FAIL /"
           " UNSUPPORTED); `no verdict` means it is byte-exact or not yet judged - `docs/STATUS.md` owns that split.", "",
           "| id | address | function | status | crash state | symptom | cause | fix | where |",
           "|---|---|---|---|---|---|---|---|---|"]
    for cid, addr, fn, st, state, sym, cause, fix, where in rows:
        esc = lambda s: str(s).replace("|", "\\|").replace("\n", " ")
        out.append("| %s | `%s` | `%s` | %s | **%s** | %s | %s | %s | %s |" %
                   (cid, addr, esc(fn), st, esc(state), esc(sym), esc(cause), esc(fix), esc(where)))
    out.append("")
    n_open = sum(1 for r in rows if "open" in r[4].lower())
    out.append("%d rows, %d still open." % (len(rows), n_open))
    text = "\n".join(out) + "\n"
    print(text)
    if a.write:
        open(MD, "w").write(text)
        print("wrote", MD)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
