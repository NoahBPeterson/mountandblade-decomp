// Slice s0040a640: debug-draw / bounds helpers of the shape classes: capsule (vtable 0x54a540) draw / get_min /
// get_max, the wire-capsule tessellator on the render state object, matrix rotate helpers, and the polygon
// ("face", vtable 0x54a588) clone / copy ctor / get_min.  Class names provisional.
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
using namespace rgl;

// ---- matrix / render state ---------------------------------------------------------------------
struct mat3 { vector4 a, b, c; mat3() {}
    __forceinline mat3(const vector4& a_, const vector4& b_, const vector4& c_) : a(a_), b(b_), c(c_) {} };

struct pos : matrix {
    vector4 transform_point(const vector4& p) const;          // 0x408460 (s00408110)
    mat3 rot3(const matrix& m) const;                         // 0x40a920
    vector4 rot(const vector4& p) const;                      // 0x40a9c0
    vector4 rot_ext(const vector4& p) const;                  // 0x40a9c0 (declared apart: the callers see it as external)
};

struct renderer {
    matrix stack[20];                // 0x000
    int sp;                          // 0x500
    char pad504[0x618 - 0x504];
    IDirect3DDevice9* dev;           // 0x618
    char pad61c[0x794 - 0x61c];
    matrix cur;                      // 0x794 (world matrix)
    void apply(const matrix* m);                                                       // 0x40a870
    void draw_capsule(float a, float b, float r);                                      // 0x40aa20
    void tessellate(float* pts, int* n, float a, float b, float r, int stacks, int slices, int mode);   // 0x40ab10
};
extern renderer g_render;   // 0xa17910

// ---- shapes ------------------------------------------------------------------------------------
namespace rgl {
struct shape {                       // vtable 0x54a4b8 (abstract base, 0x20 bytes)
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape();
    virtual void pure1() = 0;
    virtual void pure2() = 0;
    virtual void pure3() = 0;
    virtual void pure4() = 0;
};
}

void __cdecl vmin_ext(vector4* out, const vector4* a, const vector4* b);   // 0x40af80 (same function as vmin below; declared apart so the compiler sees it as external)
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}
static inline void set3(float* o, const float& x, const float& y, const float& z) { o[0] = x; o[1] = y; o[2] = z; }
static inline float fmin_(float a, float b) { return a < b ? a : b; }
static inline float fmax_(float a, float b) { return a > b ? a : b; }

struct capsule {                     // vtable 0x54a540, 0x64 bytes
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    vector4 v20, v30, v40, v50;
    float radius;                    // 0x60
    virtual ~capsule();
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void draw();             // slot 6   0x40a640
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual void get_min(float* out);   // slot 10  0x40ad80
    virtual void get_max(float* out);   // slot 11  0x40ade0
};

struct shape_base {                  // header of the polygon shape (inline ctor)
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};

struct face : shape_base {                // vtable 0x54a588, 0xb4 bytes
    vector4 w[4];                    // 0x20 world-space vertices
    vector4 v[4];                    // 0x60 local vertices
    vector4 normal;                  // 0xa0
    int count;                       // 0xb0
    __declspec(noinline) face(const face& o);   // 0x40ae70
    virtual ~face();
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual face* clone();           // slot 5   0x40ae40
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4 get_min();       // slot 10  0x40aef0  (by value: hidden result pointer)
};

// @ 0x40a640
void capsule::draw()
{
    vector4 d(v30.x - v20.x, v30.y - v20.y, v30.z - v20.z);
    float half = d.length() * 0.5f;
    normalize(d, d);
    vector4 e;
    vcross(e, d, vector4(1.0f, 0.0f, 0.0f));
    normalize(e, e);
    vector4 f;
    vcross(f, e, d);
    matrix m;
    m.s = f;
    m.f = e;
    m.u = d;
    float sx = v30.x + v20.x, sy = v30.y + v20.y, sz = v30.z + v20.z;
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
    t.o = ((const pos*)c)->transform_point(m->o);
    t.r = ((const pos*)c)->rot3(*m);
    *c = *(matrix*)&t;
    c->s.w = 0.0f; c->f.w = 0.0f; c->u.w = 0.0f; c->o.w = 1.0f;
    dev->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)c);
}
// @ 0x40a920
mat3 pos::rot3(const matrix& m) const { return mat3(rot_ext(m.s), rot_ext(m.f), rot_ext(m.u)); }
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
void capsule::get_min(float* out)
{
    set3(out, fmin_(v20.x, v30.x) - radius, fmin_(v20.y, v30.y) - radius, fmin_(v20.z, v30.z) - radius);
}
// @ 0x40ade0
void capsule::get_max(float* out)
{
    set3(out, fmax_(v20.x, v30.x) + radius, fmax_(v20.y, v30.y) + radius, fmax_(v20.z, v30.z) + radius);
}
// @ 0x40ae40
face* face::clone() { return new face(*this); }
// @ 0x40ae70
face::face(const face& o) : count(o.count)
{
    for (int i = 0; i < count; i++) v[i] = o.v[i];
}
// @ 0x40aef0
vector4 face::get_min()
{
    vector4 t = w[0];
    for (int i = 1; i < count; i++) vmin_ext(&t, &t, &w[i]);
    return t;
}
// @ 0x40af80
void __cdecl vmin(vector4* out, const vector4* a, const vector4* b)
{
    out->x = fmin_(a->x, b->x);
    out->y = fmin_(a->y, b->y);
    out->z = fmin_(a->z, b->z);
}
