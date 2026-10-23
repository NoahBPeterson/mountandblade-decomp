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
vector4* unpack_color(vector4* out, const unsigned* argb);  // 0x40e4e0 (returns out)
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
struct aabb { vector4 lo, hi; void update(); };      // 0x416f70 (thiscall, this = mesh+0x22c)
struct point20 { vector4 a, b; };                 // 0x70 list, 0x20
struct face20 { int pad[4]; int a, b, c; int pad1c; };   // 0x90 list, 0x20-byte records (indices at +0x10..+0x18)

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
    std::vector<vertex60> vv;    // 0x50 (allocator word, then first 0x54 / last 0x58 / end 0x5c)
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
    char pad214[0x22c - 0x214];
    aabb box;                    // 0x22c (lo), 0x23c (hi): scaled by set_color, then update()

    void ensure_local();                              // 0x412ee0
    std::vector<point20>& get70();                    // 0x40eba0 (chain to the local owner)
    std::vector<vertex60>& get50();                   // 0x40ebd0 (vertex list of the local owner)
    std::vector<face20>& get90();                     // 0x40ec30
    int add_point(const vector4* p);                  // 0x412f50
    void add_face(int a, int b, int c);               // 0x411920
    void compute_bounds();                            // 0x419300
    void refresh_slots();                             // 0x4779b0
    void transform(mesh* src, const matrix* m, unsigned color);   // 0x47aa40
    void set_color(vector4 color);             // 0x47af30
    void set_bone_index(int bone);                    // 0x47b360
};

// 0x411630: std::vector<int> assign (this = dst+4, arg = src+4)
struct intvec { int* first; int* last; int* end; void assign(intvec* src); };   // 0x411630 (thiscall)
struct vvec { void push_back(const vertex60& v); };   // 0x411420 (thiscall, callee pops 4): std::vector<vertex60>::push_back

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

// ---- accessors (the parent-chain walk: out of line at 0x40eba0 / 0x40ebd0 / 0x40ec30, partly inlined here) -----
std::vector<point20>& mesh::get70()  { return local_a1 ? v70 : parent->get70(); }
std::vector<vertex60>& mesh::get50() { return local_a1 ? vv : parent->get50(); }
std::vector<face20>& mesh::get90()   { return local_a0 ? v90 : parent->get90(); }

// @ 0x47b360 -- stamp the bone index into every vertex record, then mark the first slot dirty
void mesh::set_bone_index(int bone)
{
    ensure_local();
    for (size_t i = 0; i < vv.size(); i++)
        vv[i].bone = bone;
    dirty138 = 1;
}

// the original inlines vector4::normalize: x*x + y*y + z*z, 1/len scaling, (0,1,0) when len <= 1e-10
static __forceinline void vnorm(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
}

// @ 0x47af30 -- multiply the vertex colour terms and the key-frame colours by `color`, renormalise the colour
// vectors of every vertex, and rebuild the slot vertices when one or three components are negative.
void mesh::set_color(vector4 color)
{
    ensure_local();

    // 0x47af3b: the local point colours (0x70 list), both halves of every record
    for (point20* p = v70._Myfirst; p < v70._Mylast; p = (point20*)((char*)p + 0x20)) {
        p->a.x *= color.x;  p->a.y *= color.y;  p->a.z *= color.z;
        p->b.x *= color.x;  p->b.y *= color.y;  p->b.z *= color.z;
    }

    // 0x47af89: every key-frame record's first colour vector
    for (unsigned i = 0; i < kf_count; i++) {
#define KF(i) ((kf_rec*)((char*)kf_frames + (i) * 0x20))
        vector4* p = KF(i)->first;
        for (; p != KF(i)->last; p++) {
            p->x *= color.x;  p->y *= color.y;  p->z *= color.z;
        }
    }

    // 0x47aff3: vertex +0x30 and +0x00 terms: scale, then renormalise
    for (size_t i = 0; i < vv.size(); i++) {
        vv[i].n30.x *= color.x;  vv[i].n30.y *= color.y;  vv[i].n30.z *= color.z;
        vnorm(vv[i].n30);
        vv[i].pos.x *= color.x;  vv[i].pos.y *= color.y;  vv[i].pos.z *= color.z;
        vnorm(vv[i].pos);
    }

    long nneg = 0;
    if (color.x < 0.0f) nneg = 1;
    if (color.y < 0.0f) nneg++;
    if (color.z < 0.0f) nneg++;
    if (nneg == 1 || nneg == 3)
        refresh_slots();                            // 0x4779b0

    dirty138 = 1;
    box.lo.x *= color.x;  box.lo.y *= color.y;  box.lo.z *= color.z;
    box.hi.x *= color.x;  box.hi.y *= color.y;  box.hi.z *= color.z;
    box.update();                                  // 0x416f70

    // 0x47b1ff: with a negative component and shaders in use, the vertex +0x10 / +0x20 terms are scaled too
    if (g_use_shaders && nneg > 0) {
        for (size_t i = 0; i < vv.size(); i++) {
            vertex60* v = &vv[i];
            v->n10.x *= color.x;  v->n10.y *= color.y;  v->n10.z *= color.z;
            vnorm(v->n10);
            v->n20.x *= color.x;  v->n20.y *= color.y;  v->n20.z *= color.z;
            vnorm(v->n20);
        }
    }
}

// 0x47aa40 -- append a transformed copy of `src` to this mesh: the matrix rows are re-orthogonalised against
// +Z (f = normalise(cross(up, s)), s = cross(f, up)), `src`'s points are rotated, flattened onto the plane of
// m->u and offset by m->o, its vertices are rotated and tinted by the unpacked colour, and its faces are
// re-added with the vertex indices shifted by this mesh's existing vertex count.
static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// @ 0x47aa40
void mesh::transform(mesh* src, const matrix* m, unsigned color)
{
    if (!local_a0) {
        ((intvec*)((char*)this + 0x90))->assign((intvec*)((char*)parent + 0x90));
        local_a0 = 1;
    }
    ensure_local();

    int n70 = (int)v70.size();
    int nverts = (int)vv.size();

    matrix mm = *m;
    mm.u = vector4(0.0f, 0.0f, 1.0f);
    mm.f = cross_i(mm.u, mm.s);
    mm.f.normalize();
    mm.s = cross_i(mm.f, mm.u);

    std::vector<point20>& pts = src->get70();
    std::vector<vertex60>& sv = src->get50();

    for (size_t i = 0; i < pts.size(); i++) {
        float x = mm.s.x * pts[i].a.x + mm.f.x * pts[i].a.y + mm.u.x * pts[i].a.z;
        float y = mm.u.y * pts[i].a.z + mm.s.y * pts[i].a.x + mm.f.y * pts[i].a.y;
        float z = mm.u.z * pts[i].a.z + mm.f.z * pts[i].a.y + mm.s.z * pts[i].a.x;
        float zz = z - (y * m->u.y + x * m->u.x) / m->u.z;
        vector4 out;
        out.x = x + m->o.x;
        out.y = y + m->o.y;
        out.z = zz + m->o.z;
        add_point(&out);
    }

    vector4 tint;
    unpack_color(&tint, &color);

    for (size_t j = 0; j < sv.size(); j++) {
        vertex60 vt = sv[j];
        float x = mm.s.x * sv[j].pos.x + mm.u.x * sv[j].pos.z + mm.f.x * sv[j].pos.y;
        float y = mm.s.y * sv[j].pos.x + mm.f.y * sv[j].pos.y + mm.u.y * sv[j].pos.z;
        float z = mm.s.z * sv[j].pos.x + mm.f.z * sv[j].pos.y + mm.u.z * sv[j].pos.z;
        vt.index += n70;
        vector4 r(x, y, z);                          // w stays unset: the original copies the stack lane
        vt.pos = r;
        vt.n30 = r;
        vector4 c = *unpack_color(&tint /*scratch*/, (const unsigned*)&vt.bone);
        c.x *= tint.x;  c.y *= tint.y;  c.z *= tint.z;  c.w *= tint.w;
        vt.bone = (int)pack_color(&c);
        ((vvec*)&vv)->push_back(vt);
    }

    std::vector<face20>& faces = src->get90();
    for (size_t i = 0; i < faces.size(); i++)
        add_face(faces[i].a + nverts, faces[i].b + nverts, faces[i].c + nverts);

    dirty138 = 1;
    dirty164 = 1;
    compute_bounds();
}
