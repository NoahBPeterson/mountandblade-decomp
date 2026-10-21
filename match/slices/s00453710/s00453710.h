// Slice s00453710 declarations (names provisional; offsets from 0x453710 and the neighbour slices s00432130,
// s00434ad0, s0044e2b0, s004532f0).
#ifndef S00453710_H
#define S00453710_H
#include <stddef.h>
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable { tstring* lookup(const tstring& key) MB_NOTHROW; };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                   // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct pos : matrix {
    vector4 rot_ext(const vector4& p) const;        // 0x40a9c0
};

// item reference (s00434ad0)
struct ItemRef {
    int item;
    unsigned m4;
    int stat_1ec();                     // 0x434c50
    unsigned stat_21x(int which);       // 0x434cb0
    int stat_1f0a();                    // 0x434d70
};
struct ItemKind {                       // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                // +0x1c8
    char pad1cc[0x238 - 0x1cc];
    bool is_melee();                    // 0x433dc0
};
extern ItemKind* g_items;               // 0x5e29d0

struct CharRec {                        // troop record, 0x508 bytes
    char pad0[0x58];
    int f58;                            // +0x58 (strength)
    char pad5c[0x80 - 0x5c];
    float prof[7];                      // +0x80 weapon proficiencies
    char pad9c[0x508 - 0x9c];
};
struct GameData {
    char pad[0x13a00];
    CharRec* chars;                     // +0x13a00
    int skill_level(int troop, int skill);   // 0x437b20 (thiscall, ret 8)
};
extern GameData* g_game;                // 0x5e2930

struct Z_ { char pad[0xd8]; float fd8; };
struct Skeleton {
    int count;                          // +0
    char* elems;                        // +4 (0x1dc-byte records, frame at +0x84)
    char pad8[0x40 - 8];
    Z_* f40;                            // +0x40
    char pad44[0x50 - 0x44];
    char done;                          // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                      // +0x64
    void update();                      // 0x422130
    void update_poses();                // 0x422080
    __forceinline char* rec_inl(int i)
    {
        if (!done) {
            if (has_poses)
                update_poses();
            else
                update();
        }
        return elems + i * 0x1dc + 0x84;
    }
};
struct AgentBody {
    char pad0[0x58];
    pos m;                              // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                     // +0x304
};

struct Vec2 { float x, y; };

struct Agent;
struct Mission {
    int f0;
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;         // +0x18 (page table: id >> 4 -> page of 16 agents)
    char pad28[0x5c - 0x28];
    int local_id;                       // +0x5c
    char pad60[0x6604 - 0x60];
    int mode6604;                       // +0x6604
};
extern Mission* g_mission;              // 0x5e292c

struct Missile {                        // projectile record (param_3)
    char pad0[0x40];
    float dx, dy, dz;                   // +0x40 direction
    char pad4c[0x5c - 0x4c];
    float speed;                        // +0x5c
    float ref_speed;                    // +0x60
    float dmg_scale;                    // +0x64
};

struct Agent {
    char pad0[8];
    int id;                             // +8
    char padc[0x2c - 0xc];
    int mount;                          // +0x2c
    char pad30[0x88 - 0x30];
    int troop;                          // +0x88
    char pad8c[0xa8 - 0x8c];
    Vec2 vel;                           // +0xa8 (x, forward speed y)
    char padb0[0x100 - 0xb0];
    int f100;                           // +0x100
    char pad104[0x118 - 0x104];
    float f118;                         // +0x118
    char pad11c[0x1d0 - 0x11c];
    ItemRef held[10];                   // +0x1d0
    char pad220[0x22c - 0x220];
    int slots[5];                       // +0x22c (slots[4] at +0x23c)
    char pad240[0x250 - 0x240];
    AgentBody* body;                    // +0x250
    char pad254[0x55f8 - 0x254];
    int hand_a;                         // +0x55f8
    char pad55fc[0x5608 - 0x55fc];
    int hand_b;                         // +0x5608
    char pad560c[0x5928 - 0x560c];

    ItemRef slot_pair(int idx) const;   // 0x432790 (thiscall, ret 8)
    __forceinline ItemRef slot_pair_inl(int idx) const
    {
        const ItemRef* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &held[i];
        ItemRef r;
        r.item = -1;
        if (p) { r.item = p->item; r.m4 = p->m4; }
        return r;
    }
    __forceinline bool couching() const
    {
        if (f100 != 0x48) return false;
        return body->skel->f40->fd8 > 0.1f;
    }
    void hit_damage(int hand, Missile* missile, float* out_dmg, unsigned* out_flag, unsigned victim, float* scale);   // 0x453710
};

extern float g_pow_a;                   // 0xa18194
extern float g_pow_b;                   // 0xa18198
extern float g_lance_scale;             // 0xa181a0
extern int g_ui_enabled;                // 0x5aa060

int __cdecl round_float(float* f);      // 0x432540
float __cdecl clamp_float(const float* x, const float* lo, const float* hi);   // 0x40e3d0
#endif
