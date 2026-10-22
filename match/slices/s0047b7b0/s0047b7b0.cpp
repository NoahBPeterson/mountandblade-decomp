// Slice s0047b7b0: per-mesh vertex lighting / colour pass (0x47b7b0).
// The scene lights every vertex of a mesh, bakes the result into the vertex colours and (for the
// tangent-space material modes) also into the two extra colour words.  Names and layouts are
// provisional; the layouts come from mesh.h (copied from s0044f3c0/s0047b3b0).
#include <math.h>
#include "mesh.h"

namespace rgl {

extern int g_use_shaders;                    // 0x5e28fc: non-zero -> shader colour path
extern float g_gamma;                        // 0x5a9bfc: gamma exponent for the software colour path
unsigned pack_signed_color(const float* c);  // 0x461370 (cdecl)

struct Scene {
    char pad0[0x68];
    vector4 light_tint;                      // 0x68: colour of the ambient/directional light
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);  // 0x43ac40
    void light_mesh(mesh* m, const mat4* frame, unsigned flags);   // 0x47b7b0
};

// @ 0x47b7b0
void Scene::light_mesh(mesh* m, const mat4* frame, unsigned flags)
{
    m->ensure_local();
    mat4 lm = (*frame) * (*(mat4*)((char*)m + 0xcc));

    mat_t* mat = m->res->mat;
    unsigned mode = mat->f6c & 0x1f;
    bool flat = (mode == 0x18) && (mat->f20 & 0x40) != 0;
    bool tangent = mode == 0x18;

    vector4 tint = light_tint;

    // Flat-lit resources: no per-vertex work, just combine the scene light colour with the tint.
    if (m->res->f1c & 0x40) {
        vector4 dir, col;
        light_at(lm.o, lm.u, &dir, &col, flags, 1.0f);
        vector4 c;
        c.x = col.x * 0.7f + tint.x;
        c.y = col.y * 0.7f + tint.y;
        c.z = col.z * 0.7f + tint.z;
        c.w = 1.0f;
        if (!g_use_shaders) {
            c.x = (float)pow(c.x, g_gamma);
            c.y = (float)pow(c.y, g_gamma);
            c.z = (float)pow(c.z, g_gamma);
        }
        m->multiply_color(&c);
        return;
    }

    // The base colour of the mesh itself (only used by the software colour path).
    if (flat && !g_use_shaders) {
        vector4 c = tint;
        c.x = (float)pow(c.x, g_gamma);
        c.y = (float)pow(c.y, g_gamma);
        c.z = (float)pow(c.z, g_gamma);
        m->color = pack_color(&c) | 0xff000000;
    }

    int count = (int)m->v50.size();
    if (count <= 0)
        return;

    for (int i = 0; i < count; i++) {
        vertex60* v = &vat(m->v50, i);

        float alpha = 1.0f;
        if (flat)
            alpha = (float)(v->color & 0xff) * (1.0f / 255.0f);

        point20* p = &vat(m->v70, v->index);
        vector4 pos, nrm;
        pos.x = lm.s.x * p->a.x + lm.f.x * p->a.y + lm.u.x * p->a.z + lm.o.x;
        pos.y = lm.s.y * p->a.x + lm.f.y * p->a.y + lm.u.y * p->a.z + lm.o.y;
        pos.z = lm.s.z * p->a.x + lm.f.z * p->a.y + lm.u.z * p->a.z + lm.o.z;
        nrm.x = lm.s.x * v->pos.x + lm.f.x * v->pos.y + lm.u.x * v->pos.z;
        nrm.y = lm.s.y * v->pos.x + lm.f.y * v->pos.y + lm.u.y * v->pos.z;
        nrm.z = lm.s.z * v->pos.x + lm.f.z * v->pos.y + lm.u.z * v->pos.z;

        vector4 dir, col, dir2, col2;
        if (!g_use_shaders) {
            light_at(pos, nrm, &dir, &col, flags, alpha);
        } else {
            light_at(pos, nrm, &dir, &col, flags | 2, 1.0f);
            light_at(pos, nrm, &dir2, &col2, flags | 4, 1.0f);
        }

        if (tangent || flat) {
            vector4 vn, vb, vt;
            vn.x = lm.s.x * v->pos.x + lm.f.x * v->pos.y + lm.u.x * v->pos.z;
            vn.y = lm.s.y * v->pos.x + lm.f.y * v->pos.y + lm.u.y * v->pos.z;
            vn.z = lm.s.z * v->pos.x + lm.f.z * v->pos.y + lm.u.z * v->pos.z;
            vb.x = lm.s.x * v->f10.x + lm.f.x * v->f10.y + lm.u.x * v->f10.z;
            vb.y = lm.s.y * v->f10.x + lm.f.y * v->f10.y + lm.u.y * v->f10.z;
            vb.z = lm.s.z * v->f10.x + lm.f.z * v->f10.y + lm.u.z * v->f10.z;
            vt.x = lm.s.x * v->f20.x + lm.f.x * v->f20.y + lm.u.x * v->f20.z;
            vt.y = lm.s.y * v->f20.x + lm.f.y * v->f20.y + lm.u.y * v->f20.z;
            vt.z = lm.s.z * v->f20.x + lm.f.z * v->f20.y + lm.u.z * v->f20.z;

            float k = (col.x + col.y + col.z) * 0.33f;
            vector4 c;
            c.x = (vb.x * -dir.x + vb.y * -dir.y + vb.z * -dir.z) * k;
            c.y = (vn.x * -dir.x + vn.y * -dir.y + vn.z * -dir.z) * k;
            c.z = -(vt.x * -dir.x + vt.y * -dir.y + vt.z * -dir.z) * k;
            c.w = 1.0f;
            unsigned lit = pack_signed_color((const float*)&c);
            if (flat)
                v->f58 = v->f58 ^ ((v->f58 ^ lit) & 0xffffff);
            else
                v->f5c = v->f5c ^ ((v->f5c ^ lit) & 0xffffff);

            if (g_use_shaders && flat) {
                float k2 = (col2.x + col2.y + col2.z) * 0.33f;
                c.x = (vb.x * -dir2.x + vb.y * -dir2.y + vb.z * -dir2.z) * k2;
                c.y = (vn.x * -dir2.x + vn.y * -dir2.y + vn.z * -dir2.z) * k2;
                c.z = -(vt.x * -dir2.x + vt.y * -dir2.y + vt.z * -dir2.z) * k2;
                v->f5c = pack_signed_color((const float*)&c) & 0xffffff;
            }

        } else if (!g_use_shaders) {
            col.x += tint.x;
            col.y += tint.y;
            col.z += tint.z;
        }

        if (!g_use_shaders) {
            col.x = (float)pow(col.x, g_gamma);
            col.y = (float)pow(col.y, g_gamma);
            col.z = (float)pow(col.z, g_gamma);
        }

        vector4 vc;
        unpack_color(&vc, &v->color);
        vc.x *= col.x;
        vc.y *= col.y;
        vc.z *= col.z;
        unsigned lit = pack_color(&vc);
        if (!g_use_shaders || tangent) {
            if (flat || !tangent)
                v->color = v->color ^ ((v->color ^ lit) & 0xffffff);
        } else {
            v->f5c = lit & 0xffffff;
        }
    }
}

} // namespace rgl
