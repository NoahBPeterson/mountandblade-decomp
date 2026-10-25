// curve.h: the animated-parameter record (Curve, 0xd8 bytes) of curve.cpp (0x4338e0..0x433bf0) and the string hash
// next to it. Every offset below is read by a function of this module (cited); the rest is padding.
// Curve::phase (0x4231f0) lives in an earlier object (s00422db0) and is only declared here; Curve::step_reached
// (0x433840) is at the end of the previous object in the TU map (see MIGRATION.md) and is declared for completeness.
#ifndef AGENT_CURVE_H
#define AGENT_CURVE_H
#include <math.h>
#include "rgl.h"
#include "../tstring.h"

using rgl::vector4;

#include "../../include/quaternion.h"
#include "../../include/timer.h"

#include "../../include/fx.h"
extern float g_step_tbl[];      // 0x5a9c20 (step_reached)

// The canonical class of this record: AnimRec (src/rgl/dynamic/fx_anim.h), whose AnimRec::phase is the function at 0x4231f0. That header
// cannot be included next to rgl.h (it brings the src/rgl family of rgl_math.h), so only the member is declared, under its own name.
struct AnimRec {
    float phase();   // 0x4231f0
};

struct Curve {
    void* m0;                   // +0x00 (0x4339d0 clears it for records of a curve without f5c)
    char pad4[0x10 - 4];
    __int64 m10;                // +0x10 (0x4339d0)
    char pad18[0x1c - 0x18];
    int cur;                    // +0x1c
    Timer tm;                   // +0x20 clock id, +0x28 start tick (0x433930, 0x4339d0)
    float m30;                  // +0x30 (0x4339d0 sets 1.0)
    char pad34[0x40 - 0x34];
    Curve* recs;                // +0x40 array of 0xd8-byte records (0x4339d0, 0x433ac0, 0x433b20)
    char pad44[0x5c - 0x44];
    int f5c;                    // +0x5c (0x4339d0)
    char pad60[0x68 - 0x60];
    quaternion* p68;            // +0x68 orientation (0x4339d0)
    vector4 v6c;                // +0x6c (0x4339d0)
    char pad7c[0xa0 - 0x7c];
    float period;               // +0xa0 (0x433930, 0x433b20)
    char pada4[4];
    int mode;                   // +0xa8 (step_reached)
    int mac;                    // +0xac (0x4339d0)
    char padb0[4];
    int shape_mode;             // +0xb4 (0x4338e0, 0x433930; shape(): 1 linear, 2 sine, 5 sqrt, 4 square)
    char padb8[0xd0 - 0xb8];
    int tbl_idx;                // +0xd0 (step_reached)
    char padd4[0xd8 - 0xd4];

    // Curve is the 0xd8-byte AnimRec seen from the other side (cur +0x1c, clock +0x20, start +0x28, period +0xa0, shape_mode +0xb4
    // are the same fields as in fx_anim.h), so phase() is AnimRec::phase.
    float phase() { return ((AnimRec*)this)->phase(); }
    float shape(float x)                                    // inlined by 0x433930 / 0x433ac0
    {
        if (shape_mode == 1) return x;
        if (shape_mode == 2) return (float)sin(x * 1.5707964f);
        if (shape_mode == 5) return (float)sqrt(x);
        if (shape_mode == 4) return x * x;
        return 0.0f;
    }
    bool busy();                                    // 0x433840 (previous object)
    float unshape(float y);                                 // 0x4338e0
    void set_phase(float period);                           // 0x433930
    void reset_rec(int i);                                  // 0x4339d0
    float rec_shaped_phase(int i);                          // 0x433ac0
    void set_rec_phase(int i, float p);                     // 0x433b20
};

unsigned __stdcall hash_string(const tstring* s);           // 0x433bb0 (h = (c + h * 59) % 3571)

#endif // AGENT_CURVE_H
