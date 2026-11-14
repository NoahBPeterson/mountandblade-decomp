//-----------------------------------------------------------------------------
// File: terrain_b.cpp  (0x46e590..0x470a00)
//
// Desc: second part of the terrain object's translation unit (TU 61 of work/migrate/tu_map.csv): the shader-path
//       scene renderer, the polygon-mesh topology object (bounding box, face normals, edge creases, adjacency,
//       nav-map loader / builder), the terrain's height generator, sun lighting of the world-map particles and
//       cell refresh, the Newton ragdoll clamp and step, face-generator helpers (morph relaxation, unpack,
//       breed), the mission's per-team statistics, spawn-rule choice and record-set destructors, the VC7.1
//       <vector> / <algorithm> instantiations of the object's records, and the id pools. Names provisional.
//       Functions in ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./terrain_b.h"
#include "../../rgl/options.h"   // g_options
#include "../config/config_block.h"   // g_cfg
#include "./frustum.h"
#include "./render_batch.h"
#include "./poly_mesh.h"
#include "../placed_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "./cell_grid.h"

#include "./terrain.h"
#include "./records.h"
#include "./game_data.h"
#include "./face.h"
#include "./troop_mgr.h"
#include "./shadow_map.h"
#include "./pool.h"
#include "./agent_rec.h"
#include "./mission.h"
#include "./physics.h"
#include "./face_gen.h"
#include "../animator.h"       // Animator: unpack 0x473800, breed 0x4746e0 (g_anim); include/skin.h Skin::relax 0x4735e0
#include "./tmesh.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../int_vec.h"
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/ptr_tag.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../game_globals.h"      // canonical g_module_settings.player_wounded_treshold / g_module_settings.hero_wounded_treshold (listed directly: the audit reads only the source's own includes)
#include "../../include/phys_set.h"
#include "../../include/height_field.h"
#include "../render_batch.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../shadow_map.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_desc.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/renderer.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_a/terrain.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;
using rgl::matrix;

// a x b with the x term's second product written b.y * a.z (the x87 load order of the billboard's forward vector)
static __forceinline vector4 cross_ba(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - b.y * a.z;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static inline void frame_set(frame4& f, const rot3& r, const vector4& o)
{
    f.rot = r;
    f.o = o;
}

// the texture-space bias of the shadow map; arguments are evaluated right to left, and the original computes the x offset first
static __forceinline D3DXMATRIX shadow_bias(float oy, float ox)
{
    return D3DXMATRIX(0.5f, 0.0f, 0.0f, 0.0f,
                      0.0f, -0.5f, 0.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      ox, oy, 0.0f, 1.0f);
}

// vector4::normalize (0x407d60) as callers inline it: the header's body (sums z, y, x like length())
struct vec4n : vector4 {
    __forceinline float normalize_inl()
    {
        float len = length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};

// @ 0x46e590
void render_batch::render_scene(scene* sc)
{
    vector4 light_diffuse[10];
    for (int i = 0; i < 10; i++) {
        light_diffuse[i].x = 0.0f;
        light_diffuse[i].y = 0.0f;
        light_diffuse[i].z = 0.0f;
        light_diffuse[i].w = 1.0f;
    }
    bool sun_shadow = false;
    vec3 light_pos[10];
    if (!sc->f162 && sc->f164 == 0) {
        sky_t* sky = scene_sky(sc);
        if (sky) {
            bool lt = sc->fr.m.o.z < sky->height;
            if (sc->f169)
                lt = !lt;
            if (lt) {
                sky_t* s = scene_sky(sc);
                unsigned c = s->fog_color_a;
                vector4 fog_col = unpack_color(&c);
                fog_col.w = 1.0f;
                g_effect->SetFloat("fFogStart", s->fog_start_a);
                g_effect->SetFloat("fFogEnd", scene_sky(sc)->fog_end_a);
                g_effect->SetFloat("fFogDensity", scene_sky(sc)->fog_dens_a);
                g_effect->SetVector("vFogColor", (D3DXVECTOR4*)&fog_col);
            } else {
                sky_t* s = scene_sky(sc);
                unsigned c = s->fog_color_b;
                vector4 fog_col = unpack_color(&c);
                fog_col.w = 1.0f;
                g_effect->SetFloat("fFogStart", s->fog_start_b);
                g_effect->SetFloat("fFogEnd", scene_sky(sc)->fog_end_b);
                g_effect->SetFloat("fFogDensity", scene_sky(sc)->fog_dens_b);
                g_effect->SetVector("vFogColor", (D3DXVECTOR4*)&fog_col);
            }

            vec3 sky_dir;
            sky_dir.x = 0.0f; sky_dir.y = 0.0f; sky_dir.z = -1.0f;
            vector4 sky_col;
            sky_col.x = 0.0f; sky_col.y = 0.0f; sky_col.z = 0.0f; sky_col.w = 1.0f;
            vector4 pc(0.01f, 0.01f, 0.01f);
            bool found = false;
            int npoint = 0;
            for (unsigned i = 1; i < scene_lights(sc).size(); i++) {
                if (scene_lights(sc)[i]->type == 2) {
                    if (!found) {
                        sky_col = scene_lights(sc)[i]->diffuse;
                        sky_col.w = 1.0f;
                        sky_dir = scene_lights(sc)[i]->sun_dir;
                        found = true;
                    }
                } else {
                    npoint++;
                    const vector4& d = scene_lights(sc)[i]->diffuse;
                    pc.x += d.x;
                    pc.y += d.y;
                    pc.z += d.z;
                }
            }
            if (npoint) {
                pc.w = 1.0f;
                float m = pc.y > pc.z ? pc.y : pc.z;
                if (pc.x > m)
                    m = pc.x;
                double k = 1.0f / m;
                pc.x = (float)(pc.x * k);
                pc.y = (float)(pc.y * k);
                pc.z = (float)(pc.z * k);
            }
            g_effect->SetValue("vPointLightColor", &pc, 16);
            g_effect->SetValue("vSkyLightDir", &sky_dir, 12);
            g_effect->SetValue("vSkyLightColor", &sky_col, 16);

            int n = 0;
            for (unsigned i = 0; i < scene_lights(sc).size() && n < 10; i++) {
                if (scene_lights(sc)[i]->type == 0) {
                    light_diffuse[n] = ((Light*)scene_lights(sc)[i])->sample();
                    light_diffuse[n].w = 1.0f;
                    light_pos[n] = scene_lights(sc)[i]->position;
                    n++;
                }
            }
            g_effect->SetValue("vLightPosDir", light_pos, 0x78);
            g_effect->SetValue("vLightDiffuse", light_diffuse, 0xa0);

            vector4 sun_col;
            vec3 sun_dir;
            if (scene_lights(sc).size() != 0 && scene_lights(sc)[0]->type == 2) {
                sun_col = scene_lights(sc)[0]->diffuse;
                sun_col.w = 0.0f;
                sun_dir = scene_lights(sc)[0]->sun_dir;
                if (scene_lights(sc)[0]->diffuse.length() > 0.1f && !(scene_lights(sc)[0]->flags & 2))
                    sun_shadow = true;
                else
                    sun_shadow = false;
            } else {
                sun_col = make4(0.0f, 0.0f, 0.0f, 0.0f);
                sun_dir = vec3(0.0f, 0.0f, -1.0f);
            }
            g_effect->SetValue("vSunDir", &sun_dir, 12);
            g_effect->SetValue("vSunColor", &sun_col, 16);

            if (scene_sky(sc)->shadow) {
                IDirect3DBaseTexture9* t = tex_current(scene_sky(sc)->shadow, 0.0f);
                g_effect->SetTexture("shadowmap_texture", t);
                D3DXMATRIX proj = scene_sky(sc)->shadow_cam.projection();
                D3DXMATRIX view = scene_sky(sc)->shadow_cam.view_matrix();
                D3DXMATRIX svp = view * proj * shadow_bias(0.5f / (float)scene_sky(sc)->shadow->height + 0.5f,
                                                           0.5f / (float)scene_sky(sc)->shadow->width + 0.5f);
                g_effect->SetMatrixTranspose("matSunViewProj", &svp);
            }
        } else {
            vector4 sky_col;
            sky_col.x = 0.0f; sky_col.y = 0.0f; sky_col.z = 0.0f; sky_col.w = 1.0f;
            g_effect->SetValue("vSkyLightColor", &sky_col, 16);
            vector4 sun_col;
            sun_col.x = 0.0f; sun_col.y = 0.0f; sun_col.z = 0.0f; sun_col.w = 0.0f;
            g_effect->SetValue("vSunColor", &sun_col, 16);
        }
    } else {
        if (sc->f164) {
            if (g_render.b54b) {
                g_render.b54b = 0;
                g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            }
            if (g_render.b549 != 1) {
                g_render.b549 = 1;
                g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
                } else {
            if (g_render.b54b != 1) {
                g_render.b54b = 1;
                g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
            }
            if (g_render.b549) {
                g_render.b549 = 0;
                g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
            }
        }
        g_render.set_blend_mode(1);   // 0x40e940
        if (g_render.alpha_mode)
            g_render.dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        g_render.alpha_mode = 0;
        if (g_render.b548) {
            g_render.dev->SetRenderState(D3DRS_NORMALIZENORMALS, 0);
            g_render.b548 = 0;
        }
        if (g_render.b547) {
            g_render.dev->SetRenderState(D3DRS_SPECULARENABLE, 0);
            g_render.b547 = 0;
        }
        if (g_render.b505) {
            g_render.b505 = 0;
            g_render.dev->SetRenderState(D3DRS_FOGENABLE, 0);
        }
    }

    // texture-space matrix for camera-space texgen: 0.5 * rotation of the camera, shifted by 0.5
    frame4 a;
    a.rot = *(rot3*)&sc->fr.m;
    a.rot.s.x *= 0.5f; a.rot.s.y *= 0.5f; a.rot.s.z *= 0.5f;
    a.rot.f.x *= 0.5f; a.rot.f.y *= 0.5f; a.rot.f.z *= 0.5f;
    a.rot.u.x *= 0.5f; a.rot.u.y *= 0.5f; a.rot.u.z *= 0.5f;
    a.o = vector4(0.5f, 0.5f, 0.5f);
    {
        frame4 b;
        rot3 idt;
        frame_set(b, *(rot3*)identity3((mat3*)&idt), vector4(0.0f, 0.0f, 0.0f));
        float ang = 1.5707964f;
        ((rgl::matrix*)&b)->rotate_about_s(&ang);
        a = *(frame4*)&(*(rgl::matrix*)&b * *(rgl::matrix*)&a);
    }
    a.rot.s.w = 0.0f; a.rot.f.w = 0.0f; a.rot.u.w = 0.0f; a.o.w = 1.0f;

    sc->ambient.w = 1.0f;
    g_effect->SetVector("vAmbientColor", (D3DXVECTOR4*)&sc->ambient);
    vector4 camp;
    camp = sc->fr.m.o;
    camp.w = 0.0f;
    g_effect->SetVector("vCameraPos", (D3DXVECTOR4*)&camp);

    D3DXMATRIX proj = sc->fr.projection();
    D3DXMATRIX view = sc->fr.view_matrix();
    g_effect->SetMatrixTranspose("matView", &view);
    D3DXMATRIX viewproj = view * proj;
    g_effect->SetMatrixTranspose("matViewProj", &viewproj);

    float tv = (float)(g_ticks[2] % 1000000000) * 1e-5f;
    if (g_render.time_var != tv) {
        g_render.time_var = tv;
        g_effect->SetFloat("time_var", tv);
    }

    if (sc->f161) {
        D3DXPLANE pl(scene_clip(sc).a, scene_clip(sc).b, scene_clip(sc).c, scene_clip(sc).d);
        D3DXMATRIX inv;
        D3DXMatrixInverse(&inv, 0, &view);
        D3DXMatrixTranspose(&inv, &inv);
        D3DXPLANE pl2;
        D3DXPlaneTransform(&pl2, &pl, &inv);
        D3DXMATRIX inv2;
        D3DXMatrixInverse(&inv2, 0, &proj);
        D3DXMatrixTranspose(&inv2, &inv2);
        D3DXPLANE pl3;
        D3DXPlaneTransform(&pl3, &pl2, &inv2);
        g_render.dev->SetClipPlane(0, (const float*)&pl3);
        g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
    }

    // ---- per-mesh loop --------------------------------------------------------------------------------------
    cur_part = 0;
    cur_mat = 0;
    cur_tech = 0;
    for (std::vector<item_ref>::iterator it = scene_items(sc).begin(); it != scene_items(sc).end(); ++it) {
        {
            bool tex_ok = true;
            int fa = it->m->slots[0].f4;
            int fb = it->m->slots[0].f0;
            if (cur_vf_a != fa || cur_vf_b != fb) {
                cur_vf_a = fa;
                cur_vf_b = fb;
                dev_buffer& v = vf[fb].bufs[fa];
                g_render.dev->SetVertexDeclaration(v.decl);
                g_render.dev->SetStreamSource(0, v.vb, 0, v.elem_size);
            }
            mesh* src = it->m->local_a0 ? it->m : it->m->parent;
            int ibk = src->slots[1].f4;
            if (cur_ib != ibk) {
                cur_ib = ibk;
                g_render.dev->SetIndices(ib.bufs[ibk].ib);
            }
            material* mat;
            if (!sc->f162 && !sc->f164) {
                skin_calls* part = (skin_calls*)it->m->res;
                if ((skin_calls*)cur_part != part) {
                    cur_part = (skin*)part;
                    if (scene_sky(sc) && scene_sky(sc)->fog_meshes && !(part->flags & 1)) {
                        if (g_render.b505 != 1) {
                            g_render.b505 = 1;
                            g_render.dev->SetRenderState(D3DRS_FOGENABLE, 1);
                        }
                    } else if (g_render.b505) {
                        g_render.b505 = 0;
                        g_render.dev->SetRenderState(D3DRS_FOGENABLE, 0);
                    }
                    part->set_state();
                    texture* t;
                    t = part->tex[0];
                    if (t) {
                        IDirect3DBaseTexture9* cur = tex_current(t, it->m->f48);
                        if (g_render.tex_cache[0] != cur) {
                            g_render.tex_cache[0] = cur;
                            g_effect->SetTexture("diffuse_texture", cur);
                        }
                    }
                    t = part->tex[1];
                    if (t) {
                        IDirect3DBaseTexture9* cur = tex_current(t, it->m->f48);
                        if (g_render.tex_cache[1] != cur) {
                            g_render.tex_cache[1] = cur;
                            g_effect->SetTexture("diffuse_texture_2", cur);
                        }
                    }
                    t = part->tex[2];
                    if (t) {
                        IDirect3DBaseTexture9* cur = tex_current(t, it->m->f48);
                        if (g_render.tex_cache[2] != cur) {
                            g_render.tex_cache[2] = cur;
                            g_effect->SetTexture("normal_texture", cur);
                        }
                    }
                    t = part->tex[4];
                    if (t) {
                        IDirect3DBaseTexture9* cur = tex_current(t, it->m->f48);
                        if (g_render.tex_cache[3] != cur) {
                            g_render.tex_cache[3] = cur;
                            g_effect->SetTexture("specular_texture", cur);
                        }
                    }
                    t = part->tex[3];
                    if (t) {
                        IDirect3DBaseTexture9* cur = tex_current(t, it->m->f48);
                        if (g_render.tex_cache[4] != cur) {
                            g_render.tex_cache[4] = cur;
                            g_effect->SetTexture("env_texture", cur);
                        }
                    }
                    if (part->shader->flags20 & 0x100) {
                        t = sc->rt;
                        if (t) {
                            IDirect3DBaseTexture9* cur = tex_current(t, 0.0f);
                            if (g_render.tex_cache[4] != cur) {
                                g_render.tex_cache[4] = cur;
                                g_effect->SetTexture("env_texture", cur);
                            }
                        } else {
                            tex_ok = false;
                        }
                    }
                }
                mat = part->shader;
                int nsel = 0;
                if (sc->f160 && !(mat->flags20 & 0x80)) {
                    float lk[4];
                    int li[4];
                    unsigned i = 0;
                    int n = 0;
                    for (; i < scene_lights(sc).size() && n < 10; i++) {
                        if (scene_lights(sc)[i]->type == 0) {
                            mesh* im = it->m;
                            float dx = scene_lights(sc)[i]->position.x - im->m194.o.x;
                            float dy = scene_lights(sc)[i]->position.y - im->m194.o.y;
                            float dz = scene_lights(sc)[i]->position.z - im->m194.o.z;
                            float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                            if (dist - im->f44 < scene_lights(sc)[i]->radius) {
                                float key = (scene_lights(sc)[i]->diffuse.z + scene_lights(sc)[i]->diffuse.y + scene_lights(sc)[i]->diffuse.x) / dist;
                                bool done = false;
                                int p = nsel;
                                int j = nsel - 1;
                                do {
                                    if (j < 0)
                                        break;
                                    if (key < lk[j])
                                        done = true;
                                    else
                                        p = j;
                                    j--;
                                } while (!done);
                                if (p < 4) {
                                    for (int j = 3; j > p; j--) {
                                        lk[j] = lk[j - 1];
                                        li[j] = li[j - 1];
                                    }
                                    lk[p] = key;
                                    li[p] = n;
                                    if (nsel + 1 < 4)
                                        nsel++;
                                    else
                                        nsel = 4;
                                }
                            }
                            n++;
                        }
                    }
                    bool changed = false;
                    for (int j = 0; j < nsel; j++) {
                        if (g_render.light_idx[j] != li[j]) {
                            g_render.light_idx[j] = li[j];
                            changed = true;
                        }
                    }
                    if (changed)
                        g_effect->SetIntArray("iLightIndices", g_render.light_idx, 4);
                }
                if (g_render.light_count != nsel) {
                    g_render.light_count = nsel;
                    g_effect->SetInt("iLightPointCount", nsel);
                }
                {
                    const unsigned& col = it->m->color;
                    if (g_render.tex_factor != col) {
                        g_render.tex_factor = col;
                        vector4 c = rgl::unpack_color(&g_render.tex_factor);
                        g_effect->SetVector("vMaterialColor", (D3DXVECTOR4*)&c);
                    }
                }
                {
                    const int& col = it->m->f40;
                    if (g_render.tex_factor2 != (unsigned)col) {
                        g_render.tex_factor2 = col;
                        vector4 c = rgl::unpack_color(&g_render.tex_factor2);
                        g_effect->SetVector("vMaterialColor2", (D3DXVECTOR4*)&c);
                    }
                }
                ((mi_ops*)it->m)->setup_material();
            } else {
                skin_calls* part = (skin_calls*)it->m->res;
                if (sc->f164) {
                    if (part->shader->flags20 & 0x80000000)
                        mat = sh3;
                    else
                        mat = sh2;
                } else {
                    if (part->shader->flags20 & 0x80000000)
                        mat = sh1;
                    else
                        mat = sh0;
                }
                texture* t = part->tex[0];
                if (t) {
                    IDirect3DBaseTexture9* cur = tex_current(t, 0.0f);
                    if (g_render.tex_cache[0] != cur) {
                        g_render.tex_cache[0] = cur;
                        g_effect->SetTexture("diffuse_texture", cur);
                    }
                }
            }
            if (!tex_ok)
                goto next_item;

            // ---- material / technique ----
            if (cur_mat != mat) {
                cur_mat = mat;
                int v = 0;
                if ((mat->flags20 & 0x1000) && g_options.i7c != 2) {
                    v = 3;
                } else if (sun_shadow) {
                    if (scene_sky(sc) && scene_sky(sc)->sun_shadows && g_options.i2c) {
                        v = (g_render.use_hw_shadow != 0) + 1;
                    }
                }
                D3DXHANDLE tech = mat_techs(mat)->tech[v];
                if (cur_tech != tech) {
                    g_effect->EndPass();
                    g_effect->End();
                    cur_tech = tech;
                    g_effect->SetTechnique(tech);
                    UINT passes;
                    g_effect->Begin(&passes, 7);
                    g_effect->BeginPass(0);
                }
            }

            // ---- per-mesh constants ----
            {
                const vector4& uv = it->m->m1d4.u;
                if (g_render.tex_off[0] != uv.x || g_render.tex_off[1] != uv.y) {
                    g_render.tex_off[0] = uv.x;
                    g_render.tex_off[1] = uv.y;
                    g_effect->SetVector("texture_offset", (D3DXVECTOR4*)g_render.tex_off);
                }
                int cull = g_render.cull_setting;
                if (it->m->f4c) {
                    if (cull == 3)
                        cull = 2;
                    else if (cull == 2)
                        cull = 3;
                }
                if (cull != g_render.cull_cache) {
                    g_render.cull_cache = cull;
                    g_render.dev->SetRenderState(D3DRS_CULLMODE, cull);
                }
            }
            {
                D3DXMATRIX world;
                if (it->m->f34 == 0) {
                    skin* ip = it->m->res;
                    if (ip->flags & 0x20000) {
                        *(rot3*)&world = *(rot3*)&it->m->m194;
                        ((frame4*)&world)->o = sc->fr.m.o;
                        world._43 *= 0.75f;
                    } else {
                        world = *(D3DXMATRIX*)&it->m->m194;
                    }
                } else if (it->m->f34 == 1) {
                    frame4 w;
                    w.o = it->m->m194.o;
                    w.rot.u = it->m->m194.u;
                    w.rot.s = sc->fr.m.s;
                    w.rot.f = cross_ba(w.rot.u, w.rot.s);
                    ((vec4n&)w.rot.u).normalize_inl();
                    w.rot.s = cross_i(w.rot.f, w.rot.u);
                    world = *(D3DXMATRIX*)&w;
                } else {
                    frame4 w;
                    w.o = it->m->m194.o;
                    w.rot.s = sc->fr.m.s;
                    w.rot.f = sc->fr.m.f;
                    w.rot.u = sc->fr.m.u;
                    world = *(D3DXMATRIX*)&w;
                }
                world._14 = 0.0f; world._24 = 0.0f; world._34 = 0.0f; world._44 = 1.0f;
                D3DXMATRIX world2 = world;
                if ((it->m->res->shader->flags20 & 0x100) && sc->refl) {
                    D3DXMATRIX wproj = sc->refl->fr.projection();
                    D3DXMATRIX wview = sc->refl->fr.view_matrix();
                    wproj = world2 * wview * wproj;
                    g_effect->SetMatrixTranspose("matWaterWorldViewProj", &wproj);
                }
                g_effect->SetMatrixTranspose("matWorld", &world2);
                D3DXMATRIX wv = world2 * view;
                g_effect->SetMatrixTranspose("matWorldView", &wv);
                wv = wv * proj;
                g_effect->SetMatrixTranspose("matWorldViewProj", &wv);
            }
            if (cur_mat->flags20 & 0x80000000) {
                if (it->m->p220) {
                    if (g_render.bone_origin_cache != (void*)it->m->f224) {
                        g_render.bone_origin_cache = (void*)it->m->f224;
                        g_effect->SetVectorArray("matBoneOriginArray", ((vec_arr*)it->m->f224)->data, ((vec_arr*)it->m->f224)->count);
                    }
                    g_effect->SetMatrixTransposeArray("matWorldArray", (D3DXMATRIX*)it->m->p220, it->m->f228);
                } else {
                    vec_arr* d = (vec_arr*)&bone_origins;
                    if (g_render.bone_origin_cache != d) {
                        g_render.bone_origin_cache = d;
                        g_effect->SetVectorArray("matBoneOriginArray", d->data, d->count);
                    }
                    g_effect->SetMatrixTransposeArray("matWorldArray", (D3DXMATRIX*)world_mats, 0x20);
                }
            }
            {
                unsigned tg = mat_texgen(it->m->res->shader) & 0xff;
                if (tg == 0x30 || tg == 0x20)
                    g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)16, (D3DMATRIX*)&a);
            }
            g_effect->CommitChanges();
            {
                std::vector<face20>& faces = src->local_a0 ? src->v90 : ((mesh_calls*)src->parent)->get90_();
                int nface = (int)faces.size();
                mesh* im = it->m;
                std::vector<vertex60>& verts = im->local_a1 ? im->v50 : ((mesh_calls*)im->parent)->get50_();
                int nvert = (int)verts.size();
                g_render.dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, im->slots[0].f8, 0, nvert, src->slots[1].f8, nface * 3 / 3);
            }
          next_item:
            ;
        }
    }
    g_effect->EndPass();
    g_effect->End();
}

// @ 0x4704b0
void NavData::compute_bounds()
{
    if (verts.size()) {
        e44 = verts[0].p;
        f54 = verts[0].p;
    }
    for (unsigned i = 0; i < verts.size(); i++) {
        // pointers taken inside the loop: cl then re-reads verts._Myfirst after every store (the loop reads the raw
        // element pointer: the real operator[] here costs 28 aligned bytes; verts[0] above is the real one)
        rgl::vector4* mn = &e44;
        rgl::vector4* mx = &f54;
        if (mn->x > verts._Myfirst[i].p.x) mn->x = verts._Myfirst[i].p.x;
        if (mx->x < verts._Myfirst[i].p.x) mx->x = verts._Myfirst[i].p.x;
        if (mn->y > verts._Myfirst[i].p.y) mn->y = verts._Myfirst[i].p.y;
        if (mx->y < verts._Myfirst[i].p.y) mx->y = verts._Myfirst[i].p.y;
        if (mn->z > verts._Myfirst[i].p.z) mn->z = verts._Myfirst[i].p.z;
        if (mx->z < verts._Myfirst[i].p.z) mx->z = verts._Myfirst[i].p.z;
    }
}

// @ 0x4705e0
void PolyMesh::compute_face_normals()
{
    vector4 t;                              // the normal before it is stored (its w lane is never written)
    vector4 b;
    // a dead mention of b.x: its float view gets a symbol key here, before ay / az, and the key order is what orders
    // az * b.x and ay * b.x (the a component stays in st and is consumed, b.x is the memory operand). Some reference
    // to b.x sat before the loop in the original.
    (void)b.x;
    for (unsigned i = 0; i < faces.size(); i++) {
        MPoly& f = faces.begin()[i];
        // the corner indices are named locals: the vid[2] load then precedes the verts load in the schedule
        int i2 = f.vid[2];
        int i0 = f.vid[0];
        float ax = verts.begin()[i2].pos.x - verts.begin()[i0].pos.x;
        float ay = verts.begin()[i2].pos.y - verts.begin()[i0].pos.y;
        float az = verts.begin()[i2].pos.z - verts.begin()[i0].pos.z;
        // the second edge through corners re-derived from the face list (the original reloads vid[0] here)
        const vector4& p0 = verts.begin()[faces.begin()[i].vid[0]].pos;
        const vector4& p1 = verts.begin()[faces.begin()[i].vid[1]].pos;
        b.x = p1.x - p0.x;
        b.y = p1.y - p0.y;
        b.z = p1.z - p0.z;
        t.x = b.y * az - b.z * ay;
        t.y = b.z * ax - az * b.x;
        t.z = ay * b.x - b.y * ax;
        f.normal = t;
        MPoly& g = faces.begin()[i];
        float len = g.normal.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            g.normal.x = inv * g.normal.x;
            g.normal.y = inv * g.normal.y;
            g.normal.z = inv * g.normal.z;
        } else {
            g.normal.x = 0.0f; g.normal.y = 1.0f; g.normal.z = 0.0f;
        }
    }
}

// @ 0x470750
void PolyMesh::compute_edge_creases()
{
    vector4 s, n;                          // projected part, perpendicular part
    for (unsigned i = 0; i < edges.size(); i++) {
        MEdge* e = &edges.begin()[i];
        MVert& a = verts.begin()[e->v0];
        MVert& b = verts.begin()[e->v1];
        vector4 d = b.pos - a.pos;         // direction of the edge (a loop-scoped temporary: the x87 order of its normalisation)
        float mz = e->mid.z;
        float len = d.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            d.x = inv * d.x; d.y = inv * d.y; d.z = inv * d.z;
        } else {
            d.x = 0.0f; d.y = 1.0f; d.z = 0.0f;
        }
        for (int j = 0; j < e->nfaces; j++) {
            MPoly& f = faces.begin()[e->face[j]];
            const vector4& fn = f.normal;
            if (f.centroid.z > mz) {
                float k = (d.x * fn.x + (d.y * fn.y + d.z * fn.z)) * 0.2f;
                s.x = k * d.x; s.y = k * d.y; s.z = k * d.z;
                n.x = fn.x - s.x; n.y = fn.y - s.y; n.z = fn.z - s.z;
                float l2 = (float)sqrt((n.y * n.y + n.x * n.x) + n.z * n.z);
                e->crease[j] = (l2 > 1e-10f) ? (1.0f / l2) * n.z : 0.0f;
            } else {
                e->crease[j] = 1.0f;
            }
        }
    }
}

// @ 0x470920
void PolyMesh::build_adjacency()
{
    for (unsigned i = 0; i < edges.size(); i++)
        edges.begin()[i].nfaces = 0;
    for (unsigned i = 0; i < faces.size(); i++) {
        for (int j = 0; j < faces.begin()[i].n; j++) {
            edges.begin()[faces.begin()[i].eid[j]].face[edges.begin()[faces.begin()[i].eid[j]].nfaces] = i;
            edges.begin()[faces.begin()[i].eid[j]].nfaces++;
        }
    }
}
