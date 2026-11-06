#!/usr/bin/env python3
"""rsrc_repo.py: the exe's resources as files in the repo, and the assembler that turns them back into a .rsrc section.

  rsrc_repo.py extract EXE [--out resources]   write every resource except RT_ICON / RT_GROUP_ICON as
                                               resources/<type>/<id>_<lang>.bin, plus resources/manifest.json
                                               (the icon groups are recorded as structure only: ids, sizes, depths),
                                               and generate the placeholder icons
  rsrc_repo.py build OUT.bin RVA [--res resources]
                                               assemble a .rsrc section for a section placed at RVA

The section is an IMAGE_RESOURCE_DIRECTORY tree (type / id / language) with each level sorted as the format requires (named
entries first, case-insensitively, then ids ascending), the names and data entries after the directories, then the data.
Every data entry's OffsetToData is an RVA, computed from RVA. RT_VERSION is regenerated: the VS_VERSIONINFO is parsed,
the StringFileInfo strings named in the manifest's "version_strings" are replaced, and the structure is written out again
with new lengths and padding; the fixed binary version fields (VS_FIXEDFILEINFO) are numbers and are left as they are.
RT_ICON and RT_GROUP_ICON come from the manifest's icon groups and their .ico files (placeholders, see
make_placeholder_icons.py), keeping the manifest's ids.
"""
import json, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RES = os.path.join(ROOT, "resources")
RT = {1: "RT_CURSOR", 2: "RT_BITMAP", 3: "RT_ICON", 4: "RT_MENU", 5: "RT_DIALOG", 6: "RT_STRING", 7: "RT_FONTDIR",
      8: "RT_FONT", 9: "RT_ACCELERATOR", 10: "RT_RCDATA", 11: "RT_MESSAGETABLE", 12: "RT_GROUP_CURSOR", 14: "RT_GROUP_ICON",
      16: "RT_VERSION", 17: "RT_DLGINCLUDE", 19: "RT_PLUGPLAY", 20: "RT_VT_RC", 21: "RT_ANICURSOR", 22: "RT_ANIICON",
      23: "RT_HTML", 24: "RT_MANIFEST"}
RT_ID = {v: k for k, v in RT.items()}
ICON, GROUP_ICON, VERSION = 3, 14, 16


def up(v, al):
    return (v + al - 1) // al * al


# ---- VS_VERSIONINFO ----------------------------------------------------------------------------------------------

class VNode:
    def __init__(self, key, wtype, value, children):
        self.key, self.wtype, self.value, self.children = key, wtype, value, children   # value: bytes (text: UTF-16LE incl. NUL)


def vparse(b, off, end_limit):
    length, vlen, wtype = struct.unpack_from("<HHH", b, off)
    end = off + length
    if length < 6 or end > end_limit:
        raise ValueError("bad VS_VERSIONINFO node at %#x" % off)
    k = off + 6
    while b[k:k + 2] != b"\0\0":
        k += 2
    key = b[off + 6:k].decode("utf-16-le")
    p = up(k + 2, 4)
    vbytes = vlen * 2 if wtype == 1 else vlen
    value = bytes(b[p:p + vbytes])
    p += vbytes
    kids = []
    p = up(p, 4)
    while p < end:
        c = vparse(b, p, end)
        kids.append(c)
        p = up(p + struct.unpack_from("<H", b, p)[0], 4)
    return VNode(key, wtype, value, kids)


def vser(n, pos):
    """Serialise node n placed at absolute offset pos (alignment is relative to the start of the resource)."""
    out = bytearray(6)
    out += n.key.encode("utf-16-le") + b"\0\0"
    out += b"\0" * (up(pos + len(out), 4) - pos - len(out))
    out += n.value
    if n.children:
        out += b"\0" * (up(pos + len(out), 4) - pos - len(out))
        for c in n.children:
            out += b"\0" * (up(pos + len(out), 4) - pos - len(out))
            out += vser(c, pos + len(out))
    vlen = len(n.value) // 2 if n.wtype == 1 else len(n.value)
    struct.pack_into("<HHH", out, 0, len(out), vlen, n.wtype)
    return bytes(out)


def patch_version(blob, strings):
    root = vparse(blob, 0, len(blob))
    if vser(root, 0) != bytes(blob):
        raise ValueError("RT_VERSION does not round-trip through the parser")
    seen = set()
    for sfi in root.children:
        if sfi.key != "StringFileInfo":
            continue
        for table in sfi.children:
            for s in table.children:
                if s.key in strings:
                    s.value = (strings[s.key] + "\0").encode("utf-16-le")
                    seen.add(s.key)
    if seen != set(strings):
        raise ValueError("version strings not found: %s" % sorted(set(strings) - seen))
    return vser(root, 0)


# ---- the resource tree -------------------------------------------------------------------------------------------

def tkey(v):
    """Sort key of a directory entry: named entries (str) first, case-insensitive; then ids ascending."""
    return (0, v.upper()) if isinstance(v, str) else (1, v)


def assemble(entries, rva):
    """entries: [(type, id, lang, codepage, data)], type/id int or str -> .rsrc section bytes for a section at rva."""
    tree = {}
    for t, i, l, cp, d in entries:
        if l in tree.setdefault(t, {}).setdefault(i, {}):
            raise ValueError("duplicate resource %r/%r/%r" % (t, i, l))
        tree[t][i][l] = (cp, d)
    # sizes: every directory is 16 + 8 * entries bytes
    dirs = []                      # (offset, [(key, ('dir', child_index) | ('data', index))])
    leaves = []                    # (cp, data)
    names = []                     # strings to place after the directories

    def make(level):
        idx = len(dirs)
        dirs.append(None)
        ents = []
        for k in sorted(level, key=tkey):
            v = level[k]
            if isinstance(v, tuple):
                leaves.append(v)
                ents.append((k, ("data", len(leaves) - 1)))
            else:
                ents.append((k, ("dir", make(v))))
        dirs[idx] = ents
        return idx
    make(tree)
    offs, o = [], 0
    for ents in dirs:
        offs.append(o)
        o += 16 + 8 * len(ents)
    name_off = {}
    for ents in dirs:
        for k, _ in ents:
            if isinstance(k, str) and k not in name_off:
                name_off[k] = o
                o += 2 + 2 * len(k)
    o = up(o, 4)
    ent_off = [o + 16 * i for i in range(len(leaves))]
    o += 16 * len(leaves)
    data_off = []
    for cp, d in leaves:
        o = up(o, 4)
        data_off.append(o)
        o += len(d)
    out = bytearray(up(o, 4))
    for di, ents in enumerate(dirs):
        p = offs[di]
        named = sum(1 for k, _ in ents if isinstance(k, str))
        struct.pack_into("<IIHHHH", out, p, 0, 0, 0, 0, named, len(ents) - named)
        for j, (k, (kind, ref)) in enumerate(ents):
            kv = (0x80000000 | name_off[k]) if isinstance(k, str) else k
            tv = (0x80000000 | offs[ref]) if kind == "dir" else ent_off[ref]
            struct.pack_into("<II", out, p + 16 + 8 * j, kv, tv)
    for s, p in name_off.items():
        struct.pack_into("<H", out, p, len(s))
        out[p + 2:p + 2 + 2 * len(s)] = s.encode("utf-16-le")
    for i, (cp, d) in enumerate(leaves):
        struct.pack_into("<IIII", out, ent_off[i], rva + data_off[i], len(d), cp, 0)
        out[data_off[i]:data_off[i] + len(d)] = d
    return bytes(out)


def read_ico(path):
    """-> {(w, h, bpp): (colour byte, reserved, planes, dib bytes)} for the images stored as DIBs."""
    b = open(path, "rb").read()
    _r, typ, n = struct.unpack_from("<HHH", b)
    if typ != 1:
        raise ValueError("%s is not an .ico" % path)
    out = {}
    for k in range(n):
        w, h, col, res, planes, bpp, size, off = struct.unpack_from("<BBBBHHII", b, 6 + 16 * k)
        data = b[off:off + size]
        if data[:4] == b"\x89PNG":
            continue
        out[(w or 256, h or 256, bpp)] = (col, res, planes, data)
    return out


def load(res=RES):
    """The manifest's resources as assemble() entries, version info patched, icons from the .ico files."""
    if not os.path.exists(os.path.join(res, "manifest.json")):
        sys.exit("resources/ is empty: run tools/relink/rsrc_repo.py extract \"<your install>/mount&blade.exe\" once (they're not in git)")
    m = json.load(open(os.path.join(res, "manifest.json")))
    entries = []
    for r in m["resources"]:
        t = RT_ID.get(r["type"], r["type"])
        d = open(os.path.join(res, r["file"]), "rb").read()
        if t == VERSION:
            d = patch_version(d, m["version_strings"])
        entries.append((t, r["id"], r["lang"], r["codepage"], d))
    for g in m["icon_groups"]:
        ico = read_ico(os.path.join(res, g["ico"]))
        grp = struct.pack("<HHH", 0, 1, len(g["images"]))
        for im in g["images"]:
            col, rsv, planes, data = ico[(im["w"], im["h"], im["bpp"])]
            entries.append((ICON, im["id"], g["lang"], 0, data))
            grp += struct.pack("<BBBBHHIH", im["w"] % 256, im["h"] % 256, col, rsv, planes, im["bpp"], len(data), im["id"])
        entries.append((GROUP_ICON, g["id"], g["lang"], 0, grp))
    return entries


def build_section(rva, res=RES):
    return assemble(load(res), rva)


# ---- extract -----------------------------------------------------------------------------------------------------

ICON_FILES = {206: ("icons/mb.ico", "MB"), 227: ("icons/rc.ico", "RC")}     # placeholders, by group id


def extract(exe, res=RES):
    import pefile
    pe = pefile.PE(exe)
    man = {"_note": "Resources of Mount & Blade 1.011's exe as files; rsrc_repo.py build assembles them. RT_ICON / RT_GROUP_ICON are "
                    "not stored (icon groups here are structure only; the .ico files are placeholders). RT_VERSION is patched "
                    "with version_strings at build time.",
           "version_strings": {"FileVersion": "1.0.11_Recomp", "ProductVersion": "1.0.11_Recomp"},
           "resources": [], "icon_groups": []}
    icons = {}
    groups = []
    for te in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        t = str(te.name) if te.name else te.id
        for ie in te.directory.entries:
            i = str(ie.name) if ie.name else ie.id
            for le in ie.directory.entries:
                d = le.data.struct
                data = pe.get_data(d.OffsetToData, d.Size)
                if t == ICON:
                    icons[i] = data
                elif t == GROUP_ICON:
                    groups.append((i, le.id, data))
                else:
                    tn = RT.get(t, t) if isinstance(t, int) else t
                    fn = "%s/%s_%d.bin" % (tn, i, le.id)
                    os.makedirs(os.path.join(res, tn), exist_ok=True)
                    open(os.path.join(res, fn), "wb").write(data)
                    man["resources"].append({"type": tn, "id": i, "lang": le.id, "codepage": d.CodePage, "file": fn})
    for gid, lang, data in groups:
        _r, _t, n = struct.unpack_from("<HHH", data)
        imgs = []
        for k in range(n):
            w, h, _c, _rs, _pl, bpp, size, iid = struct.unpack_from("<BBBBHHIH", data, 6 + 14 * k)
            assert len(icons[iid]) == size
            imgs.append({"id": iid, "w": w or 256, "h": h or 256, "bpp": bpp})
        f, label = ICON_FILES[gid]
        man["icon_groups"].append({"id": gid, "lang": lang, "ico": f, "label": label, "images": imgs})
    json.dump(man, open(os.path.join(res, "manifest.json"), "w"), indent=1)
    print("wrote %d resources + %d icon groups to %s" % (len(man["resources"]), len(groups), os.path.relpath(res, ROOT)))
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import make_placeholder_icons
    sys.argv = ["make_placeholder_icons", os.path.join(res, "manifest.json")]
    make_placeholder_icons.main()


def main():
    a = sys.argv[1:]
    res = RES
    if "--out" in a or "--res" in a:
        k = a.index("--out" if "--out" in a else "--res")
        res = a[k + 1]
        del a[k:k + 2]
    if len(a) == 2 and a[0] == "extract":
        extract(a[1], res)
    elif len(a) == 3 and a[0] == "build":
        data = build_section(int(a[2], 0), res)
        open(a[1], "wb").write(data)
        print("%s: %d bytes for RVA %#x" % (a[1], len(data), int(a[2], 0)))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
