// shapes.h: the physics collision shapes of the engine and the helpers of their translation unit
// (0x407c10..0x40e940). One header for src/rgl/shapes/shapes.cpp. Names provisional (the slices' names are
// kept where symbols/slices/*.txt maps a vtable by them: sphere 0x54a4f0, capsule 0x54a540, face 0x54a588,
// manifold 0x54a5f4, composite 0x54a63c, shape_base 0x54a4b8). Every offset below is read or written by the
// function whose address is cited.
//
// Class hierarchy (vtables dumped from .rdata, all slots are virtual calls in the callers):
//   shape_base 0x54a4b8, 0x20 bytes  abstract; slots 0..13
//     sphere    0x54a4f0, 0x44 bytes  (+ copy_from at slot 14)
//     capsule   0x54a540, 0x64 bytes  (+ create_new 14, copy_from 15)
//     face      0x54a588, 0xb4 bytes  (+ copy_from 14)
//     manifold  0x54a5f4, 0x50 bytes  (+ copy_from 14)
//     composite 0x54a63c, 0x28 bytes  (+ copy_from 14)
// Slot map (shape_base): 0 dtor (scalar deleting ??_G, 0x40a540 for sphere/capsule/face), 1 shape_type (0 sphere,
// 1 capsule, 2 face, 3 composite, 4 manifold: the contact dispatcher 0x40bcb0 and composite::save 0x40ced0),
// 2 type_name, 3 load, 4 save, 5 clone, 6 draw, 7 scale, 8 transform, 9 set_mc, 10 get_min, 11 get_max,
// 12 set_m8, 13 or_m8. NOTE: the slices called slot 3 "save" and slot 4 "load" for sphere/capsule/face
// (s00408110, s00408a50): wrong; the sphere's 0x4084c0 calls stream slot 14 (r_float) and slot 19 (r_int).
#ifndef RGL_SHAPES_H
#define RGL_SHAPES_H
#include <math.h>
#include <new>
#include <vector>
#include <windows.h>
#include <d3d9.h>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"

// ---- small helpers of this object ------------------------------------------------------------------------
// 0x407c10 is the out-of-line copy of rgl::string::c_str() (buf at +4, else the address of buf itself).
struct string_view_ : rgl::string {
    const char* get() const;                                // 0x407c10
};
// 0x407c20 returns its this pointer (an unnamed inline helper that was not folded away).
struct self_ref { self_ref* self(); };                      // 0x407c20

// A frame with the point/rotation helpers the shapes and the renderer use (rgl::matrix + 0x408460, 0x40a920,
// 0x40a9c0). The Module System calls the 4x4 frame `pos`.
struct mat3 {
    rgl::vector4 s, f, u;
    mat3() {}
    __forceinline mat3(const rgl::vector4& s_, const rgl::vector4& f_, const rgl::vector4& u_) : s(s_), f(f_), u(u_) {}
};
struct pos : rgl::matrix {
    __declspec(noinline) rgl::vector4 transform_point(const rgl::vector4& p) const;      // 0x408460
    mat3 rot3(const rgl::matrix& m) const;                          // 0x40a920 (rotation rows of m through this)
    __declspec(noinline) rgl::vector4 rot(const rgl::vector4& p) const;                  // 0x40a9c0 (no translation)
    __forceinline rgl::vector4 transform_point_inl(const rgl::vector4& p) const     // the body as callers inline it
    {
        return rgl::vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                            s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                            s.z * p.x + f.z * p.y + u.z * p.z + o.z);
    }
};
void __cdecl vmin(rgl::vector4* out, const rgl::vector4* a, const rgl::vector4* b);     // 0x40af80
void __cdecl vmax(rgl::vector4* out, const rgl::vector4* a, const rgl::vector4* b);     // 0x40b080

// The "ext" declarations. The original's callers of the functions above were compiled without cl's knowledge of
// the callee (cl 13.10 keeps the callee's register use and side effects when the callee was compiled first in the
// same object: the callers then differ in register choice and in reloading fields after the call). Declaring
// the callee under a second name that is only declared reproduces the callers; shapes.cpp binds each ext
// symbol to the real function with /alternatename, so the link is unchanged.
struct pos_ext : rgl::matrix {
    rgl::vector4 transform_point(const rgl::vector4& p) const;      // 0x408460
    rgl::vector4 rot_ext(const rgl::vector4& p) const;              // 0x40a9c0
};
struct matrix_ext : rgl::matrix {
    void orthonormalize();                                          // 0x407ef0
    rgl::vector4 transform_point_to_local(const rgl::vector4& p) const;     // 0x407f70
};
struct vec4_ext : rgl::vector4 { float normalize(); };              // 0x407d60
namespace rgl {
void normalize_ext(vector4& out, const vector4& in);                // 0x407e00
}
void __cdecl vmin_ext(rgl::vector4* out, const rgl::vector4* a, const rgl::vector4* b);     // 0x40af80
void __cdecl vmax_ext(rgl::vector4* out, const rgl::vector4* a, const rgl::vector4* b);     // 0x40b080

// ---- the debug renderer: global state object 0xa17910 -----------------------------------------------------
// Only the fields these shapes use (the D3D9 render-state cache with the same address is src/game/renderer.h,
// which cannot be included here: it pulls in match/include/rgl.h, whose vector4 collides with rgl_math.h).
//   stack/sp: world-matrix stack (translate 0x408170, pop 0x4081b0, apply 0x40a870); dev (+0x618) the D3D device;
//   cur (+0x794) the current world matrix.
struct renderer {
    rgl::matrix stack[20];                  // 0x000
    int sp;                                 // 0x500
    char pad504[0x618 - 0x504];
    IDirect3DDevice9* dev;                  // 0x618
    char pad61c[0x794 - 0x61c];
    rgl::matrix cur;                        // 0x794
    __declspec(noinline) void translate(const rgl::vector4& t);                  // 0x408170
    __declspec(noinline) void pop();                                             // 0x4081b0
    __declspec(noinline) void draw_sphere(float r);                              // 0x4081f0 5x8 wireframe lat/long sphere
    void apply(const rgl::matrix* m);                       // 0x40a870 cur = cur * m
    void draw_capsule(float a, float b, float r);           // 0x40aa20
    void tessellate(float* pts, int* n, float a, float b, float r, int stacks, int slices, int mode);  // 0x40ab10
    void draw_polygon(const rgl::vector4* pts, int n);      // 0x408e60 outline of a <= 4 point polygon
};
extern renderer g_render;                                   // 0xa17910

// ---- shapes ------------------------------------------------------------------------------------------------
struct shape_base {
    int m4;                                 // +0x04  copied by every copy_from (0x4080b0, 0x408690, 0x4089e0, 0x40ce50)
    int m8;                                 // +0x08  flags: 8 initially; set_m8 0x408070, or_m8 0x408080
    int mc;                                 // +0x0c  set_mc 0x408040; composite::set_mc forwards it
    int min_x, min_y, max_x, max_y;         // +0x10..0x1c  1e8 / -1e8 (0x407ff0)
    // 0x407ff0 is the out-of-line constructor; the creation sites of composite (manifold::build_composite 0x40cb60,
    // composite::clone 0x40ce10) call it, while the constructors of the other shapes (0x408600, 0x408980, 0x40c670,
    // the inline ones in composite::load) have the same stores expanded inline: they use the second form.
    struct inline_init {};
    __declspec(noinline) shape_base();                      // 0x407ff0
    __forceinline shape_base(inline_init) : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                                // 0  0x40a540 (deleting)
    virtual int shape_type() = 0;                           // 1
    virtual rgl::string type_name() const = 0;              // 2
    virtual void load(stream_base* s) = 0;                  // 3
    virtual void save(stream_base* s) = 0;                  // 4
    virtual shape_base* clone();                            // 5  0x408020 (returns 0)
    virtual void draw();                                    // 6  0x477b70 (an empty function of another object)
    virtual void scale(const float* s);                     // 7  0x408030
    virtual void transform(const pos* m);                   // 8  0x408030
    virtual void set_mc(const int& v);                      // 9  0x408040
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x408050 (zeroes xyz, returns out)
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x408050
    virtual void set_m8(int v);                             // 12 0x408070
    virtual void or_m8(int v);                              // 13 0x408080
};

// vtable 0x54a4f0, 0x44 bytes
struct sphere : shape_base {
    rgl::vector4 local_center;              // +0x20  load 0x4084c0 / save 0x408540, scale 0x4083f0
    rgl::vector4 center;                    // +0x30  world centre: transform 0x408420, bounds 0x4085a0
    float radius;                           // +0x40  draw 0x408110
    sphere() : shape_base(inline_init()) {}
    sphere(const sphere& o);                                // 0x40c030
    virtual int shape_type();                               // 1  0x408020
    virtual rgl::string type_name() const;                  // 2  0x408090 "sphere"
    virtual void load(stream_base* s);                      // 3  0x4084c0
    virtual void save(stream_base* s);                      // 4  0x408540
    virtual shape_base* clone();                            // 5  0x40c010
    virtual void draw();                                    // 6  0x408110
    virtual void scale(const float* s);                     // 7  0x4083f0
    virtual void transform(const pos* m);                   // 8  0x408420
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x4085a0
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x4085d0
    virtual void copy_from(const sphere* o);                // 14 0x4080b0
};

// vtable 0x54a540, 0x64 bytes
struct capsule : shape_base {
    rgl::vector4 p0, p1;                    // +0x20,+0x30  world segment ends (transform 0x408770; contacts 0x409ac0..)
    rgl::vector4 l0, l1;                    // +0x40,+0x50  local segment ends (load 0x408870, scale 0x408720)
    float radius;                           // +0x60
    capsule() : shape_base(inline_init()) {}                // 0x408600
    capsule(const capsule& o);                              // 0x40a580
    virtual int shape_type();                               // 1  0x48b1f0 (another object)
    virtual rgl::string type_name() const;                  // 2  0x408630 "capsule"
    virtual void load(stream_base* s);                      // 3  0x408870
    virtual void save(stream_base* s);                      // 4  0x408910
    virtual shape_base* clone();                            // 5  0x40a560
    virtual void draw();                                    // 6  0x40a640
    virtual void scale(const float* s);                     // 7  0x408720
    virtual void transform(const pos* m);                   // 8  0x408770
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x40ad80
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x40ade0
    virtual capsule* create_new();                          // 14 0x408650
    virtual void copy_from(const capsule* o);               // 15 0x408690
};

// vtable 0x54a588, 0xb4 bytes: a convex polygon of at most 4 vertices
struct face : shape_base {
    rgl::vector4 w[4];                      // +0x20  world vertices (transform 0x408bd0; contacts read them)
    rgl::vector4 v[4];                      // +0x60  local vertices (load 0x408d60)
    rgl::vector4 normal;                    // +0xa0  world normal (0x408bd0)
    int count;                              // +0xb0  vertex count
    face() : shape_base(inline_init()), count(0) {}         // 0x408980
    face(const face& o);                                    // 0x40ae70
    virtual int shape_type();                               // 1  0x48b590 (another object)
    virtual rgl::string type_name() const;                  // 2  0x4089c0 "face"
    virtual void load(stream_base* s);                      // 3  0x408d60
    virtual void save(stream_base* s);                      // 4  0x408dd0
    virtual shape_base* clone();                            // 5  0x40ae40
    virtual void draw();                                    // 6  0x408e40
    virtual void scale(const float* s);                     // 7  0x408a50
    virtual void transform(const pos* m);                   // 8  0x408bd0
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x40aef0
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x40aff0
    virtual void copy_from(const face* o);                  // 14 0x4089e0
};

// ---- contacts (shape_contacts 0x40bcb0 and the pair functions below it) -----------------------------------
struct contact {                            // 0x24 bytes
    rgl::vector4 normal;
    rgl::vector4 point;
    float depth;
};
void segment_closest_points(const rgl::vector4* a0, const rgl::vector4* a1, const rgl::vector4* b0,
                            const rgl::vector4* b1, rgl::vector4* outA, rgl::vector4* outB);    // 0x409500
void sphere_sphere_contact(sphere* a, sphere* b, int max, contact* out, int* count);            // 0x409980
void capsule_sphere_contact(capsule* a, sphere* b, int max, contact* out, int* count);          // 0x409ac0
float capsule_sphere_gap(capsule* a, sphere* b);                                                // 0x409d20
void capsule_capsule_contact(capsule* a, capsule* b, int max, contact* out, int* count);        // 0x409e90
void polygon_sphere_contact(face* a, sphere* b, int max, contact* out, int* count);              // 0x40a000
void polygon_capsule_contact(face* a, capsule* b, int max, contact* out, int* count);            // 0x40b0f0
void shape_contacts(shape_base* a, shape_base* b, int max, contact* out, int* count);            // 0x40bcb0
int shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);                    // 0x40bfe0

// ---- std::vector instantiations (VC7.1 layout: empty allocator, first, last, end) --------------------------
// Written out so that each out-of-line member can be pinned: clear() (0x41f4b0), _Buy (0x42d570 / 0x4105c0)
// and _Xlen (0x43e2a0) live in other objects.
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

// manifold face record, 0x20 bytes (manifold::save 0x40c6e0, load 0x40d9d0, build_composite 0x40cb60)
struct mface {
    int a;                                  // +0x00
    int flags;                              // +0x04  set_m8 0x40cac0, or_m8 0x40cb00
    int n;                                  // +0x08  index count (<= 4)
    int idx[4];                             // +0x0c  indices into manifold::verts
    int m4;                                 // +0x1c  copied into face::m4 (0x40cb60)
    mface() : a(0), flags(0), n(0), m4(0) {}
};

// vtable 0x54a5f4, 0x50 bytes: a triangle/polygon mesh
struct manifold : shape_base {
    vec<rgl::vector4> verts;                // +0x20 (first at +0x24) working vertices (transform 0x40c880, bounds)
    vec<rgl::vector4> wverts;               // +0x30 vertices as loaded / scaled (0x40d9d0, 0x40c820)
    vec<mface> faces;                       // +0x40
    manifold() : shape_base(inline_init()) {}               // 0x40c670
    virtual ~manifold();                                    // 0  0x40d320 (deleting), 0x40d340
    virtual int shape_type();                               // 1  0x48c460 (another object)
    virtual rgl::string type_name() const;                  // 2  0x40c6c0 "manifold"
    virtual void load(stream_base* s);                      // 3  0x40d9d0
    virtual void save(stream_base* s);                      // 4  0x40c6e0
    virtual shape_base* clone();                            // 5  0x40cb50 -> build_composite
    virtual void scale(const float* s);                     // 7  0x40c820
    virtual void transform(const pos* m);                   // 8  0x40c880
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x40c940
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x40ca00
    virtual void set_m8(int v);                             // 12 0x40cac0
    virtual void or_m8(int v);                              // 13 0x40cb00
    virtual void copy_from(const shape_base* o);            // 14 0x40d3a0
    shape_base* build_composite();                          // 0x40cb60
};

// vtable 0x54a63c, 0x28 bytes: owns its bodies
struct composite : shape_base {
    shape_base** bodies;                    // +0x20
    int count;                              // +0x24
    composite() : bodies(0), count(0) {}
    composite(const composite& o) { ((composite*)this)->copy_from(&o); }
    virtual ~composite();                                   // 0  0x40d140 (deleting), 0x40d160
    virtual int shape_type();                               // 1  0x48b880 (another object)
    virtual rgl::string type_name() const;                  // 2  0x40ceb0 "composite_body"
    virtual void load(stream_base* s);                      // 3  0x40d7d0
    virtual void save(stream_base* s);                      // 4  0x40ced0
    virtual shape_base* clone();                            // 5  0x40ce10
    virtual void draw();                                    // 6  0x40cfb0
    virtual void scale(const float* s);                     // 7  0x40cfe0
    virtual void transform(const pos* m);                   // 8  0x40d070
    virtual void set_mc(const int& v);                      // 9  0x40d0a0
    virtual rgl::vector4* get_min(rgl::vector4* out);       // 10 0x40d1a0
    virtual rgl::vector4* get_max(rgl::vector4* out);       // 11 0x40d260
    virtual void set_m8(int v);                             // 12 0x40d0d0
    virtual void or_m8(int v);                              // 13 0x40d100
    virtual void copy_from(const shape_base* o);            // 14 0x40ce50
};

// shapes composite::load creates inline (vtables 0x54a4f0, 0x54a540, 0x54a588, 0x54a5f4) are the classes above.

// identity frame (0x40cd00, hidden return pointer) and the identity rotation (0x40cd60)
pos* __cdecl identity_pos(pos* out);                        // 0x40cd00
mat3* __cdecl identity3(mat3* out);                         // 0x40cd60

// ---- float / colour helpers and the matrix axis rotations (0x40e3d0..0x40e870) -----------------------------
float clamp(const float* x, const float* lo, const float* hi);     // 0x40e3d0
float frand();                                                      // 0x40e400
struct vec3 { float x, y, z; void add(const vec3& o); };           // 0x40e420
namespace rgl {
// The binary's 0x40e4e0 returns the colour by value (hidden return pointer, eax = it): rgl_math.h declares it as
// `void unpack_color(vector4* out, const unsigned*)`, which cannot reproduce the code. Same call ABI, other symbol.
vector4 unpack_color(const unsigned* argb);                         // 0x40e4e0
}
// rgl::pack_color (0x40e440) and rgl::matrix::rotate_about_* (0x40e570..) are
// declared in rgl_math.h.

#endif
