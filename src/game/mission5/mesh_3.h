// mesh_3.h: declarations of src/game/mission5/mesh_3.cpp (TU 117 "mesh_3", 0x52cf60..0x5305b0): the map editor's view update,
// the mesh (polygon-mesh) picking and editing members (vertex / edge / polygon ray tests, selection scale / transform /
// removal, copy), the editor panel update and the view controller's apply. The polygon mesh is PolyMesh of
// src/game/terrain_b/poly_mesh.h (Mission::nav): the picking and editing members 0x52da20..0x52edb0 are its members. The
// other classes are the views the slices of this stretch saw (suffix = the slice, e.g. MapEditor_s2cf60).
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
struct CreateGameOptionsPage {
    void update_checks();                                   // 0x4bf640 (config.h)
};
struct MapEditorDialog {                                    // rgl/mesh2/gui_edit.cpp
    void update_objects();                                  // 0x4c7e20
    void refresh();                                         // 0x4c77c0
    void apply(bool& moved, rgl::vector4& pos, bool& rotated, rgl::mat3& frame, bool& drag, rgl::vector4& dir, char& flag);   // 0x52fe40 (ret 0x1c; the view yaw is at +0x268)
};
struct Mission_s2b5c0 {
    void find_position(char* ok, rgl::vector4* pos);        // 0x52bdb0 (mission_5.cpp)
};
#include "../mission.h"                                     // Mission (add_placed 0x44f870, nav +0x6698, the order-marker meshes, props)
#include "../../include/game_data.h"                        // GameData (editor: the scene editor's state)
struct PlacedObj;                                           // placed_obj.h (below): the 0x1c8-byte record, reset 0x4352f0 (agent/agent.cpp)
struct GuiCtx;
struct MeshOwner_s2fe40;
void __stdcall build_mesh_name(MeshOwner_s2fe40* o, std::vector<rgl::mesh*>* out);   // 0x5302f0 (defined in mesh_3.cpp)
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m);   // 0x435860 (agent/agent.cpp)
void __fastcall pick_entity(GuiCtx* ctx);                   // 0x4ed8d0 (char_sheet/pick_entity.cpp)
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
#define g_editor_dialog_s2cf60 (*(EditorPanel_s2cf60*)&g_app_obj)
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


struct MapView_s2cf60 {
__forceinline void screen_to_world(vector4& p, vector4& q);         // 0x4bd2f0  picking ray (origin, target)
};

struct MapEditor_s2cf60 {
    char      pad0[0x60];
    MapView_s2cf60*  view;                                 // +0x60  the 3D view (0x52bdb0's Mission view)
    char      pad64[0x228 - 0x64];
    matrix    drag_frame;                           // +0x228  the drag frame handed to 0x52e960 / 0x52edb0
    float     yaw;                                  // +0x268  view yaw, handed to rotate_2x2

    void update();                                  // 0x52cf60
};

struct EditorPanel_s2cf60 {
__forceinline void update_checks();                    // 0x4bf640  (no arguments)
};


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
#define g_map_s2f250 (*(Map_s2f250**)&(*(void**)((char*)&g_mission + 0x0)))
#define g_objrecs_s2f250 (*(ObjRec20_s2f250**)&g_ptypes)
#define g_editor_dialog_s2f250 (*(MapEditorDialog_s2f250*)&g_app_obj)
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
struct Prop_s2f250 {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03();
    virtual void p04(); virtual void p05(); virtual void p06(); virtual void p07();
    virtual void p08(); virtual void p09(); virtual void p10(); virtual void p11();
    virtual void p12(); virtual void p13(); virtual void p14(); virtual void p15();
    virtual void p16(); virtual void p17(); virtual void p18(); virtual void p19();
    virtual void p20(); virtual void p21(); virtual void p22();
    virtual void apply(void* frame, void* pos); // slot 23 (+0x5c)
};

// ---- scene mesh_s2f250 + the 16-byte {pad, first, last, end} container ----------------------------------
struct mesh_s2f250 {
    __forceinline mesh_s2f250* clone();                // 0x41c6d0
    __forceinline void set_color(vector4 c);           // 0x47af30 (16-byte argument passed by value)
};
struct MeshVec_s2f250 {
    int alloc;                                  // +0 (unused by this function)
    mesh_s2f250** first;                               // +4
    mesh_s2f250** last;                                // +8
    mesh_s2f250** end;                                 // +0xc
    unsigned size() const { return first ? (unsigned)(last - first) : 0u; }
    unsigned capacity() const { return first ? (unsigned)(end - first) : 0u; }
    MeshVec_s2f250() : first(0), last(0), end(0) {}
    // the std::vector destructor the original runs when the local goes out of scope (0x52f7e0: operator delete 0x535712 on first, zero the pointers)
    ~MeshVec_s2f250() { if (first) ::operator delete(first); first = 0; last = 0; end = 0; }
    __forceinline void _Insert_n(mesh_s2f250** pos, unsigned n, mesh_s2f250** val)      // 0x410610 (vector<rgl::mesh*>::_Insert_n, shared by the pointer lists)
    { ((MeshPtrStdVec*)this)->insert_n((rgl::mesh**)pos, n, *(rgl::mesh**)val); }
    void push_back(mesh_s2f250* x)
    {
        if (size() < capacity()) {
            *last = x;
            last = last + 1;
        } else {
            _Insert_n(last, 1, &x);
        }
    }
};

// ---- scene object at g_map_s2f250+0x65e4: 64-byte frame at +0x58, mesh_s2f250 vector at +0x118, shown at +0x140 ---
struct mat3_s2f250 { vector4 s, f, u; };
struct smat_s2f250 : matrix {
    __forceinline void rotate_sf(const float* angle) { rotate_about_u(angle); }         // 0x40e770 (rgl::matrix::rotate_about_u)
};
struct SceneObj_s2f250 {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void recalc();                      // slot 4  (+0x10)
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8();
    virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12();
    virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16();
    virtual void s17(); virtual void s18();
    virtual void clear();                       // slot 19 (+0x4c)
    char pad14[0x58 - 4];                       // vptr is the first dword
    smat_s2f250 frame;                                 // +0x58  (s, f, u, o with o at +0x88)
    char pad98[0x118 - 0x98];
    MeshVec_s2f250 meshes;                             // +0x118
    char pad128[0x140 - 0x128];
    char visible;                               // +0x140  preview shown (rgl::entity::visible)
};

// ---- map world (0x5e292c) and frame state (0x5e2930) ---------------------------------------------
struct Map_s2f250 {
    char pad0[0x65e4];
    SceneObj_s2f250* object;                           // +0x65e4  the edited scene object
    char pad65e8[0x668c - 0x65e8];
    PlacedObj* records;                         // +0x668c  the 0x1c8 record array
    __forceinline void init_placed(PlacedObj*, const int* kind, const int* id, const int* value, const matrix*);   // 0x435860
    __forceinline void build_mesh_name(PlacedObj*, MeshVec_s2f250*);    // 0x5302f0
    __forceinline int add_placed(const int* kind, const int* id, const int* value, const matrix*, int* spawn);     // 0x44f870 (Mission::add_placed)
};
// the frame state at 0x5e2930 is GameData::editor (include/game_data.h SceneEditor)
struct ObjRec20_s2f250 {                               // 0x20-byte record at 0x5e28f8
    char pad0[0x18];
    int variants;                               // +0x18
    char pad1c[4];
};

// ---- editor objects -----------------------------------------------------------------------------
struct EditorView_s2f250 {
    char pad0[0x2f9];
    char f2f9;                                  // +0x2f9
    __forceinline void pick_entity() { ::pick_entity((GuiCtx*)this); }      // 0x4ed8d0
};
struct MapEditorDialog_s2f250 {
    __forceinline void update_objects() { ((MapEditorDialog*)this)->update_objects(); }     // 0x4c7e20
    __forceinline void refresh() { ((MapEditorDialog*)this)->refresh(); }                  // 0x4c77c0
};

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

// ---- the map editor screen object (this) ----------------------------------------------------------
// The frame of the last 0x30 dwords of the mission's first member (the scene container at g_mission+0, read here as
// Map_s2f250's +0x248..): the look direction, the placement state and the picked world position.
struct MapEditor_s2f250 {
    char pad0[0x60];
    void* view;                                 // +0x60  3D view, used by 0x52bdb0
    char pad64[0x248 - 0x64];
    float dir_x, dir_y, dir_z;                  // +0x248  editor look direction
    char pad254[0x268 - 0x254];
    float yaw;                                  // +0x268  the editor's view yaw, handed to 0x52fe40 (MapEditorDialog::apply)
    char pad26c[0x2f8 - 0x26c];
    char f2f8;                                  // +0x2f8  placement adjusted
    char pad2f9[0x304 - 0x2f9];
    vector4 f304;                               // +0x304  last picked position
    int f314;                                   // +0x314  down-repeat timer
    int f318;                                   // +0x318  up-repeat timer
    int f31c;                                   // +0x31c  selected variant
    int f320;                                   // +0x320  previous variant
    __forceinline void find_position(char* ok, vector4* pos) { ((Mission_s2b5c0*)this)->find_position(ok, pos); } // 0x52bdb0
    void update();                              // 0x52f250
};

// ======================================================= slice s0052fe40: classes, globals, helpers
#define g_items_s2fe40 (*(Item_s2fe40**)&g_items)
#define g_map_pts_s2fe40 (*(MapEntry_s2fe40**)&g_ptypes)
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

struct Item_s2fe40 : ItemKind {                     // the g_items records (0x5e29d0), reached through this 3-argument view
    __forceinline rgl::mesh* mesh_for(int lo, unsigned hi, int b)   // 0x433cb0 (the first two words are its 64-bit class mask)
    { return ItemKind::mesh_for(((unsigned __int64)hi << 32) | (unsigned)lo, b); }
};
typedef ItemRef ItemRef_s2fe40;                     // +0 item, +4 m4 (game/item_ref.h); mesh_name 0x434b20

struct MapName_s2fe40 { char pad[0x48]; };
struct MapEntry_s2fe40 {                                // = ptype (game/ptype.h: name / flags / nframes / boxes); `count` and
    tstring name;                                       // +0x00  `names` keep the names build_mesh_name uses
    __int64 flags;                                      // +0x10
    int count;                                          // +0x18
    MapName_s2fe40* names;                              // +0x1c
};



#include "../game_globals.h"
int find_in_table78(const tstring* name);               // 0x4352a0
#include "../config/config_block.h"   // Config g_cfg (0x5a9f98)



// ---- the object the mesh_s2fe40 chooser reads (offsets from this disassembly) --------------------------------
struct MeshOwner_s2fe40 {
    char pad0[0x188];
    int f188;                                           // +0x188 kind
    int f18c;                                           // +0x18c item / map point id
    tstring name;                                       // +0x190
    int f1a0;                                           // +0x1a0 variant / index
};

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
__forceinline mesh_s2f250* mesh_s2f250::clone() { return (mesh_s2f250*)((rgl::mesh*)this)->clone(); }
__forceinline void mesh_s2f250::set_color(vector4 c) { ((rgl::mesh*)this)->set_color(c); }
__forceinline void Map_s2f250::init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m)
{ ::init_placed(o, kind, id, value, m); }
__forceinline void Map_s2f250::build_mesh_name(PlacedObj* o, MeshVec_s2f250* v)
{ ::build_mesh_name((MeshOwner_s2fe40*)o, (std::vector<rgl::mesh*>*)v); }
__forceinline int Map_s2f250::add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn)
{ return ((Mission*)this)->add_placed(kind, id, value, m, spawn); }
__forceinline void MapView_s2cf60::screen_to_world(vector4& p, vector4& q) { ((MapWidget*)this)->screen_to_world(&p, &q); }
__forceinline void EditorPanel_s2cf60::update_checks() { ((CreateGameOptionsPage*)this)->update_checks(); }
__forceinline void mat3_s2cf60::rotate_fu(const float& angle) { ((matrix*)this)->rotate_about_s(&angle); }
__forceinline void mat3_s2cf60::rotate_su(const float& angle) { ((matrix*)this)->rotate_about_f(&angle); }
__forceinline void mat3_s2cf60::rotate_sf(const float& angle) { ((matrix*)this)->rotate_about_u(&angle); }

#endif // MISSION5_MESH_3_H
