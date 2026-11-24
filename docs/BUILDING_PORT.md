# Building the native port from source

The native port builds `mb` (Linux x86_64, Linux arm64, macOS arm64/x86_64, Windows x86_64) from this repository and
runs it on **your own copy of Mount & Blade 1.011** (Steam). No game files are in this repository or in the install.

You need: the game's folder (the one containing `Data/`, `Modules/`, `CommonRes/`, `mb.fx` ...), a C++ toolchain,
CMake, Ninja, Python 3, and an hour of patience for the first build (about 580 translation units).

## 1. Packages

### Ubuntu / Debian (tested on Ubuntu 24.04, x86_64 and arm64)

```sh
sudo apt install clang lld llvm cmake ninja-build pkg-config git python3 \
  libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev libxfixes-dev libxss-dev libxtst-dev libxrender-dev \
  libxkbcommon-dev libwayland-dev libasound2-dev libpulse-dev libdbus-1-dev libudev-dev libegl-dev libgl1-mesa-dev
```

`llvm` provides `llvm-objcopy`, `llvm-nm`, `llvm-cxxfilt` and `llvm-undname`, which the build uses. clang 18 or newer.
Runtime (to play): `libvulkan1` and a Vulkan driver (`mesa-vulkan-drivers`, or your GPU vendor's).

### Fedora

```sh
sudo dnf install clang lld llvm cmake ninja-build pkgconf git python3 \
  libX11-devel libXext-devel libXcursor-devel libXi-devel libXrandr-devel libXfixes-devel libXScrnSaver-devel \
  libXtst-devel libXrender-devel libxkbcommon-devel wayland-devel alsa-lib-devel pulseaudio-libs-devel \
  dbus-devel systemd-devel mesa-libEGL-devel mesa-libGL-devel
```

Runtime: `vulkan-loader` and `mesa-vulkan-drivers`.

### Arch

```sh
sudo pacman -S clang lld llvm cmake ninja pkgconf git python \
  libx11 libxext libxcursor libxi libxrandr libxfixes libxss libxtst libxrender libxkbcommon wayland alsa-lib \
  libpulse dbus systemd-libs libglvnd mesa
```

Runtime: `vulkan-icd-loader` and a Vulkan driver (`vulkan-radeon`, `vulkan-intel`, `nvidia-utils`).

### macOS (Apple silicon)

```sh
xcode-select --install
brew install cmake ninja llvm sdl3 sdl3_ttf glslang spirv-cross
```

The macOS arm64 lane takes SDL3, SDL3_ttf, glslang and SPIRV-Cross from Homebrew, so step 2 is not needed there.
The Jolt physics library is downloaded by CMake at configure time (needs `git`).

### Windows x86_64

This lane is cross-built from macOS with zig (`brew install zig`) and runs under Wine. See
`port/cmake/windows-x86_64.cmake` and `port/INTEGRATION_STATUS.md`.

## 2. Dependencies (Linux; macOS x86_64)

SDL3, SDL3_ttf, glslang, SPIRV-Cross and zlib are not packaged by most distributions at the versions the port is pinned
to, so `tools/port/deps.py` builds them statically into `work/port_deps/<target>/install`. It only uses the Python
standard library and downloads the sources once.

```sh
python3 tools/port/deps.py --target linux-x86_64 --native     # or linux-arm64 --native
```

## 3. Configure and build

Pick `<target>` = `linux-x86_64` or `linux-arm64`:

```sh
T=linux-x86_64
cmake -S port -B build/port/$T -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/$T-native.cmake \
  -DCMAKE_PREFIX_PATH=$PWD/work/port_deps/$T/install \
  -DMB_JOLT_SOURCE_DIR=$PWD/work/port_deps/src/JoltPhysics-v5.5.0 \
  -DZLIB_ROOT=$PWD/work/port_deps/$T/install -DZLIB_USE_STATIC_LIBS=ON
ninja -C build/port/$T
ctest --test-dir build/port/$T          # 5 tests, a few seconds
```

macOS arm64 (Homebrew dylibs, for development): `cmake -S port -B build/port/macos-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/macos-arm64.cmake`,
then `ninja -C build/port/macos-arm64`.

macOS arm64 or x86_64, portable (static dependencies, macOS 12+; this is what the release ships): run
`python3 tools/port/deps.py --target macos-arm64`, then configure as above with `T=macos-arm64` but
`-DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/$T.cmake` (not `$T-native.cmake`) and the same `CMAKE_PREFIX_PATH`, `MB_JOLT_SOURCE_DIR`
and `ZLIB_*` options.

The link tables the build needs (`port/link/<target>/`) are committed. They only need regenerating when `src/` changes
(`tools/port/link_tables.py --target <target>`; see `port/INTEGRATION_STATUS.md`).

## 4. Install ("make install")

```sh
cmake --install build/port/$T --prefix ~/.local        # or: sudo cmake --install ... --prefix /usr/local
```

This installs exactly two files:

- `<prefix>/libexec/mountandblade/mb`, the executable;
- `<prefix>/bin/mountandblade`, a launcher that changes into your game folder and starts `mb`.

(`DESTDIR` is honoured, for packaging. On Windows, `mb.exe` and `mountandblade.bat` go to `<prefix>/bin`.)

Release archives are made by `tools/port/package_release.py`; see [RELEASING.md](RELEASING.md).

## 5. Run

```sh
MB_GAME_DIR="/path/to/Mount and Blade" mountandblade
```

Without `MB_GAME_DIR` the launcher looks in the usual Steam library locations
(`~/.local/share/Steam`, `~/.steam/steam`, the Flatpak Steam directory, `~/Library/Application Support/Steam`) for
`steamapps/common/Mount and Blade`. If it cannot find the game it says so and exits.

Get the game files from Steam (version 1.011); on Linux, Steam's Proton download of the Windows build is fine, since
the port only reads the data files. Nothing in the game folder is modified apart from what the game itself writes.

## Where the game writes its files

| what | where |
|---|---|
| saves, screenshots, settings (`Documents`) | `~/Documents/Mount&Blade/` |
| app data (the registry store `Mount&Blade/registry.ini`) | `$XDG_DATA_HOME` or `~/.local/share` (macOS: `~/Library/Application Support`) |

`MB_USER_DIR=/some/dir` redirects both (the first under `$MB_USER_DIR/Documents`), for a portable install or a test sandbox.

## Troubleshooting

- *Black window / crash at start*: run `vulkaninfo --summary`. SDL_GPU needs a working Vulkan device on Linux. In a VM
  or without a GPU, `mesa-vulkan-drivers` provides the software renderer (lavapipe).
- *`cannot find your Mount & Blade 1.011 folder`*: set `MB_GAME_DIR`.
- *Quick check without clicking*: `MB_START=quick_battle mountandblade` starts a custom battle directly;
  stderr prints `MBSTATE ... mission_frame N` lines as the mission renders.
- Headless check (CI): `xvfb-run -a -s '-screen 0 1280x800x24' python3 tools/port/run_native.py --target $T --start quick_battle --wait 'mission_frame 30' --do shot a --do end`
  (needs `xvfb`, `mesa-vulkan-drivers`, `imagemagick`).
