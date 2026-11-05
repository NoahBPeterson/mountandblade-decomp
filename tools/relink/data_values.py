#!/usr/bin/env python3
"""data_values.py: the build's data_values gate -- do the globals our source defines at the original's addresses start
with the original's bytes?

Every other gate compares code, names, sizes or vtable slots; none compares a data VALUE. `int g_str_grow_add;`
(0x5a8e60) defined with no initialiser held 0 where the original image holds 128 (and `float g_str_grow_mul;` 0 where
it holds 2.0f): every function was byte-exact and the save-game code looped forever. For each data definition of ours
at an original address (the ones the `data` gate counts) this compares the bytes the linked image holds at load time
with the original image's bytes at that address:

  pointers     a dword that is a base relocation in our image (a pointer) is equal when both point at the same object:
               our target is mapped back through the link map and the resolver's names (or the copy of the original's
               data) to an original address; a pointer to a string literal also counts when both strings read the same.
  constructed  an object a compiler-made initialiser fills at startup (a function our objects list in .CRT$XCU refers to it) has
               no static value to compare: it is listed as "constructed at startup, not compared".
  size         the smaller of our compiled size and the largest size a declaration of that address claims, so alignment
               padding after the object is not compared.

The result goes to build.json (data_values_report: every difference with both byte strings).
"""
import bisect, os, re, struct


def coff_refs(path):
    """The globals a compiler-made startup initialiser of one object file refers to: the object's .CRT$XCU entries
    name its initialiser functions (`_$E1`); the data symbols those functions' relocations name are the storage the
    constructors fill. -> set of symbol names."""
    d = open(path, "rb").read()
    _m, nsec, _ts, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + 18 * nsym

    def nm(b):
        if b[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", b, 4)[0]
            return d[strtab + off:d.index(b"\0", strtab + off)].decode("latin1")
        return b.rstrip(b"\0").decode("latin1")

    names, sec_of, i = {}, {}, 0
    while i < nsym:
        o = symptr + 18 * i
        sec = struct.unpack_from("<h", d, o + 12)[0]
        names[i], sec_of[i] = nm(d[o:o + 8]), sec
        naux = d[o + 17]
        i += 1 + naux
    secs = []
    for k in range(nsec):
        o = 20 + optsz + 40 * k
        secs.append((nm(d[o:o + 8]), struct.unpack_from("<I", d, o + 24)[0], struct.unpack_from("<H", d, o + 32)[0]))

    def relocs(si):
        _n, ptr, cnt = secs[si - 1]
        return [struct.unpack_from("<I", d, ptr + 10 * j + 4)[0] for j in range(cnt)]

    out = set()
    for si, (name, _p, _c) in enumerate(secs, 1):
        if name.startswith(".CRT$XCU"):
            for sym in relocs(si):
                if sec_of.get(sym, 0) > 0:
                    out.update(names[r] for r in relocs(sec_of[sym]) if r in names)
    return out


def constructed_names(objs):
    """Mangled names of the globals a startup initialiser of our objects touches (see coff_refs)."""
    out = set()
    for o in objs:
        out |= coff_refs(o)
    return out


def compare(exe, symmap, rmap, img, objs, ours, claims, defined, sites, orig_of):
    """-> dict(compared, differ [..], constructed [..], not_linked [..])"""
    site_set = {va for vs in sites.values() for va in vs}
    pairs = []                                      # (our address, original address) of every name the resolver bound
    for n, d in rmap["names"].items():
        a = symmap.get(n)
        if a:
            pairs.extend((a, int(va, 16)) for va in d)
    pairs.sort()
    pa = [p[0] for p in pairs]

    def origs_of(t):
        o = orig_of(t)
        if o is not None:
            return {o}
        i = bisect.bisect_right(pa, t)
        if not i:
            return set()
        a = pa[i - 1]
        out = set()
        while i and pa[i - 1] == a:
            out.add(pairs[i - 1][1] + t - a)
            i -= 1
        return out

    def ours_read(va, n):
        for a, sz, p in exe.secs:
            if a <= va < a + sz:
                k = min(n, a + sz - va)
                return bytes(exe.d[p + va - a:p + va - a + k]) + b"\0" * (n - k)
        return b"\0" * n                                # past the file-backed part of its section: .bss

    def orig_read(va, n):
        try:
            d = img.read(va, n)
        except Exception:
            d = b""
        return d + b"\0" * (n - len(d))

    def cstr(read, va):
        out = b""
        while len(out) < 256:
            c = read(va + len(out), 1)
            if c == b"\0":
                return out
            out += c
        return None

    def eq_ptr(t, o):
        if t == o or o in origs_of(t):
            return True
        a, b = cstr(ours_read, t), cstr(orig_read, o)
        return a is not None and a == b and len(a) > 0

    ctor = constructed_names(objs)
    res = dict(compared=0, differ=[], constructed=[], not_linked=[])
    for va, names in sorted(defined.items()):
        for n in names:
            size = min(s for s, _o in ours[n])
            cl = max((c[0] for c in claims.get(va, [])), default=size)
            size = min(size, cl) if cl else size
            if n in ctor:
                res["constructed"].append(dict(va="%08x" % va, name=n, size=size))
                continue
            at = symmap.get(n)
            if not at:
                res["not_linked"].append(dict(va="%08x" % va, name=n))
                continue
            mine, theirs = ours_read(at, size), orig_read(va, size)
            res["compared"] += 1
            if mine == theirs:
                continue
            bad, i = [], 0
            while i < size:
                if at + i in site_set and i + 4 <= size:
                    t, o = struct.unpack_from("<I", mine, i)[0], struct.unpack_from("<I", theirs, i)[0]
                    if not eq_ptr(t, o):
                        bad.append(i)
                    i += 4
                else:
                    if mine[i] != theirs[i]:
                        bad.append(i)
                    i += 1
            if bad:
                res["differ"].append(dict(va="%08x" % va, name=n, size=size, linked="%08x" % at, offsets=bad[:16],
                                          ours_at_offsets=[mine[o:o + 4].hex() for o in bad[:16]],
                                          original_at_offsets=[theirs[o:o + 4].hex() for o in bad[:16]]))
    return res
