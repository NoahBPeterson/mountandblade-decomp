//-----------------------------------------------------------------------------
// File: terrain_b.cpp  (0x46e590..0x478830)
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
#include "./frustum.h"
#include "./render_batch.h"
#include "./poly_mesh.h"
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
#include "./missile.h"
#include "./face_gen.h"
#include "./entity_rec.h"
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

using rgl::vector4;
using rgl::matrix;

// @ 0x46e590
void render_batch::render_scene(scene* sc)
{
    int f164 = sc->f164;
    vector4 light_diffuse[10];
    for (int i = 0; i < 10; i++) {
        light_diffuse[i].x = 0.0f;
        light_diffuse[i].y = 0.0f;
        light_diffuse[i].z = 0.0f;
        light_diffuse[i].w = 1.0f;
    }
    bool sun_shadow = false;
    vec3 light_pos[10];
    if (!sc->b162 && f164 == 0) {
        sky_t* sky = sc->sky;
        if (sky) {
            bool lt = sc->cam.m.o.z < sky->height;
            if (sc->b169)
                lt = !lt;
            vector4 fog_col;
            if (lt) {
                unsigned c = sky->fog_color_a;
                unpack_color(&fog_col, &c);
                fog_col.w = 1.0f;
                g_effect->SetFloat("fFogStart", sky->fog_start_a);
                g_effect->SetFloat("fFogEnd", sc->sky->fog_end_a);
                g_effect->SetFloat("fFogDensity", sc->sky->fog_dens_a);
            } else {
                unsigned c = sky->fog_color_b;
                unpack_color(&fog_col, &c);
                fog_col.w = 1.0f;
                g_effect->SetFloat("fFogStart", sky->fog_start_b);
                g_effect->SetFloat("fFogEnd", sc->sky->fog_end_b);
                g_effect->SetFloat("fFogDensity", sc->sky->fog_dens_b);
            }
            g_effect->SetVector("vFogColor", (D3DXVECTOR4*)&fog_col);

            vec3 sky_dir;
            sky_dir.x = 0.0f; sky_dir.y = 0.0f; sky_dir.z = -1.0f;
            vector4 sky_col;
            sky_col.x = 0.0f; sky_col.y = 0.0f; sky_col.z = 0.0f; sky_col.w = 1.0f;
            float pc[4];
            pc[0] = 0.01f; pc[1] = 0.01f; pc[2] = 0.01f;
            bool found = false;
            int npoint = 0;
            for (unsigned i = 1; i < sc->lights.size(); i++) {
                light* l = sc->lights[i];
                if (l->type == 2) {
                    if (!found) {
                        sky_col = l->diffuse;
                        sky_col.w = 1.0f;
                        sky_dir = sc->lights[i]->sun_dir;
                        found = true;
                    }
                } else {
                    npoint++;
                    pc[0] += l->diffuse.x;
                    pc[1] += l->diffuse.y;
                    pc[2] += l->diffuse.z;
                }
            }
            if (npoint) {
                pc[3] = 1.0f;
                float m = pc[1] > pc[2] ? pc[1] : pc[2];
                if (pc[0] > m)
                    m = pc[0];
                float inv = 1.0f / m;
                pc[0] *= inv;
                pc[1] *= inv;
                pc[2] *= inv;
            }
            g_effect->SetValue("vPointLightColor", pc, 16);
            g_effect->SetValue("vSkyLightDir", &sky_dir, 12);
            g_effect->SetValue("vSkyLightColor", &sky_col, 16);

            int n = 0;
            for (unsigned i = 0; i < sc->lights.size() && n < 10; i++) {
                if (sc->lights[i]->type == 0) {
                    light_diffuse[n] = sc->lights[i]->sample();
                    light_diffuse[n].w = 1.0f;
                    light_pos[n] = sc->lights[i]->position;
                    n++;
                }
            }
            g_effect->SetValue("vLightPosDir", light_pos, 0x78);
            g_effect->SetValue("vLightDiffuse", light_diffuse, 0xa0);

            vector4 sun_col;
            vec3 sun_dir;
            if (sc->lights.size() != 0 && sc->lights[0]->type == 2) {
                sun_col = sc->lights[0]->diffuse;
                sun_col.w = 0.0f;
                sun_dir = sc->lights[0]->sun_dir;
                if (sc->lights[0]->diffuse.length() > 0.1f && !(sc->lights[0]->flags & 2))
                    sun_shadow = true;
                else
                    sun_shadow = false;
            } else {
                sun_col = make4(0.0f, 0.0f, 0.0f, 0.0f);
                sun_dir = vec3(0.0f, 0.0f, -1.0f);
            }
            g_effect->SetValue("vSunDir", &sun_dir, 12);
            g_effect->SetValue("vSunColor", &sun_col, 16);

            texture* st = sc->sky->shadow;
            if (st) {
                IDirect3DBaseTexture9* t = tex_current(st, 0.0f);
                g_effect->SetTexture("shadowmap_texture", t);
                D3DXMATRIX proj = sc->sky->shadow_cam.projection();
                D3DXMATRIX view = sc->sky->shadow_cam.view_matrix();
                D3DXMATRIX svp = view * proj * D3DXMATRIX(0.5f, 0.0f, 0.0f, 0.0f,
                                                         0.0f, -0.5f, 0.0f, 0.0f,
                                                         0.0f, 0.0f, 1.0f, 0.0f,
                                                         0.5f / (float)sc->sky->shadow->width + 0.5f,
                                                         0.5f / (float)sc->sky->shadow->height + 0.5f, 0.0f, 1.0f);
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
        if (f164) {
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
        g_render.set_blend_mode(1);
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
    a.rot = *(rot3*)&sc->cam.m;
    a.rot.s.x *= 0.5f; a.rot.s.y *= 0.5f; a.rot.s.z *= 0.5f;
    a.rot.f.x *= 0.5f; a.rot.f.y *= 0.5f; a.rot.f.z *= 0.5f; a.rot.f.w *= 0.5f;
    a.rot.u.y *= 0.5f; a.rot.u.z *= 0.5f; a.rot.u.w *= 0.5f;
    a.o = vector4(0.5f, 0.5f, 0.5f);
    {
        frame4 b;
        rot3 idt;
        b.rot = *identity3(&idt);
        b.o = vector4(0.0f, 0.0f, 0.0f);
        float ang = 1.5707964f;
        ((m4*)&b)->rotate_fu(&ang);
        a = *(frame4*)&(*(m4*)&b * *(m4*)&a);
    }
    a.rot.s.w = 0.0f; a.rot.f.w = 0.0f; a.rot.u.w = 0.0f; a.o.w = 1.0f;

    sc->ambient.w = 1.0f;
    g_effect->SetVector("vAmbientColor", (D3DXVECTOR4*)&sc->ambient);
    vector4 camp;
    camp = sc->cam.m.o;
    camp.w = 0.0f;
    g_effect->SetVector("vCameraPos", (D3DXVECTOR4*)&camp);

    D3DXMATRIX proj = sc->cam.projection();
    D3DXMATRIX view = sc->cam.view_matrix();
    g_effect->SetMatrixTranspose("matView", &view);
    D3DXMATRIX viewproj = view * proj;
    g_effect->SetMatrixTranspose("matViewProj", &viewproj);

    float tv = (float)(g_ticks[2] % 1000000000) * 1e-5f;
    if (tv != g_render.time_var) {
        g_render.time_var = tv;
        g_effect->SetFloat("time_var", tv);
    }

    if (sc->b161) {
        D3DXPLANE pl(sc->clip_plane.a, sc->clip_plane.b, sc->clip_plane.c, sc->clip_plane.d);
        D3DXMATRIX inv;
        D3DXMatrixInverse(&inv, 0, &view);
        D3DXMatrixTranspose(&inv, &inv);
        D3DXPLANE pl2;
        D3DXPlaneTransform(&pl2, &pl, &inv);
        D3DXMatrixInverse(&inv, 0, &proj);
        D3DXMatrixTranspose(&inv, &inv);
        D3DXPLANE pl3;
        D3DXPlaneTransform(&pl3, &pl2, &inv);
        g_render.dev->SetClipPlane(0, (const float*)&pl3);
        g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
    }

    // ---- per-mesh loop --------------------------------------------------------------------------------------
    AT(int, this, 0x4183d0) = 0;
    AT(int, this, 0x4183c8) = 0;
    AT(int, this, 0x4183cc) = 0;
    std::vector<item_ref>::iterator it = sc->items.begin();
    if (it != sc->items.end()) {
        do {
            mesh* item = it->m;
            bool tex_ok = true;
            int fa = item->slots[0].f4;
            int fb = item->slots[0].f0;
            if (cur_vf_a != fa || cur_vf_b != fb) {
                cur_vf_a = fa;
                vfmt* v = &vf[fa + fb * 2];
                cur_vf_b = fb;
                g_render.dev->SetVertexDeclaration(v->decl);
                g_render.dev->SetStreamSource(0, v->vb, 0, v->stride);
            }
            mesh* src = item->local_a0 ? item : item->parent;
            int ibk = src->slots[1].f4;
            if (cur_ib != ibk) {
                cur_ib = ibk;
                g_render.dev->SetIndices(AT(IDirect3DIndexBuffer9*, this, ibk * 0x4c + 0x350));
            }
            material* mat;
            if (sc->b162 || sc->f164) {
                skin_calls* part = (skin_calls*)it->m->res;
                material* pm = part->shader;
                if (sc->f164) {
                    if ((int)pm->flags20 < 0)
                        mat = AT(material*, this, 0x418824);
                    else
                        mat = AT(material*, this, 0x418820);
                } else {
                    if ((int)pm->flags20 < 0)
                        mat = AT(material*, this, 0x41881c);
                    else
                        mat = AT(material*, this, 0x418818);
                }
                texture* t = part->tex[0];
                if (t) {
                    IDirect3DBaseTexture9* cur = tex_current(t, 0.0f);
                    if (g_render.tex_cache[0] != cur) {
                        g_render.tex_cache[0] = cur;
                        g_effect->SetTexture("diffuse_texture", cur);
                    }
                }
            } else {
                skin_calls* part = (skin_calls*)item->res;
                if (AT(skin_calls*, this, 0x4183d0) != part) {
                    AT(skin_calls*, this, 0x4183d0) = part;
                    if (sc->sky && sc->sky->fog_meshes && !(part->flags & 1)) {
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
                        t = sc->env;
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
                if (sc->b160 && !(mat->flags20 & 0x80)) {
                    float lk[4];
                    int li[4];
                    int n = 0;
                    for (unsigned i = 0; i < sc->lights.size() && n < 10; i++) {
                        if (sc->lights[i]->type == 0) {
                            mesh* im = it->m;
                            float dx = sc->lights[i]->position.x - im->m194.o.x;
                            float dy = sc->lights[i]->position.y - im->m194.o.y;
                            float dz = sc->lights[i]->position.z - im->m194.o.z;
                            float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                            if (dist - im->f44 < sc->lights[i]->radius) {
                                float key = (sc->lights[i]->diffuse.z + sc->lights[i]->diffuse.y + sc->lights[i]->diffuse.x) / dist;
                                int p = nsel;
                                int j = nsel - 1;
                                bool done = false;
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
                mesh* im = it->m;
                if (g_render.color1 != im->color) {
                    g_render.color1 = im->color;
                    vector4 c;
                    unpack_color(&c, &g_render.color1);
                    g_effect->SetVector("vMaterialColor", (D3DXVECTOR4*)&c);
                }
                if (g_render.color2 != (unsigned)im->f40) {
                    g_render.color2 = im->f40;
                    vector4 c;
                    unpack_color(&c, &g_render.color2);
                    g_effect->SetVector("vMaterialColor2", (D3DXVECTOR4*)&c);
                }
                ((mi_ops*)im)->setup_material();
                if (!tex_ok)
                    goto next_item;
            }

            // ---- material / technique ----
            if (AT(material*, this, 0x4183c8) != mat) {
                AT(material*, this, 0x4183c8) = mat;
                int v = 0;
                if ((mat->flags20 & 0x1000) && g_mesh_quality != 2) {
                    v = 3;
                } else if (sun_shadow) {
                    if (sc->sky && sc->sky->sun_shadows && g_x5aa034) {
                        v = (g_render.b614 != 0) + 1;
                    }
                }
                D3DXHANDLE tech = mat_techs(mat)->tech[v];
                if (AT(D3DXHANDLE, this, 0x4183cc) != tech) {
                    g_effect->EndPass();
                    g_effect->End();
                    AT(D3DXHANDLE, this, 0x4183cc) = tech;
                    g_effect->SetTechnique(tech);
                    UINT passes;
                    g_effect->Begin(&passes, 7);
                    g_effect->BeginPass(0);
                }
            }

            // ---- per-mesh constants ----
            {
                mesh* im = it->m;
                if (im->m1d4.u.x != g_render.tex_off[0] || im->m1d4.u.y != g_render.tex_off[1]) {
                    g_render.tex_off[0] = im->m1d4.u.x;
                    g_render.tex_off[1] = im->m1d4.u.y;
                    g_effect->SetVector("texture_offset", (D3DXVECTOR4*)g_render.tex_off);
                }
                im = it->m;
                int cull = g_render.cull_setting;
                if (im->f4c) {
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
                mesh* im = it->m;
                D3DXMATRIX world;
                if (im->f34 == 0) {
                    skin* ip = im->res;
                    if (ip->flags & 0x20000) {
                        *(rot3*)&world = *(rot3*)&im->m194;
                        ((frame4*)&world)->o = sc->cam.m.o;
                        world._43 *= 0.75f;
                    } else {
                        world = *(D3DXMATRIX*)&im->m194;
                    }
                } else if (im->f34 == 1) {
                    frame4 w;
                    w.o = im->m194.o;
                    w.rot.u = im->m194.u;
                    w.rot.s = sc->cam.m.s;
                    w.rot.f = cross_i(w.rot.u, w.rot.s);
                    float len = (float)sqrt(w.rot.u.z * w.rot.u.z + w.rot.u.y * w.rot.u.y + w.rot.u.x * w.rot.u.x);
                    if (len > 1e-10f) {
                        float inv = 1.0f / len;
                        w.rot.u.x *= inv; w.rot.u.y *= inv; w.rot.u.z *= inv;
                    } else {
                        w.rot.u.x = 0.0f; w.rot.u.y = 1.0f; w.rot.u.z = 0.0f;
                    }
                    w.rot.s = cross_i(w.rot.f, w.rot.u);
                    world = *(D3DXMATRIX*)&w;
                } else {
                    frame4 w;
                    w.o = im->m194.o;
                    w.rot.s = sc->cam.m.s;
                    w.rot.f = sc->cam.m.f;
                    w.rot.u = sc->cam.m.u;
                    world = *(D3DXMATRIX*)&w;
                }
                world._14 = 0.0f; world._24 = 0.0f; world._34 = 0.0f; world._44 = 1.0f;
                D3DXMATRIX world2 = world;
                if ((it->m->res->shader->flags20 & 0x100) && sc->water) {
                    D3DXMATRIX wproj = sc->water->cam.projection();
                    D3DXMATRIX wview = sc->water->cam.view_matrix();
                    wproj = world2 * wview * wproj;
                    g_effect->SetMatrixTranspose("matWaterWorldViewProj", &wproj);
                }
                g_effect->SetMatrixTranspose("matWorld", &world2);
                D3DXMATRIX wv = world2 * view;
                g_effect->SetMatrixTranspose("matWorldView", &wv);
                wv = wv * proj;
                g_effect->SetMatrixTranspose("matWorldViewProj", &wv);
            }
            if (AT(material*, this, 0x4183c8)->flags20 & 0x80000000) {
                mesh* im = it->m;
                if (im->f220) {
                    void* bo = (void*)im->f224;
                    if (g_render.bone_origin_cache != bo) {
                        g_render.bone_origin_cache = bo;
                        g_effect->SetVectorArray("matBoneOriginArray", ((vec_arr*)bo)->data, ((vec_arr*)bo)->count);
                    }
                    g_effect->SetMatrixTransposeArray("matWorldArray", (D3DXMATRIX*)im->f220, im->f228);
                } else {
                    vec_arr* d = (vec_arr*)((char*)this + 0x4183d8);
                    if (g_render.bone_origin_cache != d) {
                        g_render.bone_origin_cache = d;
                        g_effect->SetVectorArray("matBoneOriginArray", d->data, d->count);
                    }
                    g_effect->SetMatrixTransposeArray("matWorldArray", AT(D3DXMATRIX*, this, 0x4183d4), 0x20);
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
                int nface3 = (int)faces.size() * 3;
                mesh* im = it->m;
                std::vector<vertex60>& verts = im->local_a1 ? im->v50 : ((mesh_calls*)im->parent)->get50_();
                int nvert = (int)verts.size();
                g_render.dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, im->slots[0].f8, 0, nvert, src->slots[1].f8, nface3 / 3);
            }
          next_item:
            ++it;
        } while (it != sc->items.end());
    }
    g_effect->EndPass();
    g_effect->End();
}

// @ 0x4704b0
void PointSet::compute_bounds()
{
    if (pts.size()) {
        bmin = pts[0].pos;
        bmax = pts[0].pos;
    }
    for (unsigned i = 0; i < pts.size(); i++) {
        // pointers taken inside the loop: cl then re-reads pts.begin after every store
        vector4* mn = &bmin;
        vector4* mx = &bmax;
        if (mn->x > vat(pts, i).pos.x) mn->x = vat(pts, i).pos.x;
        if (mx->x < vat(pts, i).pos.x) mx->x = vat(pts, i).pos.x;
        if (mn->y > vat(pts, i).pos.y) mn->y = vat(pts, i).pos.y;
        if (mx->y < vat(pts, i).pos.y) mx->y = vat(pts, i).pos.y;
        if (mn->z > vat(pts, i).pos.z) mn->z = vat(pts, i).pos.z;
        if (mx->z < vat(pts, i).pos.z) mx->z = vat(pts, i).pos.z;
    }
}

// @ 0x4705e0
void Mesh::compute_face_normals()
{
    vector4 t;                              // the normal before it is stored (its w lane is never written)
    vector4 b;
    for (unsigned i = 0; i < c.faces.size(); i++) {
        MPoly& f = vat(c.faces, i);
        float ax = vat(c.verts, f.vid[2]).pos.x - vat(c.verts, f.vid[0]).pos.x;
        float ay = vat(c.verts, f.vid[2]).pos.y - vat(c.verts, f.vid[0]).pos.y;
        float az = vat(c.verts, f.vid[2]).pos.z - vat(c.verts, f.vid[0]).pos.z;
        b.x = vat(c.verts, f.vid[1]).pos.x - vat(c.verts, f.vid[0]).pos.x;
        b.y = vat(c.verts, f.vid[1]).pos.y - vat(c.verts, f.vid[0]).pos.y;
        b.z = vat(c.verts, f.vid[1]).pos.z - vat(c.verts, f.vid[0]).pos.z;
        t.x = b.y * az - b.z * ay;
        t.y = b.z * ax - az * b.x;
        t.z = ay * b.x - b.y * ax;
        f.normal = t;
        MPoly& g = vat(c.faces, i);
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
void Mesh::compute_edge_creases()
{
    vector4 d, s, n;                       // direction of the edge, projected part, perpendicular part
    for (unsigned i = 0; i < c.edges.size(); i++) {
        MEdge* e = &vat(c.edges, i);
        MVert& a = vat(c.verts, e->v0);
        MVert& b = vat(c.verts, e->v1);
        d.x = b.pos.x - a.pos.x; d.y = b.pos.y - a.pos.y; d.z = b.pos.z - a.pos.z;
        float mz = e->mid.z;
        float len = d.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            d.x = d.x * inv; d.y = d.y * inv; d.z = d.z * inv;
        } else {
            d.x = 0.0f; d.y = 1.0f; d.z = 0.0f;
        }
        for (int j = 0; j < e->nfaces; j++) {
            MPoly& f = vat(c.faces, e->face[j]);
            if (f.centroid.z > mz) {
                float k = (d.x * f.normal.x + (d.y * f.normal.y + d.z * f.normal.z)) * 0.2f;
                s.x = d.x * k; s.y = d.y * k; s.z = d.z * k;
                n.x = f.normal.x - s.x; n.y = f.normal.y - s.y; n.z = f.normal.z - s.z;
                float l2 = (float)sqrt(n.z * n.z + (n.x * n.x + n.y * n.y));
                e->crease[j] = (l2 > 1e-10f) ? (1.0f / l2) * n.z : 0.0f;
            } else {
                e->crease[j] = 1.0f;
            }
        }
    }
}

// @ 0x470920
void Mesh::build_adjacency()
{
    for (unsigned i = 0; i < c.edges.size(); i++)
        vat(c.edges, i).nfaces = 0;
    for (unsigned i = 0; i < c.faces.size(); i++) {
        MPoly* f = &vat(c.faces, i);
        for (int j = 0; j < f->n; j++) {
            vat(c.edges, f->eid[j]).face[vat(c.edges, f->eid[j]).nfaces] = i;
            vat(c.edges, vat(c.faces, i).eid[j]).nfaces++;
            f = &vat(c.faces, i);
        }
    }
}

// @ 0x470a00
void Mesh::build_geometry()
{
    ((MeshExt*)this)->build_adjacency();
    for (unsigned i = 0; i < c.faces.size(); i++) {
        MPoly* f = &vat(c.faces, i);
        f->centroid = vector4(0.0f, 0.0f, 0.0f);
        for (int j = 0; j < vat(c.faces, i).n; j++) {
            MPoly& g = vat(c.faces, i);
            MVert& v = vat(c.verts, g.vid[j]);
            g.centroid.x = g.centroid.x + v.pos.x;
            g.centroid.y = v.pos.y + g.centroid.y;
            g.centroid.z = v.pos.z + g.centroid.z;
            vat(c.faces, i).nbr[j] = -1;
            MPoly& h = vat(c.faces, i);
            if (vat(c.edges, h.eid[j]).nfaces >= 2) {
                if (vat(c.edges, h.eid[j]).face[0] == (int)i)
                    h.nbr[j] = vat(c.edges, h.eid[j]).face[1];
                else if (vat(c.edges, h.eid[j]).face[1] == (int)i)
                    h.nbr[j] = vat(c.edges, h.eid[j]).face[0];
            }
        }
        MPoly& h = vat(c.faces, i);
        float s = 1.0f / (float)h.n;
        h.centroid.x = s * h.centroid.x;
        h.centroid.y = s * h.centroid.y;
        h.centroid.z = s * h.centroid.z;
    }
    for (unsigned i = 0; i < c.edges.size(); i++) {
        MEdge& e = vat(c.edges, i);
        MVert& a = vat(c.verts, e.v0);
        MVert& b = vat(c.verts, e.v1);
        e.mid = vector4((b.pos.x + a.pos.x) * 0.5f, (b.pos.y + a.pos.y) * 0.5f, (b.pos.z + a.pos.z) * 0.5f);
    }
    ((MeshExt*)this)->compute_face_normals();
}

// @ 0x470c00
void CellGrid::build_light_frame(matrix* m, vector4* amb)
{
    vector4 d;
    {
        vector4 p(0.0f, 0.0f, 0.0f);       // dies before the frame temporaries: cl shares its stack slot
        sample_lights(&p, &d, amb);
    }
    if (d.z > -0.1f) d.z = -0.1f;
    vector4* u = &m->u;
    vector4* f = &m->f;
    vector4* s = &m->s;
    *u = vector4(-d.x, -d.y, -d.z);
    *f = vector4(0.0f, 0.0f, 1.0f);
    *s = vector4(f->y * u->z - f->z * u->y, f->z * u->x - f->x * u->z, f->x * u->y - f->y * u->x);
    s->normalize();
    *f = vector4(u->y * s->z - u->z * s->y, u->z * s->x - u->x * s->z, u->x * s->y - u->y * s->x);
    f->normalize();
}

// @ 0x470d80
void MeterBar::set_slot(int idx, int value)
{
    Meter* m = slots[idx];
    if (m) {
        float v = (float)value;
        m->set_value(&v);
    }
}

// @ 0x470db0
void TroopMgr::apply_group(ArrSet* dst, int idx, rgl::mesh* obj)
{
    rgl::mesh* base = *ref->base;
    int nbase = base->v70.size();
    int n = obj->get70().size();
    dst->cnt[idx] = n;
    dst->arr[idx] = new int[n];
    for (int i = 0; i < dst->cnt[idx]; i++) {
        float best = 100000.0f;
        int bi = 0;
        const rgl::point20* bp = &*base->v70.begin();
        for (int j = 0; j < nbase; j++) {
            vector4 q = (&*obj->get70().begin())[i].a;
            if (q.x < 0.0f) q.x = -q.x;
            float dx = q.x - bp->a.x;
            float dy = q.y - bp->a.y;
            float dz = q.z - bp->a.z;
            float d = dy * dy + (dx * dx + dz * dz);
            if (d < best) { best = d; bi = j; }
            bp++;
        }
        dst->arr[idx][i] = bi;
    }
}

// @ 0x470f40
bool __stdcall ground_height(CellGrid* lv, const float* p, float* out)
{
    face* hits[0x2000];
    int count;
    int use3d;
    int cellxy[2];
    vector4 q;
    q.x = p[0];
    q.y = p[1];
    q.z = 0.0f;
    use3d = 0;
    lv->locate(&q.x, cellxy);
    count = 0;
    Cell* col = lv->cells[cellxy[0]];
    col[cellxy[1]].collect_point(&q.x, &use3d, 0x104, hits, &count);
    int n = count;
    for (int i = 0; i < n; i++) {
        if (hits[i]->m4 >= 0 && hits[i]->contains_xy(p)) {
            face* f = hits[i];
            *out = f->w[0].z - ((p[0] - f->w[0].x) * f->normal.x + (p[1] - f->w[0].y) * f->normal.y) / f->normal.z;
            return true;
        }
    }
    return false;
}

// @ 0x471040
void GameData::blend(float* out, float x, float y)
{
    vector4 wn = wind_noise();
    float s1 = 0.0f, s0 = 0.0f, s2 = 0.0f, s3 = 0.0f, sum = 0.0f;
    float* rec = &val[0][1];
    float* xy = &pos[0][0];
    int n = 4;
    do {
        n--;
        float w = 1.0f / (float)sqrt((xy[0] - x) * (xy[0] - x) + (xy[1] - y) * (xy[1] - y));
        s1 = w * rec[0] + s1;
        s0 = w * rec[-1] + s0;
        s2 = w * rec[1] + s2;
        s3 = s3 + w * rec[2];
        sum = w + sum;
        rec += 4;
        xy += 2;
    } while (n != 0);
    float t = 1.0f / sum;                    // inverse weight sum, then reused for the ground factor (address taken)
    float r0 = wn.x + t * s0;
    float r1 = wn.y + t * s1;
    float r2 = wn.z + s2 * t;
    float r3 = wn.w + t * s3;
    if (g_level == 0)
        t = 1.0f;
    else
        ground_height(g_level, &x, &t);
    out[0] = r0 - t * 10.0f;
    out[1] = r1;
    out[2] = r2 * t + r2;
    out[3] = r3 * t + r3;
}

// @ 0x4711e0
void ShadowMap::attach_camera(CellGrid* c)
{
    // marker "WL" 12 here in the original
    cam = c;
    cam->view.vmin = vector4(10000.0f, 10000.0f, 10000.0f);
    cam->view.vmax = vector4(-10000.0f, -10000.0f, -10000.0f);
    cam->view.size = vector4(1.0f, 1.0f, 1.0f);
    cam->view.center = vector4(0.0f, 0.0f, 0.0f);
    cam->view.wcenter = vector4(0.0f, 0.0f, 0.0f);
    cam->build_light_frame(&cam->view.world, &cam->shadow_amb);
    vector4 z(0.0f, 0.0f, 0.0f);
    cam->view.world.o = z;
    if (g_shadow_quality != g_shadow_quality_req) {
        release_all();
        g_shadow_quality = g_shadow_quality_req;
    }
    // marker "WL" 13 here in the original
}

// @ 0x4713d0
void Terrain::generate_heights()
{
    float r1 = (float)(rand() % 15817) * 6.322312e-05f;
    float scale = (r1 * 0.5f + 1.0f) * (wy + wx) * 0.5f;
    float r2 = (float)(rand() % 15817) * 6.322312e-05f;
    noise_seed = (r2 * 0.5f + 1.0f) * (wy + wx) * 0.2f;
    float fl = (float)((g_field_mask - 1) & flags) * 0.01f;
    float off[3];
    off[0] = (float)(rand() % 15817) * 0.6322311f;
    off[1] = (float)(rand() % 15817) * 0.6322311f;
    off[2] = (float)(rand() % 15817) * 0.6322311f;
    // relief weight from low-frequency noise
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            float p[3], n[3];
            p[0] = off[0] + cell[x][y].x;
            p[1] = off[1] + cell[x][y].y;
            p[2] = off[2] + cell[x][y].z;
            noise_fractal(noise_seed, 0.6f, 1, 0, p, n);
            float v = (n[2] + n[1] + n[0]) * fl * 9.0f;
            if (v < 0.0f) v = 0.0f;
            else if (v >= 1.0f) v = 1.0f;
            cell[x][y].f18 = ((float)cos(v * 3.1415927f) + 1.0f) * 0.45f + 0.1f;
        }
    }
    // ridges along the four borders
    float amp = (float)(rand() % 15817) * 0.00018966934f + 7.0f;
    int* lim = &h;
    for (int pass = 0; pass < 2; pass++) {
        for (int b = 0; b < 13; b++) {
            if (*lim >= 0) {
                float k = (float)sin((1.0f - (float)b * 0.07692308f) * 3.2f) * amp;
                for (int a = 0; a <= *lim; a++) {
                    int x1 = b, y1 = a, x2 = w - b, y2 = a;
                    if (pass != 0) {
                        x1 = a; y1 = b; x2 = a; y2 = h - b;
                    }
                    TCell* c1 = &cell[x1][y1];
                    float p1[3], n1[3];
                    p1[0] = off[0] + c1->x;
                    p1[1] = off[1] + c1->y;
                    p1[2] = off[2] + c1->z;
                    noise_fractal(40.0f, 0.6f, 4, 0, p1, n1);
                    float v1 = (n1[2] + n1[2] + 0.4f) * k;
                    if (c1->z < v1) { c1->z = v1; c1->f84 = v1; }
                    TCell* c2 = &cell[x2][y2];
                    float p2[3], n2[3];
                    p2[0] = off[0] + c2->x;
                    p2[1] = off[1] + c2->y;
                    p2[2] = off[2] + c2->z;
                    noise_fractal(40.0f, 0.6f, 4, 0, p2, n2);
                    float v2 = (n2[2] + n2[2] + 0.4f) * k;
                    if (c2->z < v2) { c2->z = v2; c2->f84 = v2; }
                    if (b == 0) { c1->f90 = 0; c2->f90 = 0; }
                }
            }
        }
        lim = &w;
    }
    // random brush strokes, coarse to fine
    int lv = (int)(flags >> 14 & (g_field_mask - 1));
    if (lv < 0) lv = 0;
    else if (lv > 99) lv = 99;
    float ratio = scale / ((float)pow(2.0, (double)((100 - lv) * 0.045f)) * 4.0f);
    if (ratio < 2.0f) ratio = 2.0f;
    int steps = (int)(log(ratio) * 1.442695f);
    float strength = 1.0f;
    for (int k = 0; k <= steps; k++) {
        float density;
        if (k == steps)
            density = (ratio - (float)g_bit[steps]) / (float)g_bit[steps];
        else
            density = 1.0f;
        seed_brushes(scale / (float)g_bit[k], density, strength);
        strength = strength - 0.05f;
    }
    // shift so the lowest cell sits at zero
    float lo = 10000.0f;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            if (cell[x][y].z < lo) lo = cell[x][y].z;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++) {
            float v = -lo + cell[x][y].z;
            cell[x][y].z = v;
            cell[x][y].f84 = v;
        }
}

// @ 0x471a30
float Terrain::sun_visibility(const float* p)
{
    vector4 a(0.0f, 0.0f, 0.0f);
    vector4 d;
    {
        vector4 amb;                       // light colour: unused; its slot is shared with the scratch below
        world->sample_lights(&a, &d, &amb);
    }
    vector4 s;
    s.x = d.x * 1000.0f; s.y = d.y * 1000.0f; s.z = d.z * 1000.0f;
    a.x = p[0] - s.x; a.y = p[1] - s.y; a.z = p[2] - s.z;
    s.x = d.x * 0.05f; s.y = d.y * 0.05f; s.z = d.z * 0.05f;
    d.x = p[0] - s.x; d.y = p[1] - s.y; d.z = p[2] - s.z;
    bool vis = g_use_shaders != 0 || world->hf->line_clear(&d, &a);
    if (vis)
        return 1.0f;
    return 0.0f;
}

// @ 0x471b30
void Terrain::light_particle(particle_list* l, int idx, bool trace)
{
    particle* p = &l->items[idx];
    ptype* pt = &g_ptypes[p->type];
    if (g_use_shaders == 0 || g_mesh_quality != 2) {
        for (int k = 0; k < p->nmesh; k++) {
            mesh* m = p->meshes[k];
            if (m->res->flags & 0x40) {
                float light = 1.0f;
                if ((pt->flags & 0x80000i64) != 0 && ((int*)world->hf->cols[p->cx])[p->f0c + p->cy * 4 + 2] >= 0) {
                    vector4 n;
                    n.x = -sun.x; n.y = -sun.y; n.z = -sun.z;
                    light = max2(0.0f, dot3(n, p->frame.u));
                }
                if (g_use_shaders == 0 || g_mesh_quality == 0) {
                    AnimBox* bx = &pt->boxes[p->anim];
                    vector4 sum, c;
                    sum.x = bx->mn[0] + bx->mx[0];
                    sum.y = bx->mn[1] + bx->mx[1];
                    sum.z = bx->mn[2] + bx->mx[2];
                    c.x = sum.x * 0.5f;
                    c.y = sum.y * 0.5f;
                    c.z = sum.z * 0.5f;
                    c.z = (c.z < 0.3f) ? 0.3f : c.z;
                    vector4 pos;
                    pos.x = c.x + p->frame.o.x;
                    pos.y = c.y + p->frame.o.y;
                    pos.z = c.z + p->frame.o.z;
                    if (c.z < 1.5f) {
                        vector4 d, far_, near_;
                        vscale_to(d, sun, 1000.0f);
                        far_.x = pos.x - d.x; far_.y = pos.y - d.y; far_.z = pos.z - d.z;
                        vscale_to(d, sun, 0.05f);
                        near_.x = pos.x - d.x; near_.y = pos.y - d.y; near_.z = pos.z - d.z;
                        bool visible;
                        if (trace)
                            visible = world->trace(&near_, &far_, 50.0f, 0, 0) != 0;
                        else
                            visible = world->hf->line_clear(&near_, &far_) != 0;
                        if (!visible)
                            light *= 0.1f;
                    }
                }
                vector4 wp;
                wp.w = 1.0f;
                for (int j = p->first[k]; j < p->last[k]; j++) {
                    float lf = light;
                    if (g_use_shaders != 0 && g_mesh_quality == 1) {
                        vector4 q = m->v70[m->v50[j].index].a;
                        wp.x = q.x * m->m_cc.s.x + (q.z * m->m_cc.u.x + q.y * m->m_cc.f.x) + m->m_cc.o.x;
                        wp.y = q.x * m->m_cc.s.y + (q.z * m->m_cc.u.y + q.y * m->m_cc.f.y) + m->m_cc.o.y;
                        wp.z = q.x * m->m_cc.s.z + (q.z * m->m_cc.u.z + q.y * m->m_cc.f.z) + m->m_cc.o.z;
                        float occ;
                        g_probe.test((unsigned)&wp, &occ);
                        lf = occ * light;
                    }
                    vertex60& v = m->v50[j];
                    vector4 col;
                    rgl::unpack_color(&col, (const unsigned*)&v.f58);
                    vector4 t1, t2, t3, r;
                    vscale_to(t1, sun_dif, lf);
                    vscale_to(t2, sun_amb, 1.5f);
                    t3.x = t2.x + t1.x; t3.y = t2.y + t1.y; t3.z = t2.z + t1.z;
                    r.x = t3.x * col.x; r.y = t3.y * col.y; r.z = t3.z * col.z;
                    v.color = rgl::pack_color(&r);
                }
                m->slots[0].f28 = 1;
            }
        }
    }
}

// @ 0x472020
void Terrain::light_all(particle_list* l, bool trace)
{
    for (int i = 0; i < l->count; i++)
        light_particle(l, i, trace);
}

// @ 0x472050
void Mission::update_prop_flags()
{
    if (g_flag5a9fb0 != 0) {
        if (special)
            special->flag = (g_game->f1a20c == 0 && g_game->f1a090 != 0) ? 1 : 0;
        for (unsigned i = 0; i < props.size(); i++) {
            Rec1c8& e = props[i];
            if (e.destroyed == 0) {
                if ((g_propkinds[e.kind].flags & 0x40) == 0)
                    continue;
            } else {
                if (e.destroyed != 1 && e.destroyed != 5)
                    continue;
            }
            {
                if (g_game->f1a090 != 0 && g_game->f1a20c == 0)
                    e.ent->flag = 1;
                else
                    e.ent->flag = 0;
            }
        }
    }
}

// @ 0x472160
void Mission::replace_mark(int idx)
{
    recs[idx].f790 = 0.1f;
    recs[idx].stamp = g_ticks[recs[idx].clock];
    recs[idx].cnt++;
    vector4* sv = &recs[idx].m.s;
    recs[idx].floor = 0;
    vector4* ov = &recs[idx].m.o;
    vector4 a;
    a.x = sv->x; a.y = sv->y; a.z = sv->z;
    if (recs[idx].cnt > 15) {
        recs[idx].cnt2++;
        recs[idx].cnt = 0;
        *(rgl::matrix*)sv = recs[idx].m2;
        vector4 t1, t2;
        vscale_to(t1, recs[idx].m.f, 0.7f);
        vscale_to(t2, *sv, -0.5f);
        a = vsub3(t2, t1);
    }
    int n = 0;
    vector4 b;
    do {
        n++;
        b = vadd3(a, *ov);
        float fz;
        face* fs;
        scene->floor_at(&b.x, &fz, &fs);
        b.z = fz;
        if (fs) {
            recs[idx].floor = fs;
            if (fabs(ov->z - fz) < 2.0f) {
                vector4 from = *ov;
                from.z = from.z + 0.7f;
                vector4 to = b;
                to.z = to.z + 0.7f;
                if (scene->trace(&from, &to, 15.0f, 0, 0)) {
                    *ov = b;
                    goto done;
                }
            }
        }
        a.x = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
        a.y = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
    } while (n <= 100);
    if (recs[idx].floor == 0)
        recs[idx].floor = *(face**)scene->items._Myfirst;
done:
    if (recs[idx].cnt == 0)
        recs[idx].m2 = *(rgl::matrix*)sv;
}

// @ 0x472460
void PList::free_values()
{
    for (std::list<int*>::iterator i = begin(); i != end(); ++i)
        delete[] *i;
    clear();
    n = 0;
}

// @ 0x4724c0
MRow36* vecMRow36::ufill(MRow36* f, unsigned n, const MRow36& v)
{
    ptr_tag t;
    uninit_fill_n_24(f, n, v, *this, t);
    return f + n;
}

// @ 0x4724f0
MVert* vec18::ufill(MVert* f, unsigned n, const MVert& v)
{
    ptr_tag t;
    uninit_fill_n_18(f, n, v, *this, t);
    return f + n;
}

// @ 0x472520
MPoly* vec64::ufill(MPoly* f, unsigned n, const MPoly& v)
{
    ptr_tag t;
    uninit_fill_n_64(f, n, v, *this, t);
    return f + n;
}

// @ 0x472550
MEdge* vec34::ufill(MEdge* f, unsigned n, const MEdge& v)
{
    ptr_tag t;
    uninit_fill_n_34(f, n, v, *this, t);
    return f + n;
}

// @ 0x472580
template std::vector<MEdge>::iterator std::vector<MEdge>::erase(std::vector<MEdge>::iterator, std::vector<MEdge>::iterator);

// @ 0x4725d0
template std::vector<Rec1c8>::iterator std::vector<Rec1c8>::erase(std::vector<Rec1c8>::iterator, std::vector<Rec1c8>::iterator);

// @ 0x472630
ArrSet* vecArr::ucopy(ArrSet* f, ArrSet* l, ArrSet* d)
{
    ptr_tag t;
    return uninit_copy_arrset(f, l, d, *this, t);
}

// (0x467f80: a copy of the other object's function, kept in this object so cl knows its register use)
float MissileG::ammo_scale()
{
    if (ammo_item >= 0)
        return (float)g_items_g[ammo_item].f20c * 0.01f;
    return 0.8f;
}
// @ 0x472650
bool MissileG::settle(const char* arg)
{
    const QueryPt* q = (const QueryPt*)arg;
    bool r = false;
    HeightField* hf = g_mission->scene->hf;
    if (hf) {
        float h;
        int id;
        hf->sample(q->x, q->y, &h, &id);
        if (q->z < h) {
            r = true;
            if (id >= 0) {
                NavCell* gf = &g_mission->scene->core->cells[id];
                vector4 pl = gf->trans;
                vector4 n;
                n.x = -pl.x; n.y = -pl.y; n.z = -pl.z;
                float t = (ammo_scale() + 0.02f) - ((h - q->z) * pl.z) / dot3(n, dir);
                vector4 d(t * dir.x, t * dir.y, t * dir.z);
                vector4* p = &pos;
                p->x += d.x; p->y += d.y; p->z += d.z;
                vector4* hp = &owner->hit;
                *hp = *p;
            }
        }
    }
    return r;
}

// @ 0x4727a0
void __cdecl destroy_arrsets(ArrSet* first, ArrSet* last)
{
    for (; first != last; ++first)
        first->~ArrSet();
}

// @ 0x4727e0
void __cdecl uninit_fill_n_arrset(ArrSet* first, unsigned n, const ArrSet& val, alloc_arrset& al, ptr_tag tag)
{
    ArrSet* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy(next);
        throw;
    }
}

// @ 0x472848
// (catch funclet of uninit_fill_n_arrset)

// ---- std::stable_sort of 8-byte records with a function-pointer predicate (VC7.1 <algorithm>) ----------------------------
// @ 0x472870 (_Chunked_merge: merge adjacent chunks into the temp buffer)
template void std::_Chunked_merge(It8, It8, std::_Temp_iterator<P8>, int, int, Pred8);
// @ 0x472980 (merge)
template It8 std::merge(P8*, P8*, P8*, P8*, It8, Pred8);
// @ 0x472a20 (_Merge_backward)
template It8 std::_Merge_backward(It8, It8, P8*, P8*, It8, Pred8);
// @ 0x472ad0 (_Buffered_rotate)
template It8 std::_Buffered_rotate(It8, It8, It8, int, int, std::_Temp_iterator<P8>&);

// @ 0x472c50
void PhysSet::clamp_rec(int i)
{
    if (recs[i].world) {
        for (unsigned j = 0; j < recs[i].bones.size(); j++) {
            NewtonBody* b = NewtonRagDollBoneGetBody(recs[i].bones[j]);
            if (b) {
                vector4 v;
                NewtonBodyGetVelocity(b, &v.x);
                if (v.length() > 30.0f) {
                    v.normalize();
                    v.x *= 30.0f;
                    v.y *= 30.0f;
                    v.z *= 30.0f;
                    NewtonBodySetVelocity(b, &v.x);
                }
                NewtonBodyGetOmega(b, &v.x);
                if (v.length() > 9.0f) {
                    v.normalize();
                    v.x *= 9.0f;
                    v.y *= 9.0f;
                    v.z *= 9.0f;
                    NewtonBodySetOmega(b, &v.x);
                }
            }
        }
    }
}

// @ 0x472dc0
void PhysSet::update(float dt)
{
    float step = 0.018f;
    if (nworlds > 1) {
        step = 0.021f;
        if (nworlds > 2) {
            step = 0.024f;
            if (nworlds > 3)
                step = 0.026f;
        }
    }
    if (dt > 0.075f)
        step *= 1.5f;
    while (dt > 0.0001f) {
        float s;
        if (dt < step)
            s = dt;
        else
            s = step;
        dt -= s;
        for (unsigned i = 0; i < recs.size(); i++) {
            if (recs[i].world) {
                clamp_rec(i);
                NewtonUpdate(recs[i].world, s);
            }
        }
    }
}

// @ 0x472ed0
void CellGrid::update_probes(unsigned flags)
{
    for (NavVert* pr = core->verts.first; pr < core->verts.last; pr++) {
        if (pr->level < -0.1f) {
            vector4 dir, col;
            light_at(pr->p, pr->n, &dir, &col, flags, 1.0f);
            pr->level = (col.x + col.y + col.z) * 0.33f;
        }
    }
}

// @ 0x472f90
void MAgent::reset()
{
    fd8 = -1;
    fd4 = -1;
    fdc = -1;
    fe0 = 0;
    fe4 = -1;
    fe8 = 0;
    fec = 0;
    ff0 = 1.0f;
    ff4 = 1.0f;
    ff8 = -1.0f;
    ffc = 100;
    f104 = -1;
    f100 = -1;
    f108 = -1;
    f10c = 0;
    f110 = -1;
    f114 = 0;
    f118 = 0;
    f11c = 1.0f;
    f120 = 1.0f;
    f124 = -1.0f;
    f128 = 100;
    view.init();
    f50 = vector4(0.0f, 0.0f, 0.0f);
    f55d8 = vector4(0.0f, 1.0f, 0.0f);
    f60 = vector4(0.0f, 1.0f, 0.0f);
    f14 = 0;
    f18 = 0;
    f88 = -1;
    f34 = 1;
    f38 = 9;
    f55d0 = 0;
    f70 = 0;
    f74 = 0;
    f78 = 0.15f;
    fa8 = 0;
    fac = 0;
    f58e0 = vector4(-10000.0f, -100000.0f, -100000.0f);
    f98 = vector4(0.0f, 0.0f, 0.0f);
    f5624 = vector4(0.0f, 0.0f, 0.0f);
    f56a4 = vector4(0.0f, 0.0f, 0.0f);
    fb0 = 1.0f;
    f250 = 0;
    fbc = 0;
    f84 = 0;
    f5880 = 0;
    f58c4 = 1;
    f58c5 = 1;
    f58d8 = 1;
    f58dc = -1000;
    f5838 = -1;
    f583c = 0;
    f80 = 1.0f;
    f1bc = -1;
    f1c0 = -1;
    f94 = -1;
    f5878 = 1;
    f7c = 0;
    f12c = 0;
    f138 = -1;
    f134 = -1;
    f2c = -1;
    f30 = -1;
    f130 = 0;
    f56c4 = 0;
    f56c8 = 0;
    f56cc = 0;
    f560c = 0;
    f5610 = 0;
    f5614 = -1;
    f5634 = 0;
    f5638 = 0;
    fe0 = -1;
    f10c = 0;
    f56d0 = 0;
    f3c = 1;
    fb8 = 1;
    fb4 = 0;
    f5858 = -1;
    f56b4 = vector4(0.0f, 0.0f, 0.0f);
    fc4 = vector4(100000.0f, 10000.0f, 100000.0f);
    f13c = 0; f140 = 0; f144 = 0; f148 = 0; f14c = 0; f150 = 0; f154 = 0; f158 = 0;
    f15c = 0; f160 = 0; f164 = 0; f168 = 0; f16c = 0; f170 = 0; f174 = 0; f178 = 0;
    f17c = 0; f180 = 0; f184 = 0; f188 = 0; f18c = 0; f190 = 0;
    f1a4 = 0;
    f1a8 = 0;
    f5714 = 0;
    f56f0 = 0;
    f5718 = -1;
    f22c = -1;
    f230 = -1;
    f234 = -1;
    f238 = -1;
    f56d8 = 0;
    f56dc = 0;
    f56e0 = 1.0f;
    f56e4 = 1.0f;
    f56d4 = 0;
    f5874 = 0;
    f5730 = 0;
    f1ac = 0;
    f1b0 = 0;
    f1b4 = 0;
    f1b8 = 0;
    f5830 = 2;
    f5844 = 0;
    f5850 = 90000.0f;
    if (f20)
        delete f20;
    f20 = 0;
    f24 = 0;
    f28 = 0;
    f587c = -1;
    f57b4 = 1.0f;
    fc0 = 0;
    f57d8 = 0;
    t57e0.t = tk(t57e0.state);
    t57f0.t = tk(t57f0.state);
    t5800.t = tk(t5800.state);
    t5820.t = tk(t5820.state);
    t40.t = tk(t40.state);
    f5868 = tk(f5860);
    t5918.t = tk(t5918.state);
    t5810.t = tk(t5810.state);
    f58f0 = vector4(0.0f, 0.0f, 0.0f);
    f55e8 = 0;
    f5834 = 0;
    f5900 = 0;
    f56ec = 0;
    f5728 = 0;
    f572c = 0;
}

// @ 0x473570
void TroopMgr::copy_troop(ArrSet* dst, const TroopRec* r)
{
    dst->name = r->name;
    for (int i = 0; i < r->ngroups; i++) {
        if (r->groups[i].size() != 0)
            ((TroopMgrExt*)this)->apply_group(dst, i, (rgl::mesh*)r->groups[i][0]);
    }
}

// @ 0x4735e0
void FaceRace::relax(int skip, float* v)
{
    for (unsigned i = 0; i < (unsigned)size(); i++) {
        bool done = false;
        for (int iter = 0; !done; iter++) {
            Constraint* c = &first[i];
            float sum = 0.0f;
            int free_n = 0;
            for (int j = 0; j < c->n; j++) {
                sum += v[c->t[j].idx] * c->t[j].coef;
                if (c->t[j].idx != skip)
                    free_n++;
            }
            if ((c->mode == -1 && sum < c->limit) || (c->mode == 1 && c->limit < sum)) {
                float d = c->limit - sum;
                float cnt = (float)free_n;
                if (!(cnt > 1.0f))
                    cnt = 1.0f;
                float step = d / cnt;
                for (int j = 0; j < c->n; j++) {
                    if (c->t[j].idx != skip) {
                        v[c->t[j].idx] += step / c->t[j].coef;
                        if (v[c->t[j].idx] < 0.0f)
                            v[c->t[j].idx] = 0.0f;
                        else if (v[c->t[j].idx] > 1.0f)
                            v[c->t[j].idx] = 1.0f;
                    }
                }
                if (iter > 30)
                    done = true;
            } else {
                done = true;
            }
        }
    }
}

// @ 0x473800
FaceRace* FaceGen::unpack(const unsigned __int64* key)
{
    pick_b = ((unsigned)key[0] & (g_mask64 - 1)) - 1;
    pick_a = ((unsigned)(key[0] >> 6) & (g_mask64 - 1)) - 1;
    pick_c = (unsigned)(key[0] >> 12) & (g_mask64 - 1);
    pick_d = (unsigned)(key[0] >> 18) & (g_mask64 - 1);
    FaceRace* r = &races[race];
    if (pick_b >= r->n_b)
        pick_b = -1;
    if (pick_a >= r->n_a)
        pick_a = -1;
    if (pick_c >= r->n_c)
        pick_c = 0;
    if (pick_d >= r->n_d)
        pick_d = 0;
    age = (float)(int)((unsigned)(key[0] >> 24) & (g_mask64 - 1)) * 0.015873017f;
    weight = (float)(int)((unsigned)(key[0] >> 30) & (g_mask64 - 1)) * 0.015873017f;
    int i = 0;
    for (r = &races[race]; i < r->nmorph; r = &races[race]) {
        int w = 1;
        int sh = i * 3;
        if (sh >= 63) {
            w = 2;
            sh -= 63;
        }
        morph[i] = (float)(int)((unsigned)(key[w] >> sh) & (g_mask8 - 1)) * 0.14285715f;
        i++;
    }
    return r;
}

// @ 0x473980
void Terrain::refresh_cells()
{
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            TCell* c = &(&cell[0][0])[x * 251 + y];
            c->f40 = ((TerrainExt*)this)->sun_visibility(&c->x);
        }
    }
}

// @ 0x4739f0
void __stdcall reset_spawned(int party_id)
{
    if (party_id != -1) {
        PartyMgr* pm = &g_game->parties;
        int i = pm->index_of(party_id);
        Party* p = &pm->pages[i >> 4][i & 15];
        for (int k = 0; k < p->nstacks; k++)
            p->stacks[k].spawned = 0;
    }
}

// helpers of pick_spawn (0x473a60): the party table lookup and the low-hp test, inlined in the original
inline bool agent_low_inl(int idx)
{
    if (idx == g_game->player)
        return g_game->troops[idx].hp < g_thr_self;
    return g_game->troops[idx].hp < g_thr_other;
}
inline Party* get_party(int id)
{
    PartyMgr* pm = &g_game->parties;
    int i = pm->index_of(id);
    return &pm->pages[i >> 4][i & 15];
}
// @ 0x473a60
void Mission::pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed)
{
    *out_party = -1;
    *out_stack = -1;
    SpawnRec* r = &g_modes2[mode].recs[idx];
    *out_side = -1;
    *out_flag = -1;
    *out_seed = -1;
    *out_troop = -1;
    *out_side = ((r->flags >> 12) & 7) - 1;
    int list_idx = -1;
    int local_side = -1;
    if (r->flags & 0x10) {
        *out_flag = 0;
        *out_troop = g_game->levels[f5df4].pairs[idx].a;
        if (g_game->levels[f5df4].pairs[idx].b == -0xd0cd)
            *out_seed = g_game->f4 * 0x93 + r->f1c * 0x13 + idx * 0x25 + f5df4 * 0x4f;
        else
            *out_seed = g_game->levels[f5df4].pairs[idx].b;
    } else if (r->flags & 4) {
        if (r->id == f5df8) {
            *out_troop = g_game->player;
        } else if (*out_troop == -1 && terrain.f180 == 0) {
            for (int i = 1; i < g_game->ntroops; i++) {
                unsigned v = g_game->troops[i].f3b8;
                if ((v & 0xffff) == (unsigned)f5df4 && ((v >> 16) & 0xff) == (unsigned)r->id && g_game->troops[i].used == 0)
                    *out_troop = i;
            }
        }
    } else if (r->flags & 0xc0) {
        int a = r->flags & 0x40;
        int b = r->flags & 0x80;
        if (((a && f6214 == 0) || (b && f6214 == 1)) || (f6214 == 2 && ((f6218 == 1 && a) || (f6218 == 0 && b)))) {
            local_side = 0;
            if (g_game->troops[g_game->player].used == 0) {
                *out_party = g_game->f78;
            } else {
                int k = rand_below(&cnt[1 - f6218]);
                *out_party = list[1 - f6218][k];
                list_idx = 1 - f6218;
            }
        } else {
            local_side = 1;
            int k = rand_below(&cnt[f6218]);
            *out_party = list[f6218][k];
            list_idx = f6218;
        }
    } else if (r->flags & 1) {
        int* pc = &cnt[f6218];
        int k;
        if (*pc > 1)
            k = rand() % *pc;
        else
            k = 0;
        *out_party = list[f6218][k];
        list_idx = f6218;
        *out_side = 1;
    } else {
        if (r->flags & 2) {
            int k = rand_below(&cnt[1 - f6218]);
            *out_party = list[1 - f6218][k];
            list_idx = 1 - f6218;
        } else {
            *out_party = g_game->f78;
        }
        *out_side = 0;
    }

    if (*out_party >= 0) {
        Party* party = get_party(*out_party);
        int end = party->nstacks;
        int start = 0;
        int step = 1;
        if (r->flags & 0x1000000) {
            start = party->nstacks - 1;
            end = -1;
            step = -1;
        }
        int total = party->sum_stat(0x112);
        char x4 = 0, x3 = 0, x5 = 0;
        int ncand = (list_idx == -1) ? 1 : cnt[list_idx];
        if (idx >= 0)
            done[idx] = 1;
        for (int c = 0; c < ncand; c++) {
            int tmpl = (list_idx == -1) ? g_game->f78 : list[list_idx][c];
            Party* p2 = get_party(tmpl);
            int n2 = p2->nstacks;
            int nfirst;
            for (nfirst = 0; nfirst < n2; nfirst++)
                if (p2->stacks[nfirst].flags & 1)
                    break;
            if (nfirst > 0) {
                for (int j = 0; j < nfirst; j++) {
                    Stack* s = &p2->stacks[j];
                    Troop* t = &g_game->troops[s->troop];
                    if ((t->flags & 0x10) == 0 && s->count > s->wounded + s->spawned) {
                        done[idx] = 0;
                        if ((t->flags & 0x1000000) && (r->flags2 & 0x100) == 0)
                            x5 = 1;
                        else if (t->flags & 0x4000000)
                            x4 = 1;
                        else
                            x3 = 1;
                    } else if (s->spawned == 0) {
                        if (!agent_low_inl(s->troop)) {
                            done[idx] = 0;
                            if (t->f4e4 < 0)
                                x3 = 1;
                            else
                                x5 = 1;
                        }
                    }
                }
            }
        }
        if (*out_party == g_game->f78 && party->stacks[0].spawned == 0) {
            *out_stack = 0;
            *out_troop = g_game->player;
            party->stacks[0].spawned = 1;
        }
        if (start != end) {
            for (int k = start; k != end; k += step) {
                if (*out_troop != -1)
                    break;
                Stack* s = &party->stacks[k];
                Troop* t = &g_game->troops[s->troop];
                __int64 hero = t->flags & 0x10;
                if ((hero != 0 && s->spawned == 0 && t->used == 0 && (s->troop == g_game->player || !g_game->agent_low(s->troop)))
                    || (hero == 0 && s->count > s->wounded + s->spawned)) {
                    if (rule_accepts(r, s, k, x3, x4, x5)) {
                        if (hero == 0) {
                            float chance = (float)(s->count - s->wounded) / (float)total + 0.25f;
                            float rnd = (float)(rand() % 15817) * (1.0f / 15817.0f);
                            if (rnd > chance)
                                continue;
                        }
                        *out_stack = k;
                        *out_troop = s->troop;
                        if ((g_game->troops[s->troop].flags & 0x10) == 0) {
                            *out_seed = rand();
                            if (s->spawned >= 0 && s->spawned < 32) {
                                if (*out_party == g_game->f78)
                                    *out_seed = g_game->tbl[k][s->spawned];
                            } else {
                                *out_seed = rand();
                            }
                        }
                        s->spawned++;
                    }
                }
            }
        }
    }
    if (*out_troop >= 0) {
        g_game->troops[*out_troop].used = 1;
        if (g_game->troops[*out_troop].flags & 0x10)
            *out_seed = -1;
    }
    if (*out_side == -1)
        *out_side = local_side;
}

// @ 0x474300
template<> void Pool<RecJ>::next_used(unsigned* i)
{
    ++*i;
    while (valid(*i)) {
        if (pages[*i >> 4][*i & 15].used != 0)
            return;
        ++*i;
    }
}

// @ 0x474350
ArrSet* vecArr2::ucopy(ArrSet* f, ArrSet* l, ArrSet* d)
{
    ptr_tag t;
    return uninit_copy_arrset2(f, l, d, *this, t);
}

// @ 0x474370 (_Insertion_sort)
template void std::_Insertion_sort(It8, It8, Pred8);
// @ 0x474410 (_Buffered_merge: merge two sorted runs using the temp buffer)
template void std::_Buffered_merge(It8, It8, It8, int, int, std::_Temp_iterator<P8>&, Pred8);
// @ 0x474650 (_Chunked_merge: merge chunk pairs from the temp buffer back into the vector)
template void std::_Chunked_merge(P8*, P8*, It8, int, int, Pred8);

#define RAND() (g_rand_seed = (g_rand_seed * 0x1c13 - 0xd098556) * 0x1105)
#define RND01() ((float)(RAND() % 0x3dc9) * 6.322312e-05f)

static __forceinline int round_int(const float& x)
{
    float fl = floor(x);
    int r = (int)fl;
    return (x - fl <= 0.5f) ? r : r + 1;
}
static __forceinline int lerp_int(int a, const int& b)
{
    float t = RND01() * (b - a); return a + round_int(t);
}

#define MIX(k) { RAND(); float lo = save[i + k]; morph[i + k] = (morph[i + k] - lo) * ((float)(RAND() % 0x3dc9) * 0.00012644623f) * 0.5f + lo; }

// @ 0x4746e0
void FaceGen::breed(const unsigned __int64* a, const unsigned __int64* b, int seed)
{
    g_rand_seed = (seed + 0x8a767) * 0x1ddcf5f + 0x2f784719;
    ((FaceGenExt*)this)->unpack(a);
    int s3 = pick_d; float a0 = weight; float a1 = age;
    int s1 = pick_b; int s0 = pick_a; int s5 = pick_c;
    float save[63];
    int i;
    for (i = 0; i < 63; i += 9) {
        save[i] = morph[i]; save[i+1] = morph[i+1]; save[i+2] = morph[i+2];
        save[i+3] = morph[i+3]; save[i+4] = morph[i+4]; save[i+5] = morph[i+5];
        save[i+6] = morph[i+6]; save[i+7] = morph[i+7]; save[i+8] = morph[i+8];
    }
    ((FaceGenExt*)this)->unpack(b);
    pick_b = lerp_int(s1, pick_b);
    pick_a = lerp_int(s0, pick_a);
    pick_c = lerp_int(s5, pick_c);
    pick_d = -1;
    FaceHairRec* r = &races[race].recs[pick_c];
    IntVec* v = &r->list;
    int n;
    if (v->_Myfirst == 0 || (n = (int)(v->_Mylast - v->_Myfirst)) <= 0) {
        pick_d = lerp_int(s3, pick_d);
    } else {
        RAND();
        pick_d = races[race].recs[pick_c].list._Myfirst[g_rand_seed % (unsigned)n];
    }
    weight = (float)(RAND() % 0x3dc9) * (weight - a0) * 6.322312e-05f + a0;
    age = (float)(RAND() % 0x3dc9) * (age - a1) * 6.322312e-05f + a1;
    for (i = 0; i < 63; i += 9) {
        MIX(0) MIX(1) MIX(2) MIX(3) MIX(4) MIX(5) MIX(6) MIX(7) MIX(8)
    }
    ((FaceRaceExt*)&races[race])->relax(-1, morph);
}

// inline vector4 arithmetic as the game's header implements it: 3-component results, leaving the fourth lane
// uninitialised (whole-struct copies carry it along)
static __forceinline vector4 vscale(const vector4& a, const float& s)
{
    return vector4(a.x * s, a.y * s, a.z * s);
}
static __forceinline vector4 vsum(const vector4& a, const vector4& b)
{
    return vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}
static __forceinline float normalize_inline(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv;
        v.y = v.y * inv;
        v.z = v.z * inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}
#define AGENT(m, id) ((m)->agents.pages[(id) >> 4][(id) & 0xf])

// @ 0x474e60
void Mission::update_team_stats()
{
    int t, k;
    for (t = 0; t < 8; t++) {
        Team& tm_ = teams[t];
        if ((int)tm_.leader >= 0) {
            for (k = 0; k < 3; k++) {
                ClassInfo& c = tm_.cls[k];
                if (c.kind == 2) {
                    MAgent& a = AGENT(this, tm_.leader);
                    c.pos = a.f50;
                    c.nav = AGENT(this, tm_.leader).update_nav_face();
                    c.target = AGENT(this, tm_.leader).f56b4;
                }
            }
        }
    }
    if ((float)(g_ticks[tm.clock] - tm.start) * 1e-5f >= 0.0f) {      // Timer::elapsed() inlined
        tm.start = g_ticks[tm.clock] + 110000;
        vector4 zero(0.0f, 0.0f, 0.0f);
        for (t = 0; t < 8; t++)
            for (k = 0; k < 3; k++) {
                teams[t].cls[k].count = 0.0f;
                teams[t].cls[k].sum = zero;
            }

        unsigned i = agents.first_used();
        while (i < (unsigned)agents.next_id) {
            MAgent& a = AGENT(this, i);
            if (a.f14 == 1 && a.f10 == 0) {
                int tid = a.team;
                if (tid < 0 || tid >= 8) tid = 7;
                int cl = a.troop_class();
                a.head_count = (int)(teams[tid].cls[cl].count > 0.0f ? teams[tid].cls[cl].count : teams[tid].cls[cl].count - 0.99999988f);
                if (tid >= 0 && tid < 8 && cl >= 0 && cl < 3) {
                    teams[tid].cls[cl].count = teams[tid].cls[cl].count + 1.0f;
                    teams[tid].cls[cl].sum.x = a.f50.x + teams[tid].cls[cl].sum.x;
                    teams[tid].cls[cl].sum.y = a.f50.y + teams[tid].cls[cl].sum.y;
                    teams[tid].cls[cl].sum.z = a.f50.z + teams[tid].cls[cl].sum.z;
                }
            }
            agents.next_used(&i);
        }

        for (t = 0; t < 8; t++) {
            Team& tm_ = teams[t];
            for (k = 0; k < 3; k++) {
                float n = tm_.cls[k].count;
                for (int j = 0; j < 3; j++) {
                    if (j != k) {
                        float dx = tm_.cls[k].pos.x - tm_.cls[j].pos.x;
                        float dy = tm_.cls[k].pos.y - tm_.cls[j].pos.y;
                        float dz = tm_.cls[k].pos.z - tm_.cls[j].pos.z;
                        if (sqrt(dx * dx + (dy * dy + dz * dz)) < 1.0f)
                            n = n + tm_.cls[j].count;
                    }
                }
                tm_.cls[k].half = n * 0.5f;
                tm_.cls[k].total = n;
            }
            if ((int)tm_.leader >= 0 && AGENT(this, tm_.leader).f14 != 1) {
                bool found = false;
                unsigned i2 = agents.first_used();
                while (i2 < (unsigned)agents.next_id) {
                    MAgent& a = AGENT(this, i2);
                    if (!found && a.f14 == 1 && a.f10 != 1) {
                        int tid = a.team;
                        if (tid < 0 || tid >= 8) tid = 7;
                        if (tid == t) {
                            found = true;
                            tm_.leader = i2;
                        }
                    }
                    agents.next_used(&i2);
                }
            }
        }

        for (t = 0; t < 8; t++) {
            for (k = 0; k < 3; k++) {
                float inv = 1.0f / teams[t].cls[k].count;
                teams[t].cls[k].sum.x = inv * teams[t].cls[k].sum.x;
                teams[t].cls[k].sum.y = inv * teams[t].cls[k].sum.y;
                teams[t].cls[k].sum.z = inv * teams[t].cls[k].sum.z;
            }
        }

        {
            unsigned i3 = agents.first_used();
            while (i3 < (unsigned)agents.next_id) {
                MAgent& a = AGENT(this, i3);
                if (a.f14 == 1 && a.f10 != 1) {
                    int tid = a.team;
                    if (tid < 0 || tid >= 8) tid = 7;
                    int cl = a.troop_class();
                    if (tid >= 0) {
                        a.view.v50e8.x = (float)a.head_count - teams[tid].cls[cl].count * 0.5f;
                        if (tid == 2) a.view.v50e8.y = 1.2f;
                        else if (tid == 1) a.view.v50e8.y = 0.0f;
                        else if (tid == 0) a.view.v50e8.y = -0.7f;
                    }
                }
                agents.next_used(&i3);
            }
        }

        for (t = 0; t < 8; t++) {
            Team& tm_ = teams[t];
            if ((int)tm_.leader >= 0) {
                for (k = 0; k < 3; k++) {
                    ClassInfo& c = tm_.cls[k];
                    vector4 v;
                    v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
                    unsigned i4 = agents.first_used();
                    while (i4 < (unsigned)agents.next_id) {
                        MAgent& a = AGENT(this, i4);
                        if (a.f14 == 1 && a.f10 != 1) {
                            int tid = a.team;
                            if (tid < 0 || tid >= 8) tid = 7;
                            a.troop_class();
                            if (teams[t].relation[tid] == -1) {
                                vector4 d;
                                d.x = a.f50.x - c.pos.x;
                                d.y = a.f50.y - c.pos.y;
                                d.z = a.f50.z - c.pos.z;
                                float len = normalize_inline(d);
                                if (len < 2.0f) len = 2.0f;
                                else if (len > 400.0f) len = 400.0f;
                                float s = 1.0f / len;
                                volatile float tx = d.x * s;   // original spills this term to a
                                v.x = v.x + tx;                // float slot, rounding it before the add
                                v.y = v.y + d.y * s;
                                v.z = v.z + d.z * s;
                            }
                        }
                        agents.next_used(&i4);
                    }
                    v.z = 0.0f;
                    if (v.length() > 1e-11f) {
                        normalize_inline(v);
                        if (c.dir.length_squared() < 0.5f) {
                            c.dir = v;
                        } else {
                            c.dir = vsum(vscale(c.dir, 0.8f), vscale(v, 0.2f));
                            normalize_inline(c.dir);
                        }
                    }
                }
            }
        }

        {
            unsigned i5 = agents.first_used();
            while (i5 < (unsigned)agents.next_id) {
                AGENT(this, i5).f55e8 = 0;
                agents.next_used(&i5);
            }
        }
        {
            unsigned i6 = agents.first_used();
            while (i6 < (unsigned)agents.next_id) {
                int o = AGENT(this, i6).view.target;
                if (o >= 0 && o < agents.next_id && AGENT(this, o).used != 0)
                    AGENT(this, o).f55e8++;
                agents.next_used(&i6);
            }
        }
    }
}

// @ 0x4759c0
void Mission::update_team_counts()
{
    int old[8][6];
    int* dst = &old[0][0];
    int* src = &counts[0][0];
    for (int t = 8; t != 0; t--) {
        for (int j = 6; j != 0; j--) {
            *dst = *src;
            *src = 0;
            dst++;
            src++;
        }
    }
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.next_id) {
        MAgent& a = agents.pages[i >> 4][i & 0xf];
        if (a.f10 == 0 && a.team >= 0 && a.team < 8)
            counts[a.team][a.f14]++;
        agents.next_used(&i);
    }
    for (int t = 0; t < 8; t++) {
        for (int j = 0; j < 6; j++)
            if (old[t][j] != counts[t][j]) changed_at = (float)g_ticks[2] * 1e-5f;
    }
}

// @ 0x475b60
void vecArrS::destroy(ArrSet* f, ArrSet* l)
{
    ptr_tag t;
    destroy_range_arrset_x(f, l, *this, t);
}
// @ 0x475b80
ArrSet* vecArrS::ufill(ArrSet* f, unsigned n, const ArrSet& v)
{
    ptr_tag t;
    uninit_fill_n_arrset_x(f, n, v, *this, t);
    return f + n;
}

// @ 0x475bb0 (_Buffered_merge_sort)
template void std::_Buffered_merge_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);

Cell::~Cell() {}                // 0x431ab0: defined here so that the array delete below is emitted as in the original

// @ 0x475c60
__declspec(noinline) CellGridBase::~CellGridBase()
{
    for (int i = 0; i < n[0]; i++)
        if (cells[i])
            delete[] cells[i];
    delete[] cells;
    pool0.free_values();
    pool1.free_values();
}

// @ 0x475d50
CellGrid::~CellGrid()
{
    cleanup();
}

// @ 0x475e50
ViewState::ViewState()
{
    t58.init(2);
    t98.init(2);
    ta8.init(2);
    t5118.init(2);
    t5128.init(2);
    v516c.x = 0.0f; v516c.y = 0.0f; v516c.z = 0.0f; v516c.w = 0.0f;
    Vec2 z; z.x = 0.0f; z.y = 0.0f;
    v50e8 = z;
}

// @ 0x475f20
template std::vector<ArrSet>::iterator std::vector<ArrSet>::erase(std::vector<ArrSet>::iterator, std::vector<ArrSet>::iterator);
// @ 0x475f70 (_Stable_sort, recursive)
template void std::_Stable_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);

// ---- vector<> instantiations of the object's records (VC7.1) ---------------------------------------------------------
// @ 0x476020 vector<E36x>::reserve (its catch(...) funclet follows at 0x476111), 0x476130 vector<E136>::reserve
// (funclet 0x476225), 0x476240 vector<E36x>::insert, 0x4762b0 vector<E136>::insert, 0x476320 vector<Rec1c8>::_Buy.
template void std::vector<E36x>::reserve(size_t);
template void std::vector<E136>::reserve(size_t);
template std::vector<E36x>::iterator std::vector<E36x>::insert(std::vector<E36x>::iterator, const E36x&);
template std::vector<E136>::iterator std::vector<E136>::insert(std::vector<E136>::iterator, const E136&);
template bool std::vector<Rec1c8>::_Buy(size_t);

// @ 0x476370 (+ 0x476515: catch funclet and cold branches)
template void std::vector<MRow36>::_Insert_n(std::vector<MRow36>::iterator, size_t, const MRow36&);
// @ 0x476620 (+ 0x4767de)
template void std::vector<MVert>::_Insert_n(std::vector<MVert>::iterator, size_t, const MVert&);
// @ 0x4768f0 (+ 0x476a9a)
template void std::vector<MPoly>::_Insert_n(std::vector<MPoly>::iterator, size_t, const MPoly&);
// @ 0x476bb0 (+ 0x476d51: catch funclet and cold branches)
template void std::vector<MEdge>::_Insert_n(std::vector<MEdge>::iterator, size_t, const MEdge&);
// @ 0x476e60 (+ 0x47701d, 0x4770dc)
template void std::vector<ArrSet>::_Insert_n(std::vector<ArrSet>::iterator, size_t, const ArrSet&);

// 0x477160 vector<Rec1c8> assignment (no marker: a scope-less operator= annotation would capture every operator= callee)
template std::vector<Rec1c8>& std::vector<Rec1c8>::operator=(const std::vector<Rec1c8>&);
// @ 0x4772f0
template void std::vector<E36x>::push_back(const E36x&);
// @ 0x477380
template void std::vector<E136>::push_back(const E136&);
// @ 0x477410
template std::vector<MRow36>::iterator std::vector<MRow36>::insert(std::vector<MRow36>::iterator, const MRow36&);
// @ 0x477480
template void std::vector<MVert>::resize(size_t, MVert);
// @ 0x477530
template std::vector<MVert>::iterator std::vector<MVert>::insert(std::vector<MVert>::iterator, const MVert&);
// @ 0x4775a0
template void std::vector<MPoly>::resize(size_t, MPoly);
// @ 0x477640
template std::vector<MPoly>::iterator std::vector<MPoly>::insert(std::vector<MPoly>::iterator, const MPoly&);
// @ 0x4776b0
template void std::vector<MEdge>::resize(size_t, MEdge);
// @ 0x477750
template std::vector<MEdge>::iterator std::vector<MEdge>::insert(std::vector<MEdge>::iterator, const MEdge&);
// @ 0x4777c0
template void std::vector<Rec1c8>::resize(size_t, Rec1c8);
// @ 0x477880
template void std::vector<ArrSet>::resize(size_t, ArrSet);

// @ 0x477940 std::copy over MRow56 (operator= is memberwise: vector<int>::operator= 0x43f0a0 for the two vectors)
MRow56* __cdecl copy_e56(MRow56* first, MRow56* last, MRow56* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}

// @ 0x4779b0
void mesh_flip::flip_faces()
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    for (unsigned i = 0; i < v90.size(); i++) {
        int t = v90[i].a;
        v90[i].a = v90[i].c;
        v90[i].c = t;
    }
    slots[1].f28 = 1;
    slots[0].f28 = 1;
}

// @ 0x477a30
MAgent::MAgent()
{
    f20 = 0; f24 = 0; f28 = 0;
    t40.state = 2; t40.t = tk(2);
    view.init_timers();
    subB.init();
    t57c8.state = 2; t57c8.t = tk(2); f57d8 = 0;
    t57e0.state = 2; t57e0.t = tk(2);
    t57f0.state = 2; t57f0.t = tk(2);
    t5800.state = 2; t5800.t = tk(2);
    t5810.state = 2; t5810.t = tk(2);
    t5820.state = 2; t5820.t = tk(2);
    f5860 = 0; f5868 = tk(0);
    t5918.state = 2; t5918.t = tk(2);
}

// @ 0x477b70
void EmptySlot::slot() {}

// @ 0x477b80
template void std::vector<MRow36>::push_back(const MRow36&);
// @ 0x477c10
template void std::vector<MVert>::push_back(const MVert&);
// @ 0x477ca0 resize(n) pads with MPoly()
template void std::vector<MPoly>::resize(size_t);
// @ 0x477d20
template void std::vector<MPoly>::push_back(const MPoly&);
// @ 0x477db0 resize(n) pads with MEdge()
template void std::vector<MEdge>::resize(size_t);
// @ 0x477e10
template void std::vector<MEdge>::push_back(const MEdge&);
// @ 0x477ea0 resize(n) pads with Rec1c8() (the inlined constructor 0x437830)
template void std::vector<Rec1c8>::resize(size_t);

// ---- id pools ---------------------------------------------------------------------------------------------------
// @ 0x477f00
template<> void Pool<MAgent>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < next_id; i++) {
        pages[(unsigned)i >> 4][i & 0xf].used = 0;
        free_ids.push_back(i);
    }
}
// @ 0x477f90
template<> void Pool<RecJ>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < next_id; i++) {
        pages[(unsigned)i >> 4][i & 0xf].used = 0;
        free_ids.push_back(i);
    }
}
// @ 0x478020
template<> int Pool<MAgent>::alloc()
{
    if (!(free_ids.size() > 0)) {
        MAgent* blk = new MAgent[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].used = 0;
            blk[j].id = next_id + j;
            free_ids.push_back(next_id + j);
        }
        next_id += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    MAgent* r = &pages[id >> 4][id & 0xf];
    r->used = 1;
    r->serial = serial;
    serial++;
    return id;
}

// ---- the nav map (Mesh) ---------------------------------------------------------------------------------------------
// @ 0x478150
NavCore::NavCore()
{
    e44 = vector4(0.0f, 0.0f, 0.0f);
    f54 = vector4(0.0f, 0.0f, 0.0f);
    m64 = 0;
}

// @ 0x4781d0
void Mesh::load(stream_base* s)
{
    int n1, n2, n3;
    s->r_int(&n1);
    MVert v;
    v.flag = 0;
    c.verts.resize(n1, v);
    for (int i = 0; i < n1; i++) {
        MVert* p = &c.verts[i];
        s->r_float(&p->pos.x).r_float(&p->pos.y).r_float(&p->pos.z);
    }
    s->r_int(&n2);
    MEdge e;
    c.edges.resize(n2, e);
    for (int i = 0; i < n2; i++) {
        s->r_int(&c.edges[i].nfaces);
        s->r_int(&c.edges[i].v0);
        s->r_int(&c.edges[i].v1);
        s->r_int(&c.edges[i].face[0]);
        s->r_int(&c.edges[i].face[1]);
    }
    s->r_int(&n3);
    c.faces.resize_default(n3);
    for (int i = 0; i < n3; i++) {
        s->r_int(&c.faces[i].n);
        for (int j = 0; j < c.faces[i].n; j++)
            s->r_int(&c.faces[i].vid[j]);
        for (int j = 0; j < c.faces[i].n; j++)
            s->r_int(&c.faces[i].eid[j]);
        s->r_int(&c.faces[i].f5c);
    }
}

// @ 0x478450
int Mesh::add_edge(int a, int b)
{
    raw_t raw;
    MEdge e(raw);
    e.face[0] = -10000;
    e.face[1] = -10000;
    e.v0 = a;
    e.flag = 0;
    e.nfaces = 0;
    e.crease[0] = 1.0f;
    e.crease[1] = 1.0f;
    e.v1 = b;
    c.edges.push_back(e);
    return (int)c.edges.size() - 1;
}

// @ 0x4784d0
int Mesh::add_face(int n, int v0, int v1, int v2, int v3, int e0, int e1, int e2, int e3)
{
    raw_t raw;
    MPoly f(raw);
    f.n = n;
    f.vid[0] = v0; f.vid[1] = v1; f.vid[2] = v2; f.vid[3] = v3;
    f.f0 = 0;
    f.eid[0] = e0; f.eid[1] = e1;
    f.f5c = 0;
    f.f60 = 0;
    f.eid[2] = e2; f.eid[3] = e3;
    c.faces.push_back(f);
    return (int)c.faces.size() - 1;
}

// @ 0x478580
void Mesh::clear()
{
    MVert v;
    v.flag = 0;
    c.verts.resize(0, v);
    c.edges.resize_default(0);
    c.faces.resize_default(0);
}

// @ 0x4785e0
void Mesh::build(HeightField* hf, int step_)
{
    step = step_;
    clear();
    int nx = (hf->w - 0x18) / step;
    int ny = (hf->h - 0x18) / step;
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            MVert v;
            v.flag = 0;
            v.pos = vector4((float)(step * ix + 0xc) * hf->scale, (float)(step * iy + 0xc) * hf->scale,
                            hf->cols[step * ix + 0xc][step * iy + 0xc].h + 0.4f);
            c.verts.push_back(v);
        }
    }
    for (int r = 0; r < ny; r++)
        for (int k = 0; k < nx - 1; k++)
            add_edge(r * nx + k, r * nx + k + 1);
    int nh = (nx - 1) * ny;
    for (int r = 0; r < ny - 1; r++)
        for (int k = 0; k < nx; k++)
            add_edge(r * nx + k, r * nx + k + nx);
    for (int i = 0; i < nx - 1; i++)
        for (int j = 0; j < ny - 1; j++)
            add_face(4, i + j * nx, i + 1 + j * nx, i + 1 + (j + 1) * nx, i + (j + 1) * nx,
                     j * (nx - 1) + i, nh + j * nx + i + 1, (j + 1) * (nx - 1) + i, nh + j * nx + i);
    ((MeshExt*)this)->build_adjacency();
}
