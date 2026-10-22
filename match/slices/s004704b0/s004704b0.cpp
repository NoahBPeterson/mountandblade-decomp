// Slice s004704b0: a polygon-mesh topology object (vertices, per-vertex records, faces, edges): bounding box,
// face normals, per-edge crease factors, face/edge adjacency, centroids, plus the sun-light camera frame
// (0x470c00) and a small UI forwarder (0x470d80). ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#include <math.h>
#include <stddef.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]

struct Pt24 { vector4 pos; char pad[0x14]; };                 // 0x24 bytes: position + attributes
struct Vtx18 { float x, y, z; float pad[3]; };                // 0x18 bytes
struct Face {                                                 // 0x64 bytes
    char pad0[8];
    vector4 centroid;                                         // 0x08
    vector4 normal;                                           // 0x18
    int vid[4];                                               // 0x28 vertex ids
    int eid[4];                                               // 0x38 edge ids
    int n;                                                    // 0x48 corner count
    int nbr[4];                                               // 0x4c neighbour face per edge (-1: none)
    char pad5c[8];
};
struct Edge {                                                 // 0x34 bytes
    int v0, v1;                                               // 0x00
    int face[2];                                              // 0x08 adjacent faces
    int nfaces;                                               // 0x10
    int pad14;
    float crease[3];                                          // 0x18 per-adjacent-face factor
    vector4 mid;                                              // 0x24 midpoint
};
OFFSET_CHECK(Face, n, 0x48);
OFFSET_CHECK(Edge, mid, 0x24);

template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// Bounding-box owner: a point array and its box (a different class from the topology below: its box at
// 0x44 would overlap the edge array's end pointer at 0x38).
struct PointSet {
    std::vector<Pt24> pts;                // 0x00 (first at 0x04)
    char pad10[0x44 - 0x10];
    vector4 bmin;                         // 0x44
    vector4 bmax;                         // 0x54
    void compute_bounds();                        // 0x4704b0
};
OFFSET_CHECK(PointSet, bmax, 0x54);

struct Mesh {
    char pad0[0x18];
    std::vector<Vtx18> verts;             // 0x18 (first at 0x1c)
    std::vector<Face> faces;              // 0x28 (first at 0x2c)
    std::vector<Edge> edges;              // 0x38 (first at 0x3c)
    void compute_face_normals();                  // 0x4705e0
    void compute_edge_creases();                  // 0x470750
    void build_adjacency();                       // 0x470920
    void build_geometry();                        // 0x470a00
};

// @ 0x4704b0
void PointSet::compute_bounds()
{
    if (pts.size()) {
        bmin = pts[0].pos;
        bmax = pts[0].pos;
    }
    for (unsigned i = 0; i < pts.size(); i++) {
        // pointers taken inside the loop: cl then re-reads pts.begin after every store
        vector4* mn = &bmin;
        vector4* mx = &bmax;
        if (mn->x > vat(pts, i).pos.x) mn->x = vat(pts, i).pos.x;
        if (mx->x < vat(pts, i).pos.x) mx->x = vat(pts, i).pos.x;
        if (mn->y > vat(pts, i).pos.y) mn->y = vat(pts, i).pos.y;
        if (mx->y < vat(pts, i).pos.y) mx->y = vat(pts, i).pos.y;
        if (mn->z > vat(pts, i).pos.z) mn->z = vat(pts, i).pos.z;
        if (mx->z < vat(pts, i).pos.z) mx->z = vat(pts, i).pos.z;
    }
}

// @ 0x470750
void Mesh::compute_edge_creases()
{
    vector4 d, s, n;                       // direction of the edge, projected part, perpendicular part
    for (unsigned i = 0; i < edges.size(); i++) {
        Edge* e = &vat(edges, i);
        Vtx18& a = vat(verts, e->v0);
        Vtx18& b = vat(verts, e->v1);
        d.x = b.x - a.x; d.y = b.y - a.y; d.z = b.z - a.z;
        float mz = e->mid.z;
        float len = d.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            d.x = d.x * inv; d.y = d.y * inv; d.z = d.z * inv;
        } else {
            d.x = 0.0f; d.y = 1.0f; d.z = 0.0f;
        }
        for (int j = 0; j < e->nfaces; j++) {
            Face& f = vat(faces, e->face[j]);
            if (f.centroid.z > mz) {
                float k = (d.x * f.normal.x + (d.y * f.normal.y + d.z * f.normal.z)) * 0.2f;
                s.x = d.x * k; s.y = d.y * k; s.z = d.z * k;
                n.x = f.normal.x - s.x; n.y = f.normal.y - s.y; n.z = f.normal.z - s.z;
                float l2 = (float)sqrt(n.z * n.z + (n.x * n.x + n.y * n.y));
                e->crease[j] = (l2 > 1e-10f) ? (1.0f / l2) * n.z : 0.0f;
            } else {
                e->crease[j] = 1.0f;
            }
        }
    }
}

// The same two functions seen from outside this TU: the original caller was compiled without their bodies, so
// cl must not assume their register use (it would keep `this` in ecx across the calls).
struct MeshExt {
    void build_adjacency();                       // 0x470920
    void compute_face_normals();                  // 0x4705e0
};
// @ 0x470a00
void Mesh::build_geometry()
{
    ((MeshExt*)this)->build_adjacency();
    for (unsigned i = 0; i < faces.size(); i++) {
        Face* f = &vat(faces, i);
        f->centroid = vector4(0.0f, 0.0f, 0.0f);
        for (int j = 0; j < vat(faces, i).n; j++) {
            Face& g = vat(faces, i);
            Vtx18& v = vat(verts, g.vid[j]);
            g.centroid.x = g.centroid.x + v.x;
            g.centroid.y = v.y + g.centroid.y;
            g.centroid.z = v.z + g.centroid.z;
            vat(faces, i).nbr[j] = -1;
            Face& h = vat(faces, i);
            if (vat(edges, h.eid[j]).nfaces >= 2) {
                if (vat(edges, h.eid[j]).face[0] == (int)i)
                    h.nbr[j] = vat(edges, h.eid[j]).face[1];
                else if (vat(edges, h.eid[j]).face[1] == (int)i)
                    h.nbr[j] = vat(edges, h.eid[j]).face[0];
            }
        }
        Face& h = vat(faces, i);
        float s = 1.0f / (float)h.n;
        h.centroid.x = s * h.centroid.x;
        h.centroid.y = s * h.centroid.y;
        h.centroid.z = s * h.centroid.z;
    }
    for (unsigned i = 0; i < edges.size(); i++) {
        Edge& e = vat(edges, i);
        Vtx18& a = vat(verts, e.v0);
        Vtx18& b = vat(verts, e.v1);
        e.mid = vector4((b.x + a.x) * 0.5f, (b.y + a.y) * 0.5f, (b.z + a.z) * 0.5f);
    }
    ((MeshExt*)this)->compute_face_normals();
}

// @ 0x470920
void Mesh::build_adjacency()
{
    for (unsigned i = 0; i < edges.size(); i++)
        vat(edges, i).nfaces = 0;
    for (unsigned i = 0; i < faces.size(); i++) {
        Face* f = &vat(faces, i);
        for (int j = 0; j < f->n; j++) {
            vat(edges, f->eid[j]).face[vat(edges, f->eid[j]).nfaces] = i;
            vat(edges, vat(faces, i).eid[j]).nfaces++;
            f = &vat(faces, i);
        }
    }
}

// @ 0x4705e0
void Mesh::compute_face_normals()
{
    vector4 t;                              // the normal before it is stored (its w lane is never written)
    vector4 b;
    for (unsigned i = 0; i < faces.size(); i++) {
        Face& f = vat(faces, i);
        float ax = vat(verts, f.vid[2]).x - vat(verts, f.vid[0]).x;
        float ay = vat(verts, f.vid[2]).y - vat(verts, f.vid[0]).y;
        float az = vat(verts, f.vid[2]).z - vat(verts, f.vid[0]).z;
        b.x = vat(verts, f.vid[1]).x - vat(verts, f.vid[0]).x;
        b.y = vat(verts, f.vid[1]).y - vat(verts, f.vid[0]).y;
        b.z = vat(verts, f.vid[1]).z - vat(verts, f.vid[0]).z;
        t.x = b.y * az - b.z * ay;
        t.y = b.z * ax - az * b.x;
        t.z = ay * b.x - b.y * ax;
        f.normal = t;
        Face& g = vat(faces, i);
        float len = g.normal.length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            g.normal.x = inv * g.normal.x;
            g.normal.y = inv * g.normal.y;
            g.normal.z = inv * g.normal.z;
        } else {
            g.normal.x = 0.0f; g.normal.y = 1.0f; g.normal.z = 0.0f;
        }
    }
}

// ---- the scene object the light camera hangs off (same Ctx as s0042aaf0 / s00463790; only what is used) ----
struct Ctx {
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60
    void build_light_frame(matrix* m, vector4* amb);                    // 0x470c00
};

// @ 0x470c00
void Ctx::build_light_frame(matrix* m, vector4* amb)
{
    vector4 d;
    {
        vector4 p(0.0f, 0.0f, 0.0f);       // dies before the frame temporaries: cl shares its stack slot
        sample_lights(&p, &d, amb);
    }
    if (d.z > -0.1f) d.z = -0.1f;
    vector4* u = &m->u;
    vector4* f = &m->f;
    vector4* s = &m->s;
    *u = vector4(-d.x, -d.y, -d.z);
    *f = vector4(0.0f, 0.0f, 1.0f);
    *s = vector4(f->y * u->z - f->z * u->y, f->z * u->x - f->x * u->z, f->x * u->y - f->y * u->x);
    s->normalize();
    *f = vector4(u->y * s->z - u->z * s->y, u->z * s->x - u->x * s->z, u->x * s->y - u->y * s->x);
    f->normalize();
}

// ---- a HUD element table (0x180: one widget pointer per slot) ----
struct Meter { void set_value(float* v); };          // 0x439000 (thiscall, ret 4; ICF-folded with node_groups::cap_all)
struct MeterBar {
    char pad0[0x180];
    Meter* slots[16];                                // 0x180
    void set_slot(int idx, int value);               // 0x470d80
};
// @ 0x470d80
void MeterBar::set_slot(int idx, int value)
{
    Meter* m = slots[idx];
    if (m) {
        float v = (float)value;
        m->set_value(&v);
    }
}
