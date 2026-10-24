// shape.h: the engine's collision shapes, one definition for src/rgl (shapes, scene, resources) and src/game.
// The classes with their own translation unit (src/rgl/shapes/shapes.cpp, 0x407c10..0x40e940) keep their
// definitions of the members there; the callers in other objects use the same classes through this header.
//
//   shape_base 0x54a4b8, 0x20 bytes    vptr + m4/m8/mc/min_x.. (m4 >= 0: the shape is active, 0x470f40)
//     sphere    0x54a4f0, 0x44 bytes   (+ copy_from at slot 14)
//     capsule   0x54a540, 0x64 bytes   (+ create_new 14, copy_from 15)
//     face      0x54a588, 0xb4 bytes   (+ copy_from 14)
//     manifold  0x54a5f4, 0x50 bytes   (src/rgl/shapes/shapes.h: its members need that file's vec<T>)
//     composite 0x54a63c, 0x28 bytes   (+ copy_from 14)
// Slot map (shape_base): 0 dtor (scalar deleting), 1 shape_type (0 sphere, 1 capsule, 2 face, 3 composite,
// 4 manifold: the contact dispatcher 0x40bcb0, composite::save 0x40ced0, body copy 0x4285e0 / 0x439100),
// 2 type_name, 3 load, 4 save, 5 clone, 6 draw, 7 scale, 8 transform, 9 set_mc, 10 get_min, 11 get_max,
// 12 set_m8, 13 or_m8. Every offset below is read or written by the function whose address is cited.
//
// The slots are declared non-pure although the originals' vtable 0x54a4b8 holds _purecall for 1..4: the body
// resource reader (src/rgl/core/resmgr.cpp) instantiates classes derived from shape_base without overriding
// them. Needs rgl::vector4 (rgl_base.h picks the module's own declaration) and `pos` (mat4.h).
//
// Constructors: the original has two shapes of "shape_base()". 0x407ff0 is the out-of-line one (the default
// constructors of composite, and the capsule built in src/game/strat/strat_entity.cpp 0x42a480, call it); the
// constructors of sphere, capsule (0x408600), face (0x408980), the argument constructors in
// src/rgl/xmlcfg/game_util.cpp (0x4332a0, 0x433310) and the local capsule of 0x43f6d0 have the same stores
// expanded inline: they use shape_base(inline_init()).
#ifndef MB_SHAPE_H
#define MB_SHAPE_H
#include "./rgl_base.h"
#include "./mat4.h"

namespace rgl { struct string; }
struct stream_base;

struct shape_base {
    int m4;                                 // +0x04  copied by every copy_from (0x4080b0, 0x408690, 0x4089e0, 0x40ce50)
    int m8;                                 // +0x08  flags: 8 initially; set_m8 0x408070, or_m8 0x408080
    int mc;                                 // +0x0c  set_mc 0x408040; composite::set_mc forwards it
    int min_x, min_y, max_x, max_y;         // +0x10..0x1c  1e8 / -1e8 (0x407ff0)
    struct inline_init {};
    __declspec(noinline) shape_base();                      // 0x407ff0
    __forceinline shape_base(inline_init) : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                                // 0  0x40a540 (deleting)
    virtual int shape_type();                               // 1
    virtual rgl::string type_name() const;                  // 2
    virtual void load(stream_base* s);                      // 3
    virtual void save(stream_base* s);                      // 4
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
    sphere(rgl::vector4 c, const float* r);                 // 0x4332a0 (src/rgl/xmlcfg/game_util.cpp)
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
    capsule(float r, const rgl::vector4* a, const rgl::vector4* b);     // 0x433310 (src/rgl/xmlcfg/game_util.cpp)
    struct outline_init {};
    capsule(outline_init) {}                                // the base constructor out of line (0x407ff0): strat_entity.cpp 0x42a480
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
    rgl::vector4 w[4];                      // +0x20  world vertices (transform 0x408bd0; contacts read them; 0x470f40 w[0])
    rgl::vector4 v[4];                      // +0x60  local vertices (load 0x408d60; 0x479980)
    rgl::vector4 normal;                    // +0xa0  world normal (0x408bd0, 0x433440)
    int count;                              // +0xb0  vertex count (0x4333c0)
    face() : shape_base(inline_init()), count(0) {}         // 0x408980
    face(const face& o);                                    // 0x40ae70
    void intersect(rgl::vector4* out, const rgl::vector4* a, const rgl::vector4* b);   // 0x51a2d0 (ret 0xc; inventory_panel/map_update.cpp 0x5191e0 calls it after a shape_type() == 2 test: the point where the segment a..b meets the face)
    bool contains_xy(const float* p) const;                 // 0x4333c0 (src/rgl/xmlcfg/game_util.cpp)
    float plane_z(const float* p) const;                    // 0x433440 (height of the plane at p; same file)
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

// vtable 0x54a63c, 0x28 bytes: owns its bodies. The copy constructor 0x433470 is out of line (game_util.cpp, called by
// 0x439100); shapes.cpp (clone 0x40ce10) and the body copy 0x4285e0 expand it inline.
struct composite : shape_base {
    shape_base** bodies;                    // +0x20
    int count;                              // +0x24
    composite() : bodies(0), count(0) {}
    composite(const composite& o);                          // 0x433470
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

// vtable 0x54a5f4, 0x50 bytes: a triangle/polygon mesh. The full class (verts, wverts, faces: three vec<T> of
// src/rgl/shapes/shapes.h, which collide with the other vec templates of src/game and src/rgl/scene) is defined by
// shapes.h, which sets MB_SHAPE_FULL_MANIFOLD before including this file; everyone else sees the view, which has the
// size and the one member they call (clone through shape_type() == 4, build_composite 0x40cb60).
#ifndef MB_SHAPE_FULL_MANIFOLD
struct manifold : shape_base {
    char pad[0x30];
    shape_base* build_composite();                          // 0x40cb60
};
#endif

// manifold face record, 0x20 bytes (manifold::save 0x40c6e0, load 0x40d9d0, build_composite 0x40cb60)
struct mface {
    int a;                                  // +0x00
    int flags;                              // +0x04  set_m8 0x40cac0, or_m8 0x40cb00
    int n;                                  // +0x08  index count (<= 4)
    int idx[4];                             // +0x0c  indices into manifold::verts
    int m4;                                 // +0x1c  copied into face::m4 (0x40cb60)
    mface() : a(0), flags(0), n(0), m4(0) {}
};

// a contact point, 0x24 bytes (shape_contacts 0x40bcb0 and the pair functions; the point is copied out by 0x43a720,
// 0x43f6d0)
struct contact {
    rgl::vector4 normal;
    rgl::vector4 point;
    float depth;
};
int shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);                    // 0x40bfe0

#endif // MB_SHAPE_H
