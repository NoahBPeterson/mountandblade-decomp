// scene_paths_and_wind_noise.h: classes of scene_paths_and_wind_noise.cpp (0x4636d0..0x464200): the five-array holder, the face texture's hair colour, the game
// state's path helpers and the shadow-map manager. Names provisional; each offset cites the function that reads or
// writes it.
#ifndef TERRAIN_A_GAME_STATE_H
#define TERRAIN_A_GAME_STATE_H
#include <windows.h>
#include <d3d9.h>
#include "game/terrain/common.h"
#include "engine/core/tstring.h"
#include "engine/scene/frustum_rgl_scene.h"

// the terrain_a objects only call the dtor of ArrSet (its out-of-line copy 0x463760 is defined in scene_paths_and_wind_noise.cpp)
#define ARRSET_DTOR_OUT_OF_LINE
#include "engine/core/arr_set.h"

// ---- a skin's face texture: FaceTexture::hair_color 0x463790 samples its hair colour ramp -----------------------------
#include "engine/core/vec_elems.h"      // FaceTexture (Skin::face_textures; shared with mesh_slot)
extern float g_ramp_gamma;                      // 0x5a9bfc (0x463790)

#include "engine/math/vec.h"
#include "engine/core/block16.h"    // Block16: the 16-byte default block (0, 0.5, 0, 0)  0x4638b0

// ---- game state (global object 0x5e2930 = GameData, src/game/data/game_data.h: its path helpers agent_low 0x4638f0, scene_path 0x463950,
// module_path 0x4639d0 are defined in scene_paths_and_wind_noise.cpp) ----
#include "game/data/game_data.h"
#include "game/common/game_globals.h"      // g_module_settings.show_quest_notes, g_module_settings.player_wounded_treshold, g_module_settings.disable_food_slot, g_display_wp_one_handed (canonical, defined in game/app/options_dialog_and_module_tables.cpp)
extern char g_path_buf[];                       // 0x5e2480 (0x4639d0)


rgl::vector4 __stdcall wind_noise();            // 0x463a80 (hidden return pointer, ret 4)

// ---- shadow map: render targets, the light camera, the frustum fit, the system-memory read-back ------------------------
// The shadow map's targets are engine/resource/resource.h's skin (get_shadow_skin 0x41c420: shader_name +0x20, shader +0x30, flags
// +0x1c, tex[0] its render texture) and texture (get_render_target 0x41b7d0 / get_texture 0x41c400).



#include "engine/render/renderer_include.h"   // canonical: g_render.dev (0xa17f28)
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/config/options.h"   // canonical: g_options.i7c (0x5aa084)
#include "engine/resource/resource.h"   // g_resmgr (0x5d6fa0)

#include "engine/render/shadow_map.h"                    // ShadowMap (shared with terrain_b)
#include "engine/render/render_flags.h"

#endif // TERRAIN_A_GAME_STATE_H
