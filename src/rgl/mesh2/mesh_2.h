// mesh_2.h: classes of mesh_2.cpp (0x4ca600..0x4ce720, TU map 104 "mesh_2"): the screen-mode queue, the graph
// editor's edge/face operations (graph_obj.h), the big editor records, the std::vector instantiations the linker placed
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
#include "./graph_obj.h"
#include "./screen_cfg.h"

using rgl::vector4;
using rgl::matrix;

#include "../../include/crt.h"   // canonical: operator delete[] (0x53570d)

// ============================================================================================================
// 0x4cb610..0x4cbc80: the big editor record's releasers and vector helpers
// ============================================================================================================
// the engine's virtual "destroy the contents then free" record (0x475d50)
struct res_rec {
    virtual void free_contents() = 0;            // +0x00
    virtual void v04() = 0; virtual void v08() = 0; virtual void v0c() = 0;
    virtual void v10() = 0; virtual void v14() = 0; virtual void v18() = 0; virtual void v1c() = 0;
    virtual void v20() = 0; virtual void v24() = 0; virtual void v28() = 0; virtual void v2c() = 0;
    virtual void v30() = 0; virtual void v34() = 0; virtual void v38() = 0; virtual void v3c() = 0;
    virtual void v40() = 0; virtual void v44() = 0; virtual void v48() = 0; virtual void release() = 0;  // +0x4c
    // the direct (non-virtual) teardown entry at 0x475d50: thiscall, `this` in ecx
    void remove();                               // 0x475d50
};

// the lib's 4-arg _Destroy_range (0x4727a0) takes (first, last, allocator&, tag): the tag is an empty
// class passed by value, so the call materialises a local `ptr_tag` before the branch.
#include "../../include/ptr_tag.h"
// The definition (game/terrain_b: destroy_arrsets) is cdecl with two parameters; the callers push the allocator and the tag as well
// (the caller pops), so it is called through the original's four-parameter type.
struct ArrSet;
void __cdecl destroy_arrsets(ArrSet* first, ArrSet* last); // 0x4727a0
typedef void (__cdecl *obj_free_t)(ArrSet*, ArrSet*, void*, ptr_tag);

struct vec3_holder {
    void clear();                                // 0x4cb6b0
    char pad0[4];
    void* begin;                                 // +0x04
    void* end;                                   // +0x08
    void* cap;                                   // +0x0c
};

// a list node that only counts elements
struct list_node {
    int* next;                                   // +0x00
    int* prev;                                   // +0x04
    int  count;                                  // +0x08
    void grow(unsigned n);                       // 0x4cb6f0
};


#include "../../include/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0
struct tvec {
    char pad0[4];
    tstring* begin;                              // +0x04
    tstring* end;                                // +0x08
    tstring* cap;                                // +0x0c
    unsigned size() const { return begin ? (unsigned)(end - begin) : 0; }
    bool reserve_50(unsigned n);                 // 0x4cbb90
    bool reserve_38(unsigned n);                 // 0x4cbbe0
    bool reserve_204(unsigned n);                // 0x4cbc30
    bool reserve_b0(unsigned n);                 // 0x4cbc80
    // the lib's _Xlen (0x43e2a0): never returns. The original loads ecx = this before the call, while the definition (agent_combat.cpp
    // vec_xlen) is a free cdecl function without parameters (?vec_xlen@@YAXXZ): the call goes to that symbol through the fastcall type
    // that the original's `mov ecx, this; call` stands for
    __forceinline void _Xlen() const { VEC_XLEN(this); }                    // 0x43e2a0
};

// ============================================================================================================
// 0x4cbcd0..0x4cd850: std::vector instantiations and the element types of this object
// ============================================================================================================
struct E44  { int v[11]; ~E44() {} }; // 0x2c-byte record (empty, user-declared dtor: the original's
                                      // _Destroy loop survives, the call inlines away)
#include "../../include/vec_elems.h"   // R44, b16, E50, FaceTexture (shared with game/cls_map)


#include "../../include/e68.h"           // E68, scalar4 (shared with game/ui/mission.h)

// _Nonscalar_ptr_iterator_tag: an empty tag that cl 13.10 still passes as one 4-byte stack slot
// (which is where the last `mov eax, [esp+8]` in 0x4cc260 comes from).
struct nonscalar_tag {};
// 0x4ca7e0: std::_Destroy_range for the 0xb0-byte record FaceTexture (frees +0x20, then destroys the string at +0), defined in this object


struct vec4f {
    float x, y, z, w;
    vec4f() {}
    vec4f(float a, float b, float c) : x(a), y(b), z(c) {}
};

// the ragdoll bone of skeleton_bodies.xml (E50, 0x50 bytes): the field names are the attributes RagBone::load 0x4d3330
// (options_panel/ragdoll_models.cpp) reads into them
struct Obj7a0 {
    tstring name;                           // 0x00
    float bone_mass;                        // 0x10 (2.0)
    float cone_min;                         // 0x14 (-0.01)
    float cone_max;                         // 0x18 (1.0)
    float twist_limit;                      // 0x1c (1.0)
    vec4f rotate;                           // 0x20 rotate_side, rotate_forward, rotate_up
    vec4f socket_dir;                       // 0x30
    int bodies_alloc;                       // 0x40 the bodies vector's allocator word (uninitialised)
    void* bodies_first;                     // 0x44 the bodies (RagBody, 0x2c bytes)
    void* bodies_last;                      // 0x48
    void* bodies_end;                       // 0x4c
    Obj7a0();                               // 0x4cd7a0
};

// ============================================================================================================
// 0x4cd850..0x4cdf20: skeleton fix-up and the csv row (the translation tables are LangTables, mesh2_common.h)
// ============================================================================================================
// the skeleton fix_skeletons looks up is resource.h's frame_set (get_object70 0x418cf0: nbones +0x18, bones +0x1c)
struct SkelBone {                                               // the resource bone (core/resource_io.h's Bone, 0xdc bytes) with its link vector as the source's R44 records
    int f0;                                                     // +0
    int parent;                                                 // +4
    unsigned flags;                                             // +8
    matrix local;                                               // +0xc
    matrix world;                                               // +0x4c
    tstring name;                                               // +0x8c
    int bone_mass;                                              // +0x9c (source +0x10)
    int cone_min;                                               // +0xa0 (source +0x14)
    int cone_max;                                               // +0xa4 (source +0x18)
    int twist_limit;                                            // +0xa8 (source +0x1c)
    b16 rotate;                                                 // +0xac
    b16 socket_dir;                                             // +0xbc
    std::vector<R44> bodies;                                    // +0xcc (source +0x40)
};
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

#include "../../game/trigger_vecs.h"   // GameVec: the trigger list (resize 0x4c1e10, stamp 0x4c1ee0)
struct game_vec : GameVec {                                     // the 12-byte trigger list at MissionObj +0x6c, zeroed by its constructor
    void clear();
    game_vec() { count = 0; data = 0; cur = 0; }
};

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

// One scene prop (scene_props.txt, 0x78 bytes; ctor 0x4ce2a0). include/rec78.h's Rec78 is the same record.
struct MissionObj {
    tstring name;                                               // +0x00
    int flags;                                                  // +0x10 (Rec78::flags)
    int f14;                                                    // +0x14
    vector4 v18;                                                // +0x18 (0,0,0)
    vector4 frame;                                              // +0x28 (1,1,1; .w = 1.0f)
    int f38;                                                    // +0x38
    int f3c;                                                    // +0x3c
    std::vector<rgl::mesh*> meshes;                             // +0x40 (the meshes named by the prop's mesh prefix)
    body_part* shape;                                           // +0x50
    shape_base* body;                                           // +0x54 (deleted by the record's destructor)
    vector4 center;                                             // +0x58 (0,0,0; the centre of the bounds in read())
    float radius;                                               // +0x68 (0.1f)
    game_vec triggers;                                          // +0x6c ("Mission object %s")
    MissionObj();                                               // 0x4ce2a0
    void read(stream_base* s);                                  // 0x4ce360 (ret 4)
};

// FaceTexture (0xb0, src/include/vec_elems.h): its default constructor 0x4ce680 is defined in mesh_2.cpp.




#endif
