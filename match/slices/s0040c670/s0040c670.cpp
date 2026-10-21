// Slice s0040c670: manifold shape basics: ctor, save, scale/transform, bounds, flags, conversion to composite_body, identity pos helpers, composite_body type/clone/save/update.
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

// @ 0x40c670
manifold::manifold() {}
// @ 0x40c6c0
string manifold::type_name() const { return "manifold"; }
// @ 0x40c6e0
void manifold::save(stream* s)
{
    int n = verts.size();
    s->write_i(&n);
    for (int i = 0; i < n; i++) {
        float* p = &verts[i].x;
        s->write_f(p)->write_f(p + 1)->write_f(p + 2);
    }
    int m = faces.size();
    s->write_i(&m);
    for (int i = 0; i < m; i++) {
        s->write_i(&faces[i].a);
        s->write_i(&faces[i].flags);
        s->write_i(&faces[i].n);
        for (int j = 0; j < faces[i].n; j++)
            s->write_i(&faces[i].idx[j]);
    }
}
// @ 0x40c820
void manifold::scale(const float* s)
{
    for (unsigned i = 0; i < verts.size(); i++) {
        wverts[i].x *= s[0];
        wverts[i].y *= s[1];
        wverts[i].z *= s[2];
    }
}
// @ 0x40c880
void manifold::transform(const pos* m)
{
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4& p = wverts[i];
        float y = m->s.y * p.x; y += m->f.y * p.y; y += m->u.y * p.z; y += m->o.y;
        float z = m->s.z * p.x; z += m->f.z * p.y; z += m->u.z * p.z + m->o.z;
        vector4 t(m->f.x * p.y + m->u.x * p.z + p.x * m->s.x + m->o.x, y, z);
        verts[i] = t;
    }
}
// @ 0x40c940
vector4* manifold::get_min(vector4* out)
{
    vector4 m(1e6f, 1e6f, 1e6f);
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4* v = &verts[i];
        float a = v->x;
        if (!(m.x < a)) m.x = a;
        float b = v->y;
        if (!(m.y < b)) m.y = b;
        float c = v->z;
        if (!(m.z < c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40ca00
vector4* manifold::get_max(vector4* out)
{
    vector4 m(-1e6f, -1e6f, -1e6f);
    for (unsigned i = 0; i < verts.size(); i++) {
        const vector4* v = &verts[i];
        float a = v->x;
        if (!(m.x > a)) m.x = a;
        float b = v->y;
        if (!(m.y > b)) m.y = b;
        float c = v->z;
        if (!(m.z > c)) m.z = c;
    }
    *out = m;
    return out;
}
// @ 0x40cac0
void manifold::set_m8(int v)
{
    m8 = v;
    int n = faces.size();
    for (int i = 0; i < n; i++) faces[i].flags = v;
}
// @ 0x40cb00
void manifold::or_m8(int v)
{
    m8 |= v;
    int n = faces.size();
    for (int i = 0; i < n; i++) faces[i].flags = faces[i].flags | v;
}
// @ 0x40cb50
shape_base* manifold::clone() { return build_composite(); }
// @ 0x40cb60
shape_base* manifold::build_composite()
{
    composite* c = new composite;
    c->count = faces.size();
    c->bodies = new shape_base*[c->count];
    for (int i = 0; i < c->count; i++) {
        face* f = new face;
        c->bodies[i] = f;
        c->bodies[i]->m4 = faces[i].m4;
        c->bodies[i]->m8 = faces[i].flags;
        ((face*)c->bodies[i])->count = faces[i].n;
        for (int j = 0; j < faces[i].n; j++)
            ((face*)c->bodies[i])->v[j] = verts[faces[i].idx[j]];
    }
    pos ident;
    c->transform(identity_pos(&ident));
    c->set_mc(mc);
    c->m8 = m8;
    return c;
}
// @ 0x40cd00
pos* identity_pos(pos* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    mat3 m;
    *(mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}
// @ 0x40cd60
mat3* identity3(mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}
// @ 0x40ce10
shape_base* composite::clone() { return new composite(*this); }
// @ 0x40ce50
void composite::copy_from(const shape_base* o)
{
    const composite* c = (const composite*)o;
    count = c->count;
    bodies = new shape_base*[count];
    for (int i = 0; i < count; i++) bodies[i] = c->bodies[i]->clone();
    m4 = c->m4;
    m8 = c->m8;
}
// @ 0x40ceb0
string composite::type_name() const { return "composite_body"; }
// @ 0x40ced0
void composite::save(stream* s)
{
    name_string name;
    s->write_i(&count);
    for (int i = 0; i < count; i++) {
        if (bodies[i]->shape_type() == 0) s->write_s("sphere");
        else if (bodies[i]->shape_type() == 1) s->write_s("capsule");
        else if (bodies[i]->shape_type() == 2) s->write_s("face");
        else if (bodies[i]->shape_type() == 4) s->write_s("manifold");
        else log_error("unrecognized body \n");
        bodies[i]->save(s);
    }
}
// @ 0x40cfb0
void composite::update()
{
    for (int i = 0; i < count; i++) bodies[i]->update();
}
