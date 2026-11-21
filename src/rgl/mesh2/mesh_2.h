// mesh_2.h: classes of mesh_2.cpp (0x4ca600..0x4ce720, TU map 104 "mesh_2"): the screen-mode queue, the graph
// editor's edge/face operations (PolyMesh, game/terrain_b/poly_mesh.h), the big editor records, the std::vector instantiations the linker placed
// in this object, the skeleton fix-up, the language tables' constructor and loader, the csv row and the scene
// object readers. Merged from the slices s004ca550 (0x4ca600..0x4ca9b0), s004caa40, s004cb3a0, s004cbcd0, s004cc680,
// s004cd010, s004cd850 and s004cdfd0 (up to 0x4ce680). Names are provisional unless a vtable maps them.
#ifndef MESH2_MESH_2_H
#define MESH2_MESH_2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <new>
#include <vector>
#include <memory>
#include <stdexcept>
#include "../rgl_math.h"
#include "../stream.h"
#include "../../game/tstring.h"
#include "../../include/rag_model.h"
#include "../../game/strat/gui_manager.h"
using rgl::mesh;                 // the loaded-mesh class: rgl/mesh.h (gui_manager.h includes it)
#include "./mesh2_common.h"
#include "../../game/terrain_b/poly_mesh.h"   // PolyMesh: the map editor's polygon mesh, whose members 0x4c53f0..0x4cb940 this module defines
#include "./screen_cfg.h"

using rgl::vector4;
using rgl::matrix;

#include "../../include/crt.h"   // canonical: operator delete[] (0x53570d)

// ============================================================================================================
// 0x4cb610..0x4cbc80: the big editor record's releasers and vector helpers
// ============================================================================================================
// 0x4cb6b0 is std::vector<ArrSet>::_Tidy (from the explicit instantiation in mesh_2.cpp; its _Destroy_range call is
// the out-of-line 0x4727a0, terrain_b)

// a list node that only counts elements
struct list_node {
    int* next;                                   // +0x00
    int* prev;                                   // +0x04
    int  count;                                  // +0x08
    void grow(unsigned n);                       // 0x4cb6f0
};


#include "../../include/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0 (out of line: vector<int>'s members here call it)
// 0x4cbb90 / 0x4cbbe0 / 0x4cbc30 / 0x4cbc80 are the _Buy members of std::vector<RagdollBone> / <ArrSet> / <Constraint> /
// <FaceTexture> (the explicit instantiations in mesh_2.cpp)

// ============================================================================================================
// 0x4cbcd0..0x4cd850: std::vector instantiations and the element types of this object
// ============================================================================================================
struct E44  { int v[11]; ~E44() {} }; // 0x2c-byte record (empty, user-declared dtor: the original's
                                      // _Destroy loop survives, the call inlines away)
#include "../../include/vec_elems.h"   // FaceTexture (shared with game/cls_map)


#include "../../include/e68.h"           // E68, scalar4 (shared with game/ui/mission.h)

// _Nonscalar_ptr_iterator_tag: an empty tag that cl 13.10 still passes as one 4-byte stack slot
// (which is where the last `mov eax, [esp+8]` in 0x4cc260 comes from).
struct nonscalar_tag {};
// 0x4ca7e0: std::_Destroy_range for the 0xb0-byte record FaceTexture (frees +0x20, then destroys the string at +0), defined in this object


#include "../core/bone.h"   // RagdollBone (the ragdoll bone record, ctor 0x4cd7a0), Bone / CapDef (fix_skeletons 0x4cd850)

// ============================================================================================================
// 0x4cd850..0x4cdf20: skeleton fix-up and the csv row (the translation tables are LangTables, mesh2_common.h)
// ============================================================================================================
// the skeleton fix_skeletons looks up is resource.h's frame_set (get_object70 0x418cf0: nbones +0x18, bones +0x1c, core/bone.h's Bone)
struct named_object;
namespace rgl { struct mesh; }
#include "../resource.h"   // resmgr (global at 0x5d6fa0): the one class; find_meshes 0x41b960 (hidden return pointer first, ret 8)
namespace std { template<> vector<rgl::mesh*, allocator<rgl::mesh*> >& vector<rgl::mesh*, allocator<rgl::mesh*> >::operator=(const vector<rgl::mesh*, allocator<rgl::mesh*> >&); }   // 0x43f0a0 (the out-of-line copy-assignment; the linker folded every pointer vector's onto vector<int>'s)

// the 0x18-byte alternative-text record: its implicit ctor only builds the leading tstring
struct alt_rec {
    tstring s;                                                  // +0x00
    int a;                                                      // +0x10
    int b;                                                      // +0x14
};
// the tstring vector a csv row carries: 0xc bytes (three raw pointers, no leading pad -- the original's
// ctor writes exactly these three dwords and its clear() frees the buffer and re-zeroes them)
struct str_vec {
    tstring* first;                                             // +0x00
    tstring* last;                                              // +0x04
    tstring* end;                                               // +0x08
    str_vec() : first(0), last(0), end(0) { }
    void clear() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
// the item kind record (item_kinds.txt): include/item_kind.h's ItemKind, whose constructor this is (the names are ItemKind's)
struct csv_row {                                                // 0x238 bytes (0x4cdf20)
    tstring name;                                               // +0x00
    tstring plural;                                             // +0x10
    tstring key;                                                // +0x20 the item id
    char pad30[0x4];
    str_vec slots;                                              // +0x34 (ItemKind::slots' three pointers)
    float f40;                                                  // +0x40 (1.0f)
    int mesh_count;                                             // +0x44 (0)
    alt_rec meshes[16];                                         // +0x48 the mesh variants (ItemMesh: name, 64-bit flags)
    int type_flags;                                             // +0x1c8 (0)
    char pad1cc[0x1d8 - 0x1cc];
    int value;                                                  // +0x1d8 (10) the price
    char pad1dc[0x4];
    int f1e0;                                                   // +0x1e0 (0) modifier usage bits
    int f1e4;                                                   // +0x1e4 (0)
    char pad1e8[0x21c - 0x1e8];
    float price_rate;                                           // +0x21c (1.0f)
    int abundance;                                              // +0x220 (100)
    float merchandise_probability;                              // +0x224 (1.0f)
    str_vec sound;                                              // +0x228 (ItemKind::sound, the trigger list)
    char pad234[0x4];
    csv_row();                                                  // 0x4cdf20
};

extern SkeletonBody g_skeletons;                      // 0xa180e8 (the registered skeletons, filled from skeleton_bodies.xml)
#include "../../game/alloc_construct.h"   // alloc_tstring 0x436c90
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, alloc_tstring& al, ptr_tag tag);   // 0x438f00 (agent_stats.cpp)
// std::vector<tstring>::_Insert_n (0x43e310, protected): reached through a class that makes it public
struct tstring_vec_pub : std::vector<tstring> { using std::vector<tstring>::_Insert_n; };
void __fastcall fix_skeletons(std::vector<RagModel>* list);               // 0x4cd850


// ============================================================================================================
// 0x4cdfd0..0x4ce680: the scene object tables
// ============================================================================================================
// (`mesh` is the opaque loaded-mesh class; gui_manager.h declares a sized stand-in)
#include "../resource.h"                                    // body_part (the body resource: shape at +0x18, copy_shape 0x439100), shape_base
inline void mesh_node_bounds(mesh* node, vector4* mn, vector4* mx) { node->calc_bounds(mn, mx); }   // 0x438d80
void vmin(vector4* r, const vector4* a, const vector4* b);  // 0x40af80
void vmax(vector4* r, const vector4* a, const vector4* b);  // 0x40b080

// 0x4cdfd0 fills a heap block: a header dword (count) followed by `count` records of 0x48 bytes.
struct shape_entry {                                            // 0x48 bytes
    tstring name;                                               // +0x00
    std::vector<rgl::mesh*> nodes;                              // +0x10 (std::vector: allocator word, first, last, end)
    body_part* shape;                                           // +0x20 (0 when the name is "0")
    shape_base* body;                                           // +0x24 (shape->copy_shape())
    vector4 mn;                                                 // +0x28 (1e7f, merged with 0x438d80 bounds)
    vector4 mx;                                                 // +0x38 (-1e7f, merged with 0x438d80 bounds)
    shape_entry() : shape(0), body(0) {}
};

struct ObjectList {                                             // a flora kind (flora_kinds.txt, g_ptypes; options_panel/module_load.cpp reads it)
    tstring name;                                               // +0x00
    unsigned __int64 flags;                                     // +0x10 (read as one 64-bit word)
    int count;                                                  // +0x18
    shape_entry* entries;                                       // +0x1c (points past the block header)
    void read(stream_base* s);                                  // 0x4cdfd0 (ret 4)
};

#include "../../include/mission_obj.h"      // MissionObj: one scene prop kind (ctor 0x4ce2a0, read 0x4ce360 below)

// FaceTexture (0xb0, src/include/vec_elems.h): its default constructor 0x4ce680 is defined in mesh_2.cpp.




#endif
