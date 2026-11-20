// world_grid.h: the world's collision grid (src/rgl/dynamic/world_grid.cpp, 0x423c00..0x425370): placing collision shapes
// into the cells of the grid, the cell's sorted shape vector, and the ground geometry (NavData) that the shapes of an
// entity are registered with. One header for the classes
// of this object.
//
// The classes are views of the shared ones; this header adds only members, never fields:
//   world_grid : rgl::world      the members of rgl::world that world.h does not declare (relocate, add_shape,
//                                add_tree, reg_shape); rgl::world::add_item (0x423c00) is declared in world.h
//   (strategic_entity::add_shape 0x424100, slot 28 of the strategic entity's table, is declared in scene_entity.h)
//   the ground geometry is NavData (terrain_b/nav_data.h: its vertex / cell vectors, NavData::add 0x424220)
// The shapes are src/rgl/shapes' shape_base / face / composite (the slices called them Obj, kind() = shape_type()); that
// header cannot be included next to resource.h (scene.h pulls it in, both define shape_base), so the grid sees them as
// views of resource.h's shape_base that add the vtable slots 5..13 and the two fields it reads.
// The slices' names: World (world_grid), Registry (world_grid::reg_shape), Scene (the strategic entity's add_shape), Cell (rgl::cell),
// Store (NavData), ObjVec / ObjList (rgl::cell's vector / list), Obj (shape_base).
#ifndef RGL_DYNAMIC_WORLD_GRID_H
#define RGL_DYNAMIC_WORLD_GRID_H
#include <vector>
#include <list>
#include <new>
#include <memory>
#include "../rgl_math.h"
#include "../resource.h"
#include "../scene/scene_math.h"
#include "../scene/frustum.h"
#include "../scene/scene.h"
#include "../scene/emitter.h"
#include "../scene/scene_entity.h"
#include "../scene/world.h"
#include "../../include/vec_xlen.h"

#include "../../game/terrain_b/nav_data.h"   // NavData (rgl::world::ground): the ground geometry

rgl::vector4 face_normal(const rgl::vector4* a, const rgl::vector4* b, const rgl::vector4* c);   // 0x424650

// ---- the world's members ------------------------------------------------------------------------------------------
int2* min2(int2* out, const int2* a, const int2* b);        // 0x423e70
int2* max2(int2* out, const int2* a, const int2* b);        // 0x423ea0

#endif
