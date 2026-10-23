// Slice s004d1410: two operations of the mesh editor (editor actions 0x4a7 and 0x49b in s004fe860). Names are
// provisional; types and layout are in s004d1410.h.
//   0x4d1410 build_first_pass : with exactly two selected vertices, split every face that has both of them as
//            corners (and no selected-to-selected edge) along the diagonal into two triangles.
//   0x4d1880 build_second_pass: extrude every selected boundary edge: add a vertex per endpoint (unless a loose
//            edge already ends there) at the average of vertex + edge normal, then a quad and its new top edge.
// Both are complete; neither is byte-exact (see nonmatching.txt). The 4th lane of the triangle normal copied
// into Face::normal is uninitialised stack in both builds (vector4 temporary), so it is not comparable.
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
#include "s004d1410.h"

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
bool Mesh::build_first_pass()
{
    // exactly two selected vertices, otherwise nothing happens (returns false)
    int reached = 0;
    for (unsigned i = 0; i < verts.size(); i++)
        if (verts[i].sel)
            reached++;
    if (reached != 2)
        return false;

    bool built = false;
    for (unsigned i = 0; i < faces.size(); i++) {
        // the face must hold exactly two selected corners ...
        int hits = 0;
        for (int j = 0; j < faces[i].n; j++)
            if (verts[faces[i].vid[j]].sel)
                hits++;
        if (hits != 2)
            continue;
        // ... and none of its edges may join two selected vertices
        int joined = 0;
        for (int j = 0; j < faces[i].n; j++) {
            if (verts[edges[faces[i].eid[j]].v[0]].sel && verts[edges[faces[i].eid[j]].v[1]].sel)
                joined = 1;
        }
        if (joined)
            continue;

        // split the face along a diagonal into two triangles: faces[i] keeps the first, nf gets the second
        Face nf;
        nf.vid[3] = -100000;
        nf.eid[3] = -100000;
        faces[i].n = 3;
        nf.n = 3;
        Edge e;
        if (verts[faces[i].vid[0]].sel) {
            nf.vid[1] = faces[i].vid[2];
            nf.vid[0] = faces[i].vid[0];
            e.v[1] = faces[i].vid[2];
            e.v[0] = faces[i].vid[0];
            nf.eid[0] = edges.size();
            nf.eid[1] = faces[i].eid[2];
            nf.vid[2] = faces[i].vid[3];
            nf.eid[2] = faces[i].eid[3];
            faces[i].eid[2] = edges.size();
        } else {
            nf.vid[1] = faces[i].vid[2];
            nf.eid[0] = faces[i].eid[1];
            e.v[1] = faces[i].vid[3];
            e.v[0] = faces[i].vid[1];
            nf.vid[0] = faces[i].vid[1];
            nf.eid[1] = faces[i].eid[2];
            nf.vid[2] = faces[i].vid[3];
            nf.eid[2] = edges.size();
            faces[i].vid[2] = faces[i].vid[3];
            faces[i].eid[1] = edges.size();
            faces[i].eid[2] = faces[i].eid[3];
        }

        // plane normal of the new triangle: (p1 - p0) x (p2 - p0), normalised ((0,1,0) when degenerate)
        Vtx* p0 = &verts[nf.vid[0]];
        Vtx* p1 = &verts[nf.vid[1]];
        Vtx* p2 = &verts[nf.vid[2]];
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
        e.face[1] = faces.size();
        edges.push_back(e);
        faces.push_back(nf);
        build_adjacency();
        built = true;
    }
    return built;
}

// @ 0x4d1880
void Mesh::build_second_pass()
{
    calc_edge_mode(1);
    f7c = -1;

    for (unsigned i = 0; i < edges.size(); i++)
        edges[i].sel = 0;

    for (int i = 0; (unsigned)i < edges.size(); i++) {
        if (!edges[i].sel2)
            continue;
        if (edges[i].nfaces != 1)
            continue;

        int a[2] = { -1, -1 };      // far vertex of the neighbouring edge / the new vertex
        int b[2] = { -1, -1 };      // that edge / the new edge
        for (int k = 0; k < 2; k++) {
            int vi = edges[i].v[k];
            Vtx* pv = &verts[vi];
            int count = 1;
            vector4 sum;
            {
                vector4 nn = normal(&i);
                sum.x = pv->pos.x + nn.x;
                sum.y = nn.y + pv->pos.y;
                sum.z = nn.z + pv->pos.z;
            }
            for (int j = 0; (unsigned)j < edges.size(); j++) {
                Edge& g = edges[j];
                if (g.v[0] != vi && g.v[1] != vi)
                    continue;
                if (i == j)
                    continue;
                if (g.nfaces >= 2)
                    continue;
                if (g.nfaces == 0) {
                    b[k] = j;
                    if (g.v[0] == vi)
                        a[k] = g.v[1];
                    else
                        a[k] = g.v[0];
                    continue;
                }
                if (!g.sel2)
                    continue;
                Vtx* q = &verts[edges[i].v[k]];
                vector4 nj = normal(&j);
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
                Vtx nv(sum.x * inv, sum.y * inv, sum.z * inv);
                Edge ne(vi, (int)verts.size());
                verts.push_back(nv);
                edges.push_back(ne);
            }
        }

        int p, q;
        Face nf;
        nf.n = 4;
        find_triangle(&i, &p, &q);
        nf.eid[2] = (int)edges.size();
        nf.vid[0] = q;
        nf.vid[1] = p;
        nf.eid[0] = i;
        if (p == edges[i].v[0]) {
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
        Edge ne2(a[0], a[1]);
        edges.push_back(ne2);
        faces.push_back(nf);
        edges[edges.size() - 1].sel = 1;
    }
    build_adjacency();
}
