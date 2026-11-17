# 021: Struct padding and offset-named fields: name them, keep a pad only where the bytes are unknown

Written 2026-11-16 (triage pass; read-only on `src/`). Area: cleanup / 64-bit foundation | Effort: L | Impact: high for 006 and modding | Depends on: - (feeds 006, 016, 019)

## Why this exists

The decomp grew from offsets out of the binary. A function was first written as "the object has something at +0x5a8, and this code reads an int there", so the
struct got `char pad0[0x5a8]; int f5a8;`. Padding is the scaffolding that kept every later field at its right offset while the rest was still unknown, and the
`fNN` names are the same thing for scalars: the name is the offset because nothing else was known. The byte-exact build never needed it removed. The portable
64-bit build (006) does: a byte pad keeps its size while the pointer fields around it grow from 4 to 8 bytes, so every field after it moves, and any offset
arithmetic written against the 32-bit layout (`(char*)p + 0x5a8`) lands on the wrong bytes.

## Counts (2026-11-16, script scan of all `.h/.cpp/.hpp/.inc` under `src/`)

Per-site table: [021-padding-sites.tsv](021-padding-sites.tsv) (file, line, struct, member, bytes, class; `D`/`Dn` rows are offset-named scalars, `Dn` = a
name is already in the comment). Scan limits: struct membership is tracked by brace depth, sizes are evaluated from the array expression, and the class
is a heuristic (below). Counts differ a little from the earlier 1491/684 figures (those included other spellings).

Pad/gap/unk/reserved arrays: **1431 in 165 files, 497 structs.**

| class | meaning | arrays | files | bytes |
|---|---|---:|---:|---:|
| A | some code in `src/` reads a raw offset inside the pad (upper bound, see note) | 361 | 88 | 1.09 M (excl. two pads over 1 M) |
| B | nothing in `src/` touches it | 805 | 144 | 287 K |
| C1 | pad inside a `union` standing in for the other members | 1 | 1 | 16 |
| C2 | leading `pad0[..]` (base-class stand-in) or a partial "view" struct over a big object (`SiteKeys`, `Placement`, `MapScreen` ...) | 264 | 84 | 1.44 M |
| D | offset-named scalars (`f5a8`, `v56b4`, `lvl58`, `w70`; name == `+0xNN` comment or `f`+hex) | 954 | 141 | 4 B each typ. |

Notes on the classes:

- **A** is "a raw access (`(char*)p + 0xNN`, `F(this,0xNN,T)`, `at<T>(0xNN)`, `((int*)p)[k]`) with an offset inside the pad's range, in a file that
  names the struct". It overcounts (an offset that belongs to another type in the same file matches); a hand check of 12 sampled rows found most were
  genuine (`GameData::pad68`, `FireRay::pad40`, `Ramp::pad18`, `GuiCtx::pad000`) and a few were another type's offset (`SceneObjV::pad20` hit at 0x40).
  Treat A as "check first, probably nameable"; ticket 022 owns the access sites, this ticket owns the declarations. 101 of the 361 are 16 bytes or
  fewer: a single field each.
- **B** has 376 arrays of 8 bytes or less (alignment gaps, a lone unknown int); the rest are real unknown runs. The struct size matters for most:
  44 of the 497 structs carry a `sizeof(S) == 0x..` check (188 of the B arrays are in those structs), and others are allocated with `new S[n]` (Party
  pages are `new Party[16]`, `Troop`, `Agent` tables) or embedded in arrays, so a B pad cannot be deleted, only kept or replaced by named fields.
- **C2** is mostly not a union at all. Sixty-odd are **view structs**: a name for a few fields of a huge singleton (the game object at 0x5e2930 viewed as
  `SiteKeys { char pad0[0x1a1f0]; int cur; ... }`). These have no real `sizeof`; their pad is the whole object up to the field. There is only one pad
  directly inside a union (the 158 unions in headers rarely use pads).
- **D**: 797 of the 954 are `f`-prefixed (`f55c0`), the rest `v`, `lvl`, `w`, `p`, `i`. Only about **359 (38%)** already have a descriptive word in the
  trailing comment (after dropping `raw`, `int`, `char`, `r_int`), e.g. `f30 // ui_done button`, `f18 // shaped phase`, `f230 (the inverse frame)`. The
  other 62% have a bare offset or `(r_int)` and need a reader/writer look (the `f55c0 -> last_attacker` model of ticket 019).

## Worst 15 by pad bytes

`sizeof` is not computed (no builds in the triage); none of these has a size check, and the rows that are views have no meaningful size. "Pad bytes" is the
sum of all pad arrays in the struct.

| struct | file | pad bytes | sizeof / note |
|---|---|---:|---|
| MapScreen | src/game/char_sheet/save_game.h | 138,795,291 | view of the map-screen object, `padview[0x845d984-..]`; no size check |
| conv_rec | src/rgl/mesh2/gui_edit.h | 12,824,788 | `pad24` 12.8 M inside a view; no check |
| GameMapView | src/game/mission5/game_2.h | 107,096 | view |
| SiteKeys | src/game/mission_ui2/mission_3.h | 107,000 | view over the game object, `pad0[0x1a1f0]` |
| CampaignGame_s2b5c0 | src/game/mission5/mission_5.h | 106,975 | view |
| Game_s2cf60 | src/game/mission5/mesh_3.h | 106,956 | view |
| GameData_s261e0 | src/game/mission5/mission_5.h | 106,947 | view, 16 pads |
| Frame | src/rgl/mesh2/gui_edit.h | 106,904 | view |
| GameConfig | src/game/config/config.h | 106,892 | view |
| Frame__s2f250 | src/game/mission5/mesh_3.h | 106,827 | view |
| CampaignGame_s2bf40 | src/game/mission5/mission_5.h | 106,704 | view |
| EditorGameView | src/game/agent5/editor_dialog.cpp | 106,668 | view |
| Game_s284a0 | src/game/mission5/mission_5.h | 106,636 | view |
| OutRecList | src/game/char_sheet/save_records.h | 82,004 | view over the save writer |
| Placement | src/game/agent/agent.h | 81,908 | view, placement block at +0x13fb4 |

The real (non-view) structs rank differently; by pad bytes: GameData_s261e0 (106,947), OuterGame (80,384), Game_s257f0 and Game_s26130 (80,380 each),
Mission_s261e0 (26,553), Mission_s257f0 (26,344), progress_bar.h MapScreen (26,256). Most of the top of both lists are the numbered `_sNNNN`
overlay structs in src/game/mission5/mission_5.h (242 pads in that one file) and mesh_3.h (55). By array count the worst files are mission_5.h 242,
gui_edit.h 57, mesh_3.h 55, script_interpreter.h 49, script_interpreter.cpp 48, agent_obj.h 44, mesh_game.cpp 38, game_data.h 37, party/game.h 32.

## Behaviour on 64-bit, per class

- **A (accessed pads)**: worst case. The raw access encodes the 32-bit offset; on 64-bit the pad stays the same size, the pointers before it grow, and the
  field the access meant has moved. Naming the field makes the compiler recompute the offset. These are silent miscompiles, not link errors.
- **B (untouched pads)**: harmless to correctness only while nothing else reads them. They still inflate each struct by the 32-bit size of what they stand
  for (a 0x40 pad that hides four pointers is 0x20 bytes too small on 64-bit, so later fields no longer line up with whatever the original wrote). Where
  the struct is read from a save game or the original's tables (`new S[n]`, record arrays), the real layout is already fixed by the file, not by the pad.
  Size asserts on B structs are wrong on LP64 by construction and must be 32-bit-only.
- **C1/C2 views**: a view struct whose pad is the whole object up to the field only works if the object itself is that layout. On 64-bit the object has to
  become a real struct (006); until then every view is a miscompile risk, and the pad is the proof it was never typed. Leading `pad0` base stand-ins want a
  real base class (the object's vtable pointer is 8 bytes, not 4).
- **D (offset-named fields)**: no layout risk by themselves (they are real members), but the name is a lie once the 64-bit layout moves the offset
  (`f5a8` at 0x5b0). Renaming is a pure cleanup: no byte change, no behaviour change.

## Plan

1. **Name fields from their uses.** For each class D scalar, read its writers and readers (`rg` the member, then the callers) and give it a real name; start with
   the 359 that already have a word in the comment (the comment becomes the name, mostly mechanical), then the rest. A reviewed rename table per file keeps
   it greppable.
2. **Replace raw-offset accesses with the named fields** (ticket 022 owns the access sites; this ticket owns the struct declarations. A lane does both in
   the same file so the pad can shrink as the accesses go). For class A: split the pad around the accessed offset into `pad + named field + pad`, then
   point the access at the field.
3. **Keep a pad only where the bytes are truly unknown** (class B). Merge adjacent pads, drop alignment-only gaps that the compiler regenerates, and mark each
   survivor `// unknown: original-only` with the last offset evidence. A pad that is not byte-significant on MSVC is gone; one that is stays.
4. **Views (C2)**: convert each into the real class when its object is typed (006), or into an accessor on the existing owner; do not add new views.
5. **Size checks**: 69 `sizeof(...) == ` lines (about 55 distinct size asserts, 44 on pad-carrying structs) are wrong on LP64. Wrap them in
   `#if !defined(_WIN64) && !defined(__LP64__)` (32-bit only) in the same pass; the tree has `layout_check.py` to keep the 32-bit side honest.
6. Every new field with an offset comment gets a 32-bit-only `offsetof` check, so the next rename cannot move anything.

## Rule: byte-exact first

Every edit stays byte-exact for MSVC. Run `chk.py` per function on the touched rows and `work/commit_frozen.sh` once per batch (per the repo's gate rules); keep only
edits where no bytes move. Splitting `char pad[N]` into `pad + int named + pad` of the same total size must not change `sizeof` or any offset; a rename must
not change codegen. If a rename moves bytes (a member order or alignment effect), revert it and keep the name in a comment.

## Lanes (one per header or subsystem, largest first)

| lane | scope | pads | scalars |
|---|---|---:|---:|
| L1 | src/game/mission5/mission_5.h (the `_sNNNN` overlay structs) | 242 | see tsv |
| L2 | src/rgl/mesh2/gui_edit.h, rec_tables.h; src/game/mission5/mesh_3.h | 112+ | |
| L3 | src/game/script/script_interpreter.{h,cpp} (Mission, AgentTable views) | 97 | |
| L4 | src/game/agent_obj.h, agent/*, agent4/agent_logic.cpp | 44+ | |
| L5 | src/game/mesh_slot/mesh_game.cpp, party/game.h, include/game_data.h | 107 | |
| L6 | src/game/char_sheet/* (save_game.h, save_records.h; MapScreen views) | | |
| L7 | src/game/terrain_a, terrain_b, ui, config, cls_map | | |
| L8 | the 44 size-check headers: `#if` guard pass (does not touch layout) | done | 274 checks in 50 files go through MB_LAYOUT32_ASSERT (src/include/layout32.h, 32-bit x86 only); same_code identical on 168 TUs, commit_frozen gate PASS; x86_64 burndown 19/187 -> 178/187 compile (9 fail) |

Per-file counts for any lane: `rg -c` in the tsv by file column (`cut -f1 docs/tickets/021-padding-sites.tsv | sort | uniq -c | sort -rn`).
Recommended first lane: L1 (the largest file, mostly class A/C2) after L8's size-guard pass lands, because the guard is small, has no byte risk, and unblocks the
64-bit compile of every struct file.

## Not done by this ticket

Typing the views as real classes is 006's job; this ticket only makes sure no field is hiding behind a pad or an offset-shaped name when that work starts.
