// Slice s0047aa40: three rgl::mesh methods that rewrite the per-vertex data of a mesh (transform, colour,
// bone index) and then flag the vertex/slot caches dirty.
#include <math.h>
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

struct mstr { char* p; int size, cap, extra; };
extern int g_use_shaders;                         // 0x5e28fc (non-zero: programmable pipeline)
void unpack_color(vector4* out, const unsigned* argb);      // 0x40e4e0
unsigned pack_color(const vector4* c);                      // 0x40e440

// 0x60-byte vertex record (stride confirmed by 0x47b360's 0x60 pointer step and the /0x60 size maths)
struct vertex60 {
    vector4 pos;                  // 0x00 (position, renormalised by set_color's shader path)
    vector4 n10;                  // 0x10
    vector4 n20;                  // 0x20
    vector4 n30;                  // 0x30
    float f40, f44;               // 0x40
    float c48, c4c;               // 0x48
    int index;                    // 0x50
    int bone;                     // 0x54 (bone index, written by set_bone_index)
    int f58, f5c;
};
struct point20 { vector4 a, b; };                 // 0x70 list, 0x20
struct face20 { int a, b, c, d, e, f; };          // 0x90 list, 0x20

struct mesh {
    void** vt;
    mstr name;                   // 0x04
    int f0c;
    mesh* parent;                // 0x18 (chain link for get50/get70/get90)
    mstr name2;                  // 0x1c
    void* res;                   // 0x2c
    unsigned flags;              // 0x30
    int f34;
    char f38;
    int f3c, f40;
    float f44;
    int f48, f4c;
    vertex60* verts;             // 0x54 (the 0x60-stride vertex array; +0x50 is its vector header)
    vertex60* verts_end;         // 0x58
    vertex60* verts_cap;         // 0x5c
    char pad60[0x10];
    std::vector<point20> v70;    // 0x70
    char pad80[0x10];
    std::vector<face20> v90;     // 0x90
    char local_a0, local_a1;     // 0xa0, 0xa1
    int kf_frames;               // 0xa4
    int kf_times;                // 0xa8
    unsigned kf_count;           // 0xac
    char padb0[0x10];
    void* p_c0;                  // 0xc0
    float c4, c8;
    matrix m_cc;                 // 0xcc
    unsigned mask;               // 0x10c
    char pad110[0x138 - 0x110];
    char dirty138;               // 0x138 (slot 0 dirty)
    char pad139[0x164 - 0x139];
    char dirty164;               // 0x164 (slot 1 dirty)
    char pad165[0x214 - 0x165];
    vector4 c214, c224;          // 0x214, 0x224
    vector4 c234, c244;          // 0x234, 0x244

    void ensure_local();                              // 0x412ee0
    std::vector<point20>& get70();                    // 0x40eba0 (chain to the local owner)
    std::vector<face20>& get90();                     // 0x40ec30
    int add_point(const vector4* p);                  // 0x412f50
    void add_face(int a, int b, int c);               // 0x411920
    void compute_bounds();                            // 0x419300
    void refresh_slots();                             // 0x4779b0
    void transform(const matrix& m);                  // 0x47aa40
    void set_color(const vector4& color);             // 0x47af30
    void set_bone_index(int bone);                    // 0x47b360
};

// 0x411630: std::vector<int> assign (this = dst+4, arg = src+4)
struct intvec { int* first; int* last; int* end; };
void intvec_assign(intvec* dst, intvec* src);         // 0x411630
void aabb_update(void* box);                          // 0x416f70 (aabb::update)

// the key-frame colour entries are 0x20-byte records with two vector<vector4> members
struct kf_rec {
    int f0;
    vector4* first;                                  // +4
    vector4* last;                                   // +8
    vector4* end;                                    // +0xc
    vector4* nfirst;                                 // +0x10
    vector4* nlast;                                  // +0x14
    vector4* nend;                                   // +0x18
    int f1c;
};

// ---- accessors (the parent-chain walk is out of line in the original: 0x40eba0 / 0x40ebd0 / 0x40ec30) -----
std::vector<point20>& mesh::get70()  { return local_a1 ? v70 : parent->get70(); }
std::vector<face20>& mesh::get90()   { return local_a0 ? v90 : parent->get90(); }

// @ 0x47b360 -- stamp the bone index into every vertex record, then mark the first slot dirty
void mesh::set_bone_index(int bone)
{
    ensure_local();
    size_t n = (size_t)(verts_end - verts);
    vertex60* p = verts;
    for (size_t i = 0; i < n; i++, p = (vertex60*)((char*)p + sizeof(vertex60)))
        p->bone = bone;
    dirty138 = 1;
}

// @ 0x47af30 -- multiply the vertex colour terms and the key-frame colours by `color`, renormalise the two
// colour vectors of every vertex, and rebuild the slot vertices when the colour is fully/partly zero.
void mesh::set_color(const vector4& color)
{
    const float* args = (const float*)&color;
    const float& cx = *args;
    const float& cy = args[1];
    const float& cz = args[2];
    ensure_local();

    // 0x47af3b: the local point colours (0x70 list), both halves of every record
    for (point20* p = v70._Myfirst; p != v70._Mylast; p = (point20*)((char*)p + 0x20)) {
        p->a.x *= cx;  p->a.y *= cy;  p->a.z *= cz;
        p->b.x *= cx;  p->b.y *= cy;  p->b.z *= cz;
    }

    // 0x47af89: every key-frame record's first colour vector
    for (unsigned i = 0; i < kf_count; i++) {
        kf_rec* k = (kf_rec*)((char*)kf_frames + i * 0x20);
        vector4* p = k->first;
        vector4* e = k->last;
        for (; p != e; p++) {
            p->x *= cx;  p->y *= cy;  p->z *= cz;
        }
    }

    // 0x47aff3: vertices 0x30 and 0x20 terms: scale, then renormalise
    size_t n = (size_t)(verts_end - verts);
    for (size_t i = 0; i < n; i++) {
        vertex60* v = &verts[i];
        v->n30.x *= cx;  v->n30.y *= cy;  v->n30.z *= cz;
        v->n30.normalize();
        v->n20.x *= cx;  v->n20.y *= cy;  v->n20.z *= cz;
        v->n20.normalize();
    }

    long nzero = 0;
    if (cx == 0.0f) nzero = 1;
    if (cy == 0.0f) nzero++;
    if (cz == 0.0f) nzero++;
    if (nzero == 1 || nzero == 3)
        refresh_slots();                            // 0x4779b0

    dirty138 = 1;
    c214.x *= cx;  c214.y *= cy;  c214.z *= cz;   // 0x22c
    c224.x *= cx;  c224.y *= cy;  c224.z *= cz;   // 0x23c
    aabb_update((void*)((char*)this + 0x22c));     // 0x416f70

    // 0x47b1ff: with a zero component and shaders in use, the vertex 0x10/0x00 terms are scaled too
    if (g_use_shaders && nzero > 0) {
        for (size_t i = 0; i < n; i++) {
            vertex60* v = &verts[i];
            v->n10.x *= cx;  v->n10.y *= cy;  v->n10.z *= cz;
            v->n10.normalize();
            v->pos.x *= cx;  v->pos.y *= cy;  v->pos.z *= cz;
            v->pos.normalize();
        }
    }
}

// @ 0x47aa40 -- mesh::transform(const matrix&): give the mesh its own face list, build the inverse basis
// from the matrix, project every point (rest and current) with it, re-add the faces with the current point
// indices, mark both slots dirty and rebuild the bounds.
void mesh::transform(const matrix& m)
{
    if (!local_a0) {
        intvec_assign((intvec*)((char*)this + 0x90), (intvec*)((char*)parent + 0x90));
        local_a0 = 1;
    }
    ensure_local();

    std::vector<point20>& pts = get70();
    int n70 = pts._Myfirst ? (int)(pts._Mylast - pts._Myfirst) >> 5 : 0;
    size_t nverts = (size_t)(verts_end - verts);

    matrix mm = m;

    // the frame at +0x70..+0x90 is a 0x90-byte matrix (rows s, f, u, o, then 0x30-scale terms); its first row
    // is normalised and the result is the inverse basis used for the projection
    vector4 r0, r1, r2;
    r0.set(mm.f.y * mm.u.z - mm.f.z * mm.u.y,
           mm.f.z * mm.u.x - mm.f.x * mm.u.z,
           mm.f.x * mm.u.y - mm.f.y * mm.u.x);
    r0.normalize();
    r1.set(mm.u.y * mm.s.z - mm.u.z * mm.s.y,
           mm.u.z * mm.s.x - mm.u.x * mm.s.z,
           mm.u.x * mm.s.y - mm.u.y * mm.s.x);
    r2.set(mm.s.y * mm.f.z - mm.s.z * mm.f.y,
           mm.s.z * mm.f.x - mm.s.x * mm.f.z,
           mm.s.x * mm.f.y - mm.s.y * mm.f.x);

    for (int i = 0; i < n70; i++) {
        point20* s = &pts[(size_t)i];
        vector4 a = s->a;
        vector4 out;
        out.x = (a.x * r0.x + a.y * r1.x + a.z * r2.x) + mm.o.x;
        out.y = (a.x * r0.y + a.y * r1.y + a.z * r2.y) + mm.o.y;
        out.z = (a.x * r0.z + a.y * r1.z + a.z * r2.z) + mm.o.z;
        out.w = a.w;
        add_point(&out);
    }

    for (size_t i = 0; i < nverts; i++) {
        vertex60& v = verts[i];
        vector4 a = v.pos;
        v.pos.x = a.x * r0.x + a.y * r1.x + a.z * r2.x + mm.o.x;
        v.pos.y = a.x * r0.y + a.y * r1.y + a.z * r2.y + mm.o.y;
        v.pos.z = a.x * r0.z + a.y * r1.z + a.z * r2.z + mm.o.z;
    }

    std::vector<face20>& faces = get90();
    for (size_t i = 0; i < faces.size(); i++) {
        face20& f = faces[i];
        add_face(f.a, f.b, f.c);
    }

    dirty138 = 1;
    dirty164 = 1;
    compute_bounds();
}
