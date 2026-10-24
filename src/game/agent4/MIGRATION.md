# src/game/agent4 migration: agent_logic

Region 0x444c50..0x460160 of the TU map (`agent_6` = TU 49 from 0x444c50 and `agent_7` = TU 50 from 0x44e970), merged
from 31 slices of `match/slices/` (`s00444c50` .. `s0045ff30`; the tail of `s0045ff30` from 0x460160 is the dialog /
script-text object and is not part of this module) into **one** module, `agent_logic.cpp`. The slices are unchanged and
stay the reference. Flags: the default `/O2 /MT /Gy /TP` (no `/EH`: cl 13.10 emits `try`/`catch(...)` without it,
warning C4530, as in src/game/agent3).

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| agent_logic.cpp | 0x444c50..0x460160 | 65 (63 functions + 2 funclet rows of `vector<E56>::_Insert_n`) | 23 | 23 | 0 | 0 | 42 rows (40 functions + the 2 funclet rows) | 0 |

No regression: every function that is byte-exact in its slice is byte-exact here (23 of 23, checked VA by VA; the four
`std::vector<E56>` helpers 0x445700, 0x445e70, 0x44cf90, 0x44fa40 included). Every one of the 23 is `agree` in the
resolution audit. Nonmatching rows whose diff changed against the slice (the slices' nonmatching rows are copied with
their reasons; the row says `[agent_logic.cpp: N differing bytes here, M in the slice]` when it differs):

| VA | function | slice | here |
|---|---|---|---|
| 0x448eb0 | ai_update_control | 9814 | 9762 |
| 0x44d000 | choose_action | 2392 | 2317 |
| 0x452c00 | MsgLog::add | 379 | 357 |
| 0x4556d0 | update_controls | 6688 | 6686 |
| 0x457710 | tick_death | 716 | 603 |
| 0x45a170 | swing_trace | 7048 | 7019 |
| 0x4465e0 | update_ai | 8700 | 8932 |
| 0x44be50 | spawn_missile | 678 | 700 |
| 0x44e2b0 | shoot | 1455 | 1461 |
| 0x44f870 | Mission::add_placed | 185 | 217 |
| 0x453710 | hit_damage | 2475 | 2509 |
| 0x4543a0 | ragdoll_update | 2372 | 2391 |
| 0x4552c0 | draw_name | 349 | 439 |
| 0x457b40 | process_blow | 7619 | 7635 |
| 0x45f660 | update | 872 | 878 |

The first six are closer to the original than their slices, the last nine marginally further (the canonical class
definitions replace the slices' tailor-made local views, and several slice-local inline helpers had to be restated as
forced-inline members; a function that is far from the original anyway moves by a few percent either way).
No byte-exact function is among them.

Two checks beyond the byte diff caught real mistakes of the first conversion and are kept as the method: (1) the memory
displacement multiset of each nonmatching function was compared with the slice's own build (a field that the slice and
the canonical header both call `a` but that sit at +0 and +8 respectively -- `ClassRec::a` of the slices is
`ClassInfo::kind` -- and a `vel` that is +0x56a4 in the slice and +0x60 in the header were found this way); (2) `equiv`
on `update_ai` reported 3 execution mismatches before the `ClassInfo::kind` fix and none after.

## Boundaries (one module instead of two)

The map splits the range at 0x44e970 (agent_6 / agent_7). Hard evidence says it is **one** translation unit, and one that
reaches back to at least 0x4350c0 and so also covers the already migrated src/game/agent, agent2 and agent3:

| fact | evidence |
|---|---|
| `slot_class_impl` 0x4350c0 is a **static with a register convention** (item index in EAX, flag on the stack) | its callers are 0x4385e0 (agent2), 0x448eb0, 0x44e2b0 (this module's first half), 0x453710 and 0x457b40 (second half). A static helper can only be called from its own TU, so first and second half, and 0x4385e0, are one TU. It is emitted once here (first half; the second half calls it). |
| `ui_weapon_class_name` 0x43a280 (type in EAX, destination buffer in ESI) | static, called from `process_blow` 0x457b40 |
| `decode_utf8` 0x487b00 (EDI/EDX convention) | static, called by `Font::layout_text` 0x4520d0 |
| every call between the two halves goes through functions of this range | the only forward calls inside the range are the catch funclet jump 0x445ea0 -> 0x4461ad and 0x44c210 -> 0x461200 (`PlacedObj::update_1c0`, a later object, now declared in placed_obj.h) |

So the map's agent_6 / agent_7 split (class-change evidence only, low) is dropped. The module keeps both halves in one
file in address order (cl emits bottom-up). Start 0x444c50: `NavMesh::find_path` follows the catch funclet of 0x444bc0
(agent3, 37 identical bytes inside our compile). End 0x460160: `Agent::tick` is 0x22f bytes, 0x45ff30 + 0x22f is the
16-byte aligned start of `ScriptState::expand_token`, a different object (dialog text); the other functions of
`s0045ff30` are not in this module. Four `std::vector<E56>` instantiations (0x445700 `_Ucopy`, 0x445e70 `_Ufill`,
0x445ea0 `_Insert_n` with its two catch funclets 0x44608f / 0x44614e, 0x44cf90 `insert`, 0x44fa40 `push_back`) are kept in
place as explicit instantiations / small definitions exactly as the slices have them (the funclet rows stay "not
callable" rows).

The module name is from content (the agent's logic: path search, AI, attacks, movement, damage, ragdoll, mesh
editing of the lod meshes it drives); rename when better evidence appears.

## Classes and the single definition per class

Rule used: the class exists already -> include its header; extend it only by a pure addition at an unused offset (a pad
split) or an extra member declaration; never wrap existing canonical fields in unions (tried first, it changed cl's
codegen of `smooth_turn` and the `AgentTable` members and `__int64` / `Timer` members at odd offsets grew
`sizeof(Agent)`; now a `typedef char Agent_size_check[sizeof(Agent) == 0x5928 ? 1 : -1]` guards it). Other-typed views
of canonical bytes are accessor inlines (`f294()`, `move_dir()`, `ai_ta()`, `prev_move()`, `f2ec_f()`, `f310_f()`,
`attack_delay()`, `pface()`, `ppos()`, `last_blow()`), and the slices' names are renamed in the module body.

New in `agent4/`: `agent_logic.h` (the troop-class voice table `VoiceTable`, the scene entity / missile / blow / label
views `SceneEnt`, `EntVec`, `MeshVec`, `Missile` (0xb0 bytes), `Blow` (0x58), `HitInfo`, `AgentFx`, `AgentBodyV`, the Newton
Game Dynamics 1.x imports, `BoneW`, `res_shape`, globals), `rgl_mesh.h` (below).

Pure additions to canonical headers (all at offsets proven by a function of this module; the Agent fields cite it in the
line comment):

| header | addition |
|---|---|
| `src/game/agent_obj.h` | forward declarations `NewtonBody`, `Missile`, `AgentFx`, `Emit`, `ClassInfo`; `posm::rotate_su` 0x40e670, `rot_ext` 0x40a9c0, `rotate_sf` 0x40e770; `Agent` free fields `flags3c`, `t40`, `rot[3]`, `f7c`, `f84`, `fb4`, `fc0`, `fxp`, `fx1ac`..`fx1b8`, `armor[3]`, `goal_face_done`, `f288`..`f290`, `f2cc`..`f2ec`, `ai_tb`, `ai_tc`, `f310`, `f314`, `target`, `path_steps`, `wp_x/y/z`, `speed`, `veer`, `f5368`, `f536c`, `ai_timer`, `ai_timer2`, `f5390`, `f5394`, `f53a0`, `f53bc`, `f5570`, `last_hit`, `f55c0`, `f55c4`, `in_flags`, `f55d4`, `f5614`, `f5618`, `f5620`, `has_blow`, `impulse`, `v56a4`, `weapon_mode`, `f56e0`, `f56e8`, `f56ec`, `xp_pool`, `fr1`/`fr2` (the swing frames; their `o.x` words are what `on_wield_reset` sets to -2e7), `f57b4`, `hit_once`, `prop_done`, `f57c0`, `t57c8`, `t5800`, `t5810`, `clock`, `deadline`, `queued`, `mount_id` (0x5838), `mount_body`, `f5844`, `spawn`, `f5874`, `need_name`, `hold`, `f58c4`, `f58c5`, `link_vel`, `f58d8`, `hit_vec`, `hit_bone`, `hit_pos`, `t5918`; accessors above; member declarations of the 40 functions of this module |
| `src/game/nav_mesh.h`, new `src/game/hnode.h` | `NavFace::parent`/`c`, `NavEdge`, `NavMesh::open`, `edges`, `find_path`, `reachable`, the A* priority queue `PQ`; `hnode` moved out of agent2/agent_types.h (which defines `HNODE_LOAD_ORDER_COPY` to keep its user copy constructor; agent2 12/12 and 53/53 re-verified) |
| `src/game/mission.h` | `spawn_missile`, `spawn_prop`, `init_placed` (thiscall; the declaration in agent/agent.h is the slice-local stdcall form), `add_placed`, `blood_burst`, `f6610`, `dismount_count` (0x62e0) |
| `src/game/world.h` | `hf` (+0x64), `faces` (+0x58), `query_box`, `create_entity`, `find_entity`, `reg`, `light_at`, `floor_z`, `floor_at` |
| `src/game/terrain_desc.h` | `f158` (+0x158, Mission +0x6528), `f184` |
| `src/include/game_data.h` | `f1a1f0`..`f1a204`, `set_matrix`, `cnt_killed`, `cnt_killed_friend`, `cnt_ko`, `cnt_ko_friend` (0x98..0xa4) |
| `src/include/party.h` | (`Party::skill_level` 0x4b2370 was added by another agent; used here) |
| `src/include/vec.h` | `vec2::rotate_b` 0x4327f0 (defined in rgl/xmlcfg/game_util.cpp under `vec2x`) |
| `src/game/emit.h` | `res_obj` fields `f4c`, `xform`, `f10c`, `clone()`; `Emit::set_v184` 0x433740, `reset_pending` 0x433810 |
| `src/game/placed_obj.h` | `update_1c0()` 0x461200; `hp()` (a float view of `m1c0`) |
| `src/game/skeleton.h` | `Region::m4`..`bone`, `Skeleton::rvx..rvz`, `psys`, `frozen`, `set_visible`, `attach`, `attach_psys` 0x442460, `f68` (pad split) |
| `src/game/channel.h` | `curve` (+0xb4, pad split), `phase()` 0x4231f0 |
| `src/game/font.h` | `draw_glyph` 0x44f5d0, `layout_clear` 0x452bb0, `rgl::mesh` forward declaration (the `Font` fields `x_ratio`, `y_ratio`, `font_scale`, `line_spacing` that `layout_text` reads are another agent's names for the same bytes) |
| `src/include/fx.h`, `msg_log.h`, `phys_set.h`, `agent/item_kind.h`, `agent/agent.h`, `agent2/agent_types.h`, `agent3/agent_types.h` | `FxDef::skin_lods`; `MsgLog::total_h`, `cur`, `text_buf`, `bg_buf`, `age`, `height`, `color`, `lines` (the `add` declaration is the other agent's); `PhysSet::create_world`; `resmgr::find_meshes`, `get_object60`, `get_material`; `Wielder::make_ranged_mesh`; `ChanRec` fields |

Not merged / deliberate local views (reported):

- `rgl::mesh`: the canonical class (src/rgl/mesh.h) is in the `rgl_math.h` universe; the game code includes
  `match/include/rgl.h`; the two `vector4` declarations cannot be included together. `rgl_mesh.h` declares the same
  0x260-byte layout in the game universe (trimmed to the members this object uses; offsets from src/rgl/mesh.h).
- `Blow` (agent_logic.h) vs the terrain_b `RecZSubB`; `Missile` (agent_logic.h, 0xb0) vs the terrain_a pool record
  view and agent/agent.h `Wielder` (the same record): three universes, one layout each, noted in the headers.
- `Mission::init_placed` is thiscall (`mov ecx, this` before the call), the declaration in agent/agent.h is stdcall;
  the call in `spawn_prop` goes through the thiscall declaration.
- `Glyph` field naming: the layout code reads the glyph metrics by offset through `Glyph`'s own fields.
- Calls the original makes with an unused `this` in ECX are declared on small view structs (`MissionLog::log_timed`
  0x453020, `AgentCall::probe_contact` 0x43a720 and `charged_0x48` 0x432470 out of line, `AnimSlotCall::set` 0x4378a0)
  and named in `symbols/slices/agent_logic.txt` where the canonical declaration has another form.
- `Agent::process_blow(int)` is called by `update` with the float `dt` pushed as its dword; `update_anims(int)`
  likewise with `g_blend_rate`.

## Removed slice-local tricks

- 30 copies of `Agent` (all views of the one 0x5928-byte record), 10 of `Mission`, 8 of `tstring`, `ItemRef`, `ItemKind`,
  `AnimSlot`/`OrderSlot`, `Timer`/`gui_timer`, `pos`/`matrix`, `capsule`, `shape_base`, `GameData`, `World`, `Troop`:
  the canonical headers.
- Slice names renamed in bodies by regex: `mount_id` (0x2c) -> `f2c`, `dir` -> `vel`, `held` -> `pairs`,
  `wield_a/b` -> `wield[0/1]`, `order[]` -> `anim[]`, `blow_*` -> `last_blow().*`, `state` -> `f5830`, `hp` -> `health`,
  `cnt_killed/cnt_ko` -> `kill_count/wounded_count`, `refill_ammo` (0x445a90) -> `top_up_ammo` (0x438520 is `refill_ammo`).
- `slot_class_impl` (0x4350c0): one copy; the table in `s0044e2b0` was wrong (8->3, 9->4, 10->5, 0x10/0x11->6, 4->2,
  2->0; fixed).
- `Mission::init_placed` stdcall -> thiscall (above).

Kept (needed for the bytes): `__declspec(noinline)` on the register-convention statics 0x4350c0 and 0x43a280 and on the
helpers cl kept out of line; `__forceinline` statics for the pieces cl inlined at constant arguments (`set_order_inl`,
`order1`, `aset_inl`, `restart_inl`, `slot_pair_inl`), with `ANIM_SLOT_SET_OUT_OF_LINE` defined so that `AnimSlot::set` is
a real call (0x4378a0) except in the two functions where the original inlines it.

## Verification

- `tools/matching/run_all.py --manifest src/game/agent4/agent_logic.manifest.txt`: 23 / 23 byte-exact.
- `tools/matching/compile_all.py --filter src/game/agent4`: compiled.
- Resolution audit (`tools/difftest/resaudit_all.py`): 23 of 23 agree (`symbols/slices/agent_logic.txt` names
  `rgl::mat4::operator*` 0x40e870 and the symbols the nonmatching rows need).
- Nonmatching rows: `tools/difftest/sweep_nonmatching.py --filter agent_logic`: 42 rows, 28 WEAK (no mismatch), 2 PASS, 12
  UNSUPPORTED, 0 FAIL; the verdict is the `[equiv: ...]` tag of each row in `agent_logic.nonmatching.txt` (UNSUPPORTED:
  the explicit-instantiation / funclet rows, harness cannot build valid inputs for spawn_missile / shoot / start_ragdoll,
  and rows where the positional resolution audit cannot pair the symbols of a function that is not byte-exact).
- Regression of the shared header changes: every manifest under src/game, src/rgl/xmlcfg and src/rgl/shapes whose sources
  include one of the changed headers (59 manifests) re-run: all byte-exact as before; `compile_all` on all of `src/`.
