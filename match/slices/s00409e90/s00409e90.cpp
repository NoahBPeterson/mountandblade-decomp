// Slice s00409e90: capsule/capsule and sphere/polygon contact generation, capsule shape class (clone, copy ctor).
#include "rgl.h"
using namespace rgl;
namespace rgl { void normalize(vector4& out, const vector4& in); } // 0x407e00

// 0x24-byte contact record
struct contact {
    vector4 normal;
    vector4 point;
    float depth;
};

// collision shape record views (provisional): capsule segment p0 at +0x20, p1 at +0x30, radius at +0x60;
// sphere centre at +0x30, radius at +0x40.
struct coll_shape {
    char pad0[0x20];
    vector4 p0;
    vector4 p1;
    float radius;
    char pad1[0x1c];
    float cap_radius;
};
// convex polygon (provisional): up to 4 vertices at +0x20 (16 bytes each), vertex count at +0xb0
struct coll_poly {
    char pad0[0x20];
    vector4 v[4];
    char pad1[0x50];
    int count;
};

void segment_closest_points(const vector4* a0, const vector4* a1, const vector4* b0, const vector4* b1, vector4* outA, vector4* outB); // 0x409500

// @ 0x409e90
void capsule_capsule_contact(coll_shape* a, coll_shape* b, int max, contact* out, int* count)
{
    vector4 pa, pb;
    segment_closest_points(&a->p0, &a->p1, &b->p0, &b->p1, &pa, &pb);
    vector4 d(pa.x - pb.x, pa.y - pb.y, pa.z - pb.z);
    float depth = (a->cap_radius + b->cap_radius) - (float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z));
    if (!(depth < 0.0f)) {
        int n = ++*count;
        if (n <= max) {
            float t = b->cap_radius / (a->cap_radius + b->cap_radius);
            vector4 dt(d.x * t, d.y * t, d.z * t);
            pa.x = dt.x + pb.x; pa.y = dt.y + pb.y; pa.z = dt.z + pb.z;
            normalize(pb, d);
            contact c;
            c.normal = pb;
            c.point = pa;
            c.depth = depth;
            out[n - 1] = c;
        }
    }
}

// @ 0x40a000
// sphere b against convex polygon a: face contact, or edge contact when the centre is outside an edge.
void polygon_sphere_contact(coll_poly* a, coll_shape* b, int max, contact* out, int* count)
{
    vector4 v[4];
    v[0] = a->v[0]; v[1] = a->v[1]; v[2] = a->v[2]; v[3] = a->v[3];
    vector4 e1(v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z);
    vector4 e2(v[2].x - v[1].x, v[2].y - v[1].y, v[2].z - v[1].z);
    vector4 n(e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e2.z * e1.x, e2.y * e1.x - e1.y * e2.x);
    normalize(n, n);
    float dist = (b->p1.x - v[0].x) * n.x + ((b->p1.y - v[0].y) * n.y + (b->p1.z - v[0].z) * n.z);
    if (dist > b->radius) return;
    float negr = -b->radius;
    if (dist < negr) return;

    vector4 nn(-n.x, -n.y, -n.z);
    bool hit = true;
    bool outside = false;
    contact c;
    c.normal = nn;
    c.point = vector4(nn.x * dist + b->p1.x, nn.y * dist + b->p1.y, nn.z * dist + b->p1.z);
    c.point.w = nn.w;
    c.depth = b->radius - dist;
    int cnt = a->count;
    int i = 0;
    do {
        ++i;
        int j = i % cnt;
        const vector4& s = v[i - 1];
        vector4 e(v[j].x - s.x, v[j].y - s.y, v[j].z - s.z);
        vector4 ed;
        normalize(ed, e);
        vector4 w(b->p1.x - s.x, b->p1.y - s.y, b->p1.z - s.z);
        float cx = ed.y * w.z - ed.z * w.y;
        float cy = ed.z * w.x - w.z * ed.x;
        float cz = w.y * ed.x - ed.y * w.x;
        if (cx * n.x + (cy * n.y + n.z * cz) < 0.0) {
            outside = true;
            float t;
            if (b->radius < (float)sqrt(cx * cx + (cy * cy + cz * cz))
                || (t = w.x * ed.x + (ed.y * w.y + ed.z * w.z), !(negr < t))
                || !(t < (float)sqrt(e.x * e.x + (e.y * e.y + e.z * e.z)) + b->radius)) {
                hit = false;
            } else {
                hit = true;
                vector4 p(ed.x * t + s.x, ed.y * t + s.y, ed.z * t + s.z);
                p.w = ed.w;
                c.point = p;
                c.depth = b->radius - (float)sqrt((p.x - b->p1.x) * (p.x - b->p1.x) + ((p.y - b->p1.y) * (p.y - b->p1.y) + (p.z - b->p1.z) * (p.z - b->p1.z)));
            }
        }
    } while (!outside && i < cnt);
    if (hit) {
        int k = ++*count;
        if (k <= max) out[k - 1] = c;
    }
}

// ---- capsule shape class ----------------------------------------------------------------------
namespace rgl {

// Abstract shape base (vtable 0x54a4b8; see also s00407c10, which has the full slot list). Only the
// members the capsule copy-constructor touches are declared here.
struct sphere {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~sphere();                          // 0x40a540
    virtual void pure1() = 0;
    virtual void pure2() = 0;
    virtual void pure3() = 0;
    virtual void pure4() = 0;
};

// @ 0x40a540
sphere::~sphere() {}

// vtable 0x54a540, size 0x64. Name provisional: p0/p1 are the segment ends used by the contact code.
struct capsule : sphere {
    vector4 p0, p1, v40, v50;
    float radius;
    capsule(const capsule& o);                  // 0x40a580
    virtual void pure1();
    virtual void pure2();
    virtual void pure3();
    virtual void pure4();
    virtual sphere* clone();                    // slot 5, 0x40a560
};

// @ 0x40a560
sphere* capsule::clone() { return new capsule(*this); }

// @ 0x40a580
capsule::capsule(const capsule& o) : sphere(o), p0(o.p0), p1(o.p1), v40(o.v40), v50(o.v50), radius(o.radius) {}

} // namespace rgl
