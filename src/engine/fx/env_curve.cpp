//-----------------------------------------------------------------------------
// File: env_curve.cpp  (0x4338e0..0x433bf0)
//
// Desc: the animated parameter (EnvRec, engine/fx/fx.h) helpers: inverse of the easing shape, set the phase, reset / read /
//       set a sub-record's phase, and the string hash that follows them. Functions in ascending address order
//       (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "engine/fx/env_curve.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/math/quaternion.h"
#include "engine/core/timer.h"

// EnvRec::shape (0x4232b0, out of line in fx_key_frames.cpp) as this object has it: expanded inline (0x433ac0)
inline float EnvRec::eased(float x)
{
    if (shape_mode == 1) return x;
    if (shape_mode == 2) return (float)sin(x * 1.5707964f);
    if (shape_mode == 5) return (float)sqrt(x);
    if (shape_mode == 4) return x * x;
    return 0.0f;
}
// @ 0x4338e0
float EnvRec::unshape(float y)
{
    if (shape_mode == 1) return y;
    if (shape_mode == 2) return (float)asin(y) * 0.63661975f;
    if (shape_mode == 5) return y * y;
    if (shape_mode == 4) return (float)sqrt(y);
    return 0.0f;
}
// @ 0x433930
void EnvRec::set_phase(float p)
{
    float ph = phase();
    float v;
    if (shape_mode == 1) v = ph;
    else if (shape_mode == 2) v = (float)sin(ph * 1.5707964f);
    else if (shape_mode == 5) v = (float)sqrt(ph);
    else if (shape_mode == 4) v = ph * ph;
    else v = 0.0f;
    period = p;
    int ticks = (int)(unshape(v) * p * 100000.0f);
    tm.start = g_ticks[tm.clock] - ticks;
}
// @ 0x4339d0
void FxDef::reset_rec(int i)
{
    if (keep_weight == 0) env[i].weight = 0;
    env[i].target = 1.0f;
    Timer* t = &env[i].tm;
    t->start = g_ticks[t->clock];
    env[i].flag = 1;
    env[i].flip_mask = 0;
    env[i].add_pos = vector4(0.0f, 0.0f, 0.0f);
    env[i].ref[0] = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}
// @ 0x433ac0
float FxDef::rec_shaped_phase(int i)
{
    EnvRec* r = &env[i];
    return r->eased(r->phase());
}
// @ 0x433b20
void FxDef::set_rec_phase(int i, float p)
{
    if (p < 0.0f) p = 0.0f;
    else if (p > 1.0f) p = 1.0f;
    EnvRec* r = &env[i];
    int ticks = (int)(r->unshape(p) * r->period * 100000.0f);
    r->tm.start = g_ticks[r->tm.clock] - ticks;
}
// @ 0x433bb0
unsigned __stdcall hash_string(const tstring* s)
{
    unsigned h = 0;
    for (unsigned i = 0; i < (unsigned)s->len; i++) h = (s->buf[i] + h * 59) % 3571;
    return h;
}
