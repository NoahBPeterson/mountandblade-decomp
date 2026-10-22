// Slice s005189e0: two per-frame campaign-map update steps of the game data (g_game, 0x5e2930).
// 0x5189e0 rebuilds the party-track mesh (the "track"/"battle_track" ribbons drawn behind every visible
// map event) and tints the freshly appended vertices by the event's age and the player's tracking skill;
// 0x518d70 is the town training-camp update (parties are flagged dirty, the player's stacks collect the
// trainer-skill XP share, heroes gain it directly and the others through the per-stack XP share, with the
// "training complete"/"soldiers ready to upgrade" log messages).
// Layouts come from s004947a0 (GameData/Party/Troop/Stack), s004902a0 (party_at/pages) and s0044eb00
// (rgl::mesh/vertex60/mat4/pack_color). Names are provisional.
#include <windows.h>
#include <stdio.h>
#include <vector>
#include "rgl.h"

// ---- tstring / string table / message log (see s0042aaf0, s004947a0) ----------------------------------------------
struct tstring {
    int pos;                                    // +0x00
    char* buf;                                  // +0x04
    int cap;                                    // +0x08
    int len;                                    // +0x0c
    tstring(const char* s) throw();             // 0x42bbe0
    tstring(const tstring& o) throw();          // 0x42c110
    ~tstring() throw();                         // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable {
    tstring* lookup(const tstring& key) throw();     // 0x43a1e0
};
extern StrTable g_strtable;                     // 0x5e2a18
struct MsgLog {
    void add(tstring msg, int color);           // 0x452c00 (thiscall, ret 0x14)
};
extern MsgLog g_msglog;                         // 0x5d7080

// ---- rgl mesh bits (see s0044eb00) ---------------------------------------------------------------------------------
namespace rgl {
struct vec2 { float x, y; };
struct mat4 { vector4 s, f, u, o; };            // 64-byte transform
struct vertex60 {                               // 0x60 bytes, vector at mesh+0x50
    vector4 pos;                                // +0x00
    vector4 f10;                                // +0x10
    vector4 f20;                                // +0x20
    vector4 normal;                             // +0x30
    vec2 uv;                                    // +0x40
    int f48, f4c;                               // +0x48
    int index;                                  // +0x50
    unsigned color;                             // +0x54 (packed ARGB)
    int f58, f5c;                               // +0x58
};
struct mesh {
    char pad0[0x50];
    std::vector<vertex60> v50;                  // +0x50
    void clear_geometry();                                                              // 0x428e80
    void append_skinned(mesh* other, const mat4* m, int bone, unsigned col, char flip);  // 0x44eb00
};
unsigned pack_color(const vector4* c);          // 0x40e440 (cdecl)
}
struct resmgr {
    rgl::mesh* get_object(const tstring& name);      // 0x419540 (ret 4)
};
extern resmgr g_resmgr;                         // 0x5d6fa0

// ---- game data (object at 0x5e2930) --------------------------------------------------------------------------------
struct Troop {                                  // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned flags;                             // +0x30 (0x10 = hero)
    char pad34[0x50 - 0x34];
    int f50;                                    // +0x50
    char pad54[0xa0 - 0x54];
    float f0a0;                                 // +0xa0 (xp progress towards the next level)
    char pada4[0xb0 - 0xa4];
    int f0b0;                                   // +0xb0 (level)
    char padb4[0x508 - 0xb4];
};
struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00
    int troop;                                  // +0x04
    int flags;                                  // +0x08 (bit 0 = end-of-list marker)
    int f0c;                                    // +0x0c
    int f10;                                    // +0x10
    int ready;                                  // +0x14 (soldiers ready to upgrade)
    float xp;                                   // +0x18
    int f1c;                                    // +0x1c
};
struct Party {                                  // 0x5ac8 bytes
    int id;                                     // +0x00
    int state;                                  // +0x04 (1 = party slot in use)
    char pad8[0xa8 - 8];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[4];
    unsigned char f8b0;                         // +0x8b0
    char pad8b1[3];
    unsigned char f8b4;                         // +0x8b4 (objective list dirty)
    char pad8b5[0x5ac8 - 0x8b5];
    int skill(int id);                          // 0x4b2370 (thiscall, ret 4)
    void add_xp(int slot, int amount);          // 0x4b1d80 (thiscall, ret 8)
};
struct PartyMgr {                               // embedded in the game data at +0xf8
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0 (id >> 4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc (party id bound / "not found" sentinel)
    int index_of(unsigned id);                  // 0x48ad00 (thiscall, ret 4)
    unsigned first_party_id();                  // 0x460d20 (thiscall, ret 0)
    void next_party_id(unsigned* id);           // 0x460ca0 (thiscall, ret 4)
};
static __forceinline Party* party_at(PartyMgr* m, unsigned id)
{
    int i = m->index_of(id);
    return m->pages[i >> 4] + (i & 0xf);
}
struct event_rec {                              // 0x5c bytes, vector at game+0x13a60
    rgl::vector4 pos;                           // +0x00
    float angle;                                // +0x10
    float f14;                                  // +0x14 (age in frames)
    unsigned flags;                             // +0x18 (0x20000000 visible, 0x10000000 battle track)
    rgl::mat4 m;                                // +0x1c
};
struct Cfg30 {                                  // object at game+0x30
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06(const float* d);           // +0x18
};
struct GameData {
    char pad0[0x2c];
    rgl::mesh* f2c;                             // +0x2c (party-track mesh)
    Cfg30* f30;                                 // +0x30
    char pad34[0x78 - 0x34];
    int f78;                                    // +0x78 (default/player party id)
    char pad7c[0x8c - 0x7c];
    int f8c;                                    // +0x8c (player troop id)
    char pad90[0x95 - 0x90];
    char f95;                                   // +0x95
    char pad96[0xf8 - 0x96];
    PartyMgr parties;                           // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                              // +0x13a00
    char pad13a04[0x13a60 - 0x13a04];
    std::vector<event_rec> events;              // +0x13a60
    int skill(int troop, int id);               // 0x437b20 (thiscall, ret 8)
    int add_experience(int troop, int amount);  // 0x4428c0 (thiscall, ret 8)
    void update_tracks();                       // 0x5189e0
    void train_troops();                        // 0x518d70
};
extern float g_train_limit;                     // 0xa18138
extern float g_train_limit_hero;                // 0xa1813c

// @ 0x5189e0
void GameData::update_tracks()
{
    f2c->clear_geometry();

    rgl::mesh* track;
    {
        tstring n("track");
        track = g_resmgr.get_object(n);
    }
    rgl::mesh* battle;
    {
        tstring n("battle_track");
        battle = g_resmgr.get_object(n);
    }

    float denom = (float)party_at(&parties, f78)->skill(0x10) + 8.0f;

    for (event_rec* p = events._Myfirst; p < events._Mylast; ++p) {
        if (!f95 && !(p->flags & 0x20000000))
            continue;

        int first = (int)f2c->v50.size();
        rgl::mesh* m = track;
        if (p->flags & 0x10000000)
            m = battle;
        f2c->append_skinned(m, &p->m, -1, -1, 0);

        float t = p->f14 / denom;
        t = (t < 1.0f) ? t : 1.0f;
        t *= 3.0f;

        rgl::vector4 c;
        if (t < 1.0f) {
            float u = 1.0f - t;
            c = rgl::vector4(0.8f * t + 1.2f * u,
                             0.6f * t + 0.9f * u,
                             0.9f * t + 0.9f * u);
        } else {
            float u = t - 1.0f;
            if (u > 1.0f)
                u = 1.0f;
            float v = 1.0f - u;
            c = rgl::vector4(0.9f * u + 0.8f * v,
                             0.6f * u + 0.6f * v,
                             0.6f * u + 0.9f * v);
        }

        unsigned col = rgl::pack_color(&c);
        if (party_at(&parties, f78)->skill(0x10) < 2)
            col = 0xffa8a8a8;
        col |= 0xff000000;

        for (int i = first; i < f2c->v50.size(); i++)
            f2c->v50[i].color = col;
    }

    float d = 1000000.0f;
    f30->v06(&d);
}

// @ 0x518d70
void GameData::train_troops()
{
    for (unsigned id = parties.first_party_id(); id < parties.count; parties.next_party_id(&id))
        party_at(&parties, id)->f8b4 = 1;

    Party* p = party_at(&parties, f78);

    int gain[64];
    for (int i = 0; i < p->nstacks; i++)
        gain[i] = 0;

    int limit[11] = {0, 4, 10, 0x10, 0x17, 0x1e, 0x26, 0x2e, 0x37, 0x41, 0x50};
    int total = 0;

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            continue;
        if (!(troops[s.troop].flags & 0x10))
            continue;
        if (troops[s.troop].f0a0 < (s.troop == f8c ? g_train_limit_hero : g_train_limit))
            continue;
        int lvl = skill(s.troop, 0x11);
        if (lvl > 10)
            lvl = 10;
        int cost = limit[lvl];
        if (!cost)
            continue;

        for (int j = 0; j < p->nstacks; j++) {
            Stack& q = p->stacks[j];
            if (j == i)
                continue;
            if (q.flags & 1)
                continue;
            if (troops[q.troop].f0b0 >= troops[s.troop].f0b0)
                continue;
            if (troops[q.troop].flags & 0x10) {
                if (troops[q.troop].f0a0 < (q.troop == f8c ? g_train_limit_hero : g_train_limit))
                    continue;
                gain[j] += cost;
                total += cost;
            } else {
                if (!troops[q.troop].f50)
                    continue;
                int ready = q.count - q.f10;
                gain[j] += cost * (ready > 0 ? ready : 0);
                total += cost * (ready > 0 ? ready : 0);
            }
        }
    }

    if (total != 0) {
        g_msglog.add(*g_strtable.lookup(tstring("ui_days_training_is_complete")), -1);
        char buf[512];
        sprintf(buf, g_strtable.lookup(tstring("ui_total_experience_gained_through_training_number"))->c_str(), total);
        g_msglog.add(tstring(buf), -1);
    }

    bool changed = false;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& q = p->stacks[i];
        if (troops[q.troop].flags & 0x10) {
            add_experience(q.troop, gain[i]);
        } else {
            int old = q.ready;
            p->add_xp(i, gain[i]);
            if (q.ready > old)
                changed = true;
        }
    }
    if (changed)
        g_msglog.add(*g_strtable.lookup(tstring("ui_some_soldiers_are_ready_to_upgrade")), -1);
}
