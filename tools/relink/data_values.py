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
  constructed  a global a compiler-made initialiser writes at startup (a function our objects list in .CRT$XCU) has no static
               value to compare. Only a write counts: a constructor call on its address, a `lea`, an indexed store ->
               skipped as "constructed", the whole object; direct stores `mov [g + k], x` -> only those bytes are skipped
               and the rest compared; a global the initialiser only reads (`g_bit`, `g_5a9f20`) is compared in full.
  size         the smaller of our compiled size and the largest size a declaration of that address claims, so alignment
               padding after the object is not compared.
  extent       the original's object may be longer than ours: the comparison runs over its whole extent (the distance to
               the next label or used address of the original; orig_extents). Past our end the linked image holds another
               object's bytes, so a longer original is a "short object" whether or not the tail happens to match
               (`g_nbr[8]` was followed by a string in our image; the original has a 9th entry {0,0} that the terrain code
               indexes). Excluded: alignment padding the original has as zeros, objects with no known extent, tails no code
               can reach (a scalar read only through register-free operands of its own size) and the objects listed in
               data_short_reviewed.txt (checked by hand: the tail is another object).

The result goes to build.json (data_values_report: every difference with both byte strings).
"""
import bisect, os, re, struct


def coff_writes(path):
    """What the compiler-made startup initialisers of one object file do to the globals they refer to: the object's
    .CRT$XCU entries name its initialiser functions (`_$E1`); every relocation of such a function that names a data
    symbol is classified by the instruction that holds it:
      a store to [sym + k] (mov / fstp / add ... with a write operand, no index register) writes size bytes at k;
      a load or compare of [sym + k] only reads it (the global keeps its static bytes);
      anything else (the address taken for a constructor call or a `lea`, an indexed operand) may write the whole object.
    -> {symbol name: [(offset, size)] of the direct stores, or None when the whole object may be written};
       a symbol only read is absent."""
    import capstone
    from capstone.x86 import X86_OP_MEM, X86_OP_IMM
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
        secs.append((nm(d[o:o + 8]), struct.unpack_from("<I", d, o + 20)[0], struct.unpack_from("<I", d, o + 16)[0],
                     struct.unpack_from("<I", d, o + 24)[0], struct.unpack_from("<H", d, o + 32)[0]))

    def relocs(si):
        _n, _raw, _sz, ptr, cnt = secs[si - 1]
        return [struct.unpack_from("<IIH", d, ptr + 10 * j) for j in range(cnt)]       # (offset, symbol, type)

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    out = {}

    def note(sym, rng):
        if sym in out and out[sym] is None:
            return
        if rng is None:
            out[sym] = None
        else:
            out.setdefault(sym, []).append(rng)

    for si, (name, _r, _s, _p, _c) in enumerate(secs, 1):
        if not name.startswith(".CRT$XCU"):
            continue
        for _off, fsym, _t in relocs(si):
            fs = sec_of.get(fsym, 0)
            if fs <= 0:
                continue
            _n, raw, size, _p2, _c2 = secs[fs - 1]
            code = d[raw:raw + size]
            spans = [(ins.address, ins.address + ins.size, ins) for ins in md.disasm(code, 0)]
            starts = [sp[0] for sp in spans]
            for roff, rsym, rtype in relocs(fs):
                if rsym not in names or rtype == 20 or sec_of.get(rsym, 0) <= 0:      # REL32: a call / jump to code
                    continue
                if size < roff + 4:
                    continue
                addend = struct.unpack_from("<i", code, roff)[0]
                k = bisect.bisect_right(starts, roff) - 1
                ins = spans[k][2] if k >= 0 and spans[k][0] <= roff < spans[k][1] else None
                rng = None
                if ins is not None:
                    for op in ins.operands:
                        if op.type == X86_OP_MEM and op.mem.disp == addend and not op.mem.base and not op.mem.index \
                                and op.mem.segment == 0:
                            acc = op.access
                            if acc & 2:                                         # CS_AC_WRITE
                                rng = (addend, op.size)
                            elif acc & 1:                                       # CS_AC_READ only
                                rng = False
                            break
                if rng is False:
                    continue
                note(names[rsym], rng)
    return out


def constructed_info(objs):
    """{mangled name: None (the initialisers may write the whole object) | [(offset, size)] (their direct stores)}
    for the globals the startup initialisers of our objects write (see coff_writes)."""
    out = {}
    for o in objs:
        for n, v in coff_writes(o).items():
            if n in out and out[n] is None:
                continue
            out[n] = None if v is None else out.get(n, []) + v
    return out


REVIEWED = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data_short_reviewed.txt")


def reviewed_short():
    """{original address: reason} of the short objects somebody checked (data_short_reviewed.txt: `ADDR  # reason`, one per
    line): the tail past our end is another object of the original that the indexed code never reaches, not a missing
    element. The gate fails on every other short object."""
    got = {}
    if os.path.exists(REVIEWED):
        for line in open(REVIEWED):
            body, _s, why = line.partition("#")
            if body.split():
                got[int(body.split()[0], 16)] = why.strip()
    return got


def is_padding(va, size, ext, tail):
    """The original's bytes [va + size, va + ext) are alignment padding: all zero, and the next object's address is
    what rounding the end of this one up to a 2 / 4 / 8 / 16-byte alignment gives (g_nbr's missing 8 bytes are zero
    too, but its end was already 8-aligned: that is an object, not padding)."""
    if any(tail):
        return False
    end, nxt = va + size, va + ext
    return any(nxt % a == 0 and (end + a - 1) // a * a == nxt for a in (2, 4, 8, 16))


def orig_extents(img, rmap):
    """-> function(va, size) -> (extent, reachable) for an object of ours at original address va that is `size` bytes.
    extent: the size of the original object that starts at va: the distance to the first label or used address of the
        original (tools/relink/data_coverage.py's bounds: an address an instruction uses at any alignment, a name of the
        resolver's, the target of a pointer stored in the data) at or past va + size. Addresses inside our size are
        fields of the object; one right at va + size makes the extent equal to size. None when va is not in the static data.
    reachable: some use inside [va, va + size) can address bytes past it: an indexed operand (`[eax*4 + va]`), the
        address as a value (push / lea / mov reg, offset) or a pointer stored in the data. An object only ever read
        through register-free operands of its own size cannot reach a tail: the tail is another object no code addresses."""
    import data_coverage as DC
    from glue import RDATA, DATA, BSS, IAT_LO, IAT_HI, section_of
    starts, game = DC.game_functions()
    bounds, refs = [], {}
    DC.data_items(img, rmap, starts, game, bounds_out=bounds, refs_out=refs)
    # a dword anywhere in the initialised data that points into the static data is a reference too (a table of 12-byte
    # records holding string pointers is no run of 4+ pointers, yet each string is an object of its own)
    ptr_targets = set()
    for sec in (RDATA, DATA):
        data = img.read(sec[1], sec[2] - sec[1])
        for off in range(0, len(data) - 3, 4):
            v = struct.unpack_from("<I", data, off)[0]
            if RDATA[1] <= v < BSS[2] and not IAT_LO <= v < IAT_HI and not IAT_LO <= sec[1] + off < IAT_HI:
                ptr_targets.add(v)
    bounds = sorted(set(bounds) | ptr_targets | set(refs))
    escaping = sorted(v for v, k in refs.items() if k & {"indexed", "imm"}) 
    escaping = sorted(set(escaping) | ptr_targets)

    def extent(va, size):
        sec = section_of(va)
        if sec is None:
            return None, False
        i = bisect.bisect_left(bounds, va + size)
        nxt = bounds[i] if i < len(bounds) else sec[2]
        j = bisect.bisect_left(escaping, va)
        return max(size, min(nxt, sec[2]) - va), j < len(escaping) and escaping[j] < va + size
    return extent


def compare(exe, symmap, rmap, img, objs, ours, claims, defined, sites, orig_of, extent_of=lambda va, size: (None, False), reviewed=None):
    """-> dict(compared, differ [..], short [..], constructed [..], not_linked [..])"""
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

    ctor = constructed_info(objs)
    res = dict(compared=0, differ=[], short=[], short_reviewed=[], unreachable_tail=[], constructed=[], not_linked=[], partly_compared=0, no_extent=0)
    for va, names in sorted(defined.items()):
        for n in names:
            size = min(s for s, _o in ours[n])
            cl = max((c[0] for c in claims.get(va, [])), default=size)
            size = min(size, cl) if cl else size
            ext, reach = extent_of(va, size)
            if ext is None:
                res["no_extent"] += 1
                ext = size
            at = symmap.get(n)
            # the original object's whole extent is compared: past our end the linked image holds another object's bytes
            tail_ok = ext <= size or is_padding(va, size, ext, orig_read(va + size, ext - size))
            cmp_n = size if tail_ok else ext
            written = None                                  # byte offsets the startup code writes (not compared)
            if n in ctor:
                rngs = ctor[n]
                written = set(range(cmp_n)) if rngs is None else {o for off, sz in rngs for o in range(off, off + sz)}
                if rngs is None or len(written & set(range(size))) >= size:
                    res["constructed"].append(dict(va="%08x" % va, name=n, size=size, written="whole",
                                                   why="its constructor / initialiser takes its address" if rngs is None
                                                   else "initialiser stores cover all %d bytes" % size))
                    if tail_ok:
                        continue
                    written = set(range(size)) if rngs is None else written     # the tail past our end is still compared
                else:
                    res["partly_compared"] += 1
                    res["constructed"].append(dict(va="%08x" % va, name=n, size=size,
                                                   written=["+%x/%d" % r for r in sorted(rngs)],
                                                   why="initialiser stores; the other bytes are compared"))
            if not at:
                res["not_linked"].append(dict(va="%08x" % va, name=n))
                continue
            mine, theirs = ours_read(at, cmp_n), orig_read(va, cmp_n)
            res["compared"] += 1
            bad, i = [], 0
            while i < cmp_n:
                if at + i in site_set and i + 4 <= cmp_n:
                    t, o = struct.unpack_from("<I", mine, i)[0], struct.unpack_from("<I", theirs, i)[0]
                    if not eq_ptr(t, o) and not (written and i in written):
                        bad.append(i)
                    i += 4
                else:
                    if mine[i] != theirs[i] and not (written and i in written):
                        bad.append(i)
                    i += 1
            head = [o for o in bad if o < size]
            tail = [o for o in bad if o >= size]
            if head:
                res["differ"].append(dict(va="%08x" % va, name=n, size=size, linked="%08x" % at, offsets=head[:16],
                                          ours_at_offsets=[mine[o:o + 4].hex() for o in head[:16]],
                                          original_at_offsets=[theirs[o:o + 4].hex() for o in head[:16]]))
            if not tail_ok:
                row = dict(va="%08x" % va, name=n, size=size, extent=ext, linked="%08x" % at,
                           tail_differs=bool(tail), offsets=tail[:16],
                           ours_at_offsets=[mine[o:o + 4].hex() for o in tail[:16]],
                           original_at_offsets=[theirs[o:o + 4].hex() for o in tail[:16]],
                           original_tail=theirs[size:size + 32].hex())
                if not reach:                       # only register-free operands of its own size touch it: no code can read the tail
                    res["unreachable_tail"].append(row)
                elif va in (reviewed or {}):
                    res["short_reviewed"].append(dict(row, why=reviewed[va]))
                else:
                    res["short"].append(row)
    return res
