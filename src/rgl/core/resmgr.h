// resmgr.h: what src/rgl/core/resmgr.cpp (0x419190..0x41e46f) defines or needs besides the shared headers
// (../stream.h, ../resource.h, ../resource_manager.h, ../particles/mesh_obj.h, ./resource_io.h): the resource
// manager's full layout, the skeleton animation record, the LOD group, the mesh members the shared mesh header
// lacks, and the string members the shared string header lacks. Names provisional.
//
// The resource manager object at 0x5d6fa0 is one object seen through two classes: `resource_manager`
// (../resource.h, 0x417bb0..0x419190: lookups and device maintenance, a partial layout) and `resmgr` (here:
// the full layout; registration, serialisation, loading, destruction). `resmgr` calls the first class's
// members through base() and never redefines them.
//
// Header gaps worked around locally (the shared headers are not ours to edit; see MIGRATION.md):
//   - rgl::string (../rgl_string.h) has no copy constructor (0x42c110) and no range_equals/match (0x42c160):
//     `str_x` adds them.
//   - rgl::mesh (../particles/mesh_obj.h) has no compute_bounds (0x419300), linearise_vertex_colors (0x419360),
//     compute_tangents (0x41ce80) or clone (0x41c6d0): `mesh_x` adds them. Its vertex60 names the first
//     vector `pos` and the one at +0x30 `normal`; the stream carries the normal at +0 (the working normal,
//     tangent frame input) and a copy at +0x30, so the tangent builder reads pos/f10/f20 as normal/tan/bit.
//   - named_object (../resource.h) has no slots 2 and 3 (the serialisers `load`/`save`): `named_object_x`.
//   - resource.h's key_array has no constructor/destructor (the original owns both arrays): `vec_array`.
//   - resource.h's frame_set stores the bone count and array as ints (f18, f1c): read through frame_x.
#ifndef RGL_CORE_RESMGR_H
#define RGL_CORE_RESMGR_H
#include <stdio.h>
#include <vector>
#include <list>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../resource_manager.h"
#include "./resource_io.h"            // Bone, skeleton, stage_io, material_io, globals; includes ../particles/mesh_obj.h

// ---- string members the shared header lacks ----------------------------------------------------------------
struct str_x : rgl::string {
    str_x() {}
    str_x(const rgl::string& o);                                   // 0x42c110 (copy constructor)
    // this[*from .. min(*to, len)) == o (0x42c160; returns a byte)
    bool range_equals(int* from, int* to, const rgl::string* o) const;
};

// ---- resources ------------------------------------------------------------------------------------------------
// slots 2 and 3 of the resource root's vtable: the stream serialisers (material, skin; mesh/body are saved by
// free functions)
struct named_object_x : named_object {
    virtual void slot2();
    virtual void save(stream_base* s);
};

// 12-byte key array that owns both arrays (resource.h's key_array, plus constructor and destructor; the
// constructor stores n first, as skel_anim's inlined copy 0x41b4e0 does)
struct vec_array : key_array {
    vec_array() { n = 0; data = 0; keys = 0; }
    ~vec_array()                                                        // 0x4196a0 (the body of allocator::destroy)
    {
        if (data)
            delete[] data;
        if (keys)
            delete[] keys;
    }
};

// skeleton model: resource.h's frame_set read through its real field types
struct frame_x : frame_set {
    int& nbones() { return f18; }
    Bone*& bones() { return *(Bone**)&f1c; }
};

// vtable 0x54b24c, 0x34 bytes, type() 8: skeleton animation (ctor 0x41b4e0, dtor 0x41b520, deleting 0x41bed0)
struct skel_anim : named_object {
    std::vector<vec_array> keys;               // +0x18 (first at +0x1c)
    vec_array tail;                            // +0x28
    skel_anim();                               // 0x41b4e0
    virtual int type();                        // 0x48b4a0 (8)
};

// ---- meshes ------------------------------------------------------------------------------------------------
// (0x30-byte "tangent frame" views of rgl::vertex60: see the header comment)
#include "../mesh_x.h"                   // mesh_x (shared with src/game/terrain_a)

// 16-byte list of owned meshes (resource_manager's LOD groups own their meshes)
struct mesh_owner : std::vector<rgl::mesh*> {
    __declspec(noinline) ~mesh_owner();        // 0x419420
    void forget();                             // 0x419470 (null the entries without deleting)
};

// LOD group (0xe0 bytes, ctor 0x41c2a0): five owned mesh lists. ../particles/mesh_obj.h's lod_mesh is the same
// object without the destructor-owning lists, the constructor and the name.
struct lod_group {
    rgl::aabb bounds;                          // +0 (ctor 0x40eaf0)
    mesh_owner lists[5];                       // +0x34
    int count;                                 // +0x84
    int cur;                                   // +0x88
    rgl::matrix m8c;                           // +0x8c
    unsigned mask;                             // +0xcc
    rgl::string name;                          // +0xd0
    lod_group();                               // 0x41c2a0
};

// Resource-file readers. The original's file reader classes, as load_pack (0x41e150) builds them on its stack:
// the destructor of the in-memory stream (0x417c90) and of the text stream class 0x54a890 are user-declared
// (empty) and inlined there, which leaves their vtable stores in the temporaries' destructors. ./stream.h's
// classes have these destructors out of line (or not at all), so the two reader chains are repeated here.
// vtable 0x54a890: the text stream class with a user-declared destructor (stream.h: text_stream2)
struct text_reader_b : stream_base {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    STREAM_SLOTS
    __forceinline text_reader_b() {}
    ~text_reader_b() {}
};
struct text_reader_base : text_reader_b {
    __forceinline text_reader_base() { failed = 1; fp = 0; state = 0; }
};
// vtable 0x54ac88: the text resource reader (stream.h: tstream3)
struct text_reader : text_reader_base {
    __forceinline text_reader() {}
    STREAM_SLOTS
};
// vtable 0x54ad68: in-memory stream with the inline destructor body of 0x417c90
struct mem_stream_inl : stream_base {
    STREAM_SLOTS
    __forceinline mem_stream_inl() {}
    ~mem_stream_inl() {}
};
struct bin_reader_b : mem_stream_inl {
    FILE* fp;                                  // +0x30
    char failed;                               // +0x34
    __forceinline bin_reader_b() { failed = 1; fp = 0; state = 0; }
};
// vtable 0x54ae48: binary file reader (stream.h: bin_reader; ctor 0x417c50 inline here)
struct file_reader : bin_reader_b {
    __forceinline file_reader() {}
    STREAM_SLOTS
};

// ---- the resource manager (global object at 0x5d6fa0) -----------------------------------------------------------------
// Full layout, from the constructor 0x41b000, add 0x41ba90, save 0x419a30 and the destructor 0x419e90.
struct resmgr {
    char f0;                                   // +0x00
    unsigned c4, c8, cc;                       // +0x04 id counters for textures, shaders, materials (1..1000)
    std::vector<lod_group*> lods;              // +0x10 LOD groups (make_lod_mesh 0x41ca60)
    std::vector<rgl::mesh*> meshes;            // +0x20 type 1
    std::vector<texture*> textures;            // +0x30 type 2
    std::vector<material*> shaders;            // +0x40 type 4
    std::vector<skin*> materials;              // +0x50 type 3
    std::vector<body_part*> bodies;            // +0x60 type 6
    std::vector<frame_set*> skeletons;         // +0x70 type 7
    std::vector<skel_anim*> anims;             // +0x80 type 8
    std::vector<effect_set*> effects;          // +0x90 (technique sets of the shader files)
    std::list<named_object*> blocks;           // +0xa0 the array blocks to delete[] (one per loaded record group)
    std::vector<named_object*> all;            // +0xac every registered resource
    rgl::string cur;                           // +0xbc resource root
    char padcc[2];                             // +0xcc
    char meshes_ready;                         // +0xce (set once by linearise_meshes)

    resmgr();                                  // 0x41b000
    ~resmgr();                                 // 0x419e90
    resource_manager& base() { return *(resource_manager*)this; }
    rgl::mesh* get_object(const rgl::string* name);                    // 0x419540
    int restore(ID3DXEffect* fx);                                      // 0x419580 (device reset: textures, effects)
    int invalidate();                                                  // 0x4195e0 (device lost)
    void linearise_meshes();                                           // 0x419640
    void save(stream_base* s);                                         // 0x419a30
    void add(named_object* r);                                         // 0x41ba90
    texture* get_render_target(int w, int h, int fmt, int ext, int f34);   // 0x41b7d0
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);    // 0x41b960
    effect_set* get_small(str_x name);                                 // 0x41bf40
    void load_shaders();                                               // 0x41c000
    texture* get_texture(int w, int h, int fmt);                       // 0x41c400
    skin* get_shadow_skin(int w, int h, int fmt, int f34);             // 0x41c420
    int load_all(int check_exists, rgl::string* root);                 // 0x41c540
    lod_group* make_lod_mesh(const rgl::string& pat, bool copy);       // 0x41ca60
    lod_group* get_lod_mesh(const rgl::string& name);                  // 0x41cdc0
    stream_base* load_stream(stream_base* s);                          // 0x41db20
    void load_pack(const rgl::string& name, int user);                 // 0x41e150
};
extern resmgr g_resmgr_full;                                           // 0x5d6fa0 (g_resmgr of resource.h)

// ---- serialisers (cdecl) -------------------------------------------------------------------------------------
stream_base* save_elem14(stream_base* s, rgl::bone_influence* e);      // 0x419190
stream_base* save_frame(stream_base* s, rgl::frame_t* f);              // 0x419200
stream_base* save_keyframes(stream_base* s, rgl::keyframes* k);        // 0x4194c0
stream_base* save_mesh(stream_base* s, rgl::mesh* m);                  // 0x419760
stream_base* read_frame_set(stream_base* s, frame_x* m);               // 0x41b180
stream_base* read_body(stream_base* s, body_part* b);                  // 0x41b5a0
stream_base* read_skel_anim(stream_base* s, skel_anim* a);             // 0x41c060
stream_base* read_bone_influence(stream_base* s, rgl::bone_influence* b);   // 0x41c110
stream_base* read_vec_pair(stream_base* s, rgl::frame_t* p);           // 0x41c180
stream_base* read_keyframes(stream_base* s, rgl::keyframes* k);        // 0x41c340 (morph key frames)
stream_base* read_mesh(stream_base* s, rgl::mesh* m);                  // 0x41d6f0

#endif
