// channel.h: one animation channel (0xd8 bytes; Skeleton::chans, +0x40 of the skeleton), ONE definition for
// src/game/agent2 (agent_ai.cpp 0x43b100.., agent_stats.cpp 0x439fc0 start_anim / 0x43a980 start_channel: the blend, bone-mask
// and rate fields) and src/game/agent3 (agent_combat.cpp 0x4406a0: the charge value read through chans->charge()).
// Same record as `Curve` of src/game/agent/curve.h (0x433840 / 0x433930 / 0x4339d0 / 0x433ac0 / 0x433b20).
#ifndef GAME_CHANNEL_H
#define GAME_CHANNEL_H
#include "../include/rgl_base.h"
struct Anim;
struct Channel {                                        // 0xd8 bytes
    char pad0[8];
    unsigned flags;                                     // +8 (bone mask set)
    int fc;                                             // +0xc
    unsigned mask_lo, mask_hi;                          // +0x10 / +0x14 (bone mask bits)
    char pad18[0x38 - 0x18];
    float blend;                                        // +0x38 (0x43a980)
    int f3c;                                            // +0x3c
    char pad40[0x44 - 0x40];
    int k44, k48, k4c, k50;                             // +0x44 (0x439fc0: 0x33, 0x33, a, b)
    char pad54[8];
    Anim* anim;                                         // +0x5c
    rgl::vector4* pa;                                   // +0x60 (one point per track)
    rgl::vector4* pb;                                   // +0x64
    float* xform;                                       // +0x68 (quaternion blocks at +0, +0x70, +0x80, ...)
    float v6c[4];                                       // +0x6c
    rgl::vector4 va, vb;                                // +0x7c, +0x8c
    char pad9c[0xa0 - 0x9c];
    float period;                                       // +0xa0
    char pada4[4];
    int mode;                                           // +0xa8
    char padac[0xc0 - 0xac];
    float w0, w1;                                       // +0xc0 / +0xc4
    char padc8[0xcc - 0xc8];
    int rate;                                           // +0xcc
    char padd0[0xd8 - 0xd0];
    bool busy();                                        // 0x433840 (Curve::step_reached)
    void set_phase(float period);                       // 0x433930
    // agent_combat.cpp 0x4406a0 (Agent::charged_0x48, inlined) compares the float at +0xd8 of the first channel with 0.1
    // (the first dword past this record). Read through a pointer so that the record keeps its 0xd8-byte stride.
    float charge() const { return *(const float*)((const char*)this + 0xd8); }
};
#endif // GAME_CHANNEL_H
