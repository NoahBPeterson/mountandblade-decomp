// resmgr.h: what src/rgl/core/resmgr.cpp (0x419190..0x41e46f) defines or needs besides the shared headers
// (../stream.h, ../resource.h, ../resource_manager.h, ../particles/mesh_obj.h, ./resource_io.h): the resource
// manager's full layout, the skeleton animation record, the LOD group, the mesh members the shared mesh header
// lacks. Names provisional.
//
// The resource manager object at 0x5d6fa0 is `resmgr` (../resource.h): lookups and device maintenance are defined in
// ../resource_manager.cpp (0x417bb0..0x419190), registration, serialisation, loading and destruction in resmgr.cpp.
//
// Header gaps worked around locally (the shared headers are not ours to edit; see MIGRATION.md):
//   - rgl::mesh (../particles/mesh_obj.h) has no compute_bounds (0x419300), linearise_vertex_colors (0x419360),
//     compute_tangents (0x41ce80) or clone (0x41c6d0): `rgl::mesh` adds them. Its vertex60 names the first
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
#include "../../game/tstring.h"   // tstring: the class that defines the string members the resource manager calls (rgl::string is the same 16-byte object)
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../resource_manager.h"
#include "./resource_io.h"            // Bone, skeleton, stage_io, material_io, globals; includes ../particles/mesh_obj.h

// ---- resources ------------------------------------------------------------------------------------------------
// slots 2 and 3 of the resource root's vtable: the stream serialisers (material, skin; mesh/body are saved by
// free functions)
struct named_object_x : named_object {
    virtual void slot2();
    virtual void save(stream_base* s);
};

// skeleton model: resource.h's frame_set read through its real field types
struct frame_x : frame_set {
    int& nbones() { return f18; }
    Bone*& bones() { return *(Bone**)&f1c; }
};

// ---- meshes ------------------------------------------------------------------------------------------------
// (0x30-byte "tangent frame" views of rgl::vertex60: see the header comment)
#include "../mesh.h"                   // rgl::mesh (shared with src/game/terrain_a)

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

// load_pack (0x41e150) builds the file readers on its stack: the user-declared (empty) destructors of the in-memory stream
// (0x417c90) and of the text stream class 0x54a890 and the readers' constructors are inlined there (stream_inline.h).
#include "../stream_inline.h"

// (the resource manager class itself, `resmgr`, is ../resource.h's: one class, the lookups and the loader are both its members)

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
