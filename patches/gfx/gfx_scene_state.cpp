// patches/gfx/gfx_scene_state.cpp: RenderScene::apply_state with the gfx set's scene redirect (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose when gfx_render_scale != 1 or gfx_fsr = 1 (rgl_config.txt): before the pass sets
// its viewport it asks gfx_scale_scene_begin (gfx_scale.cpp) whether it draws the 3D view; if so render target 0 is the
// scaled scene target and the viewports below are the pass's own, rescaled to it. Otherwise (and always with the defaults) this is the
// faithful function of src/game/terrain_a/terrain_render.cpp, copied as it is with the viewports read from locals.
#include "../../src/game/terrain_a/common.h"
#include "../../src/rgl/options.h"   // g_options
#include "../../src/game/terrain_a/terrain.h"
#include "../../src/game/terrain_a/game_state.h"
#include "../../src/game/terrain_a/misc.h"
#include "../../src/game/render_batch.h"
#include "../../src/include/frustum.h"
#include "../../src/include/mat4.h"
#include "./gfx.h"

// @ replaces 0x46ca50
void RenderScene::apply_state()
{
    D3DVIEWPORT9 vp;
    DWORD clear = 0;
    // gfx: the scaled scene target, when this pass draws the 3D view (gfx_scale.cpp); both viewports are rescaled to it
    D3DVIEWPORT9 local = vp_local, alt = vp_alt;
    gfx_scale_scene_begin(g_render.dev, cam.perspective, (flags & 0x100) != 0, &local, use_alt_vp ? &alt : 0);
    if (use_alt_vp) {
        vp.X = alt.X;
        vp.Y = alt.Y;
        vp.Width = alt.Width;
        vp.Height = alt.Height;
        vp.MinZ = alt.MinZ;
        vp.MaxZ = alt.MaxZ;
    } else {
        vp.X = local.X;
        vp.Y = local.Y;
        vp.Width = local.Width;
        vp.Height = local.Height;
        vp.MinZ = local.MinZ;
        vp.MaxZ = local.MaxZ;
    }
    g_render.dev->SetViewport(&vp);
    if (flags & 0x100)
        clear = D3DCLEAR_TARGET;
    if (flags & 0x200)
        clear |= D3DCLEAR_ZBUFFER;
    if (clear)
        g_render.dev->Clear(0, 0, clear, clear_color, 1.0f, 0);
    if (use_alt_vp) {
        vp.X = local.X;
        vp.Y = local.Y;
        vp.Width = local.Width;
        vp.Height = local.Height;
        vp.MinZ = local.MinZ;
        vp.MaxZ = local.MaxZ;
        g_render.dev->SetViewport(&vp);
    }
    if (msaa)
        g_render.dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 1);
    else
        g_render.dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if (clip_on) {
        if (g_use_shaders == 0) {
            g_render.dev->SetClipPlane(0, clip_plane);
            g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
        }
    } else {
        g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    }
    rgl::frustum* c = &cam;
    {
        D3DXMATRIX proj = c->projection();
        g_render.dev->SetTransform(D3DTS_PROJECTION, &proj);
    }
    g_render.set_view((const rgl::mat4*)&c->m);
    if (cull_flip) {
        if (g_render.use_hw_shadow)
            g_render.cull_setting = 1;
        else
            g_render.cull_setting = cam.left_handed ? 2 : 3;
    } else {
        g_render.cull_setting = (cam.left_handed != 0) + 2;
    }
    unsigned char fog_enable = (fog && fog->enabled) ? 1 : 0;
    if (g_render.b505 != fog_enable) {
        g_render.b505 = fog_enable;
        g_render.dev->SetRenderState(D3DRS_FOGENABLE, fog_enable);
    }
    if (fog && fog->enabled) {
        bool below = cam.m.o.z < fog->level;
        if (fog_invert)
            below = !below;
        if (below) {
            g_render.dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_render.dev->SetRenderState(D3DRS_FOGCOLOR, fog->b_color);
            g_render.dev->SetRenderState(D3DRS_FOGSTART, fog->b_start);
            g_render.dev->SetRenderState(D3DRS_FOGEND, fog->b_end);
        } else {
            g_render.dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_render.dev->SetRenderState(D3DRS_FOGCOLOR, fog->a_color);
            g_render.dev->SetRenderState(D3DRS_FOGSTART, fog->a_start);
            g_render.dev->SetRenderState(D3DRS_FOGEND, fog->a_end);
        }
    }
    if (g_use_shaders == 0) {
        if (lights.size() > 0 && lights_active && g_options.char_detail) {
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
                g_render.dev->SetLight(i, &L);
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
                g_render.dev->SetLight(i, &L);
            }
        } else {
            lights_active = 0;
            if (g_render.b506) {
                g_render.b506 = 0;
                g_render.dev->SetRenderState(D3DRS_LIGHTING, 0);
            }
        }
    }
}
