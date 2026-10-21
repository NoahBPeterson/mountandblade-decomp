// Slice s004333c0: collision-face helpers, composite copy ctor, a message ring buffer, a height field sampler,
// scene-object placement setters and the animated-parameter (Curve) helpers around 0x4338e0..0x433bf0, plus an
// item-kind mesh-name lookup. Names provisional.
#include "s004333c0.h"

// @ 0x4333c0
bool face::contains_xy(const float* p) const
{
    bool inside = true;
    int i = 0;
    const vector4* a = v;
    do {
        if (i >= count) return inside;
        i++;
        int j = i % count;
        float e = v[j].x - a->x, f = v[j].y - a->y, g = p[0] - a->x, h = p[1] - a->y;
        if (h * e - g * f < 0.0f)
            inside = false;
        a++;
    } while (inside);
    return inside;
}
// @ 0x433440
float face::plane_z(const float* p) const
{
    return v[0].z - ((p[1] - v[0].y) * normal.y + (p[0] - v[0].x) * normal.x) / normal.z;
}
// @ 0x433470
composite::composite(const composite& o) { ((shape_base*)this)->copy_from(&o); }
// @ 0x4334b0
void MsgLog::push(const tstring& s, int v)
{
    if (enabled && ring.count < 32) {
        int idx = (ring.head + ring.count) % 32;
        ring.names[idx] = s;
        ring.vals[idx] = v;
        ring.count++;
    }
}
static inline int ifloor(float f) { return (int)(f > 0.0f ? f : f - 0.99999988f); }
// @ 0x433510
void HeightField::sample(float x, float y, float* out_h, int* out_id)
{
    float inv = 1.0f / cell;
    float t[2];
    t[0] = x * inv;
    t[1] = y * inv;
    int ix = ifloor(t[0]);
    if (ix < 0) ix = 0;
    else { int r = w - 1; if (ix < w) r = ix; ix = r; }
    int iy = ifloor(t[1]);
    if (iy < 0) iy = 0;
    else { int r = h - 1; if (iy < h) r = iy; iy = r; }
    float fu = (float)fmod((double)t[0], 1.0);
    float fv = (float)fmod((double)t[1], 1.0);
    HCell** pc = &cols[ix];
    *out_id = pc[0][iy].id;
    HCell* a = &pc[0][iy];
    if (a->flags & 2) {
        if (fu > fv) {
            *out_h = (a->h - pc[1][iy].h) * (1.0f - fu) + (pc[1][iy + 1].h - pc[1][iy].h) * fv + pc[1][iy].h;
            return;
        }
        *out_h = (pc[0][iy].h - pc[0][iy + 1].h) * (1.0f - fv) + (pc[1][iy + 1].h - pc[0][iy + 1].h) * fu + pc[0][iy + 1].h;
    } else {
        if (1.0f - fu > fv) {
            *out_h = (pc[1][iy].h - pc[0][iy].h) * fu + (pc[0][iy + 1].h - pc[0][iy].h) * fv + pc[0][iy].h;
            return;
        }
        *out_h = (pc[1][iy].h - pc[1][iy + 1].h) * (1.0f - fv) + (a[1].h - pc[1][iy + 1].h) * (1.0f - fu) + pc[1][iy + 1].h;
    }
    if (pc[0][iy].flags & 1) *out_id = pc[0][iy].id2;
}
// @ 0x4336d0
bool in_box(const vector4* lo, const vector4* hi, const vector4* p)
{
    return lo->x <= p->x && p->x <= hi->x && lo->y <= p->y && p->y <= hi->y && lo->z <= p->z && p->z <= hi->z;
}
// @ 0x433740
void Obj3::set_v184(const vector4* v) { v184 = *v; }
// @ 0x433770
void Obj3::set_placement(const matrix* m, const vector4* a, const vector4* b, int c)
{
    unsigned f = flags & ~1u;
    flags = f;
    m134 = *m;
    v174 = *a;
    v184 = *b;
    m194 = c;
    if (f & 2) {
        m19c = m1a0 = 1000000000;
        m1a4 = 0;
    }
}
// @ 0x433810
void Obj3::reset_pending(int v)
{
    flags &= ~1u;
    m19c = m1a0 = v;
    m1a4 = 0;
}
// @ 0x433840
bool Curve::step_reached()
{
    bool r;
    float t = (float)(g_time[tm.clock] - tm.start) * 1e-5f;
    if (mode == 20) {
        float inv = 1.0f / period;
        int a = (int)((t - g_step_tbl[tbl_idx]) * inv);
        int b = (int)(t * inv);
        if (a < b) return true;
        return false;
    }
    if (mode != 21 && mode != 22) return r;
    if (t >= period) return true;
    return false;
}
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
    tm.start = g_time[tm.clock] - ticks;
}
// @ 0x4339d0
void Curve::reset_rec(int i)
{
    if (f5c == 0) recs[i].m0 = 0;
    recs[i].m30 = 1.0f;
    Timer* t = &recs[i].tm;
    t->start = g_time[t->clock];
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
    r->tm.start = g_time[r->tm.clock] - ticks;
}
// @ 0x433bb0
unsigned __stdcall hash_string(const tstring* s)
{
    unsigned h = 0;
    for (unsigned i = 0; i < (unsigned)s->len; i++) h = (s->buf[i] + h * 59) % 3571;
    return h;
}
// @ 0x433bf0
tstring ItemKind::mesh_name_for(int bit)
{
    int found = -1;
    unsigned __int64 mask = 1i64 << bit;
    int n = mesh_count;
    for (int i = 0; i < n; i++) {
        if ((meshes[i].flags & 0xF000000000000000ui64) == 0) {
            if (found == -1) found = i;
            if (meshes[i].flags & mask) found = i;
        }
    }
    if (found >= 0) return meshes[found].name;
    return name;
}
