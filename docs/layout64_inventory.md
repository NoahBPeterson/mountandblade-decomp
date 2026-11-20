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
- `trigger_entity` (0x490430) returns `void*` (the prop's `PlacedObj::ent`, or the agent's `Agent::body` by
  `pages[idx >> 4][idx & 0xf]` instead of `page + rem * 0x5928 + 0x250`); play_sound keeps it as `char*`.

- terrain (batch 3): `Terrain::layer` is the 15 real records `TerrainLayer[15]` at +0x18 (name, type, flags, path_bit,
  `skin* mat`, `skin* mat2`, uv_scale): the old `TerrainLayer[14]` view at +0x28 and the `tlayer` view 0x10 bytes early
  are gone (load_layer fills `layer[idx + 4]`), LayerNames constructs / destroys the names through the records (the name
  is raw tstring storage so the base keeps the original's construction order); `GroundType::mat` / `mat2` are `skin*`;
  `Terrain::water_mesh[5][5]` / `layer_mesh[15][5][5]` are `rgl::mesh*` (were `int arr2[25]` / `arr3[15][25]`); the
  ground-type name walk uses a `GroundType*` with an intptr_t compare.
- the sco terrain description travels as `TerrainDesc*`: `Terrain::load_layer(TerrainDesc*, int)` (reads
  `layers[idx].type / data[x * 420 + y]`), `generate(rgl::world*, TerrainDesc*, char)` (the scene was an `int seed`),
  `update(TerrainDesc*, ..)`; manifest symbols updated.
- world map build (game_2.cpp): the cell meshes are `rgl::mesh*` (`cmv`), `add_cell_mesh(entity*, rgl::mesh*)`,
  `GameMapView::ents` is `std::vector<rgl::mesh*>`, the wind light is a `Light*`.
- tools/port/objcmp.py: the per-symbol before/after MSVC object comparison used for every edit (`objcmp.py cmp
  [--changed] TU..`, `objcmp.py includers HEADER`).

- item display meshes: `setup_item_meshes(.., rgl::lod_mesh2** meshes, int* unused, ..)` (was `char* outbase, int`;
  the slots are `meshes[4..7]` / `[12..15]`, mesh_scene's `int* p = (int*)(outbase + 0x10)` walk is a `lod_mesh2**`),
  agent_hud clears the 22 display pointers as `node_groups*`.
- FxDef members for the effect list calls: `add_group(rgl::lod_mesh2*)` (0x478fb0, was `IdHolder::add(int)` over a pad
  view), `elem_add_psys(int, rgl::psys*)` / `elem_set_data(int, void*)` (0x488e20 / 0x488e40, were
  `ElemList::call0/call1(int, int)` over a 0x1dc-byte `ElemVt` view); the script's EntityMeshes forwards to them.
- the script debugger: `DialogBlock::run_line(__int64* locals, int*, int* script)` / `renumber(__int64* locals, ..)`;
  `View::create_text_overlay(const tstring*, int)` (was `int arg`).
- mesh_start's weather walk reads the scene's `NavData::cells` as `NavCell*` (`pos`, `ids[8]`; was `unsigned p` with
  +0x74 / +0x24 / += 0x88); `Terrain::destroy_water(void* unused)` (callers passed pointers as int).

- the editor brush: `Terrain::brush(TerrainDesc*, ..)` reads `desc->layers[i]` (the `BrushElem_s2c2b0` / `Elem_s2ce00`
  views with `int name[4]` are gone); their methods are `TerrainDescEntry::clear` (0x52ce00) / `put` (0x52ce70).
- `Terrain::water_layer_type` (+0x280, was `pad280`; the constructor zeroes it); tessellate's
  `*(void**)((char*)this + 0x60)` is `layer[1].mat2`.
- mission_ui.h keeps a declaration-only polymorphic `ElemVt`: without a polymorphic class declared there cl 13.10 swaps
  two loads in video_page.cpp's Recf0::frame_update (0x4d63b0, run_all DIFF after batch 2); a forward declaration or
  including fx.h does not restore it.
- `EventContext::key_a` is `rgl::lod_mesh2*`; `trigger_item` (0x48cec0) returns `rgl::lod_mesh2*` and
  `trigger_mesh_obj` (0x48cf40) `rgl::mesh*`, the prop entity's lists read as pointer vectors.
- `c49e780::View::items` is a `std::vector<scene_obj*>`; `push` goes through the shared pointer-vector push_back
  (0x4936b0, `vector<Party*>`'s) instead of `vector<int>`'s.

- integration lane (2026-11-19, found running the native build, many with an ASan build): RenderScene's members before
  fog; mesh_part is the skin it views; render_cache.h uses include/renderer.h; MissionUi::get_xy's mesh material/slots;
  Font / clip_pair / C4c3050 texts / manifold view / music_name / loaded_snd_table / EmitDef / flora kinds (ptype) / PMesh
  (gui_edit.h) / E176 (material pointers) sized for 64-bit; HashTab::insert returns its HOut*; WVt slot 0 is a virtual
  destructor (Itanium's two destructor slots); IDENTITY_VALUE goes through rgl::identity_frame() (arm64 passes a struct
  result pointer in x8, not as the first argument). See port/INTEGRATION_STATUS.md.

- integration lane, second session (2026-11-19): Skin::read's voice lookup through SoundGroup; PresentationRec in
  include/prsnt_rec.h (load_used indexes it; MenuKind / MenuKindRec name the string); MissionTemplate::triggers is a
  TriggerVec; the screens' fields through head_rec / C4c2fb0 (were `slot_as<int>(n)[k]`); ItemRec's item source at
  `&owned`; mesh_node / NibbleObj derive from rgl::mesh (no padded views); fix_skeletons reads frame_set and the bone's
  real leading members; growable_array is graph_obj's words (Local derives from it, no pads); simple triggers are
  trigger_entry (include/trigger_entry.h); Troop's tail pad dropped (alignment gives 0x508, asserted); NavGraph's
  first vector; grid_cell carries rgl::cell's tail; the 0x88-byte ground faces (ground_g136 x2, NavNode, GeomFace,
  Rec88) end in their `face*`; ChainGraph is rgl::vec members with sizeof strides.

### Remaining (open)

After batch 3 (brush, key_a / trigger_item, View): pointer -> int 33, int -> pointer 37, `-Wshorten-64-to-32` 159.
Before it (same measurement, 187 src/ objects, unique locations): pointer -> int 37 (was 211; 75 before the
terrain / item-mesh / script batches), int -> pointer 41 (was 113; 75), `-Wshorten-64-to-32` 162. By file: script_interpreter
.cpp 20 + .h 5, agent_logic 7, gui_edit 6 (the uninitialized-copy helpers called through casts), terrain_render 4, trigger_vec
3, party_2 3, options_dlg 3, mission_3 2, mesh_pass 2, pick_entity 2, and one each in 13 files.


See the per-file warning list (`build/port/macos-arm64-warn`, rebuild to refresh; `-Wpointer-to-int-cast` and
`-Wint-to-pointer-cast` lines) and ticket 022's TSV. Next by risk, all in-game rather than menu:

- emit_particle(15) (0x47ff60, byte-exact) still reads `layer[15].type` one past the array (water_layer_type at 32
  bits; out of range at 64 bits): the typed `layerno < 15 ? .. : water_layer_type` changes the row's bytes.
- `DialogBlock::step_less` / `pop_try` (0x49d7f0 / 0x49f7c0) take `int locals` and reuse that slot as an int scratch
  (`get_operand(.., &locals)`); the script's widget_mgr calls pass `(int)&text` (`create_game_button_overlay`,
  `add_item`, `animate_size/pos`): widget_mgr.h signatures.
- `PropEnt` (game/prop_ent.h) is a 32-bit-offset view of rgl::entity (`pad4[0x108 - 4]`, `Vec` of int words): mission_3
  and terrain_a's PlacedObj should use rgl::entity's `lods` / `meshes`; `View` keeps a 32-bit pad before `list` (+0x2c).
- `sel_list::add(int)` (0x50d0e0, widget_text.cpp / note_window.cpp) is really `gui_manager::add(MissionUi*)`: `dirty`
  (+0x1708) and a push onto `objs` (+0); note_window passes `(int)this->panel`. sel_list stays for the index sort lists.
- harmless as they stand (value ignored or never a pointer): `Create3DEnvironment((int)hInstance)` (unused),
  `ShadowMap::read_pixel((int)g_render.dev, ..)` (unused), `m_dw334 = (DWORD)"m"`, quest_page's `(int)(long)` id.
- `-Wshorten-64-to-32` (161): size_t / ptrdiff_t into int; not fixed, sizes stay below 2^31.
