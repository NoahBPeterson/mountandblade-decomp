//-----------------------------------------------------------------------------
// File: strat_entity.cpp  (0x42a090..0x42b6d0)
//
// Desc: the strategic entity (StratEnt: the party/town icons on the campaign map): per-frame render, ground-shadow
//       update (ground-face cache swept with a capsule, the shadow decal meshes, the shadow-map render target),
//       and the scene-world (Ctx) helpers it uses: first directional light, lighting sampler, grid box query.
//       Functions in ascending address order. The next object (strat_ctx.cpp) starts at Ctx::ambient 0x42b6d0.
//       Names provisional.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include "./strat_ext.h"       // (listed directly: the resolution audit scans only the source's own includes)
#include "./strat_entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../cell.h"

// @ 0x42a090
void StratEnt::render(scene* sc, render_info* out)
{
    if (f140) {
        if (fx)
            fx->draw(sc, &pos, &f144);
        MeshEnt::render(sc, out);
        if ((sc->mask & 1) && (mode & 3) && g_shadows_on) {
            Light* l = ctx->first_dir_light();
            if (l && !(l->flags & 2)) {
                bool a = f160 * f294 > 0.05f;
                bool b = f164 * f294 > 0.05f;
                if (a || b)
                    update_shadows(sc, a, b);
            }
        }
        for (Psys** p = emitters_first; p != emitters_last; ++p)
            (*p)->update(sc);
        f31c++;
    }
}

// @ 0x42a1a0
Light* Ctx::first_dir_light()
{
    Light* found = 0;
    unsigned i = 0;
    while (i < lights.size()) {
        if (lights[i]->type == 2 && !(lights[i]->flags & 1))
            found = lights[i];
        i++;
        if (found)
            break;
    }
    return found;
}

// @ 0x42a1f0
void StratEnt::update_shadows(scene* sc, bool a, bool b)
{
    float dx = pos.o.x - last_pos.x;
    float dy = pos.o.y - last_pos.y;
    float dz = pos.o.z - last_pos.z;
    float dist = (float)sqrt(dx * dx + (dz * dz + dy * dy));
    if (dist > radius * 0.2f) {
        update_ground_cache();
        last_pos = pos.o;
    }
    if ((float)(g_time[f2a0] - t2a8) * 1e-05f >= -0.0001f) {
        if (a)
            create_shadow_mesh(&shadow_a, 1);
        if (b) {
            create_shadow_mesh(&shadow_b, 0);
            tstring name("shadow_default");
            shadow_b->res = g_resmgr.lookup(name);
        }
    }
    if (a) {
        if (mode & 2) {
            if ((float)(g_time[f2b0] - t2b8) * 1e-05f >= -0.0001f) {
                float k = -1.0f / (f15c * 25.0f);
                t2b8 = g_time[f2b0] - (int)((k - (float)(rand() % 0x3dc9) * 6.3223115e-07f) * 100000.0f);
                shadow_tick();
                if (p288 != 0) {
                    shadow_a->res = (res_t*)p288;
                } else {
                    tstring name("shadow_default");
                    shadow_a->res = g_resmgr.lookup(name);
                }
            }
        }
        shadow_a->render(sc, rgl::identity());
        f170 = 1;
    }
    if (b) {
        shadow_b->render(sc, rgl::identity());
    }
}

// @ 0x42a480
void StratEnt::update_ground_cache()
{
    matrix m;
    vector4 amb;
    build_shadow_frame(&m, &amb);
    float reach = radius * 3.0f;
    capsule cap((capsule::outline_init()));
    cap.p0 = tp(pos, off);
    vector4 t(m.u.x * reach, m.u.y * reach, m.u.z * reach);
    cap.p1 = vector4(cap.p0.x - t.x, cap.p0.y - t.y, cap.p0.z - t.z);
    cap.radius = radius * 1.1f;
    vector4 tmp;
    const vector4* mn = cap.get_min(&tmp);
    vector4 bmin(mn->x - 0.1f, mn->y - 0.1f, mn->z - 0.1f);
    const vector4* mx = cap.get_max(&tmp);
    vector4 bmax(mx->x + 0.1f, mx->y + 0.1f, mx->z + 0.1f);
    shape_base* found[0x2000];
    int nfound;
    ctx->query_box(&bmin.x, &bmax.x, 0x104, found, &nfound);
    nids = 0;
    for (int i = 0; i < nfound; i++) {
        if (nids >= 0x40)
            return;
        int id = *(int*)((char*)found[i] + 4);
        if (id < 0)
            continue;
        GFace* face = &ctx->ground->faces[id];
        vector4 nu(-m.u.x, -m.u.y, -m.u.z);
        const float* nn = face->normal;
        if (!(nu.x * nn[0] + nu.y * nn[1] + nu.z * nn[2] < 0.0f))
            continue;
        if (shape_contact_count(&cap, found[i], 0, 0) == 0)
            continue;
        face = &ctx->ground->faces[*(int*)((char*)found[i] + 4)];
        vector4 c[4];
        for (int k = 0; k < face->n; k++) {
            GVert* gv = &ctx->ground->verts[face->idx[k]];
            vector4 v = gv->p;
            c[k] = m.transform_point_to_local(v);
            float inv = 0.5f / radius;
            c[k].x = inv * c[k].x;
            c[k].y = inv * c[k].y;
            c[k].z = inv * c[k].z;
        }
        if ((c[0].z <= -0.5f || c[1].z <= -0.5f || c[2].z <= -0.5f || (face->n == 4 && c[3].z <= -0.5f)) &&
            (c[0].z - c[0].x <= -0.5f || c[1].z - c[1].x <= -0.5f || c[2].z - c[2].x <= -0.5f || (face->n == 4 && c[3].z - c[3].x <= -0.5f)) &&
            (c[0].x + c[0].z <= -0.5f || c[1].x + c[1].z <= -0.5f || c[2].x + c[2].z <= -0.5f || (face->n == 4 && c[3].x + c[3].z <= -0.5f)) &&
            (c[0].z - c[0].y <= -0.5f || c[1].z - c[1].y <= -0.5f || c[2].z - c[2].y <= -0.5f || (face->n == 4 && c[3].z - c[3].y <= -0.5f)) &&
            (c[0].y + c[0].z <= -0.5f || c[1].y + c[1].z <= -0.5f || c[2].y + c[2].z <= -0.5f || (face->n == 4 && c[3].y + c[3].z <= -0.5f))) {
            ids[nids] = *(int*)((char*)found[i] + 4);
            nids++;
        }
    }
}

// @ 0x42aa50
void Ctx::query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count)
{
    int lo[2], hi[2];
    locate(bmin, lo);
    locate(bmax, hi);
    *count = 0;
    for (int i = lo[0]; i <= hi[0]; i++) {
        if (lo[1] <= hi[1]) {
            int flat = 1;
            for (int j = lo[1]; j <= hi[1]; j++)
                cells[i][j].collect(bmin, bmax, &flat, mask, out, count);
        }
    }
}

static inline vector4 crossv(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// @ 0x42aaf0
void StratEnt::build_shadow_frame(matrix* m, vector4* amb)
{
    vector4* o = &m->o;
    *o = pos.o;
    *o = tp(pos, off);
    vector4 d;
    ctx->sample_lights(&pos.o, &d, amb);
    if (d.z > -0.1f) d.z = -0.1f;
    vector4* u = &m->u;
    vector4* f = &m->f;
    *u = vector4(-d.x, -d.y, -d.z);
    *f = vector4(0.0f, 0.0f, 1.0f);
    m->s = crossv(*f, *u);
    m->s.normalize();
    *f = crossv(*u, m->s);
    f->normalize();
    vector4 t;
    t.x = radius * u->x;
    t.y = u->y * radius;
    t.z = u->z * radius;
    o->x = t.x + o->x;
    o->y = t.y + o->y;
    o->z = t.z + o->z;
}

static __forceinline vector4 vsubv(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vscl(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 vmulr(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline float nrm(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv; v.y = v.y * inv; v.z = v.z * inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}

// @ 0x42ad60
void Ctx::sample_lights(const vector4* p, vector4* dir, vector4* amb)
{
    *dir = vector4(0.0f, 0.0f, 0.0f);
    *amb = vector4(0.0f, 0.0f, 0.0f);
    for (unsigned i = 0; i < lights.size(); i++) {
        Light* l = lights[i];
        vector4 d, c;
        if (l->type == 2 && !(l->flags & 1)) {
            d = l->dir;
            c = l->color;
        } else {
            d = vector4(p->x - l->pos.x, p->y - l->pos.y, p->z - l->pos.z);
            float len = nrm(d);
            float k = 1.0f / (len * len);
            c = vector4(k * l->color.x, k * l->color.y, k * l->color.z);
        }
        float s = c.x + c.y + c.z;
        vector4 w = vscl(s, vscl(0.33f, d));
        dir->x = w.x + dir->x;
        dir->y = w.y + dir->y;
        dir->z = w.z + dir->z;
        amb->x = c.x + amb->x;
        amb->y = c.y + amb->y;
        amb->z = c.z + amb->z;
    }
    float len = dir->length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        dir->x = inv * dir->x;
        dir->y = inv * dir->y;
        dir->z = inv * dir->z;
    } else {
        dir->x = 0.0f; dir->y = 1.0f; dir->z = 0.0f;
    }
    if (amb->x > 0.95f) amb->x = 0.95f;
    if (amb->y > 0.95f) amb->y = 0.95f;
    if (amb->z > 0.95f) amb->z = 0.95f;
}

// @ 0x42b080
// Builds (or refills) the shadow decal mesh from the ground faces under the entity: one vertex per
// ground vertex lifted along its normal, with texture coordinates projected on the shadow camera and an
// alpha that falls off with the distance from the entity; 1..3 sample vertices per ground vertex.
void StratEnt::create_shadow_mesh(mesh** slot, char flag)
{
    int vtx[3][4];
    vector4 amb;
    int pt[4];
    matrix fr;
    float ir;
    int nt;
    float ik;
    int j;
    int pi;
    vector4 p;
    int i;
    vec2 off[3];
    float s0;
    unsigned col;
    float u, v;
    vec2 uv;
    int f5c;
    int q;
    GFace* face;
    const vector4* n;
    float k, dx, dy, dz, shade, c, dx2, dy2, dz2, a, t;
    int al;
    build_shadow_frame(&fr, &amb);
    if (*slot == 0) {
        *slot = new mesh;
        {
            tstring name("shadow_default");
            (*slot)->res = g_resmgr.lookup(name);
        }
        (*slot)->slots[1].f4 = 1;
        (*slot)->slots[0].f4 = 1;
    }
    (*slot)->clear_all();
    vector4 a2 = ctx->ambient();
    nt = 1;
    s0 = ((a2.z + a2.y) + a2.x) * 0.35f;
    if (f15c > 0.5f) {
        nt = 2;
        if (f15c > 0.7f) nt = 3;
    }
    if (nt == 1) {
        off[0] = vec2c(0.0f, 0.0f);
    } else if (nt == 2) {
        off[0] = vec2c(-0.004296875f, 0.0046875f);
        off[1] = vec2c(0.00390625f, -0.004296875f);
    } else {
        off[0] = vec2c(0.00078125f, 0.00546875f);
        off[1] = vec2c(-0.005078125f, -0.00546875f);
        off[2] = vec2c(0.00546875f, -0.005078125f);
    }
    for (i = 0; i < nids; i++) {
        face = &ctx->ground->faces[ids[i]];
        for (j = 0; j < face->n; j++) {
            k = radius * 0.03f;
            p = ctx->ground->verts[face->idx[j]].p;
            n = &ctx->ground->verts[face->idx[j]].n;
            dy = k * n->y;
            dz = k * n->z;
            p.x = k * n->x + p.x;
            p.y = p.y + dy;
            p.z = p.z + dz;
            pi = (*slot)->add_point(&p);
            pt[j] = pi;
            ir = 1.0f / radius;
            ik = ir * 0.5f;
            u = (fr.s.x * (p.x - fr.o.x) + (fr.s.y * (p.y - fr.o.y) + fr.s.z * (p.z - fr.o.z))) * ik;
            v = (fr.f.y * (p.y - fr.o.y) + (fr.f.x * (p.x - fr.o.x) + fr.f.z * (p.z - fr.o.z))) * ik;
            if (!flag) {
                u = u * 1.5f;
                v = v * 1.5f;
            }
            u = u + 0.5f;
            v = v + 0.5f;
            shade = ctx->ground->verts[face->idx[j]].shade;
            c = shade / (s0 + shade);
            if (c < 0.0f) c = 0.0f;
            else if (c > 0.75f) c = 0.75f;
            if (nt == 2) {
                c = 1.0f - (float)sqrt(1.0f - c);
            } else if (nt == 3) {
                c = (3.0f - (float)sqrt(9.0f - (c - 0.01f) * 12.0f)) * 0.125f;
            }
            dx2 = p.x - pos.o.x;
            dy2 = p.y - pos.o.y;
            dz2 = p.z - pos.o.z;
            a = (float)sqrt(dx2 * dx2 + dy2 * dy2 + dz2 * dz2) * ir * 1.3333334f - 3.8f;
            if (a < 0.2f) a = 0.2f;
            else if (a >= 1.0f) a = 1.0f;
            t = c * (1.0f - a) * f294;
            if (flag)
                t = t * f160;
            else
                t = t * f164 * 0.9f;
            al = (int)(t * 256.0f);
            if (al < 0) al = 0;
            else if (al >= 0x100) al = 0xff;
            col = (al << 24) | 0xffffff;
            for (q = 0; q < nt; q++) {
                uv = vec2c(u + off[q].x, v + off[q].y);
                f5c = 0;
                vtx[q][j] = (*slot)->add_vertex(pi, &uv, &col, n, &f5c);
            }
        }
        for (q = 0; q < nt; q++) {
            (*slot)->add_face(vtx[q][1], vtx[q][2], vtx[q][0]);
            if (face->n > 3)
                (*slot)->add_face(vtx[q][0], vtx[q][2], vtx[q][3]);
        }
    }
    (*slot)->slots[0].f28 = 1;
    (*slot)->slots[1].f28 = 1;
    (*slot)->color = 0xff000915;
}
