// Shared declarations of slice s004362e0 (provisional names; evidence in comments).
#ifndef S004362E0_H
#define S004362E0_H
#include "rgl.h"
#include <math.h>
using rgl::vector4;

// item kind record (0x238 bytes, array at 0x5e29d0), see s004333c0 / s00434ad0
struct ItemKind {
    char pad0[0x1c8];
    unsigned type;                      // +0x1c8 (low byte = kind)
    char pad1cc[4];
    unsigned __int64 flags;             // +0x1d0 (64-bit item flags)
    char pad1d8[0x238 - 0x1d8];
};
extern ItemKind* g_items;               // 0x5e29d0

// item with modifier byte (s00434ad0 ItemRef): item index + amount/modifier word
struct Pair {
    int a;                              // item index
    unsigned b;                         // +4: amount (low byte), modifier (byte 3)
    int max_amount() const;             // 0x434b70
    int stat_1e8() const;               // 0x434df0
};

struct Agent {
    char pad0[0x2c];
    int f2c;                            // +0x2c
    char pad30[0x1d0 - 0x30];
    Pair pairs[11];                     // +0x1d0
    int pad228;
    int slots[5];                       // +0x22c (slots[1] at +0x230, slots[4] at +0x23c)
    Pair slot_pair(int idx) const;      // 0x432790 (s00432130)
    inline Pair slot_pair_inl(int idx) const;   // same body, inlined by the compiler at some call sites
    void pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant);   // 0x4362e0
    void weapon_info(int idx, unsigned* skill, int* stat);                                   // 0x436810
    Pair best_ammo(int idx, int* missing, int* total, int* slot_out);                        // 0x436880
    int total_missing_ammo();                                                                // 0x436a70
};

inline Pair Agent::slot_pair_inl(int idx) const
{
    const Pair* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
    Pair r;
    r.a = -1;
    if (p) {
        r.a = p->a;
        r.b = p->b;
    }
    return r;
}

struct vec2 {
    float x, y;
    float normalize();                  // 0x4367c0 (returns the old length, like vector4::normalize)
};

// quaternion, memory order w,x,y,z (s00423540)
struct quaternion { float w, x, y, z; };
void slerp(quaternion* out, const quaternion* a, const quaternion* b, const float* t);      // 0x4238c0

struct KeyTable {
    quaternion* vals; int* keys; int count;
    void find(int* cur, float* frac, float* key);               // 0x423a50
    quaternion sample(int key);                                 // 0x436ae0 (struct returned through a hidden pointer)
};

struct KeyFrames {
    vector4* vals; int* keys; int count;
    void find(int* a, int* b, float* t, const float* time);     // 0x40f0d0
};
struct KeyHolder {
    char pad0[0x28];
    KeyFrames kf;                       // +0x28
    vector4 sample(int key);            // 0x436b90 (struct returned through a hidden pointer)
};
#endif
