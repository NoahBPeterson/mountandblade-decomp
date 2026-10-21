// Slice s004291c0: skeleton skinning of a particle-system mesh (FxDef::skin), the transposed rotation helper, the
// group skinning loop and the render scene constructor.
#include "world.h"

// @ 0x429670
mat3* mat4::rot_t(mat3* out) const
{
    mat3 t;
    t.s.x = s.x; t.s.y = f.x; t.s.z = u.x;
    t.f.x = s.y; t.f.y = f.y; t.f.z = u.y;
    t.u.x = s.z; t.u.y = f.z; t.u.z = u.z;
    *out = t;
    return out;
}

// @ 0x4296d0
skin_iter skin_iter::operator+(int n) const
{
    skin_iter t = *this;
    t.cur += n;
    return t;
}

// @ 0x4296f0
void FxDef::skin_group(lod_mesh2* g)
{
    if (g->cur < g->count) {
        for (mesh** it = g->lists[g->cur].v._Myfirst; it < g->lists[g->cur].v._Mylast; it++)
            skin((Psys*)*it, 0);
    }
}

// @ 0x429740
scene::scene()
    : f0(0), f4(0), f8(0), fc(0), f10(0), f14(1.0f), f18(0), f1c(0), f20(0), f24(0), f28(0), f2c(1.0f)
{
    vector4* a = &ambient;
    *a = vector4(1.0f, 1.0f, 1.0f);
    mask = 1;
    f154 = 0;
    f158 = 0;
    f161 = 0;
    f16c = 0;
    f170 = 0;
    f174 = 0;
    f178 = 0;
    f160 = 0;
    f162 = 0;
    f164 = 0;
    f190 = 0;
    ambient.w = 1.0f;
    f15c = 0;
    f1c4 = 0;
    f18c = 0;
    f168 = 0;
    f1b0 = 0;
    f1a5 = 0;
    f169 = 0;
    f16a = 0;
    f1ac = 1;
    reset();
}

static __forceinline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }

// @ 0x4291c0
void FxDef::skin(Psys* P, char flag)
{
    P->ensure_skin();
    if (P->bone_mats == 0)
        P->bone_mats = new mat4[32];
    mat4* B = P->bone_mats;
    for (int k = 0; k < P->nbones; k++) {
        mat3 tmp1, tmp2;
        mat3* t = elems[k].m4.rot_t(&tmp1);
        *(mat3*)&B[k] = ((pos*)&elems[k].m84)->rot3(*(rgl::matrix*)t);
        B[k].o = elems[k].m84.o;
        B[k].s.w = 0.0f;
        B[k].f.w = 0.0f;
        B[k].u.w = 0.0f;
        B[k].o.w = 1.0f;
    }
    P->bone_src = (const char*)res + 0x20;
    if (P->res->mat->f20 >= 0) {
        P->ensure_local();
        std::vector<elem30>& sk = P->get60();
        for (unsigned i = 0; i < P->v50.size(); i++) {
            vertex60* V = &*(P->v50.begin() + i);
            point20* Pt = &P->v70._Myfirst[V->index];
            skin_iter base((SkinVert*)sk._Myfirst);
            SkinVert* sv = (base + i).cur;
            vector4* acc = &Pt->b;
            *acc = vector4(0.0f, 0.0f, 0.0f);
            vector4* nacc = &V->nrm0;
            *nacc = vector4(0.0f, 0.0f, 0.0f);
            const int* pb = sv->bone;
            int cnt = 4;
            do {
                float w = ((const float*)pb)[-4];
                vector4 o = elems[*pb].m4.o;
                float dx = Pt->a.x - o.x;
                float dy = Pt->a.y - o.y;
                float dz = Pt->a.z - o.z;
                const mat4* M = &B[*pb];
                vector4 v(dx * M->s.x + (dy * M->f.x + dz * M->u.x) + M->o.x,
                          dx * M->s.y + (dy * M->f.y + dz * M->u.y) + M->o.y,
                          dx * M->s.z + (dy * M->f.z + dz * M->u.z) + M->o.z);
                vector4 sc = vscale(w, v);
                acc->x += sc.x;
                acc->y += sc.y;
                acc->z += sc.z;
                M = &B[*pb];
                vector4 n(M->s.x * V->nrm.x + (M->f.x * V->nrm.y + M->u.x * V->nrm.z),
                          M->s.y * V->nrm.x + (M->f.y * V->nrm.y + M->u.y * V->nrm.z),
                          M->s.z * V->nrm.x + (M->f.z * V->nrm.y + M->u.z * V->nrm.z));
                vector4 sn = vscale(w, n);
                nacc->x += sn.x;
                nacc->y += sn.y;
                nacc->z += sn.z;
                pb++;
                cnt--;
            } while (cnt != 0);
        }
        P->res->flags |= 0x800;
        if (!(P->res->flags & 0x800)) {
            unsigned i = 0;
            while (i < P->v50.size()) {
                vector4* n = &((vertex60&)P->v50[i]).nrm0;
                float len = n->length();
                if (len > 1e-10f) {
                    float inv = 1.0f / len;
                    i++;
                    n->x = inv * n->x;
                    n->y = inv * n->y;
                    n->z = inv * n->z;
                } else {
                    n->x = 0.0f;
                    n->y = 1.0f;
                    n->z = 0.0f;
                    i++;
                }
            }
        }
        P->f138 = 1;
        P->f114 = (flag == 0);
    }
}
