// patches/gfx/gfx.h: what the gfx patch set's objects share (docs/patches.md, "gfx").
//
//   gfx_config.cpp       the four rgl_config.txt keys: read at start-up, written back by rgl_config::save (0x415fb0)
//   gfx_device.cpp       Direct3DCreate9 import-slot hook -> CreateDevice -> device vtable hooks; anisotropic filtering
//   gfx_scale.cpp        the scaled scene target and the resolve to the backbuffer (bilinear, or FSR 1 EASU + RCAS)
//   gfx_scene_state.cpp  RenderScene::apply_state (0x46ca50) with the scene redirected and its viewport scaled
//
// Only plain types cross these files, so each can include d3d9.h its own way (gfx_device.cpp uses CINTERFACE for the
// vtable offsets).
#pragma once

struct IDirect3DDevice9;
struct _D3DVIEWPORT9;

struct gfx_settings {
    int anisotropy;             // gfx_anisotropy: 0 (off), 2, 4, 8 or 16; capped by D3DCAPS9.MaxAnisotropy
    float render_scale;         // gfx_render_scale: 0.25 .. 1.0, the 3D scene's size per axis
    int fsr;                    // gfx_fsr: 0 bilinear upscale, 1 FSR 1 (EASU + RCAS)
    float fsr_sharpness;        // gfx_fsr_sharpness: RCAS stops, 0 = sharpest .. 2
    int log;                    // gfx_log: 1 writes gfx_log.txt in the game folder (diagnostics)
};
extern gfx_settings g_gfx;

void gfx_load_settings();                               // gfx_config.cpp (idempotent)
bool gfx_scaling();                                     // render_scale < 1: the scene is redirected
void gfx_log(const char* fmt, ...);                     // gfx_config.cpp: only when g_gfx.log

// gfx_device.cpp: the real (unhooked) entry points, for the other files' own passes
extern long (__stdcall* gfx_real_EndScene)(IDirect3DDevice9*);
extern long (__stdcall* gfx_real_SetSamplerState)(IDirect3DDevice9*, unsigned long, int, unsigned long);
extern int g_gfx_in_pass;                               // set while gfx_scale.cpp draws: the hooks pass everything through

// gfx_scale.cpp
void gfx_scale_device_created(IDirect3DDevice9* dev);   // shaders, vertex declaration, caps
void gfx_scale_release();                               // before Reset: the D3DPOOL_DEFAULT targets
void gfx_scale_end_scene(IDirect3DDevice9* dev);        // EndScene hook: resolve a redirected scene first
void gfx_scale_present(IDirect3DDevice9* dev);          // Present hook: resolve, then the frame's targets go stale
// apply_state: called before the viewport is set. Returns true when this scene draws into the scaled target; vp and
// alt (may be 0) are then rescaled to it.
bool gfx_scale_scene_begin(IDirect3DDevice9* dev, int perspective, int clears_colour, _D3DVIEWPORT9* vp, _D3DVIEWPORT9* alt);
