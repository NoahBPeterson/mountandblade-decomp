// Slice s00494e40: party troop-stack experience award + stack consolidation on the game data
// (0x494e40), the random-experience helper (0x4952a0), a scene-effect spawn helper (0x4952f0),
// a cdecl agent placement routine (0x495430), and three small scene-container helpers
// (0x4955e0 add emitter, 0x495650 push a scene object, 0x495700 remove an object slot).
// Names provisional unless noted. Layouts reused from the neighbouring verified slices
// (s004947a0 GameData/Party/Stack, s0048ca30 item/agent slots, s00492b30 vector/deque layouts).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <deque>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- rgl string (16 bytes; see s0042aaf0/tstring.h) ---------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring() {}
    tstring(const char* s) throw();              // 0x42bbe0
    tstring(const tstring& o) throw();           // 0x42c110
    ~tstring() throw();                          // 0x42bc40
    void operator=(const tstring& o) throw();    // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- the UI root object (0x5d7080; see s0042c9b0) ----------------------------------------------------
struct gui_obj;
struct gui_manager {
    char pad0[0x1728];
    void reset();                                  // 0x42d1b0
    void set_o568(gui_obj* p, bool clear_lines);   // 0x42cdb0
};
extern gui_manager g_gui;                          // 0x5d7080
extern unsigned char g_5d75e0;                     // 0x5d75e0

// ---- game records ------------------------------------------------------------------------------------
struct Troop {                                     // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned __int64 flags;                        // +0x30 (0x10 = hero)
    char pad34[0x50 - 0x34];
    int f50;                                       // +0x50
    int f54;                                       // +0x54
    char pad58[0x9c - 0x58];
    int f9c;                                       // +0x9c
    char padA0[0x508 - 0xa0];
};

// one 0x20-byte stack entry: count, troop, flag byte, then the share-in/should-be counters
struct Stack {
    int count;                                     // +0x00
    int troop;                                     // +0x04
    union { int flags; struct { unsigned char b8; char pad9[3]; }; };  // +0x08 (bit 0 = end marker)
    int f0c;                                       // +0x0c
    int f10;                                       // +0x10
    int f14;                                       // +0x14
    int f18;                                       // +0x18
    int f1c;                                       // +0x1c
};

struct Party;                                      // 0x5ac8 bytes (real size filled in below)
struct PartyMgr {                                  // embedded at game+0xf8; the rest of g_game
    char pad0[0x138d0];
    Party** pages;                                 // +0x138d0 (game+0x139c8; index>>4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    unsigned nquests;                              // +0x138dc (game+0x139d4)
    char pad138e0[0x13908 - 0x138e0];
    Troop* troops;                                 // +0x13908 (game+0x13a00)
    char pad1390c[0x13ebc - 0x1390c];
    vector4 m_pos;                                 // +0x13ebc (game+0x13fb4)
    vector4 m_b;                                   // +0x13ecc (game+0x13fc4; b.x,b.y,b.z)
    char pad13edc[0x13f1c - 0x13edc];
    vector4 m0, m1, m2, m3;                        // +0x13f1c (game+0x14014; rows s,f,u,o)
    int index_of(int id);                          // 0x48ad00 (thiscall, ret 4)
    Party* get_party(int id);                      // 0x51ee50 (thiscall, ret 4)
};

struct Party {                                     // 0x5ac8 bytes
    char pad0[0x4];
    int state;                                     // +0x04 (1 = in mission)
    char pad8[0xa8 - 0x8];
    Stack stacks[64];                              // +0xa8
    int nstacks;                                   // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    unsigned char f8b4;                            // +0x8b4
    char pad8b5[0x5ac8 - 0x8b5];
    void slot_remove(int* out, int idx, int flag);        // 0x489540 (ret 0xc)
    void clear_exp_share(int slot, int amount);           // 0x4b1d80 (ret 8)
    int slot_add(int item, int amount, unsigned mask, int mod, char merge);  // 0x48c840 (ret 0x14)
};

extern int max_scene_slots();                      // 0x4b1700 (== 0x20)

struct GameData {                                  // 0x5e2930
    char pad0[0x8c];
    int f8c;                                       // +0x08c
    char pad90[0xf8 - 0x90];
    PartyMgr pm;                                   // +0x0f8
    Troop* troops_() const { return *(Troop* const*)((const char*)this + 0x13a00); }   // same table as pm.troops
    int add_party_exp(int party, int amount);      // 0x4428c0 (ret 8)
    int add_random_experience(int amount);         // 0x4952a0
    void award_experience(int party, float amount, int mode);   // 0x494e40 (ret 0xc)
};
extern GameData* g_game;                           // 0x5e2930

// ---- scene-effect objects ----------------------------------------------------------------------------
// created by 0x478f30 (0x7c bytes; ctor 0x461480)
struct EffObj {
    int f0;                                        // +0x00
    vector4 pos;                                   // +0x04
    vector4 f14;                                   // +0x14
    vector4 f24;                                   // +0x24
    char pad34[0x54 - 0x34];
    float f54;                                     // +0x54
    char pad58[0x70 - 0x58];
    int f70;                                       // +0x70
    int f74;                                       // +0x74
    char pad78[0x7c - 0x78];
};
struct EffMgr {
    EffObj* create();                              // 0x478f30
};
// `this` of 0x4952f0 holds the effect manager pointer in its first field
struct EffMgrRef {
    EffMgr* mgr;                                   // +0
    void spawn(int a1, int a2);                    // 0x4952f0 (ret 8)
};

// ---- 0x4952a0: award a random fraction of an experience amount to the default party ----------------
// @ 0x4952a0
int GameData::add_random_experience(int amount)
{
    int r = rand() % 15817;
    return add_party_exp(f8c, (int)(((float)r * (1.0f / 15817.0f) + 0.5f) * amount));
}

// ---- 0x494e40: award experience to the troop stacks of a party in the mission ----------------------
// The first pass weighs the party (heroes count 10 / 3, troops count their size), the second
// distributes `amount` over that weight with a random jitter, and the third folds the "share-in"
// counters of the stacks into the leader's stack (adding or removing a stack as needed).
// @ 0x494e40
void GameData::award_experience(int party, float amount, int mode)
{
    if ((unsigned)party >= pm.nquests)
        return;
    PartyMgr& m2 = pm;
    int idx = m2.index_of(party);
    if (idx < 0)
        return;
    if (m2.pages[(unsigned)idx >> 4][idx & 15].state != 1)
        return;
    idx = m2.index_of(party);                      // the original looks the party up a second time
    Party* p = &m2.pages[idx >> 4][idx & 15];
    int m = mode != 0 ? mode - 1 : rand() % 2;

    int total = 0;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops_()[s.troop].flags & 0x10)
            total = (int)((float)total + (i == 0 ? 10.0f : 3.0f));
        else
            total += s.count;
    }
    float per = amount / (float)total;

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops_()[s.troop].flags & 0x10) {
            int r = rand() % 15817;
            float v = ((float)r * (1.0f / 31634.0f) + 0.75f) * per;
            if (i == 0)
                add_party_exp(s.troop, (int)(v * 10.0f));
            else
                add_party_exp(s.troop, (int)(v * 3.0f));
        } else {
            int r = rand() % 15817;
            float v = (((float)r * (1.0f / 15817.0f)) * 0.5f + 0.75f) * (float)s.count * per;
            p->clear_exp_share(i, (int)v);
        }
    }

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            break;
        if (troops_()[s.troop].flags & 0x10)
            continue;
        if (s.f14 <= 0)
            continue;
        int lead = troops_()[s.troop].f50;
        if (m == 1 && (unsigned)troops_()[s.troop].f54 > 0)
            lead = troops_()[s.troop].f54;

        int n = 0;
        for (int j = 0; j < p->nstacks; j++) {
            if (p->stacks[j].flags & 1)
                break;
            n++;
        }
        int k = -1;
        for (int j = 0; j < n; j++)
            if (p->stacks[j].troop == lead)
                k = j;

        if (k >= 0) {
            Stack& d = p->stacks[k];
            d.count += s.f14;
            int extra = (s.f14 - s.count) + s.f10;
            if (extra < 0)
                extra = 0;
            d.f10 += extra;
            if (s.f14 == s.count) {
                int out[8];
                p->slot_remove(out, i, 0);
                i--;
            } else {
                s.count -= s.f14;
                int c = s.count;
                if (s.f10 < c)
                    c = s.f10;
                s.f10 = c;
                s.f14 = 0;
            }
        } else {
            if (s.f14 == s.count) {
                s.troop = lead;
                s.f14 = 0;
                s.f18 = 0;
            } else {
                int n2 = 0;
                for (int j = 0; j < p->nstacks; j++) {
                    if (p->stacks[j].flags & 1)
                        break;
                    n2++;
                }
                if (n2 < max_scene_slots()) {
                    int x = (s.f14 - s.count) + s.f10;
                    if (x < 0)
                        x = 0;
                    p->slot_add(lead, s.f14, 0, x, 0);
                    s.count -= s.f14;
                    int c = s.count;
                    if (s.f10 < c)
                        c = s.f10;
                    s.f10 = c;
                    s.f14 = 0;
                }
            }
        }
    }
}

// ---- 0x4952f0: create a scene-effect object placed by the game-data frame -------------------------
// @ 0x4952f0
void EffMgrRef::spawn(int a1, int a2)
{
    EffObj* o = mgr->create();                     // 0x478f30
    o->f70 = a1;
    o->f74 = a2;
    o->pos = g_game->pm.m_pos;
    o->f0 = 0;
    vector4 v;
    v.x = g_game->pm.m2.x * g_game->pm.m_b.z + g_game->pm.m1.x * g_game->pm.m_b.y +
          g_game->pm.m0.x * g_game->pm.m_b.x + g_game->pm.m3.x;
    v.y = g_game->pm.m0.y * g_game->pm.m_b.x + g_game->pm.m2.y * g_game->pm.m_b.z +
          g_game->pm.m1.y * g_game->pm.m_b.y + g_game->pm.m3.y;
    v.z = g_game->pm.m0.z * g_game->pm.m_b.x + g_game->pm.m2.z * g_game->pm.m_b.z +
          g_game->pm.m1.z * g_game->pm.m_b.y + g_game->pm.m3.z;
    o->f24 = v;
    o->f14 = o->pos;
    o->f54 = (o->pos.z + o->pos.y + o->pos.x) * 10.0f;
}

// ---- 0x495430: show/hide the carried meshes of an agent's four slots --------------------------------
// cdecl; arg1 = the agent body holder (skeleton at +0x304), arg2 = four carried item refs
// {item, modifier} indexed [3..0], arg3 = the agent's three per-slot mesh arrays A/B/C (four
// pointers each). For every valid item, item_kind::get_carry_pos yields the local frame M, an
// attach-group index and the bone the item occupies; the per-bone `placed` bitmap refuses a second
// item on an occupied bone (and a shield with a small modifier). A/B meshes are moved onto the bone
// (removed from the skeleton, re-framed, added to the bone's group); C meshes are only shown/hidden.
// @ 0x495430
struct ItemKind {                                  // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned char type;                            // +0x1c8 (7 = shield)
    char pad1c9[0x1d0 - 0x1c9];
    unsigned f1d0;                                 // +0x1d0
    unsigned f1d4;                                 // +0x1d4
    char pad1d8[0x238 - 0x1d8];
    void get_carry_pos(rgl::matrix* out, int* attach, int* bone);   // 0x433e40 (thiscall, ret 0xc)
};
extern ItemKind* g_items;                          // 0x5e29d0

struct Mesh {                                      // a carried mesh: visibility mask and local frame
    char pad0[0x8c];
    rgl::matrix frame;                             // +0x8c
    unsigned vis;                                  // +0xcc (0xffff shown, 0 hidden)
};

struct ItemRefMin { int item; unsigned m4; };      // an 8-byte carried item ref
struct PlaceCtx {                                  // the agent's three per-slot mesh arrays
    char pad0[0x10];
    Mesh* c1[4];                                   // +0x10
    Mesh* c2[4];                                   // +0x20
    Mesh* c3[4];                                   // +0x30
};
struct SkelElem {                                  // 0x1dc bytes: runtime state of one bone
    char pad0[0x1dc];
    void add_group(Mesh* m);                       // 0x4423f0 (thiscall, ret 4)
};
struct Skeleton {
    int count;                                     // +0
    SkelElem* elems;                               // +4 (0x1dc-byte records)
    void remove_shape(Mesh* m);                    // 0x439ec0 (thiscall, ret 4)
};
struct AgentBody {
    char pad0[0x304];
    Skeleton* skel;                                // +0x304
};

// @ 0x495430
void __cdecl place_scene_agents(AgentBody* holder, ItemRefMin* table, PlaceCtx* ctx)
{
    char placed[11] = {0};
    for (int i = 3; i >= 0; i--) {
        int id = table[i].item;
        if (id < 0)
            continue;
        ItemKind* it = &g_items[id];
        if (((it->f1d0 & 0xf0000000) | (it->f1d4 & 7)) == 0)
            continue;
        rgl::matrix M;
        int attach;
        int bone;
        it->get_carry_pos(&M, &attach, &bone);
        if (placed[bone] != 0 || (it->type == 7 && (table[i].m4 & 0xffff) <= 0xa)) {
            if (ctx->c3[i] != 0)
                ctx->c3[i]->vis = 0;
            continue;
        }
        placed[bone] = 1;
        if (ctx->c3[i] != 0) {
            ctx->c3[i]->vis = 0xffff;
            continue;
        }
        if (ctx->c1[i] != 0) {
            ctx->c1[i]->vis = 0xffff;
            Skeleton* sk = holder->skel;
            sk->remove_shape(ctx->c1[i]);
            ctx->c1[i]->frame = M;
            sk->elems[attach].add_group(ctx->c1[i]);
        }
        if (ctx->c2[i] != 0) {
            ctx->c2[i]->vis = 0xffff;
            Skeleton* sk = holder->skel;
            sk->remove_shape(ctx->c2[i]);
            ctx->c2[i]->frame = M;
            sk->elems[attach].add_group(ctx->c2[i]);
        }
    }
}

// ---- 0x4955e0: register an emitter in a table and record its index ----------------------------------
struct Emitter;
struct EmitTarget {                                // virtual object at this+0x59c0
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24();
    virtual void add(Emitter* e);                  // +0x64
};
struct EmitTable {                                 // global at 0x8ed5090
    int f0;
    int count;                                     // +4
    Emitter* setup(int* idx, int* out, int* out2); // 0x479d20 (thiscall, ret 0xc)
};
extern EmitTable g_emit_defs;                      // 0x8ed5090

struct EmitterCtl {
    char pad0[0x59c0];
    EmitTarget* target;                            // +0x59c0
    char pad59c4[0x59f4 - 0x59c4];
    std::vector<int> list;                         // +0x59f4
    void add(int e);                               // 0x4955e0 (ret 4)
};
// @ 0x4955e0
void EmitterCtl::add(int e)
{
    if (target == 0) return;
    int tmp[4];
    tmp[0] = 0;
    tmp[1] = 0;
    tmp[2] = 0;
    tmp[3] = 0;
    Emitter* em = g_emit_defs.setup(&e, &tmp[1], &tmp[0]);
    if (em != 0) {
        target->add(em);
        list.push_back(e);
    }
}

// ---- 0x495650: append a fresh scene object to the list at this+0x10 and return it -------------------
struct Frustum { char d[0x134]; Frustum(); };  // 0x41ef60
struct SceneBase { char d[0x36c]; SceneBase(); };  // 0x493ab0
struct SceneObject : SceneBase { Frustum frustum; };   // 0x4a0 total

struct SceneList {
    char pad0[0x10];
    std::vector<SceneObject*> v;                   // +0x10
    SceneObject* push();                           // 0x495650
};
// @ 0x495650
SceneObject* SceneList::push()
{
    SceneObject* o = new SceneObject;
    v.push_back(o);
    return v[v.size() - 1];
}

// ---- 0x495700: remove a scene object slot, tearing down the active deque ----------------------------
struct SceneItem {                                 // object referenced by the array at scene+0
    virtual void s0();
    virtual void s1();                             // +0x04
    virtual char s2();                             // +0x08
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual char s7();                             // +0x1c
    virtual char s8();                             // +0x20
};
struct GlobVec {                                   // global holder at 0x5e299c
    int f0;
    void* first;                                   // +4
    int f8;                                        // +8
    int fc;                                        // +0xc
};
extern GlobVec* g_5e299c;                          // 0x5e299c

struct SceneMap {
    SceneItem* arr[30];                            // +0x00
    std::deque<int> dq;                            // +0x78
    void remove(int idx);                          // 0x495700 (ret 4)
    void activate(int idx);                        // 0x494260 (ret 4)
};
// @ 0x495700
void SceneMap::remove(int idx)
{
    GlobVec* g = g_5e299c;
    if (g != 0) {
        g_5e299c = 0;
        if (g->first != 0)
            operator delete(g->first);
        g->first = 0;
        g->f8 = 0;
        g->fc = 0;
        g_5e299c = 0;
    }
    g_gui.set_o568(0, true);
    if (arr[idx]->s7()) {
        while (dq.size() != 0) {
            arr[dq.back()]->s1();
            dq.pop_back();
        }
        g_gui.reset();
    }
    activate(idx);
}
