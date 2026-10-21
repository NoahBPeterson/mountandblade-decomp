// Slice s00416540: texture loading (D3DX), material records (ctor/dtor, load/save, pixel shader creation),
// mesh-skin record (ctor, save) and the axis-aligned box helper ctor.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <new>                // declares operator new[]: without it cl calls operator new (0x53545e) for new T[n]
#include "mbres.h"

unsigned noise_hash(int x);                    // 0x415f70 (s00415c80)

extern int g_lod;                              // 0x5a9f9c texture detail level
extern int g_shader_flag;                      // 0x5e28fc
extern int g_use_vs;                           // 0x5a9fcc "use_vertex_shaders"
extern int g_use_ps;                           // 0x5a9fd0 "use_pixel_shaders"
extern IDirect3DDevice9* g_dev;                // 0xa17f28
extern D3DCAPS9 g_caps;                        // 0xa17f2c

// A texture record (vtable not needed here). Only the fields these two methods touch.
struct texture : res_base {
    char loaded;                   // +0x18
    char pad19[3];
    unsigned id;                   // +0x1c (hashed to pick the dropped mip levels)
    int external;                  // +0x20
    unsigned flags;                // +0x24 (bit 1 full detail, bit 2 needs shader flag, bits 24..27 cube/array count)
    int count;                     // +0x28
    char pad2c[0x14];
    IDirect3DTexture9** tex;       // +0x40
    IUnknown* p44;                 // +0x44
    IUnknown* p48;                 // +0x48
    int f4c;                       // +0x4c
    int load(int check_exists, tstring* root);     // 0x416540
    int unload();                                  // 0x4167c0
};

// @ 0x416540
int texture::load(int check_exists, tstring* root)
{
    unsigned fl = flags;
    int hr = 0;
    if ((fl & 4) && !g_shader_flag)
        return 0;
    if (!loaded) {
        if (!external) {
            int skip = 0;
            if (g_lod >= 25) {
                int h = noise_hash(id);
                if ((float)(h & 0xf) * 0.0625f < (100.0f - (float)g_lod) * 0.013333334f)
                    skip = 1;
            } else {
                skip = 1;
                int h = noise_hash(id);
                if ((float)(h & 0xf) * 0.0625f < (25.0f - (float)g_lod) * 0.04f)
                    skip = 2;
            }
            if (fl & 2)
                skip = 0;
            count = 1;
            bool multi = (fl & 0xf000000) != 0;
            if (multi) {
                count = ((int)fl >> 24) & 0xf;
                count *= 4;
            }
            tex = new IDirect3DTexture9*[count];
            for (int i = 0; i < count; i++) {
                char path[1024];
                char full[1024];
                WIN32_FIND_DATAA fd;
                if (multi) {
                    sprintf(path, "Textures\\%s_%d.dds", name.c_str(), i);
                    sprintf(full, "%sTextures\\%s_%d.dds", root->c_str(), name.c_str(), i);
                } else {
                    sprintf(path, "Textures\\%s", name.c_str());
                    sprintf(full, "%sTextures\\%s", root->c_str(), name.c_str());
                }
                if (check_exists) {
                    HANDLE h = FindFirstFileA(full, &fd);
                    if (h != INVALID_HANDLE_VALUE) {
                        FindClose(h);
                        int k = 0;
                        char c;
                        do {
                            c = full[k];
                            path[k] = c;
                            k++;
                        } while (c != 0);
                    }
                }
                hr = D3DXCreateTextureFromFileExA(g_dev, path, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE,
                        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
                        D3DX_SKIP_DDS_MIP_LEVELS(skip, D3DX_FILTER_BOX), 0, 0, 0, &tex[i]);
                if (hr < 0)
                    mb_error("Unable to open texture file %s", path);
            }
        }
        loaded = 1;
    }
    return hr;
}

// @ 0x4167c0
int texture::unload()
{
    if (loaded) {
        if (p48) {
            p48->Release();
            p48 = 0;
        }
        for (int i = 0; i < count; i++)
            tex[i]->Release();
        if (p44) {
            p44->Release();
            p44 = 0;
        }
        loaded = 0;
        f4c = 0;
    }
    if (tex) {
        delete[] tex;
        tex = 0;
        count = 0;
    }
    return 0;
}

// @ 0x416830
material::material()
{
    ps_done = 0;
    flags20 = 0;
    flags1c = 0;
    nstages = 0;
    nalt = 0;
    for (int i = 0; i < 4; i++) {
        stages[i].d = 0;
        stages[i].b = 1;
        stages[i].c = 1;
        stages[i].a = -1;
    }
    ps = 0;
    fc0 = 0;
    f18 = 0;
}

// @ 0x4168f0
material::~material() {}

// @ 0x416930
bool material::supported()
{
    bool ok = true;
    unsigned shdr = flags20 & 0x20000000;
    if (shdr != 0 && !g_shader_flag)
        ok = false;
    if (shdr == 0 && g_shader_flag)
        ok = false;
    if ((flags1c & 0x1000) && (g_caps.PixelShaderVersion < 0xffff0101 || !g_use_ps))
        ok = false;
    if (shdr != 0 && (g_caps.VertexShaderVersion < 0xfffe0101 || !g_use_vs))
        ok = false;
    if (flags20 & 0x10000)
        ok = false;
    return ok;
}

// @ 0x4169a0
void material::load(stream_base* s)
{
    tstring tmp;
    s->r_tstr(&name);
    s->r_uint(&flags20);
    s->r_uint(&flags1c);
    s->r_tstr(&ps_file);
    s->r_int(&nalt);
    if (nstages > 3)
        mb_error("Number of elternatives exceeds %d for material %s.", 3, name);
    for (int i = 0; i < nalt; i++)
        s->r_tstr(&alts[i]);
    s->r_int(&nstages);
    if (nstages > 4)
        mb_error("Number of stages exceeds %d for material %s.", 4, name);
    for (int i = 0; i < nstages; i++)
        stages[i].load(s);
}

// @ 0x416ad0
void stage_t::load(stream_base* s)
{
    s->r_int(&a);
    s->r_uint((unsigned*)&b);
    s->r_uint((unsigned*)&c);
    s->r_uint((unsigned*)&d);
}

// @ 0x416b10
void material::save(stream_base* s)
{
    s->w_tstr(&name);
    s->w_uint(&flags20);
    s->w_uint(&flags1c);
    s->w_tstr(&ps_file);
    s->w_int(&nalt);
    for (int i = 0; i < nalt; i++)
        s->w_tstr(&alts[i]);
    s->w_int(&nstages);
    for (int i = 0; i < nstages; i++)
        stages[i].save(s);
}

// @ 0x416bc0
void stage_t::save(stream_base* s)
{
    s->w_int(&a);
    s->w_uint((unsigned*)&b);
    s->w_uint((unsigned*)&c);
    s->w_uint((unsigned*)&d);
}

// @ 0x416c00
void material::create_shaders()
{
    if (!ps_done) {
        if ((flags20 & 0x10000000) && g_use_ps && supported()) {
            ID3DXBuffer* code;
            ID3DXBuffer* errors;
            if (D3DXAssembleShaderFromFileA(ps_file.c_str(), 0, 0, 0, &code, &errors) < 0)
                mb_error("Failed to assemble shader %s from file", name.c_str());
            if (g_dev->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &ps) < 0)
                mb_error("Failed to create pixel shader %s", name.c_str());
        }
        ps_done = 1;
    }
}

// @ 0x416cd0
skin::skin()
{
    f5c = -1;
    f60 = -1;
    f64 = 0;
    id = 0;
    for (int i = 0; i < 5; i++)
        refs[i] = 0;
    color = rgl::vector4(0.0f, 0.0f, 0.0f);
    f30 = 0;
    f44 = 0.0f;
    f68 = 0;
}

// three floats of one vector, through a pointer
static inline void w3(stream_base& s, float* p) { s.w_float(&p[0]).w_float(&p[1]).w_float(&p[2]); }

// @ 0x416d50
void skin::save(stream_base* s)
{
    s->w_tstr(&name);
    s->w_uint(&id);
    s->w_tstr(&file);
    for (int i = 0; i < 5; i++) {
        if (refs[i] == 0) {
            s->w_cstr("none");
        } else {
            tstring t("");
            res_base* r = refs[i];
            for (int j = 0; j < r->name.len; j++) {
                int c = r->name.buf[j];
                if ((char)c == '.')
                    break;
                t.push_back(c);
                r = refs[i];
            }
            s->w_tstr(&t);
        }
    }
    w3(s->w_float(&f44), &color.x);
}

// @ 0x416e40
void aabb::reset()
{
    min = rgl::vector4(1e8f, 1e8f, 1e8f);
    max = rgl::vector4(-1e8f, -1e8f, -1e8f);
    radius = 0.0f;
    center = rgl::vector4(0.0f, 0.0f, 0.0f);
}
