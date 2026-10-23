//-----------------------------------------------------------------------------
// File: wind.cpp  (0x461200..0x461630)
//
// Desc: the tail of the dialog TU's neighbour: an agent hit-point initialiser, two std::copy / copy_backward
//       instantiations for 24-byte elements, a triangle-normal helper, signed colour packing, a clock-step
//       setter and the wind object. Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include "./common.h"
#include "./wind.h"
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

using rgl::vector4;

// @ 0x461200
void Agent::init_max_hp()
{
    if (hp > 0.0f)
        max_hp = hp;
    else
        max_hp = 100.0f;
}

// @ 0x461230
elem24* __cdecl copy_range(elem24* first, elem24* last, elem24* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}

// @ 0x461280
elem24* __cdecl copy_backward24(elem24* f, elem24* l, elem24* d) { while (f != l) *--d = *--l; return d; }

// @ 0x4612d0
static inline vector4 sub_i(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline void cross_i(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}
vector4 tri_normal(const vector4* p)
{
    vector4 a = sub_i(p[1], p[0]);
    vector4 b = sub_i(p[2], p[0]);
    vector4 n;
    cross_i(n, a, b);
    n.normalize();
    return n;
}

// @ 0x461370
// packs components in [-1,1] (x,y,z,w) into 0xWWXXYYZZ bytes: v * 128 + 128, clamped to 255
unsigned pack_signed_color(const float* c)
{
    unsigned a = (unsigned)(c[3] * 128.0f + 128.0);
    unsigned r = (unsigned)(c[0] * 128.0f + 128.0);
    unsigned g = (unsigned)(c[1] * 128.0f + 128.0);
    unsigned b = (unsigned)(c[2] * 128.0f + 128.0);
    unsigned ca = 255;
    if (a < 256) ca = a;
    unsigned cr = r < 256 ? r : 255;
    unsigned cg = g < 256 ? g : 255;
    unsigned cb = b < 256 ? b : 255;
    return (((ca << 8 | cr) << 8) | cg) << 8 | cb;
}

// @ 0x461420
void set_clock_step(int* clock, float step)
{
    g_ticks_dt[*clock] = (__int64)(step * 100000.0f);
    g_ticks[*clock] += g_ticks_dt[*clock];
    g_step_tbl[*clock] = step;
}

// @ 0x461480
wind* wind::init()
{
    vector4 z(0.0f, 0.0f, 0.0f);
    v4 = z; v14 = z; v24 = z; v34 = z;
    v44 = vector4(0.0f, 1.0f, 0.0f);
    f0 = 0;
    f54 = 0.0f; f58 = 0.0f; f5c = 1.0f; f60 = 0.0f; f64 = 0.0f; f68 = 0.0f; f6c = 0.0f;
    strength = 0.0f;
    noise_seed = 0.0f;
    phase = (unsigned)rand() % 0x3f9;
    return this;
}

// @ 0x461570
vector4 wind::sample()
{
    vector4 in;
    in.y = 0.0f; in.z = 0.0f;
    in.x = (float)(g_ticks[2] % 1000000000) * 1e-5f + (float)phase;
    vector4 n;
    noise_fractal(noise_seed, 0.6f, 3, 0, &in.x, &n.x);
    float k = n.x * strength * 15.0f + 1.0f;
    n.x = k * v4.x; n.y = k * v4.y; n.z = k * v4.z;
    return n;
}
