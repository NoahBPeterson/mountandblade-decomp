// Slice s0046e590: the shader-path scene renderer (render_batch::render_scene): per-scene effect constants (fog, point/sky/sun
// lights, sun shadow matrix, camera, time), render-state resets, then the per-mesh loop (vertex format / index buffer
// binding, part textures, light selection, billboard and world matrices, bone arrays, DrawIndexedPrimitive).
// ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <vector>
#include "rgl.h"
#include "mesh.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::face20;
using rgl::vertex60;

// ---- globals ------------------------------------------------------------------------------------------------
extern ID3DXEffect* g_effect;               // 0x5e293c
extern __int64 g_time[];                    // 0x5e2948 clock table (1e-5 s ticks)
extern int g_x5aa084;                       // 0x5aa084 (2: shader quality / mode)
extern int g_x5aa034;                       // 0x5aa034

using rgl::unpack_color;                    // 0x40e4e0 (cdecl, returns out; declared in mesh.h)
vector4* __cdecl identity3(vector4* out);                  // placeholder (not used)

// ---- frustum (layout from s00461bd0) ---------------------------------------------------------------------------
struct pos : matrix {
    matrix relative(const matrix& o) const;     // 0x4329d0
};
struct plane4 { vector4 pos, normal; };
struct frustum {
    char perspective;               // +0
    char left_handed;               // +1
    char pad2[2];
    int nplanes;                    // +4
    plane4 planes[6];               // +8
    pos m;                          // +0xc8 (s = right, f = up, u = backwards)
    float left, right, top, bottom, nearz, farz;   // +0x108
    D3DXMATRIX projection();                // 0x4620d0 (hidden return pointer)
    D3DXMATRIX view_matrix();               // 0x462170 (hidden return pointer)
};

// ---- renderer (global object at 0xa17910) -----------------------------------------------------------------------
struct renderer {
    char pad000[0x505];
    unsigned char b505;                 // 0x505 cached D3DRS_FOGENABLE
    char pad506[0x547 - 0x506];
    unsigned char b547, b548, b549, b54a, b54b;     // 0x547.. cached render-state flags
    char pad54c[0x590 - 0x54c];
    int alpha_mode;                     // 0x590
    unsigned color1;                    // 0x594 cached material colour 1
    unsigned color2;                    // 0x598 cached material colour 2
    char pad59c[0x5b8 - 0x59c];
    void* bone_origin_cache;            // 0x5b8
    int cull_setting;                   // 0x5bc
    int cull_cache;                     // 0x5c0
    void* tex_cache[5];                 // 0x5c4 diffuse, diffuse2, normal, specular, env
    int light_count;                    // 0x5d8
    int light_idx[4];                   // 0x5dc
    char pad5ec[4];
    float tex_off[2];                   // 0x5f0
    char pad5f8[0x600 - 0x5f8];
    float time_var;                     // 0x600
    char pad604[0x614 - 0x604];
    unsigned char b614;                 // 0x614
    char pad615[3];
    IDirect3DDevice9* dev;              // 0x618
    void set_blend_mode(int mode);      // 0x40e940
};
extern renderer g_render;               // 0xa17910

// ---- scene objects --------------------------------------------------------------------------------------------
struct texres {                         // texture record (s00416540)
    char pad0[0x18];
    char loaded;                        // 0x18
    char pad19[0x24 - 0x19];
    unsigned flags;                     // 0x24
    int count;                          // 0x28
    int width;                          // 0x2c
    int height;                         // 0x30
    char pad34[0x40 - 0x34];
    IDirect3DBaseTexture9** tex;        // 0x40
    IDirect3DBaseTexture9* current(const float& t) const
    {
        IDirect3DBaseTexture9* r;
        if (loaded) {
            int i = 0;
            if (flags & 0xf000000)
                i = (int)((float)count * t) % count;
            r = tex[i];
        } else {
            r = 0;
        }
        return r;
    }
};

struct tech_tbl { char pad[0x1c]; D3DXHANDLE tech[4]; };
struct material {
    char pad0[0x20];
    unsigned flags;                     // 0x20
    char pad24[0x74 - 0x24];
    unsigned texgen;                    // 0x74
    char pad78[0xc0 - 0x78];
    tech_tbl* techs;                    // 0xc0
};
struct mesh_part {
    char pad0[0x1c];
    unsigned flags;                     // 0x1c
    char pad20[0x30 - 0x20];
    material* mat;                      // 0x30
    char pad34[0x48 - 0x34];
    texres* diffuse;                    // 0x48
    texres* diffuse2;                   // 0x4c
    texres* normal;                     // 0x50
    texres* env;                        // 0x54
    texres* specular;                   // 0x58
    void set_state();                   // 0x461f80
};

struct vec3 {
    float x, y, z;
    vec3() {}
    vec3(float a, float b, float c) { x = a; y = b; z = c; }
};
static __forceinline vector4 make4(float a, float b, float c, float d)
{
    vector4 v;
    v.x = a; v.y = b; v.z = c; v.w = d;
    return v;
}
struct light {                          // 0x7c bytes (layout from s00461280)
    int type;                           // 0: point/dir with flicker, 2: sun
    vector4 diffuse;                    // 0x04
    vector4 v14;
    vec3 position;                      // 0x24
    char pad30[0x44 - 0x30];
    vec3 sun_dir;                       // 0x44
    char pad50[0x54 - 0x50];
    float radius;                       // 0x54
    char pad58[0x6c - 0x58];
    unsigned flags;                     // 0x6c
    char pad70[0x7c - 0x70];
    vector4 sample();                   // 0x461570
};

struct sky_t {
    char pad0[4];
    float fog_start_b, fog_end_b;       // 0x04, 0x08
    unsigned fog_color_b;               // 0x0c
    char fog_meshes;                    // 0x10
    char pad11[3];
    float fog_dens_b;                   // 0x14
    float fog_start_a, fog_end_a;       // 0x18, 0x1c
    unsigned fog_color_a;               // 0x20
    float height;                       // 0x24
    float fog_dens_a;                   // 0x28
    char sun_shadows;                   // 0x2c
    char pad2d[3];
    texres* shadow;                     // 0x30
    frustum shadow_cam;                 // 0x34
};

struct item_ref { mesh* m; int extra; };

struct scene {
    char pad0[0x30];
    frustum cam;                        // 0x30
    IUnknown* rt;                       // 0x150
    texres* env;                        // 0x154
    scene* water;                       // 0x158
    IUnknown* depth;                    // 0x15c
    char b160, b161, b162, pad163;      // 0x160
    int f164;                           // 0x164
    char pad168;
    char b169;                          // 0x169
    char pad16a[2];
    D3DXPLANE clip_plane;               // 0x16c
    vector4 ambient;                    // 0x17c
    char pad18c[0x194 - 0x18c];
    std::vector<light*> lights;         // 0x194
    char pad1a4[0x1b0 - 0x1a4];
    sky_t* sky;                         // 0x1b0
    std::vector<item_ref> items;        // 0x1b4
};

// ---- the batch renderer object (a ~4 MB object; only the touched fields are modelled) -----------------------------
struct vfmt {                               // 0x4c bytes: vertex format slot
    int f0;
    UINT stride;                            // 0x04
    IDirect3DVertexBuffer9* vb;             // 0x08
    char pad0c[0x48 - 0x0c];
    IDirect3DVertexDeclaration9* decl;      // 0x48
};
struct render_batch {
    char pad0[0x14];
    vfmt vf[10];                            // 0x14
    char pad30c[0x350 - 0x30c];
    IDirect3DIndexBuffer9* ib0;             // 0x350 (0x4c-byte slots)
    char pad354[0x3b4 - 0x354];
    int cur_vf_a;                           // 0x3b4
    int cur_ib;                             // 0x3b8
    int cur_vf_b;                           // 0x3bc
    void render_scene(scene* sc);           // 0x46e590
};

struct rot3 { vector4 s, f, u; };
static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
struct vec_arr { D3DXVECTOR4* data; UINT count; };
struct mi_ops { void setup_material(); };   // 0x40f3f0 (mesh instance, thiscall)
struct frame4 { rot3 rot; vector4 o; };
rot3* __cdecl identity3(rot3* out);        // 0x40cd60
struct m4 {
    vector4 s, f, u, o;
    m4 operator*(const m4& b) const;        // 0x40e870 (this = left operand)
    void rotate_fu(const float* angle);     // 0x40e570
};

#define AT(T, base, off) (*(T*)((char*)(base) + (off)))

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

            texres* st = sc->sky->shadow;
            if (st) {
                IDirect3DBaseTexture9* t = st->current(0.0f);
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

    float tv = (float)(g_time[2] % 1000000000) * 1e-5f;
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
                mesh_part* part = (mesh_part*)it->m->res;
                material* pm = part->mat;
                if (sc->f164) {
                    if ((int)pm->flags < 0)
                        mat = AT(material*, this, 0x418824);
                    else
                        mat = AT(material*, this, 0x418820);
                } else {
                    if ((int)pm->flags < 0)
                        mat = AT(material*, this, 0x41881c);
                    else
                        mat = AT(material*, this, 0x418818);
                }
                texres* t = part->diffuse;
                if (t) {
                    IDirect3DBaseTexture9* cur = t->current(0.0f);
                    if (g_render.tex_cache[0] != cur) {
                        g_render.tex_cache[0] = cur;
                        g_effect->SetTexture("diffuse_texture", cur);
                    }
                }
            } else {
                mesh_part* part = (mesh_part*)item->res;
                if (AT(mesh_part*, this, 0x4183d0) != part) {
                    AT(mesh_part*, this, 0x4183d0) = part;
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
                    texres* t;
                    t = part->diffuse;
                    if (t) {
                        IDirect3DBaseTexture9* cur = t->current(it->m->f48);
                        if (g_render.tex_cache[0] != cur) {
                            g_render.tex_cache[0] = cur;
                            g_effect->SetTexture("diffuse_texture", cur);
                        }
                    }
                    t = part->diffuse2;
                    if (t) {
                        IDirect3DBaseTexture9* cur = t->current(it->m->f48);
                        if (g_render.tex_cache[1] != cur) {
                            g_render.tex_cache[1] = cur;
                            g_effect->SetTexture("diffuse_texture_2", cur);
                        }
                    }
                    t = part->normal;
                    if (t) {
                        IDirect3DBaseTexture9* cur = t->current(it->m->f48);
                        if (g_render.tex_cache[2] != cur) {
                            g_render.tex_cache[2] = cur;
                            g_effect->SetTexture("normal_texture", cur);
                        }
                    }
                    t = part->specular;
                    if (t) {
                        IDirect3DBaseTexture9* cur = t->current(it->m->f48);
                        if (g_render.tex_cache[3] != cur) {
                            g_render.tex_cache[3] = cur;
                            g_effect->SetTexture("specular_texture", cur);
                        }
                    }
                    t = part->env;
                    if (t) {
                        IDirect3DBaseTexture9* cur = t->current(it->m->f48);
                        if (g_render.tex_cache[4] != cur) {
                            g_render.tex_cache[4] = cur;
                            g_effect->SetTexture("env_texture", cur);
                        }
                    }
                    if (part->mat->flags & 0x100) {
                        t = sc->env;
                        if (t) {
                            IDirect3DBaseTexture9* cur = t->current(0.0f);
                            if (g_render.tex_cache[4] != cur) {
                                g_render.tex_cache[4] = cur;
                                g_effect->SetTexture("env_texture", cur);
                            }
                        } else {
                            tex_ok = false;
                        }
                    }
                }
                mat = part->mat;
                int nsel = 0;
                if (sc->b160 && !(mat->flags & 0x80)) {
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
                if ((mat->flags & 0x1000) && g_x5aa084 != 2) {
                    v = 3;
                } else if (sun_shadow) {
                    if (sc->sky && sc->sky->sun_shadows && g_x5aa034) {
                        v = (g_render.b614 != 0) + 1;
                    }
                }
                D3DXHANDLE tech = mat->techs->tech[v];
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
                    mesh_part* ip = (mesh_part*)im->res;
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
                if ((((mesh_part*)it->m->res)->mat->flags & 0x100) && sc->water) {
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
            if (AT(material*, this, 0x4183c8)->flags & 0x80000000) {
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
                unsigned tg = ((mesh_part*)it->m->res)->mat->texgen & 0xff;
                if (tg == 0x30 || tg == 0x20)
                    g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)16, (D3DMATRIX*)&a);
            }
            g_effect->CommitChanges();
            {
                std::vector<face20>& faces = src->local_a0 ? src->v90 : src->parent->get90_();
                int nface3 = (int)faces.size() * 3;
                mesh* im = it->m;
                std::vector<vertex60>& verts = im->local_a1 ? im->v50 : im->parent->get50_();
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
