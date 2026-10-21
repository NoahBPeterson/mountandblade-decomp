// Slice s0040d7d0: composite_body/manifold load (deserialisation) and the std::vector resize/insert helpers they use.
// Collision shapes of the physics layer: 'manifold' (triangle mesh, vtable 0x54a5f4) and 'composite_body' (vtable 0x54a63c).
#include "rgl.h"
#include <vector>
#include <new>
#include <math.h>
using namespace rgl;


// The original's std::vector<T> instantiations (VC7.1 Dinkumware layout: empty allocator slot, first, last, end).
// Written out so that each out-of-line member can be pinned; clear() (0x41f4b0) and _Buy (0x42d570 / 0x4105c0)
// live in other translation units.
template<class T> T* uninit_copy(T* first, T* last, T* dest, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag);
template<class T> void uninit_fill_n(T* first, unsigned count, const T& val, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag);
template<class T> struct vec {
    std::allocator<T> al;
    T* first; T* last; T* end;
    vec() : first(0), last(0), end(0) {}
    ~vec() { tidy(); }
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    struct iter {                                       // std::_Ptrit
        T* cur;
        iter() : cur(0) {}
        iter(T* p) : cur(p) {}
        T& operator*() const { return *cur; }
        iter& operator+=(int n) { cur += n; return *this; }
        iter operator+(int n) const { iter t = *this; return t += n; }
    };
    iter begin() { return iter(first); }
    T& operator[](unsigned i) { return *(begin() + i); }
    void tidy() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
    static __forceinline T* copy_range(T* f, T* l, T* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
    static __forceinline T* copy_backward_range(T* f, T* l, T* d) { while (f != l) *--d = *--l; return d; }
    static __forceinline void fill_range(T* f, T* l, const T& v) { for (; f != l; ++f) *f = v; }
    __forceinline T* ucopy_in(T* f, T* l, T* p);
    iter end_() { return iter(last); }
    vec& operator=(const vec& r);
    void clear();                                       // 0x41f4b0
    bool buy(unsigned n);                               // 0x42d570 (vector4), 0x4105c0 (mface)
    __declspec(noreturn) void xlen() const;             // 0x43e2a0 (vector4): throws length_error
    void resize(unsigned n) { resize(n, T()); }
    void resize(unsigned n, T val);                     // 0x40db80 (vector4), 0x40dff0 (mface)
    iter erase(iter f, iter l);                         // 0x40dbf0 (vector4), 0x40fd40 (mface)
    void insert_n(iter where, unsigned count, const T& val);   // 0x40dc50 (vector4), 0x40e060 (mface)
    T* ufill(T* p, unsigned count, const T& val);
};

// Serialization stream. Family 1 (slots 3,8,10) writes, family 2 (slots 14,19,21) reads.
struct stream {
    virtual void s0(); virtual void s1(); virtual void s2();
    virtual stream* write_f(float* p);                  // slot 3
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void write_i(void* p);                      // slot 8
    virtual void s9();
    virtual void write_s(const char* s);                // slot 10
    virtual void s11(); virtual void s12(); virtual void s13();
    virtual stream* read_f(float* p);                   // slot 14
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual stream* read_i(void* p);                    // slot 19
    virtual void s20();
    virtual void read_s(void* s);                       // slot 21
};

struct pos : matrix {};

// 16-byte string-like record used as a local by the (de)serializers: ctor 0x42bbd0 zeroes 4 dwords, dtor 0x42bc40 frees.
struct name_string {
    int a; char* p; int b, c;
    name_string();                                      // 0x42bbd0
    ~name_string();                                     // 0x42bc40
    bool equals(const char* lit) const;                 // 0x42bd10
    const char* get() { return p ? p : (const char*)&p; }    // as ptr_or_self::get (0x407c10)
};
void log_error(const char* fmt, ...);                           // 0x42cc80

// Common 0x20-byte header of the collision shapes (vptr + 7 ints); vtable 0x54a4b8 when it is the base.
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1 (0 sphere, 1 capsule, 2 face, 4 manifold)
    virtual string type_name() const;                   // slot 2
    virtual void load(stream* s);                       // slot 3
    virtual void save(stream* s);                       // slot 4
    virtual shape_base* clone();                        // slot 5
    virtual void update();                              // slot 6
    virtual void scale(const float* s);                 // slot 7
    virtual void transform(const pos* m);               // slot 8
    virtual void set_mc(const int& v);                  // slot 9
    virtual vector4* get_min(vector4* out);             // slot 10 (returns out)
    virtual vector4* get_max(vector4* out);             // slot 11 (returns out)
    virtual void set_m8(int v);                         // slot 12
    virtual void or_m8(int v);                          // slot 13
    virtual void copy_from(const shape_base* o);        // slot 14
};

// The same base constructor as an out-of-line function (0x407ff0, defined in another TU).
struct shape_ext : shape_base { shape_ext(); };            // 0x407ff0

// polygon shape, vtable 0x54a588 (see s00408110)
struct face : shape_base {
    vector4 v20, v30, v40, v50;
    vector4 v[4];                // 0x60
    vector4 normal;              // 0xa0
    int count;                   // 0xb0
    face() : count(0) {}
};

struct mface {                                          // 0x20 bytes: manifold face record
    int a, flags, n, idx[4], m4;
    mface() : a(0), flags(0), n(0), m4(0) {}
};

// vtable 0x54a5f4, size 0x50
struct manifold : shape_base {
    vec<vector4> verts;                                 // 0x20 (data ptr at 0x24)
    vec<vector4> wverts;                                // 0x30 (vertices as loaded / scaled)
    vec<mface> faces;                                   // 0x40
    manifold();                                         // 0x40c670
    virtual ~manifold();                                // 0x40d320
    virtual int shape_type();                           // 0x48c460
    virtual string type_name() const;                   // 0x40c6c0
    virtual void load(stream* s);                       // 0x40d9d0
    virtual void save(stream* s);                       // 0x40c6e0
    virtual shape_base* clone();                        // 0x40cb50 -> 0x40cb60
    virtual void update();                              // 0x477b70
    virtual void scale(const float* s);                 // 0x40c820
    virtual void transform(const pos* m);               // 0x40c880
    virtual void set_mc(const int& v);                  // 0x408040
    virtual vector4* get_min(vector4* out);             // 0x40c940
    virtual vector4* get_max(vector4* out);             // 0x40ca00
    virtual void set_m8(int v);                         // 0x40cac0
    virtual void or_m8(int v);                          // 0x40cb00
    virtual void copy_from(const shape_base* o);        // 0x40d3a0
    shape_base* build_composite();                      // 0x40cb60
};

// vtable 0x54a63c, size 0x28
struct composite : shape_ext {
    shape_base** bodies;                                // 0x20
    int count;                                          // 0x24
    composite() : bodies(0), count(0) {}
    composite(const composite& o) { ((shape_base*)this)->copy_from(&o); }
    virtual ~composite();                               // 0x40d140
    virtual int shape_type();                           // 0x48b880
    virtual string type_name() const;                   // 0x40ceb0
    virtual void load(stream* s);                       // 0x40d7d0
    virtual void save(stream* s);                       // 0x40ced0
    virtual shape_base* clone();                        // 0x40ce10
    virtual void update();                              // 0x40cfb0
    virtual void scale(const float* s);                 // 0x40cfe0
    virtual void transform(const pos* m);               // 0x40d070
    virtual void set_mc(const int& v);                  // 0x40d0a0
    virtual vector4* get_min(vector4* out);             // 0x40d1a0
    virtual vector4* get_max(vector4* out);             // 0x40d260
    virtual void set_m8(int v);                         // 0x40d0d0
    virtual void or_m8(int v);                          // 0x40d100
    virtual void copy_from(const shape_base* o);        // 0x40ce50
};

// shapes created by composite::load (vtables 0x54a4f0 and 0x54a540); only the constructors matter here
struct sphere_shape : shape_base { vector4 v20, v30; int m40; };            // size 0x44, vtable 0x54a4f0
struct capsule : shape_base { vector4 p0, p1, p2, p3; float radius; };      // size 0x64, vtable 0x54a540

struct mat3 { vector4 s, f, u; };
__declspec(noinline) mat3* identity3(mat3* out);                             // 0x40cd60
__declspec(noinline) pos* identity_pos(pos* out);                            // 0x40cd00

// @ 0x40d7d0
void composite::load(stream* s)
{
    name_string name;
    s->read_i(&count);
    bodies = new shape_base*[count];
    for (int i = 0; i < count; i++) {
        s->read_s(&name);
        if (name.equals("sphere")) bodies[i] = new sphere_shape;
        else if (name.equals("capsule")) bodies[i] = new capsule;
        else if (name.equals("face")) bodies[i] = new face;
        else if (name.equals("manifold")) bodies[i] = new manifold;
        else log_error("unrecognized body  %s \n", name.get());
        bodies[i]->load(s);
    }
}
// @ 0x40d9d0
void manifold::load(stream* s)
{
    int n; int m; int i; int j;
    s->read_i(&n);
    verts.resize(n);
    wverts.resize(n);
    for (i = 0; i < n; i++) {
        float* p = &wverts[i].x;
        s->read_f(p)->read_f(p + 1)->read_f(p + 2);
        verts[i] = wverts[i];
    }
    s->read_i(&m);
    faces.resize(m);
    for (i = 0; i < m; i++) {
        s->read_i(&faces[i].a);
        s->read_i(&faces[i].flags);
        s->read_i(&faces[i].n);
        for (j = 0; j < faces[i].n; j++)
            s->read_i(&faces[i].idx[j]);
    }
}

// std::_Uninitialized_copy (the out-of-line uninit_copy is 0x40d550 / 0x40d750, in the previous slice)
template<class T> static __forceinline T* ucopy(T* first, T* last, T* dest, std::allocator<T>& al)
{
    return uninit_copy(first, last, dest, al, std::_Ptr_cat(first, dest));
}
// std::vector::_Ucopy
template<class T> __forceinline T* vec<T>::ucopy_in(T* f, T* l, T* p) { return ucopy(f, l, p, al); }

// std::_Uninit_fill_n (nonscalar) with the allocator's construct() expanded
template<class T> __declspec(noinline) void uninit_fill_n(T* first, unsigned count, const T& val, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    T* next = first;
    try {
        for (; 0 < count; --count, ++first)
            ::new ((void*)first) T(val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}
// std::_Uninitialized_fill_n
template<class T> static __forceinline void ufill_n(T* first, unsigned count, const T& val, std::allocator<T>& al)
{
    uninit_fill_n(first, count, val, al, std::_Ptr_cat(first, first));
}
// std::vector::_Ufill
template<class T> __forceinline T* vec<T>::ufill(T* p, unsigned count, const T& val)
{
    ufill_n(p, count, val, al);
    return p + count;
}

// the mface instances of these live in the next slice
template<> void vec<mface>::insert_n(iter where, unsigned count, const mface& val);       // 0x40e060
template<> vec<mface>::iter vec<mface>::erase(iter f, iter l);                            // 0x40fd40

// std::vector::resize(n, val)
template<class T> __declspec(noinline) void vec<T>::resize(unsigned n, T val)
{
    if (size() < n)
        insert_n(end_(), n - size(), val);
    else if (n < size())
        erase(begin() + n, end_());
}
// std::vector::erase(first, last)
template<class T> __declspec(noinline) typename vec<T>::iter vec<T>::erase(iter f, iter l)
{
    if (f.cur != l.cur) {
        T* p = copy_range(l.cur, last, f.cur);
        last = p;
    }
    return f;
}
// std::vector::_Insert_n
template<class T> __declspec(noinline) void vec<T>::insert_n(iter where_, unsigned count, const T& val)
{
    T tmp = val;
    unsigned cap = capacity();
    T* where = where_.cur;
    if (count == 0)
        ;
    else if (al.max_size() - size() < count)
        xlen();
    else if (cap < size() + count) {
        cap = al.max_size() - cap / 2 < cap ? 0 : cap + cap / 2;
        if (cap < size() + count)
            cap = size() + count;
        T* newvec = al.allocate(cap);
        T* ptr = newvec;
        try {
            ptr = ucopy_in(first, where, newvec);
            ptr = ufill(ptr, count, tmp);
            ucopy_in(where, last, ptr);
        } catch (...) {
            al.deallocate(newvec, cap);
            throw;
        }
        count += size();
        if (first != 0)
            al.deallocate(first, end - first);
        end = newvec + cap;
        last = newvec + count;
        first = newvec;
    } else if ((unsigned)(last - where) < count) {
        ucopy_in(where, last, where + count);
        try {
            ufill(last, count - (last - where), tmp);
        } catch (...) {
            throw;
        }
        last += count;
        fill_range(where, last - count, tmp);
    } else {
        T* oldend = last;
        last = ucopy_in(oldend - count, oldend, last);
        copy_backward_range(where, oldend - count, oldend);
        fill_range(where, where + count, tmp);
    }
}
// @ 0x40db80
template void vec<vector4>::resize(unsigned, vector4);
// @ 0x40dbf0
template vec<vector4>::iter vec<vector4>::erase(iter, iter);
// @ 0x40dc50
template void vec<vector4>::insert_n(iter, unsigned, const vector4&);
// @ 0x40df60
template void uninit_fill_n<vector4>(vector4*, unsigned, const vector4&, std::allocator<vector4>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x40dff0
template void vec<mface>::resize(unsigned, mface);
