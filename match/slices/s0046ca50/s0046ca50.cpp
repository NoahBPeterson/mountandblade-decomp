// Slice s0046ca50: Scene::apply_state (0x46ca50): per-frame Direct3D 9 setup of a scene/view object: viewport
// and clear, projection and view transforms, cull mode, fog render states, clip plane, and the fixed-function
// light list (not used when shaders are on). ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- globals -----------------------------------------------------------------------------------
extern IDirect3DDevice9* g_device;      // 0xa17f28
extern int g_use_shaders;               // 0x5e28fc
extern int g_lights_enabled;            // 0x5aa024
extern char g_hw_shadow;                // 0xa17f24
extern int g_cull_mode;                 // 0xa17ecc (cached D3DCULL_* of the next draw)
extern unsigned char g_fog_on;                   // 0xa17e15 cached D3DRS_FOGENABLE
extern unsigned char g_lighting_on;              // 0xa17e16 cached D3DRS_LIGHTING

// ---- renderer (global at 0xa17910, see s00461280) ---------------------------------------------------
struct renderer {
    void set_view(const matrix* p);     // 0x461630
    void set_lighting(bool on);         // 0x461ba0
};
extern renderer g_render;               // 0xa17910

// ---- view frustum (layout from s0041e470 / s00461bd0) ------------------------------------------------
struct plane { vector4 pos, normal; };
struct frustum {
    char perspective;                   // +0
    char left_handed;                   // +1
    char pad2[2];
    int nplanes;                        // +4
    plane planes[6];                    // +8
    matrix m;                           // +0xc8
    float left, right, top, bottom, nearz, farz;   // +0x108
    D3DXMATRIX projection();            // 0x4620d0 (hidden return pointer)
};

// light source record (one per scene light); types 0/1/2 map to D3DLIGHT_POINT/SPOT/DIRECTIONAL
struct light_src {
    int type;                           // 0x00
    float diffuse[3];                   // 0x04
    int pad10;
    float specular[3];                  // 0x14
    int pad20;
    float pos[3];                       // 0x24
    int pad30[5];
    float dir[3];                       // 0x44
    int pad50;
    float range;                        // 0x54
    float atten0, atten1, atten2;       // 0x58
    float theta, phi;                   // 0x64
};
// fog record at Scene+0x1b0: two parameter sets selected by camera height against `level`
struct fog_rec {
    int f0;
    DWORD a_start;                      // 0x04
    DWORD a_end;                        // 0x08
    DWORD a_color;                      // 0x0c  (set A: camera at/above the level)
    char enabled;                       // 0x10
    char pad11[7];
    DWORD b_start;                      // 0x18
    DWORD b_end;                        // 0x1c
    DWORD b_color;                      // 0x20  (set B: camera below the level)
    float level;                        // 0x24
};

struct Scene {
    D3DVIEWPORT9 vp_local;              // 0x00
    D3DVIEWPORT9 vp_alt;                // 0x18
    frustum cam;                        // 0x30
    char pad150[0x160 - 0x150];
    char lights_active;                 // 0x160
    char clip_on;                       // 0x161
    char pad162[6];
    char cull_flip;                     // 0x168
    char fog_invert;                    // 0x169
    char msaa;                          // 0x16a
    char pad16b;
    float clip_plane[4];                // 0x16c
    float ambient[3];                   // 0x17c
    int pad188;
    unsigned flags;                     // 0x18c  bit 8: clear colour, bit 9: clear depth
    DWORD clear_color;                  // 0x190
    std::vector<light_src*> lights;     // 0x194 (first at +0x198)
    char pad1a4;
    char use_alt_vp;                    // 0x1a5
    char pad1a6[0x1b0 - 0x1a6];
    fog_rec* fog;                       // 0x1b0
    void apply_state();                 // 0x46ca50
};

// @ 0x46ca50
void Scene::apply_state()
{
    D3DVIEWPORT9 vp;
    DWORD clear = 0;
    if (use_alt_vp) {
        vp.X = vp_alt.X;
        vp.Y = vp_alt.Y;
        vp.Width = vp_alt.Width;
        vp.Height = vp_alt.Height;
        vp.MinZ = vp_alt.MinZ;
        vp.MaxZ = vp_alt.MaxZ;
    } else {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
    }
    g_device->SetViewport(&vp);
    if (flags & 0x100)
        clear = D3DCLEAR_TARGET;
    if (flags & 0x200)
        clear |= D3DCLEAR_ZBUFFER;
    if (clear)
        g_device->Clear(0, 0, clear, clear_color, 1.0f, 0);
    if (use_alt_vp) {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
        g_device->SetViewport(&vp);
    }
    if (msaa)
        g_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 1);
    else
        g_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if (clip_on) {
        if (g_use_shaders == 0) {
            g_device->SetClipPlane(0, clip_plane);
            g_device->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
        }
    } else {
        g_device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    }
    frustum* c = &cam;
    {
        D3DXMATRIX proj = c->projection();
        g_device->SetTransform(D3DTS_PROJECTION, &proj);
    }
    g_render.set_view(&c->m);
    if (cull_flip) {
        if (g_hw_shadow)
            g_cull_mode = 1;
        else
            g_cull_mode = cam.left_handed ? 2 : 3;
    } else {
        g_cull_mode = (cam.left_handed != 0) + 2;
    }
    unsigned char fog_enable = (fog && fog->enabled) ? 1 : 0;
    if (g_fog_on != fog_enable) {
        g_fog_on = fog_enable;
        g_device->SetRenderState(D3DRS_FOGENABLE, fog_enable);
    }
    if (fog && fog->enabled) {
        bool below = cam.m.o.z < fog->level;
        if (fog_invert)
            below = !below;
        if (below) {
            g_device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_device->SetRenderState(D3DRS_FOGCOLOR, fog->b_color);
            g_device->SetRenderState(D3DRS_FOGSTART, fog->b_start);
            g_device->SetRenderState(D3DRS_FOGEND, fog->b_end);
        } else {
            g_device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_device->SetRenderState(D3DRS_FOGCOLOR, fog->a_color);
            g_device->SetRenderState(D3DRS_FOGSTART, fog->a_start);
            g_device->SetRenderState(D3DRS_FOGEND, fog->a_end);
        }
    }
    if (g_use_shaders == 0) {
        if (lights.size() > 0 && lights_active && g_lights_enabled) {
            lights_active = 1;
            g_render.set_lighting(true);
            bool found_spot = false;
            D3DLIGHT9 L;
            unsigned i;
            for (i = 0; i < lights.size() && (int)i < 0x40; i++) {
                memset(&L, 0, sizeof(L));
                if (lights[i]->type == 0)
                    L.Type = D3DLIGHT_POINT;
                else if (lights[i]->type == 1)
                    L.Type = D3DLIGHT_SPOT;
                else if (lights[i]->type == 2)
                    L.Type = D3DLIGHT_DIRECTIONAL;
                L.Diffuse.r = lights[i]->diffuse[0];
                L.Diffuse.g = lights[i]->diffuse[1];
                L.Diffuse.b = lights[i]->diffuse[2];
                L.Ambient.r = 0;
                L.Ambient.g = 0;
                L.Ambient.b = 0;
                if (!found_spot && lights[i]->type == 2) {
                    found_spot = true;
                    L.Ambient.r = ambient[0];
                    L.Ambient.g = ambient[1];
                    L.Ambient.b = ambient[2];
                }
                L.Specular.r = lights[i]->specular[0];
                L.Specular.g = lights[i]->specular[1];
                L.Specular.b = lights[i]->specular[2];
                L.Position.x = lights[i]->pos[0];
                L.Position.y = lights[i]->pos[1];
                L.Position.z = lights[i]->pos[2];
                L.Direction.x = lights[i]->dir[0];
                L.Direction.y = lights[i]->dir[1];
                L.Direction.z = lights[i]->dir[2];
                L.Attenuation0 = lights[i]->atten0;
                L.Attenuation1 = lights[i]->atten1;
                L.Attenuation2 = lights[i]->atten2;
                L.Range = lights[i]->range;
                L.Theta = lights[i]->theta;
                L.Phi = lights[i]->phi;
                L.Falloff = 1.0f;
                g_device->SetLight(i, &L);
            }
            if (!found_spot) {
                memset(&L, 0, sizeof(L));
                L.Diffuse.r = 0;
                L.Diffuse.g = 0;
                L.Diffuse.b = 0;
                L.Type = D3DLIGHT_DIRECTIONAL;
                L.Ambient.r = ambient[0];
                L.Ambient.g = ambient[1];
                L.Ambient.b = ambient[2];
                L.Specular.r = 0;
                L.Specular.g = 0;
                L.Specular.b = 0;
                L.Position.x = 1000000.0f;
                L.Position.y = 0;
                L.Position.z = 0;
                L.Direction.x = -1.0f;
                L.Direction.y = 0;
                L.Direction.z = 0;
                L.Range = 1000000.0f;
                g_device->SetLight(i, &L);
            }
        } else {
            lights_active = 0;
            if (g_lighting_on) {
                g_lighting_on = 0;
                g_device->SetRenderState(D3DRS_LIGHTING, 0);
            }
        }
    }
}
