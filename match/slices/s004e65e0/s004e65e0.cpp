// Slice s004e65e0: one mesh builder.
//   0x4e65e0  Shape::build -- clears the owner mesh and, for every 0x18-byte record of the shape (a
//             vector4 position plus a flag byte at +0x10), builds a four-point spike: the record's
//             position becomes the translation row of a local frame (identity_pos 0x40cd00) and four
//             points are placed in that frame; four triangles then stitch them into a closed
//             tetrahedron (add_point 0x412f50, add_vertex 0x413c50, add_face 0x411920). Width and
//             colour come from the shape's mode (+0x78) and the record flag; the owner's two dirty
//             flags (+0x138/+0x164) are set once at the end.
// Names and the two class layouts are provisional ("Shape" is whatever owns the record vector); every
// offset and every float constant is read off the disassembly / the image.
#include "rgl.h"

namespace rgl {

struct vec2 {
    float x, y;
    vec2() {}
    vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct mesh {                                   // the 0x260-byte rgl::mesh (see s00428b10)
    char pad[0x260];
    void clear_geometry();                      // 0x428e80
    int add_point(const vector4* p);            // 0x412f50: push one point, returns its index
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos,
                   const int* f5c);             // 0x413c50: append a 0x60-byte vertex record
    int add_face(int a, int b, int c);          // 0x411920: append a triangle
};

} // namespace rgl

void identity_pos(void* m);                     // 0x40cd00: fill a rgl::matrix with the identity

using rgl::matrix;
using rgl::mesh;
using rgl::vec2;
using rgl::vector4;

inline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// one 0x18-byte shape record: the position and a flag byte (+0x10)
struct rec18 {
    vector4 pos;                                // +0x00
    char flag;                                  // +0x10
    char pad[7];
};

// the shape that owns the record vector (this); only these three fields are touched
struct Shape {
    char pad[0x1c];
    rec18* recs;                                // +0x1c
    rec18* recs_end;                            // +0x20
    char pad2[0x78 - 0x24];
    int mode;                                   // +0x78

    void build(mesh* owner);                    // 0x4e65e0
};

// @ 0x4e65e0
void Shape::build(mesh* owner)
{
    owner->clear_geometry();                                    // 0x428e80

    matrix m;                                                   // the frame, ES+0x18
    identity_pos(&m);                                           // 0x40cd00

    int off = 0;
    for (unsigned i = 0;; ++i, off += 0x18) {
        if (recs == 0)
            break;
        if (i >= (unsigned)(((char*)recs_end - (char*)recs) / 0x18))
            break;
        rec18* e = (rec18*)((char*)recs + off);

        float w;
        unsigned color;
        if (mode == 0) {
            if (e->flag != 0) {
                w = 2.0f;
                color = 0xaaff5555u;
            } else {
                w = 1.4f;
                color = 0xaa4455ddu;
            }
        } else {
            w = 1.0f;
            color = 0x77555577u;
        }

        m.o = e->pos;                                           // the record is the origin of the frame

        // the four spikes, in the frame's local coordinates
        vector4 p0, p1, p2, p3;
        p0.x = m.s.x * (w * 0.034f) + m.f.x * (w * 0.0154f) + m.u.x * (w * 0.2f) + m.o.x;
        p0.y = m.s.y * (w * 0.034f) + m.f.y * (w * 0.0154f) + m.u.y * (w * 0.2f) + m.o.y;
        p0.z = m.s.z * (w * 0.034f) + m.f.z * (w * 0.0154f) + m.u.z * (w * 0.2f) + m.o.z;
        p1.x = m.s.x * (w * -0.0602f) + m.f.x * (w * -0.148f) + m.u.x * (w * -0.0666f) + m.o.x;
        p1.y = m.s.y * (w * -0.0602f) + m.f.y * (w * -0.148f) + m.u.y * (w * -0.0666f) + m.o.y;
        p1.z = m.s.z * (w * -0.0602f) + m.f.z * (w * -0.148f) + m.u.z * (w * -0.0666f) + m.o.z;
        p2.x = m.s.x * (w * -0.0602f) + m.f.x * (w * 0.1786f) + m.u.x * (w * -0.0666f) + m.o.x;
        p2.y = m.s.y * (w * -0.0602f) + m.f.y * (w * 0.1786f) + m.u.y * (w * -0.0666f) + m.o.y;
        p2.z = m.s.z * (w * -0.0602f) + m.f.z * (w * 0.1786f) + m.u.z * (w * -0.0666f) + m.o.z;
        p3.x = m.s.x * (w * 0.2226f) + m.f.x * (w * 0.0154f) + m.u.x * (w * -0.0666f) + m.o.x;
        p3.y = m.s.y * (w * 0.2226f) + m.f.y * (w * 0.0154f) + m.u.y * (w * -0.0666f) + m.o.y;
        p3.z = m.s.z * (w * 0.2226f) + m.f.z * (w * 0.0154f) + m.u.z * (w * -0.0666f) + m.o.z;

        vector4 ring[4];
        ring[0] = p0;
        ring[1] = p1;
        ring[2] = p2;
        ring[3] = p3;

        int idx[4];
        vec2 uv(0.0f, 0.0f);
        vector4 up(0.0f, 0.0f, 1.0f);
        int zero = 0;
        for (int k = 0; k < 4; ++k) {
            int p = owner->add_point(&ring[k]);
            idx[k] = owner->add_vertex(p, &uv, &color, &up, &zero);
        }
        owner->add_face(idx[0], idx[1], idx[3]);
        owner->add_face(idx[0], idx[3], idx[2]);
        owner->add_face(idx[0], idx[2], idx[1]);
        owner->add_face(idx[1], idx[2], idx[3]);
    }

    *((char*)owner + 0x164) = 1;
    *((char*)owner + 0x138) = 1;
}
