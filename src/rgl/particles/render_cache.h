// render_cache.h: the part of the Direct3D 9 render-state cache (global object 0xa17910) that mesh_obj.cpp
// uses: the blend-mode and texture-coordinate-generation setters (0x40e940, 0x40ea00) and the fields the mesh
// material setup (0x40f3f0) reads. It is the same object as src/game/renderer.h's `renderer` (which has the
// rest of the fields and the other setters); that header cannot be included next to src/rgl/resource.h (both
// define global `material` and `mesh_inst`-style classes), so this module keeps its own view of it under
// another name. Merge into one `renderer` class when the headers are unified.
#ifndef RGL_RENDER_CACHE_H
#define RGL_RENDER_CACHE_H
#include <windows.h>
#include <d3d9.h>

struct render_cache {
    char pad000[0x574];
    int stage_xform[3];              // 0x574 per-stage D3DTSS_TEXTURETRANSFORMFLAGS cache   (setup_material 0x40f3f0)
    int stage_texgen[3];             // 0x580 per-stage TEXCOORDINDEX mode cache             (0x40ea00)
    int blend_mode;                  // 0x58c 0 off, 1 alpha, 2 additive, else multiply      (0x40e940)
    char pad590[4];
    unsigned tex_factor;             // 0x594 cached D3DRS_TEXTUREFACTOR                     (0x40f3f0)
    char pad598[0x618 - 0x598];
    IDirect3DDevice9* dev;           // 0x618
    void set_blend_mode(int mode);                  // 0x40e940
    void set_texcoord_gen(int stage, int mode);     // 0x40ea00
};
extern render_cache g_render;                 // 0xa17910

#endif
