// agent_combat.h: the classes of agent_combat.cpp (0x43e190..0x441360): the game font's glyph metrics, the
// element records of the std::vector / std::deque instantiations the linker kept from this object, the pool of
// 0xb0-byte records, and (in agent_types.h) the agent's combat helpers. Offsets are the ones read by a function
// of this module (the address is cited at each field). Names provisional.
#ifndef AGENT3_COMBAT_H
#define AGENT3_COMBAT_H
#include <vector>
#include <deque>
#include <memory>
#include <new>
#include <algorithm>
#include <math.h>
#include "./agent_types.h"

// ---- collision shapes used by the hit tests -------------------------------------------------------------------------
// vtable 0x54a540 (rgl::capsule, 0x64 bytes; src/rgl/shapes/shapes.h has the full class). The default constructor is
// inlined into 0x43f6d0, the (radius, a, b) one is 0x433310.
struct capsule : shape_base {
    vector4 v20, v30, v40, v50;
    float radius;
    capsule() {}
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
    virtual ~capsule() {}
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void scale(const float* s);              // slot 7   0x408720
    virtual void transform(const matrix& m);         // slot 8   0x408770
};
struct contact { vector4 a; vector4 point; int extra; };          // 0x24 bytes (0x43f6d0)

#include "../font.h"

// ---- element types of the container instantiations -----------------------------------------------------------
struct E12 { int v[3]; };                            // 0x0c (vector<E12>)
struct E32 { int v[8]; };                            // 0x20 (deque<E32>)
struct E48 { int v[12]; };                           // 0x30 (vector<E48>)
// 0x1c8-byte polymorphic record (vtable 0x590b84, copy ctor 0x437bb0, deleting dtor 0x437870; src/game/agent/agent.h
// calls it PlacedObj). Layout reduced to what the copy / destroy paths touch.
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    char body1[0x190 - 0x10];
    tstring name;                               // +0x190
    char body2[0x1c8 - 0x1a0];
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
};

// ---- id pool: records handed out from pages of 16 (the agent table's allocator) ----------------------------------
// 0xb0-byte record: +4 in-use flag, +8 id, +0xc serial number (ctor at 0x4377d0)
struct PoolRec {
    int f0;
    int used;                   // +4
    int id;                     // +8
    int serial;                 // +0xc
    char pad[0xb0 - 0x10];
    PoolRec();                  // 0x4377d0
    ~PoolRec();
};
struct Pool {
    int next_id;                // +0: ids handed out to pages so far
    std::vector<int> free_ids;  // +4
    std::vector<PoolRec*> pages; // +0x14 (first at +0x18)
    int serial;                 // +0x24
    int alloc();                // 0x440c90 (thiscall, no args; returns the id)
};

// ---- blend / range tables and helpers called by the agent code ------------------------------------------------------
struct RangePair { float lo, hi; };
extern RangePair g_range_tbl[];                         // 0x590aac (indexed by Agent::variant)
float __cdecl blend_target(void* a, void* b);           // 0x4381e0
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
extern int g_ctl_mode;                                  // 0x5aa074 (0 = target, 1 = mouse, 2 = mirrored mouse)
extern int g_stick_mode;                                // 0x5aa078 (0 = none, 1 = stick)
extern int g_invert;                                    // 0x5aa01c
extern float g_stick_x;                                 // 0xa178e0
extern float g_stick_y;                                 // 0xa178e4
int __cdecl group_base(int id);                         // 0x432130
int __cdecl not_group_base(int id);                     // 0x4320b0

#endif // AGENT3_COMBAT_H
