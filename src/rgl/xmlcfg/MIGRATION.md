# rgl/xmlcfg migration (0x431400-0x43383f)

The map's TU 40 (`ti_xml_base_2`, 75 functions, "engine code right after TinyXML") is migrated from five slices
(`s00431050` (all but its TinyXML `Parse`, already in `src/tinyxml`), `s00431960`, `s00432130`, `s00432a50`,
`s004333c0` (its first 10 functions; the Curve/ItemKind tail is `src/game/agent`)) into two modules, both
`/O2 /MT /Gy /TP`, no per-row flags, no `/EH`. `match/slices` is unchanged.

## Summary

| module | range | functions | byte-exact before (slices) | byte-exact after | improved | regressed | nonmatching | partial |
|---|---|---|---|---|---|---|---|---|
| records.cpp | 0x431400..0x4320af | 27 | 27 | 27 | 0 | 0 | 0 | 0 |
| game_util.cpp | 0x4320b0..0x43383f | 48 | 41 | 41 | 0 | 0 | 7 | 0 |
| **total** | | **75** | **68** | **68** | **0** | **0** | **7** | **0** |

No regression, no improvement. The seven nonmatching rows keep the slices' diff sizes (`game_util.nonmatching.txt`):
0x432860 108, 0x432c40 279, 0x432e30 79, 0x432f60 142, 0x4333c0 65, 0x4334b0 36, 0x433510 368. One spelling
change did matter: `fill_weights`/`pick` first went 230 / 209 bytes off when the record table pointer and the
flags byte were read once into locals; they must be re-read on every use (`recs()`, `flags130()` accessors), then
`pick` is exact again and `fill_weights` is back at 142.

Verification: `chk.py` (27 + 48 of 48 VAs, same diffs as the slices), `run_all.py --manifest` on both manifests
(27/27, 41/41 byte-exact), resolution audit (68 rows: 67 agree, 1 unverified = 0x4316b0, a 5-byte tail jump to
`tstring::~tstring`), `equiv.py` on the 7 nonmatching rows (all PASS, 100% instruction coverage, every edge
covered on both builds; 0x432f60 265 valid inputs, 0x433510 355, 0x4334b0 363, the others 400).
`tools/status.py --no-write`: migrated 1103 functions (includes these 75).

## Boundaries

No call inside the range is a forward call (facts.json: every intra-range call goes to a lower address), so
the map's single TU has no hard boundary evidence in it. Split into two modules on content only:

| module | start | end |
|---|---|---|
| records | 0x431400: the TinyXML parser ends at 0x431400 (map); it begins with `IfaceA`'s deleting dtor | 0x4320b0: content (record dtors / vector deleting dtors / `std::vector<Item>` vs. game helpers); `BigObj::reset` 0x432080 stays with the 0x5000-byte record family |
| game_util | 0x4320b0 | 0x433840: `Curve::step_reached` (see src/game/agent/MIGRATION.md) |

Why two and not one or many: the first 27 functions are all compiler-generated deleting destructors and
destructor bodies of record classes (COMDATs; most of their classes are constructed in later objects), the rest
are small gameplay helpers with no relation to them. Within game_util there are no class pairs that prove a
boundary, so it stays one object ("fewer, larger").

Possible inner splits (soft evidence, NOT applied): two call sites only match when the callee is declared but not
defined in the same file, i.e. when cl knows nothing about its register use:
- `id_matches_dir` (0x4326d0) -> `group_base` (0x432130): with `group_base` visible the caller is 111 bytes off
  (`group_base_ext` declaration kept);
- `AgentX::pick` (0x433040) -> `AgentX::fill_weights` (0x432f60): 217 bytes off when visible (`fill_weights_x` kept).
In cl 13.10 a callee compiled earlier in the same object has its register use known, so these callees probably
lie in earlier objects: a boundary in (0x432130, 0x4326d0] and one at 0x433040. Other "opaque" pairs in the
region (is_neutral -> is_friend 0x4323a0, relative -> mul_t) are not opaque, so they share an object. The
shapes/agent migrations kept the same trick inside one module, so this is left as a note for the link-order
step.

## Header decisions

records.h is self-contained (shares only `src/game/tstring.h`).

game_util.h reuses `src/game/agent/agent.h` (Agent, ItemRef, Mission/`g_mission`, vec2, HeightField/HCell,
`round_float`, `clamp_index`, `Agent::slot_pair`), `curve.h` (`g_time` = the tick table 0x5e2948), `src/game/mat4.h`
(`rgl::mat4::relative` is 0x4329d0, `rgl::rot3` is the 3x3 frame) and `src/game/tstring.h`. Members those headers
do not declare cannot be added, so they are defined on classes derived from them with no new fields: `AgentX : Agent`
(is_enemy 0x432350, is_friend 0x4323a0, is_neutral 0x4323f0, speed_scale 0x432450, f100_is_0x48_and_charged
0x432470, fill_weights 0x432f60, pick 0x433040, hand_kind 0x433140), `vec2x : vec2` (rotate_a 0x432600, rotate_b
0x4327f0), `rot3x : rgl::rot3` (mul_t 0x432860), `HeightFieldX : HeightField` (sample 0x433510). Fields agent.h does not
list (+0x100 state, +0x88 item index, +0xd8 entries stride 0x2c, +0x130 flags) are read by offset with the
citing address in a comment.

Finding: the slice's `Actor` (s00432a50) is the 0x1c8-byte agent record: `f2c` is +0x2c, and `tbl` at +0x12c is
agent.h's `f12c` (`TroopDef**`), a pointer to the table of 0x30-byte records (items +0x10, count +0x14, flags
+0x18). `Actor::pick/fill_weights/hand_kind` are Agent methods; the slice's `Pair` is agent.h's `ItemRef` (the
`slot_pair` definition keeps the whole-struct copy `r = *p`, byte-exact); the slice's mission relation matrix is
agent.h's `Mission::relation`.

### Header conflicts (reported, not edited)
- `src/rgl/shapes/shapes.h` has `sphere`/`capsule`/`face`/`composite` but not the out-of-line members of this
  object: the argument constructors `(vector4, const float*)` 0x4332a0 and `(float, const vector4*, const vector4*)`
  0x433310, the out-of-line `composite(const composite&)` 0x433470 (inline in shapes.h) and `face::contains_xy` 0x4333c0 /
  `plane_z` 0x433440. It also cannot be included with `rgl.h` (agent.h). game_util.h therefore holds minimal copies
  (`shape_base`, `sphere_shape`, `capsule`, `composite`, `face`; same offsets, names kept for the vtable symbols
  `??_7sphere_shape@@6B@` / `??_7capsule@@6B@` / `??_7composite@@6B@` in `symbols/slices/game_util.txt`), like
  `src/rgl/scene/shape_clone.h`. Fix: add these members to shapes.h's classes and drop the copies.
  The slice's `face` had `v[8]` from +0x20; the verified split is `w[4]` (+0x20) and local `v[4]` (+0x60), shapes.h.
- `src/rgl/particles/mesh_obj.h` (`rgl::mesh`, which includes `rgl_math.h`) cannot be included with `rgl.h`;
  `mesh_node::clamp_c8` (0x433190, a `mesh` member: +0xa1 local flag, +0xa4 key frames, +0xc8) uses a partial view with
  the same offsets. Fix: declare `clamp_c8` in mesh_obj.h's `mesh`.
- `agent.h` defines `IfaceB` (with three data words at +4) and `RecJ` (ctor declared only). records.h defines
  `IfaceA`, `IfaceB`, `RecJ` without those words: the vector deleting destructor `??_ERecJ` 0x431750 needs a
  definition with a constructor in this file, and `IfaceB` with data words would change the records' layouts here
  (checked against `RecF`/`RecJ` sizes 0x98/0xb0 only through the destructors, which do not touch them). The two
  headers are never included together.
- `Mission`, `HeightField`, `Agent` of agent.h lack nothing this object reads except the fields listed above.
- `match/include/rgl.h` has no `mat3`; `rgl::rot3` of mat4.h is used. The three other `mat3` (shapes.h,
  phys_set.h, scene_math.h) are the same layout.

## Tricks kept / removed

Kept (needed for byte-exactness, all documented where used):
- `group_base_ext` / `fill_weights_x` declaration-only aliases for two same-object-looking callees (above).
- `__declspec(noinline)` on `AgentX::is_friend` (0x4323a0 is called, not inlined, from `is_neutral`; `is_enemy` is
  inlined) and on `rot3x::mul_t` (called from `relative`).
- Constructors and `make_*_array` helpers in records.cpp that make cl emit the vtables and vector deleting
  destructors (`??_E`) of RecF/RecJ/RecR/RecY/RecZ; the helpers are not in the binary (the `??_E` symbols exist
  because of them). `template class std::vector<Item>` emits `_Destroy`/`_Tidy` (0x431a30/0x431a60).
- `#include <new>` first: without it cl 13.10 frees arrays with plain `operator delete`.
- `#pragma intrinsic(sin, cos, fabs)` before the headers (as in the slices).

Removed: the slice-local `tstring`/`IfaceA`/`IfaceB`/`Item` redefinitions in each of the two record slices (one
header now), the slice-local `Mission`, `Agent`, `Pair`, `vec2`, `vec3`/`mat3`/`pos`, `Vec3`-on-`vec3` duplicates
and `Timer` clock table `g_ticks` (now `g_time` of curve.h), the `rgl.h` `rot3`-by-hand `mat3` and the `Z_/Y_/X_` chain
(now `SkelX`, 0x432470). `round_float` takes `float*` as agent.h declares it (the slice had `const float*`; the
symbol is `?round_float@@YAHPAM@Z`, the bytes are identical).

## Nonmatching rows (all equiv PASS)
See `game_util.nonmatching.txt`; the reasons are the slices' (operand order of the 3x3 products, stack slot
assignment of temporaries, `add eax,ebx` vs folded index, load order in the edge products, ring index
arithmetic, the height-field interpolation temporaries). Variants of `fill_weights` (local copies of the record
pointer) were re-tried here and were worse.
