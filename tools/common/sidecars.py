"""tools/common/sidecars.py: where the per-source sidecar files live. The ONE place that knows.

Every decompiled source src/<path>/<stem>.cpp has up to three sidecars, kept in a mirror tree outside src/:

    match/manifests/<path>/<stem>.manifest.txt      rows that are byte-exact (run_all.py checks them)
    match/manifests/<path>/<stem>.nonmatching.txt   rows still open, with the chk.py aligned figure and the notes
    match/manifests/<path>/<stem>.partial.txt       rows that are not decompiled yet (assembly transcriptions)

<path>/<stem> is the source's path under src/ without the extension: src/game/agent/ai/agent_ai.cpp ->
match/manifests/game/agent/ai/agent_ai.manifest.txt. Sources outside src/ (patches/) keep a sidecar next to them. A row's first field ("../src/game/agent/ai/agent_ai.cpp") is relative
to match/ and does not depend on where the sidecar sits.

Functions (all paths absolute; a source may be given absolute or relative to the repository root):

    manifest_for(source) / nonmatching_for(source) / partial_for(source)   the sidecar path (it may not exist yet)
    sidecar_for(source, kind)                                              the same for kind in KINDS
    all_manifests() / all_nonmatching() / all_partials() / all_sidecars(kind)   sorted existing files (root=: another checkout)
    source_for(sidecar)                                                    the src/ .cpp path the sidecar belongs to
    source_rel_for(sidecar)                                                the same, relative to the repository root
    kind_of(path)                                                          'manifest' / 'nonmatching' / 'partial' / None
    stem_of(sidecar)                                                       the file stem (agent_ai)
"""
import glob, os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "src")
DIR = os.path.join(ROOT, "match", "manifests")
KINDS = ("manifest", "nonmatching", "partial")


def _abs(p):
    return p if os.path.isabs(p) else os.path.join(ROOT, p)


def sidecar_for(source, kind):
    if kind not in KINDS:
        raise ValueError("sidecar kind %r not in %r" % (kind, KINDS))
    rel = os.path.relpath(os.path.normpath(_abs(source)), SRC)
    if rel.startswith(".."):
        # a source outside src/ (patches/<set>/<name>.cpp: docs/patches.md) keeps its optional sidecar next to it
        return os.path.splitext(_abs(source))[0] + "." + kind + ".txt"
    base, _ext = os.path.splitext(rel)
    return os.path.join(DIR, base + "." + kind + ".txt")


def manifest_for(source):
    return sidecar_for(source, "manifest")


def nonmatching_for(source):
    return sidecar_for(source, "nonmatching")


def partial_for(source):
    return sidecar_for(source, "partial")


def all_sidecars(kind, root=None):
    """Sorted existing sidecars of one kind. `root`: another checkout of the repository (tools that scan old commits'
    worktrees); one from before match/manifests existed keeps its sidecars next to the sources, in src/."""
    if kind not in KINDS:
        raise ValueError("sidecar kind %r not in %r" % (kind, KINDS))
    base = DIR if root is None else os.path.join(root, "match", "manifests")
    if root is not None and not os.path.isdir(base):
        base = os.path.join(root, "src")        # legacy layout (before the 2026-11 batch C)
    return sorted(glob.glob(os.path.join(base, "**", "*." + kind + ".txt"), recursive=True))


def all_manifests(root=None):
    return all_sidecars("manifest", root)


def all_nonmatching(root=None):
    return all_sidecars("nonmatching", root)


def all_partials(root=None):
    return all_sidecars("partial", root)


def kind_of(path):
    b = os.path.basename(path)
    for k in KINDS:
        if b.endswith("." + k + ".txt"):
            return k
    return None


def stem_of(sidecar):
    b = os.path.basename(sidecar)
    return b[:-len("." + kind_of(sidecar) + ".txt")]


def source_for(sidecar):
    """The .cpp the sidecar belongs to (the file need not exist)."""
    k = kind_of(sidecar)
    if k is None:
        raise ValueError("%s is not a sidecar" % sidecar)
    rel = os.path.relpath(os.path.normpath(_abs(sidecar)), DIR)
    return os.path.join(SRC, rel[:-len("." + k + ".txt")] + ".cpp")


def source_rel_for(sidecar):
    return os.path.relpath(source_for(sidecar), ROOT)
