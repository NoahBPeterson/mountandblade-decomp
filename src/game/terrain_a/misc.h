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

struct RuleRec { int f0; unsigned flags; unsigned f8; };    // flags: 0x100/0x200/0x400 and 0x10000/0x20000/0x40000 (0x467940)
struct CandRec { int f0; int troop; unsigned char f8; };
// rule_accepts (0x467940) is Mission::rule_accepts (game/mission.h): a thiscall member that ignores this

// ---- std::vector<T> (VC7.1 layout: allocator at +0, _First +4, _Last +8, _End +0xc) and allocator::construct stubs ----
#include "../alloc_construct.h"      // alloc_rec36 0x436cb0, alloc_rec136 0x436cd0
struct alloc24 { void construct(elem24* p, const elem24& v); void destroy(elem24* p) { p->~elem24(); } };            // 0x468130
struct alloc100 { void construct(particle100* p, const particle100& v); void destroy(particle100* p) { p->~particle100(); } };   // 0x468170
struct alloc52 { void construct(elem52* p, const elem52& v); void destroy(elem52* p) { p->~elem52(); } };            // 0x468190

struct vec36 { alloc_rec36 _Alval; rec36* first; rec36* last; rec36* end; int size() const; rec36* _Ucopy(rec36* f, rec36* l, rec36* d); };         // 0x467ac0 / 0x46c150
struct vec24 { alloc24 _Alval; elem24* first; elem24* last; elem24* end; int size() const; elem24* _Ucopy(elem24* f, elem24* l, elem24* d); };   // 0x467ae0 / 0x46c170
struct vec52 { alloc52 _Alval; elem52* first; elem52* last; elem52* end; int size() const; elem52* _Ucopy(elem52* f, elem52* l, elem52* d); };   // 0x467b00 / 0x46c1b0
struct vec516 { int alloc; s516* first; s516* last; s516* end; int size() const; };      // 0x467b20
struct vec100 { alloc100 _Alval; particle100* first; particle100* last; particle100* end; particle100* _Ucopy(particle100* f, particle100* l, particle100* d); };   // 0x46c190

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


// ---- texture / shader / skin: the members the binders add (views of src/rgl/resource.h's classes) ---------------------
struct texture_x : texture {
    void bind(int stage, float t);              // 0x4683c0 (+0x28 is the frame count of an animated texture)
};
struct material_x : material {
    void bind(texture** textures, const float* t);      // 0x468440
};
struct skin_x : skin {
    void apply_state(const float* t);           // 0x4684d0 (flags +0x1c, shader +0x30, specular colour +0x34, power +0x44)
};

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
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found);    // 0x46b190
extern unsigned __int64 g_bit[];                // 0x5a8e68 (g_bit[i] == 1 << i)


// ---- scene view setup (0x46ca50) -------------------------------------------------------------------------------------
struct light_src {                              // one scene light; types 0/1/2 map to D3DLIGHT_POINT/SPOT/DIRECTIONAL
    int type;                                   // 0x00
    float diffuse[3];                           // 0x04
    int pad10;
    float specular[3];                          // 0x14
    int pad20;
    float pos[3];                               // 0x24
    int pad30[5];
    float dir[3];                               // 0x44
    int pad50;
    float range;                                // 0x54
    float atten0, atten1, atten2;               // 0x58
    float theta, phi;                           // 0x64
};
// (the vertex / index buffer uploader 0x46d060 is dev_buffer::upload, src/game/dev_buffer.h)
extern char g_scene_dirty;                    // 0x5e2938 (set when a buffer must be recreated larger)

// ---- shadow-view draw pass (0x46d8c0) ------------------------------------------------------------------------------------
struct light_t {
    int type;                                   // 0: point, 2: always on
    char pad4[0x24 - 4];
    rgl::vector4 pos;                           // 0x24
    char pad34[0x54 - 0x34];
    float range;                                // 0x54
};
#include "../../include/mat4.h"
using rgl::rot3;
// (the shadow-view draw 0x46d8c0 is render_batch::draw, src/game/render_batch.h)
extern unsigned g_stage_arg[6];                 // 0x5a9c00 D3DTA_* by 3-bit code

#endif // TERRAIN_A_MISC_H
