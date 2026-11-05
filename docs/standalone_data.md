# Recreating the original's static data in source (standalone link)

> **Done.** The exe no longer holds a copy of the original's data (`no-data-copy` branch, 2026-11-05). `glue.py` stops the build
> if a symbol resolves to original data with no definition in our source. The rest of this file records how the data moved.

Goal: `tools/relink/glue.py` must stop copying the original's `.rdata/.data/.bss` into `mb_data.obj`. Every data
label the link still needs from that copy is a failure. This file is how a datum goes from "label in the copy" to
"definition in our source". Pilot module: `cls_map` (66 labels, see the end).

## 0. First look at the label

`work/relink/standalone/build.json` and `indep_list.json` list a label by mangled name. The same address is often
referenced under several names with different types (`g_menus` is a `void*` here, a `GameMenu*` in gui_edit, a
`char*` in agent_hud). Run the alias listing before touching anything: every name at one address is the *same* datum,
and defining it in one module while another keeps its own `extern` makes two storage locations (the data gate fails,
and writers and readers stop seeing each other). So there are four cases:

| case | what the original had | what to write |
|---|---|---|
| fake constant | `.rdata` float/dword used as `x * g_01` | delete the extern, write the literal (`x * 0.1f`). Code bytes stay equal: relocations are masked and cl pools the same `__real@` constant. Do it even when other modules share the label; only your reference goes. |
| string label | `.rdata` string used as `tstring k(g_key_x)` | the literal `tstring k("ui_x")` |
| global used only here | `.data/.bss` object | a definition in the module, `// 0xADDR` annotated, declared once in a header |
| field of a bigger object | `g_cfg + 0x54` named `g_bpp`, `g_options + 0x1c` named `g_char_detail`, `g_a73314` (= `g_mission_ctx.mode`) | no label at all: use the member of the canonical object (`g_cfg.display_bits`, `g_options.i1c`, `g_mission_ctx.mode`); add the member to the shared struct if the padding hides it |

## 1. Where definitions live, names, annotation

- One TU defines, one shared header declares. The definition goes in the module that uses the datum most (or owns
  the object's code); put it in a `---- data this module owns ----` block right after the includes. The declaration
  goes in the module's header, or in the object's canonical header when one exists (`src/game/menu_rec.h`,
  `src/game/game_globals.h`, `src/include/game_recs.h`, `src/game/config/core.h`, `src/rgl/options.h`). Delete every
  per-module `extern` of the same global; include the canonical header instead (listed directly in the .cpp: the
  resolution audit reads only a source's own includes).
- Keep the name the repository already uses (the resolver also maps by name). A definition always carries the
  annotation on its own line: `float g_5a9f24 = 0.65f;   // 0x5a9f24 (what it is)`. The harness
  (`tools/difftest/resolve.py`) and the data gate (`data_size_audit.py`) read `// 0xADDR` to bind name -> address.
- Never write the address as a number in code, never a placeholder, never a glue alias.

```cpp
// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_video_played;                              // 0x5e2a10 (.bss: no initialiser, size from the code)
float g_5a9f24 = 0.65f;                          // 0x5a9f24
float g_5a9f28[2] = { 0.0f, 0.21450001f };       // 0x5a9f28 {x, y}; [0] is set by static_init_547a60
```

## 2. gen_data.py

`.venv/bin/python tools/relink/gen_data.py SYMBOL|0xADDR [--type T] [--header H] [--name N] [--const] [--extent]`

It reads the image once (`tools/difftest/resolve.py Image`, bss reads as zeros) and prints the definition line to
paste (its output is reviewed source; nothing is read at link time).

```
$ gen_data.py 0x5a9f28 --type 'float[2]' --extent
float g_5a9f28[2] = {0.0f, 0.21450001f};  // 0x5a9f28
// item 0x5a9f28..0x5a9f30 (8 bytes, borrowed from image); instruction uses at +0x0 +0x4
$ gen_data.py 0x59736c --type 'char[]'          ->  char g_59736c[] = "ui_return_to_game";  // 0x59736c
$ gen_data.py 0x5aa008 --type 'struct options' --header src/rgl/options.h --name g_options
options g_options = {/*i00*/ 3, /*i04*/ 3, /*f08*/ 0.4f, ...};  // 0x5aa008
$ gen_data.py 0x5924a0 --type 'void*[4]'         (pointers print as the symbol our source defines there)
$ gen_data.py --items 0xa732f0 0xa73450          (items of an address range with the offsets the code uses)
```

- Types: scalars, `T*`, `T[N]`, `char[]` (one string), `struct NAME` from `--header` (simple `T name;` members,
  `int a, b;`, arrays, nested structs, methods skipped), `raw[N]` (dwords; only where the original itself was a number
  table). Floats print as the shortest literal that round-trips through float32 (`0.21450001f`, not `0.2145f`:
  the check is on the bits). A dword in the image with no symbol of ours at that address prints
  `/* TODO: no symbol for 0x... */ nullptr` and the tool exits 2: define the target (or fix its annotation) and
  regenerate. A `TODO` is never committed.
- Member-function pointers and vtable slots print as `&Class::name`; cast them for the table's element type.
- `--extent` shows the data_coverage.py item (`data_items()` in `tools/relink/data_coverage.py`): start, end, class and
  every offset inside it that an instruction of the original uses. Items are cached in
  `work/relink/data_items.json`.
- A value read from the image is the *pre-initialisation* value. Where the original's dynamic initialiser overwrote
  it (`static_init_*`), see section 4.

## 3. Neighbour indexing

Code that reaches one global through the address of another (`g_a73348 + kb`, `OPT_F(o)` over `g_music_volume`,
`g_a732fc[side]`) only works while the copy keeps the original's adjacency. The rule: the offsets the code
uses (`--extent`, `--items`) must lie inside ONE object whose members are real. Examples done in `cls_map`:

- `g_ragdolls .. g_particle`, `g_scale_x/y/z`, the `orig::g_music_volume ..` family: all fields of `options g_options`
  (`g_options.i1c`, `g_options.f80`).
- `g_a73314/g_a73348/g_a73428/g_a7342c/g_a73430/g_a73438`: members of `MissionCtx` (`mode`, `grid[0x38]`, `cols`,
  `depth`, `rows`, `side`); `g_a732fc[side]` = `party[side - 1]`; `g_mission_ctx[i]` becomes `((int*)&g_mission_ctx)[i]`
  (an honest table view of the context, not a second label).
- `g_hinstance`, `g_init_flag`, `g_fullscreen_flag`, `g_res_scale`: `Core` members at +0x358, +0x354, +0x346, +0x008
  (`using CD3DApplication::m_bStartFullscreen;` publishes a protected framework member, the pattern core.h already
  uses).
- `g_bpp` is `g_cfg.display_bits`.

If a struct's padding hides a member, replace `char padN[..]` with the members (keep the offsets: the comment
`// +0x48` and the following member's offset are the check; `data_size_audit.py` flags a definition smaller than a
declaration of its address).

## 4. Constructors and static_init

`src/game/static_init` hand-decompiles the original's dynamic initialisers (`static_init_5479c0` ... run from the
CRT initialiser table). The idiomatic end state is the real definition `Foo g_foo;` with its real constructor and the
hand stub gone. Never define it and keep the stub (double construction). In the pilot this applies to:

- `g_game_keys` (0xa73730, `memset(.., 0xff, 0x4a*4)` + `fn_4c2ea0`), `g_mission_ctx` (0xa73300, `MetaMission::init`),
  `g_presentations` + `g_main_menu_kind` + `g_menu_kind_count` (0xa75f2c.., `rec_table` ctor + `RefList::retire` dtor):
  constructed by `static_init_*`. Left for the static_init owner; cls_map only declares them.
- `g_5a9f28[0]`: the original computes `1.0f - g_5a9f20` at startup (`static_init_547a60`). cls_map defines the pair
  (`{0.0f, 0.2145f}`), static_init keeps writing element 0 through the shared declaration. The static_init owner
  should turn it into `float g_5a9f28[2] = { 1.0f - g_5a9f20, 0.2145f };` and delete the stub.
- Objects with constructors that static_init does not construct (`tstring g_meeting_name`, 0xa70464, four other names
  for the same string in other modules): a plain definition would add a ctor call the original does not run at that
  point; resolve together with the owners of the other names.

## 5. Pitfalls met

- Same address, several mangled names: unify through the canonical header first (case table above). `indep_list.json`'s
  `mods == ['cls_map']` lists *names*; the alias listing shows which of them are truly sole.
- A const table defined in the TU that reads it can be constant-folded by cl: check `chk.py` before and after.
- Floats that are a computed constant: `rand() % 0x3dc9 * (1.0f / 0x3dc9)` is the literal 0x54a6bc; reproduce the
  expression the programmer plausibly wrote when the bits match (`1.0f / 15817`).
- `sed` on macOS: use python for multi-line edits; a scripted rename must not touch `orig::` prefixes of other
  modules' aliases blindly.
- bss has no bytes in the image (`Image.read` is short); gen_data zero-fills. For bss the right size comes from the code
  (`--extent` and the type), not from the image.
- When a gate fails with "two storage locations", search the other names at the address
  (`python3 scratchpad alias listing` = group `indep_list.json` by `addr`).

## 6. Procedure per module (what the fan-out does)

1. `grep` the module's labels in `indep_list.json`; group by address and list every other name at that address.
2. Fake constants / strings: literal. Fields of canonical objects: member access. Sole globals: definition.
3. `run_all.py --manifest <module manifest>` and `chk.py <cpp> <nonmatching vas>` before and after; no byte-exact row may
   be lost, chk counts may not rise.
4. `build.py --out <scratch>`: the glue data-label count in `build.json` drops by what you converted; the data gate
   passes.
5. Spot-check `equiv.py <module> <va> --src ...` for two or three touched functions.

## 7. Pilot result (cls_map)

Glue data labels 1412 -> 1352 (rdata 320 -> 297, data 264 -> 245, bss 866 -> 847; `build.py` data gate: 24 definitions at
original addresses, one storage each). Of the 66 labels only cls_map referenced by name, 60 are gone:

| kind | labels | how |
|---|---|---|
| rdata float constants | 9 (+ the cls_map references of `g_549cdc`, `g_549d70`) | literals (`0.3f`, `0.39f`, `0.65f`, `0.25f/0.5f/0.75f`, `0.00528f`, `1.0f / 0x3dc9`) |
| rdata string labels | 15 | string literals |
| data: option block / config fields | 11 `g_options` aliases + `g_bpp` (+ 20 `orig::` option aliases shared with quest_page etc.) | `g_options.i1c`, `g_cfg.display_bits` |
| data: owned | `g_5a9f24`, `g_5a9f28[2]` | definitions with gen_data values |
| data: log flags | 2 | the canonical `unsigned char` types (`game_globals.h`) |
| bss: application object | 4 | `Core` members (`init_flag`, `hinstance`, `m_bStartFullscreen`, `m_fMinFrameTime`) |
| bss: mission context | 7 | `MissionCtx` members (`mode`, `grid`, `cols`, `depth`, `rows`, `side`, `party[]`) |
| bss: tables | 7 (`g_menus`, `g_5e29d0` -> `g_items`, `g_propkinds`, `g_ptypes`, `g_env_rows`, `g_map_icons`, `g_party_templates`) | the canonical typed declaration instead of a `void*`/`char*` alias |
| bss: owned | `g_video_played`, `g_5e2928` | definitions in cls_map.cpp |

Still labels (6, all bss): `g_presentations`, `g_main_menu_kind`, `g_menu_kind_count` (one object at 0xa75f2c that
static_init constructs), `g_game_keys` (0xa73730, static_init memsets and fills it), `g_presentation` (5 incompatible
types in other modules), `g_meeting_name` (a `tstring` with a constructor, 5 names for 0xa70464).

Not data: `stop_all`, `GlyphMap::insert`, `M28::M28`, `SkinMagic_Load/Init` are code symbols (the code lane).

Effort: a literal or a string label is minutes (grep, replace, delete the extern); a field alias is ~10 minutes (find the
member, rename); a canonical-header unification is 20-40 minutes because of header conflicts (cl compile loop 3-5 min on
a loaded machine; `cl /Zs` syntax-only is the fast check). The pilot took about 4 hours, dominated by waiting for
compiles, builds (resolve_map alone 15 min under load) and run_all.

## 8. Lane D3 (mission5 / mesh_slot / terrain / rgl): what the data work found

- Object overlays: one object, several views is the main reason a label stays. The unifications done: the input device record
  at 0xa170c8 is `Slot` (include `game/slot.h`; `Keyboard` is a typedef; `down[]` / `prev[]` / `axis[]` are the key tables and look
  axes, `init_devices` is 0x4bc430); the render state cache at 0xa17910 is `renderer` (its constructor is 0x4bc2d0, the window
  size and present interval are members at +0x74c / +0x750 / +0x7d4); `DialogMgr` (0x8ed50c8), the skill table `SkillRec[42]`
  (0x8ee3100: `max_rank` at +0x34 is what Troop::nibble clamps to, `cap` at +0x38 is the character sheet's), `item_mod[43]`
  (0xa75f70), the 0x20-byte `ptype` table and the 0x78-byte `Rec78` / `PropKind` table.
- A partial view named like the real class mangles identically (`struct renderer { ... }` in rgl/particles/render_cache.h,
  `struct ShadowMap` in char_sheet/cs_common.h): that is how a TU that cannot include the real header still shares the storage
  without an alias. The same name with a different type (`extern MarkerList g_list_a`, `extern char g_cfg[]`) is a second
  symbol and a glue alias.
- Annotations can be wrong by one field. `g_mat_vec` was annotated 0xa732c0 but the vectors begin at 0xa732bc (allocator
  dword first): masked relocations hide this in the byte-exact rows and the standalone exe would have read `last` instead of `first`.
  The first/last/end offsets in the original's code, not the annotation, decide (xref the operand in the image).
- Names with two original addresses fail the build: three lane names collided with other addresses (`g_cursor_x` 0x5d7094
  float vs int 0x5d70b4, `g_pending` 0x5e2a14 vs 0xa18210, `g_emit_pos` 0xa708c0 vs 0x8eb235c, `g_spawn_hi`); the D3 side was renamed
  (`g_warp_x`, `g_pending_force`, `g_world_color`).
- A constant that cl reassociates when it is a literal (`ch * 250.0f * 0.0625f`: the power-of-two factor moves in front) is
  defined in another TU as the original had it (`g_54ab2c` in rgl/mesh2/gui_edit.cpp).
- Compiler-constructed objects replace their hand stubs in static_init.cpp (`g_module_path`, `g_dialogs`, `g_item_mods`,
  `g_skeletons`, `g_render`): the stub, its destructor stub and its table entry go in the same change, and the manifest rows of
  the removed stubs go with them (the constructors themselves, 0x4c2470 / 0x4bc2d0, stay byte-exact as members).
## 8. Lane D2 (char_sheet and the UI core globals): what was learned

- **Do not add `#include "../game_globals.h"` to every module.** game_globals.h declares the canonical type of hundreds of
  addresses, and a module that still keeps its own (wrong-typed) extern of one of them stops compiling (`g_presentations`,
  `g_log_flag95`, `g_music_situation`, `g_display_wp_one_handed` in cls_map, progress_bar, game_app, char_sheet ...). The
  globals every UI module touches (viewport size, window-dirty flag, mission clock, developer mode, the input key/axis state)
  live in the leaf header `src/game/ui_globals.h`, which game_globals.h includes; modules with clashing externs include only
  that one. A module that needs both includes game_globals.h.
- **Neighbour names are members.** `g_cur`, `g_dummy_mat`, `g_dummy0`, `g_dummy1` are `g_anim.cur / .weights / .scale / .time`
  (the Animator at 0x8ed4f60); `g_5a9f50` (+ `g_5a9f54` / `g_5a9f5c`) is `g_debug_log.buf` (the Logger at 0x5a9f30 is a text_stream: `buf` is +0x20); `g_log_path` / `g_log_flag78` are
  `g_log.name / .autoflush`; `g_save_prefix` is `g_module_name.buf` (the `p ? p : (char*)&p` idiom is `tstring::c_str()`);
  `g_a73a04` / `g_a73a08` are the `g_id_stack` fields; `g_a17f28` is `g_render.dev`.
- **One class per object.** `g_strtable` had two classes (StrTable in game/str_table.h, LangTables in rgl/mesh2/mesh2_common.h)
  and eight partial copies; it is now `StrTable` (str_table.h: buckets, ctor 0x4cda20, load, find, lookup, release_strings) with
  one definition `StrTable g_strtable;` in mesh_2.cpp, built by the compiler's initialiser; static_init's stubs 0x547a80 /
  0x548470 are gone. Same recipe for the objects still open below.
- **Literals are not free for x87 code.** Replacing a fake constant by its literal keeps every byte-exact function exact, but a
  few nonmatching functions move by a few bytes (register allocation shifts); see the lane report.
- **A definition inside another object is a second owner (the D2 runtime bug).** D2 defined `g_screen_w` / `g_screen_h`
  (0x5d75c8 / 0x5d75cc), `g_div`, `g_gui_dirty`, `g_mission_clock`, `g_fov_k` as globals of their own, but all of them are
  bytes of the gui manager g_gui (0x5d7080, 0xb350 bytes), which the glue lays out once from the original's image and which
  `Core::update_view_rect` (0x4bace0) writes through its own member. The save/load / loading screen (`SaveLoadUi::run`
  0x4eb8e0) read the zero copy for its `SetViewport` / `Clear`, so nothing was cleared and every frame stayed on screen
  (status texts stacked, the loading screen stayed under the main menu). The same mistake with the same shape: `g_dev_mode` /
  `g_5a9fa0` / `g_5a9fb8` (members of `g_cfg`: enable_edit_mode, show_framerate, debug_mode), `g_5a9f50` (the Logger's
  `buf`), `g_skin_name` (`StrTable::language`), `g_skill_clamp` (`g_skill_tbl[36].cap`), the g_anim words (`g_anim.skins`,
  `.skins_end`, `.char_count`, `.item`, `.material`, `.skins_alloc`). The data gate now fails the build on a label inside
  another object's claimed range (`data_size_audit.py`, OVERLAP; `data_overlaps_known.txt` lists the 44 pairs main already
  had, which only shrink). Rule: an address that lies inside a bigger object is a member of its class when the class is
  reachable, else a label (`extern`, no definition) -- never a definition of its own.
- `g_gui` 0x5d7080 is ONE `gui_manager` (strat/gui_manager.h, sizeof 0xb350, every interior label a member: `screen_w`, `screen_h`,
  `dirty` (g_gui_dirty), `wheel` (g_mission_clock), `fov_k`, `aspect` (g_div), `mouse_dx/dy`, `lines` (g_strs), `drag`, `overlay`, `label_obj`, ...),
  defined once in strat/gui_manager.cpp and built by the compiler's initialiser; the message-log / screen-input / loading-mesh /
  sound-host views are gone (static_init's 0x5479c0 / 0x548140 rows too). Because the compiler-made initialiser now sits in the
  gui_manager module's .CRT$XCU, it runs before the static_init table's entries (the original ran it second, right after
  resmgr 0x5479a0): the only inversion is gui before resmgr, and ~gui_manager now runs after ~resmgr (which clears the `res` of registered meshes; the exit-time deletes of unregistered ones read freed materials, not observed to fault in smoke). It goes away when resmgr becomes a compiler-constructed global too.
- Open (each is a many-module class unification like g_strtable): `g_windows` 0xa73980 (+ `g_wDC` = slot 23; WindowMgrW / main_menu / screen_cfg / raw
  arrays; ctor = conv_zero_node 0x4c8c20, dtor Tail2 0x4c4d10 on the deque at +0x78), `g_terrain` 0xa76630 (nine `Terrain`
  partials, BitWords, DustTerrain, TerrainGen), the vtables 0x591fb0 / 0x592848 / 0x597050 / 0x598698 (vtable lane).
## 8. Lane D6 (agents / script / static_init / game roots): what was done, patterns, what is left

New patterns beyond the pilot:

- **Element of a table another global already is**: `g_mask_a8/ed0/ef8/f18` are `g_bit[8/13/18/22]` (the 1<<i table at
  0x5a8e68), `g_size_mask`/`g_type_mask`/`g_mask_w` are `(unsigned)g_bit[2]`/`g_bit[4]`. `g_sound_ids`, `g_levelup_sound`,
  `g_snd_224..2e0`, `g_sound_blood` are `g_sound_mgr.slot[1/43/44/45/46/73/74]`; `g_sound_count` is `g_sound_mgr.ngroups`;
  `g_front` is `g_sound_mgr.groups`; `g_listener` is `g_sound_mgr.frame` (the listener frame, position `.o`). `g_menu_b` is
  `g_map_flags.f8`. One `SoundMgr g_sound_mgr` (agent_fx_sound.cpp) replaces the `SoundW`/`SoundSys`/`SoundTable`/`music_set`
  views (their member functions are SoundMgr members now: `play_ui`, `stop_all`).
- **Startup initialisers as real definitions** (`float g_5a9f28[2] = { 1.0f - g_5a9f20, ... }`, `unsigned g_river_mask_b =
  (unsigned)g_bit[2]`, `std::string g_base64_chars("A..Z..+/", 0x40)`, `PhysSet g_phys;` with `PhysSet() { reset(); }`): cl emits
  the dynamic initialiser (`_$E1`, `_$E3`, the destructor `_$F`/`_$E2`) and it is byte-identical to the original's, so the
  hand stub in static_init.cpp is deleted and its manifest row moves to the defining module (`_$E<n>` rows; the numbers skip
  by two when the variable is a scalar: check the object's symbol list). The initialiser is only emitted when the
  initialising expression is not constant-foldable: if both objects are defined in one TU cl folds
  `float b = 1.0f - a` into a static value and no `_$E` exists. Put the source object in another TU (here g_5a9f20 and
  g_5a9be0 live in progress_bar.cpp, their derived partners in cls_map.cpp).
- **`const` datum a literal cannot replace**: `g_time_scale` (0x590e28, 60.0f) multiplies a `double`; a literal makes cl use the
  double constant and changes list_saves (2 bytes). It stays a defined datum, defined in a TU other than its reader.
- **Typed views of a root** (g_game): `party/game.h`'s `Game` and `include/game_data.h`'s `GameData` describe one object and cannot
  be included together. The one symbol is `GameData* g_game` (widget_text.cpp); a module that sees the object through another
  class casts it with a macro (`g_game_G`, `g_game_ed`) instead of declaring a second `g_game` of another type (which is a
  second mangled name and a glue alias).
- Do not let a script add `#include "game_globals.h"` / `game_data.h` / `mission.h` to every user of a name: those headers
  clash with the local view structs of many modules. Redeclaring the same type (`extern int g_sound_muted;`) is legal and keeps
  the module compiling; the aliases (different names or different types at one address) are what must go.

Still open (and why): the module.ini settings block 0xa180f8..0xa181d0 is one object (`static_init_547bc0` constructs it) whose
members are split over lanes D1/D3/D5/D6; it needs one `ModuleInfo` struct + ctor and every lane switching to its members at
once. The log objects 0x5a9f30/0x5a9f60 (debug log, `g_log`, with `g_log_file`/`g_log_flag94/95` as members) belong to D1/D3;
the key table members `g_cam_*`/`g_key_*` (0xa17108.., elements of `g_keyboard.down[]`, D3) and the mouse pair 0x5d7094/98
likewise. `tstring` globals without a static_init constructor (`g_meeting_name` x5 names, `g_skin_name`, `g_module_path_copy`)
need a decision: a plain definition adds a constructor call the original does not run.
