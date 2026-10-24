# src/game/map_editor migration: the map editor update and the end-of-code screens (0x5305b0..0x5342a6)

Region of the TU map (`work/migrate/tu_map.csv` TUs 118 `map_editor_dialog`, 119 `party_3`, 120 `screen`); the end of the
game code (0x5342a6, the static CRT follows). T regime: every module is in binary order and every member another function of
the file calls is `__declspec(noinline)` (declared so in the header). The staging slices (`s005305b0`, `s00531380`,
`s00531800`, `s005324d0`, `s00532de0`, `s005337a0`, `s00534160`) are unchanged. Flags `/O2 /MT /Gy /TP` for all rows.

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| map_editor.cpp | 0x5305b0..0x531590 | 5 | 2 | 2 | 3 | 0 |
| exchange_screens.cpp | 0x531590..0x5342a6 | 21 | 11 | 11 | 10 | 0 |
| **total** | | **26** | **13** | **13** | **13** | **0** |

No regressions: the 13 byte-exact VAs of the slices are the 13 rows of the two manifests (`run_all`: 2/2 and 11/11).
Every function that had source still has it; nothing is partial. Improved nonmatching rows: 0x5337a0 `on_tick` 402 -> 12 diff
bytes (same size; two real source bugs, see below), 0x532740 `swap_rows` 135 -> 100, 0x5325b0 `apply` 199 -> 144, 0x533070
`on_event` (rewritten: it was 192 bytes short), 0x5305b0 `update` 2916 -> 2891. 0x532830 `stack_remove` and 0x532de0
`update_party_sprites` have more differing bytes than in the slices (1043 -> 1092, 488 -> 497) because they now use the vector
page load of the canonical `PartyMgr` (the original's `lea` + load, which the slices could not produce): the structure is
closer, the diff count is not a similarity measure there.

Module name: `exchange_screens`, not `party_screen`, because `src/game/inventory_panel/party_screen.cpp` exists and the
sweep / equiv tools key on the module stem.

## Boundary decisions

Hard evidence: none. TUs 118 / 119 / 120 are class-name changes only (T regime: forward calls prove nothing), and the
only inner structure is that the 8 per-frame hooks of TUs 119 / 120 sit in the first virtual slot of 8 different vtables
(0x595810, 0x595858, 0x59587c, 0x595a08, 0x59619c, 0x5961cc, 0x5961f0, 0x596214; found by scanning .rdata for the function
addresses), i.e. they are the members of 8 screen classes, not one `Menu` / `Mission` / `Screen` / `GameObj` as the slices had it.

- **0x5305b0 starts `map_editor`** (class `MapEditorDialog`, 5 functions up to 0x531590). It is separated from the screens
  after it by the class change at 0x531590 (`Menu::select`, vtable 0x595810). Weak.
- **0x531590..0x5342a6 is one module** (`exchange_screens`): TU 119 (party_3, 19 functions) and TU 120 (screen, 2 functions
  0x534180 / 0x534230 + the iterator helper 0x534160) merged. The two `screen` functions are hooks of screens of the same
  family as the rest (`MissionSource` embedded at +0x30, option id at +0x18), and 0x534160 (`vector<int>::iterator + n`,
  out-of-line) is called from 0x533aa0 (before it) and from TU 120's family 0x4d5xxx / 0x516xxx, so it belongs to no
  smaller unit. A wrong split would cost one file move; the file keeps the TU order so it can be cut at 0x534180.
- The post-CRT initializer 0x547e40 (global 0xa73300, `MenuCtl`) and its atexit thunk 0x548200 are not part of the code
  region and are not in the modules.

## Classes

| here | slice names | evidence |
|---|---|---|
| `MapEditorDialog` (canonical, `src/rgl/mesh2/gui_edit.h`; **extended**) | `MapEditorDialog` | `update` 0x5305b0 |
| `PlacedObj` / `Rec1c8` (canonical) as the record the editor edits; `rgl::entity` (canonical) as its scene object | `MapRecord`, `MapEntity`, `SceneFrame`, `PlacedObj` (slice-local) | stride 0x1c8 (`Mission::props`), `PlacedObj::m1b4` is an `rgl::entity*` (frame +0x58, flags +0x140 / +0x141, vtable slot 0x5c = `set_local_scaled_dirty(matrix*, vector4*)`) |
| `MapMission : Mission` | `Mission` | `remove_prop` 0x5313c0 (vtable-less view: only member declarations and typed views of the ints at +0x65d0 / +0x65d8 / +0x65dc) |
| `PropVec : std::vector<Rec1c8>` | `PropVec` | out-of-line `erase` 0x531470 |
| `pos3 : rgl::matrix` | `pos3`, `MatFrame` | `mul` 0x40e870, `move_forward` 0x531380 (called by `update` with a local) |
| `StartMenu`, `ContinueMenu` (share `MenuBase`: buttons +0x30, options +0x94, count +0x114) | `Menu` | vtables 0x595810 / 0x595858 |
| `MenuCtl` (object 0xa73300) | `MissionUi`, `Screen`, `AuxObj`, `g_mui` | setup_entry / weight read +0x10 .. +0x160; 0x513f80, 0x513b40, 0x514890 are its members |
| `XferWin` (party exchange window, vtable 0x59587c) | `Mission`, `Party`, `GameObj` | 0x531800 + clear_selection / get_cell / move_men / apply / swap_rows / stack_remove / set_selection all index the same grid (+0xb8), slots (+0x4c8), cursor (+0x8dc..) |
| `PartyX : Party`, `PartyMgrX : PartyMgr`, `GameDataX : GameData` | `Party` (as the window!), `PartyMgr`, `GameObj` | only member declarations of this region; 0x532db0 `compact` is a real `Party` member; 0x532de0 runs with `this` = the game data |
| `TickScreen` (0x595a08), `InputScreen` (0x5961cc), `EventScreen` (0x59619c), `SourceScreen` (0x5961f0 / 0x596214) | `Screen`, `GameObj` | vtable slot 0 |
| `ListWidget : Widget` | `Widget` | `get3` 0x533a60 (+0x19c / +0x1ac arrays, +0x1cc index) |
| `FrameFlags` (0x5e291c), `KeyDown` (0xa73730), `MenuItem`, `MenuEntry`, `TplRec`, `TplOpt`, `VecIter`, `OutIntVec : IntVec` | `Flags`, `FlagSet`, `KeyTable`, ... | local, none exists elsewhere |

Canonical headers included: `src/rgl/mesh2/gui_edit.h`, `src/rgl/mesh.h`, `src/rgl/scene/world.h` (`rgl::world`,
`rgl::entity`), `src/include/mat4.h`, `game_data.h`, `party.h`, `troop.h`, `msg_log.h`, `rec1c8.h`, `dialog_rec.h`
(`DialogBlock::exec` 0x4875e0 is the option emit of `on_event`), `src/game/mission.h`, `placed_obj.h`, `int_vec.h`
(`push_back` with the inlined fast path and the library `insert_n` 0x43e7b0), `str_table.h`, `options_panel/hud.h`
(`Widget::used`, `ValueSource`, `MainMenu`, `MissionSource`, the mouse / clock / scroll globals), `ui/ui_common.h`
(`g_ticks`).

**Canonical header extended (pure additions at unused offsets, nothing moved):** `src/rgl/mesh2/gui_edit.h`,
`MapEditorDialog`: the former `char pad[0x374]` is split to name `cam_x/y/z` (+0x248/+0x24c/+0x250, proven by 0x5305b0:
negated into the editor frame), `picked` (+0x2cc), `f2f8` / `f2f9` (+0x2f8 / +0x2f9) and the members `update` (0x5305b0),
`add_selected` (0x5314e0), `select_entity` (0x4ea0c0), `handle_input` (0x52fe40). `gui_edit.cpp` still builds 49/49 byte-exact.
Not edited (they had uncommitted changes of other agents when this ran, so the missing offsets are read through local
accessors / derived views, each citing its function): `game_data.h` (editor fields +0x1a164 frame, +0x1a1a4 position,
+0x1a1b4 selected, +0x1a1b8 selection list, +0x1a1c8 / +0x1a1cc: `ed_*()` in map_editor.h; `Troop::f54`; stack ids
+0x14058), `mission.h`, `party.h` (`PartyX`), `troop.h`.

Conflicts worked around: `gui_edit.h` `MapRecord` has a `tstring` at +0x18 where the record's frame is (0x5305b0 copies the
0x40 bytes at +0x18 as a matrix; the string is at +0x190 in `PlacedObj`), so `PlacedObj` is used as the record and
`MapRecord*` appears only as `void*` in the dialog's member declarations; `gui_edit.h` `Target` (a vtable-slot-0x5c
`set_name`) is the same object as `rgl::entity` slot 23. `Mission` in `char_sheet/dust.h` / `conv_open.h` are other
partial views (not included). `src/game/inventory_panel/party_screen.cpp` (another agent) is unrelated.

## Bugs found in the slices (all fixed here)

- **`set_value` arity** (0x5337a0, 0x533070, 0x533aa0): `ValueSource::set_value` 0x48baa0 is `ret 4` (one float); the
  `push ecx` before the expression is the float's stack slot. The slices passed a second argument. Fixing it took
  `on_tick` from 402 diff bytes to 12 and removed the equiv FAIL (`fake-arity`).
- **`operator delete` vs `free`** (0x533aa0): the original's free blocks call 0x535712 (`operator delete`), the slice called
  `free`; the byte compare masks the call target, the resolution audit does not.
- **`on_tick` commit flag** is a `char` (frame 0x10), not an int.
- **`on_event` (0x533070)** was rewritten against the disassembly: the menu row's key word at +0x20 is 64 bit (the slice
  loaded 32), the scroll value block is duplicated per widget (0xec / 0xe8), the state-0 handler (`f18 == 100`: commit) is
  reached only after an emitted option (a hotkey or no pick returns when no command is queued, which the slice got wrong),
  and `v` in the 5 / 7 / 6 case starts at 0 with the dead 9 / 10 branches kept. 1635 -> 1809 of 1827 bytes.
- `PartyMgr::get(pid, slot, 0xd2)` (0x531800) is a call to 0x51ee50 (`ret 4`) with three pushed words: `PartyMgrX::get3`
  keeps the original call shape (stack imbalance included); the 1-argument `get` is the canonical one.
- `Party::apply` / `swap_rows` / `stack_remove` / `clear_selection` ... were members of "Party" in the slice; they are
  members of the window (`this` has the grid at +0xb8). `stack_remove` is a thiscall member (`mov ecx, ebp` before the call
  in 0x531800, `ret 0x14`), not a `__stdcall` free function.

## Tricks kept, tricks removed

Kept: `__declspec(noinline)` on every member another function of the file calls (declared in the headers); binary order;
`#define pv ((PropVec&)props)` in `remove_prop` (a reference local hoists `this + 0x6688` and loses the match);
`static __forceinline` for the `set_anim` / `clear_anim` / `anim_value` helpers of the scroll screens; the unreachable
`st == 9 / 10` branches of `on_event`; offset-based access (`F(p, off, type)`, `GI(off)`) where the member is not (yet) in a
canonical header. Removed: the slices' `Game`, `Mission`, `Party`, `GameData`, `Widget`, `Source`, `Flags`, `MsgLog`,
`PlacedObj`, `SceneEnt`, `IntVec`, `StrTable`, `MissionSource`, `MainMenu` copies; the invented `record_count` raw pointer
pair (now `Mission::props`); the 32-bit key word and the two-argument `set_value`; `free`.

## Remaining nonmatching (all complete behaviourally, none partial)

See `map_editor.nonmatching.txt` and `exchange_screens.nonmatching.txt` for the per-row measurements. Causes: x87 / call
scheduling (0x531380, 0x5314e0, 0x5316f0, 0x5337a0: 7, 61, 47, 12 diff bytes), register allocation and frame shape (0x5305b0,
0x5325b0, 0x532740, 0x532830, 0x533070, 0x533aa0), two 18-byte WinLicense marker blocks inside 0x532de0 (not code, not
emittable from C++), and `lea` + `mov` element loads of `ListWidget::get3` that every source form folds.

## Verification

- `chk.py` on every `// @ 0x...` VA: 13 MATCH; `run_all.py --manifest`: 2/2 and 11/11 byte-exact; `compile_all.py --filter
  src/game/map_editor`: 2/2 compiled.
- Resolution audit (`resaudit_all.py --slices src:...`, `work/migrate/scratch_map_editor_audit.jsonl`): 13 rows, 13 exact /
  agree. `symbols/slices/exchange_screens.txt` maps `FrameFlags::commit` (0x513710) and `operator delete` (0x535712);
  `symbols/slices/map_editor.txt` maps `rgl::identity3` (0x40cd60, the shapes module defines the global spelling).
- `sweep_nonmatching.py` (equiv with `--fp-tol`, full mismatch tally), 13 rows, **no FAIL**:
  - PASS (4): 0x531380, 0x5325b0 (289 valid, 20/20 edges), 0x532740, 0x533a60.
  - WEAK, no mismatch (6): 0x5314e0 (7/8 edges both builds), 0x5316f0 (7/8), 0x532830 (17/18), 0x532de0 (harness cannot
    model the party rows; the marker blocks), 0x533070 (72 / 75 of 100 edges), 0x533aa0 (15 / 78: the animation vectors are
    not part of the input surface).
  - UNSUPPORTED (3): 0x5305b0 (the audit pairs ours against the wrong operand at 0x531222: after the virtual `set_frame` call the
    original reloads g_game into esi and g_mission into edi, our instruction stream orders the loads differently; 26 agree, 2
    unverified, 0 local copies), 0x531800 (0/389 valid inputs: widget / party tables and D3D imports), 0x5337a0 (0/400: `_purecall`
    and fake-arity on the widget vtables).
