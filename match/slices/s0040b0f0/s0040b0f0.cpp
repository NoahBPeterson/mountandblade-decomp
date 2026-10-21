// Slice s0040b0f0: convex polygon (face) vs capsule contact generation (sibling of 0x409e90 capsule/capsule and
// 0x40a000 polygon/sphere in slice s00409e90; same contact record, same polygon view).
//
// Codegen levers that mattered (cl 13.10, x87):
//  - vector4 temporaries built with the 3-float ctor (te, qd, tt, st, dv, sm, gc ...) make cl compute all three
//    components first and spill z; a bare float expression does not.
//  - pax..pbz are plain floats: cl keeps the first of each triple in an x87 register across the cross product.
//  - cross_i / normalize_i are the bodies of rgl::cross(a,b) (0x407dc0) and vector4::normalize() (0x407d60) inlined.
//  - pp1 is volatile only to keep it out of ebp (the original reloads it from its stack home).
#include "rgl.h"
using namespace rgl;
namespace rgl { void normalize(vector4& out, const vector4& in); } // 0x407e00

// 0x24-byte contact record
struct contact {
    vector4 normal;
    vector4 point;
    float depth;
};

// capsule view (provisional): segment p0 at +0x20, p1 at +0x30, radius at +0x60.
struct coll_shape {
    char pad0[0x20];
    vector4 p0;
    vector4 p1;
    char pad1[0x20];
    float radius;
};
// convex polygon (provisional): up to 4 vertices at +0x20, plane normal at +0xa0, vertex count at +0xb0
struct coll_poly {
    char pad0[0x20];
    vector4 v[4];
    char pad1[0x40];
    vector4 normal;
    int count;
};

static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static __forceinline float normalize_i(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}

// @ 0x40b0f0
void polygon_capsule_contact(coll_poly* poly, coll_shape* cap, int max, contact* out, int* count)
{
    vector4 v[4];
    v[0] = poly->v[0]; v[1] = poly->v[1]; v[2] = poly->v[2]; v[3] = poly->v[3];
    const vector4* pp0 = &cap->p0;
    vector4 p0 = *pp0;
    const vector4* volatile pp1 = &cap->p1;
    vector4 p1 = *pp1;
    vector4 d(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
    vector4 t0(p0.x - v[0].x, p0.y - v[0].y, p0.z - v[0].z);
    float s0 = t0.x * poly->normal.x + (t0.y * poly->normal.y + t0.z * poly->normal.z);
    vector4 t1(p1.x - v[0].x, p1.y - v[0].y, p1.z - v[0].z);
    float s1 = t1.x * poly->normal.x + (t1.y * poly->normal.y + t1.z * poly->normal.z);
    float negr = -cap->radius;
    if (s0 < negr && s1 < negr) return;
    if (s0 > cap->radius && s1 > cap->radius) return;
    vector4 nn(-poly->normal.x, -poly->normal.y, -poly->normal.z);
    int i = 0;
    const vector4* cur = v;
    float depth = cap->radius - (s0 < s1 ? s0 : s1);
    bool done = false;
    bool ok = true;
    bool edge = false;
    vector4 ep;
    do {
        vector4 a = *cur;
        ++i;
        int j = i % poly->count;
        vector4 b = v[j];
        vector4 e(b.x - a.x, b.y - a.y, b.z - a.z);
        vector4 w(a.x - p0.x, a.y - p0.y, a.z - p0.z);
        float pax = p0.x - a.x, pay = p0.y - a.y, paz = p0.z - a.z;
        float pbx = p1.x - a.x, pby = p1.y - a.y, pbz = p1.z - a.z;
        vector4 c = cross_i(poly->normal, e);
        if (c.length_squared() == 0.0f) { ok = false; done = true; }
        normalize_i(c);
        float nx = c.x, ny = c.y, nz = c.z;
        float q0 = ny * pay + (nz * paz + nx * pax);
        float q1 = ny * pby + (nz * pbz + nx * pbx);
        if (!(q0 > 0.0f) || !(q1 > 0.0f)) {
            if (q0 < negr && q1 < negr) {
                ok = false; done = true;
            } else {
                float wd = w.x * d.x + (w.y * d.y + w.z * d.z);
                float ed = e.x * d.x + (e.y * d.y + e.z * d.z);
                float dd = d.x * d.x + (d.y * d.y + d.z * d.z);
                float t = (ed * wd - (w.x * e.x + (w.y * e.y + w.z * e.z)) * dd)
                          / ((e.x * e.x + (e.y * e.y + e.z * e.z)) * dd - ed * ed);
                float u = (t * ed + wd) / dd;
                if (t < 0.0) t = 0.0f;
                else if (t > 1.0) t = 1.0f;
                if (u < 0.0) u = 0.0f;
                else if (u > 1.0) u = 1.0f;
                vector4 te(t * e.x, t * e.y, t * e.z);
                vector4 pe(te.x + a.x, te.y + a.y, te.z + a.z);
                vector4 qd(d.x * u, d.y * u, d.z * u);
                vector4 qe(qd.x + p0.x, qd.y + p0.y, qd.z + p0.z);
                vector4 g(qe.x - pe.x, qe.y - pe.y, qe.z - pe.z);
                vector4 ne;
                vector4 etmp = e;
                normalize(ne, etmp);
                vector4 gc(g.z * ne.y - g.y * ne.z, ne.z * g.x - g.z * ne.x, g.y * ne.x - ne.y * g.x);
                if (gc.x * poly->normal.x + (gc.y * poly->normal.y + gc.z * poly->normal.z) < 0.0f) {
                    done = true;
                    if (cap->radius - (float)sqrt(g.x * g.x + (g.y * g.y + g.z * g.z)) < 0.0f) {
                        ok = false;
                    } else {
                        ok = true;
                        ep = pe;
                        edge = true;
                    }
                }
            }
        }
        cur += 1;
    } while (!done && i < poly->count);
    if (!ok) return;
    int n = ++*count;
    if (n > max) return;
    if (!edge) {
        if (s0 >= 0.0f && s1 > 0.0f) {
            if (s0 < s1) {
                vector4 tt(s0 * poly->normal.x, s0 * poly->normal.y, s0 * poly->normal.z);
                ep = vector4(pp0->x - tt.x, pp0->y - tt.y, pp0->z - tt.z);
            } else {
                const vector4* q1 = pp1;
                vector4 tt(s1 * poly->normal.x, s1 * poly->normal.y, s1 * poly->normal.z);
                ep = vector4(q1->x - tt.x, q1->y - tt.y, q1->z - tt.z);
            }
        } else if (s0 < 0.0f && s1 < 0.0f) {
            if (s0 > s1) {
                vector4 tt(s0 * poly->normal.x, s0 * poly->normal.y, s0 * poly->normal.z);
                ep = vector4(pp0->x - tt.x, pp0->y - tt.y, pp0->z - tt.z);
            } else {
                const vector4* q1 = pp1;
                vector4 tt(s1 * poly->normal.x, s1 * poly->normal.y, s1 * poly->normal.z);
                ep = vector4(q1->x - tt.x, q1->y - tt.y, q1->z - tt.z);
            }
        } else {
            const vector4* q1 = pp1;
            vector4 dir(q1->x - pp0->x, q1->y - pp0->y, q1->z - pp0->z);
            dir.normalize();
            vector4 dv(poly->v[0].x - pp0->x, poly->v[0].y - pp0->y, poly->v[0].z - pp0->z);
            float k = dir.x * poly->normal.x + (dir.y * poly->normal.y + dir.z * poly->normal.z);
            if (k == 0.0f) {
                vector4 sm(q1->x + pp0->x, q1->y + pp0->y, q1->z + pp0->z);
                ep = vector4(sm.x * 0.5f, sm.y * 0.5f, sm.z * 0.5f);
            } else {
                k = (dv.x * poly->normal.x + (dv.y * poly->normal.y + dv.z * poly->normal.z)) / k;
                vector4 st(dir.x * k, dir.y * k, dir.z * k);
                ep = vector4(st.x + pp0->x, st.y + pp0->y, st.z + pp0->z);
            }
        }
    }
    contact c;
    c.normal = nn;
    c.point = ep;
    c.depth = depth;
    out[n - 1] = c;
}
