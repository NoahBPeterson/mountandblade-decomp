// patches/gfx/gfx_device.cpp: the Direct3D 9 hooks of the gfx set, and anisotropic filtering (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose (only when a gfx_* key in rgl_config.txt asks for it; with the defaults the hooks pass
// every call through).
//
// Hooks. A static constructor swaps our Direct3DCreate9 import slot (patches/devfast/iat_hook.h); the replacement patches
// the IDirect3D9 vtable's CreateDevice, which patches the device vtable's Reset, Present, EndScene and SetSamplerState.
// The game reaches the device only through its vtable (and so does d3dx9_31's effect framework), so every call passes
// here. No function of the reconstruction is replaced by this file. The hooks are set whatever the settings, since the
// launcher dialog and the Video Options page can turn the features on later (gfx_apply_saved, applied at a Present).
//
// Anisotropic filtering (gfx_anisotropy = N > 0). The game sets D3DSAMP_MAXANISOTROPY to min(caps, 5) and a LINEAR
// or ANISOTROPIC MINFILTER from its own option (renderer::reset_state 0x4617b0); the effect file sets its own sampler
// states. Here:
//   - D3DSAMP_MAXANISOTROPY is always N (capped by D3DCAPS9.MaxAnisotropy), on every sampler after CreateDevice/Reset
//     and whenever the game sets it;
//   - a LINEAR MINFILTER becomes ANISOTROPIC only while that sampler's MIPFILTER is not NONE (mip-mapped textures).
//     POINT stays POINT (CD3DFont's text, src/dxframework/d3dfont.cpp), and so do un-mipped LINEAR samplers (render
//     targets, shadow maps). The MINFILTER is re-derived when the MIPFILTER changes later.
//
// Render scale. CreateDevice drops D3DCREATE_PUREDEVICE (gfx_scale.cpp saves and restores the game's device state around
// its passes with a state block, which a pure device does not offer reliably; the game asks for hardware vertex
// processing without it anyway, flags 0x40 under CrossOver).
#define CINTERFACE
#include <windows.h>
#include <d3d9.h>
#include <stddef.h>
#include "../devfast/iat_hook.h"
#include "./gfx.h"

long (__stdcall* gfx_real_EndScene)(IDirect3DDevice9*);
long (__stdcall* gfx_real_SetSamplerState)(IDirect3DDevice9*, unsigned long, int, unsigned long);
int g_gfx_in_pass;
int g_gfx_device_made;
int g_gfx_apply_pending;

typedef IDirect3D9* (WINAPI* create9_fn)(UINT);
typedef HRESULT (STDMETHODCALLTYPE* create_device_fn)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef HRESULT (STDMETHODCALLTYPE* reset_fn)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef HRESULT (STDMETHODCALLTYPE* present_fn)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT (STDMETHODCALLTYPE* end_scene_fn)(IDirect3DDevice9*);
typedef HRESULT (STDMETHODCALLTYPE* sampler_fn)(IDirect3DDevice9*, DWORD, D3DSAMPLERSTATETYPE, DWORD);

static create9_fn s_create9;
static create_device_fn s_create_device;
static reset_fn s_reset;
static present_fn s_present;
static end_scene_fn s_end_scene;
static sampler_fn s_sampler;

// ---- anisotropy -------------------------------------------------------------------------------------------------------
enum { NSAMP = 16 };
static DWORD s_level;                       // the MAXANISOTROPY to force, 0 = feature off
static DWORD s_caps_aniso;                  // D3DCAPS9.MaxAnisotropy
static DWORD s_min[NSAMP], s_mip[NSAMP];    // what the game last asked for
static DWORD s_max_req[NSAMP];              // the MAXANISOTROPY the game last asked for
static DWORD s_set_min[NSAMP];              // what we last passed on

static DWORD effective_min(DWORD s)
{
    if (s_min[s] == D3DTEXF_LINEAR && s_mip[s] != D3DTEXF_NONE)
        return D3DTEXF_ANISOTROPIC;
    return s_min[s];
}

static void aniso_device_state(IDirect3DDevice9* dev)
{
    // the device defaults after CreateDevice / Reset: MINFILTER POINT, MIPFILTER NONE
    for (DWORD s = 0; s < NSAMP; s++) {
        s_min[s] = s_set_min[s] = D3DTEXF_POINT;
        s_mip[s] = D3DTEXF_NONE;
        s_max_req[s] = 1;
        if (s_level)
            s_sampler(dev, s, D3DSAMP_MAXANISOTROPY, s_level);
    }
}

static HRESULT STDMETHODCALLTYPE hook_sampler(IDirect3DDevice9* dev, DWORD s, D3DSAMPLERSTATETYPE type, DWORD value)
{
    if (g_gfx_in_pass || s >= NSAMP)
        return s_sampler(dev, s, type, value);
    // tracked with the feature off too, so that turning it on or off later (aniso_change) knows what the game asked for
    if (type == D3DSAMP_MAXANISOTROPY)
        s_max_req[s] = value;
    else if (type == D3DSAMP_MINFILTER)
        s_min[s] = value;
    else if (type == D3DSAMP_MIPFILTER)
        s_mip[s] = value;
    if (g_gfx.log && (type == D3DSAMP_MAXANISOTROPY || type == D3DSAMP_MINFILTER) && value < 32) {
        // each (sampler, state, value) the game or its effect asks for, once: mb.fx's samplers ask for ANISOTROPIC
        // and MaxAnisotropy 4 themselves
        static DWORD seen[NSAMP][2];
        DWORD bit = 1u << value, *m = &seen[s][type == D3DSAMP_MINFILTER];
        if (!(*m & bit)) {
            *m |= bit;
            gfx_log("sampler %lu: %s %lu asked (level %lu)\n", s, type == D3DSAMP_MINFILTER ? "MINFILTER" : "MAXANISOTROPY",
                    value, s_level);
        }
    }
    if (!s_level) {
        if (type == D3DSAMP_MINFILTER)
            s_set_min[s] = value;
        return s_sampler(dev, s, type, value);
    }
    switch (type) {
    case D3DSAMP_MAXANISOTROPY:
        return s_sampler(dev, s, type, s_level);
    case D3DSAMP_MINFILTER:
        s_set_min[s] = effective_min(s);
        return s_sampler(dev, s, type, s_set_min[s]);
    case D3DSAMP_MIPFILTER: {
        HRESULT hr = s_sampler(dev, s, type, value);
        DWORD m = effective_min(s);
        if (m != s_set_min[s]) {
            s_set_min[s] = m;
            s_sampler(dev, s, D3DSAMP_MINFILTER, m);
        }
        return hr;
    }
    default:
        return s_sampler(dev, s, type, value);
    }
}

static DWORD aniso_level(int asked)
{
    DWORD l = (DWORD)asked;
    if (l > s_caps_aniso)
        l = s_caps_aniso;
    return l < 2 ? 0 : l;
}

// a new level at run time: every sampler's MAXANISOTROPY and MINFILTER as the game asked, through the new level
static void aniso_change(IDirect3DDevice9* dev, DWORD level)
{
    if (level == s_level)
        return;
    s_level = level;
    for (DWORD s = 0; s < NSAMP; s++) {
        s_sampler(dev, s, D3DSAMP_MAXANISOTROPY, s_level ? s_level : s_max_req[s]);
        DWORD m = s_level ? effective_min(s) : s_min[s];
        if (m != s_set_min[s]) {
            s_set_min[s] = m;
            s_sampler(dev, s, D3DSAMP_MINFILTER, m);
        }
    }
}

// ---- device hooks -----------------------------------------------------------------------------------------------------
static HRESULT STDMETHODCALLTYPE hook_reset(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* pp)
{
    gfx_scale_release();                    // our D3DPOOL_DEFAULT targets, or Reset fails
    HRESULT hr = s_reset(dev, pp);
    gfx_log("Reset %ux%u ms %d: hr %08lx\n", pp->BackBufferWidth, pp->BackBufferHeight, pp->MultiSampleType, hr);
    if (SUCCEEDED(hr))
        aniso_device_state(dev);
    return hr;
}

static HRESULT STDMETHODCALLTYPE hook_present(IDirect3DDevice9* dev, CONST RECT* a, CONST RECT* b, HWND w, CONST RGNDATA* r)
{
    if (!g_gfx_in_pass) {
        gfx_scale_present(dev);
        if (g_gfx_apply_pending) {
            // a UI changed the settings (gfx_apply_saved): no redirected scene is in flight between frames
            g_gfx_apply_pending = 0;
            g_gfx = g_gfx_saved;
            aniso_change(dev, aniso_level(g_gfx.anisotropy));
            gfx_scale_release();            // the targets are remade at the new scale by the next scene
            gfx_scale_device_created(dev);  // (the shaders, if the first device could not make them)
            gfx_log("applied: anisotropy %lu, render_scale %.3f, fsr %d, fsr_sharpness %.3f\n", s_level,
                    g_gfx.render_scale, g_gfx.fsr, g_gfx.fsr_sharpness);
        }
    }
    return s_present(dev, a, b, w, r);
}

static HRESULT STDMETHODCALLTYPE hook_end_scene(IDirect3DDevice9* dev)
{
    if (!g_gfx_in_pass)
        gfx_scale_end_scene(dev);
    return s_end_scene(dev);
}

static void put_slot(void** vtbl, size_t off, void* fn, void** save)
{
    void** slot = (void**)((char*)vtbl + off);
    if (*slot == fn)
        return;
    DWORD old;
    VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    if (save)
        *save = *slot;
    *slot = fn;
    VirtualProtect(slot, sizeof(void*), old, &old);
}

static HRESULT STDMETHODCALLTYPE hook_create_device(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD flags,
                                                   D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** out)
{
    flags &= ~D3DCREATE_PUREDEVICE;
    HRESULT hr = s_create_device(d3d, adapter, type, focus, flags, pp, out);
    gfx_log("CreateDevice flags %08lx %ux%u fmt %d ms %d: hr %08lx\n", flags, pp->BackBufferWidth, pp->BackBufferHeight,
            pp->BackBufferFormat, pp->MultiSampleType, hr);
    if (FAILED(hr) || !*out)
        return hr;
    IDirect3DDevice9* dev = *out;
    void** vt = (void**)dev->lpVtbl;
    if (!s_reset) {
        put_slot(vt, offsetof(IDirect3DDevice9Vtbl, Reset), (void*)hook_reset, (void**)&s_reset);
        put_slot(vt, offsetof(IDirect3DDevice9Vtbl, Present), (void*)hook_present, (void**)&s_present);
        put_slot(vt, offsetof(IDirect3DDevice9Vtbl, EndScene), (void*)hook_end_scene, (void**)&s_end_scene);
        put_slot(vt, offsetof(IDirect3DDevice9Vtbl, SetSamplerState), (void*)hook_sampler, (void**)&s_sampler);
        gfx_real_EndScene = (long (__stdcall*)(struct IDirect3DDevice9*))s_end_scene;
        gfx_real_SetSamplerState = (long (__stdcall*)(struct IDirect3DDevice9*, unsigned long, int, unsigned long))s_sampler;
    }
    D3DCAPS9 caps;
    s_caps_aniso = SUCCEEDED(dev->lpVtbl->GetDeviceCaps(dev, &caps)) ? caps.MaxAnisotropy : 0;
    if (g_gfx_apply_pending) {              // (the launcher applies before the device; nothing should be pending)
        g_gfx_apply_pending = 0;
        g_gfx = g_gfx_saved;
    }
    s_level = aniso_level(g_gfx.anisotropy);
    gfx_log("anisotropy: asked %d, caps %lu -> %lu\n", g_gfx.anisotropy, s_caps_aniso, s_level);
    aniso_device_state(dev);
    g_gfx_device_made = 1;
    gfx_scale_device_created(dev);
    return hr;
}

static IDirect3D9* WINAPI hook_create9(UINT sdk)
{
    IDirect3D9* d3d = s_create9(sdk);
    if (d3d)
        put_slot((void**)d3d->lpVtbl, offsetof(IDirect3D9Vtbl, CreateDevice), (void*)hook_create_device, (void**)&s_create_device);
    return d3d;
}

static struct gfx_init {
    gfx_init()
    {
        gfx_load_settings();
        s_create9 = (create9_fn)iat_hook("d3d9.dll", "Direct3DCreate9", (void*)hook_create9);
        gfx_log("Direct3DCreate9 hook: %s\n", s_create9 ? "set" : "IMPORT NOT FOUND");
    }
} s_gfx_init;
