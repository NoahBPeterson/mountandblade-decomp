# src/game/ui migration: item refs, inventory, effects, mission UI (0x4625d0..0x4636d0, 0x488560..0x48b930)

Two ranges of the TU map (`docs/tu_map.md`; `work/migrate/tu_map.csv` TUs 55-74, Migration plan M2e) are moved from
their `match/slices/` staging slices into seven translation units. The slices are unchanged and stay the
reference. `src/game/renderer.cpp` ends at 0x4625d0, where this region starts.

Every module builds with `/O2 /MT /Gy /TP` (one flag set, no per-row flags, no `/EH*`). Only real headers
(`<math.h>`, `<string.h>`, `<stdlib.h>`) plus `src/game/tstring.h`, `src/rgl/{rgl_math.h,mesh.h}` and the headers in this
directory. The only inline asm is the 6-instruction `decode_utf8` call helper in ui_widgets_a.cpp (custom register
ABI of 0x487b00, allowed by docs/matching.md).

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| item_ref.cpp | 0x4625d0..0x462ac0 | 7 | 7 | 7 | 0 | 0 |
| troop_inv.cpp | 0x462ac0..0x4636d0 | 14 | 11 | 11 | 3 | 0 |
| mission_ui.cpp | 0x488560..0x48b2d0 | 119 | 98 | 103 | 15 | 1 |
| ui_widgets_a.cpp | 0x48b2d0..0x48b550 | 7 | 5 | 6 | 0 | 1 |
| ui_slider.cpp | 0x48b550..0x48b5a0 | 2 | 2 | 2 | 0 | 0 |
| ui_slider_key.cpp | 0x48b5a0..0x48b6e0 | 1 | 0 | 1 | 0 | 0 |
| ui_widgets_b.cpp | 0x48b6e0..0x48b930 | 6 | 5 | 5 | 1 | 0 |
| **total** | | **156** | **128** | **135** | **19** | **2** |

(The functions are the rows of `work/migrate/tu_functions.csv` in the two ranges: 122 exact, 18 nonmatching, 16
partial in the slices; 135 exact + 19 nonmatching + 2 partial here; 156 = 135 + 19 + 2.)

Regressions: none. Every function byte-exact in a slice is byte-exact in its module (checked by diffing the VA sets).

Improved (slice status -> module status), 7 functions:
- 0x488b50, 0x488d50, 0x488e90 (mission_ui), 0x489ee0 `World::sl_roll_other`, 0x489f40 `World::sl_value_factor`:
  nonmatching/partial in the slices -> MATCH in the module (one consistent set of headers: the shared `g_ticks`,
  the `MissionUi` / `World` members). The individual cause of each was not traced; no function got worse.
- 0x48b4f0 `UiB::show` (widgets_a), 0x48b5a0 `UiB::key` (ui_slider_key): see the boundary at 0x48b550 below.
Further rows moved from partial to complete-and-equivalent (still not byte-exact): 0x488720, 0x489540 (now returns
`out`, which was the equiv mismatch; 310 inputs PASS), 0x489620 (evaluation order of the two calls fixed: the slice's
resolution-audit disagreement is gone), 0x48a080, 0x48a150, 0x48a290, 0x48a000, 0x4897a0.

## Boundaries (what changed against the brief)

Forward-call rule (cl emits a TU callee-first, so a call to a higher address leaves the TU) re-checked with
`work/migrate/facts.json` over both ranges (calls and tail jumps; the WinLicense `EB 10 57 4C` marker jumps do not
occur in this region). The only calls that cross a module boundary forward are:

| call | boundary it proves |
|---|---|
| 0x48b200 `UiA::key` -> 0x48b2d0 `play_sound` | a TU starts at 0x48b2d0 (high) |
| 0x48b4f0 `UiB::show` -> 0x48b550 `UiB::value` | the TU ends before 0x48b550 (high) |
| 0x48b5a0 `UiB::key` -> 0x48b6e0 `UiB::set_value` | the TU ends before 0x48b6e0 (high) |
| 0x48b8d0 `UiD::show` -> 0x48b930 `UiD::value` | the TU ends before 0x48b930 (high) |

No forward call enters either range from the preceding code (renderer.cpp, 0x487000..0x488560), and none leaves
0x4625d0..0x4636d0 or 0x488560..0x48b2d0 internally, so those two runs are single objects as far as the calls can
tell (class-name changes only, low).

- **item_ref | troop_inv at 0x462ac0**: kept from the map (class change ItemRef|Agent, low). `slot_value` 0x462c00
  calls `item_slot_source` 0x462ac0 as a `static` function with the register convention (argument in EAX), which
  puts the helper in the same object as its callers (0x462c00 and the rest of the Troop inventory code): the map's
  start of "inv" is consistent with that.
- **troop_inv end 0x4636d0**: map class change (Inv|ArrSet, low); no call evidence either way. `ViewState::init`
  (0x463370) and `get_dir` (0x463610) belong by content to the same object as `Placed::read` 0x4631c0 (the map keeps
  them in "inv"); kept.
- **mission_ui: TUs 65, 66, 67 (effect, mission_2, mission_ui; 0x488560..0x48b1c0 in the map) merged into one
  object 0x488560..0x48b2d0.** The map's inner splits (66 "possible", class changes Effect|MissionUi|Party|Sel|
  Mission) have no forward-call support, and every intra-range call goes backwards. The class names fail as a
  boundary signal here because the same classes recur on both sides (MissionUi at 0x488720 and 0x48ae20, Party at
  0x489420 and 0x463190, World at 0x489c50 and 0x48a290). The end at 0x48b2d0 is proven (table row 1); it also
  absorbs the map's `ui_a` (0x48b1c0, `UiA` ctor/update/key: 0x48b1c0..0x48b2d0), because 0x48b200 -> 0x48b2d0 puts
  the end after `UiA::key` and nothing separates `UiA` from the preceding `MissionUi` functions.
  Names: "mission_ui" after the class at both ends (MissionUi::render at 0x488720 and the constructor 0x48ae20).
  The slice called `MissionUi` `Effect` / `widget_base` / `Buffer`; the vtable 0x591c80 maps to `MissionUi` (see
  `symbols/slices/mission_ui.txt`).
- **ui_widgets_a 0x48b2d0..0x48b550**: start and end proven (rows 1 and 2). Holds the click sound, `UiA`'s update /
  type code, `MissionUi::make_text` (text fit through the UTF-8 decoder), and the slider constructor and `show`.
- **ui_slider 0x48b550..0x48b5a0 | ui_slider_key 0x48b5a0..0x48b6e0 (the map has them as one TU, ui_b_2)**: split.
  `UiB::key` calls `UiB::value` (0x48b550) out of line; a same-object `value()` would be inlined by cl (the
  equivalent merged build of `key` differs by 299 bytes and loses the register allocation, see the slice notes
  of s0048add0 0x48b5a0), so `value` is an external function to `key`. That is evidence for two objects at
  0x48b5a0 (medium: it relies on cl's inlining of a small same-TU function). Splitting also makes `UiB::show`
  0x48b4f0 and `UiB::key` byte-exact (they were nonmatching in the slices, for the same reason: with `value()`
  visible cl assumes its register use).
- **ui_widgets_b 0x48b6e0..0x48b930**: start proven (row 3), end proven (row 4). The map's class changes inside
  (UiB|UiC at 0x48b7f0, UiC|UiD at 0x48b890) have no call evidence; kept as one object.

Hard boundaries after this work: 0x48b2d0, 0x48b550, 0x48b6e0, 0x48b930 (all forward calls); 0x48b5a0 medium; the
rest class names only.

## Headers (one per class)

| header | classes | verified by |
|---|---|---|
| item_ref.h | `snd_channel`, `snd_slot` (0x78), `snd_bank`, `ItemKind` (0x238), `ItemRef` (8), item constants | 0x4625d0..0x462a10; 0x48b2d0 (volume) |
| troop_inv.h | `Troop` inventory members, `Placed`, `ViewState` (0x5374) | 0x462ac0..0x463610 |
| party.h | `Stack20`, `Party` (0x5ac8: flags +0xa0, stacks +0xa8, count +0x8a8) | 0x489420..0x489620, 0x463190 |
| mission.h | `Mission`, `MissionView`, `NamedRec`, `Sel`, `Placed` records | 0x489890..0x48a420 |
| world.h | `World` (party/item value queries, +0x13a00 troop table, +0x13a58 party records) | 0x489c50..0x48a290 |
| mission_ui.h | `MissionUi` (+0xf0 screen, six track pairs, text placement), `ElemList`, `TimerList` | 0x488560..0x48ae20 |
| ui_widgets.h | `UiA`, `UiB`, `UiC`, `UiD`, `SoundMgr` | 0x48b1c0..0x48b8d0 |
| ui_common.h | `g_ticks` (0x5e2948), `g_master_volume` | 0x4625d0, 0x488560 |

Each field carries the address of a function that reads it. Class names that `symbols/slices/*.txt` maps to
vtables are kept (`UiA`, `UiB`, `UiC`, `UiD`, `MissionUi`).

### Conflicts with headers outside this directory (not edited, reported)

- `Party`: `src/game/dialog_manager.h` defines `struct Party` (0x5ac8 bytes, only +4/+8/+0x8c named); `ui/party.h`
  defines the same 0x5ac8-byte class with the stack list (+0xa8 x 64, count +0x8a8, flags +0xa0). Same class, two
  definitions; the ui one is a superset (the two agree on the size). The definitions cannot be included together.
- `ItemRef`, `ItemKind`, `Troop`, `Mission`: `src/game/agent/{agent.h,item_kind.h}` define the same classes for
  the 0x434xxx/0x435xxx members; `ui/item_ref.h`, `troop_inv.h`, `mission.h` define the members of this region.
  `ItemRef` (8 bytes: item, packed word) is identical in layout; `Troop` and `Mission` are partial views of the same
  large records in both places. They have to be merged into one header per class before the link-order skeleton.
- Nothing else in `src/rgl` or `src/game/tstring.h` / `mat4.h` was missing or wrong for this region.

## Canonical names (slices -> modules)

| slices | module |
|---|---|
| `Effect`, `widget_base`, `Buffer` (s004885e0, s0048add0) | `MissionUi` (vtable 0x591c80) |
| `Inv` (s00462e20), `Entry8` (s00488b20) | `Troop`, `ItemRef` |
| `cls_c` (0x48b760) | `UiC` |
| `sl_roll_other` free function (s00489d10) | `World::sl_roll_other` |

## Verification

- `chk.py` on every `// @ 0x...` VA of each module: 135 MATCH (rows in `<module>.manifest.txt`).
- `run_all.py --manifest` on each manifest: 7/7, 11/11, 103/103, 6/6, 2/2, 1/1, 5/5 byte-exact.
- Resolution audit (`resaudit_all.py --slices src:...`, `work/migrate/scratch_ui_audit.jsonl`): 135 rows, 135
  exact/agree, 0 disagree. One "local copy" note remains: 0x462c00 `slot_value` calls `item_slot_source`
  (0x462ac0), the static register-convention helper of the same object, mapped in `symbols/slices/troop_inv.txt`;
  it is the original function itself, not a duplicate.
- Equivalence (`equiv.py <module> <va> --src ... --flags "/O2 /MT /Gy /TP"`) on the 19 nonmatching rows: 13 PASS,
  5 WEAK and 1 UNSUPPORTED (0 FAIL):
  - PASS (13): 0x463370, 0x463610, 0x48b760, 0x488720, 0x488f80, 0x489090, 0x489540, 0x4897a0, 0x489890, 0x48a000,
    0x48a150, 0x48a290, 0x48ae20.
  - WEAK (no mismatch; both builds miss the same edges, each blocked by state the sandbox never seeds): 0x489620,
    0x489c50, 0x489d10, 0x489e70, 0x48a080 (5).
  - UNSUPPORTED: 0x462e20 `Troop::pick` (every input faults on the random item-table pointers; harness limit).
- The two partial rows are the only FAILs of the harness and are not counted as migrated-equivalent:
  - 0x48b3a0 `MissionUi::make_text`: the custom-ABI `decode_utf8` fake sees different stack arguments in the two builds.
  - 0x48a420 `Mission::get_frame`: the original forwards an uninitialised stack lane into `orthonormalize`
    (4/400 inputs differ); not reproducible.
- `tools/status.py --no-write`: these rows count as "Migrated into src/" (1028 functions in the run that included
  the other directories).

## Removed slice-local tricks

- Per-slice duplicate declarations of `g_ticks`, `World`, `Party`, `ItemRef`, `Troop`: one header each (above).
- `Effect`/`widget_base`/`Buffer` aliases of `MissionUi`.
- Also changed in this pass: `Party::remove_row` returns `int*` (= `out`, as the original does in eax).
- Call-order temporaries in `Party::find_or_add` (0x489620): `int ct = count_tail(); int mx = max_4b1700();` instead of
  a compare of two calls (cl evaluates the right operand first).

Kept, with reason:
- `_ReadWriteBarrier()` in `MissionUi::set_a/b/c` and similar (0x488560..): pins the store of the period ahead of the
  `g_ticks` load; zero code emitted (intrinsic).
- `volatile` state/clock fields of `snd_slot` and `MissionUi::clock`: the original re-reads them and never CSEs
  `g_ticks[clk]` (0x4625d0, 0x488e90).
- `rnd()` (`__forceinline` floor/round): cl inlines round-to-nearest here; the out-of-line copy is 0x432540 in
  another object.
- The 6-instruction `decode_utf8` asm helper (0x48b3a0): the only way to call 0x487b00's custom ABI; within the
  allowed 8 instructions.
- `goto tail_count` in `find_or_add` (0x489620): kept from the slice; the function is nonmatching and equivalent.
