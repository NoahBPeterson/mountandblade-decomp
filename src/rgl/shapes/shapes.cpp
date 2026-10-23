// shapes.cpp (0x407c10..0x40e940): the physics collision shapes (sphere, capsule, face, manifold, composite), their
// contact generation, and the vector/matrix helpers that sit in the same object. Migrated from slices
// s00407c10, s00408110, s00408a50, s00408e60, s00409500, s00409e90, s0040a640, s0040aff0, s0040b0f0, s0040bcb0,
// s0040c670, s0040cfe0, s0040d7d0 and s0040e060 (all but its last function 0x40e940).
// Boundary: the start 0x407c10 follows dxutil's CArrayList; the end 0x40e940 is only the next agent's start (renderer::
// set_blend_mode and the psys region continue the same emission order). See MIGRATION.md.
#include <string>
#include <exception>
#include <stdlib.h>
#include "./shapes.h"
using namespace rgl;

// Bind the ext declarations of shapes.h to the functions they stand for (see there).
#pragma comment(linker, "/alternatename:?transform_point@pos_ext@@QBE?AUvector4@rgl@@ABU23@@Z=?transform_point@pos@@QBE?AUvector4@rgl@@ABU23@@Z")
#pragma comment(linker, "/alternatename:?rot_ext@pos_ext@@QBE?AUvector4@rgl@@ABU23@@Z=?rot@pos@@QBE?AUvector4@rgl@@ABU23@@Z")
#pragma comment(linker, "/alternatename:?normalize_ext@rgl@@YAXAAUvector4@1@ABU21@@Z=?normalize@rgl@@YAXAAUvector4@1@ABU21@@Z")
#pragma comment(linker, "/alternatename:?orthonormalize@matrix_ext@@QAEXXZ=?orthonormalize@matrix@rgl@@QAEXXZ")
#pragma comment(linker, "/alternatename:?transform_point_to_local@matrix_ext@@QBE?AUvector4@rgl@@ABU23@@Z=?transform_point_to_local@matrix@rgl@@QBE?AUvector4@2@ABU32@@Z")
#pragma comment(linker, "/alternatename:?normalize@vec4_ext@@QAEMXZ=?normalize@vector4@rgl@@QAEMXZ")
#pragma comment(linker, "/alternatename:?vmin_ext@@YAXPAUvector4@rgl@@PBU12@1@Z=?vmin@@YAXPAUvector4@rgl@@PBU12@1@Z")
#pragma comment(linker, "/alternatename:?vmax_ext@@YAXPAUvector4@rgl@@PBU12@1@Z=?vmax@@YAXPAUvector4@rgl@@PBU12@1@Z")

// Other objects (src/rgl/scene/shape_clone.h) call the out-of-line base constructor 0x407ff0 as shape_ext::shape_ext().
#pragma comment(linker, "/alternatename:??0shape_ext@@QAE@XZ=??0shape_base@@QAE@XZ")

// ---- vector4 / matrix (0x407c30..0x407f70) ---------------------------------------------------------------
// @ 0x407c10
const char* string_view_::get() const { return buf ? buf : (const char*)&buf; }
// @ 0x407c20
self_ref* self_ref::self() { return this; }

namespace rgl {

// @ 0x407c30
vector4& vector4::set(const float& x_, const float& y_, const float& z_) { x = x_; y = y_; z = z_; return *this; }
// @ 0x407c50
float vector4::out_length() const { return length(); }
// @ 0x407c80
float vector4::out_length_squared() const { return length_squared(); }
// @ 0x407ca0
vector4 vector4::operator-(const vector4& o) const { return vector4(x - o.x, y - o.y, z - o.z); }
// @ 0x407cd0
vector4 vector4::operator*(const float& s) const { return vector4(x * s, y * s, z * s); }
// @ 0x407d00
vector4 operator*(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
// @ 0x407d30
float length(const vector4& v) { return v.length(); }
// @ 0x407d60
float vector4::normalize()
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
// @ 0x407dc0
vector4 cross(const vector4& a, const vector4& b) { return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
// @ 0x407e00
void normalize(vector4& out, const vector4& in)
{
    float len = in.length();
    if (len > 0.0f) {
        float inv = 1.0f / len;
        out.x = in.x * inv; out.y = in.y * inv; out.z = in.z * inv;
    } else {
        out = vector4(0.0f, 1.0f, 0.0f);
    }
}
// @ 0x407e90
float dot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
// @ 0x407eb0
void cross(vector4& r, const vector4& a, const vector4& b) { r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }
// @ 0x407ef0
void matrix::orthonormalize()
{
    normalize(f, f);
    cross(s, f, u);
    normalize(s, s);
    cross(u, s, f);
}
// @ 0x407f70
vector4 matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
}

} // namespace rgl

// ---- shape_base (0x407ff0..0x4080b0) -----------------------------------------------------------------------
// @ 0x407ff0
shape_base::shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
// @ 0x408020
shape_base* shape_base::clone() { return 0; }
// @ 0x408030
void shape_base::scale(const float*) {}
void shape_base::transform(const pos*) {}                  // 0x408030 (folded with scale)
// @ 0x408040
void shape_base::set_mc(const int& v) { mc = v; }
// @ 0x408050
vector4* shape_base::get_min(vector4* out) { out->x = 0; out->y = 0; out->z = 0; return out; }
vector4* shape_base::get_max(vector4* out) { out->x = 0; out->y = 0; out->z = 0; return out; }   // 0x408050
// @ 0x408070
void shape_base::set_m8(int v) { m8 = v; }
// @ 0x408080
void shape_base::or_m8(int v) { m8 |= v; }
void shape_base::draw() {}                                 // 0x477b70 (the empty function of another object)

// ---- sphere (0x408020 .. 0x4085d0) ---------------------------------------------------------------------------
int sphere::shape_type() { return 0; }                     // 0x408020 (folded with shape_base::clone)
// @ 0x408090
string sphere::type_name() const { return "sphere"; }
// @ 0x4080b0
void sphere::copy_from(const sphere* o)
{
    m4 = o->m4;
    local_center = o->local_center;
    center = o->local_center;
    radius = o->radius;
    m8 = o->m8;
}
// @ 0x408110
void sphere::draw()
{
    g_render.stack[g_render.sp] = g_render.cur;
    g_render.sp = g_render.sp + 1;
    g_render.translate(center);
    g_render.draw_sphere(radius);
    g_render.pop();
}
// @ 0x408170
void renderer::translate(const vector4& t)
{
    vector4* o = &cur.o;
    o->x += t.x;
    o->y = t.y + o->y;
    o->z = t.z + o->z;
    dev->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)&cur);
}
// @ 0x4081b0
void renderer::pop()
{
    sp = sp - 1;
    cur = stack[sp];
    dev->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)&cur);
}
// @ 0x4081f0
void renderer::draw_sphere(float r)
{
    float pts[12288];
    int n = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            vector4 c[4];
            for (int k = 0; k < 4; k++) {
                int lat = i, lon = j;
                if (k == 1 || k == 2) lat = i + 1;
                if (k == 2 || k == 3) lon = j + 1;
                float a = (float)lon * 0.7853982f;
                float b = (float)lat * 0.62831855f - 1.5707964f;
                c[k].x = (float)(cos(a) * cos(b) * r);
                c[k].y = (float)(sin(a) * cos(b) * r);
                c[k].z = (float)(sin(b) * r);
            }
            pts[n] = c[0].x; pts[n + 1] = c[0].y; pts[n + 2] = c[0].z; n += 3;
            pts[n] = c[1].x; pts[n + 1] = c[1].y; pts[n + 2] = c[1].z; n += 3;
            pts[n] = c[1].x; pts[n + 1] = c[1].y; pts[n + 2] = c[1].z; n += 3;
            pts[n] = c[2].x; pts[n + 1] = c[2].y; pts[n + 2] = c[2].z; n += 3;
            pts[n] = c[2].x; pts[n + 1] = c[2].y; pts[n + 2] = c[2].z; n += 3;
            pts[n] = c[3].x; pts[n + 1] = c[3].y; pts[n + 2] = c[3].z; n += 3;
            pts[n] = c[3].x; pts[n + 1] = c[3].y; pts[n + 2] = c[3].z; n += 3;
            pts[n] = c[0].x; pts[n + 1] = c[0].y; pts[n + 2] = c[0].z; n += 3;
        }
    }
    dev->SetTexture(0, 0);
    dev->SetRenderState(D3DRS_AMBIENT, 0xffbffff2);
    dev->SetVertexShader(0);
    dev->DrawPrimitiveUP(D3DPT_LINELIST, 0xa0, pts, 12);
}
// @ 0x4083f0
void sphere::scale(const float* s)
{
    local_center.x = s[0] * local_center.x;
    local_center.y = s[1] * local_center.y;
    local_center.z = s[2] * local_center.z;
    radius = (float)fabs(s[0]) * radius;
}
// @ 0x408420
void sphere::transform(const pos* m)
{
    center = ((const pos_ext*)m)->transform_point(local_center);
}
// @ 0x408460
vector4 pos::transform_point(const vector4& p) const
{
    return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                   s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                   s.z * p.x + f.z * p.y + u.z * p.z + o.z);
}

// stream helpers: read / write three floats (0x408510 reads: stream slot 14 = r_float; 0x408570 writes: slot 3)
__declspec(noinline) stream_base* read_vec3(stream_base* s, float* v);     // 0x408510
__declspec(noinline) stream_base* write_vec3(stream_base* s, float* v);    // 0x408570
// @ 0x408510
stream_base* read_vec3(stream_base* s, float* v)
{
    s->r_float(v).r_float(v + 1).r_float(v + 2);
    return s;
}
// @ 0x408570
stream_base* write_vec3(stream_base* s, float* v)
{
    s->w_float(v).w_float(v + 1).w_float(v + 2);
    return s;
}
static __forceinline void write3_inl(stream_base* s, float* v) { s->w_float(v).w_float(v + 1).w_float(v + 2); }
static __forceinline void read3_inl(stream_base* s, float* v) { s->r_float(v).r_float(v + 1).r_float(v + 2); }
static inline void set3(float* o, const float& x, const float& y, const float& z) { o[0] = x; o[1] = y; o[2] = z; }
static inline float fmin_(float a, float b) { return a < b ? a : b; }
static inline float fmax_(float a, float b) { return a > b ? a : b; }
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x4084c0
void sphere::load(stream_base* s)
{
    read_vec3(&s->r_float(&radius), &local_center.x);
    center = local_center;
    s->r_int(&m8);
}
// @ 0x408540
void sphere::save(stream_base* s)
{
    write_vec3(&s->w_float(&radius), &local_center.x);
    s->w_int(&m8);
}
// @ 0x4085a0
vector4* sphere::get_min(vector4* out)
{
    set3(&out->x, center.x - radius, center.y - radius, center.z - radius);
    return out;
}
// @ 0x4085d0
vector4* sphere::get_max(vector4* out)
{
    set3(&out->x, center.x + radius, center.y + radius, center.z + radius);
    return out;
}

// ---- capsule (0x408600..0x408910) ------------------------------------------------------------------------------
// @ 0x408630
string capsule::type_name() const { return "capsule"; }
// @ 0x408650
capsule* capsule::create_new() { return new capsule; }
// @ 0x408690
void capsule::copy_from(const capsule* o)
{
    m4 = o->m4;
    l0 = o->l0;
    l1 = o->l1;
    p0 = l0;
    p1 = l1;
    radius = o->radius;
    m8 = o->m8;
}
// @ 0x408720
void capsule::scale(const float* s)
{
    l0.x = s[0] * l0.x;
    l0.y = s[1] * l0.y;
    l0.z = s[2] * l0.z;
    l1.x = s[0] * l1.x;
    l1.y = s[1] * l1.y;
    l1.z = s[2] * l1.z;
    radius = (float)fabs(s[0]) * radius;
}
// @ 0x408770
void capsule::transform(const pos* m)
{
    p0 = m->transform_point_inl(l0);
    p1 = m->transform_point_inl(l1);
}
// @ 0x408870
void capsule::load(stream_base* s)
{
    float* a = &l0.x;
    stream_base* t = &s->r_float(&radius);
    read3_inl(t, a);
    read3_inl(t, &l1.x);
    p0 = l0;
    p1 = l1;
    s->r_int(&m8);
}
// @ 0x408910
void capsule::save(stream_base* s)
{
    float* a = &l0.x;
    stream_base* t = &s->w_float(&radius);
    write3_inl(t, a);
    write3_inl(t, &l1.x);
    s->w_int(&m8);
}

// ---- face (0x408980..0x408e40) ---------------------------------------------------------------------------------
// @ 0x4089c0
string face::type_name() const { return "face"; }
// @ 0x4089e0
void face::copy_from(const face* o)
{
    m4 = o->m4;
    count = o->count;
    for (int i = 0; i < count; i++) v[i] = o->v[i];
    m8 = o->m8;
}

// vector4 with the (1e-10) normalize that callers inline (rgl::vector4::normalize at 0x407d60)
struct vec4i : vector4 {
    float normalize_inl()
    {
        float len = length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};

// @ 0x408a50
void face::scale(const float* s)
{
    for (int i = 0; i < count; i++) {
        v[i].x *= s[0];
        v[i].y = v[i].y * s[1];
        v[i].z *= s[2];
    }
    int neg = 0;
    if (s[0] < 0.0f) neg = 1;
    if (s[1] < 0.0f) neg++;
    if (s[2] < 0.0f) neg++;
    if (neg == 1 || neg == 3) {
        normal = vector4(-normal.x, -normal.y, -normal.z);
        if (count == 3) {
            vector4 t = v[0];
            v[0] = v[2];
            v[2] = t;
        } else {
            vector4 t = v[1];
            v[1] = v[3];
            v[3] = t;
        }
    }
}
// @ 0x408bd0
void face::transform(const pos* m)
{
    for (int i = 0; i < count; i++) {
        const matrix& mm = *m;
        const vector4& s = mm.s, &f = mm.f, &u = mm.u, &o = mm.o;
        const vector4& p = v[i];
        w[i] = vector4((s.x * p.x + f.x * p.y) + u.x * p.z + o.x,
                       (s.y * p.x + f.y * p.y) + u.y * p.z + o.y,
                       (f.z * p.y + s.z * p.x) + u.z * p.z + o.z);
    }
    vector4 e1 = vsub(w[2], w[1]);
    vector4 e0 = vsub(w[1], w[0]);
    vcross(normal, e0, e1);
    ((vec4i&)normal).normalize_inl();
}
// @ 0x408d60
void face::load(stream_base* s)
{
    s->r_int(&count);
    int i = 0;
    if (count > 0) {
        float* p = &v[0].z;
        do {
            s->r_float(p - 2).r_float(p - 1).r_float(p);
            i++;
            p += 4;
        } while (i < count);
    }
    s->r_int(&m8);
}
// @ 0x408dd0
void face::save(stream_base* s)
{
    s->w_int(&count);
    int i = 0;
    if (count > 0) {
        float* p = &v[0].z;
        do {
            s->w_float(p - 2).w_float(p - 1).w_float(p);
            i++;
            p += 4;
        } while (i < count);
    }
    s->w_int(&m8);
}
// @ 0x408e40
void face::draw()
{
    g_render.draw_polygon(w, count);
}
// @ 0x408e60
// Draws the outline of a convex polygon (at most 4 points), shrunk 1% towards its centroid, as a line list.
void renderer::draw_polygon(const vector4* pts, int n)
{
    vector4 shrunk[4];
    float lines[8][3];
    vector4 c;
    c.x = 0.0f; c.y = 0.0f; c.z = 0.0f;
    int i;
    for (i = 0; i < n; i++) {
        c.x += pts[i].x;
        c.y += pts[i].y;
        c.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    c.x = c.x * inv;
    c.y = c.y * inv;
    c.z = c.z * inv;
    for (i = 0; i < n; i++) {
        vector4 t((pts[i].x - c.x) * 0.99f + c.x, (pts[i].y - c.y) * 0.99f + c.y, (pts[i].z - c.z) * 0.99f + c.z);
        shrunk[i] = t;
    }
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        lines[2 * i][0] = shrunk[i].x;
        lines[2 * i][1] = shrunk[i].y;
        lines[2 * i][2] = shrunk[i].z;
        lines[2 * i + 1][0] = shrunk[j].x;
        lines[2 * i + 1][1] = shrunk[j].y;
        lines[2 * i + 1][2] = shrunk[j].z;
    }
    dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    dev->SetRenderState(D3DRS_FOGENABLE, 0);
    dev->SetRenderState(D3DRS_LIGHTING, 0);
    dev->SetTexture(0, 0);
    dev->SetRenderState(D3DRS_AMBIENT, 0xffbffff2);
    dev->SetVertexShader(0);
    dev->DrawPrimitiveUP(D3DPT_LINELIST, n, lines, 12);
}

// ---- contact generation (0x409500..0x40b0f0) -------------------------------------------------------------------
// @ 0x409980
void sphere_sphere_contact(sphere* a, sphere* b, int max, contact* out, int* count)
{
    vector4 d(a->center.x - b->center.x, a->center.y - b->center.y, a->center.z - b->center.z);
    float depth = (a->radius + b->radius) - (float)sqrt(d.z * d.z + (d.x * d.x + d.y * d.y));
    if (!(depth < 0.0f)) {
        int n = ++*count;
        if (n <= max) {
            vector4 nrm;
            normalize_ext(nrm, d);
            float t = b->radius / (a->radius + b->radius);
            vector4 dt(d.x * t, d.y * t, d.z * t);
            d.x = dt.x + b->center.x; d.y = dt.y + b->center.y; d.z = dt.z + b->center.z;
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
    vector4 l3 = ((matrix_ext&)m).transform_point_to_local(*b0);
    vector4 l4 = ((matrix_ext&)m).transform_point_to_local(*b1);
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
// capsule a (segment p0-p1, radius at +0x60) against sphere b (centre +0x30, radius +0x40).
// The original reuses two vector4 slots: A holds the segment direction, then the closest point, then the contact
// normal; B holds the unit direction, then the closest-point-to-centre vector.
void capsule_sphere_contact(capsule* a, sphere* b, int max, contact* out, int* count)
{
    vector4 A(a->p1.x - a->p0.x, a->p1.y - a->p0.y, a->p1.z - a->p0.z);
    vector4 B;
    normalize_ext(B, A);
    vector4 v(b->center.x - a->p0.x, b->center.y - a->p0.y, b->center.z - a->p0.z);
    float t = v.x * B.x + (v.y * B.y + v.z * B.z);
    if (t < 0.0f) A = a->p0;
    else if ((float)sqrt(A.x * A.x + (A.y * A.y + A.z * A.z)) < t) A = a->p1;
    else {
        vector4 tmp(B.x * t, B.y * t, B.z * t);
        A = vector4(tmp.x + a->p0.x, tmp.y + a->p0.y, tmp.z + a->p0.z);
        A.w = B.w;
    }
    B = vector4(A.x - b->center.x, A.y - b->center.y, A.z - b->center.z);
    float depth = (a->radius + b->radius) - (float)sqrt(B.x * B.x + (B.y * B.y + B.z * B.z));
    if (0.0f <= depth) {
        int n = ++*count;
        if (n <= max) {
            normalize_ext(A, B);
            contact c;
            c.normal = A;
            volatile float zr = A.z * b->radius;                 // the original spills this product through a float slot
            c.point = vector4(A.x * b->radius + b->center.x, A.y * b->radius + b->center.y, zr + b->center.z);
            c.point.w = B.w;
            c.depth = depth;
            out[n - 1] = c;
        }
    }
}

// @ 0x409d20
// (distance from sphere b's centre to capsule a's axis) - (r_a + r_b)
float capsule_sphere_gap(capsule* a, sphere* b)
{
    vector4 d(a->p1.x - a->p0.x, a->p1.y - a->p0.y, a->p1.z - a->p0.z);
    vector4 dir;
    normalize_ext(dir, d);
    vector4 v(b->center.x - a->p0.x, b->center.y - a->p0.y, b->center.z - a->p0.z);
    float t = v.x * dir.x + (v.y * dir.y + v.z * dir.z);
    vector4 c;
    if (t < 0.0f) c = a->p0;
    else if ((float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z)) < t) c = a->p1;
    else c = vector4(dir.x * t + a->p0.x, dir.y * t + a->p0.y, dir.z * t + a->p0.z);
    float ex = c.x - b->center.x, ey = c.y - b->center.y, ez = c.z - b->center.z;
    return (float)sqrt(ex * ex + (ey * ey + ez * ez)) - (a->radius + b->radius);
}

// @ 0x409e90
void capsule_capsule_contact(capsule* a, capsule* b, int max, contact* out, int* count)
{
    vector4 pa, pb;
    segment_closest_points(&a->p0, &a->p1, &b->p0, &b->p1, &pa, &pb);
    vector4 d(pa.x - pb.x, pa.y - pb.y, pa.z - pb.z);
    float depth = (a->radius + b->radius) - (float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z));
    if (!(depth < 0.0f)) {
        int n = ++*count;
        if (n <= max) {
            float t = b->radius / (a->radius + b->radius);
            vector4 dt(d.x * t, d.y * t, d.z * t);
            pa.x = dt.x + pb.x; pa.y = dt.y + pb.y; pa.z = dt.z + pb.z;
            normalize_ext(pb, d);
            contact c;
            c.normal = pb;
            c.point = pa;
            c.depth = depth;
            out[n - 1] = c;
        }
    }
}

// @ 0x40a000
// sphere b against convex polygon a: face contact, or edge contact when the centre is outside an edge.
void polygon_sphere_contact(face* a, sphere* b, int max, contact* out, int* count)
{
    vector4 v[4];
    vector4 e, ed, w;
    v[0] = a->w[0]; v[1] = a->w[1]; v[2] = a->w[2]; v[3] = a->w[3];
    float e1x = v[1].x - v[0].x;
    float e1y = v[1].y - v[0].y;
    vector4 n;
    n.x = e1y * (v[2].z - v[1].z) - (v[1].z - v[0].z) * (v[2].y - v[1].y);
    n.y = (v[1].z - v[0].z) * (v[2].x - v[1].x) - (v[2].z - v[1].z) * e1x;
    n.z = (v[2].y - v[1].y) * e1x - e1y * (v[2].x - v[1].x);
    normalize_ext(n, n);
    float dist = (b->center.x - v[0].x) * n.x + ((b->center.y - v[0].y) * n.y + (b->center.z - v[0].z) * n.z);
    if (b->radius < dist)
        return;
    float negr = -b->radius;
    if (dist < negr)
        return;

    vector4 nn(-n.x, -n.y, -n.z);
    bool outside = false;
    bool hit = true;
    float depth_z = nn.z * dist;
    vector4 pt(nn.x * dist + b->center.x, nn.y * dist + b->center.y, depth_z + b->center.z);
    pt.w = nn.w;
    float depth = b->radius - dist;
    int cnt = a->count;
    int i = 0;
    do {
        ++i;
        int j = i % cnt;
        const vector4& s = v[i - 1];
        e = vector4(v[j].x - s.x, v[j].y - s.y, v[j].z - s.z);
        normalize_ext(ed, e);
        w = vector4(b->center.x - s.x, b->center.y - s.y, b->center.z - s.z);
        float cx = ed.y * w.z - ed.z * w.y;
        float cy = ed.z * w.x - w.z * ed.x;
        float cz = w.y * ed.x - ed.y * w.x;
        if (cx * n.x + (cy * n.y + n.z * cz) < 0.0f) {
            outside = true;
            float t;
            if (b->radius < (float)sqrt(cx * cx + (cy * cy + cz * cz))
                || (t = w.x * ed.x + (ed.y * w.y + ed.z * w.z), !(negr < t))
                || !(t < (float)sqrt(e.x * e.x + (e.y * e.y + e.z * e.z)) + b->radius)) {
                hit = false;
            } else {
                hit = true;
                float dz = ed.z * t;
                pt = vector4(ed.x * t + s.x, ed.y * t + s.y, ed.z * t + s.z);
                pt.w = ed.w;
                float fx = pt.x - b->center.x, fy = pt.y - b->center.y, fz = pt.z - b->center.z;
                depth = b->radius - (float)sqrt(fx * fx + (fy * fy + fz * fz));
                (void)dz;
            }
        }
    } while (!outside && i < cnt);
    if (hit) {
        int k = ++*count;
        if (k <= max) {
            contact c;
            c.normal = nn;
            c.point = pt;
            c.depth = depth;
            out[k - 1] = c;
        }
    }
}

// ---- sphere / capsule / face copies, capsule draw and bounds (0x40a540..0x40aff0) --------------------------------
// @ 0x40a540
// (the scalar deleting destructor of shape_base / sphere / capsule / face is generated; one body for all four)
// @ 0x40a560
shape_base* capsule::clone() { return new capsule(*this); }
// @ 0x40a580
capsule::capsule(const capsule& o) : shape_base(o), p0(o.p0), p1(o.p1), l0(o.l0), l1(o.l1), radius(o.radius) {}

// @ 0x40a640
void capsule::draw()
{
    vector4 d(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
    float half = d.length() * 0.5f;
    normalize_ext(d, d);
    vector4 e;
    vcross(e, d, vector4(1.0f, 0.0f, 0.0f));
    normalize_ext(e, e);
    vector4 f;
    vcross(f, e, d);
    matrix m;
    m.s = f;
    m.f = e;
    m.u = d;
    float sx = p1.x + p0.x, sy = p1.y + p0.y, sz = p1.z + p0.z;
    vector4 mid;
    mid.x = sx * 0.5f; mid.y = sy * 0.5f; mid.z = sz * 0.5f;
    m.o = mid;
    g_render.stack[g_render.sp] = g_render.cur;
    g_render.sp = g_render.sp + 1;
    g_render.apply(&m);
    g_render.draw_capsule(-half, half, radius);
    g_render.sp = g_render.sp - 1;
    g_render.cur = g_render.stack[g_render.sp];
    g_render.dev->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)&g_render.cur);
}
// @ 0x40a870
void renderer::apply(const matrix* m)
{
    struct { mat3 r; vector4 o; } t;
    matrix* c = &cur;
    t.o = ((const pos_ext*)c)->transform_point(m->o);
    t.r = ((const pos*)c)->rot3(*m);
    *c = *(matrix*)&t;
    c->s.w = 0.0f; c->f.w = 0.0f; c->u.w = 0.0f; c->o.w = 1.0f;
    dev->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)c);
}
// @ 0x40a920
mat3 pos::rot3(const matrix& m) const { return mat3(((const pos_ext*)this)->rot_ext(m.s), ((const pos_ext*)this)->rot_ext(m.f), ((const pos_ext*)this)->rot_ext(m.u)); }
// @ 0x40a9c0
vector4 pos::rot(const vector4& p) const
{
    return vector4(p.x * s.x + (f.x * p.y + u.x * p.z),
                   f.y * p.y + (s.y * p.x + u.y * p.z),
                   f.z * p.y + (s.z * p.x + u.z * p.z));
}
// @ 0x40aa20
void renderer::draw_capsule(float a, float b, float r)
{
    int n;
    float pts[12288];
    n = 0;
    tessellate(pts, &n, a, b, r, 3, 8, 1);
    tessellate(pts, &n, a, b, r, 1, 8, 0);
    tessellate(pts, &n, a, b, r, 3, 8, -1);
    dev->SetTexture(0, 0);
    dev->SetRenderState(D3DRS_AMBIENT, 0xffbffff2);
    dev->SetVertexShader(0);
    dev->DrawPrimitiveUP(D3DPT_LINELIST, 0xe0, pts, 12);
}
// @ 0x40ab10
void renderer::tessellate(float* pts, int* n, float a, float b, float r, int stacks, int slices, int mode)
{
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            vector4 c[4];
            for (int k = 0; k < 4; k++) {
                int lat = i, lon = j;
                if (k == 1 || k == 2) lat = i + 1;
                if (k == 2 || k == 3) lon = j + 1;
                float lonang = (float)lon * 6.2831855f / (float)slices;
                float latang;
                if (mode > 0) latang = (float)lat * 1.5707964f / stacks - 1.5707964f;
                else if (mode < 0) latang = (float)lat * 1.5707964f / stacks;
                else latang = 0.0f;
                c[k].x = (float)(cos(lonang) * cos(latang) * r);
                c[k].y = (float)(sin(lonang) * cos(latang) * r);
                if (mode > 0) c[k].z = (float)(sin(latang) * r + a);
                else if (mode < 0 || k == 1 || k == 2) c[k].z = (float)(sin(latang) * r + b);
                else c[k].z = (float)(sin(latang) * r + a);
            }
            pts[*n] = c[0].x; pts[*n + 1] = c[0].y; pts[*n + 2] = c[0].z; *n += 3;
            pts[*n] = c[1].x; pts[*n + 1] = c[1].y; pts[*n + 2] = c[1].z; *n += 3;
            pts[*n] = c[1].x; pts[*n + 1] = c[1].y; pts[*n + 2] = c[1].z; *n += 3;
            pts[*n] = c[2].x; pts[*n + 1] = c[2].y; pts[*n + 2] = c[2].z; *n += 3;
            pts[*n] = c[2].x; pts[*n + 1] = c[2].y; pts[*n + 2] = c[2].z; *n += 3;
            pts[*n] = c[3].x; pts[*n + 1] = c[3].y; pts[*n + 2] = c[3].z; *n += 3;
            pts[*n] = c[3].x; pts[*n + 1] = c[3].y; pts[*n + 2] = c[3].z; *n += 3;
            pts[*n] = c[0].x; pts[*n + 1] = c[0].y; pts[*n + 2] = c[0].z; *n += 3;
        }
    }
}
// @ 0x40ad80
vector4* capsule::get_min(vector4* out)
{
    set3(&out->x, fmin_(p0.x, p1.x) - radius, fmin_(p0.y, p1.y) - radius, fmin_(p0.z, p1.z) - radius);
    return out;
}
// @ 0x40ade0
vector4* capsule::get_max(vector4* out)
{
    set3(&out->x, fmax_(p0.x, p1.x) + radius, fmax_(p0.y, p1.y) + radius, fmax_(p0.z, p1.z) + radius);
    return out;
}
// @ 0x40ae40
shape_base* face::clone() { return new face(*this); }
// @ 0x40ae70
face::face(const face& o) : shape_base(inline_init()), count(o.count)
{
    for (int i = 0; i < count; i++) v[i] = o.v[i];
}
// @ 0x40aef0
vector4* face::get_min(vector4* out)
{
    vector4 t = w[0];
    for (int i = 1; i < count; i++) vmin_ext(&t, &t, &w[i]);
    *out = t;
    return out;
}
// @ 0x40af80
__declspec(noinline) void __cdecl vmin(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmin_(a->x, b->x);
    out->y = fmin_(a->y, b->y);
    out->z = fmin_(a->z, b->z);
}
// @ 0x40aff0
vector4* face::get_max(vector4* out)
{
    vector4 t = w[0];
    for (int i = 1; i < count; i++) vmax_ext(&t, &t, &w[i]);
    *out = t;
    return out;
}
// @ 0x40b080
__declspec(noinline) void __cdecl vmax(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmax_(a->x, b->x);
    out->y = fmax_(a->y, b->y);
    out->z = fmax_(a->z, b->z);
}

// ---- polygon / capsule contact (0x40b0f0) ---------------------------------------------------------------------
// Codegen levers that mattered (cl 13.10, x87):
//  - vector4 temporaries built with the 3-float ctor (te, qd, tt, st, dv, sm, gc ...) make cl compute all three
//    components first and spill z; a bare float expression does not.
//  - pax..pbz are plain floats: cl keeps the first of each triple in an x87 register across the cross product.
//  - cross_i / normalize_i are the bodies of rgl::cross(a,b) (0x407dc0) and vector4::normalize() (0x407d60) inlined.
//  - pp1 is volatile only to keep it out of ebp (the original reloads it from its stack home).
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
void polygon_capsule_contact(face* poly, capsule* cap, int max, contact* out, int* count)
{
    vector4 v[4];
    v[0] = poly->w[0]; v[1] = poly->w[1]; v[2] = poly->w[2]; v[3] = poly->w[3];
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
                normalize_ext(ne, etmp);
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
            ((vec4_ext&)dir).normalize();
            vector4 dv(poly->w[0].x - pp0->x, poly->w[0].y - pp0->y, poly->w[0].z - pp0->z);
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

// ---- shape-vs-shape dispatcher (0x40bcb0) and sphere clone / copy ctor ------------------------------------------
// @ 0x40bcb0
__declspec(noinline) void shape_contacts(shape_base* a, shape_base* b, int max, contact* out, int* count)
{
    if (a->shape_type() == 3) {
        if (b->shape_type() == 3) {
            for (int i = 0; i < ((composite*)a)->count; i++) {
                if (*count != 0 && *count >= max) break;
                for (int j = 0; j < ((composite*)b)->count; j++) {
                    if (*count != 0 && *count >= max) break;
                    shape_contacts(((composite*)a)->bodies[i], ((composite*)b)->bodies[j], max, out, count);
                }
            }
        } else {
            for (int i = 0; i < ((composite*)a)->count; i++) {
                if (*count != 0 && *count >= max) break;
                shape_contacts(((composite*)a)->bodies[i], b, max, out, count);
            }
        }
    } else if (b->shape_type() == 3) {
        for (int i = 0; i < ((composite*)b)->count; i++) {
            if (*count != 0 && *count >= max) break;
            shape_contacts(a, ((composite*)b)->bodies[i], max, out, count);
        }
    } else if (a->shape_type() == 0) {
        if (b->shape_type() == 0) {
            sphere_sphere_contact((sphere*)a, (sphere*)b, max, out, count);
        } else {
            int first = *count;
            shape_contacts(b, a, max, out, count);
            for (int i = first; i < *count && i < max; i++) {
                out[i].normal.x = out[i].normal.x * -1.0f;
                out[i].normal.y = out[i].normal.y * -1.0f;
                out[i].normal.z = out[i].normal.z * -1.0f;
            }
        }
    } else if (a->shape_type() == 1) {
        if (b->shape_type() == 0) {
            capsule_sphere_contact((capsule*)a, (sphere*)b, max, out, count);
        } else if (b->shape_type() == 1) {
            capsule_capsule_contact((capsule*)a, (capsule*)b, max, out, count);
        } else {
            int first = *count;
            shape_contacts(b, a, max, out, count);
            for (int i = first; i < *count && i < max; i++) {
                out[i].normal.x = out[i].normal.x * -1.0f;
                out[i].normal.y = out[i].normal.y * -1.0f;
                out[i].normal.z = out[i].normal.z * -1.0f;
            }
        }
    } else if (a->shape_type() == 2) {
        if (b->shape_type() == 0) {
            polygon_sphere_contact((face*)a, (sphere*)b, max, out, count);
        } else if (b->shape_type() == 1) {
            polygon_capsule_contact((face*)a, (capsule*)b, max, out, count);
        } else {
            b->shape_type();
        }
    }
}
// @ 0x40bfe0
int shape_contact_count(shape_base* a, shape_base* b, int max, contact* out)
{
    int n = 0;
    shape_contacts(a, b, max, out, &n);
    return n;
}
// @ 0x40c010
shape_base* sphere::clone() { return new sphere(*this); }
// @ 0x40c030
sphere::sphere(const sphere& o) : shape_base(o), local_center(o.local_center), center(o.center), radius(o.radius) {}

// ---- std::string members and std exceptions placed here by the linker (0x40c0c0..0x40c630) ----------------------
typedef std::string str;
// @ 0x40c0c0
template str& str::erase(size_t, size_t);
// @ 0x40c140
template void str::_Copy(size_t, size_t);
// 0x40c1b1: catch(...) funclet of _Copy (retry with _Newres = _Newsize), emitted by cl inside _Copy
// 0x40c257: catch(...) funclet of _Copy ({ _Tidy(true); throw; }), emitted by cl inside _Copy
// @ 0x40c290
template str::~basic_string();
// @ 0x40c360
template str& str::assign(const str&, size_t, size_t);
// @ 0x40c450
template str& str::assign(const char*, size_t);
// @ 0x40c540
template str::basic_string(const char*);

// copies of <stdexcept>, out-of-line so that they are emitted
class logic_error : public std::exception {
public:
    explicit logic_error(const std::string& m);
    logic_error(const logic_error&);
    virtual ~logic_error();
    virtual const char* what() const;               // 0x40c300 (libcpmt)
private:
    std::string _Str;
};
class length_error : public logic_error {
public:
    explicit length_error(const std::string& m);
    length_error(const length_error&);
    virtual ~length_error() {}                      // ??_G at 0x40c5c0
};
// @ 0x40c2c0
logic_error::~logic_error() {}
// 0x40c310
// (scalar deleting destructor of logic_error, vtable 0x59c978 slot 0: generated; forced into the object here)
void* force_vtables(const std::string& s) { logic_error* p = new logic_error(s); delete p; length_error* q = new length_error(s); delete q; return 0; }
// @ 0x40c580
logic_error::logic_error(const std::string& m) : _Str(m) {}
// @ 0x40c5c0
// (length_error scalar deleting destructor, vtable 0x59c984 slot 0: generated from the inline ~length_error)
// @ 0x40c5f0
logic_error::logic_error(const logic_error& o) : std::exception(o), _Str(o._Str) {}
// @ 0x40c630
length_error::length_error(const length_error& o) : logic_error(o) {}

// ---- manifold (0x40c670..0x40cb60) -----------------------------------------------------------------------------
// @ 0x40c6c0
string manifold::type_name() const { return "manifold"; }
// @ 0x40c6e0
void manifold::save(stream_base* s)
{
    int n = verts.size();
    s->w_int(&n);
    for (int i = 0; i < n; i++) {
        float* p = &verts[i].x;
        s->w_float(p).w_float(p + 1).w_float(p + 2);
    }
    int m = faces.size();
    s->w_int(&m);
    for (int i = 0; i < m; i++) {
        s->w_int(&faces[i].a);
        s->w_int(&faces[i].flags);
        s->w_int(&faces[i].n);
        for (int j = 0; j < faces[i].n; j++)
            s->w_int(&faces[i].idx[j]);
    }
}
// @ 0x40c820
void manifold::scale(const float* s)
{
    for (unsigned i = 0; i < verts.size(); i++) {
        wverts[i].x *= s[0];
        wverts[i].y *= s[1];
        wverts[i].z *= s[2];
    }
}
// @ 0x40c880
void manifold::transform(const pos* m)
{
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4& p = wverts[i];
        float y = m->s.y * p.x; y += m->f.y * p.y; y += m->u.y * p.z; y += m->o.y;
        float z = m->s.z * p.x; z += m->f.z * p.y; z += m->u.z * p.z + m->o.z;
        vector4 t(m->f.x * p.y + m->u.x * p.z + p.x * m->s.x + m->o.x, y, z);
        verts[i] = t;
    }
}
// @ 0x40c940
vector4* manifold::get_min(vector4* out)
{
    vector4 m(1e6f, 1e6f, 1e6f);
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4* v = &verts[i];
        float a = v->x;
        if (!(m.x < a)) m.x = a;
        float b = v->y;
        if (!(m.y < b)) m.y = b;
        float c = v->z;
        if (!(m.z < c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40ca00
vector4* manifold::get_max(vector4* out)
{
    vector4 m(-1e6f, -1e6f, -1e6f);
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4* v = &verts[i];
        float a = v->x;
        if (!(m.x > a)) m.x = a;
        float b = v->y;
        if (!(m.y > b)) m.y = b;
        float c = v->z;
        if (!(m.z > c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40cac0
void manifold::set_m8(int v)
{
    m8 = v;
    int n = faces.size();
    for (int i = 0; i < n; i++) faces[i].flags = v;
}
// @ 0x40cb00
void manifold::or_m8(int v)
{
    m8 |= v;
    int n = faces.size();
    for (int i = 0; i < n; i++) faces[i].flags = faces[i].flags | v;
}
// @ 0x40cb50
shape_base* manifold::clone() { return build_composite(); }
// @ 0x40cb60
shape_base* manifold::build_composite()
{
    composite* c = new composite;
    c->count = faces.size();
    c->bodies = new shape_base*[c->count];
    for (int i = 0; i < c->count; i++) {
        face* f = new face;
        c->bodies[i] = f;
        c->bodies[i]->m4 = faces[i].m4;
        c->bodies[i]->m8 = faces[i].flags;
        ((face*)c->bodies[i])->count = faces[i].n;
        for (int j = 0; j < faces[i].n; j++)
            ((face*)c->bodies[i])->v[j] = verts[faces[i].idx[j]];
    }
    pos ident;
    c->transform(identity_pos(&ident));
    c->set_mc(mc);
    c->m8 = m8;
    return c;
}
// @ 0x40cd00
__declspec(noinline) pos* __cdecl identity_pos(pos* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    mat3 m;
    *(mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}
// @ 0x40cd60
__declspec(noinline) mat3* __cdecl identity3(mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}

// ---- composite (0x40ce10..0x40d320) ----------------------------------------------------------------------------
// @ 0x40ce10
shape_base* composite::clone() { return new composite(*this); }
// @ 0x40ce50
void composite::copy_from(const shape_base* o)
{
    const composite* c = (const composite*)o;
    count = c->count;
    bodies = new shape_base*[count];
    for (int i = 0; i < count; i++) bodies[i] = c->bodies[i]->clone();
    m4 = c->m4;
    m8 = c->m8;
}
// @ 0x40ceb0
string composite::type_name() const { return "composite_body"; }
// @ 0x40ced0
void composite::save(stream_base* s)
{
    string name;
    s->w_int(&count);
    for (int i = 0; i < count; i++) {
        if (bodies[i]->shape_type() == 0) s->w_cstr("sphere");
        else if (bodies[i]->shape_type() == 1) s->w_cstr("capsule");
        else if (bodies[i]->shape_type() == 2) s->w_cstr("face");
        else if (bodies[i]->shape_type() == 4) s->w_cstr("manifold");
        else mb_error("unrecognized body \n");
        bodies[i]->save(s);
    }
}
// @ 0x40cfb0
void composite::draw()
{
    for (int i = 0; i < count; i++) bodies[i]->draw();
}
// @ 0x40cfe0
void composite::scale(const float* s)
{
    if (s[0] < 0.999999f || s[0] > 1.000001f ||
        s[1] < 0.999999f || s[1] > 1.000001f ||
        s[2] < 0.999999f || s[2] > 1.000001f) {
        for (int i = 0; i < count; i++) bodies[i]->scale(s);
    }
}
// @ 0x40d070
void composite::transform(const pos* m)
{
    for (int i = 0; i < count; i++) bodies[i]->transform(m);
}
// @ 0x40d0a0
void composite::set_mc(const int& v)
{
    for (int i = 0; i < count; i++) bodies[i]->set_mc(v);
}
// @ 0x40d0d0
void composite::set_m8(int v)
{
    m8 = v;
    for (int i = 0; i < count; i++) bodies[i]->set_m8(v);
}
// @ 0x40d100
void composite::or_m8(int v)
{
    m8 |= v;
    for (int i = 0; i < count; i++) bodies[i]->or_m8(v);
}
// @ 0x40d160
composite::~composite()
{
    for (int i = 0; i < count; i++)
        if (bodies[i]) delete bodies[i];
    delete[] bodies;
}
// @ 0x40d1a0
vector4* composite::get_min(vector4* out)
{
    vector4 m(1e6f, 1e6f, 1e6f);
    for (int i = 0; i < count; i++) {
        vector4 t;
        vector4* p = bodies[i]->get_min(&t);
        float a = p->x;
        if (!(m.x < a)) m.x = a;
        float b = p->y;
        if (!(m.y < b)) m.y = b;
        float c = p->z;
        if (!(m.z < c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40d260
vector4* composite::get_max(vector4* out)
{
    vector4 m(-1e6f, -1e6f, -1e6f);
    for (int i = 0; i < count; i++) {
        vector4 t;
        vector4* p = bodies[i]->get_max(&t);
        float a = p->x;
        if (!(m.x > a)) m.x = a;
        float b = p->y;
        if (!(m.y > b)) m.y = b;
        float c = p->z;
        if (!(m.z > c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40d340
manifold::~manifold() {}
// @ 0x40d3a0
void manifold::copy_from(const shape_base* o)
{
    const manifold* m = (const manifold*)o;
    verts = m->verts;
    faces = m->faces;
    m8 = m->m8;
}

// ---- std::vector<vector4> / std::vector<mface> (0x40d3d0..0x40e3bb) ------------------------------------------------
// std::_Uninit_copy (nonscalar) with the allocator's construct() expanded
template<class T> __declspec(noinline) T* uninit_copy(T* first, T* last, T* dest, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    T* next = dest;
    try {
        for (; first != last; ++dest, ++first) {
            ::new ((void*)dest) T(*first);
        }
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// std::_Uninitialized_copy
template<class T> static __forceinline T* ucopy(T* first, T* last, T* dest, std::allocator<T>& al)
{
    return uninit_copy(first, last, dest, al, std::_Ptr_cat(first, dest));
}

// std::vector::_Ucopy
template<class T> __forceinline T* vec<T>::ucopy_in(T* f, T* l, T* p) { return ucopy(f, l, p, al); }

// std::vector::operator=
template<class T> __declspec(noinline) vec<T>& vec<T>::operator=(const vec<T>& r)
{
    if (this != &r) {
        if (r.size() == 0) clear();
        else if (r.size() <= size()) {
            copy_range(r.first, r.last, first);
            last = first + r.size();
        } else if (r.size() <= capacity()) {
            T* p = r.first + size();
            copy_range(r.first, p, first);
            last = ucopy_in(p, r.last, last);
        } else {
            if (first != 0) operator delete(first);
            if (buy(r.size())) last = ucopy_in(r.first, r.last, first);
        }
    }
    return *this;
}
// @ 0x40d3d0
template vec<vector4>& vec<vector4>::operator=(const vec<vector4>&);
// @ 0x40d550
template vector4* uninit_copy<vector4>(vector4*, vector4*, vector4*, std::allocator<vector4>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x40d5e0
template vec<mface>& vec<mface>::operator=(const vec<mface>&);
// @ 0x40d750
template mface* uninit_copy<mface>(mface*, mface*, mface*, std::allocator<mface>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x40d7d0
void composite::load(stream_base* s)
{
    string name;
    s->r_int(&count);
    bodies = new shape_base*[count];
    for (int i = 0; i < count; i++) {
        s->r_tstr(&name);
        if (name == "sphere") bodies[i] = new sphere;
        else if (name == "capsule") bodies[i] = new capsule;
        else if (name == "face") bodies[i] = new face;
        else if (name == "manifold") bodies[i] = new manifold;
        else mb_error("unrecognized body  %s \n", ((string_view_&)name).get());
        bodies[i]->load(s);
    }
}
// @ 0x40d9d0
void manifold::load(stream_base* s)
{
    int n; int m; int i; int j;
    s->r_int(&n);
    verts.resize(n);
    wverts.resize(n);
    for (i = 0; i < n; i++) {
        float* p = &wverts[i].x;
        s->r_float(p).r_float(p + 1).r_float(p + 2);
        verts[i] = wverts[i];
    }
    s->r_int(&m);
    faces.resize(m);
    for (i = 0; i < m; i++) {
        s->r_int(&faces[i].a);
        s->r_int(&faces[i].flags);
        s->r_int(&faces[i].n);
        for (j = 0; j < faces[i].n; j++)
            s->r_int(&faces[i].idx[j]);
    }
}

// std::_Uninit_fill_n (nonscalar) with the allocator's construct() expanded
template<class T> __declspec(noinline) void uninit_fill_n(T* first, unsigned count, const T& val, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    T* next = first;
    try {
        for (; 0 < count; --count, ++first)
            ::new ((void*)first) T(val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}
// std::_Uninitialized_fill_n
template<class T> static __forceinline void ufill_n(T* first, unsigned count, const T& val, std::allocator<T>& al)
{
    uninit_fill_n(first, count, val, al, std::_Ptr_cat(first, first));
}
// std::vector::_Ufill
template<class T> __forceinline T* vec<T>::ufill(T* p, unsigned count, const T& val)
{
    ufill_n(p, count, val, al);
    return p + count;
}

// the mface erase lives in a later object
template<> vec<mface>::iter vec<mface>::erase(iter f, iter l);                            // 0x40fd40

// std::vector::resize(n, val)
template<class T> __declspec(noinline) void vec<T>::resize(unsigned n, T val)
{
    if (size() < n)
        insert_n(end_(), n - size(), val);
    else if (n < size())
        erase(begin() + n, end_());
}
// std::vector::erase(first, last)
template<class T> __declspec(noinline) typename vec<T>::iter vec<T>::erase(iter f, iter l)
{
    if (f.cur != l.cur) {
        T* p = copy_range(l.cur, last, f.cur);
        last = p;
    }
    return f;
}
// std::vector::_Insert_n
template<class T> __declspec(noinline) void vec<T>::insert_n(iter where_, unsigned count, const T& val)
{
    T tmp = val;
    unsigned cap = capacity();
    T* where = where_.cur;
    if (count == 0)
        ;
    else if (al.max_size() - size() < count)
        xlen();
    else if (cap < size() + count) {
        cap = al.max_size() - cap / 2 < cap ? 0 : cap + cap / 2;
        if (cap < size() + count)
            cap = size() + count;
        T* newvec = al.allocate(cap);
        T* ptr = newvec;
        try {
            ptr = ucopy_in(first, where, newvec);
            ptr = ufill(ptr, count, tmp);
            ucopy_in(where, last, ptr);
        } catch (...) {
            al.deallocate(newvec, cap);
            throw;
        }
        count += size();
        if (first != 0)
            al.deallocate(first, end - first);
        end = newvec + cap;
        last = newvec + count;
        first = newvec;
    } else if ((unsigned)(last - where) < count) {
        ucopy_in(where, last, where + count);
        try {
            ufill(last, count - (last - where), tmp);
        } catch (...) {
            throw;
        }
        last += count;
        fill_range(where, last - count, tmp);
    } else {
        T* oldend = last;
        last = ucopy_in(oldend - count, oldend, last);
        copy_backward_range(where, oldend - count, oldend);
        fill_range(where, where + count, tmp);
    }
}
// @ 0x40db80
template void vec<vector4>::resize(unsigned, vector4);
// @ 0x40dbf0
template vec<vector4>::iter vec<vector4>::erase(iter, iter);
// @ 0x40dc50
template void vec<vector4>::insert_n(iter, unsigned, const vector4&);
// @ 0x40df60
template void uninit_fill_n<vector4>(vector4*, unsigned, const vector4&, std::allocator<vector4>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x40dff0
template void vec<mface>::resize(unsigned, mface);
// @ 0x40e060
template void vec<mface>::insert_n(iter, unsigned, const mface&);
// @ 0x40e350
template void uninit_fill_n<mface>(mface*, unsigned, const mface&, std::allocator<mface>&, std::_Nonscalar_ptr_iterator_tag);

// ---- float / colour helpers (0x40e3d0..0x40e4e0) ---------------------------------------------------------------
// @ 0x40e3d0
float clamp(const float* x, const float* lo, const float* hi)
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
// @ 0x40e400
// uniform random in [0, 15817 * 6.3223e-5) = [0, 1)
float frand()
{
    return (float)(rand() % 15817) * (1.0f / 15817.0f);
}
// @ 0x40e420
void vec3::add(const vec3& o)
{
    x = o.x + x;
    y = o.y + y;
    z = o.z + z;
}

namespace rgl {

static inline unsigned min255(unsigned v) { return v < 255 ? v : 255; }
// @ 0x40e440
// pack (r, g, b, a) floats in 0..1 into 0xAARRGGBB
unsigned pack_color(const vector4* c)
{
    unsigned a = (unsigned)(c->w * 256.0f);
    unsigned r = (unsigned)(c->x * 256.0f);
    unsigned g = (unsigned)(c->y * 256.0f);
    unsigned b = (unsigned)(c->z * 256.0f);
    return (((min255(a) << 8 | min255(r)) << 8) | min255(g)) << 8 | min255(b);
}
// @ 0x40e4e0
// unpack 0xAARRGGBB into (r, g, b, a) floats in 0..1
vector4 unpack_color(const unsigned* c)
{
    vector4 v;
    const unsigned char* b = (const unsigned char*)c;
    v.w = b[3] * (1.0f / 255.0f);
    v.x = b[2] * (1.0f / 255.0f);
    v.y = b[1] * (1.0f / 255.0f);
    v.z = (*c & 0xff) * (1.0f / 255.0f);
    return v;
}

// ---- matrix axis rotations and product (0x40e570..0x40e870) -----------------------------------------------------
// Inline vector4 helpers as the original's object had them (scalar passed by const reference, so a double
// sin/cos result is stored as a float temporary per use).
static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub2(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x40e570
void matrix::rotate_about_s(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 nf, nu;
    nf = vadd(vmul(f, cos((double)*angle)), vmul(u, sn));
    nu = vsub2(vmul(u, cos((double)*angle)), vmul(f, sn));
    u = nu;
    f = nf;
}
// @ 0x40e670
void matrix::rotate_about_f(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nu;
    ns = vsub2(vmul(s, cos((double)*angle)), vmul(u, sn));
    nu = vadd(vmul(u, cos((double)*angle)), vmul(s, sn));
    s = ns;
    u = nu;
}
// @ 0x40e770
void matrix::rotate_about_u(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = vadd(vmul(s, cos((double)*angle)), vmul(f, sn));
    nf = vsub2(vmul(f, cos((double)*angle)), vmul(s, sn));
    s = ns;
    f = nf;
}
// @ 0x40e870
// matrix product returned by value (hidden return pointer): rotation rows via pos::rot3 (0x40a920), origin =
// this->transform_point(m.o) with the transform inlined
matrix matrix::operator*(const matrix& m) const
{
    struct { mat3 r; vector4 o; } t;
    t.o = ((const pos*)this)->transform_point_inl(m.o);
    t.r = ((const pos*)this)->rot3(m);
    return *(matrix*)&t;
}

} // namespace rgl
