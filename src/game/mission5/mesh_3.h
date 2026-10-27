// mesh_3.h: declarations of src/game/mission5/mesh_3.cpp (TU 117 "mesh_3", 0x52cf60..0x5305b0): the map editor's view update,
// the mesh (polygon-mesh) picking and editing members (vertex / edge / polygon ray tests, selection scale / transform /
// removal, copy), the editor panel update and the view controller's apply. The classes are the views the slices of this
// stretch saw (suffix = the slice, e.g. Mesh_s2da20); the polygon mesh is PolyMesh of src/game/terrain_b/poly_mesh.h
// (mface = MVert, medge = MEdge, mpoly = MPoly there, +0x18 / +0x28 / +0x38 of the Mesh view = NavCore c.verts / faces / edges).
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
#include "../../rgl/mesh_x.h"

// ---- canonical declarations -----------------------------------------------------------------------------------------------------
// The slices below each saw the same objects through their own view classes. The members they call that other objects define are
// declared here under the defining classes (the names the definitions carry) and reached from the views by forwarders at the end of
// this header (the views are layout-compatible: each cites the offsets it shares).
struct graph_obj {                                          // the polygon mesh as rgl/mesh2/gui_edit.cpp / mesh_2.cpp define its members
    void select_mode(int mode, char on);                    // 0x4c53f0
    void update_terrain(int mode, const float* dir);        // 0x4c7f90
    void calc_vertex_mode(int mode);                        // 0x4c54f0
    void remove_node(int index);                            // 0x4c8af0
    void remove_edge(int index);                            // 0x4ca9b0
    void remove_face(int index);                            // 0x4cb940
};
struct PolyMesh {
    void build_adjacency();                                 // 0x470920 (terrain_b/terrain_b.cpp)
};
struct Shape {                                              // char_sheet/progress_bar.cpp
    void build_spikes(rgl::mesh* m);                        // 0x4e65e0
    void build_ribbons(rgl::mesh* m);                       // 0x4e6a70
    void polygon_to_mesh(rgl::mesh* m);                     // 0x4e75a0
};
struct camera_ray {
    void screen_to_world(rgl::vector4* from, rgl::vector4* to);   // 0x4bd2f0 (config/config.cpp)
};
struct CreateGameOptionsPage {
    void update_checks();                                   // 0x4bf640 (config.h)
};
struct MapEditorDialog {                                    // rgl/mesh2/gui_edit.cpp
    void update_objects();                                  // 0x4c7e20
    void refresh();                                         // 0x4c77c0
};
struct Mission_s2b5c0 {
    void find_position(char* ok, rgl::vector4* pos);        // 0x52bdb0 (mission_5.cpp)
};
struct Mission {
    int add_placed(const int* kind, const int* id, const int* value, const rgl::matrix* m, int* spawn);   // 0x44f870 (agent4/agent_logic.cpp)
};
struct PlacedObj {
    void reset();                                           // 0x4352f0 (agent/agent.cpp)
};
struct GuiCtx;
struct MeshOwner_s2fe40;
void __stdcall build_mesh_name(MeshOwner_s2fe40* o, std::vector<rgl::mesh*>* out);   // 0x5302f0 (defined in mesh_3.cpp)
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m);   // 0x435860 (agent/agent.cpp)
void __fastcall pick_entity(GuiCtx* ctx);                   // 0x4ed8d0 (char_sheet/pick_entity.cpp)
struct ItemRef {
    tstring mesh_name();                                    // 0x434b20 (item_ref.h)
};
struct res_obj;
struct ItemKind {                                           // include/item_kind.h
    tstring mesh_name_for(int bit);                         // 0x433bf0
    res_obj* mesh_for(unsigned __int64 cls, int bit);       // 0x433cb0 (returns the res_obj that is the mesh)
};
struct resmgr {
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);   // 0x41b960 (rgl/core/resmgr.h)
};
struct Vec3; struct pos;
void vec3_sub(Vec3* out, const Vec3* a, const Vec3* b);     // 0x432830 (cdecl)
pos* __cdecl identity_pos(pos* out);                        // 0x40cd00
namespace rgl { struct mat3; }
rgl::mat3* identity3(rgl::mat3* out);                       // 0x40cd60 (cdecl; returns out)
struct mface;
struct MVert { char b[0x18]; };  struct MPoly { char b[0x64]; };  struct MEdge { char b[0x34]; };   // terrain_b/poly_mesh.h
template<class T> struct vec { void* v0; void* v1; void* v2; void* v3; void clear(); };     // entity.h's vec<T>: vec<mface>::clear is 0x41f4b0, shared by every 4-byte-element vector
namespace std {
template<> void vector<MVert, allocator<MVert> >::push_back(const MVert&);              // 0x477c10 (terrain_b)
template<> void vector<MPoly, allocator<MPoly> >::push_back(const MPoly&);              // 0x477d20
template<> void vector<MEdge, allocator<MEdge> >::push_back(const MEdge&);              // 0x477e10
}
#include "../std_vec_out_of_line.h"

using rgl::vector4;
using rgl::matrix;

// ---- the globals the slices saw with their own view types (one raw declaration each)
extern void* g_game; // 0x5e2930
extern char g_app_obj[]; // 0x8ee3c48
extern char g_mission[]; // 0x5e292c
#include "../ptype.h"
extern char g_windows[]; // 0xa73980
extern void* g_items; // 0x5e29d0
#include "../prop_kind.h"


// ======================================================= slice s0052cf60: classes, globals, helpers
#define g_game_s2cf60 (*(Game_s2cf60**)&g_game)
#define g_scene_s2cf60 (*(Scene_s2cf60**)&(*(void**)((char*)&g_mission + 0x0)))
#define g_editor_dialog_s2cf60 (*(EditorPanel_s2cf60*)g_app_obj)
// Slice s0052cf60: the world-map mesh_s2cf60 editor's per-frame update (0x52cf60, 2749 bytes).
//
// One thiscall member of the map editor.  Each frame it
//   * mirrors the game's selected sub-option into the editable mesh_s2cf60 (g_scene_s2cf60->mesh_s2cf60, a gui_manager_s2cf60
//     living at 0x5e292c+0x6698),
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

// ---- gui_manager_s2cf60: the editable mesh_s2cf60 (layout from slices s004c5a20 and s0052da20) ----------------
// vertices/polys/edges vectors at +0x18/+0x28/+0x38 (0x18/0x64/0x34-byte elements), +0x78 mode,
// +0x7c current element.  The members past +0x80 are only seen in this run of functions.
struct gui_manager_s2cf60 {
    char pad0[0x78];
    int  mode;                                      // +0x78  0 vertex, 1 edge, 2 polygon
    int  cur;                                       // +0x7c  current element index
    char pad80[0x94 - 0x80];
    rgl::mesh* mesh94;                              // +0x94  rebuilt with build_tri
    rgl::mesh* mesh98;                              // +0x98  rebuilt with build_tetra
    rgl::mesh* mesh9c;                              // +0x9c  rebuilt with build_ribbon
    char padA0[0xac - 0xa0];
    char need_redraw[3];                            // +0xac..+0xae  one rebuild flag per mode

__forceinline unsigned pick_vert(const vector4& p, const vector4& d);     // 0x52da20
__forceinline unsigned pick_edge(const vector4& p, const vector4& d);     // 0x52dc00
__forceinline unsigned pick_poly(const vector4& p, const vector4& d);     // 0x52df40
__forceinline void select_mode(int mode, char on);                        // 0x4c53f0
__forceinline void toggle_flag(int mode, int index);                   // 0x52e510 (provisional)
__forceinline void scale_selected(int mode, const vector4& c);                // 0x52e590 (provisional)
__forceinline void transform_selected(int mode, const float* m);                   // 0x52e6d0 (provisional)
__forceinline void remove_selected();                                         // 0x52e870 (provisional)
__forceinline void make_copy(void* state);                               // 0x52e960 (provisional)
__forceinline void apply_copy(void* state);                                // 0x52edb0 (provisional)
__forceinline void update_terrain(int mode, const vector4& dir);          // 0x4c7f90
__forceinline void build_spikes(rgl::mesh* m);                             // 0x4e65e0
__forceinline void build_ribbons(rgl::mesh* m);                            // 0x4e6a70
__forceinline void polygon_to_mesh(rgl::mesh* m);                               // 0x4e75a0
};

// ---- engine globals ---------------------------------------------------------------------------
struct Game_s2cf60 {
    char pad0[0x1a1cc];
    int dirty;                                      // +0x1a1cc  view dirty, cleared by the renderer
    int sub_option;                                 // +0x1a1d0  selected map-edit sub-option (0..2)
};


struct Scene_s2cf60 {
    char pad0[0x6698];
    gui_manager_s2cf60 mesh_s2cf60;                               // +0x6698  (s0052f250 calls this object Map)
};


struct MapView_s2cf60 {
__forceinline void screen_to_world(vector4& p, vector4& q);         // 0x4bd2f0  picking ray (origin, target)
};

struct MapEditor_s2cf60 {
    char      pad0[0x60];
    MapView_s2cf60*  view;                                 // +0x60
    char      pad64[0x228 - 0x64];
    char      drag_state[0x268 - 0x228];            // +0x228  handed to 0x52e960 / 0x52edb0
    float     angle;                                // +0x268  handed to rotate_2x2

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

struct mat2_s2cf60 {                                       // 2x2 rotation matrix, 0x10 bytes
    float m[4];
void rotate_2x2(float angle);                       // 0x4f3b40 (thiscall, ret 4: the original passes the matrix in ecx; the definition in char_sheet/terrain_page_input.cpp declares it cdecl, so this one stays a view)
};

// ---- per-key input state ----------------------------------------------------------------------
// Three 0x400-byte tables of per-scan-code ints at 0xa17000 / 0xa17400 / 0xa17800: held state,
// this frame's press, and the previous frame's press (s0052f250 names 0xa17458/0xa17858
// (*(int*)((char*)&g_keyboard + 0x390))/(*(int*)((char*)&g_keyboard + 0x790))).  Only the codes read here are declared.
extern int g_a170dc;                                // 0xa170dc  held, code 0x37
extern int g_a170e0;                                // 0xa170e0  held, code 0x38
extern int g_a170e4;                                // 0xa170e4  held, code 0x39
extern char g_keyboard[]; // 0xa170c8
extern int g_a17128;                                // 0xa17128  held, code 0x4a
extern int g_a1715c;                                // 0xa1715c  held, code 0x57
extern int g_a1717c;                                // 0xa1717c  held, code 0x5f
extern int g_a17184;                                // 0xa17184  held, code 0x61
extern int g_a17188;                                // 0xa17188  held, code 0x62
extern int g_a17190;                                // 0xa17190  held, code 0x64
extern int g_a17194;                                // 0xa17194  held, code 0x65
extern int g_a171ac;                                // 0xa171ac  held, code 0x6b
extern int g_a171b4;                                // 0xa171b4  held, code 0x6d
extern int g_a173b4;                                // 0xa173b4  held, code 0xed
extern int g_a17420;                                // 0xa17420  press, code 0x08
extern int g_a174dc;                                // 0xa174dc  press, code 0x37
extern int g_a174e0;                                // 0xa174e0  press, code 0x38
extern int g_a174e4;                                // 0xa174e4  press, code 0x39
extern int g_a1758c;                                // 0xa1758c  press, code 0x63
extern int g_a17590;                                // 0xa17590  press, code 0x64
extern int g_a17820;                                // 0xa17820  previous, code 0x08
extern const float g_one;                           // 0x549cdc = 1.0f
extern const float g_hilite_dim;                    // 0x54b4d0 = 0.05f
extern const float g_rot_step;                      // 0x590c60 = 0.005f
extern const double g_percent;                      // 0x59c480 = 0.01

// ======================================================= slice s0052da20: classes, globals, helpers

// Mesh_s2da20 editing helpers: picking the vertex/edge/polygon nearest to a mouse ray.
// Class layout recovered from this run of functions (see symbols/slices/s0052da20.txt).

// A tiny dynamic array.  Layout verified here: the data pointers live at +4/+8 of the
// object, i.e. one dword precedes the first element pointer.
template<class T> struct vec_s2da20 {
    int  unk0;      // +0x0
    T*   first;     // +0x4
    T*   last;      // +0x8   (end of used range)
    T*   end;       // +0xc
    int size() const { return first ? (int)(last - first) : 0; }
    void clear();                       // 0x41f4b0
    void push_back(const T& v);         // 0x477c10 / 0x477d20 / 0x477e10
};

struct mface_s2da20 {                          // 0x18, vec_s2da20<mface_s2da20>::clear is 0x41f4b0
    rgl::vector4 pos;                   // 0x00
    unsigned char flag;                 // 0x10
    char pad[3];
    int index;                          // 0x14
};

struct medge_s2da20 {                          // 0x34
    int v0;                             // 0x00
    int v1;                             // 0x04
    char pad08[0x0c];
    unsigned char flag;                 // 0x14
    char pad15[0x34 - 0x15];
};

struct mpoly_s2da20 {                          // 0x64
    char pad00[0x28];
    int idx[4];                         // 0x28
    int idx2[4];                        // 0x38
    int count;                          // 0x48
    char pad4c[0x14];
    unsigned char flag;                 // 0x60
    char pad61[3];
};

struct MeshData_s2da20 {
    char pad0[0x18];
    vec_s2da20<mface_s2da20> verts;                   // 0x18
    vec_s2da20<mpoly_s2da20> polys;                   // 0x28
    vec_s2da20<medge_s2da20> edges;                   // 0x38
    vec_s2da20<mface_s2da20> verts2;                  // 0x48
    vec_s2da20<mpoly_s2da20> polys2;                  // 0x58
    vec_s2da20<medge_s2da20> edges2;                  // 0x68
    int mode;                           // 0x78
    int selected;                       // 0x7c
};

// The ray casters are declared only: at /O2 cl 13.10 summarises the body of a callee defined
// earlier or later in the same TU and then hoists the loop-invariant vector reads out of the
// picking loops below, which the original build (separate TU) could not do.  Calling through
// this opaque view keeps the loops byte-identical.
struct MeshImpl_s2da20 : MeshData_s2da20 {
    void vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);                          // 0x52dae0
    void edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);                          // 0x52dcc0
    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);                                 // 0x52dfd0
};

namespace rgl {
__forceinline void vec3_sub(vector4& out, const vector4& a, const vector4& b)   // 0x432830: out = a - b (xyz)
{ ::vec3_sub((Vec3*)&out, (const Vec3*)&a, (const Vec3*)&b); }
}

struct Mesh_s2da20 : MeshData_s2da20 {    unsigned pick_vert(const rgl::vector4& p, const rgl::vector4& d);
    unsigned pick_edge(const rgl::vector4& p, const rgl::vector4& d);
    unsigned pick_poly(const rgl::vector4& p, const rgl::vector4& d);
    void vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);
    void edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* a, float* b);
    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);
};

// ======================================================= slice s0052dfd0: classes, globals, helpers

// Mesh_s2dfd0 editing operations: ray/polygon picking and the selection transforms.
// Class layout recovered from this run of functions (see symbols/slices/s0052dfd0.txt).

// A tiny dynamic array.  Layout verified here: the data pointers live at +4/+8 of the
// object, i.e. one dword precedes the first element pointer.
template<class T> struct vec_s2dfd0 {
    int  unk0;      // +0x0
    T*   first;     // +0x4
    T*   last;      // +0x8   (end of used range)
    T*   end;       // +0xc
    int size() const { return first ? (int)(last - first) : 0; }
    void clear();                       // 0x41f4b0
    void push_back(const T& v);         // 0x477c10 / 0x477d20 / 0x477e10
};

struct mface_s2dfd0 {                          // 0x18
    rgl::vector4 pos;                   // 0x00
    unsigned char flag;                 // 0x10   (selection / visibility flag)
    char pad[3];
    int index;                          // 0x14   (selection stamp used by the transforms)
};

struct medge_s2dfd0 {                          // 0x34
    int v0;                             // 0x00
    int v1;                             // 0x04
    char pad08[0x0c];
    unsigned char flag;                 // 0x14
    char pad15[0x34 - 0x15];
};

struct mpoly_s2dfd0 {                          // 0x64
    char pad00[0x28];
    int idx[4];                         // 0x28   vertex indices (count <= 4)
    int idx2[4];                        // 0x38
    int count;                          // 0x48
    char pad4c[0x14];
    unsigned char flag;                 // 0x60
    char pad61[3];
};

// (rgl::vec3_sub: defined above)

struct Mesh_s2dfd0 {
    char pad0[0x18];
    vec_s2dfd0<mface_s2dfd0> verts;                   // 0x18
    vec_s2dfd0<mpoly_s2dfd0> polys;                   // 0x28
    vec_s2dfd0<medge_s2dfd0> edges;                   // 0x38
    vec_s2dfd0<mface_s2dfd0> verts2;                  // 0x48
    vec_s2dfd0<mpoly_s2dfd0> polys2;                  // 0x58
    vec_s2dfd0<medge_s2dfd0> edges2;                  // 0x68
    int mode;                           // 0x78   0 = vertex, 1 = edge, 2 = polygon
    int selected;                       // 0x7c

    void poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                  float* dist);
    void toggle_flag(int type, int index);   // 0x52e510
    void scale_selected(int dummy, const rgl::vector4* scale);
    void transform_selected(int dummy, const float* m);   // 0x52e6d0
    void remove_selected();   // 0x52e870

    __forceinline void calc_vertex_mode(int mode) { ((graph_obj*)this)->calc_vertex_mode(mode); }    // 0x4c54f0   re-stamps the +0x14 selection field
    __forceinline void remove_node(int index) { ((graph_obj*)this)->remove_node(index); }        // 0x4c8af0
    __forceinline void remove_edge(int index) { ((graph_obj*)this)->remove_edge(index); }        // 0x4ca9b0
    __forceinline void remove_face(int index) { ((graph_obj*)this)->remove_face(index); }        // 0x4cb940
    __forceinline void build_adjacency() { ((PolyMesh*)this)->build_adjacency(); }     // 0x470920
};

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// ======================================================= slice s0052e960: classes, globals, helpers

// Mesh_s2e960 editing: duplicate the selected polygons into the secondary buffers, projecting every
// copied vertex through a basis built from the caller's matrix.

// A tiny dynamic array.  Layout verified here: the data pointers live at +4/+8 of the
// object, i.e. one dword precedes the first element pointer.
template<class T> struct vec_s2e960 {
    int  unk0;      // +0x0
    T*   first;     // +0x4
    T*   last;      // +0x8   (end of used range)
    T*   end;       // +0xc
    int size() const { return first ? (int)(last - first) : 0; }
    __forceinline void clear() { ((vec<mface>*)this)->clear(); }                       // 0x41f4b0 (vec<mface>::clear, shared by every 4-byte-element vector)
    __forceinline void push_back(const T& v);         // 0x477c10 / 0x477d20 / 0x477e10 (std::vector<MVert / MPoly / MEdge>::push_back)
};

struct mface_s2e960 {                          // 0x18
    rgl::vector4 pos;                   // 0x00
    unsigned char flag;                 // 0x10
    char pad[3];
    int index;                          // 0x14   (source index this copy came from)
};

struct medge_s2e960 {                          // 0x34
    int v0;                             // 0x00
    int v1;                             // 0x04
    int unk08;                          // 0x08   (-10000, empty marker)
    int unk0c;                          // 0x0c   (-10000)
    int unk10;                          // 0x10
    unsigned char flag;                 // 0x14
    char pad15[3];
    float t0;                           // 0x18   (1.0f on a fresh edge)
    float t1;                           // 0x1c
    int index;                          // 0x20   (source edge index)
    char pad24[0x34 - 0x24];
};

struct mpoly_s2e960 {                          // 0x64
    int unk00;                          // 0x00
    char pad04[0x24];
    int idx[4];                         // 0x28   vertex indices (count <= 4)
    int idx2[4];                        // 0x38   edge indices
    int count;                          // 0x48
    char pad4c[0x10];
    int unk5c;                          // 0x5c
    unsigned char flag;                 // 0x60
    char pad61[3];
};

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
    return vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z + m.o.x,
                   m.s.y * p.x + m.f.y * p.y + m.u.y * p.z + m.o.y,
                   m.s.z * p.x + m.f.z * p.y + m.u.z * p.z + m.o.z);
}
}

struct Mesh_s2e960 {
    char pad0[0x18];    vec_s2e960<mface_s2e960> verts;                   // 0x18
    vec_s2e960<mpoly_s2e960> polys;                   // 0x28
    vec_s2e960<medge_s2e960> edges;                   // 0x38
    vec_s2e960<mface_s2e960> verts2;                  // 0x48   copy buffers
    vec_s2e960<mpoly_s2e960> polys2;                  // 0x58
    vec_s2e960<medge_s2e960> edges2;                  // 0x68
    int mode;                           // 0x78   0 = vertex, 1 = edge, 2 = polygon
    int selected;                       // 0x7c

    void make_copy(const rgl::matrix& m);
    int  vertex_index(int index, const rgl::matrix& m);
    int  edge_index(int index, const rgl::matrix& m);
    void apply_copy(const rgl::matrix& m);

    __forceinline void build_adjacency() { ((PolyMesh*)this)->build_adjacency(); }     // 0x470920
};

// Duplicate every flagged polygon into polys2, mapping its vertex/edge references through
// find-or-create helpers so shared vertices stay shared in the copy.  The basis rows are
// (s, f, up) with up = world Z: s = f x up normalised, f = up x s.
// ======================================================= slice s0052f250: classes, globals, helpers
#define g_map_s2f250 (*(Map_s2f250**)&(*(void**)((char*)&g_mission + 0x0)))
#define g_game_s2f250 (*(Frame__s2f250**)&g_game)
#define g_objrecs_s2f250 (*(ObjRec20_s2f250**)&g_ptypes)
#define g_editor_view_s2f250 (*(EditorView_s2f250**)&(*(void**)((char*)&g_windows + 0x38)))
#define g_editor_dialog_s2f250 (*(MapEditorDialog_s2f250*)g_app_obj)
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

// ---- 0x1c8-byte placed-object record (layout from s0044f3c0 / s00435490, vtable 0x590b84) --------
struct IfaceB_s2f250 {
    int f4, f8, fc;
    IfaceB_s2f250() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad_s2f250 {
    int a, b, c, d;
    Quad_s2f250() : a(2), c(((int*)g_ticks)[4]), d(((int*)g_ticks)[5]) {}
};
struct PlacedObj_s2f250 : IfaceB_s2f250 {
    int m10, m14;                               // +0x10
    matrix a18, a58, a98, ad8, a118;            // +0x18 .. +0x157
    vector4 v158;                               // +0x158
    int m168;                                   // +0x168
    float f16c;                                 // +0x16c
    Quad_s2f250 q170;                                  // +0x170
    int m180, m184, m188, m18c;                 // +0x180 (m188 = kind)
    tstring name;                               // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0;           // +0x1a0 (m1a0 = slot number)
    void* f1b4;                                 // +0x1b4  spawn handle, virtual +0x5c
    int m1b8, m1bc, m1c0, m1c4;                 // +0x1b8
    PlacedObj_s2f250() { reset(); }                    // 0x437830 (inlined here)
    __forceinline void reset() { ((PlacedObj*)this)->reset(); }     // 0x4352f0 (PlacedObj::reset)
    virtual void s_last() {}                    // placeholder: original table 0x590b84 has 3 slots, ours 2
};

// ---- the non-virtual slot at +0x5c of the object behind record->f1b4 ------------------------------
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
    __forceinline void _Insert_n(mesh_s2f250** pos, unsigned n, mesh_s2f250** val)      // 0x410610 (ICF'd vector<int>::_Insert_n)
    { ((int_stdvec*)this)->insert_n((int*)pos, n, *(int*)val); }
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
    char f140;                                  // +0x140  preview shown
};

// ---- map world (0x5e292c) and frame state (0x5e2930) ---------------------------------------------
struct Map_s2f250 {
    char pad0[0x65e4];
    SceneObj_s2f250* object;                           // +0x65e4  the edited scene object
    char pad65e8[0x668c - 0x65e8];
    PlacedObj_s2f250* records;                         // +0x668c  the 0x1c8 record array
    __forceinline void init_placed(PlacedObj_s2f250*, const int* kind, const int* id, const int* value, const matrix*);   // 0x435860
    __forceinline void build_mesh_name(PlacedObj_s2f250*, MeshVec_s2f250*);    // 0x5302f0
    __forceinline int add_placed(const int* kind, const int* id, const int* value, const matrix*, int* spawn);     // 0x44f870 (Mission::add_placed)
};
struct IntVec_s2f250 {
    int* first;                                 // +0
    int* last;                                  // +4
    int* end;                                   // +8
    // the vector starts 4 bytes before `first` (its allocator word is the field in front: Frame__s2f250::selected)
    __forceinline void resize(unsigned n, int v) { ((std::vector<int>*)((char*)this - 4))->resize(n, v); }     // 0x43f310
    __forceinline void push_back_out(const int& v) { ((std::vector<int>*)((char*)this - 4))->push_back(v); }  // 0x440b30
};
struct Frame__s2f250 {
    char pad0[0x1a09c];
    vector4 color;                              // +0x1a09c  (placement colour / eye)
    float f1a0ac;                               // +0x1a0ac  placement z offset
    char pad1a0b0[0x1a0b8 - 0x1a0b0];
    int mode;                                   // +0x1a0b8  kind being placed
    int id;                                     // +0x1a0bc  record id
    int prev_mode;                              // +0x1a0c0
    int prev_id;                                // +0x1a0c4
    char commit;                                // +0x1a0c8  force-place flag
    char pad1a0c9[3];
    int rand_rot;                               // +0x1a0cc
    int rand_scale;                             // +0x1a0d0
    char pad1a0d4[0x1a164 - 0x1a0d4];
    matrix cam;                                 // +0x1a164
    char pad1a1a4[0x1a1b4 - 0x1a1a4];
    int selected;                               // +0x1a1b4
    IntVec_s2f250 sel;                                 // +0x1a1b8
    char pad1a1c4[0x1a1cc - 0x1a1c4];
    int dirty;                                  // +0x1a1cc
};
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



extern int g_key_inc_prev;                      // 0xa17814
extern int g_key_dec_prev;                      // 0xa177f4
extern int g_cursor_x;                          // 0x5d70b4
extern int g_cursor_y;                          // 0x5d70b8
extern int g_saved_x;                           // 0x5e2130
extern int g_saved_y;                           // 0x5e2134
extern HWND g_editor_hwnd;                      // 0x8ee3d30



// ---- callees / helpers not local to this slice ---------------------------------------------------
void set_list_scroll(int dir);                      // 0x4bb260 (cdecl)
__forceinline matrix* identity(matrix* out) { return (matrix*)identity_pos((pos*)out); }                    // 0x40cd00 (cdecl, returns out)
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
struct MapEditor_s2f250 {
    char pad0[0x60];
    void* view;                                 // +0x60  3D view, used by 0x52bdb0
    char pad64[0x248 - 0x64];
    float dir_x, dir_y, dir_z;                  // +0x248  editor look direction
    char pad254[0x268 - 0x254];
    int f268;                                   // +0x268
    char pad26c[0x2f8 - 0x26c];
    char f2f8;                                  // +0x2f8  placement adjusted
    char pad2f9[0x304 - 0x2f9];
    vector4 f304;                               // +0x304  last picked position
    int f314;                                   // +0x314  down-repeat timer
    int f318;                                   // +0x318  up-repeat timer
    int f31c;                                   // +0x31c  selected variant
    int f320;                                   // +0x320  previous variant
    __forceinline void find_position(char* ok, vector4* pos) { ((Mission_s2b5c0*)this)->find_position(ok, pos); } // 0x52bdb0
    __forceinline void apply(char* a, vector4* off, char* b, mat3_s2f250* frame, char* c, vector4* col, char* out); // 0x52fe40 (ViewCtrl_s2fe40::apply)
    void update();                              // 0x52f250
};

// ======================================================= slice s0052fe40: classes, globals, helpers
#define g_items_s2fe40 (*(Item_s2fe40**)&g_items)
#define g_map_pts_s2fe40 (*(MapEntry_s2fe40**)&g_ptypes)
#define g_map_points_s2fe40 (*(MapPoint_s2fe40**)&g_propkinds)
#define g_resmgr_s2fe40 (*(resmgr_s2fe40*)&g_resmgr)
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
extern int   g_a1715c;                                  // 0xa1715c
extern int   g_a171b4;                                  // 0xa171b4
extern int   g_a173b4;                                  // 0xa173b4
extern int   g_a17120;                                  // 0xa17120
extern int   g_a17520;                                  // 0xa17520
extern int   g_a17188;                                  // 0xa17188
extern int   g_a17128;                                  // 0xa17128
extern int   g_a17184;                                  // 0xa17184
extern int   g_a1712c;                                  // 0xa1712c
extern int   g_a17194;                                  // 0xa17194
extern int   g_a17594;                                  // 0xa17594

extern float  g_k005;                                   // 0x590c60 (0.005f)
extern double g_d001;                                   // 0x59c480 (0.01)
extern const float g_one;                                    // 0x549cdc (1.0f)
extern float  g_k005b;                                  // 0x54b4d0 (0.05f)

// ---- game tables ---------------------------------------------------------------------------------------
namespace rgl {
struct mesh_s2fe40;
}

struct Item_s2fe40;
struct ItemRef_s2fe40 {
    int item;                                           // +0
    unsigned m4;                                        // +4
    __forceinline tstring mesh_name() { return ((ItemRef*)this)->mesh_name(); }     // 0x434b20 (returns tstring by value)
};

struct Item_s2fe40 {
    char pad0[0x20];
    tstring name;                                       // +0x20
    char pad30[0x1c8 - 0x30];
    int type_flags;                                     // +0x1c8 (low byte: item type)
    char pad1cc[0x238 - 0x1cc];
    __forceinline tstring mesh_name_for(int bit) { return ((ItemKind*)this)->mesh_name_for(bit); }   // 0x433bf0 (returns tstring by value)
    __forceinline rgl::mesh* mesh_for(int lo, unsigned hi, int b) { return (rgl::mesh*)((ItemKind*)this)->mesh_for(((unsigned __int64)hi << 32) | (unsigned)lo, b); }   // 0x433cb0 (the first two words are its 64-bit class mask)
};

extern int  g_item_kind_count;                               // 0x5e29d4
int find_item(const tstring* name);                    // 0x434ad0

struct MapName_s2fe40 { char pad[0x48]; };
struct MapEntry_s2fe40 {
    tstring name;                                       // +0x00
    char pad10[8];
    int count;                                          // +0x18
    MapName_s2fe40* names;                                     // +0x1c
};

extern int g_ptype_count;                                 // 0x5e29e8

struct MapPoint_s2fe40 {
    char pad0[0x10];
    unsigned flags;                                     // +0x10 (bit 14)
    char pad14[0x40 - 0x14];
    std::vector<rgl::mesh*> meshes;                     // +0x40
    char pad50[0x78 - 0x50];
};

#include "../game_globals.h"
int find_in_table78(const tstring* name);               // 0x4352a0
extern char g_cfg[]; // 0x5a9f98

// ---- resource manager (0x5d6fa0) -----------------------------------------------------------------------
struct resmgr_s2fe40 {
    __forceinline std::vector<rgl::mesh*> find_meshes(const tstring* name) { return ((resmgr*)this)->find_meshes((const rgl::string*)name); }   // 0x41b960
};


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

struct Mat2_s2fe40 {
    float m[4];
    void rotate_2x2(float angle);                           // 0x4f3b40 (thiscall, ret 4; see mat2_s2cf60)
};
struct vector2_s2fe40 {
    float x, y;
    vector2_s2fe40() {}
    vector2_s2fe40(float a, float b) : x(a), y(b) {}
};

// ---- the view object (`this`; +0x268 yaw, layout from s005284a0) --------------------------------------
struct ViewCtrl_s2fe40 {
    char pad0[0x268];
    float yaw;                                          // +0x268

    void apply(char* f1, float* pos, char* f2, float* out, char* f3, float* vec, char* f4);
};

// rotate one width-2 row of the view matrix (the original inlines this three times)
static void rotate_pair_s2fe40(float* p, float c, float s)
{
    float x = p[0], y = p[1];
    p[0] = c * x - s * y;
    p[1] = c * y + s * x;
}


// ---- forwarders: the views reach the functions other objects define through the classes that define them -------------------------------
inline void MapEditor_s2f250::apply(char* a, vector4* off, char* b, mat3_s2f250* frame, char* c, vector4* col, char* out)
{ ((ViewCtrl_s2fe40*)this)->apply(a, (float*)off, b, (float*)frame, c, (float*)col, out); }
__forceinline mesh_s2f250* mesh_s2f250::clone() { return (mesh_s2f250*)((mesh_x*)this)->clone(); }
__forceinline void mesh_s2f250::set_color(vector4 c) { ((mesh_x*)this)->set_color(c); }
__forceinline void Map_s2f250::init_placed(PlacedObj_s2f250* o, const int* kind, const int* id, const int* value, const matrix* m)
{ ::init_placed((PlacedObj*)o, kind, id, value, m); }
__forceinline void Map_s2f250::build_mesh_name(PlacedObj_s2f250* o, MeshVec_s2f250* v)
{ ::build_mesh_name((MeshOwner_s2fe40*)o, (std::vector<rgl::mesh*>*)v); }
__forceinline int Map_s2f250::add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn)
{ return ((Mission*)this)->add_placed(kind, id, value, m, spawn); }
__forceinline unsigned gui_manager_s2cf60::pick_vert(const vector4& p, const vector4& d) { return ((Mesh_s2da20*)this)->pick_vert(p, d); }
__forceinline unsigned gui_manager_s2cf60::pick_edge(const vector4& p, const vector4& d) { return ((Mesh_s2da20*)this)->pick_edge(p, d); }
__forceinline unsigned gui_manager_s2cf60::pick_poly(const vector4& p, const vector4& d) { return ((Mesh_s2da20*)this)->pick_poly(p, d); }
__forceinline void gui_manager_s2cf60::select_mode(int mode, char on) { ((graph_obj*)this)->select_mode(mode, on); }
__forceinline void gui_manager_s2cf60::toggle_flag(int mode, int index) { ((Mesh_s2dfd0*)this)->toggle_flag(mode, index); }
__forceinline void gui_manager_s2cf60::scale_selected(int mode, const vector4& c) { ((Mesh_s2dfd0*)this)->scale_selected(mode, &c); }
__forceinline void gui_manager_s2cf60::transform_selected(int mode, const float* m) { ((Mesh_s2dfd0*)this)->transform_selected(mode, m); }
__forceinline void gui_manager_s2cf60::remove_selected() { ((Mesh_s2dfd0*)this)->remove_selected(); }
__forceinline void gui_manager_s2cf60::make_copy(void* state) { ((Mesh_s2e960*)this)->make_copy(*(const matrix*)state); }
__forceinline void gui_manager_s2cf60::apply_copy(void* state) { ((Mesh_s2e960*)this)->apply_copy(*(const matrix*)state); }
__forceinline void gui_manager_s2cf60::update_terrain(int mode, const vector4& dir) { ((graph_obj*)this)->update_terrain(mode, &dir.x); }
__forceinline void gui_manager_s2cf60::build_spikes(rgl::mesh* m) { ((Shape*)this)->build_spikes(m); }
__forceinline void gui_manager_s2cf60::build_ribbons(rgl::mesh* m) { ((Shape*)this)->build_ribbons(m); }
__forceinline void gui_manager_s2cf60::polygon_to_mesh(rgl::mesh* m) { ((Shape*)this)->polygon_to_mesh(m); }
__forceinline void MapView_s2cf60::screen_to_world(vector4& p, vector4& q) { ((camera_ray*)this)->screen_to_world(&p, &q); }
__forceinline void EditorPanel_s2cf60::update_checks() { ((CreateGameOptionsPage*)this)->update_checks(); }
__forceinline void mat3_s2cf60::rotate_fu(const float& angle) { ((matrix*)this)->rotate_about_s(&angle); }
__forceinline void mat3_s2cf60::rotate_su(const float& angle) { ((matrix*)this)->rotate_about_f(&angle); }
__forceinline void mat3_s2cf60::rotate_sf(const float& angle) { ((matrix*)this)->rotate_about_u(&angle); }
template<> __forceinline void vec_s2e960<mface_s2e960>::push_back(const mface_s2e960& v) { ((std::vector<MVert>*)this)->push_back(*(const MVert*)&v); }
template<> __forceinline void vec_s2e960<mpoly_s2e960>::push_back(const mpoly_s2e960& v) { ((std::vector<MPoly>*)this)->push_back(*(const MPoly*)&v); }
template<> __forceinline void vec_s2e960<medge_s2e960>::push_back(const medge_s2e960& v) { ((std::vector<MEdge>*)this)->push_back(*(const MEdge*)&v); }

#endif // MISSION5_MESH_3_H
