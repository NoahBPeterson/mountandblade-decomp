// view_params.h: the viewport / light parameter block (constructor 0x46b940, src/game/terrain_a/terrain_a.cpp).
// One definition for terrain_a (misc.h: f0..f30 and the two matrices written by the constructor 0x46b940) and
// terrain_b (cell_grid.h: the five shadow-camera bound vectors read by ShadowMap::attach_camera 0x4711e0 and
// embedded in CellGrid at +0x22c.. / +0x27c / +0x2bc).
#ifndef GAME_VIEW_PARAMS_H
#define GAME_VIEW_PARAMS_H
#include "../include/rgl_base.h"
#include "../include/frustum.h"

struct ViewParams {
    float f0, f4, f8;                           // +0x00 1000, -1, -100
    int fc;                                     // +0x0c 0x384878
    char b10;                                   // +0x10
    float f14, f18, f1c;                        // +0x14 0.1, -1, -100
    int f20;                                    // +0x20 0x384878
    float f24;                                  // +0x24 -1e7
    float f28;                                  // +0x28 0.1
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
