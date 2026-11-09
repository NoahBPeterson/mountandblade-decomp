#!/usr/bin/env python3
"""Fingerprint one compiled function, so an equivalence verdict is only re-run when that function's code changed.

fn_sha(obj, symbol) -> sha1 hex of the symbol's bytes (up to the next symbol in its section, int3 padding stripped) with
every relocated field as the object holds it (the addend), plus each relocation's offset, type and TARGET NAME. So the fingerprint changes when an
instruction changes, and when an unchanged instruction now calls or reads a different symbol; it does not change when
some other function in the same object changes. None when the object has no such symbol.

The equiv verdict also depends on things outside the function's bytes (the original's code, the harness itself, the
row's options in row_opts.json, data the function reads): sweep_nonmatching.py folds the row options into its key, and
`--force` re-runs everything after a harness change.

usage: fn_hash.py <obj> <symbol>      prints the fingerprint
"""
import hashlib, struct, sys


def _coff(path):
    d = open(path, "rb").read()
    _m, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from("<HHIIIHH", d, 0)
    strtab = symptr + nsym * 18

    def name(raw):
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            return d[strtab + off: d.index(b"\0", strtab + off)].decode("latin1")
        return raw.split(b"\0")[0].decode("latin1")
    secs = []
    for k in range(nsec):
        o = 20 + optsz + 40 * k
        size, ptr, relptr = struct.unpack_from("<III", d, o + 16)
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        relocs = [struct.unpack_from("<IIH", d, relptr + 10 * j) for j in range(nrel)]
        secs.append((d[ptr:ptr + size] if ptr else b"\0" * size, relocs))
    names, syms, i = {}, {}, 0
    while i < nsym:
        o = symptr + 18 * i
        nm = name(d[o:o + 8])
        value, secno, typ, cls, naux = struct.unpack_from("<IhHBB", d, o + 8)
        names[i] = nm
        if secno > 0:
            syms.setdefault(secno - 1, []).append((value, nm))
        i += 1 + naux
    return secs, names, syms


def fn_sha(obj, symbol):
    secs, names, syms = _coff(obj)
    for si, lst in syms.items():
        for off, nm in lst:
            if nm != symbol:
                continue
            data, relocs = secs[si]
            nxt = sorted(o for o, _ in lst if o > off)
            end = nxt[0] if nxt else len(data)
            body = bytearray(data[off:end])
            if nxt:
                while body and body[-1] == 0xCC:
                    body.pop()
            h = hashlib.sha1()
            tags = []
            for va, idx, typ in sorted(relocs):
                if off <= va < off + len(body):
                    # the field keeps its bytes: in an object file it holds the addend ([g+8] vs [g+4]), not an address
                    tags.append("%x:%d:%s" % (va - off, typ, names.get(idx, "?")))
            h.update(bytes(body))
            h.update("\n".join(tags).encode())
            return h.hexdigest()
    return None


if __name__ == "__main__":
    print(fn_sha(sys.argv[1], sys.argv[2]))
