"""cxxchunk.py: cut a C++ source file into its top-level chunks (just enough C++ for the decompiled sources).

A chunk is a run of whole lines: the comments and blank lines in front of a top-level construct, then the construct (a
preprocessor directive with its continuation lines, a declaration ending in ';', a function / namespace body ending in
its closing brace, a struct / enum / class with its trailing ';', or an initialised definition up to its ';').
Used by tools/migrate/restructure_split.py (batch B) to decide which helper definitions of a big file every part needs.
"""
import re

IDENT = re.compile(r"[A-Za-z_]\w*")


class Chunk:
    __slots__ = ("first", "last", "text", "code", "kind", "provides")

    def __init__(self, first, last, text, code):
        self.first, self.last, self.text, self.code = first, last, text, code   # 1-based inclusive line numbers
        self.kind, self.provides = "other", set()


def strip_code(text):
    """text with comments, string and char literals blanked (same length, newlines kept)."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i)); i = j
        elif c == "/" and text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join(ch if ch == "\n" else " " for ch in text[i:j])); i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(c + " " * max(0, j - i - 1) + (c if j < n and text[j] == c else "")); i = j + 1
        else:
            out.append(c); i += 1
    return "".join(out)


def chunks(text):
    """-> list of Chunk covering every line of `text` (the trailing comment-only run is its own 'tail' chunk)."""
    code = strip_code(text)
    lines = text.split("\n")
    clines = code.split("\n")
    res = []
    depth_b = depth_p = 0
    start = 1                      # first line of the chunk being built
    i = 0                          # line index
    in_construct = False           # something other than comments/blank seen in the current chunk
    head_end = None                # code seen before the first top-level '{' of the chunk
    buf = []                       # code text of the chunk so far
    def flush(last):
        nonlocal start, in_construct, head_end, buf
        c = Chunk(start, last, "\n".join(lines[start - 1:last]), "\n".join(clines[start - 1:last]))
        res.append(c)
        start = last + 1; in_construct = False; head_end = None; buf = []
    nl = len(lines)
    while i < nl:
        cl = clines[i]
        st = cl.strip()
        if depth_b == 0 and depth_p == 0 and not in_construct and st.startswith("#"):
            j = i
            while lines[j].rstrip().endswith("\\") and j + 1 < nl:
                j += 1
            flush(j + 1); i = j + 1
            continue
        if st:
            in_construct = True
        k = 0
        end_here = False
        while k < len(cl):
            ch = cl[k]
            if ch == "(":
                depth_p += 1
            elif ch == ")":
                depth_p -= 1
            elif ch == "{":
                if depth_b == 0 and depth_p == 0 and head_end is None:
                    head_end = len("\n".join(buf)) + len(cl[:k]) + (1 if buf else 0)
                depth_b += 1
            elif ch == "}":
                depth_b -= 1
                if depth_b == 0 and depth_p == 0:
                    head = ("\n".join(buf) + ("\n" if buf else "") + cl[:k])[: head_end if head_end is not None else None]
                    whole_head = head
                    # a function / namespace body ends at its '}'; struct / enum / initialiser wait for ';'
                    if re.search(r"\)\s*(const\s*)?(throw\s*\(\s*\)\s*)?(:[^{]*)?$", whole_head.strip()) and "=" not in whole_head.replace("==", ""):
                        end_here = "fn"
                    elif re.match(r"\s*(template\s*<[^>]*>\s*)?(extern\s*\"[^\"]*\"\s*|namespace\b)", whole_head) or re.match(r"\s*namespace\b", whole_head):
                        end_here = "ns"
                    else:
                        end_here = "agg"
                    rest = cl[k + 1:]
                    if end_here != "fn" or True:
                        # absorb a following ';' on the same line
                        if rest.strip().startswith(";") and end_here != "ns":
                            pass
                    if end_here in ("fn", "ns") and not rest.strip().startswith(";"):
                        flush(i + 1); end_here = True
                        k = len(cl); break
                    end_here = False
            elif ch == ";" and depth_b == 0 and depth_p == 0:
                flush(i + 1)
                k = len(cl); break
            k += 1
        else:
            pass
        if not (len(res) and res[-1].last == i + 1):
            buf.append(cl)
        else:
            buf = []
        i += 1
    if start <= nl and "\n".join(lines[start - 1:]).strip():
        c = Chunk(start, nl, "\n".join(lines[start - 1:]), "\n".join(clines[start - 1:]))
        c.kind = "tail"
        res.append(c)
    for c in res:
        classify(c)
    return res


_KW_SKIP = {"static", "inline", "__forceinline", "__declspec", "noinline", "const", "extern", "unsigned", "signed", "struct",
            "class", "union", "enum", "typedef", "template", "operator", "return", "typename", "volatile", "virtual", "void",
            "int", "char", "short", "long", "float", "double", "bool", "namespace", "using", "nothrow", "NOINLINE"}


def classify(c):
    code = c.code
    s = code.strip()
    if not s:
        c.kind = "blank"; return
    if s.startswith("#"):
        m = re.match(r"#\s*(\w+)\s*(\w+)?", s)
        d = m.group(1) if m else ""
        c.kind = "pp:" + d
        if d == "define" and m.group(2):
            c.provides.add(m.group(2))
        return
    body = s
    # strip a leading template<...> header
    t = re.sub(r"^template\s*<[^{};]*?>\s*(?=[A-Za-z_])", "", body, count=1) if body.startswith("template") and not re.match(r"template\s*<>", body) and not re.match(r"template\s+[A-Za-z_]", body) else body
    if re.match(r"template\s+[A-Za-z_:<]", body) or re.match(r"template\s*<>", body):
        c.kind = "inst"; return                  # explicit instantiation / specialisation: emits code here
    head = re.split(r"[{;=]", t, 1)[0]
    words = re.findall(r"[A-Za-z_]\w*", head)
    first = words[0] if words else ""
    # leading specifiers
    spec = set(re.findall(r"\b(static|inline|__forceinline|extern|typedef|using|namespace|struct|class|union|enum)\b", head.split("(")[0]))
    is_fn = "(" in head and not re.match(r"\s*(typedef|struct|class|union|enum|namespace|using)\b", head)
    if first == "typedef":
        c.kind = "type"
        m = re.findall(r"[A-Za-z_]\w*", re.split(r"[;]", t)[0].replace("}", " ").split("{")[-1] if "{" in t else head)
        if m: c.provides.add(m[-1])
        if "{" in t:
            c.provides.update(enum_names(t))
        return
    if first == "using":
        c.kind = "type"; c.provides.update(re.findall(r"[A-Za-z_]\w*", head)[-1:]); return
    if first == "namespace":
        c.kind = "ns"; return
    if first in ("struct", "class", "union", "enum") and not is_fn and not re.search(r"\}\s*[A-Za-z_]\w*\s*(\[[^\]]*\]\s*)*(=.*)?;?\s*$", t.rstrip()):
        c.kind = "type"
        m = re.match(r"(?:struct|class|union|enum)\s+(?:__declspec\([^)]*\)\s*)?([A-Za-z_]\w*)", head)
        if m: c.provides.add(m.group(1))
        if first == "enum": c.provides.update(enum_names(t))
        return
    if "static" in spec or "inline" in spec or "__forceinline" in spec or "__declspec" in head and "noinline" in head and "inline" in head:
        if is_fn:
            c.kind = "helper_fn"
            h2 = re.sub(r"__declspec\s*\([^)]*\)|__forceinline|__cdecl|__stdcall|__fastcall|__thiscall", " ", head)
            h2 = re.sub(r"\boperator\s*([^\s(]+|\(\))", "operator", h2)
            nm = re.findall(r"([A-Za-z_~]\w*)\s*\(", h2[: h2.find("(") + 1])
            nm = [x for x in nm if x not in _KW_SKIP]
            if nm:
                c.provides.add(nm[-1])
            for q in re.findall(r"([A-Za-z_]\w*)\s*::\s*([A-Za-z_~]\w*)\s*\(", h2):
                c.provides.add(q[1])
        else:
            c.kind = "static_data"
            nm = re.findall(r"[A-Za-z_]\w*", re.split(r"[=\[;{]", head)[0])
            if nm: c.provides.add(nm[-1])
        return
    if first == "extern":
        c.kind = "extern"
        nm = re.findall(r"[A-Za-z_]\w*", re.split(r"[=\[;{(]", head)[0])
        if nm: c.provides.add(nm[-1])
        return
    if is_fn and "{" not in code and "=" not in head:
        c.kind = "decl"                      # a function declaration
        h2 = re.sub(r"__declspec\s*\([^)]*\)|__forceinline|__cdecl|__stdcall|__fastcall|__thiscall", " ", head)
        nm = [x for x in re.findall(r"([A-Za-z_~]\w*)\s*\(", h2[: h2.find("(") + 1]) if x not in _KW_SKIP]
        if nm:
            c.provides.add(nm[-1])
        return
    c.kind = "def"


def enum_names(text):
    m = re.search(r"\{(.*)\}", text, re.S)
    if not m:
        return set()
    out = set()
    for part in m.group(1).split(","):
        n = re.match(r"\s*([A-Za-z_]\w*)", part)
        if n: out.add(n.group(1))
    return out


def idents(code):
    return set(IDENT.findall(code))
