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

// 0x413b60 vector::erase (std::vector<rgl::vertex60>, explicit instantiation)
template std::vector<rgl::vertex60>::iterator std::vector<rgl::vertex60>::erase(std::vector<rgl::vertex60>::iterator, std::vector<rgl::vertex60>::iterator);

// 0x413bb0 vector::resize (std::vector<rgl::vertex60>, explicit instantiation)
template void std::vector<rgl::vertex60>::resize(size_t, rgl::vertex60);

namespace rgl {

// @ 0x413c50
int mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)v50.size();
    v50.resize(n + 1);
    (&*v50.begin())[n].index = index;
    (&*v50.begin())[n].uv = *uv;
    (&*v50.begin())[n].color = *color;
    (&*v50.begin())[n].f5c = *f5c;
    vertex60* vv = v50._Myfirst + n;
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}

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
    m->res = r;
    m->slots[1].f4 = 1;
    m->slots[0].f4 = 1;
    m->ensure_local();
    vpush(meshes, m);
    return m;
}

static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsubl(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vmul(const vector4& a, const float& k) { return vector4(a.x * k, a.y * k, a.z * k); }

static __forceinline float curve(float t, const curve4& c)
{
    if (t < c.t0)
        return c.v0;
    if (t > c.t1)
        return c.v1;
    return (c.v1 - c.v0) * ((t - c.t0) / (c.t1 - c.t0)) + c.v0;
}

// the out-of-line chain accessors (0x40eba0, 0x40ebd0, 0x40ec30): emit tests the local flag itself and calls them on
// the parent (mesh.h's inline get70() would expand one level more)
struct mesh_acc : rgl::mesh {
    std::vector<point20>& get70_();     // 0x40eba0
    std::vector<vertex60>& get50_();    // 0x40ebd0
    std::vector<face20>& get90_();      // 0x40ec30
};

// out-of-line std::vector<vertex60> append (push_back's slow path), called by the original
struct vertex_vec : std::vector<vertex60> { void push_ext(const vertex60& v); };   // 0x411420

// @ 0x413e50
void psys::emit(particle* p, matrix* m, mesh* out, const float* ref)
{
    // c is reused throughout, as in the original: cross-product scratch, then colour (r, g, b, a), then the
    // quad's first corner and finally the four corner indices
    union { float col[4]; int idx[4]; } c;
    vector4& t = *(vector4*)c.col;
    unsigned type = flags & 0xf00;
    if (type == 0x100) {
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        vector4* f = &m->f;
        *f = d;
        t.x = f->y * m->u.z - m->u.y * f->z;
        t.y = m->u.x * f->z - f->x * m->u.z;
        t.z = m->u.y * f->x - m->u.x * f->y;
        m->s = t;
        m->s.normalize();
        t.x = m->s.z * m->u.y - m->s.y * m->u.z;
        t.y = m->s.x * m->u.z - m->s.z * m->u.x;
        t.z = m->s.y * m->u.x - m->u.y * m->s.x;
        m->f = t;
        m->rotate_about_f(&p->rot);
    } else if (type == 0x200) {
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        vector4* f = &m->f;
        *f = d;
        t.x = f->y * 1.0f - f->z * 0.0f;
        t.y = f->z * 0.0f - f->x * 1.0f;
        t.z = f->x * 0.0f - f->y * 0.0f;
        m->s = t;
        m->s.normalize();
        t.x = f->z * m->s.y - m->s.z * f->y;
        t.y = m->s.z * f->x - f->z * m->s.x;
        t.z = f->y * m->s.x - m->s.y * f->x;
        m->u = t;
        m->rotate_about_f(&p->rot);
    } else if (type == 0x300) {
        vector4 d;
        d.x = p->pos.x - ref[0];
        d.y = p->pos.y - ref[1];
        d.z = p->pos.z - ref[2];
        d.normalize();
        m->f = d;
        m->u = p->dir;
        m->u.normalize();
        m->s = cross(m->f, m->u);
        m->s.normalize();
        m->u = cross(m->s, m->f);
        m->rotate_about_f(&p->rot);
    } else if (type == 0x400) {
        m->f = p->dir;
        m->f.normalize();
        m->s = cross(m->f, m->u);
        m->s.normalize();
        m->u = cross(m->s, m->f);
        m->rotate_about_f(&p->rot);
    } else {
        memcpy(m, p, 12 * sizeof(float));
        m->rotate_about_u(&p->rot);
    }

    // colour and alpha over the particle's life
    // the colour (r, g, b, a) is also reused as the scratch vector / index slots below, as in the original
    c.col[0] = curve(p->t, r) * brightness;
    c.col[1] = curve(p->t, g) * brightness;
    c.col[2] = curve(p->t, b) * brightness;
    if (p->t < a.t0)
        c.col[3] = (p->t / a.t0) * a.v0;
    else if (p->t > a.t1)
        c.col[3] = ((1.0f - p->t) / (1.0f - a.t1)) * a.v1;
    else
        c.col[3] = (a.v1 - a.v0) * ((p->t - a.t0) / (a.t1 - a.t0)) + a.v0;
    unsigned color = pack_color((const vector4*)c.col);
    float size = curve(p->t, s);

    if (src) {
        m->o = p->pos;
        int npoints = out->v70.size();
        int nverts = out->v50.size();
        std::vector<point20>& pts = src->local_a1 ? src->v70 : ((mesh_acc*)src->parent)->get70_();
        for (unsigned i = 0; i < pts.size(); i++) {
            float k = size * p->scale;
            float x = k * pts[i].a.x;
            float y = k * pts[i].a.y;
            float z = k * pts[i].a.z;
            c.col[0] = (x * m->s.x + (y * m->f.x + z * m->u.x)) + m->o.x;
            c.col[1] = (y * m->f.y + (z * m->u.y + x * m->s.y)) + m->o.y;
            c.col[2] = (y * m->f.z + (z * m->u.z + x * m->s.z)) + m->o.z;
            out->add_point((const vector4*)c.col);
        }
        std::vector<vertex60>& vts = src->local_a1 ? src->v50 : ((mesh_acc*)src->parent)->get50_();
        for (unsigned i = 0; i < vts.size(); i++) {
            vertex60 vx = vts[i];
            float dx = m->s.x * vts[i].pos.x + (m->f.x * vts[i].pos.y + m->u.x * vts[i].pos.z);
            float dy = m->s.y * vts[i].pos.x + (m->f.y * vts[i].pos.y + m->u.y * vts[i].pos.z);
            float dz = m->s.z * vts[i].pos.x + (m->f.z * vts[i].pos.y + m->u.z * vts[i].pos.z);
            vx.index += npoints;
            vx.pos.w = c.col[3];
            vx.normal.w = c.col[3];
            vx.pos.x = dx; vx.pos.y = dy; vx.pos.z = dz;
            vx.normal.x = dx; vx.normal.y = dy; vx.normal.z = dz;
            vx.color = color;
            ((vertex_vec*)&out->v50)->push_ext(vx);
        }
        std::vector<face20>& fcs = src->local_a0 ? src->v90 : ((mesh_acc*)src->parent)->get90_();
        for (unsigned i = 0; i < fcs.size(); i++)
            out->add_face(fcs[i].a + nverts, fcs[i].b + nverts, fcs[i].c + nverts);
    } else {
        // camera-facing quad: half-width along s, half-height along u
        vector4 hs0 = vmul(m->s, half_w);
        vector4 hu0 = vmul(m->u, half_h);
        vector4 hu = vmul(vmul(hu0, size), p->scale);
        vector4 hs = vmul(vmul(hs0, size), p->scale);
        c.col[0] = (hu.x + p->pos.x) + hs.x; c.col[1] = (hu.y + p->pos.y) + hs.y; c.col[2] = (hu.z + p->pos.z) + hs.z;
        c.idx[0] = out->add_point((const vector4*)c.col);
        vector4 vb = vsubl(vadd(hu, p->pos), hs);
        c.idx[1] = out->add_point(&vb);
        vector4 vc = vsubl(vsubl(p->pos, hu), hs);
        c.idx[2] = out->add_point(&vc);
        vector4 vd = vadd(vsubl(p->pos, hu), hs);
        c.idx[3] = out->add_point(&vd);
        unsigned colorv;
        int f5c;
        int v0, v1, v2, v3;
        f5c = 0; colorv = color;
        v0 = out->add_vertex(c.idx[0], &uv[0], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v1 = out->add_vertex(c.idx[1], &uv[1], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v2 = out->add_vertex(c.idx[2], &uv[2], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v3 = out->add_vertex(c.idx[3], &uv[3], &colorv, &m->f, &f5c);
        out->add_face(v0, v1, v2);
        out->add_face(v0, v2, v3);
    }
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
        m.u = axis;
        m.f = v->axis;
        vector4 t, t2;
        t.x = ((m.f.y)*( m.u.z)) - ((m.u.y)*( m.f.z));
        t.y = ((m.f.z)*( m.u.x)) - ((m.u.z)*( m.f.x));
        t.z = ((m.f.x)*( m.u.y)) - ((m.f.y)*( m.u.x));
        m.s = t;
        m.s.normalize();
        t2.x = ((m.s.z)*( m.u.y)) - ((m.s.y)*( m.u.z));
        t2.y = ((m.u.z)*( m.s.x)) - ((m.s.z)*( m.u.x));
        t2.z = ((m.s.y)*( m.u.x)) - ((m.s.x)*( m.u.y));
        m.f = t2;
        if (begin != end) {
            particle* q = begin;
            do {
                emit(q, &m, cache, (const float*)&v->eye);
                q++;
            } while (q != end);
        }
    }
}

} // namespace rgl
