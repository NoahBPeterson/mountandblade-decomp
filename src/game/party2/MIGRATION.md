# src/game/party2 migration: camera (TU 92) and party_2 (TU 93), 0x49b1c0..0x4b8f40

Region of the TU map (`work/migrate/tu_map.csv`, TUs 92 "camera" and 93 "party_2"). The staging slices in `match/slices/`
(s0049aca0 .. s004b8xxx) are unchanged and stay the reference. Every module builds with `/O2 /MT /Gy /TP` (one flag
set, no per-row flags; the `/EHsc` of the slice row for 0x4b21e6 is not needed).

## Summary

| module | range | rows | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| camera.cpp | 0x49b1c0..0x4b1160 (without 0x49cc60..0x49d4b0 and 0x49fa10, owned by src/game/script) | 37 (36 functions + 1 catch funclet) | 9 | 11 | 26 (25 functions + the funclet) | 0 |
| party_2.cpp | 0x4b1160..0x4b8f40 | 83 (81 functions + 2 funclets of `_Insert_n`) | 38 | 42 | 41 (39 functions + 2 funclets) | 0 |
| **total** | | **120** | **47** | **53** | **67** | **0** |

Regressions: none (the byte-exact VA set of the slices is a subset of the manifests). New exact rows: 0x49b720
`add_gold`, 0x49b820 `remove_gold` (were 244 / 249 diff bytes), 0x4b1b50, 0x4b2180, 0x4b32f0, 0x4b49d0. 0x4b38f0 matches only through its byte-identical twin and is kept nonmatching (see below).
The 24 migrated functions of the script interpreter (0x49cc60..0x49d4b0, `src/game/script/script_interpreter.*`) and
0x49fa10 were not touched; 0x49b0f0..0x49b170 belong to the widget_node module.
Improved nonmatching rows (diff bytes slice -> now): 0x49b630 170 -> 163, 0x49baa0 232 -> 229, 0x49f060 725 -> 611, 0x4b4320 273 -> 200,
0x4b09e0 `emit` 1485 -> 1453, 0x4b2c00 1324 -> 1323, 0x4b36a0 520 -> 518, 0x4b4a00 2278 -> 2275, 0x4b6870 437 -> 419,
0x4b6b40 1664 -> 1578, 0x4b7460 795 -> 793, 0x4b80d0 2986 -> 2971. Slightly worse (not exact either way): 0x49b930 +1, 0x49f7c0 +5,
0x4b1160 +7, 0x4b3910 +1, 0x4b3ce0 +4, 0x4b4800 +1, 0x4b6180 +15, 0x4b7840 +13 (all listed in the nonmatching files).

## Where the region splits

- **Start: 0x49b1c0** (`vector<NodeA*>::_Insert_n`, the "next range" start named in `src/game/config/MIGRATION.md`;
  0x49ad20..0x49b170 are the widget_node module).
- **0x4b1160 (`scene_step`) starts module 2.** The tu_map TU boundary lies somewhere in the window 0x4b1160..0x4b14b0;
  `scene_step`'s address is stored by 0x4b14b0 / 0x4b14f0 (tableau code), the first function that is only reached
  bottom-up from the interpreter's forward call 0x49fa10 -> 0x4b14b0. Evidence for the cut: B regime (callees first):
  0x4b0940 `run_line`, 0x4b09e0 `emit` and 0x4b1160 form a bottom-up chain; the interpreter 0x49fa10 calls 0x4b14b0 forward
  (a forward call proves the callee is in a later TU, here only "after the interpreter"). No forward call proves any split
  inside TU 93 (the 0x4b2c00 -> 0x4b32c7 call is a WinLicense jump), so TU 93 is one module.
- **End: 0x4b8f40.**
- The interpreter 0x49fa10 and the validators 0x49cc60..0x49d4b0 sit inside TU 92 but belong to `src/game/script`;
  camera.cpp therefore has a hole there.

## Class identification (slice name -> here), with the evidence

| slice name | here | evidence |
|---|---|---|
| `Interp` (s0049d500), `ScriptHolder` (s004b0940), `Mission::pop_party` (s0049f060) | `Elem` / `DialogBlock` (include/dialog_rec.h): `Elem::set_lvalue` 0x49d500, `Elem::get_operand` 0x49d650, `DialogBlock::step_less` 0x49d7f0, `renumber` 0x49d930, `link` 0x49dd30, `pop_try` 0x49f7c0, `run_line` 0x4b0940, `emit` 0x4b09e0 | `Interp` mixed the statement (p0/p1 at +0x10/+0x14 = `vals[0]`) and the block (`instrs`/`count`/`linked` = `items`/`count`/`done`, text tstring at +0xc); 0x49f7c0 reads `table[idx].kind` = `items[idx].id` and calls 0x49d7f0 and 0x49d650 / 0x49d500 on the entry; `emit` calls it on `this` |
| `Mission` (s0049b630) | `GameData` (0x5e2930): `add_gold` 0x49b720, `remove_gold` 0x49b820, `start_quest` 0x49b930, `label_text` 0x49baa0 | `script_interpreter.h` names them (`add_gold`, `remove_player_gold`, `start_quest`); Troop +0x3b4 is the gold counter, QuestRec +0x118 is `giver_troop` (written by `start_quest(quest, giver_troop)`), +0x11c the start time |
| `Mission::set_font_mesh` 0x49b630 | `HudRoot::set_font_mesh` (the 0x5d7080 object, font mesh at +0x1480, flag at +0x132c) | the slice's `Mission` pads gave +0x36788; the disassembly has +0x1480 / +0x132c / +0x158c (= mesh.mask) |
| `Party` (s0049e780: `attach_sprite`, `add_sprite`, `add_member_sprite`, `add_sprite_at`) | GameData members (`this+0xcc` = `BattleTable`, `+0xe4` its pages, `+0xf8` = `PartyMgr`): 0x49e9c0 creates the battle record between two parties, 0x49ebd0 spawns a party from a party template, 0x49ef00 `create_companion_party`, 0x49f020 `spawn_around` (names from script_interpreter.h) | the "ObjRec" is the canonical `Party` (name +0xc, key +0x20, pos +0x30, dir +0x40, battle index +0x88, owner +0x8c, flags +0xa0, state +0x8e4); not converted yet (see below) |
| `OrderEntry` / `OrderObj` (s0049bea0) | `Party` / its `model` (+0x59c0, `PartyModel`) and `label` (+0x59e8) | 0x49bea0 takes ONE argument (the party id, `ret 4`); the slice's second parameter `mode` is the id stored at entity +0xc; `OrderObj::refresh()` 0x4b7840 is `Party::update_flags`, `OrderEntry::finalize()` 0x4b2880 is `Party::place_on_map` |
| `Camera : Object` (s0049deb0) | `Camera : MissionUi` (vtable 0x5931a8, size 0x4a8) | `Object` is MissionUi (base ctor 0x48ae20, vtable 0x591c80); not converted yet |
| `Mission` (s0049c2c0, s0049bea0) | `MissionX::give_order` 0x49c2c0 / `add_reinforcements` 0x49c170 (script_interpreter.h) | kept as slice views |
| `PVec` (s0049aca0) | `std::vector<NodeA*>` | 0x49b1c0 is the out-of-line `_Insert_n` |
| `mstr` (s0049aca0) | `tstring` | same layout and constructors |
| local `Font`, `tstring` | `Font` (src/game/font.h), `tstring` (src/game/tstring.h) | canonical headers, byte-identical results |

## Layout of the files

camera.cpp: the include list, then one namespace per slice (`c49aca0` text-list panel, `c49b630` army/battle icon records,
`c49bea0` order arrow and reinforcement check, `c49c2c0` give_order, `c49deb0` view camera, `c49e780` scene sprites,
`c49f060` party bookkeeping / triggers) that keeps the slice's still-provisional object views (they define several
different classes of the same name: `Mission`, `Party`, `Scene`, `Unit`; only one of each exists globally), then the global
sections: `GameData` members, and the script block runner on the canonical `DialogBlock` / `Elem` (types in camera.h).
Each function keeps its `// @ 0x...` marker (chk/resolver bind by it). The audit resolves scopes through the namespaces.

party_2.h / party_2.cpp: see the header comment; shared classes are canonical (Party, GameData, Troop, NavData / CellGrid,
face); the 0x5c-byte event vector, the open-list helpers and the battle record are declared in party_2.h only.

## Shared (canonical) headers: pure additions made

- `src/include/party.h`: includes `./timer.h` and `./ptr_vec.h`; `namespace rgl { struct mesh; }`; `PartyV4`; the map-party
  fields carved out of pads (pos +0x30, dirx/diry +0x40, oldx/oldy +0x48, angle +0x50, radius +0x64, alpha +0x68 (named
  `alpha`, `scale()` exists), f6c .. f94, f8ac/f8b0, f8b5, `attached` +0x8b8, f8c8, f8cc, `clk8d0` +0x8d0, interval,
  state*, f8f4, f8f8, f900/f904, `clk908`, action +0x918, target +0x91c, order_pos/f924.. +0x924, order_state +0x934,
  t940, clk948, f958, goal +0x95c, dest +0x964, `path[0x400]` +0x96c, `detail[]` +0x196c, cursor, npath, mesh +0x5974,
  map_mask, moving, f5984.., kind +0x5998, target_pos +0x599c, tgt +0x59a4, tgt_id, model +0x59c0, trail_len, trail
  +0x59c8, label +0x59e8, e59ec/e59f0, deferred +0x59f4, ignore_timer +0x5ab0, visited +0x5ac0), each field with the proving
  function address; the member declarations of this module; `PartyMgr::is_live` 0x4903e0. Overlaid names are unions of
  plain members (int vs float views of one offset).
- `src/include/game_data.h`: `namespace rgl { struct mesh; }` and the method declarations `find_at`, `init_event`, `party_value`,
  `refresh_party`, `push_event`, `push_party_event`, `color_of`, `is_night`, `remove_troops`, `relation`, `list_members`,
  `on_member_killed`, `remove_party`, `label_text`, `deploy_party`, `fill_parties`, `count_units`, `add_gold`, `remove_gold`,
  `start_quest`. Fields it does not declare are reached by offset (`GD(T, off)` through `this`, `GDP` through `g_game`).
- `src/include/troop.h`: `Troop::food_factor` 0x4b21f0. `src/include/dialog_rec.h`: the `DialogBlock` / `Elem` methods above
  (and `pop_try`). `src/game/terrain_b/cell_grid.h`: `NavData::linked / can_move / reset / find_path`, `CellGrid::remove_entity`.
  `src/rgl/mesh.h`: `mesh::in_use` 0x4b17d0, `set_time`, `translate`, `clone` 0x41c6d0, `clear_geometry` 0x428e80, `compute_normals`
  0x451c50; `scene_registry::select`. `src/game/config/widget_mgr.h`: `add_tableau_button` 0x4b14f0.
- Concurrent edits by other agents to these headers (`remove_slot`, `display_name`, the mission_ui2 group in game_data.h) were
  kept; my duplicates were removed.

## Header conflicts and local views (reported, not worked around in shared headers)

- Two incompatible rgl math flavors: `match/include/rgl.h` and `src/rgl/rgl_math.h` cannot be included together. Both modules
  use `rgl_math.h` (the slices' rgl.h code compiles unchanged against it); `src/include/rgl_base.h` must not be included from party.h.
- `src/rgl/stream.h` (`stream_base`, `text_stream`) cannot be used with the rest of camera.cpp (its `rgl::string` clashes with
  mesh.h); camera.h carries `LogStreamBase` / `LogStream` views of the text log 0x5a9f60 (slot 10 = `w_cstr`).
- `script_interpreter.h` has its own views (`GameX`, `MissionX`, `Operation`, `BattleRec`, `TableauVec`) of classes that are
  canonical elsewhere; it cannot be co-included with the canonical headers, so camera.cpp declares `ScriptStmt` (the callee
  0x49fa10 = `Operation::execute`) locally.
- Still local views in camera.cpp (documented at the namespace): the text-list panel (`PObj`, `NodeA`, `UiA`), the view camera
  (`Object`, `Camera`, `Scene`, `Frustum`, which are MissionUi and friends but MissionUi's header pulls the whole UI),
  the slice `Mission`/`Party`/`Scene` views for 0x49bbb0, 0x49bea0, 0x49c170, 0x49c2c0, 0x49e780..0x49f3d0. They are the
  next candidates for conversion (see below); every class name the slices use is unique inside its namespace.

## STL and helper insight (party_2)

Many "hand-written" helpers of the slices are VC7.1 STL instantiations (`_Copy_backward_opt`, `_Uninit_copy`,
`vector<E>::_Ucopy` / `_Ufill`, `_Push_heap`, `_Pop_heap` with a `Dist*` argument, `_Adjust_heap`). The world-map open list is
`std::priority_queue<OpenItem, vector, cmp>` (min-heap on the float at +8). The explicit instantiations of `vector<E92>` and
`vector<E12>` (`_Insert_n`, `insert`, `push_back`) are in party_2.cpp; the linker kept one twin of identical bodies
(`/OPT:ICF`), so a call to the "wrong" twin is a resolution-audit disagreement even when the bytes are identical
(0x4b38f0 is therefore a nonmatching row, not an exact one).
The four 9-byte `throw_stub_*` functions (0x4b21e6, 0x4b2514, 0x4b26d6, 0x4b33b4) are the separately emitted catch(...) handlers of the
neighbouring COMDATs; the original has them as real functions and they are exact rows. 0x4b566f and 0x4b5937 are the
symbol-less tails (catch funclet and cold branches) of `vector<E92>::_Insert_n` / `vector<E12>::_Insert_n` and are recorded as
0-diff pseudo rows. 0x49b37d is the same for `vector<NodeA*>::_Insert_n`.

## Tricks kept, tricks removed

Kept (needed for an exact row or to keep a callee opaque): `MB_NOINLINE` (`__declspec(noinline)` before a name breaks the tools'
marker reader), `__fastcall` free functions emulating thiscall members (`vec_push_back`), the by-value empty classes (`ptr_tag`,
`HeapGt`) that push a stack slot, `static __declspec(noinline) check_party` (the original's file-static with its argument in EAX),
`__forceinline value_message` (cl does not inline it by itself, the original does) written as one `sprintf(buf,
lookup(s)->c_str(), string_value(n).c_str())` expression so the temporary is destroyed after the call as in the original.
Removed: the slices' `MB_NOTHROW` (`throw()`) on tstring (no effect with the canonical tstring), the local `tstring`, `mstr` and
`Font` copies, `reset_script_out` (= `link`), `Mission::pop_party` (= `DialogBlock::pop_try`), the `buf[0] = 0` initialisation
before `sprintf` in the value messages (the original has none). The WinLicense "WL" marker inside 0x49b930 (a jump over two
bytes after the prologue) is not reproduced.

## Behaviour bugs the equivalence run found (and this migration fixed)

The slices of this region could not be run by `equiv.py` (unresolved references, wrong receivers); with the annotations and the
canonical classes in place every row runs, and the first full sweep showed four real differences from the original, all fixed:

- `NavData::find_path` 0x4b6b40: the open-list element count was divided by `sizeof(OpenItem)` twice (so the leftover-list drain and the
  A* loop never ran); the loop tests the `dist` of the element popped in the previous round (0 at the start), not the front's;
  `push_heap` is only called for a hole > 0; the vertex of each path step is looked up on the previous step's record (index 0 when the
  edge is not found); the return value is a `bool` (AL only: `NavData::find_path` / `Party::start_path` are `bool` now); `chain` keeps one
  spare slot because the original also writes `chain[0x2000]`.
- `Party::step_route` 0x4b4320: arriving at the last node ends the move at once (`moving = 0`) instead of testing the cursor.
- `GameData::deploy_party` 0x49f060: the neighbour node record is read once (the slice re-read it with an extra `PartyMgr::get`).
- `Party::plan_order` 0x4b3ce0: the two search loops had the `terrain_is_road` call and the inline `(k & 7) == 0 || k == 7` test
  swapped (the resolution audit paired a `terrain_is_water` call site of the original with our road call).
- Smaller: `DialogBlock::emit` renumbers through `renumber` and `link` (the slice called them `format` / `reset_script_out`), the value
  messages of `add_gold` / `remove_gold` no longer initialise `buf[0]` (the original does not), 0x49baa0 builds `pos` with the
  3-float constructor (the 4th word is an uninitialised stack word, as in the original) and reads the font before `clear_geometry`.

Remaining equiv state (`sweep_nonmatching.py --filter camera|party_2`): no FAIL; PASS / WEAK rows have no mismatch; UNSUPPORTED rows are
the ones whose harness run is not possible (pure-virtual import, fault limits on random pointers) or whose resolution audit pairs
two call sites positionally (reasons in the nonmatching files). One exact row, 0x4b5bb0 `vector<E12>::push_back`, shows as `unverified` in
the audit (its only unpaired reference is inside an unwind funclet).

## Verification (what was run)

1. `tools/matching/chk.py` on both modules (all rows; the diff sizes are in the nonmatching files).
2. `run_all.py --manifest`: camera 11 / 11, party_2 42 / 42 byte-exact.
3. `compile_all.py --filter src/game/party2` and `--filter src/`: 113 / 113 sources compile (the shared header additions break nobody).
4. `resaudit_all.py --slices src:camera.manifest.txt,src:party_2.manifest.txt`: 53 rows, 52 agree, 1 unverified, 0 disagree (after moving
   0x4b38f0 out of the manifest: it resolved a call to the byte-identical twin construct_items instead of 0x4b2670). The 7 "local copy"
   lines are the std instantiations our object defines where the original calls its own copy (byte-identical).
5. `sweep_nonmatching.py --filter camera` / `--filter party_2`: camera 26 rows (2 PASS, 12 WEAK, 12 UNSUPPORTED), party_2 39 rows (7 PASS, 25 WEAK,
   7 UNSUPPORTED), no FAIL; every row's result is in its reason in the nonmatching files. `symbols/slices/camera.txt` binds the two globals above the
   annotation limit (`g_body_table`, `g_body_count`).
6. `tools/status.py --no-write` at the end.

## Known problems and open work

- The nonmatching rows with large diffs (tick 2971, think 2275, plan_order 1256, tick_stats 1323, find_path 1548, update_flags 1352,
  detach 1528, give_order 2124, emit 1453) are complete in logic; frames and register allocation differ.
- Convert next: the camera views to `Camera : MissionUi` (src/game/ui/mission_ui.h), 0x49e9c0..0x49f020 and 0x49bbb0 / 0x49bea0 to
  GameData members on the canonical `Party` / `BattleRec` / `PartyModel` (the structure is identified above), 0x49c170 / 0x49c2c0 to
  the canonical `Mission`.
- A `std::priority_queue<OpenItem>` rewrite of `NavData::find_path` may match better than the hand-written open list.
- `alternatename` pragmas for the `push_back_ext` / `vec_push_back` externals were not added.
