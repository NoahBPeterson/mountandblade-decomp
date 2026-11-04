// cell_world.h: rgl::world's members that other modules define under the name of their own view of the scene object
// (floor_at / light_at / nearest_face... are agent2's `level`, reg_shape / add_tree are rgl/dynamic's world_grid, collect_point
// is game/cell.h's cell). The views are declared in cell_grid.h; each forwarder below is the call the original makes.
#ifndef TERRAIN_B_CELL_WORLD_H
#define TERRAIN_B_CELL_WORLD_H
#include "./cell_grid.h"
#include "../../rgl/scene/world.h"

__forceinline void rgl::world::floor_at(const float* p, float* out_z, face** out_face) { ((level*)this)->floor_at(p, out_z, out_face); }
__forceinline void rgl::world::light_at(rgl::vector4 p, rgl::vector4 n, rgl::vector4* out_dir, rgl::vector4* out_col, unsigned flags, float scale)
{ ((level*)this)->light_at(p, n, out_dir, out_col, flags, scale); }
__forceinline void rgl::world::reg_shape(void* o) { ((world_grid*)this)->reg_shape((grid_shape*)o); }
__forceinline void rgl::world::post(void* o) { ((world_grid*)this)->add_tree((grid_shape*)o); }
__forceinline void rgl::cell::collect_point(const float* p, int* use3d, unsigned mask, face** out, int* count)
{ ((::cell*)this)->collect_point(p, use3d, mask, (shape_base**)out, count); }

#endif
