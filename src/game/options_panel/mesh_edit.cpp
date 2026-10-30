// mesh_edit.cpp: the polygon-mesh editor operations (0x4d1410..0x4d2df0): split-quad, extrude-boundary-edge,
// add-triangle, finish (duplicate the boundary vertices of the marked faces) and split-marked-edge, on the
// canonical PolyMesh through mesh_edit.h. Names provisional.
#include <windows.h>
#include <stdlib.h>
#include "./mesh_edit.h"

// The vector arithmetic is inlined in the original (rgl's out-of-line operator- / cross are not called).
static inline vector4 vsub(const vector4& a, const vector4& b)
{
    return vector4(a.x - b.x, a.y - b.y, a.z - b.z);
}
static inline vector4 vcross(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// @ 0x4d1410
bool EditMesh::build_first_pass()
{
    // exactly two selected vertices, otherwise nothing happens (returns false)
    int reached = 0;
    for (unsigned i = 0; i < c.verts.size(); i++)
        if (c.verts[i].flag)
            reached++;
    if (reached != 2)
        return false;

    bool built = false;
    for (unsigned i = 0; i < c.faces.size(); i++) {
        // the face must hold exactly two selected corners ...
        int hits = 0;
        for (int j = 0; j < c.faces[i].n; j++)
            if (c.verts[c.faces[i].vid[j]].flag)
                hits++;
        if (hits != 2)
            continue;
        // ... and none of its c.edges may join two selected vertices
        int joined = 0;
        for (int j = 0; j < c.faces[i].n; j++) {
            if (c.verts[c.edges[c.faces[i].eid[j]].v0].flag && c.verts[c.edges[c.faces[i].eid[j]].v1].flag)
                joined = 1;
        }
        if (joined)
            continue;

        // split the face along a diagonal into two triangles: c.faces[i] keeps the first, nf gets the second
        PartFace nf;
        nf.vid[3] = -100000;
        nf.eid[3] = -100000;
        c.faces[i].n = 3;
        nf.n = 3;
        PartEdge e;
        if (c.verts[c.faces[i].vid[0]].flag) {
            nf.vid[1] = c.faces[i].vid[2];
            nf.vid[0] = c.faces[i].vid[0];
            e.v1 = c.faces[i].vid[2];
            e.v0 = c.faces[i].vid[0];
            nf.eid[0] = c.edges.size();
            nf.eid[1] = c.faces[i].eid[2];
            nf.vid[2] = c.faces[i].vid[3];
            nf.eid[2] = c.faces[i].eid[3];
            c.faces[i].eid[2] = c.edges.size();
        } else {
            nf.vid[1] = c.faces[i].vid[2];
            nf.eid[0] = c.faces[i].eid[1];
            e.v1 = c.faces[i].vid[3];
            e.v0 = c.faces[i].vid[1];
            nf.vid[0] = c.faces[i].vid[1];
            nf.eid[1] = c.faces[i].eid[2];
            nf.vid[2] = c.faces[i].vid[3];
            nf.eid[2] = c.edges.size();
            c.faces[i].vid[2] = c.faces[i].vid[3];
            c.faces[i].eid[1] = c.edges.size();
            c.faces[i].eid[2] = c.faces[i].eid[3];
        }

        // plane normal of the new triangle: (p1 - p0) x (p2 - p0), normalised ((0,1,0) when degenerate)
        MVert* p0 = &c.verts[nf.vid[0]];
        MVert* p1 = &c.verts[nf.vid[1]];
        MVert* p2 = &c.verts[nf.vid[2]];
        nf.normal = vcross(vsub(p1->pos, p0->pos), vsub(p2->pos, p0->pos));
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
        e.face[1] = c.faces.size();
        c.edges.push_back(e);
        c.faces.push_back(nf);
        build_adjacency();
        built = true;
    }
    return built;
}

// @ 0x4d1880
void EditMesh::build_second_pass()
{
    graph(this)->calc_edge_mode(1);
    *(int*)((char*)this + 0x7c) = -1;

    for (unsigned i = 0; i < c.edges.size(); i++)
        c.edges[i].flag = 0;

    for (int i = 0; (unsigned)i < c.edges.size(); i++) {
        if (!edge_word(c.edges[i]))
            continue;
        if (c.edges[i].nfaces != 1)
            continue;

        int a[2] = { -1, -1 };      // far vertex of the neighbouring edge / the new vertex
        int b[2] = { -1, -1 };      // that edge / the new edge
        for (int k = 0; k < 2; k++) {
            int vi = (&c.edges[i].v0)[k];
            MVert* pv = &c.verts[vi];
            int count = 1;
            vector4 sum;
            {
                vector4 nn = graph(this)->normal(&i);
                sum.x = pv->pos.x + nn.x;
                sum.y = nn.y + pv->pos.y;
                sum.z = nn.z + pv->pos.z;
            }
            for (int j = 0; (unsigned)j < c.edges.size(); j++) {
                MEdge& g = c.edges[j];
                if (g.v0 != vi && g.v1 != vi)
                    continue;
                if (i == j)
                    continue;
                if (g.nfaces >= 2)
                    continue;
                if (g.nfaces == 0) {
                    b[k] = j;
                    if (g.v0 == vi)
                        a[k] = g.v1;
                    else
                        a[k] = g.v0;
                    continue;
                }
                if (!edge_word(g))
                    continue;
                MVert* q = &c.verts[(&c.edges[i].v0)[k]];
                vector4 nj = graph(this)->normal(&j);
                count++;
                float tx = nj.x + q->pos.x, ty = nj.y + q->pos.y, tz = nj.z + q->pos.z;
                sum.x += tx;
                sum.y += ty;
                sum.z += tz;
            }
            if (b[k] < 0) {
                b[k] = (int)c.edges.size();
                a[k] = (int)c.verts.size();
                float inv = 1.0f / (float)count;
                NewVert nv(sum.x * inv, sum.y * inv, sum.z * inv);
                NewEdge ne(vi, (int)c.verts.size());
                c.verts.push_back(nv);
                c.edges.push_back(ne);
            }
        }

        int p, q;
        PartFace nf;
        nf.n = 4;
        graph(this)->find_triangle(&i, &p, &q);
        nf.eid[2] = (int)c.edges.size();
        nf.vid[0] = q;
        nf.vid[1] = p;
        nf.eid[0] = i;
        if (p == c.edges[i].v0) {
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
        NewEdge ne2(a[0], a[1]);
        c.edges.push_back(ne2);
        c.faces.push_back(nf);
        c.edges[c.edges.size() - 1].flag = 1;
    }
    build_adjacency();
}

// @ 0x4d1d80
bool EditMesh::add_triangle(int* tri)
{
    OwnedIntVec found;
    for (unsigned i = 0; i < c.faces.size(); i++) {
        int cnt = 0;
        for (int j = 0; j < vat(c.faces, i).n; j++) {
            int v = vat(c.faces, i).vid[j];
            if (v == tri[0]) cnt++;
            if (v == tri[1]) cnt++;
            if (v == tri[2]) cnt++;
        }
        if (cnt > 0)
            found.push_back((int)i);
        if (cnt > 2)
            return false;
    }
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    for (unsigned i = 0; i < found.size(); i++) {
        nx = nx + vat(c.faces, found[i]).normal.x;
        ny = ny + vat(c.faces, found[i]).normal.y;
        nz = nz + vat(c.faces, found[i]).normal.z;
    }
    Vec4 nrm(nx / (float)found.size(), ny / (float)found.size(), nz / (float)found.size());
    nrm.normalize_inl();
    float d1x = vat(c.verts, tri[2]).pos.x - vat(c.verts, tri[0]).pos.x;
    float d1y = vat(c.verts, tri[2]).pos.y - vat(c.verts, tri[0]).pos.y;
    float d1z = vat(c.verts, tri[2]).pos.z - vat(c.verts, tri[0]).pos.z;
    volatile float d2x = vat(c.verts, tri[1]).pos.x - vat(c.verts, tri[0]).pos.x;     // the original spills these three
    volatile float d2y = vat(c.verts, tri[1]).pos.y - vat(c.verts, tri[0]).pos.y;     // and the first two cross
    volatile float d2z = vat(c.verts, tri[1]).pos.z - vat(c.verts, tri[0]).pos.z;     // components as floats (rounded),
    volatile float cx = d2y * d1z - d2z * d1y;                                        // while d1 and cz stay in x87 registers
    volatile float cy = d2z * d1x - d2x * d1z;
    float cz = d2x * d1y - d2y * d1x;
    if (cz * nrm.z + cy * nrm.y + cx * nrm.x < 0.0f) {     // faces away from the neighbours' normal
        int t = tri[0];
        tri[0] = tri[2];
        tri[2] = t;
    }
    int eid[3] = {-1, -1, -1};
    for (unsigned i = 0; i < c.edges.size(); i++) {
        for (int j = 0; j < 3; j++) {
            MEdge& e = vat(c.edges, i);
            if ((e.v0 == tri[j] && e.v1 == tri[(j + 1) % 3]) ||
                (e.v1 == tri[j] && e.v0 == tri[(j + 1) % 3])) {
                eid[j] = (int)i;
                if (e.nfaces == 2)
                    return false;
            }
        }
    }
    MPoly face;
    face.n = 3;
    for (int j = 0; j < 3; j++) {
        face.vid[j] = tri[j];
        if (eid[j] == -1) {
            eid[j] = (int)c.edges.size();
            NewEdge e(tri[j], tri[(j + 1) % 3]);
            c.edges.push_back(e);
        }
        face.eid[j] = eid[j];
    }
    c.faces.push_back(face);
    build_adjacency();
    return true;
}

// @ 0x4d22d0
void EditMesh::finish()
{
    for (unsigned i = 0; i < c.edges.size(); i++)
        edge_word(vat(c.edges, i)) = 0;
    Vec4 n(0.0f, 0.0f, 0.0f);
    int cnt = 0;
    for (unsigned i = 0; i < c.faces.size(); i++) {
        if (vat(c.faces, i).f60) {
            n.x = n.x + vat(c.faces, i).normal.x;
            cnt++;
            n.y = n.y + vat(c.faces, i).normal.y;
            n.z = n.z + vat(c.faces, i).normal.z;
            for (int j = 0; j < vat(c.faces, i).n; j++)
                edge_word(vat(c.edges, vat(c.faces, i).eid[j]))++;
        }
    }
    if (cnt == 0)
        return;
    int nv = (int)c.verts.size();
    for (unsigned i = 0; i < c.edges.size(); i++) {
        if (edge_word(vat(c.edges, i)) == 1) {
            NewEdge e(vat(c.edges, i).v0, vat(c.edges, i).v1);
            for (int j = 0; j < 2; j++) {
                int k = nv;
                while (k < (int)c.verts.size() && vat(c.verts, k).pad14 != (&e.v0)[j])
                    k++;
                if (k >= (int)c.verts.size()) {         // no copy yet: append one (the scan ran out: 0x4d24a3 jae)
                    MVert v;                            // the pad after flag is left as the stack had it
                    v.pos = vat(c.verts, (&e.v0)[j]).pos;
                    v.flag = 0;
                    v.pad14 = (&e.v0)[j];
                    (&e.v0)[j] = (int)c.verts.size();     // the index the copy gets (read before the push)
                    c.verts.push_back(v);
                } else {
                    (&e.v0)[j] = k;
                }
            }
            for (unsigned f = 0; f < c.faces.size(); f++) {
                for (int j = 0; j < vat(c.faces, f).n; j++) {
                    if (vat(c.faces, f).f60) {
                        if (vat(c.faces, f).eid[j] == (int)i)
                            vat(c.faces, f).eid[j] = (int)c.edges.size();
                        if (vat(c.faces, f).vid[j] == vat(c.verts, e.v0).pad14)
                            vat(c.faces, f).vid[j] = e.v0;
                        else if (vat(c.faces, f).vid[j] == vat(c.verts, e.v1).pad14)
                            vat(c.faces, f).vid[j] = e.v1;
                    }
                }
            }
            c.edges.push_back(e);
        }
    }
    for (unsigned i = 0; i < c.edges.size(); i++) {
        if (edge_word(vat(c.edges, i)) > 1) {
            for (int k = nv; k < (int)c.verts.size(); k++) {
                if (vat(c.edges, i).v0 == vat(c.verts, k).pad14)
                    vat(c.edges, i).v0 = k;
                if (vat(c.edges, i).v1 == vat(c.verts, k).pad14)
                    vat(c.edges, i).v1 = k;
            }
        }
    }
    build_adjacency();
    float inv = 1.0f / (float)cnt;
    n.x = n.x * inv;
    n.y = n.y * inv;
    n.z = n.z * inv;
    n.normalize_inl();
    graph(this)->update_terrain(2, (const float*)&n);
    graph(this)->clean();
}

// @ 0x4d27d0
bool EditMesh::split_marked_edge()
{
    int cnt = 0;
    int mark = -1;
    for (unsigned i = 0; i < c.edges.size(); i++) {
        if (vat(c.edges, i).flag) {
            cnt++;
            mark = (int)i;
        }
    }
    if (cnt != 1)
        return false;
    int split = (int)c.edges.size();        // index the copy of the marked edge will get
    MVert v;
    v.flag = 0;
    volatile float zsum = vat(c.verts, vat(c.edges, mark).v0).pos.z + vat(c.verts, vat(c.edges, mark).v1).pos.z;
    v.pos = vector4((vat(c.verts, vat(c.edges, mark).v0).pos.x + vat(c.verts, vat(c.edges, mark).v1).pos.x) * 0.5f,
                    (vat(c.verts, vat(c.edges, mark).v0).pos.y + vat(c.verts, vat(c.edges, mark).v1).pos.y) * 0.5f,
                    zsum * 0.5f);
    NewEdge ne(vat(c.edges, mark).v0, (int)c.verts.size());
    ne.flag = 1;
    c.edges.push_back(ne);
    for (int slot = 0; slot < vat(c.edges, mark).nfaces; slot++) {
        int fi = vat(c.edges, mark).face[slot];
        int n = vat(c.faces, fi).n;
        int corners[4];
        int nc = 0;
        for (int j = 0; j < n; j++) {
            int vi = vat(c.faces, fi).vid[j];
            if (vi != vat(c.edges, mark).v0 && vi != vat(c.edges, mark).v1)
                corners[nc++] = j;
        }
        if (nc == 2 && abs(corners[0] - corners[1]) != 1) {
            int t = corners[0];
            corners[0] = corners[1];
            corners[1] = t;
        }
        for (int k = 0; k < nc; k++) {
            MPoly nf;
            int cn = corners[k];
            nf.n = 3;
            nf.vid[0] = vat(c.faces, fi).vid[(cn + n - 1) % n];
            nf.vid[1] = vat(c.faces, fi).vid[cn];
            nf.vid[2] = (int)c.verts.size();
            nf.eid[0] = vat(c.faces, fi).eid[(cn + n - 1) % n];
            int last = (int)c.edges.size() - 1;         // the split edge, or the previous corner's edge
            if ((nf.vid[2] == vat(c.edges, last).v0 && nf.vid[0] == vat(c.edges, last).v1) ||
                (nf.vid[2] == vat(c.edges, last).v1 && nf.vid[0] == vat(c.edges, last).v0))
                nf.eid[2] = last;
            else
                nf.eid[2] = mark;
            nf.eid[1] = (int)c.edges.size();
            NewEdge e2(nf.vid[1], nf.vid[2]);
            c.edges.push_back(e2);
            c.faces.push_back(nf);
        }
        int cn = nc ? corners[nc - 1] : n;   // nc == 0 (no real face): the original reads corners[-1], the slot holding n (0x4d2c10)
        int v0 = vat(c.faces, fi).vid[cn];
        int v1 = vat(c.faces, fi).vid[(cn + 1) % n];
        int v2 = (int)c.verts.size();
        int e0 = vat(c.faces, fi).eid[cn];
        int e1;
        if ((v1 == vat(c.edges, split).v0 && v2 == vat(c.edges, split).v1) ||
            (v1 == vat(c.edges, split).v1 && v2 == vat(c.edges, split).v0))
            e1 = split;
        else
            e1 = mark;
        vat(c.faces, fi).n = 3;
        vat(c.faces, fi).vid[0] = v0;
        vat(c.faces, fi).vid[1] = v1;
        vat(c.faces, fi).vid[2] = v2;
        vat(c.faces, fi).eid[0] = e0;
        vat(c.faces, fi).eid[1] = e1;
        vat(c.faces, fi).eid[2] = (int)c.edges.size() - 1;
    }
    vat(c.edges, mark).v0 = (int)c.verts.size();
    c.verts.push_back(v);
    build_adjacency();
    return true;
}

// @ 0x4d2df0
// Appends a 6 x 6 quad around p: four vertices, four edges, one face, then rebuilds the adjacency.
void EditMesh::add_quad(const vector4* p)
{
    NewVert a(p->x - 3.0f, p->y - 3.0f, p->z);
    NewVert b(p->x + 3.0f, p->y - 3.0f, p->z);
    NewVert cv(p->x + 3.0f, p->y + 3.0f, p->z);
    NewVert d(p->x - 3.0f, p->y + 3.0f, p->z);
    QuadEdge e0, e1, e2, e3;
    PartFace face;

    e0.nfaces = 0; e0.flag = 0; e0.crease[0] = 1.0f; e0.crease[1] = 1.0f;
    e1.nfaces = 0; e1.flag = 0; e1.crease[0] = 1.0f; e1.crease[1] = 1.0f;
    e2.nfaces = 0; e2.flag = 0; e2.crease[0] = 1.0f; e2.crease[1] = 1.0f;
    e3.nfaces = 0; e3.flag = 0; e3.crease[0] = 1.0f; e3.crease[1] = 1.0f;

    e0.v0 = c.verts.size(); e0.v1 = c.verts.size() + 1;
    e1.v0 = c.verts.size() + 1; e1.v1 = c.verts.size() + 2;
    e2.v0 = c.verts.size() + 2; e2.v1 = c.verts.size() + 3;
    e3.v0 = c.verts.size() + 3; e3.v1 = c.verts.size();

    face.vid[0] = c.verts.size();
    face.vid[1] = c.verts.size() + 1;
    face.vid[2] = c.verts.size() + 2;
    face.vid[3] = c.verts.size() + 3;
    face.eid[0] = c.edges.size();
    face.eid[1] = c.edges.size() + 1;
    face.eid[2] = c.edges.size() + 2;
    face.eid[3] = c.edges.size() + 3;
    face.n = 4;

    c.verts.push_back(a);
    c.verts.push_back(b);
    c.verts.push_back(cv);
    c.verts.push_back(d);
    c.edges.push_back(e0);
    c.edges.push_back(e1);
    c.edges.push_back(e2);
    c.edges.push_back(e3);
    c.faces.push_back(face);
    build_adjacency();
}
