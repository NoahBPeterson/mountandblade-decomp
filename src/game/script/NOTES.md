# script_interpreter: the Module System operation interpreter (0x49fa10)

`Operation::execute(__int64* locals, int* context, int* error)` (thiscall, `ret 0xc`, 69416 bytes with its
jump table) runs one operation of a script, trigger, menu or dialog block. It is the largest function of the
game: a `switch (opcode - 30)` over a 2356-entry jump table at 0x4ae458, 591 real cases (the other labels go
to the "Unrecognized opcode %d." default). Control opcodes below 30 (`call_script`, `try_begin`, ...,
`store_script_param_*`) are run by the block runner (0x4b09e0) and never reach it.

Files:

| file | content |
|---|---|
| `script_interpreter.cpp` | the validators 0x49cc60..0x49d4b0 and `Operation::execute` |
| `script_interpreter.h` | every record, global and callee the interpreter touches (views of the shared headers, see below) |
| `script_opcodes.h` | `namespace op`: every header_operations.py name as an enum value, plus the flag bits `op::neg` (0x80000000) and `op::this_or_next` (0x40000000) |
| `script_interpreter.manifest.txt` | byte-exact rows (report and four validators) |
| `script_interpreter.nonmatching.txt` | `execute` and the other 18 validators: complete, not byte-exact |
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
- `chk.py src/game/script/script_interpreter.cpp 0x49fa10`: 69100 vs 69416 bytes, 46168 differing bytes.
  `run_all.py --manifest src/game/script/script_interpreter.manifest.txt`: 5 / 5 byte-exact.
- Resolution audit (resaudit.py's `audit()` driven by `work/match/scratch_si_resaudit.py`, with
  `work/match/scratch_si_symbols.txt` for the globals above the annotation range, see open problems):
  the five byte-exact rows agree completely. `execute`: 280 references agree, 46 unverified (strings, our own
  inline helpers, callees used once in code that does not align), 22 flagged "disagree"; every flagged one
  was checked against the original case and is an alignment artifact of the 70 KB stream (neighbouring cases
  with the same shape paired off by one: the five quest calls, check_faction/check_scene/..., the overlay
  creators), an argument evaluation order difference (`party_join`: the original builds the party name before
  the UI lookup) or an addressing-form difference (`pos[i].f` as `0xa708a0 + i*64` vs `0xa70890 + 16`). No
  wrong callee or global was found.

## Open problems

1. **Frame layout** (the bulk of the byte diff). The original packs nearly all temporaries and block locals
   into a handful of slots (validator argument temps at esp+0x10 or 0x18, the set_lvalue value at 0x18,
   clamp/`vector4::set` arguments at 0x10/0x20/0x24/0x28 reusing the dead `opcode` and `i` slots) and keeps
   params at 0x2c, types at 0xb4, or_next at 0xf4, neg at 0x100, the message buffer at 0x920 and four 0x400
   link buffers above it (frame 0x1ec0). cl 13.10 does the same packing on parts of this file (any 300 cases
   compiled alone pack well) but gives up as the function grows: compiled whole, ~200 named locals and ~200
   temporaries keep slots of their own (frame 0x2630; every stack displacement then differs). Synthetic tests
   reproduce the cut-off (a switch with ~600 cases of three address-taken locals packs into 4 slots, ~700
   cases into none). Declaration order does not change the layout. The original therefore had fewer packing
   candidates than this source; finding the spelling that gets under the limit (fewer named locals passed by
   address, more temporaries, or shared function-scope scratch variables like `lv_`) is the next step.
2. Validators: the 65-byte range checks differ by the argument register (EAX vs the original's ECX) and the
   register of the `g_cur_opcode` load; `check_party`, `warn_tableau`, `warn_scene_prop_instance`,
   `warn_agent` and `check_string_arg` need register conventions cl only chooses itself.
3. `trigger_item_mesh` (0x4904a0) takes EAX in the original; a plain call here (3 call sites).
4. Resolution of globals above 0x1900000 (`g_dialogs` 0x8ed50c8 and its fields, `g_in_meta_mission`
   0x8ee30ec, `g_skin_count`, `g_meshes`, `g_mesh_count`, `g_particle_system_count`, `g_particle_mgr`) needs
   `symbols/` lines, which are outside this module: the facts are in `work/match/scratch_si_symbols.txt`
   (format of `symbols/slices/*.txt`) for the orchestrator to copy to `symbols/slices/script_interpreter.txt`.
