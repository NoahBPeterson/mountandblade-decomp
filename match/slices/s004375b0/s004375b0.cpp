// Slice s004375b0: combat helper methods (aim-error falloff, order-slot bookkeeping, skill level lookup) and the
// 0x1c8-byte Agent record: constructor, deleting destructor, copy constructor and assignment; also the small
// RecJ record constructor / reset (vtable 0x590b78).
#include <stdlib.h>
#include <new>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
    tstring(const tstring& o);                  // 0x42c110
    tstring& operator=(const tstring& o);       // 0x42bc50
};

extern int g_tick_lo;                           // 0x5e2958 (g_time[2], 1e-5 s ticks)
extern int g_tick_hi;                           // 0x5e295c
extern int g_clamp_level;                       // 0x8ee3860 (provisional: highest allowed skill tier)

// ---- troop record (0x508 bytes, array pointer at world+0x13a00) ----
struct Troop {
    char pad0[0x74];
    unsigned m74;                               // +0x74 (packed 4-bit values; bits 12..15 = a skill tier)
    char pad78[0x80 - 0x78];
    float f80[8];                               // +0x80 (per-slot values)
    float falloff(float dist, float k);         // 0x4375b0
    void adjust(int i, float k);                // 0x437770
};
extern const int g_tier_dist[];                 // 0x590a70: 60,100,...,460

// @ 0x4375b0
float Troop::falloff(float dist, float k)
{
    float f = 70.0f / (dist + 70.0f);
    int n = (m74 >> 12) & 0xf;
    if (n < 0)
        n = 0;
    else if (n > g_clamp_level)
        n = g_clamp_level;
    int tier = n;
    if (tier >= 11)
        tier = 10;
    float th = (float)g_tier_dist[tier];
    if (dist > th)
        f = f / (dist - th + 20.0f) * 20.0f;
    if (dist > 50.0f) f *= 0.65f;
    if (dist > 100.0f) f *= 0.65f;
    if (dist > 150.0f) f *= 0.65f;
    if (dist > 200.0f) f *= 0.65f;
    if (dist > 250.0f) f *= 0.65f;
    if (dist > 300.0f) f *= 0.65f;
    if (dist > 350.0f) f *= 0.65f;
    if (dist > 400.0f) f *= 0.65f;
    if (dist > 450.0f) f *= 0.65f;
    if (dist > 500.0f) f *= 0.65f;
    if (dist > 550.0f) f *= 0.65f;
    if (dist > 600.0f) f *= 0.65f;
    if (dist > 650.0f) f *= 0.65f;
    if (dist > 700.0f) f *= 0.65f;
    return f * k + f * k + dist;
}

// The same function seen from outside this TU (cl must not assume the register use of the body above).
struct TroopExt { float falloff(float dist, float k); };   // 0x4375b0
// @ 0x437770
void Troop::adjust(int i, float k)
{
    float r = ((TroopExt*)this)->falloff(f80[i], k);
    f80[i] = r;
    if (r < 0.0f) {
        f80[i] = 0.0f;
        return;
    }
    if (r > 1000.0f)
        f80[i] = 1000.0f;
}

// ---------------------------------------------------------------------------------------------------------
// Polymorphic records derived from IfaceB (vtable 0x5908f8 = ??_7IfaceB@@6B@, see s00431050). IfaceB carries
// three words at +4..+0xc (copied by the Agent copy constructor 0x437bb0).
// ---------------------------------------------------------------------------------------------------------
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};

struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};

// RecJ: 0xb0-byte record, vtable 0x590b78: slot 0 = vector deleting dtor 0x431750 (s00431050), slot 1 = reset().
struct RecJ : IfaceB {
    int pad10[(0x50 - 0x10) / 4];
    int m50;                                    // +0x50
    int pad54[(0x70 - 0x54) / 4];
    Quad q70;                                   // +0x70 (a = 2, c/d = clock tick copied from g_time[2])
    int m80;                                    // +0x80
    int pad84;
    int m88;                                    // +0x88
    int pad8c[(0x98 - 0x8c) / 4];
    int m98;                                    // +0x98
    int pad9c[(0xb0 - 0x9c) / 4];
    RecJ();                                     // 0x4377d0
    virtual void slot1();                       // reset: 0x437810
};

// @ 0x4377d0
RecJ::RecJ()
{
    slot1();
}
// @ 0x437810
void RecJ::slot1()
{
    m50 = 0;
    m98 = 0;
    m88 = -1;
    m80 = -1;
}


// Agent: 0x1c8-byte record, vtable 0x590b84 (see s00434ad0 for the touched fields). Slots 1 and 2 are IfaceB's.
struct Agent : IfaceB {
    int m10, m14;                               // +0x10
    matrix a18, a58, a98, ad8, a118;            // +0x18 .. +0x157 (five 0x40-byte placements)
    vector4 v158;                               // +0x158
    int m168;                                   // +0x168
    float f16c;                                 // +0x16c
    Quad q170;                                  // +0x170 (a = 2 on construction, c/d = clock tick)
    int m180, m184, m188, m18c;                 // +0x180
    tstring name;                               // +0x190
    int m1a0, m1a4, m1a8, m1ac, m1b0, m1b4, m1b8, m1bc, m1c0, m1c4;   // +0x1a0
    Agent();                                    // 0x437830
    Agent(const Agent& o);                      // 0x437bb0
    Agent& operator=(const Agent& o);           // 0x437d60
    // implicit virtual destructor: deleting destructor 0x437870
    void reset();                               // 0x4352f0 (s00434ad0)
};

// @ 0x437830
Agent::Agent()
{
    reset();
}

// Agent copy constructor and assignment (explicit memberwise copies; the compiler-generated ones are identical).
// @ 0x437bb0
Agent::Agent(const Agent& o)
    : IfaceB(o), m10(o.m10), m14(o.m14), a18(o.a18), a58(o.a58), a98(o.a98), ad8(o.ad8), a118(o.a118),
      v158(o.v158), m168(o.m168), f16c(o.f16c), q170(o.q170),
      m180(o.m180), m184(o.m184), m188(o.m188), m18c(o.m18c), name(o.name), m1a0(o.m1a0), m1a4(o.m1a4),
      m1a8(o.m1a8), m1ac(o.m1ac), m1b0(o.m1b0), m1b4(o.m1b4), m1b8(o.m1b8), m1bc(o.m1bc), m1c0(o.m1c0),
      m1c4(o.m1c4)
{
}
// @ 0x437d60
Agent& Agent::operator=(const Agent& o)
{
    IfaceB::operator=(o);
    m10 = o.m10; m14 = o.m14;
    a18 = o.a18; a58 = o.a58; a98 = o.a98; ad8 = o.ad8; a118 = o.a118;
    v158 = o.v158; m168 = o.m168; f16c = o.f16c;
    q170 = o.q170;
    m180 = o.m180; m184 = o.m184; m188 = o.m188; m18c = o.m18c;
    name = o.name;
    m1a0 = o.m1a0; m1a4 = o.m1a4; m1a8 = o.m1a8; m1ac = o.m1ac; m1b0 = o.m1b0; m1b4 = o.m1b4;
    m1b8 = o.m1b8; m1bc = o.m1bc; m1c0 = o.m1c0; m1c4 = o.m1c4;
    return *this;
}

// ---------------------------------------------------------------------------------------------------------
// Combatant (the large per-agent object; names provisional, only touched fields are laid out).
// ---------------------------------------------------------------------------------------------------------
struct vec2 { float x, y; };

// 0x2c-byte order slot, two of them at combatant+0xd4 / +0x100: (kind, target) pair with an expiry stamp.
struct OrderSlot {
    int a;                                      // +0
    int b;                                      // +4 (target id, -1 = none)
    char pad8[0x14 - 8];
    unsigned m14;                               // +0x14 (low 17 bits = priority)
    char pad18[0x20 - 0x18];
    float rate;                                 // +0x20
    char pad24[0x2c - 0x24];
    __declspec(noinline) bool set(int id, unsigned prio);   // 0x4378a0
};

// @ 0x4378a0
bool OrderSlot::set(int id, unsigned prio)
{
    if ((prio & 0x1ffff) < (m14 & 0x1ffff) && (a != -1 || b != -1))
        return false;
    b = id;
    m14 = prio;
    if (id == -1)
        m14 = 0;
    return true;
}

struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; };    // 0x30 bytes
struct DefList { TroopDef* defs; int count; };
struct Curve { void set_rec_phase(int i, float p); };                            // 0x433b20 (s004333c0)
struct CurveOwner { char pad[0x304]; Curve* curve; };                            // +0x304
struct SlotLink { char pad[0xcc]; int m_cc; };

struct Combatant {
    char pad0[0x10];
    int m10;                                    // +0x10
    char pad14[0x2c - 0x14];
    int m2c;                                    // +0x2c
    char pad30[0x60 - 0x30];
    vec2 dir;                                   // +0x60
    char pad68[0xd4 - 0x68];
    OrderSlot slots[2];                         // +0xd4
    DefList* defs;                              // +0x12c
    char pad130[0x14c - 0x130];
    SlotLink* links[4];                         // +0x14c
    char pad15c[0x1d0 - 0x15c];
    struct { int id; unsigned m4; } held[4];    // +0x1d0
    char pad1f0[0x22c - 0x1f0];
    int cur_held;                               // +0x22c
    char pad230[0x250 - 0x230];
    CurveOwner* owner;                          // +0x250
    char pad254[0x560c - 0x254];
    int m560c, m5610;                           // +0x560c
    char pad5614[0x56c8 - 0x5614];
    float turn;                                 // +0x56c8
    void get_dir(vec2* out);                    // 0x4378f0
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
};

// @ 0x4378f0
void Combatant::get_dir(vec2* out)
{
    vec2 v = dir;
    float l = (float)sqrt(v.x * v.x + v.y * v.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        v.x = inv * v.x;
        v.y = inv * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
    if (m10 == 0 && m2c != -1 && (m560c != 0 || m5610 != 0)) {
        float t = turn;
        float c = (float)cos((double)t);
        float s = (float)sin((double)t);
        float nx = c * v.x - s * v.y;
        float tt = c * v.y;
        v.y = tt + s * v.x;
        v.x = nx;
    }
    *out = v;
}

// @ 0x4379c0
void Combatant::set_order(int slot, int id, unsigned prio, float rate)
{
    if (id >= -1 && id < defs->count) {
        if (slots[slot].set(id, prio)) {
            slots[slot].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
            if (prio & 0x4000000)
                owner->curve->set_rec_phase(slot, 0.0f);
            if (slot == 0) {
                if (id >= 0 && (defs->defs[id].flags & 0x400) && slots[1].a != -1 &&
                    (prio & 0x1ffff) > (slots[1].m14 & 0x1ffff))
                    slots[1].set(-1, prio & 0x1ffff);
            } else if (slot == 1) {
                int k = cur_held;
                if (k >= 0 && held[k].id >= 0) {
                    if (prio & 0x10000000)
                        links[k]->m_cc = 0;
                    else
                        links[k]->m_cc = 0xffff;
                }
            }
        }
    }
}

struct World {
    char pad0[0x13a00];
    char* troops;                               // +0x13a00 (array of 0x508-byte records)
    int skill_level(int agent, int skill);      // 0x437b20
};
extern int g_ev_a;                              // 0xa70458
extern int g_ev_b;                              // 0xa7045c
struct EventSink { char emit(int id, int a, int b); };    // 0x435790 (s00435490)
struct SkillCap { char pad[0x34]; int cap; char pad2[0x44 - 0x38]; };   // 0x44 bytes, table at 0x8ee3100
extern SkillCap g_skill_tbl[];                  // 0x8ee3100

// @ 0x437b20
int World::skill_level(int agent, int skill)
{
    unsigned w = ((unsigned*)(troops + 0x68))[agent * 0x142 + skill / 8];
    int lvl = (w >> (skill % 8 * 4)) & 0xf;
    g_ev_a = 0;
    g_ev_b = 0;
    ((EventSink*)this)->emit(0x17, agent, skill);
    lvl += g_ev_a;
    if (lvl < 0)
        return 0;
    int cap = g_skill_tbl[skill].cap;
    if (lvl > cap)
        return cap;
    return lvl;
}
