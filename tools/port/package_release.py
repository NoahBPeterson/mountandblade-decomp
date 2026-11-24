#!/usr/bin/env python3
"""package_release.py: turn a built port target into a release archive (the GitHub release's per-target asset).

usage: python3 tools/port/package_release.py --target linux-x86_64 --version v0.1.0 [--build build/port/<target>] [--out dist]

Runs `cmake --install` into dist/mountandblade-<version>-<target>/ (Unix: bin/mountandblade launcher +
libexec/mountandblade/mb; Windows: bin/mb.exe + mountandblade.bat), adds LICENSE, THIRD_PARTY_LICENSES.txt (collected from
the dependency sources in work/port_deps/src and port/audio/third_party) and a short README.txt, ad-hoc signs the macOS
binary, and writes <name>.tar.gz (Windows: .zip) plus its sha256 line to <out>/SHA256SUMS.<target>. No game files.
"""
import argparse, glob, hashlib, os, shutil, subprocess, sys, tarfile, zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SRC = os.path.join(ROOT, 'work', 'port_deps', 'src')

# component -> license files, relative to its source dir (work/port_deps/src/<glob>)
LICENSES = [
    ('SDL3 (zlib licence)',                         'SDL3-*',        ['LICENSE.txt']),
    ('SDL3_ttf (zlib licence)',                     'SDL3_ttf-*',    ['LICENSE.txt']),
    ('FreeType (used under the FreeType Licence)',  'SDL3_ttf-*',    ['external/freetype/docs/FTL.TXT']),
    ('HarfBuzz',                                    'SDL3_ttf-*',    ['external/harfbuzz/COPYING']),
    ('PlutoSVG',                                    'SDL3_ttf-*',    ['external/plutosvg/LICENSE']),
    ('PlutoVG',                                     'SDL3_ttf-*',    ['external/plutovg/LICENSE']),
    ('glslang',                                     'glslang-*',     ['LICENSE.txt']),
    ('SPIRV-Cross (used under the MIT option of Apache-2.0 OR MIT)', 'SPIRV-Cross-*', ['LICENSES/MIT.txt']),
    ('zlib',                                        'zlib-*',        ['LICENSE']),
    ('Jolt Physics',                                'JoltPhysics-*', ['LICENSE']),
]
OWN = [('stb_vorbis (Sean Barrett; public domain / MIT)', os.path.join(ROOT, 'port', 'audio', 'third_party', 'LICENSE.stb'))]


def third_party():
    out = ['Third-party components linked into this binary (sources are fetched by tools/port/deps.py at the versions\n'
           'pinned there). Each licence text follows.\n']
    missing = []
    for name, d, files in LICENSES:
        hits = glob.glob(os.path.join(SRC, d))
        if not hits:
            missing.append(name)
            continue
        for f in files:
            p = os.path.join(hits[0], f)
            if not os.path.exists(p):
                missing.append('%s: %s' % (name, f))
                continue
            out.append('\n' + '=' * 100 + '\n%s  [%s/%s]\n' % (name, os.path.basename(hits[0]), f) + '=' * 100 + '\n')
            out.append(open(p, encoding='utf-8', errors='replace').read())
    for name, p in OWN:
        out.append('\n' + '=' * 100 + '\n%s\n' % name + '=' * 100 + '\n')
        out.append(open(p, encoding='utf-8', errors='replace').read())
    if missing:
        sys.exit('package_release.py: licence files missing (run tools/port/deps.py first): ' + ', '.join(missing))
    return ''.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--target', required=True)
    ap.add_argument('--version', required=True)
    ap.add_argument('--build')
    ap.add_argument('--out', default=os.path.join(ROOT, 'dist'))
    a = ap.parse_args()
    build = os.path.abspath(a.build or os.path.join(ROOT, 'build', 'port', a.target))
    win = a.target.startswith('windows-')
    mac = a.target.startswith('macos-')
    name = 'mountandblade-%s-%s' % (a.version, a.target)
    out = os.path.abspath(a.out)
    stage = os.path.join(out, name)
    shutil.rmtree(stage, ignore_errors=True)
    os.makedirs(out, exist_ok=True)
    subprocess.run(['cmake', '--install', build, '--prefix', stage], check=True, stdout=subprocess.DEVNULL)
    if mac:
        exe = os.path.join(stage, 'libexec', 'mountandblade', 'mb')
        subprocess.run(['codesign', '--force', '--sign', '-', exe], check=True)   # arm64 macOS refuses unsigned code
    shutil.copy(os.path.join(ROOT, 'LICENSE'), stage)
    open(os.path.join(stage, 'THIRD_PARTY_LICENSES.txt'), 'w').write(third_party())
    notes = ''
    if mac:
        notes = ('\nmacOS (the binary is ad-hoc signed, not notarized): after downloading, remove the quarantine flag once:\n'
                 '  xattr -dr com.apple.quarantine .\n')
        run = 'MB_GAME_DIR="/path/to/Mount and Blade" ./bin/mountandblade'
    elif win:
        run = 'set MB_GAME_DIR=C:\\path\\to\\Mount and Blade   then   bin\\mountandblade.bat\n  (or copy bin\\mb.exe into your game folder and run it there). Also runs under Wine.'
    else:
        notes = '\nNeeds a Vulkan driver (libvulkan1 + mesa-vulkan-drivers or your GPU vendor\'s) and glibc 2.38 or newer (Ubuntu 24.04+).\nOn older distributions, build from source (docs/BUILDING_PORT.md).\n'
        run = 'MB_GAME_DIR="/path/to/Mount and Blade" ./bin/mountandblade'
    t = open(os.path.join(ROOT, 'port', 'install', 'README.release.txt')).read()
    t = t.replace('@TARGET@', a.target).replace('@VERSION@', a.version).replace('@RUN@', run).replace('@NOTES@', notes)
    open(os.path.join(stage, 'README.txt'), 'w').write(t)

    if win:
        arc = os.path.join(out, name + '.zip')
        with zipfile.ZipFile(arc, 'w', zipfile.ZIP_DEFLATED) as z:
            for dp, _, fs in os.walk(stage):
                for f in sorted(fs):
                    p = os.path.join(dp, f)
                    z.write(p, os.path.join(name, os.path.relpath(p, stage)))
    else:
        arc = os.path.join(out, name + '.tar.gz')
        with tarfile.open(arc, 'w:gz') as tf:
            tf.add(stage, arcname=name)
    h = hashlib.sha256(open(arc, 'rb').read()).hexdigest()
    open(os.path.join(out, 'SHA256SUMS.' + a.target), 'w').write('%s  %s\n' % (h, os.path.basename(arc)))
    print('%s  %d bytes  sha256 %s' % (arc, os.path.getsize(arc), h[:16]))


if __name__ == '__main__':
    main()
