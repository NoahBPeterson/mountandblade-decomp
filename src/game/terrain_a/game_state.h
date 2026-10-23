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
struct Ramp {
    char pad0[0x2c];
    struct Stop { float c[3]; float pad; } stops[8];    // +0x2c (0x463790)
    int n;                                      // +0xac (0x463790)
    unsigned eval(float* t);                    // 0x463790
};
extern int g_use_shaders;                       // 0x5e28fc (0: colours are gamma-corrected on output; 0x463790)
extern float g_ramp_gamma;                      // 0x5a9bfc (0x463790)

#include "../../include/vec.h"
struct Block16 {                                // 16-byte default block (0, 0.5, 0, 0)  0x4638b0
    Vec2 p, q;
    Block16();                                  // 0x4638b0
};

// ---- game state (global object 0x5e2930 seen through its path helpers) --------------------------------------------------
struct AgentLow {                               // 0x508-byte agent record: only +0xa0 is read (0x4638f0)
    char pad0[0xa0];
    float f_a0;                                 // +0xa0
    char pad_a4[0x508 - 0xa4];
};
struct SceneObjRec {                            // 0x494-byte record: name at +0x10 (0x463950)
    char pad0[0x10];
    tstring name;                               // +0x10
    char pad20[0x494 - 0x20];
};
extern float g_thr_self;                        // 0xa1813c (0x4638f0)
extern float g_thr_other;                       // 0xa18138 (0x4638f0)
extern char g_path_buf[];                       // 0x5e2480 (0x4639d0)

struct GameState {
    char pad0[0x1c];
    tstring module;                             // +0x1c (0x4639d0)
    char pad2c[0x8c - 0x2c];
    int cur_agent;                              // +0x8c (0x4638f0)
    char pad90[0x13a00 - 0x90];
    AgentLow* agents;                           // +0x13a00 (0x4638f0)
    char pad13a04[0x13a50 - 0x13a04];
    SceneObjRec* sceneobjs;                     // +0x13a50 (0x463950)
    bool agent_low(int idx);                    // 0x4638f0
    tstring scene_path(int idx);                // 0x463950
    const char* module_path(const char* sub);   // 0x4639d0
};

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
struct ShResMgr {                               // global resource manager at 0x5d6fa0 (calls only; layout in src/rgl/resource.h)
    ShSkin* get_shadow_skin(int w, int h, int fmt, int f34);   // 0x41c420
    int resolve_shader(tstring* name);          // 0x418e90
    void release(void* p);                      // 0x4175f0
};
extern ShResMgr g_shadow_resmgr;                // 0x5d6fa0
extern int g_shadow_quality;                    // 0x5aa000 (0x463bc0)
extern int g_max_tex_w;                         // 0xa17f84 (0x463bc0)
extern int g_max_tex_h;                         // 0xa17f88 (0x463bc0)
extern char g_use_hw_shadow;                    // 0xa17f24 (0x463e10, 0x464070, 0x4640d0)
extern IDirect3DDevice9* g_device;              // 0xa17f28 (0x4640d0, 0x4641a0)
extern int g_mesh_quality;                      // 0x5aa084 (cleared when the read-back texture cannot be created; 0x4640d0)

#include "../shadow_map.h"                    // ShadowMap (shared with terrain_b)

#endif // TERRAIN_A_GAME_STATE_H
