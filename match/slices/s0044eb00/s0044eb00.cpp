// Slice s0044eb00: rgl::mesh editing helpers (append with skinning/tint, uniform scale, translate).
// Names provisional; layouts come from mesh.h (copied from s0041ffe0).
#include "mesh.h"

namespace rgl {

// 0x411630 vector<face20>::operator=, 0x411420 vector<vertex60>::push_back, 0x440aa0 vector<elem30>::push_back,
// 0x412ee0 ensure_local, 0x412f50 add_point, 0x411920 add_face, 0x411d20 build_skin,
// 0x40eba0/0x40ebd0/0x40ec30/0x40ec60 parent-chain accessors, 0x4201b0 aabb::extend_box: all defined elsewhere.

// @ 0x44eb00
void mesh::append_skinned(mesh* other, const mat4* m, int bone, unsigned col, char flip)
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    int npts = (int)v70.size();
    int nverts = (int)v50.size();
    if (other->f228 > f228) f228 = other->f228;
    if (bone + 1 > f228) f228 = bone + 1;
    std::vector<point20>& pts = other->get70();
    std::vector<vertex60>& vs = other->get50();
    std::vector<elem30>& sk = other->get60();
    vector4 p;
    for (unsigned i = 0; i < pts.size(); i++) {
        const vector4& q = pts[i].a;
        p.x = m->f.x * q.y + m->u.x * q.z + q.x * m->s.x + m->o.x;
        p.y = m->s.y * q.x + m->f.y * q.y + m->u.y * q.z + m->o.y;
        p.z = m->s.z * q.x + m->f.z * q.y + m->u.z * q.z + m->o.z;
        add_point(&p);
    }
    if (sk.size() != 0) {
        if (v60.size() < v50.size())
            build_skin();
        for (unsigned i = 0; i < sk.size(); i++) {
            elem30 e = sk[i];
            v60.push_back(e);
        }
    } else {
        if (bone >= 0) {
            if (v60.size() < v50.size())
                build_skin();
            for (unsigned i = 0; i < vs.size(); i++) {
                elem30 e;
                e.w[0] = 1.0f; e.w[3] = 0.0f; e.w[2] = 0.0f; e.w[1] = 0.0f;
                e.bone[0] = bone; e.bone[3] = 0; e.bone[2] = 0; e.bone[1] = 0;
                e.bonef[0] = (float)bone; e.bonef[3] = 0; e.bonef[2] = 0; e.bonef[1] = 0;
                v60.push_back(e);
            }
        }
    }
    vector4& tint = p;
    unpack_color(&tint, &col);
    for (unsigned i = 0; i < vs.size(); i++) {
        const vector4& n = vs[i].pos;
        vector4 t;
        t.x = n.x * m->s.x + (m->f.x * n.y + m->u.x * n.z);
        t.y = m->f.y * n.y + (m->u.y * n.z + n.x * m->s.y);
        t.z = m->f.z * n.y + (m->u.z * n.z + n.x * m->s.z);
        vertex60 v = vs[i];
        v.pos = t;
        v.normal = t;
        v.index += npts;
        vector4 tmp;
        vector4 c = *unpack_color(&tmp, &v.color);
        c.x = c.x * tint.x; c.y = c.y * tint.y; c.z = c.z * tint.z; c.w = c.w * tint.w;
        v.color = pack_color(&c);
        v.f58 = v.color;
        v50.push_back(v);
    }
    std::vector<face20>& fs = other->local_a0 ? other->v90 : other->parent->get90_();
    for (unsigned i = 0; i < fs.size(); i++) {
        if (flip)
            add_face(fs[i].a + nverts, fs[i].c + nverts, fs[i].b + nverts);
        else
            add_face(fs[i].a + nverts, fs[i].b + nverts, fs[i].c + nverts);
    }
    slots[0].f28 = 1;
    slots[1].f28 = 1;
    v22c.extend_box(&other->v22c, m);
}

// @ 0x44f0c0
void mesh::scale(float s)
{
    ensure_local();
    for (unsigned i = 0; i < v70.size(); i++) {
        point20* p = &(&*v70.begin())[i];
        p->a.x = s * p->a.x;
        p->a.y = s * p->a.y;
        p->a.z = s * p->a.z;
        p = &(&*v70.begin())[i];
        p->b.x = s * p->b.x;
        p->b.y = s * p->b.y;
        p->b.z = s * p->b.z;
    }
    for (unsigned j = 0; j < (unsigned)kf.count; j++) {
        for (vector4* q = kf.frames[j].verts._Myfirst; q != kf.frames[j].verts._Mylast; q++) {
            q->x = s * q->x;
            q->y = s * q->y;
            q->z = s * q->z;
        }
    }
    char* dirty = &slots[0].f28;
    *dirty = 1;
    v22c.min.x = s * v22c.min.x;
    v22c.min.y = s * v22c.min.y;
    v22c.min.z = s * v22c.min.z;
    v22c.max.x = s * v22c.max.x;
    v22c.max.y = s * v22c.max.y;
    v22c.max.z = s * v22c.max.z;
    v22c.center.x = s * v22c.center.x;
    v22c.center.y = s * v22c.center.y;
    v22c.center.z = s * v22c.center.z;
    v22c.radius = s * v22c.radius;
}

// @ 0x44f240
void mesh::translate(const vector4* d)
{
    ensure_local();
    for (unsigned i = 0; i < v70.size(); i++) {
        point20* p = &(&*v70.begin())[i];
        p->a.x = p->a.x + d->x;
        p->a.y = d->y + p->a.y;
        p->a.z = d->z + p->a.z;
        p = &(&*v70.begin())[i];
        p->b.x = p->b.x + d->x;
        p->b.y = p->b.y + d->y;
        p->b.z = p->b.z + d->z;
    }
    for (int j = 0; j < kf.count; j++) {
        for (unsigned k = 0; k < kf.frames[j].verts.size(); k++) {
            vector4* q = &(&*kf.frames[j].verts.begin())[k];
            q->x = d->x + q->x;
            q->y = d->y + q->y;
            q->z = d->z + q->z;
        }
    }
    char* dirty = &slots[0].f28;
    *dirty = 1;
    v22c.min.x = v22c.min.x + d->x;
    v22c.min.y = v22c.min.y + d->y;
    v22c.min.z = d->z + v22c.min.z;
    v22c.max.x = v22c.max.x + d->x;
    v22c.max.y = v22c.max.y + d->y;
    v22c.max.z = v22c.max.z + d->z;
    v22c.center.x = v22c.center.x + d->x;
    v22c.center.y = v22c.center.y + d->y;
    v22c.center.z = v22c.center.z + d->z;
}

} // namespace rgl
