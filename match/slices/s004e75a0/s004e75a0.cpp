// Slice s004e75a0: polygons/faces of a map mesh -> render mesh (0x4e75a0), and the map loader's block
// reader (0x4e7a70). Names provisional. Model notes:
//  - rgl::mesh keeps its 0x20-byte point vector at +0x1c (element = rest + current position) and the
//    0x64-byte face records between +0x2c and +0x30 (index into v70 at +0x2c.., corner count at +0x48,
//    u16 at +0x60, tint rgb at +0x80..0x88).
//  - a map Block (s004c7bf0) is 0x68 bytes with its point vector at +0x04 and index at +0x10.
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;

inline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
inline vector4 operator-(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// 16-byte engine string (ctor 0x42bbd0, dtor 0x42bc40): pos, buf, cap, len
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
};

// terrain-triangle normal (0x4612d0): n = (p1-p0) x (p2-p0), normalized
void tri_normal(vector4* n, const vector4* tri);                    // 0x4612d0
// packs a 0..1 rgb(t?) colour into a dword (0x40e440)
unsigned pack_color(const vector4* c);                             // 0x40e440

namespace rgl {

struct vec2 { float x, y; };
struct point { vector4 a; float f10, f14; };    // 0x18: position + two extra floats

struct mesh;

struct face64 {                                 // the 0x64-byte face record of a map mesh
    char pad0[0x28];
    int idx[8];                                 // +0x28: indices into the point vector
    int count;                                  // +0x48: corner count
    char pad4c[0x60 - 0x4c];
    char flag;                                  // +0x60: material variant flag (tested as a byte)
    char pad61[0x64 - 0x61];
};

struct mesh_t {
    char pad0[0x1c];
    point* v70;                                 // 0x1c: points (element 0x18)
    char pad20[0x2c - 0x20];
    face64* faces;                              // 0x2c: face records (0x64 bytes) or null
    face64* faces_end;                          // 0x30: one past the last face
    char pad34[0x78 - 0x34];
    int mode;                                   // 0x78 (2: map mesh, 1: ribbon)
    char pad7c[0x80 - 0x7c];
    float lx, ly, lz;                           // 0x80, 0x84, 0x88: light direction
    char pad8c[0x138 - 0x8c];
    char dirty138;                              // 0x138
    char pad139[0x164 - 0x139];
    char dirty164;                              // 0x164

    unsigned nfaces() const { return faces == 0 ? 0 : (unsigned)(faces_end - faces); }
    void clear_geometry();                      // 0x428e80
    int add_point(const vector4* p);            // 0x412f50
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    int add_face(int a, int b, int c);          // 0x411920
    void finish();                              // 0x4705e0
    void polygon_to_mesh(mesh_t* out);          // 0x4e75a0
};

} // namespace rgl

// a stream the loader reads its block list from (slot 0x4c reads a name)
struct map_stream {
    virtual void s0();
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual void s8();
    virtual void s9();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual void s19(void* name);               // 0x4c: reads a name into the given string
};

// a map Block (s004c7bf0), 0x68 bytes: its name string, a second string at +0x10, the 0x18-byte terrain
// vertices at +0x1c, the read pose at +0x28, material arrays. The loader stores the block's position into
// the block's second string word at +0x10.
struct Block {
    tstring name;                               // +0x00
    tstring name2;                              // +0x10
    char pad20[0x68 - 0x20];
    void read_terrain(map_stream* stream);      // 0x4c83a0
};

// the map loader (0x4e7a70's this)
struct map_loader {
    char pad0[4];
    Block* blocks;                              // +0x04
    void resize(unsigned n);                    // 0x4e40f0 (vector<Block>::resize)
    void map_layouts();                         // 0x4c84f0
    int read(map_stream* stream);               // 0x4e7a70
};

namespace rgl {

// @ 0x4e75a0
void mesh_t::polygon_to_mesh(mesh_t* out)
{
    out->clear_geometry();
    unsigned index = 0;
    int off = 0;
    while (index < nfaces()) {
        face64* f = (face64*)((char*)faces + off);
        int size = f->count;
        int k;
        float cx = 0.0f, cy = 0.0f, cz = 0.0f;
        for (k = 0; k < size; k++) {
            const vector4& p = v70[f->idx[k]].a;
            cx += p.x;
            cy += p.y;
            cz += p.z;
        }
        float inv = 1.0f / (float)size;
        cx = cx * inv;
        cy = cy * inv;
        cz = cz * inv;
        vector4 corners[4];
        for (k = 0; k < size; k++) {
            const vector4& p = v70[f->idx[k]].a;
            float dx = cx - p.x;
            float dy = cy - p.y;
            float dz = cz - p.z;
            float len = sqrtf(dz * dz + dy * dy + dx * dx);
            if (len > 1e-10f) {
                inv = 1.0f / len;
                dx = dx * inv;
                dy = dy * inv;
                dz = dz * inv;
            } else {
                dx = 0.0f;
                dy = 1.0f;
                dz = 0.0f;
            }
            corners[k] = vector4(dx * 0.1f + p.x, dy * 0.1f + p.y, dz * 0.1f + p.z);
        }
        vector4 normal;
        tri_normal(&normal, corners);
        vector4 c;
        if (mode == 2 && f->flag) {
            c.x = 1.0f;
            c.y = 0.5f;
            c.z = 0.4f;
            c.w = 0.7f;
        } else if (mode == 2) {
            c.x = 1.0f;
            c.y = 0.9f;
            c.z = 0.4f;
            c.w = 0.7f;
        } else {
            c.x = 0.9f;
            c.y = 0.9f;
            c.z = 0.9f;
            c.w = 0.45f;
        }
        float v = ((normal.y * ly + normal.z * lz) + normal.x * lx + 1.0f) * 0.5f;
        float w = 1.0f - v;
        c.x = (w * 0.294f + v) * c.x;
        c.y = (w * 0.0f + v) * c.y;
        c.z = (w * 0.286f + v) * c.z;
        unsigned color = pack_color(&c);
        int points[4];
        int verts[4];
        for (k = 0; k < f->count; k++) {
            int zero = 0;
            vec2 uv;
            uv.x = 0.0f;
            uv.y = 0.0f;
            vector4 nn;
            nn.x = 0.0f;
            nn.y = 0.0f;
            nn.z = 1.0f;
            points[k] = out->add_point(&corners[k]);
            verts[k] = out->add_vertex(points[k], &uv, &color, &nn, &zero);
        }
        out->add_face(verts[0], verts[1], verts[2]);
        out->add_face(verts[0], verts[2], verts[1]);
        if (f->count > 3) {
            out->add_face(verts[0], verts[2], verts[3]);
            out->add_face(verts[0], verts[3], verts[2]);
        }
        index++;
        off += 0x64;
    }
    out->dirty164 = 1;
    out->dirty138 = 1;
    finish();
}

} // namespace rgl

// @ 0x4e7a70
int map_loader::read(map_stream* stream)
{
    map_loader* self = this;
    Block dummy;
    stream->s19(&dummy);                        // 0x4c reads the block-list name
    tstring name;
    resize((unsigned)name.len);
    for (int i = 0; i < (int)name.len; i++) {
        Block* b = (Block*)((char*)self->blocks + i * 0x68);
        b->read_terrain(stream);
        b->name2.pos = i;                       // the loader stores the block's position at +0x10
    }
    self->map_layouts();
    return 0;
}
