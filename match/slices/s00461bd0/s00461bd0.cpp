// Slice s00461bd0: renderer texture-stage / sampler / alpha-test state setters, material and mesh-part state
// application, projection-matrix helpers of the frustum, device vertex/index buffer restore after a device reset,
// a 2D cell-grid allocator and the setters of an envelope/fade parameter block. Names are provisional.
// Flags: default /O2 /MT /Gy /TP.
//
// Codegen levers found here: by-value returns (projection(), view_matrix(), to_clip()) are hidden-return-pointer
// functions (eax = the pointer at `ret`); `v = t; v.x = f(t.x)` lets cl keep v's fields in registers; a local
// declared once and assigned in every arm (`HRESULT hr;` in restore()) is tail-duplicated; a volatile re-read stops
// cl from CSE-ing a field load across a loop test (restore_all()).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <string.h>
#include <new>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- globals -----------------------------------------------------------------------------------
extern ID3DXEffect* g_effect;               // 0x5e293c
extern unsigned g_stage_arg[8];             // 0x5a9c00 D3DTA_* by 3-bit code
void mb_error(const char* fmt, ...);        // 0x42cc80

struct pos : matrix {
    matrix relative(const matrix& o) const;     // 0x4329d0 (returns by value)
};
pos* __cdecl identity_pos(pos* out);            // 0x40cd00 (returns out)

// ---- renderer (global object at 0xa17910) -------------------------------------------------------
struct renderer {
    char pad000[0x547];
    unsigned char b547, b548, b549, b54a, b54b;     // 0x547..0x54b: cached render-state flags
    char pad54c[0x558 - 0x54c];
    IDirect3DPixelShader9* cur_ps;                  // 0x558
    int stage_color[3];                             // 0x55c
    int stage_alpha[3];                             // 0x568
    char pad574[0x590 - 0x574];
    int alpha_mode;                                 // 0x590  0 off, 1 (ref 8), 2 (ref 0x88), 3 (ref 0xfb)
    char pad594[0x59c - 0x594];
    int stage_samp[3];                              // 0x59c
    char pad5a8[0x5ec - 0x5a8];
    float material_power;                           // 0x5ec
    char pad5f0[0x604 - 0x5f0];
    float spec[3];                                  // 0x604
    char pad610[0x618 - 0x610];
    IDirect3DDevice9* dev;                          // 0x618

    void set_stage_color(int stage, int flags);             // 0x461bd0
    void set_stage_alpha(int stage, int flags);             // 0x461c60
    void set_alpha_mode(int mode);                          // 0x461cf0
    void set_stage_sampler(int stage, unsigned flags);      // 0x461d90
    void set_material(float power, const float* specular);  // 0x461e10
    void set_specular(const float* c);                      // 0x461eb0
};
extern renderer g_render;                       // 0xa17910

// @ 0x461bd0
void renderer::set_stage_color(int stage, int flags)
{
    if (stage < 3 && stage_color[stage] != flags) {
        stage_color[stage] = flags;
        dev->SetTextureStageState(stage, D3DTSS_COLOROP, flags & 0x1f);
        dev->SetTextureStageState(stage, D3DTSS_COLORARG1, g_stage_arg[(flags >> 5) & 7]);
        dev->SetTextureStageState(stage, D3DTSS_COLORARG2, g_stage_arg[(flags >> 8) & 7]);
    }
}

// @ 0x461c60
void renderer::set_stage_alpha(int stage, int flags)
{
    if (stage < 3 && stage_alpha[stage] != flags) {
        stage_alpha[stage] = flags;
        dev->SetTextureStageState(stage, D3DTSS_ALPHAOP, flags & 0x1f);
        dev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, g_stage_arg[(flags >> 5) & 7]);
        dev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, g_stage_arg[(flags >> 8) & 7]);
    }
}

// @ 0x461cf0
void renderer::set_alpha_mode(int mode)
{
    if (mode != alpha_mode) {
        if (mode == 0) {
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
            alpha_mode = 0;
            return;
        }
        if (alpha_mode == 0)
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
        IDirect3DDevice9* d = dev;
        if (mode == 1) {
            d->SetRenderState(D3DRS_ALPHAREF, 8);
        } else if (mode == 2) {
            d->SetRenderState(D3DRS_ALPHAREF, 0x88);
            alpha_mode = 2;
            return;
        } else {
            d->SetRenderState(D3DRS_ALPHAREF, 0xfb);
        }
    }
    alpha_mode = mode;
}

// @ 0x461d90
void renderer::set_stage_sampler(int stage, unsigned flags)
{
    if (flags != (unsigned)stage_samp[stage]) {
        if (flags & 1)
            dev->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        else
            dev->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        if (flags & 2) {
            dev->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            stage_samp[stage] = flags;
            return;
        }
        dev->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        stage_samp[stage] = flags;
    }
}

// @ 0x461e10
void renderer::set_material(float power, const float* specular)
{
    D3DMATERIAL9 m;
    memset(&m, 0, sizeof m);
    m.Specular.r = specular[0];
    m.Specular.g = specular[1];
    m.Specular.b = specular[2];
    m.Power = power;
    m.Diffuse.r = m.Ambient.r = 1.0f;
    m.Diffuse.g = m.Ambient.g = 1.0f;
    m.Diffuse.b = m.Ambient.b = 1.0f;
    m.Diffuse.a = m.Ambient.a = 1.0f;
    m.Specular.a = 1.0f;
    dev->SetMaterial(&m);
}

// @ 0x461eb0
void renderer::set_specular(const float* c)
{
    if (spec[0] != c[0] || spec[1] != c[1] || spec[2] != c[2]) {
        spec[0] = c[0];
        spec[1] = c[1];
        spec[2] = c[2];
        g_effect->SetVector("vSpecularColor", (const D3DXVECTOR4*)spec);
    }
}

// ---- material / mesh part ------------------------------------------------------------------------
struct material {
    char pad00[0x20];
    unsigned flags;                 // 0x20  bit 28: has a pixel shader, bit 29: shader set elsewhere
    char pad24[0xbc - 0x24];
    IDirect3DPixelShader9* ps;      // 0xbc
    void bind_pixel_shader();       // 0x461f20
};

// @ 0x461f20
void material::bind_pixel_shader()
{
    if (flags & 0x20000000) return;
    if (flags & 0x10000000) {
        IDirect3DPixelShader9* shader = ps;
        if (shader != g_render.cur_ps) {
            g_render.dev->SetPixelShader(shader);
            g_render.cur_ps = shader;
        }
    } else if (g_render.cur_ps) {
        g_render.dev->SetPixelShader(0);
        g_render.cur_ps = 0;
    }
}

struct mat_rec { char pad00[0x20]; unsigned flags; };      // bit 5: specular
struct mesh_part {
    char pad00[0x1c];
    unsigned flags;                 // 0x1c  bit 3: z-write off, bit 4: depth test off, bit 11: normalize normals,
                                    //       bits 12..13: alpha-test mode
    char pad20[0x30 - 0x20];
    mat_rec* mat;                   // 0x30
    float spec_color[3];            // 0x34
    char pad40[4];
    float power;                    // 0x44
    char pad48[0x64 - 0x48];
    int refs;                       // 0x64  number of mesh instances using this part
    void set_state();               // 0x461f80
};

// @ 0x461f80
void mesh_part::set_state()
{
    unsigned char depth_test = (unsigned char)(~(flags >> 4) & 1);
    if (depth_test != g_render.b54b) {
        g_render.b54b = depth_test;
        if (depth_test)
            g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        else
            g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    }
    unsigned char zoff = (unsigned char)((flags >> 3) & 1);
    if (zoff != g_render.b549) {
        g_render.b549 = zoff;
        g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, zoff == 0);
    }
    g_render.set_alpha_mode((flags >> 12) & 3);
    unsigned char norm = (unsigned char)((flags >> 11) & 1);
    if (norm != g_render.b548) {
        g_render.dev->SetRenderState(D3DRS_NORMALIZENORMALS, norm);
        g_render.b548 = norm;
    }
    unsigned char spec = (unsigned char)((mat->flags >> 5) & 1);
    if (spec != g_render.b547) {
        g_render.dev->SetRenderState(D3DRS_SPECULARENABLE, spec);
        g_render.b547 = spec;
    }
    if (mat->flags & 0x20) {
        if (power != g_render.material_power) {
            g_effect->SetFloat("fMaterialPower", power);
            g_render.material_power = power;
        }
        g_render.set_specular(spec_color);
    }
}

// ---- mesh instance: shared part reference ----------------------------------------------------------
struct mesh_inst {
    char pad00[0x2c];
    mesh_part* part;                // 0x2c
    void set_part(mesh_part* p);    // 0x4620b0
};

// @ 0x4620b0
void mesh_inst::set_part(mesh_part* p)
{
    if (part) part->refs--;
    part = p;
    if (p) p->refs++;
}

// ---- view frustum (layout from s0041e470) ----------------------------------------------------------
struct plane { vector4 pos, normal; };
struct frustum {
    char perspective;               // +0
    char left_handed;               // +1
    char pad2[2];
    int nplanes;                    // +4
    plane planes[6];                // +8
    pos m;                          // +0xc8 (s = right, f = up, u = backwards)
    float left, right, top, bottom, nearz, farz;   // +0x108
    D3DXMATRIX projection();                // 0x4620d0 (hidden return pointer)
    matrix view_matrix();                   // 0x462170 (hidden return pointer)
    vector4 to_clip(const vector4& p);      // 0x4621f0 (hidden return pointer)
};

// @ 0x4620d0
D3DXMATRIX frustum::projection()
{
    D3DXMATRIX mat;
    if (left_handed) {
        if (perspective)
            D3DXMatrixPerspectiveOffCenterRH(&mat, left, right, bottom, top, nearz, farz);
        else
            D3DXMatrixOrthoOffCenterLH(&mat, left, right, bottom, top, nearz, farz);
    } else {
        if (perspective)
            D3DXMatrixPerspectiveOffCenterRH(&mat, left, right, bottom, top, nearz, farz);
        else
            D3DXMatrixOrthoOffCenterRH(&mat, left, right, bottom, top, nearz, farz);
    }
    return mat;
}

// @ 0x462170
matrix frustum::view_matrix()
{
    pos id;
    matrix rel;
    rel = m.relative(*identity_pos(&id));
    rel.s.w = 0.0f; rel.f.w = 0.0f; rel.u.w = 0.0f; rel.o.w = 1.0f;
    return rel;
}

// @ 0x4621f0
vector4 frustum::to_clip(const vector4& p)
{
    vector4 t = m.transform_point_to_local(p);
    vector4 v = t;
    if (!perspective) {
        v.x = (t.x - left) / (right - left);
        v.y = (t.y - bottom) / (top - bottom);
        v.z = (-t.z - nearz) / (farz - nearz);
    }
    return v;
}

// @ 0x4622a0
struct sort_rec { char pad[0x218]; __int64 key; };
bool __cdecl rec_less(sort_rec* const* a, sort_rec* const* b)
{
    return (*a)->key < (*b)->key;
}

// ---- device vertex / index buffer pair (restore after reset) -------------------------------------
struct dev_buffer {
    int type;                       // 0x00  0 vertex buffer, 1 index buffer
    int elem_size;                  // 0x04
    IDirect3DVertexBuffer9* vb;     // 0x08
    unsigned fvf;                   // 0x0c
    char pad10[4];
    int generation;                 // 0x14
    char pad18[0x28 - 0x18];
    int count;                      // 0x28  capacity requested
    int loaded;                     // 0x2c  capacity currently on the device
    char pad30[0x44 - 0x30];
    IDirect3DIndexBuffer9* ib;      // 0x44
    char pad48[4];
    HRESULT restore();              // 0x4622e0
};
struct buffer_pair {
    dev_buffer bufs[2];             // stride 0x4c
    void restore_all();             // 0x4623c0
};
extern IDirect3DDevice9* g_device;  // 0xa17f28

// @ 0x4622e0
HRESULT dev_buffer::restore()
{
    HRESULT hr;                     // never assigned on the fall-through path (the original returns garbage)
    if (type == 0 && count != 0) {
        hr = g_device->CreateVertexBuffer(elem_size * count, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, fvf, D3DPOOL_DEFAULT, &vb, 0);
        if (hr < 0) mb_error("Unable to restore vertex buffer.");
        else loaded = count;
    } else if (type == 1 && count != 0) {
        hr = g_device->CreateIndexBuffer(count * 2, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, 0);
        if (hr < 0) mb_error("Unable to restore index buffer.");
        else loaded = count;
    }
    generation++;
    return hr;
}

// @ 0x4623c0
void buffer_pair::restore_all()
{
    for (int i = 0; i < 2; i++) {
        dev_buffer& b = bufs[i];
        if (b.loaded < b.count) {
            if (b.loaded != 0) {
                if (b.type == 1)
                    b.ib->Release();
                else if (*(volatile int*)&b.count != 0 && b.vb)     // (a plain read gets CSE'd with the loop test; the original re-reads)
                    b.vb->Release();
                b.generation++;
                b.loaded = 0;
            }
            b.restore();
        }
    }
}

// ---- 2D cell grid (rows of cells), see s0041e470 for the destructor ----------------------------------
struct grid_cell {
    float h; int flags; int id, id2;
    grid_cell() : flags(0), id(-1), id2(-1) {}
};
struct cell_rows {
    grid_cell** rows;               // +0
    int pad[4];
    int last;                       // +0x14  highest column index
    int rows_n;                     // +0x18  highest row index
    ~cell_rows();                   // 0x41ecc0
    void alloc(const int* dim);     // 0x462420
};

// @ 0x462420
void cell_rows::alloc(const int* dim)
{
    this->~cell_rows();
    last = dim[0];
    rows_n = dim[1];
    rows = new grid_cell*[last + 1];
    for (int i = 0; i <= last; i++)
        rows[i] = new grid_cell[rows_n + 1];
}

// ---- envelope / fade parameters ----------------------------------------------------------------------
struct envelope {
    char pad00[0xdc];
    int fdc;                        // 0xdc
    float inv_time;                 // 0xe0
    int fe4;                        // 0xe4
    char dirty;                     // 0xe8
    char pade9[3];
    float time;                     // 0xec
    void set_fade(const float* t, const int* v);    // 0x4624c0
};

// @ 0x4624c0
void envelope::set_fade(const float* t, const int* v)
{
    dirty = 1;
    fdc = 0;
    float d = *t > 0.0001f ? *t : 0.0001f;
    float inv = 1.0f / d;
    if (inv < 0.0f) inv = 0.0f;
    else if (inv >= 10000.0f) inv = 10000.0f;
    inv_time = inv;
    fe4 = *v;
    time = *t;
}
