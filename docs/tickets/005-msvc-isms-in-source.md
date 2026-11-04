# 005 Removing the MSVC-only constructs that block a clang build

Area: port | Effort: M | Impact: high | Depends on: 004

## Summary

Inventory and fix the constructs that exist only to coax cl 13.10 into byte-exact output or that use MSVC-only features. Fixes must
leave the MSVC build byte-identical (verify with `run_all.py`), so they are either macros that expand to the same tokens under cl,
or edits proven neutral by the byte-exact count.

## Inventory (measured 2026-11-02, `src/**/*.{cpp,h}`)

| construct | count | portable fix |
|---|---:|---|
| calling-convention keywords | 931 in 158 files (`__cdecl` 556, `__stdcall` 204, `__fastcall` 160, `__thiscall` 11) | empty on 64-bit; on i686 clang supports all four; prelude macro |
| `__int64` | 712 in 117 files | `#define __int64 long long` (prelude) or `int64_t` typedef in a shared header |
| `__declspec(noinline)` 266, `dllimport` 54, `noreturn` 7, `naked` 1 | 328 | `-fms-extensions`, or `MB_NOINLINE` macro; `dllimport` of Newton/FMOD disappears with 010/011; the naked thunk (`Rec1208::init_screen`, `jmp ConvScreen::open`) becomes a plain forwarding method |
| `#pragma comment(linker, "/alternatename:...")` | 60 (shapes.cpp, fx_anim.cpp, ...) | These bind `*_ext` twin declarations to the real definition so cl keeps a call out-of-line (docs/Papercuts.md "_ext twin"). On clang: `__attribute__((alias))` wrappers or simply `#define` the twin to the real symbol under the prelude and add `[[gnu::noinline]]`; the twin only exists for codegen matching |
| `__asm` blocks | 20 in 2 files (`WL_MARKER`, 18 bytes of `_emit`) | WinLicense protection markers: `#define WL_MARKER(id)` -> empty in the portable config (they have no runtime effect) |
| MSVC CRT internals | `f->_flag & _IOEOF` (config.cpp `Config::load_file`), `_stat`, `_findfirst`, `_itoa` 19, `_snprintf` 47 | replace by `feof`/`getline`, `stat`, `mbp::find_first`, `snprintf` (write helpers in `port/`) |
| `i64` literal suffix | 1 | `-fms-extensions` accepts it in clang; no source change |
| `operator new(unsigned)` definitions | 1 | `size_t` |
| `hash_map` | 1 | `std::unordered_map` |
| explicit instantiations of STL internals (`std::_Destroy`, `std::_Uninit_copy`, `_Nonscalar_ptr_iterator_tag`) | ~10 TUs | delete; they exist to reproduce the original's out-of-line template instances |
| `tchar.h`, `io.h`, `malloc.h` | 3 | shim headers |
| x87-specific: `__ftol2`, `_controlfp` | 6 / 0 | ordinary casts |

Load-bearing (not cosmetic) items: (1) `rand()`: 244 call sites; module scripts' `store_random` and terrain generation use it. Behaviour with
another libc `rand()` differs but is still a valid RNG; if exact seeds matter for scene files, provide the MSVC LCG
(`seed*214013+2531011 >> 16 & 0x7fff`) as `mb::rand` - 10 lines. (2) Anything keyed on `sizeof(long)`/`sizeof(int)` in serialized data: none found
(`sizeof(int|long|void*)` 5 hits, all in layout asserts).

## Detailed plan

1. Add `MB_*` macros in one shared header only where a plain rewrite is not byte-neutral; prefer rewrites (`__int64` -> `int64_t`) when `run_all.py` stays green.
2. Replace the 60 alternatename pragmas with a macro `MB_ALIAS(twin, real)` that is the pragma under `_MSC_VER` and an `asm`-label alias under clang;
   `shapes.cpp` first (8 pragmas).
3. Run ASan/UBSan builds; fix the uninitialised reads they find (docs/equiv_failures.md already documents `add_vertex` copying an uninitialised vertex).

## Code sketch

```cpp
// src/include/mb_compat.h
#if defined(_MSC_VER)
#  define MB_ALIAS(twin, real)   /* MSVC: keep the existing #pragma comment(linker, ...) lines as they are */
#else
#  define MB_ALIAS(twin, real)   /* defined as a real forwarding definition in port/alias.cpp */
#endif
```

(cl 13.10 has no `__pragma`; the macro form needs a different trick, so the first step is to generate `port/alias.cpp` from the pragma lines by script
instead of touching `src/`.)

## Risks

Touching `src/` risks byte-exact regressions: land each change with the `run_all.py` module checks.

## Acceptance

ticket 002 probe: categories `msvc-syntax` and `stl-internal` at 0; MSVC counts unchanged.

## Effort

M (2-4 wk).
