// scene_entity.h: the scene entity family. rgl::entity (vtable 0x54b460, 0x15c bytes, ctor 0x41fba0, deleting dtor
// 0x4214e0 / body 0x4213f0) and its derived classes, one vtable each:
//   0x54b4d8 mesh_entity, 0x54b548 collision_entity (rigid-body base), 0x54b5d0 strategic_entity,
//   0x54b668 dynamic_entity (not in these modules), 0x54b730 skeleton_entity, 0x54b7b8 static_entity.
// The vtable slots are read from the binary (see the cited addresses). Offsets cite a function that reads them.
// ../../game/entity.h defines rgl::entity too (entity.cpp); this header supersedes it (same layout, complete).
#ifndef SCENE_ENTITY_H
#define SCENE_ENTITY_H
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "./scene_math.h"
#include "./scene.h"
#include "./emitter.h"

struct FxDef;                   // fx.h (global, like FxElem)

namespace rgl {

struct world;
struct psys;

// object delete-able through its first virtual slot (an entity's frame helper, a record)
struct Obj { virtual ~Obj(); int owner; int id; };

// The collision shape an entity carries at +0x98 (shape_base of ../resource.h seen through the slots the entity
// code calls): slot 8 positions it, slots 10 / 11 return its two extents. StrategicEntity::update 0x421b70,
// entity::sync 0x41fcd0 (slots 8, 10, 11).
struct coll_shape {
    virtual ~coll_shape();                                      // 0
    virtual int shape_type();                                   // 1
    virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void set_frame(const matrix* m);                    // 8 (+0x20)
    virtual void s9();
    virtual vector4 get_a();                                    // 10 (+0x28)
    virtual vector4 get_b();                                    // 11 (+0x2c)
};

// bounding box with the extend_box member of 0x4201b0 (entity::box at +0xd4, mesh::bounds at +0x22c)
struct box_ext : aabb {
    void extend_box(const aabb* b, const matrix* m);            // 0x4201b0
};

// items attached to an entity (entity::items, +0x128): placed by set_frame (slot 8) and registered with the world
struct item_obj {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void set_frame(const matrix* m);                    // 8 (+0x20)
};
struct dtor_obj { virtual ~dtor_obj(); };                       // anything deleted through slot 0

struct entity {
    virtual ~entity();                                          // 0x4213f0 (body; scalar deleting dtor ??_Gentity is 0x4214e0, slot 0)
    virtual int type_id();                                      // 1: 0x408020
    virtual string type_name();                                 // 2: 0x41fcb0 "entity"
    virtual void sync();                                        // 3: 0x41fcd0
    virtual void recalc();                                      // 4: 0x41ffe0
    virtual float get_radius();                                 // 5: 0x41fd80 (returns fbc)
    virtual void set_fbc(const float* p);                       // 6: 0x41fd90
    virtual void update(float dt);                              // 7: 0x41fda0
    virtual void set_f144(const float* p);                      // 8: 0x41fdd0
    virtual void render(scene* sc, render_info* out);           // 9: 0x420310
    virtual bool awake();                                       // 10: 0x4280e0
    virtual void s11();                                         // 11: 0x4280e0
    virtual const vector4* local_origin();                      // 12: 0x41fde0
    virtual const vector4* get_vac();                           // 13: 0x41fdf0
    virtual const vector4* get_v9c();                           // 14: 0x41fe00
    virtual int is_dirty();                                     // 15: 0x41fe10 (the flag at +0x141; entity.cpp defines it int)
    virtual void load(stream_base* s);                          // 16: 0x4284e0
    virtual int slot17();                                       // 17: 0x408030
    virtual void set_mode(unsigned m);                          // 18: 0x41fe30
    virtual void clear();                                       // 19: 0x420960
    virtual void set_local(const matrix* m);                    // 20: 0x41fe50
    virtual void set_local_dirty(const mat4* m);                // 21: 0x41fe70 (entity.cpp: const mat4*)
    virtual void set_local_scaled(const matrix* m, const vector4* k);        // 22: 0x41fe90
    virtual void set_local_scaled_dirty(const matrix* m, const vector4* k);  // 23: 0x41ff40

    world* owner;               // 0x04  World::create_entity 0x428440
    int id;                     // 0x08  World::create_entity, world::remove_entity 0x41f0d0
    int fc, f10;                // 0x0c
    void* refs;                 // 0x14  cell memberships (world::release_links 0x41ee20)
    matrix prev;                // 0x18  previous frame (StrategicEntity::update 0x421b70: prev = pos)
    smat local;                 // 0x58  the frame (entity::set_local 0x41fe50)
    coll_shape* shape;          // 0x98  collision shape (load 0x4284e0), the frame source of sync 0x41fcd0
    vector4 v9c;                // 0x9c  (StrategicEntity::update stores shape->get_a())
    vector4 vac;                // 0xac
    float fbc;                  // 0xbc  radius (recalc 0x41ffe0)
    float fc0;                  // 0xc0
    vector4 vc4;                // 0xc4  box centre in the local frame (recalc)
    box_ext box;                // 0xd4  (0x34 bytes)
    std::vector<lod_mesh2*> lods;   // 0x108
    std::vector<mesh*> meshes;      // 0x118
    std::vector<item_obj*> items;   // 0x128 attached entities (StrategicEntity::update 0x421b70)
    dtor_obj* p138;             // 0x138 owned objects, deleted by ~entity 0x4213f0
    dtor_obj* p13c;             // 0x13c
    char f140, f141, f142;      // 0x140 visible, dirty flags
    float f144;                 // 0x144 lod distance (passed by address to the lod select)
    unsigned mode;              // 0x148
    vector4 scale;              // 0x14c

    entity();                   // 0x41fba0
};

struct mesh_entity : entity {                                   // vtable 0x54b4d8
    virtual int type_id();                                      // 1: 0x48b1f0
    virtual string type_name();                                 // 2: 0x4214c0 "mesh_entity"
};

struct Contact { vector4 p, n; };                               // 0x20 stored contact (point, normal)
struct contact_vec {            // std::vector<Contact> written out (VC7.1 layout: allocator slot, first, last, end)
    int alloc;
    Contact *first, *last, *end;
    void _Tidy() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
};

struct collision_entity : entity {                              // vtable 0x54b548
    float restitution;          // 0x15c (0.9: inline ctor in skeleton_entity::skeleton_entity 0x427d80)
    contact_vec contacts;       // 0x160
    collision_entity() { contacts.first = 0; contacts.last = 0; contacts.end = 0; restitution = 0.9f; }
    // inline: the body (0x427cd0) is inlined into ~skeleton_entity 0x427e10; out-of-line copy 0x427cd0, deleting 0x427d20
    virtual ~collision_entity() { contacts._Tidy(); }
    virtual int type_id();                                      // 1: 0x48b880
    virtual string type_name();                                 // 2: 0x421500 "collision_entity"
    virtual bool is_dynamic();                                  // 24: 0x48b170
    virtual vector4* point_velocity(vector4* out, const vector4* p);        // 25: 0x421520 (default: zero)
    virtual void apply_impulse(const vector4* p, const vector4* j);         // 26: 0x421540 (default: nothing)
    virtual float impulse_response(const vector4* p, const vector4* n);     // 27: 0x421550 (default: 0.0)
};

struct skeleton_entity : collision_entity {                     // vtable 0x54b730, 0x178 bytes
    FxDef* fx;                  // 0x170 the skeleton / effect instance
    int nfx;                    // 0x174
    skeleton_entity();                                          // 0x427d80
    virtual ~skeleton_entity();                                 // 0x427e10 (body; scalar deleting dtor 0x427e80, slot 0)
    virtual int type_id();                                      // 1: 0x427dc0 (0xb)
    virtual string type_name();                                 // 2: 0x427dd0
    virtual void update(float dt);                              // 7: 0x427df0
    virtual void load(stream_base* s);                          // 16: 0x4286e0
    virtual void render_fx(scene* sc);                          // 28: 0x428f80
};

struct static_entity : entity {                                 // vtable 0x54b7b8
    virtual ~static_entity() {}                                 // 0: 0x4280f0
    virtual int type_id();                                      // 1: 0x493300
    virtual string type_name();                                 // 2: 0x4280c0
    virtual bool awake();                                       // 10: 0x4280e0
    virtual void s11();                                         // 11: 0x48b170
};

// the strategic entity (vtable 0x54b5d0, 0x320 bytes, ctor 0x421720): a mesh entity that carries an effect definition
// and a list of emitters. Offsets: ctor 0x421720 (the stores), update 0x421b70, dtor 0x428120, render 0x42a090.
struct strategic_entity : entity {
    int f15c;                   // 0x15c
    float f160, f164;           // 0x160, 0x164
    int f168, f16c, f170;       // 0x168, 0x16c, 0x170
    int ids[64];                // 0x174 (StratEnt::update_ground_cache 0x42a480)
    int f274;                   // 0x274
    float f278;                 // 0x278 (-1e8)
    char pad27c[0x288 - 0x27c];
    void* p288;                 // 0x288 resource released through the manager
    scene* f28c;                // 0x28c shadow scene (deleted by the dtor 0x428120)
    int* f290;                  // 0x290 plain array
    float f294;                 // 0x294
    int f298;                   // 0x298 (-1)
    char pad29c[4];
    int f2a0;                   // 0x2a0
    char pad2a4[4];
    __int64 t2a8;               // 0x2a8 clock stamp
    int f2b0;                   // 0x2b0
    char pad2b4[4];
    __int64 t2b8;               // 0x2b8
    int f2c0;                   // 0x2c0
    char pad2c4[0x2ec - 0x2c4];
    int f2ec;                   // 0x2ec
    vector4 last_pos;           // 0x2f0
    float thresh;               // 0x300
    FxDef* fx;                  // 0x304
    std::vector<psys*> emitters;    // 0x308 (first at 0x30c)
    float radius;               // 0x318
    int f31c;                   // 0x31c

    strategic_entity();                                         // 0x421720
    virtual ~strategic_entity();                                // 0x428120 (body; scalar deleting dtor 0x428220, slot 0)
    virtual int type_id();                                      // 1: 0x48b390
    virtual string type_name();                                 // 2: 0x421870
    virtual float get_radius();                                 // 5: 0x421890
    virtual void update(float dt);                              // 7: 0x421b70
    virtual void render(scene* sc, render_info* out);           // 9: 0x42a090
    virtual bool awake();                                       // 10: 0x48b170
    virtual void load(stream_base* s);                          // 16: 0x4286d0
    virtual void set_color(int c);                              // 24: 0x4218a0
    virtual void add_emitter(psys* e);                          // 25: 0x421aa0
    virtual void clear_emitters();                              // 26: 0x421b00
    virtual int find_emitter(int tag);                          // 27: 0x421a50
};

// the rigid body entity (vtable 0x54b668, 0x1d8 bytes) is defined in ../dynamic/dynamic_entity.h (collision_entity's
// rigid-body derivation); the entity factory (entity_types.cpp) includes that header for its constructor 0x425370.
struct dynamic_entity;

} // namespace rgl
#endif
