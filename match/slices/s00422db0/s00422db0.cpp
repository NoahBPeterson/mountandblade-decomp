// Slice s00422db0: a quaternion type (memory order w,x,y,z), then the key-frame curve evaluator of a
// scene effect definition (records of 0xd8 bytes at +0x40). Names are provisional.
#include <math.h>
#include "rgl.h"

struct quaternion {
    float w, x, y, z;
    quaternion() {}
    quaternion(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
    quaternion& set(const float* x_, const float* y_, const float* z_, const float* w_);   // 0x422db0
    void add(const quaternion& o);                                                         // 0x422de0
    quaternion operator+(const quaternion& o) const;                                       // 0x422e10
    void normalize();                                                                      // 0x422e40
    quaternion mul(const quaternion& o) const;                                             // 0x422eb0
    quaternion mul_conj(const quaternion& o) const;                                        // 0x422f60
    float dot(const quaternion& o) const;                                                  // 0x423010
    bool dot_negative(const quaternion& o) const;                                          // 0x423040
};
void quat_to_matrix(rgl::matrix* m, const quaternion* q);                                  // 0x423080
void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q);                   // 0x423160

// @ 0x422db0
quaternion& quaternion::set(const float* x_, const float* y_, const float* z_, const float* w_)
{
    x = *x_; y = *y_; z = *z_; w = *w_;
    return *this;
}
// @ 0x422de0
void quaternion::add(const quaternion& o)
{
    x += o.x; y += o.y; z += o.z; w += o.w;
}
// @ 0x422e10
quaternion quaternion::operator+(const quaternion& o) const
{
    return quaternion(o.x + x, o.y + y, o.z + z, o.w + w);
}
// @ 0x422e40
void quaternion::normalize()
{
    float len = (float)sqrt(w * w + x * x + y * y + z * z);
    if (len <= 1e-7f) {
        x = 1.0f; y = 0.0f; z = 0.0f; w = 0.0f;
    } else {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv; w *= inv;
    }
}
// @ 0x422eb0
quaternion quaternion::mul(const quaternion& o) const
{
    quaternion r;
    r.x = ((y * o.z - o.y * z) + o.w * x) + o.x * w;
    r.y = ((o.x * z - o.z * x) + y * o.w) + o.y * w;
    r.z = ((o.y * x - o.x * y) + o.w * z) + w * o.z;
    r.w = o.w * w - ((o.x * x + o.y * y) + o.z * z);
    return r;
}
// @ 0x422f60
quaternion quaternion::mul_conj(const quaternion& o) const
{
    quaternion r;
    r.x = ((z * o.y - o.z * y) + w * o.x) - x * o.w;
    r.y = ((o.z * x - z * o.x) + w * o.y) - o.w * y;
    r.z = ((o.x * y - x * o.y) + o.z * w) - z * o.w;
    r.w = ((x * o.x + o.z * z) + o.y * y) + o.w * w;
    return r;
}
// @ 0x423010
float quaternion::dot(const quaternion& o) const
{
    return o.z * z + o.y * y + o.x * x + o.w * w;
}
// @ 0x423040
bool quaternion::dot_negative(const quaternion& o) const
{
    return o.z * z + o.y * y + o.x * x + o.w * w < 0.0f;
}
// @ 0x423080
void quat_to_matrix(rgl::matrix* m, const quaternion* q)
{
    float x2 = q->x + q->x;
    float y2 = q->y + q->y;
    float z2 = q->z + q->z;
    float xx = x2 * q->x;
    float xy = y2 * q->x;
    float xz = z2 * q->x;
    float yy = y2 * q->y;
    float yz = z2 * q->y;
    float zz = z2 * q->z;
    float wx = x2 * q->w;
    float wy = y2 * q->w;
    float wz = z2 * q->w;
    m->s.x = 1.0f - (zz + yy);
    m->s.y = xy + wz;
    m->s.z = xz - wy;
    m->f.x = xy - wz;
    m->f.y = 1.0f - (zz + xx);
    m->f.z = wx + yz;
    m->u.x = wy + xz;
    m->u.y = yz - wx;
    m->u.z = 1.0f - (yy + xx);
}
// @ 0x423160
void quat_to_axis_angle(rgl::vector4* axis, float* angle, const quaternion* q)
{
    float w = q->w;
    if (w > 0.99999988f) {
        axis->x = 1.0f; axis->y = 0.0f; axis->z = 0.0f;
        *angle = 0.0f;
        return;
    }
    float s = (float)sqrt(1.0f - w * w);
    if (s < 1e-4f) s = 1.0f;
    float inv = 1.0f / s;
    axis->x = inv * q->x;
    axis->y = inv * q->y;
    axis->z = inv * q->z;
    *angle = (float)acos(w) * 2;
}

// Key-frame curve of an effect definition (provisional layout).
struct KeyTable {
    rgl::vector4* vals; int* keys; int count;     // values, key array (ints), key count
    void find(int* cur, float* frac, float* key);                                          // 0x423a50
};
struct KeyObj { char pad0[0x28]; KeyTable tbl; };
struct CurveRec {                       // 0xd8 bytes
    char pad0[0x18];
    float f18;
    char pad1[0x4c - 0x1c];
    int key_begin, key_end;             // +0x4c, +0x50
    char pad2[0x5c - 0x54];
    KeyObj* table;                      // +0x5c
    char pad3[0x7c - 0x60];
    rgl::vector4 v7c;                   // +0x7c
    rgl::vector4 v8c;                   // +0x8c
    int type;                           // +0x9c
    char pad4[0xd8 - 0xa0];
};
extern __int64 g_time[];                // 0x5e2948
struct Curve {
    char pad0[0x1c];
    int cur;                            // +0x1c
    int clock;                          // +0x20
    char pad1[4];
    __int64 start;                      // +0x28
    char pad2[0x40 - 0x30];
    CurveRec* recs;                     // +0x40
    char pad3[0xa0 - 0x44];
    float period;                       // +0xa0
    char pad4[4];
    int mode;                           // +0xa8
    int flag;                           // +0xac
    char pad5[4];
    int shape_mode;                     // +0xb4
    float phase();                                                                         // 0x4231f0
    float shape(float x);                                                                  // 0x4232b0
    rgl::vector4 key_value(int i);                                                         // 0x423300
    rgl::vector4 lerp_value(int i);                                                        // 0x423390
    rgl::vector4 table_value(int i);                                                       // 0x423440
};
// @ 0x4231f0
float Curve::phase()
{
    float t = (float)(g_time[clock] - start) * 1e-5f;
    if (mode == 20)
        return (float)fmod(t, period) / period;
    if (mode == 21) {
        if (!(t < period))
            return period / period;
    } else if (mode == 22) {
        if (!(t < period)) {
            flag = 0;
            return period / period;
        }
    }
    return t / period;
}
// @ 0x4232b0
float Curve::shape(float x)
{
    if (shape_mode == 1) return x;
    if (shape_mode == 2) return (float)sin(x * 1.5707964f);
    if (shape_mode == 5) return (float)sqrt(x);
    if (shape_mode == 4) return x * x;
    return 0.0f;
}

// local stand-ins for the inline vector4 operators (the effect code inlines them; rgl.h declares them out of line)
static inline rgl::vector4 vmul(const rgl::vector4& a, const float& s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
static inline rgl::vector4 vmul(const float& s, const rgl::vector4& a) { return rgl::vector4(s * a.x, s * a.y, s * a.z); }
static inline rgl::vector4 vadd(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// @ 0x423300
rgl::vector4 Curve::key_value(int i)
{
    rgl::vector4 v;
    if (recs[i].type == 14) v = lerp_value(i);
    else if (recs[i].type == 13) v = table_value(i);
    else if (recs[i].type == 11) v = recs[i].v7c;
    else v = recs[i].v8c;
    return v;
}
// @ 0x423390
rgl::vector4 Curve::lerp_value(int i)
{
    float t = 1.0f - recs[i].f18;
    CurveRec* r = &recs[i];
    rgl::vector4 v = vadd(vmul(r->v8c, r->f18), vmul(t, r->v7c));
    return v;
}
// @ 0x423440
rgl::vector4 Curve::table_value(int i)
{
    CurveRec& r = recs[i];
    if (!r.table) return rgl::vector4(0.0f, 0.0f, 0.0f);
    int a = r.key_begin;
    float key = (float)(r.key_end - a) * r.f18 + a;
    float frac;
    r.table->tbl.find(&cur, &frac, &key);
    float s = 1.0f - frac;
    const rgl::vector4* e = &r.table->tbl.vals[cur];
    rgl::vector4 v = vadd(vmul(frac, e[1]), vmul(s, e[0]));
    return v;
}
