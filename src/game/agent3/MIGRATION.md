# src/game/agent3 migration: agent_combat, agent_fx_sound

Region 0x43e190..0x444c28 of the TU map (`agent_4` = TU 46 from 0x43e190, and the first 29 rows of `agent_5` = TU 48
from 0x441ff0; the five rows of `phys_set` 0x441360..0x441ff0 are already in src/game/agent/phys_set.cpp and are
not touched), merged from thirteen slices of `match/slices/` (`s0043db60` (its tail from 0x43e190), `s0043e514`,
`s0043edf0`, `s0043f6d0`, `s00440070`, `s00440a30` (all but `destroy_world` 0x441360), `s004417d0` (its tail from
0x441ff0), `s00442180`, `s004428c0`, `s00443270`, `s00443aa0`, `s00444370`) into two modules. The slices are
unchanged and stay the reference. Both modules build with the default flags `/O2 /MT /Gy /TP` (the slices' `/EHa`
for 0x444bc0 is not needed: cl 13.10 emits `try`/`catch(...)` without `/EH`, warning C4530).

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| agent_combat.cpp | 0x43e190..0x441360 | 44 (37 functions + 7 funclet / cold-tail rows) | 28 | 28 | 0 | 0 | 16 (7 of them funclet rows) | 0 |
| agent_fx_sound.cpp | 0x441ff0..0x444c28 | 29 (27 functions + 2 funclet rows) | 15 | 15 | 0 | 0 | 14 (2 of them funclet rows) | 0 |
| **total** | | **73** | **43** | **43** | **0** | **0** | **30** | **0** |

(The TU map's counts: TU 46 has 44 rows, TU 48 has 29 rows before 0x444c50, which is where `NavMesh::find_path`
and the next object start.) No improvement and no regression: every function byte-exact in its slice is
byte-exact in its module, and every nonmatching row has the same diff size as in its slice except
`update_equipment_display` (0x443aa0, 1647 -> 1613: zeroing `bone_used[11]` with an explicit loop instead of `= {0}` gives a closer frame; size 2257 vs 2243 as before).

## Boundaries

There is **no forward call anywhere in 0x43e190..0x444c50**, and no call from the previous object (0x43c9c0..0x43e150)
into it or from it into the next one (facts.json checked both ways, the check finds the known 0x401910 -> 0x4036e0
forward call), so no hard evidence exists for any start or end; the map's boundaries are class-name changes (low).
They were kept, with the following caveats:

| boundary | map evidence | check |
|---|---|---|
| 0x43e190 (start of agent_combat) | class change Agent -> Font (low) | `Font::advance` / `measure` are the first real functions after two `std` COMDAT helpers (`deque<T>::_Xlen` 0x43e0e0, the E56 destroyer 0x43e150) that belong to the previous object per the map; this region calls those helpers backwards (0x43e590 -> 0x43e0e0, 0x442080 / 0x444bc0 -> 0x43e150), which is what a neighbour in the same TU or any later TU would do. Kept. |
| 0x441360 / 0x441ff0 (phys_set, already migrated) | class change Agent -> PhysSet, PhysSet -> Poser (low) | `vector<E56>` (= `PhysRec`, src/game/agent/phys_set.h) helpers are instantiated on **both** sides of phys_set (0x43e6e0, 0x43f480, 0x440a30, 0x440dc0 before; 0x442060, 0x442080, 0x444bc0 after) and 0x442080 / 0x444bc0 are the callees of its vector. This hints that 0x43e190..0x444c28 including phys_set may be one object, but that is not provable: the three modules stay as the map has them (two here, because phys_set sits in the middle and belongs to another directory). If the relink needs one object, concatenate agent_combat, phys_set, agent_fx_sound in that order. |
| 0x444c28 (end of agent_fx_sound) | the catch funclet of 0x444bc0 belongs to it (checked: 37 identical bytes inside our compile of 0x444bc0); 0x444c50 starts `NavMesh::find_path` (class change Agent -> NavMesh, low) | kept |
| inside agent_fx_sound (map: "14 possible inner splits", free gaps at 0x442110, 0x442180, 0x442390, 0x4424c0, 0x442dc0, 0x442ed0, 0x443430, 0x4435c0, 0x443730, 0x443870, 0x443a00, 0x443aa0, 0x444370) | free gaps = a function boundary with no named class change | The classes change several times (Poser, world, FxElem / FxDef, EmitTable, SoundMgr, GameData, Mission, Agent) but nothing ties any of them to a TU of its own (no forward call, no statics, no string in common), so the whole run stays one module as the task prefers when evidence is weak. |

The module names come from content (`agent_combat`: aim / hit / shoot / pick / line of sight / fire check;
`agent_fx_sound`: the effect and sound machinery plus the equipment display); rename when better evidence appears.

## Classes and the single header per class

Slices invented per-slice structs under the same names. Every offset below is read by a function of this directory;
the field lines in the headers cite the addresses.

| header | classes |
|---|---|
| `agent_types.h` (shared by both modules) | `Agent` (0x5928 bytes), `AnimSlot`, `DefList`, `Threat`, `AgentBody`, `Skeleton` (+ `SkelData`, `ChanRec`, `CapDef`, `Channel`), `ItemRef`, `E56` / `alloc_e56`, `Mission` + `AgentTable`, `world`, `GameData` (+ `TroopRec`, `Slot494`, `Party`, `PartyMgr`), `posm`, `shape_base` |
| `agent_combat.h` | `Font` / `Glyph` / `GlyphMap`, `capsule`, `contact`, `E12`, `E32`, `E48`, `Rec1c8` / `IfaceB`, `PoolRec` / `Pool`, the control / blend globals |
| `agent_fx_sound.h` | `Poser`, `Obj` / `enode` / `cell`, `Psys` / `FxElem` / `FxDef`, `Emit` / `EmitDef` / `EmitTable` / `res_obj` / `EmitRes`, `SoundMgr` (+ `SoundDef`, `SoundGroup`), `MsgLog` / `StrTable`, `CamRec`, `Mesh` / `node_list` / `Region` / `mat3`, FMOD imports, the `itcf_*` constants |

Slice name -> module name mapping (every slice's `Agent` was a view of the same record; every offset below was
checked against the other views, no two views disagree):

| slice view | module | evidence |
|---|---|---|
| `Agent` of s0043f6d0, s00440070, s00440a30, s00443270, s004428c0, s00443aa0, s00444370, `AimAgent` (s0043edf0) | one `Agent` | the union of the views, ordered by offset; `AimAgent::aim_point_and_dir` is `Agent::aim_point_and_dir` (body at +0x250, called by `line_of_sight` 0x440e30 as an Agent member) |
| `Pair` (s0043f6d0, s00440070), `ItemRef` (s00443aa0, s00444370) | `ItemRef` {item, m4} | same 8-byte entries of `pairs[11]` at +0x1d0 |
| `slots[5]` (s00440070) = `cur[2]`, `prev[2]`, `f23c` (s00443aa0 / s00444370) | one anonymous union over +0x22c..+0x240 | slots[4] at +0x23c is `f23c` |
| `OrderSlot order[2]` (s00444370), `AnimSlot slot[2]` (s0043d040 nav header), `action` (+0x100) and `f118` (s00440070) | `AnimSlot anim[2]` at +0xd4 (stride 0x2c) | `action` = anim[1].cur (+0x100), `f118` = anim[1].scale (+0x118), `OrderSlot::set` (0x4378a0, inlined) = `AnimSlot::set` |
| `Mission`, `Mission2` (s004428c0), `AgentTable` | one `Mission` with `AgentTable` at +4 | `scene` +0, count +4, page table (a `std::vector<Agent*>`) first at +0x18, `player_id` +0x5c, `relation` +0x53c8, `time_idx` +0x62e8, `view` +0x6624, `dead` +0x6678 |
| `level`, `Level`, `World`, `world` (s00442180, s00440a30, s004428c0) | `world` | the object at `Mission::scene`: `trace_ray` 0x439d60, `alloc_emitter` 0x427ea0, `collect_boxed` 0x442180, cells +0x28 |
| `Group` / `Region` (s00443aa0) | `Skeleton` / `Region` | `Group` {count, regions} is `Skeleton` {nbones, recs}; a `Region` is the 0x1dc-byte bone record (`Skeleton::recs`), count +0x1ac, mesh vector +0x1b0 |
| `CurveOwner::curve->set_rec_phase` (s00444370) | `body->skel->set_rec_phase` | +0x304 of the body is the same word |
| `SoundSystem` (s00443270, s00444370), `SoundMgr` (s00442180) | `SoundMgr` (`g_sound` 0x5aa128) | same method 0x442650 |
| `Emit` (s00442180 `obj` +8 and `f4` +4; s004428c0 `res` +4) | one `Emit` | 0x4424c0 stores the resource at +8 and its `f2c` word at +4; 0x442ca0 reads the flags at +0x1c **of the +4 word**, so +4 is a pointer (`EmitRes*`), +8 the `res_obj*` |
| `mat4` (s00442180, s004428c0) | `rgl::mat4` (src/game/mat4.h) | same 64-byte frame and `operator*` 0x40e870 |
| `identity_pos(&m)` + local | `rgl::identity()` by value | as in src/game/agent (0x40cd00 hidden-return form), `EmitTable::setup` has the same diff either way |
| `GameData` (s004428c0, s00440070, s00443270, s00443aa0), `Troop508` / `TroopRec` | `GameData`, `TroopRec` | chars at +0x13a00, `flags` +0x30 = the low nibble class read by 0x443870, slots +0x13a50 |
| `ItemKind` / `item_kind` / `ItemKind2` (s00440070, s00443aa0, s00444370) | `ItemKind` of src/game/agent/item_kind.h | included as is (`flags` 64 bit, `weight`, `get_carry_pos`); the byte `type` of s00443aa0 is `itype()` (a byte read of `type_flags`) |
| `E12`, `E32`, `E48`, `E56`, `Rec1c8`, `tstring` (copied into four slices) | one definition each; `tstring` is src/game/tstring.h | `E56` is the `PhysRec` of phys_set.h (same 0x38-byte record) |

## Removed slice-local tricks

- Seven copies of `Agent`, four of `Mission`, three of `AgentBody` / `Skeleton`, `Pair` / `ItemRef`, `posm` / `pos` / `pos2`,
  the `tstring` stub: one definition each.
- `(char*)this + 0x60` / `blk60` / `blk294[0x56b4-0x294]` layout-filler arrays as `blend_target` arguments: `&vel` (the
  vector at +0x60) and `&fx` (the vector at +0x55d8); +0x294 stays a filler array because its fields are unknown.
- `pick_front()` and `find_target()` (both 0x43b9e0): one member.
- The per-slice `agent_by_id`: one inline in agent_types.h (the page table is a `std::vector<Agent*>`).
- The slice-local `#define itp_type_*`: the enum of item_kind.h; the `itcf_*` bits stay as macros (Module System names).
- `AimAgent`, `Mission2`, `SoundSystem`, `OrderSlot`, `Curve` / `CurveOwner`, `Group`, `Level` / `World` / `level`: merged (above).

Kept, with reason:
- `agent_by_id_raw` in `Agent::pick_attack_dir` (0x43f970): the original reads the page table as a raw pointer array
  (813 differing bytes vs 837 with the vector form; every other function is the same either way).
- `__declspec(noinline)` on `Font::advance` (otherwise inlined into `Font::measure`) and on `alloc_e56::construct`
  (a separate 21-byte function in the binary, called from the uninit helpers).
- `template<> void std::vector<int>::push_back(const int&);` in agent_fx_sound.cpp (declaration only): 0x440b30 in
  agent_combat.cpp is the folded out-of-line instance that `SoundMgr::play` calls.
- `volatile int hi[2]` in `world::collect_boxed` (the original reads the upper cell indices from memory at every test).
- Explicit template instantiations with `// @ 0x...` markers for the std rows (0x43e310 .. 0x440dc0), plus
  `vector<E48>::_Insert_n`, which the instance at 0x43f3a0 needs (the binary's copy is at 0x4108b0, an earlier object).
- `Agent::slot_pair_inl` (0x432790 inlined) next to the out-of-line `slot_pair`: some call sites inline it, some call it.
- `*(int*)&maxdist` / `*(float*)&limit` in `pick_shout_target` (the original copies the float through an int).

## Header conflicts (existing headers not edited)

`Agent`, `Mission`, `AgentBody`, `Skeleton`, `ItemRef`, `shape_base`, `capsule`, `world`, `GameData` and `Emit` already
have (smaller) definitions in src/game/agent/agent.h, src/game/agent/phys_set.h, src/game/ui/*.h,
src/game/strat/strat_ext.h, src/rgl/shapes/shapes.h or src/rgl/scene/*.h; they cannot be included together with
agent_types.h. The layouts agree where they overlap (cited at each field); agent_types.h is a superset for `Agent` and
`Mission` and should replace the agent.h views when the relink needs one definition. Also: src/game/agent/item_kind.h
declares `struct res_obj;` and `resmgr::get_object` returning it, which this module completes (`res_obj`, `EmitRes` in
agent_fx_sound.h); `ItemKind` lacks a byte alias for the item type (worked around with `itype()`); `PhysRec` (phys_set.h)
and `E56` are the same record; `PlacedObj` (agent.h) is `Rec1c8` / `IfaceB` here (vtable 0x590b84).

## Verification

- `chk.py` on every VA of both modules (rows listed in the manifests): 28 / 28 and 15 / 15 exact, the nonmatching rows have the same diff as before
  (table in the `*.nonmatching.txt` files).
- `run_all.py --manifest`: agent_combat 28 / 28, agent_fx_sound 15 / 15 byte-exact.
- Resolution audit (`work/migrate/scratch_agent3_audit.jsonl`, `resaudit_all.py --slices src:...` for both manifests):
  43 rows, 41 **agree**, 2 unverified (0x43e310 and 0x43ea00: the only reference the audit cannot pair is `~tstring`,
  which these two std instances call from the cold tail beyond the compared body, 0x43e578 and its twin; no disagree).
  `symbols/slices/agent_combat.txt` and `agent_fx_sound.txt` add what the sweep could not resolve (`Rec1c8::operator=`,
  `vector<tstring>::_Destroy` 0x42d360, `copy_backward<tstring>` 0x438a10, the `capsule` and `Rec1c8` vtables, `g_ticks`,
  `g_resmgr`, `g_emit_defs`, two `rgl::vector4` members). Each module's .cpp also includes agent_types.h and tstring.h
  directly, because the audit reads the annotations of the headers a source includes itself, not recursively.
- Equivalence (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`, logs `work/migrate/scratch_agent3_equiv_*.log`;
  the structured driver `work/migrate/scratch_agent3_eq.py` is the slices' `scratch_s00440070_eq.py` pointed at the module,
  for the agent-vs-agent rows): **no FAIL**. PASS: 0x43f560, 0x440070, 0x442ca0. WEAK, no mismatch (missed edges are
  the same ones on both builds, inputs the harness cannot form): 0x43e190, 0x43f970, 0x43fd10, 0x4406a0, 0x440e30,
  0x441020, 0x4424c0, 0x442650, 0x4428c0, 0x443020, 0x443140, 0x443270, 0x443430, 0x4435c0, 0x443730, 0x443aa0,
  0x444370. UNSUPPORTED (harness): 0x43f6d0 (stubbed `shape_contact_count` writes its return address into the
  buffer), the funclet rows (not callable).
- `tools/status.py --no-write` counts the rows as migrated.

## Tried and not improved (this migration)

Whole-TU context did not move any nonmatching row (callees defined earlier in the same object do not change
`line_of_sight` -> `aim_point_and_dir` or `fire_check` -> `line_of_sight`). Re-tried: `Font::advance` (3 spellings,
size 85..93 vs 97), `can_hit` slot-pair temporaries (declaration order of u/f and of the pair temporaries),
`pick_threat` (position of `score`, an array form), `play_hit_sound` (5 spellings of the `g_game->slots[...]` load).
