// shadow_map.h: the shadow map object (render targets, light camera, frustum fit, system-memory read-back).
// One definition for terrain_a (game_state.cpp 0x463b70..0x4641a0, terrain_a.cpp 0x468c20/0x468c80) and terrain_b
// (attach_camera 0x4711e0, release_all). `cam` is a CellGrid (terrain_b/cell_grid.h; attach_camera stores it and fills its
// view block) that terrain_a reads as a LightCam (game_state.h; same object, offsets 0x104..0x2bc).
#ifndef GAME_SHADOW_MAP_H
#define GAME_SHADOW_MAP_H
#include <windows.h>
#include <d3d9.h>
#include "../include/rgl_base.h"
#include "../include/frustum.h"

struct ShSkin;
struct CellGrid;
struct LightCam;

struct ShadowMap {
    ShSkin* target;                             // +0 (0x463b70)
    char pad4[4];
    ShSkin* target2;                            // +8 (0x463b70)
    IDirect3DSurface9* surf;                    // +0xc system-memory copy of the map (0x463b70, 0x4640d0)
    int w, h;                                   // +0x10 / +0x14 (0x463bc0)
    int f18, f1c;                               // +0x18 / +0x1c (0x463e90)
    int w2, h2;                                 // +0x20 / +0x24
    int f28;                                    // +0x28
    float f2c;                                  // +0x2c
    char pad30[0x48 - 0x30];
    rgl::frustum fr;                            // +0x48 copy of the light camera frustum (0x463e90)
    char pad168[0x17a - 0x168];
    char valid;                                 // +0x17a
    char pad17b[0x1f0 - 0x17b];
    LightCam* cam;                              // +0x1f0 the light camera: terrain_b's CellGrid (attach_camera casts), seen as a LightCam in game_state.h
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
    void attach_camera(CellGrid* c);            // 0x4711e0 (terrain_b.cpp)
    bool test(unsigned p, float* out);          // 0x468c80 (depth test of a world point; terrain_a.cpp; src/game/terrain_b calls it Probe::test)
    bool sample(const rgl::vector4* p, float* out);    // 0x4faee0 (ret 8: nine depth taps around the point; src/game/agent5)
};

#endif // GAME_SHADOW_MAP_H
