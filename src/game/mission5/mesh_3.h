// mesh_3.h: declarations of src/game/mission5/mesh_3.cpp (TU 117 "mesh_3", 0x52cf60..0x5305b0): the map editor's view update,
// the mesh (polygon-mesh) picking and editing members (vertex / edge / polygon ray tests, selection scale / transform /
// removal, copy), the editor panel update and the view controller's apply. The polygon mesh is PolyMesh of
// src/game/terrain_b/poly_mesh.h (Mission::nav): the picking and editing members 0x52da20..0x52edb0 are its members. The
// other classes are the views the slices of this stretch saw (suffix = the slice, e.g. mat3_s2cf60).
// tstring, the streams and rgl::vector4 / rgl::matrix are the shared headers' (src/game/tstring.h, src/rgl/stream.h,
// src/rgl/rgl_math.h). Offsets cite the function that proves them in each view's comments.
#ifndef MISSION5_MESH_3_H
#define MISSION5_MESH_3_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <new>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"
#include "../key_names.h"

// ---- canonical declarations -----------------------------------------------------------------------------------------------------
// The slices below each saw the same objects through their own view classes. The members they call that other objects define are
// declared here under the defining classes (the names the definitions carry) and reached from the views by forwarders at the end of
// this header (the views are layout-compatible: each cites the offsets it shares).
#include "../terrain_b/poly_mesh.h"     // PolyMesh (Mission::nav): MVert / MPoly / MEdge, the editor's members
#include "../map_widget.h"            // MapWidget::screen_to_world 0x4bd2f0 (config/config.cpp)
#include "../mission.h"                                     // Mission (add_placed 0x44f870, nav +0x6698, the order-marker meshes, props)
#include "../../include/game_data.h"                        // GameData (editor: the scene editor's state)
#include "../options_panel/mission_item_recs.h"              // MissionBlocker: the map editor screen (find_position 0x52bdb0)
#include "../../rgl/scene/scene_entity.h"                    // rgl::entity: Mission::edit_obj (the placement preview)
struct PlacedObj;                                           // placed_obj.h (below): the 0x1c8-byte record, reset 0x4352f0 (agent/agent.cpp)
void __stdcall build_mesh_name(PlacedObj* o, std::vector<rgl::mesh*>* out);   // 0x5302f0 (mesh_poly_ray.cpp): the mesh list of a placed object
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m);   // 0x435860 (agent/agent.cpp)
#include "../item_ref.h"                                    // ItemRef (item / m4 / mesh_name 0x434b20) and ItemKind (include/item_kind.h:
                                                        // mesh_name_for 0x433bf0, mesh_for 0x433cb0, type_flags +0x1c8)
struct Vec3; namespace rgl { struct matrix; } typedef rgl::matrix pos;
void vec3_sub(Vec3* out, const Vec3* a, const Vec3* b);     // 0x432830 (cdecl)
namespace rgl { struct mat3; }
struct mface;
// PolyMesh::make_copy 0x52e960 empties the copy buffers through the library's clear() out of line: one body for every vector of
// trivially destructible elements (VC7.1's clear() is _Tidy(); /OPT:ICF folded the frameless copies into 0x41f4b0, entity.cpp)
namespace std {
template<> void vector<MVert, allocator<MVert> >::clear();              // 0x41f4b0
template<> void vector<MPoly, allocator<MPoly> >::clear();              // 0x41f4b0
template<> void vector<MEdge, allocator<MEdge> >::clear();              // 0x41f4b0
}
#include "../std_vec_out_of_line.h"
#include "../std_vec_insert_n.h"      // vector<rgl::mesh*>::_Insert_n 0x410610

using rgl::vector4;
using rgl::matrix;

// ---- the globals the slices saw with their own view types (the canonical declaration; the per-slice views below cast it)
struct Mission; struct GameData;
extern ItemKind* g_items;                        // 0x5e29d0 (include/item_kind.h)
extern Mission* g_mission;                       // 0x5e292c
extern GameData* g_game;                         // 0x5e2930
#include "../config/core.h"      // GameApp g_app_obj (0x8ee3c48)
#include "../ptype.h"
#include "../window_mgr.h"   // g_windows (0xa73980)
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
#include "../key_names.h"      // g_keyboard (0xa170c8)
#include "../config/config_block.h"


// ======================================================= slice s0052cf60: classes, globals, helpers
// Slice s0052cf60: the world-map mesh_s2cf60 editor's per-frame update (0x52cf60, 2749 bytes).
//
// One thiscall member of the map editor.  Each frame it
//   * mirrors the game's selected sub-option into the edited polygon mesh (g_mission->nav, a PolyMesh),
//   * picks the mesh_s2cf60 element under the cursor (vertex / edge / polygon, depending on the mesh_s2cf60 mode),
//   * handles the three mode-switch keys,
//   * applies the translate (0x17124) and the three rotation (0x1715c / 0x17188 / 0x17128 / 0x17184)
//     keys, building the transform with identity3 + matrix_rotate_fu/su/sf,
//   * sets the per-mode highlight colour (set_colour) - the original shares that code between several
//     entry points, hence the goto labels L59f/L5c6/... mirroring the basic blocks,
//   * runs the two drag keys, and
//   * rebuilds whichever of the three rgl meshes were flagged dirty.
//
// Names are the repo's where an earlier slice already resolved them (select_mode 0x4c53f0,
// update_terrain 0x4c7f90, pick_vert/edge/poly 0x52da20/0x52dc00/0x52df40, build_tetra/ribbon/tri
// 0x4e65e0/0x4e6a70/0x4e75a0, cell_rect 0x4bd2f0, identity3 0x40cd60, matrix_rotate_fu/su/sf
// 0x40e570/0x40e670/0x40e770, rotate_2x2 0x4f3b40, update_option_checks 0x4bf640).  The remaining
// names are provisional; for those the address is the evidence.

using rgl::vector4;



namespace rgl {
struct mesh_s2cf60;                                        // only used through pointers here
}

// ---- engine globals ---------------------------------------------------------------------------






// ---- transform helpers ------------------------------------------------------------------------
struct mat3_s2cf60 {                                       // three basis rows, 0x30 bytes
    float m[12];
__forceinline void rotate_fu(const float& angle);             // 0x40e570  forward/up rows by *angle
__forceinline void rotate_su(const float& angle);             // 0x40e670  side/up rows by *angle
__forceinline void rotate_sf(const float& angle);             // 0x40e770  side/forward rows by *angle
};
__forceinline void identity3(mat3_s2cf60* m) { identity3((rgl::mat3*)m); }      // 0x40cd60 (cdecl)

#include "../../include/rot2.h"                               // rot2: the 2x2 rotation matrix, rotate_2x2 0x4f3b40

// ---- per-key input state ----------------------------------------------------------------------
// Three 0x400-byte tables of per-scan-code ints at 0xa17000 / 0xa17400 / 0xa17800: held state,
// this frame's press, and the previous frame's press (s0052f250 names 0xa17458/0xa17858
// g_keyboard.down[225] / g_keyboard.prev[225]).  Only the codes read here are declared.















#include "../slot.h"                        // Slot, g_keyboard (0xa170c8)


extern const float g_hilite_dim;                    // 0x54b4d0 = 0.05f
extern const float g_rot_step;                      // 0x590c60 = 0.005f
extern const double g_percent;                      // 0x59c480 = 0.01

// ======================================================= slice s0052da20: classes, globals, helpers

// The polygon mesh's picking members (PolyMesh::pick_vert .. poly_ray 0x52da20..0x52dfd0): nearest vertex / edge / polygon to a
// mouse ray.

namespace rgl {
__forceinline void vec3_sub(vector4& out, const vector4& a, const vector4& b)   // 0x432830: out = a - b (xyz)
{ ::vec3_sub((Vec3*)&out, (const Vec3*)&a, (const Vec3*)&b); }
}

// ======================================================= slice s0052dfd0: classes, globals, helpers

// The polygon mesh's editing members (PolyMesh::toggle_flag .. remove_selected 0x52e510..0x52e870): selection toggles and transforms.

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// ======================================================= slice s0052e960: classes, globals, helpers

// PolyMesh::make_copy .. apply_copy (0x52e960..0x52edb0): duplicate the selected polygons into the copy buffers, projecting every
// copied vertex through a basis built from the caller's matrix.

// Inline copy of the vector cross product: the original inlines this in the frame builder
// (rgl::cross at 0x407eb0 is the out-of-line version and shows up as a call instead).
namespace rgl {
inline vector4 vcross_s2e960(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x);
}
}

// Basis -> world: the copy buffers hold coordinates in the (s, f, u) frame, so applying the
// copy is just a column combination plus the origin.
namespace rgl {
inline vector4 to_world_s2e960(const matrix& m, const vector4& p)
{
    // x row: the y and z terms are summed first (0x52f02c..0x52f04d; without the parentheses cl adds the x term
    // second). Still open in apply_copy: the original multiplies the z term before the y term there.
    return vector4((m.u.x * p.z + m.f.x * p.y) + m.s.x * p.x + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}
}

// Duplicate every flagged polygon into copy_faces, mapping its vertex/edge references through
// find-or-create helpers so shared vertices stay shared in the copy.  The basis rows are
// (s, f, up) with up = world Z: s = f x up normalised, f = up x s.
// ======================================================= slice s0052f250: classes, globals, helpers
// Slice s0052f250: the map editor's per-frame object placement update (0x52f250, 3049 bytes).
// Each frame it repeats the scroll keys into adjust_mode (0x4bb260), rebuilds the preview entity
// (clear, a temporary 0x1c8 placed-object record, its meshes cloned into the entity, a camera frame
// built from the editor's direction), lets 0x52fe40 nudge the placement, picks a world position with
// 0x52bdb0, and finally either just moves the preview or commits a placed object through
// Mission::add_placed (0x44f870) + the map-editor dialog.  Names and layouts are provisional; every
// offset here is read off this function's disassembly (see work/match/scratch_s0052f250_dis.txt).

using rgl::vector4;
using rgl::matrix;



// ---- engine string (16 bytes) -------------------------------------------------------------------


#include "../../include/fx.h"

// ---- the 0x1c8-byte placed-object record (vtable 0x590b84): the one class of src/game/placed_obj.h (this slice used to carry its own
// IfaceB / Quad / PlacedObj copies, with a vtable of their own) ------------------------------------------------------------------
#include "../placed_obj.h"

// ---- the non-virtual slot at +0x5c of the object behind record->m1b4 ------------------------------


struct mat3_s2f250 { vector4 s, f, u; };

// ---- map world (0x5e292c) and frame state (0x5e2930) ---------------------------------------------
// the frame state at 0x5e2930 is GameData::editor (include/game_data.h SceneEditor)

// ---- editor objects -----------------------------------------------------------------------------

// ---- globals (all offsets/addresses from the disassembly) ----------------------------------------






// ---- callees / helpers not local to this slice ---------------------------------------------------
void set_list_scroll(int dir);                      // 0x4bb260 (cdecl)
__forceinline mat3_s2f250* identity3(mat3_s2f250* out) { return (mat3_s2f250*)identity3((rgl::mat3*)out); }   // 0x40cd60 (cdecl, returns out)

// The original inlines a component-wise cross (0x407dc0's body); keep the same evaluation order.
static __forceinline vector4 cross_inline_s2f250(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
// Inlined vector4::normalize (0x407d60): (0,1,0) when the length is not > 1e-10.
static __forceinline void normalize_inline_s2f250(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float s = 1.0f / len;
        v.x *= s;
        v.y *= s;
        v.z *= s;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}


// ======================================================= slice s0052fe40: classes, globals, helpers
// Slice s0052fe40: the mission view's per-frame camera update and the mesh_s2fe40-name
// chooser used by the level/object browser.
//   0x52fe40  View::apply: integrate the look axes into the eye position, build the
//             view matrix (identity + fu/su/sf rotations + a z rotation for the
//             2x2 heading), and scale the per-frame velocity vector.  Seven flag
//             out-parameters tell the caller which parts were touched.
//   0x5302f0  build_mesh_name: choose the mesh_s2fe40 list for the selected object
//             (item / map / map point / passage / entry arrow) and look it up in
//             the resource manager.
// All names are provisional; every offset is read off this disassembly.
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos)



// ---- engine string (layout from s0042b790 / s004333c0) ------------------------------------------------


// ---- global input axes / camera flags (0xa17xxx block; names provisional) -------------------------------

extern float  g_k005;                                   // 0x590c60 (0.005f)
extern double g_d001;                                   // 0x59c480 (0.01)
extern float  g_k005b;                                  // 0x54b4d0 (0.05f)

// ---- game tables ---------------------------------------------------------------------------------------
namespace rgl {
struct mesh_s2fe40;
}




#include "../game_globals.h"
int find_in_table78(const tstring* name);               // 0x4352a0
#include "../config/config_block.h"   // Config g_cfg (0x5a9f98)




// ---- matrix / rotation helpers -------------------------------------------------------------------------
struct Matrix12_s2fe40 {
    float v[12];
    __forceinline void rotate_fu(const float* angle) { ((matrix*)this)->rotate_about_s(angle); }     // 0x40e570
    __forceinline void rotate_su(const float* angle) { ((matrix*)this)->rotate_about_f(angle); }     // 0x40e670
    __forceinline void rotate_sf(const float* angle) { ((matrix*)this)->rotate_about_u(angle); }     // 0x40e770
};
__forceinline Matrix12_s2fe40* identity3(Matrix12_s2fe40* out) { return (Matrix12_s2fe40*)identity3((rgl::mat3*)out); }     // 0x40cd60


// rotate one width-2 row of the view matrix (the original inlines this three times)
static void rotate_pair_s2fe40(float* p, float c, float s)
{
    float x = p[0], y = p[1];
    p[0] = c * x - s * y;
    p[1] = c * y + s * x;
}


// ---- forwarders: the views reach the functions other objects define through the classes that define them -------------------------------
__forceinline void mat3_s2cf60::rotate_fu(const float& angle) { ((matrix*)this)->rotate_about_s(&angle); }
__forceinline void mat3_s2cf60::rotate_su(const float& angle) { ((matrix*)this)->rotate_about_f(&angle); }
__forceinline void mat3_s2cf60::rotate_sf(const float& angle) { ((matrix*)this)->rotate_about_u(&angle); }

#endif // MISSION5_MESH_3_H
