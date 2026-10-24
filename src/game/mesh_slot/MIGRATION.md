# M4 mesh_slot: TU 63 "mesh" and TU 64 "slot" (0x47aa40..0x488560)

Moved from `match/slices/*` (left unchanged) into original translation units, following the `terrain_b`, `ui` and
`rgl/shapes` migrations. 58 functions, 20 byte-exact (the same 20 as in the slices: no regression), 38 nonmatching,
0 partial (every source is complete).

## Before / after

| module | range | functions | byte-exact | nonmatching | partial |
|---|---|---|---|---|---|
| mesh_ops.cpp | 0x0047aa40..0x0047c9b0 | 13 | 4 | 9 | 0 |
| mesh_scene.cpp | 0x0047cae0..0x0047df80 | 12 | 5 | 7 | 0 |
| mesh_pass.cpp | 0x0047e520..0x0047ee60 | 2 | 0 | 2 | 0 |
| mesh_char.cpp | 0x0047f650..0x0047fef0 | 6 | 3 | 3 | 0 |
| mesh_terrain.cpp | 0x0047ff60..0x00482170 | 7 | 1 | 6 | 0 |
| mesh_appear.cpp | 0x00482240..0x00482240 | 1 | 0 | 1 | 0 |
| mesh_game.cpp | 0x004832b0..0x00483f60 | 6 | 2 | 4 | 0 |
| mesh_start.cpp | 0x00484590..0x004875a0 | 2 | 0 | 2 | 0 |
| slot.cpp | 0x004875e0..0x00487c40 | 9 | 5 | 4 | 0 |

Before (slices): the same 20 functions byte-exact, 38 nonmatching. Several nonmatching rows improved or changed:
0x47af30 407, 0x47d3e0 1572 (was 1750), 0x47f650 666 (was 828), 0x47ff60 3275 (was 3514, rewritten), 0x481c40 51 (63),
0x483f60 1330 (1396), 0x482240 3222 (3292), 0x47b7b0 2219 (2226). Slightly worse: 0x47e520 1968 (1844), 0x47c1d0 1022 (1007),
0x480db0 1500 (1473).

## Boundary decisions

* TU 63 is 0x47aa40..0x4875e0 and TU 64 is 0x4875e0..0x488560. Evidence is class-evidence only (weak): this region is a
  "callees before callers" (B) regime and has no forward call inside it; the forward calls from 0x47cfe0, 0x47d3e0,
  0x482240, 0x4832b0, 0x483870 and 0x484590 all go to 0x48add0 / 0x48ad00, outside the region. `near_round` 0x4875a0 is the last
  function of TU 63 (moved out of the slice that merged it with `slot`); `slot.cpp` starts at 0x4875e0.
* TU 63 is split into eight `mesh_*.cpp` files (consecutive parts, binary order kept) only because the canonical
  headers cannot be co-included (`terrain_b/poly_mesh.h` global `::vat` vs `rgl::vat`; `terrain_a/misc.h`
  `MissionCam* g_mission` vs `Mission* g_mission`; `match/include/rgl.h` vs `src/rgl/rgl_math.h`) and the slices used
  incompatible local views of the same giant object. One manifest / nonmatching file per cpp.
* Flags `/O2 /MT /Gy /TP /Isrc/game/include` (cl 13.10).

## The giant object

The Mission, the terrain object (`Terrain` / `terrain_obj`) and the slices' "Game" are one object. Canonical headers are
included (`rgl/mesh.h`, `rgl/scene/{scene,world,scene_entity}.h`, `game/mission.h`, `terrain_a/{terrain,game_state}.h`,
`terrain_b/troop_mgr.h`, `shadow_map.h`, `item_kind.h`, `item_ref.h`, `rec1c8.h`, `arr_set.h`). The slices' `h` (+0x278) is
the canonical `w` and `w` (+0x27c) the canonical `h`. View classes (derived, same layout, member functions only): `mesh_x`,
`world_x`, `TerrainX`, `EChar : TroopMgr`, `ShadowMapP`. Local full-layout records kept (no canonical class yet, reported):
`MissileU`, `AgentS` (0x5928), `MatRec`, `PhysRec`, `PropRecV`, and the self-contained types of `mesh_start.cpp`.

## Canonical header change (pure addition)

`src/game/arr_set.h`: inline default constructor `ArrSet()` zeroing `arr[5]` / `cnt[5]`, needed by
`vector<ArrSet>::resize` in `EChar::use_carry` (0x47de60), whose `_Insert_n` fill the original inlines. No offset moved.
`symbols/slices/mesh_scene.txt`: `0047a980` -> `std::vector<PhysRec>::resize`.

## Slice bugs found and fixed

* 0x47b7b0 `light_mesh`: the packed lighting colour channels were (vb, vn, -vt); the original is (vb, -vt, vn) with the
  z / y dot products summed z, y, x (both blocks).
* 0x47aa40 `mesh::transform`: the colour unpack output aliased the tint (`c.x *= tint.x` squared the colour); and the original walks
  a cached source pointer (stale when `src` is `this`).
* 0x47c1d0 `build_mirror`: the flipped `f48` is a float, the slice truncated it to int.
* 0x47ff60: the slice's `build_chunk` was the wrong shape; rewritten as `TerrainX::emit_particle`, 5 args, thiscall.
* 0x47fbd0 `load_scene`: the local inline-constructor `bin_reader_o` (the real constructor is out of line) makes it exact.

## Tricks kept

`#define vat rgl::vat` (header conflict), `NOINL` (`__declspec(noinline)` on a definition line breaks the `// @ 0x...`
marker scan), members through `this` via macros (`props`, `SC`) where local references changed register allocation
(0x47ced0, 0x47fce0), the `UNPACK_COLOR` function-pointer cast, `WL_MARK` `__asm _emit` WinLicense markers in
`mesh_start.cpp`, the `hword` macro in `regenerate_water`, `MAT(T, off)` raw-offset accessors in `mesh_game.cpp`.
Tricks removed: the slices' duplicated class definitions and `Channel` name (now `SlotChan`).

## Verification

* `chk.py` per module, `run_all.py --manifest` per module (all byte-exact rows), `compile_all.py --filter src/game/mesh_slot`: 9/9.
* `resaudit_all.py`: 20 byte-exact rows, 20 agree.
* `sweep_nonmatching.py`: no FAIL in any of the 38 rows. mesh_ops: 7 WEAK, 2 PASS (the three FAILs of the slices were
  the bugs listed above). Others: PASS/WEAK where the harness could run; UNSUPPORTED rows are an unresolved global or helper
  symbol (not in `symbols/`) or no valid `this` could be built (reason in each row).

## Open work

* `mesh_start.cpp` keeps the slice's local raw-offset types (not converted to the canonical `Mission`).
* 0x47e520 is 124 bytes worse than the slice; the `UNSUPPORTED` rows need symbols for the unresolved globals to be run.
