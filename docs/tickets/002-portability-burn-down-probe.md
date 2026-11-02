# 002 Portability burn-down: a repeatable clang probe

Area: port | Effort: S | Impact: medium | Depends on: none

## Summary

Run `clang -fsyntax-only` over every `src/**/*.cpp` and report the first blocking error per TU, grouped, recorded in a history
file so the port has a progress number like the match does. Sketch: `patches/port_sketch/probe_syntax.py` (run once,
2026-11-02, ~4 min serial on arm64).

## Evidence (first run: host arm64, Apple clang 21, `-std=c++03`, MSVC keywords stubbed)

```
files 180 syntax-ok 9
 84 'windows.h' file not found          49 'd3d9.h' file not found
  9 'mb_troop_size_chk' negative array   5 'mesh_size_check' negative array     (layout asserts that already exist: 64-bit canaries)
  4 unknown type name '_Nonscalar_ptr_iterator_tag'   2 no member '_Myfirst'   2 out-of-line vector::resize mismatch   (VC7.1 STL internals)
  2 winsock2.h, 1 tchar.h, 1 io.h, 1 malloc.h, 1 hash_map, 1 operator new(size_t), 1 'i64' suffix, 1 std::_Destroy instantiation
```

133/180 TUs (74%) are blocked first by Windows/D3D headers. The STL-internals class (`_Myfirst` appears 287 times in 66 files)
is handled by *replacing the hand-written container views with real `std::vector`* (ticket 006), not by emulating VC7.1.

## Plan

1. Move the sketch to `tools/port/probe_syntax.py`; add `--jobs`, `--target {x86_64-linux,aarch64-apple-macos,i686-linux}`,
   `--prelude`, `--shim-dir`, `-std=c++14` (the portable build may use a modern dialect; C++03 only reflects the source today).
2. Categories by regex: `header:windows`, `header:d3d9`, `header:crt`, `layout-assert`, `stl-internal`, `msvc-syntax`, `other`.
3. Append to `docs/port_history.json`; print in `tools/progress.py`. Keep a "never regress" list (the 9 passing TUs).
4. `--all-errors` mode for per-category counts when a shim unblocks a wall.

## Risks

Syntax-only says nothing about ABI or behaviour. Results differ between libc++ and libstdc++ slightly.

## Acceptance

One command prints the table and writes JSON; baseline 9/180 recorded.

## Effort

S (half a day).
