#!/usr/bin/env python3
"""profile_report.py: symbolise patches/profile's samples and split them into the game's load phases.

    profile_report.py --samples game/profile_samples.bin --exe <dir>/mb_standalone.exe [--log stamps.tsv] [--top 25]

<dir> must hold the mb_standalone.map of the same build. Each sample is the main thread's EIP plus the stack dwords that
fall in our .text; a dword counts as a frame only when the instruction before it is a call (E8 rel32, or FF /2 with a
2/3/6-byte encoding), which drops most stale words. Per phase (the spans between logwatch.py's stamped log lines; the
whole run without --log) it prints the wall time, the sample count, and the top functions by
  self      the innermost of our functions in the sample (a sample inside Wine/d3d is charged to our caller), and
  inclusive any of our functions anywhere in the sample (counted once per sample).
"""
import argparse, bisect, collections, os, re, struct, sys

import pefile


def load_samples(path):
    d = open(path, "rb").read()
    magic, tick0, lo, hi = struct.unpack_from("<4I", d, 0)
    if magic != 0x5350424d:
        sys.exit("%s: not a profile_samples.bin (magic %08x)" % (path, magic))
    epoch0 = ((hi << 32 | lo) - 116444736000000000) // 10000          # FILETIME -> unix ms
    out, o = [], 16
    while o + 12 <= len(d):
        tick, eip, n = struct.unpack_from("<3I", d, o)
        if o + 12 + 4 * n > len(d):
            break
        addrs = struct.unpack_from("<%dI" % n, d, o + 12)
        out.append((epoch0 + (tick - tick0), eip, addrs))
        o += 12 + 4 * n
    return out


def load_modules(path, game_dir):
    """[(base, size, short name, [(export rva, name)] for DLLs in the game folder)] from the sampler's module list."""
    out = []
    if not os.path.exists(path):
        return out
    for line in open(path, errors="replace"):
        parts = line.rstrip("\n").split(" ", 2)
        if len(parts) < 3:
            continue
        b, size, full = int(parts[0], 16), int(parts[1], 16), parts[2]
        short = full.replace("\\", "/").rsplit("/", 1)[-1]
        exps = []
        local = os.path.join(game_dir, short)
        if os.path.exists(local) and not short.lower().startswith("mb_"):
            try:
                pe = pefile.PE(local, fast_load=True)
                pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
                exps = sorted((e.address, (e.name or b"#%d" % e.ordinal).decode()) for e in
                              getattr(pe, "DIRECTORY_ENTRY_EXPORT", pefile.Structure({})).symbols)
            except Exception:
                exps = []
        out.append((b, size, short, exps))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--samples", required=True)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--log", help="logwatch.py output: phase boundaries")
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--min-phase-ms", type=int, default=150, help="fold shorter phases into the next one")
    ap.add_argument("--modules", help="profile_modules.txt (default: next to --samples): charge samples outside our "
                    "image to the DLL they are in, by its nearest export when the DLL is in the game folder")
    a = ap.parse_args()

    pe = pefile.PE(a.exe)
    base = pe.OPTIONAL_HEADER.ImageBase
    img = pe.get_memory_mapped_image()
    text = pe.sections[0]
    lo, hi = base + text.VirtualAddress, base + text.VirtualAddress + text.Misc_VirtualSize
    mp = open(os.path.splitext(a.exe)[0] + ".map", errors="replace").read()
    fn = sorted({(int(v, 16), n) for n, v in re.findall(r"^\s*0001:\S+\s+(\S+)\s+([0-9a-f]{8})\s", mp, re.M)})
    starts = [v for v, _ in fn]

    def name(va):
        i = bisect.bisect(starts, va) - 1
        return fn[i][1] if i >= 0 else "?"

    def after_call(va):
        o = va - base
        return (img[o - 5] == 0xE8 or (img[o - 2] == 0xFF and (img[o - 1] >> 3) & 7 == 2)
                or (img[o - 3] == 0xFF and (img[o - 2] >> 3) & 7 == 2) or (img[o - 6] == 0xFF and (img[o - 5] >> 3) & 7 == 2))

    mods = load_modules(a.modules or os.path.join(os.path.dirname(a.samples), "profile_modules.txt"),
                        os.path.dirname(a.samples))

    def where(va):
        for b, size, short, exps in mods:
            if b <= va < b + size:
                if exps:
                    i = bisect.bisect([e for e, _n in exps], va - b) - 1
                    if i >= 0:
                        return "%s!%s" % (short, exps[i][1])
                return short
        return "[unknown %08x]" % (va & 0xfff00000)

    samples = load_samples(a.samples)
    if not samples:
        sys.exit("no samples")
    phases = [(samples[0][0], "start")]
    if a.log:
        for line in open(a.log, errors="replace"):
            t, _, txt = line.rstrip("\n").partition("\t")
            txt = txt.strip()
            if txt and t.isdigit() and samples[0][0] <= int(t) <= samples[-1][0]:
                phases.append((int(t), txt[:60]))
    merged = []
    for t, txt in phases:
        if merged and t - merged[-1][0] < a.min_phase_ms:
            merged[-1] = (merged[-1][0], merged[-1][1] + " / " + txt)
        else:
            merged.append((t, txt))
    bounds = [t for t, _ in merged] + [samples[-1][0] + 1]

    per = collections.defaultdict(lambda: (collections.Counter(), collections.Counter(), [0]))
    ext = collections.defaultdict(collections.Counter)      # phase -> where an EIP outside our image is
    for t, eip, addrs in samples:
        k = bisect.bisect(bounds, t) - 1
        selfc, incl, cnt = per[k]
        cnt[0] += 1
        frames = [eip] if lo <= eip < hi else []
        if not frames:
            ext[k][where(eip)] += 1
        frames += [x for x in addrs if after_call(x)]
        names = [name(x) for x in frames]
        selfc[names[0] if names else "[outside our code]"] += 1
        for nm in set(names):
            incl[nm] += 1
    total = len(samples)
    print("%d samples over %.1f s" % (total, (samples[-1][0] - samples[0][0]) / 1000))
    for k, (t, txt) in enumerate(merged):
        if k not in per:
            continue
        selfc, incl, cnt = per[k]
        dur = bounds[k + 1] - t
        print("\n=== %6.1f s  %5d samples  after: %s" % (dur / 1000, cnt[0], txt))
        print("  self:")
        for nm, c in selfc.most_common(a.top):
            print("    %5.1f%%  %s" % (100.0 * c / cnt[0], nm[:110]))
        print("  inclusive:")
        for nm, c in incl.most_common(a.top):
            print("    %5.1f%%  %s" % (100.0 * c / cnt[0], nm[:110]))
        if ext[k]:
            print("  EIP outside our image:")
            for nm, c in ext[k].most_common(min(a.top, 12)):
                print("    %5.1f%%  %s" % (100.0 * c / cnt[0], nm[:110]))


if __name__ == "__main__":
    sys.exit(main())
