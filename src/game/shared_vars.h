// shared_vars.h: canonical declarations of the HUD / game-variable / message-log globals that several modules declared under
// different names (one storage per address; the standalone data lane, docs/standalone_data.md). Small on purpose (no windows.h):
// game_globals.h includes it, and modules whose own views of other addresses conflict with game_globals.h include only this.
#ifndef GAME_SHARED_VARS_H
#define GAME_SHARED_VARS_H
#include "tstring.h"

extern int g_sound_paused; // 0x5a9fd4 (the script engine reads it as g_warnings_off_a)
extern void* g_overlay; // 0x5d75e8 (== the UI root's object at +0x568)
extern tstring g_strs[0x50]; // 0x5d7850 the HUD line cache (0x5d7850 .. 0x5d7d50)
extern int g_list0; // 0x5d8020
extern int g_list1; // 0x5d8024
extern float g_list2; // 0x5d8028
extern unsigned g_panel_color; // 0x5d8760 name colour of the HUD line (-1 = white)
extern unsigned g_panel_bg; // 0x5d8764 background tint of the HUD line
extern int g_sel_ent; // 0x5d8780
extern char g_5e2630; // 0x5e2630
// 256-entry key table; two declarations (char[256], int[0x40]) of the same storage.
extern char g_key_states[256]; // 0x5e27c8
// Incremented in game_app / gui_manager.cpp bump_counter, cleared in world_app.
extern int g_counter; // 0x5e28cc
extern int g_map_icon_count; // 0x5e29f4
// Count of g_mission_templates (script 0x49d070).
extern int g_mission_template_count; // 0x5e2a04
// Set by hud.cpp, cleared by world_app, test-and-set latch in mission_ctx.cpp.
extern char g_saved; // 0x5e2a08
extern int g_scan_module_textures; // 0xa18108
// game_variables.txt key "scan_module_sounds" (trigger_vec.cpp loader);
extern int g_scan_module_sounds; // 0xa1810c
// game_variables.txt key "time_multiplier" (trigger_vec.cpp loader);
extern float g_time_multiplier; // 0xa18110
// game_variables.txt key "track_spotting_multiplier" (trigger_vec.cpp loader);
extern float g_track_spotting; // 0xa18118
// game_variables.txt key "map_sea_speed_x" (trigger_vec.cpp loader);
extern float g_map_sea_speed_x; // 0xa18148
// game_variables.txt key "map_sea_speed_y" (trigger_vec.cpp loader);
extern float g_map_sea_speed_y; // 0xa1814c
// game_variables.txt key "map_river_speed_x" (trigger_vec.cpp loader);
extern float g_map_river_speed_x; // 0xa18154
// game_variables.txt key "map_river_speed_y" (trigger_vec.cpp loader);
extern float g_map_river_speed_y; // 0xa18158
// game_variables.txt key "show_faction_color" (trigger_vec.cpp loader);
extern int g_show_faction_color; // 0xa181b0
// game_variables.txt key "show_party_ids_instead_of_names" (trigger_vec.cpp loader);
extern unsigned char g_show_party_ids_instead_of_names; // 0xa181b8

#endif // GAME_SHARED_VARS_H
