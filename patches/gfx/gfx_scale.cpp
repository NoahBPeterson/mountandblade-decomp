// patches/gfx/gfx_scale.cpp: render-resolution scale, supersampling and AMD FSR 1 for the 3D scene (patch set "gfx").
//
// CHANGES BEHAVIOUR on purpose when gfx_render_scale != 1 or gfx_fsr = 1 (rgl_config.txt): the 3D view renders at
// gfx_render_scale times the backbuffer per axis (0.25 .. 2) into a target of ours, and is resolved to the backbuffer:
//   scale < 1   upscale: one bilinear tap (gfx_fsr = 0), or FSR 1 EASU to the backbuffer size then RCAS (gfx_fsr = 1)
//   scale = 1   only with gfx_fsr = 1: RCAS of the native-size scene (sharpening only)
//   scale > 1   supersampling: downsample to the backbuffer size, exact 2x with one bilinear tap (the 2x2 box), any
//               other ratio with ps_down's tent filter (one output pixel's radius); then RCAS when gfx_fsr = 1.
//               EASU is an upscaler and never runs here. The scale is capped so the target fits
//               D3DCAPS9.MaxTextureWidth / Height; when the targets cannot be made, 1.5 and then (with FSR) 1 are tried,
//               and then the scene is not redirected.
//
// Where the split is. The frame is a list of rgl::scene passes (render_batch::render 0x47e520, render_passes.cpp), each set
// up by RenderScene::apply_state (0x46ca50): its viewport, clear and camera. gfx_scene_state.cpp's apply_state asks
// gfx_scale_scene_begin first. A pass is redirected into our scaled target when
//   - render target 0 is the backbuffer (shadow maps and other texture targets are left alone),
//   - its camera is a perspective one (the GUI's passes are orthographic), and
//   - its viewport covers at least 3/4 of the backbuffer's width and height (small 3D views inside the GUI, such as the
//     character preview, stay native). The campaign map is such a view: its MapWidget leaves the map bar below it
//     (1280x681 of 1280x720). The target keeps the backbuffer's coordinates (the viewport is scaled with it), and the
//     resolve writes only the union of the redirected viewports back (a scissor), so the GUI around them is kept.
// Consecutive qualifying passes share the target (and our depth-stencil, as they share the backbuffer's in the
// original). The target is resolved to the backbuffer (bilinear, or EASU then RCAS) when a pass that does not qualify
// comes next on the backbuffer (the GUI over the scene), and at EndScene and Present, before anything else can draw.
// So the 2D GUI and text are drawn after the resolve, at native resolution.
//
// The passes save and restore all of the game's device state with a D3DSBT_ALL state block: the game caches its render
// states (src/engine/render/renderer.cpp) and must find them as it left them. Resolution targets are D3DPOOL_DEFAULT: released
// before every Reset (gfx_device.cpp) and recreated, at the new backbuffer size, on the next qualifying pass. A
// multisampled backbuffer gets a multisampled scene target (and depth-stencil) of the same type, resolved with
// StretchRect before the upscale. Without ps_3_0, or when a target cannot be created, the scene is not redirected.
// The settings can change at run time (gfx_device.cpp applies a UI's change at a Present): the targets are released
// and remade at the new scale by the next qualifying pass.
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <string.h>
#include "./gfx.h"
#include "./shaders/gfx_shaders.h"

static IDirect3DVertexShader9* s_vs;
static IDirect3DPixelShader9* s_ps_copy;
static IDirect3DPixelShader9* s_ps_easu;
static IDirect3DPixelShader9* s_ps_rcas;
static IDirect3DPixelShader9* s_ps_down;
static IDirect3DVertexDeclaration9* s_decl;
static bool s_usable;                       // shaders created: redirection may happen
static bool s_tried;                        // shaders tried on this device
static UINT s_max_w, s_max_h;               // D3DCAPS9.MaxTextureWidth / Height

// targets, for one backbuffer size
static UINT s_bw, s_bh;                     // the backbuffer size they were made for (0: none)
static UINT s_sw, s_sh;                     // the scaled scene size
static D3DFORMAT s_fmt;
static D3DMULTISAMPLE_TYPE s_ms;
static DWORD s_msq;
static IDirect3DTexture9* s_scene_tex;      // the scene at s_sw x s_sh, sampled by the upscale
static IDirect3DSurface9* s_scene_surf;     // its level 0
static IDirect3DSurface9* s_scene_ms;       // the multisampled colour target, 0 without MSAA
static IDirect3DSurface9* s_scene_ds;       // the scaled depth-stencil
static IDirect3DTexture9* s_easu_tex;       // backbuffer-sized input of RCAS: EASU's or the downsample's output
static IDirect3DSurface9* s_easu_surf;      // (only with FSR at a scale != 1)
static IDirect3DTexture9* s_copy_tex;       // the backbuffer copied before a redirect (only when needed)
static IDirect3DSurface9* s_copy_surf;
static bool s_failed;                       // creating the targets for this size failed: do not retry every pass

// the frame
static bool s_active;                       // render target 0 is our scene target
static bool s_valid;                        // our target holds this frame's scene so far (a later pass may continue it)
static bool s_bb_drawn;                     // something was drawn to the backbuffer this frame outside our target
static IDirect3DSurface9* s_saved_ds;       // the depth-stencil we replaced (referenced), 0 if none
static RECT s_rect;                         // the backbuffer pixels the redirected passes cover (their viewports' union)
static unsigned s_frame, s_log_left = 400;

template <class T> static void release(T*& p)
{
    if (p) {
        p->Release();
        p = 0;
    }
}

static IDirect3DSurface9* scene_target() { return s_scene_ms ? s_scene_ms : s_scene_surf; }

void gfx_scale_release()
{
    release(s_saved_ds);
    release(s_scene_surf);
    release(s_scene_tex);
    release(s_scene_ms);
    release(s_scene_ds);
    release(s_easu_surf);
    release(s_easu_tex);
    release(s_copy_surf);
    release(s_copy_tex);
    s_bw = s_bh = 0;
    s_failed = false;
    s_active = s_valid = s_bb_drawn = false;
}

// the device is about to go (GameApp::DeleteDeviceObjects, gfx_teardown.cpp): everything we made on it, the shaders and
// the declaration too, or the framework's final Release finds references left (D3DAPPERR_NONZEROREFCOUNT). A new device
// makes them again.
void gfx_scale_device_deleted()
{
    gfx_scale_release();
    release(s_vs);
    release(s_ps_copy);
    release(s_ps_easu);
    release(s_ps_rcas);
    release(s_ps_down);
    release(s_decl);
    s_usable = false;
    s_tried = false;
    gfx_log("device objects deleted: targets, shaders and declaration released\n");
}

void gfx_scale_device_created(IDirect3DDevice9* dev)
{
    gfx_scale_release();
    if (s_tried)                            // the device lives as long as the game (Reset keeps shaders and declarations)
        return;
    s_tried = true;
    D3DCAPS9 caps;
    dev->GetDeviceCaps(&caps);
    s_max_w = caps.MaxTextureWidth;
    s_max_h = caps.MaxTextureHeight;
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0)) {
        gfx_log("render scale off: no vs_3_0 / ps_3_0 (ps %08lx vs %08lx)\n", caps.PixelShaderVersion, caps.VertexShaderVersion);
        return;
    }
    static const D3DVERTEXELEMENT9 el[] = {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    HRESULT hr = dev->CreateVertexDeclaration(el, &s_decl);
    if (SUCCEEDED(hr)) hr = dev->CreateVertexShader((const DWORD*)g_vs_quad, &s_vs);
    if (SUCCEEDED(hr)) hr = dev->CreatePixelShader((const DWORD*)g_ps_copy, &s_ps_copy);
    if (SUCCEEDED(hr)) hr = dev->CreatePixelShader((const DWORD*)g_ps_easu, &s_ps_easu);
    if (SUCCEEDED(hr)) hr = dev->CreatePixelShader((const DWORD*)g_ps_rcas, &s_ps_rcas);
    if (SUCCEEDED(hr)) hr = dev->CreatePixelShader((const DWORD*)g_ps_down, &s_ps_down);
    s_usable = SUCCEEDED(hr);
    gfx_log("render scale %.3f, fsr %d: shaders %s (%08lx), max texture %ux%u\n", g_gfx.render_scale, g_gfx.fsr,
            s_usable ? "ok" : "FAILED", hr, s_max_w, s_max_h);
}

// ---- targets ----------------------------------------------------------------------------------------------------------
static bool make_tex(IDirect3DDevice9* dev, UINT w, UINT h, IDirect3DTexture9** t, IDirect3DSurface9** s)
{
    if (FAILED(dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, s_fmt, D3DPOOL_DEFAULT, t, 0)))
        return false;
    return SUCCEEDED((*t)->GetSurfaceLevel(0, s));
}

static bool make_targets(IDirect3DDevice9* dev, const D3DSURFACE_DESC& d, float scale, const D3DSURFACE_DESC* ds_desc);

static const char* mode_name()
{
    if (s_sw < s_bw || s_sh < s_bh)
        return g_gfx.fsr ? "upscale EASU + RCAS" : "upscale bilinear";
    if (s_sw == s_bw && s_sh == s_bh)
        return "RCAS only";
    if (s_sw == 2 * s_bw && s_sh == 2 * s_bh)
        return g_gfx.fsr ? "downsample 2x box + RCAS" : "downsample 2x box";
    return g_gfx.fsr ? "downsample tent + RCAS" : "downsample tent";
}

static bool ensure_targets(IDirect3DDevice9* dev, IDirect3DSurface9* bb, const D3DSURFACE_DESC* ds_desc)
{
    D3DSURFACE_DESC d;
    bb->GetDesc(&d);
    if (s_bw == d.Width && s_bh == d.Height && s_ms == d.MultiSampleType && s_msq == d.MultiSampleQuality)
        return !s_failed;
    // the scales to try: the asked one, capped by the largest texture; then, supersampling, 1.5 and (with FSR, RCAS
    // only) 1
    float want = g_gfx.render_scale;
    if (want > 1.0f && s_max_w && s_max_h) {
        float cap = (float)s_max_w / d.Width < (float)s_max_h / d.Height ? (float)s_max_w / d.Width : (float)s_max_h / d.Height;
        if (want > cap)
            want = cap < 1.0f ? 1.0f : cap;
    }
    float tries[3];
    int n = 0;
    tries[n++] = want;
    if (want > 1.5f)
        tries[n++] = 1.5f;
    if (want > 1.0f && g_gfx.fsr)
        tries[n++] = 1.0f;
    bool ok = false;
    for (int t = 0; t < n && !ok; t++) {
        if (want == 1.0f && !g_gfx.fsr)
            break;
        ok = make_targets(dev, d, tries[t], ds_desc);
        if (!ok)
            gfx_log("targets for %ux%u at scale %.3f: FAILED\n", d.Width, d.Height, tries[t]);
    }
    if (!ok) {
        gfx_scale_release();
        s_bw = d.Width;         // remembered with s_failed: no retry until the size changes or the device resets
        s_bh = d.Height;
        s_ms = d.MultiSampleType;
        s_msq = d.MultiSampleQuality;
        s_failed = true;
    }
    return ok;
}

static bool make_targets(IDirect3DDevice9* dev, const D3DSURFACE_DESC& d, float scale, const D3DSURFACE_DESC* ds_desc)
{
    gfx_scale_release();
    s_bw = d.Width;
    s_bh = d.Height;
    s_ms = d.MultiSampleType;
    s_msq = d.MultiSampleQuality;
    s_sw = scale == 1.0f ? d.Width : (UINT)floor(d.Width * scale + 0.5f);
    s_sh = scale == 1.0f ? d.Height : (UINT)floor(d.Height * scale + 0.5f);
    if (s_sw < 1) s_sw = 1;
    if (s_sh < 1) s_sh = 1;
    if (s_max_w && s_sw > s_max_w) s_sw = s_max_w;
    if (s_max_h && s_sh > s_max_h) s_sh = s_max_h;
    s_fmt = d.Format;
    bool ok = make_tex(dev, s_sw, s_sh, &s_scene_tex, &s_scene_surf);
    if (!ok) {
        release(s_scene_surf);
        release(s_scene_tex);
        s_fmt = D3DFMT_A8R8G8B8;
        ok = make_tex(dev, s_sw, s_sh, &s_scene_tex, &s_scene_surf);
    }
    if (ok && s_ms != D3DMULTISAMPLE_NONE)
        ok = SUCCEEDED(dev->CreateRenderTarget(s_sw, s_sh, s_fmt, s_ms, s_msq, FALSE, &s_scene_ms, 0));
    D3DFORMAT dsf = ds_desc ? ds_desc->Format : D3DFMT_D24S8;
    if (ok)     // contents kept (Discard FALSE): a later pass of the frame may continue on it
        ok = SUCCEEDED(dev->CreateDepthStencilSurface(s_sw, s_sh, dsf, s_ms, s_msq, FALSE, &s_scene_ds, 0));
    bool native = s_sw == s_bw && s_sh == s_bh;
    if (ok && g_gfx.fsr && !native)
        ok = make_tex(dev, s_bw, s_bh, &s_easu_tex, &s_easu_surf);
    gfx_log("targets for %ux%u (fmt %d ms %d/%lu): scene %ux%u (scale %.3f, %s), depth fmt %d, %s\n", s_bw, s_bh,
            d.Format, s_ms, s_msq, s_sw, s_sh, scale, mode_name(), dsf, ok ? "ok" : "FAILED");
    return ok;
}

// ---- full-screen passes -----------------------------------------------------------------------------------------------
struct quad_vertex { float x, y, z, w, u, v; };

// draws src over the whole of render target 0 (w x h) with ps, writing only inside clip when given; the caller has saved
// the device state
static void draw_quad(IDirect3DDevice9* dev, UINT w, UINT h, IDirect3DPixelShader9* ps, IDirect3DTexture9* src, bool linear,
                      const RECT* clip = 0)
{
    D3DVIEWPORT9 vp = { 0, 0, w, h, 0.0f, 1.0f };
    dev->SetViewport(&vp);
    dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    if (clip)
        dev->SetScissorRect(clip);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, clip ? TRUE : FALSE);
    dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xf);
    dev->SetVertexDeclaration(s_decl);
    dev->SetVertexShader(s_vs);
    dev->SetPixelShader(ps);
    dev->SetTexture(0, src);
    for (DWORD s = 1; s < 4; s++)
        dev->SetTexture(s, 0);
    DWORD f = linear ? D3DTEXF_LINEAR : D3DTEXF_POINT;
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, f);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, f);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
    dev->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
    // D3D9 maps pixel centres to integer coordinates: move the quad up-left by half a pixel so texel i's centre lands on
    // pixel i's centre
    float dx = -1.0f / w, dy = 1.0f / h;
    quad_vertex q[4] = {
        { -1.0f + dx,  1.0f + dy, 0.5f, 1.0f, 0.0f, 0.0f },
        {  1.0f + dx,  1.0f + dy, 0.5f, 1.0f, 1.0f, 0.0f },
        { -1.0f + dx, -1.0f + dy, 0.5f, 1.0f, 0.0f, 1.0f },
        {  1.0f + dx, -1.0f + dy, 0.5f, 1.0f, 1.0f, 1.0f },
    };
    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(quad_vertex));
}

// FsrEasuCon (ffx_fsr1.h), as floats: input viewport = input size = the scene (top-left aligned), output = backbuffer
static void fsr_easu_con(float con[16], float in_w, float in_h, float out_w, float out_h)
{
    con[0] = in_w * (1.0f / out_w);
    con[1] = in_h * (1.0f / out_h);
    con[2] = 0.5f * in_w * (1.0f / out_w) - 0.5f;
    con[3] = 0.5f * in_h * (1.0f / out_h) - 0.5f;
    con[4] = 1.0f / in_w;
    con[5] = 1.0f / in_h;
    con[6] = 1.0f * (1.0f / in_w);
    con[7] = -1.0f * (1.0f / in_h);
    con[8] = -1.0f * (1.0f / in_w);
    con[9] = 2.0f * (1.0f / in_h);
    con[10] = 1.0f * (1.0f / in_w);
    con[11] = 2.0f * (1.0f / in_h);
    con[12] = 0.0f * (1.0f / in_w);
    con[13] = 4.0f * (1.0f / in_h);
    con[14] = con[15] = 0.0f;
}

// FsrRcasCon: sharpness in stops -> the linear factor
static float fsr_rcas_con(float sharpness)
{
    return (float)pow(2.0, -(double)sharpness);
}

// The scene target -> backbuffer. RT 0 and the depth-stencil end as the game had them before the redirect.
static void resolve(IDirect3DDevice9* dev)
{
    if (!s_active)
        return;
    s_active = false;
    g_gfx_in_pass = 1;
    IDirect3DStateBlock9* sb = 0;
    dev->CreateStateBlock(D3DSBT_ALL, &sb);
    IDirect3DSurface9* bb = 0;
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    RECT r = { 0, 0, (LONG)s_sw, (LONG)s_sh };
    if (s_scene_ms)
        dev->StretchRect(s_scene_ms, &r, s_scene_surf, &r, D3DTEXF_NONE);
    // our depth-stencil leaves before the passes: they draw at the backbuffer's size
    if (s_saved_ds) {
        dev->SetDepthStencilSurface(s_saved_ds);
        release(s_saved_ds);
    }
    // the backbuffer keeps what is outside the redirected passes' viewports (the campaign map's bar below its view)
    bool whole = s_rect.left == 0 && s_rect.top == 0 && s_rect.right == (LONG)s_bw && s_rect.bottom == (LONG)s_bh;
    const RECT* clip = whole ? 0 : &s_rect;
    // the backbuffer-sized image RCAS sharpens: EASU's or the downsample's output, or the native-size scene itself
    IDirect3DTexture9* rcas_in = 0;
    if (s_sw < s_bw || s_sh < s_bh) {                   // upscale
        if (g_gfx.fsr && s_easu_tex) {
            float con[16];
            fsr_easu_con(con, (float)s_sw, (float)s_sh, (float)s_bw, (float)s_bh);
            dev->SetRenderTarget(0, s_easu_surf);
            dev->SetPixelShaderConstantF(0, con, 4);
            draw_quad(dev, s_bw, s_bh, s_ps_easu, s_scene_tex, false);
            rcas_in = s_easu_tex;
        } else {
            dev->SetRenderTarget(0, bb);
            draw_quad(dev, s_bw, s_bh, s_ps_copy, s_scene_tex, true, clip);
        }
    } else if (s_sw == s_bw && s_sh == s_bh) {          // native: only with FSR, RCAS only
        if (g_gfx.fsr) {
            rcas_in = s_scene_tex;
        } else {
            dev->SetRenderTarget(0, bb);
            draw_quad(dev, s_bw, s_bh, s_ps_copy, s_scene_tex, false, clip);
        }
    } else {                                            // supersampled: downsample, to RCAS's input with FSR
        bool sharpen = g_gfx.fsr && s_easu_tex;
        dev->SetRenderTarget(0, sharpen ? s_easu_surf : bb);
        if (s_sw == 2 * s_bw && s_sh == 2 * s_bh) {
            draw_quad(dev, s_bw, s_bh, s_ps_copy, s_scene_tex, true, sharpen ? 0 : clip);   // the tap at each 2x2 block's centre: a box
        } else {
            float dc[4] = { (float)s_sw / s_bw, (float)s_sh / s_bh, 1.0f / s_sw, 1.0f / s_sh };
            dev->SetPixelShaderConstantF(5, dc, 1);
            draw_quad(dev, s_bw, s_bh, s_ps_down, s_scene_tex, false, sharpen ? 0 : clip);
        }
        if (sharpen)
            rcas_in = s_easu_tex;
    }
    if (rcas_in) {
        float rc[4] = { fsr_rcas_con(g_gfx.fsr_sharpness), 0.0f, 1.0f / s_bw, 1.0f / s_bh };
        dev->SetRenderTarget(0, bb);
        dev->SetPixelShaderConstantF(4, rc, 1);
        draw_quad(dev, s_bw, s_bh, s_ps_rcas, rcas_in, false, clip);
    }
    if (sb) {
        sb->Apply();
        sb->Release();
    }
    bb->Release();
    g_gfx_in_pass = 0;
}

// the backbuffer as it is -> the scene target, before a scene that does not clear its colour continues on it
static void copy_in(IDirect3DDevice9* dev, IDirect3DSurface9* bb)
{
    if (!s_copy_tex && !make_tex(dev, s_bw, s_bh, &s_copy_tex, &s_copy_surf)) {
        release(s_copy_surf);
        release(s_copy_tex);
        return;
    }
    g_gfx_in_pass = 1;
    IDirect3DStateBlock9* sb = 0;
    dev->CreateStateBlock(D3DSBT_ALL, &sb);
    dev->StretchRect(bb, 0, s_copy_surf, 0, D3DTEXF_NONE);
    dev->SetRenderTarget(0, scene_target());
    draw_quad(dev, s_sw, s_sh, s_ps_copy, s_copy_tex, true);
    if (sb) {
        sb->Apply();
        sb->Release();
    }
    g_gfx_in_pass = 0;
}

static void scale_vp(D3DVIEWPORT9* v)
{
    UINT x1 = (UINT)floor((v->X + v->Width) * (double)s_sw / s_bw + 0.5);
    UINT y1 = (UINT)floor((v->Y + v->Height) * (double)s_sh / s_bh + 0.5);
    v->X = (DWORD)floor(v->X * (double)s_sw / s_bw + 0.5);
    v->Y = (DWORD)floor(v->Y * (double)s_sh / s_bh + 0.5);
    v->Width = x1 > v->X ? x1 - v->X : 1;
    v->Height = y1 > v->Y ? y1 - v->Y : 1;
}

bool gfx_scale_scene_begin(IDirect3DDevice9* dev, int perspective, int clears_colour, D3DVIEWPORT9* vp, D3DVIEWPORT9* alt)
{
    if (!s_usable || !gfx_scaling())
        return false;
    IDirect3DSurface9* rt = 0;
    IDirect3DSurface9* bb = 0;
    dev->GetRenderTarget(0, &rt);
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    D3DSURFACE_DESC bd;
    bb->GetDesc(&bd);
    bool ours = s_active && rt == scene_target();
    bool on_bb = rt == bb;
    // the 3D view: the whole backbuffer, or most of it (the campaign map's view stops above its bar: MapWidget at 0.04 ..
    // 0.75 of the GUI's height, 1280x681 of 1280x720); the GUI's small 3D views (portraits, the face generator) are not
    bool inside = vp->X + vp->Width <= bd.Width && vp->Y + vp->Height <= bd.Height;
    bool big = inside && vp->Width * 4 >= bd.Width * 3 && vp->Height * 4 >= bd.Height * 3;
    bool want = (ours || on_bb) && perspective && big;
    if (g_gfx.log) {
        // the first passes, then each kind of pass the first time it shows (the menu alone fills the first 400)
        D3DSURFACE_DESC rd;
        rd.Width = rd.Height = 0;
        rt->GetDesc(&rd);
        unsigned sig[8] = { (unsigned)perspective, (unsigned)clears_colour, vp->X, vp->Y, vp->Width, vp->Height,
                            ours ? 2u : on_bb ? 1u : 0u, alt ? (alt->Width << 16 | alt->Height) : 0u };
        static unsigned seen[128][8];
        static int nseen;
        bool fresh = true;
        for (int i = 0; i < nseen && fresh; i++)
            fresh = memcmp(seen[i], sig, sizeof(sig)) != 0;
        if (fresh && nseen < 128)
            memcpy(seen[nseen++], sig, sizeof(sig));
        if (s_log_left || fresh) {
            if (s_log_left)
                s_log_left--;
            gfx_log("frame %u scene: persp %d clear %d vp %lu,%lu %lux%lu%s rt %s %ux%u -> %s\n", s_frame, perspective,
                    clears_colour, vp->X, vp->Y, vp->Width, vp->Height, alt ? " (alt vp)" : "",
                    ours ? "ours" : on_bb ? "backbuffer" : "texture", rd.Width, rd.Height, want ? "scaled" : "native");
        }
    }
    bool redirected = false;
    if (ours && !want) {
        resolve(dev);                       // a pass on the backbuffer after the scene (the GUI): the scene goes first
        s_bb_drawn = true;
        s_valid = false;                    // the backbuffer now has more than our target
    } else if (ours) {
        redirected = true;
    } else if (on_bb && !want) {
        s_bb_drawn = true;
        s_valid = false;
    } else if (want) {
        IDirect3DSurface9* ds = 0;
        D3DSURFACE_DESC dd;
        bool replace_ds = SUCCEEDED(dev->GetDepthStencilSurface(&ds)) && ds && SUCCEEDED(ds->GetDesc(&dd))
                          && dd.Width == bd.Width && dd.Height == bd.Height;
        if (ensure_targets(dev, bb, replace_ds ? &dd : 0)) {
            bool start = !s_valid;
            bool copied = start && s_bb_drawn && !clears_colour;
            if (copied)
                copy_in(dev, bb);
            dev->SetRenderTarget(0, scene_target());
            if (replace_ds) {
                s_saved_ds = ds;
                ds = 0;
                dev->SetDepthStencilSurface(s_scene_ds);
            }
            if (start) {
                // the backbuffer's depth was cleared by the frame (GameApp::Render clears Z); ours starts the same.
                // (SetRenderTarget made the viewport the whole target.) Under a viewport that is not the whole
                // backbuffer the colour is cleared too, so that the resolve's taps along its edge read black, not an
                // old frame
                bool whole = vp->X == 0 && vp->Y == 0 && vp->Width == bd.Width && vp->Height == bd.Height;
                dev->Clear(0, 0, D3DCLEAR_ZBUFFER | (whole || copied ? 0 : D3DCLEAR_TARGET), 0, 1.0f, 0);
                s_valid = true;
                s_rect.left = s_rect.top = s_rect.right = s_rect.bottom = 0;
            }
            s_active = true;
            redirected = true;
        }
        if (ds)
            ds->Release();
    }
    if (redirected) {
        // what the resolve writes back: the union of the passes' viewports (the clear's too)
        for (int i = 0; i < 2; i++) {
            const D3DVIEWPORT9* v = i ? alt : vp;
            if (!v)
                continue;
            RECT r = { (LONG)v->X, (LONG)v->Y, (LONG)(v->X + v->Width), (LONG)(v->Y + v->Height) };
            if (r.right > (LONG)bd.Width) r.right = bd.Width;
            if (r.bottom > (LONG)bd.Height) r.bottom = bd.Height;
            if (s_rect.right <= s_rect.left || s_rect.bottom <= s_rect.top) {
                s_rect = r;
            } else {
                if (r.left < s_rect.left) s_rect.left = r.left;
                if (r.top < s_rect.top) s_rect.top = r.top;
                if (r.right > s_rect.right) s_rect.right = r.right;
                if (r.bottom > s_rect.bottom) s_rect.bottom = r.bottom;
            }
        }
        scale_vp(vp);
        if (alt)
            scale_vp(alt);
    }
    rt->Release();
    bb->Release();
    return redirected;
}

void gfx_scale_end_scene(IDirect3DDevice9* dev)
{
    resolve(dev);
}

void gfx_scale_present(IDirect3DDevice9* dev)
{
    if (s_active) {
        dev->BeginScene();
        resolve(dev);
        gfx_real_EndScene(dev);
    }
    s_valid = false;
    s_bb_drawn = false;
    s_frame++;
}
