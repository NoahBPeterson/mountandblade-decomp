# Proposed source tree

Status: DONE (2026-11-22; section 9 says how it went). Was: PROPOSAL, nothing had moved. Companion file: [`source_tree_moves.tsv`](source_tree_moves.tsv) (one row per current file; a later move tool consumes it). Generated from the tree at the commit that adds this file; the cpp line counts are those of that commit.

## 1. Principles applied

- Target shape as agreed: `src/engine/` (rgl layer), `src/game/` by subsystem, `src/platform/dxframework/`, `third_party/tinyxml/`.
- **No file is merged or renamed away from its original object.** Most of the 185 .cpp files are already one recovered original object (TU) or a proven piece of one (visibility twins in docs/tu_map.md, docs/original_tus.md, per-directory MIGRATION.md); 32 of them are under 100 lines for exactly that reason. They move one-to-one. Merging them would erase boundary evidence and is not proposed.
- Only four files are over ~3k lines (agent_logic 11,720; script_interpreter 7,044; mission_5 3,224; party_2 3,158). They are split in section 3, at the recovered TU boundaries where there are any and otherwise at class changes, in the file's existing (binary) function order.
- Names come from what the code does. The community names used: Module System operation interpreter (`Operation::execute`, `script`), `Mission`, `Party`, `Troop`, `Faction`, `Item`/`item_kind`, `Scene`/`terrain`, `dialog`, rgl:: for engine classes. Where a file is a grab-bag that was only ever a TU map entry (`cls_map`, `progress_bar`, `world_data`, `gui_edit`, `trigger_vec`), the new name is the dominant content and the description says it is mixed (section 6 lists them). Identifier renames (C4c3290 ...) are a later step.
- Headers: a header follows the .cpp whose code it declares (same stem). Every other header keeps its current owner: a header that sits in an old batch directory goes with that directory's new home, and the shared headers of `src/game/*.h` and `src/include/*.h` are placed by the class they declare (table in the TSV, column `note` says which rule placed each). Engine-wide headers go to `src/engine/<topic>/`; game-wide ones to the subsystem owning the class, `src/game/common/` only for the five that have no owner. **Proposed shared include root: `src/`** (one `-I src`; includes become `"engine/math/mat4.h"`, `"game/data/troop.h"`), replacing the `../../include/x.h` relative chains. No `src/include/` directory survives.

## 2. The tree

`lines` is the approximate size of the file (for split parts: the cut plus the shared prelude that every part keeps). Headers are listed by name only; their individual destinations and the rule that placed each are in the TSV. Per-file sidecars (`.manifest.txt`, `.nonmatching.txt`, `.partial.txt`, 349 files) always travel with their .cpp and keep its stem.

```
src/engine/{audio,core,fx,io,math,physics,render,resource,scene}
src/game/{agent/{ai,combat,hud,movement,physics},app,campaign,common,config,data,dialog,editor/terrain_page,input,mission,party,save,script,terrain,ui/{hud,screens/{character_sheet,conversation,face_generator,inventory,options},widgets}}
src/platform/dxframework
third_party/tinyxml
```

### `src/engine/`

the rgl layer (TaleWorlds rgl:: engine): everything that does not know about Mount & Blade game rules.

### `src/engine/audio/`

sound manager, FMOD and music-player declarations (headers only; the code is in game/ files today).

Headers (3): `fmod_api.h`, `music_player.h`, `sound_mgr.h`

### `src/engine/core/`

engine strings (tstring), out-of-line STL instantiation helpers, CRT declarations, pools and small POD element types.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `tstring.cpp` | 406 | 0x42bac0..0x42cb70 | engine string tstring: every out-of-line member | `game/strat/tstring.cpp` |

Headers (27): `alloc_construct.h`, `arr_set.h`, `block16.h`, `clock_table.h`, `crt.h`, `engvec.h`, `glyph_hash.h`, `glyph_table_calls.h`, `hnode.h`, `identity_value.h`, `junk_tag.h`, `layout32.h`, `pod_elems.h`, `pool.h`, `ptr_tag.h`, `rgl_base.h`, `rgl_globals.h`, `rgl_string.h`, `std_vec_insert_n.h`, `std_vec_out_of_line.h`, `std_vec_push_back.h`, `tail2.h`, `timer.h`, `tstring.h`, `vec_elems.h`, `vec_table.h`, `vec_xlen.h`

### `src/engine/fx/`

effect definitions (FxDef / FxElem): skeleton-driven particle effects, key-frame animation, envelope curves.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `fx_def_set_color.cpp` | 35 | 0x4218e0..0x421990 | FxDef::set_color | `rgl/scene/fxdef_set_color.cpp` |
| `mesh_list_color.cpp` | 32 | 0x421990..0x4219c0 | mesh_list::set_color (LOD list tint) | `rgl/scene/mesh_list_color.cpp` |
| `strategic_emitters.cpp` | 119 | 0x4219c0..0x421df0 | FxElem::set_color, strategic entity emitter list and update | `rgl/scene/strategic_emitters.cpp` |
| `fx_animation.cpp` | 225 | 0x421df0..0x422db0 | effect definition per-frame animation: FxDef::update, envelope and pose passes | `rgl/dynamic/fx_anim.cpp` |
| `fx_key_frames.cpp` | 333 | 0x422db0..0x423c00 | quaternion helpers, key-frame evaluators (vector/quaternion keys, slerp), parent chain | `rgl/dynamic/fx_keys.cpp` |
| `fx_def_init.cpp` | 82 | 0x428820..0x428b10 | FxDef constructor and init (element array from the skeleton resource) | `rgl/scene/fxdef_init.cpp` |
| `fx_support.cpp` | 144 | 0x428b10..0x428ff0 | effect-definition setup/render helpers: quaternion from matrix, envelope arrays, FxElem ctor | `rgl/scene/fx_support.cpp` |
| `fx_def_render.cpp` | 52 | 0x428ff0..0x4291c0 | FxDef::render | `rgl/scene/fxdef_render.cpp` |
| `fx_def_skin.cpp` | 111 | 0x4291c0..0x429670 | FxDef::skin (bone-blended particle mesh) | `rgl/scene/fxdef_skin.cpp` |
| `env_curve.cpp` | 78 | 0x4338e0..0x433bf0 | animated-parameter (EnvRec) helpers: easing inverse, phase set/reset, string hash | `game/agent/curve.cpp` |

Headers (6): `anim_set.h`, `env_curve.h`, `fx.h`, `fx_animation.h`, `fx_rgl_scene.h`, `keyframes.h`

### `src/engine/io/`

text and in-memory binary streams.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `text_stream.cpp` | 534 | 0x4150f0..0x4158ff | text stream open/close/write slots, stream root, named_object destructor | `rgl/text_stream.cpp` |
| `mem_stream.cpp` | 364 | 0x417010..0x417baf | in-memory binary stream, key-array serialisers, resource-record helpers | `rgl/mem_stream.cpp` |

Headers (4): `mem_stream.h`, `stream.h`, `stream_inline.h`, `text_stream.h`

### `src/engine/math/`

vectors, matrices, quaternions, gradient noise.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `noise.cpp` | 242 | 0x414ae0..0x4150ef | 3-D gradient noise and its fractal (octave) sum | `rgl/noise.cpp` |

Headers (9): `float_consts.h`, `mat4.h`, `mat4_include.h`, `noise.h`, `quaternion.h`, `rgl_math.h`, `rot2.h`, `scene_math.h`, `vec.h`

### `src/engine/physics/`

collision shapes, rigid bodies, the world collision grid, ragdoll bones, Newton physics set.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `collision_shapes.cpp` | 1521 | 0x407c10..0x40e940 | collision shapes (sphere, capsule, face, manifold, composite) and their contact generation | `rgl/shapes/shapes.cpp` |
| `world_cells.cpp` | 183 | 0x423c00..0x424100 | world collision grid cells: add_item, cell rectangle relocation, cell shape vectors | `rgl/dynamic/world_cells.cpp` |
| `world_grid.cpp` | 185 | 0x424100..0x425370 | entity shape registration, ground geometry vertex/face-group store, add_tree / add_shape | `rgl/dynamic/world_grid.cpp` |
| `rigid_body.cpp` | 308 | 0x425370..0x426490 | rigid-body entity: velocity, impulses, force step, contact resolution, integration | `rgl/dynamic/dynamic_entity.cpp` |
| `world_collide.cpp` | 350 | 0x426490..0x427d80 | world per-cell shape/entity queries, box tests, sort instances, on_entity / on_shape | `rgl/dynamic/world_collide.cpp` |

Headers (11): `bone.h`, `collision_shapes.h`, `constraint.h`, `height_field.h`, `phys_set.h`, `poly.h`, `rag_model.h`, `rigid_body.h`, `rigid_body_rgl_dynamic.h`, `shape.h`, `world_grid.h`

### `src/engine/render/`

Direct3D 9 render-state cache, rgl::mesh, particle systems, materials, fonts, lights, shadow maps.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `mesh.cpp` | 832 | 0x40e940..0x413b5f | rgl::mesh with particle emitters, morph key frames, skinning table, render-state setters | `rgl/particles/mesh_obj.cpp` |
| `particle_system.cpp` | 269 | 0x413b60..0x414adf | particle system: mesh pool, per-particle geometry builder, per-frame driver | `rgl/psys.cpp` |
| `mesh_edit_members.cpp` | 374 | 0x44e970..0x44f5d0 | rgl::mesh members: add_quad, append_skinned, scale, translate, transform | `game/agent4/agent_logic.cpp (lines 6433-6696)` |
| `renderer.cpp` | 428 | 0x461630..0x4625d0 | Direct3D 9 render-state cache (view setup, state reset, sampler/material/alpha setters) | `game/renderer.cpp` |

Headers (22): `d3d_caps.h`, `d3d_xform.h`, `dev_buffer.h`, `face_texture.h`, `font.h`, `light.h`, `mesh.h`, `mesh_parts.h`, `mesh_prims.h`, `mesh_rgl_particles.h`, `mesh_table.h`, `particle_system.h`, `render_batch.h`, `render_cache.h`, `render_flags.h`, `renderer.h`, `renderer_include.h`, `renderer_xform.h`, `shadow_map.h`, `skin.h`, `skinmagic.h`, `texture_ctor.h`

### `src/engine/resource/`

resource manager, resource (de)serialisation, skeleton loading.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `resource_io.cpp` | 722 | 0x415900..0x417010 | text-stream read slots, matrix/skeleton serialisers, rgl_config.txt writer, texture (D3DX), shader and material records | `rgl/core/resource_io.cpp` |
| `resource_manager_init.cpp` | 695 | 0x417bb0..0x41918f | resource manager first half: binary file streams, key tables, options.dat writer, render-target creation | `rgl/resource_manager.cpp` |
| `resource_manager.cpp` | 1218 | 0x419190..0x41e46f | resource manager second half: registration, serialisation, loading, destruction; mesh/skeleton serialisers, LOD groups | `rgl/core/resmgr.cpp` |
| `skeleton_load.cpp` | 47 | 0x50fc60..0x50fc60 | SkeletonBody::load from a TinyXML document | `game/inventory_panel/skeleton_load.cpp` |

Headers (6): `res_mgr.h`, `resource.h`, `resource_io.h`, `resource_manager.h`, `resource_manager_init.h`, `skeleton.h`

### `src/engine/scene/`

entities, world, frustum, scene rendering, strategic (map icon) entity.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `frustum_grid.cpp` | 140 | 0x41e470..0x41ef1a | view-frustum plane test and builder, world cell-grid helpers, entity link release | `rgl/core/frustum_grid.cpp` |
| `world_items.cpp` | 103 | 0x41ef30..0x41f1d0 | view frustum ctor, world emitter partition, entity removal | `rgl/scene/world_items.cpp` |
| `fx_element_pool.cpp` | 155 | 0x41f1d0..0x41fee0 | effect element destructor and psys-list helpers, scene teardown, chunked free-list pool | `game/entity.cpp` |
| `entity.cpp` | 118 | 0x41fba0..0x41fee0 | rgl::entity constructor and small virtual accessors/setters | `rgl/scene/entity_base.cpp` |
| `entity_recalc.cpp` | 102 | 0x41fee0..0x4201b0 | entity scaled-frame setter and bounds recalculation | `rgl/scene/entity_recalc.cpp` |
| `entity_render.cpp` | 105 | 0x4201b0..0x4205b0 | aabb::extend_box and rgl::entity::render | `rgl/scene/entity_render.cpp` |
| `scene_objects.cpp` | 461 | 0x4205b0..0x4218e0 | LOD selection, cell/ref pools, world teardown, entity base destructor, effect-definition destructor | `rgl/scene/scene_objects.cpp` |
| `entity_types.cpp` | 182 | 0x427d80..0x4285e0 | skeleton and static entity classes, emitter pool, entity factory, entity::load | `rgl/scene/entity_types.cpp` |
| `shape_load.cpp` | 83 | 0x4285e0..0x428820 | shape resource deep copy by type, load slots of the strategic and skeleton entities | `rgl/scene/shape_load.cpp` |
| `render_scene.cpp` | 282 | 0x429670..0x42a090 | render scene constructor and world::render_scene (frustum-culled draw, water reflection) | `rgl/scene/render_scene.cpp` |
| `strategic_entity.cpp` | 416 | 0x42a090..0x42b6d0 | strategic entity (party/town icons on the map): render, ground-shadow update | `game/strat/strat_entity.cpp` |
| `strategic_shadow.cpp` | 122 | 0x42b6d0..0x42bac0 | scene-world ambient colour, strategic entity shadow-map render target | `game/strat/strat_ctx.cpp` |

Headers (12): `emitter.h`, `frustum.h`, `frustum_grid.h`, `frustum_rgl_scene.h`, `scene.h`, `scene_entity.h`, `scene_rec.h`, `shape_clone.h`, `strat_ext.h`, `strategic_entity.h`, `view_params.h`, `world.h`

### `src/game/`

Mount & Blade game code, organised by subsystem.

### `src/game/agent/`

agents (soldiers and horses in a mission): stats, effects, ragdoll, frame update.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_item_pick.cpp` | 213 | 0x433040..0x4338df | weighted item pick (Agent::pick), agent class test, morph time clamp, shape argument constructors | `rgl/xmlcfg/agent_pick.cpp` |
| `agent_queries.cpp` | 1180 | 0x434b20..0x437770 | item references and stats, troop accessors, agent camera/steering, equipment queries, sound selection, height-field line test | `game/agent/agent.cpp` |
| `agent_stats.cpp` | 1080 | 0x437830..0x43a280 | placed-object record, agent combat-stat and order helpers, skill lookup, level queries, animation controller start | `game/agent2/agent_stats.cpp` |
| `agent_effects_sound.cpp` | 1052 | 0x441ff0..0x444c50 | int lists, record copy helpers, entities-in-box query, effect element/definition lists, emitter setup, SoundMgr::play, experience, effect spawn, dead queue, target scans, sound pickers, equipment display, wield change | `game/agent4/agent_logic.cpp (lines 2422-3363)` |
| `agent_ragdoll_blood.cpp` | 1017 | 0x44f5d0..0x452c00 | Font::draw_glyph, ranged-weapon mesh, placed-object append, Newton world creation, start_ragdoll, dust, blood, mesh normals, text layout, carried-item refresh, drop_held | `game/agent4/agent_logic.cpp (lines 6697-7603)` |
| `agent_frame_update.cpp` | 1299 | 0x45c220..0x460160 | update_attack, move_step, update, tick | `game/agent4/agent_logic.cpp (lines 10532-11720)` |

Headers (8): `agent_logic.h`, `agent_obj.h`, `agent_queries.h`, `agent_stats.h`, `anim_slot.h`, `animator.h`, `blow.h`, `missile.h`

### `src/game/agent/ai/`

agent AI: targeting, goals, path following, action choice.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_targeting.cpp` | 1462 | 0x43a280..0x43e190 | weapon-class names, hand position, body release, quiver, static body rebuild, light accumulator (0x43a280..0x43b100, the tail of the TU the old agent_stats.cpp began); agent-table iteration, nearest-enemy list, target pickers, aim-error update, input poll, nav-face tracking, move animation, animation blend update, path goals (0x43b100..0x43e190) | `game/agent4/agent_logic.cpp (lines 111-1462)` |
| `agent_ai.cpp` | 3179 | 0x444c50..0x44e970 | nav-mesh path search (find_path, reachable), goal and path planning, ammo top-up, weapon mode, attack start, update_ai, slot_class_impl (static, EAX), ai_update_control, cycle_slot, spawn_missile, spawn_prop, choose_action, ai_idle_tick, shoot | `game/agent4/agent_logic.cpp (lines 3364-6432)` |

Headers (3): `agent_ai.h`, `agent_ai_game.h`, `agent_types.h`

### `src/game/agent/combat/`

attack checks, damage, blow delivery and swing trace.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_attack_checks.cpp` | 881 | 0x43e190..0x441360 | font glyph metrics, std::vector / deque instances, std::logic_error family (0x40c2c0..0x40c670), aim point, capsule hit, attack direction, start_shoot, sound ids, can_hit, line of sight, fire check | `game/agent4/agent_logic.cpp (lines 1463-2233)` |
| `agent_damage.cpp` | 1172 | 0x452c00..0x4554c0 | message log add, label box, shield_hit, hit_damage, on_removed, ragdoll_update, reduce_blow, draw_name | `game/agent4/agent_logic.cpp (lines 7604-8665)` |
| `agent_blows.cpp` | 1217 | 0x457b40..0x45c220 | process_blow, swing_trace | `game/agent4/agent_logic.cpp (lines 9425-10531)` |

Headers (3): `agent_combat.h`, `agent_fx_sound.h`, `agent_types.h`

### `src/game/agent/hud/`

mission HUD driven by agent state.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_hud.cpp` | 1798 | 0x4f7d70..0x501a20 | mission agent re-init, mission HUD, status widget, camera tick, item mesh builder, game-log HUD | `game/agent5/agent_hud.cpp` |
| `hud_vector_instances.cpp` | 100 | 0x4fe0b0..0x4fe860 | vector<NotePage> / <Skin> / <RagModel> instantiations between the HUD and the editor dialog | `game/agent5/agent_vec.cpp` |

Headers (1): `agent_hud.h`

### `src/game/agent/movement/`

agent controls, corpse and death state.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_corpse_and_controls.cpp` | 869 | 0x4554c0..0x457b40 | spawn_corpse, tick_death, note_blow, update_controls | `game/agent4/agent_logic.cpp (lines 8666-9424)` |

### `src/game/agent/physics/`

agent ragdoll / static collision (Newton) glue.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `agent_static_collision.cpp` | 298 | 0x441360..0x441ff0 | Newton set: destroy world, add ragdoll bone, static tree-collision build, capsule body; Agent::build_static_collision | `game/agent4/agent_logic.cpp (lines 2234-2421)` |

Headers (1): `phys_set.h`

### `src/game/app/`

application entry: WinMain, GameApp, loading screen, window procedure, launcher-side option dialogs.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `options_dialog_and_module_tables.cpp` | 1585 | 0x4be742..0x4c2c50 | script-code tables, vector helpers, module-folder scan, app destructor, options dialog, device-object restore | `game/triggers/trigger_vec.cpp` |
| `world_app.cpp` | 420 | 0x4d3720..0x4d4b80 | mission object ctor/dtor, restore_defaults, application constructor and main window procedure | `game/options_panel/world_app.cpp` |
| `winmain_and_menu_screens.cpp` | 2296 | 0x4d92c0..0x4e3eb0 | WinMain, font load, application cleanup, main-menu / options / key-binding screen builders (TU 107) | `game/cls_map/cls_map.cpp` |
| `loading_screen.cpp` | 273 | 0x50d140..0x50fbf0 | loading screen: module table load, frame/quads meshes, record reader, music track | `game/inventory_panel/loading_screen.cpp` |
| `game_app.cpp` | 309 | 0x50f130..0x50f390 | main-loop game update, window procedure override, loading-screen step | `game/inventory_panel/game_app.cpp` |

Headers (2): `options_dialog_and_module_tables.h`, `winmain_and_menu_screens.h`

### `src/game/campaign/`

campaign map (world-map) state and screens: game tick, party tracks, map panel, new game, map building, loaders of the game state.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `game_data_party_quest.cpp` | 2036 | 0x490030..0x4958f0 | GameData party / quest / experience members, container-widget helpers, message-log quest lines | `game/world_data/world_data.cpp` |
| `campaign_screen_init.cpp` | 195 | 0x4ecce0..0x4ecce0 | CampaignScreen::init_screen, camera_mark_entities | `game/char_sheet/screen_init.cpp` |
| `campaign_screen_camera.cpp` | 182 | 0x4ed630..0x4ed6b0 | CampaignScreen::camera_update, MapWidget constructor | `game/char_sheet/screen_camera.cpp` |
| `map_widget_render.cpp` | 25 | 0x4ed6f0..0x4ed6f0 | MapWidget::render | `game/char_sheet/render_shape.cpp` |
| `game_tick.cpp` | 669 | 0x516ac0..0x518680 | campaign map frame tick: clock fan-out, daily colour blend, weather drift, weekly desertion report | `game/inventory_panel/game_tick.cpp` |
| `party_tracks.cpp` | 256 | 0x518680..0x5191e0 | party-track ageing, track ribbons, town training camp | `game/inventory_panel/party_tracks.cpp` |
| `map_panel.cpp` | 904 | 0x5191e0..0x51be20 | campaign map screen: target picker and info panel | `game/inventory_panel/map_panel.cpp` |
| `map_tooltip.cpp` | 213 | 0x51be20..0x51c090 | party tooltip helpers (separate original object) | `game/inventory_panel/map_tooltip.cpp` |
| `game_state_load.cpp` | 986 | 0x51d2b0..0x51db70 | game-state object: init/reset/ctor, create-game screen, map graph search, save-game loaders | `game/party/game.cpp` |
| `new_game_setup.cpp` | 697 | 0x51eb90..0x520b20 | party record reader, new-game start-up, troop item reset and level-up | `game/mission5/troop_rec.cpp` |
| `map_builder_and_data_load.cpp` | 1464 | 0x520b20..0x525160 | map and world-map scene builders, map.txt reader, Game::load_game_data and its record readers | `game/mission5/game_2.cpp` |

Headers (20): `campaign_screen.h`, `campaign_screen_camera.h`, `campaign_screen_init.h`, `data_files.h`, `event.h`, `event_pool.h`, `externs.h`, `game_data_party_quest.h`, `game_state_load.h`, `game_tick.h`, `map_builder_and_data_load.h`, `map_flags.h`, `map_icon.h`, `map_panel.h`, `map_widget.h`, `map_widget_render.h`, `mark_rec.h`, `new_game_setup.h`, `party_window_class.h`, `track_rec.h`

### `src/game/common/`

game-wide helpers and globals used by several subsystems.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `game_util.cpp` | 348 | 0x4320b0..0x4338df | small game helpers: team relation tests, min/max/clamp/round, 3x3 frame algebra, Newton force callback, tick timer | `rgl/xmlcfg/game_util.cpp` |

Headers (5): `game_globals.h`, `game_util.h`, `iface.h`, `level_global.h`, `shared_vars.h`

### `src/game/config/`

rgl_config.txt / options reader, launcher configuration and options dialogs, DirectInput and render-batch setup.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `launcher_options_dialog.cpp` | 465 | 0x4b8f40..0x4b9e70 | update-checker HTTP client, registry helpers, launcher options dialog, version check | `game/config/options_dlg.cpp` |
| `config.cpp` | 1852 | 0x4b9e70..0x4be730 | user folders, D3D start-up probes, screenshot writer, rgl_config.txt reader, DirectInput set, render-batch tables, record loaders | `game/config/config.cpp` |
| `launcher_config_dialog.cpp` | 291 | 0x4cf100..0x4d0620 | launcher configuration dialog (display modes, tabs, dialog procedure) | `game/options_panel/config_dialog.cpp` |

Headers (6): `config.h`, `config_block.h`, `core.h`, `ini_file.h`, `launcher_options_dialog.h`, `options.h`

### `src/game/data/`

game data records (Module System records: items, troops, factions, triggers, presentations ...) and their loaders.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `record_destructors.cpp` | 87 | 0x431400..0x4320af | deleting destructors of engine record classes, vector<Item> helpers, per-mission reset | `rgl/xmlcfg/records.cpp` |
| `item_kind.cpp` | 335 | 0x433bf0..0x434b20 | item definition helpers: mesh variant lookup, weapon-type predicates, carry placement, lookup by name | `game/agent/item_kind.cpp` |
| `troop_proficiency.cpp` | 37 | 0x437770..0x437830 | Troop::adjust (proficiency step by falloff) and the Missile constructor | `game/agent/troop_adjust.cpp` |
| `item_ref.cpp` | 154 | 0x4625d0..0x462ac0 | sound-slot scheduler, item mesh lookup, modifier-adjusted item stats | `game/ui/item_ref.cpp` |
| `troop_inventory.cpp` | 408 | 0x462ac0..0x4636d0 | troop inventory: item-to-slot mapping, slot tests, weighted random item picker | `game/ui/troop_inv.cpp` |
| `troop_inventory_stats.cpp` | 246 | 0x48be00..0x48c330 | inventory compaction, attribute and skill setters, inventory sort | `game/mission_ui2/stat_panel_x.cpp` |
| `object_table_spawners.cpp` | 190 | 0x499050..0x499830 | resource record to mesh to entity helpers, tableau table spawners | `game/world_data/obj_tbl.cpp` |
| `module_option_records.cpp` | 694 | 0x4c2c50..0x4c48d0 | module/option record classes (head_rec family), options loader, renderer setup, music/sound/string-pair loaders | `rgl/mesh2/rec_tables.cpp` |
| `trigger_records.cpp` | 46 | 0x4c3120..0x4d0620 | trigger record bss and trigger constructors | `game/options_panel/trigger_records.cpp` |
| `module_record_loaders.cpp` | 127 | 0x4c8ba0..0x4d1360 | module-system record loaders (item modifiers, flora, ground, skyboxes, mission types) | `game/options_panel/module_load.cpp` |
| `item_head_records.cpp` | 35 | 0x4ce870..0x4ce960 | constructors of the two item head records and shared deleting destructor | `game/triggers/item_rec.cpp` |
| `mission_item_records.cpp` | 206 | 0x4ce960..0x4cf0d0 | item / mission-object record types and per-frame updater | `game/options_panel/mission_items.cpp` |
| `ragdoll_models.cpp` | 97 | 0x4d3330..0x4d3720 | ragdoll bone loader (TinyXML) and model record constructors | `game/options_panel/ragdoll_models.cpp` |
| `module_files_loader.cpp` | 310 | 0x50d5e0..0x50d5e0 | module-data loader: one function opening every module data file in turn | `game/inventory_panel/module_files.cpp` |
| `record_slots_head_rec.cpp` | 14 | 0x595740..0x596c68 | slot 1/2/3 overrides of the head_rec record classes (mesh2 view) | `game/options_panel/rec_slots_hc.cpp` |
| `record_slots_screen_records.cpp` | 25 | 0x595740..0x596c68 | slot 1/2/3 overrides of the head_rec record classes (screen records) | `game/options_panel/rec_slots_c4c.cpp` |
| `record_slots_trigger.cpp` | 22 | - | trigger records slot 1/2/3 | `game/options_panel/rec_slots_tr.cpp` |

Headers (49): `e68.h`, `elem_calls.h`, `emit.h`, `env_row.h`, `game_data.h`, `game_parts.h`, `game_recs.h`, `ground_type.h`, `head_rec.h`, `head_rec_rgl_mesh2.h`, `item_head_records.h`, `item_kind.h`, `item_kind_include.h`, `item_mod.h`, `item_ref.h`, `item_ref_game_ui.h`, `list_modes.h`, `map10007.h`, `menu_rec.h`, `mission_item_recs.h`, `mode_rec.h`, `module_name.h`, `module_option_records.h`, `module_record_loaders.h`, `module_settings.h`, `object_table_spawners.h`, `presentation.h`, `prsnt_rec.h`, `prsnt_table.h`, `ptype.h`, `raw_place.h`, `rec34.h`, `rec_dtors.h`, `rec_table.h`, `record_destructors.h`, `scene_object.h`, `skill_cap.h`, `skill_rec.h`, `str_table.h`, `string_table.h`, `tableau.h`, `tableau_table.h`, `trigger_entry.h`, `trigger_rec.h`, `trigger_records.h`, `trigger_vecs.h`, `troop.h`, `troop_inventory.h`, `troop_inventory_stats.h`

### `src/game/dialog/`

conversation manager and dialog records.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `dialog_manager.cpp` | 433 | 0x460160..0x461200 | conversation manager: {token} expander, script-block reader, dialog records | `game/dialog_manager.cpp` |

Headers (3): `dialog_manager.h`, `dialog_mgr.h`, `dialog_rec.h`

### `src/game/editor/`

map / scene / mesh editor code (terrain brush, polygon-mesh picking and editing).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `editor_selection_and_translation.cpp` | 1750 | 0x4c48d0..0x4ca600 | editor record loaders, PolyMesh selection modes, view/wheel, translated names, main-page and editor dialog refresh (TU 102+103 merged) | `rgl/mesh2/gui_edit.cpp` |
| `poly_mesh_edge_face_ops.cpp` | 740 | 0x4ca600..0x4ce720 | screen-mode queue, graph editor edge/face operations, record releasers, skeleton fix | `rgl/mesh2/mesh_2.cpp` |
| `poly_mesh_edit_ops.cpp` | 530 | 0x4d1410..0x4d2df0 | polygon-mesh editor operations: split quad, extrude, add triangle, finish | `game/options_panel/mesh_edit.cpp` |
| `editor_widgets.cpp` | 281 | 0x4f5d20..0x4f6680 | key label, arrow-button and slider factories of the editor panels | `game/char_sheet/editor_ui.cpp` |
| `terrain_brush_dialog.cpp` | 605 | 0x4fe860..0x500ad0 | map editor terrain (brush) dialog procedure, one 8390-byte function | `game/agent5/editor_dialog.cpp` |
| `terrain_brush.cpp` | 469 | 0x52bf40..0x52cf60 | Terrain::update / clear_brushed / brush (the scene editor brush), TerrainDescEntry clear / put, Terrain::clamp_cell | `game/mission5/mission_5.cpp (lines 2869-3224)` |
| `poly_mesh_pick.cpp` | 302 | 0x52cf60..0x52dae0 | map editor per-frame update, polygon-mesh picking (pick_vert) | `game/mission5/mesh_3.cpp` |
| `poly_mesh_vert_ray.cpp` | 54 | 0x52dae0..0x52dcc0 | Mesh::vert_ray and pick_edge (visibility twin) | `game/mission5/mesh_vert_ray.cpp` |
| `poly_mesh_edge_ray.cpp` | 75 | 0x52dcc0..0x52dfd0 | Mesh::edge_ray and pick_poly (visibility twin) | `game/mission5/mesh_edge_ray.cpp` |
| `poly_mesh_poly_ray_edit.cpp` | 766 | 0x52dfd0..0x5305b0 | Mesh::poly_ray, toggle_flag, scale/transform/remove of the selection | `game/mission5/mesh_poly_ray.cpp` |
| `map_editor_update.cpp` | 330 | 0x5305b0..0x531590 | map editor per-frame update, prop list removal, selection | `game/map_editor/map_editor.cpp` |

Headers (8): `editor_selection_and_translation.h`, `editor_widgets.h`, `map_editor_update.h`, `mesh2_common.h`, `poly_mesh_edge_face_ops.h`, `poly_mesh_edit_ops.h`, `poly_mesh_pick.h`, `screen_cfg.h`

### `src/game/editor/terrain_page/`

the editor terrain page (brush settings panel).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `terrain_page_init.cpp` | 243 | 0x4f1460..0x4f2610 | TerrainPage::init_screen, frustum::init, region_name | `game/char_sheet/facegen_win.cpp` |
| `terrain_page.cpp` | 118 | 0x4f2730..0x4f2ae0 | TerrainPage::update and hex-text helpers | `game/char_sheet/terrain_page.cpp` |
| `terrain_page_update.cpp` | 166 | 0x4f34c0..0x4f34c0 | TerrainPage::frame_update, copy_clipboard | `game/char_sheet/terrain_page_update.cpp` |
| `terrain_page_input.cpp` | 167 | 0x4f3570..0x4f3cf0 | TerrainPage::read_fields, simulate, 2x2 rotation, paste | `game/char_sheet/terrain_page_input.cpp` |

Headers (5): `terrain_page.h`, `terrain_page_game.h`, `terrain_page_init.h`, `terrain_page_input.h`, `terrain_page_update.h`

### `src/game/input/`

key tables, key-code groups and the input-device record.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `key_groups.cpp` | 46 | 0x4320b0..0x432350 | key-code group tables | `rgl/xmlcfg/key_groups.cpp` |
| `input_device.cpp` | 399 | 0x4875e0..0x488560 | 0x878-byte input-device record: channel play, key poll, key-edge test, per-frame update, UTF-8 decoder | `game/mesh_slot/slot.cpp` |

Headers (5): `game_keys.h`, `key_groups.h`, `key_names.h`, `key_table.h`, `slot.h`

### `src/game/mission/`

mission (battle) scene: start-up, scene build, agent spawn, per-frame tick, HUD and orders.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `scene_container_grid.cpp` | 717 | 0x478830..0x47aa40 | scene container grid, nav mesh, mission part of the terrain object's TU | `game/terrain_b/mission.cpp` |
| `mesh_vertex_ops.cpp` | 588 | 0x47aa40..0x47cae0 | TU 63 part 1: mesh vertex rewrites, per-vertex lighting, mirror/seam builder, skin blends, ground snap | `game/mesh_slot/mesh_ops.cpp` |
| `scene_entities.cpp` | 642 | 0x47cae0..0x47e520 | TU 63 part 2: scene-entity builder, light passes, carried-item meshes, projectile update | `game/mesh_slot/mesh_scene.cpp` |
| `render_passes.cpp` | 381 | 0x47e520..0x47f650 | TU 63 part 3: per-pass render driver, polygon-topology rebuild | `game/mesh_slot/mesh_pass.cpp` |
| `character_meshes.cpp` | 255 | 0x47f650..0x47ff60 | TU 63 part 4: character mesh collector, scene-object loader, shadow-map passes | `game/mesh_slot/mesh_char.cpp` |
| `terrain_water_regen.cpp` | 905 | 0x480db0..0x482240 | TU 63 part 5: terrain water, re-tessellation, regeneration | `game/mesh_slot/mesh_terrain.cpp` |
| `agent_appearance.cpp` | 340 | 0x482240..0x4832b0 | TU 63 part 6: agent appearance builder | `game/mesh_slot/mesh_appear.cpp` |
| `agent_spawn_and_tick.cpp` | 443 | 0x4832b0..0x484590 | TU 63 part 7: mission agent spawner and per-frame mission tick | `game/mesh_slot/mesh_game.cpp` |
| `mission_start_setup.cpp` | 1010 | 0x484590..0x4875e0 | TU 63 part 8: Mission::start_setup (12 KB) and rounding helper | `game/mesh_slot/mesh_start.cpp` |
| `mission_context.cpp` | 1194 | 0x513b10..0x516ac0 | mission start-up object: setup, template pick, scene start, hero table, battle start, map-screen frame | `game/inventory_panel/mission_ctx.cpp` |
| `hero_ammo.cpp` | 50 | 0x513eb0..0x513f80 | GameData::clear_hero_ammo (separate original object) | `game/inventory_panel/hero_ammo.cpp` |
| `mission_frame_update.cpp` | 1297 | 0x525160..0x5284a0 | game-loop event handler, dialogue / conversation update, speaker and listener finders, the mission screen frame tick (0x5261e0), scene .sco save path | `game/mission5/mission_5.cpp (lines 114-1297)` |
| `mission_hud_and_orders.cpp` | 1684 | 0x5284a0..0x52bf40 | view controller, mission HUD markers, party heroes, agent bookkeeping, interaction probe, order markers and menu, order_agent, update_ui, cursor, find_position | `game/mission5/mission_5.cpp (lines 1298-2868)` |
| `pick_entity.cpp` | 187 | - | MissionBlocker::pick_entity | `game/char_sheet/pick_entity.cpp` |

Headers (17): `appearance_lods.h`, `camera.h`, `camera_class.h`, `inv_common.h`, `mesh_build.h`, `mesh_slot.h`, `mission.h`, `mission_context.h`, `mission_ctx_rec.h`, `mission_frame_update.h`, `mission_obj.h`, `mission_type.h`, `party_model.h`, `pick_entity.h`, `placed_obj.h`, `scene_container_grid.h`, `trial_mode.h`

### `src/game/party/`

map parties: stacks, orders, path following, party tree operations, stack loot.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `stacks_and_loot.cpp` | 872 | 0x48c330..0x48e5d0 | party stack editors, loot generators, item reference setters | `game/mission_ui2/mission_3.cpp` |
| `party_members.cpp` | 137 | 0x48f790..0x48fd60 | member count, add/remove, fill from party template | `game/mission_ui2/party.cpp` |
| `party_tree_ops.cpp` | 52 | 0x48fd60..0x490030 | party tree: re-add and collect stacks, faction relation | `game/mission_ui2/game_data.cpp` |
| `party_stacks_and_orders.cpp` | 1742 | 0x4b1160..0x4b4a00 | tableau render callback, colour_of, Party stack economy (value, xp, speed, skills, rotation), STL heap helpers, Event::bind_host, orders and planning, tick_stats, NavData::reset/can_move, collect_targets, plan_order, step_route, force_add_members, party_value | `game/party2/party_2.cpp (lines 47-1742)` |
| `party_track_allocator.cpp` | 8 | 0x4b2160..0x4b2160 | std::allocator<TrackRec>::construct (separate TU so callers stay opaque) | `game/party2/party_2_copy.cpp` |
| `party_think_and_nav.cpp` | 1462 | 0x4b4a00..0x4b8f40 | Party::think, march_time, push_event / push_party_event, Event::detach / commit, leave_battle, NavData::find_path, Event nav animation, start_path, update_nav, update_flags, update_hud_text, Party::tick | `game/party2/party_2.cpp (lines 1743-3158)` |

Headers (5): `party.h`, `party_map.h`, `party_x.h`, `slot_array.h`, `stacks_and_loot.h`

### `src/game/save/`

save-game writers and state serialisation.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `save_game.cpp` | 196 | 0x4e87d0..0x4e8e70 | save_game, SaveCtx::write, GameData::load_header, save-path log | `game/char_sheet/save_game.cpp` |
| `save_state.cpp` | 240 | 0x4e8f10..0x4e97e0 | GameData::save_state, trigger/clock writers | `game/char_sheet/save_state.cpp` |
| `save_records.cpp` | 245 | 0x4e9850..0x4e9fe0 | Troop / ItemRef / Event / mission record writers | `game/char_sheet/save_records.cpp` |

Headers (4): `packed_state.h`, `save_game.h`, `save_records.h`, `save_state.h`

### `src/game/script/`

Module System operation interpreter (Operation::execute and its helpers).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `script_support.cpp` | 739 | 0x49b1c0..0x49cc60 | Mission::place, set_font_mesh, GameData gold / quest / label helpers, battle icon, order arrow, check_slot, Mission::order (the slices c49aca0..c49c2c0) | `game/script/script_interpreter.cpp (lines 163-739)` |
| `operation_execute.cpp` | 6221 | 0x49cc60..0x4b0940 | validators (register-convention statics), operand accessors and try linker (Elem / DialogBlock), the view Camera, party attach / spawn / deploy helpers, trigger enter/exec, trigger_item_mesh, Operation::execute (4,100-line switch over 591 opcodes) | `game/script/script_interpreter.cpp (lines 740-6798)` |
| `dialog_block_runner.cpp` | 408 | 0x4b0940..0x4b1160 | DialogBlock::run_line and DialogBlock::emit: the control-opcode block runner (opcodes below 30) | `game/script/script_interpreter.cpp (lines 6799-7044)` |

Headers (4): `script_globals.h`, `script_interpreter.h`, `script_opcodes.h`, `script_state.h`

### `src/game/terrain/`

world-map terrain, polygon mesh, wind and dust particles.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `wind_helpers.cpp` | 103 | 0x461200..0x461630 | agent hit-point init, std::copy instances, triangle normal, wind helpers | `game/terrain_a/wind.cpp` |
| `scene_paths_and_wind_noise.cpp` | 348 | 0x4636d0..0x464200 | scene/module path helpers, colour-ramp sampler, wind noise, shadow map | `game/terrain_a/game_state.cpp` |
| `terrain_cells.cpp` | 1071 | 0x464200..0x466cd0 | sco terrain description reader, cell record, position-to-cell, terrain generation passes | `game/terrain_a/terrain_a.cpp` |
| `terrain_particles.cpp` | 425 | 0x466cd0..0x467f80 | terrain particle emitters, triangle corners, sun | `game/terrain_a/terrain_emit.cpp` |
| `terrain_render.cpp` | 1860 | 0x4681b0..0x46e590 | terrain rendering and the std instantiations it calls | `game/terrain_a/terrain_render.cpp` |
| `terrain_scene_renderer.cpp` | 746 | 0x46e590..0x470a00 | shader-path scene renderer, polygon-mesh topology object | `game/terrain_b/terrain_b.cpp` |
| `poly_mesh.cpp` | 1784 | 0x470a00..0x478830 | polygon-mesh geometry builder (build_geometry, build) and neighbours | `game/terrain_b/poly_mesh.cpp` |
| `dust_particles.cpp` | 190 | 0x4f47f0..0x4f49c6 | Terrain::update_particles / emit_cell (dust cells), vector reserve instances | `game/char_sheet/dust.cpp` |

Headers (24): `agent_rec.h`, `cell_grid.h`, `common.h`, `dust_particles.h`, `emit.h`, `face.h`, `face_gen.h`, `frustum.h`, `game_data.h`, `misc.h`, `nav_data.h`, `physics.h`, `poly_mesh.h`, `records.h`, `render_batch.h`, `scene_file.h`, `scene_paths_and_wind_noise.h`, `shadow_map.h`, `terrain.h`, `terrain_desc.h`, `terrain_game_terrain_b.h`, `terrain_scene_renderer.h`, `troop_mgr.h`, `wind_helpers.h`

### `src/game/ui/`

UI root (gui_manager), message log, UI globals.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `gui_manager.cpp` | 218 | 0x42cb70..0x42d820 | reporting helpers (message boxes, log flush), widget dispatch, UI root gui_manager | `game/strat/gui_manager.cpp` |
| `msg_log_ui.cpp` | 29 | 0x4f1390..0x4f13f0 | gui_manager message log ring pop / flush | `game/char_sheet/msg_log_ui.cpp` |

Headers (6): `gui_manager.h`, `msg_log_ui.h`, `note_block.h`, `ui_globals.h`, `view_rect.h`, `window_mgr.h`

### `src/game/ui/hud/`

mission item HUD callbacks.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `mission_hud_buttons.cpp` | 195 | 0x4d4b80..0x4d5410 | mesh editor selection handler, mission object dtor wrapper, mission button handler, item callbacks | `game/options_panel/hud.cpp` |
| `mission_item_source.cpp` | 171 | 0x4d5410..0x4d5ba0 | mission item source per-frame handler, menu record slot 3 | `game/options_panel/hud_source.cpp` |

Headers (1): `mission_hud_buttons.h`

### `src/game/ui/screens/`

full-screen UI screens (party, notes, save panel, quest log, menus ...).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `mission_window.cpp` | 242 | 0x4dae40..0x4db810 | mission and meeting window screens | `game/cls_map/mission_window.cpp` |
| `progress_bar_and_load_screens.cpp` | 1370 | 0x4e3eb0..0x4e87d0 | progress_bar class, string drawing, save-list screen, ini_file helpers, PolyMesh spikes/ribbons, TableauTbl/Skin readers (TU 108) | `game/cls_map/progress_bar.cpp` |
| `save_panel.cpp` | 210 | 0x4ea850..0x4ea850 | save-list entity select and frame update (C4c31c0) | `game/char_sheet/save_panel.cpp` |
| `screen_misc.cpp` | 267 | 0x4ebc10..0x4ec460 | take_screenshot, RagModel copy instances, gui_manager::process, head_rec::frame_hook | `game/char_sheet/screen.cpp` |
| `quest_log.cpp` | 191 | 0x4ee230..0x4eea90 | quest log screen: refresh, scroll track, quest list | `game/char_sheet/quest_log.cpp` |
| `party_screen.cpp` | 1355 | 0x502cc0..0x507400 | party / prisoner management screen | `game/inventory_panel/party_screen.cpp` |
| `mission_menu_and_exchange_screens.cpp` | 925 | 0x531590..0x533a60 | mission start menu and party exchange screens | `game/map_editor/exchange_screens.cpp` |
| `scroll_list_screen.cpp` | 257 | 0x533a60..0x5342a6 | scroll-list screen and item-source hooks | `game/map_editor/list_screens.cpp` |
| `notes_window.cpp` | 757 | - | notes page builder (troop / faction / party / quest reports) | `game/inventory_panel/note_window.cpp` |

Headers (9): `mission_menu_and_exchange_screens.h`, `party_screen.h`, `party_screen_class.h`, `progress_bar_and_load_screens.h`, `quest_log.h`, `result_window.h`, `save_panel.h`, `screen_misc.h`, `screen_records.h`

### `src/game/ui/screens/character_sheet/`

the character sheet screen.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `character_sheet_update.cpp` | 229 | 0x4eeb10..0x4eeb10 | CharSheet::frame_update | `game/char_sheet/cs_update.cpp` |
| `character_sheet_troop.cpp` | 144 | 0x4ef420..0x4ef6d0 | CharSheet::apply_to_troop, skill rows, Troop::steps_to_point | `game/char_sheet/cs_troop.cpp` |
| `character_sheet_weapons.cpp` | 73 | 0x4f04f0..0x4f06f0 | weapon class caption and CharSheet::update_weapons | `game/char_sheet/cs_weapons.cpp` |
| `character_sheet_helpers.cpp` | 167 | 0x5093d0..0x509770 | ini_file::get_int, message window init, CharSheet::load_troop / fill_skills, make_button | `game/inventory_panel/char_window.cpp` |
| `character_sheet_detail.cpp` | 151 | - | CharSheet::update_detail | `game/char_sheet/cs_detail.cpp` |
| `character_sheet_init.cpp` | 473 | - | CharSheet::init_screen: character window builder | `game/inventory_panel/char_sheet.cpp` |
| `character_sheet_panels.cpp` | 112 | - | CharSheet::refresh_panels | `game/char_sheet/cs_panels.cpp` |
| `character_sheet_view.cpp` | 217 | - | CharSheet::open_view: 3-D portrait view | `game/inventory_panel/view_open.cpp` |

Headers (8): `char_sheet_panel.h`, `character_sheet_detail.h`, `character_sheet_panels.h`, `character_sheet_troop.h`, `character_sheet_update.h`, `character_sheet_weapons.h`, `cs_common.h`, `cs_mat.h`

### `src/game/ui/screens/conversation/`

the conversation screen.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `conversation_init.cpp` | 213 | - | Rec1208::init_screen (conversation screen) | `game/char_sheet/conv_open.cpp` |
| `conversation_refresh.cpp` | 152 | - | Rec1208::refresh | `game/char_sheet/conv_refresh.cpp` |

Headers (3): `conv_screen.h`, `conversation_init.h`, `conversation_refresh.h`

### `src/game/ui/screens/face_generator/`

the face generator screen.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `face_generator_update.cpp` | 216 | 0x4ea8e0..0x4eb050 | FaceGen::update / frame_update | `game/char_sheet/cam_view.cpp` |
| `face_generator_refresh.cpp` | 246 | 0x4eb270..0x4ebb80 | FaceGen::update_view / refresh, Animator randomise / pack_state | `game/char_sheet/facegen_misc.cpp` |
| `face_generator_init.cpp` | 227 | 0x4f49d0..0x4f49d0 | FaceGen::init_screen | `game/char_sheet/facegen_init.cpp` |

Headers (4): `face_gen.h`, `face_generator_init.h`, `face_generator_refresh.h`, `face_generator_update.h`

### `src/game/ui/screens/inventory/`

inventory / trade screen.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `inventory_window_create.cpp` | 330 | 0x500ad0..0x501a20 | PartyWindow::create: inventory window builder, money label | `game/agent5/party_create.cpp` |
| `inventory_window.cpp` | 399 | 0x501a20..0x502cc0 | inventory window: equipment menu builder, slot factory, page builder, encumbrance refresh | `game/party/party_window.cpp` |
| `inventory_panel.cpp` | 1207 | 0x50fe20..0x513b10 | inventory / trade screen: frame step, slot state, grid refresh, tooltip, price rules, slot-transfer engine | `game/inventory_panel/inventory_panel.cpp` |

Headers (1): `inventory_window.h`

### `src/game/ui/screens/options/`

options and character panel pages.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `quest_page.cpp` | 289 | 0x4d5ba0..0x4d6300 | quest-log page and options-change handler | `game/options_panel/quest_page.cpp` |
| `keys_page.cpp` | 335 | 0x4d6b80..0x4d7270 | performance caption and controls (key assignment) page | `game/options_panel/keys_page.cpp` |
| `options_panel.cpp` | 657 | 0x4d7370..0x4d92c0 | options / character panel, faction report, character import / export | `game/options_panel/options_panel.cpp` |
| `video_page.cpp` | 252 | - | video options page change handler | `game/options_panel/video_page.cpp` |

Headers (1): `options_panel.h`

### `src/game/ui/widgets/`

widget base class, buttons, sliders, labels, progress bar, text box, drop-down, widget manager.

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `widget_virtual_slots.cpp` | 126 | 0x42cd60..0x42d170 | virtual slots of the widget base and derived widgets | `game/ui/widget_slots.cpp` |
| `widget_base.cpp` | 1672 | 0x488560..0x48b2d0 | widget base MissionUi (animation, flags, text, constructor), list helpers and neighbouring records | `game/ui/mission_ui.cpp` |
| `toggle_and_text_fit.cpp` | 142 | 0x48b2d0..0x48b550 | UI click sound, toggle widget, text fitting, slider constructor | `game/ui/ui_widgets_a.cpp` |
| `slider_value.cpp` | 28 | 0x48b550..0x48b5a0 | slider value readout and type code | `game/ui/ui_slider.cpp` |
| `slider_input.cpp` | 59 | 0x48b5a0..0x48b6e0 | slider mouse handler (grab, drag, release) | `game/ui/ui_slider_key.cpp` |
| `label_and_progress_bar.cpp` | 81 | 0x48b6e0..0x48b930 | slider value setter, label widget, progress bar ctor/show | `game/ui/ui_widgets_b.cpp` |
| `progress_bar_value.cpp` | 28 | 0x48b930..0x48b960 | progress bar value getter | `game/mission_ui2/ui_d_value.cpp` |
| `progress_bar_input.cpp` | 63 | 0x48b960..0x48baa0 | progress bar key handler | `game/mission_ui2/ui_d_key.cpp` |
| `text_editor_widget.cpp` | 171 | 0x48baa0..0x48be00 | progress bar value setter, single-line text editor widget (UiE) | `game/config/ui_widgets_c.cpp` |
| `widget_overlay_api.cpp` | 303 | 0x48e5d0..0x48ef40 | widget manager animated setters, string record tables, script string / inventory add, scene props | `game/mission_ui2/slot_api.cpp` |
| `container_and_tableau_misc.cpp` | 235 | 0x48ef40..0x48f790 | container frame release, field panel relayout, party record reset, tableau camera setters | `game/mission_ui2/mission_4.cpp` |
| `hud_bar_widgets.cpp` | 793 | 0x4958f0..0x499050 | rgl::mesh helpers, slider factory, map-legend mesh, mission HUD bar widgets | `game/world_data/mesh_hud.cpp` |
| `widget_manager.cpp` | 398 | 0x499830..0x49ad20 | editor window widget manager: button/slider/dropdown factories, placement helpers | `game/config/widget_mgr.cpp` |
| `drop_down_list.cpp` | 80 | 0x49ad20..0x49b1c0 | DropDown::create list container, NodeA, vector<NodeA*> members | `game/config/widget_node.cpp` |
| `text_box.cpp` | 214 | 0x50cc60..0x50d520 | widget flag getter, text box ctor, index-list sorter, vector<Skin>::resize, 0x1f8 scene record reader | `game/inventory_panel/widget_text.cpp` |

Headers (17): `container.h`, `container_and_tableau_misc.h`, `hud_bar_widgets.h`, `map_list.h`, `mission.h`, `note_page.h`, `party.h`, `text_editor_widget.h`, `ui_common.h`, `ui_frame.h`, `ui_widget.h`, `ui_widgets.h`, `widget_base.h`, `widget_manager.h`, `widget_overlay_api.h`, `world.h`, `wret.h`

### `src/platform/`

platform-level shared headers (WinLicense markers, image addresses).

Headers (2): `image_addrs.h`, `wl_marker.h`

### `src/platform/dxframework/`

Microsoft DirectX SDK sample framework as modified by TaleWorlds (verbatim origin, 6 objects).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `d3dapp.cpp` | 1574 | 0x401000..0x403120 | DX9 sample framework application class (window, device loop), as modified by TaleWorlds | `dxframework/d3dapp.cpp` |
| `d3denumeration.cpp` | 711 | 0x4035d0..0x404480 | DX9 sample framework: adapter / device / mode enumeration | `dxframework/d3denumeration.cpp` |
| `d3dfont.cpp` | 561 | 0x404500..0x404f00 | DX9 sample framework: texture-based font | `dxframework/d3dfont.cpp` |
| `d3dsettings.cpp` | 874 | 0x4055e0..0x407230 | DX9 sample framework: settings and change-settings dialog | `dxframework/d3dsettings.cpp` |
| `d3dutil.cpp` | 88 | 0x407250..0x407250 | DX9 sample framework: D3DUtil_D3DFormatToString (only linked function) | `dxframework/d3dutil.cpp` |
| `dxutil.cpp` | 331 | 0x407660..0x407bc0 | DX9 sample framework: DXUtil_Timer, string conversion, CArrayList | `dxframework/dxutil.cpp` |

Headers (6): `d3dapp.h`, `d3denumeration.h`, `d3dfont.h`, `d3dsettings.h`, `d3dutil.h`, `dxutil.h`

### `third_party/tinyxml/`

TinyXML 2.5 (3 objects).

| file | lines | address range | what it holds | current file |
|---|---:|---|---|---|
| `tinystr.cpp` | 93 | 0x42d820..0x42d950 | TinyXML 2.5 string class | `tinyxml/tinystr.cpp` |
| `tinyxml.cpp` | 1120 | 0x42d9a0..0x42f6c0 | TinyXML 2.5 DOM classes | `tinyxml/tinyxml.cpp` |
| `tinyxmlparser.cpp` | 1281 | 0x42f720..0x431050 | TinyXML 2.5 parser | `tinyxml/tinyxmlparser.cpp` |

Headers (2): `tinystr.h`, `tinyxml.h`

## 3. Split plan for the four files over ~3k lines

How to read the cuts: `from`/`to` are 1-based inclusive line numbers in the file as it is at the commit that adds this document (the file is also keyed by a text marker so the move tool can re-find the cut if lines drift). Each cut is placed on the line after the closing brace of the last function of the previous part, so `#define`s, `static` helpers and comment blocks that lead into a function go with that function. Every part also receives the file's **prelude** (the lines before the first part: file comment, `#include`s, shared inline helpers and `#define`s). The move tool copies the prelude verbatim into each part; trimming each copy to what the part needs is manual follow-up work and is the main cost of a split.

General evidence rule used for every cut: (a) a TU boundary from `work/migrate/tu_map.csv` (forward call = reliable, class change = about 50%) wins; (b) otherwise a class change between two runs of functions in the file; (c) never a cut inside one function, and never a cut that separates a callee from a register-convention caller (see section 5).

### 3.1 `src/game/agent4/agent_logic.cpp` (11,720 lines, 0x43a280..0x460160) -> 11 files

Map: TU 45 `agent_3` 0x439fc0..0x43e190, TU 46 `agent_4`, 47 `phys_set`, 48 `agent_5`, 49 `agent_6` 0x444c50..0x44e970, TU 50 `agent_7` 0x44e970..0x460160, TU 51 `dialog_mgr` from 0x460160 (all class-change boundaries, low confidence, B regime). docs/original_tus.md A1 argues the whole 0x43a280..0x45f660 is one object (the register-convention statics `slot_class_impl` 0x4350c0 and `ui_weapon_class_name` 0x43a280 are called from several of these parts), and a 2026-11-12 test found no consistent visible-callee boundary. So the TU-map seams are the best-supported cuts available, but they are **not proven**: every cut below is a class change, and the plan cuts only at the five that the earlier waves already treated as module seams (agent_stats tail / agent_ai / agent_combat / phys_set / agent_fx_sound, which the current file marks with banners) plus class changes inside TU 50. The old seam at 0x43b100 (agent_ai) is dropped because it is no map boundary; TU 49 is kept whole.

| # | new file | from | to | lines (with prelude) | first function of the part | address range | why the cut is here |
|---|---|---:|---:|---:|---|---|---|
| 1 | `game/agent/ai/agent_targeting.cpp` | 111 | 1462 | 1462 | `0x43a280` static __declspec(noinline) void ui_weapon_class_name(int type, char* out) | 0x43a280..0x43e190 | TU 45 "agent_3" of the map (0x439fc0..0x43e190, class changes at both ends); the file starts at 0x43a280 because the first 0x2c0 bytes of the TU stay in agent_stats.cpp (old module seam, the register-convention static ui_weapon_class_name starts here). The old agent_ai seam at 0x43b100 is NOT kept: it is no map boundary |
| 2 | `game/agent/combat/agent_attack_checks.cpp` | 1463 | 2233 | 881 | `0x43e190` float Font::advance(int ch) | 0x43e190..0x441360 | TU 46 "agent_4" of the map: class change Agent|Font at 0x43e190 and Agent|PhysSet at 0x441360 (both low) |
| 3 | `game/agent/physics/agent_static_collision.cpp` | 2234 | 2421 | 298 | `0x441360` void PhysSet::destroy_world(int idx) | 0x441360..0x441ff0 | TU 47 "phys_set" of the map (5 functions, class PhysSet; class changes at both ends, low) |
| 4 | `game/agent/agent_effects_sound.cpp` | 2422 | 3363 | 1052 | `0x441ff0` void rgl::lod_mesh::add_mesh(int idx, rgl::mesh* val) | 0x441ff0..0x444c50 | TU 48 "agent_5" of the map (class changes PhysSet|Poser at 0x441ff0 and Agent|NavMesh at 0x444c50, low; 14 possible inner splits unexplored) |
| 5 | `game/agent/ai/agent_ai.cpp` | 3364 | 6432 | 3179 | `0x444c50` bool PolyMesh::find_path(int from, int to, int* out_faces, vector4* out_pos, int | 0x444c50..0x44e970 | TU 49 "agent_6" of the map, kept whole (about 3,190 lines with the prelude): class change Agent|NavMesh at 0x444c50, Agent|mesh at 0x44e970 (low) |
| 6 | `engine/render/mesh_edit_members.cpp` | 6433 | 6696 | 374 | `0x44e970` void rgl::mesh::add_quad(const vector4* p0, const vector4* p1, const vec2* uv0,  | 0x44e970..0x44f5d0 | start of TU 50 "agent_7" (map boundary 0x44e970, class change Agent|mesh); TU 50 is 5,280 lines and has to be cut inside: its first class run is rgl::mesh (engine code), ends at Font::draw_glyph (class change) |
| 7 | `game/agent/agent_ragdoll_blood.cpp` | 6697 | 7603 | 1017 | `0x44f5d0` void Font::draw_glyph(int ch, const vector4* pos, const vec2* scale, mesh* m, co | 0x44f5d0..0x452c00 | inside TU 50: class change rgl::mesh -> Font / Missile / PhysSet / Agent; ends before gui_manager::add (message log) |
| 8 | `game/agent/combat/agent_damage.cpp` | 7604 | 8665 | 1172 | `0x452c00` void gui_manager::add(tstring msg, int color) | 0x452c00..0x4554c0 | inside TU 50: class change Agent ragdoll/blood -> gui_manager message log and the damage chain |
| 9 | `game/agent/movement/agent_corpse_and_controls.cpp` | 8666 | 9424 | 869 | `0x4554c0` void Agent::spawn_corpse() | 0x4554c0..0x457b40 | inside TU 50: damage chain -> corpse / death state / control update; ends where process_blow (the caller of ui_weapon_class_name) begins |
| 10 | `game/agent/combat/agent_blows.cpp` | 9425 | 10531 | 1217 | `0x457b40` void Agent::process_blow(float unused) | 0x457b40..0x45c220 | inside TU 50: the blow delivery and swing trace pair (two functions of 7 KB each) |
| 11 | `game/agent/agent_frame_update.cpp` | 10532 | 11720 | 1299 | `0x45c220` void Agent::update_attack(float dt) | 0x45c220..0x460160 | inside TU 50: per-frame update chain; ends at 0x460160 where the dialog TU (TU 51) starts |

Contents: `agent_targeting.cpp`: weapon-class names, hand position, body release, quiver, static body rebuild, light accumulator (0x43a280..0x43b100, the tail of the TU the old agent_stats.cpp began); agent-table iteration, nearest-enemy list, target pickers, aim-error update, input poll, nav-face tracking, move animation, animation blend update, path goals (0x43b100..0x43e190); `agent_attack_checks.cpp`: font glyph metrics, std::vector / deque instances, std::logic_error family (0x40c2c0..0x40c670), aim point, capsule hit, attack direction, start_shoot, sound ids, can_hit, line of sight, fire check; `agent_static_collision.cpp`: Newton set: destroy world, add ragdoll bone, static tree-collision build, capsule body; Agent::build_static_collision; `agent_effects_sound.cpp`: int lists, record copy helpers, entities-in-box query, effect element/definition lists, emitter setup, SoundMgr::play, experience, effect spawn, dead queue, target scans, sound pickers, equipment display, wield change; `agent_ai.cpp`: nav-mesh path search (find_path, reachable), goal and path planning, ammo top-up, weapon mode, attack start, update_ai, slot_class_impl (static, EAX), ai_update_control, cycle_slot, spawn_missile, spawn_prop, choose_action, ai_idle_tick, shoot; `mesh_edit_members.cpp`: rgl::mesh members: add_quad, append_skinned, scale, translate, transform; `agent_ragdoll_blood.cpp`: Font::draw_glyph, ranged-weapon mesh, placed-object append, Newton world creation, start_ragdoll, dust, blood, mesh normals, text layout, carried-item refresh, drop_held; `agent_damage.cpp`: message log add, label box, shield_hit, hit_damage, on_removed, ragdoll_update, reduce_blow, draw_name; `agent_corpse_and_controls.cpp`: spawn_corpse, tick_death, note_blow, update_controls; `agent_blows.cpp`: process_blow, swing_trace; `agent_frame_update.cpp`: update_attack, move_step, update, tick

### 3.2 `src/game/script/script_interpreter.cpp` (7,044 lines, 0x49b1c0..0x4b1160) -> 3 files

Map: TU 92 `camera` 0x49b1c0..0x4b14b0 (high confidence at both ends: forward calls 0x49b170->0x49b1c0 and 0x49fa10->0x4b14b0; 20 possible inner splits unexplored), so this is **one original object** and every cut is a class change. `Operation::execute` (0x49fa10..0x4b0940, about 4,100 lines, a single `switch` with 591 case bodies) cannot be split without rewriting it, and the file that holds it must also hold the 14 validators (`check_*`/`warn_*`, register arguments ECX/EAX/EDI/EDX/ESI, called only from execute) and the Camera object (original_tus.md A2: exact only with the real body visible). The plan therefore cuts off only the two ends. Result: `operation_execute.cpp` stays about 6,200 lines. This is the one file that stays over 3k, and only because execute is one function. Alternative B (riskier, not encoded in the TSV): cut `operation_execute.cpp` further at 0x49d500 (operand accessors, DialogBlock::step_less / renumber / link, about 280 lines), 0x49deb0 (Camera, about 220), 0x49e780 (party attach / spawn / deploy / trigger helpers, about 490); that leaves the validators plus execute at about 5,000 lines and exposes Camera and the helpers to the visible-body problem.

| # | new file | from | to | lines (with prelude) | first function of the part | address range | why the cut is here |
|---|---|---:|---:|---:|---|---|---|
| 1 | `game/script/script_support.cpp` | 163 | 739 | 739 | `0x49b1c0` template void std::vector<NodeA*>::_Insert_n(std::vector<NodeA*>::iterator, size | 0x49b1c0..0x49cc60 | TU 92 "camera" of the map (0x49b1c0..0x4b14b0, high at both ends by forward calls, 20 possible inner splits) is ONE object, so this is a class-change cut: GameData / Mission helper members before the validators (the first 0x49b1c0 vector<NodeA*>::_Insert_n is config/widget_node.cpp's); no static or register-convention function crosses |
| 2 | `game/script/operation_execute.cpp` | 740 | 6798 | 6221 | `0x49cc60` static __declspec(noinline) void report(const char* msg) | 0x49cc60..0x4b0940 | NOT cut further: the 14 validators take register arguments (ECX/EAX/EDI/EDX/ESI) and are called only by execute, so they must share its object; Camera is proven merged with the interpreter (original_tus.md A2: exact only with the real body visible); execute is one function and cannot be split |
| 3 | `game/script/dialog_block_runner.cpp` | 6799 | 7044 | 408 | `0x4b0940` char DialogBlock::run_line(__int64* locals, int* idx, int* script) | 0x4b0940..0x4b1160 | class change after execute (inside TU 92); the TU ends at 0x4b1160 (tableau callback 0x4b14b0 is called forward from execute: medium-high boundary) |

Contents: `script_support.cpp`: Mission::place, set_font_mesh, GameData gold / quest / label helpers, battle icon, order arrow, check_slot, Mission::order (the slices c49aca0..c49c2c0); `operation_execute.cpp`: validators (register-convention statics), operand accessors and try linker (Elem / DialogBlock), the view Camera, party attach / spawn / deploy helpers, trigger enter/exec, trigger_item_mesh, Operation::execute (4,100-line switch over 591 opcodes); `dialog_block_runner.cpp`: DialogBlock::run_line and DialogBlock::emit: the control-opcode block runner (opcodes below 30)

### 3.3 `src/game/mission5/mission_5.cpp` (3,224 lines, 0x525160..0x52cf60) -> 3 files

Map: TU 116 `mission_5` 0x5249f0..0x52cf60, T regime (forward calls are normal there, so only class changes are usable; 2 possible inner splits). The file begins at 0x525160 (the first 0x770 bytes of the TU stayed with game_2.cpp) and ends where the map-editor mesh object (TU 117, class change Elem|Mesh) begins at 0x52cf60.

| # | new file | from | to | lines (with prelude) | first function of the part | address range | why the cut is here |
|---|---|---:|---:|---:|---|---|---|
| 1 | `game/mission/mission_frame_update.cpp` | 114 | 1297 | 1297 | `0x525160` void Rec1208::frame_update(float code_f) | 0x525160..0x5284a0 | TU 116 "mission_5" of the map (0x5249f0..0x52cf60, T regime, so class changes only; 2 possible inner splits); this file starts at 0x525160 (class change after the record readers of game_2); cut at MissionBlocker::update_view: class change from frame tick / save to the view controller and HUD |
| 2 | `game/mission/mission_hud_and_orders.cpp` | 1298 | 2868 | 1684 | `0x5284a0` void MissionBlocker::update_view(float dt) | 0x5284a0..0x52bf40 | MissionBlocker / Mission view, HUD and order members; ends where the editor-side Terrain members start |
| 3 | `game/editor/terrain_brush.cpp` | 2869 | 3224 | 469 | `0x52bf40` void Terrain::update(TerrainDesc* desc, vector4* pos, int sel, int mode, int fla | 0x52bf40..0x52cf60 | class change MissionBlocker -> Terrain; the TU ends at 0x52cf60 where the map editor mesh object (mesh_3) starts |

Contents: `mission_frame_update.cpp`: game-loop event handler, dialogue / conversation update, speaker and listener finders, the mission screen frame tick (0x5261e0), scene .sco save path; `mission_hud_and_orders.cpp`: view controller, mission HUD markers, party heroes, agent bookkeeping, interaction probe, order markers and menu, order_agent, update_ui, cursor, find_position; `terrain_brush.cpp`: Terrain::update / clear_brushed / brush (the scene editor brush), TerrainDescEntry clear / put, Terrain::clamp_cell

### 3.4 `src/game/party2/party_2.cpp` (3,158 lines, 0x4b1160..0x4b8f40) -> 2 files

Map: TU 93 `party_2` 0x4b14b0..0x4b8f40 (high at the start: forward call 0x49fa10->0x4b14b0; low at the end; 10 possible inner splits). The file also holds 0x4b1160..0x4b14b0, which the map gives to TU 92 (kept here because the tableau callback is stored by address from the same code). Only one cut is proposed because the file is just over 3k and nothing inside it is evidenced: Party::think is the largest function and starts the per-frame driver half. **This is the weakest cut in the plan**; leaving party_2.cpp whole (3,158 lines) is a defensible alternative.

| # | new file | from | to | lines (with prelude) | first function of the part | address range | why the cut is here |
|---|---|---:|---:|---:|---|---|---|
| 1 | `game/party/party_stacks_and_orders.cpp` | 47 | 1742 | 1742 | `0x4b1160` void scene_step(skin* m) | 0x4b1160..0x4b4a00 | TU 93 "party_2" of the map (0x4b14b0..0x4b8f40, 81 functions, 10 possible inner splits; the file starts at 0x4b1160 because the tableau callback 0x4b1160..0x4b14b0 was kept with it, TU 92 per the map); cut at Party::think, where the AI/battle/nav driver begins (function boundary inside the Party class run; the file is 3,158 lines and has no recovered inner boundary) |
| 2 | `game/party/party_think_and_nav.cpp` | 1743 | 3158 | 1462 | `0x4b4a00` void Party::think() | 0x4b4a00..0x4b8f40 | per-frame driver of the map party and the world-map path finder; no TU evidence inside, cut is by function size and theme only (weakest cut of the four) |

Contents: `party_stacks_and_orders.cpp`: tableau render callback, colour_of, Party stack economy (value, xp, speed, skills, rotation), STL heap helpers, Event::bind_host, orders and planning, tick_stats, NavData::reset/can_move, collect_targets, plan_order, step_route, force_add_members, party_value; `party_think_and_nav.cpp`: Party::think, march_time, push_event / push_party_event, Event::detach / commit, leave_battle, NavData::find_path, Event nav animation, start_path, update_nav, update_flags, update_hud_text, Party::tick

Sidecars of the four split files are partitioned by the address column (third field) of each row; see the `sidecar-split` rows of the TSV. The only rows outside 0x401000..0x534300 are the std::logic_error family 0x40c2c0..0x40c670 (go with `agent_attack_checks.cpp`) and the two post-CRT initialiser rows `_$E1` 0x547d80 / `_$E2` 0x5484b0 of agent_logic (the g_phys initialiser and its atexit thunk; go with `agent_static_collision.cpp`, which defines `g_phys`).

Files that stay above ~3k after the plan: `operation_execute.cpp` (about 6,200, one 4,100-line function, see 3.2) and `agent_ai.cpp` (about 3,180, TU 49 kept whole; a cut at `Agent::cycle_slot` 0x44bc30 would give 2,360 + 830 but is not a map boundary).

## 4. Rules for everything that is not a .cpp

- **Sidecars** (`<stem>.manifest.txt`, `.nonmatching.txt`, `.partial.txt`; 349 files, all paired with a .cpp): move with their .cpp, same stem. Their rows are `../src/<old path>` strings in the first field; the move tool must rewrite that field (and `tools/` that read it). For split files the rows are partitioned by VA (above).
- **Headers** (333): rule per header in the TSV `note` column. Headers of split files are not split: `agent_logic.h`, `script_interpreter.h` keep their names; `mission_5.h` becomes `game/mission/mission_frame_update.h`; `party_2.h` becomes `game/party/party_map.h`.
- **Same-name headers**: 12 new paths would be claimed by two or more current headers (they are verified "views" of the same class, e.g. `include/item_kind.h` and `agent/item_kind.h`, `include/mat4.h` and `game/mat4.h`, three `frustum.h`). The TSV gives the first one the plain name and suffixes the others with their old directory (`..._<old_dir>.h`) so the table stays unambiguous; reconciling them is a later step (the per-dir MIGRATION.md notes call it open work). They are: `engine/fx/fx.h` <- `include/fx.h`, `rgl/scene/fx.h`; `engine/math/mat4.h` <- `game/mat4.h`, `include/mat4.h`; `engine/physics/rigid_body.h` <- `rgl/dynamic/dynamic_entity.h`, `rgl/dynamic/rigid_body.h`; `engine/render/mesh.h` <- `rgl/mesh.h`, `rgl/particles/mesh_obj.h`; `engine/render/renderer.h` <- `game/renderer.h`, `include/renderer.h`; `engine/scene/frustum.h` <- `include/frustum.h`, `rgl/scene/frustum.h`; `game/agent/ai/agent_ai.h` <- `game/agent2/agent_ai.h`, `game/agent_ai.h`; `game/data/head_rec.h` <- `game/head_rec.h`, `rgl/mesh2/head_rec.h`; `game/data/item_kind.h` <- `game/agent/item_kind.h`, `include/item_kind.h`; `game/data/item_ref.h` <- `game/item_ref.h`, `game/ui/item_ref.h`; `game/editor/terrain_page/terrain_page.h` <- `game/char_sheet/terrain_page.h`, `game/terrain_page.h`; `game/terrain/terrain.h` <- `game/terrain_a/terrain.h`, `game/terrain_b/terrain.h`.
- **Notes**: the 35 `MIGRATION.md` / `NOTES.md` / `PROGRESS.md` files document waves and boundary evidence of directories that stop existing; proposed to move to `docs/migration/<old dir with / as _>.md` (e.g. `docs/migration/game_agent4.md`). They are history, not design docs.
- **Not moved**: `src/**` is the only tree touched. `port/`, `patches/`, `tools/`, `docs/`, `match/`, `work/` stay; they reference `src/...` paths (196 files outside `src/` and `work/` match `src/(game|rgl|include|dxframework|tinyxml)`; `port/tu/game/...`, `port/link/*/owners.tsv`, `patches/**` and the `tools/` that read manifests are the main ones). The move tool needs a path-rewrite pass over those, driven by this TSV.

## 5. Hazards of the proposed splits (read before approving section 3)

1. **Register-convention statics and visible callees.** cl 13.10 gives a static function a register convention only because it sees every caller; the original has such statics in the interpreter (14 validators, `trigger_item_mesh` 0x4904a0) and in agent_logic (`slot_class_impl` 0x4350c0, caller in 5 places; `ui_weapon_class_name` 0x43a280, caller `process_blow` 0x457b40 in another part). Today agent_stats.cpp already carries a stand-in copy of `slot_class_impl`. After the split each part needs the helper either in a shared header as a `static` (one emitted copy per part, folded by /OPT:ICF) or must stay in the same file as its callers. Same for the inline vector helpers (`vsub`, `vneg`, `vdot`, `first_used`, `next_used`, `vnorm`) that the file defines once. The plan does not solve this; it only avoids it for the interpreter (validators stay with execute).
2. **Byte-matching can regress when a file is cut.** docs/original_tus.md records that "same-unit callee body" visibility changes rows (agent_logic: making 22 earlier functions opaque to the tail cost `Agent::update` its match; Camera + interpreter proved a merge). Suggested gate for the move tool: build each part, run `chk.py` on the part's rows, compare with the pre-split exact set, and keep the cut only if no exact row is lost (or the loss is explained). Do the four splits last, one per commit.
3. **Prelude duplication**: each of the 11 + 3 + 3 + 2 parts starts with the same 46..162 lines (includes, `#define`s, inline helpers); trimming is manual.

## 6. Open questions for the user

1. Accept `operation_execute.cpp` at ~6,200 lines (execute is one function and its register-convention validators must share its object), or take alternative B in 3.2?
2. `agent_ai.cpp` ~3,180 lines: keep TU 49 whole (proposed) or cut at `cycle_slot` 0x44bc30 (not a map boundary)?
3. `party_2.cpp` (3,158): split at `Party::think` as proposed (no evidence, theme only) or leave whole? Same question for `mission_5.cpp`, whose cuts are class changes in a T-regime TU (weakest evidence class).
4. Grab-bag files named by dominant content, not by a single class: `game/app/winmain_and_menu_screens.cpp` (cls_map: WinMain, Font load, ~20 menu / options / key screens), `game/ui/screens/progress_bar_and_load_screens.cpp`, `game/campaign/game_data_party_quest.cpp` (world_data), `game/editor/editor_selection_and_translation.cpp` (gui_edit: PolyMesh selection plus translated-name helpers plus ItemKind::read), `game/app/options_dialog_and_module_tables.cpp` (trigger_vec), `game/data/module_option_records.cpp`, `game/campaign/game_state_load.cpp`, `game/terrain/wind_helpers.cpp`. Are these names acceptable, or do you want them split now at class changes (each has 10-40 class runs) before they get a final name?
5. `src/rgl/xmlcfg/` contains no XML code (records deleting destructors, key-code groups, game helpers, weighted item pick), so the directory dissolves into `game/data`, `game/input`, `game/common`, `game/agent`. The only XML users are TinyXML (`third_party`) and its callers `engine/resource/skeleton_load.cpp` and `game/data/ragdoll_models.cpp`; there is no "xml config" module to name. OK?
6. Engine versus game: I moved the engine-class code that sat under src/game into `src/engine/` (`renderer.cpp`, `entity.cpp`, `strat/*` strategic entity, `tstring.cpp`, `curve.cpp` env curves, `skeleton_load.cpp`, and `rgl::mesh` members cut out of agent_logic into `engine/render/mesh_edit_members.cpp`). The eight `mesh_slot` parts (TU 63) stay together in `game/mission/` though `mesh_vertex_ops.cpp` and `terrain_water_regen.cpp` are rgl-ish. `gui_manager.cpp` stays in `game/ui/` although it holds the engine reporting helpers (message boxes, log flush). Confirm the boundary or tell me which way to push these.
7. Depth: deepest path becomes `src/game/ui/screens/character_sheet/x.cpp` (4 levels under src, was 2). Acceptable, or flatten `ui/screens/<screen>/` to `ui/<screen>_*.cpp`?
8. Include style: one include root `src/` with module-prefixed includes (proposed) versus keeping relative includes and only rewriting them. The move tool needs to know before it runs.
9. `docs/migration/` for the 35 notes files, or keep them beside the code in the new directories (names would collide: several old directories merge into one)?
10. The 12 same-name header collisions in section 4: keep the suffix scheme for now and reconcile the views in a later step?
11. Tiny proven-TU files (32 under 100 lines, e.g. `rec_slots_hc.cpp` 14 lines, `ui_d_value.cpp` 28): stay separate (proposed). Should any group of consecutive proven-neighbour files be merged into one file with `#include` of the parts instead?
12. `game/agent/hud/agent_hud.cpp` is mostly mission HUD (Mission / MissionBlocker / Agent); the user taxonomy lists agent/hud, so it sits there, but `game/mission/` or `game/ui/hud/` fit as well.

## 7. Before / after

| | before | after |
|---|---:|---:|
| .cpp files | 185 | 200 |
| headers | 333 | 333 (same files, new places; 12 name-suffixed) |
| directories that hold source | 35 | 41 (incl. `third_party/tinyxml`) |
| directory depth under `src/` (levels, deepest .cpp) | 2 | 4 |
| largest .cpp (lines) | 11,720 (`agent4/agent_logic.cpp`) | about 6,220 (`script/operation_execute.cpp`, one-function exception); next 3,180 (`agent/ai/agent_ai.cpp`), then 2,296 |
| files >= 3,000 lines | 4 | 2 |
| files 1,000-2,999 | 23 | 33 |
| files 300-999 | 57 | 63 |
| files 100-299 | 69 | 70 |
| files < 100 | 32 | 32 |
| split files | - | 4 files -> 19 parts (11 + 3 + 3 + 2) |
| total .cpp lines | 105,096 | about 106,790 (+1,700: the preludes copied into the 19 parts) |

(Sizes "after" include each part's copy of the prelude. Line counts are `wc -l` of the .cpp files at the commit that adds this document.)

## 8. Decisions (the user, 2026-11-22: "go with your recommendations")

1. Accept `script/operation_execute.cpp` at about 6,200 lines: `execute` is one function and cannot be split across files.
2. Keep `agent_ai.cpp` (TU 49) whole: no recovered seam to cut at.
3. `mission_5.cpp`: split as proposed (its cuts are class changes). `party_2.cpp`: keep whole (its cut has no evidence).
4. Grab-bag names as proposed.
5. Dissolve `rgl/xmlcfg` into game/data, game/input, game/common and game/agent as proposed.
6. Engine-class code under src/game moves to src/engine as proposed.
7. Depth 4 is fine.
8. One include root `src/`, module-prefixed includes (`"engine/math/mat4.h"`).
9. The notes files move to `docs/migration/`.
10. The 12 same-name headers keep the directory suffix now; merging them is a later step.
11. The 32 files under 100 lines stay separate (proven original TUs).
12. `agent_hud.cpp` goes to `game/ui/hud/` (it is mostly mission HUD).

## 9. Outcome (tools/migrate/restructure.py)

Carried out by `tools/migrate/restructure.py` (batch A: every 1:1 move, `git mv`, plus the rewrite of every path reference; batch B: the splits,
`restructure_split.py` + `cxxchunk.py`). Deviations from the plan above, all decided while doing it:

- TinyXML is `src/third_party/tinyxml/`, not a top-level `third_party/`: it stays under the one include root and under every tool that walks `src/`.
- `party_2.cpp` stays whole as `src/game/party/party_map.cpp` (section 8, item 3); `agent_hud.cpp` + `.h` are in `src/game/ui/hud/`.
- `symbols/slices/<module>.txt` follows a renamed module (split by address for the split files).
- The include root is `src/` (`-I src`, also put on the command line by `tools/matching/cl.sh` so the laptop compile server sees it).
- `script_interpreter.cpp`: `g_script_error` was a file static of the one object that `execute` and `DialogBlock::run_line` shared; it stays static in
  `operation_execute.cpp` and `dialog_block_runner.cpp` reaches it through `script_error_get()` / `script_error_set()` (two new functions).
- Splits (batch B): agent_logic.cpp -> 11 parts, script_interpreter.cpp -> 3, mission_5.cpp -> 3 (cuts and evidence in section 3; `agent_ai.cpp` stays
  whole). Each part carries the old file's `#include`s and the `static` / `inline` helpers, types and macros its code mentions
  (`cxxchunk.py` finds them; nothing that emits code is copied), so the parts are a little longer than their cut.
  `*_INSTANTIATED_HERE` defines go to the part that owns the code they make a header emit. Sidecar rows were partitioned by address
  (`tools/migrate/rows_after_split.py` moved the rows whose symbol another part emits and demoted the rows that stopped matching).
- Byte-exact rows after the split: 4889 -> 4880. Nine rows of agent_logic moved from a `.manifest.txt` to the part's `.nonmatching.txt` with the figure
  `chk.py` measures (aligned bytes): `Agent::pick_front` 85, `pick_duel` 164, `fire_check` 12, `pick_threat` 10, `pick_reachable_goal` 32,
  `Font::draw_glyph` 17, `Agent::refresh_carried` 6, `gui_manager::add` 523, `Agent::update_attack` 282. Every row of script_interpreter and mission_5
  survived (`Operation::execute` kept its bytes). `ui_weapon_class_name` (0x43a280, a register-convention static whose only caller is
  `process_blow`) and two implicit COMDAT rows are emitted by the part that uses them now, so their rows moved there.

## 10. Revision of the agent_logic split (2026-11-22, the follow-up to batch B)

The nine rows batch B demoted (pick_front, pick_duel, fire_check, pick_threat, pick_reachable_goal, Font::draw_glyph, refresh_carried,
gui_manager::add, update_attack) were the evidence that the cuts of section 3.1 sit inside one original object: `docs/original_tus.md` A1
(register-convention statics called from several parts; the 2026-11-12 test that making 22 earlier functions opaque to the tail costs
`Agent::update` its match) said so, and the rows confirm it. Bisecting the old file (`head -N` of the pre-split source, `chk.py` on
`Agent::pick_front`) showed pick_front matches exactly when the text of `vector4::normalize`'s visible body (3,387 lines further down, in the
part that became agent_ai.cpp) is in the object, although that body is defined AFTER pick_front; c2scope's `icount` showed the symptom
(g_mission and `this` become Globregs candidates). So cl takes facts about callee bodies from the whole object, in either direction, and no
part of it can be compiled without the others. docs/c2scope_evaluation.md, section "restructure rows", has the questions and answers.

Decision (the user's rule: a file is one or more whole original TUs): the eleven parts are one file again,
`src/game/agent/agent_logic.cpp` (0x43a280..0x460160 and the std::logic_error family; the pre-split text of 7d9c7934, 11,720 lines),
including the rgl::mesh members that had been moved to `src/engine/render/mesh_edit_members.cpp` (they sit inside the same object, the
first class run of TU 50 is not a TU boundary). All nine rows are byte-exact again (chk.py 9/9). Sections 3.1 and the agent_logic parts of
sections 2 and 9 are superseded; script_interpreter.cpp and mission_5.cpp keep the split of section 3 (see their own notes).

## 11. Batch C (2026-11-22): the sidecars leave src/

The 357 `*.manifest.txt`, `*.nonmatching.txt` and `*.partial.txt` files moved (`git mv`, contents unchanged) to the mirror tree
`match/manifests/<path under src/>/<stem>.<kind>.txt` (src/game/agent/ai/agent_ai.cpp ->
match/manifests/game/agent/ai/agent_ai.manifest.txt). A row's first field (`../src/<path>.cpp`) is relative to `match/` and did not change.
`tools/common/sidecars.py` is the one module that knows the layout (`manifest_for(source)`, `nonmatching_for`, `partial_for`, `all_manifests()`,
`all_nonmatching()`, `all_partials()`, `source_for(sidecar)`, `stem_of`, `kind_of`); the ~60 tools that globbed `src/**/*.manifest.txt`, took
`src[:-4] + ".manifest.txt"` or compared suffixes use it; `docs/equiv_verdicts.json` "file" fields and the docs follow. Sections 1 to 9 describe
the plan in terms of sidecars beside their .cpp; read "beside" as "under match/manifests/".
