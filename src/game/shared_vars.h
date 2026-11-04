// shared_vars.h: canonical declarations of the HUD / game-variable / message-log globals that several modules declared under
// different names (one storage per address; the standalone data lane, docs/standalone_data.md). Small on purpose (no windows.h):
// game_globals.h includes it, and modules whose own views of other addresses conflict with game_globals.h include only this.
#ifndef GAME_SHARED_VARS_H
#define GAME_SHARED_VARS_H
#include "tstring.h"

namespace rgl { struct mesh; }

extern char g_5e2630; // 0x5e2630
// 256-entry key table; two declarations (char[256], int[0x40]) of the same storage.
extern char g_key_states[256]; // 0x5e27c8
// Incremented in game_app / gui_manager.cpp bump_counter, cleared in world_app.
extern int g_counter; // 0x5e28cc
extern int g_display_wp[7]; // 0x5e2900 game_variables.txt display_wp_one_handed, _two_handed, _polearms, _archery, _crossbows, _throwing, _firearms (the weapon-proficiency rows the character sheet shows)
struct ID3DXEffect;
extern ID3DXEffect* g_effect; // 0x5e293c the D3DX effect (shader file) in use

extern int g_map_icon_count; // 0x5e29f4
// Count of g_mission_templates (script 0x49d070).
extern int g_mission_template_count; // 0x5e2a04
// Set by hud.cpp, cleared by world_app, test-and-set latch in mission_ctx.cpp.
extern char g_saved; // 0x5e2a08
#include "../include/script_state.h"   // TriggerResult, g_script.trigger_result (0xa70458)

#endif // GAME_SHARED_VARS_H
