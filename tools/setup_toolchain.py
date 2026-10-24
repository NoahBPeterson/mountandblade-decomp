#!/usr/bin/env python3
"""Set up the compiler toolchain from installers you supply. Nothing is downloaded; every input and output is
checked against known hashes, so a toolchain built here is byte-identical to the one the project matches with.

usage: setup_toolchain.py vctoolkit <VCToolkitSetup.exe>   # cl 13.10.3052 -> work/toolchain/vc71tk/{bin,include,lib}
       setup_toolchain.py winsdk <VCForPython27.msi>        # Windows SDK 6.0A + sal.h -> work/toolchain/{vc9sp1,sal}
       setup_toolchain.py dxsdk <DXSDK_Jun10.exe>           # DirectX SDK June 2010 headers/libs -> work/ext/dxsdk_jun10
       setup_toolchain.py wine                              # wine prefix -> work/toolchain/wineprefix-mb
       setup_toolchain.py check                             # compile a module and verify it byte-exact

Needs: msiextract (Homebrew/apt: msitools), 7z (p7zip), wine, and the project venv (uv sync). Existing outputs are
never overwritten: delete a directory to redo its step.

VCToolkitSetup.exe is an InstallShield "streamed" setup. Its payload is a list of files, each stored with
InstallShield's byte encoding (flag 2): swap the nibbles of each byte, XOR it with a key made from the file's name
(XOR 13 35 86 07, repeated), invert. The decoded "Microsoft Visual C++ Toolkit 2003.msi" is then an ordinary MSI.
The installer is never run.
"""
import hashlib, os, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TC = os.path.join(ROOT, "work", "toolchain")

INPUT_SHA1 = {
    "vctoolkit": "956c81c3106b97042c4126b23c81885c4b5211f4",   # VCToolkitSetup.exe (Microsoft, 2004)
    "winsdk": "7800d037ba962f288f9b952001106d35ef57befe",      # VCForPython27.msi (Microsoft, 2014)
}
OUTPUT_SHA256 = {
    "vc71tk/bin/cl.exe": "9e45887874de83876ada2adc9943daa9094d1c1f1f9c3d93eac5ad57a1cbee2a",
    "vc71tk/bin/c1xx.dll": "08eb5a13d22974adb311c9f03e3c9e60a08d8e48f4ba193fa65cf157ded00640",
    "vc71tk/bin/c2.dll": "d440f00231efe276cf001cf6257b4568469c72c3033eb28d5323e6ac91f19972",
}
DX_SHA256 = {
    "Include/d3dx9.h": "7b20fea554f2aaef744d17e31ddf303b7c6b4bd4c8d279bad8cb2998781cab79",
    "Include/d3dx9math.h": "27ac937a3eb64f9ed917a4265ac4e1f6817a94bd15ca7a5cc1eb63c2cb9ea1d9",
    "Include/d3dx9core.h": "7e360611940e9520c9dd983dbe9e48a78df8b723d95de1a57a63580de52a0207",
    "Lib/x86/d3dx9.lib": "0b1cd009d5e5ed15080c4d7ad0245a273242da07e0747921e80ac3ccf9bc0fc2",
}


def die(msg):
    sys.exit("error: " + msg)


def digest(path, algo):
    h = hashlib.new(algo)
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def need(tool):
    if not shutil.which(tool):
        die("%s not found on PATH (install msitools / p7zip / wine)" % tool)


def check_input(kind, path):
    if not os.path.isfile(path):
        die("no such file: %s" % path)
    got = digest(path, "sha1")
    if got != INPUT_SHA1[kind]:
        die("%s has SHA-1 %s, expected %s: a different build of the installer" % (path, got, INPUT_SHA1[kind]))


def fresh(dst):
    if os.path.lexists(dst):
        die("%s already exists; delete it to redo this step" % os.path.relpath(dst, ROOT))


def installshield_files(blob):
    """(name, decoded bytes) for every file of an InstallShield streamed payload."""
    if not blob.startswith(b"InstallShield\0"):
        die("not an InstallShield stream")
    count = struct.unpack_from("<H", blob, 14)[0]
    off = 0x2e
    for _ in range(count):
        name = blob[off:off + 260].split(b"\0")[0]
        flags, _unk, size = struct.unpack_from("<III", blob, off + 260)
        data = blob[off + 0x138:off + 0x138 + size]
        off += 0x138 + size
        if flags & 2:
            key = bytes(c ^ (0x13, 0x35, 0x86, 0x07)[i % 4] for i, c in enumerate(name))
            swap = bytes(((b << 4) | (b >> 4)) & 0xFF for b in range(256))
            data = data.translate(swap)
            n = len(data)
            stream = (key * (n // len(key) + 1))[:n]
            data = (int.from_bytes(data, "little") ^ int.from_bytes(stream, "little")) ^ ((1 << (8 * n)) - 1)
            data = data.to_bytes(n, "little")
        yield name.decode("latin-1"), data


def vctoolkit(setup_exe):
    check_input("vctoolkit", setup_exe)
    need("msiextract"), need("7z")
    dst = os.path.join(TC, "vc71tk")
    fresh(dst)
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["7z", "x", "-y", "-o" + tmp, setup_exe, "[0]"], check=True, stdout=subprocess.DEVNULL)
        blob = open(os.path.join(tmp, "[0]"), "rb").read()
        msi = None
        for name, data in installshield_files(blob):
            if name.lower().endswith("toolkit 2003.msi"):
                msi = os.path.join(tmp, "toolkit.msi")
                open(msi, "wb").write(data)
        if not msi:
            die("no Toolkit MSI in the installer payload")
        out = os.path.join(tmp, "msi")
        os.makedirs(out)
        subprocess.run(["msiextract", "-C", out, msi], check=True, stdout=subprocess.DEVNULL)
        src = os.path.join(out, "Program Files", "Microsoft Visual C++ Toolkit 2003")
        os.makedirs(dst)
        for sub in ("bin", "include", "lib"):
            shutil.copytree(os.path.join(src, sub), os.path.join(dst, sub))
    verify(OUTPUT_SHA256, TC)
    print("cl 13.10.3052 in", os.path.relpath(dst, ROOT))


def winsdk(msi):
    check_input("winsdk", msi)
    need("msiextract")
    dst, sal = os.path.join(TC, "vc9sp1"), os.path.join(TC, "sal")
    fresh(dst), fresh(sal)
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run(["msiextract", "-C", tmp, msi], check=True, stdout=subprocess.DEVNULL)
        src = os.path.join(tmp, "Microsoft", "Visual C++ for Python", "9.0")
        shutil.copytree(src, dst)
    # SDK 6.0A's specstrings.h includes <sal.h>, which VC7.1 lacks; cl.sh puts only this one VC9 header after VC7.1's
    os.makedirs(sal)
    shutil.copy2(os.path.join(dst, "VC", "include", "sal.h"), sal)
    print("Windows SDK 6.0A in", os.path.relpath(dst, ROOT), "and sal.h in", os.path.relpath(sal, ROOT))


def dxsdk(setup_exe):
    need("7z")
    dst = os.path.join(ROOT, "work", "ext", "dxsdk_jun10")
    fresh(dst)
    os.makedirs(dst)
    subprocess.run(["7z", "x", "-y", "-o" + dst, setup_exe, "DXSDK/Include/*", "DXSDK/Lib/x86/*"], check=True,
                   stdout=subprocess.DEVNULL)
    verify(DX_SHA256, os.path.join(dst, "DXSDK"))
    print("DirectX SDK June 2010 headers and x86 libs in", os.path.relpath(dst, ROOT))


def wine():
    need("wine")
    dst = os.path.join(TC, "wineprefix-mb")
    fresh(dst)
    subprocess.run(["wineboot", "-i"], env=dict(os.environ, WINEPREFIX=dst, WINEDEBUG="-all"), check=True)
    print("wine prefix in", os.path.relpath(dst, ROOT))


def verify(table, base):
    bad = []
    for rel, want in table.items():
        p = os.path.join(base, rel)
        got = digest(p, "sha256") if os.path.isfile(p) else "missing"
        if got != want:
            bad.append("%s: %s" % (rel, got))
    if bad:
        die("output does not match the project's toolchain:\n  " + "\n  ".join(bad))


def check():
    py = os.path.join(ROOT, ".venv", "bin", "python")
    man = os.path.join(ROOT, "src", "game", "strat", "tstring.manifest.txt")
    r = subprocess.run([py, os.path.join(ROOT, "tools", "matching", "run_all.py"), "--manifest", man],
                       capture_output=True, text=True)
    last = (r.stdout.strip().splitlines() or ["(no output)"])[-1]
    print(last)
    if not last.startswith("33 / 33"):
        die("expected 33 / 33 byte-exact for tstring; see the output above and tools/matching/cl.sh")
    print("toolchain OK")


def main(argv):
    if len(argv) >= 2 and argv[0] in ("vctoolkit", "winsdk", "dxsdk") :
        {"vctoolkit": vctoolkit, "winsdk": winsdk, "dxsdk": dxsdk}[argv[0]](os.path.abspath(argv[1]))
    elif argv[:1] == ["wine"]:
        wine()
    elif argv[:1] == ["check"]:
        check()
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main(sys.argv[1:])
