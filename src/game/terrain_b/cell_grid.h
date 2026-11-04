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
#include "../../rgl/scene/world.h"            // rgl::world: the scene container with all its members
#include "../../include/view_params.h"       // ViewParams (shared with terrain_a); the five vectors after the frustum are the shadow camera bounds

#endif // TERRAIN_B_CELL_GRID_H
