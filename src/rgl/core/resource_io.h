// resource_io.h: what src/rgl/core/resource_io.cpp (0x415900..0x417010) defines or needs besides the shared
// headers (../stream.h, ../resource.h, ../mesh.h, ../resource_manager.h): token parsing, matrix and skeleton
// serialisers, the texture/shader/material records' helpers, render globals. Names provisional.
//
// Header gaps worked around locally (the shared headers are not ours to edit):
//   - resource.h's `stage_t` has no load/save: `stage_io` derives from it and adds them (same thiscall).
//   - resource.h's `material` has no create_shaders (0x416c00): `material_io` derives from it.
//   - resource_manager.h's `rgl_config` has no fields: rgl_config::save (0x415fb0) indexes `this` as ints.
#ifndef RGL_CORE_RESOURCE_IO_H
#define RGL_CORE_RESOURCE_IO_H
#include <stdio.h>
#include <d3d9.h>
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../particles/mesh_obj.h"
#include "../resource_manager.h"

// atoi-like parser for a token (at most 15 characters, a leading '-' is the sign)
int mb_parse_int(const char* s);                                       // 0x4159a0

// the 3x3 rotation (rows s, f, u: 9 floats), and the whole frame (rotation, then the origin's 3 floats)
stream_base& w_rotation(stream_base& s, rgl::matrix* m);               // 0x415df0
stream_base& r_rotation(stream_base& s, rgl::matrix* m);               // 0x415e70
stream_base& w_matrix(stream_base& s, rgl::matrix* m);                 // 0x415ef0
stream_base& r_matrix(stream_base& s, rgl::matrix* m);                 // 0x415f30

// hash of an int through the permutation table g_noise_perm (noise.h)
unsigned noise_hash(int x);                                            // 0x415f70

// One bone of a skeleton model (0xdc bytes; ctor 0x41b0c0, dtor 0x41a510, vector deleting dtor 0x41a550):
// w_skeleton 0x416490 and read_frame_set 0x41b180 give the first 0x9c bytes, the ctor the rest.
struct Bone {
    int f0;                        // +0
    int parent;                    // +4  (< 0: root)
    unsigned flags;                // +8
    rgl::matrix local;             // +0xc
    rgl::matrix world;             // +0x4c
    rgl::string name;              // +0x8c
    float f9c;                     // +0x9c (2.0 in the ctor)
    int fa0, fa4, fa8;             // +0xa0
    rgl::vector4 va_c;             // +0xac
    rgl::vector4 vb_c;             // +0xbc
    std::vector<int> link;         // +0xcc (first at +0xd0)
    Bone();                        // 0x41b0c0
    // the real destructor 0x41a510 frees the link vector and destroys the name (implicit)
};
// A skeleton model as w_skeleton sees it (resource.h's frame_set: name +4, bone count +0x18, bones +0x1c).
struct skeleton {
    int vptr;
    rgl::string name;              // +4
    int pad14;
    int nbones;                    // +0x18
    Bone* bones;                   // +0x1c
};
stream_base& w_skeleton(stream_base& s, skeleton* k);                  // 0x416490

// resource.h's stage_t / material plus the members it lacks
struct stage_io : stage_t {
    __declspec(noinline) void load(stream_base* s);    // 0x416ad0
    __declspec(noinline) void save(stream_base* s);    // 0x416bc0
};
struct material_io : material {
    void create_shaders();                              // 0x416c00
};

// globals of the render configuration (g_rgl_config at 0x5a9f98 is the int array they live in)
extern int g_texture_detail;                           // 0x5a9f9c "texture_detail" (rgl_config +4)
extern int g_use_vertex_shaders;                       // 0x5a9fcc "use_vertex_shaders" (rgl_config +0x34)
extern int g_use_pixel_shaders;                        // 0x5a9fd0 "use_pixel_shaders" (rgl_config +0x38)
extern D3DCAPS9 g_caps;                                // 0xa17f2c

#endif
