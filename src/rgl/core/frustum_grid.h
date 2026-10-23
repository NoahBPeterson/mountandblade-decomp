// frustum_grid.h: what src/rgl/core/frustum_grid.cpp (0x41e470..0x41ef1a) needs besides the scene headers
// (../scene/frustum.h: plane, frustum; ../scene/world.h: world, world_aux, cell, cellref): views onto fields those
// headers keep as padding, and one small record type. Names provisional.
#ifndef RGL_CORE_FRUSTUM_GRID_H
#define RGL_CORE_FRUSTUM_GRID_H
#include "../scene/world.h"

// the row table of world_aux (0x41ecc0): `rows` at +0, `last` (index of the last row) at +0x14
struct cell_rows_view {
    float** rows;        // +0
    int pad[4];
    int last;            // +0x14
};

// the grid geometry world::to_cell (0x41ed00) reads from the world object: the cell counts are rgl::world's
// nrows/ncols (+8, +0xc); the origin and the cell size are in its padding
struct grid_view {
    char pad0[8];
    int n[2];            // +8
    float lo[2];         // +0x10
    float pad18[2];
    float step[2];       // +0x20
};

// 0xd8-byte record owning three float arrays at +0x60; only its vector deleting destructor (0x41ed70) is in this
// range
struct buf3 {
    char pad[0x60];
    float* a;
    float* b;
    float* c;
    char pad2[0xd8 - 0x6c];
    ~buf3();                                              // 0x41ed70 (vector deleting)
};

#endif
