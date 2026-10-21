// Slice s00451710: blood splatter on carried meshes. dust_vertices tints the vertices of a mesh that lie near a
// point towards a colour (random falloff), Agent::apply_blood runs it over every lod_mesh attached to the agent's
// body, mesh::compute_normals recomputes the vertex normals from the faces, and append_plain is the thin wrapper
// around append_skinned. Names provisional.
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "mesh.h"

using rgl::vector4;
using rgl::mat4;
using rgl::mesh;
using rgl::lod_mesh;

// a (cdecl) 0x451710 and friends
// @ 0x451710
void dust_vertices(mesh* m, const mat4* xf, const float* center, const float* radius, const float* strength,
                   const vector4* tint)
{
    bool dirty = false;
    char loc = m->local_a1;
    std::vector<rgl::vertex60>& vs = loc ? m->v50 : m->parent->get50_();
    std::vector<rgl::point20>& ps = loc ? m->v70 : m->parent->get70_();
    for (unsigned i = 0; i < vs.size(); i++) {
        rgl::point20* pt = &(&*ps.begin())[(&*vs.begin())[i].index];
        vector4 w;
        w.x = xf->s.x * pt->b.x + xf->u.x * pt->b.z + xf->f.x * pt->b.y + xf->o.x;
        w.y = xf->s.y * pt->b.x + xf->u.y * pt->b.z + xf->f.y * pt->b.y + xf->o.y;
        w.z = xf->s.z * pt->b.x + xf->u.z * pt->b.z + xf->f.z * pt->b.y + xf->o.z;
        float dx = w.x - center[0];
        float dy = w.y - center[1];
        float dz = w.z - center[2];
        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < *radius && (float)(rand() % 15817) * (1.0f / 15817.0f) < 0.6f) {
            float a = (1.0f - dist / *radius) * 1.5f;
            float s = ((float)(rand() % 15817) * (0.6f / 15817.0f) + 0.4f) * a;
            s = s * *strength;
            if (s < 0.0f)
                continue;
            if (s >= 1.0f)
                s = 1.0f;
            else if (!(s > 0.02f))
                continue;
            m->ensure_local();
            vector4 c;
            rgl::unpack_color(&c, &(&*vs.begin())[i].color);
            float omt = 1.0f - s;
            vector4 o = rgl::vadd(rgl::vscale(omt, c), rgl::vscale(s, *tint));
            o.w = c.w;
            (&*m->v50.begin())[i].color = rgl::pack_color(&o);
            dirty = true;
        }
    }
    if (dirty)
        m->slots[0].f28 = 1;
}

// ---- the agent whose body carries lod_meshes -----------------------------------------------------------------
struct ShapeGroup {                                     // object at body+0x304
    void all_shapes(lod_mesh** out, int* count);        // 0x43a130 (ret 8): up to 32 pointers
};
struct AgentBody {
    char pad0[0x58];
    rgl::mat4 m;                                        // +0x58
    char pad98[0x304 - 0x98];
    ShapeGroup* group;                                  // +0x304
};
struct Agent {
    char pad0[0x250];
    AgentBody* body;                                    // +0x250
    void apply_blood(const float* center, float amount);   // 0x4519f0 (ret 8)
};

// @ 0x4519f0
void Agent::apply_blood(const float* center, float amount)
{
    float radius;
    float t = amount * 0.06f;
    if (t < 0.0f)
        radius = 0.0f;
    else if (t >= 0.7f)
        radius = 0.7f;
    else
        radius = t;
    float strength;
    float u = amount * 0.07f;
    if (u < 0.0f)
        strength = 0.0f;
    else if (u >= 1.0f)
        strength = 1.0f;
    else
        strength = u;
    vector4 tint;
    tint.x = 0.4f;
    tint.y = 0.01f;
    tint.z = 0.01f;
    tint.w = 1.0f;
    if (body) {
        lod_mesh* shapes[32];
        int n;
        body->group->all_shapes(shapes, &n);
        for (int s = 0; s < n; s++) {
            lod_mesh* L = shapes[s];
            if (L && L->mask && L->ready()) {
                int cur = L->cur;
                for (int k = 0; k < L->count; k++) {
                    std::vector<mesh*>& list = L->lists[k].v;
                    for (unsigned j = 0; j < list.size(); j++) {
                        mesh** slot = &(&*list.begin())[j];
                        if (*slot) {
                            rgl::mat4 xf = body->m;
                            if (cur == k) {
                                xf = (&*L->lists[cur].v.begin())[j]->m194;
                            } else {
                                std::vector<mesh*>& cl = L->lists[cur].v;
                                if (cl.size() != 0)
                                    xf = (*cl.begin())->m194;
                            }
                            dust_vertices(*slot, &xf, center, &radius, &strength, &tint);
                        }
                    }
                }
            }
        }
    }
}

static inline void cross_inl(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
}
// @ 0x451c50
void mesh::compute_normals()
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    std::vector<vector4> fnorm;
    fnorm.resize(v90.size());
    unsigned i;
    for (i = 0; i < v90.size(); i++) {
        face20* f = &(&*v90.begin())[i];
        vertex60* vs = &*v50.begin();
        point20* ps = &*v70.begin();
        vector4* p0 = &ps[vs[f->a].index].a;
        vector4* p1 = &ps[vs[f->b].index].a;
        vector4* p2 = &ps[vs[f->c].index].a;
        vector4 e1;
        e1.x = p1->x - p0->x; e1.y = p1->y - p0->y; e1.z = p1->z - p0->z;
        vector4 e2;
        e2.x = p2->x - p0->x; e2.y = p2->y - p0->y; e2.z = p2->z - p0->z;
        vector4& n = fnorm[i];
        cross_inl(n, e1, e2);
        float len = (float)sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            n.x = inv * n.x;
            n.y = inv * n.y;
            n.z = inv * n.z;
        } else {
            n.x = 0.0f;
            n.y = 1.0f;
            n.z = 0.0f;
        }
    }
    for (i = 0; i < v50.size(); i++) {
        vector4 z;
        z.x = 0.0f; z.y = 0.0f; z.z = 0.0f;
        (&*v50.begin())[i].normal = z;
    }
    for (i = 0; i < v90.size(); i++) {
        vector4* n = &(&*v50.begin())[(&*v90.begin())[i].a].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
        n = &(&*v50.begin())[(&*v90.begin())[i].b].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
        n = &(&*v50.begin())[(&*v90.begin())[i].c].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
    }
    for (i = 0; i < v50.size(); i++) {
        vector4* n = &(&*v50.begin())[i].normal;
        if (n->x * n->x + n->y * n->y + n->z * n->z > 0.0001f) {
            float len = (float)sqrt(n->x * n->x + n->y * n->y + n->z * n->z);
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                n->x = inv * n->x;
                n->y = inv * n->y;
                n->z = inv * n->z;
            } else {
                n->x = 0.0f; n->y = 1.0f; n->z = 0.0f;
            }
        } else {
            vector4 t;
            t.x = 0.0f; t.y = -1.0f; t.z = 0.0f;
            *n = t;
        }
    }
    slots[0].f28 = 1;
}

// @ 0x4520b0
void mesh::append_plain(mesh* other, const mat4* m)
{
    append_skinned(other, m, -1, -1, 0);
}
