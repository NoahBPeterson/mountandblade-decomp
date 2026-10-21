// Slice s00413e50: particle system - builds the frame, colour and geometry for one particle.
#include "psys.h"

namespace rgl { unsigned pack_color(const vector4* c); }
unsigned pack_color(const float* c);          // 0x40e440

// 0x407eb0 inlined by the original: r = a x b
static __forceinline void cross_in(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
}

static __forceinline float curve(float t, const curve4& c)
{
    if (t < c.t0)
        return c.v0;
    if (t > c.t1)
        return c.v1;
    return (c.v1 - c.v0) * ((t - c.t0) / (c.t1 - c.t0)) + c.v0;
}

// the engine's inline vertex helper: colour and f5c travel by address
static __forceinline int add_vtx(mesh* m, int idx, const float* uv, unsigned color, const vector4* n, int f5c)
{
    return m->add_vertex(idx, uv, &color, n, &f5c);
}

struct vertex_vec : std::vector<vertex60> { void push_ext(const vertex60& v); };   // 0x411420

// @ 0x413e50
void psys::emit(particle* p, mat4* m, mesh* out, const float* ref)
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
        t.x = m->s.z * f->y - m->s.y * f->z;
        t.y = m->s.x * f->z - m->s.z * f->x;
        t.z = m->s.y * f->x - f->y * m->s.x;
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
    unsigned color = pack_color(c.col);
    float size = curve(p->t, s);

    if (src) {
        m->o = p->pos;
        int npoints = out->v70.size();
        int nverts = out->v50.size();
        std::vector<point20>& pts = src->local_a1 ? src->v70 : src->parent->get70();
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
        std::vector<vertex60>& vts = src->local_a1 ? src->v50 : src->parent->get50();
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
        std::vector<face20>& fcs = src->local_a0 ? src->v90 : src->parent->get90();
        for (unsigned i = 0; i < fcs.size(); i++)
            out->add_face(fcs[i].a + nverts, fcs[i].b + nverts, fcs[i].c + nverts);
    } else {
        // camera-facing quad: half-width along s, half-height along u
        float hu_x = ((half_h * m->u.x) * size) * p->scale;
        float hu_y = ((m->u.y * half_h) * size) * p->scale;
        float hu_z = ((m->u.z * half_h) * size) * p->scale;
        float hs_x = (half_w * m->s.x) * size;
        float hs_xs = hs_x * p->scale;
        float hs_y = ((m->s.y * half_w) * size) * p->scale;
        float hs_z = ((m->s.z * half_w) * size) * p->scale;
        c.col[0] = (hu_x + p->pos.x) + hs_xs;
        c.col[1] = (hu_y + p->pos.y) + hs_y;
        c.col[2] = (hu_z + p->pos.z) + hs_z;
        c.idx[0] = out->add_point((const vector4*)c.col);
        vector4 vb;
        vb.x = (hu_x + p->pos.x) - hs_xs;
        vb.y = (hu_y + p->pos.y) - hs_y;
        vb.z = (hu_z + p->pos.z) - hs_z;
        c.idx[1] = out->add_point(&vb);
        vb.x = (p->pos.x - hu_x) - hs_xs;
        vb.y = (p->pos.y - hu_y) - hs_y;
        vb.z = (p->pos.z - hu_z) - hs_z;
        c.idx[2] = out->add_point(&vb);
        vb.x = (p->pos.x - hu_x) + hs_xs;
        vb.y = (p->pos.y - hu_y) + hs_y;
        vb.z = (p->pos.z - hu_z) + hs_z;
        c.idx[3] = out->add_point(&vb);
        unsigned colorv;
        int f5c;
        int v0, v1, v2, v3;
        f5c = 0; colorv = color;
        v0 = out->add_vertex(c.idx[0], &uv[0][0], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v1 = out->add_vertex(c.idx[1], &uv[1][0], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v2 = out->add_vertex(c.idx[2], &uv[2][0], &colorv, &m->f, &f5c);
        f5c = 0; colorv = color;
        v3 = out->add_vertex(c.idx[3], &uv[3][0], &colorv, &m->f, &f5c);
        out->add_face(v0, v1, v2);
        out->add_face(v0, v2, v3);
    }
}
