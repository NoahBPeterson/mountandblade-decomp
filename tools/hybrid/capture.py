#!/usr/bin/env python3
"""capture.py: build a runnable copy of the original exe that snapshots chosen functions' entry states.

usage: capture.py [--targets nonmatching|FILE|VA,...] [--max-hits N] [--out work/hybrid/mb.capture.exe]
                  [--dir 'C:\\...\\'] [--run SECS] [--play]

The copy is the stage-1 runnable image (work/hybrid/mb.base.exe, tools/hybrid/build.py) plus a section .capt
holding tools/hybrid/capture.c, whose entry point plants int3 at every target and then starts the game. On the
first --max-hits calls of each target it writes cap_<seq>_<va>.bin (registers + the writable pages that changed
since the previous snapshot; capture.c has the format) into the game folder (or --dir). The original exe is
never touched: the copy is written as mb_capture.exe next to it (as tools/hybrid/play.sh does) and the snapshots
are moved to work/capture/ afterwards. tools/difftest/snapreplay.py replays them through the original function
and ours.

--targets nonmatching (default): every row of every src/ module's *.nonmatching.txt (complete but not byte-exact:
the functions whose behaviour can differ). --run SECS starts it under CrossOver (--play presses Play at the
launcher), waits, ends it, and collects the snapshots.
"""
import argparse, glob, os, re, shutil, struct, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
sys.path.insert(0, os.path.join(ROOT, "tools", "relink"))
BASE_EXE = os.path.join(ROOT, "work", "hybrid", "mb.base.exe")
IMAGE_BASE = 0x400000
OEP = 0x005390BA
GAME_WIN = r"C:\Program Files (x86)\Steam\steamapps\common\Mount and Blade" + "\\"
COPY = "mb_capture.exe"


def nonmatching_vas():
    vas = set()
    for f in glob.glob(os.path.join(ROOT, "src", "**", "*.nonmatching.txt"), recursive=True):
        for line in open(f, errors="replace"):
            p = line.split()
            if p and re.fullmatch(r"[0-9a-fA-F]{8}", p[0]):
                vas.add(int(p[0], 16))
    return vas


def link(coff, base_va):
    """Lay the object's sections out from base_va and apply its relocations: -> (bytes, {symbol: va})."""
    keep = [i for i, s in enumerate(coff.sections)
            if not s.name.startswith((".debug", ".drectve")) and (s.data or s.size)]
    at, off = {}, 0
    for i in keep:
        off = (off + 15) & ~15
        at[i] = off
        off += max(len(coff.sections[i].data or b""), coff.sections[i].size)
    blob = bytearray(off)
    for i in keep:
        d = coff.sections[i].data
        if d:
            blob[at[i]:at[i] + len(d)] = d

    def sym_va(s):
        if s.secno <= 0 or s.secno - 1 not in at:
            raise SystemExit("capture.c: unresolved symbol %s" % s.name)
        return base_va + at[s.secno - 1] + s.value

    for i in keep:
        for roff, si, typ in coff.sections[i].relocs:
            p = at[i] + roff
            a = struct.unpack_from("<I", blob, p)[0]
            s = sym_va(coff.syms[si])
            if typ == 6:                                     # IMAGE_REL_I386_DIR32
                v = s + a
            elif typ == 7:                                   # DIR32NB
                v = s + a - IMAGE_BASE
            elif typ == 0x14:                                # REL32
                v = s + a - (base_va + p + 4)
            else:
                raise SystemExit("capture.c: relocation type %#x" % typ)
            struct.pack_into("<I", blob, p, v & 0xFFFFFFFF)
    syms = {s.name: sym_va(s) for s in coff.syms.values() if s.defined and s.cls in (2, 3) and s.secno - 1 in at}
    return blob, syms


def build(targets, max_hits, out, dirpath, arm=0):
    import pefile
    import resolve as R
    import slice as S
    from coff import Coff
    import importlib.util
    spec = importlib.util.spec_from_file_location("hybrid_build", os.path.join(HERE, "build.py"))
    B = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(B)                       # (tools/relink has a build.py too)
    img = R.Image()
    t0, tn = img.section(".text")
    targets = sorted(v for v in targets if t0 <= v < t0 + tn and v in img.start_set)
    obj, log = S.compile_obj(os.path.join(HERE, "capture.c"), ["/O1", "/Zl", "/Oi-", "/W3", "/TC"])
    if obj is None:
        raise SystemExit("capture.c failed to compile:\n" + log)
    coff = Coff(obj)
    pe = pefile.PE(BASE_EXE)
    pe.__data__ = bytearray(pe.__data__)
    last = pe.sections[-1]
    sa = pe.OPTIONAL_HEADER.SectionAlignment
    rva = B.align(last.VirtualAddress + max(last.Misc_VirtualSize, last.SizeOfRawData), sa)
    base_va = IMAGE_BASE + rva
    blob, syms = link(coff, base_va)
    # the tables after the runtime: bps[n], orig[n], hits[n]
    blob = bytearray(blob)
    blob += b"\0" * (-len(blob) % 16)
    off_bps = len(blob)
    blob += struct.pack("<%dI" % len(targets), *targets)
    off_orig = len(blob)
    blob += bytes(img.read(v, 1)[0] for v in targets)
    off_hits = len(blob)
    blob += b"\0" * len(targets)
    iat = {v: k for k, v in img.imports.items()}
    cfg = syms["_cap_cfg"] - base_va
    assert struct.unpack_from("<I", blob, cfg)[0] == 0x4643424d, "cap_cfg not found"
    d = dirpath.encode("latin-1")
    struct.pack_into("<11I", blob, cfg, 0x4643424d, OEP, iat["kernel32!GetProcAddress"], iat["kernel32!GetModuleHandleA"],
                     len(targets), base_va + off_bps, base_va + off_orig, base_va + off_hits, max_hits,
                     t0, t0 + tn)
    blob[cfg + 44:cfg + 44 + 128] = d.ljust(128, b"\0")[:128]
    struct.pack_into("<2I", blob, cfg + 44 + 128, arm, img.read(arm, 1)[0] if arm else 0)
    B.add_section(pe, ".capt", rva, bytes(blob), 0xE0000020)      # code | execute | read | write
    for s in pe.sections:
        if s.Name.rstrip(b"\0") == b".text":
            s.Characteristics |= 0x80000000                         # writable: the int3s are planted at run time
    pe.OPTIONAL_HEADER.AddressOfEntryPoint = syms["_cap_entry"] - IMAGE_BASE
    pe.OPTIONAL_HEADER.CheckSum = pe.generate_checksum()
    pe.write(out)
    print("%s: %d targets, runtime at %08x (%d bytes), entry %08x, max %d hits each"
          % (os.path.relpath(out, ROOT), len(targets), base_va, len(blob), syms["_cap_entry"], max_hits))
    return targets


def run(out, secs, play):
    """Run the copy from the game folder, press Play, wait, end it, move the snapshots to work/capture/."""
    import crashrun as C
    game = os.path.join(ROOT, "game")
    for f in glob.glob(os.path.join(game, "cap_*.bin")):
        os.remove(f)
    shutil.copyfile(out, os.path.join(game, COPY))
    p = subprocess.Popen([C.CX, "--bottle", "Steam", "--cx-app", GAME_WIN + COPY], cwd=game,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=dict(os.environ, WINEDEBUG="-all"))
    if play:
        C.WINPATH = GAME_WIN + COPY
        press(secs)
    time.sleep(secs)
    for line in subprocess.run(["ps", "-axo", "pid=,command="], capture_output=True, text=True).stdout.splitlines():
        pid, cmd = line.strip().split(None, 1)
        if cmd.startswith(GAME_WIN + COPY):
            os.kill(int(pid), 9)
    p.wait(30)
    dst = os.path.join(ROOT, "work", "capture")
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    n = size = 0
    for f in sorted(glob.glob(os.path.join(game, "cap_*.bin"))):
        size += os.path.getsize(f)
        shutil.move(f, dst)
        n += 1
    print("%d snapshots, %.1f MB -> work/capture/" % (n, size / 1e6))


def press(limit):
    """Press Return at the launcher until the game's log shows loading (crashrun.press_play, by process name)."""
    proc = '(first process whose name contains "mb_capture")'
    log = os.path.join(ROOT, "game", "rgl_log.txt")
    t0 = time.time()
    while time.time() - t0 < limit:
        try:
            if os.path.getmtime(log) >= t0 - 1 and os.path.getsize(log) > 200:
                return
        except OSError:
            pass
        r = subprocess.run(["osascript", "-e", 'tell application "System Events" to get name of window 1 of ' + proc],
                           capture_output=True, text=True)
        if r.returncode == 0:
            time.sleep(3)
            subprocess.run(["osascript", "-e", 'tell application "System Events" to set frontmost of %s to true' % proc,
                            "-e", "delay 0.5", "-e",
                            'tell application "System Events" to if (name of (first process whose frontmost is true)) '
                            'contains "mb_capture" then key code 36'], capture_output=True)
        time.sleep(3)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--targets", default="nonmatching")
    ap.add_argument("--max-hits", type=int, default=2)
    ap.add_argument("--out", default=os.path.join(ROOT, "work", "hybrid", "mb.capture.exe"))
    ap.add_argument("--dir", default="")
    ap.add_argument("--run", type=int, metavar="SECS")
    ap.add_argument("--play", action="store_true")
    ap.add_argument("--arm", help="VA: plant the targets only when this function is first called (e.g. 4dcef0, "
                                  "MainMenuScreen::create: skip the loading screens)")
    a = ap.parse_args()
    if a.targets == "nonmatching":
        targets = nonmatching_vas()
    elif os.path.exists(a.targets):
        targets = {int(l.split()[0], 16) for l in open(a.targets) if l.strip() and not l.startswith("#")}
    else:
        targets = {int(v, 16) for v in a.targets.split(",")}
    build(targets, a.max_hits, a.out, a.dir, int(a.arm, 16) if a.arm else 0)
    if a.run:
        run(a.out, a.run, a.play)
    return 0


if __name__ == "__main__":
    sys.exit(main())
