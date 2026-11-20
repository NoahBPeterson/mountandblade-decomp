// view_params.h: the viewport / light parameter block (constructor 0x46b940, src/game/terrain_a/terrain_a.cpp).
// One definition for terrain_a (misc.h: f0..f30 and the two matrices written by the constructor 0x46b940) and
// terrain_b (cell_grid.h: the five shadow-camera bound vectors read by ShadowMap::attach_camera 0x4711e0 and
// embedded in CellGrid at +0x22c.. / +0x27c / +0x2bc).
#ifndef GAME_VIEW_PARAMS_H
#define GAME_VIEW_PARAMS_H
#include "./rgl_base.h"
#include "./frustum.h"
struct texture;                                 // rgl/resource.h

struct ViewParams {
    float f0;                                   // +0x00 1000
    // the fog of render_batch::render_scene 0x46e590 (fFogStart / fFogEnd / fFogDensity / vFogColor): this set above
    // water_level, the water_fog_* set when the camera is below it
    float fog_start;                            // +0x04 -1   world +0xdc: MissionUi's 0x488cd0 (set_fog) and set_fade 0x4624c0 write it
    float fog_end;                              // +0x08 -100 world +0xe0
    int fog_color;                              // +0x0c 0x384878  world +0xe4 (a packed colour: set_fog copies it as the bits of a float, set_fade as an int)
    char fog_on;                                // +0x10 world +0xe8 (set by every fog / fade write; render_scene fogs the meshes when set)
    float fog_density;                          // +0x14 0.1  world +0xec (set_fog: 3 / fog_end, clamped; the fade time of set_fade)
    float water_fog_start;                      // +0x18 -1   world +0xf0 (set_params 0x462540)
    float water_fog_end;                        // +0x1c -100 world +0xf4
    int water_fog_color;                        // +0x20 0x384878 world +0xf8
    float water_level;                          // +0x24 -1e7 world +0xfc (set_params copies the bits of a float)
    float water_fog_density;                    // +0x28 0.1  world +0x100
    char sun_shadows;                           // +0x2c render_scene: the sun's shadow map applies (0 after world::clear)
    texture* shadow;                            // +0x30 the sun's shadow map (render_scene: shadowmap_texture), 0 from the ctor
    rgl::frustum fr;                            // +0x34 the shadow camera (render_scene: matSunViewProj)
    rgl::vector4 vmin;                          // +0x154 (CellGrid +0x22c)
    rgl::vector4 vmax;                          // +0x164
    rgl::vector4 size;                          // +0x174
    rgl::vector4 center;                        // +0x184
    rgl::vector4 wcenter;                       // +0x194
    rgl::matrix world;                          // +0x1a4 (copy of identity by 0x46b940; CellGrid +0x27c)
    rgl::matrix light;                          // +0x1e4 (copy of identity; CellGrid +0x2bc)
    ViewParams();                               // 0x46b940
};

#endif // GAME_VIEW_PARAMS_H
