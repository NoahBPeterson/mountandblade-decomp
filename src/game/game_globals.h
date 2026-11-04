// game_globals.h: canonical declarations of the game-wide globals that several modules declared under different names (sorted by address).
#ifndef GAME_GAME_GLOBALS_H
#define GAME_GAME_GLOBALS_H

#include <windows.h>
#include <stdio.h>
#include "tstring.h"
#include "shared_vars.h"
#include "./ui_globals.h"   // screen size, ui-dirty flag, mission clock, dev mode (a leaf header the UI modules include without the rest)

struct MapIcon;
struct PresentationTable;
struct AnimDefV;
struct DialogMgr;   // dialog_manager.h

// The table of single-bit masks (8-byte entries 1<<i) starts here;
extern unsigned __int64 g_bit[]; // 0x5a8e68 (g_bit[i] == 1 << i)
// Experience needed per level (rdata, 64 entries; game_2.cpp defines it; entry n+1 is the next level's bound).
extern const int g_level_xp[64]; // 0x5937c8
// -1 = nothing pending: set to -1 by agent_hud (0x4f7f00 path) and world_app, tested in mission_5.cpp (0 = rebuild mission geometry, then reset to -1).
extern int g_pending_action; // 0x5a9f18 -1 = none; 0 = rebuild mission geometry
// Log file handle shared by cls_map.cpp (g_5a9f90) and char_sheet/editor_ui.h (g_log_file).
extern FILE* g_log_file; // 0x5a9f90
// Same flag, named g_5a9f94 (char) in cls_map.cpp.
extern unsigned char g_log_flag94; // 0x5a9f94
// Same flag, named g_5a9f95 (char) in cls_map.cpp.
extern unsigned char g_log_flag95; // 0x5a9f95
// game_variables key "map_max_x" (trigger_vec.cpp);
extern float g_map_max_x; // 0x5aa118
// game_variables key "map_max_y" (trigger_vec.cpp);
extern float g_map_max_y; // 0x5aa11c
// game_variables key "map_min_x" (trigger_vec.cpp);
extern float g_map_min_x; // 0x5aa120
// game_variables key "map_min_y" (trigger_vec.cpp);
extern float g_map_min_y; // 0x5aa124
// Same word (agent_logic.h g_input_flags / agent_logic.cpp g_flags_5e2470).
extern unsigned g_input_flags; // 0x5e2470
#include "./script/script_globals.h"   // g_cur_param, g_lineno, g_script_error, g_cur_opcode, g_or_latch (0x5e2590..0x5e25a4)
#include "module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
// Event of the loader thread (cls_map.cpp init sets g_mt_inited then InitializeCriticalSection(&g_mt_cs));
extern HANDLE g_mt_event; // 0x5e28d8
// Critical section guarded by g_mt_inited (mission_5.cpp Enter/LeaveCriticalSection(&g_cs)).
extern CRITICAL_SECTION g_mt_cs; // 0x5e28dc
// game_variables key display_wp_one_handed ...
// Consumers: mesh_pass.cpp (rebuild before the next pass, then cleared), facegen_misc (test and clear).
extern char g_scene_dirty; // 0x5e2938 rebuild latch
// Script op check_menu (0x49ce90) bounds-checks against it.
extern int g_menu_count; // 0x5e29b4
// game_variables key compatible_module_version (trigger_vec.cpp).
extern int g_compatible_module_version; // 0x5e29b8
// game_variables key module_version.
extern int g_module_version; // 0x5e29c0
// 6 declarations; the "show download button" reading is the same trial-version flag.
#include "./trial_mode.h"   // g_trial_mode (0x5e29c4)
// Count of g_propkinds (prop_kind.h header comment: count 0x5e29e0).
extern int g_propkind_count; // 0x5e29e0
// module_files.cpp loader (count g_map_icon_count at 0x5e29f4);
#include "./map_icon.h"      // g_map_icons (0x5e29f0)
// Script op validation (0x49d0c0).
extern int g_party_template_count; // 0x5e29fc
// Set to 1 by cls_map.cpp right after InitializeCriticalSection(&g_mt_cs);
extern unsigned char g_mt_inited; // 0x5e2a09
// Accumulates dt while the flag at 0x5e2a09 is set (mesh_game.cpp) and is applied as one physics step later (gui_edit.cpp g_load_dt).
extern float g_paused_time; // 0x5e2a0c
// "the skin name; buf at 0x5fe88c".
// Same int (mission_ctx.cpp g_a170c0).
// game_variables.txt key "scan_module_sounds" (trigger_vec.cpp loader);
// game_variables.txt key "time_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "seeing_range" (trigger_vec.cpp loader);
// game_variables.txt key "track_spotting_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "blood_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "skill_leadership_bonus" (trigger_vec.cpp loader);
// game_variables.txt key "skill_prisoner_management_bonus" (trigger_vec.cpp loader);
// game_variables.txt key "base_companion_limit" (trigger_vec.cpp loader);
// game_variables.txt key "player_xp_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "hero_xp_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "regulars_xp_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "hero_wounded_treshold" (trigger_vec.cpp loader);
// game_variables.txt key "player_wounded_treshold" (trigger_vec.cpp loader);
// game_variables.txt key "num_hints" (trigger_vec.cpp loader);
// game_variables.txt key "map_sea_direction" (trigger_vec.cpp loader);
// game_variables.txt key "map_sea_speed_x" (trigger_vec.cpp loader);
// game_variables.txt key "map_sea_speed_y" (trigger_vec.cpp loader);
// game_variables.txt key "map_river_direction" (trigger_vec.cpp loader);
// game_variables.txt key "map_river_speed_x" (trigger_vec.cpp loader);
// game_variables.txt key "map_river_speed_y" (trigger_vec.cpp loader);
// game_variables keys map_tree_types / map_snow / map_steppe / map_desert_tree_types are four consecutive ints (0xa1815c..0xa18168).
// game_variables.txt key "damage_interrupt_attack_treshold" (trigger_vec.cpp loader);
// game_variables keys armor_soak_factor_against_cut/pierce/blunt are consecutive floats (the array g_module_settings.armor_soak_factor[3]).
// armor_reduction_factor_against_cut/pierce/blunt consecutive floats (the array g_module_settings.armor_reduction_factor[3]).
// game_variables.txt key "missile_damage_speed_power" (trigger_vec.cpp loader);
// game_variables.txt key "melee_damage_speed_power" (trigger_vec.cpp loader);
// game_variables.txt key "horse_charge_damage_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "couched_lance_damage_multiplier" (trigger_vec.cpp loader);
// game_variables.txt key "enable_quick_battles" (trigger_vec.cpp loader);
// game_variables.txt key "disable_food_slot" (trigger_vec.cpp loader);
// game_variables.txt key "limit_hair_colors" (trigger_vec.cpp loader);
// game_variables.txt key "show_faction_color" (trigger_vec.cpp loader);
// game_variables.txt key "show_quest_notes" (trigger_vec.cpp loader);
// game_variables.txt key "show_party_ids_instead_of_names" (trigger_vec.cpp loader);
// game_variables.txt key "main_menu_font_color_red" (trigger_vec.cpp loader);
// game_variables.txt key "main_menu_font_color_green" (trigger_vec.cpp loader);
// game_variables.txt key "main_menu_font_color_blue" (trigger_vec.cpp loader);
// Number of script emitters (the array is g_scripts, emitter_rec.h); camera.cpp defines it, game_2.cpp walks the emitters.
extern int g_emitter_count; // 0xa75eb4
// Page grid of the map (game_2.h);
extern char g_map_pages[]; // 0xa73094 page grid of the map
// Column count of g_map_pages.
extern int g_page_cols; // 0xa73294
// Row count of g_map_pages.
extern int g_page_rows; // 0xa73298
// Script op music_set_situation 0x4a174a writes it;
extern int g_music_situation; // 0xa73a10
// Number of scenes opened so far: each mission stamps its id at +0x5270 (hud.cpp, mission_ctx.cpp).
extern int g_scene_counter; // 0xa735c0
// module_files.cpp loads presentations into it (0x4c6950);
extern PresentationTable g_presentations; // 0xa75f2c
// 0x30-byte animation definitions indexed by animation id (agent_logic.cpp g_anims[0x1a3]).
extern AnimDefV* g_anims; // 0xa76628
// Count belonging to the skin table (script troop_set_type validates against it);
// DialogMgr (dialog_manager.h, +0 count, +4 recs ...);
extern DialogMgr g_dialogs; // 0x8ed50c8
// The empty string copied by the conversation refresh (conv_refresh.cpp);

// The module directory string;
extern tstring g_module_path; // 0x8ee3c28
// trigger_vec.cpp load_game_variables: the selected module name;
#include "./module_name.h"      // g_module_name (0x8ee3c38)


#endif // GAME_GAME_GLOBALS_H
