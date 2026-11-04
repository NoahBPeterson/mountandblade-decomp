// height_field.h: the height field of the mission scene (CellGrid::hf / level::hf, scene+0x64): columns of
// 16-byte cells. The one class of the scene's cell table: destructor 0x41ecc0 (rgl/core/frustum_grid.cpp) and
// allocator 0x462420 (game/renderer.cpp).
//   HCell       : 16-byte cell. h/flags/id/id2 read by 0x471b30 (id at ints [cy*4 + 2]); the constructor is the
//                 HeightField::alloc 0x462420 runs inline for every `new grid_cell[n]` (flags 0, id -1, id2 -1;
//                 h left uninitialised).
//   HeightField : cols +0 (0x436da0, 0x471b30, 0x4785e0), bounds +4 (x/y: world origin of the tile grid (0x436da0),
//                 w/h +0x14/+0x18 (highest column / row index: 0x462420, 0x4785e0), cell +0x1c cell size (0x436da0;
//                 terrain_b 0x4785e0 calls it scale). sample 0x433510 is defined in rgl/xmlcfg/game_util.cpp
// Needs rgl::vector4 (rgl_base.h picks the module's own declaration).
#ifndef MB_HEIGHT_FIELD_H
#define MB_HEIGHT_FIELD_H
#include "./rgl_base.h"

struct HCell {
    float h; unsigned flags; int id; int id2;
    HCell() : flags(0), id(-1), id2(-1) {}
};
struct HeightField {
    HCell** cols;                       // +0
    rgl::vector4 bounds;                // +4 world origin of the tile grid (x, y: 0x436da0), set from the terrain (0x481570)
    int w, h;                           // +0x14, +0x18
    float cell;                         // +0x1c
    ~HeightField();                                                         // 0x41ecc0 (frees the column tables)
    void alloc(const int* dim);                                             // 0x462420 (dim[0] = highest column, dim[1] = highest row)
    bool line_clear(const rgl::vector4* a, const rgl::vector4* b);          // 0x436da0 (thiscall, ret 8; returns al)
    void sample(float x, float y, float* out_h, int* out_id);               // 0x433510 (thiscall, ret 8 x2)
};

#endif // MB_HEIGHT_FIELD_H
