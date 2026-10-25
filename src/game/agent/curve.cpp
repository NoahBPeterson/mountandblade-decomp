//-----------------------------------------------------------------------------
// File: curve.cpp  (0x4338e0..0x433bf0)
//
// Desc: the animated parameter ("Curve") helpers: inverse of the easing shape, set the phase, reset / read /
//       set a sub-record's phase, and the string hash that follows them. Functions in ascending address order
//       (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./curve.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/quaternion.h"
#include "../../include/timer.h"

// @ 0x4338e0
float Curve::unshape(float y)
{
    if (shape_mode == 1) return y;
    if (shape_mode == 2) return (float)asin(y) * 0.63661975f;
    if (shape_mode == 5) return y * y;
    if (shape_mode == 4) return (float)sqrt(y);
    return 0.0f;
}
// @ 0x433930
void Curve::set_phase(float p)
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
void Curve::reset_rec(int i)
{
    if (f5c == 0) recs[i].m0 = 0;
    recs[i].m30 = 1.0f;
    Timer* t = &recs[i].tm;
    t->start = g_ticks[t->clock];
    recs[i].mac = 1;
    recs[i].m10 = 0;
    recs[i].v6c = vector4(0.0f, 0.0f, 0.0f);
    *recs[i].p68 = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
}
// @ 0x433ac0
float Curve::rec_shaped_phase(int i)
{
    Curve* r = &recs[i];
    return r->shape(r->phase());
}
// @ 0x433b20
void Curve::set_rec_phase(int i, float p)
{
    if (p < 0.0f) p = 0.0f;
    else if (p > 1.0f) p = 1.0f;
    Curve* r = &recs[i];
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
