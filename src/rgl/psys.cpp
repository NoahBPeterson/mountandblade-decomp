// psys.cpp (0x413b60..0x414adf): the particle system. std::vector<vertex60> erase/resize (first instantiated
// here), the mesh vertex append, the particle mesh pool, the per-particle geometry builder and the per-frame
// driver. Migrated from slices s00413320 (0x413b60, 0x413bb0), s00413c50 (0x413c50, 0x413d30),
// s00413e50 (0x413e50) and s00414960 (0x414960).
// Boundary: mesh::render (0x413750, previous object) calls psys::update (0x414960) forward.
#include <string.h>
#include <vector>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "./stream.h"
#include "./resource.h"
#include "./mesh.h"
#include "./psys.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../include/keyframes.h"
#include "../include/mat4.h"
#include "../include/mesh_prims.h"
#include "../include/vec.h"
#include "../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)

// 0x413b60 vector::erase (std::vector<rgl::vertex60>, explicit instantiation)
template std::vector<rgl::vertex60>::iterator std::vector<rgl::vertex60>::erase(std::vector<rgl::vertex60>::iterator, std::vector<rgl::vertex60>::iterator);

// 0x413bb0 vector::resize (std::vector<rgl::vertex60>, explicit instantiation)
template void std::vector<rgl::vertex60>::resize(size_t, rgl::vertex60);

// out-of-line std::vector<vertex60>::push_back, called by the original
template<> void std::vector<rgl::vertex60, std::allocator<rgl::vertex60> >::push_back(const rgl::vertex60& v);   // 0x411420 (rgl/particles/mesh_obj.cpp)

namespace rgl {

// @ 0x413c50
int mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)vertices.size();
    vertices.resize(n + 1);
    (&*vertices.begin())[n].index = index;
    (&*vertices.begin())[n].uv = *uv;
    (&*vertices.begin())[n].color = *color;
    (&*vertices.begin())[n].f5c = *f5c;
    vertex60* vv = vertices._Myfirst + n;
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}

// the original reads the pool through an inline helper here; inhabititing the call through one keeps
// cl's register map on this=esi, list=edi like the image's (probe: plain vpush puts the list in esi)
static __forceinline void mesh_pool_push(mesh_pool* p, mesh* const& m) { vpush(p->meshes, m); }

// @ 0x413d30
mesh* mesh_pool::get_mesh(skin* r)
{
    mesh* found = 0;
    if (!r) {
        string name("default");
        r = g_resmgr.get_material(name);
    }
    for (unsigned i = 0; i < meshes.size(); ) {
        mesh* m = vat(meshes, i);
        if (m->res == r) found = m;
        i++;
        if (found) return found;
    }
    mesh* m = new mesh;
    mesh* nm = m;                   // the pushed element: its home is stored before ensure_local, between the two flag stores
    m->res = r;
    m->slots[1].f4 = 1;
    nm->slots[0].f4 = 1;
    m->ensure_local();
    mesh_pool_push(this, nm);
    return m;
}


static __forceinline float eval_curve(float t, const curve4& c)
{
    if (t < c.t0)
        return c.v0;
    if (t > c.t1)
        return c.v1;
    return (c.v1 - c.v0) * ((t - c.t0) / (c.t1 - c.t0)) + c.v0;
}
// the inline vertex append: the colour and f5c arguments travel by address, the uv is the corner's pair
static __forceinline int add_vtx(mesh* m, int idx, const vec2* uv, unsigned color, const vector4* n, int f5c)
{
    return m->add_vertex(idx, uv, &color, n, &f5c);
}
// @ 0x413e50
void psys::emit(particle* p, matrix* frame, mesh* out, const float* ref)
{
    // the basis is built first; the original keeps the frame pointer in ebx for the whole body
    matrix* m = frame;
    // the header stores the curves as float pairs; psys.cpp is the one module that reads them as curve4
    const curve4* curves = (const curve4*)curve;
    // this scratch is the colour/point below, as in the original
    float col[4];
    // the cross products of the basis block
    vector4 t;
    unsigned type = flags & 0xf00;
    if (type == 0x100) {
        // a x b is the normal of the plane holding both rows; the other mode inverts the two factors
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        vector4* f = &m->f;
        *f = d;
        m->s = cross(m->f, m->u);
        m->s.normalize();
        f->x = m->s.z * m->u.y - m->s.y * m->u.z;
        f->y = m->s.x * m->u.z - m->s.z * m->u.x;
        f->z = m->s.y * m->u.x - m->u.y * m->s.x;
        m->rotate_about_f(&p->angle);
    } else if (type == 0x200) {
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        vector4* f = &m->f;
        *f = d;
        m->s.x = f->y * 1.0f - f->z * 0.0f;
        m->s.y = f->z * 0.0f - f->x * 1.0f;
        m->s.z = f->x * 0.0f - f->y * 0.0f;
        m->s.normalize();
        m->u.x = m->f.z * m->s.y - m->s.z * m->f.y;
        m->u.y = m->s.z * m->f.x - m->f.z * m->s.x;
        m->u.z = m->f.y * m->s.x - m->s.y * m->f.x;
        m->rotate_about_f(&p->angle);
    } else if (type == 0x300) {
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        m->f = d;
        m->u = p->vel;
        m->u.normalize();
        m->s = cross(m->f, m->u);
        m->s.normalize();
        m->u = cross(m->s, m->f);
        m->rotate_about_f(&p->angle);
    } else if (type == 0x400) {
        m->f = p->vel;
        m->f.normalize();
        m->s = cross(m->f, m->u);
        m->s.normalize();
        m->u = cross(m->s, m->f);
        m->rotate_about_f(&p->angle);
    } else {
        memcpy(m, p, 12 * sizeof(float));
        m->rotate_about_u(&p->angle);
    }

    // colour over the particle's life: each channel's curve, scaled by the tint
    col[0] = eval_curve(p->age, curves[1]) * tint.x;
    col[1] = eval_curve(p->age, curves[2]) * tint.x;
    col[2] = eval_curve(p->age, curves[3]) * tint.x;
    // the opacity ramp: the first curve is anchored at 0 at both ends of the particle's life
    // (it rises from 0 at t=0 to v0 at t0, then falls back to 0 at t=1), unlike the colour and size curves
    float time = p->age;
    if (time < curves[0].t0)
        col[3] = time / curves[0].t0 * curves[0].v0;
    else if (time > curves[0].t1)
        col[3] = (1.0f - time) / (1.0f - curves[0].t1) * curves[0].v1;
    else
        col[3] = (curves[0].v1 - curves[0].v0) * ((time - curves[0].t0) / (curves[0].t1 - curves[0].t0)) + curves[0].v0;
    unsigned color = pack_color((const vector4*)col);
    float size = eval_curve(p->age, curves[4]);

    if (src) {
        m->o = p->pos;
        int npoints = out->points.size();
        int nverts = out->vertices.size();
        std::vector<point20>& pts = src->local_points ? src->points : src->parent->get70_();
        for (unsigned i = 0; i < pts.size(); i++) {
            float k = size * p->scale;
            float x = k * pts[i].a.x;
            float y = k * pts[i].a.y;
            float z = k * pts[i].a.z;
            col[0] = (x * m->s.x + (y * m->f.x + z * m->u.x)) + m->o.x;
            col[1] = (y * m->f.y + (z * m->u.y + x * m->s.y)) + m->o.y;
            col[2] = (y * m->f.z + (z * m->u.z + x * m->s.z)) + m->o.z;
            out->add_point((const vector4*)col);
        }
        std::vector<vertex60>& vts = src->local_points ? src->vertices : src->parent->get50_();
        for (unsigned i = 0; i < vts.size(); i++) {
            vertex60 vx = vts[i];
            float dx = m->u.x * vts[i].pos.z + m->f.x * vts[i].pos.y + m->s.x * vts[i].pos.x;
            float dy = m->u.y * vts[i].pos.z + m->f.y * vts[i].pos.y + m->s.y * vts[i].pos.x;
            float dz = m->u.z * vts[i].pos.z + m->f.z * vts[i].pos.y + m->s.z * vts[i].pos.x;
            vx.index += npoints;
            vx.pos.w = col[3];
            vx.normal.w = col[3];
            vx.pos.x = dx; vx.pos.y = dy; vx.pos.z = dz;
            vx.normal.x = dx; vx.normal.y = dy; vx.normal.z = dz;
            vx.color = color;
            out->vertices.push_back(vx);
        }
        std::vector<face20>& fcs = src->local_faces ? src->faces : src->parent->get90_();
        for (unsigned i = 0; i < fcs.size(); i++)
            out->add_face(fcs[i].a + nverts, fcs[i].b + nverts, fcs[i].c + nverts);
    } else {
        // camera-facing quad: half-width along s, half-height along u
        vector4 hs = ((m->s * half.u) * size) * p->scale;
        vector4 hu = ((m->u * half.v) * size) * p->scale;
        int v0, v1, v2, v3;
        col[0] = (hu.x + p->pos.x) + hs.x;
        col[1] = (hu.y + p->pos.y) + hs.y;
        col[2] = (hu.z + p->pos.z) + hs.z;
        v0 = out->add_point((const vector4*)col);
        vector4 vb = (hu + p->pos) - hs;
        v1 = out->add_point(&vb);
        vector4 vc = (p->pos - hu) - hs;
        v2 = out->add_point(&vc);
        vector4 vd = (p->pos - hu) + hs;
        v3 = out->add_point(&vd);
        v0 = add_vtx(out, v0, (const vec2*)&quad[0], color, &m->f, 0);
        v1 = add_vtx(out, v1, (const vec2*)&quad[1], color, &m->f, 0);
        v2 = add_vtx(out, v2, (const vec2*)&quad[2], color, &m->f, 0);
        v3 = add_vtx(out, v3, (const vec2*)&quad[3], color, &m->f, 0);
        out->add_face(v0, v1, v2);
        out->add_face(v0, v2, v3);
    }
}static inline vector4 cross_of(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - b.y * a.z;
    r.y = a.z * b.x - b.z * a.x;
    r.z = a.x * b.y - b.x * a.y;
    return r;
}

// @ 0x414960
// Builds the frame the particles of this system are oriented by (s = axis-x-view, f = u x s, u = axis),
// looks the output mesh up on first use and emits every particle.
void psys::update(scene* sc)
{
    mesh_pool* v = sc->view;
    if (v) {
        if (cache == 0)
            cache = v->get_mesh(res);
        matrix m;
        m.u = axis_z;
        m.f = v->axis;
        vector4 t, t2;
        // a dead mention of m.u.y: its float view gets a symbol key here, before the inliner makes the views of
        // m.f.x / m.s.x, and the key order is what orders `m.f.x * m.u.y` and `m.s.x * m.u.y` (fld of the x factor first).
        // Some reference to u.y sat here in the original; an unused `float uy = m.u.y;` gives the same bytes.
        (void)m.u.y;
        t = cross_of(m.f, m.u);
        m.s = t;
        m.s.normalize();
        particle* q = &*particles.begin();
        t2 = cross_of(m.u, m.s);
        m.f = t2;
        if (q != &*particles.end()) {
            do {
                emit(q, &m, cache, (const float*)&v->eye);
                q++;
            } while (q != &*particles.end());
        }
    }
}

} // namespace rgl