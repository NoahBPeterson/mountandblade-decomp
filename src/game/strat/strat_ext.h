// strat_ext.h: classes that this directory USES but does not own (render layer, resources, collision shapes).
// Only the fields and methods the functions of strat_entity.cpp / gui_manager.cpp touch are declared; the
// owning modules (other TUs: mesh 0x412a90.., scene 0x428f30/0x429740, frustum 0x41e4f0/0x41ef60, resource
// manager 0x418a70.., shapes 0x407ff0..) declare the full classes. Layouts are the ones the byte-exact slice
// functions verified; each offset is cited by the function that reads/writes it in strat_entity.h.
// Global-namespace names (scene, mesh, ...) are kept: the symbols map vtables/mangled names by them.
#ifndef STRAT_EXT_H
#define STRAT_EXT_H
#include <stddef.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../mat4.h"
#include "../tstring.h"
#include "../../rgl/mesh.h"                    // rgl::res_t / mat_t / res_obj4 / elem30 / elem14 / mesh_base (one definition for the rgl.h universe)
#include "../../include/mesh_prims.h"
#include "../../include/frustum.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mat4;
using rgl::aabb;
using rgl::point20;
using rgl::plane;
using rgl::frustum;

#define OFFSET_CHECK3(T, m, off) typedef char T##_##m##_at3_##off[(offsetof(T, m) == (off)) ? 1 : -1]

struct StratEnt;
struct MeshEnt;
struct Ctx;

// the mesh, scene and resource classes are the shared ones (src/rgl/mesh.h, resource.h)
#include "../../include/vec.h"
// vec2 is an aggregate; strat_entity.cpp builds its uv offsets through a constructor call (a temporary, then a copy)
struct vec2c : vec2 { vec2c(float x_, float y_) { x = x_; y = y_; } };
#include "../../include/mesh_parts.h"        // rgl::vertex60, frame_t, obj_c0, slot, render_info, keyframes
using rgl::vertex60;
using rgl::frame_t;
using rgl::obj_c0;
using rgl::slot;
using rgl::render_info;
using rgl::face20;                           // 0x20, vector at +0x90 (src/include/mesh_prims.h)
using rgl::keyframes;
using rgl::mesh;
using rgl::scene;

#include "../../include/clock_table.h"   // g_ticks (the clock table)


struct Psys { };                                         // (update 0x414960 is rgl::psys::update below)
struct FxDef { void draw(scene* sc, const matrix* m, const float* p); };   // 0x428ff0 (provisional)

// ---- the collision capsule built on the stack by update_ground_cache (vtable 0x54a540, size 0x64): src/include/shape.h.
// Its base constructor is the out-of-line 0x407ff0 here (capsule(capsule::outline_init())).
#include "../../include/shape.h"

// ---- callees declared under the name their definition has -----------------------------------------------------------------
// The stand-in classes above keep the global names the manifest rows bind (StratEnt::render takes a global scene*); the definitions of
// the functions they call are in the rgl modules under rgl:: names (or another class of the module), so a call binds its symbol through
// these declarations and casts the object, which is the one the definition works on (the same object under the stand-in's name).
namespace rgl {                                                  // (rgl::scene and rgl::mesh are rgl/mesh.h's: constructor 0x429740, add_point 0x412f50, add_vertex 0x413c50, render 0x413750)
struct psys { void update(scene* sc); };                         // 0x414960 (rgl/psys.cpp)
struct world { void to_cell(const float* p, int* out); };        // 0x41ed00 (rgl/core/frustum_grid.cpp)
struct mesh_ext { void clear_geometry(); };                      // 0x428e80 (rgl/scene/fx_support.cpp)
struct entity { virtual void render(scene* sc, render_info* out); };   // 0x420310 (rgl/scene/entity_render): the base render of MeshEnt
}
#include "../../rgl/core/resmgr.h"                        // resmgr: the full layout of the resource manager 0x5d6fa0 (get_shadow_skin 0x41c420); resource.h's resource_manager is its lookup half

#endif // STRAT_EXT_H
