// mesh_edit.cpp: the polygon-mesh editor operations (0x4d1410..0x4d2df0): split-quad, extrude-boundary-edge,
// add-triangle, finish (duplicate the boundary vertices of the marked faces) and split-marked-edge, on the
// canonical PolyMesh through mesh_edit.h. Names provisional.
#include <windows.h>
#include <stdlib.h>
#include "./mesh_edit.h"

// The vector arithmetic is inlined in the original (rgl's out-of-line operator- / cross are not called).
static inline vector4 vcross(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// The polygon constructor as this file sees it (0x477ca0, written out: a loop that cl unrolls would re-run its
// optimizer over the whole caller and fold every materialised element address).
struct EFace : MPoly {
    EFace() : MPoly(raw_t())
    {
        stamp = 0; f5c = 0; f60 = 0; n = 0;
        vid[0] = -100000; vid[1] = -100000; vid[2] = -100000; vid[3] = -100000;
        eid[0] = -100000; eid[1] = -100000; eid[2] = -100000; eid[3] = -100000;
    }
};

static inline void vaddto(vector4& a, const vector4& o) { a.x += o.x; a.y += o.y; a.z += o.z; }

// A vertex record as add_quad and finish default-construct it: only the flag is set.
struct QVert : MVert {
    QVert() { flag = 0; }
};

// @ 0x4d1410
bool EditMesh::build_first_pass()
{
    // exactly two selected vertices, otherwise nothing happens (returns false)
    int reached = 0;
    for (unsigned i = 0; i < verts.size(); i++)
        if (verts[i].flag)
            reached++;
    if (reached != 2)
        return false;

    bool built = false;
    for (unsigned i = 0; i < faces.size(); i++) {
        // the face must hold exactly two selected corners ...
        int hits = 0;
        for (int j = 0; j < faces[i].n; j++)
            if (verts[faces[i].vid[j]].flag)
                hits++;
        if (hits != 2)
            continue;
        // ... and none of its edges may join two selected vertices
        int joined = 0;
        for (int j = 0; j < faces[i].n; j++) {
            if (verts[edges[faces[i].eid[j]].v0].flag && verts[edges[faces[i].eid[j]].v1].flag)
                joined = 1;
        }
        if (joined)
            continue;

        // split the face along a diagonal into two triangles: faces[i] keeps the first, nf gets the second
        PartFace nf;
        nf.vid[3] = -100000;
        nf.eid[3] = -100000;
        faces[i].n = 3;
        nf.n = 3;
        PartEdge e;
        int a0;
        if (verts[faces[i].vid[0]].flag) {
            nf.vid[1] = faces[i].vid[2];
            e.v0 = faces[i].vid[0];
            a0 = faces[i].vid[0];
            e.v1 = faces[i].vid[2];
            nf.vid[2] = faces[i].vid[3];
            nf.vid[0] = a0;
            nf.eid[0] = edges.size();
            nf.eid[1] = faces[i].eid[2];
            nf.eid[2] = faces[i].eid[3];
            faces[i].eid[2] = edges.size();
        } else {
            nf.vid[1] = faces[i].vid[2];
            e.v0 = faces[i].vid[1];
            a0 = faces[i].vid[1];
            e.v1 = faces[i].vid[3];
            nf.vid[2] = faces[i].vid[3];
            nf.eid[0] = faces[i].eid[1];
            nf.vid[0] = a0;
            nf.eid[1] = faces[i].eid[2];
            nf.eid[2] = edges.size();
            faces[i].vid[2] = faces[i].vid[3];
            faces[i].eid[1] = edges.size();
            faces[i].eid[2] = faces[i].eid[3];
        }

        // plane normal of the new triangle: (p1 - p0) x (p2 - p0), normalised ((0,1,0) when degenerate)
        nf.normal = vcross(verts[nf.vid[1]].pos - verts[a0].pos, verts[nf.vid[2]].pos - verts[a0].pos);
        float len = (float)sqrt(nf.normal.x * nf.normal.x + nf.normal.y * nf.normal.y + nf.normal.z * nf.normal.z);
        if (len > 1e-10f) {
            len = 1.0f / len;
            nf.normal.x *= len;
            nf.normal.y *= len;
            nf.normal.z *= len;
        } else {
            nf.normal.x = 0.0f;
            nf.normal.y = 1.0f;
            nf.normal.z = 0.0f;
        }

        e.face[0] = (int)i;
        e.face[1] = faces.size();
        edges.push_back(e);
        faces.push_back(nf);
        build_adjacency();
        built = true;
    }
    return built;
}

namespace rgl { float vector4::normalize()
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
} }
// An edge's two endpoints as the array the original indexes.
struct EV { int v[2]; };

// Local copies of 0x4c5f60 and 0x4c8000 (owned by rgl/mesh2/gui_edit.cpp); see the note on update_terrain below.
void graph_obj::find_triangle(const int* e, int* a, int* b)
{
    int ei = *e;
    graph_face* f = &faces[edges[ei].f0];
    int idx = -1;
    for (int k = 0; k < f->count; k++) {
        if (ei == f->ends[k]) {
            idx = k;
            break;
        }
    }
    *a = f->verts[idx];
    graph_face* g = &faces[edges[*e].f0];
    *b = g->verts[(idx + 1) % g->count];
}
rgl::vector4 graph_obj::normal(const int* e)
{
    int a, b;
    find_triangle(e, &a, &b);
    node* w = &nodes[a];
    node* v = &nodes[b];
    float dx = v->x - w->x;
    float dy = v->y - w->y;
    float dz = v->z - w->z;
    float* m = (float*)&faces[(*(std::vector<edge>*)((char*)this + 0x38))[*e].f0];   // the edge through vector::operator[] (its reference is materialised)
    rgl::vector4 res;
    res.x = dy * m[8] - dz * m[7];
    res.y = dz * m[6] - dx * m[8];
    res.z = dx * m[7] - dy * m[6];
    res.normalize();
    return res;
}

// @ 0x4d1880
void EditMesh::build_second_pass()
{
    graph(this)->calc_edge_mode(1);
    *(int*)((char*)this + 0x7c) = -1;

    for (unsigned i = 0; i < edges.size(); i++)
        edges[i].flag = 0;

    for (int i = 0; (unsigned)i < edges.size(); i++) {
        if (!edge_word(edges[i]))
            continue;
        if (edges[i].nfaces != 1)
            continue;

        int b[2] = { -1, -1 };      // the neighbouring edge / the new edge
        int a[2] = { -1, -1 };      // far vertex of that edge / the new vertex
        for (int k = 0; k < 2; k++) {
            int count = 1;
            // &edges[0] plus a raw index: cl folds i*0x34 + k*4 into an IV of its own, unlike operator[] (materialised element)
            MVert* pv = &verts[((EV&)(&edges[0])[i]).v[k]];
            vector4 sum = graph(this)->normal(&i) + pv->pos;
            for (int j = 0; (unsigned)j < edges.size(); j++) {
                if (edges[j].v0 != ((EV&)edges[i]).v[k] && edges[j].v1 != ((EV&)edges[i]).v[k])
                    continue;
                if (i == j)
                    continue;
                if (edges[j].nfaces >= 2)
                    continue;
                if (edges[j].nfaces == 0) {
                    b[k] = j;
                    if (edges[j].v0 == ((EV&)edges[i]).v[k])
                        a[k] = edges[j].v1;
                    else
                        a[k] = edges[j].v0;
                    continue;
                }
                if (!edge_word(edges[j]))
                    continue;
                MVert* q = &verts[((EV&)(&edges[0])[i]).v[k]];
                vector4 nj = graph(this)->normal(&j) + q->pos;
                count++;
                vaddto(sum, nj);
            }
            if (b[k] < 0) {
                b[k] = (int)edges.size();
                a[k] = (int)verts.size();
                MEdge ne;
                ne.v0 = ((EV&)edges[i]).v[k];
                QVert nv;
                nv.pos.x = sum.x * (1.0f / (float)count);
                nv.pos.y = sum.y * (1.0f / (float)count);
                nv.pos.z = sum.z * (1.0f / (float)count);
                ne.v1 = (int)verts.size();
                verts.push_back(nv);
                edges.push_back(ne);
            }
        }

        int p, q;
        PartFace nf;
        nf.n = 4;
        graph(this)->find_triangle(&i, &p, &q);
        nf.eid[0] = i;
        nf.eid[2] = (int)edges.size();
        nf.vid[0] = q;
        nf.vid[1] = p;
        if (p == edges[i].v0) {
            nf.eid[1] = b[0];
            nf.eid[3] = b[1];
            nf.vid[2] = a[0];
            nf.vid[3] = a[1];
        } else {
            nf.eid[1] = b[1];
            nf.eid[3] = b[0];
            nf.vid[2] = a[1];
            nf.vid[3] = a[0];
        }
        MEdge ne2;
        ne2.v0 = a[0];
        ne2.v1 = a[1];
        edges.push_back(ne2);
        faces.push_back(nf);
        edges[edges.size() - 1].flag = 1;
    }
    build_adjacency();
}

// @ 0x4d1d80
bool EditMesh::add_triangle(int* tri)
{
    std::vector<int> found;
    for (int i = 0; i < faces.size(); i++) {
        int cnt = 0;
        for (int j = 0; j < faces[i].n; j++) {
            int v = faces[i].vid[j];
            if (v == tri[0]) cnt++;
            if (v == tri[1]) cnt++;
            if (v == tri[2]) cnt++;
        }
        if (cnt > 0)
            found.push_back(i);
        if (cnt > 2)
            return false;
    }
    Vec4 n(0.0f, 0.0f, 0.0f);
    for (int k = 0; k < found.size(); k++) {
        vaddto(n, faces[found[k]].normal);
    }
    n.x /= (float)found.size();
    n.y /= (float)found.size();
    n.z /= (float)found.size();
    n.normalize_inl();
    vector4 d1 = verts[tri[2]].pos - verts[tri[0]].pos;
    vector4 d2 = verts[tri[1]].pos - verts[tri[0]].pos;
    vector4 cr = vcross(d2, d1);
    if (cr.z * n.z + cr.y * n.y + cr.x * n.x < 0.0f) {     // faces away from the neighbours' normal
        int t = tri[0];
        tri[0] = tri[2];
        tri[2] = t;
    }
    int eid[3] = {-1, -1, -1};
    for (int i = 0; i < edges.size(); i++) {
        for (int j = 0; j < 3; j++) {
            if ((edges[i].v0 == tri[j] && edges[i].v1 == tri[(j + 1) % 3]) ||
                (edges[i].v1 == tri[j] && edges[i].v0 == tri[(j + 1) % 3])) {
                eid[j] = i;
                if (edges[i].nfaces == 2)
                    return false;
            }
        }
    }
    EFace face;
    face.n = 3;
    for (int j = 0; j < 3; j++) {
        face.vid[j] = tri[j];
        if (eid[j] == -1) {
            MEdge e;
            e.v0 = tri[j];
            e.v1 = tri[(j + 1) % 3];
            eid[j] = (int)edges.size();
            edges.push_back(e);
        }
        face.eid[j] = eid[j];
    }
    faces.push_back(face);
    build_adjacency();
    return true;
}

// A local copy of 0x4c7f90 (owned by rgl/mesh2/gui_edit.cpp): the original compiled this editor code in the same object, so finish()
// sees that its direction argument reaches no opaque callee: the accumulated normal stays in x87 registers (see codegen.md section 7).
void graph_obj::update_terrain(int idx, const float* d)
{
    calc_vertex_mode(idx);
    for (unsigned i = 0, off = 0;; i++, off += 0x18) {
        char* base = *(char**)((char*)this + 0x1c);
        if (!base)
            break;
        unsigned count = (unsigned)((*(char**)((char*)this + 0x20) - base) / 0x18);
        if (i >= count)
            break;
        float* p = (float*)(*(char**)((char*)this + 0x1c) + off);
        if (((int*)p)[5] != 0) {
            p[0] += d[0];
            p[1] += d[1];
            p[2] += d[2];
        }
    }
}
// @ 0x4d22d0
void EditMesh::finish()
{
    for (unsigned i = 0; i < edges.size(); i++)
        edge_word(edges[i]) = 0;
    Vec4 n(0.0f, 0.0f, 0.0f);
    int cnt = 0;
    for (unsigned i = 0; i < faces.size(); i++) {
        if (faces[i].f60) {
            cnt++;
            vaddto(n, faces[i].normal);
            for (int j = 0; j < faces[i].n; j++)
                edge_word(edges[faces[i].eid[j]])++;
        }
    }
    if (cnt == 0)
        return;
    int nv = (int)verts.size();
    for (unsigned i = 0; i < edges.size(); i++) {
        if (edge_word(edges[i]) == 1) {
            MEdge e;
            e.v0 = edges[i].v0;
            e.v1 = edges[i].v1;
            for (int j = 0; j < 2; j++) {
                int found = -1;
                for (unsigned k = nv; k < verts.size(); k++) {
                    if (verts[k].pad14 == (&edges[i].v0)[j]) {
                        found = (int)k;
                        break;
                    }
                }
                if (found == -1) {                      // no copy yet: append one
                    QVert v;                            // the pad after flag is left as the stack had it
                    v.pos = verts[(&edges[i].v0)[j]].pos;
                    v.pad14 = (&edges[i].v0)[j];
                    found = (int)verts.size();          // the index the copy gets (read before the push)
                    verts.push_back(v);
                }
                (&e.v0)[j] = found;
            }
            for (unsigned f = 0; f < faces.size(); f++) {
                for (int j = 0; j < faces[f].n; j++) {
                    if (faces[f].f60) {
                        if (faces[f].eid[j] == (int)i)
                            faces[f].eid[j] = (int)edges.size();
                        if (faces[f].vid[j] == verts[e.v0].pad14)
                            faces[f].vid[j] = e.v0;
                        else if (faces[f].vid[j] == verts[e.v1].pad14)
                            faces[f].vid[j] = e.v1;
                    }
                }
            }
            edges.push_back(e);
        }
    }
    for (unsigned i = 0; i < edges.size(); i++) {
        if (edge_word(edges[i]) > 1) {
            for (unsigned k = nv; k < verts.size(); k++) {
                if (edges[i].v0 == verts[k].pad14)
                    edges[i].v0 = k;
                if (edges[i].v1 == verts[k].pad14)
                    edges[i].v1 = k;
            }
        }
    }
    build_adjacency();
    float inv = 1.0f / (float)cnt;
    n.x = n.x * inv; n.y = n.y * inv; n.z = n.z * inv;
    n.normalize_inl();
    graph(this)->update_terrain(2, (const float*)&n);
    graph(this)->clean();
}

// @ 0x4d27d0
bool EditMesh::split_marked_edge()
{
    int mark = -1;
    int cnt = 0;
    for (unsigned i = 0; i < edges.size(); i++) {
        if (edges[i].flag) {
            mark = (int)i;
            cnt++;
        }
    }
    if (cnt != 1)
        return false;
    MVert v;
    v.flag = 0;
    int split = (int)edges.size();        // index the copy of the marked edge will get
    vector4 mid = verts[edges[mark].v0].pos + verts[edges[mark].v1].pos;
    v.pos = mid * 0.5f;
    MEdge ne;
    ne.v0 = edges[mark].v0;
    ne.v1 = (int)verts.size();
    ne.flag = 1;
    edges.push_back(ne);
    for (int slot = 0; slot < edges[mark].nfaces; slot++) {
        int last = split;                       // 0x4d2987: reset to the split edge for every slot, then follows each new edge
        int n = faces[edges[mark].face[slot]].n;
        int corners[4];
        int nc = 0;
        for (int j = 0; j < n; j++) {
            MEdge& em = edges[mark];
            if (faces[edges[mark].face[slot]].vid[j] != em.v0 &&
                faces[edges[mark].face[slot]].vid[j] != em.v1) {
                corners[nc] = j;
                nc++;
            }
        }
        if (nc == 2 && abs(corners[0] - corners[1]) != 1) {
            int t = corners[0];
            corners[0] = corners[1];
            corners[1] = t;
        }
        for (int k = 0; k < nc; k++) {
            EFace nf;
            nf.n = 3;
            nf.vid[0] = faces[edges[mark].face[slot]].vid[(corners[k] + n - 1) % n];
            nf.vid[1] = faces[edges[mark].face[slot]].vid[corners[k]];
            nf.vid[2] = (int)verts.size();
            nf.eid[0] = faces[edges[mark].face[slot]].eid[(corners[k] + n - 1) % n];
            if ((nf.vid[2] == edges[last].v0 && nf.vid[0] == edges[last].v1) ||
                (nf.vid[2] == edges[last].v1 && nf.vid[0] == edges[last].v0))
                nf.eid[2] = last;
            else
                nf.eid[2] = mark;
            last = (int)edges.size();                   // 0x4d2b72: the size read before the push
            nf.eid[1] = (int)edges.size();
            MEdge e2;
            e2.v0 = nf.vid[1];
            e2.v1 = nf.vid[2];
            edges.push_back(e2);
            faces.push_back(nf);
        }
        // nc == 0 (no real face): the original reads corners[-1], the slot holding n (0x4d2c10)
        EFace f2;
        f2.n = 3;
        f2.vid[0] = faces[edges[mark].face[slot]].vid[corners[nc - 1]];
        f2.vid[1] = faces[edges[mark].face[slot]].vid[(corners[nc - 1] + 1) % n];
        f2.vid[2] = (int)verts.size();
        f2.eid[0] = faces[edges[mark].face[slot]].eid[corners[nc - 1]];
        if ((f2.vid[1] == edges[split].v0 && f2.vid[2] == edges[split].v1) ||
            (f2.vid[1] == edges[split].v1 && f2.vid[2] == edges[split].v0))
            f2.eid[1] = split;
        else
            f2.eid[1] = mark;
        f2.eid[2] = last;
        faces[edges[mark].face[slot]].n = f2.n;
        faces[edges[mark].face[slot]].vid[0] = f2.vid[0];
        faces[edges[mark].face[slot]].vid[1] = f2.vid[1];
        faces[edges[mark].face[slot]].vid[2] = f2.vid[2];
        faces[edges[mark].face[slot]].eid[0] = f2.eid[0];
        faces[edges[mark].face[slot]].eid[1] = f2.eid[1];
        faces[edges[mark].face[slot]].eid[2] = f2.eid[2];
    }
    edges[mark].v0 = (int)verts.size();
    verts.push_back(v);
    build_adjacency();
    return true;
}

// @ 0x4d2df0
// Appends a 6 x 6 quad around p: four vertices, four edges, one face, then rebuilds the adjacency.
void EditMesh::add_quad(const vector4* p)
{
    PartFace face;
    QVert a, b, cv, d;
    MEdge e0, e1, e2, e3;
    float x0, y0, z, x1, y1;
    { vector4 t; x0 = p->x - 3.0f; t.x = x0; y0 = p->y - 3.0f; t.y = y0; z = p->z; t.z = z; a.pos = t; }
    { vector4 t; x1 = p->x + 3.0f; t.x = x1; t.y = y0; t.z = z; b.pos = t; }
    { vector4 t; t.x = x1; y1 = p->y + 3.0f; t.y = y1; t.z = z; cv.pos = t; }
    { vector4 t; t.x = x0; t.y = y1; t.z = z; d.pos = t; }

    e0.v0 = verts.size(); e0.v1 = verts.size() + 1;
    e1.v0 = verts.size() + 1; e1.v1 = verts.size() + 2;
    e2.v0 = verts.size() + 2; e2.v1 = verts.size() + 3;
    e3.v0 = verts.size() + 3; e3.v1 = verts.size();

    face.vid[0] = verts.size();
    face.vid[1] = verts.size() + 1;
    face.vid[2] = verts.size() + 2;
    face.vid[3] = verts.size() + 3;
    face.eid[0] = edges.size();
    face.eid[1] = edges.size() + 1;
    face.eid[2] = edges.size() + 2;
    face.eid[3] = edges.size() + 3;
    face.n = 4;

    verts.push_back(a);
    verts.push_back(b);
    verts.push_back(cv);
    verts.push_back(d);
    edges.push_back(e0);
    edges.push_back(e1);
    edges.push_back(e2);
    edges.push_back(e3);
    faces.push_back(face);
    build_adjacency();
}
