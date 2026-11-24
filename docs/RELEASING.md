# Releasing

Binaries are built by GitHub Actions (`.github/workflows/release.yml`) and published as a **pre-release**. They contain no
game files; users need their own Mount & Blade 1.011.

## Cut a release

```sh
git tag v0.1.0 && git push origin v0.1.0
```

A `v*` tag push builds five archives in parallel and, when all succeed, creates the GitHub release with them and a
combined `SHA256SUMS`:

| target | runner | notes |
| --- | --- | --- |
| linux-x86_64, linux-arm64 | ubuntu-24.04, ubuntu-24.04-arm | native clang + lld; needs glibc 2.38+ at run time; ctest runs |
| macos-arm64, macos-x86_64 | macos-15 | static deps from `tools/port/deps.py`, macOS 12+, ad-hoc signed; x86_64 is cross-built |
| windows-x86_64 | macos-15 | zig + mingw-w64 cross build (Itanium ABI); not tested in CI (ctest needs Wine) |

Run the workflow by hand (Actions, "release", Run workflow) for a dry run: it builds and uploads the archives as workflow
artifacts, and creates no release.

## Locally

```sh
python3 tools/port/deps.py --target <target> [--native]     # once
# configure + ninja as in BUILDING_PORT.md, then:
python3 tools/port/package_release.py --target <target> --version v0.1.0 [--build build/port/<dir>]
```

writes `dist/mountandblade-<version>-<target>.tar.gz` (`.zip` on Windows) and `dist/SHA256SUMS.<target>`.
Each archive holds the install layout, `LICENSE` (GPL-2.0), `THIRD_PARTY_LICENSES.txt` and `README.txt`.

## Keep in step

- `CMAKE_OSX_DEPLOYMENT_TARGET` in `port/cmake/macos-*.cmake` and `-DCMAKE_OSX_DEPLOYMENT_TARGET` in `tools/port/deps.py`.
- New dependencies: add their licence to `LICENSES` in `package_release.py`.
