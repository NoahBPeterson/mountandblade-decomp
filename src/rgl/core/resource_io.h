// resource_io.h: what src/rgl/core/resource_io.cpp (0x415900..0x417010) defines or needs besides the shared
// headers (../stream.h, ../resource.h, ../mesh.h, ../resource_manager.h): token parsing, matrix and skeleton
// serialisers, the texture/shader/material records' helpers, render globals. Names provisional.
//
// Header gaps worked around locally (the shared headers are not ours to edit):
//   - resource.h's `stage_t` has no load/save: `stage_io` derives from it and adds them (same thiscall).
//   - the option block writer 0x415fb0 is Config::save (game/config/config_block.h).
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
#include "../../include/d3d_caps.h"

// atoi-like parser for a token (at most 15 characters, a leading '-' is the sign)
int mb_parse_int(const char* s);                                       // 0x4159a0

// the 3x3 rotation (rows s, f, u: 9 floats), and the whole frame (rotation, then the origin's 3 floats)
stream_base& w_rotation(stream_base& s, rgl::matrix* m);               // 0x415df0
stream_base& r_rotation(stream_base& s, rgl::matrix* m);               // 0x415e70
stream_base& w_matrix(stream_base& s, rgl::matrix* m);                 // 0x415ef0
stream_base& r_matrix(stream_base& s, rgl::matrix* m);                 // 0x415f30

// hash of an int through the permutation table g_noise_perm (noise.h)
unsigned noise_hash(int x);                                            // 0x415f70

#include "./bone.h"                    // Bone (one bone of a skeleton model) and its CapDef capsules
// a skeleton model (resource.h's frame_set: name, bone count, bones)
stream_base& w_skeleton(stream_base& s, frame_set* k);                 // 0x416490

// resource.h's stage_t / material plus the members it lacks
struct stage_io : stage_t {
    __declspec(noinline) void load(stream_base* s);    // 0x416ad0
    __declspec(noinline) void save(stream_base* s);    // 0x416bc0
};

// globals of the render configuration (g_rgl_config at 0x5a9f98 is the int array they live in)

#endif
