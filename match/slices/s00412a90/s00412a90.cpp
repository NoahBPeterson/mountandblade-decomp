// Slice s00412a90: mesh instance object (0x260 bytes): ctor, scene mesh cache, ensure_local, add_point, append.
#include <string.h>
#include "mesh.h"

namespace rgl {


// @ 0x412a90
mesh::mesh()
{
    mat4 tmp;
    {
        slot* s = slots;
        for (int n = 3; n; n--, s++) {
            s->f0 = 0; s->f4 = 0; s->f8 = 0; s->fc = 0;
            s->f28 = 1; s->f29 = 0;
            memset(&s->p, 0, 8);
            s->f20 = 0; s->f24 = -1;
        }
    }
    v22c.init();
    c4 = -1.0f;
    c8 = -1.0f;
    parent = 0;
    local_a0 = 1;
    local_a1 = 1;
    f4c = 0;
    res = 0;
    flags = 0;
    f34 = 0;
    f38 = 0;
    color = (unsigned)-1;
    f40 = -1;
    f44 = 0;
    m_cc = *identity(&tmp);
    m1d4 = *identity(&tmp);
    mask = 0xffff;
    m194 = *identity(&tmp);
    f48 = 0;
    f220 = 0; f224 = 0; f228 = 0;
    p_c0 = 0;
    slots[1].f4 = 0;
    slots[0].f4 = 0;
    f214 = 0x80;
}

// @ 0x412c30
mesh* scene::get_mesh(res_t* r)
{
    unsigned found = (unsigned)-1;
    for (unsigned i = 0; i < cache.size(); ) {
        if (vat(cache, i)->res == r) found = i;
        i++;
        if (found != (unsigned)-1) return vat(cache, found);
    }
    unsigned n = cache.size();
    mesh* m = new mesh;
    m->res = r;
    m->flags |= 0xa0000000;
    m->slots[1].f4 = 1;
    m->slots[0].f4 = 1;
    m->f44 = 10000.0f;
    m->f38 = 1;
    vpush(cache, m);
    return vat(cache, n);
}


// @ 0x412ee0
void mesh::ensure_local()
{
    if (!local_a1) {
        v50 = parent->v50;
        v60 = parent->v60;
        v70 = parent->v70;
        v80 = parent->v80;
        kf = parent->kf;
        local_a1 = 1;
    }
}

// @ 0x412f50
int mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)v70.size();
    v70.resize(n + 1);
    v70[n].a = *p;
    v70[n].b = *p;
    return n;
}

// @ 0x412fe0
void mesh::append(mesh* other, const mat4* m)
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    int npts = (int)v70.size();
    int nverts = (int)v50.size();
    std::vector<point20>& pts = other->get70();
    std::vector<vertex60>& vs = other->get50();
    vector4 p;
    for (unsigned i = 0; i < pts.size(); i++) {
        const vector4& q = pts[i].b;
        p.x = m->f.x * q.y + m->u.x * q.z + q.x * m->s.x + m->o.x;
        p.y = m->s.y * q.x + m->u.y * q.z + m->f.y * q.y + m->o.y;
        p.z = m->s.z * q.x + m->u.z * q.z + m->f.z * q.y + m->o.z;
        add_point(&p);
    }
    unpack_color(&p, &other->color);
    for (unsigned i = 0; i < vs.size(); i++) {
        const vector4& n = vs[i].normal;
        vertex60 v = vs[i];
        vector4 t;
        t.x = (m->f.x * n.y + m->u.x * n.z) + m->s.x * n.x;
        t.y = (m->f.y * n.y + m->u.y * n.z) + m->s.y * n.x;
        t.z = (m->f.z * n.y + m->u.z * n.z) + m->s.z * n.x;
        v.normal = t;
        v.index += npts;
        vector4 col;
        unpack_color(&col, &v.color);
        col.x *= p.x; col.y *= p.y; col.z *= p.z; col.w *= p.w;
        v.color = pack_color(&col);
        v50.push_back(v);
    }
    std::vector<face20>& fs = other->get90();
    for (unsigned i = 0; i < fs.size(); i++)
        add_face(fs[i].a + nverts, fs[i].b + nverts, fs[i].c + nverts);
    slots[0].f28 = 1;
    slots[1].f28 = 1;
}


} // namespace rgl

// vector<elem14>::operator= (0x412d50), explicit instantiation
template std::vector<rgl::elem14>& std::vector<rgl::elem14>::operator=(const std::vector<rgl::elem14>&);
