# src/game/agent2 migration: agent_stats, agent_ai

Region 0x437830..0x43e190 of the TU map (`agent_2` = TU 44 from 0x437830 and `agent_3` = TU 45 up to 0x43e190; the task
range ended at 0x43e150, but the function at 0x43e150 (`tidy_vec_pair`, 0x40 bytes) and `deque_xlen` 0x43e0e0 are the
last two functions before `Font::advance` 0x43e190, which src/game/agent3 starts with, so they are defined here).
Merged from twelve slices of `match/slices/` (`s004375b0` (from 0x437830), `s00437f00`, `s00438860` (+ `_b`), `s00439100`,
`s00439a50`, `s0043a280`, `s0043ac40`, `s0043b5d0`, `s0043bdb0`, `s0043c9c0`, `s0043d040`, `s0043db60` (up to 0x43e150))
into two modules. The slices are unchanged and stay the reference. Both modules build with the default flags
`/O2 /MT /Gy /TP` (the `/EHa` of `s00438860` is not needed: `try`/`catch(...)` compiles as in the original without it, warning C4530).

## Summary

| module | range | rows | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| agent_stats.cpp | 0x437830..0x43b100 | 73 (67 functions + 6 catch-funclet rows) | 50 | 53 | 3 | 0 | 20 (6 of them funclet rows) | 0 |
| agent_ai.cpp | 0x43b100..0x43e190 | 27 | 9 | 12 | 3 | 0 | 15 | 0 |
| **total** | | **100** | **59** | **65** | **6** | **0** | **35** (29 functions + 6 funclet rows) | **0** |

The TU map's count for the two TUs is 54 + 46 = 100 rows. No regression: every function byte-exact in its slice is byte-exact
in its module. Newly byte-exact: `Agent::set_order` 0x4379c0 (4 -> 0), `level::floor_at` 0x439be0 (284 -> 0),
`Skeleton::remove_shape` 0x439ec0 (8 -> 0), `AgentTable::next_used` 0x43b160 (16 -> 0), the hash lookup 0x43b210 (104 -> 0)
and `Agent::collect_enemies` 0x43b300 (6 -> 0). Other nonmatching rows changed (diff bytes, slice -> module):
`pick_front` 0x43b9e0 351 -> 24, `find_talk_target` 0x43b8a0 236 -> 99, `hit_chance` 0x438cb0 83 -> 63,
`trace_ray` 0x439d60 173 -> 166, `hand_pos` 0x43a510 207 -> 194, `pick_duel` 0x43bb80 449 -> 438, `nearest_face` 0x439a50
330 -> 332 (the only one that got worse: `NavMesh::dist` is visible in the object here, see Findings). The other 22 rows are
unchanged from their slices (the tables are in the `*.nonmatching.txt` files, with the slice reason text updated where it changed).

## Boundaries

There is **no forward call** in 0x437830..0x43e190 (facts.json: every call to a higher address goes to the CRT / a library
function: 0x535712, 0x535a64, 0x5363e0, 0x5368e6, 0x536920, 0x536a9f, 0x4b2520, 0x4c6010), and none from the previous object
into it or from it into the next one, so there is no hard evidence for any start or end; the map's boundaries are class
changes (low). The map has two TUs here (`agent_2` 0x437830..0x439fc0, `agent_3` 0x439fc0..0x43e190); the region became two modules,
but the inner boundary moved from **0x439fc0 to 0x43b100**:

| boundary | map | decision and evidence |
|---|---|---|
| 0x437830 (start) | class Troop -> Agent (low) | kept. 0x437830 is the constructor of the 0x1c8-byte record (calls `reset()` 0x4352f0); src/game/agent/agent.cpp ends with `RecJ` at 0x437810 |
| 0x439fc0 (map's inner boundary) | class level -> Group (low) | **dropped**. The class change is really at 0x439ec0 (`Skeleton::remove_shape`, in the map's start window 0x439d60..0x439fc0), not at 0x439fc0, and functions on both sides depend on each other's code generation: `Agent::hand_pos` 0x43a510 is 14 bytes closer to the original when `level::floor_at` 0x439be0 is visible to cl (194 vs 208), `Agent::start_channel` 0x43a980 calls `Skeleton::start_anim` 0x439fc0 |
| 0x43b100 (new inner boundary) | free gap / class level -> Table1f8 | **new, soft evidence** (see below). It is the class change between `level::light_at` (0x43ac40) and the first `Table1f8`/`AgentTable` accessor; anywhere in (0x43abf0, 0x43b160] fits the data |
| 0x43e190 (end) | class Agent -> Font (low) | kept; `deque_xlen` 0x43e0e0 and the E56 destroyer 0x43e150 belong here (src/game/agent3/MIGRATION.md leaves them to the previous object) |

Soft evidence for 0x43b100 (code-generation visibility; scripts `work/migrate/scratch_agent2_tumap.py`, `..._tuan.py`): cl
uses what it knows about callee bodies defined earlier in the same translation unit (register use, x87 stack use). Compiling the
region as one object with all bodies visible makes two functions of the second half worse than their slices, which compiled
against bodiless declarations: `update_nav_face` 0x43cb30 148 -> 198 and `set_goal_near` 0x43dd70 70 -> 87 (both call
`NavMesh::inside` 0x4391d0, `level::locate_near` 0x43abf0, `level::nearest_face` 0x439a50), while `advance_path` 0x43df60 gets 12 bytes
better (310 -> 298). Hiding the bodies of everything below 0x43b100 reproduces the slice values exactly (148 / 70 / 310) with no
declaration tricks, which is what a separate object does. Conversely the first half needs bodies
visible across its length (`Agent::damage_factor` 0x4385e0 keeps its 1.0 on the x87 stack across the call to `Agent::slot_pair`
0x432790 only when that body is defined in the object, 301 vs 240 differing bytes; `set_default_order` 0x4382e0 / `set_order` 0x4379c0;
`floor_at` needs `collect_point`). The evidence is soft because `NavMesh::dist` (0x439280) shows the same "hidden" effect on
`nearest_face` (330 vs 332) although both are surely in the first object, and because the effect depends on whether a body is
`inline`: the two modules can be concatenated into one object by `#include`-ing agent_stats.cpp and agent_ai.cpp in that order
(checked: it compiles and the same 65 rows are byte-exact; only `update_nav_face` / `set_goal_near` get worse as above and `advance_path`
better), if the relink wants a single object.

A by-product worth reporting to the owner of src/game/agent: `damage_factor` seeing `Agent::slot_pair` 0x432790 and
`slot_class_impl` 0x4350c0 (the static register-convention function) suggests the object of the first half starts at or before 0x432790,
i.e. it contains the whole of src/game/agent/* (curve, item_kind, agent), whose boundaries (0x4338e0, 0x433bf0, 0x434b20, 0x437830)
are also class changes only.

## Classes and the single header per class

Slices invented per-slice structs under the same names. The module has one header for the classes both modules use
(`agent_types.h`) and a small one per module (`agent_stats.h`: translation table, collision-shape views, PhysSet members;
`agent_ai.h`: record table, hash map, the E56 destroyer). Every offset in the headers is read by a function of this directory;
each field line cites an address.

| slice names | module name | evidence |
|---|---|---|
| `Agent` of s004375b0/s004413e0 (the 0x1c8 record, ctor 0x437830), `E1c8` (s00438860, s0043a280, s0043ac40, s0043db60) | `PlacedObj` (`typedef PlacedObj E1c8` for the std helper rows) | same vtable 0x590b84 (`??_7PlacedObj@@6B@` in symbols/slices/s0044f3c0.txt); src/game/agent/agent.h calls it PlacedObj (its `reset()` 0x4352f0 is what the constructor calls) |
| `Combatant`, `Cam`, `AgentH`, `AgentQ`, `AgentA`, `AgentBodyOwner`, `Agent` of s00437f00/s00438860/s0043ac40/s0043b5d0/s0043bdb0/s0043c9c0/s0043d040/s0043db60 | `Agent` (0x5928 bytes) | the union of the views; `Cam::smooth` 0x438860 reads +0x60 (move vector), +0x55d8 (facing), +0x56c4/+0x56c8, the same fields as `get_dir` 0x4378f0 and `update_anims` 0x43d040 |
| `OrderSlot`, `AnimSlot` (agent_nav.h), `slots[2]` | `AnimSlot` (2 x 0x2c bytes at +0xd4) | `set` 0x4378a0 is called with `anim[slot]` by `set_order`; src/game/agent3 uses the same name |
| `Pair`, `ItemRef` | `ItemRef` (item, packed word) | `Pair::stat_1e8` is `ItemRef::stat_1e8` of src/game/agent/agent.h; `Flagged::set_kind` (s00438860) is `ItemRef::set_modifier` 0x438bc0: flags dword at +4, `refresh` 0x437510 is `ItemRef::reduce` |
| `World` (s004375b0, s00437f00), `GameData` (g_game 0x5e2930) | `GameData` | `skill_level` 0x437b20 indexes `chars[agent].nib` of the 0x508-byte troop record; `Game` of s00437f00 (declared at 0x5e292c) is the mission, not the game data |
| `Game`/`Mission` (g_mission 0x5e292c) with `teams[8]`/`ClassRec`/`TeamRec` | `Mission` with `TeamRec teams[8]` at +0x5284 (stride 0x16c) | the relation matrix `relation[a*0x5b+b]` at +0x53c8 is the `rel[10]` array behind the three `HandRec` of each team record (0x5284 + 0x144), so `has_weapon_for` 0x437f00 (`teams[t].cls[k].a/b` at +0x528c), `hand_pos` 0x43a510 (`cls[c].hand[h]`) and `collect_enemies` 0x43b300 read one table |
| `Scene`, `Ctx`, `level` | `level` | `light_at` 0x43ac40, `locate_near` 0x43abf0 and the nav queries read the same object (+0xb4 nav, +0xb8 light vector, +0x28 cells) |
| `Group`, `Skel`, `Skeleton` | `Skeleton` (animation controller at body+0x304) | `Group::init_link` 0x439fc0 is called by `Agent::start_channel` as `Skel::start`; `chans` at +0x40 is the same array `update_anims` writes; src/game/agent3 reads `body->skel` |
| `LinkRec`, `Chan`, `Channel` | `Channel` (0xd8 bytes; `Curve` of src/game/agent/curve.h) | `start_channel` writes period/blend/mode, `init_link` the key points, `update_anims` the masks/weights |
| `Poly`, `Elem12`, `AnimCmd`, `SubAnim`, `AnimDef`, `TroopDef`, `DefList` | `Anim`, `Elem12`, `SubAnim`, `AnimDef`, `DefList` | `init_link` samples `Poly::point`/`Elem12::point` (0x436b90 / 0x436ae0, KeyHolder / KeyTable of agent.h); the 0x30-byte definition record's flags at +0x18 are read by `set_order`, `update_anims` |
| `AmmoDisplay::set_value` (s0043a280) | `node_groups::cap_all` | both are 0x439000; the quiver display is a mesh node group capped at the arrow count |
| `AmmoMesh`, `SlotLink`, `Mesh` | `Mesh` (`f_cc` at +0xcc) | `set_order` and `update_quiver` read/write the same word through `links[k]` |
| `IntHash`, `HNode`, `HIter` (s0043ac40) | `stdext::hash_map<int, HashVal>` (VC7.1 `<hash_map>`) | 0x43b210 is byte-exact with the library's own `_Hash::lower_bound` instantiated over `hash_map<int, X>` (the value type is not known; any 4-byte `X` gives the same code) |
| `NavMesh`, `NavFace`, `NavVert`, `cell`, `HeightField`, `level` (s00439100 header) | the same names | one definition; `NavMesh::trace` takes `vector4 from` by value (the 4 dwords of the slice's `x, y, z, unused`) |
| `Rec` (s00438860 hit chance), `ItemKind` (s00437f00) | `ItemKind` of src/game/agent/item_kind.h (included) | `f204`, `f1f4`, `f214/f218`, `type_flags` are its fields |
| `tstring`, `StrTable`/`LangTables` | src/game/tstring.h, `StrTab` | `lookup` 0x43a1e0 (`symbols/slices/gui_edit.txt` calls it `LangTables::lookup`) |
| `Mission::agents` page table | `AgentTable::pages` (vector at mission+0x18) | `AgentTable` at mission+4 has count at +0, pages at +0x14 |

## Removed slice-local tricks

- Twelve per-slice copies of `Agent`/`Mission`/`ItemKind`/`ItemRef`/`E1c8`/`tstring`/`shape_base` ... : one definition each.
- `AgentTableExt` (a bodiless second view of the agent table so that cl "must not assume the register use of the body")
  and the `Ext` views needed in a single object: gone; the AgentTable members are `inline` (as header members are) and the second
  module has no bodies of the first to see.
- `s00438860_b.cpp` (the `remove_copy` instantiation compiled apart because `<vector>` pulls in `<new>`) and the `/EHa` flag: the
  helpers compile with `<new>` and the default flags; `remove_copy` is no longer instantiated by hand, cl produces the instance from the
  `std::remove` call of `Skeleton::remove_shape`.
- The hand-written `IntHash` / `HIter` / `HNode` classes: the library's own `_Hash::lower_bound`.
- `inline void* operator new(size_t, void*)` (no `<new>`), `ptr_cat` tag classes defined per slice, the `World`/`Game` name swap.
- The `uninit_*` helper declarations with `int tag` in one slice and `ptr_tag` in another: one `ptr_tag` type.
- `identity_pos(pos*)` with a `pos` subclass: `matrix* identity_pos(matrix*)` (0x40cd00 cdecl returning its argument; the by-value
  `rgl::identity()` form compiles to a different call shape).

Kept, with reason:
- `Agent::slot_pair` (0x432790) is **defined in agent_stats.cpp** although the function belongs to an earlier object: cl only keeps the x87
  stack value (`r = 1.0f`) live across the call in `damage_factor` when the body is in the object (240 vs 301 differing bytes). It is a
  duplicate external definition for a relink (symbols/slices/agent_stats.txt maps the call to 0x432790); the owner of 0x432790 must
  drop one of the two, or the first half must be merged into that object.
- `slot_class_impl` 0x4350c0 as a `static __declspec(noinline)` copy (the register-convention function of src/game/agent, ICF-folded onto
  that copy) and `ui_weapon_class_name` 0x43a280 as a static with a stand-in caller `ui_weapon_class_name_user` that is not in the binary
  (it makes cl pass the buffer in ESI; the real caller is at 0x4598d5 in another object; the static itself is a header function folded
  onto this copy).
- `__declspec(noinline)` on `AgentTable::get` (cl would otherwise inline it into `update_aim_error`, whose call order then differs from the
  original and fails the equivalence run), and on `AnimSlot::set` (together with `inline`, which is what makes `set_order` exact: an
  inline-declared callee does not give cl register-use knowledge).
- The empty inline `virtual ~shape_base() {}`: the original's local `capsule` of `trace_ray` has no destructor call.
- The shape views (`shape_base`, `sphere_shape`, `capsule`, `face_shape`, `composite`, `manifold`) are minimal declarations without
  constructors (no vtable is emitted); src/rgl/shapes/shapes.h cannot be included together with rgl.h.

## Findings (code-generation rules found while merging)

- Members defined in a class body (`inline`) do not leak register-use knowledge to their callers even when cl does not inline them:
  `AgentTable::first_used/next_used` as `inline` makes `collect_enemies` exact (6 -> 0) and `find_talk_target` 236 -> 99; `AnimSlot::set` as
  `inline` makes `set_order` exact (4 -> 0). A plain external function defined in the object does leak it (`slot_pair`).
- `AgentTable::next_used` is exact with `++*i; while ((int)*i < count) { unsigned v = *i; if (page[v>>4][v&15].used) break; ++*i; }`
  (the slice's `do { } while ((*i = ++v) < count)` copies the index into a second register).
- 0x43b210 is a library instance (`stdext::hash_map<int, X>::lower_bound`); the vector helpers 0x43a4a0/0x43a4d0/0x43a4f0/0x438c90/0x43b2b0/0x43b2e0/0x43db60
  and the `_Uninit_*` rows keep their hand-written spelling (byte-exact) because a real `std::vector<PlacedObj>` instantiation pulls in
  more members than the binary keeps.
- The relation lookup in the pickers must go through the mission pointer already held in a register (`m->teams[a].rel[b]`), not a fresh
  `g_mission` read: `pick_front` 351 -> 24.
- A by-value `ItemRef` copy (`cur_pair()`) is a store barrier that keeps cl from reusing page-table lookups after it (`pick_random` 424 -> 374).
- `catch(...)` funclets of the `_Uninit_*` helpers compile without `/EH` (warning C4530) into the parent's COMDAT tail with no symbol; all six
  were compared byte for byte against the original (`work/migrate/scratch_agent2_funclets.py`): 0 differing bytes, relocations masked.
- The 0x43b100..0x43e190 half of the region contains the `rand()%0x3dc9 * 6.322312e-05` jitter in `set_goal_near`, which only compiles
  to the original's x87 sequence with the mission pointer as a local assigned inside the loop (slice note, still true).

## Verification

- `chk.py` on every row of both modules; `run_all.py --manifest`: agent_stats 53/53, agent_ai 12/12 byte-exact.
- Resolution audit (`work/migrate/scratch_agent2_audit.jsonl`, `resaudit_all.py --slices src:...` for both manifests): 65 rows, 65 **agree**.
  The audit only reads the annotations of headers the .cpp includes directly, so both .cpp files include `agent_types.h` by name.
  `symbols/slices/agent_stats.txt` and `agent_ai.txt` add the deleting destructor and vtable of `PlacedObj`, the local `slot_pair` copy,
  the ICF-folded `remove_copy`, `g_skill_tbl` (beyond the annotation range) and the library `lower_bound`.
- Equivalence (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`, logs `work/migrate/scratch_agent2_equiv_<va>.log`): **no FAIL**.
  The verdict of every row is in its `*.nonmatching.txt` line. PASS: 0x438cb0, 0x4391d0, 0x439280, 0x4396b0, 0x439780, 0x43a510, 0x43a800,
  0x43b7b0, 0x43db90, 0x43dcb0, 0x43df60. WEAK, no mismatch (the missed edges are the same ones on both builds, inputs the harness cannot
  form: random memory behind the mission page table, light list, nav mesh): the others except UNSUPPORTED rows: 0x43a720
  `probe_shape_contact` (0/400 inputs valid: virtual `get_min`/`get_max` on random shapes and the box query), 0x43d040 `update_anims`
  (the resolution audit pairs instructions wrongly in code that differs over 2700 bytes and reports two "disagree" on the bone-mask
  globals; the order of those globals was compared by hand with the original and the slice's own `--inputs 3000` run was PASS) and the six
  funclet rows (not callable). `update_aim_error` 0x43bdb0 failed once (call order of `AgentTable::get`, see Kept) and is WEAK with 91% coverage now.
- `tools/status.py --no-write` counts the rows as migrated.

## Header conflicts with other src/ headers (not edited)

All of these define smaller or different views of the same objects under the same names and cannot be included together with
`agent_types.h`; the layouts agree where they overlap. The unification of these classes has to be done by the orchestrator
(this module's `agent_types.h` has the superset for the fields below).

- `src/game/agent/agent.h`: `Agent` (+0x1d0 pairs, +0x22c slots, +0x250 body: same as here; it lacks +4 used, +0x2c f2c.., the
  nav/path/anim/input fields), `Mission` (this header has `teams[8]` at +0x5284, f5c, nav at +0x6698), `ItemRef` (no `set_modifier`; its
  `stat_21x` returns `unsigned`, here `int`, the harness does not care), `PlacedObj` (no constructors / vtable; here `IfaceB`-derived with the
  five frames, name and copy members), `IfaceB`, `Quad`, `AgentBody`, `Skeleton` (`{nbones, recs}` only), `vec2`, `HeightField`, `KeyHolder`.
- `src/game/agent/phys_set.h`: `PhysRec`, `PhysSet` (declares `rebuild_static_body` 0x43a900; `none_sleeping` 0x43a8a0 is not declared there, both are
  defined in agent_stats.cpp), the Newton imports, `shape_base` / `capsule_shape` / `face_shape` views.
- `src/game/agent/curve.h`: `Curve` is the 0xd8-byte `Channel` here (`busy()` is `Curve::step_reached` 0x433840, `set_phase` 0x433930,
  `rec_phase` 0x433ac0 / `reset_rec` 0x4339d0 / `set_rec_phase` 0x433b20 are members of the controller, not of the channel).
- `src/game/agent3/agent_types.h`: `Agent` (0x5928 bytes, view without +4/+0x30/+0x8c/+0xa8/+0xc4/+0x25c../+0x5340/+0x53a4/+0x5624/+0x587c),
  `Mission`, `AgentBody`, `Skeleton` (+0x30 shapes vector, +0x40 chans here), `AnimSlot`, `DefList`, `Threat`, `ItemRef`, `GameData`, `TroopRec`,
  `world` (= `level` here), `E56`/`alloc_e56` (the destroyer 0x43e150 is `tidy_vec_pair` here). Its `Agent::duration()` is `attack_speed()`
  0x438360, `find_target()` is `pick_front` 0x43b9e0, its `slot_pair_inl` / `charged_0x48` are the same inline bodies.
- `src/game/ui/*.h` (`Agent`, `Mission`, `ItemRef`, `ItemKind`, `Troop`) as in the notes of src/game/agent and src/game/agent3.
- `src/rgl/shapes/shapes.h` defines the real shape classes; `agent_stats.h` has minimal views.
- The 0x1c8-byte record is `PlacedObj` here, `Agent` in the slices and symbols/slices/s004375b0.txt (`??_7Agent@@6B@`), `Rec1c8` in src/game/agent3.
  `symbols/slices/agent_stats.txt` adds the `??_GPlacedObj@@UAEPAXI@Z` line for 0x437870.

## Problems / flags

- A stray `scratch_agent2_offs.obj` landed in the repository root when `cl.sh` was run without `/Fo` (removed).
- The explicit instantiation `template ... std::remove_copy` of the slice instantiated the `vector<int>` variant; the manifest row of
  0x438910 now names the `vector<shape_base*>` instance cl creates from `remove_shape` (both are byte-identical to 0x438910, ICF).
- `Agent::slot_pair` and `slot_class_impl` are defined here although they belong to an earlier object (see Kept); the first half
  of the region probably belongs to one object with src/game/agent/*.
- Not attempted / not solved: the large register-allocation rows (`update_anims`, `update_aim_error`, `light_at`, `move_anim`, ...): the per-row
  reasons in the nonmatching files keep the slice authors' notes, updated where this module changed them.
