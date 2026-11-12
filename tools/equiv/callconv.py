"""Stack argument bytes of a callee, from its MSVC decorated name (new; not in spore-decomp).

The frame-slot model of rename_core.py needs to know which bytes above esp a call reads: a callee that pops its
arguments (`ret N`) says so itself, but a __cdecl callee (`ret`) does not. Its decorated name does:
    parse("?f@@YAXHPAUv@@N@Z") -> {"conv": "cdecl", "args": 16, "pops": 0}
Fails closed: anything it does not understand (by-value class arguments, varargs, $-template arguments it cannot
skip, function-pointer types it cannot skip) returns None, and the caller falls back to "the call may read the
whole frame". A wrong (too small) size would be unsound, so tools/equiv/test_callconv.py checks the parser against
the `ret N` of every callee-pops function the original calls (via our objects' symbols).
"""

PRIM4 = set("CDEFGHIJKM")             # char .. long, float: one stack dword
PRIM8 = set("NO")                     # double, long double (64-bit in MSVC)
CONV = {"A": "cdecl", "B": "cdecl", "C": "pascal", "D": "pascal", "E": "thiscall", "F": "thiscall",
        "G": "stdcall", "H": "stdcall", "I": "fastcall", "J": "fastcall"}
STATIC_MEMBER = set("CDKLST")
MEMBER = set("ABEFIJMNQRUV")


class Fail(Exception):
    pass


class P:
    def __init__(self, s):
        self.s, self.i = s, 0
        self.argrefs = []             # sizes of argument types whose encoding is longer than one character

    def peek(self, k=0):
        if self.i + k >= len(self.s):
            raise Fail("end")
        return self.s[self.i + k]

    def take(self, n=1):
        if self.i + n > len(self.s):
            raise Fail("end")
        t = self.s[self.i:self.i + n]
        self.i += n
        return t

    # --- names -----------------------------------------------------------------------------------------------
    def fragment(self):
        """One name fragment (identifier@, backref digit, ?$template, ?operator)."""
        c = self.peek()
        if c.isdigit():
            self.take()
            return
        if c == "?":
            if self.peek(1) == "$":
                self.take(2)
                self.ident()
                self.template_args()
                return
            if self.peek(1) == "?":                 # a nested decorated name (local static scope etc.)
                raise Fail("nested name")
            raise Fail("special name in scope")
        self.ident()

    def ident(self):
        j = self.s.find("@", self.i)
        if j < 0:
            raise Fail("identifier")
        self.i = j + 1

    def template_args(self):
        saved = self.argrefs
        self.argrefs = []                            # templates have their own back-reference tables
        try:
            while self.peek() != "@":
                if self.peek() == "$":
                    self.take()
                    k = self.take()
                    if k == "0":
                        self.number()
                    else:
                        raise Fail("template $%s" % k)
                else:
                    self.type(arg=True)
            self.take()
        finally:
            self.argrefs = saved

    def number(self):
        c = self.take()
        if c == "?":
            c = self.take()
        if c.isdigit():
            return
        while c != "@":
            if not "A" <= c <= "P":
                raise Fail("number")
            c = self.take()

    def qualname_rest(self):
        """Scope fragments up to and including the terminating '@'."""
        while self.peek() != "@":
            self.fragment()
        self.take()

    # --- types -----------------------------------------------------------------------------------------------
    def type(self, arg=False):
        """Skip one type; -> its size on the stack (bytes) or None when unknown (a class by value)."""
        start = self.i
        size = self._type()
        if arg and self.i - start > 1:
            self.argrefs.append(size)
        return size

    def _type(self):
        c = self.take()
        if c in PRIM4:
            return 4
        if c in PRIM8:
            return 8
        if c == "X":
            return 0
        if c == "_":
            k = self.take()
            if k in "JK":
                return 8
            if k in "DEFGHINW":
                return 4
            raise Fail("_%s" % k)
        if c in "PQRS":                              # pointers
            k = self.peek()
            if k == "E":                             # __ptr64
                self.take()
                k = self.peek()
            if k == "6":                             # function pointer
                self.take()
                self.functype()
                return 4
            if k in "ABCD":
                self.take()
                if self.peek() == "Y":               # array pointee: Y<n><dims>type
                    self.take()
                    n = self.dimcount()
                    for _ in range(n):
                        self.number()
                self.type()
                return 4
            raise Fail("pointer %s" % k)
        if c in "AB":                                # references
            k = self.take()
            if k not in "ABCD":
                raise Fail("reference %s" % k)
            self.type()
            return 4
        if c in "VUT":                               # class / struct / union by value
            self.qualname_rest()
            return None
        if c == "W":                                 # enum
            self.take()
            self.qualname_rest()
            return 4
        if c.isdigit():
            k = int(c)
            if k >= len(self.argrefs):
                raise Fail("backref %d" % k)
            return self.argrefs[k]
        if c == "?":                                 # ?A<type> (a cv-qualified return / template argument)
            self.take()
            return self._type()
        raise Fail("type %s" % c)

    def dimcount(self):
        c = self.take()
        if c.isdigit():
            return int(c) + 1
        n = 0
        while c != "@":
            n = n * 16 + (ord(c) - ord("A"))
            c = self.take()
        return n

    def functype(self):
        """conv, return type, argument list, throw spec of a function (pointer) type."""
        self.take()                                  # calling convention
        if self.peek() == "@":
            self.take()
        else:
            self.type()
        self.arglist()
        if self.peek() == "Z":
            self.take()

    def arglist(self):
        """-> total stack bytes, or raise Fail (varargs, unknown sizes)."""
        if self.peek() == "X":
            self.take()
            return 0
        total = 0
        unknown = False
        while True:
            c = self.peek()
            if c == "@":
                self.take()
                break
            if c == "Z":
                raise Fail("varargs")
            sz = self.type(arg=True)
            if sz is None:
                unknown = True
            else:
                total += (sz + 3) & ~3
        if unknown:
            raise Fail("class argument by value")
        return total


def parse(name):
    """-> {"conv", "args" (stack bytes, including a hidden return pointer), "pops" (bytes the callee removes)} or
    None."""
    if name.startswith("__imp_"):
        name = name[6:]
    if name.startswith("@"):                          # __fastcall C name: @f@N
        try:
            n = int(name.rsplit("@", 1)[1])
        except ValueError:
            return None
        return {"conv": "fastcall", "args": n, "pops": max(n - 8, 0)}
    if name.startswith("_") and not name.startswith("__"):
        if "@" in name:                               # __stdcall C name: _f@N
            try:
                n = int(name.rsplit("@", 1)[1])
            except ValueError:
                return None
            return {"conv": "stdcall", "args": n, "pops": n}
        return C_CDECL.get(name[1:])
    if not name.startswith("?"):
        return None
    p = P(name)
    try:
        p.take()
        # the function's own name: ?name@, ??0 (ctor), ??_G (special), ?$template
        if p.peek() == "?":
            p.take()
            k = p.take()
            if k == "_":
                p.take()
            elif k == "$":
                p.ident()
                p.template_args()
        else:
            p.ident()
        p.qualname_rest()
        fc = p.take()
        if fc == "Y" or fc == "Z":
            pass
        elif fc in STATIC_MEMBER:
            pass
        elif fc in MEMBER:
            k = p.take()
            if k == "E":
                k = p.take()
            if k not in "ABCD":
                return None
        else:
            return None                               # thunks ($), data, vtables
        cv = p.take()
        conv = CONV.get(cv)
        if conv is None:
            return None
        hidden = 0
        if p.peek() == "@":                           # ctor / dtor: no return type
            p.take()
        else:
            if p.peek() == "?":                       # ?A<class>: returned by value through a hidden pointer
                p.take(2)
                if p.peek() in "VUT":
                    hidden = 4
                p.type()
            else:
                rs = p.type()
                if rs is None:
                    hidden = 4
        args = p.arglist() + hidden
    except Fail:
        return None
    pops = 0 if conv == "cdecl" else (max(args - 8, 0) if conv == "fastcall" else args)
    return {"conv": conv, "args": args, "pops": pops}


# __cdecl C functions of the static CRT that M&B code calls, with their stack argument bytes (prototypes from the
# VC7.1 headers). Anything not listed is unknown.
C_CDECL = {k: {"conv": "cdecl", "args": v, "pops": 0} for k, v in {
    "malloc": 4, "free": 4, "calloc": 8, "realloc": 8, "strlen": 4, "strcpy": 8, "strcat": 8, "strcmp": 8,
    "strncpy": 12, "strncmp": 12, "stricmp": 8, "_stricmp": 8, "strchr": 8, "strrchr": 8, "strstr": 8,
    "memcpy": 12, "memset": 12, "memmove": 12, "memcmp": 12, "atoi": 4, "atol": 4, "atof": 4, "abs": 4,
    "rand": 0, "srand": 4, "time": 4, "fclose": 4, "fopen": 8, "fread": 16, "fwrite": 16, "fgetc": 4,
    "fseek": 12, "ftell": 4, "wcslen": 4, "wcscpy": 8, "toupper": 4, "tolower": 4, "isspace": 4,
    "sqrt": 8, "sin": 8, "cos": 8, "atan2": 16, "pow": 16, "fabs": 8, "floor": 8, "ceil": 8, "fmod": 16,
    "exp": 8, "log": 8, "tan": 8, "asin": 8, "acos": 8, "atan": 8, "qsort": 16, "_purecall": 0,
}.items()}
