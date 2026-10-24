// mesh_ops.cpp: TU 63 "mesh", part 1 (0x47aa40..0x47cae0): mesh vertex rewrites (transform, colour, bone index), the
// lod mesh operations that drive them, the per-vertex lighting pass, the mirror/seam builder, the two skin blends and
// the ground snap. See MIGRATION.md for the boundary and the header notes.
#include "./mesh_slot.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through troop_mgr.h) declares a global ::vat with the same signature
#define vat rgl::vat

// the original inlines vector4::normalize: x*x + y*y + z*z, 1/len scaling, (0,1,0) when len <= 1e-10
static __forceinline void vnorm(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
}

static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// std::vector<int>::operator= taken as the plain assign at 0x411630 (this = dst vector, arg = src vector)
struct intvec {
    int al; int* first; int* last; int* end;
    void assign(intvec* src);                       // 0x411630 (thiscall)
};
struct vvec {
    void push_back(const vertex60& v);              // 0x411420 (thiscall, callee pops 4): std::vector<vertex60>::push_back
};

// @ 0x47aa40 -- append a transformed copy of `src` to this mesh: the matrix rows are re-orthogonalised against +Z
// (f = normalise(cross(up, s)), s = cross(f, up)), `src`'s points are rotated, flattened onto the plane of m->u and
// offset by m->o, its vertices are rotated and tinted by the unpacked colour, and its faces are re-added with the
// vertex indices shifted by this mesh's existing vertex count.
void mesh_x::transform(mesh* src, const matrix* m, unsigned color)
{
    if (!local_a0) {
        ((intvec*)&v90)->assign((intvec*)&parent->v90);
        local_a0 = 1;
    }
    ensure_local();

    int n70 = (int)v70.size();
    int nverts = (int)v50.size();

    matrix mm = *m;
    mm.u = vector4(0.0f, 0.0f, 1.0f);
    mm.f = cross_i(mm.u, mm.s);
    mm.f.normalize();
    mm.s = cross_i(mm.f, mm.u);

    std::vector<point20>& pts = ((mesh_x*)src)->get70_rec();
    std::vector<vertex60>& sv = ((mesh_x*)src)->get50_rec();

    for (size_t i = 0; i < pts.size(); i++) {
        float x = mm.s.x * pts[i].a.x + mm.f.x * pts[i].a.y + mm.u.x * pts[i].a.z;
        float y = mm.u.y * pts[i].a.z + mm.s.y * pts[i].a.x + mm.f.y * pts[i].a.y;
        float z = mm.u.z * pts[i].a.z + mm.f.z * pts[i].a.y + mm.s.z * pts[i].a.x;
        float zz = z - (y * m->u.y + x * m->u.x) / m->u.z;
        vector4 out;
        out.x = x + m->o.x;
        out.y = y + m->o.y;
        out.z = zz + m->o.z;
        add_point(&out);
    }

    vector4 tint;
    unpack_color(&tint, &color);

    vertex60* sp = sv._Myfirst;                      // the original walks a cached pointer (stale if src is this)
    for (size_t j = 0; j < sv.size(); j++, sp++) {
        vertex60 vt = *sp;
        float x = mm.s.x * sp->pos.x + mm.u.x * sp->pos.z + mm.f.x * sp->pos.y;
        float y = mm.s.y * sp->pos.x + mm.f.y * sp->pos.y + mm.u.y * sp->pos.z;
        float z = mm.s.z * sp->pos.x + mm.f.z * sp->pos.y + mm.u.z * sp->pos.z;
        vt.index += n70;
        vector4 r(x, y, z);                          // w stays unset: the original copies the stack lane
        vt.pos = r;
        vt.normal = r;
        vector4 uc;
        vector4 c = *UNPACK_COLOR(&uc, &vt.color);
        c.x *= tint.x;  c.y *= tint.y;  c.z *= tint.z;  c.w *= tint.w;
        vt.color = pack_color(&c);
        ((vvec*)&v50)->push_back(vt);
    }

    std::vector<face20>& faces = ((mesh_x*)src)->get90_rec();
    for (size_t i = 0; i < faces.size(); i++)
        add_face(faces[i].a + nverts, faces[i].b + nverts, faces[i].c + nverts);

    slots[0].f28 = 1;
    slots[1].f28 = 1;
    finish_load();
}

// @ 0x47af30 -- multiply the vertex colour terms and the key-frame colours by `color`, renormalise the colour
// vectors of every vertex, and rebuild the slot vertices when one or three components are negative.
void mesh_x::set_color(vector4 color)
{
    ensure_local();

    // 0x47af3b: the local point colours (0x70 list), both halves of every record
    for (point20* p = v70._Myfirst; p < v70._Mylast; p = (point20*)((char*)p + 0x20)) {
        p->a.x *= color.x;  p->a.y *= color.y;  p->a.z *= color.z;
        p->b.x *= color.x;  p->b.y *= color.y;  p->b.z *= color.z;
    }

    // 0x47af89: every key-frame record's first colour vector
    for (int i = 0; i < kf.count; i++) {
        vector4* p = kf.frames[i].verts._Myfirst;
        for (; p != kf.frames[i].verts._Mylast; p++) {
            p->x *= color.x;  p->y *= color.y;  p->z *= color.z;
        }
    }

    // 0x47aff3: vertex +0x30 and +0x00 terms: scale, then renormalise
    for (size_t i = 0; i < v50.size(); i++) {
        v50[i].normal.x *= color.x;  v50[i].normal.y *= color.y;  v50[i].normal.z *= color.z;
        vnorm(v50[i].normal);
        v50[i].pos.x *= color.x;  v50[i].pos.y *= color.y;  v50[i].pos.z *= color.z;
        vnorm(v50[i].pos);
    }

    long nneg = 0;
    if (color.x < 0.0f) nneg = 1;
    if (color.y < 0.0f) nneg++;
    if (color.z < 0.0f) nneg++;
    if (nneg == 1 || nneg == 3)
        flip_faces();                               // 0x4779b0

    slots[0].f28 = 1;
    bounds.min.x *= color.x;  bounds.min.y *= color.y;  bounds.min.z *= color.z;
    bounds.max.x *= color.x;  bounds.max.y *= color.y;  bounds.max.z *= color.z;
    bounds.update();                                // 0x416f70

    // 0x47b1ff: with a negative component and shaders in use, the vertex +0x10 / +0x20 terms are scaled too
    if (g_use_shaders && nneg > 0) {
        for (size_t i = 0; i < v50.size(); i++) {
            vertex60* v = &v50[i];
            v->f10.x *= color.x;  v->f10.y *= color.y;  v->f10.z *= color.z;
            vnorm(v->f10);
            v->f20.x *= color.x;  v->f20.y *= color.y;  v->f20.z *= color.z;
            vnorm(v->f20);
        }
    }
}

// @ 0x47b360 -- stamp the bone index into every vertex record, then mark the first slot dirty
void mesh_x::set_bone_index(int bone)
{
    ensure_local();
    for (size_t i = 0; i < v50.size(); i++)
        v50[i].color = bone;                         // +0x54 (the vertex's colour word doubles as the bone index here)
    slots[0].f28 = 1;
}

// @ 0x47b3b0
void mesh_x::multiply_color(const vector4* tint)
{
    ensure_local();
    for (unsigned i = 0; i < v50.size(); i++) {
        vertex60* v = &vat(v50, i);
        vector4 c;
        unpack_color(&c, &v->color);
        c.x = c.x * tint->x;
        c.y = c.y * tint->y;
        c.z = c.z * tint->z;
        v->color &= 0xff000000;
        v->color |= pack_color(&c) & 0xffffff;
    }
    slots[0].f28 = 1;
}

// @ 0x47b480
void mesh_list_x::append(mesh_list* src, int bone, const matrix* m, const unsigned* color)
{
    for (std::vector<mesh*>::iterator it = src->v.begin(); it < src->v.end(); ++it) {
        if (bone == -1) {
            mesh* c = ((mesh_x*)*it)->clone();
            vpush(v, c);
        } else {
            mesh* c = new mesh;
            skin* old = c->res;
            skin* nw = (*it)->res;
            if (old)
                old->f64--;
            c->res = nw;
            if (nw)
                nw->f64++;
            vpush(v, c);
            ((mesh_x*)c)->append_skinned(*it, m, bone, *color, 0);
        }
    }
}

// @ 0x47b5b0
void lod_mesh_x::set_color(unsigned color)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& l = lists[i].v;
        for (std::vector<mesh*>::iterator it = l.begin(); it < l.end(); ++it)
            ((mesh_x*)*it)->set_bone_index(color);
    }
}

// @ 0x47b610
void lod_mesh_x::transform(const matrix& m)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& l = lists[i].v;
        for (std::vector<mesh*>::iterator it = l.begin(); it < l.end(); ++it)
            ((mesh_x*)*it)->transform(m);
    }
}

// @ 0x47b690
void lod_mesh_x::append(lod_mesh* src, int bone, const matrix* m, const unsigned* color)
{
    if (count < src->count) {
        int old = count;
        count = src->count;
        if (old > 0) {
            unsigned white = 0xffffffff;
            matrix ident;
            for (int i = old; i < count; i++)
                ((mesh_list_x*)&lists[i])->append(&lists[old - 1], -1, identity_pos(&ident), &white);
        }
    }
    if (src->count < count && src->count > 0) {
        for (int i = src->count; i < count; i++)
            ((mesh_list_x*)&lists[i])->append(&src->lists[src->count - 1], bone, m, color);
    }
    for (int i = 0; i < src->count; i++)
        ((mesh_list_x*)&lists[i])->append(&src->lists[i], bone, m, color);
}

// @ 0x47b7b0 -- the scene lights every vertex of a mesh, bakes the result into the vertex colours and (for the
// tangent-space material modes) also into the two extra colour words.
void world_x::light_mesh(mesh* m, const matrix* frame, unsigned flags)
{
    m->ensure_local();
    matrix lm = (*frame) * m->m_cc;

    material* mat = m->res->shader;
    unsigned mode = mat->stages[0].b & 0x1f;          // +0x6c
    bool flat = (mode == 0x18) && (mat->flags20 & 0x40) != 0;
    bool tangent = mode == 0x18;

    vector4 tint = ambient;

    // Flat-lit resources: no per-vertex work, just combine the scene light colour with the tint.
    if (m->res->flags & 0x40) {
        vector4 dir, col;
        light_at(lm.o, lm.u, &dir, &col, flags, 1.0f);
        vector4 c;
        c.x = col.x * 0.7f + tint.x;
        c.y = col.y * 0.7f + tint.y;
        c.z = col.z * 0.7f + tint.z;
        c.w = 1.0f;
        if (!g_use_shaders) {
            c.x = (float)pow(c.x, g_ramp_gamma);
            c.y = (float)pow(c.y, g_ramp_gamma);
            c.z = (float)pow(c.z, g_ramp_gamma);
        }
        ((mesh_x*)m)->multiply_color(&c);
        return;
    }

    // The base colour of the mesh itself (only used by the software colour path).
    if (flat && !g_use_shaders) {
        vector4 c = tint;
        c.x = (float)pow(c.x, g_ramp_gamma);
        c.y = (float)pow(c.y, g_ramp_gamma);
        c.z = (float)pow(c.z, g_ramp_gamma);
        m->color = pack_color(&c) | 0xff000000;
    }

    int count = (int)m->v50.size();
    if (count <= 0)
        return;

    for (int i = 0; i < count; i++) {
        vertex60* v = &vat(m->v50, i);

        float alpha = 1.0f;
        if (flat)
            alpha = (float)(v->color & 0xff) * (1.0f / 255.0f);

        point20* p = &vat(m->v70, v->index);
        vector4 pos, nrm;
        pos.x = lm.s.x * p->a.x + lm.f.x * p->a.y + lm.u.x * p->a.z + lm.o.x;
        pos.y = lm.s.y * p->a.x + lm.f.y * p->a.y + lm.u.y * p->a.z + lm.o.y;
        pos.z = lm.s.z * p->a.x + lm.f.z * p->a.y + lm.u.z * p->a.z + lm.o.z;
        nrm.x = lm.s.x * v->pos.x + lm.f.x * v->pos.y + lm.u.x * v->pos.z;
        nrm.y = lm.s.y * v->pos.x + lm.f.y * v->pos.y + lm.u.y * v->pos.z;
        nrm.z = lm.s.z * v->pos.x + lm.f.z * v->pos.y + lm.u.z * v->pos.z;

        vector4 dir, col, dir2, col2;
        if (!g_use_shaders) {
            light_at(pos, nrm, &dir, &col, flags, alpha);
        } else {
            light_at(pos, nrm, &dir, &col, flags | 2, 1.0f);
            light_at(pos, nrm, &dir2, &col2, flags | 4, 1.0f);
        }

        if (tangent || flat) {
            vector4 vn, vb, vt;
            vn.x = lm.s.x * v->pos.x + lm.f.x * v->pos.y + lm.u.x * v->pos.z;
            vn.y = lm.s.y * v->pos.x + lm.f.y * v->pos.y + lm.u.y * v->pos.z;
            vn.z = lm.s.z * v->pos.x + lm.f.z * v->pos.y + lm.u.z * v->pos.z;
            vb.x = lm.s.x * v->f10.x + lm.f.x * v->f10.y + lm.u.x * v->f10.z;
            vb.y = lm.s.y * v->f10.x + lm.f.y * v->f10.y + lm.u.y * v->f10.z;
            vb.z = lm.s.z * v->f10.x + lm.f.z * v->f10.y + lm.u.z * v->f10.z;
            vt.x = lm.s.x * v->f20.x + lm.f.x * v->f20.y + lm.u.x * v->f20.z;
            vt.y = lm.s.y * v->f20.x + lm.f.y * v->f20.y + lm.u.y * v->f20.z;
            vt.z = lm.s.z * v->f20.x + lm.f.z * v->f20.y + lm.u.z * v->f20.z;

            float k = (col.x + col.y + col.z) * 0.33f;
            vector4 c;
            c.x = (vb.x * -dir.x + vb.y * -dir.y + vb.z * -dir.z) * k;
            c.y = -((vt.z * -dir.z + vt.y * -dir.y + vt.x * -dir.x) * k);
            c.z = (vn.z * -dir.z + vn.y * -dir.y + vn.x * -dir.x) * k;
            c.w = 1.0f;
            unsigned lit = pack_signed_color((const float*)&c);
            if (flat)
                v->f58 = v->f58 ^ ((v->f58 ^ lit) & 0xffffff);
            else
                v->f5c = v->f5c ^ ((v->f5c ^ lit) & 0xffffff);

            if (g_use_shaders && flat) {
                float k2 = (col2.x + col2.y + col2.z) * 0.33f;
                c.x = (vb.x * -dir2.x + vb.y * -dir2.y + vb.z * -dir2.z) * k2;
                c.y = -((vt.z * -dir2.z + vt.y * -dir2.y + vt.x * -dir2.x) * k2);
                c.z = (vn.z * -dir2.z + vn.y * -dir2.y + vn.x * -dir2.x) * k2;
                v->f5c = pack_signed_color((const float*)&c) & 0xffffff;
            }

        } else if (!g_use_shaders) {
            col.x += tint.x;
            col.y += tint.y;
            col.z += tint.z;
        }

        if (!g_use_shaders) {
            col.x = (float)pow(col.x, g_ramp_gamma);
            col.y = (float)pow(col.y, g_ramp_gamma);
            col.z = (float)pow(col.z, g_ramp_gamma);
        }

        vector4 vc;
        unpack_color(&vc, &v->color);
        vc.x *= col.x;
        vc.y *= col.y;
        vc.z *= col.z;
        unsigned lit = pack_color(&vc);
        if (!g_use_shaders || tangent) {
            if (flat || !tangent)
                v->color = v->color ^ ((v->color ^ lit) & 0xffffff);
        } else {
            v->f5c = lit & 0xffffff;
        }
    }
}

// @ 0x47c1d0 -- mirror a mesh about x = 0: points with x > 0.001 get a reflected twin, vertices and faces follow.
void EChar::build_mirror(mesh* m, int* remap, int* count, char flip)
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
                    *(float*)&t.f48 = 1.0f - *(float*)&t.f48;
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
                *(float*)&t.f48 = 1.0f - *(float*)&t.f48;
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

// @ 0x47c6b0 -- weighted bone-matrix blend of the rest points
void EChar::blend(mesh* m, int unused, const float* weights)
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

// @ 0x47c810 -- index-driven blend: `set` is an ArrSet, `*index` the slot whose point map selects the offsets
void EChar::blend_indexed(mesh* m, void* set, int* index, const float* weights)
{
    m->ensure_local();
    ArrSet* s = (ArrSet*)set;
    if ((int)m->v70.size() != s->cnt[*index])
        return;
    unsigned i = 0;
    while (i < m->v70.size()) {
        point20* p = &vat(m->v70, i);
        for (int j = 0; j < bone_count; j++) {
            int bi = s->arr[*index][i];
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

// @ 0x47c9b0 -- pull the points of a mesh toward the terrain height near the map edge
void TerrainX::snap(mesh* m)
{
    m->ensure_local();
    int off = 0;
    for (unsigned i = 0; i < m->v70.size(); i++, off += 0x20) {
        point20* p = (point20*)((char*)m->v70._Myfirst + off);
        vector4 g = sample(&p->a.x);
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
