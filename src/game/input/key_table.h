// key_table.h: key_array, the 12-byte key-frame table: n keys (ints) and n 16-byte values (positions, or quaternions
// stored w x y z and serialised y z w x). ONE definition for src/rgl (engine/resource/resource.h: skel_anim's tracks and tail,
// key_clip; the readers / writers of mem_stream.cpp, operator= 0x417850; find 0x423a50 in engine/fx/fx_key_frames.cpp) and
// src/game (game/agent/agent_queries.cpp: sample 0x436ae0). The slices called it KeyTable / Elem12 / vec_array.
#ifndef MB_KEY_TABLE_H
#define MB_KEY_TABLE_H
struct quaternion;                             // engine/math/quaternion.h (sample returns one by value; its users include it)
namespace rgl { struct vector4; }

struct key_array {
    union {
        rgl::vector4* data;                    // +0 position keys (skel_anim::tail)
        quaternion* rots;                      // +0 rotation keys (skel_anim::keys, one table per bone)
    };
    int* keys;                                 // +4
    int n;                                     // +8
    void operator=(const key_array& o);        // 0x417850
    __declspec(noinline) void find(int* cur, float* frac, float* key);         // 0x423a50
    quaternion sample(int key);                                                 // 0x436ae0 (hidden return pointer)
};

#endif // MB_KEY_TABLE_H
