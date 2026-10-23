//-----------------------------------------------------------------------------
// File: renderer.cpp  (0x461630..0x4625d0)
//
// Desc: the Direct3D 9 render-state cache: view setup, the default state reset (with the shader gamma
//       uniforms), texture-stage / sampler / alpha-test / material / specular setters, material and mesh-part
//       state application, the view frustum's projection / view / clip helpers, a render-queue key compare,
//       vertex / index buffer restore after a device reset, a 2D cell-grid allocator and the envelope parameter
//       setters. Engine (rgl) code by its content, placed under src/game for now. Names provisional. Functions
//       in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <string.h>
#include <new>
#include "rgl.h"
#include "./mat4.h"
#include "./renderer.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "./dev_buffer.h"
#include "../include/frustum.h"
#include "../include/mat4.h"
#include "../include/height_field.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)

using rgl::vector4;
using rgl::mat4;

// copies a frame into a D3D matrix, forcing the w column to (0, 0, 0, 1)
static inline void to_d3d(D3DMATRIX* out, const mat4& m)
{
    *(mat4*)out = m;
    out->_14 = 0.0f; out->_24 = 0.0f; out->_34 = 0.0f; out->_44 = 1.0f;
}

// @ 0x461630
void renderer::set_view(const mat4* p)
{
    to_d3d(&view, rgl::identity());
    to_d3d(&view2, rgl::identity());
    f500 = 0;
    rgl::rot3 r;
    r.s.x = 1.0f; r.s.y = 0.0f; r.s.z = 0.0f;
    r.f.x = 0.0f; r.f.y = 1.0f; r.f.z = 0.0f;
    r.u.x = 0.0f; r.u.y = 0.0f; r.u.z = 1.0f;
    {
        mat4 rel;
        rel = p->relative(rgl::identity());
        to_d3d(&view, mat4_ro(r, vector4(0.0f, 0.0f, 0.0f)) * rel);
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
    cur_ps = 0;
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
    f5f0 = -1233.213134765625f;         // sentinels: no cached value matches them
    f5f4 = 3345663.25f;
    f5f8 = 0.0f;
    f5fc = 0.0f;
    f600 = -2312321280.0f;
    material_power = -12312.2138671875f;
    spec[0] = -234.23422241210938f;
    spec[1] = -234.23422241210938f;
    spec[2] = -234.23422241210938f;
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

// @ 0x4620b0
void mesh_inst::set_part(mesh_part* p)
{
    if (part) part->refs--;
    part = p;
    if (p) p->refs++;
}

// @ 0x4620d0
D3DXMATRIX rgl::frustum::projection()
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
mat4 frustum_view::view_matrix()
{
    mat4 rel;
    rel = ((mat4&)m).relative(rgl::identity());
    rel.s.w = 0.0f; rel.f.w = 0.0f; rel.u.w = 0.0f; rel.o.w = 1.0f;
    return rel;
}

// @ 0x4621f0
vector4 rgl::frustum::to_clip(const vector4& p)
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
bool __cdecl rec_less(sort_rec* const* a, sort_rec* const* b)
{
    return (*a)->key < (*b)->key;
}

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

// @ 0x462420
void cell_rows::alloc(const int* dim)
{
    this->~cell_rows();
    last = dim[0];
    rows_n = dim[1];
    rows = new HCell*[last + 1];
    for (int i = 0; i <= last; i++)
        rows[i] = new HCell[rows_n + 1];
}

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
    fade_time = *t;
}
// @ 0x462540
void envelope::set_params(float a, float t, const int* b, const int* c)
{
    f0 = a;
    dirty = 1;
    time = t;
    f8 = *b;
    fc = *c;
    float s = (1.0f / t) * 3.0f;
    if (s < 0.001f) smooth = 0.001f;
    else if (s >= 0.95f) smooth = 0.95f;
    else smooth = s;
}

