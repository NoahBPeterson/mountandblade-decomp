// dyn.h: the physics-enabled scene entity "dynamic_entity" (vtable 0x54b668, ctor 0x425370) and the pieces of the
// world / spatial grid it talks to. Shared by slices s004251c0, s004259a0, s004260a0 (copied into each).
// Names are provisional; offsets come from the member functions at 0x4251c0..0x4269e0.
#ifndef MB_DYN_H
#define MB_DYN_H
#include <vector>
#include "rgl.h"

namespace rgl {

// the optional frame source behind entity::parent (vtable slots used: 8, 10, 11) -- same as entity.h
struct frame_src {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void apply(const matrix* m);                // slot 8  (+0x20)
    virtual void s9();
    virtual vector4 get_a();                            // slot 10 (+0x28)
    virtual vector4 get_b();                            // slot 11 (+0x2c)
};

// entity.h's Obj: vtable, owner (+4: the world) and id (+8: group mask)
struct world;

// scene object base: all entity virtuals (vtable 0x54b460 ...), owner (+4: the world) and id (+8: group mask)
// (same name as entity.h's Obj, which only needs the destructor)
struct Obj {
    virtual ~Obj();                                     // 0
    virtual int slot1();                                // 1: type mask
    virtual string type_name();                         // 2: "entity" / "dynamic_entity"
    virtual void sync();                                // 3
    virtual void recalc();                              // 4
    virtual float get_fbc();                            // 5
    virtual void set_fbc(const float* p);               // 6
    virtual void commit(float dt);                      // 7
    virtual void set_f144(const float* p);              // 8
    virtual void render(void* sc, void* out);           // 9
    virtual bool awake();                               // 10
    virtual void s11();                                 // 11
    virtual const vector4* local_origin();              // 12
    virtual const vector4* get_max();                   // 13: 0x41fdf0 (vac)
    virtual const vector4* get_min();                   // 14: 0x41fe00 (v9c)
    virtual int is_dirty();                             // 15
    virtual void s16(); virtual int slot17();           // 16, 17
    virtual void set_mode(unsigned m);                  // 18
    virtual void clear();                               // 19
    virtual void set_local(const matrix* m);            // 20
    virtual void set_local_dirty(const matrix* m);      // 21
    virtual void s22(); virtual void s23();             // 22, 23
    virtual bool is_dynamic();                          // 24 (0x60)

    world* owner;                                       // 4
    int id;                                             // 8
};

// collision shape in a grid cell (sphere/capsule/face/composite..., see s00408110, s0040cfe0): m8 is the group mask,
// cells the rectangle of grid cells it was added to (min x, min y, max x, max y)
struct shape {
    virtual void s0(); virtual int type();              // type(): 3 = composite (group of children), 4 = skipped
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4* get_min(vector4* out);             // slot 10 (+0x28)
    virtual vector4* get_max(vector4* out);             // slot 11 (+0x2c)
    int m4;
    unsigned m8;
    int mc;
    int cells[4];                                       // 0x10
};
struct composite : shape {                              // type() == 3
    shape** bodies;                                     // 0x20
    int count;                                          // 0x24
};

// one record of a cell's entity list: next, (pad), entity
struct enode { enode* next; int pad4; Obj* ent; };

// one grid cell (0x34 bytes): the shapes in it and the list of entities in it
struct cell {
    std::vector<shape*> shapes;                         // 0x00
    char pad10[0x20];
    enode* ents;                                        // 0x30
    void add(shape* s);                                 // 0x423f10 (s00423ed0)
    void collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, shape** out, int* count);   // 0x426500
};

// reference from an entity to one grid cell
struct cell_ref { cell_ref* next; int pad4; int grid; int idx; };

// the world: owns the cell grids. cells[grid] points at an array of 0x34-byte cells.
struct world {
    char pad0[0x28];
    cell** cells;                                       // 0x28
    void locate(const vector4* p, int* out) const;      // 0x41ed00 (s0041e470 takes the same pointer as float*)
    void add_shape(shape* s);                           // 0x4252c0: add to every cell its box covers
    void add_tree(shape* s);                            // 0x425240: same, recursing into composites
    void query_shapes(struct entity* e, shape** out, int* count, int* use3d, unsigned mask);     // 0x426490
    void query_entities(struct entity* e, unsigned mask, bool ordered, std::vector<Obj*>* out);   // 0x426750
    bool less(Obj* a, Obj* b);                          // 0x426960
};

struct contact { vector4 p, n; };                       // 0x20
// std::vector<contact> layout (VC7.1: empty allocator slot, first, last, end), with the out-of-line clear at 0x41f4b0
struct contact_list {
    int al; contact* first; contact* last; contact* end;
    contact_list() : first(0), last(0), end(0) {}
    void clear();                                       // 0x41f4b0
};

struct entity : Obj {
    int fc, f10;
    cell_ref* cells;            // 0x14: grid cells the entity overlaps
    matrix world_m;             // 0x18
    matrix local;               // 0x58
    frame_src* parent;          // 0x98
    vector4 v9c;                // 0x9c: bounds min (from the frame source)
    vector4 vac;                // 0xac: bounds max
    char pad_bc[0x15c - 0xbc];
    entity();                                           // 0x41fba0
};

// rigid-body state of the dynamic entity. Its constructor is inline (forced) and runs before the final vptr store,
// which is why the vptr store of dynamic_entity::dynamic_entity (0x425370) sits between f15c and mass.
struct rigid_base : entity {
    float f15c;                 // 0x15c  (0.9)
    contact_list contacts;      // 0x160
    float mass;                 // 0x170
    float inertia;              // 0x174
    float dt;                   // 0x178
    vector4 force;              // 0x17c
    vector4 torque;             // 0x18c
    vector4 vel;                // 0x19c
    vector4 angvel;             // 0x1ac
    float lin_damp;             // 0x1bc
    float ang_damp;             // 0x1c0
    vector4 shift;              // 0x1c4: pending position delta
    bool grounded;              // 0x1d4
    bool asleep;                // 0x1d5

    __forceinline rigid_base()
    {
        vel = vector4(0.0f, 0.0f, 0.0f);
        angvel = vector4(0.0f, 0.0f, 0.0f);
        force = vector4(0.0f, 0.0f, 0.0f);
        torque = vector4(0.0f, 0.0f, 0.0f);
        shift = vector4(0.0f, 0.0f, 0.0f);
        asleep = false;
        f15c = 0.9f;
    }
};

struct dynamic_entity : rigid_base {
    virtual string type_name();                         // 2: 0x425490
    virtual void commit(float dt);                      // 7: 0x425730
    virtual bool awake();                               // 10: 0x4254b0
    virtual void point_velocity(vector4* out, const vector4* p);           // 25 (0x64): v + w x (p - o)
    virtual void apply_impulse(const vector4* p, const vector4* j);        // 26 (0x68)
    virtual float impulse_response(const vector4* p, const vector4* n);    // 27 (0x6c)
    virtual void apply_forces(float dt);                                   // 28 (0x70)
    virtual void collide();                                                // 29 (0x74)
    virtual void integrate(float dt);                                      // 30 (0x78)
    virtual void resolve();                                                // 31 (0x7c)
    virtual void add_torque(const vector4* t);                             // 32 (0x80)
    virtual void add_torque_at(const vector4* p, const vector4* f);        // 33 (0x84)
    virtual void on_entity(Obj* e);                                        // 34 (0x88)
    virtual void on_shape(shape* s);                                       // 35 (0x8c)

    dynamic_entity();                                                      // 0x425370
};

} // namespace rgl
#endif
