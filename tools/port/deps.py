#!/usr/bin/env python3
"""deps.py: the port's third-party dependencies, built static per target into work/port_deps/<target>/install.

The macos-arm64 lane takes SDL3 / SDL3_ttf / glslang / SPIRV-Cross from brew; the cross targets cannot (brew is
arm64-only), so this builds the same upstream releases from source with the target's toolchain. Jolt is fetched
only: the port build compiles it per target (port/physics, MB_JOLT_SOURCE_DIR points at the checkout here).

usage: .venv/bin/python tools/port/deps.py --target macos-x86_64 [--native] [--only sdl3,glslang] [--fetch-only] [--force]
  --native    no toolchain file: build with the host compiler (an amd64 Linux machine building for itself)
  --only      comma list of components (default: all, in dependency order)
  --force     rebuild even if the component's install stamp is current

Sources are downloaded once into work/port_deps/src/ (shared by every target); builds and installs are per target.
A component is skipped when its install stamp matches its tag and options. Logs: work/port_deps/<target>/<name>.log.
"""
import argparse, os, subprocess, sys, tarfile, urllib.request

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SRC = os.path.join(ROOT, 'work', 'port_deps', 'src')

# name -> (github repo, tag). Pinned to what brew ships for the macos-arm64 lane (2026-11-22): sdl3 3.4.18,
# sdl3_ttf 3.2.2, glslang 16.6.0 / spirv-cross 1.4.363.0 (both the vulkan-sdk-1.4.363.0 tag), Jolt v5.5.0.
COMPONENTS = {
    'zlib':          ('madler/zlib', 'v1.3.1'),
    'SDL3':          ('libsdl-org/SDL', 'release-3.4.18'),
    'SDL3_ttf':      ('libsdl-org/SDL_ttf', 'release-3.2.2'),
    'glslang':       ('KhronosGroup/glslang', 'vulkan-sdk-1.4.363.0'),
    'SPIRV-Cross':   ('KhronosGroup/SPIRV-Cross', 'vulkan-sdk-1.4.363.0'),
    'JoltPhysics':   ('jrouwe/JoltPhysics', 'v5.5.0'),
}
ORDER = ['zlib', 'SDL3', 'SDL3_ttf', 'glslang', 'SPIRV-Cross', 'JoltPhysics']
FETCH_ONLY = {'JoltPhysics'}          # compiled by the port build itself, per target
# glslang needs no SPIRV-Headers checkout: ENABLE_OPT=OFF and it bundles SPIRV/spirv.hpp11.


def log(msg):
    print(msg, flush=True)


def fetch(name):
    repo, tag = COMPONENTS[name]
    d = os.path.join(SRC, '%s-%s' % (name, tag))
    stamp = os.path.join(d, '.mb_fetched')
    if os.path.exists(stamp):
        return d
    os.makedirs(SRC, exist_ok=True)
    url = 'https://codeload.github.com/%s/tar.gz/refs/tags/%s' % (repo, tag)
    tgz = os.path.join(SRC, '%s-%s.tar.gz' % (name, tag))
    log('fetch %s %s' % (name, tag))
    urllib.request.urlretrieve(url, tgz)
    with tarfile.open(tgz) as tf:
        members = tf.getmembers()
        top = members[0].name.split('/')[0]
        for m in members:                                   # strip the one top-level directory
            m.name = m.name[len(top) + 1:] if '/' in m.name else ''
        tf.extractall(d, filter='data')
    os.remove(tgz)
    if name == 'SDL3_ttf':                                  # vendored freetype + harfbuzz: pinned downloads
        subprocess.run(['bash', 'external/download.sh'], cwd=d, check=True)
    open(stamp, 'w').write(tag + '\n')
    return d


def cmake_args(name, prefix, toolchain, target):
    a = ['-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_SHARED_LIBS=OFF',
         '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
         '-DCMAKE_INSTALL_PREFIX=' + prefix, '-DCMAKE_PREFIX_PATH=' + prefix]
    if toolchain:
        a.append('-DCMAKE_TOOLCHAIN_FILE=' + toolchain)
    if target.startswith('macos-'):
        # As a CACHE arg, not just the toolchain's normal variable: projects with cmake_minimum_required < 3.21
        # (zlib, SPIRV-Cross) ignore the toolchain variable under CMake >= 3.21 policies and build host-arch.
        a.append('-DCMAKE_OSX_ARCHITECTURES=' + target.split('-', 1)[1])
    if name == 'SDL3':
        a += ['-DSDL_TESTS=OFF', '-DSDL_INSTALL_TESTS=OFF']
    elif name == 'SDL3_ttf':
        a += ['-DSDLTTF_VENDORED=ON', '-DSDLTTF_SAMPLES=OFF']
    elif name == 'glslang':
        a += ['-DENABLE_OPT=OFF', '-DGLSLANG_TESTS=OFF', '-DENABLE_GLSLANG_BINARIES=OFF']
    elif name == 'SPIRV-Cross':
        a += ['-DSPIRV_CROSS_STATIC=ON', '-DSPIRV_CROSS_SHARED=OFF', '-DSPIRV_CROSS_CLI=OFF',
              '-DSPIRV_CROSS_ENABLE_TESTS=OFF']
    elif name == 'zlib':
        a += ['-DZLIB_BUILD_EXAMPLES=OFF']
    return a


def build(name, target, native, force, jobs):
    src = fetch(name)
    if name in FETCH_ONLY:
        log('%s: source ready at %s (the port build compiles it)' % (name, src))
        return True
    base = os.path.join(ROOT, 'work', 'port_deps', target)
    prefix = os.path.join(base, 'install')
    bdir = os.path.join(base, 'build', name)
    logfile = os.path.join(base, name + '.log')
    toolchain = None if native else os.path.join(ROOT, 'port', 'cmake', target + '.cmake')
    args = cmake_args(name, prefix, toolchain, target)
    stamp = os.path.join(prefix, '.mb_' + name)
    want = '%s %s\n' % (COMPONENTS[name][1], ' '.join(args))
    if not force and os.path.exists(stamp) and open(stamp).read() == want:
        log('%s: up to date' % name)
        return True
    os.makedirs(bdir, exist_ok=True)
    os.makedirs(prefix, exist_ok=True)
    log('build %s -> %s' % (name, prefix))
    with open(logfile, 'w') as lf:
        for cmd in [['cmake', '-S', src, '-B', bdir] + args,
                    ['cmake', '--build', bdir, '--parallel', str(jobs)],
                    ['cmake', '--install', bdir]]:
            p = subprocess.run(cmd, stdout=lf, stderr=subprocess.STDOUT)
            if p.returncode != 0:
                log('%s FAILED (%s); tail of %s:' % (name, cmd[0], logfile))
                with open(logfile) as f:
                    print(''.join(f.readlines()[-30:]))
                return False
    open(stamp, 'w').write(want)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--target', required=True, help='macos-x86_64 | linux-x86_64 | windows-x86_64 (toolchain file name)')
    ap.add_argument('--native', action='store_true', help='host compiler, no toolchain file (build inside the target machine)')
    ap.add_argument('--only', default='', help='comma list of components')
    ap.add_argument('--fetch-only', action='store_true')
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    names = [n.strip() for n in a.only.split(',') if n.strip()] or ORDER
    for n in names:
        if n not in COMPONENTS:
            sys.exit('unknown component %r (have: %s)' % (n, ', '.join(ORDER)))
    for n in names:
        if a.fetch_only:
            fetch(n)
        elif not build(n, a.target, a.native, a.force, a.jobs):
            sys.exit(1)
    log('done')


if __name__ == '__main__':
    main()