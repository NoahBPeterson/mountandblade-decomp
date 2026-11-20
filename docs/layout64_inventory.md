# layout64 inventory: what breaks silently when src/ runs as a 64-bit program

Lane layout64 (docs/port_lanes.md, ticket 006). Measured 2026-11-19 on main, macos-arm64 (clang, LP64). The 64-bit build
compiles every src/ TU; what is listed here compiles and is wrong at run time. On macOS arm64 every heap and image
address is above 4 GB, so **any pointer squeezed through a 32-bit integer is a crash**, not a latent risk.

## How it was measured

- Warnings: a separate build dir with src/'s warning flags replaced (port/CMakeLists.txt cache option
  `MB_SRC_WARNINGS`, default `-Wno-everything`):

      cmake -S port -B build/port/macos-arm64-warn -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/port/cmake/macos-arm64.cmake \
        "-DMB_SRC_WARNINGS=-Wno-everything;-Wshorten-64-to-32;-Wpointer-to-int-cast;-Wint-to-pointer-cast;-Wvoid-pointer-to-int-cast"
      ninja -C build/port/macos-arm64-warn -k 0 <the src/ objects>

  Counts are unique source locations (a header warning counts once, not once per TU).
- Text scans: `tools/port/layout64_scan.py src out.json` (raw `(char*)p + N`, `F(x, 0xNN)` macros, `char padNN[]`,
  plain `long`, `(int)this` / `(int)&x`).
- Raw offset accesses are ticket 022's (docs/tickets/022-offset-sites.tsv, clang-AST based, 4591 sites with class and
  status); this lane does not re-inventory them, it fixes the ones on the startup path.

## Totals (baseline before this lane's fixes)

| category | count | 64-bit effect |
|---|---:|---|
| pointer -> int casts (`-Wpointer-to-int-cast`, `-Wvoid-pointer-to-int-cast`) | 211 | truncation: crash when the int goes back to a pointer |
| int -> pointer casts (`-Wint-to-pointer-cast`, `-Wint-to-void-pointer-cast`) | 113 | the other half of the same round trips: an `int` field / parameter that holds a pointer |
| `-Wshorten-64-to-32` | 161 | size_t / ptrdiff_t into int: sizes and indices < 2^31, benign (not fixed) |
| raw `(char*)p + N` text matches | 230 | ticket 022 (AST count: 1535 kind A; open: E 267, N 692, U 352, V 9) |
| `F(x, 0xNN)` field macros | 62 | game_tick.cpp / party_tracks.cpp; same as raw offsets |
| `char padNN[]` / `unkNN[]` arrays | 1278 | harmless unless the pad hides a pointer that is reached by offset or by a differently typed view |
| plain `long` | 67 | 8 bytes on LP64; none found in a serialised record (file I/O goes through stream_base's typed slots) |
| `(int)this` / `(int)&x` text matches | 33 | subset of the casts above |

Files with p2i + i2p + raw offsets + F macros > 0: 90. Top of the list:

| file | p2i | i2p | raw | F() |
|---|---:|---:|---:|---:|
| src/game/script/script_interpreter.cpp | 17 | 24 | 4 | 0 |
| src/game/inventory_panel/game_tick.cpp | 0 | 0 | 3 | 31 |
| src/game/inventory_panel/party_tracks.cpp | 0 | 0 | 1 | 31 |
| src/rgl/dynamic/fx_anim.cpp | 31 | 0 | 0 | 0 |
| src/game/mission5/mission_5.cpp | 3 | 2 | 19 | 0 |
| src/game/inventory_panel/mission_ctx.cpp | 5 | 2 | 17 | 0 |
| src/game/config/config.cpp | 0 | 8 | 13 | 0 |
| src/game/ui/named_rec.h | 20 | 0 | 0 | 0 |
| src/game/agent4/agent_logic.cpp | 10 | 1 | 8 | 0 |
| src/game/mesh_slot/mesh_appear.cpp | 16 | 0 | 1 | 0 |
| src/game/mesh_slot/mesh_terrain.cpp | 2 | 6 | 8 | 0 |
| src/game/char_sheet/save_panel.cpp | 3 | 0 | 12 | 0 |
| src/rgl/mesh2/gui_edit.cpp | 0 | 12 | 1 | 0 |
| src/game/mesh_slot/mesh_pass.cpp | 1 | 12 | 0 | 0 |
| src/game/char_sheet/save_state.cpp | 0 | 0 | 13 | 0 |
| src/game/terrain_a/terrain_a.cpp | 8 | 1 | 3 | 0 |
| src/game/mesh_slot/mesh_scene.cpp | 4 | 3 | 5 | 0 |
| src/game/inventory_panel/note_window.cpp | 4 | 1 | 7 | 0 |
| src/game/mission5/game_2.cpp | 3 | 8 | 0 | 0 |
| src/game/mesh_slot/mesh_start.cpp | 4 | 3 | 4 | 0 |

The rgl core (resource manager, mesh, material, texture, shader, stream) is nearly clean: its only casts were the render
queue (`idx_weight` entries holding the mesh pointer) and fx_anim's bone stride. The truncations sit in the game layer
and in the records the game layer views with other types.

## Recurring shapes and the fix used

| shape | example | fix |
|---|---|---|
| an `int` member that holds a pointer | `MissionUi::f124` (the widget's Font), `renderer::width` (really the IDirect3D9) | the member with its real type and name |
| an `int` parameter that carries a pointer | `Font::layout_text(.., int arg, ..)`, `Font::draw_glyph(.., int arg)` (the colour word) | the real pointer parameter (the mangled name changes: manifest row updated) |
| one 8-byte POD record shared by a vector instance for two uses | `idx_weight` / `E8`: (vertex index, weight) and (mesh pointer, weight) | an anonymous `union { int index; mesh* m; }` keeps the one type (so the one shared template instance) and gives the pointer its type |
| a record view over a pad | `sort_rec { char pad[0x218]; __int64 key; }` read as a mesh | the real class: `rec_less(rgl::mesh* const*, ..)` reads `mesh::sortkey` |
| integer address arithmetic | `(int)elems + i * sizeof(FxBone)` | `(char*)elems + i * sizeof(FxBone)` (identical cl code; `((FxBone*)elems)[i]` moved the row further) |

Verification of every src/ edit: the MSVC object of every TU that sees the edit is compiled before (a detached worktree
of the base commit) and after, and compared section by section (code and data bytes plus relocations by symbol name;
renamed symbols mapped; .sxdata / .debug ignored). Identical objects mean every row, byte-exact or not, is unchanged.

## Status

### Fixed

- renderer (startup path): `renderer::d3d` (0x74c, was `int width`) and `renderer::backbuffer_format` (0x750, was
  `int height`), published by GameApp::InitDeviceObjects; `renderer::use_df24_shadow` (0x615). The function at
  0x4bc350 was `apply_input_settings(InputSet*)` called on `(InputSet*)&g_render`: it is the shadow depth-format probe
  (`IDirect3D9::CheckDeviceFormat` for D24S8, then DF24), now `renderer::check_shadow_formats()`.
- GUI text: `MissionUi::font` (+0x124, was `int f124`) and `MissionUi::set_font(Font*)` (vtable +0xa0, was
  `set_pos(int)`); `Font::layout_text` / `Font::draw_glyph` take `const unsigned* color` (was `int arg`).
- render queue: `idx_weight::m` / `E8::m` (the queued mesh), `rec_less` on `rgl::mesh`; render_batch::render and
  render_batch::add_mesh use them.
- fx_anim bone stride (`(char*)elems`).
- GUI render tree: every widget's `render(scene*, float*, int c, int d)` (vtable +0x28, 17 classes) takes the clip
  corners as `float*` (ContainerBase::render 0x490710 passed `(int)&l.mx`, progress_bar's caller `(int)zero`).
- the dialog box: `NamedRec::answer` (`int*`, +0x4c) / `NamedRec::text` (`tstring*`, +0x50), `setup2(.., int* answer)`;
  the save screen passes `&sel220` / `&t224` instead of `(int)((char*)this + 0x220)`; the widget pointers +0x2c..+0x40
  are pointer-sized so the view keeps ResultWindow's 64-bit layout.
- `make_editor_list(parent, name, UiD* scroll)` (was `int`), 0x4f62f0, and its six callers.
- FxElem `add_psys(rgl::mesh*)` / `add_group(rgl::lod_mesh2*)` (were `int`), with their mesh_appear / world_data /
  mesh_scene callers.
- shadow map: `ShadowMap::target` is `skin*`, `target2` is `texture*` (the ShSkin / ShTexRes views with pads are gone);
  mission_ctx's `shadow_stale` reads `texture::last_frame`; the cam / level comparisons compare pointers.
- `scan_saves(int* ids, ..)` (the load-game list), `PartyMgr::get` as `pages[k >> 4] + (k & 0xf)` (5 copies),
  the campaign screen's ui_terrain button (`MissionUi*`), `IDirect3DDevice9::ShowCursor` instead of a vtable-offset
  call, pick_entity's PlacedObj indexing, mesh_game's skeleton local.

- font glyph table (startup: font_data.xml has codes up to 1613, so Font::load inserts into the hash map): GlyphMap's
  members now have the library's widths (`size_t` count / mask / max index, a `std::vector<ptrdiff_t>` of node
  pointers, the list allocator word pointer-sized), GlyphNode is the list node (next, prev, key, Glyph), HashTab's
  mask / max index are `size_t`, and scalar4 carries a pointer member (`union { int v; void* p; }`) for the bucket fill
  of HashTab::insert and E68's materials. Checked with clang: GlyphMap matches port/stl's `_Hash` (head +16, buckets
  +32, mask +64, max index +72, sizeof 80) and the node's key / value sit at +16 / +20.

### Remaining (open)

After the fixes above (same measurement, unique locations): pointer -> int 75 (was 211), int -> pointer 75 (was 113),
`-Wshorten-64-to-32` 163.


See the per-file warning list (`build/port/macos-arm64-warn`, rebuild to refresh; `-Wpointer-to-int-cast` and
`-Wint-to-pointer-cast` lines) and ticket 022's TSV. Next by risk, all in-game rather than menu:

- `trigger_entity` (0x490430) returns the scene entity as `int`; the script interpreter casts it back (5 sites).
- terrain: `Terrain::arr2[25]` / `arr3[15][25]` hold mesh pointers as `int`; `TerrainLayer::fc` / `f10` hold
  materials as `int` (terrain_a.cpp, terrain_emit.cpp, mesh_terrain.cpp); `GroundType` lookups walk with `int p`.
- world map build (game_2.cpp): `int cmv = (int)new rgl::mesh()`, `add_cell_mesh(cell, int)`, the ents `vector<int>`.
- mesh_scene / mesh_appear: the item mesh slot `int* p = (int*)(outbase + 0x10)` holds a lod_mesh pointer.
- `setup_item_meshes(.., int weapon_flags_ptr, ..)` (agent_hud, mesh_game, script_interpreter).
- the script interpreter's presentation calls (`create_text_overlay((int)&text)`, `add_item(overlay, (int)&text)`,
  `ElemList::call0/call1(mesh, (int)ptr)`), `renumber((int)buf, ..)` / `run_line` in the debugger.
- mesh_start.cpp's `unsigned p = (unsigned)objs->first` walk with raw +0x74 / +0x24 offsets.
- harmless as they stand (value ignored or never a pointer): `Create3DEnvironment((int)hInstance)` (unused),
  `ShadowMap::read_pixel((int)g_render.dev, ..)` (unused), `m_dw334 = (DWORD)"m"`, quest_page's `(int)(long)` id.
- `-Wshorten-64-to-32` (161): size_t / ptrdiff_t into int; not fixed, sizes stay below 2^31.
