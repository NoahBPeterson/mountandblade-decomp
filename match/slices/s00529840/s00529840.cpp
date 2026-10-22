// Slice s00529840: the mission object's per-agent update (0x529840) and the party-stack helpers it
// dispatches to (0x529b70 / 0x529bc0 / 0x529c30 / 0x529d00), plus the scene/mission record sync (0x529d80).
// Everything here is a member of the object at 0x5e292c (agent table at +4); 0x529bc0 is a free __stdcall.
#include <windows.h>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "rgl.h"

using rgl::vector4;

// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------
struct Agent;
struct AgentTable {
    int count;                                                // +0x00
    char pad4[0x18 - 4];
    Agent** pages;                                            // +0x18 (id >> 4 -> page of 16)

    unsigned first_used();                                    // 0x43b1b0 (thiscall)
    void next_used(unsigned* p);                              // 0x43b160 (thiscall, ret 4)
};
struct Agent {
    char pad0[0x10];
    int f10;                                                  // +0x10
    int f14;                                                  // +0x14
    char pad18[0x2c - 0x18];
    int mount;                                                // +0x2c
    char pad30[0x50 - 0x30];
    int f50;                                                  // +0x50
    char pad54[0x88 - 0x54];
    int troop;                                                // +0x88
    char pad8c[0x94 - 0x8c];
    int f94;                                                  // +0x94
    char pad98[0x1bc - 0x98];
    int f1bc;                                                 // +0x1bc (party id)
    int f1c0;                                                 // +0x1c0 (party stack slot)
    char pad1c4[0x55d8 - 0x1c4];
    vector4 facing;                                           // +0x55d8
    char pad55e8[0x571c - 0x55e8];
    float f571c;                                              // +0x571c
    int pad5720;                                              // +0x5720
    float f5724;                                              // +0x5724
    int pad5728[2];                                           // +0x5728
    int f5730;                                                // +0x5730
    char pad5734[0x5928 - 0x5734];
};

// ---- party stacks (0x20-byte records at party+0xa8; layout from s00490a30) ------------------------------
struct Stack {
    int count;                                                // +0x00
    int troop;                                                // +0x04
    int flags;                                                // +0x08
    int f0c;                                                  // +0x0c
    int f10;                                                  // +0x10
    int f14, f18, f1c;
};
struct Party {
    char pad0[0xa8];
    Stack stacks[64];                                         // +0xa8
    int nstacks;                                              // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];

    void clear_exp_share(int slot, int value);                // 0x4b1d80 (thiscall, ret 8)
    int skill(int n);                                         // 0x4b2370 (thiscall, ret 4)
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                            // +0x138d0

    int index_of(unsigned id);                                // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                                       // 0x51ee50 (thiscall, ret 4)
    Party* at(int idx) { return &pages[idx >> 4][idx & 0xf]; }
};

// ---- troop records (0x508 bytes in the table at g_game+0x13a00) ----------------------------------------
struct Troop {
    char pad0[0x30];
    unsigned flags;                                           // +0x30 (0x10 = hero)
    char pad34[0x50 - 0x34];
    int f50;                                                  // +0x50
    char pad54[0xa0 - 0x54];
    float f_a0;                                               // +0xa0
    char pad_a4[0xb0 - 0xa4];
    int b0;                                                   // +0xb0 (level)
    char pad_b4[0x508 - 0xb4];
};

struct GameData {
    char pad0[0x78];
    int cur_party;                                            // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                            // +0x8c
};

// ---- 0x1c8-byte mission/scene record (layout from s00440a30) -------------------------------------------
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    void operator=(const tstring& o);                         // 0x42bc50
};
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    int f10;                                                  // +0x10
    int f14;                                                  // +0x14
    char body1[0x18c - 0x18];
    int f18c;                                                 // +0x18c
    tstring name;                                             // +0x190
    char body2[0x1c8 - 0x1a0];

    Rec1c8();                                                 // 0x437830
    Rec1c8(const Rec1c8& o);                                  // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);                       // 0x437d60
};

// ---- scene record list (the std::vector is at scene+0x104) ---------------------------------------------
struct Scene {
    char pad0[0x104];
    std::vector<Rec1c8> recs;                                 // +0x104
    char pad114[0x494 - 0x114];
};

// ---- the mission object at 0x5e292c --------------------------------------------------------------------
struct Mission {
    int f0;                                                   // +0x00
    AgentTable agents;                                        // +0x04
    char pad20[0x5c - 0x20];
    unsigned cur_agent;                                       // +0x5c
    char pad60[0x5df4 - 0x60];
    int f5df4;                                                // +0x5df4 (scene index)
    char pad5df8[0x6688 - 0x5df8];
    std::vector<Rec1c8> records;                              // +0x6688

    void fn_529840(int mode);                                 // 0x529840 (thiscall, ret 4)
    void fn_529b70(Agent* a);                                 // 0x529b70 (thiscall, ret 4)
    void fn_529c30(int* hist, int party, int slot, int value, int flag); // 0x529c30 (thiscall, ret 0x14)
    void fn_529d00(Agent* a);                                 // 0x529d00 (thiscall, ret 4)
    void fn_529d80();                                         // 0x529d80 (thiscall)
};

// ---- externals -----------------------------------------------------------------------------------------
extern Mission* g_scene;                                      // 0x5e292c
extern GameData* g_game;                                      // 0x5e2930
extern float g_exp_rate;                                      // 0x590c54 (1.0f)

inline PartyMgr* party_mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }
inline Troop* troop_tbl() { return *(Troop**)((char*)g_game + 0x13a00); }

__declspec(noinline) void __stdcall fn_529bc0(Agent* a, int add); // 0x529bc0 (ret 8)

// @ 0x529840
void Mission::fn_529840(int)
{
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        Agent* a = &agents.pages[i >> 4][i & 0xf];
        if (a->f10 != 1 && a->f10 == 0) {
            if (a->f1bc >= 0 && a->f1c0 >= 0) {
                Troop* t = &troop_tbl()[a->troop];
                if (!(t->flags & 0x10) && a->f1bc == g_game->cur_party && t->f50 > 0) {
                    float xp = (float)a->f5730;
                    int cur = troop_tbl()[g_game->cur_troop].b0;
                    if (t->b0 > cur) {
                        int diff = t->b0 - cur;
                        xp = xp / (diff + g_exp_rate);
                        xp = xp * g_exp_rate;
                    }
                    int v = (int)xp;
                    party_mgr()->get(a->f1bc)->clear_exp_share(a->f1c0, v);
                }
                t = &troop_tbl()[a->troop];
                if (t->flags & 0x10) {
                    t->f_a0 = a->f5724;
                    Party* p = party_mgr()->at(party_mgr()->index_of(a->f1bc));
                    int skill = p->skill(9);
                    float v = a->f571c - a->f5724;
                    if (v < 0.0f) v = 0.0f;
                    troop_tbl()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                }
                if (a->f14 != 1) {
                    if (a->f14 == 5)
                        fn_529b70(a);
                    else if (a->f14 == 4)
                        fn_529d00(a);
                }
            } else {
                Troop* t = &troop_tbl()[a->troop];
                if (t->flags & 0x10) {
                    t->f_a0 = a->f5724;
                    if (a->troop == g_game->cur_troop) {
                        Party* p = party_mgr()->at(party_mgr()->index_of(g_game->cur_party));
                        int skill = p->skill(9);
                        float v = a->f571c - a->f5724;
                        if (v < 0.0f) v = 0.0f;
                        troop_tbl()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                    }
                }
            }
        }
        agents.next_used(&i);
    }
}

// @ 0x529b70
void Mission::fn_529b70(Agent* a)
{
    fn_529bc0(a, 1);
    if (a->f1bc == g_game->cur_party)
        fn_529c30((int*)((char*)g_game + 0x1405c), a->f1bc, a->f1c0, a->f94, 0);
}

// @ 0x529bc0
__declspec(noinline) void __stdcall fn_529bc0(Agent* a, int add)
{
    if (a->f1bc < 0) return;
    int slot = a->f1c0;
    if (slot < 0) return;
    if (add)
        party_mgr()->get(a->f1bc)->stacks[slot].count--;
    else
        party_mgr()->get(a->f1bc)->stacks[slot].f10++;
}

// @ 0x529c30
void Mission::fn_529c30(int* hist, int party_id, int slot, int value, int flag)
{
    Party* p = party_mgr()->at(party_mgr()->index_of(party_id));
    int count = p->stacks[slot].count;
    int* h = hist + slot * 32;
    int found = -1;
    int i = 0;
    while (found == -1 && i < count) {
        if (h[i] == value) found = i;
        i++;
    }
    if (found >= 0) {
        if (found < 0x1f)
            memcpy(h + found, h + found + 1, (0x1f - found) * 4);
        h[31] = rand();
        if (flag && p->stacks[slot].count <= 0x20)
            h[p->stacks[slot].count - 1] = value;
    }
}

// @ 0x529d00
void Mission::fn_529d00(Agent* a)
{
    if (a->f1bc >= 0) {
        int slot = a->f1c0;
        if (slot >= 0) {
            Party* p = party_mgr()->get(a->f1bc);
            int* v = (int*)p->stacks + slot * 8 + 4;
            (*v)++;
        }
    }
    if (a->f1bc == g_game->cur_party)
        fn_529c30((int*)((char*)g_game + 0x1405c), a->f1bc, a->f1c0, a->f94, 1);
}

// @ 0x529d80
void Mission::fn_529d80()
{
    Scene* sc = (Scene*)(*(char**)((char*)g_game + 0x13a50) + f5df4 * 0x494);
    for (unsigned i = 0; i < records.size(); i++) {
        Rec1c8* r = &records[i];
        if (r->f10 == 0 && r->f14 >= 0) {
            r->f18c = sc->recs[r->f14].f18c;
            r->name = sc->recs[r->f14].name;
        }
    }
    sc->recs.resize(0);
    for (unsigned i = 0; i < records.size(); i++) {
        Rec1c8* r = &records[i];
        if (r->f10 == 0) {
            sc->recs.push_back(*r);
            r->f14 = (int)sc->recs.size() - 1;
        }
    }
}
