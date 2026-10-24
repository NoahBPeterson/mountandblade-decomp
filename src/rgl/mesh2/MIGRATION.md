# src/rgl/mesh2 migration: 0x4c2c50..0x4ce720 (record tables, map/mesh editor, mesh_2)

The region of the TU map (`work/migrate/tu_map.csv` TUs 100..104) is moved from 21 `match/slices/` slices into three
translation units. The slices are unchanged. All modules build with `/O2 /MT /Gy /TP` (one flag set). Note: the region
holds ~280 functions of the TU map; 190 of them had slice source (the rest are EH catch funclets and the 0x4ce720+ tail,
which belongs to the next object).

## Summary

| module | range | functions with source | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| rec_tables.cpp | 0x4c2c50..0x4c48d0 | 39 (+4 funclets) | 21 | 21 | 9 | 13 (incl. 4 funclets) |
| gui_edit.cpp | 0x4c48d0..0x4ca600 | 94 | 23 | 32 | 35 | 27 |
| mesh_2.cpp | 0x4ca600..0x4ce720 | 32 (+9 template catch funclets, 50 template-instance rows) | 48 | 50 | 19 (incl. 9 funclets) | 5 |
| **total** | | | **92** | **103** | | |

Regressions: none (VA sets diffed: every slice-exact VA in range is exact in a module). Improved (11 functions, nonmatching,
partial or without source -> exact): 0x4c6080, 0x4c6100, 0x4c6190, 0x4c61f0, 0x4c62b0 (`PartyTemplate`/`SkillRec`/`MenuDoor`
translated-name accessors; the slices attributed them to a wrong `gui_manager`), 0x4c6d00 (`MenuDoor::translated_door`, the slice
called it `MapObject::mesh_name`), 0x4c85e0, 0x4c8ba0, 0x4c8bf0 (declared `__fastcall` now, `this` in ecx),
0x4ca7e0 and 0x4cbb10 (the `std::vector<E176>::_Destroy_range` / `vector<tstring>::resize` instances that the module's
explicit instantiations provide; the slices had hand-written look-alikes as nonmatching).

## Boundaries (evidence)

| boundary | evidence |
|---|---|
| 0x4c2c50 (start) | TU map: class change BigObj|C4c2c50 (low). Not touched. |
| rec_tables | 0x4c2c50..0x4c48d0: TU 100+101 merged. Inside is the "T regime" (0x4c3050..0x4c4640, helpers right after their caller; docs/tu_map.md), so functions are kept in binary order and helpers defined after a caller are `__declspec(noinline)` (`dtor`). The 0x4c31c0 split of the map (class change C4c3050|C4c31c0) has no call evidence and both sides share the base `head_rec`, so one object. End 0x4c48d0 = regime change T>B (map, window 0x4c4640..0x4c4c00; kept). |
| gui_edit | 0x4c48d0..0x4ca600: TU 102+103 merged. No call crosses forward inside (`rg` over facts.json), and the classes recur on both sides (graph editor methods 0x4c53f0, 0x4c5a20, 0x4c7f90, 0x4c8000, 0x4c8af0 interleave with the dialogs and record accessors). End proven: 0x4ca550 `screen_text::refresh` calls 0x4ca600 **forward** (hard). |
| mesh_2 | 0x4ca600..0x4ce720: start proven by the same forward call. The map ends the TU at 0x4ce870; this brief ends at 0x4ce720 (`SceneGen` ctor and the `ItemRec` pair, 0x4ce720..0x4ce960, are another module's). `LangTables` ctor/loader (0x4cda20..) and the vector instantiations carry no boundary evidence and stay. |

The brief expected "~90 functions": that is the exact set (92 in the slices). The rest of the sourced rows are nonmatching/partial.

## Headers (one per class)

| header | classes | verified by |
|---|---|---|
| mesh2_common.h | `LangTables` (the 0xdf3-bucket translation table; `g_strtable` 0x5e2a18 and `g_lang` 0x5a8e68 are instances), `lang_bucket`, allocator entry points, `write_row/find_or/suffix_or` | 0x4c6010, 0x4cda20 |
| head_rec.h | `head_rec` (vtable 0x5947d8, +4, +8 = 0x63, name +0x1c), `head_cam`, `head_obj2`, `w_stream` | 0x4c2c50, 0x4c49f0..0x4c4cc0 |
| graph_obj.h | `node` (0x18), `edge` (0x34), `face` (0x64), `graph_obj` (+0x08..+0x7c, 31 methods) | 0x4c5a20, 0x4c5f60, 0x4c8000, 0x4ca8c0, 0x4cb3a0, 0x4cb780 |
| screen_cfg.h | `dev_rec`, `dev_item`, `screen_cfg`, `screen_text`, `dev_list` | 0x4ca550, 0x4ca600, 0x4ca6c0 |
| rec_tables.h | `C4c2c50`, `C4c2fb0`, `C4c3050`, `C4c3160`, `C4c31c0`, `C4c3290` (all `: head_rec`), options loader/renderer setup, music/sound objects | 0x4c2c50..0x4c3350 |
| gui_edit.h | records `RecA/RecB/RecC`, `ItemPage`, `MapRecord`, `MapScreen`, `MapEditorDialog`, `ConfigDialog`, `Block`, layout group (`conv_rec` ...), `GameData` and the language-export records | per function |
| mesh_2.h | `big_holder`, `rec_2e4/1ac/178`, `big_obj`, `tvec`, vector element types `E44/E516/E56/E50/E68/E176/...`, `csv_row`, `MissionObj`, `MissionRec2`, `ObjectList` | per function |

Merged class identities found on the way (names kept where `symbols/slices/*.txt` or a manifest row maps them):
- `gui_manager` in s004c5080/s004c5a20 (`verts/polys/edges` at +0x1c/+0x2c/+0x3c) and `graph_obj` in s004ca550/caa40/cb3a0 and the
  `conv_rec` +0x1c vector of s004c84f0 are one class: the editor mesh graph (`graph_obj`). `Block::normal` 0x4c8000 and
  `MapEditorDialog::update_terrain` 0x4c7f90 are `graph_obj` methods too (they call 0x4c5f60 / 0x4c54f0 on `this`).
- `StrTable` (s004c6330/6ca0/7290/7bf0), `LangTable` (c8ca0) and `LangTables` (cd850) are one class.
- `C4c2da8` (c2b20), `head_rec` (c46d0) and `Base0`: one base, `head_rec`. `particle_cam` (c3d90) = `head_cam`.
- 0x4c6080/6100/62b0/6d00 accessors belong to the language records of s004c8ca0 (`PartyTemplate`, `SkillRec`, `MenuDoor`), not to the
  editor.

### Conflicts with headers outside this directory (not edited, reported)

- `src/game/strat/gui_manager.h`: `gui_manager` has no names for +0x16e8..+0x16f4 (view x/y/scale) and +0x1708 (`wheel_step`
  0x4c52f0, `clear_selection` 0x4c5330, `set_view` 0x4c5190 use them); declared here on a layout-only view `gui_view`
  (gui_edit.h). It also has no `g_gui` / `g_log` declaration for the strat module's users (we declare `g_gui`).
- `src/game/tstring.h`: `tstring::operator=` returns `void`. The compiler-generated assignment of a record with a `tstring` member then
  calls the copy constructor on the member (found by the resolution audit on `E50::operator=` 0x4cd270); `E50` spells its
  `operator=` out. It also lacks nothing else used here.
- `src/rgl/stream.h`: `bin_reader` has an out-of-line ctor (0x417c50) but the options loader 0x4c3a40 and 0x4c3a40-like callers
  inline it; `options_stream` (rec_tables.h) is a trivially-destructible view of it.
- `src/rgl/scene/frustum.h` (`rgl::frustum`) is used as is for `head_cam`/`head_obj2`: ctor 0x41ef60 verified (0x4c4cc0 is byte-exact).

## Tricks removed / kept

Removed (replaced by real structure): per-slice copies of `tstring`, `stream_base`, `StrTable`, the `TCTOR/TDTOR` method-address macros
(now `tstring` members, `CONSTRUCT_TSTRING` only where the original calls the constructor on raw storage), `g_ticks`/`g_time`
duplicates, `AccessCodeStream` (now `stream_base`), the `printf("%f")` junk at the top of 0x4c3510, wrong record names.
Kept: vtable stores by constant (`*(void**)this = (void*)0x595740`) in the `C4c*` dtors (the vtables are not defined here);
`CONSTRUCT_TSTRING` (the standard placement new adds a null test the original lacks); `__declspec(noinline)` on `C4c2c50::dtor`,
`C4c2fb0::dtor`, `C4c3290::dtor` (T regime); explicit instantiations of `std::vector` (the binary's out-of-line members);
the `LangTablesBase` inline-ctor base (forces the table clear before the string members).

## Verification

- `chk.py` on every `// @ 0x` VA plus the template-instance VAs; `run_all.py --manifest`: 21/21, 32/32, 50/50 byte-exact.
  The 50 mesh_2 rows include 31 `std::vector` members that have no marker: their symbol names come from the slices' manifests
  (the size fallback of chk names the wrong twin, e.g. 0x4cd270 `E50::operator=` vs the copy ctor).
- Resolution audit (`work/migrate/scratch_rgl_mesh2_audit.jsonl`): 103 rows, 98 agree, 5 unverified (2-3 instruction functions:
  0x4c30b0, 0x4c3330, 0x4c3340, 0x4c6e00, and 0x4cbf90 whose only strong pair is its ICF'd `_Uninit_copy`). 0 disagree. Three rows report "local
  copies": `vector<E56>::operator=` 0x4cca70, `vector<R44>` copy ctor 0x4cd080 and `vector<E176>` copy ctor 0x4cd170 call ICF-folded
  twins elsewhere in the image; they are mapped in `symbols/slices/mesh_2.txt` (the names `E56`/`E44`/`E516` are reused by other slices'
  symbol files, which had produced false disagreements).
- Equivalence on the sourced non-exact rows (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`): rows with PASS or
  WEAK, or UNSUPPORTED with only a harness reason ("0/400 valid inputs": user32/malloc/RaiseException/fault models), are in
  `<module>.nonmatching.txt` with the verdict on the row. Rows with FAIL, or whose resolution audit disagrees or cannot resolve
  a reference, are moved to `<module>.partial.txt` (`equiv FAIL` / the audit text is on the row): all FAIL rows of the slices'
  "complete" claims were demoted (rec_tables: 0x4c3d90, 0x4c3e10, 0x4c3ef0, 0x4c3f60, 0x4c4090, plus 0x4c4100 and 0x4c4340 whose audit disagrees on `ftol2` / `operator new[]`; gui_edit: the editor
  `select`/`calc`/`swap` group 0x4c5190..0x4c5940, 0x4c5c80, 0x4c65d0, 0x4c6620 and 0x4c7290 (audit), 0x4c6900, 0x4c6aa0, 0x4c6b30, 0x4c6e30, 0x4c7ee0, 0x4c8000..0x4c83a0,
  0x4c8630, 0x4c8810, 0x4c8840, 0x4c8980, 0x4c8a40; mesh_2: 0x4ca600, 0x4ca8c0, 0x4caa40, 0x4cd850 (audit), 0x4cdfd0).
  Funclet rows (EH catch bodies without a symbol of their own) are listed with 0 bytes.
- `tools/status.py --no-write`: 1719 functions "Migrated into src/" in the run that included the other directories.

## Problems / not done

- 0x4c3510 `renderer_setup` and 0x4c3a40 `options_record::load` are rewritten from the disassembly (the slices had a bogus
  `printf` and a wrong stream) but stay partial: the first contains WinLicense jumps and D3DCAPS9 offsets the SDK header does not
  reproduce, the second reads through the inline `bin_reader`.
- Several nonmatching rows are accepted on harness limits (user32 imports, EH `RaiseException`, pool limits): see each row.
- `graph_obj::combine` 0x4caa40 (2293 bytes off) and `GameData::save_language_files` 0x4c8ca0 (6320 bytes, 4282 off) are carried over as
  they were; neither passes the audit.

## Addendum (M4 fix-up): 0x4ce720 `Terrain::Terrain`
The TU map ends this TU at 0x4ce870, so the constructor at 0x4ce720 (the slices' `SceneGen::SceneGen(int)`) belongs to
`mesh_2.cpp`, as its last function. It is the constructor of the world-map `Terrain` object of `src/game/terrain_a/terrain.h`
(`boxes` at +0x8440578 are the 0x900 `rgl::aabb`, `Terrain::reset` 0x4647e0 is the call at the end), not a class of its own, and it takes
NO argument (the slice's `int a` was the uninitialised w of the vector4 temporaries; `ret` without pop). Nonmatching (203 bytes,
285 bytes against 321: two 18-byte WinLicense markers inside the original; the rest differs only in two scheduling choices).
equiv PASS (220 valid, all edges, bit identical; needs `--budget 2000000 --run-seconds 120`).

`terrain.h` additions (all proven by 0x4ce720; the layout of every existing member is unchanged and terrain_a / terrain_b /
mesh_slot / agent modules still compile and match): `particle_list() : count(0)` (the three inline stores at +0xc3b0e4,
+0x343b4ec, +0x5c3b8f4), `tail` (tstring at +0x845e784), `v845e7b8` (a `std::vector<void*>`, element type unknown, first/last/end at
+0x845e7bc.., zeroed), `Terrain();`, and `LayerNames` as the former `pad0[4]` first member: its constructor builds the 15 layer-name
tstrings at +0x18 + 0x28 * i (calls of 0x42bbd0), which in the original run before the member constructions; being the first member it is
constructed first. Kept trick: the ctor order (names, lists, boxes, tail, vector) is the natural member order only because of that member.
0x4ce870 onward is `src/game/triggers/item_rec.cpp`.
