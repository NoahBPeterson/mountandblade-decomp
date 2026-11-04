// game_state.h: classes of game_state.cpp (0x4636d0..0x464200): the five-array holder, a colour ramp, the game
// state's path helpers and the shadow-map manager. Names provisional; each offset cites the function that reads or
// writes it.
#ifndef TERRAIN_A_GAME_STATE_H
#define TERRAIN_A_GAME_STATE_H
#include <windows.h>
#include <d3d9.h>
#include "./common.h"
#include "../tstring.h"
#include "../../rgl/scene/frustum.h"

// the terrain_a objects only call the dtor of ArrSet (its out-of-line copy 0x463760 is defined in game_state.cpp)
#define ARRSET_DTOR_OUT_OF_LINE
#include "../arr_set.h"

// ---- colour ramp: n stops (3 floats each, stride 0x10) from +0x2c, count at +0xac -----------------------------------
#include "../ramp.h"                      // Ramp (the 0xb0-byte colour-ramp / material record; shared with mesh_slot)
extern float g_ramp_gamma;                      // 0x5a9bfc (0x463790)

#include "../../include/vec.h"
struct Block16 {                                // 16-byte default block (0, 0.5, 0, 0)  0x4638b0
    Vec2 p, q;
    Block16();                                  // 0x4638b0
};

// ---- game state (global object 0x5e2930 = GameData, src/include/game_data.h: its path helpers agent_low 0x4638f0, scene_path 0x463950,
// module_path 0x4639d0 are defined in game_state.cpp) ----
#include "../../include/game_data.h"
#include "../game_globals.h"      // g_module_settings.show_quest_notes, g_module_settings.player_wounded_treshold, g_module_settings.disable_food_slot, g_display_wp_one_handed (canonical, defined in triggers/trigger_vec.cpp)
extern char g_path_buf[];                       // 0x5e2480 (0x4639d0)


rgl::vector4 __stdcall wind_noise();            // 0x463a80 (hidden return pointer, ret 4)

// ---- shadow map: render targets, the light camera, the frustum fit, the system-memory read-back ------------------------
struct LightCam {                               // the light camera object (ShadowMap::cam)
    char pad0[0x104];
    char dirty;                                 // +0x104 (0x464070)
    char pad105[3];
    void* target;                               // +0x108 render target of the light camera (0x464070)
    rgl::frustum fr;                            // +0x10c (0x463e90: world -> frustum frame)
    rgl::vector4 vmin;                          // +0x22c (0x463c20, 0x463cc0, 0x463e90)
    rgl::vector4 vmax;                          // +0x23c
    rgl::vector4 size;                          // +0x24c
    rgl::vector4 center;                        // +0x25c
    rgl::vector4 wcenter;                       // +0x26c
    rgl::matrix world;                          // +0x27c
    rgl::matrix light;                          // +0x2bc
};
struct ShTexRes {                               // object behind a render target's reference slot
    char pad0[0x24];
    int f24;                                    // +0x24 (0x463e10: 0x300000)
    char pad28[0x40 - 0x28];
    IDirect3DTexture9** tex;                    // +0x40 (0x4640d0)
};
struct ShSkin {                                 // render target resource returned by get_shadow_skin (0x41c420)
    char pad0[0x1c];
    unsigned id;                                // +0x1c (0x463e10: 0x108)
    tstring file;                               // +0x20 (0x463e10)
    int shader;                                 // +0x30 (0x463e10)
    char pad34[0x48 - 0x34];
    ShTexRes* refs[5];                          // +0x48 (0x463e10, 0x464070, 0x4640d0)
};



#include "../../include/renderer.h"   // canonical: g_render.dev (0xa17f28)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../rgl/options.h"   // canonical: g_options.i7c (0x5aa084)
#include "../../rgl/resource.h"   // g_resmgr (0x5d6fa0)

#include "../shadow_map.h"                    // ShadowMap (shared with terrain_b)
#include "../../include/render_flags.h"

#endif // TERRAIN_A_GAME_STATE_H
