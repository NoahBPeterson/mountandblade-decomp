// cell_grid.h: what the callers of the scene container (rgl::world, src/engine/scene/world.h; Mission::scene, +0) see besides
// the class itself. The slices called the one 0x344-byte object CellGrid, NavStore, Scene, World, Ctx, SceneW, SunCtx,
// LightCtx and Level; it is rgl::world now (cell_world.h forwards the members that other modules define under the names of
// their own views of it). Offsets are the ones read or written by a function of terrain_scene_renderer.cpp / scene_container_grid.cpp (cited at each
// field in engine/scene/world.h).
#ifndef TERRAIN_B_CELL_GRID_H
#define TERRAIN_B_CELL_GRID_H
#include <stddef.h>
#include <vector>
#include <list>
#include "engine/math/rgl_math.h"
#include "game/terrain/frustum.h"
#include "game/terrain/poly_mesh.h"
#include "game/terrain/nav_data.h"
#include "engine/physics/height_field.h"      // HCell, HeightField (cols 0x471b30, 0x4785e0; line_clear 0x436da0; sample 0x433510)
#include "engine/scene/world.h"            // rgl::world: the scene container with all its members
#include "engine/scene/view_params.h"       // ViewParams (shared with terrain_a); the five vectors after the frustum are the shadow camera bounds

#endif // TERRAIN_B_CELL_GRID_H
