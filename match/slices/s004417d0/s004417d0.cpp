// Slice s004417d0: Agent::build_static_collision (0x4417d0: builds the Newton tree collision of the static faces
// around the agent and adds a body for every capsule shape), a small member that appends to one of five int
// lists, the 0x38-byte E56 uninitialized-copy helper and a push_back on an object with a virtual hook.
// All names provisional. 0x442080 has an EH frame (built with /EHsc).
#include <stddef.h>
#include <vector>
#include <new>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// helper: cross product returned by value (the original builds a temporary and copies it)
// ---- collision shapes (layouts from s00440a30 ai_agent.h / s004333c0.h) ---------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base();
    virtual int shape_type();                           // slot 1: 1 = capsule, 2 = polygon face
};
struct capsule_shape : shape_base {                     // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
};
struct face_shape : shape_base {                        // vtable 0x54a588, size 0xb4
    vector4 v[8];                                       // +0x20
    vector4 normal;                                     // +0xa0
    int count;                                          // +0xb0
};

struct mat3 { vector4 s, f, u; };
mat3* __cdecl identity3(mat3* out);                     // 0x40cd60

// a 4-row frame as the original builds it inline: three rows from identity3, origin zero
struct frame {
    vector4 s, f, u, o;
};

struct NewtonCollision;
extern "C" {
__declspec(dllimport) void __cdecl NewtonTreeCollisionAddFace(const NewtonCollision* c, int count, const float* verts, int stride, int attr);   // 0x54927c
}

// ---- the physics body table (object at 0xa181f8; see s0043a280) ---------------------------------------------
struct PhysRec {                                        // 0x38 bytes
    char pad0[0x2c];
    const NewtonCollision* collision;                   // +0x2c
    char pad30[8];
};
struct PhysSet {
    std::vector<PhysRec> recs;                          // +0 (first at +4)
    void begin_static(int idx);                         // 0x441610
    void add_capsule(int idx, const int* radius, const float* length, const int* zero, const frame* local, const frame* offset, int flag);   // 0x4416c0
    void rebuild_static_body(int idx);                  // 0x43a900
};
extern PhysSet g_phys;                                  // 0xa181f8

// ---- scene and mission ------------------------------------------------------------------------------------------
struct Ctx {
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
};
struct Mission { Ctx* scene; };
extern Mission* g_mission;                              // 0x5e292c

struct AgentBody {
    char pad0[0x58];
    matrix m;                                           // +0x58
};
struct Agent {
    char pad0[0x250];
    AgentBody* body;                                    // +0x250
    char pad254[0x58dc - 0x254];
    int phys_idx;                                       // +0x58dc
    vector4 anchor;                                     // +0x58e0
    void build_static_collision();                      // 0x4417d0
};

// the original inlines vector4::normalize (returns the old length) and the out-parameter cross product
static __forceinline float vnorm(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}

static __forceinline vector4 cross3(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
    return r;
}

// @ 0x4417d0
void Agent::build_static_collision()
{
    anchor = body->m.o;
    g_phys.begin_static(phys_idx);
    shape_base* hits[0x2000];
    int cnt;
    {
        vector4 bmin, bmax;
        bmin.x = body->m.o.x - 5.0f;
        bmin.y = body->m.o.y - 5.0f;
        bmin.z = body->m.o.z - 5.0f;
        bmax.x = body->m.o.x + 5.0f;
        bmax.y = body->m.o.y + 5.0f;
        bmax.z = body->m.o.z + 5.0f;
        g_mission->scene->query_box(&bmin.x, &bmax.x, 0x8106, hits, &cnt);
    }
    int i;
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 2) {
            face_shape* f = (face_shape*)hits[i];
            int tri[12];
            tri[0] = *(int*)&f->v[0].x; tri[1] = *(int*)&f->v[0].y; tri[2] = *(int*)&f->v[0].z;
            tri[3] = *(int*)&f->v[1].x; tri[4] = *(int*)&f->v[1].y; tri[5] = *(int*)&f->v[1].z;
            tri[6] = *(int*)&f->v[2].x; tri[7] = *(int*)&f->v[2].y; tri[8] = *(int*)&f->v[2].z;
            tri[9] = *(int*)&f->v[3].x; tri[10] = *(int*)&f->v[3].y; tri[11] = *(int*)&f->v[3].z;
            NewtonTreeCollisionAddFace(g_phys.recs[phys_idx].collision, ((face_shape*)hits[i])->count, (float*)tri, 12, 0);
        }
    }
    g_phys.rebuild_static_body(phys_idx);
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 1) {
            capsule_shape* c = (capsule_shape*)hits[i];
            vector4 dir;
            int radius = *(int*)&c->radius;
            dir.x = c->p1.x - c->p0.x; dir.y = c->p1.y - c->p0.y; dir.z = c->p1.z - c->p0.z;
            float len = vnorm(dir);
            int zero = 0;
            frame R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t;
            mat3* ip = identity3(&t);
            *(mat3*)&R = *ip;
            R.s = dir;
            R.o = z0;
            vector4 c1 = cross3(R.s, R.f);
            if (c1.length() < 0.001f) {
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
                R.u = cross3(R.s, R.f);
                vnorm(R.u);
            } else {
                R.u = c1;
                vnorm(R.u);
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
            }
            vector4 zero2(0.0f, 0.0f, 0.0f);
            vector4 mid(c->p0.x + c->p1.x, c->p0.y + c->p1.y, c->p0.z + c->p1.z);
            mid.x *= 0.5f; mid.y *= 0.5f; mid.z *= 0.5f;
            R.o = mid;
            frame M;
            mat3 t2;
            mat3* ip2 = identity3(&t2);
            *(mat3*)&M = *ip2;
            M.o = zero2;
            g_phys.add_capsule(phys_idx, &radius, &len, &zero, &R, &M, 1);
        }
    }
}

// ---- <vector> helpers (VC7.1) ----------------------------------------------------------------------------------
// 0x38-byte record: three ints, two int vectors, three ints (see s00440a30).
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};
struct alloc_e56 {
    __declspec(noinline) void construct(E56* p, const E56& v);      // 0x442060
    void destroy(E56* p);                                           // 0x43e150
};
// @ 0x442060
void alloc_e56::construct(E56* p, const E56& v) { new ((void*)p) E56(v); }

// @ 0x442080 (+ catch-all funclet 0x4420ea: destroys [next, dest), then `throw;`)  std::_Uninit_copy (non-scalar tag)
// @ 0x4420ea
E56* __cdecl uninit_copy_e56(const E56* first, const E56* last, E56* dest, alloc_e56& al)
{
    E56* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            al.destroy(next);
        throw;
    }
    return dest;
}

// ---- objects that keep int lists ------------------------------------------------------------------------------
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

struct Poser {
    virtual void slot0();
    virtual void slot1();
    virtual void slot2();
    virtual void slot3();
    virtual void changed();                                 // slot 4 (+0x10)
    char pad4[0x34 - 4];
    std::vector<int> lists[5];                              // +0x34 (stride 0x10)
    int nlists;                                             // +0x84
    char pad88[0x118 - 0x88];
    std::vector<int> extra;                                 // +0x118
    void add_to_list(int idx, int val);                     // 0x441ff0
    void add_extra(int val);                                // 0x442110
};
// @ 0x441ff0
void Poser::add_to_list(int idx, int val)
{
    vpush(lists[idx], val);
    if (idx + 1 > nlists)
        nlists = idx + 1;
}
// @ 0x442110
void Poser::add_extra(int val)
{
    vpush(extra, val);
    changed();
}
