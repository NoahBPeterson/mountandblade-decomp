// Slice s00439a50: queries of the level grid and navigation mesh (names provisional): nearest nav face to a point,
// the floor under a point (point query of the cell + plane height of the face polygons), a segment sweep with a
// capsule, removal of a shape from the group's lists, setup of a 0xd8-byte link record, collection of the group's
// shapes into a 32-entry array, and the string-table lookup ("STR NOT FOUND: ").
#include <string.h>
#include <math.h>
#include <vector>
#include "s00439a50.h"

// ---- level: nearest nav face, floor, swept segment ----------------------------------------------------------
// nearest nav face to p: a face containing p (best |dz| < 2 below) wins over near faces (score 10 - dist - |dz|)
// @ 0x439a50
int level::nearest_face(const float* p)
{
    NavMesh* m = nav;
    int best = -1;
    float near_score = 0.0f;
    bool have = false;
    float best_dz = 10000.0f;
    if (m) {
        unsigned i = 0;
        while (i < m->faces.size()) {
            int idx = i;
            float d = m->dist(&idx, p);
            if (d <= 0.0f) {
                NavFace* f = &m->faces[i];
                const NavVert* v = &m->verts[f->vert_p()[0]];
                float dz = p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz);
                if (dz > -2.0f) {
                    dz = (float)fabs(dz);
                    if (!have || dz < best_dz) {
                        best = i;
                        best_dz = dz;
                        have = true;
                    }
                }
            } else if (d < 10.0f) {
                NavFace* f = &m->faces[i];
                const NavVert* v = &m->verts[f->vert_p()[0]];
                float dz = p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz);
                if (dz > -2.0f) {
                    float s = (10.0f - d) - (float)fabs(dz);
                    if (s > near_score) {
                        best = i;
                        near_score = s;
                    }
                }
            }
            i++;
        }
    }
    return best;
}

// highest floor polygon under (or at most 1 above) p; falls back to the lowest polygon above it
// @ 0x439be0
void level::floor_at(const float* p, float* out_z, face_shape** out_face)
{
    int n = 0;
    *out_z = -1000000.0f;
    shape_base* shapes[0x2000];
    int count;
    {
        int use3d = 0;
        int ci[2];
        locate(p, ci);
        count = 0;
        cells[ci[0]][ci[1]].collect_point(p, &use3d, 0x304, shapes, &count);
    }
    {
        float pt[2];
        pt[0] = p[0];
        pt[1] = p[1];
        *out_face = 0;
        face_shape* far_face = 0;
        float far_z = 100000.0f;
        for (; n < count; n++) {
            face_shape* s = (face_shape*)shapes[n];
            if (s->m4 >= 0 && s->contains_xy(pt)) {
                float z = s->v[0].z - ((pt[0] - s->v[0].x) * s->normal.x + (pt[1] - s->v[0].y) * s->normal.y) / s->normal.z;
                if (z < p[2] + 1.0f) {
                    if (*out_z < z) {
                        *out_z = z;
                        *out_face = s;
                    }
                } else if (*out_z < far_z) {
                    far_z = z;
                    far_face = s;
                }
            }
        }
        if (*out_z <= -900000.0f) {
            *out_z = far_z;
            *out_face = far_face;
        }
    }
}

// @ 0x439d40
float level::floor_z(const float* p)
{
    float z;
    face_shape* f;
    floor_at(p, &z, &f);
    return z;
}

// sweep the segment a->b (clipped to maxlen) with a thin capsule; false if the height field blocks it or a shape
// is hit (that shape goes to *hit)
// @ 0x439d60
bool level::trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused)
{
    bool ok = true;
    if (hit) *hit = 0;
    if (hf == 0 || (ok = hf->line_clear((const vector4*)a, (const vector4*)b))) {
        vector4 d;
        d.x = b[0] - a[0];
        d.y = b[1] - a[1];
        d.z = b[2] - a[2];
        float len = d.normalize();
        if (maxlen < len) len = maxlen;
        vector4 t;
        t.x = d.x * len;
        t.y = d.y * len;
        t.z = d.z * len;
        d.x = t.x + a[0];
        d.y = t.y + a[1];
        d.z = t.z + a[2];
        capsule cap(0.015f, (const vector4*)a, &d);
        shape_base* shapes[0x2000];
        int n;
        query_segment(a, (const float*)&d, 0x114, shapes, &n);
        int i = 0;
        for (; i < n; i++) {
            if (!ok) break;
            ok = shape_contact_count(shapes[i], &cap, 0, 0) == 0;
        }
        if (!ok) {
            if (hit) *hit = shapes[i - 1];
        }
    }
    return ok;
}

// ---- group of regions ---------------------------------------------------------------------------------------
namespace std {
template<class _FI, class _Ty> _FI remove(_FI _First, _FI _Last, const _Ty& _Val);     // 0x41f080 (opaque, as in s0041ee20_b)
}

struct Elem12 {                                         // 12-byte element of a polygon's index list
    int a[3];
    vector4 point(int idx) const;                       // 0x436ae0
};
struct Poly {                                           // object with a list of 12-byte elements at +0x1c
    char pad0[0x18];
    std::vector<Elem12> elems;                          // 0x18 (first at 0x1c)
    vector4 point(int idx) const;                       // 0x436b90
};
// 0xd8-byte link record
struct LinkRec {
    char pad0[0x44];
    int k44, k48, k4c, k50;                             // 0x44
    char pad54[8];
    Poly* poly;                                         // 0x5c
    vector4* pa;                                        // 0x60 (one point per element)
    vector4* pb;                                        // 0x64
    char pad68[0x7c - 0x68];
    vector4 va, vb;                                     // 0x7c, 0x8c
    char pad9c[0xd8 - 0x9c];
};
struct Region {                                         // 0x1dc bytes
    char pad0[0x1b0];
    std::vector<shape_base*> shapes;                    // 0x1b0
    char pad1c0[0x1dc - 0x1c0];
};
struct Group {
    int count;                                          // 0x00 number of regions
    Region* regions;                                    // 0x04
    char pad8[0x30 - 8];
    std::vector<shape_base*> shapes;                    // 0x30 (0x10 bytes)
    LinkRec* recs;                                      // 0x40
    void remove_shape(shape_base* s);                   // 0x439ec0
    void init_link(int rec, Poly* poly, int a, int b);  // 0x439fc0
    void all_shapes(shape_base** out, int* count);      // 0x43a130
};

// @ 0x439ec0
void Group::remove_shape(shape_base* s)
{
    int i;
    if (s) {
        bool done = false;
        std::vector<shape_base*>::iterator it = std::remove(shapes.begin(), shapes.end(), s);
        if (it == shapes.end()) {
            std::vector<shape_base*>::iterator r;
            for (i = 0; i < count; i++) {
                if (done) break;
                r = std::remove(regions[i].shapes.begin(), regions[i].shapes.end(), s);
                if (r != regions[i].shapes.end()) {
                    regions[i].shapes.erase(r, regions[i].shapes.end());
                    done = true;
                }
            }
        } else
            shapes.erase(it, shapes.end());
    }
}

// @ 0x439fc0
void Group::init_link(int rec, Poly* poly, int a, int b)
{
    if (poly->elems.size() >= (unsigned)count) {
        recs[rec].k44 = 0x33;
        recs[rec].k48 = 0x33;
        recs[rec].k4c = a;
        recs[rec].k50 = b;
        recs[rec].va = poly->point(a);
        recs[rec].vb = poly->point(b);
        recs[rec].poly = poly;
        for (int i = 0; i < count; i++) {
            recs[rec].pa[i] = poly->elems[i].point(a);
            recs[rec].pb[i] = poly->elems[i].point(b);
        }
    }
}

// all shapes of the group and of its regions, at most 32
// @ 0x43a130
void Group::all_shapes(shape_base** out, int* n)
{
    int i = 0;
    *n = 0;
    for (unsigned k = 0; k < shapes.size(); k++) {
        if (*n < 0x20) {
            out[*n] = shapes[k];
            (*n)++;
        }
    }
    for (int j = 0; j < count; j++) {
        for (unsigned k = 0; k < regions[j].shapes.size(); k++) {
            if (*n < 0x20) {
                out[*n] = regions[j].shapes[k];
                (*n)++;
            }
        }
    }
}

// ---- string table ("STR NOT FOUND: ") -----------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    char equals(const tstring& o) const;                // 0x42bdd0
    void assign(const char* s);                         // 0x42bcb0
    void append(const tstring& o);                      // 0x42c1e0
};
unsigned __stdcall hash_string(const tstring* s);      // 0x433bb0
struct StrBucket { std::vector<tstring> v; };          // 16 bytes
struct StrTab {
    StrBucket keys[0xdf3];                              // 0x0000
    StrBucket vals[0xdf3];                              // 0xdf30
    tstring notfound;                                   // 0x1be60
    tstring* lookup(const tstring& key);                // 0x43a1e0
};

// @ 0x43a1e0
tstring* StrTab::lookup(const tstring& key)
{
    unsigned h = hash_string(&key);
    unsigned n = keys[h].v.size();
    for (unsigned i = 0; (int)i < (int)n; i++) {
        if (keys[h].v[i].equals(key))
            return &vals[h].v[i];
    }
    notfound.assign("STR NOT FOUND: ");
    notfound.append(key);
    return &notfound;
}
