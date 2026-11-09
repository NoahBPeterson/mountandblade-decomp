# script_interpreter: the Module System operation interpreter (0x49fa10) and the rest of its original object

`Operation::execute(__int64* locals, int* context, int* error)` (thiscall, `ret 0xc`, 69416 bytes with its
jump table) runs one operation of a script, trigger, menu or dialog block. It is the largest function of the
game: a `switch (opcode - 30)` over a 2356-entry jump table at 0x4ae458, 591 real cases (the other labels go
to the "Unrecognized opcode %d." default). Control opcodes below 30 (`call_script`, `try_begin`, ...,
`store_script_param_*`) are run by the block runner (0x4b09e0) and never reach it.

Files:

| file | content |
|---|---|
| `script_interpreter.cpp` | one original translation unit, bodies in address order: party2/camera.cpp was merged in (panel views, GameData members, order code, validators, operand accessors, Camera, sprites, `Operation::execute`, the block runner) |
| `script_interpreter.h` | every record, global and callee the interpreter touches (views of the shared headers, see below) |
| `script_opcodes.h` | `namespace op`: every header_operations.py name as an enum value, plus the flag bits `op::neg` (0x80000000) and `op::this_or_next` (0x40000000) |
| `script_interpreter.manifest.txt` | byte-exact rows |
| `script_interpreter.nonmatching.txt` | `execute` and 22 other rows: complete, not byte-exact |
| `PROGRESS.md` | cases done per opcode range (591 / 591) |

Build: `/O2 /MT /Gy /TP`, no `/EH`, nothing else. It compiles without warnings.

## Shape of `execute`

```
g_cur_opcode = id;                         // 0x5e25a0, printed by every validator
neg = id & 0x80000000; or_next = id & 0x40000000;
g_script_error = 0; result = 1;
if (g_or_latch) { if (!or_next) g_or_latch = 0; return 1; }   // this_or_next chain already satisfied
opcode = id & 0x0fffffff;
params[0..2] = 0;
for (i = 0; i < n; i++) params[i] = get_operand(locals, &i, &types[i]);   // 0x49d650, may advance i
switch (opcode) { ... default: report("Unrecognized opcode %d.") }
if (neg) result = !result;
if (result) { if (or_next) g_or_latch = 1; } else if (or_next) result = 1;
*error = g_script_error; return result;
```

- Operands are 64-bit (`__int64 params[16]`) with a type word each (`int types[16]`, used by the string
  validator: 0 = string register, 3 = string, 0x16 = quick string). Cases cast them to `int` exactly where the
  original reads only the low dword; every range test that the original does on the full 64-bit value is
  written `IN_RANGE(params[k], lo, hi)` (`lo <= v && v < hi` in 64 bits), and `SLOT_OK` is the slot-number
  range 0..0xfffff.
- Results go through `Operation::set_lvalue(locals, const __int64&)` (0x49d500, operand 0 is the lvalue:
  register, global variable or local variable). The `SET_LV(e)` macro stores into one function-scope
  `__int64 lv_` and passes it: with a by-value temporary per call site, cl 13.10 gave each of the ~220 sites a
  frame slot of its own (frame 0x2d18, 6 KB more code). The original uses one slot (esp+0x18) for all of them.
- Case bodies are in opcode order; the original emits them in that order too (581 of 591 targets ascend).
  Several cases jump into a neighbour's tail in the original (a shared "store EAX" / "copy a frame to pos[p0]"
  sequence); they are written as independent bodies and cl merges what it merges.
- Cases 1631/1632 (`party_count_companions_of_type` / `party_count_prisoners_of_type`) are one body that
  tests `opcode == 0x660`, as in the original.
- Two jump-table entries have no header_operations.py name in 1.011: 1974 (named `spawn_scene_prop` after
  later versions; here it only validates the prop id) and 2180 (`store_troop_op_2180`: sum over the main
  party's companion stacks of ceil(15% of a hero's missing hit points) or a regular stack's wounded count).

## Callees with special conventions

- Validators (`report` 0x49cc60, `check_*` / `warn_*` 0x49cd00..0x49d4b0): all take the value by reference.
  The original passes the pointer in ECX for most, EAX for `check_party` and `warn_tableau`, EDI for
  `warn_scene_prop_instance`, EDX for `warn_agent`, and ECX (type) + ESI (value) for `check_string_arg`:
  cl's conventions for functions it can see in the same object. `report` and `check_troop` / `check_quest` /
  `check_faction` / `check_scene` are byte-exact as external `__fastcall` functions (byte-exact here, which
  `match/slices/s0049cc60` is not for `report`); the other validators are `static` (cl chooses EAX; 7 bytes off
  for the 65-byte range checks, which is closer than the slice's 28 with `__fastcall`).
- `trigger_entity(context)` (0x490430) is `__fastcall`. `trigger_item_mesh(context)` (0x4904a0) takes the
  context in EAX in the original: it is a static of the object of `match/slices/s004902a0` (`agent_of` there),
  outside this module, so it is declared as a plain call (the call site pushes instead of loading EAX).
- `ceil_float_comdat(float)` (0x404590) is the out-of-line `<math.h>` `float ceil(float)` the linker took from
  d3dfont.obj (see src/dxframework/MIGRATION.md); case 2180 is its only caller.
- The by-pointer helpers `clamp_index` (0x432510), `round_i64` (0x4875a0), `round_float` (0x432540),
  `rand_below` (0x432580), `imax_ref` (0x4324f0) and `fmin_ref` (0x4324b0) are declared again with reference
  parameters (same ABI as the pointer spellings in agent.h / game_util.h) so cases can pass temporaries, as the
  original's store order for clamp_index (hi, lo, v: right to left) shows.
- x87: `atan2` is called with doubles (the float overload would call `atan2f`); `store_sqrt/pow/sin/cos/tan`
  keep the original's `fild mult` / `fdiv st(1)` shape by computing in float with `m = (float)multiplier`.

## Headers

Reused, not redefined: `src/game/tstring.h`, `src/game/dialog_manager.h` (GameData/g_game, PartyMgr, Party,
TroopRec, ScriptState/g_script, Elem = the 0x98-byte operation record, DialogMgr, StrTable),
`src/game/agent/agent.h` (ItemRef, Agent, Mission/g_mission, Level, round_float, clamp_index, rand_below),
match/include/rgl.h through them (vector4, length, operator-). What they lack is in `script_interpreter.h` as
"view" structs over the same memory (suffix X: `GameX`, `PartyX`, `TroopX`, `AgentX`, `MissionX`,
`ItemKindX`, `ItemRefX`, ...) or as plain padded structs for records no header defines (`QuestRec`,
`FactionRec`, `SceneRec`, `PartyTemplateRec`, `BattleRec`, `TeamRec`, `EntryPoint`, `PropInst`, `Light`,
`MTEntry`/`MissionTemplate`, `Presentation`, `TableauVec`, `SceneEntity` (virtual slots 25/27), ...).
Every field and method carries the address of an instruction of the interpreter that uses it. `SI_ASSERT`
pins the layout of the big records (party 0x5ac8, troop 0x508, agent 0x5928, item kind 0x238, quest 0x138,
mission offsets of teams, props, camera and time speed). No file outside `src/game/script/` was changed.

Conflicts worked around locally:
- `identity_pos` exists in `item_kind.h` with another signature: the frame is `identity_frame()` (0x40cd00),
  the rotation `identity_rot()` (0x40cd60).
- game_util.h (`AgentX`, `MsgLog`, `Timer`) cannot be included next to agent.h's names: the agent methods,
  the message log and the timers (`MTimer`, `Timer16`) are declared on this module's own views.
- `EnvRow` (terrain_b/emit.h) declares the fog colour as a float: `EnvRowX`/`g_env_rows_x` (same address)
  reads it as the dword the original loads.

## Original behaviour reproduced (deliberately)

- `party_template_set_slot` bounds the slot number by the party template count.
- `add_quest_note_from_dialog` validates its quest id as a troop id.
- `party_set_ai_initiative` clamps the party id (not the value) to 0..100 and stores that.
- `remove_troops_from_prisoners` formats its message but never shows it.
- `add_gold_to_party` only clears the party's carry-gold byte (+0xa7).
- `set_party_creation_random_limits` clamps both limits to 0..101.
- `modify_visitors_at_site` validates the old scene, not the new one.
- `party_get_attached_party_with_rank` has no lower bound on the rank; `cur_tableau_add_override_item` no
  bound on the item count; `store_quest_number` / `store_quest_item` no quest id check.
- `store_troop_kind_count` and `store_num_regular_prisoners` walk the main party's stacks but add the sizes of
  the stacks of party `<id>` (the troop id / party id operand); `store_num_free_stacks` counts used stacks on
  the main party.
- `val_min` / `val_max` / `val_clamp` work on the low 32 bits; `shuffle_range` swaps in 32-bit values;
  division by zero (`val_div`, `store_div`) stores 40000000.

## Verification

- Every case was decompiled from the Ghidra output (case labels converted from the float-typed switch) and
  read against an annotated disassembly of the case's address range (scratch tool `case.py`, kept with the
  other helpers in `work/match/scratch_si_tools/`: `jt.py` reads the jump table, `gh.py` re-runs the Ghidra
  text with integer case labels, `progress.py` regenerates PROGRESS.md, `build.sh` compiles and runs chk).
  The equivalence harness was not run on `execute` (70 KB, custom-register callees, the presentation /
  tableau / mission objects); this reading is the check.
- `chk.py src/game/script/script_interpreter.cpp 0x49fa10`: see script_interpreter.nonmatching.txt for the current figures.
  `run_all.py --manifest src/game/script/script_interpreter.manifest.txt`: 23 / 23 byte-exact.
- Resolution audit (resaudit.py's `audit()` driven by `work/match/scratch_si_resaudit.py`, with
  `work/match/scratch_si_symbols.txt` for the globals above the annotation range, see open problems):
  the five byte-exact rows agree completely. `execute`: 280 references agree, 46 unverified (strings, our own
  inline helpers, callees used once in code that does not align), 22 flagged "disagree"; every flagged one
  was checked against the original case and is an alignment artifact of the 70 KB stream (neighbouring cases
  with the same shape paired off by one: the five quest calls, check_faction/check_scene/..., the overlay
  creators), an argument evaluation order difference (`party_join`: the original builds the party name before
  the UI lookup) or an addressing-form difference (`pos[i].f` as `0xa708a0 + i*64` vs `0xa70890 + 16`). No
  wrong callee or global was found.

## What the object can see (the key to the register map)

The merge settled the old contradiction. `set_lvalue` 0x49d500 is in this object, and cl sees every body in the file whatever its position
(moving it to the end changes nothing). The earlier "visible set_lvalue breaks the register map" (18 identical) had a different cause:
**nothing took `params`' address any more**, so its dwords stayed in registers across every call. The front end only has to see the address taken once:
`__int64* args = params;` is enough, and the reference or pointer need not be used (`const __int64& x = params[0]` and an inline helper
taking `const __int64&` do the same). With that:
- `esi` = params[0] low and `ebx` = params[1] low, with reloads after calls, exactly as in the original.
- `SET_LV` is a by-value temporary again; the shared `lv_` is gone. The temporaries pack at 0x18, sharing with validator temporaries as in the original.
- `SlotArray::set` stays visible, so its slot temporary shares 0x10 with validator temporaries as in the original.
- The prologue is instruction for instruction the original's, with params at 0x2c, except `opcode` (0x10 here, 0x20 there).

Other callees whose bodies this object sees, as duplicates of the canonical definitions (build.py takes each from its row's module): `Map10007::lookup` (`inline`),
`PartyMgr::get/is_live/next_party_id`, `Party::first_flagged/count_tail/sum_*`, `TableauTbl::current`, `AgentTable::get/next_used`, `min_int`, `clamp_index`, `round_float`,
`rand_below`, `round_i64`, the vector4 operators, `imax_ref`, `fmin_ref`, `clamp`, `rotate_about_*`, `Sel::set_vec`, `set_fog`, `EmitTable::spawn`, `set_camera_parameters`,
`SlotArray::get/set`, `NamedSelRec::set_sel`. Removing any one was re-measured after the merge: none gains identical cases (most lose cases or frame; a few are neutral).
`Party::max_stacks` (0x4b1700, after this TU) stays opaque.

Tool: `work/match/bm4o1_c.py --real lo hi` lists real slots (push-adjusted) per case on both sides, `--vs` the low slots per case. That is how the two stray dwords
below params were found: one slot used only by `add_companion_party`, whose forwarder passed three floats where the original passes a by-value `vector4` (ret 0x14).

## What keeps a temporary private (the frame)

Rule, measured: **a local whose address reaches a callee cl cannot see keeps a private frame slot**, because cl assumes the callee may keep the pointer. Visible callees let it pack, and the rule follows chains: the whole chain has to be visible. The frame is exact at 0x1ec0 with these (all duplicates of the canonical rows, above execute in script_interpreter.cpp):
- `TableauTbl::spawn_scaled`, `spawn_scaled2` (+24 identical cases, frame -0x10), and `rgl::unpack_color`, which `spawn_scaled` passes the colour pointer to.
- `MissionView::setup`: prop_instance_animate_to_position's duration and zero.
- `ItemRef::set_modifier`, `reduce`, `stat_1e8` and `Mission::place` -> `add_placed` -> `init_placed`: spawn_item's ItemRef. Any one link opaque keeps it private.
So when a slot is private here and low in the original, look down the callee chain for an opaque function. The older open item about set_battle_advantage's private slots no longer shows up in the slot lists.
Removing each of the 47 `noinline` bodies above execute was re-measured with the frame right: none gains (most lose 1 to 300 cases).

High frame order: solved, see the third lane's section below.

`create_text_overlay` / `create_button_overlay` are called directly now.

## Merge test (docs/original_tus.md A2), 2026-11-06

Measured in a scratch merge (`work/match/bm4o2_merge.py`, `bm4o2_patch.py`). Not applied: the modules stay separate. Baseline 328 identical, frame 0x1ee8.
| merged with this file | execute | other rows |
|---|---|---|
| world_data, mesh_hud, obj_tbl, widget_mgr, widget_node | 352, frame 0x1ed8 | exact lost: 0x490a30, 0x490fa0, 0x491e50, 0x494020; 6 more world_data rows worse, 3 better; 0x499050 +3, 0x49ad20 +29 |
| same without world_data | 352, frame 0x1ed8 | no exact row lost; 0x499050 +3, 0x49ad20 +29, 0x49bbb0 -22 |
| mesh_hud only | 328, frame 0x1ee8 | no gain |
| obj_tbl only | 352, frame 0x1ed8 | all of the gain |
- The whole gain is `spawn_scaled` and `spawn_scaled2` being visible, and that is available without a merge (see above).
- Against world_data: 0x490a30, 0x490fa0 and 0x491e50 need `Map10007::lookup` opaque, and execute needs it visible (123 identical without it). One object cannot have both; the body's position in the file does not matter. 0x494020 was lost to `RGL_VECTOR4_OPS_OUT_OF_LINE`, which this file defines and which world_data needs off.
- Against widget_node: 0x49ad46 calls the ContainerBase constructor out of line where Camera::Camera expands it; that is the +29. It agrees with the forward call 0x49aca0 -> 0x49ad20.
- Still unexplained: `agent_of` 0x4904a0 taking EAX with execute as its only caller.
- Visible bodies do not prove a shared object: `PartyMgr::get` 0x51ee50 and `lookup` 0x48ad00 are visible here and far outside this range, so they look like header inlines.

## 2026-11-06, third lane: fix top-down, judge by "mod-stack+regs"

Figures: see script_interpreter.nonmatching.txt. Levers, in order of yield:
- **Case order.** Nine cases were out of the original's order (1112, 1113, 1505, 1506, 1535, 1536, 1612, 1784, 2383); the source now has the original's order (`bm4o3_c.py --order` prints differences). Rotation is a chain in that order.
- **Forwarders again.** `CS()` is a macro in execute now, `create_*_overlay`, `create_mesh_overlay_with_tableau`, NamedRec `setup`/`setup2` and GameData `get_name`, `event_string` are called directly. The earlier "direct overlay calls rotate later cases" was an earlier wrong case (position_set_z_to_ground_level): fixed top-down, it gains.
- **`check_string_arg(value, type)`.** The parameter order was wrong (the row stays byte-exact either way, arguments are evaluated right to left, so call sites flipped `lea ecx`/`lea esi`): +14 mod-stack.
- **Named strings.** `tstring("x")` as a temporary pushes eax (the ctor's return); the original pushes `lea r,[tmp]`: `{ tstring msg(buf); g_gui.push(msg, c); }`, `{ tstring sep(": "); ...append(sep); }`. And locals that weren't in the original go: `tstring text = ...; nm = expand(text)` and `const char* arg0 = CS(...)` were expressions.
- **High frame order: solved.** All tstring slots match the original now. It was the forwarders' temporaries and locals above, not the slot allocator. The slots are laid out by the temporaries' creation, so wrong temporaries shifted everything.
- **`inline __declspec(noinline)` versus `__declspec(noinline)`.** `AgentTable::get` was plain visible: an x87 value stayed on the FP stack across it. As `inline` (a header inline, compiled later) the float spills as in the original (store_agent_hit_points, agent_set_speed_limit). Making each of the other 47 visible bodies `inline` changes nothing.
- A float computed in both arms is one expression per arm, CSE hoists it (`position_move_x`). `bool v = x != 0; g = v;` gives `mov al,1 / xor al,al` (set_show_messages). `switch ... default: __assume(0)` drops the range check (change_screen_notes). `size() > 0` gives `jbe`. `(__int64)(int)(x & 0xff)` gives cdq where x is unsigned (item_get_type and neighbours: +6 cases each, they were rotating what follows).
- **The R1 cases are not rotation.** `[eax+edx+0x64]` against `[edx+eax+0x64]` flips with the number of *used* symbols in the whole function, in cases before and after the edit (codegen.md 6.2). It does not move with unused locals. Adding named locals one by one flips groups on and off (check_quest_*, troop_*, slot cases) with no monotonic pattern, so "identical" is noisy by about 30 cases for any single edit: judge by mod-stack+regs. These should settle when the symbol count is right.

**The three ABI differences are applied (2026-11-06, the `SI_TRUE_ABI` measuring macro is gone).** `Mission::get_frame` 0x48a420 returns the matrix by value (execute is its only caller, 5 sites). `sl_item_flag_all` 0x48a210, `sl_item_set_all` 0x48a250 are GameData members with an unused `this`, and execute calls them and `is_night` through `(GameData*)g_game`, so the call sites load `mov ecx,g_game`. `trigger_item_mesh` 0x4904a0 moved here from world_data as a `static`: cl passes the context in EAX at the three call sites, so the row belongs to this object, and it is byte-exact.

Open, top-down: hero_can_join family (still as in the section above), player_has_item, store_conversation_agent (2199: `cmp esi,[ebx]` reloads the count where ours CSEs it across the visible next_used; the tail of its first arm is the X form `mov [18],eax; lea eax,[18]` that 2200..2263 share, so this one case probably rotates the store block), quest note indexing `(i*0x138 + j*16) + base` (ours `(base + i*0x138) + j*16`), set_visitor/set_visitors (tree order, 19 bytes), store_troop_health's `h` (private 0xac, 0x10 there: max_hit_points is opaque here, perhaps a visible inline).

Tools: `work/match/bm4o3_c.py` (`--bm NAME...` masked diff with source, `--both`, `--order`, `--cnt N`, `BMD=1` compiles with SI_TRUE_ABI), `bm4o3_tv.py` (edit variants in parallel), `bm4o3_tf.py`, `bm4o3_rows.sh`.

## 2026-11-06, fourth lane

Figures: see script_interpreter.nonmatching.txt. New levers, by yield:
- **A by-value matrix goes through `ASSIGN_MAT`, right-hand side first.** `mission_cam_get_position` alone gained 24 cases and lost 1; the three `cam_frame` sites gained 10 and lost 1. Plain `script_pos(i) = get_frame()` computes the destination before the call.
- **A parameter the original keeps across a call is a named local taken before the call.** `troop_add_item`'s `int item` fixed its own case and rotated 6 more into place (`push edi` in the original). Same for `store_num_regular_prisoners`' `pid`. Elsewhere a named `int a = (int)params[k]` is neutral or wrong: `agent_get_position` needed it removed.
- **Branch sense.** `val_div` and `store_div` test `!= 0` first, `set_visitor` tests `!= -1` first, `store_attacker_count` / `store_defender_count` end in `!side`.
- `set_merchandise_modifier_quality` stores `sf_2` before `sf_1`. `troop_set_health` computes the troop in each arm, but cl still hoists the index to before the test; several spellings tried, not solved.
- `identical` swung from 444 to 414 on edits that left mod-stack+regs at 500 (`store_troop_kind_count`'s read order alone did it, and an unrelated local in `troop_add_proficiency_points` did too). Undoing the read order later, with two more edits in place, stayed at 414. This looks like the 6.2 group flipping, though which edit holds it now wasn't isolated. Judge by mod-stack+regs.

Equivalence findings (2026-11-06):
- **The harness cannot map `g_script_error`.** It is a file static, so our copy lives in the object's own `.bss`, not at 0x5e259c: ours writes 1 where the harness doesn't look, and reads 0 where a real callee wrote 1. That is the FAIL on execute, `set_lvalue` and `step_less` (`.data 0x5e259c`, orig 1, ours 0) and on `DialogBlock::emit` (`.data 0x5a9f8c`, 0x3b vs 0x34: the "At %s." log line isn't written). A symbols line can't help, because a static has no external name. Measured on a scratch copy with the variable `extern` at its real address: `set_lvalue` 169 valid, 0 mismatched; `step_less` WEAK, 275 valid, no mismatch; `emit` 159 valid, 0 mismatched (44% of instructions). The source stays as it is.
- `GameData::label_text` 0x49baa0: no logic difference was found by reading both sides (same constants, arguments and order). The mismatch was vertices off by exactly 2^8. The likely cause is the uninitialised w lane of `pos`, whose slot differed by 4 between the frames; that part is inferred, not traced. It's byte-exact now, and the run shows 73 valid, 0 mismatched, 100% coverage, cut short by the time limit (not a PASS verdict).
- `Mission::spawn_battle_icon` 0x49bbb0: the tools were testing the wrong function. `marker_names` also reads names from comments of the form `// 0x<va> word`, and the header comment `0x49bbb0  spawn the battle icon` bound the row to `EmitTable::spawn`. The comment block now writes `[0x...]`. Right symbol: PASS, 205 valid, all edges, 100%.

`DialogBlock::emit`: `check_script_param` has to be noinline (cl inlined it, frame 0x10f4 against 0xafc without it). `items[line]` is reloaded after every call and `line * 0x98` is kept in esi, so statements are written through `line`. Still open: the original enters the loop by `jmp` to the `idx < count` test, where ours threads the first test because idx is known to be 0; the three `set_lvalue` tails of 0x15..0x17 are one in the original; the key tests' layout (the 0x3b arm is last there).

## 2026-11-06, fifth lane

Figures: see script_interpreter.nonmatching.txt. 414 -> 460 identical, 500 -> 507 ignoring slots and registers.
- **The 6.2 flip group is pinned: `troop()` is the header's `__forceinline` accessor, not a macro.** Removing `#define troop` alone gained 43 cases and lost none (all troop, quest, faction and scene `[imul + ptr + k]` forms). It is not a count of formals or of used locals: 91 pass-through `__forceinline int f(int)` calls, chains of up to 800 copied locals and dead locals don't flip it (400 and 800 locals lose one unrelated case). Making `quest()` a forwarder flips troop and scene only from its 13th source site on, and `party()` or `mission()` as macros flip it back (-44, -19). `faction`, `quest` and `game` stay macros (removing each loses 2 to 4); `scene` is neutral either way. So a future edit that swings the group by about 30 is about forwarders, and one spelling flips it.
- The other direct gains: `party_prisoner_stack_get_troop_dna` tests `==` first with `g` taken after `first_flagged`; `troop_add_proficiency_points` is one `+=` with no pointer local; `party_get_skill_level` goes through `parties.get` directly, so the argument is pushed before `this` (the `party()` forwarder evaluates `this` first; that holds at any `party(x)->f(arg)` where the original pushes first). `team_get_order_position` is right-hand side first (`v4_ref`, as `ASSIGN_MAT`). `team_set_order_position` keeps `m` from before `point_in_rect` through `select_team`, and a named `t`.
- Tableau cases: `a.set(1.0f, 1.0f, 1.0f)` and `b.set((float)params[2], ...)` as expressions, not named x, y, z (+2 same in 1992).
- **`set_pos` 0x491e50 takes the position by value.** Called as `void set_pos(Vec2 at, int id, float radius)`, `party_relocate_near_party` is identical at once: the two dwords are pushed after `id` and the radius, from one `PartyMgr::get`. No two-float spelling and no forwarder can do that. Not applied: world_data defines the row with two floats, so the symbol would not resolve (`?set_pos@GameW@@QAEXMMHM@Z`).
- **The SET_LV tails, three forms.** 0x49fccb (Y form) is the only real hub, 46 jumps in the original and 42 here; every other tail has no predecessors. Per case the forms are: merged into the hub, unmerged X (`lea eax`), unmerged Y (`lea edx`). From 2199 to the end ours is a permutation of the original: orig merged = ours unmerged Y, orig X = ours merged, orig Y = ours X. A Y tail identical to the hub's stays unmerged here, so merging isn't textual on the final code. Spelling the SET_LV differently in 2200 changes nothing, and neither does correcting 2199's shape (below). Inferred, not shown: the state comes from earlier in the function, so fix earlier structural cases first and re-read this cluster (about 25 cases).
- `store_conversation_agent`: `int v` with its address taken gives the original's `je`, the surviving `mov [v],eax` and `cmp esi,[ebx]`, but makes v private (0xb4, frame +4, and the frame shift loses 67 slot-bearing cases; opaque `next_used` measures the same now, 67 lost with mod-stack+regs down only 1, so the old "opaque loses 56" was probably this frame shift too). The address has to reach a visible callee for v to pack, and no such call is known. A function-level `int v` also keeps the dead store and goes private.
- **Low slots.** Orig puts named scalars at 0x20 with the validator temporary at 0x10 (1492, 1696, 1994, 1995); ours gives the named one 0x10 and, in 1696, the temporary 0x20. That pushes 1996..2016, 2110..2112 and 2175's temporaries to private 0xac. Function-level declaration, a ternary and declaration order don't move it. The order isn't by declaration or code order; unsolved.
- `set_visitor`, `set_visitors`: orig `(scene*0x494 + k*8) + ptr`, ours `(ptr + scene*0x494) + k*8`; `scene()` as forwarder doesn't change it.
- A helper written between `// @ 0x49fa10` and `execute` becomes the marker's symbol: chk and equiv then test the helper. Keep helpers above the marker (next to `mat_ref`).
- Equivalence: `g_script_error` no longer gives a false FAIL. 195 valid, 0 mismatched, cut by the 180 s limit (UNSUPPORTED), coverage 0.7%.

## 2026-11-06, sixth lane

Figures: 461 -> 507 identical, 484 -> 514 ignoring slots, 508 -> 535 ignoring slots and registers, same size 533, size 69424 vs 69416, frame exact. Tools: `work/match/bm5o2_c.py` (bm4o3_c plus `--tails`, `--slot HEX o|m`, opcode and `i` slots in the summary), `bm5o2_tv.py`, `bm5o2_gen.py`, `bm5o2_rv.py` (variants of a non-execute row).

**Solved after the pause: a visible `first_party_id` (0x460d20) is the low-slot fix.** Opaque, it kills `params[1]` in ebx, so `end` takes ebx and stays out of memory. Visible, with 2311's `end` restored, opcode goes to 0x20, `i` stays at 0x28 and identical goes 485 -> 497. `AgentTable::first_used` made no difference (kept opaque), so 2199's count reload is still open and its cause isn't this. The next bullets are the record at the pause; where they say "probe" or "provisional", this paragraph replaces them. 2311 now has a block-scoped first `id` and a shared `end`; its bottom test is still a `while` with `if (n > pick) break`, where the original looks like do/while, but do/while loses the rotation that follows (-4). n at 0xac, end at 0x28 and the second id at 0x24 are still off.
- **Same mechanism, general rule: when the original reuses a value across a call and we reload, look for an opaque callee.** Visible `rgl::identity_pos` and `identity3` made get_scene_boundaries identical (identity3 alone doesn't).
- **A named index and no `dst` pointer** for `script_pos`: `int p = (int)params[0]; ASSIGN_MAT(script_pos(p), ...); script_pos(p).o = ...` made agent_get_look_position and party_get_ai_target_position identical, and helped prop_instance_get_scale and party_set_ai_target_position.
- Direct `g_mission_templates[...]` (no `t` pointer) at the store: +2 ignoring slots and registers. Direct `props[k]` instead of `prop_at` works in 1850 and 1851, but also doing 1855 flips the quest group (-15), the 6.2 flip again, so 1855 and 1860 keep `prop_at`.
- store01_random_parties_in_range is `while (b == a && n < 100000)`; store_partner_quest tests `(char)((flags >> 8) & 1)`. Both identical.
- `particle_systems` is a raw `PSys**` here and the original has `lea; mov [eax]`, so it is probably a real vector; 1696's dead `lea` is the same sign. Reference spellings don't produce it.
- **add_sprite 827 -> 15:** pairs copied as pairs (`pos`, dir from a temporary, `ai_target`, the 64-bit flags) and a by-value angle in `rot2`. Left: `imul`/`lea` scheduled around `fsin`. It reads x and y as one pair (stand-in, marked in the source).
- spawn_battle_icon: the original loads `mov ecx,edi` before calling 0x495b20, so that function is really a member with an unused `this`; mesh_hud defines it `__stdcall`. The ternary pushing per arm is unsolved: `tstring` in each arm gives 779.
- Camera::Camera: statement order and field-wise tail don't move `mov eax,edi`. deploy_party and emit were not worked on.

Record at the pause:
- **The SET_LV hub is a third form.** 0x49fccb is `cdq; lea ecx,[18]; ...` (Z), not the Y form the fifth lane recorded. X, Y and Z are one scratch register rotating, which is the permutation seen from 2199 on.
- **store_conversation_agent had one SET_LV after the join** (+18, no loss). cl duplicates it into both arms, so the original's two calls come from one source call. Left in 2199: `cmp esi,[ebx]` reload against our CSE spill of `count`, and `mov ecx,eax` after `get`.
- Notes: the troop note cases spell `((GameX*)g_game)->troops[i].notes[k]` instead of `troop()` (+2). Spelling set_visitor the same way doesn't help; there the original still loads the scene index before the pointer. Not solved.
- **Low slots: the cause is `end` in store_random_party_of_template.** Measured with `unsigned volatile end` in 2311 only (probe, not applied): opcode moves to 0x20 as in the original, `i` stays at 0x28, identical 497 (+17, -5, the 5 being the rotation fix below), ignoring slots unchanged. So `end` has to be memory-homed; 2310's `end` doesn't drive it. Keep a second `end =` def in pass 2: the same probe with `deflt` read inline in pass 2 doesn't flip.
- Inferred, not shown: `end` takes ebx because the `params[1]` value in ebx doesn't reach the first loop here (we reload `mov ecx,[34]`, the original uses ebx after `get` and `next_party_id`). So something in that loop kills it for us and not for the original. Next check: `first_party_id` isn't defined in this file, so it is opaque here, while `next_party_id` and `get` are visible. Try a visible body for `first_party_id` (and `AgentTable::first_used`, which could also explain 2199's reload). I had only located this when paused; nothing tested.
- **2311 in the source is provisional.** `while (id < deflt && n <= pick)` was applied because it fixes the rotation after it (+4, str_store_*), but its shape is further from the original (n=88/89, same 56 vs 59). The original's shape is `n <= pick` first with `end` hoisted, as in the earlier do/while. Expect to restore that once `end` is solved.
- Not stand-ins, not applied: no volatile is in the source.
- Also seen: store_partner_quest tests a bit by shift (`mov ebx,[ecx+0x14]; shr ebx,8; test bl,1`), the obvious spelling didn't change it. store01_random_parties_in_range looks like `while (b == a && n < 100000) { ++n; b = ...; }` (not tried). The 2380..2385 tails are the same rotation, starting at or before 2360. A single-SET_LV form of 2383/2384 flipped the whole function (identical 398, ignoring slots and registers 455), cause not looked at, not applied.

## 2026-11-06, Fable lane

Figures: 507 -> 512 identical, 514 -> 520 ignoring slots, 535 -> 543 ignoring slots and registers, same size 533 -> 542; size 69492 vs 69416 (the join cases grew to the original's sizes), frame exact. Tools: `work/match/bm5f_c.py` (bm5o2_c plus `--raw NAME` side by side, `--grep REGEX` over the original's case text, `--lru`), `bm5f_tv.py` (also patches script_interpreter.h), `bm5f_mk.py file.py NAME` (writes the variant for `bm5f_c.py <file> --raw`).
- **hero_can_join family: a `GameX* game` local, and the first call reads the global.** `GameX* game = g_game; int main = game->main_party; PartyMgr* m = &game->parties; stacks = party(game()->main_party)->max_stacks();` gives the original's `mov edx,esi; push edx; lea ecx,[eax+0xf8]; lea edi,[eax+0xf8]` (the formal is a copy of the CSE'd load, and `m` comes from the local pointer so it is not CSE'd with the forwarder's `g_game->parties`). All four are identical modulo callee-saved registers now (R).
- **1534 troop_inventory_slot_set_item_amount identical: `int missing = ...max_amount() - (int)params[2];` before `r.b.amount = 0`.** The params[2] read has to precede the byte store (it may alias); with it, each arm's tail is 19 bytes and cl does not cross-jump them (ours merged a 23-byte tail).
- **party_wound_members identical: no `pm` local, `party(p)` everywhere.** The original's `add esi,0xf8` comes after `found = -1` and `i = 0`: it is the CSE temp of the forwarder's `g_game->parties` (visible `get` keeps it live), created at the first use.
- **mission_time_speed_move_to_value identical: `MissionX* m = mission(); if (m && ...)`.** The variable must be the first use. With `if (g_mission && ...) { m = mission(); }` the test's load is a CSE temp and `m` a copy of it, and the copy lands after the test (`mov eax,[G]; test; ...; mov esi,eax`). The same shape (load into a scratch, use, copy to a callee-saved register) is still in 1630, 1696, agent_set_team (`mov eax,[s34]; push eax; mov esi,eax` where the original loads esi): a forceinline forwarder for the call (F2) and declaration order did not fix those; the first use is a push there, not a test.
- set_relation: `sf_2 = 1.0f, sf_1 = -1.0f` (1.0 stored first), and `si_0 = ..., si_1 = ...` inside each message arm (the original stores them after the `jge`/`jle`). Same instruction count now; the rest is scratch and callee-saved rotation (`mov eax,[G]` is 1 byte shorter than `mov ecx,[G]`, so the size differs by 11).
- 1942 mission_tpl_entry_add_override_item: `int& n = ...num_override_items;` gives the original's dead `lea edx,[edx+ecx+0x24]` (a materialised reference) and `n` in esi. Same shape; registers left.
- 1633/1634: the original's `jl` goes to `xor eax,eax; jmp 0x49fccb` and the else arm ends `sub eax,ecx; jmp 0x49fccb`: one `SET_LV((__int64)v)` with an `int v` after the join, `members` spilled at 0x10 because `p` keeps ebx. Written that way (C1) it loses 5 cases and grows; C2/C3 unchanged. Not applied.
- Not working: set_visitor/set_visitors `(scene*0x494 + k*8) + ptr` (an `extern int g_visitor_scene` declared first, a `visitor_at(s, k)` forceinline helper, direct spelling, a `scenes` local reloaded per statement: all unchanged); troop_set_health (the hoisted `params[0]*0x508` comes from a head merge of the arms' identical prefixes, which the original's arms do not have because their scratch registers differ; H1..H4 unchanged or -7); player_has_item `lea ebp,[ecx+edx]` (Q1..Q4 unchanged); `std::vector<PSys*> particle_systems` in SceneEntity does not give the `lea; mov [eax]` (VB).
- **Callee-saved pick order is not LRU** (`--lru`): the original's picks per case against the last-use order of ebx/esi/edi/ebp show no relation. Declaration order does move it: in hero_can_join, `m` before `main` gives m=edi (right) but swaps main/stacks (esi/ebx). The original's order there is m, stacks, main, i.e. `main` is processed last; cause unknown.
- 2201 store_partner_faction, 2323, 2325: the original cross-jumps into a neighbour's tail (`mov edx,[g_dialogs.fc]; jmp` into store_troop_faction's `mov eax,[G]; imul edx...`; 2323 jumps after check_troop into 2322's tail; 2325 after `mov ecx,[s34]` into 2326's). Ours has the identical text but cl did not merge them; which tails merge seems to follow the scratch rotation state.
- Tool artefact fixed: jump-table `jmp dword ptr [r*4 + table]` was compared unmasked (change_screen_notes was D for that alone).
- **position_set_z_to_ground_level identical with no `m` local (`mission()` everywhere) plus `prop_at` as a macro.** Each alone flips {add_quest_note_from_dialog, add_quest_note_from_sreg, class_is_listening_order} (-3); together they cancel (512 identical). So two flip-group edits can be paired: when a right edit loses a group, look for a second edit that flips the same group.
- agent_has_item_equipped: `agent(a)` forwarder, no `t` (the `result = 0` store then precedes `add edi,4`); agent_set_scripted_destination: no `o` pointer and a named `float z` (the original re-reads params[1] for the z store after the opaque floor_z call; a CSE'd `o` let ours keep the address); team_set_order_position: no named `t` (the original reads params[0] after the opaque calls). All three closer, none identical.
- **Split webs (open).** 1630, 1696, 1771, 1210, 2344: a named `int x = (int)params[k]` is loaded into a scratch, used (push, `or`, `imul`), then copied to a callee-saved register; the original loads the callee-saved register and copies to a scratch for the use. Tried without effect: a forceinline forwarder for the call (F2), declaration before the validator (G2), `*(int*)&params[k]` (LOW: flips 16 SIB cases and merges 2201's tail instead), `__int64 p` (P64, worse). hero_can_join gets the original's copy from the forwarder's formal, so the copy direction follows what the IL has, not a spelling of the load.
- Accessor toggles re-measured (S1..S7): `scene` inline and `party` as a macro are neutral now (the latter was -44 earlier), `faction` inline -2, `quest` inline -6, `mission` as a macro -50 (but it merges 2201's tail and fixes agent_has_item_equipped's alignment).
- Rejected: opaque `rand_below` (RB: identical 60, the register map collapses); `i` as the loop bound in 2310/2311 (I1: `i` moves to 0x2c, -408); `mission()->agents.count` in 2199's loop test (T3: -20).
- 2311's original has `n` at 0xac (private: its address goes to rand_below and it is read after), `end` at 0x28 and the second `id` at 0x24; ours has `n` at 0x28, `end` at 0x24, `id` at 0xac. 2310's `end` is at 0x28 there (ours 0x18). Slot 0x28 is shared by `i` and case locals on both sides (16 cases there, 13 here), so `end` is a separate local that joins `i`'s group in the original.
- troop_loot_troop: only `n` (ebp there, `[esp+0x20]` here, so ours hoists the 0x3000 mask into ebp) and the alignment differ; `do/while (--n)` and a `flags` local change nothing.
- 2380/2381: the original's store_enemy_count is `push 1; call count_agents; jmp 0x49fccb` and store_friend_count is `push 0; jmp` into a later `call; jmp` tail (in store_attacker/defender_count); ours has the merge direction swapped (friend_count is the full copy). 2370/2371 are the same swap (timer_a full there, timer_b merged into the 0x49fccc hub; ours the reverse). The merge direction is the cross-jumper's choice, not a spelling.
- Equivalence after this lane: 36 valid, 0 mismatched, bit-identical, 0.6% coverage (the 180 s limit; UNSUPPORTED as before). run_all: 40 / 40.
- **What remains, by cause (78 cases):** callee-saved pick order (36, 101, 102, 105, 106, 150, 1530, 1667, 1920, 2110, 2111, 2055 and the esi/edi halves of 1270, 1794, 1791); scratch rotation and the SET_LV tail form X/Y/Z it selects (1210, 1792, 1793, 1906, 1910..1912, 1935, 1936, 1940, 1941, 2360, 2372, 2385, plus the cross-jump direction in 2201, 2323, 2325, 2332, 2370, 2371, 2380..2384); split webs (1630, 1696, 1771, 1210, 2344); slot assignment (1722, 1980, 1989, 1992, 1993, 2158, 2310, 2126, 2311, 2159, 1791's si_0); address-tree order in the set_visitor family (1263..1265); troop_set_health's hoisted index; player_has_item's `lea`; the materialised vector reference in 1968 and 1696; 1855/1860 evaluating the position argument before the prop lookup; 2199's count reload; 2154/2155 and 1730's unfolded memory operand; 1633/1634's single SET_LV after the join; loop-head alignment padding that follows from earlier sizes (1729, 2180, 1121, 1225 are one scheduled instruction each).

## 2026-11-08, register-rule lane (bm6o)

Tools: `work/match/bm6o_probe.py file.cpp [--asm FN]` (compiles a probe file, prints per function the register each `gN`
load lands in), `bm6o_gen.py` / `bm6o_gen2.py` / `bm6o_gen3.py` (random probe generators), `bm6o_score.py` /
`bm6o_score3.py` (score candidate rules), `bm6o_grid.py VARIANT` (two-value exchange-rate grids, `NC=600` wraps the
probe in a 600-case switch), `bm6o_mx2.py` (mini-execute probes with long-lived webs), `bm6o_sub.py CASE --keep a:b`
(execute with only some case blocks), `bm6o_tv.py` (bm5f_tv against `bm6o_base.pkl`, the baseline of this lane).

**Callee-saved pick rule, measured in small functions (docs/codegen.md section 7, "Callee-saved register order"):** webs are colored in priority order and
take the first free callee-saved register in the fixed order **esi, edi, ebx, ebp**. Priority = references of the web
(defs count as well as uses), highest first; ties go to the web defined first. Declaration order does not count.
- 150/150 random straight-line probes (4 values); 150/150 with declaration order shuffled apart from assignment
  order; 150/150 with values used as pointer bases. Every other key scored 14..87/150 (first use, last use, value id,
  live-range length, uses/length, declaration).
- A named variable that holds a call result counts its copy and the two-address result it is coalesced with:
  `int b = q(1); f(b - r(2));` counts 4, `f(q(1) - r(2));`'s temporary at most 2 (bm6o_nt.cpp). hero_can_join's
  original order (main, m, then the max_stacks value) is exactly what the temporary form gives in a probe
  (hcj_v1), and every spelling with a named `stacks` gives ours (hcj_w1..w8).
- `x += y` counts 2 (a use and a def). `x = q(x)` starts a new web each time.
- Uses under conditions count less: one `if` about 1/2 (exchange boundary m = 2n), two levels, `a && b`, or a
  second `if` after a join about 1/3, the same at three levels; an `else` arm 1/2; loop bodies do not count more
  (counter webs compete instead). A 600-case switch around the probe doesn't change any of this.
- In a switch, long-lived webs (this, locals, params dwords) are dead in cases that don't use them, so a case's values
  take the free registers in priority order (bm6o_mx2: every probe followed this).

**But execute's own picks don't follow from the case text.** Compiled alone, our entering_town source gives the
original's registers (game=ebp, mp=ebx, pm=edi: bm6o_et.cpp), and our party_count_members_of_type source loads `t`
and `p` straight into callee-saved registers as the original does (bm6o_sw.cpp, the split web is gone). In execute
both come out as ours. Compiling execute with only some case blocks (bm6o_sub.py) moves entering_town's pick
non-monotonically with the subset: blocks 0..40 and 0..80 give game=ebp with mp rematerialised, 0..60, 0..100,
0..125 and 0..150 give ours. In isolation one extra top-level use of `game` flips it to ours, two deep-conditional
uses don't, so game and mp are close. So the R and split-web cases come from function-wide state (what the other
cases do to the weights or the tie order), not from their own spelling, which is why about 35 local spellings failed.
The case count isn't that state: the grid boundaries are the same with the probe in an 8-, 137-, 591- or 600-case switch.
It isn't the 6.2 id flip group either. `mission()` as a macro (S7: -50 identical, the whole flip group) and `party()` as a
macro (S5: neutral) leave entering_town, 1667 and 1630 byte-for-byte as they were (bm6o_v5.py). So the state is
stable against symbol-id changes but differs from a lone compile. The next thing to check is the long-lived webs
around a case (which callee-saved registers are really free there). The mini-execute probes say all four are free
when unused, but they don't reproduce the full function's shapes (mp rematerialised, params[0] not in esi).

hero_can_join in execute (variants in bm6o_v1.py, bm6o_v4.py, all measured against bm6o_base.pkl, none applied):
- The rule says the max_stacks value should be a temporary, not `stacks` (the probe gives the original's order then).
  In execute every temporary form (no `stacks`: NS2, NS4; `-ff + max_stacks()`: T1; `party(main)`: T6; `+ 0`: T7)
  makes cl evaluate `m->get(main)->first_flagged()` first and keep `g_game` in esi across it. The probe doesn't
  reorder. NS2 511/540 msr, NS4 511/538, T1 513 identical but 541 msr (it gains agent_has_item_equipped and turns
  hero_can_join from R to D), T6 512/541, T7 511/540. The named `stacks` stays: it's the only form that keeps
  the call order here.
- Also unchanged in execute: entering_town with `game` renamed (E1), `pm` declared before `mp` (E2), `mp` declared
  first (E3); 1667 with a named `PartyMgr* pm` (P1, P1b); start_mission_conversation with a named argument or a
  direct `g_mission` (P1b, S1).
- start_mission_conversation is scheduling, not priority: the original pushes params[0] (esi) before loading
  g_mission, so esi is free for `m`. Ours loads `m` first, so `m` gets edi.

## Open problems

0. Scratch registers: **cause found.** The rotation starts at the first temporary: `mov ecx,[edi]` for `g_cur_opcode = id` in the original,
   `mov eax,[edi]` otherwise. Eax is taken because the zero web (`xor eax,eax`) already exists there: `g_script_error = 0` comes first in the
   source, and the scheduler sinks its store below the `id` loads. It may do that only if `g_script_error` cannot alias `*this`, i.e. it is a
   **file static** whose address is never taken. The image agrees: every reference to 0x5e259c (and to 0x5e2590..0x5e25a4) is inside
   0x49cc60..0x4b10d7, one translation unit that also holds the block runner now in party2/camera.cpp. With an extern the rotation is right but
   the store stays at the top of the function (four prologue instructions differ). `#define SI_ERR_STATIC` selects the static; it cannot be the
   default until camera.cpp's users of `g_script_error` (report's copy, DialogBlock's runner) live in this object, because camera.obj needs the symbol.
   The rotation is then a chain: each scratch pick depends on the temporaries before it in IL order, across cases, so one case with an extra or
   missing temporary rotates the following cases until something resynchronises. Identical-case counts jump around for this reason (a single
   fix lost 49 cases and gained 20); fix cases top-down, and judge edits by "mod-stack+regs" as well as "identical".
   Measured by compiling a generated copy with `SI_ERR_STATIC` defined (the source's relative includes rewritten).
   Other spellings found: pure visible callees can be reordered, so `Party::max_stacks` (0x4b1700, after this TU) stays opaque; false-block hub:
   `result = 0` tails all jump to `ge`'s block in the original, which needs every other tail in `if (ok) result=1; else result=0;` form;
   by-value `__forceinline` wrappers spill their arguments (call the real signature, as map_get_*_position does now); RHS-first assignment
   (`ASSIGN_MAT`, `ASSIGN_STR`).
   **Settled in the 2026-11-06 round.** `result` is address-taken in the original (`char* result_addr = &result;` here): a char store through an address-taken local may alias, so `main_party_has_troop` reloads `num_stacks` and keeps `i` in memory.
   Its counter is the function's `i` (slot 0x28, also the operand loop's). The `mov edx,[i]; inc edx; ...; mov [i],edx` that looked like a fold problem was rotation: `inc dword ptr` came from the rotated state, and the case is identical once the wrapper fix below is in. No spelling of the increment was needed.
   The 68 cases lost whenever that loop was right were compensation between wrong cases, not a real cost.
   **Levers found, by yield:**
   - A `__forceinline` forwarder or accessor evaluates `this` and pointer expressions into temporaries *before* the call's arguments, which rotates scratch registers. `((GameW*)g_game)->can_transfer(g_dialogs.f10, ((GameX*)g_game)->main_party, 0)` made party_can_join, party_can_join_as_prisoner (with the original's `push 1; jmp` cross-jump) and party_can_join_party exact.
     So execute now calls game and presentation methods directly (77 sites), and `game()`, `faction()`, `quest()`, `scene()` are macros for the length of execute only (`troop()` no longer: see the fifth lane) (`#undef` right after it). Measured and rejected: macros for `mission()` (loses 35), `party()` (loses 1), `script_pos()` (loses 15); removing PartyV's no-argument forwarders (loses 2); direct calls on `mission()` (neutral).
   - A callee the original sees: esi/ebx survive the call. `Troop::inv_count` + `Troop::nibble` (+7) and `Mission::team_count` (+1) are now visible duplicates. `vec2x::rotate_b`, made visible, keeps the angle and index in eax/edx across the call where the original has esi/edi, so it stays opaque. That is a usable test: **callee-saved registers across a call suggest opaque, caller-saved ones visible**.
   - Use the real container operation. `warn_scene_prop_instance` matched at once with `value >= mission()->props.size()`, after the hand-written `_Mylast - _Myfirst` spellings had rotated it.
   - A `||` group places a shared tail where separate `if`s don't, and re-testing `f & 0x40` rather than `a` stops jump threading (`check_slot` 0x49c170).
   - A base pointer read through a reference (`rgl::matrix& p`) gives `add esi,IMM`; a local index gives the original's `shl esi,6; lea ecx,[esi+IMM]` (position_rotate_z). position_get_rotation_around_z takes `rgl::vector4& f = script_pos(i).f`.
   - `ASSIGN_MAT` gives the "copy into pos[p0]" hub the original jumps to (715, 716, 1850, 1851).
   `hero_can_join` and `troops_can_join` are still open, but no longer look like the key to the chain: changing the temporaries there did not move the following case's registers. Evidence for the next attempt: `lea rN,[eax+0xf8]; lea rM,[eax+0xf8]` occurs **six times in .text up to 0x54a000**, all in these four cases and party_get_free_companions/prisoners_capacity (1633, 1634), and `mov r,callee-saved; push r` only in these four within execute.
   All six follow one order: get (fresh) + max_stacks, get (cached pointer) + first_flagged, get (fresh `g_game`) + sum_head, get (cached pointer) + max_members. The cached `parties` pointer survives the opaque max_stacks, so it looks like a source variable, yet the first call's `this` doesn't merge with it. Tried without success: about 20 spellings (wrappers on GameX/PartyMgr, reference and pointer locals, assignment inside the argument, address-taken `main`, one-expression forms, first_flagged first). In one expression cl evaluates first_flagged first, as the original does at 0x4aca11 (a different case).
   Also open: storing clamp_index's result straight to an absolute address (`g_mission_ctx.f1c`, `g_or_latch`, `g_script.reg[0]`) gives set_battle_advantage private slots, where a store through `g_game` shares 0x10/0x20/0x24. The original stores to the absolute address and shares. Block scope and the negative constant were tested and aren't the cause. Those three private dwords sit just below `types`, whose offset (0xbc here, 0xb4 there) contributes to set_result_string and its family staying at "S"/"R".
1. **Frame layout**: size solved (0x1ec0, see above). What follows is the earlier list of private slots, kept for history:
   `set_battle_advantage` (same source as `party_set_morale`, which packs; cause unknown), `spawn_item`'s ItemRef (visible `set_modifier` alone does not help), the
   `cur_tableau_add_*` and `cur_*_set_tableau_*` temporaries, `prop_instance_animate_to_position`, `race_completed_by_player`. The validator temporary leaves slot 0x10 at case 1993 in the original and about 2100 here.
   Dropping cases only removes their own slots: no sign of a cumulative packing limit on this source.
2. Validators: the 65-byte range checks differ by the argument register (EAX vs the original's ECX) and the
   register of the `g_cur_opcode` load; `check_party`, `warn_tableau`, `warn_scene_prop_instance`,
   `warn_agent` and `check_string_arg` need register conventions cl only chooses itself.
3. `trigger_item_mesh` (0x4904a0) takes EAX in the original; a plain call here (3 call sites).
4. Resolution of globals above 0x1900000 (`g_dialogs` 0x8ed50c8 and its fields, `g_in_meta_mission`
   0x8ee30ec, `g_skin_count`, `g_meshes`, `g_mesh_count`, `g_particle_system_count`, `g_particle_mgr`) needs
   `symbols/` lines, which are outside this module: the facts are in `work/match/scratch_si_symbols.txt`
   (format of `symbols/slices/*.txt`) for the orchestrator to copy to `symbols/slices/script_interpreter.txt`.

## Translation unit

The original object runs at least from 0x49a0b0 to 0x4b1xxx; every reference to 0x5e2590..0x5e25a4 lies in it. party2/camera.cpp is merged here, so `g_script_error` is a true file static.
`GameCompanion::create_companion_party` is a local view because include/game_data.h still declares the old three-float signature: move it to `GameData` when that header is next edited (no other caller exists).
