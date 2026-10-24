# src/game/char_sheet migration: save writer, screens, character sheet, terrain page, dialogue (0x4e87d0..0x4f7d70)

TU 109 of the map (`work/migrate/tu_map.csv`, "char_sheet", T regime, 119 rows). The staging slices in `match/slices/`
(s004e85d0 .. s004f73c0) are unchanged. Every module builds with the default flags `/O2 /MT /Gy /TP` (the `/EHa`
the e96_vec slice used is not needed; cl 13.10 emits the `try`/`catch(...)` code without it).

## Summary

| module | range | functions | exact before (slices) | exact after | nonmatching | partial |
|---|---|---|---|---|---|---|
| save_game.cpp | 0x4e87d0..0x4e8e70 | 5 | 3 | 3 | 2 | 0 |
| save_state.cpp | 0x4e8f10..0x4e97e0 | 17 | 14 | 14 | 3 | 0 |
| save_records.cpp | 0x4e9850..0x4ea030 | 9 | 7 | 7 | 2 | 0 |
| save_panel.cpp | 0x4ea0c0..0x4ea850 | 3 | 1 | 1 | 2 | 0 |
| cam_view.cpp | 0x4ea8e0..0x4eb050 | 2 | 1 | 1 | 1 | 0 |
| facegen_misc.cpp | 0x4eb270..0x4ebb80 | 9 | 1 | 1 | 8 | 0 |
| screen.cpp | 0x4ebc10..0x4ec460 | 6 | 5 | 5 | 1 | 0 |
| screen_init.cpp | 0x4ec4d0..0x4ecce0 | 2 | 0 | 0 | 2 | 0 |
| screen_camera.cpp | 0x4ece20..0x4ed6b0 | 3 | 2 | 2 | 1 | 0 |
| render_shape.cpp | 0x4ed6f0..0x4ed6f0 | 1 | 0 | 0 | 1 | 0 |
| pick_entity.cpp | 0x4ed8d0..0x4ed8d0 | 1 | 0 | 0 | 1 | 0 |
| quest_log.cpp | 0x4ee230..0x4eea90 | 4 | 1 | 1 | 3 | 0 |
| cs_update.cpp | 0x4eeb10..0x4eeb10 | 1 | 0 | 0 | 1 | 0 |
| cs_troop.cpp | 0x4ef420..0x4ef7d0 | 4 | 2 | 2 | 2 | 0 |
| cs_detail.cpp | 0x4ef9f0..0x4ef9f0 | 1 | 0 | 0 | 1 | 0 |
| cs_weapons.cpp | 0x4f04f0..0x4f06f0 | 2 | 2 | 2 | 0 | 0 |
| cs_panels.cpp | 0x4f0820..0x4f0820 | 1 | 0 | 0 | 1 | 0 |
| msg_log_ui.cpp | 0x4f1390..0x4f13f0 | 2 | 2 | 2 | 0 | 0 |
| facegen_win.cpp | 0x4f1460..0x4f2610 | 3 | 1 | 1 | 2 | 0 |
| terrain_page.cpp | 0x4f2730..0x4f2ae0 | 7 | 7 | 7 | 0 | 0 |
| terrain_page_update.cpp | 0x4f2d70..0x4f34c0 | 2 | 0 | 0 | 2 | 0 |
| terrain_page_input.cpp | 0x4f3570..0x4f3cf0 | 6 | 0 | 0 | 6 | 0 |
| dust.cpp | 0x4f3e10..0x4f4060 | 2 | 0 | 0 | 2 | 0 |
| e96_vec.cpp | 0x4f46d0..0x4f4960 | 4 | 4 | 4 | 0 | 0 |
| facegen_init.cpp | 0x4f49d0..0x4f49d0 | 1 | 0 | 0 | 1 | 0 |
| editor_ui.cpp | 0x4f5d20..0x4f6680 | 12 | 7 | 7 | 5 | 0 |
| conv_open.cpp | 0x4f66e0..0x4f66e0 | 1 | 0 | 0 | 1 | 0 |
| conv_refresh.cpp | 0x4f73c0..0x4f73c0 | 1 | 0 | 0 | 1 | 0 |
| **total** | | **112** | **59** | **60** | **52** | **0** |

(119 rows of tu_functions.csv = 112 functions + the interior continuation 0x4eb8ed of `SaveLoadUi::run` 0x4eb8e0 and
the symbol-less EH funclets 0x4ebe7a, 0x4f4856, 0x4f49c6, 0x4f6658, 0x4f47d8, 0x4f4947, which are listed with 0 or 1
differing bytes in the nonmatching files of their parents, as in the slices.)

No regressions: the 59 byte-exact VAs of the slices in this range are a subset of the 60 module rows. Improved: 0x4f2610
`region_name` (nonmatching, 0 diff bytes, in s004f1460) is now a byte-exact row. 0x4f25a0 `rgl::frustum::init` (it lived
in s004f8c80 and was only a stub in s004f25a0) is now a module function (27 bytes differ, as in its slice).
No function became partial: all 59 non-exact rows have complete source (no partial.txt files are needed).

## Boundaries and module granularity

This is a T-regime region (helpers sit after their caller, forward calls prove nothing) and the map has a single TU
here, with class-name changes only. **No boundary evidence was found in either direction**; the 28 modules are
**one per staging slice, not TU splits**. They are one object file's functions in binary order; the module list can be
merged into one `char_sheet.cpp` once the layouts below are reconciled. Merging was not done because the slices
disagree about the same classes, and every merge needs the offsets re-proved from disassembly:

| class (slice-local views) | modules that define a different layout |
|---|---|
| `Game` / `GameRecord` / `List8` / `OutStream` | save_game, save_state, save_records (`List8` has an allocator word in one, not in the other; `GameRecord` three subsets) |
| `CharSheet` / `Game` / `TroopRec` / `UiWidget` | cs_update, cs_troop, cs_detail, cs_weapons, cs_panels |
| `Screen` / `Game` / `PartyRec` / `Overlay` / `MapWidget` | screen, screen_init, screen_camera |
| `TerrainPage` / `BitWords` / `Widget` / `World` | terrain_page, terrain_page_update, terrain_page_input, facegen_win |
| `ConvScreen` / `Widget` (different vtable slot naming) | conv_open, conv_refresh |
| `FaceGen` / `Animator` / `face_body` / `Game` | facegen_misc, facegen_init |

Each module keeps its own header (`<module>.h`, the slice preamble: slice-local layouts with their proving addresses in
comments). This still defines some classes more than once across this directory; they are the slices' unreconciled
views, not new definitions. Layouts were NOT re-verified against the disassembly beyond what byte-exactness proves.
Names of the form `facegen_win` (the terrain-generator window's `init`), `TerrainPage` (same window), `FaceGen` are
the slices' provisional names; the real class of the 0x4f1460..0x4f4060 run is the terrain generator page.

The out-of-range tails of the slices were dropped: s004e85d0's `MapScreen::init`/`frame_step` (0x4e85d0, 0x4e8720),
s004f73c0's `Agent::refresh`/`Agent::init` and builders (0x4f7d70 and later belong to the next region).

## Canonical headers

- `src/game/tstring.h`: used by every module except dust, e96_vec, render_shape and the three terrain_page modules (those
  slices derive from `rgl::string`). The slice-local `tstring` is gone. The slices' `tstring::append(char)` is
  `push_back(char)` (0x42be70) and their `clear()` annotated 0x42bfb0 is `spaces_to_underscores` in the canonical header;
  facegen_misc was adjusted (`push_back`).
- `src/game/str_table.h`: save_game, cam_view, facegen_init, quest_log, conv_refresh.
- `src/include/msg_log.h` (pure additions of member declarations, no layout change): `MsgLog::Ring::pop` 0x4f13f0,
  `MsgLog::add` 0x452c00, `MsgLog::flush` 0x4f1390 (msg_log_ui.cpp defines pop and flush). The slice's `indices[32]` is the
  canonical `vals[32]` (+0xb2c0); the slice's second array at ring+0x288 is not used by these two functions and is not
  added.
- `src/include/frustum.h` (pure addition): `rgl::frustum::init(fov, aspect, znear, zfar)` 0x4f25a0, defined in
  facegen_win.cpp; the slice's flag/f108..f11c are `perspective`, `left`, `right`, `top`, `bottom`, `nearz`, `farz`
  (0x4f25a0 stores f108 = -p, f10c = near*t, f110 = near*t*aspect, f114 = -(p*aspect)).
- The other slice-local classes (Widget, Game, mat4, mesh, resmgr, ...) are not swapped for the canonical ones: their
  slice views differ in members, vtable slot naming or offsets, and swapping was not attempted beyond tstring/StrTable/MsgLog.
  `#include "rgl.h"` (match/include) is still used by most modules for `rgl::vector4`/`matrix`.

## Verification

- `run_all.py --manifest` on every module: all rows byte-exact (60 / 60).
- `compile_all.py --filter src/game/char_sheet`: 28 / 28 sources compile.
- Resolution audit: 59 agree, 1 not-audited, 0 disagree. One local copy is reported: `screenshots_path` in screen.h
  (0x4babf0, a file-local function of src/game/config that takes its argument in ESI; the caller 0x4ebc10 only matches
  with a local `static` copy, the same keeper trick as config.cpp). It is a kept trick.
- Equivalence of the nonmatching rows (`sweep_nonmatching.py`, --fp-tol, full mismatch tally): 6 PASS, 8 WEAK (both sides
  miss the same edges / too few valid inputs), 43 UNSUPPORTED (the harness reasons recorded in the rows: FMOD and malloc
  imports, vtable-call chains, fake arity), 0 FAIL.
- The module's own `symbols/slices/` entries: none added.

## Tricks

Kept (from the slices): `__declspec(noinline)` on the save helpers, `__forceinline` helpers, the ESI-convention local copy
of `screenshots_path`, `#pragma intrinsic(tan)` in `frustum::init`, `volatile`/frame-shaping locals in the editor factories.
Not reproducible, so the rows stay nonmatching: the WinLicense `EB 10 "WL  "` markers of `save_game` (0x4e8980) and
`save_state` (0x4e8f10, 0x4e9190) (the rules forbid `_emit` in a manifest row; the slices verified them byte-exact in a
scratch copy), the `/Gh`-style frame of `quest_log` 0x4ee230, and x87 scheduling in the camera and terrain functions.
