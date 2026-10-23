// height_field.h: the height field of the mission scene (CellGrid::hf / level::hf, scene+0x64): columns of
// 16-byte cells. It is the object whose destructor 0x41ecc0 and allocator 0x462420 are rgl::world_aux
// (src/rgl/scene/world.h) / cell_rows (src/game/renderer.h) in those modules' views; those names stay there because
// their manifest rows bind them by mangled name.
//   HCell       : 16-byte cell. h/flags/id/id2 read by 0x471b30 (id at ints [cy*4 + 2]); the constructor is the
//                 one cell_rows::alloc 0x462420 runs inline for every `new grid_cell[n]` (flags 0, id -1, id2 -1;
//                 h left uninitialised).
//   HeightField : cols +0 (0x436da0, 0x471b30, 0x4785e0), x0/y0 +4/+8 world origin of the tile grid (0x436da0),
//                 w/h +0x14/+0x18 (highest column / row index: 0x462420, 0x4785e0), cell +0x1c cell size (0x436da0;
//                 terrain_b 0x4785e0 calls it scale). sample 0x433510 is defined by HeightFieldX
//                 (src/rgl/xmlcfg/game_util.h) under its own name.
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
    float x0, y0;                       // +4, +8 world origin of the tile grid (0x436da0)
    char padc[0x14 - 0xc];
    int w, h;                           // +0x14, +0x18
    float cell;                         // +0x1c
    bool line_clear(const rgl::vector4* a, const rgl::vector4* b);          // 0x436da0 (thiscall, ret 8; returns al)
    void sample(float x, float y, float* out_h, int* out_id);               // 0x433510 (thiscall, ret 8 x2)
};

#endif // MB_HEIGHT_FIELD_H
