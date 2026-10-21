// Slice s00461280: the renderer's default device-state reset and view-matrix setup, the lighting toggle, a triangle
// normal helper, signed-colour packing, a clock-step setter, the wind object (noise-driven direction scaling) and a
// std::copy_backward instantiation for 24-byte elements. Names are provisional. Flags: default /O2 /MT /Gy /TP.
//
// Codegen levers found here (see the report): functions returning a vector4/matrix/D3DX matrix by value take a
// hidden return pointer (eax holds it at `ret`), `rel = f()` (assignment, not init) copies a returned temporary,
// and a named local in a nested block shares its stack slot with compiler temporaries.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <stdlib.h>
#include "rgl.h"

using rgl::vector4;

struct elem24 { int a[6]; };

// ---- globals -----------------------------------------------------------------------------------
extern __int64 g_time[];                    // 0x5e2948 clock table (1e-5 s ticks)
extern __int64 g_dt[];                      // 0x5e2970 per-clock last step (ticks)
extern float g_step_tbl[];                  // 0x5a9c20 per-clock step (seconds)
extern int g_use_shaders;                   // 0x5e28fc
extern int g_use_wbuffer;                   // 0x5a9fdc
extern int g_aniso_filter;                  // 0x5aa040
extern float g_gamma;                       // 0x5aa088
extern ID3DXEffect* g_effect;               // 0x5e293c
extern unsigned g_stage_arg[8];             // 0x5a9c00 D3DTA_* by 3-bit code

struct V4 { float x, y, z, w; };
struct mat4 {
    V4 s, f, u, o;
    mat4 operator*(const mat4& b) const;    // 0x40e870
};
struct pos : rgl::matrix {
    rgl::matrix relative(const rgl::matrix& o) const;       // 0x4329d0 (returns by value)
};
pos __cdecl identity_pos();                 // 0x40cd00 (hidden return pointer; returns it)
void __cdecl noise_fractal(float a, float b, int c, int d, const float* in, float* out);   // 0x414fc0

// ---- renderer (global object at 0xa17910) ------------------------------------------------------------------
struct renderer {
    char pad000[0x500];
    int f500;                       // 0x500
    char pad504;                    // 0x504
    char b505;                      // 0x505
    bool b506;                      // 0x506  D3DRS_LIGHTING cache
    char light_on[64];              // 0x507
    char b547, b548, b549, b54a, b54b;
    int f54c[3];                    // 0x54c
    int f558;                       // 0x558
    int stage_color[3];             // 0x55c
    int stage_alpha[3];             // 0x568
    int stage_xform[3];             // 0x574
    int stage_texgen[3];            // 0x580
    int blend_mode;                 // 0x58c
    int alpha_mode;                 // 0x590  alpha-test mode (see slice s00461bd0)
    unsigned tex_factor;            // 0x594
    unsigned tex_factor2;           // 0x598
    int stage_samp[3];              // 0x59c
    char pad5a8[0x5b4 - 0x5a8];
    int f5b4, f5b8, f5bc, f5c0;
    int f5c4, f5c8, f5cc, f5d0, f5d4, f5d8, f5dc;
    char pad5e0[0x5ec - 0x5e0];
    int f5ec, f5f0, f5f4, f5f8, f5fc, f600;
    float spec[3];                  // 0x604
    int f610;
    char pad614[4];
    IDirect3DDevice9* dev;          // 0x618
    char pad61c[0x688 - 0x61c];
    int max_aniso;                  // 0x688
    char pad68c[0x754 - 0x68c];
    D3DMATRIX view;                 // 0x754
    D3DMATRIX view2;                // 0x794

    void set_view(const pos* p);                                // 0x461630
    void reset_state();                                         // 0x4617b0
    void set_lighting(bool on);                                 // 0x461ba0
};

// @ 0x461280
elem24* __cdecl copy_backward24(elem24* f, elem24* l, elem24* d) { while (f != l) *--d = *--l; return d; }

// @ 0x4612d0
static inline vector4 sub_i(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline void cross_i(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}
vector4 tri_normal(const vector4* p)
{
    vector4 a = sub_i(p[1], p[0]);
    vector4 b = sub_i(p[2], p[0]);
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
    g_dt[*clock] = (__int64)(step * 100000.0f);
    g_time[*clock] += g_dt[*clock];
    g_step_tbl[*clock] = step;
}

// ---- wind / noise object (0x7c bytes) -----------------------------------------------------------------
// wind direction vector at +4 scaled by a fractal-noise factor driven by the game clock (clock 2, wrapped at 1e9
// ticks) plus a random per-instance phase.
struct wind {
    int f0;
    vector4 v4, v14, v24, v34, v44;
    float f54, f58, f5c, f60, f64, f68, f6c;
    float strength;                 // 0x70
    float noise_seed;               // 0x74
    int phase;                      // 0x78
    wind* init();                   // 0x461480
    vector4 sample();               // 0x461570 (hidden return pointer)
};

// @ 0x461480
wind* wind::init()
{
    vector4 z(0.0f, 0.0f, 0.0f);
    v4 = z; v14 = z; v24 = z; v34 = z;
    v44 = vector4(0.0f, 1.0f, 0.0f);
    f0 = 0;
    f54 = 0.0f; f58 = 0.0f; f5c = 1.0f; f60 = 0.0f; f64 = 0.0f; f68 = 0.0f; f6c = 0.0f;
    strength = 0.0f;
    noise_seed = 0.0f;
    phase = (unsigned)rand() % 0x3f9;
    return this;
}

// @ 0x461570
vector4 wind::sample()
{
    vector4 in;
    in.y = 0.0f; in.z = 0.0f;
    in.x = (float)(g_time[2] % 1000000000) * 1e-5f + (float)phase;
    vector4 n;
    noise_fractal(noise_seed, 0.6f, 3, 0, &in.x, &n.x);
    float k = n.x * strength * 15.0f + 1.0f;
    n.x = k * v4.x; n.y = k * v4.y; n.z = k * v4.z;
    return n;
}

// ---- matrix conversion to the D3D layout -------------------------------------------------------
static inline void to_d3d(D3DMATRIX* out, const rgl::matrix& m)
{
    *(rgl::matrix*)out = m;
    out->_14 = 0.0f; out->_24 = 0.0f; out->_34 = 0.0f; out->_44 = 1.0f;
}

struct rot3 { vector4 s, f, u; };                       // 3x3 rotation, rows padded to 4 floats
struct frame4 {                                         // same layout as matrix / mat4
    rot3 rot; vector4 o;
    frame4(const rot3& r, const vector4& origin) { rot = r; o = origin; }
    frame4(const rgl::matrix& m) { *(rgl::matrix*)this = m; }
    frame4 operator*(const frame4& b) const;            // 0x40e870 (pos_mul)
};
// @ 0x461630
void renderer::set_view(const pos* p)
{
    to_d3d(&view, identity_pos());
    to_d3d(&view2, identity_pos());
    f500 = 0;
    rot3 r;
    r.s.x = 1.0f; r.s.y = 0.0f; r.s.z = 0.0f;
    r.f.x = 0.0f; r.f.y = 1.0f; r.f.z = 0.0f;
    r.u.x = 0.0f; r.u.y = 0.0f; r.u.z = 1.0f;
    {
        rgl::matrix rel;
        rel = p->relative(identity_pos());
        to_d3d(&view, *(rgl::matrix*)&(frame4(r, vector4(0.0f, 0.0f, 0.0f)) * *(frame4*)&rel));
    }
    dev->SetTransform(D3DTS_VIEW, &view);
}

// @ 0x4617b0
void renderer::reset_state()
{
    dev->SetRenderState(D3DRS_ZENABLE, 1);
    if (g_use_wbuffer) dev->SetRenderState(D3DRS_ZENABLE, D3DZB_USEW);
    alpha_mode = 0;
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    b549 = 0;
    dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    b54b = 1;
    dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    dev->SetRenderState(D3DRS_RANGEFOGENABLE, 1);
    b505 = 0;
    dev->SetRenderState(D3DRS_FOGENABLE, 0);
    b506 = 0;
    if (!g_use_shaders) {
        dev->SetRenderState(D3DRS_LIGHTING, 0);
        for (int i = 0; i < 64; i++) {
            light_on[i] = 0;
            dev->LightEnable(i, 0);
        }
    }
    b548 = 0;
    dev->SetRenderState(D3DRS_NORMALIZENORMALS, 0);
    dev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, 1);
    dev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 0);
    f558 = 0;
    dev->SetPixelShader(0);
    for (int i = 0; i < 3; i++) {
        dev->SetTexture(i, 0);
        f54c[i] = 0;
        stage_color[i] = -1;
        stage_alpha[i] = -1;
        if (g_aniso_filter) {
            dev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
            dev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
        } else {
            dev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
            dev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        }
        dev->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        int a = max_aniso;
        if (a >= 5) a = 5;
        dev->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, a);
        stage_xform[i] = 0;
        dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        stage_texgen[i] = 0;
        dev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, 0);
        dev->SetSamplerState(i, D3DSAMP_ADDRESSU, 1);
        dev->SetSamplerState(i, D3DSAMP_ADDRESSV, 1);
        stage_samp[i] = 0;
    }
    dev->SetRenderState(D3DRS_SPECULARENABLE, 0);
    b547 = 0;
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    f5c0 = -1;
    f5b4 = -1;
    tex_factor = 0x12345678;
    tex_factor2 = 0x12345678;
    blend_mode = 0;
    f5bc = 2;
    f5f0 = 0xc49a26d2;
    f5f4 = 0x4a4c33fd;
    f5f8 = 0;
    f5fc = 0;
    f600 = 0xcf09d339;
    f5ec = 0xc64060db;
    *(int*)&spec[0] = 0xc36a3bf6;
    *(int*)&spec[1] = 0xc36a3bf6;
    *(int*)&spec[2] = 0xc36a3bf6;
    f610 = 0;
    if (g_use_shaders) {
        f5c4 = -457; f5c8 = -457; f5cc = -457; f5d0 = -457; f5d4 = -457;
        f5d8 = 0;
        f5dc = 0xffbd6370;
        g_effect->SetInt("iLightPointCount", 0);
        f5b8 = 0xffffcfdf;
        float inv = 1.0f / g_gamma;
        D3DXVECTOR4 gamma(g_gamma, g_gamma, g_gamma, g_gamma);
        D3DXVECTOR4 ginv(inv, inv, inv, inv);
        g_effect->SetVector("output_gamma", &gamma);
        g_effect->SetVector("output_gamma_inv", &ginv);
    }
}

// @ 0x461ba0
void renderer::set_lighting(bool on)
{
    if (b506 != on) {
        b506 = on;
        dev->SetRenderState(D3DRS_LIGHTING, on);
    }
}
