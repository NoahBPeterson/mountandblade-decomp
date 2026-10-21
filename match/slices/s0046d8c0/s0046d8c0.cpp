// Slice s0046d8c0: scene renderer pass that draws a list of mesh instances with the D3D9 fixed-function
// pipeline (shadow/texture-matrix variant: world transform built from a camera-relative frame, per-mesh
// vertex declaration / stream / index buffer binding, light enable by distance, material setup, and
// DrawIndexedPrimitive). Names are provisional. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include "mesh.h"

using rgl::vector4;
using rgl::mat4;
using rgl::mesh;

// ---- globals ---------------------------------------------------------------------------------------
struct renderer {
    void set_blend_mode(int mode);                  // 0x40e940
    void set_stage_color(int stage, unsigned f);    // 0x461bd0
    void set_stage_alpha(int stage, unsigned f);    // 0x461c60
};
extern renderer g_render;                // 0xa17910
extern IDirect3DDevice9* g_dev;          // 0xa17f28
extern unsigned g_stage_arg[8];          // 0x5a9c00 D3DTA_* by 3-bit code

extern char g_lighting;                  // 0xa17e16 cached D3DRS_LIGHTING
extern char g_b15;                       // 0xa17e15 cached D3DRS_FOGENABLE
extern char g_light_on[64];              // 0xa17e17 per-light enable cache
extern char g_b57;                       // 0xa17e57 cached D3DRS_SPECULARENABLE
extern char g_b58;                       // 0xa17e58 cached D3DRS_NORMALIZENORMALS
extern char g_zwrite;                    // 0xa17e59 cached D3DRS_ZWRITEENABLE
extern char g_zfunc_always;              // 0xa17e5b
extern int g_e68;                        // 0xa17e68 pixel shader bound
extern int g_tss_a;                      // 0xa17e6c
extern int g_tss_b;                      // 0xa17e70
extern int g_tss_c;                      // 0xa17e78
extern int g_tss_d;                      // 0xa17e7c
extern int g_alpha_test;                 // 0xa17ea0
extern int g_texfactor;                  // 0xa17ea4
extern int g_cull_base;                  // 0xa17ecc
extern int g_cull_cur;                   // 0xa17ed0

// ---- scene (the second argument) ---------------------------------------------------------------------
struct light_t {
    int type;                            // 0: point, 2: always on
    char pad4[0x24 - 4];
    vector4 pos;                         // 0x24
    char pad34[0x54 - 0x34];
    float range;                         // 0x54
};
struct flag_obj { char pad[0x10]; char on; };
struct rot3 { vector4 s, f, u; };
struct entry { mesh* m; int extra; };    // 8 bytes
struct view_t {
    char pad0[0xf8];
    rot3 rot;                            // 0xf8
    vector4 org;                         // 0x128
    char pad138[0x160 - 0x138];
    char b160;                           // lights enabled
    char pad161;
    char b162;                           // pass kind
    char pad163;
    int f164;                            // pass kind
    char pad168[0x194 - 0x168];
    std::vector<light_t*> lights;        // 0x194
    char pad1a4[0x1b0 - 0x1a4];
    flag_obj* p1b0;
    std::vector<entry> ents;             // 0x1b4
};

// ---- the draw object (this) ---------------------------------------------------------------------------
struct vb_slot {                         // 0x4c bytes
    char pad0[4];
    UINT stride;                         // 4
    IDirect3DVertexBuffer9* vb;          // 8
    char padc[0x48 - 0xc];
    IDirect3DVertexDeclaration9* decl;   // 0x48
};
struct drawer {
    char pad0[0x14];
    vb_slot vbs[10];                     // 0x14
    char pad30c[0x3b4 - 0x30c];
    int cur_a;                           // 0x3b4
    int cur_ib;                          // 0x3b8
    int cur_b;                           // 0x3bc
    char pad3c0[0x4183c8 - 0x3c0];
    rgl::mat_t* cur_mat;                 // 0x4183c8
    int f4183cc;
    rgl::res_t* cur_part;                // 0x4183d0
    IDirect3DIndexBuffer9* ib(int i) { return *(IDirect3DIndexBuffer9**)((char*)this + 0x350 + i * 0x4c); }
    void draw(view_t* v);                // 0x46d8c0
};

static __forceinline vector4 sub_i(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.z * b.y - b.z * a.y, b.z * a.x - a.z * b.x, a.y * b.x - b.y * a.x);
}

// @ 0x46d8c0
void drawer::draw(view_t* v)
{
    if (v->b162 || v->f164) {
        if (g_lighting) {
            g_lighting = 0;
            g_dev->SetRenderState(D3DRS_LIGHTING, 0);
        }
        if (g_alpha_test)
            g_dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        g_alpha_test = 0;
        g_render.set_blend_mode(2);
        if (v->f164) {
            if (g_zfunc_always) {
                g_zfunc_always = 0;
                g_dev->SetRenderState(D3DRS_ZFUNC, 8);
            }
            if (g_zwrite != 1) {
                g_zwrite = 1;
                g_dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            g_render.set_stage_alpha(0, 0x4a4);
        } else {
            if (g_zfunc_always != 1) {
                g_zfunc_always = 1;
                g_dev->SetRenderState(D3DRS_ZFUNC, 4);
            }
            if (g_zwrite) {
                g_zwrite = 0;
                g_dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
            }
            g_render.set_stage_alpha(0, 0xa2);
        }
        if (g_b58) {
            g_dev->SetRenderState(D3DRS_NORMALIZENORMALS, 0);
            g_b58 = 0;
        }
        if (g_b57) {
            g_dev->SetRenderState(D3DRS_SPECULARENABLE, 0);
            g_b57 = 0;
        }
        g_render.set_stage_color(0, 0xa2);
        g_render.set_stage_color(1, 2);
        g_render.set_stage_alpha(1, 2);
        g_render.set_stage_color(2, 1);
        g_render.set_stage_alpha(2, 1);
    }

    mat4 M;
    (rot3&)M = v->rot;
    M.s.x *= 0.5f; M.s.y *= 0.5f; M.s.z *= 0.5f;
    vector4 half(0.5f, 0.5f, 0.5f);
    M.o = half;
    M.f.x *= 0.5f; M.f.y *= 0.5f; M.f.z *= 0.5f;
    M.u.x *= 0.5f; M.u.y *= 0.5f; M.u.z *= 0.5f;
    mat4 A;
    rgl::identity(&A);
    float ang = 1.5707964f;
    A.rotate_fu(&ang);
    M = A * M;
    cur_part = 0;
    cur_mat = 0;
    M.s.w = 0.0f; M.f.w = 0.0f; M.u.w = 0.0f; M.o.w = 1.0f;

    entry* p = v->ents._Myfirst;
    if (p == v->ents._Mylast) return;
    do {
        rgl::res_t* part = p->m->res;
        int b = p->m->slots[0].f4;
        int a = p->m->slots[0].f0;
        if (cur_a != b || cur_b != a) {
            cur_a = b;
            cur_b = a;
            g_dev->SetVertexDeclaration(vbs[b + a * 2].decl);
            g_dev->SetStreamSource(0, vbs[b + a * 2].vb, 0, vbs[b + a * 2].stride);
        }
        mesh* node = p->m->local_a0 ? p->m : p->m->parent;
        int ibi = node->slots[1].f4;
        if (cur_ib != ibi) {
            cur_ib = ibi;
            g_dev->SetIndices(ib(ibi));
        }
        if (!v->b162) {
            if (v->f164) goto L_state;
            if (part != cur_part) {
                cur_part = part;
                part->apply_state((const float*)&p->m->f48);
                if (cur_mat != part->mat) {
                    part->mat->bind_pixel_shader();
                    cur_mat = part->mat;
                }
            }
            if (v->p1b0 && v->p1b0->on) {
                if (part->flags & 1) {
                    if (g_b15) {
                        g_b15 = 0;
                        g_dev->SetRenderState(D3DRS_FOGENABLE, 0);
                    }
                } else {
                    if (g_b15 != 1) {
                        g_b15 = 1;
                        g_dev->SetRenderState(D3DRS_FOGENABLE, 1);
                    }
                }
            }
            if (v->b160 && !(part->mat->f20 & 0x80)) {
                if (g_lighting != 1) {
                    g_lighting = 1;
                    g_dev->SetRenderState(D3DRS_LIGHTING, 1);
                }
                unsigned i = 0;
                light_t** b;
                while ((b = v->lights._Myfirst, b != 0) && i < (unsigned)(v->lights._Mylast - b) && (int)i < 0x40) {
                    light_t** pl = &b[i];
                    if ((*pl)->type == 2) {
                        if (g_light_on[i] != 1) {
                            g_light_on[i] = 1;
                            g_dev->LightEnable(i, 1);
                        }
                        i = i + 1;
                    } else {
                        light_t* l = *pl;
                        vector4 d = sub_i(l->pos, p->m->m194.o);
                        if (d.length() - p->m->f44 < (*pl)->range) {
                            if (g_light_on[i] != 1) {
                                g_light_on[i] = 1;
                                g_dev->LightEnable(i, 1);
                            }
                            i = i + 1;
                        } else {
                            if (g_light_on[i]) {
                                g_light_on[i] = 0;
                                g_dev->LightEnable(i, 0);
                            }
                            i = i + 1;
                        }
                    }
                }
            } else {
                if (g_lighting) {
                    g_lighting = 0;
                    g_dev->SetRenderState(D3DRS_LIGHTING, 0);
                }
            }
            p->m->setup_material();
            unsigned k = part->mat->f74 & 0xff;
            if (k == 0x30)
                g_dev->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
            else if (k == 0x20)
                g_dev->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
        } else {
            if (v->f164) {
L_state:
                if (g_tss_c != 0xa2) {
                    g_tss_c = 0xa2;
                    g_dev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
                    g_dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, g_stage_arg[5]);
                    g_dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_tss_d != 1) {
                    g_tss_d = 1;
                    g_dev->SetTextureStageState(1, D3DTSS_ALPHAOP, 1);
                    g_dev->SetTextureStageState(1, D3DTSS_ALPHAARG1, g_stage_arg[0]);
                    g_dev->SetTextureStageState(1, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_tss_a != 0xa2) {
                    g_tss_a = 0xa2;
                    g_dev->SetTextureStageState(0, D3DTSS_COLOROP, 2);
                    g_dev->SetTextureStageState(0, D3DTSS_COLORARG1, g_stage_arg[5]);
                    g_dev->SetTextureStageState(0, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
                if (g_tss_b != 1) {
                    g_tss_b = 1;
                    g_dev->SetTextureStageState(1, D3DTSS_COLOROP, 1);
                    g_dev->SetTextureStageState(1, D3DTSS_COLORARG1, g_stage_arg[0]);
                    g_dev->SetTextureStageState(1, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
            } else {
                if (!(p->m->mask & 0x8000)) {
                    if (g_zwrite) {
                        g_zwrite = 0;
                        g_dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
                    }
                    part->p48->bind(0, 0.0f);
                    g_render.set_stage_alpha(0, 0xa2);
                    g_render.set_stage_alpha(1, 0xaa);
                } else {
                    if (part->p48) {
                        if (g_zwrite != 1) {
                            g_zwrite = 1;
                            g_dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        part->p48->bind(0, 0.0f);
                        g_render.set_stage_alpha(0, 0x4a4);
                    } else {
                        if (g_zwrite != 1) {
                            g_zwrite = 1;
                            g_dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        g_render.set_stage_alpha(0, 0xa2);
                    }
                    g_render.set_stage_alpha(1, 2);
                }
            }
            if (g_e68) {
                g_dev->SetPixelShader(0);
                g_e68 = 0;
            }
            int col = p->m->color;
            if (g_texfactor != col) {
                g_texfactor = col;
                g_dev->SetRenderState(D3DRS_TEXTUREFACTOR, col);
            }
        }

        int cull = g_cull_base;
        if (p->m->f4c) {
            if (cull == 3) cull = 2;
            else if (cull == 2) cull = 3;
        }
        if (cull != g_cull_cur) {
            g_cull_cur = cull;
            g_dev->SetRenderState(D3DRS_CULLMODE, cull);
        }

        if (p->m->f34 == 0) {
            if (!(part->flags & 0x20000)) {
                A = p->m->m194;
            } else {
                (rot3&)A = *(rot3*)&p->m->m194;
                A.o = v->org;
                A.o.z = A.o.z * 0.75f;
            }
        } else if (p->m->f34 == 1) {
            mat4 B;
            B.o = p->m->m194.o;
            B.u = p->m->m194.u;
            B.s = v->rot.s;
            B.f = cross_i(B.s, B.u);
            B.u.normalize();
            B.s = cross_i(B.u, B.f);
            A = B;
        } else {
            mat4 B;
            B.o = p->m->m194.o;
            B.s = v->rot.s; B.f = v->rot.f; B.u = v->rot.u;
            A = B;
        }
        A.s.w = 0.0f; A.f.w = 0.0f; A.u.w = 0.0f; A.o.w = 1.0f;
        g_dev->SetTransform((D3DTRANSFORMSTATETYPE)0x100, (const D3DMATRIX*)&A);

        std::vector<rgl::face20>& fv = node->local_a0 ? node->v90 : node->parent->get90_();
        int nf = fv.size();
        std::vector<rgl::vertex60>& vv = p->m->local_a1 ? p->m->v50 : p->m->parent->get50_();
        int nv = vv.size();
        g_dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, p->m->slots[0].f8, 0, nv, node->slots[1].f8, (nf * 3) / 3);
        p++;
    } while (p != v->ents._Mylast);
}
