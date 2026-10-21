// Slice s00467bf0: std::fill / copy_backward / allocator::construct / _Uninit_copy instantiations for 24-, 52-,
// 56- and 100-byte elements (the missile / prop / particle vector types), the transform tween update of a prop
// (0x467c50), two Missile members (0x467f80 speed scale, 0x467fb0 init), per-component vector min/max (0x468300,
// 0x468360, AABB extend helpers), and the texture-stage binder of a material slot (0x4683c0 / 0x468440).
// Type names are provisional. Flags: /O2 /MT /Gy /TP /EHa for the STL helpers (see s0040f300), default for the rest.
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <new>
#include "rgl.h"
#include "tstring.h"

using rgl::vector4;
using rgl::matrix;

struct elem24  { int a[6]; };
struct elem52  { int a[13]; };
struct particle { int a[25]; };      // 100 bytes (see s0040ea00)
struct elem56  { tstring name; int a[10]; };    // 0x38

// ---- fill / copy_backward -------------------------------------------------------------------
// @ 0x467bf0
void __cdecl fill52(elem52* f, elem52* l, const elem52& v) { for (; f != l; ++f) *f = v; }
// @ 0x467c20
elem52* __cdecl copy_backward52(elem52* f, elem52* l, elem52* d) { while (f != l) *--d = *--l; return d; }
// @ 0x468050
elem56* __cdecl copy56(elem56* f, elem56* l, elem56* d) { for (; f != l; ++f, ++d) *d = *f; return d; }
// @ 0x4680c0
elem56* __cdecl copy_backward56(elem56* f, elem56* l, elem56* d) { while (f != l) *--d = *--l; return d; }

// ---- allocator::construct; _Uninit_copy ------------------------------------------------------
struct alloc24 { void construct(elem24* p, const elem24& v); void destroy(elem24* p) { p->~elem24(); } };      // 0x468130
struct alloc100 { void construct(particle* p, const particle& v); void destroy(particle* p) { p->~particle(); } };   // 0x468170
struct alloc52 { void construct(elem52* p, const elem52& v); void destroy(elem52* p) { p->~elem52(); } };     // 0x468190

struct alloc24_def { void construct(elem24* p, const elem24& v); };
struct alloc100_def { void construct(particle* p, const particle& v); };
struct alloc52_def { void construct(elem52* p, const elem52& v); };
// @ 0x468130
void alloc24_def::construct(elem24* p, const elem24& v) { new ((void*)p) elem24(v); }
// @ 0x468170
void alloc100_def::construct(particle* p, const particle& v) { new ((void*)p) particle(v); }
// @ 0x468190
void alloc52_def::construct(elem52* p, const elem52& v) { new ((void*)p) elem52(v); }

// @ 0x4681b0
elem24* __cdecl uninit_copy24(elem24* f, elem24* l, elem24* d, alloc24& al, int tag)
{
    elem24* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x468220
particle* __cdecl uninit_copy100(particle* f, particle* l, particle* d, alloc100& al, int tag)
{
    particle* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x468290
elem52* __cdecl uninit_copy52(elem52* f, elem52* l, elem52* d, alloc52& al, int tag)
{
    elem52* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// ---- timed transform tween of a scene prop ------------------------------------------------------
extern __int64 g_ticks[];       // 0x5e2948: 64-bit clocks, 100 kHz units

struct PropEnt {                // the scene entity the prop drives (vtable slot 23 takes the new frame)
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22();
    virtual void set_frame(const matrix* m, const void* extra);     // slot 23 (+0x5c)
};

struct Prop {
    char pad0[0x18];
    matrix cur;                 // +0x18
    char pad58[0xd8 - 0x58];
    matrix to;                  // +0xd8  target frame
    matrix from;                // +0x118 start frame
    char extra[0x10];           // +0x158
    int state;                  // +0x168  (-1 = idle)
    float duration;             // +0x16c
    int clock;                  // +0x170
    char pad174[4];
    __int64 start;              // +0x178
    float last_t;               // +0x180
    char pad184[0x1b4 - 0x184];
    PropEnt* ent;               // +0x1b4
    void update(int unused);    // 0x467c50
};
inline vector4 vscale(const vector4& v, float f) { return vector4(v.x * f, v.y * f, v.z * f); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
// @ 0x467c50
void Prop::update(int unused)
{
    if (state >= 0) {
        __int64 dt = g_ticks[clock] - start;
        float t = (dt * 1e-5f) / duration;
        if (t > 1.0f) {
            cur = to;
            state = -1;
            cur.orthonormalize();
            ent->set_frame(&cur, extra);
        }
        if (state >= 0 && t != last_t) {
            matrix m;
            m.o = vadd(vscale(to.o, t), vscale(from.o, 1.0f - t));
            m.s = vadd(vscale(to.s, t), vscale(from.s, 1.0f - t));
            m.f = vadd(vscale(to.f, t), vscale(from.f, 1.0f - t));
            m.u = vadd(vscale(to.u, t), vscale(from.u, 1.0f - t));
            m.orthonormalize();
            cur = m;
            if (ent) ent->set_frame(&cur, extra);
        }
        last_t = t;
    }
}

// ---- Missile (pool record, 0xb0 bytes; see s0044bc30) -------------------------------------------------
struct ItemKind {               // 0x238 bytes, array at 0x5e29d0
    char pad0[0x20c];
    int f20c;
    char pad210[0x238 - 0x210];
};
extern ItemKind* g_items;       // 0x5e29d0
struct Mission { char pad[0x6654]; float cam[3]; };     // +0x6654 camera position
extern Mission* g_mission;      // 0x5e292c

struct Missile {
    char pad0[0x20];
    vector4 p20;                // +0x20
    char pad30[0x40 - 0x30];
    vector4 p40;                // +0x40
    char pad50[4];
    float f54;                  // +0x54
    char pad58[0x68 - 0x58];
    int f68;                    // +0x68
    char pad6c[4];
    int clock;                  // +0x70
    char pad74[4];
    __int64 t0;                 // +0x78
    char pad80[8];
    int ammo_item;              // +0x88
    char pad8c[4];
    char f90;                   // +0x90
    char pad91[3];
    float f94;                  // +0x94
    char pad98[4];
    char f9c;                   // +0x9c
    char pad9d[3];
    int fa0;                    // +0xa0
    int fa4;                    // +0xa4
    float ammo_scale();         // 0x467f80
    void init();                // 0x467fb0
};
// @ 0x467f80
float Missile::ammo_scale()
{
    if (ammo_item >= 0)
        return (float)g_items[ammo_item].f20c * 0.01f;
    return 0.8f;
}
// @ 0x467fb0
void Missile::init()
{
    f54 = 0.01f;
    f68 = 0;
    t0 = g_ticks[clock];
    f90 = 0;
    fa0 = 0;
    f94 = 1.0f;
    fa4 = -1;
    f9c = 1;
    vector4 d(g_mission->cam[0] - p20.x, g_mission->cam[1] - p20.y, g_mission->cam[2] - p20.z);
    if (0.1f < d.x * p40.x + (d.z * p40.z + d.y * p40.y))
        fa0 = 1;
}

// ---- component-wise max / min of two 3-vectors ---------------------------------------------------------
inline float fmax_(float x, float y) { return x > y ? x : y; }
inline float fmin_(float x, float y) { return x < y ? x : y; }
// @ 0x468300
void __cdecl vmax3(float* a, const float* b)
{
    a[0] = fmax_(a[0], b[0]);
    a[1] = fmax_(a[1], b[1]);
    a[2] = fmax_(a[2], b[2]);
}
// @ 0x468360
void __cdecl vmin3(float* a, const float* b)
{
    a[0] = fmin_(a[0], b[0]);
    a[1] = fmin_(a[1], b[1]);
    a[2] = fmin_(a[2], b[2]);
}

// ---- material texture stages ------------------------------------------------------------------------------
struct renderer {
    char pad[1];
    void set_stage_colorop(int stage, int v);    // 0x461bd0  SetTextureStageState(D3DTSS_COLOROP, v & 0x1f), cached at +0x55c
    void set_stage_alphaop(int stage, int v);    // 0x461c60  SetTextureStageState(D3DTSS_ALPHAOP, v & 0x1f), cached at +0x568
    void set_sampler_clamp(int stage, int v);    // 0x461d90  SetSamplerState ADDRESSU/V wrap or clamp (bits 0, 1), cached at +0x59c
};
extern renderer g_render;                // 0xa17910
extern IDirect3DDevice9* g_dev;          // 0xa17f28
extern IDirect3DBaseTexture9* g_tex_cache[];   // 0xa17e5c: texture bound per stage

struct Texture {
    char pad0[0x18];
    char loaded;                // +0x18
    char pad19[0x24 - 0x19];
    int flags;                  // +0x24  bits 24..27 animated, 20..21 mode
    int nframes;                // +0x28
    char pad2c[0x40 - 0x2c];
    IDirect3DBaseTexture9** frames;   // +0x40
    void bind(int stage, float t);    // 0x4683c0
};
// @ 0x4683c0
void Texture::bind(int stage, float t)
{
    if (loaded) {
        int idx = 0;
        if (flags & 0x0f000000)
            idx = (int)(nframes * t) % nframes;
        IDirect3DBaseTexture9* tex = frames[idx];
        if (g_tex_cache[stage] != tex) {
            g_dev->SetTexture(stage, tex);
            g_tex_cache[stage] = tex;
        }
        g_render.set_sampler_clamp(stage, (flags >> 20) & 3);
    }
}

struct Stage { int tex; int colorop; int alphaop; unsigned flags; int pad; };     // 0x14 bytes (flags: bit 8 = texture transform, low byte = texgen mode; see s0040f300)
struct material {
    char pad0[0x68];
    Stage stage[4];             // +0x68
    int nstages;                // +0xb8
    void bind(Texture** textures, const float* t);     // 0x468440
};
// @ 0x468440
void material::bind(Texture** textures, const float* t)
{
    int n = nstages;
    int last = -1;
    int k = 0;
    if (0 < n) {
        Stage* s = stage;
        for (; k < n; ++k, ++s) {
            last = k;
            if (s->tex != -1 && textures[s->tex])
                textures[s->tex]->bind(k, *t);
            g_render.set_stage_colorop(k, s->colorop);
            g_render.set_stage_alphaop(k, s->alphaop);
        }
    }
    g_render.set_stage_colorop(last + 1, 1);
    g_render.set_stage_alphaop(last + 1, 1);
}
