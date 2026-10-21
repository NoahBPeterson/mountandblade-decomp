// Slice s00409500: collision contact generation (sphere/capsule primitives).
#include "rgl.h"
using namespace rgl;
namespace rgl { void normalize(vector4& out, const vector4& in); } // 0x407e00

// collision shape record (provisional): sphere centre at +0x30, radius at +0x40;
// capsule segment p0 at +0x20, p1 at +0x30, radius at +0x60.
struct coll_shape {
    char pad0[0x20];
    vector4 p0;     // 0x20
    vector4 p1;     // 0x30 (also sphere centre)
    float radius;   // 0x40 (sphere)
    char pad1[0x1c];
    float cap_radius; // 0x60
};
// 0x24-byte contact record
struct contact {
    vector4 normal;
    vector4 point;
    float depth;
};


// @ 0x409980
void sphere_sphere_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count)
{
    vector4 d(a->p1.x - b->p1.x, a->p1.y - b->p1.y, a->p1.z - b->p1.z);
    float depth = (a->radius + b->radius) - (float)sqrt(d.z * d.z + (d.x * d.x + d.y * d.y));
    if (!(depth < 0.0f)) {
        int n = ++*count;
        if (n <= max) {
            vector4 nrm;
            normalize(nrm, d);
            float t = b->radius / (a->radius + b->radius);
            vector4 dt(d.x * t, d.y * t, d.z * t);
            d.x = dt.x + b->p1.x; d.y = dt.y + b->p1.y; d.z = dt.z + b->p1.z;
            contact c;
            c.normal = nrm;
            c.point = d;
            c.point.w = nrm.w;
            c.depth = depth;
            out[n - 1] = c;
        }
    }
}
// @ 0x409500
// Closest points between segments [a0,a1] and [b0,b1]; outA on the first, outB on the second.
void segment_closest_points(const vector4* a0, const vector4* a1, const vector4* b0, const vector4* b1, vector4* outA, vector4* outB)
{
    vector4 d1(a1->x - a0->x, a1->y - a0->y, a1->z - a0->z);
    vector4 d2(b1->x - b0->x, b1->y - b0->y, b1->z - b0->z);
    vector4 r(a0->x - b0->x, a0->y - b0->y, a0->z - b0->z);
    // c, b, a stay in x87 registers (extended precision) in the original; only e is rounded to float
    double c = r.x * d2.x + r.y * d2.y + r.z * d2.z;
    double b = d1.x * d2.x + d1.y * d2.y + d1.z * d2.z;
    volatile float e_r = d2.x * d2.x + d2.y * d2.y + d2.z * d2.z;   // e is rounded to float in the original
    float e = e_r;
    double a = d1.x * d1.x + d1.y * d1.y + d1.z * d1.z;
    float denom = (float)(a * e - b * b);
    if (!(denom < 1e-06f)) {
        float sn = (b * c - e * (r.x * d1.x + r.y * d1.y + r.z * d1.z)) / denom;
        float tn = (sn * b + c) / e;
        if (sn < 0.0f) sn = 0.0f; else if (1.0f < sn) sn = 1.0f;
        if (tn < 0.0f) tn = 0.0f; else if (1.0f < tn) tn = 1.0f;
        vector4 pa;
        pa.x = sn * d1.x + a0->x; pa.y = sn * d1.y + a0->y; pa.z = sn * d1.z + a0->z;
        *outA = pa;
        vector4 pb;
        pb.x = d2.x * tn + b0->x; pb.y = d2.y * tn + b0->y; pb.z = tn * d2.z + b0->z;
        *outB = pb;
        return;
    }
    // (nearly) parallel: work in a frame whose forward axis is a0->a1.
    matrix m;
    m.o = *a0;
    vector4 nr(-r.x, -r.y, -r.z);
    m.u = nr;
    m.f = d1;
    m.orthonormalize();
    vector4 l3 = m.transform_point_to_local(*b0);
    vector4 l4 = m.transform_point_to_local(*b1);
    volatile float len_r = (float)sqrt(a);   // the original rounds the length to float before comparing
    float len = len_r;
    float lo = l3.y, hi = l4.y;
    bool swapped = false;
    if (l4.y < l3.y) { lo = l4.y; hi = l3.y; swapped = true; }
    const vector4* pick = b0;
    if (len < lo) {
        *outA = *a1;
        if (swapped) pick = b1;
    } else {
        if (hi < 0.0f) {
            *outA = *a0;
            if (!swapped) pick = b1;
        } else {
            if (0.0f < lo) {
                if (swapped) pick = b1;
            } else if (!swapped) pick = b1;
            *outB = *pick;
            float dot = m.u.x * nr.x + m.u.y * nr.y + m.u.z * nr.z;
            vector4 pa;
            pa.x = outB->x - m.u.x * dot; pa.y = outB->y - m.u.y * dot; pa.z = outB->z - m.u.z * dot;
            *outA = pa;
            return;
        }
    }
    *outB = *pick;
}

// @ 0x409ac0
// capsule a (segment p0-p1, radius at +0x60) against sphere b (centre +0x30, radius +0x40)
void capsule_sphere_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count)
{
    vector4 d(a->p1.x - a->p0.x, a->p1.y - a->p0.y, a->p1.z - a->p0.z);
    vector4 dir;
    normalize(dir, d);
    vector4 v(b->p1.x - a->p0.x, b->p1.y - a->p0.y, b->p1.z - a->p0.z);
    float t = v.x * dir.x + (v.y * dir.y + v.z * dir.z);
    vector4 cl;
    if (t < 0.0f) cl = a->p0;
    else if ((float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z)) < t) cl = a->p1;
    else { cl = vector4(dir.x * t + a->p0.x, dir.y * t + a->p0.y, dir.z * t + a->p0.z); cl.w = dir.w; }
    vector4 e(cl.x - b->p1.x, cl.y - b->p1.y, cl.z - b->p1.z);
    float depth = (a->cap_radius + b->radius) - (float)sqrt(e.x * e.x + (e.y * e.y + e.z * e.z));
    if (0.0f <= depth) {
        int n = ++*count;
        if (n <= max) {
            vector4 nrm;
            normalize(nrm, e);
            contact c;
            c.normal = nrm;
            c.normal.w = cl.w;
            c.point = vector4(nrm.x * b->radius + b->p1.x, nrm.y * b->radius + b->p1.y, nrm.z * b->radius + b->p1.z);
            c.point.w = dir.w;
            c.depth = depth;
            out[n - 1] = c;
        }
    }
}

// @ 0x409d20
// (distance from sphere b's centre to capsule a's axis) - (r_a + r_b)
float capsule_sphere_gap(coll_shape* a, coll_shape* b)
{
    vector4 d(a->p1.x - a->p0.x, a->p1.y - a->p0.y, a->p1.z - a->p0.z);
    vector4 dir;
    normalize(dir, d);
    vector4 v(b->p1.x - a->p0.x, b->p1.y - a->p0.y, b->p1.z - a->p0.z);
    float t = v.x * dir.x + (v.y * dir.y + v.z * dir.z);
    vector4 c;
    if (t < 0.0f) c = a->p0;
    else if ((float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z)) < t) c = a->p1;
    else c = vector4(dir.x * t + a->p0.x, dir.y * t + a->p0.y, dir.z * t + a->p0.z);
    float ex = c.x - b->p1.x, ey = c.y - b->p1.y, ez = c.z - b->p1.z;
    return (float)sqrt(ex * ex + (ey * ey + ez * ez)) - (a->cap_radius + b->radius);
}
