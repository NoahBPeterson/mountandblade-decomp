#!/usr/bin/env python3
"""gen_data.py: print a C++ definition of an original static datum, read once from the image.

usage: gen_data.py SYMBOL|0xADDR [--type TYPE] [--name NAME] [--header H ...] [--const] [--extent]
       gen_data.py --items 0xLO 0xHI           list the data items (data_coverage.py's) in an address range

The generated text is committed, reviewed source: nothing is copied at link time. This tool reads the original
image (tools/difftest/resolve.py Image) and the resolution map (work/relink/resolve_map.json, from resolve_map.py),
and prints `TYPE NAME = {...};  // 0xADDR`, which goes into the module that owns the datum (docs/standalone_data.md).

TYPE grammar (default: int for a 4-byte item, char[] when the bytes up to the first NUL are printable):
    int unsigned short ushort char uchar float double       scalars (signed/unsigned as written)
    T*                                                      pointer: 0 -> nullptr, a symbol our source defines at the
                                                            target -> `&sym` / function name, a printable rdata string
                                                            -> a string literal (T = char), else a TODO marker
    T[N]  T[]                                               array (N omitted: up to the item's end; char[] = one string)
    struct NAME                                             a struct from --header files (fields `T name;`, `T name[N];`,
                                                            `T* name;`, nested structs; methods and comments skipped)
    raw[N]                                                  N dwords as hex (only where the original itself was a number table)
  Floats print as the shortest literal that round-trips through float32 ("0.5f", "0.00528f"); a value that is not
  exactly such a literal is flagged. A dword pointing into the image with no symbol our source defines for it prints
  `/* TODO: no symbol for 0x... */`: the output must never be committed with one (define the target, then regenerate).

--extent  also print, per item, what the original's code touches (data_coverage.py's item logic): the item's start and
          end, every address inside it that an instruction uses (so `g_x + 4` neighbour indexing is visible), and the
          names the resolution map gives to addresses inside it. If the item is larger than TYPE, the extra
          bytes belong to a neighbour or to the same object: make one struct whose members cover the accessed range.
Item data is cached in work/relink/data_items.json (rebuilt when resolve_map.json is newer).
"""
import argparse, json, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "matching"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

MAP = os.path.join(ROOT, "work", "relink", "resolve_map.json")
ITEMS = os.path.join(ROOT, "work", "relink", "data_items.json")

SCALARS = {"int": ("i", 4), "unsigned": ("I", 4), "unsigned int": ("I", 4), "short": ("h", 2), "ushort": ("H", 2),
           "unsigned short": ("H", 2), "char": ("b", 1), "uchar": ("B", 1), "unsigned char": ("B", 1),
           "float": ("f", 4), "double": ("d", 8), "bool": ("B", 1), "long": ("i", 4), "DWORD": ("I", 4)}


# ------------------------------------------------------------------ names

def plain_name(mangled):
    """Source spelling of a mangled name: ?g_x@@3HA -> g_x, ?f@C@@QAEXXZ -> C::f, ??_7C@@6B@ -> None."""
    if mangled.startswith("??_7") or mangled.startswith("??_C") or mangled.startswith("__real@"):
        return None
    if not mangled.startswith("?"):
        return mangled.lstrip("_@").split("@")[0] if mangled.startswith(("_", "@")) else mangled
    parts = mangled[1:].split("@@", 1)[0].split("@")
    return "::".join(reversed([p for p in parts if p]))


class Names:
    def __init__(self, path=MAP):
        self.rm = json.load(open(path))
        self.at = {}                          # va -> (mangled, module) of a definition in our source
        for va, (nm, mod) in self.rm["defined_at"].items():
            self.at[int(va, 16)] = (nm, mod)
        self.by_name = {}
        for nm, d in self.rm["names"].items():
            for va, e in d.items():
                self.by_name.setdefault(nm, []).append((int(va, 16), e["defined"]))

    def defined(self, va):
        return self.at.get(va)

    def find(self, text):
        """Address of a symbol given as 0xADDR, a mangled name, or a unique plain name / substring."""
        if re.fullmatch(r"(0x)?[0-9a-fA-F]{6,8}", text):
            return int(text, 16)
        if text in self.by_name:
            return self.by_name[text][0][0]
        hits = [(nm, v) for nm, v in self.by_name.items() if plain_name(nm) == text]
        if not hits:
            hits = [(nm, v) for nm, v in self.by_name.items() if text in nm]
        addrs = {a for _, v in hits for a, _ in v}
        if len(addrs) != 1:
            sys.exit("%s: %d candidate addresses: %s" % (text, len(addrs), sorted(hex(a) for a in addrs)[:8]))
        return addrs.pop()


# ------------------------------------------------------------------ type parsing

class Struct:
    def __init__(self, name):
        self.name, self.fields = name, []     # (type text, member name)


def parse_headers(paths):
    """{struct name: Struct} for the simple data structs (no methods needed) of the given headers."""
    out = {}
    for p in paths:
        txt = open(p, errors="replace").read()
        txt = re.sub(r"//[^\n]*", "", txt)
        txt = re.sub(r"/\*.*?\*/", "", txt, flags=re.S)
        for m in re.finditer(r"\b(?:struct|class)\s+(\w+)\s*(?::[^{;]*)?\{", txt):
            i, depth = m.end(), 1
            while depth and i < len(txt):
                depth += {"{": 1, "}": -1}.get(txt[i], 0)
                i += 1
            body = txt[m.end():i - 1]
            st = Struct(m[1])
            for stmt in body.split(";"):
                stmt = stmt.strip()
                if not stmt or "(" in stmt or "{" in stmt or stmt.startswith(("static", "typedef", "friend", "virtual", "public", "private")):
                    continue
                fm = re.fullmatch(r"((?:const\s+)?(?:struct\s+)?[\w:]+(?:\s+\w+)?(?:\s*\*+)?)\s+(\w+(?:\[[^\]]*\])*(?:\s*,\s*\*?\s*\w+(?:\[[^\]]*\])*)*)", stmt)
                if not fm:
                    st.fields = None
                    break
                for decl in fm[2].split(","):
                    dm = re.fullmatch(r"\s*(\*?)\s*(\w+)((?:\[[^\]]*\])*)\s*", decl)
                    st.fields.append((fm[1].strip() + dm[1] + dm[3].replace(" ", ""), dm[2]))
            if st.fields:
                out[st.name] = st
    return out


def split_type(t):
    """('float', [2, 3]) for float[2][3]; the dims are ints or None for []."""
    dims = []
    while True:
        m = re.fullmatch(r"(.*?)\s*\[([^\]]*)\]", t)
        if not m:
            break
        t = m[1]
        dims.insert(0, int(m[2], 0) if m[2].strip() else None)
    return t.strip(), dims


class Reader:
    """Image reads that zero-fill past the file data (.bss has no raw bytes)."""
    def __init__(self, img):
        self.img = img

    def read(self, va, n):
        b = self.img.read(va, n)
        return b + b"\0" * (n - len(b))

    def u32(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]

    def in_image(self, va):
        return self.img.in_image(va)

    def in_text(self, va):
        return self.img.in_text(va)


class Gen:
    def __init__(self, img, names, structs):
        self.img, self.names, self.structs = Reader(img), names, structs
        self.todo = 0

    # --- sizes
    def sizeof(self, t):
        base, dims = split_type(t)
        n = 1
        for d in dims:
            n *= d or 0
        return n * self.sizeof1(base)

    def sizeof1(self, b):
        b = b.replace("const ", "").strip()
        if b.endswith("*"):
            return 4
        if b in SCALARS:
            return SCALARS[b][1]
        b = b.replace("struct ", "")
        if b in self.structs:
            return self.layout(self.structs[b])[1]
        sys.exit("unknown type %r (use --header for structs)" % b)

    def layout(self, st):
        off, offs, al = 0, [], 1
        for ty, nm in st.fields:
            a = self.align(ty)
            al = max(al, a)
            off = (off + a - 1) // a * a
            offs.append(off)
            off += self.sizeof(ty)
        return offs, (off + al - 1) // al * al

    def align(self, t):
        base, _ = split_type(t)
        base = base.replace("const ", "").replace("struct ", "").strip()
        if base.endswith("*"):
            return 4
        if base in SCALARS:
            return min(SCALARS[base][1], 4) if base != "double" else 8
        return max(self.align(ty) for ty, _ in self.structs[base].fields)

    # --- values
    def float_lit(self, raw, dbl=False):
        if dbl:
            v = struct.unpack("<d", raw)[0]
            if v != v or v in (float("inf"), float("-inf")):
                self.todo += 1
                return "/* TODO: non-finite double 0x%s */ 0.0" % raw[::-1].hex()
            s = repr(v)
            return s if ("." in s or "e" in s) else s + ".0"
        v = struct.unpack("<f", raw)[0]
        if v != v or v in (float("inf"), float("-inf")):
            self.todo += 1
            return "/* TODO: non-finite float 0x%s */ 0.0f" % raw[::-1].hex()
        if v == 0.0 and raw[3] & 0x80:
            return "-0.0f"
        for n in range(1, 10):
            s = "%.*g" % (n, v)
            if struct.pack("<f", float(s)) == raw:
                break
        if "e" not in s and "." not in s:
            s += ".0"
        return s + "f"

    def cstring(self, va):
        """Printable NUL-terminated string at va, or None."""
        if not self.img.in_image(va):
            return None
        try:
            b = self.img.read(va, 256)
        except Exception:
            return None
        n = b.find(b"\0")
        if n < 0:
            return None
        return b[:n].decode("latin-1")

    def quote(self, s):
        out = []
        for ch in s:
            o = ord(ch)
            if ch == '"' or ch == "\\":
                out.append("\\" + ch)
            elif ch == "\n":
                out.append("\\n")
            elif ch == "\t":
                out.append("\\t")
            elif ch == "\r":
                out.append("\\r")
            elif 32 <= o < 127:
                out.append(ch)
            else:
                out.append("\\%03o" % o)
        return '"%s"' % "".join(out)

    def pointer(self, va, pointee):
        if va == 0:
            return "nullptr"
        d = self.names.defined(va)
        if d:
            nm = plain_name(d[0])
            if nm:
                is_func = self.img.in_text(va)
                arr = "[" in d[0] or d[0].endswith("PADA")
                return nm if (is_func and "::" not in nm) else "&" + nm
            return "/* vtable or compiler data %s */ nullptr" % d[0]
        if pointee in ("char", "const char") and self.img.in_image(va):
            s = self.cstring(va)
            if s is not None and all(32 <= ord(c) < 127 or c in "\n\t\r" for c in s):
                return self.quote(s)
        if self.img.in_image(va):
            self.todo += 1
            return "/* TODO: no symbol for 0x%08x */ nullptr" % va
        return "0x%x" % va                       # not an image address: a plain integer

    def value(self, t, va):
        """C++ initialiser text for an object of type t at va."""
        base, dims = split_type(t)
        base = base.replace("const ", "").replace("struct ", "").strip() if not base.endswith("*") else base.replace("const ", "")
        if dims:
            d0 = dims[0]
            sub = base + "".join("[%d]" % d for d in dims[1:])
            esz = self.sizeof(sub)
            if base == "char" and len(dims) == 1:
                raw = self.img.read(va, d0 if d0 else 256)
                s = raw[:raw.find(b"\0")].decode("latin-1") if b"\0" in raw else raw.decode("latin-1")
                return self.quote(s)
            items = [self.value(sub, va + i * esz) for i in range(d0)]
            return "{" + ", ".join(items) + "}"
        if base.endswith("*"):
            return self.pointer(self.img.u32(va), base[:-1].strip())
        if base in SCALARS:
            code, sz = SCALARS[base]
            raw = self.img.read(va, sz)
            if code == "f":
                return self.float_lit(raw)
            if code == "d":
                return self.float_lit(raw, True)
            v = struct.unpack("<" + code, raw)[0]
            if base in ("unsigned", "unsigned int", "DWORD", "ushort", "unsigned short", "uchar", "unsigned char") and v > 9:
                return "0x%x" % v
            return str(v)
        st = self.structs[base]
        offs, size = self.layout(st)
        parts = []
        for (ty, nm), o in zip(st.fields, offs):
            parts.append("/*%s*/ %s" % (nm, self.value(ty, va + o)))
        return "{" + ", ".join(parts) + "}"


def guess_type(img, names, va, extent):
    n = extent if extent else 4
    s = None
    try:
        b = img.read(va, min(n, 256))
        z = b.find(b"\0")
        if z > 1 and all(32 <= c < 127 or c in (9, 10, 13) for c in b[:z]):
            return "char[]"
    except Exception:
        pass
    return "int" if n == 4 else "raw[%d]" % max(1, n // 4)


# ------------------------------------------------------------------ items

def load_items(img, names):
    if os.path.exists(ITEMS) and os.path.getmtime(ITEMS) > os.path.getmtime(MAP):
        return json.load(open(ITEMS))
    import data_coverage as dc
    starts, game = dc.game_functions()
    items, uses, named = dc.data_items(img, names.rm, starts, game)
    out = {"items": [[s, e, c, sorted(a for a in uses if s <= a < e)] for s, e, c in items],
           "names": {str(a): v[1] for a, v in named.items()}}
    json.dump(out, open(ITEMS, "w"))
    return out


def item_at(data, va):
    best = None
    for s, e, c, acc in data["items"]:
        if s <= va < e:
            best = (s, e, c, acc)
    return best


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("sym", nargs="?")
    ap.add_argument("--type")
    ap.add_argument("--name")
    ap.add_argument("--header", action="append", default=[])
    ap.add_argument("--const", action="store_true")
    ap.add_argument("--extent", action="store_true")
    ap.add_argument("--items", nargs=2, metavar=("LO", "HI"))
    a = ap.parse_args()
    import resolve as R
    img = R.Image()
    names = Names()
    if a.items:
        lo, hi = int(a.items[0], 16), int(a.items[1], 16)
        data = load_items(img, names)
        for s, e, c, acc in data["items"]:
            if lo <= s < hi:
                nm = names.defined(s)
                print("0x%08x +0x%-4x %-22s %s uses:%s" % (s, e - s, c, plain_name(nm[0]) if nm else "-",
                                                         " ".join("+%x" % (x - s) for x in acc)))
        return 0
    if not a.sym:
        ap.error("SYMBOL or --items")
    va = names.find(a.sym)
    data = load_items(img, names)
    it = item_at(data, va)
    extent = (it[1] - va) if it else None
    ty = a.type or guess_type(img, names, va, extent)
    g = Gen(img, names, parse_headers(a.header))
    if ty.startswith("raw["):
        n = int(ty[4:-1], 0)
        body = "{" + ", ".join("0x%x" % img.u32(va + 4 * i) for i in range(n)) + "}"
        decl_t, dims = "unsigned", [n]
    else:
        body = g.value(ty, va)
        decl_t, dims = split_type(ty)
    nm = a.name
    if not nm:
        d = names.defined(va)
        nm = (plain_name(d[0]) if d else None) or "g_%x" % va
    ds = "".join("[%d]" % d if d else "[]" for d in dims)
    if decl_t == "char" and dims == [None]:
        ds = "[]"
    print("%s%s %s%s = %s;  // 0x%x" % ("const " if a.const else "", decl_t.replace("struct ", ""), nm, ds, body, va))
    if a.extent:
        if it:
            print("// item 0x%x..0x%x (%d bytes, %s); instruction uses at %s" % (
                it[0], it[1], it[1] - it[0], it[2], " ".join("+0x%x" % (x - it[0]) for x in it[3]) or "none"))
            ins = [(x, nm_[0]) for x, nm_ in names.at.items() if it[0] <= x < it[1] and x != va]
            for x, n_ in sorted(ins):
                print("//   also defined inside: 0x%x %s" % (x, plain_name(n_)))
        else:
            print("// no item covers 0x%x" % va)
    if g.todo:
        print("// %d TODO marker(s): resolve before committing" % g.todo, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
