// entity.h: the scene "entity" object (vtable 0x54b460, ctor 0x41fba0, size 0x15c): a frame with a local and a
// cached world transform, an optional frame source (parent), a bounding box and three child lists.
// Shared by slices s0041f720, s0041ffe0, s00420960 (copied into each). Names are provisional.
#ifndef MB_ENTITY_H
#define MB_ENTITY_H
#include <vector>
#include "mesh.h"

namespace rgl {

struct mat3 { vector4 s, f, u; };

// matrix whose rows can be scaled in place (0x41fee0)
struct smat : mat4 {
    __declspec(noinline) void scale_rows(const vector4* k);                  // 0x41fee0
};

// the optional frame source behind entity::parent (vtable slots used: 8, 10, 11)
struct frame_src {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void apply(const mat4* m);                  // slot 8  (+0x20)
    virtual void s9();
    virtual vector4 get_a();                            // slot 10 (+0x28)
    virtual vector4 get_b();                            // slot 11 (+0x2c)
};

// Pointer element types (only their identity matters: each resize/_Insert_n instantiation is a separate
// function in the original). Obj: vtable + owner (+4) + id (+8).
struct Obj { virtual ~Obj(); int owner; int id; };
struct Poly { virtual ~Poly(); };               // anything with a virtual deleting destructor

// std::vector<point20>::iterator (one pointer, returned by value); operator+ is the out-of-line 0x40f880.
struct iter32 {
    point20* _Ptr;
    iter32() {}
    iter32(point20* p) : _Ptr(p) {}
    iter32 operator+(int off) const;                    // 0x40f880
};
// std::vector<point20>::operator[] const, out of line at 0x40fb80
struct v70_view : std::vector<point20> {
    const point20& at(unsigned i) const;                // 0x40fb80
};

// five lists of child meshes owned by an LOD record; the destructor deletes every child.
struct mesh_list2 {
    std::vector<mesh*> v;
    ~mesh_list2() { for (std::vector<mesh*>::iterator it = v.begin(); it < v.end(); ++it) delete *it; }
};
struct lod_mesh2 {          // 0xe0 bytes; same record as lod_mesh in mesh.h, with the name at 0xd0
    char pad0[0x34];
    mesh_list2 lists[5];    // 0x34
    int count;              // 0x84
    int cur;                // 0x88
    mat4 m8c;               // 0x8c
    unsigned mask;          // 0xcc
    mstr name;              // 0xd0
    void select(const float* dist);                     // 0x4205b0
    void render(scene* sc, const mat4& m);              // 0x413b00
};

// out parameter of entity::render: a hit flag and the view depth of the first point
struct render_info { char valid; float depth; };

struct entity {
    virtual ~entity();                                  // 0: 0x4214e0
    virtual int slot1();                                // 1: 0x408020
    virtual string type_name();                         // 2: 0x41fcb0  "entity"
    virtual void sync();                                // 3: 0x41fcd0
    virtual void recalc();                              // 4: 0x41ffe0
    virtual float get_fbc();                            // 5: 0x41fd80
    virtual void set_fbc(const float* p);               // 6: 0x41fd90
    virtual void commit(int unused);                    // 7: 0x41fda0
    virtual void set_f144(const float* p);              // 8: 0x41fdd0
    virtual void render(scene* sc, render_info* out);          // 9: 0x420310
    virtual void s10();                                 // 10: 0x4280e0
    virtual void s11();                                 // 11: 0x4280e0
    virtual const vector4* local_origin();              // 12: 0x41fde0
    virtual const vector4* get_vac();                   // 13: 0x41fdf0
    virtual const vector4* get_v9c();                   // 14: 0x41fe00
    virtual int is_dirty();                             // 15: 0x41fe10
    virtual void s16();                                 // 16: 0x4284e0
    virtual int slot17();                               // 17: 0x408030
    virtual void set_mode(unsigned m);                  // 18: 0x41fe30
    virtual void clear();                               // 19: 0x420960
    virtual void set_local(const mat4* m);              // 20: 0x41fe50
    virtual void set_local_dirty(const mat4* m);        // 21: 0x41fe70
    virtual void set_local_scaled(const mat4* m, const vector4* k);        // 22: 0x41fe90
    virtual void set_local_scaled_dirty(const mat4* m, const vector4* k);  // 23: 0x41ff40

    int f4, f8, fc, f10, f14;
    mat4 world;                 // 0x18
    smat local;                 // 0x58
    frame_src* parent;          // 0x98
    vector4 v9c;                // 0x9c
    vector4 vac;                // 0xac
    float fbc;                  // 0xbc
    float fc0;                  // 0xc0
    vector4 vc4;                // 0xc4
    aabb box;                   // 0xd4
    std::vector<lod_mesh2*> lods;   // 0x108
    std::vector<mesh*> meshes;      // 0x118
    std::vector<Obj*> v128;         // 0x128
    int f138, f13c;
    char f140, f141, f142;
    float f144;
    unsigned mode;              // 0x148
    vector4 scale;              // 0x14c

    entity();                                           // 0x41fba0
};

} // namespace rgl
#endif
