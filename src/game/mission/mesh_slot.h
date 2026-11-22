// mesh_slot.h: declarations shared by the modules of src/game/mesh_slot (TU 63 "mesh" 0x47aa40..0x4875e0 and TU 64
// "slot" 0x4875e0..0x488560 of work/migrate/tu_map.csv). Views (derived classes that only add member functions:
// a class cannot be extended from outside), globals and cdecl helpers; offsets cite the function that proves them.
#ifndef MESH_SLOT_H
#define MESH_SLOT_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <new>
#include "engine/math/rgl_math.h"
#include "engine/render/mesh.h"
#include "engine/render/mesh.h"
#include "engine/scene/world.h"
#include "engine/core/tstring.h"
#include "engine/core/arr_set.h"
#include "game/terrain/terrain.h"
#include "game/terrain/troop_mgr.h"
#include "game/mission/placed_obj.h"
#include "engine/core/timer.h"
#include "engine/physics/shape.h"
#include "game/data/item_kind_include.h"
#include "game/data/item_ref.h"
#include "game/mission/mission.h"
#include "engine/render/shadow_map.h"

#define NOINL __declspec(noinline)     // keeps the definition's name visible to tools/difftest/slice.py's marker scan
using rgl::vector4;
using rgl::matrix;

extern float g_ramp_gamma;                      // 0x5a9bfc gamma exponent of the software colour path (0x463790)
unsigned pack_signed_color(const float* c);     // 0x461370 (cdecl)

// ---- members of rgl::mesh defined in this TU (0x47aa40..0x47b3b0) ------------------------------------------------
// rgl::mesh (the members above) is src/engine/render/mesh.h
// (compute_normals 0x451c50 is rgl::mesh's own member, reset 0x428f30 rgl::scene's; clone 0x47a370 is rgl::lod_mesh's)
struct scene_ms2 : rgl::scene {
};

#include "engine/resource/resource.h"   // resmgr g_resmgr (0x5d6fa0): find_meshes 0x41b960, make_lod_mesh 0x41ca60, get_lod_mesh 0x41cdc0, get_render_target 0x41b7d0, get_texture 0x41c400, get_shadow_skin 0x41c420


// ---- the scene container's lighting pass --------------------------------------------------------------------------
struct Geom_t;
// (create_entity 0x428440 is rgl::world's own member, src/engine/scene/world.h; light_at 0x43ac40 is level::light_at of game/agent/ai/agent_types.h, see mesh_vertex_ops.cpp)



// ---- the character / troop mesh owner is the skin record (src/engine/render/skin.h, 0x1f8 bytes): the blend / mirror / collect members below
// are defined in this module's TUs (mesh_vertex_ops.cpp, scene_entities.cpp, character_meshes.cpp); apply_group / copy_troop are terrain_b's.
#include "engine/core/vec_elems.h"      // FaceTexture (Skin::face_textures)
#include "engine/render/skin.h"


#include "game/agent/animator.h"       // Animator, g_anim (the animation state, global object at 0x8ed4f60)


// ---- members of Terrain (src/game/terrain/terrain.h) defined in this TU --------------------------------------------
// The object is the Terrain of game/terrain/terrain.h when the world map is up and the Mission (game/mission/mission.h) in a battle: the
// same ~138 MB global (props at +0x6688 is Mission::props, wx/wy at +0x270 Terrain::wx). Its scene-entity builder methods are
// members of Terrain (terrain.h).
#include "game/config/options.h"   // g_options 0x5aa008
#include "engine/render/render_flags.h"

#endif // MESH_SLOT_H
