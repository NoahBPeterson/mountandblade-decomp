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
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"
#include "../tstring.h"
#include "../arr_set.h"
#include "../terrain_a/terrain.h"
#include "../terrain_b/troop_mgr.h"
#include "../placed_obj.h"
#include "../../include/timer.h"
#include "../../include/shape.h"
#include "../../include/item_kind.h"
#include "../item_ref.h"
#include "../mission.h"
#include "../shadow_map.h"

#define NOINL __declspec(noinline)     // keeps the definition's name visible to tools/difftest/slice.py's marker scan
using rgl::vector4;
using rgl::matrix;

extern float g_ramp_gamma;                      // 0x5a9bfc gamma exponent of the software colour path (0x463790)
unsigned pack_signed_color(const float* c);     // 0x461370 (cdecl)

// ---- members of rgl::mesh defined in this TU (0x47aa40..0x47b3b0) ------------------------------------------------
// rgl::mesh (the members above) is src/rgl/mesh.h
// (compute_normals 0x451c50 is rgl::mesh's own member, reset 0x428f30 rgl::scene's; clone 0x47a370 is tmesh::clone, terrain_b/tmesh.h: only that
// member is declared, under its class, because the header brings another layout)
struct tmesh {
    tmesh* clone();   // 0x47a370
};
struct lod_mesh_ms : rgl::lod_mesh {
    rgl::lod_mesh* clone() { return (rgl::lod_mesh*)((tmesh*)this)->clone(); }     // 0x47a370
};
struct scene_ms2 : rgl::scene {
};

#include "../../rgl/resource.h"   // resmgr g_resmgr (0x5d6fa0): find_meshes 0x41b960, make_lod_mesh 0x41ca60, get_lod_mesh 0x41cdc0, get_render_target 0x41b7d0, get_texture 0x41c400, get_shadow_skin 0x41c420

// ---- the lod mesh and its lists: members defined here -------------------------------------------------------------
#include "../lod_mesh_x.h"             // mesh_list_x / lod_mesh_x (shared with terrain_a/misc.h)

// ---- the scene container's lighting pass --------------------------------------------------------------------------
struct Geom_t;
// (create_entity 0x428440 is rgl::world's own member, src/rgl/scene/world.h; light_at 0x43ac40 is level::light_at of agent2/agent_types.h, see mesh_ops.cpp)



// ---- the character / troop mesh owner is the skin record (src/include/skin.h, 0x1f8 bytes): the blend / mirror / collect members below
// are defined in this module's TUs (mesh_ops.cpp, mesh_scene.cpp, mesh_char.cpp); apply_group / copy_troop are terrain_b's.
#include "../../include/vec_elems.h"      // E176 (Skin::face_textures)
#include "../../include/skin.h"

// the colour ramp / material record of the character owner's face textures (0xb0 bytes, ramp.h: Ramp, E176 of include/vec_elems.h; terrain_a/game_state.h includes it too)
#include "../ramp.h"

#include "../animator.h"       // Animator, g_anim (the animation state, global object at 0x8ed4f60)


// ---- members of Terrain (src/game/terrain_a/terrain.h) defined in this TU --------------------------------------------
// The object is the Terrain of terrain_a/terrain.h when the world map is up and the Mission (game/mission.h) in a battle: the
// same ~138 MB global (props at +0x6688 is Mission::props, wx/wy at +0x270 Terrain::wx). Its scene-entity builder methods are
// members of Terrain (terrain.h).
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../../include/render_flags.h"
struct ShadowMapP : ShadowMap {
    rgl::scene& sc() { return *(rgl::scene*)((char*)this + 0x18); }
    NOINL void build_hw();   // 0x47fce0
    NOINL void build_sw();   // 0x47fdd0
    NOINL void render();     // 0x47fef0
};

#endif // MESH_SLOT_H
