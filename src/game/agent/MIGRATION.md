# src/game/agent migration: curve, item_kind, agent, phys_set

Region 0x4338e0..0x437830 plus 0x441360..0x441ff0 of the TU map (`curve_2`, `item_kind`, `agent`, `phys_set`),
merged from twelve slices (`s004333c0` (its last 6 functions), `s00433cb0`, `s00433e40`, `s00434ad0`,
`s00435490`, `s00435c10`, `s004362e0`, `s00436c90`, `s004375b0` (its first 4), `s00440a30` (its last
function), `s004413e0`, `s004417d0` (its first function)) into four modules. The slices under `match/slices/`
are unchanged and stay the reference. All modules build with the default flags `/O2 /MT /Gy /TP` (no per-row
flags, no `/EH`: the `try/catch` of `uninit_copy_int3` compiles as in the original without it).

## Summary

| module | range | functions (index rows) | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| curve.cpp | 0x4338e0..0x433bf0 | 6 | 6 | 6 | 0 | 0 | 0 | 0 |
| item_kind.cpp | 0x433bf0..0x434b20 | 7 | 5 | 5 | 0 | 0 | 2 | 0 |
| agent.cpp | 0x434b20..0x437830 | 74 (73 + 1 catch funclet) | 60 | 60 | 0 | 0 | 14 | 0 |
| phys_set.cpp | 0x441360..0x441ff0 | 5 | 4 | 4 | 0 | 0 | 1 | 0 |
| **total** | | **92** | **75** | **75** | **0** | **0** | **17** | **0** |

The TU map's counts per range are reproduced (curve_2 6, item_kind 7, agent 74, phys_set 5). No improvement and
no regression: every function that was byte-exact in its slice is byte-exact in its module, every nonmatching
row has the same diff size as in its slice (the table is in the `*.nonmatching.txt` files). Several spellings of
`ItemRef::reduce` were tried again (local copy of the word, hoisted clamp, `int` clamp, mask form: 17 / 17 / 27 /
32 bytes); none moved it. The migration did change two things that were not neutral:

- `Troop::nibble` (0x4351a0): the slice's `*(int*)(g_skill_tbl + i * 0x44 + 0x34)` over a `char[]` became
  `g_skill_tbl[i].cap` over a `SkillCap[]` (0x44-byte records, cap at +0x34); the first attempt (copying the
  expression onto the array of structs) scaled the index twice and gave 21 differing bytes, the array form is
  byte-exact.
- `HeightField::line_clear` (0x436da0, nonmatching): the equivalence harness found a **real behavioural
  difference** between the slice's source and the original, once the module changed the run's input stream: the
  original continues when `t0 < t1` is false because of a NaN (`fcom st(1); test ah,1; jne continue`: C0 is set
  by an unordered compare), the slice's `if (!(t0 < t1)) return true;` returns (`test ah,5; jnp`). The module
  uses `if (t0 >= t1) return true;`, which continues on NaN like the original. (Diff size unchanged, 1148.)

## Boundaries

There is **no forward call anywhere in the region** (facts.json: no function calls a higher address inside
0x433840..0x442060), so no hard boundary evidence exists for any of the four starts; the map's boundaries are
all class-name changes (low). They were checked against the call graph and content and kept:

| module | start evidence | end evidence |
|---|---|---|
| curve | map: class change Obj3 -> Curve, "low". The class change is really at 0x433840 (see below); 0x4338e0 kept as assigned | class change Curve -> ItemKind at 0x433bf0 (hash_string 0x433bb0 is a string helper, kept with the Curve helpers it follows: no call ties it either way) |
| item_kind | class change at 0x433bf0 (low); `ItemKind::mesh_name_for` and the type predicates | class change ItemKind -> ItemRef at 0x434b20 (low). `find_item` 0x434ad0 (a lookup over `g_items`) stays here |
| agent | class change at 0x434b20 (low) | the map's end 0x437830 (class Troop -> Agent, low). `Troop::falloff/adjust` and `RecJ` are the last members; 0x437830 is the `Agent` (0x1c8 record) constructor of the next object |
| phys_set | class change at 0x441360 (low); `PhysSet` members only | next object starts at 0x441ff0 (`Poser`), class change low |

Notes on what the call graph does and does not show:
- **curve: start probably 0x433840.** `Curve::step_reached` (0x433840, nonmatching in s004333c0, mode 0x14/0x15/0x16
  tests of the same 0xd8-byte record) is a Curve method, so the Obj3 -> Curve class change sits at 0x433840, not
  0x4338e0. The map put it there only because 0x433840 had no symbol name (a nonmatching row), the same
  situation as `FxElem::~FxElem` in src/game/MIGRATION.md. It is outside this task's range, so `curve.cpp` does
  not define it (`curve.h` only declares it); whoever owns 0x4333c0..0x4338e0 has to take it into this object
  or the Curve class has to be shared.
- **item_kind/agent split is weak.** `ItemRef::mesh_name` (0x434b20) calls `ItemKind::mesh_name_for` backwards,
  as everything in the region does; nothing proves an object boundary at 0x434b20, and `item_kind` + `agent`
  could be one object. They are kept as the map has them because the content differs (the item definition
  record and its helpers vs. the per-agent accessors).
- **The 0x434b70 / 0x4375b0 "seen from outside" tricks are gone.** The slices called `max_amount` (from
  `room_left`) and `Troop::falloff` (from `Troop::adjust`) through a second, bodyless declaration (`ItemRefExt`,
  `TroopExt`) "because cl must not assume the register use of the body". In the module both calls are plain
  member calls with the definition in the same file and the rows are byte-exact. (So these pairs are consistent
  with being in one object; this is not evidence for a boundary either.)
- **0x4350c0 is a static function included in many objects.** `slot_class_impl` takes its first argument in EAX
  (cl's custom convention for a `static` function) and is called from five functions of later objects (0x4385e0, 0x448eb0,
  0x44e2b0, 0x453710, 0x457b40): a `static` helper from a shared header that /OPT:ICF folded onto this copy. It is
  kept as `static __declspec(noinline)` with an external wrapper `slot_class()` (needed only so the compiler keeps
  the static function; the wrapper is not in the binary).
- 0x436d76 is the `catch(...) { throw; }` funclet of `uninit_copy_int3` (0x436d10), inside that COMDAT.

## Classes and the single header per class

Slices invented per-slice structs under the same names; the module unified them (every offset below is read by
a function of this directory; each field line in the headers cites the address):

| header | classes |
|---|---|
| `curve.h` | `Curve` (0xd8 bytes), `quaternion` (w, x, y, z), `hash_string` |
| `item_kind.h` | `ItemKind` (0x238 bytes: name +0x20, meshes +0x44/+0x48, type +0x1c8, flags +0x1d0 (64 bit), stat bases +0x1e8..+0x218), `ItemMesh`, item type enum, `pos2` (matrix + the three rotation members), local `resmgr` |
| `agent.h` | `ItemRef`, `Troop`, `Wielder`, `PlacedObj`, `RecZSubB`, `Mission`, `Level`, `SoundCtl`, `Placement`, `Agent`, `AgentBody`, `Skeleton`, `BoneRec`, `KeyTable`, `KeyHolder`, `IfaceB`/`Quad`/`RecJ`, `HeightField`, `Grid2`, `vec2`, tables |
| `phys_set.h` | `PhysSet`, `PhysRec`, the Newton Game Dynamics 1.x imports (IAT slot cited at each), the collision shapes `build_static_collision` walks |

Slice-name -> module-name mapping (all slices' `Agent` were views of different classes):

| slices | module | evidence |
|---|---|---|
| `Pair` (s00435490/s00435c10/s004362e0), `ItemRef` (s00434ad0, s00436c90) | `ItemRef` (item index + packed word; `Pair::value()` is `ItemRef::max_amount()`, 0x434b70) | `Pair::value` is documented at 0x434b70 in s00435c10; the byte layout (item, word: amount low byte, modifier byte 3) is the same in all four |
| `Agent::has_flag10/stat_int/scaled_b0/nibble` (s00434ad0) | `Troop::...` | `nibble` reads +0x68 + 4*(i/8), which contains +0x74, the field `Troop::falloff` (s004375b0) reads with `(m74 >> 12) & 0xf`; `stat_int` truncates the floats at +0x80 (`Troop::f80`, `Troop::adjust`); `flags30` +0x30 is what src/game/ui/troop_inv.h also calls `Troop::flags30` |
| `Agent::ranged_ref` (s00434ad0) | `Wielder::ranged_ref` | the record has two item references (ints) at +0x80/+0x88, which is not the `Agent` layout (`f80` is a float there) and not the `Troop` one (floats at +0x80); the owner is not known, so a class of its own |
| `Agent::reset` (s00434ad0), `PlacedObj::reset` (s00435490) | `PlacedObj::reset` | fields +0x10..+0x1b8 of the 0x1c8-byte record (vtable 0x590b84) that the next object's constructor (0x437830, `Agent` there) and copy constructor (0x437bb0) use; `init_placed` (0x435860) writes the same record (kind +0x188, id +0x18c, name +0x190, value +0x1a0) |
| `Rows::init` (s00434ad0) | `RecZSubB::init` | `symbols/slices/s00477530.txt` names 0x4353a0 `RecZSubB::init` |
| `Agent` (s00435490/s00435c10/s004362e0/s004417d0) | `Agent` (the mission agent: team +0x1c4, equipment `ItemRef pairs[11]` +0x1d0, slot indices +0x22c, body +0x250, ...) | the union of the four slices' views; `Z_/Y_/X_` of s00435c10 are `BoneRec` / `Skeleton` / `AgentBody` (s00440a30's `Skeleton` has the same `{nbones, recs}` layout) |
| `Mission` (s00435490, s00435c10), `Mission{Ctx* scene}` (s004417d0), `Ctx`/`level` | `Mission` + `Level` | `scene` at +0 and `f5c` at +0x5c are different views of the one object (g_mission 0x5e292c) |
| `Rows`, `Rec20`/`Rec78`/`NamedRec20`/`NamedRec78`, `EmitSub`/`Emitter` | `RecZSubB`, `Rec20`, `Rec78`, `EmitSub`/`Emitter` | |
| `vec2` (s004362e0, inline normalize in s00436c90) and `vec2e` (out-of-line one) | one `vec2` with the out-of-line `normalize` (0x4367c0, defined in agent.cpp); `Grid2::walk` writes the same normalisation out by hand, as the original does | both walk and line_clear match as before |
| `ItemKind::mesh_name_for` (s004333c0), `item_kind::get_carry_pos` (s00433e40), `ItemKind::mesh_for/is_*/ammo_skill` (s00433cb0) | `ItemKind` | `flags` (+0x1d0) is `unsigned __int64` (s004362e0, s00433e40); s00435490's 32-bit `flags` is its low half: `classify` reads `(unsigned)r->flags` |
| `pos`, `pos2`, `identity_pos(pos*)` / `identity_pos()` | `rgl::mat4` + `rgl::identity()` (src/game/mat4.h) for `PlacedObj::md8`; `pos2 : rgl::matrix` for `get_carry_pos` | by-value form of 0x40cd00 matches both callers (as in src/game/MIGRATION.md) |
| `PhysRec`/`PhysSet` (s00440a30, s004413e0, s004417d0), `add_capsule`/`begin_static` of s004417d0 | one `PhysRec`/`PhysSet`, `add_capsule_body`/`begin_static_build` | the same members under their s004413e0 names |
| `g_item_count` (0x5e29d4) | `g_item_kind_count` | `?g_item_count@@3HA` is also the mangled name symbols/slices/s004832b0.txt gives 0x8ed4f88, so the resolver cannot tell them apart |

## Removed slice-local tricks

- `ItemRefExt` / `TroopExt` second declarations (above).
- `ItemRec`, `NamedRec20`, `NamedRec78`, `ItemKind`/`ItemRef`/`Pair` copies: one definition each.
- the hand-declared `(const int*)0x590a9c` upper bound in `init_placed`: `extern const int g_value_max; // 0x590a9c`
  (byte-exact either way; the symbol is resolver-visible).
- `identity_pos(pos*)` + `pos tmp`: `rgl::identity()` by value.
- `Curve::unshape` was `__declspec(noinline)` in the slice header: not needed.
- `Z_/Y_/X_` pointer chain: `body->skel->recs`.
- the slice-level `#define itcf_*` constants stay in item_kind.cpp (Module System header_items.py names).

Kept, with reason:
- `slot_class_impl` as a `static __declspec(noinline)` function behind an external wrapper (above).
- Explicit instantiation `template int* std::_Copy_opt<int*, int*>(...)` (0x435a10): the instance cl emitted in
  this object, marked `// 0x435a10 <name>` like the entity module's template rows.
- The `__declspec(noinline)` on `allocN::construct` (the `AGENT_ALLOC` macro in agent.h): these tiny constructors
  are separate functions in the binary and called from `uninit_copy_int3`.
- `static inline cross_inl` / `vscale` / `vadd`: the original compiled these into the callers.

## Verification

- `chk.py` on every module VA; `run_all.py --manifest` on each manifest: curve 6/6, item_kind 5/5, agent 60/60,
  phys_set 4/4 byte-exact.
- Resolution audit (`work/migrate/scratch_agent_audit.jsonl`, `resaudit_all.py --slices src:...` for the four
  manifests): 75 rows, 75 **agree**. `symbols/slices/{agent,item_kind,curve,phys_set}.txt` add the globals the
  sweep could not resolve (`g_items`, `g_item_kind_count`, `g_time`, `g_skill_tbl`, `g_clamp_level` (0x8ee3860, outside
  the annotation range), `g_mission`, `g_phys`) and four ICF-folded `std::vector<4-byte>` instances (0x43f310,
  0x43e7b0, 0x440b30) that phys_set.cpp instantiates. A declaration's address comment must end with the
  address of the object: a trailing `(0x4375b0)` after `// 0x8ee3860` made the resolver bind `g_clamp_level` to
  the function (one "disagree" before the comment was fixed).
- Equivalence (`equiv.py agent|item_kind|phys_set <va> --src ... --flags "/O2 /MT /Gy /TP"`, logs
  `work/migrate/scratch_agent_equiv_*.log`): **no FAIL in the final state**.
  - PASS: 0x434cb0, 0x434df0, 0x4351f0, 0x4352f0, 0x4353a0, 0x435c10, 0x436210, 0x437290, 0x437510.
  - WEAK, no mismatch (missed edges are the same ones on both builds, inputs the harness cannot form):
    0x433cb0, 0x433e40, 0x435520, 0x4362e0, 0x436880, 0x436da0.
  - UNSUPPORTED (harness): 0x4417d0 (Newton imports unmodelled, 0/400 valid), 0x436d76 (a funclet, not callable).
  - 0x436da0 failed once (above) and was fixed.
- `tools/status.py --no-write` counts the rows as migrated.

## Header conflicts with other src/ headers (not edited)

- `src/game/ui/item_ref.h` (another module) defines `ItemKind`, `ItemMesh`, `ItemRef`, the `itp_type_*` / `imod_*` enums,
  `round_float`, `g_items`, `g_item_count` with a different subset of fields (value +0x1d8, f1fc, f204, f21c, f224; mesh_name; no `flags`, no
  stat bases) and `src/game/ui/troop_inv.h` defines `Troop` (+0x30 flags30, +0x80 stat, inventory +0xb4), `Rec78`, `g_tbl78`,
  `find_in_table78`. The layouts do not contradict mine (the unions are consistent: both agree on `Troop::nibble` 0x4351a0,
  `flags30`, `stat[8]`, `ItemRef::{item, m4/b}`, `ItemKind::{mesh_count, meshes, type_flags}`), but the classes are defined
  twice. These headers have to be merged into one `ItemKind` / `ItemRef` / `Troop`; the union of the fields
  is in `item_kind.h`, `agent.h`. Their `imod_*` enum stops at `imod_lordly`; the slices' list here has all 43.
- `src/game/ui/item_ref.h` names the global `g_item_count`; mine is `g_item_kind_count` (see above).
- `src/rgl/resource.h` defines `resource_manager g_resmgr` (0x5d6fa0) but no `get_object(const tstring*)` (0x419540), so
  `item_kind.h` declares a local `resmgr` class and `g_resmgr` of that type; it cannot include resource.h (d3d9/d3dx9, `rgl::string`).
  A single `resmgr` with `get_object` has to replace the local one.
- `src/rgl/shapes/shapes.h`, `src/rgl/resource.h`, `src/game/strat/strat_ext.h` define `shape_base` (and `mat3`, `vec2`) too;
  `phys_set.h` declares the minimal `shape_base` / `capsule_shape` / `face_shape` views `build_static_collision` needs, without
  constructors (no vtable is emitted).
- `src/game/mat4.h` / `tstring.h` were used unchanged. `src/rgl/rgl_math.h` (which has `rotate_about_*` = 0x40e570..0x40e770)
  cannot be included together with `match/include/rgl.h` (both define `rgl::vector4`), so `pos2` re-declares the three
  members; a `rgl::matrix` with those members in rgl.h would remove `pos2`.
- The 0x1c8-byte record is `PlacedObj` here (reset 0x4352f0, init_placed 0x435860, symbol `??_7PlacedObj@@6B@` in
  s0044f3c0) and `Agent` in the next object (ctor 0x437830, copy ctor 0x437bb0, `??_7Agent@@6B@`; `Rec1c8` in s0043e514). The
  owner of 0x437830.. has to pick one name and put `reset()` into the class; this module defines `reset` as
  `PlacedObj::reset`.
- `Curve::step_reached` (0x433840): see Boundaries (curve start).
- `PhysSet::create_world` (0x44fad0) and `PhysSet::rebuild_static_body` (0x43a900), `PhysSet`'s remaining members, are in
  other objects; `phys_set.h` declares only 0x43a900. Whoever migrates 0x439fc0..0x43e190 (`PhysSet:2`) must use this class.

## Problems / flags

- A background job was started with a shell `&` once by mistake (an equivalence run for 0x433e40); it finished normally.
- The resolution audit binds symbols to a decl's *last* address comment; header comments citing function addresses
  after the object address break it (above).
- `facts.py`/`tu_map.py` put 0x433840 into the previous TU although it is a Curve method (name-less nonmatching row).
