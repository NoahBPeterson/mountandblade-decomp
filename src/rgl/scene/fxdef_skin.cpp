// fxdef_skin.cpp (0x4291c0..0x429670): FxDef::skin, the skinning of a particle-system mesh by the skeleton: bone
// matrices from the element frames, then every vertex's position and normal blended over its four bones. Migrated
// from slice s004291c0 (0x4291c0).
// Boundary: a single function. FxDef::render (0x428ff0) calls it forward (high) and it calls mat4::rot_t 0x429670
// and skin_iter::operator+ 0x4296d0 forward (high).
#include <vector>
#include <new>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../resource.h"
#include "./scene_math.h"
#include "./frustum.h"
#include "./scene.h"
#include "./emitter.h"
#include "./scene_entity.h"
#include "./world.h"
#include "./fx.h"

using namespace rgl;

static __forceinline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }

// @ 0x4291c0
void FxDef::skin(mesh* mm, char flag)
{
    mesh_ext* P = (mesh_ext*)mm;
    P->ensure_skin();
    if (P->bone_mats() == 0)
        P->bone_mats() = new mat4[32];
    mat4* B = P->bone_mats();
    for (int k = 0; k < P->nbones(); k++) {
        mat3 tmp1, tmp2;
        mat3* t = elems[k].m4.rot_t(&tmp1);
        *(mat3*)&B[k] = elems[k].m84.rot3(*(matrix*)t);
        B[k].o = elems[k].m84.o;
        B[k].s.w = 0.0f;
        B[k].f.w = 0.0f;
        B[k].u.w = 0.0f;
        B[k].o.w = 1.0f;
    }
    P->bone_src() = (const char*)res + 0x20;
    if ((int)P->res->shader->flags20 >= 0) {
        P->ensure_local();
        std::vector<elem30>& sk = P->get60();
        for (unsigned i = 0; i < P->v50.size(); i++) {
            vertex60* V = &*(P->v50.begin() + i);
            point20* Pt = &P->v70._Myfirst[V->index];
            skin_iter base((SkinVert*)sk._Myfirst);
            SkinVert* sv = (base + i).cur;
            vector4* acc = &Pt->b;
            *acc = vector4(0.0f, 0.0f, 0.0f);
            vector4* nacc = &V->normal;
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
                vector4 n(M->s.x * V->pos.x + (M->f.x * V->pos.y + M->u.x * V->pos.z),
                          M->s.y * V->pos.x + (M->f.y * V->pos.y + M->u.y * V->pos.z),
                          M->s.z * V->pos.x + (M->f.z * V->pos.y + M->u.z * V->pos.z));
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
                vector4* n = &((vertex60&)P->v50[i]).normal;
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
        P->slots[0].f28 = 1;
        P->slots[0].f4 = (flag == 0);
    }
}
