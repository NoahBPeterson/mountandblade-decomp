// view_params.h: the viewport / light parameter block (constructor 0x46b940, src/game/terrain_a/terrain_a.cpp).
// One definition for terrain_a (misc.h: f0..f30 and the two matrices written by the constructor 0x46b940) and
// terrain_b (cell_grid.h: the five shadow-camera bound vectors read by ShadowMap::attach_camera 0x4711e0 and
// embedded in CellGrid at +0x22c.. / +0x27c / +0x2bc).
#ifndef GAME_VIEW_PARAMS_H
#define GAME_VIEW_PARAMS_H
#include "./rgl_base.h"
#include "./frustum.h"

struct ViewParams {
    float f0;                                   // +0x00 1000
    float fog_ref;                              // +0x04 -1   world +0xdc: MissionUi's 0x488cd0 (set_fog) and set_fade 0x4624c0 write it
    float fog_rate;                             // +0x08 -100 world +0xe0
    int fog_color;                              // +0x0c 0x384878  world +0xe4 (a packed colour: set_fog copies it as the bits of a float, set_fade as an int)
    char fog_dirty;                             // +0x10 world +0xe8 (set by every fog / fade write)
    float fog_blend;                            // +0x14 0.1  world +0xec (the smoothing of set_fog, the fade time of set_fade)
    float env_peak;                             // +0x18 -1   world +0xf0 (set_params 0x462540)
    float env_time;                             // +0x1c -100 world +0xf4
    int env_color;                              // +0x20 0x384878 world +0xf8
    float env_c;                                // +0x24 -1e7 world +0xfc (set_params copies the bits of a float)
    float env_smooth;                           // +0x28 0.1  world +0x100
    char b2c;                                   // +0x2c
    int f30;                                    // +0x30
    rgl::frustum fr;                            // +0x34
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
