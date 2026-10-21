// Types shared by the effect-entity slices s00421110 / s00421aa0 (this file is copied into each slice dir; keep the
// copies identical). Layouts recovered from the code at 0x421110-0x422130. ALL NAMES ARE PROVISIONAL.
#ifndef FX_TYPES_H
#define FX_TYPES_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "psys.h"

#define OFFSET_CHECK2(T, m, off) typedef char T##_##m##_at2_##off[(offsetof(T, m) == (off)) ? 1 : -1]

// std::vector<Psys*> with a helper that tints every member (0x421990)
struct PsysList : std::vector<Psys*> {
    void set_color(const int& c);                                 // 0x421990
};
// group of up to 5 psys lists, count at +0x84
struct PsysGroup {
    char pad0[0x34];
    PsysList lists[5];                                            // +0x34, 0x10 each
    int nlists;                                                   // +0x84
};
OFFSET_CHECK2(PsysGroup, nlists, 0x84);

// 0x1dc-byte element of an FxDef (vtable 0x54b454; dtor 0x41f1d0)
struct FxElem {
    virtual void v0();
    char pad0[0x40];
    matrix64 m44;                                                 // +0x44
    matrix64 m84;                                                 // +0x84
    char pad1[0x19c - 0xc4];
    PsysList psys;                                                // +0x19c (first +0x1a0)
    char pad2[4];
    std::vector<PsysGroup*> groups;                               // +0x1b0 (first +0x1b4)
    int parent;                                                   // +0x1c0 (-1 = none)
    int has_arr;                                                  // +0x1c4
    int* arr;                                                     // +0x1c8
    char pad3[4];
    float f1d0, f1d4;                                             // +0x1d0, +0x1d4
    char pad4[4];
    ~FxElem();                                                    // 0x41f1d0
    void set_color(int c);                                        // 0x4219c0
};
OFFSET_CHECK2(FxElem, m44, 0x44);
OFFSET_CHECK2(FxElem, psys, 0x19c);
OFFSET_CHECK2(FxElem, groups, 0x1b0);
OFFSET_CHECK2(FxElem, parent, 0x1c0);
OFFSET_CHECK2(FxElem, f1d0, 0x1d0);

// 0xd8-byte envelope record. Its inline dtor frees three arrays, so delete[] goes through the compiler-generated
// vector deleting destructor (0x41ed70).
struct EnvRec {
    float cur;                                                    // +0x00
    char pad0[0x30 - 4];
    float target;                                                 // +0x30
    float release;                                                // +0x34
    float attack;                                                 // +0x38
    char pad1[0x60 - 0x3c];
    float* k0; float* k1; float* k2;                              // +0x60 / +0x64 / +0x68
    char pad2[0xd8 - 0x6c];
    ~EnvRec() { delete[] k0; delete[] k1; delete[] k2; }
};
// effect definition: array of FxElem (count, ptr), psys lists, 0xd8-byte envelope records
struct FxDef {
    int count;                                                    // +0
    FxElem* elems;                                                // +4
    char pad0[0x20 - 8];
    std::vector<Psys*> psys;                                      // +0x20 (first +0x24)
    std::vector<PsysGroup*> groups;                               // +0x30 (first +0x34)
    EnvRec* env;                                                  // +0x40
    int nenv;                                                     // +0x44
    int* ints;                                                    // +0x48
    char pad1[4];
    bool b50;                                                     // +0x50  pose valid
    char pad2[3];
    float f54;                                                    // +0x54  elapsed time
    bool b58;                                                     // +0x58  frozen
    char pad3[0x64 - 0x59];
    void* p64;                                                    // +0x64  second pose source (provisional)
    ~FxDef();                                                     // 0x421680
    void clear_defs();                                            // 0x420ec0
    void set_color(int c);                                        // 0x4218e0
    void update(float dt, const matrix64* m);                     // 0x421df0
    void update_envelopes(float dt);                              // 0x421ef0
    void update_poses();                                          // 0x422080
    void update_poses2();                                         // 0x422130
};
OFFSET_CHECK2(FxDef, psys, 0x20);
OFFSET_CHECK2(FxDef, groups, 0x30);
OFFSET_CHECK2(FxDef, env, 0x40);
OFFSET_CHECK2(FxDef, b50, 0x50);
OFFSET_CHECK2(FxDef, b58, 0x58);
OFFSET_CHECK2(FxDef, p64, 0x64);

// scene entity base (vtable 0x54b460, 54 slots, ctor 0x41fba0, scalar deleting dtor 0x4214e0, dtor 0x4213f0)
struct VObj { virtual ~VObj(); };
struct Ent3 { virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
              virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
              virtual void set_frame(const rgl::matrix* m); };    // slot 8 (+0x20)
struct Ctx { void attach(Ent3* e); };                             // 0x423c00
// object at MeshEntity::p98 (vtable slots 8, 10, 11 used by StrategicEntity::update)
struct Child : VObj {
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7();
    virtual void set_frame(const rgl::matrix* m);                 // slot 8
    virtual void s9();
    virtual rgl::vector4 get_a();                                 // slot 10
    virtual rgl::vector4 get_b();                                 // slot 11
};
struct MeshEntity {
    MeshEntity();                                                 // 0x41fba0
    virtual ~MeshEntity();                                        // slot 0
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
    virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual bool wants_items();                                   // slot 15
    rgl::string type_name();                                      // 0x4214c0
    void cleanup_psys();                                          // 0x420960
    Ctx* ctx;                                                     // +4
    char pad0[0x18 - 8];
    matrix64 prev;                                                // +0x18
    matrix64 pos;                                                 // +0x58
    Child* p98;                                                   // +0x98
    rgl::vector4 v9c, vac;                                        // +0x9c / +0xac
    float fbc;                                                    // +0xbc
    char pad2[0x108 - 0xc0];
    std::vector<void*> v108;                                      // +0x108
    std::vector<Psys*> psys;                                      // +0x118
    std::vector<Ent3*> items;                                     // +0x128
    VObj* p138;
    VObj* p13c;
    char pad3[0x141 - 0x140];
    bool f141, f142;                                              // +0x141 / +0x142
};
OFFSET_CHECK2(MeshEntity, v9c, 0x9c);
OFFSET_CHECK2(MeshEntity, fbc, 0xbc);
OFFSET_CHECK2(MeshEntity, f141, 0x141);
OFFSET_CHECK2(MeshEntity, ctx, 4);
OFFSET_CHECK2(MeshEntity, prev, 0x18);
OFFSET_CHECK2(MeshEntity, pos, 0x58);
OFFSET_CHECK2(MeshEntity, v108, 0x108);
OFFSET_CHECK2(MeshEntity, psys, 0x118);
OFFSET_CHECK2(MeshEntity, items, 0x128);
OFFSET_CHECK2(MeshEntity, p138, 0x138);

// strategic_entity (vtable 0x54b5d0, ctor 0x421720): a mesh entity carrying an effect definition and emitters
extern __int64 g_time[];                                          // 0x5e2948 clock table
struct Mgr { void release(void* p); };                            // 0x4175f0 (this = 0x5d6fa0)
extern Mgr g_mgr;                                                 // 0x5d6fa0
matrix64* identity(matrix64* out);                                         // 0x40cd00 (cdecl, hidden return pointer)
struct StrategicEntity : MeshEntity {
    StrategicEntity();                                            // 0x421720
    rgl::string type_name();                                      // 0x421870
    char pad0[0x15c - sizeof(MeshEntity)];
    int f15c;                                                     // +0x15c
    float f160, f164;                                             // +0x160 / +0x164
    int f168, f16c, f170;                                         // +0x168 / +0x16c / +0x170
    char pad1[0x274 - 0x174];
    int f274;                                                     // +0x274
    float f278;                                                   // +0x278
    char pad2[0x288 - 0x27c];
    void* p288;                                                   // +0x288
    int f28c, f290;                                               // +0x28c / +0x290
    float f294;                                                   // +0x294
    int f298;                                                     // +0x298
    char pad3[4];
    int f2a0;                                                     // +0x2a0
    char pad4[4];
    __int64 t2a8;                                                 // +0x2a8
    int f2b0;                                                     // +0x2b0
    char pad5[4];
    __int64 t2b8;                                                 // +0x2b8
    int f2c0;                                                     // +0x2c0
    char pad6[0x2ec - 0x2c4];
    int f2ec;                                                     // +0x2ec
    rgl::vector4 last_pos;                                        // +0x2f0
    float thresh;                                                 // +0x300
    FxDef* fx;                                                    // +0x304
    std::vector<Emitter*> emitters;                               // +0x308 (first +0x30c)
    float radius;                                                 // +0x318
    int f31c;                                                     // +0x31c
    float get_radius();                                           // 0x421890
    void set_color(int c);                                        // 0x4218a0
    int find_emitter(int tag);                                    // 0x421a50
    void add_emitter(Emitter* e);                                 // 0x421aa0
    void clear_emitters();                                        // 0x421b00
    void update(float dt);                                        // 0x421b70
};
OFFSET_CHECK2(StrategicEntity, f15c, 0x15c);
OFFSET_CHECK2(StrategicEntity, f274, 0x274);
OFFSET_CHECK2(StrategicEntity, p288, 0x288);
OFFSET_CHECK2(StrategicEntity, t2a8, 0x2a8);
OFFSET_CHECK2(StrategicEntity, t2b8, 0x2b8);
OFFSET_CHECK2(StrategicEntity, last_pos, 0x2f0);
OFFSET_CHECK2(StrategicEntity, fx, 0x304);
OFFSET_CHECK2(StrategicEntity, emitters, 0x308);
OFFSET_CHECK2(StrategicEntity, radius, 0x318);
#endif
