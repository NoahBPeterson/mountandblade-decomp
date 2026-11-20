// shadow_map.h: the shadow map object (render targets, light camera, frustum fit, system-memory read-back).
// One definition for terrain_a (game_state.cpp 0x463b70..0x4641a0, terrain_a.cpp 0x468c20/0x468c80) and terrain_b
// (attach_camera 0x4711e0, release_all). `cam` is the scene (rgl::world): attach_camera stores it and fills its view block.
#ifndef GAME_SHADOW_MAP_H
#define GAME_SHADOW_MAP_H
#include <stddef.h>
#include "../include/layout32.h"
#include <windows.h>
#include <d3d9.h>
#include "../include/rgl_base.h"
#include "../include/frustum.h"
#include "../rgl/scene_rec.h"    // rgl::scene

struct skin;                                    // rgl/resource.h
struct texture;
namespace rgl { struct world; }

struct ShadowMap {
    skin* target;                               // +0 the shadow skin (0x463b70, create_target 0x463e10)
    int f4;                                     // +4
    texture* target2;                           // +8 the depth texture (0x463b70)
    IDirect3DSurface9* surf;                    // +0xc system-memory copy of the map (0x463b70, 0x4640d0)
    int w, h;                                   // +0x10 / +0x14 (0x463bc0)
    rgl::scene scn;                             // +0x18 the render scene of the light's view (ctor 0x429740, dtor 0x41f4e0; setup 0x463e90 sets its viewport block, w2/h2 are its f20/f24)
    rgl::world* cam;                            // +0x1f0 the scene whose sun casts (attach_camera stores it): its view block (ViewParams) holds the light
                                                //        frame, the caster bounds and the shadow camera
    ShadowMap();                                // the inlined constructor in 0x547de0: the scene, then the four dwords and cam cleared
    ~ShadowMap();                               // 0x548490: the scene's destructor
    void release_all();                         // 0x463b70
    void pick_size();                           // 0x463bc0
    rgl::vector4 to_unit(const rgl::vector4& p);    // 0x463c20
    void fit_bounds();                          // 0x463cc0
    void create_target(skin** slot);            // 0x463e10
    void setup();                               // 0x463e90
    void bind_target();                         // 0x464070
    void sync_readback();                       // 0x4640d0
    void read_pixel(int unused, int x, int y, unsigned* a, unsigned* b);   // 0x4641a0
    void extend(const rgl::vector4* p);         // 0x468c20 (grow the caster bounds by a world point; terrain_a.cpp)
    void attach_camera(rgl::world* c);            // 0x4711e0 (terrain_b.cpp)
    void build_hw();                            // 0x47fce0 (mesh_slot/mesh_char.cpp)
    void build_sw();                            // 0x47fdd0 (mesh_slot/mesh_char.cpp)
    void render();                              // 0x47fef0 (mesh_slot/mesh_char.cpp: the caster pass into the target)
    bool test(const rgl::vector4* p, float* out);   // 0x468c80 (depth test of a world point; terrain_a.cpp; src/game/terrain_b calls it Probe::test)
    bool sample(const rgl::vector4* p, float* out);    // 0x4faee0 (ret 8: nine depth taps around the point; src/game/agent5)
};

extern ShadowMap g_shadow_map; // 0xa70248
MB_LAYOUT32_ASSERT(shadow_map_layout_check, (offsetof(ShadowMap, scn) == 0x18 && offsetof(ShadowMap, cam) == 0x1f0 && sizeof(ShadowMap) == 0x1f4));
#endif // GAME_SHADOW_MAP_H
