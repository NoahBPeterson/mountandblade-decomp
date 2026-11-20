#!/usr/bin/env python3
"""ntrace.py: read a Newton 1.x call trace (format: newton_trace.h, written by proxy/newton_proxy.c).

    ntrace.py summary TRACE            op counts, worlds (lifetime in steps, bodies, bones), probes
    ntrace.py dump TRACE [--world N] [--ops a,b] [--limit N]   one line per record
    ntrace.py worlds TRACE             one line per world: steps, ragdoll bones, mount capsule, time span, concurrency
    ntrace.py extract TRACE OUT N[,N]  copy the records of the given world instances (numbers from `worlds`; and everything that refers to their
                                       handles) into a smaller trace, e.g. a reference trace for the repo
"""
import os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def load_specs():
    src = open(os.path.join(HERE, "newton_trace.h")).read()
    names = re.findall(r"^\s+NT_(\w+)(?: = (\d+))?,", src.split("enum NtOp")[1].split("};")[0], re.M)
    ops, n = {}, 0
    for name, val in names:
        n = int(val) if val else n
        ops[n] = name
        n += 1
    arr = src.split("kNtSpec[NT_OP_MAX] = {")[1].split("};")[0]
    arr = re.sub(r"/\*.*?\*/", "", arr, flags=re.S)
    items = [x.strip() for x in arr.split(",") if x.strip()]
    specs = {}
    for i, it in enumerate(items):
        if it.startswith('"'):
            specs[i] = it.strip('"')
    return ops, specs


OPS, SPECS = load_specs()
BYNAME = {v: k for k, v in OPS.items()}
SIZES = {"h": 4, "i": 4, "f": 4, "3": 12, "M": 64}


def decode(op, payload):
    """(inputs, outputs) lists of python values for one record."""
    spec = SPECS.get(op, "")
    vals, pos, outs = [], 0, None
    for c in spec:
        if c == "|":
            outs = len(vals)
            continue
        if c == "h":
            vals.append(struct.unpack_from("<I", payload, pos)[0]); pos += 4
        elif c == "i":
            vals.append(struct.unpack_from("<i", payload, pos)[0]); pos += 4
        elif c == "f":
            vals.append(struct.unpack_from("<f", payload, pos)[0]); pos += 4
        elif c == "3":
            vals.append(struct.unpack_from("<3f", payload, pos)); pos += 12
        elif c == "M":
            vals.append(struct.unpack_from("<16f", payload, pos)); pos += 64
        elif c == "m":
            present = payload[pos]; pos += 1
            if present:
                vals.append(struct.unpack_from("<16f", payload, pos)); pos += 64
            else:
                vals.append(None)
        elif c == "F":
            n = struct.unpack_from("<i", payload, pos)[0]; pos += 4
            vals.append([struct.unpack_from("<3f", payload, pos + 12 * k) for k in range(n)]); pos += 12 * n
    if outs is None:
        outs = len(vals)
    return vals[:outs], vals[outs:]


def records(path):
    """yields (index, op, thread, payload bytes, raw record bytes)"""
    import gzip
    with (gzip.open if path.endswith(".gz") else open)(path, "rb") as f:
        data = f.read()
    if data[:4] != b"NWTR":
        raise SystemExit("%s: not a Newton trace" % path)
    pos, i = 8, 0
    while pos + 4 <= len(data):
        op, th, ln = struct.unpack_from("<BBH", data, pos)
        if pos + 4 + ln > len(data):
            break   # truncated tail (the game was killed mid-buffer)
        yield i, op, th, data[pos + 4:pos + 4 + ln], data[pos:pos + 4 + ln]
        pos += 4 + ln
        i += 1


def fmt(v):
    if isinstance(v, float):
        return "%.6g" % v
    if isinstance(v, tuple):
        return "(" + " ".join("%.5g" % x for x in v) + ")"
    if isinstance(v, list):
        return "[" + " ".join(fmt(x) for x in v) + "]"
    if isinstance(v, int) and v > 0xFFFF:
        return "%08x" % v
    return str(v)


class WorldIndex:
    """maps every handle to its world (bodies, collisions, ragdolls, bones)"""

    def __init__(self):
        self.owner = {}       # handle -> world instance number (worlds' addresses are reused: instances are counted)
        self.info = {}        # instance -> stats
        self.handle = {}      # instance -> world handle

    def note(self, op, ins, outs):
        name = OPS.get(op, "?")
        o = self.owner
        if name == "Create":
            w = len(self.info) + 1
            self.info[w] = {"steps": 0, "bones": 0, "bodies": 0, "first": None, "last": None, "t0": None, "t1": None,
                            "mass": [], "capsules": 0, "destroyed": False}
            self.handle[w] = outs[0]
            o[outs[0]] = w
        elif name in ("CreateCapsule", "CreateTreeCollision", "CreateBody", "CreateRagDoll") and outs and outs[0]:
            o[outs[0]] = o.get(ins[0])
            if name == "CreateBody" and o.get(ins[0]) in self.info:
                self.info[o[ins[0]]]["bodies"] += 1
        elif name == "RagDollAddBone" and outs[0]:
            w = o.get(ins[0])
            o[outs[0]] = w
            if w in self.info:
                self.info[w]["bones"] += 1
        elif name == "RagDollBoneGetBody" and outs[0]:
            o[outs[0]] = o.get(ins[0])
        elif name == "ProbeBoneInit":
            o[ins[1]] = o.get(ins[0])

    def world_of(self, op, ins):
        if OPS.get(op) == "Create":
            return None
        return self.owner.get(ins[0]) if ins else None


def cmd_summary(path):
    counts, th = {}, {0: 0, 1: 0}
    wi = WorldIndex()
    n = 0
    for i, op, t, p, _ in records(path):
        ins, outs = decode(op, p)
        wi.note(op, ins, outs)
        counts[op] = counts.get(op, 0) + 1
        th[t] = th.get(t, 0) + 1
        n += 1
    print("%d records, %d worlds, threads: %s" % (n, len(wi.info), th))
    for op in sorted(counts):
        print("  %-42s %d" % (OPS.get(op, op), counts[op]))


def world_table(path):
    wi = WorldIndex()
    t_now = 0
    for i, op, t, p, _ in records(path):
        ins, outs = decode(op, p)
        wi.note(op, ins, outs)
        name = OPS.get(op)
        if name == "UpdateEnd":
            t_now = ins[1]
            w = wi.info.get(wi.owner.get(ins[0]))
            if w:
                w["steps"] += 1
                w["t0"] = t_now if w["t0"] is None else w["t0"]
                w["t1"] = t_now
        elif name == "Destroy" and wi.owner.get(ins[0]) in wi.info:
            wi.info[wi.owner[ins[0]]]["destroyed"] = True
        elif name == "CreateCapsule":
            w = wi.info.get(wi.owner.get(ins[0]))
            if w:
                w["capsules"] += 1
        if ins:
            w = wi.info.get(wi.world_of(op, ins))
            if w is not None:
                w["first"] = i if w["first"] is None else w["first"]
                w["last"] = i
    return wi


def cmd_worlds(path):
    wi = world_table(path)
    info = wi.info
    spans = [(k, v["t0"], v["t1"]) for k, v in info.items() if v["t0"] is not None]
    print("%-4s %-9s %6s %5s %6s %8s %14s %5s" % ("#", "handle", "steps", "bones", "bodies", "capsules", "time ms", "conc"))
    for k, v in info.items():
        conc = 0
        if v["t0"] is not None:
            conc = sum(1 for kk, a, b in spans if kk != k and a <= v["t1"] and b >= v["t0"])
        print("%-4d %08x %6d %5d %6d %8d %14s %5d%s" % (k, wi.handle[k], v["steps"], v["bones"], v["bodies"], v["capsules"],
              "%s-%s" % (v["t0"], v["t1"]), conc, "" if v["destroyed"] else " (not destroyed)"))


def cmd_dump(path, world=None, ops=None, limit=None):
    wi = WorldIndex()
    shown = 0
    for i, op, t, p, _ in records(path):
        ins, outs = decode(op, p)
        wi.note(op, ins, outs)
        name = OPS.get(op, str(op))
        if ops and name not in ops:
            continue
        if world is not None:
            w = wi.world_of(op, ins)
            if w != world:
                continue
        print("%7d %s %-34s %s | %s" % (i, "T1" if t else "  ", name, " ".join(fmt(x) for x in ins), " ".join(fmt(x) for x in outs)))
        shown += 1
        if limit and shown >= limit:
            break


def cmd_extract(path, out, worlds):
    """keeps the records whose first handle belongs to one of the worlds (callback markers and probes included)"""
    wi = WorldIndex()
    keep = []
    for i, op, t, p, raw in records(path):
        ins, outs = decode(op, p)
        wi.note(op, ins, outs)
        name = OPS.get(op)
        w = wi.owner.get(outs[0]) if name == "Create" else wi.world_of(op, ins)
        if w in worlds:
            keep.append(raw)
    with open(out, "wb") as f:
        f.write(b"NWTR" + struct.pack("<I", 1))
        for r in keep:
            f.write(r)
    print("%d records -> %s (%d bytes)" % (len(keep), out, os.path.getsize(out)))


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    if a[0] == "summary":
        cmd_summary(a[1])
    elif a[0] == "worlds":
        cmd_worlds(a[1])
    elif a[0] == "dump":
        world = ops = limit = None
        if "--world" in a:
            world = int(a[a.index("--world") + 1])
        if "--ops" in a:
            ops = set(a[a.index("--ops") + 1].split(","))
        if "--limit" in a:
            limit = int(a[a.index("--limit") + 1])
        cmd_dump(a[1], world, ops, limit)
    elif a[0] == "extract":
        cmd_extract(a[1], a[2], {int(x) for x in a[3].split(",")})
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
