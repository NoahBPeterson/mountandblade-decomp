// shadow_map.h: the shadow map object (render targets, light camera, frustum fit, system-memory read-back).
// One definition for terrain_a (game_state.cpp 0x463b70..0x4641a0, terrain_a.cpp 0x468c20/0x468c80) and terrain_b
// (attach_camera 0x4711e0, release_all). `cam` is a CellGrid (terrain_b/cell_grid.h; attach_camera stores it and fills its
// view block) that terrain_a reads as a LightCam (game_state.h; same object, offsets 0x104..0x2bc).
#ifndef GAME_SHADOW_MAP_H
#define GAME_SHADOW_MAP_H
#include <stddef.h>
#include <windows.h>
#include <d3d9.h>
#include "../include/rgl_base.h"
#include "../include/frustum.h"
#include "../rgl/scene_rec.h"    // rgl::scene

struct ShSkin;
namespace rgl { struct world; }
struct LightCam;

struct ShadowMap {
    ShSkin* target;                             // +0 (0x463b70)
    int f4;                                     // +4
    ShSkin* target2;                            // +8 (0x463b70)
    IDirect3DSurface9* surf;                    // +0xc system-memory copy of the map (0x463b70, 0x4640d0)
    int w, h;                                   // +0x10 / +0x14 (0x463bc0)
    rgl::scene scn;                             // +0x18 the render scene of the light's view (ctor 0x429740, dtor 0x41f4e0; setup 0x463e90 sets its viewport block, w2/h2 are its f20/f24)
    LightCam* cam;                              // +0x1f0 the light camera: terrain_b's CellGrid (attach_camera casts), seen as a LightCam in game_state.h
    ShadowMap();                                // the inlined constructor in 0x547de0: the scene, then the four dwords and cam cleared
    ~ShadowMap();                               // 0x548490: the scene's destructor
    void release_all();                         // 0x463b70
    void pick_size();                           // 0x463bc0
    rgl::vector4 to_unit(const rgl::vector4& p);    // 0x463c20
    void fit_bounds();                          // 0x463cc0
    void create_target(ShSkin** slot);          // 0x463e10
    void setup();                               // 0x463e90
    void bind_target();                         // 0x464070
    void sync_readback();                       // 0x4640d0
    void read_pixel(int unused, int x, int y, unsigned* a, unsigned* b);   // 0x4641a0
    void extend(const rgl::vector4* p);         // 0x468c20 (grow the caster bounds by a world point; terrain_a.cpp)
    void attach_camera(rgl::world* c);            // 0x4711e0 (terrain_b.cpp)
    bool test(unsigned p, float* out);          // 0x468c80 (depth test of a world point; terrain_a.cpp; src/game/terrain_b calls it Probe::test)
    bool sample(const rgl::vector4* p, float* out);    // 0x4faee0 (ret 8: nine depth taps around the point; src/game/agent5)
};

extern ShadowMap g_shadow_map; // 0xa70248
typedef char shadow_map_layout_check[(offsetof(ShadowMap, scn) == 0x18 && offsetof(ShadowMap, cam) == 0x1f0 && sizeof(ShadowMap) == 0x1f4) ? 1 : -1];
#endif // GAME_SHADOW_MAP_H
