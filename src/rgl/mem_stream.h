// mem_stream.h: what src/rgl/mem_stream.cpp (0x417010..0x417baf) defines besides the classes of stream.h,
// resource.h and mesh.h: the key-array serialisers and two small helpers. Names provisional.
#ifndef RGL_MEM_STREAM_H
#define RGL_MEM_STREAM_H
#include "./stream.h"
#include "./resource.h"

// writes the body part's name, its shape kind ("sphere", "capsule", "face", "composite", "manifold") and the shape
stream_base& w_body_part(stream_base& s, body_part* b);                // 0x417200
// key arrays: `n`, then per key `int key; y z w x` (quaternions) or `int key; x y z` (positions)
stream_base& w_key_array_yzwx(stream_base& s, key_array* a);           // 0x417330
stream_base& r_key_array_yzwx(stream_base& s, key_array* a);           // 0x4173b0 (allocates keys/data)
stream_base& w_key_array_xyz(stream_base& s, key_array* a);            // 0x4174a0
stream_base& r_key_array_xyz(stream_base& s, key_array* a);            // 0x417520
void __stdcall clear_f14(named_object* r);                             // 0x4175f0  // old name of Mgr::release, removed after the names merge

#endif
