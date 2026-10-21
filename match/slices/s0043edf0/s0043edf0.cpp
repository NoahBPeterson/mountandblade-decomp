// Slice s0043edf0: VC7.1 <vector>/<deque> instantiations (continued from s0043e514), a Skeleton accessor and
// the agent camera/aim-point helper at 0x43f560. Built WITHOUT an EH switch (see s0043e514.cpp).
//   0x43edf0  vector<E12>::_Insert_n            (+ cold tail 0x43ef97)
//   0x43f0a0  vector<int>::operator=
//   0x43f1f0  deque<E32>::push_back
//   0x43f260  Skeleton::rec(i)  (animator record i, brought up to date first)
//   0x43f2a0  vector<tstring>::push_back
//   0x43f310  vector<int>::resize(n, val)
//   0x43f3a0  vector<E48>::insert(where, val)   (iterator returned through a hidden pointer)
//   0x43f410  vector<Rec1c8>::insert
//   0x43f480  vector<E56>::_Destroy(first, last)
//   0x43f4a0  vector<E12>::insert
//   0x43f510  E56::E56(const E56&)
//   0x43f560  Agent::aim_point_and_dir(...)
#include <vector>
#include <deque>
#include <memory>
#include <new>

// ---- element types ------------------------------------------------------------------------------------
struct E12 { int v[3]; };
struct E32 { int v[8]; };
struct E48 { int v[12]; };

struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};

// 0x38-byte record: see s0043e514 (destroy range 0x43e6e0).
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};

// 0x1c8-byte polymorphic record (see s0043e514).
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    char body1[0x190 - 0x10];
    tstring name;                               // +0x190
    char body2[0x1c8 - 0x1a0];
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
};

// ---- skeleton (animator): see s0043e514 -------------------------------------------------------------
struct Skeleton {
    int nbones;                                 // +0
    char* recs;                                 // +4 (0x1dc-byte records)
    char pad8[0x50 - 8];
    char done;                                  // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                              // +0x64
    void update();                              // 0x422130
    void update_poses();                        // 0x422080
    char* rec(int i);                           // 0x43f260
};

// @ 0x43f260
char* Skeleton::rec(int i)
{
    if (!done) {
        if (has_poses)
            update_poses();
        else
            update();
    }
    return recs + i * 0x1dc + 0x84;
}

// ---- instantiations ---------------------------------------------------------------------------------
// @ 0x43edf0
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);
// @ 0x43f0a0
template std::vector<int>& std::vector<int>::operator=(const std::vector<int>&);
// @ 0x43f1f0
template void std::deque<E32>::push_back(const E32&);
// @ 0x43f2a0
template void std::vector<tstring>::push_back(const tstring&);
// 0x43e310 (s0043e514 / s0043db60)
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
// @ 0x43f310
template void std::vector<int>::resize(size_t, int);
// @ 0x43f3a0
template std::vector<E48>::iterator std::vector<E48>::insert(std::vector<E48>::iterator, const E48&);
// 0x4108b0 (s004108b0)
template void std::vector<E48>::_Insert_n(std::vector<E48>::iterator, size_t, const E48&);
// @ 0x43f410
template std::vector<Rec1c8>::iterator std::vector<Rec1c8>::insert(std::vector<Rec1c8>::iterator, const Rec1c8&);
// 0x43ea00 (s0043e514)
template void std::vector<Rec1c8>::_Insert_n(std::vector<Rec1c8>::iterator, size_t, const Rec1c8&);
// @ 0x43f480
template void std::vector<E56>::_Destroy(E56*, E56*);
// 0x43e6e0 (s0043e514)
template void std::_Destroy_range(E56*, E56*, std::allocator<E56>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x43f4a0
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);

// @ 0x43f510
E56::E56(const E56& o)
    : a(o.a), b(o.b), c(o.c), v1(o.v1), v2(o.v2), d(o.d), e(o.e), f(o.f)
{
}

// ---- agent aim point ----------------------------------------------------------------------------------
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// One 0x1dc-byte animator record seen at skeleton->recs + 0x1140 (the head/hand bone used as aim origin).
struct AimRec {
    char pad0[0x10];
    vector4 offs;               // +0x10
    vector4 dir;                // +0x20
    vector4 base;               // +0x30
};
struct AgentBody {
    char pad0[0x58];
    matrix m;                   // +0x58 (s, f, u rows and origin)
    char pad98[0x304 - 0x98];
    Skeleton* skel;             // +0x304
};
struct AimAgent {
    char pad0[0x250];
    AgentBody* body;            // +0x250
    void aim_point_and_dir(vector4* pos_out, vector4* dir_out);     // 0x43f560
};

static inline vector4 vscale(const vector4& v, float k) { return vector4(v.x * k, v.y * k, v.z * k); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }


static inline vector4 to_world(const matrix& m, const vector4& t)
{
    return vector4(t.x * m.s.x + (t.y * m.f.x + t.z * m.u.x) + m.o.x,
                   t.x * m.s.y + (t.y * m.f.y + t.z * m.u.y) + m.o.y,
                   t.x * m.s.z + (t.y * m.f.z + t.z * m.u.z) + m.o.z);
}
static inline vector4 rot_world(const matrix& m, const vector4& d)
{
    return vector4(m.s.x * d.x + (m.f.x * d.y + m.u.x * d.z),
                   m.s.y * d.x + (m.f.y * d.y + m.u.y * d.z),
                   m.s.z * d.x + (m.f.z * d.y + m.u.z * d.z));
}
// @ 0x43f560
void AimAgent::aim_point_and_dir(vector4* pos_out, vector4* dir_out)
{
    Skeleton* sk = body->skel;
    if (!sk->done) {
        if (sk->has_poses)
            sk->update_poses();
        else
            sk->update();
    }
    AimRec* r = (AimRec*)(sk->recs + 0x1140);
    vector4 t = vadd(vscale(r->offs, 0.2f), r->base);
    const matrix& m = body->m;
    *pos_out = to_world(m, t);
    *dir_out = rot_world(body->m, r->dir);
}
