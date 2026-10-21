// Slice s00438860: assorted engine code at 0x438860..0x4390d0 (names provisional): a smoothed camera-style
// value, VC7.1 <algorithm>/<memory> instantiations (remove_copy, copy_backward, fill, _Uninit_copy,
// _Uninit_fill_n, _Adjust_heap for several element types), the bounds / path-height helpers of the mesh
// instance, and a hit-chance estimate. Flags: /O2 /MT /Gy /TP /EHa (like s0040f300: the catch(...) funclets
// after the _Uninit_* helpers are the `throw;` tails of the helper above them).
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// placement new without the matching placement delete (with <new> cl wraps the throwing copy ctor in an EH frame)
inline void* operator new(size_t, void* p) { return p; }

// 16-byte engine string (layout in s00429890/tstring.h)
struct tstring {
    int pos; char* buf; int cap; int len;
    ~tstring();                                  // 0x42bc40
};

// ---- element types of the STL instantiations ---------------------------------------------------
struct E12 { int a[3]; };                        // 12 bytes (heap node: key float at +8)
struct hnode {                                   // 12-byte heap node; user-written copy (matches the original's load order)
    int a, b; float key;
    hnode() {}
    hnode(const hnode& o) : a(o.a), b(o.b), key(o.key) {}
    hnode& operator=(const hnode& o) { a = o.a; b = o.b; key = o.key; return *this; }
};
struct E1c8 {                                    // 0x1c8-byte polymorphic record (virtual dtor); copy ctor 0x437bb0
    virtual ~E1c8();
    char body[0x1c8 - 4];
    E1c8(const E1c8& o);                         // 0x437bb0
    E1c8& operator=(const E1c8& o);              // 0x437d60
};

// allocator::construct overloads (members; the bodies live elsewhere, declared opaque here)
struct allocE12  { void construct(E12* p, const E12& v); };             // 0x436cf0
struct allocstr  { void construct(tstring* p, const tstring& v); };     // 0x436c90
struct allocint  { void construct(int* p, const int& v); };             // 0x436d80
struct allocE1c8_def { void construct(E1c8* p, const E1c8& v); };
struct allocE1c8 { void construct(E1c8* p, const E1c8& v); };           // 0x439040

// ---- smoothed value (0x438860) -----------------------------------------------------------------
float __cdecl blend_target(void* a, void* b);                            // 0x4381e0
float __cdecl clamp_ref(const float* v, const float* lo, const float* hi);   // 0x4325a0
extern char g_range_tbl[];                                               // 0x590aac: {lo, hi} float pairs per mode

struct Cam {
    char pad0[0x60];
    char blk60[0x55d8 - 0x60];
    char blk55d8[0x56c4 - 0x55d8];
    int idx;                                     // 0x56c4
    float cur;                                   // 0x56c8
    void smooth(float dt);                       // 0x438860
};

// @ 0x438860
void Cam::smooth(float dt)
{
    float v = blend_target(blk60, blk55d8);
    int o = idx * 8;
    float c;
    if (*(float*)(g_range_tbl + o) <= v && v < *(float*)(g_range_tbl + o + 4))
        c = v;
    else
        c = clamp_ref(&v, (float*)(g_range_tbl + o), (float*)(g_range_tbl + o + 4));
    float d = c - cur;
    if (!(fabs(d) < 0.8f))
        c = d * dt * 9.0f + cur;
    cur = c;
}

// ---- 0x438910 (std::remove_copy over vector<int>::iterator) is compiled in s00438860_b.cpp: <vector> brings in <new>,
// whose placement operator new/delete pair changes the EH frames of the _Uninit_* helpers below.

// ---- std::_Uninit_fill_n for 12-byte elements ---------------------------------------------------
// @ 0x438960
void __cdecl uninit_fill_n12(E12* f, unsigned n, const E12& v, allocE12& al, int tag)
{
    E12* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) ;
        throw;
    }
}
// @ 0x4389c4
// (catch funclet of uninit_fill_n12: `throw;`)

// ---- std::copy_backward for 0x1c8-byte elements -------------------------------------------------
// @ 0x4389d0
E1c8* __cdecl copy_backward1c8(E1c8* f, E1c8* l, E1c8* d)
{
    while (f != l) {
        --l; --d;
        *d = *l;
    }
    return d;
}

// ---- std::_Uninit_copy for tstring --------------------------------------------------------------
// @ 0x438a10
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, allocstr& al, int tag)
{
    tstring* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) s->~tstring();
        throw;
    }
    return d;
}
// @ 0x438a7a
// (catch funclet of uninit_copy_str: destroys [s, d) then `throw;`)

// ---- std::_Uninit_copy for 4-byte elements ------------------------------------------------------
// @ 0x438aa0
int* __cdecl uninit_copy_int(int* f, int* l, int* d, allocint& al, int tag)
{
    int* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) ;
        throw;
    }
    return d;
}
// @ 0x438b06
// (catch funclet of uninit_copy_int: `throw;`)

// ---- std::_Adjust_heap (min-heap on key) for 12-byte nodes --------------------------------------
struct node_gt { bool operator()(const hnode& a, const hnode& b) const { return a.key > b.key; } };
void __cdecl push_heap_node(hnode* first, int hole, int top, hnode val, node_gt pred);   // 0x4b2520

// @ 0x438b10
void __cdecl adjust_heap_node(hnode* first, int hole, int bottom, hnode val, node_gt pred)
{
    int top = hole;
    int child = 2 * hole + 2;
    for (; child < bottom; child = 2 * child + 2) {
        if (pred(first[child], first[child - 1]))
            --child;
        first[hole] = first[child];
        hole = child;
    }
    if (child == bottom) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    push_heap_node(first, hole, top, val, pred);
}

// ---- small members ------------------------------------------------------------------------------
struct Flagged {
    char pad0[4];
    unsigned flags;                              // 4
    void refresh(int);                           // 0x437510
    void set_kind(int k);                        // 0x438bc0
};
// @ 0x438bc0
void Flagged::set_kind(int k)
{
    if (k >= 0 && k < 0x2b) {
        ((char*)this)[7] = 0;
        flags |= k << 24;
        refresh(0);
    }
}

struct ptr_vec {
    int al; int* first; int* last; int* end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    bool empty() const { return size() == 0; }
    void pop_back_if();                          // 0x438bf0
};
// @ 0x438bf0
void ptr_vec::pop_back_if()
{
    if (!empty())
        --last;
}

struct vec1c8 {
    int al; E1c8* first; E1c8* last; E1c8* end;
    E1c8* at(int i);                             // 0x438c10
};
// @ 0x438c10
E1c8* vec1c8::at(int i) { return first + i; }

// @ 0x438c30
struct vec_int {
    int al; int* first; int* last; int* end;
    int* fill_ptr(int* f, unsigned n, const int& x);   // 0x438c30
};
int* vec_int::fill_ptr(int* f, unsigned n, const int& x)
{
    unsigned c = n;
    int* p = f;
    for (; 0 < c; --c, ++p) *p = x;
    return f + n;
}

// @ 0x438c60
void __cdecl fill1c8(E1c8* f, E1c8* l, const E1c8& v)
{
    for (; f != l; ++f) *f = v;
}

// @ 0x438c90: vector<E12>::_Ucopy -> _Uninit_copy(first, last, dest, allocator, _Ptr_cat(first, dest))
struct tag_t {};                                 // empty dispatch tag (passed by value; cl pushes a stale register)
inline tag_t ptr_cat(const E12*, const E12*) { tag_t t; return t; }
E12* __cdecl uninit_copy12(E12* f, E12* l, E12* d, void* al, tag_t t);   // 0x436d10
struct vec12 {
    E12* ucopy(E12* a, E12* b, E12* c);          // 0x438c90
};
E12* vec12::ucopy(E12* a, E12* b, E12* c) { return uninit_copy12(a, b, c, this, ptr_cat(a, c)); }

// ---- hit chance (0x438cb0) ----------------------------------------------------------------------
struct Agent {
    char pad0[0x2c];
    int f2c;
    float duration();                            // 0x438360
};
struct Rec {
    char pad0[0x1c8];
    unsigned kind;                               // 0x1c8
    char pad1cc[0x204 - 0x1cc];
    int skill;                                   // 0x204
    char pad208[0x238 - 0x208];
};
extern Rec* g_recs;                              // 0x5e29d0

// @ 0x438cb0
float __cdecl hit_chance(int* who, Agent* a)
{
    float v = a->duration() * 2.0f;
    if (a->f2c >= 0)
        v *= 0.9f;
    int idx = *who;
    int s1;
    if (idx >= 0) {
        Rec* r = &g_recs[idx];
        unsigned k = r->kind & 0xff;
        if (k == 10)
            v *= 0.65f;                          // thrown
        else if (k == 8)
            v *= 0.9f;                           // bow
        else if (k == 0x11 || k == 0x10)
            v *= 0.55f;                          // musket / pistol
        s1 = r->skill;
        if (s1 == 0) s1 = 99;
    } else
        s1 = 100;
    int s2;
    if (idx < 0)
        s2 = 100;
    else {
        s2 = g_recs[idx].skill;
        if (s2 == 0) s2 = 99;
    }
    float p = (float)(s2 * s1) * 0.0001f;
    if (!(p < 1.0f)) p = 1.0f;
    return p * v;
}

// ---- mesh instance helpers ----------------------------------------------------------------------
struct V32 { float x, y, z; float pad[5]; };     // 32-byte vertex record (x, y, z first)
struct vec_e32 {
    int al; V32* first; V32* last; V32* end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    V32& operator[](unsigned i) { return *(first + i); }
};
// min/max as written by the engine: first argument wins ties (the order decides the x87 code)
template<class T> inline T minv(T a, T b) { return (a < b) ? a : b; }
template<class T> inline T maxv(T a, T b) { return (a > b) ? a : b; }

struct mnode {
    char pad0[0x18];
    mnode* master;                               // 0x18
    char pad1c[0x70 - 0x1c];
    vec_e32 verts;                               // 0x70
    char pad80[0xa1 - 0x80];
    char is_owner;                               // 0xa1
    char pada2[2];
    struct { int a0; int* data; int count; } ints;   // 0xa4
    char pad_b0[0xc8 - 0xb0];
    float f_c8;                                  // 0xc8
    vec_e32* get_verts();                        // 0x40eba0
    void* get_ints();                            // 0x40ec00
    void calc_bounds(vector4* mn, vector4* mx);  // 0x438d80
};
// @ 0x438d80
void mnode::calc_bounds(vector4* mn, vector4* mx)
{
    *mn = vector4(1e7f, 1e7f, 1e7f);
    *mx = vector4(-1e7f, -1e7f, -1e7f);
    vec_e32* v = is_owner ? &verts : master->get_verts();
    for (unsigned i = 0; i < v->size(); ++i) {
        const V32& e = (*v)[i];
        mn->x = minv(mn->x, e.x);
        mn->y = minv(mn->y, e.y);
        mn->z = minv(mn->z, e.z);
        const V32& f = (*v)[i];
        mx->x = maxv(mx->x, f.x);
        mx->y = maxv(mx->y, f.y);
        mx->z = maxv(mx->z, f.z);
    }
}

// @ 0x438f00
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, allocstr& al, int tag)
{
    tstring* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) s->~tstring();
        throw;
    }
}
// @ 0x438f68
// (catch funclet of uninit_fill_n_str)

struct node_list {
    int al; mnode** first; mnode** last; mnode** end;
    void cap_all(const float* limit);            // 0x438f90
};
// @ 0x438f90
void node_list::cap_all(const float* limit)
{
    for (mnode** it = first; it < last; ++it) {
        mnode* n = *it;
        struct L { int a0; int* data; int count; };
        L* l = n->is_owner ? (L*)&n->ints : (L*)n->master->get_ints();
        if (0 < l->count) {
            if (*limit > (float)l->data[l->count - 1])
                n->f_c8 = (float)l->data[l->count - 1];
            else
                n->f_c8 = *limit;
        }
    }
}

struct node_groups {
    char pad0[0x34];
    node_list lists[5];                          // 0x34 (16 bytes each)
    int nlists;                                  // 0x84
    void cap_all(const float* limit);            // 0x439000
};
// @ 0x439000
void node_groups::cap_all(const float* limit)
{
    for (int i = 0; i < nlists; ++i)
        lists[i].cap_all(limit);
}

// @ 0x439040
void allocE1c8_def::construct(E1c8* p, const E1c8& v) { new ((void*)p) E1c8(v); }

// ---- std::_Uninit_copy for 0x1c8-byte elements --------------------------------------------------
// @ 0x439060
E1c8* __cdecl uninit_copy1c8(E1c8* f, E1c8* l, E1c8* d, allocE1c8& al, int tag)
{
    E1c8* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) s->~E1c8();
        throw;
    }
    return d;
}
// @ 0x4390d0
// (catch funclet of uninit_copy1c8: destroys [s, d) then `throw;`)
