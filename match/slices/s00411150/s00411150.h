// Types shared by the particle-system slices s00411150 / s00411a90 / s00412150 (this file is copied into each
// slice dir; keep the copies identical).  Layouts recovered from the code at 0x411150-0x412a85.
// ALL NAMES ARE PROVISIONAL.  The std::vector instantiations are the real VC7.1 <vector> (xmemory/memory).
#ifndef PSYS_TYPES_H
#define PSYS_TYPES_H
#include <stddef.h>
#include <vector>
#include "rgl.h"

#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]

// 64-byte frame; operator* is the thiscall at 0x40e870 (hidden return pointer, ret 8).
struct matrix64 {
    rgl::vector4 s, f, u, o;
    matrix64 operator*(const matrix64& b) const;                   // 0x40e870
};

// local stand-ins for the inline vector4 operators (rgl.h declares them out of line: 0x407cd0 / 0x407d00; the
// particle code inlines them).  v * s takes s by const reference, s * v too.
inline rgl::vector4 vmul(const rgl::vector4& a, const float& s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
inline rgl::vector4 vmul(const float& s, const rgl::vector4& a) { return rgl::vector4(s * a.x, s * a.y, s * a.z); }
inline rgl::vector4 vadd(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// 8 bytes: (index, weight)
struct IdxWeight { int index; float weight; };
// 0x14 bytes: {bone, vector<IdxWeight>} (vector at +4: allocator pad, first/last/end)
struct BoneInfluence { int bone; std::vector<IdxWeight> verts; };
// 0x60-byte vertex record; only the slot index at +0x50 is read
struct Vert96 { char pad[0x50]; int slot; char pad2[0xc]; };
// 0x30-byte skinning vertex: 4 weights, 4 bone ids, the same ids as floats
struct SkinVert { float w[4]; int bone[4]; float bonef[4]; };
// 0x20-byte POD record
struct Rec32 { int a[4]; int x, y, z; int pad; Rec32() {} };  // user ctor: resize(n) copies an UNINITIALIZED temp
// 0x20 bytes: two arrays of vector4 (ctor 0x411780 zeroes both vectors)
struct VecPair {
    std::vector<rgl::vector4> a, b;
    VecPair();
    void copy_from(const VecPair& o);                              // 0x411c10
};
// 0x64-byte particle
struct Particle {
    char pad0[0x30];
    rgl::vector4 pos;          // +0x30
    rgl::vector4 vel;          // +0x40
    float age;                 // +0x50  (0..1, dies at >= 1)
    float f54;
    float angle;               // +0x58
    float angvel;              // +0x5c
    int   f60;
};
OFFSET_CHECK(Particle, pos, 0x30);
OFFSET_CHECK(Particle, age, 0x50);
OFFSET_CHECK(Particle, f60, 0x60);

// raw 3-field owner: sets (new[]), idx (new[]), n.  dtor deletes sets then idx.
struct MorphSet {
    VecPair* sets; int* idx; int n;
    ~MorphSet() { if (sets) delete[] sets; if (idx) delete[] idx; }
    void assign(const MorphSet& o);                                // 0x4125e0
};
// scoped owner of a raw pointer (dtor deletes), +0xc0 of Psys
struct OwnedPtr { void* p; ~OwnedPtr() { if (p) delete p; } };

struct PsysDef2 { int pad[6]; int i18; };
struct PsysDef {
    char pad0[0x18]; int i18; unsigned flags1c; char pad1[0x10];
    PsysDef2* d30; char pad2[0x30]; int refcount;
};
OFFSET_CHECK(PsysDef, d30, 0x30);
OFFSET_CHECK(PsysDef, refcount, 0x64);

// attached object (flags at +0x18c, registered flag at +0x1a4, active at +0x1ac, params at +0x1b0)
struct Obj {
    char pad0[0x18c];
    unsigned flags;                       // +0x18c
    char pad1[0x14];
    bool registered;                      // +0x1a4
    char pad2[7];
    bool active;                          // +0x1ac
    char pad3[7];
    std::vector<IdxWeight> params;        // +0x1b4 (first +0x1b8)
};
OFFSET_CHECK(Obj, registered, 0x1a4);
OFFSET_CHECK(Obj, active, 0x1ac);
OFFSET_CHECK(Obj, params, 0x1b4);

struct Emitter {
    char pad0[0xc];
    std::vector<Particle> particles;      // +0x0c (first +0x10)
    float noise_scale;                    // +0x1c
    float noise_amount;                   // +0x20
    float drag;                           // +0x24
    char pad1[0x78];
    float lifetime;                       // +0xa0
    char pad2[0x20];
    rgl::vector4 accel;                   // +0xc4
    float accel_scale;                    // +0xd4
    char pad4[0x10];
    unsigned flags;                       // +0xe8
    char pad5[8];
    matrix64 world;                       // +0xf4
    matrix64 local;                       // +0x134
    char pad6[0x24];
    float f198;                           // +0x198
    int   spawn_left;                     // +0x19c
    int   spawn_total;                    // +0x1a0
    float spawn_accum;                    // +0x1a4
    char pad7[4];
    float radius;                         // +0x1ac
    char pad8[4];
    float spin_drag;                      // +0x1b4
    int   noise_index;                    // +0x1b8
    void update(const float* dt, const matrix64& frame);          // 0x412150
    void spawn(Particle* out);                                     // 0x40ec90 (provisional)
};
OFFSET_CHECK(Emitter, particles, 0xc);
OFFSET_CHECK(Emitter, lifetime, 0xa0);
OFFSET_CHECK(Emitter, accel, 0xc4);
OFFSET_CHECK(Emitter, flags, 0xe8);
OFFSET_CHECK(Emitter, world, 0xf4);
OFFSET_CHECK(Emitter, local, 0x134);
OFFSET_CHECK(Emitter, f198, 0x198);
OFFSET_CHECK(Emitter, radius, 0x1ac);
OFFSET_CHECK(Emitter, spin_drag, 0x1b4);

// rgl::string with its dtor (0x42bc40); rgl.h only declares the ctor
struct PName : rgl::string { ~PName(); };                         // 0x42bc40

struct Psys;
// base class: its dtor (0x4150f0) is tail-called by ~Psys; defined elsewhere
struct PsysBase {
    virtual ~PsysBase();
    char pad0[0x14];
    Psys* parent;                                                 // +0x18
};
struct Psys : PsysBase {
    ~Psys();                                                      // 0x4127f0, vptr 0x54a6f4
    int  add_rec2(int x, int y, int z);                           // 0x411920
    void build_skin();                                            // 0x411d20
    void update_emitters(const float* dt, const matrix64& frame); // 0x4126a0
    void ensure_skin();                                           // 0x412700
    virtual void unk_slot1();                                     // 0x48b1f0 (vtable 0x54a6f4 slot 1)
    virtual void compute_sortkey();                               // 0x412980 (slot 2)
    PName name;                                                   // +0x1c
    PsysDef* def;                                                 // +0x2c
    char pad1[0xc];
    unsigned color;                                               // +0x3c
    char pad2[0x10];
    std::vector<Vert96> verts;                                    // +0x50 (first +0x54)
    std::vector<SkinVert> skin;                                   // +0x60
    std::vector<Rec32> recs;                                      // +0x70
    std::vector<BoneInfluence> bones;                             // +0x80
    std::vector<Rec32> recs2;                                     // +0x90
    bool own_recs;                                                // +0xa0
    bool is_root;                                                 // +0xa1
    char pad3[2];
    MorphSet morph;                                               // +0xa4
    std::vector<Emitter*> emitters;                               // +0xb0 (first +0xb4)
    OwnedPtr owned;                                               // +0xc0
    char pad4[8];
    matrix64 xform;                                               // +0xcc
    char pad5[0x4];
    int a110, a114;                                               // +0x110 / +0x114
    char pad6[0x28];
    int a140;                                                     // +0x140
    char pad7[0xd0];
    int a214;                                                     // +0x214
    __int64 sortkey;                                              // +0x218
    int* p220;                                                    // +0x220
};
// owner of attached objects (provisional): vector of Obj* at +0x3a4
struct Scene {
    char pad0[0x3a4];
    std::vector<Obj*> objs;                                       // +0x3a4 (first +0x3a8)
    void register_obj(Obj* e);                                    // 0x412040
    void add_obj_param(Obj* e, int v);                            // 0x412100
};
OFFSET_CHECK(Scene, objs, 0x3a4);
OFFSET_CHECK(Psys, parent, 0x18);
OFFSET_CHECK(Psys, def, 0x2c);
OFFSET_CHECK(Psys, color, 0x3c);
OFFSET_CHECK(Psys, verts, 0x50);
OFFSET_CHECK(Psys, skin, 0x60);
OFFSET_CHECK(Psys, recs, 0x70);
OFFSET_CHECK(Psys, bones, 0x80);
OFFSET_CHECK(Psys, recs2, 0x90);
OFFSET_CHECK(Psys, own_recs, 0xa0);
OFFSET_CHECK(Psys, morph, 0xa4);
OFFSET_CHECK(Psys, emitters, 0xb0);
OFFSET_CHECK(Psys, owned, 0xc0);
OFFSET_CHECK(Psys, xform, 0xcc);
OFFSET_CHECK(Psys, a110, 0x110);
OFFSET_CHECK(Psys, a140, 0x140);
OFFSET_CHECK(Psys, a214, 0x214);
OFFSET_CHECK(Psys, sortkey, 0x218);
OFFSET_CHECK(Psys, p220, 0x220);
#endif
