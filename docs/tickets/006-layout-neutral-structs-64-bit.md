# 006 64-bit: replace hand-laid 32-bit structs with real C++ types

Area: port / modding foundation | Effort: XL | Impact: very high | Depends on: 003 (for the portable build), 004

## Summary

A 64-bit build is a goal in itself (address space for mods, macOS arm64 cannot run 32-bit at all). The source records the original's
32-bit memory layout everywhere, so 64-bit means turning those records into ordinary structs whose members are named fields, then letting the
compiler lay them out. The MSVC byte-exact build protects every step: a field replacing `int x; // ptr` or a pad-and-offset access with an identical
32-bit layout compiles to identical bytes, so `run_all.py` is the regression test for the conversion.

## Evidence

Counts over `src/` (2026-11-02; scripts in this ticket's appendix):

| pattern | count |
|---|---:|
| `// +0xNN` member offset comments | 6,856 in 278 files |
| `char padNNN[..]` filler arrays | 2,404 in 239 files |
| raw offset accesses `*(T*)((char*)p + 0xNN)` | 649 in 69 files, plus `F(this,0xNN,T)` 88 and `MAT(T,0xNN)` 14 in 2 files |
| pointer <-> int casts `(int)this`, `(int)&x`, `int x = (int)ptr` | 192 + 168 in ~54 files |
| layout asserts already in the tree | `typedef char mb_troop_size_chk[sizeof(Troop)==0x508...]` (src/include/troop.h:167), `mesh_size_check` (game/rgl_mesh.h:152), 14 TUs trip them on LP64 |
| `layout_check.py` | compiles one `offsetof(S,m)==0xNN` per commented member (tools/relink/layout_check.py) |
| STL views | `std::vector` 958 uses in 165 files; `_Myfirst/_Mylast/_Myend` 287 refs in 66 files; `vector` objects cast out of byte offsets (`(std::vector<int>*)((char*)g_game_s261e0 + 0x1a1b8)`) |
| hand-written vtable bases | `dev_rec`, 45 "placeholder virtual" vtables (ticket 003) |

Good news: serialization is typed. `stream_base` (src/rgl/stream.h) exposes `w_int/w_float/w_u64/w_i64/w_bool/w_tstr...` as virtual slots; save games, BRF/TRF
and SCO go through them (`fread/fwrite` appear only 7 times, all at buffer level: resmgr, tinyxml, options). So **file formats are pointer-size independent
today**. Verify with ticket 017's round-trip test (load a 32-bit-build save in the 64-bit build).

`.data` is 143.9 MB of static pools (0x893db94). Many pools contain pointers; doubling them is real memory (hundreds of MB), another reason to turn pools
into dynamic containers (ticket 016).

## Detailed plan

1. **Typed-field pass, header by header** (order by pointer density; start with `renderer.h`, `troop.h`, `game_data.h`, `party.h`, `mission.h`). For each struct:
   replace `char padX[n]` + offset comments with the members the pad hides (evidence is in the offset comments and the `// 0x` function citations), keep one
   `static_assert(offsetof(S,m)==0x..)` per member **guarded to 32-bit only** (`sizeof(void*)==4`) so MSVC and `-m32` builds keep the guarantee.
2. Replace raw `*(T*)((char*)p+0xNN)` with the named field (649 sites; many already have a named twin; mechanical via a script that maps (struct, offset) -> member).
3. `int` holding a pointer -> `T*`; `(int)&x` -> `intptr_t` only where arithmetic is really needed; 192+168 sites. Pointer-keyed sorted containers must not rely on pointer order.
4. Vectors: member `std::vector<T>` replaces `{lead, first, last, end}` views (the maintainer's idiomatic-types direction). A struct that embeds a `std::vector` is no longer 0xNN
   bytes in the portable build: its offsets are not load-bearing there; drop the asserts for those structs in the 64-bit configuration (per-struct tag).
5. Arrays-of-struct with explicit stride arithmetic (`slot += 0x78`, `page * 0x180 + 0x64`) -> `T arr[N]` and indexing.
6. Flip a CI job to `x86_64-linux` as soon as `sizeof(void*)==8` compiles; use ASan/UBSan.
7. `long` (4 bytes on MSVC) vs LP64 `long` (8 bytes): grep `long` (non-`long long`) in serialised structs; replace with `int32_t`.

## Code sketch

```cpp
// before (src/include/renderer.h)
IDirect3DDevice9* dev;          // 0x618
char pad61c[0x688 - 0x61c];
// after: member exists in both builds; asserts only where layout is real
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(renderer, dev) == 0x618, "renderer::dev");
#endif
```

```python
# tools/port/offset_codemod.py (sketch)
# for each struct S in headers: parse members with "// +0xNN"; for each raw access in .cpp matching (T*)((char*)X + 0xNN) where
# static type of X is S*, rewrite to X->member (or report ambiguity).
```

## Risks

- Silent truncation: `(int)ptr` stored in 32-bit fields of records saved to disk (check serialised ones first: those go through `w_int`, so they are typed - a bug would be a load-time crash).
- Behaviour differences in any code that depends on pointer ordering (`std::sort` of pointers, `std::map<T*,..>`).
- The 386 WEAK rows are unverified against the original; 64-bit conversion adds risk on exactly those functions. Use ticket 017.

## Acceptance

- `x86_64-linux` and `macos-arm64` builds with `sizeof(void*)==8`; the layout asserts live only under the 32-bit tag; `run_all.py` unchanged; ASan clean on the menu + battle script.
- Save written by the MSVC build loads in the 64-bit build and vice versa.

## Effort

XL (12-24 wk, highly parallel per header; most is review). Estimate only.
