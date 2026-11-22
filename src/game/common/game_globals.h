// game_globals.h: canonical declarations of the game-wide globals that several modules declared under different names (sorted by address).
#ifndef GAME_GAME_GLOBALS_H
#define GAME_GAME_GLOBALS_H

#include <windows.h>
#include <stdio.h>
#include "engine/core/tstring.h"
#include "game/common/shared_vars.h"
#include "game/ui/ui_globals.h"   // screen size, ui-dirty flag, mission clock, dev mode (a leaf header the UI modules include without the rest)

struct MapIcon;
struct DialogMgr;   // dialog_manager.h

// The table of single-bit masks (8-byte entries 1<<i) starts here;
extern unsigned __int64 g_bit[]; // 0x5a8e68 (g_bit[i] == 1 << i)
// Experience needed per level (rdata, 64 entries; map_builder_and_data_load.cpp defines it; entry n+1 is the next level's bound).
extern const int g_level_xp[64]; // 0x5937c8
// -1 = nothing pending: set to -1 by agent_hud (0x4f7f00 path) and world_app, tested in mission_5.cpp (0 = rebuild mission geometry, then reset to -1).
extern int g_pending_action; // 0x5a9f18 -1 = none; 0 = rebuild mission geometry
// game_variables key "map_max_x" (options_dialog_and_module_tables.cpp);
extern float g_map_max_x; // 0x5aa118
// game_variables key "map_max_y" (options_dialog_and_module_tables.cpp);
extern float g_map_max_y; // 0x5aa11c
// game_variables key "map_min_x" (options_dialog_and_module_tables.cpp);
extern float g_map_min_x; // 0x5aa120
// game_variables key "map_min_y" (options_dialog_and_module_tables.cpp);
extern float g_map_min_y; // 0x5aa124
// Same word (agent_logic.h g_input_flags / agent_logic.cpp g_flags_5e2470).
extern unsigned g_input_flags; // 0x5e2470
#include "game/script/script_globals.h"   // g_cur_param, g_lineno, g_script_error, g_cur_opcode, g_or_latch (0x5e2590..0x5e25a4)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
// Event of the loader thread (winmain_and_menu_screens.cpp init sets g_mt_inited then InitializeCriticalSection(&g_mt_cs));
extern HANDLE g_mt_event; // 0x5e28d8
// Critical section guarded by g_mt_inited (mission_5.cpp Enter/LeaveCriticalSection(&g_cs)).
extern CRITICAL_SECTION g_mt_cs; // 0x5e28dc
// game_variables key display_wp_one_handed ...
// Consumers: render_passes.cpp (rebuild before the next pass, then cleared), facegen_misc (test and clear).
extern char g_scene_dirty; // 0x5e2938 rebuild latch
// Script op check_menu (0x49ce90) bounds-checks against it.
extern int g_menu_count; // 0x5e29b4
// game_variables key compatible_module_version (options_dialog_and_module_tables.cpp).
extern int g_compatible_module_version; // 0x5e29b8
// game_variables key module_version.
extern int g_module_version; // 0x5e29c0
// 6 declarations; the "show download button" reading is the same trial-version flag.
#include "game/mission/trial_mode.h"   // g_trial_mode (0x5e29c4)
// Count of g_propkinds (game/mission/mission_obj.h).
extern int g_propkind_count; // 0x5e29e0
// module_files_loader.cpp loader (count g_map_icon_count at 0x5e29f4);
#include "game/campaign/map_icon.h"      // g_map_icons (0x5e29f0)
// Script op validation (0x49d0c0).
extern int g_party_template_count; // 0x5e29fc
// Set to 1 by winmain_and_menu_screens.cpp right after InitializeCriticalSection(&g_mt_cs);
extern unsigned char g_mt_inited; // 0x5e2a09
// Accumulates dt while the flag at 0x5e2a09 is set (agent_spawn_and_tick.cpp) and is applied as one physics step later (editor_selection_and_translation.cpp g_load_dt).
extern float g_paused_time; // 0x5e2a0c
// "the skin name; buf at 0x5fe88c".
// Same int (mission_context.cpp g_a170c0).
// game_variables.txt key "scan_module_sounds" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "time_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "seeing_range" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "track_spotting_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "blood_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "skill_leadership_bonus" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "skill_prisoner_management_bonus" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "base_companion_limit" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "player_xp_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "hero_xp_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "regulars_xp_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "hero_wounded_treshold" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "player_wounded_treshold" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "num_hints" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_sea_direction" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_sea_speed_x" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_sea_speed_y" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_river_direction" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_river_speed_x" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "map_river_speed_y" (options_dialog_and_module_tables.cpp loader);
// game_variables keys map_tree_types / map_snow / map_steppe / map_desert_tree_types are four consecutive ints (0xa1815c..0xa18168).
// game_variables.txt key "damage_interrupt_attack_treshold" (options_dialog_and_module_tables.cpp loader);
// game_variables keys armor_soak_factor_against_cut/pierce/blunt are consecutive floats (the array g_module_settings.armor_soak_factor[3]).
// armor_reduction_factor_against_cut/pierce/blunt consecutive floats (the array g_module_settings.armor_reduction_factor[3]).
// game_variables.txt key "missile_damage_speed_power" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "melee_damage_speed_power" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "horse_charge_damage_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "couched_lance_damage_multiplier" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "enable_quick_battles" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "disable_food_slot" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "limit_hair_colors" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "show_faction_color" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "show_quest_notes" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "show_party_ids_instead_of_names" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "main_menu_font_color_red" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "main_menu_font_color_green" (options_dialog_and_module_tables.cpp loader);
// game_variables.txt key "main_menu_font_color_blue" (options_dialog_and_module_tables.cpp loader);
// Script op music_set_situation 0x4a174a writes it;
// Number of scenes opened so far: each mission stamps its id at +0x5270 (mission_hud_buttons.cpp, mission_context.cpp).
// module_files_loader.cpp loads presentations into it (0x4c6950);
#include "game/data/prsnt_table.h"   // PresentationTable g_presentations (0xa75f2c)
#include "engine/fx/anim_set.h"   // AnimSet g_anim_set (0xa76628): the 0x30-byte animation definitions indexed by animation id
// Count belonging to the skin table (script troop_set_type validates against it);
// DialogMgr (dialog_manager.h, +0 count, +4 recs ...);
extern DialogMgr g_dialogs; // 0x8ed50c8
// The empty string copied by the conversation refresh (conversation_refresh.cpp);

// The module directory string;
extern tstring g_module_path; // 0x8ee3c28
// options_dialog_and_module_tables.cpp load_game_variables: the selected module name;
#include "game/data/module_name.h"      // g_module_name (0x8ee3c38)


#endif // GAME_GAME_GLOBALS_H
