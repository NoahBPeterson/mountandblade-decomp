// Slice s004e6a70: mesh geometry builder for the "particle ribbon" shapes. The function walks the shape's
// 0x34-byte segment records (mesh+0x3c .. mesh+0x40) and, for each, adds two rows of four points offset
// along the segment normal, then stitches them with add_face. Names are provisional.
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

inline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

namespace rgl {

struct vec2 { float x, y; };                    // element type of add_vertex's uv argument

struct point20 { vector4 a, b; };

struct mesh;                                    // the class that owns the shape (hidden by the
                                                // "mesh" parameter below, as in the original's TU)
struct mesh_t {                                 // rgl::mesh (0x260 bytes; see s0044e2b0/mesh.h)
    virtual ~mesh_t();
    virtual int slot1();
    virtual void slot2();
    char pad[0x1c];
    std::vector<point20> v70;                   // 0x1c
    char pad2[0x3c - 0x2c];
    point20* pts;                               // 0x3c: segment records (0x34 bytes each) or null
    point20* pts_end;                           // 0x40: one past the last record
    char pad3[0x78 - 0x44];
    int mode;                                   // 0x78 (1: two-tone segments)
    void clear_geometry();                      // 0x428e80
    int add_point(const vector4* p);            // 0x412f50
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    int add_face(int a, int b, int c);          // 0x411920
};

// a 0x34-byte segment record: point indices at +0x00/+0x04, a u16 at +0x10 and a flag byte at +0x14
// (the record is copied with a 0x34-byte memcpy, so only these fields are known).
struct rec34 {
    int a, b;                                   // +0x00, +0x04
    char pad8[8];
    unsigned short u16;                         // +0x10
    char pad12[2];
    char flag;                                  // +0x14
    char pad15[0x34 - 0x15];
};

} // namespace rgl

// @ 0x4e6a70
void build_ribbon_mesh(rgl::mesh_t* ps, rgl::mesh* owner)
{
    ps->clear_geometry();
    int elem_off = 0;
    for (unsigned i = 0; ps->pts != 0 && i < (unsigned)((char*)ps->pts_end - (char*)ps->pts) / 0x34; i++) {
        rgl::rec34* e = (rgl::rec34*)((char*)ps->pts + elem_off);
        float w;
        unsigned color;
        if (ps->mode == 1) {
            if (e->flag != 0) {
                w = 2.0f;
                color = 0xaaff5555;
            } else {
                w = 1.4f;
                color = 0xaa66cc55;
            }
        } else {
            w = 1.0f;
            color = 0x77668866;
        }
        const vector4& p0 = ps->v70[e->a].a;
        const vector4& p1 = ps->v70[e->b].a;
        vector4 d(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
        float len = d.normalize();
        vector4 y(0.0f, 1.0f, 0.0f);
        float px = y.y * d.z - y.z * d.y;
        float py = y.z * d.x - y.x * d.z;
        float pz = y.x * d.y - y.y * d.x;
        vector4 perp(px, py, pz);
        normalize(perp, perp);
        vector4 n = y - perp;
        float off = w * 0.3f;

        vector4 b(p0.x + d.x * (len - 0.3f), p0.y + d.y * (len - 0.3f), p0.z + d.z * (len - 0.3f));
        vector4 v[8];
        v[0] = b + n * (w * 0.08f);
        v[1] = b + n * (w * -0.2f);
        v[2] = b + n * (w * 0.07f);
        v[3] = b + n * (w * -0.07f);
        v[4] = p0 + n * (w * 0.08f);
        v[5] = p0 + n * (w * -0.2f);
        v[6] = p0 + n * (w * 0.07f);
        v[7] = p0 + n * (w * -0.07f);
        v[0] = v[0] + d * off;
        v[1] = v[1] + d * off;
        v[2] = v[2] + d * off;
        v[3] = v[3] + d * off;
        v[4] = v[4] + d * off;
        v[5] = v[5] + d * off;
        v[6] = v[6] + d * off;
        v[7] = v[7] + d * off;

        int idx[6];
        rgl::vec2 uv;
        uv.x = 0.0f;
        uv.y = 0.0f;
        int zero = 0;
        for (int k = 0; k < 6; k++) {
            int p = ps->add_point(&v[0]);
            idx[k] = ps->add_vertex(p, &uv, &color, &v[0], &zero);
        }
        ps->add_face(idx[0], idx[3], idx[4]);
        ps->add_face(idx[0], idx[4], idx[1]);
        ps->add_face(idx[0], idx[2], idx[3]);
        ps->add_face(idx[2], idx[5], idx[3]);
        elem_off += 0x34;
    }
    *((char*)owner + 0x164) = 1;
    *((char*)owner + 0x138) = 1;
}
