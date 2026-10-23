// Slice s0047c1d0: mesh mirror/seam construction and the two skin-blend helpers plus the ground snap.
// Layouts and names are provisional and come from mesh.h (copied from s0047b7b0).
#include <math.h>
#include "mesh.h"

namespace rgl {

// @ 0x47c1d0
void __stdcall build_mirror(mesh* m, int* remap, int* count, char flip)
{
    if (!m->local_a0) {
        m->v90 = m->parent->v90;
        m->local_a0 = 1;
    }
    m->ensure_local();

    int mapv[65536];
    int map2[65536];

    for (unsigned i = 0; i < m->v70.size(); i++) {
        remap[i] = i;
        map2[i] = -1;
    }

    int n = (int)m->v70.size();
    *count = n;
    for (int i = 0; i < n; i++) {
        point20* p = &vat(m->v70, i);
        if (p->a.x > 0.001f) {
            vector4 q;
            q.x = -p->a.x;
            q.y = p->a.y;
            q.z = p->a.z;
            q.w = p->a.w;
            m->add_point(&q);
            remap[n] = i;
            map2[i] = n;
            n++;
        }
    }

    for (unsigned i = 0; i < m->v50.size(); i++)
        mapv[i] = -1;

    int nv = (int)m->v50.size();
    int nw = nv;
    for (int i = 0; i < nv; i++) {
        vertex60* v = &vat(m->v50, i);
        int pi = v->index;
        int mi = map2[pi];
        if (mi < 0) {
            v->pos.x = 0.0f;
            float len = sqrtf(v->pos.x * v->pos.x + v->pos.y * v->pos.y + v->pos.z * v->pos.z);
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                v->pos.x *= inv;
                v->pos.y *= inv;
                v->pos.z *= inv;
            } else {
                v->pos.x = 0.0f;
                v->pos.y = 1.0f;
                v->pos.z = 0.0f;
            }
            v->normal.x = v->pos.x;
            v->normal.y = v->pos.y;
            v->normal.z = v->pos.z;
            v->normal.w = v->pos.w;
            if (v->uv.x > 0.51f) {
                vertex60 t = *v;
                t.f5c = 0;
                if (flip) {
                    t.uv.x = 1.0f - t.uv.x;
                    t.f48 = 1.0f - t.f48;
                }
                t.index = pi;
                m->v50.push_back(t);
                mapv[i] = nw;
                nw++;
            }
        } else {
            vertex60 t = *v;
            t.pos.x = -t.pos.x;
            t.normal = v->pos;
            t.f5c = 0;
            if (flip) {
                t.uv.x = 1.0f - t.uv.x;
                t.f48 = 1.0f - t.f48;
            }
            t.index = mi;
            m->v50.push_back(t);
            mapv[i] = nw;
            nw++;
        }
    }

    int nf = (int)m->v90.size();
    for (int i = 0; i < nf; i++) {
        face20 f = vat(m->v90, i);
        bool changed = false;
        if (mapv[f.a] >= 0) {
            f.a = mapv[f.a];
            changed = true;
        }
        if (mapv[f.b] >= 0) {
            f.b = mapv[f.b];
            changed = true;
        }
        int c = mapv[f.c];
        if (c >= 0)
            c = mapv[f.c];
        else if (!changed)
            continue;
        else
            c = f.c;
        int oldb = f.b;
        f.b = c;
        f.c = oldb;
        m->v90.push_back(f);
    }

    m->slots[0].f28 = 1;
    m->slots[1].f28 = 1;
}

// @ 0x47c6b0
void skin::blend(mesh* m, int unused, const float* weights)
{
    m->ensure_local();
    for (int i = 0; i < (int)m->v70.size(); i++) {
        point20* p = &vat(m->v70, i);
        for (int j = 0; j < bone_count; j++) {
            bone34* b = &bones[j];
            vector4* t = &b->trans[i];
            if (t->x * t->x + t->y * t->y + t->z * t->z > 1e-10f) {
                float w = (b->w1 - b->w0) * weights[j] + b->w0;
                vector4 d;
                d.x = w * t->x;
                d.y = w * t->y;
                d.z = w * t->z;
                vector4 v;
                v.x = p->a.x + d.x;
                v.y = p->a.y + d.y;
                v.z = p->a.z + d.z;
                p->a = v;
                p->b = v;
            }
        }
    }
}

// @ 0x47c810
void skin::blend_indexed(mesh* m, void* set, int* index, const float* weights)
{
    m->ensure_local();
    boneset* s = (boneset*)set;
    if ((int)m->v70.size() != s->counts[*index])
        return;
    unsigned i = 0;
    while (i < m->v70.size()) {
        point20* p = &vat(m->v70, i);
        for (int j = 0; j < bone_count; j++) {
            int bi = ((int*)s->maps[*index])[i];
            bone34* b = &bones[j];
            vector4* t = &b->trans[bi];
            if (t->x * t->x + t->y * t->y + t->z * t->z > 1e-10f) {
                float w = (b->w1 - b->w0) * weights[j] + b->w0;
                vector4 d;
                d.x = w * t->x;
                if (p->a.x < 0.0f)
                    d.x = -d.x;
                d.y = w * t->y;
                d.z = w * t->z;
                p->a.x += d.x;
                p->a.y += d.y;
                p->a.z += d.z;
                p->a.w = d.w;
                p->b = p->a;
            }
        }
        i++;
    }
}

// @ 0x47c9b0
void ground::snap(mesh* m)
{
    m->ensure_local();
    int off = 0;
    for (unsigned i = 0; i < m->v70.size(); i++, off += 0x20) {
        point20* p = (point20*)((char*)m->v70._Myfirst + off);
        vector4 g;
        sample(&g, &p->a);
        float f = 1.0f - sqrtf((g.x - p->a.x) * (g.x - p->a.x) +
                               (g.y - p->a.y) * (g.y - p->a.y)) * 0.02f;
        if (!(f < 0.0f)) {
            if (f >= 1.0f)
                f = 1.0f;
            else if (!(f > 1e-5f))
                continue;
            vector4 v;
            v.x = p->a.x;
            v.y = p->a.y;
            v.z = (1.0f - f) * p->a.z + (g.z - 1.5f) * f;
            v.w = p->a.w;
            p->a = v;
            p->b = v;
        }
    }
}

} // namespace rgl
