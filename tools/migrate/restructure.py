#!/usr/bin/env python3
"""restructure.py: carry out the source-tree restructure planned in docs/source_tree.md.

Driven by docs/source_tree_moves.tsv (one row per src file: old path -> new path; split rows carry cut lines, text
markers and address ranges) plus the user's decisions of docs/source_tree.md section 8, which override the TSV:
party_2.cpp stays whole (moved 1:1 to game/party/party_map.cpp), agent_ai.cpp stays whole, agent_hud.cpp goes to
game/ui/hud/, one include root src/ with module-prefixed includes, notes files to docs/migration/. One deviation: TinyXML
goes to src/third_party/tinyxml (not a top-level third_party/) so it stays under the single include root and under
every tool that walks src/.

  restructure.py a            batch A: every 1:1 move (git mv; keeps history), the three files that are split in
                              batch B stay where they are; then every path reference is rewritten
  restructure.py b            batch B: the splits (agent_logic -> 11, script_interpreter -> 3, mission_5 -> 3)
  restructure.py a --check    nothing is changed; list what is left to do and any stale reference
  restructure.py verify       scan for references to old paths / old directory names

Rerunnable and idempotent: a move whose source is gone and whose target exists is skipped, and the text rewrites only
fire on old spellings, which a finished run no longer contains.

What the rewrite pass touches (every tracked text file; never docs/source_tree*, work/, the session transcript):
  * #include lines (src/, match/, patches/, port/ ...): resolved the way cl / clang resolved them in the OLD tree
    (the including file's directory, then src/include, match/include [port/include, port/stl for port/]) and
    rewritten to the src/-rooted form "game/data/troop.h"; includes that resolve outside src stay as they are.
  * every path-like token that is a whole old path or a path-component suffix of exactly one old path
    (src/game/agent4/agent_logic.cpp, ../src/game/..., agent2/agent_stats.cpp, comment text, manifests' first field,
    tu_order.txt, port/link tables, symbols/*.json ...): replaced by the new path.
  * python joins os.path.join(ROOT, "src", "game", "config", "config.cpp").
  * port/tu/<path>.h per-TU preludes follow their .cpp.
"""
import argparse, csv, json, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
TSV = os.path.join(ROOT, "docs", "source_tree_moves.tsv")

# ---- section 8 overrides (the user's decisions) -----------------------------------------------------------------
KEEP_WHOLE = {  # old cpp -> new cpp: the TSV split rows of these are ignored
    "src/game/party2/party_2.cpp": "src/game/party/party_map.cpp",
}
SPLIT_FILES = [  # split in batch B
    "src/game/agent4/agent_logic.cpp",
    "src/game/script/script_interpreter.cpp",
    "src/game/mission5/mission_5.cpp",
]
RETARGET = {  # old path -> new path, replaces the TSV's new path (agent_hud.cpp is mostly mission HUD: game/ui/hud/)
    "src/game/agent5/agent_hud.cpp": "src/game/ui/hud/agent_hud.cpp",
    "src/game/agent5/agent_hud.h": "src/game/ui/hud/agent_hud.h",
    "src/game/agent5/agent_hud.manifest.txt": "src/game/ui/hud/agent_hud.manifest.txt",
    "src/game/agent5/agent_hud.nonmatching.txt": "src/game/ui/hud/agent_hud.nonmatching.txt",
}
THIRD_PARTY_FROM, THIRD_PARTY_TO = "third_party/tinyxml/", "src/third_party/tinyxml/"

EXCLUDE_TEXT = {  # never rewritten
    "docs/source_tree.md", "docs/source_tree_moves.tsv", "tools/migrate/restructure.py",
    "2026-10-25-204916-ive-got-mount-and-blade-installed-in-crossover.txt",
}
SRC_EXT = (".cpp", ".h", ".inl", ".c", ".hpp")


def load_rows():
    with open(TSV, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def fix_new(p):
    return THIRD_PARTY_TO + p[len(THIRD_PARTY_FROM):] if p.startswith(THIRD_PARTY_FROM) else p


def plan():
    """-> (moves: old->new for every 1:1 file, splits: list of cpp-split rows, sidecar splits: list)"""
    rows = load_rows()
    moves, splits, side_splits = {}, [], []
    for r in rows:
        k, o, n = r["kind"], r["old_path"], r["new_path"]
        if k in ("cpp", "header", "sidecar", "notes"):
            moves[o] = fix_new(RETARGET.get(o, n))
        elif k == "cpp-split":
            if o in KEEP_WHOLE: continue
            splits.append(r)
        elif k == "sidecar-split":
            stem = o.rsplit(".", 2)[0] + ".cpp"
            if stem in KEEP_WHOLE: continue
            side_splits.append(r)
    for o, n in KEEP_WHOLE.items():     # the TSV has only split rows for these: the whole file and its sidecars
        for ext in (".cpp", ".manifest.txt", ".nonmatching.txt"):
            moves[o[:-4] + ext] = n[:-4] + ext
    # symbols/slices/<module stem>.txt (address -> name hints kept per module) follows a renamed module
    for o, n in list(moves.items()):
        if load_state().get("a_symbols"):
            break                      # done: a new name may equal an old one (resmgr.txt -> resource_manager.txt)
        if o.endswith(".cpp") and os.path.basename(o) != os.path.basename(n):
            so = "symbols/slices/%s.txt" % os.path.basename(o)[:-4]
            sn = "symbols/slices/%s.txt" % os.path.basename(n)[:-4]
            if os.path.exists(os.path.join(ROOT, so)):
                moves[so] = sn
    return moves, splits, side_splits


def sh(*a, check=True):
    return subprocess.run(a, cwd=ROOT, check=check, capture_output=True, text=True)


def tracked():
    return sh("git", "ls-files", "-z").stdout.split("\0")[:-1]


# ---- the old tree, as a virtual file system ----------------------------------------------------------------------
class Tree:
    def __init__(self, moves, pending_split_olds):
        self.moves = moves
        # every old src file (moved 1:1 or split in B)
        self.old_files = set(moves) | set(pending_split_olds)
        self.rev = {v: k for k, v in moves.items()}
        self.new_files = set(moves.values())
        # suffix index over old paths: suffix (>= 2 components) -> set of old paths
        self.suffix = {}
        for o in moves:
            parts = o.split("/")
            for i in range(0, len(parts) - 1):
                if i > 0 and not o.startswith("src/"):
                    break
                self.suffix.setdefault("/".join(parts[i:]), set()).add(o)

    def old_of(self, path):  # the path a file had in the old tree
        return self.rev.get(path, path)


def old_exists(T, path):
    if path.startswith("src/"):
        return path in T.old_files
    return os.path.exists(os.path.join(ROOT, path))


def resolve_old(T, inc, includer_old, angle):
    d = os.path.dirname(includer_old)
    cands = []
    if not angle:
        cands.append(os.path.normpath(os.path.join(d, inc)))
    dirs = ["src/include", "match/include"]
    if includer_old.startswith("port/"):
        dirs = ["port/include", "port/stl"] + dirs
    for dd in dirs:
        cands.append(os.path.normpath(os.path.join(dd, inc)))
    for i, c in enumerate(cands):
        if old_exists(T, c):
            return c, (i == 0 and not angle)
    return None, False


INC_RE = re.compile(r'^(\s*#\s*include\s*)([<"])([^>"\n]+)([>"])(.*)$')


def rewrite_include_line(T, line, includer_new, is_src):
    m = INC_RE.match(line)
    if not m:
        return None
    pre, lq, inc, rq, post = m.groups()
    post = rewrite_tokens(T, post)          # the trailing comment names files too
    includer_old = T.old_of(includer_new)
    res, dir_rel = resolve_old(T, inc, includer_old, lq == "<")
    if res is None or not res.startswith("src/") or res not in T.moves:
        return pre + lq + inc + rq + post
    if not is_src and dir_rel:
        return pre + lq + inc + rq + post     # a relative path to src: the token pass handles it, keeping the relative shape
    new = T.moves[res]
    return pre + lq + new[len("src/"):] + rq + post


# ---- the token pass ------------------------------------------------------------------------------------------------
TOKEN = re.compile(r"[\w.\-]+(?:/[\w.\-]+)+")


def rewrite_tokens(T, text):
    def rep(m):
        tok = m.group(0)
        end = len(tok) - len(tok.rstrip("."))
        core = tok[:len(tok) - end] if end else tok
        lead = ""
        c = core
        while c.startswith("../") or c.startswith("./"):
            k = 3 if c.startswith("../") else 2
            lead += c[:k]; c = c[k:]
        cand = T.suffix.get(c)
        if not cand or len(cand) != 1:
            return tok
        old = next(iter(cand))
        new = T.moves[old]
        if old == new:
            return tok
        full = c.startswith("src/") or not old.startswith("src/")
        tail = new if full else new[len("src/"):]
        return lead + tail + tok[len(core):]
    return TOKEN.sub(rep, text)


JOIN_RE = re.compile(r'(["\'])src\1((?:\s*,\s*(["\'])[^"\'\n/]+\3)+)')


def rewrite_joins(T, text):
    def rep(m):
        q = m.group(1)
        parts = re.findall(r'["\']([^"\'\n]+)["\']', m.group(2))
        old = "src/" + "/".join(parts)
        if old not in T.moves or T.moves[old] == old:
            return m.group(0)
        new = T.moves[old].split("/")[1:]
        return q + "src" + q + "".join(", %s%s%s" % (q, p, q) for p in new)
    return JOIN_RE.sub(rep, text)


# ---- the stem pass: module names (a module is a .cpp stem: manifests, equiv rows, link order) ------------------------
STATE = os.path.join(ROOT, "tools", "migrate", "restructure_state.json")


def load_state():
    try:
        return json.load(open(STATE))
    except (OSError, ValueError):
        return {}


class Stems:
    def __init__(self, moves):
        base = lambda p: os.path.basename(p)
        cnt = {}
        for o in moves:
            cnt[base(o)] = cnt.get(base(o), 0) + 1
        newb = {base(n) for n in moves.values()}
        # bare file names: unique old names that change; a name that is also somebody's new name is ambiguous after the move
        self.names = {base(o): base(n) for o, n in moves.items()
                      if cnt[base(o)] == 1 and base(o) != base(n) and base(o) not in newb}
        self.stems = {base(o)[:-4]: base(n)[:-4] for o, n in moves.items()
                      if o.endswith(".cpp") and base(o) != base(n)}
        alt = lambda ks: "|".join(sorted(map(re.escape, ks), key=len, reverse=True))
        self.r_name = re.compile(r"(?<![\w/.\-])(" + alt(self.names) + r")(?![\w\-/])")
        hexv = r"(?:0x[0-9a-fA-F]{5,}|[\"'][0-9a-fA-F]{6,8}[\"'])"
        self.r_row = re.compile(r"([\"'])(" + alt(self.stems) + r")\1(?=\s*[,:]\s*" + hexv + ")")
        self.r_key = re.compile(r"([\"'])(" + alt(self.stems) + r")( [0-9a-fA-F]{6,8}\b[^\"'\n]*)\1")
        self.r_exact = re.compile(r"([\"'])(" + alt(self.stems) + r")\1")

    def rewrite(self, text, rel):
        # bare names: never on an #include line (a quoted bare name there may be a local file that merely shares a name)
        text = "\n".join(ln if INC_RE.match(ln.rstrip("\r")) else self.r_name.sub(lambda m: self.names[m.group(1)], ln)
                         for ln in text.split("\n"))
        if rel.endswith((".py", ".cpp", ".h", ".json", ".txt", ".cmake", ".sh")) and not rel.startswith("src/"):
            text = self.r_row.sub(lambda m: m.group(1) + self.stems[m.group(2)] + m.group(1), text)
            text = self.r_key.sub(lambda m: m.group(1) + self.stems[m.group(2)] + m.group(3) + m.group(1), text)
        if rel in ("docs/equiv_verdicts.json", "symbols/link_inputs.json"):
            text = self.r_exact.sub(lambda m: m.group(1) + self.stems[m.group(2)] + m.group(1), text)
        return text


def is_text(b):
    return b"\0" not in b[:8192]


def rewrite_files(T, dry=False, stems=None):
    changed = 0
    for rel in tracked():
        if rel in EXCLUDE_TEXT or rel.startswith("work/"):
            continue
        p = os.path.join(ROOT, rel)
        if os.path.islink(p) or not os.path.isfile(p):
            continue
        b = open(p, "rb").read()
        if not is_text(b):
            continue
        t = b.decode("latin-1")
        if stems is not None:
            t = stems.rewrite(t, rel)       # first: bare names never touch the path tokens the next pass rewrites
        is_src = rel.startswith("src/")
        is_code = rel.endswith(SRC_EXT)
        lines = t.split("\n")
        out = []
        for ln in lines:
            if is_code and "include" in ln:
                r = rewrite_include_line(T, ln.rstrip("\r"), rel, is_src)
                if r is not None:
                    out.append(r + ("\r" if ln.endswith("\r") else ""))
                    continue
            out.append(ln)
        t2 = "\n".join(out)
        t2 = rewrite_tokens(T, t2) if not is_src else rewrite_tokens_skip_includes(T, t2)
        if rel.endswith(".py"):
            t2 = rewrite_joins(T, t2)
        if t2 != b.decode("latin-1"):
            changed += 1
            if not dry:
                open(p, "wb").write(t2.encode("latin-1"))
    return changed


def rewrite_tokens_skip_includes(T, text):
    out = []
    for ln in text.split("\n"):
        out.append(ln if INC_RE.match(ln.rstrip("\r")) else rewrite_tokens(T, ln))
    return "\n".join(out)


# ---- moves ---------------------------------------------------------------------------------------------------------
def do_moves(moves, dry=False):
    todo = [(o, n) for o, n in moves.items() if o != n and os.path.exists(os.path.join(ROOT, o)) and not os.path.exists(os.path.join(ROOT, n))]
    clash = [(o, n) for o, n in moves.items() if o != n and os.path.exists(os.path.join(ROOT, o)) and os.path.exists(os.path.join(ROOT, n))]
    if clash and not dry:
        # a target that is still occupied by a file that moves away itself: go through a staging name
        for o, n in clash:
            tmp = n + ".restructure_tmp"
            sh("git", "mv", o, tmp)
            todo.append((tmp, n))
    if dry:
        return len(todo) + len(clash)
    for o, n in todo:
        os.makedirs(os.path.dirname(os.path.join(ROOT, n)), exist_ok=True)
        sh("git", "mv", o, n)
    # port/tu/<path>.h per-TU preludes follow their .cpp
    for o, n in moves.items():
        if o.endswith(".cpp") and o != n:
            po = "port/tu/" + o[len("src/"):-4] + ".h"
            pn = "port/tu/" + n[len("src/"):-4] + ".h"
            if os.path.exists(os.path.join(ROOT, po)) and not os.path.exists(os.path.join(ROOT, pn)):
                os.makedirs(os.path.dirname(os.path.join(ROOT, pn)), exist_ok=True)
                sh("git", "mv", po, pn)
    return len(todo)


def prune_empty_dirs():
    for base in ("src", "port/tu"):
        for dp, dn, fn in os.walk(os.path.join(ROOT, base), topdown=False):
            if not os.listdir(dp) and dp != os.path.join(ROOT, base):
                os.rmdir(dp)


def batch_a(check=False):
    moves, splits, side_splits = plan()
    pending = [o for o in (SPLIT_FILES) if os.path.exists(os.path.join(ROOT, o))]
    pending += [o[:-4] + e for o in SPLIT_FILES for e in (".manifest.txt", ".nonmatching.txt", ".partial.txt")
                if os.path.exists(os.path.join(ROOT, o[:-4] + e))]
    # the files that wait for batch B stay put (their old path is "old" until B, but they are not in `moves`)
    T = Tree(moves, pending)
    n = do_moves(moves, dry=check)
    print("moves %s: %d" % ("pending" if check else "done", n))
    state = load_state()
    stems = Stems(moves) if not state.get("a_stems") else None
    c = rewrite_files(T, dry=check, stems=stems)
    print("files %s: %d" % ("to rewrite" if check else "rewritten", c))
    if not check:
        if not os.path.exists(ANNOT):
            write_annotation_headers(moves)
        prune_empty_dirs()
        state["a_symbols"] = True
        if stems is not None:
            state["a_stems"] = True
            json.dump(state, open(STATE, "w"), indent=1)


ANNOT = os.path.join(ROOT, "tools", "difftest", "annotation_headers.json")


def write_annotation_headers(moves):
    """The resolution tools read the `// 0x...` annotations of a TU's source, of every header in the TU's own directory and
    of the headers it includes by relative path. The directories are new, so each TU keeps the headers of ITS OLD directory:
    {new cpp: [new header paths]} (tools/difftest/annotation_headers.json; slice.annotation_sources reads it)."""
    out, olds = {}, sorted(moves)
    base = subprocess.run(["git", "ls-tree", "-r", "--name-only", "HEAD"], cwd=ROOT, capture_output=True, text=True).stdout.split("\n")
    for o in olds:
        if not o.endswith(".cpp"):
            continue
        d = os.path.dirname(o)
        hs = sorted(moves[f] for f in olds if f.endswith(".h") and os.path.dirname(f) == d)
        out[moves[o]] = hs
    for o in SPLIT_FILES:                         # the split files: their old directory's headers (parts are added in batch B)
        out[o] = sorted(moves[f] for f in olds if f.endswith(".h") and os.path.dirname(f) == os.path.dirname(o))
    json.dump(out, open(ANNOT, "w"), indent=0, sort_keys=True)


def verify():
    moves, _, _ = plan()
    T = Tree(moves, SPLIT_FILES)
    bad = 0
    for rel in tracked():
        if rel in EXCLUDE_TEXT or rel.startswith("work/"):
            continue
        p = os.path.join(ROOT, rel)
        if os.path.islink(p) or not os.path.isfile(p):
            continue
        b = open(p, "rb").read()
        if not is_text(b):
            continue
        for ln, line in enumerate(b.decode("latin-1").split("\n"), 1):
            for m in TOKEN.finditer(line):
                c = m.group(0).rstrip(".")
                while c.startswith("../") or c.startswith("./"):
                    c = c[3:] if c.startswith("../") else c[2:]
                cand = T.suffix.get(c)
                if cand and len(cand) == 1 and T.moves[next(iter(cand))] != next(iter(cand)):
                    print("%s:%d: %s" % (rel, ln, m.group(0))); bad += 1
    print("stale references: %d" % bad)
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["a", "b", "verify"])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--dump", metavar="DIR", help="b: write the generated parts to DIR and change nothing else")
    a = ap.parse_args()
    if a.cmd == "a":
        batch_a(a.check)
    elif a.cmd == "b":
        import restructure_split  # noqa: F401  (batch B lives in tools/migrate/restructure_split.py)
        restructure_split.run(sys.modules[__name__], a.check, a.dump)
    else:
        sys.exit(1 if verify() else 0)


if __name__ == "__main__":
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    main()
