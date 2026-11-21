// misc.h: the classes of terrain_a.cpp (0x464200..0x46e590) that are not the terrain object: a mission helper
// block, the element stubs of the std::vector / list / stable_sort instantiations, the prop tween, the missile,
// the texture / shader / material binders, the mesh vertex-index fillers, the effect-slot record, the troop
// auto-equip, the vertex / index buffer uploader and the shadow-view draw pass. Names provisional; each offset cites
// the function that reads or writes it. Where a shared header (src/rgl/*.h, src/game/ui/*.h) already has the class
// but not the member, the member lives on a derived "view" class of the same layout (name ending in _x).
#ifndef TERRAIN_A_MISC_H
#define TERRAIN_A_MISC_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include <vector>
#include <list>
#include <algorithm>
#include <new>
#include "./common.h"
#include "./terrain.h"
#include "./game_state.h"
#include "../ui/troop_inv.h"
#include "../../rgl/resource_manager.h"
#include "../../rgl/particles/mesh_obj.h"

// ---- mission helper block (global 0x5e292c; the Mission object of src/game/ui/mission.h seen through 0x4676c0.. ) ----
struct m64 { int d[16]; };                      // 64-byte block (a matrix)
#include "../mode_rec.h"                      // ModeRec2, g_mission_templates 0x5e2a00 (+0x34 -> 0x4c-byte spawn rules)
#include "../../include/game_data.h"   // canonical: g_game (0x5e2930)

// rule_accepts (0x467940) is Mission::rule_accepts (game/mission.h): a thiscall member that ignores this

// ---- the allocator::construct instances of the 24 / 52 / 100-byte element vectors (terrain_a/terrain_emit.cpp): stay calls ----
#include "../alloc_construct.h"      // allocator<rec36 / rec136>::construct 0x436cb0 / 0x436cd0
#ifndef TERRAIN_ALLOC_DEFINED_HERE
namespace std {
template<> void allocator<elem24>::construct(elem24* p, const elem24& v);          // 0x468130
template<> void allocator<particle100>::construct(particle100* p, const particle100& v);   // 0x468170
template<> void allocator<elem52>::construct(elem52* p, const elem52& v);          // 0x468190
}
#endif

struct elem56 { tstring name; int a[10]; };     // 0x38: fill 0x46b3a0, copy 0x468050, copy_backward 0x4680c0
#include "../placed_obj.h"                    // PlacedObj: the 0x1c8-byte polymorphic record (operator= 0x437d60)
typedef PlacedObj E1c8;

// ---- timed transform tween of a scene prop (0x467c50) -------------------------------------------------------

// ---- missile (pool record; 0x467f80, 0x467fb0) --------------------------------------------------------------------
#include "../missile.h"                 // Missile (shared with agent4)

// per-component max / min of two 3-vectors (0x468300, 0x468360)
void __cdecl vmax3(float* a, const float* b);   // 0x468300
void __cdecl vmin3(float* a, const float* b);   // 0x468360

// ---- render-state cache calls (global object 0xa17910; the class is src/game/renderer.h's, which needs rgl.h) ---------
#include "../../include/renderer.h"       // renderer (shared)


// ---- the mesh members of this object (a view of rgl::mesh) -----------------------------------------------------------
#include "../../rgl/mesh.h"             // rgl::mesh (shared with src/rgl/core/resmgr.h)

// ---- effect slot record (0xd8 bytes; reset 0x4686a0, inline ctor / dtor in 0x46b9d0) --------------------------------
#include "../../include/fx.h"           // Quad4, EnvRec (shared with src/rgl/scene)
// (the record is FxDef of include/fx.h: FxDef::set_env_count is defined in terrain_a.cpp)

// ---- the troop's maximum hit points (Troop::max_hit_points 0x468770, include/troop.h) -------------------------------
#include "../skill_cap.h"   // canonical: g_skill_tbl (0x8ee3100)

// ---- troop auto-equip (0x4687a0 / 0x4687e0; a view of Troop) ---------------------------------------------------------
enum { itp_two_handed = 0x00200000, itp_civilian = 0x20000000 };
enum { ek_item_0 = 0, ek_head = 4, ek_body, ek_foot, ek_gloves, ek_horse };
enum {
    tf_hero = 0x00000010, tf_inactive = 0x00000020,
    tf_guarantee_boots = 0x00100000, tf_guarantee_armor = 0x00200000, tf_guarantee_helmet = 0x00400000,
    tf_guarantee_gloves = 0x00800000, tf_guarantee_horse = 0x01000000, tf_guarantee_shield = 0x02000000,
    tf_guarantee_ranged = 0x04000000
};

// ---- camera-relative view parameters (0x46b940) --------------------------------------------------------------------
#include "../../include/view_params.h"            // ViewParams (shared with terrain_b)

// ---- an agent's equipment slots, line of fire (0x46b190, 0x46b2d0, 0x46b410) ------------------------------------------------
struct ItemRef;                                // game/item_ref.h
ItemRef __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found);    // 0x46b190
extern unsigned __int64 g_bit[];                // 0x5a8e68 (g_bit[i] == 1 << i)


// ---- scene view setup (0x46ca50) -------------------------------------------------------------------------------------
// (the vertex / index buffer uploader 0x46d060 is dev_buffer::upload, src/game/dev_buffer.h)
extern char g_scene_dirty;                    // 0x5e2938 (set when a buffer must be recreated larger)

// ---- shadow-view draw pass (0x46d8c0) ------------------------------------------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat3;
// (the shadow-view draw 0x46d8c0 is render_batch::draw, src/game/render_batch.h)
extern unsigned g_stage_arg[6];                 // 0x5a9c00 D3DTA_* by 3-bit code

#endif // TERRAIN_A_MISC_H
