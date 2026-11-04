// cell_grid.h: what the callers of the scene container (rgl::world, src/rgl/scene/world.h; Mission::scene, +0) see besides
// the class itself. The slices called the one 0x344-byte object CellGrid, NavStore, Scene, World, Ctx, SceneW, SunCtx,
// LightCtx and Level; it is rgl::world now (cell_world.h forwards the members that other modules define under the names of
// their own views of it). Offsets are the ones read or written by a function of terrain_b.cpp / mission.cpp (cited at each
// field in rgl/scene/world.h).
#ifndef TERRAIN_B_CELL_GRID_H
#define TERRAIN_B_CELL_GRID_H
#include <stddef.h>
#include <vector>
#include <list>
#include "../../rgl/rgl_math.h"
#include "./frustum.h"
#include "./poly_mesh.h"
#include "./nav_data.h"
#include "../../include/height_field.h"      // HCell, HeightField (cols 0x471b30, 0x4785e0; line_clear 0x436da0; sample 0x433510)
#include "../../include/view_params.h"       // ViewParams (shared with terrain_a); the five vectors after the frustum are the shadow camera bounds

struct face;
// Classes of other modules that own methods the scene's callers reach (the definitions carry these names; cell_world.h
// forwards to them, so the call sites keep their spelling): the grid cell (game/cell.h), the scene as agent2 sees it
// (agent2/agent_types.h) and the collision registry (rgl/dynamic/world_grid.h).
struct shape_base; struct grid_shape;
struct cell {
    void collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count);   // 0x439780
};
struct level {
    void floor_at(const float* p, float* out_z, face** out_face);           // 0x439be0 (ret 0xc)
    void light_at(rgl::vector4 p, rgl::vector4 n, rgl::vector4* out_dir, rgl::vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (ret 0x30)
    bool trace_ray(const float* from, const float* to, float maxlen, shape_base** hit, int a);   // 0x439d60 (ret 0x14)
    int nearest_face(const float* p);   // 0x439a50 agent2/agent_stats.cpp
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);   // 0x439870
};
struct world_grid {
    void reg_shape(grid_shape* o);                                          // 0x424170
    void add_tree(grid_shape* s);                                           // 0x425240
};

// level::trace_ray returns bool; the callers test the whole of eax, so the call is made through the int-returning member type
#define TRACE_RAY(grid, from, to, maxlen, a, hit) \
    (((level*)(grid))->*(int (level::*)(const float*, const float*, float, shape_base**, int))&level::trace_ray)((const float*)(from), (const float*)(to), (maxlen), (shape_base**)(hit), (a))

#endif // TERRAIN_B_CELL_GRID_H
