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
        if (verts[faces[i].vid[0]].flag) {
            nf.vid[1] = faces[i].vid[2];
            nf.vid[0] = faces[i].vid[0];
            e.v1 = faces[i].vid[2];
            e.v0 = faces[i].vid[0];
            nf.eid[0] = edges.size();
            nf.eid[1] = faces[i].eid[2];
            nf.vid[2] = faces[i].vid[3];
            nf.eid[2] = faces[i].eid[3];
            faces[i].eid[2] = edges.size();
        } else {
            nf.vid[1] = faces[i].vid[2];
            nf.eid[0] = faces[i].eid[1];
            e.v1 = faces[i].vid[3];
            e.v0 = faces[i].vid[1];
            nf.vid[0] = faces[i].vid[1];
            nf.eid[1] = faces[i].eid[2];
            nf.vid[2] = faces[i].vid[3];
            nf.eid[2] = edges.size();
            faces[i].vid[2] = faces[i].vid[3];
            faces[i].eid[1] = edges.size();
            faces[i].eid[2] = faces[i].eid[3];
        }

        // plane normal of the new triangle: (p1 - p0) x (p2 - p0), normalised ((0,1,0) when degenerate)
        MVert* p0 = &verts[nf.vid[0]];
        MVert* p1 = &verts[nf.vid[1]];
        MVert* p2 = &verts[nf.vid[2]];
        nf.normal = vcross(p1->pos - p0->pos, p2->pos - p0->pos);
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

        int a[2] = { -1, -1 };      // far vertex of the neighbouring edge / the new vertex
        int b[2] = { -1, -1 };      // that edge / the new edge
        for (int k = 0; k < 2; k++) {
            int vi = (&edges[i].v0)[k];
            MVert* pv = &verts[vi];
            int count = 1;
            vector4 sum;
            {
                vector4 nn = graph(this)->normal(&i);
                sum.x = pv->pos.x + nn.x;
                sum.y = nn.y + pv->pos.y;
                sum.z = nn.z + pv->pos.z;
            }
            for (int j = 0; (unsigned)j < edges.size(); j++) {
                MEdge& g = edges[j];
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
                MVert* q = &verts[(&edges[i].v0)[k]];
                vector4 nj = graph(this)->normal(&j);
                count++;
                float tx = nj.x + q->pos.x, ty = nj.y + q->pos.y, tz = nj.z + q->pos.z;
                sum.x += tx;
                sum.y += ty;
                sum.z += tz;
            }
            if (b[k] < 0) {
                b[k] = (int)edges.size();
                a[k] = (int)verts.size();
                float inv = 1.0f / (float)count;
                NewVert nv(sum.x * inv, sum.y * inv, sum.z * inv);
                NewEdge ne(vi, (int)verts.size());
                verts.push_back(nv);
                edges.push_back(ne);
            }
        }

        int p, q;
        PartFace nf;
        nf.n = 4;
        graph(this)->find_triangle(&i, &p, &q);
        nf.eid[2] = (int)edges.size();
        nf.vid[0] = q;
        nf.vid[1] = p;
        nf.eid[0] = i;
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
        NewEdge ne2(a[0], a[1]);
        edges.push_back(ne2);
        faces.push_back(nf);
        edges[edges.size() - 1].flag = 1;
    }
    build_adjacency();
}

// @ 0x4d1d80
bool EditMesh::add_triangle(int* tri)
{
    OwnedIntVec found;
    for (unsigned i = 0; i < faces.size(); i++) {
        int cnt = 0;
        for (int j = 0; j < vat(faces, i).n; j++) {
            int v = vat(faces, i).vid[j];
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
        nx = nx + vat(faces, found[i]).normal.x;
        ny = ny + vat(faces, found[i]).normal.y;
        nz = nz + vat(faces, found[i]).normal.z;
    }
    Vec4 nrm(nx / (float)found.size(), ny / (float)found.size(), nz / (float)found.size());
    nrm.normalize_inl();
    float d1x = vat(verts, tri[2]).pos.x - vat(verts, tri[0]).pos.x;
    float d1y = vat(verts, tri[2]).pos.y - vat(verts, tri[0]).pos.y;
    float d1z = vat(verts, tri[2]).pos.z - vat(verts, tri[0]).pos.z;
    Vec4 d2(vat(verts, tri[1]).pos.x - vat(verts, tri[0]).pos.x,       // vector locals live in memory as floats
            vat(verts, tri[1]).pos.y - vat(verts, tri[0]).pos.y,
            vat(verts, tri[1]).pos.z - vat(verts, tri[0]).pos.z);
    Vec4 cr;
    cr.x = d2.y * d1z - d2.z * d1y;
    cr.y = d2.z * d1x - d2.x * d1z;
    float cz = d2.x * d1y - d2.y * d1x;
    if (cz * nrm.z + cr.y * nrm.y + cr.x * nrm.x < 0.0f) {     // faces away from the neighbours' normal
        int t = tri[0];
        tri[0] = tri[2];
        tri[2] = t;
    }
    int eid[3] = {-1, -1, -1};
    for (unsigned i = 0; i < edges.size(); i++) {
        for (int j = 0; j < 3; j++) {
            MEdge& e = vat(edges, i);
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
            eid[j] = (int)edges.size();
            NewEdge e(tri[j], tri[(j + 1) % 3]);
            edges.push_back(e);
        }
        face.eid[j] = eid[j];
    }
    faces.push_back(face);
    build_adjacency();
    return true;
}

// @ 0x4d22d0
void EditMesh::finish()
{
    for (unsigned i = 0; i < edges.size(); i++)
        edge_word(vat(edges, i)) = 0;
    Vec4 n(0.0f, 0.0f, 0.0f);
    int cnt = 0;
    for (unsigned i = 0; i < faces.size(); i++) {
        if (vat(faces, i).f60) {
            n.x = n.x + vat(faces, i).normal.x;
            cnt++;
            n.y = n.y + vat(faces, i).normal.y;
            n.z = n.z + vat(faces, i).normal.z;
            for (int j = 0; j < vat(faces, i).n; j++)
                edge_word(vat(edges, vat(faces, i).eid[j]))++;
        }
    }
    if (cnt == 0)
        return;
    int nv = (int)verts.size();
    for (unsigned i = 0; i < edges.size(); i++) {
        if (edge_word(vat(edges, i)) == 1) {
            NewEdge e(vat(edges, i).v0, vat(edges, i).v1);
            for (int j = 0; j < 2; j++) {
                int k = nv;
                while (k < (int)verts.size() && vat(verts, k).pad14 != (&e.v0)[j])
                    k++;
                if (k >= (int)verts.size()) {         // no copy yet: append one (the scan ran out: 0x4d24a3 jae)
                    MVert v;                            // the pad after flag is left as the stack had it
                    v.pos = vat(verts, (&e.v0)[j]).pos;
                    v.flag = 0;
                    v.pad14 = (&e.v0)[j];
                    (&e.v0)[j] = (int)verts.size();     // the index the copy gets (read before the push)
                    verts.push_back(v);
                } else {
                    (&e.v0)[j] = k;
                }
            }
            for (unsigned f = 0; f < faces.size(); f++) {
                for (int j = 0; j < vat(faces, f).n; j++) {
                    if (vat(faces, f).f60) {
                        if (vat(faces, f).eid[j] == (int)i)
                            vat(faces, f).eid[j] = (int)edges.size();
                        if (vat(faces, f).vid[j] == vat(verts, e.v0).pad14)
                            vat(faces, f).vid[j] = e.v0;
                        else if (vat(faces, f).vid[j] == vat(verts, e.v1).pad14)
                            vat(faces, f).vid[j] = e.v1;
                    }
                }
            }
            edges.push_back(e);
        }
    }
    for (unsigned i = 0; i < edges.size(); i++) {
        if (edge_word(vat(edges, i)) > 1) {
            for (int k = nv; k < (int)verts.size(); k++) {
                if (vat(edges, i).v0 == vat(verts, k).pad14)
                    vat(edges, i).v0 = k;
                if (vat(edges, i).v1 == vat(verts, k).pad14)
                    vat(edges, i).v1 = k;
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
    for (unsigned i = 0; i < edges.size(); i++) {
        if (vat(edges, i).flag) {
            cnt++;
            mark = (int)i;
        }
    }
    if (cnt != 1)
        return false;
    int split = (int)edges.size();        // index the copy of the marked edge will get
    MVert v;
    v.flag = 0;
    v.pos = vector4((vat(verts, vat(edges, mark).v0).pos.x + vat(verts, vat(edges, mark).v1).pos.x) * 0.5f,
                    (vat(verts, vat(edges, mark).v0).pos.y + vat(verts, vat(edges, mark).v1).pos.y) * 0.5f,
                    vat(verts, vat(edges, mark).v0).pos.z + vat(verts, vat(edges, mark).v1).pos.z);
    v.pos.z *= 0.5f;                        // the z sum is stored as a float first, then halved in place
    NewEdge ne(vat(edges, mark).v0, (int)verts.size());
    ne.flag = 1;
    edges.push_back(ne);
    for (int slot = 0; slot < vat(edges, mark).nfaces; slot++) {
        int fi = vat(edges, mark).face[slot];
        int n = vat(faces, fi).n;
        int last = split;                       // 0x4d2987: reset to the split edge for every slot, then follows each new edge
        int corners[4];
        int nc = 0;
        for (int j = 0; j < n; j++) {
            int vi = vat(faces, fi).vid[j];
            if (vi != vat(edges, mark).v0 && vi != vat(edges, mark).v1)
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
            nf.vid[0] = vat(faces, fi).vid[(cn + n - 1) % n];
            nf.vid[1] = vat(faces, fi).vid[cn];
            nf.vid[2] = (int)verts.size();
            nf.eid[0] = vat(faces, fi).eid[(cn + n - 1) % n];
            if ((nf.vid[2] == vat(edges, last).v0 && nf.vid[0] == vat(edges, last).v1) ||
                (nf.vid[2] == vat(edges, last).v1 && nf.vid[0] == vat(edges, last).v0))
                nf.eid[2] = last;
            else
                nf.eid[2] = mark;
            nf.eid[1] = (int)edges.size();
            last = nf.eid[1];                           // 0x4d2b72: the size read before the push
            NewEdge e2(nf.vid[1], nf.vid[2]);
            edges.push_back(e2);
            faces.push_back(nf);
        }
        int cn = nc ? corners[nc - 1] : n;   // nc == 0 (no real face): the original reads corners[-1], the slot holding n (0x4d2c10)
        int v0 = vat(faces, fi).vid[cn];
        int v1 = vat(faces, fi).vid[(cn + 1) % n];
        int v2 = (int)verts.size();
        int e0 = vat(faces, fi).eid[cn];
        int e1;
        if ((v1 == vat(edges, split).v0 && v2 == vat(edges, split).v1) ||
            (v1 == vat(edges, split).v1 && v2 == vat(edges, split).v0))
            e1 = split;
        else
            e1 = mark;
        vat(faces, fi).n = 3;
        vat(faces, fi).vid[0] = v0;
        vat(faces, fi).vid[1] = v1;
        vat(faces, fi).vid[2] = v2;
        vat(faces, fi).eid[0] = e0;
        vat(faces, fi).eid[1] = e1;
        vat(faces, fi).eid[2] = last;             // 0x4d2d73
    }
    vat(edges, mark).v0 = (int)verts.size();
    verts.push_back(v);
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
