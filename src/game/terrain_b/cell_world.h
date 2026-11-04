// cell_world.h: CellGrid's members that are rgl::world's (to_cell 0x41ed00, create_entity 0x428440, reset 0x421390: rgl/core/frustum_grid.cpp,
// rgl/scene/entity_types.cpp, rgl/scene/world.cpp) are forwarded to the full class (rgl/scene/world.h): CellGrid is the same 0x344-byte object.
#ifndef TERRAIN_B_CELL_WORLD_H
#define TERRAIN_B_CELL_WORLD_H
#include "./cell_grid.h"
#include "../../rgl/scene/world.h"

__forceinline void CellGrid::to_cell(const float* p, int* out) const { ((rgl::world*)this)->to_cell(p, out); }
__forceinline Entity* CellGrid::create_entity(int type_id) { return (Entity*)((rgl::world*)this)->create_entity(type_id); }
__forceinline void CellGrid::reset() { ((rgl::world*)this)->reset(); }

#endif
