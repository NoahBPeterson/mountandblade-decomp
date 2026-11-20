// port/gfx/tests/gfx_test.cpp: drives the D3D9 / D3DX implementation through the API src/ uses and checks the pixels.
//   mb_gfx_test <game dir> <out dir> [scenario...]
// Each scenario renders into the device back buffer, reads it back (GetRenderTargetData into a SYSTEMMEM surface),
// checks a few pixels and writes <out>/<scenario>.png via D3DXSaveSurfaceToFileA. Exit code = number of failures.
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "port_prelude.h"
#include <d3d9.h>
#include <d3dx9.h>
#undef min
#undef max
#undef fopen

extern "C" SDL_Window* mbp_sdl_window(struct HWND__* h) { return (SDL_Window*)h; }   // the platform lane's seam, for the test

static int W = 320, H = 240;
static std::string g_game, g_out;
static int g_fail = 0;
static IDirect3DDevice9* dev = nullptr;
static IDirect3D9* d3d = nullptr;

static unsigned pixel_at(IDirect3DSurface9* sys, int x, int y)
{
    D3DLOCKED_RECT lr;
    sys->LockRect(&lr, nullptr, D3DLOCK_READONLY);
    unsigned v = ((unsigned*)((char*)lr.pBits + y * lr.Pitch))[x];
    sys->UnlockRect();
    return v & 0xffffff;
}

static bool near_color(unsigned a, unsigned b, int tol)
{
    for (int s = 0; s < 24; s += 8) {
        int ca = (a >> s) & 255, cb = (b >> s) & 255;
        if (abs(ca - cb) > tol) return false;
    }
    return true;
}

struct Check { int x, y; unsigned rgb; int tol; };

static void finish(const char* name, std::vector<Check> checks)
{
    IDirect3DSurface9* bb = nullptr;
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    IDirect3DSurface9* sys = nullptr;
    dev->CreateOffscreenPlainSurface(W, H, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, nullptr);
    HRESULT hr = dev->GetRenderTargetData(bb, sys);
    bool ok = SUCCEEDED(hr), read = ok;
    std::string detail;
    for (const Check& c : checks) {
        unsigned got = read ? pixel_at(sys, c.x, c.y) : 0;
        if (!read || !near_color(got, c.rgb, c.tol)) {
            ok = false;
            char b[96];
            snprintf(b, sizeof b, " (%d,%d)=%06x want %06x", c.x, c.y, got, c.rgb);
            detail += b;
        }
    }
    std::string path = g_out + "/" + name + ".png";
    D3DXSaveSurfaceToFileA(path.c_str(), D3DXIFF_PNG, sys, nullptr, nullptr);
    printf("%-14s %s%s  -> %s\n", name, ok ? "PASS" : "FAIL", detail.c_str(), path.c_str());
    if (!ok) g_fail++;
    sys->Release();
    bb->Release();
    dev->Present(nullptr, nullptr, nullptr, nullptr);
}

static void reset_ffp()
{
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetTexture(0, nullptr);
    dev->SetTexture(1, nullptr);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    D3DMATRIX id = {};
    id._11 = id._22 = id._33 = id._44 = 1;
    dev->SetTransform(D3DTS_WORLD, &id);
    dev->SetTransform(D3DTS_VIEW, &id);
    dev->SetTransform(D3DTS_PROJECTION, &id);
}

static unsigned g_texel00 = 0;

// texel (0,0) of level 0 as 0xRRGGBB (A8R8G8B8 / X8R8G8B8 / DXT1-5)
static unsigned texel00(IDirect3DTexture9* t)
{
    D3DSURFACE_DESC d;
    t->GetLevelDesc(0, &d);
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, nullptr, D3DLOCK_READONLY))) return 0;
    const unsigned char* p = (const unsigned char*)lr.pBits;
    unsigned v = 0;
    if (d.Format == D3DFMT_A8R8G8B8 || d.Format == D3DFMT_X8R8G8B8) v = *(const unsigned*)p & 0xffffff;
    else {
        const unsigned char* b = d.Format == D3DFMT_DXT1 ? p : p + 8;
        unsigned c0 = b[0] | (b[1] << 8), c1 = b[2] | (b[3] << 8);
        unsigned idx = b[4] & 3;
        auto ex = [](unsigned c, int* o) { o[0] = ((c >> 11) & 31) * 255 / 31; o[1] = ((c >> 5) & 63) * 255 / 63; o[2] = (c & 31) * 255 / 31; };
        int a[3], bb[3], r[3];
        ex(c0, a); ex(c1, bb);
        for (int i = 0; i < 3; i++) {
            if (idx == 0) r[i] = a[i];
            else if (idx == 1) r[i] = bb[i];
            else if (c0 > c1 || d.Format != D3DFMT_DXT1) r[i] = idx == 2 ? (2 * a[i] + bb[i]) / 3 : (a[i] + 2 * bb[i]) / 3;
            else r[i] = idx == 2 ? (a[i] + bb[i]) / 2 : 0;
        }
        v = (r[0] << 16) | (r[1] << 8) | r[2];
    }
    t->UnlockRect(0);
    return v;
}

struct TLVert { float x, y, z, rhw; DWORD c; };
struct TLTex { float x, y, z, rhw; DWORD c; float u, v; };

static void sc_clear()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0x33, 0x66, 0x99), 1.0f, 0);
    // a partial clear through the viewport
    D3DVIEWPORT9 vp = {0, 0, (DWORD)W / 2, (DWORD)H / 2, 0, 1};
    dev->SetViewport(&vp);
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0xff, 0x80, 0x00), 1.0f, 0);
    D3DVIEWPORT9 full = {0, 0, (DWORD)W, (DWORD)H, 0, 1};
    dev->SetViewport(&full);
    finish("clear", {{W - 10, H - 10, 0x336699, 2}, {10, 10, 0xff8000, 2}, {W / 2 + 2, 10, 0x336699, 2}});
}

static void sc_rhw_triangle()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    reset_ffp();
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    // a full-height quad split into two halves: red left, green right (clockwise in screen space)
    TLVert v[6] = {
        {0, 0, 0.5f, 1, 0xffff0000}, {(float)W / 2, 0, 0.5f, 1, 0xffff0000}, {0, (float)H, 0.5f, 1, 0xffff0000},
        {(float)W / 2, 0, 0.5f, 1, 0xff00ff00}, {(float)W, 0, 0.5f, 1, 0xff00ff00}, {(float)W, (float)H, 0.5f, 1, 0xff00ff00},
    };
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, v, sizeof(TLVert));
    finish("rhw_triangle", {{5, 5, 0xff0000, 2}, {W - 5, 5, 0x00ff00, 2}, {W - 3, H / 2, 0x00ff00, 2}, {W / 2 + 3, H - 5, 0x000000, 2}});
}

static void sc_cull()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    reset_ffp();
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    // left: screen-clockwise (D3D front face), right: counter-clockwise. D3DCULL_CCW must keep only the left one.
    TLVert v[6] = {
        {10, 10, 0.5f, 1, 0xffffffff}, {150, 10, 0.5f, 1, 0xffffffff}, {10, 200, 0.5f, 1, 0xffffffff},
        {170, 10, 0.5f, 1, 0xffffffff}, {170, 200, 0.5f, 1, 0xffffffff}, {310, 10, 0.5f, 1, 0xffffffff},
    };
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, v, sizeof(TLVert));
    finish("cull", {{20, 20, 0xffffff, 2}, {180, 20, 0x000000, 2}});
}

static IDirect3DTexture9* load_tex(const char* name)
{
    std::string p = g_game + "/Textures/" + name;
    IDirect3DTexture9* t = nullptr;
    HRESULT hr = D3DXCreateTextureFromFileExA(dev, p.c_str(), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                              D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER, D3DX_SKIP_DDS_MIP_LEVELS(0, D3DX_FILTER_BOX), 0, nullptr, nullptr, &t);
    if (FAILED(hr)) printf("cannot load %s (%08x)\n", p.c_str(), (unsigned)hr);
    return t;
}

// textured quad through VB + IB (Lock / Unlock), world/view/projection transforms and the stage cascade
static void sc_textured(const char* tex_name, const char* shot)
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(20, 20, 20), 1.0f, 0);
    reset_ffp();
    IDirect3DTexture9* t = load_tex(tex_name);
    if (!t) { g_fail++; return; }
    D3DSURFACE_DESC d;
    t->GetLevelDesc(0, &d);
    printf("  %s: %ux%u fmt %c%c%c%c levels %u\n", tex_name, d.Width, d.Height, (char)d.Format, (char)(d.Format >> 8), (char)(d.Format >> 16),
           (char)(d.Format >> 24), (unsigned)t->GetLevelCount());
    struct V { float x, y, z; DWORD c; float u, v; };
    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DIndexBuffer9* ib = nullptr;
    dev->CreateVertexBuffer(4 * sizeof(V), D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_MANAGED, &vb, nullptr);
    dev->CreateIndexBuffer(6 * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib, nullptr);
    V* pv;
    vb->Lock(0, 0, (void**)&pv, 0);
    V q[4] = {{-1, 1, 0, 0xffffffff, 0, 0}, {1, 1, 0, 0xffffffff, 1, 0}, {1, -1, 0, 0xffffffff, 1, 1}, {-1, -1, 0, 0xffffffff, 0, 1}};
    memcpy(pv, q, sizeof q);
    vb->Unlock();
    WORD* pi;
    ib->Lock(0, 0, (void**)&pi, 0);
    WORD idx[6] = {0, 1, 2, 0, 2, 3};
    memcpy(pi, idx, sizeof idx);
    ib->Unlock();
    D3DXMATRIX proj;
    D3DXMatrixOrthoOffCenterLH(&proj, -1, 1, -1, 1, 0, 1);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
    dev->SetTexture(0, t);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    dev->SetStreamSource(0, vb, 0, sizeof(V));
    dev->SetIndices(ib);
    dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
    // expected: the texture's own top-left texel (decoded on the CPU from the locked D3D copy)
    std::vector<Check> checks;
    unsigned want = texel00(t);
    if (strcmp(tex_name, "cursor.dds") == 0) g_texel00 = want;
    checks.push_back({0, 0, want, 6});
    finish(shot, checks);
    vb->Release();
    ib->Release();
    t->Release();
}

static void sc_lighting()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    reset_ffp();
    struct V { float x, y, z, nx, ny, nz; };
    V q[6] = {{-1, 1, 0, 0, 0, -1}, {1, 1, 0, 0, 0, -1}, {1, -1, 0, 0, 0, -1}, {-1, 1, 0, 0, 0, -1}, {1, -1, 0, 0, 0, -1}, {-1, -1, 0, 0, 0, -1}};
    D3DXMATRIX proj;
    D3DXMatrixOrthoOffCenterLH(&proj, -1, 1, -1, 1, -1, 1);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
    dev->SetRenderState(D3DRS_LIGHTING, TRUE);
    dev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(0x20, 0x20, 0x20));
    D3DMATERIAL9 m = {};
    m.Diffuse = {1, 0.5f, 0.25f, 1};
    m.Ambient = {1, 1, 1, 1};
    dev->SetMaterial(&m);
    D3DLIGHT9 l = {};
    l.Type = D3DLIGHT_DIRECTIONAL;
    l.Diffuse = {1, 1, 1, 1};
    l.Direction = {0, 0, 1};      // shining along +z onto the -z facing quad: full intensity
    dev->SetLight(0, &l);
    dev->LightEnable(0, TRUE);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, q, sizeof(V));
    dev->LightEnable(0, FALSE);
    // diffuse (1, .5, .25) + ambient .125 -> (255, 160, 96)
    finish("ffp_lighting", {{W / 2, H / 2, 0xffa060, 4}});
}

static void sc_effect()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 40), 1.0f, 0);
    reset_ffp();
    std::string fxp = g_game + "/mb.fx";
    ID3DXEffect* fx = nullptr;
    ID3DXBuffer* err = nullptr;
    Uint64 t0 = SDL_GetTicks();
    HRESULT hr = D3DXCreateEffectFromFileA(dev, fxp.c_str(), nullptr, nullptr, 0x800, nullptr, &fx, &err);
    if (FAILED(hr)) { printf("effect: create failed %08x\n", (unsigned)hr); g_fail++; return; }
    IDirect3DTexture9* t = load_tex("cursor.dds");
    // the game's "regular" vertex declaration (rec_tables.cpp): position, normal, colour 0/1, texcoord
    D3DVERTEXELEMENT9 el[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0}, {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
        {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1}, {0, 20, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
        {0, 32, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0}, D3DDECL_END()};
    IDirect3DVertexDeclaration9* decl = nullptr;
    dev->CreateVertexDeclaration(el, &decl);
    struct V { float x, y, z; DWORD c0, c1; float nx, ny, nz, u, v; };
    // left half of the screen: x -1..0 (to check matrices: the quad is moved by matWorldViewProj)
    V q[6] = {{-1, 1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 0, 0}, {0, 1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 1, 0}, {0, -1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 1, 1},
              {-1, 1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 0, 0}, {0, -1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 1, 1}, {-1, -1, 0.5f, 0xffffffff, 0xff000000, 0, 0, -1, 0, 1}};
    D3DXHANDLE tech = fx->GetTechniqueByName("no_shading");
    if (!tech) { printf("effect: technique no_shading missing\n"); g_fail++; return; }
    fx->SetTechnique(tech);
    // translate by +1 in x: the quad lands on the right half
    D3DXMATRIX m(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1);
    // the game hands its row-vector matrices over transposed (SetMatrixTranspose); mb.fx multiplies mul(M, v)
    fx->SetMatrixTranspose("matWorldViewProj", &m);
    fx->SetMatrixTranspose("matWorld", &m);
    fx->SetMatrixTranspose("matWorldView", &m);
    D3DXVECTOR4 mc(1, 1, 1, 1);
    fx->SetVector("vMaterialColor", &mc);
    fx->SetTexture("diffuse_texture", t);
    UINT passes = 0;
    fx->Begin(&passes, 7);
    fx->BeginPass(0);
    dev->SetVertexDeclaration(decl);
    fx->CommitChanges();
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, q, sizeof(V));
    fx->EndPass();
    fx->End();
    Uint64 t1 = SDL_GetTicks();
    printf("  effect: %u pass(es), first draw incl. shader compile %llu ms\n", passes, (unsigned long long)(t1 - t0));
    // left half: clear colour (the quad was translated right); right half: the texture's top-left texel
    finish("effect", {{W / 4, H / 2, 0x000028, 2}, {W / 2 + 2, 2, g_texel00, 12}});
    // the right half must show the texture (not the clear colour)
    IDirect3DSurface9* bb = nullptr;
    (void)bb;
    fx->Release();
    decl->Release();
    if (t) t->Release();
}

static void sc_asm_ps()
{
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    reset_ffp();
    std::string p = g_game + "/myshader.pp";
    ID3DXBuffer* code = nullptr;
    ID3DXBuffer* errs = nullptr;
    if (FAILED(D3DXAssembleShaderFromFileA(p.c_str(), nullptr, nullptr, 0, &code, &errs))) { printf("asm failed\n"); g_fail++; return; }
    IDirect3DPixelShader9* ps = nullptr;
    if (FAILED(dev->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &ps))) { printf("CreatePixelShader failed\n"); g_fail++; return; }
    // myshader.pp: tex t0; mul r1.rgb, t0.a, v1; mad r0.rgb, t0, v0, r1; mov r0.a, v0.a  -> with a white texture: v0 + v1
    IDirect3DTexture9* t = nullptr;
    dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr);
    D3DLOCKED_RECT lr;
    t->LockRect(0, &lr, nullptr, 0);
    for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) ((DWORD*)((char*)lr.pBits + y * lr.Pitch))[x] = 0xffffffff;
    t->UnlockRect(0);
    dev->SetTexture(0, t);
    dev->SetPixelShader(ps);
    TLTex v[6];
    float xs[6] = {0, (float)W, (float)W, 0, (float)W, 0}, ys[6] = {0, 0, (float)H, 0, (float)H, (float)H};
    for (int i = 0; i < 6; i++) v[i] = {xs[i], ys[i], 0.5f, 1, 0xff402010, 0, 0};
    struct TV { float x, y, z, rhw; DWORD c, s; float u, vv; } tv[6];
    for (int i = 0; i < 6; i++) tv[i] = {xs[i], ys[i], 0.5f, 1, 0xff402010, 0xff102040, 0, 0};
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, tv, sizeof(TV));
    dev->SetPixelShader(nullptr);
    finish("asm_ps", {{W / 2, H / 2, 0x504050, 3}});
    ps->Release();
    t->Release();
    code->Release();
}

// render to a texture, then sample it
static void sc_render_target()
{
    reset_ffp();
    IDirect3DTexture9* rt = nullptr;
    if (FAILED(D3DXCreateTexture(dev, 64, 64, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &rt))) { printf("rt create failed\n"); g_fail++; return; }
    IDirect3DSurface9* rts = nullptr;
    rt->GetSurfaceLevel(0, &rts);
    IDirect3DSurface9* old = nullptr;
    dev->GetRenderTarget(0, &old);
    dev->SetRenderTarget(0, rts);
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0x10, 0xc0, 0x30), 1.0f, 0);
    dev->SetRenderTarget(0, old);
    dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    dev->SetTexture(0, rt);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    TLTex v[6];
    float xs[6] = {0, 100, 100, 0, 100, 0}, ys[6] = {0, 0, 100, 0, 100, 100}, us[6] = {0, 1, 1, 0, 1, 0}, vs[6] = {0, 0, 1, 0, 1, 1};
    for (int i = 0; i < 6; i++) v[i] = {xs[i], ys[i], 0.5f, 1, 0xffffffff, us[i], vs[i]};
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, v, sizeof(TLTex));
    finish("render_target", {{50, 50, 0x10c030, 2}, {150, 150, 0, 2}});
    dev->SetTexture(0, nullptr);
    rts->Release();
    old->Release();
    rt->Release();
}

int main(int argc, char** argv)
{
    g_game = argc > 1 ? argv[1] : "game";
    g_out = argc > 2 ? argv[2] : ".";
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (!SDL_Init(SDL_INIT_VIDEO)) { printf("SDL_Init: %s\n", SDL_GetError()); return 99; }
    SDL_Window* win = SDL_CreateWindow("mb_gfx_test", W, H, 0);
    if (!win) { printf("window: %s\n", SDL_GetError()); return 99; }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = W;
    pp.BackBufferHeight = H;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.hDeviceWindow = (HWND)win;
    if (FAILED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, (HWND)win, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &dev))) { printf("CreateDevice failed\n"); return 99; }
    std::vector<std::string> only(argv + std::min(argc, 3), argv + argc);
    auto want = [&](const char* n) { if (only.empty()) return true; for (auto& o : only) if (o == n) return true; return false; };
    dev->BeginScene();
    if (want("clear")) sc_clear();
    if (want("rhw")) sc_rhw_triangle();
    if (want("cull")) sc_cull();
    if (want("textured")) sc_textured("cursor.dds", "textured_argb");
    if (want("textured")) sc_textured("banners_a.dds", "textured_dxt");
    if (want("lighting")) sc_lighting();
    if (want("rt")) sc_render_target();
    if (want("asm")) sc_asm_ps();
    if (want("effect")) sc_effect();
    dev->EndScene();
    // keep the window on screen briefly so `screencapture -l` can grab it when run by hand
    if (getenv("MBG_TEST_HOLD")) SDL_Delay((Uint32)atoi(getenv("MBG_TEST_HOLD")));
    dev->Release();
    d3d->Release();
    SDL_DestroyWindow(win);
    SDL_Quit();
    printf("%d failure(s)\n", g_fail);
    return g_fail;
}
