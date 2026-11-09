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
#include "../shared_vars.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


#include "../../include/clock_table.h"

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
static inline void cross_i(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}
vector4 tri_normal(const vector4* p)
{
    vector4 a = (p[1] - p[0]);
    vector4 b = (p[2] - p[0]);
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
    g_tick_dt[*clock] = (__int64)(step * 100000.0f);
    g_ticks[*clock] += g_tick_dt[*clock];
    g_step_tbl[*clock] = step;
}



// @ 0x461480
Light::Light() throw()
{
    vector4 z(0.0f, 0.0f, 0.0f);
    color.x = z.x; color.y = z.y; color.z = z.z; color.w = z.w;
    base_color.x = z.x; base_color.y = z.y; base_color.z = z.z; base_color.w = z.w;
    pos.x = z.x; pos.y = z.y; pos.z = z.z; pos.w = z.w;
    v34.x = z.x; v34.y = z.y; v34.z = z.z; v34.w = z.w;
    type = 0;
    dir = vector4(0.0f, 1.0f, 0.0f);
    range = 0.0f; f58 = 0.0f; f5c = 1.0f; f60 = 0.0f; f64 = 0.0f; f68 = 0.0f; flags = 0;
    flicker_magnitude = 0.0f;
    flicker_interval = 0.0f;
    phase = (unsigned)rand() % 0x3f9;
}

// @ 0x461570
vector4 Light::sample()
{
    vector4 in;
    in.y = 0.0f; in.z = 0.0f;
    in.x = (float)(g_ticks[2] % 1000000000) * 1e-5f + (float)phase;
    vector4 n;
    noise_fractal(flicker_interval, 0.6f, 3, 0, &in.x, &n.x);
    float k = n.x * flicker_magnitude * 15.0f + 1.0f;
    n.x = k * color.x; n.y = k * color.y; n.z = k * color.z;
    return n;
}
