// cell_world.h: CellGrid::to_cell (0x41ed00) is rgl::world::to_cell (rgl/core/frustum_grid.cpp, a non-const member of the same object).
// The view forwards to it through a one-member stub of the class; the translation units that include rgl/scene/world.h (the
// full class) must not include this header.
#ifndef TERRAIN_B_CELL_WORLD_H
#define TERRAIN_B_CELL_WORLD_H
#include "./cell_grid.h"

namespace rgl { struct world { void to_cell(const float* p, int* out); }; }   // 0x41ed00
__forceinline void CellGrid::to_cell(const float* p, int* out) const { ((rgl::world*)this)->to_cell(p, out); }

#endif // TERRAIN_B_CELL_WORLD_H
