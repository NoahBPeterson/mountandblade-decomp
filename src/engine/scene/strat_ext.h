// strat_ext.h: classes that this directory USES but does not own (render layer, resources, collision shapes).
// Only the fields and methods the functions of strategic_entity.cpp / gui_manager.cpp touch are declared; the
// owning modules (other TUs: mesh 0x412a90.., scene 0x428f30/0x429740, frustum 0x41e4f0/0x41ef60, resource
// manager 0x418a70.., shapes 0x407ff0..) declare the full classes. Layouts are the ones the byte-exact slice
// functions verified; each offset is cited by the function that reads/writes it in strategic_entity.h.
// Global-namespace names (scene, mesh, ...) are kept: the symbols map vtables/mangled names by them.
#ifndef STRAT_EXT_H
#define STRAT_EXT_H
#include <stddef.h>
#include "engine/core/layout32.h"
#include <vector>
#include "engine/math/rgl_math.h"
#include "engine/math/mat4.h"
#include "engine/core/tstring.h"
#include "engine/render/mesh.h"                    // rgl::res_t / mat_t / res_obj4 / elem30 / elem14 / mesh_base (one definition for the rgl.h universe)
#include "engine/render/mesh_prims.h"
#include "engine/scene/frustum.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mat4;
using rgl::aabb;
using rgl::point20;
using rgl::plane;
using rgl::frustum;

#define OFFSET_CHECK3(T, m, off) MB_LAYOUT32_ASSERT(T##_##m##_at3_##off, (offsetof(T, m) == (off)))


// the mesh, scene and resource classes are the shared ones (src/engine/render/mesh.h, resource.h)
#include "engine/math/vec.h"
// vec2 is an aggregate; strategic_entity.cpp builds its uv offsets through a constructor call (a temporary, then a copy)
struct vec2c : vec2 { vec2c(float x_, float y_) { x = x_; y = y_; } };
#include "engine/render/mesh_parts.h"        // rgl::vertex60, frame_t, slot, render_info, keyframes
using rgl::vertex60;
using rgl::frame_t;
using rgl::slot;
using rgl::render_info;
using rgl::face20;                           // 0x20, vector at +0x90 (src/engine/render/mesh_prims.h)
using rgl::keyframes;
using rgl::mesh;
using rgl::scene;

#include "engine/core/clock_table.h"   // g_ticks (the clock table)


#include "engine/fx/fx.h"                          // FxDef (render 0x428ff0)

// ---- the collision capsule built on the stack by update_ground_cache (vtable 0x54a540, size 0x64): src/engine/physics/shape.h.
// Its base constructor is the out-of-line 0x407ff0 here (capsule(capsule::outline_init())).
#include "engine/physics/shape.h"

// ---- callees declared under the name their definition has -----------------------------------------------------------------
// The stand-in classes above keep the global names the manifest rows bind (StratEnt::render takes a global scene*); the definitions of
// the functions they call are in the rgl modules under rgl:: names (or another class of the module), so a call binds its symbol through
// these declarations and casts the object, which is the one the definition works on (the same object under the stand-in's name).
#include "engine/scene/scene_entity.h"            // rgl::entity / strategic_entity / psys (the entity classes the member functions work on)
#include "engine/scene/world.h"                   // rgl::world (to_cell 0x41ed00)
#include "engine/scene/scene.h"                   // rgl::mesh (clear_geometry 0x428e80)

#endif // STRAT_EXT_H
