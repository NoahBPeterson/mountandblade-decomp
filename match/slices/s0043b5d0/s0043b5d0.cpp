// Slice s0043b5d0: agent AI target selection around 0x43b5d0..0x43bda3 (all names provisional). Each function walks
// the mission's agent table (or the agent's own sorted list of the 16 nearest enemies) and scores candidates by
// distance, facing and weapon class: a randomised "who to attack" pick, the agent's velocity projected through
// its body matrix, the nearest facing agent that can be talked to / hit, and two facing-weighted pickers.
// Neighbour: s0043ac40 (same Agent / AgentTable layouts; copied here).
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;

// ---- shared with s0043ac40 / s0043bdb0 -------------------------------------------------------------------------
struct ItemRef {
    int item;
    unsigned m4;
    int max_amount();                   // 0x434b70
    int stat_1ec();                     // 0x434c50 (fastcall == thiscall)
};
struct ItemKind { char pad0[0x1c8]; unsigned type_flags; char pad1cc[0x238 - 0x1cc]; };   // 0x238, array at 0x5e29d0
extern ItemKind* g_items;               // 0x5e29d0
struct GameData {
    char pad[0x13a00];
    int skill(int troop, int id);       // 0x437b20 (thiscall, ret 8)
};
extern GameData* g_game;                // 0x5e2930

struct Threat { int id; float dist; vector4 dir; };   // 0x18 bytes
struct Vec2 { float x, y; };
struct Agent;
struct AgentTable {
    int count;                                         // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                         // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                       // 0x43b160
    int first_used();                                  // 0x43b1b0
};
struct Body {                                          // object at agent+0x250 (matrix rows of 0x10 bytes from +0x58)
    char pad0[0x58];
    rgl::matrix m;                                     // +0x58: s, f, u, o
};
struct Agent {
    char pad0[4];
    int used;                                          // +4
    int id;                                            // +8
    char pad0c[4];
    int f10;                                           // +0x10
    int f14;                                           // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                           // +0x2c (mount / rider index, -1 none)
    int f30;                                           // +0x30
    char pad34[0x50 - 0x34];
    vector4 pos;                                       // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                         // +0x88
    ItemRef f8c;                                       // +0x8c
    char pad94[0xa8 - 0x94];
    Vec2 vel;                                          // +0xa8
    char padb0[0x1c4 - 0xb0];
    int team;                                          // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef slots[8];                                  // +0x1d0
    char pad210[0x22c - 0x210];
    int cur_slot;                                      // +0x22c
    char pad230[0x250 - 0x230];
    Body* body;                                        // +0x250
    char pad254[0x53ec - 0x254];
    Threat threats[16];                                // +0x53ec (sorted by distance, nearest first)
    int nthreat;                                       // +0x556c
    char pad5570[0x55d8 - 0x5570];
    float fx, fy, fz;                                  // +0x55d8 (facing direction)
    char pad55e4[4];
    int f55e8;                                         // +0x55e8
    char pad55ec[0x560c - 0x55ec];
    int active;                                        // +0x560c
    char pad5610[0x5830 - 0x5610];
    int f5830;                                         // +0x5830
    char pad5834[0x5928 - 0x5834];
    unsigned pick_random();                            // 0x43b5d0 (fastcall)
    vector4* local_velocity(vector4* out);             // 0x43b7b0
    unsigned find_talk_target();                       // 0x43b8a0 (fastcall)
    unsigned pick_front();                             // 0x43b9e0 (fastcall)
    unsigned pick_duel();                              // 0x43bb80 (fastcall)
};
struct Mission {
    int f0;
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;                        // +0x18 (page table; the AgentTable at +4 has the same vector at +0x14)
    char pad28[0x53c8 - 0x28];
    int relation[8 * 0x5b];                            // +0x53c8 (team x team, stride 0x5b)
    AgentTable* table() { return (AgentTable*)((char*)this + 4); }
    bool allied(int a, int b)
    {
        if (a < 0 || a >= 8 || b < 0 || b >= 8) return false;
        return relation[a * 0x5b + b] == 1;
    }
    bool hostile(int a, int b)
    {
        if (a < 0 || a >= 8 || b < 0 || b >= 8) return false;
        return relation[a * 0x5b + b] == -1;
    }
};
extern Mission* g_mission;                             // 0x5e292c

static __forceinline bool hostile_to(int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return g_mission->relation[a * 0x5b + b] == -1;
}
static __forceinline bool allied_to(int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return g_mission->relation[a * 0x5b + b] == 1;
}

static __forceinline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// @ 0x43b5d0
unsigned Agent::pick_random()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned id = (unsigned)-1;
    int n = 0;
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            id = r->id;
            Agent* o = &g_mission->agents[id >> 4][id & 0xf];
            if (o->f14 == 1) {
                float dx = o->pos.x - pos.x;
                float dy = o->pos.y - pos.y;
                float dz = o->pos.z - pos.z;
                float v = 3.0f / (float)sqrt(dx * dx + (dy * dy + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                Agent* o2 = &g_mission->agents[id >> 4][id & 0xf];
                ItemRef* cur = 0;
                if (o2->cur_slot >= 0) cur = &o2->slots[o2->cur_slot];
                int item = -1;
                if (cur) item = cur->item;
                if (item >= 0) {
                    unsigned t = g_items[item].type_flags & 0xff;
                    if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11) v = v * 1.5f;
                }
                if (g_mission->agents[id >> 4][id & 0xf].f2c != -1) v = v * 0.85f;
                int w = g_mission->agents[id >> 4][id & 0xf].f55e8;
                v = v / ((float)w + 4.0f);
                v = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * v;
                if (v > bestv) { bestv = v; best = r->id; }
            }
            n++;
            r++;
            id = best;
        } while (n < nthreat);
    }
    return id;
}

// @ 0x43b7b0
vector4* Agent::local_velocity(vector4* out)
{
    unsigned idx = f2c;
    vector4 v;
    if ((int)idx >= 0) {
        Agent* a = &g_mission->agents[idx >> 4][idx & 0xf];
        v.x = a->vel.x; v.y = a->vel.y;
    } else {
        v.x = vel.x; v.y = vel.y;
    }
    v.z = 0.0f;
    if (body) {
        const rgl::matrix& m = body->m;
        float rz = (v.y * m.f.z + v.x * m.s.z) + m.u.z * 0.0f;
        float ry = (v.y * m.f.y + v.x * m.s.y) + m.u.y * 0.0f;
        float rx = (v.y * m.f.x + v.x * m.s.x) + m.u.x * 0.0f;
        out->x = rx; out->y = ry; out->z = rz;
        return out;
    }
    *out = v;
    return out;
}

// @ 0x43b8a0
unsigned Agent::find_talk_target()
{
    Mission* m = g_mission;
    AgentTable* t = m->table();
    unsigned best = (unsigned)-1;
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents[i >> 4][i & 0xf];
            if (o->f10 == 1 && o->f30 == -1 && o->f14 == 1 && o->f5830 == 2) {
                int a = g_game->skill(troop, 0x18);
                int b = o->f8c.stat_1ec();
                if (b <= a) {
                    vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                    float len = d.normalize();
                    if (len < 1.0f && d.x * fx + d.y * fy > 0.1f)
                        best = i;
                }
            }
            m = g_mission;
            t = m->table();
            t->next_used(&i);
        } while (i < (unsigned)t->count);
    }
    return best;
}

// @ 0x43b9e0
unsigned Agent::pick_front()
{
    Mission* m = g_mission;
    AgentTable* t = m->table();
    unsigned best = (unsigned)-1;
    float bestv = 0.07f;
    unsigned i = t->first_used();
    unsigned n = t->count;
    if (i < n) {
        int myteam = team;
        do {
            Agent* o = &m->agents[i >> 4][i & 0xf];
            if (!allied_to(myteam, o) && o->f14 == 1 && o->f10 != 1) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + d.y * fy) + 0.5f, 0.0f, 1.2f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                if (b * a > bestv) { best = i; bestv = b * a; }
            }
            t->next_used(&i);
        } while (i < n);
    }
    return best;
}

// @ 0x43bb80
unsigned Agent::pick_duel()
{
    Mission* m = g_mission;
    AgentTable* t = m->table();
    unsigned best = (unsigned)-1;
    float bestv = 0.0f;
    unsigned i = t->first_used();
    unsigned n = t->count;
    if (i < n) {
        Agent** pg = &m->agents[0];
        int myteam = team;
        do {
            Agent* o = &pg[i >> 4][i & 0xf];
            if (hostile_to(myteam, o) && o->f14 == 1 && o->active != 0) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + (d.y * fy + d.z * fz)) + 0.8f, 0.0f, 1.0f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                float od = -(d.x * o->fx + (d.y * o->fy + d.z * o->fz));
                float c = clampf(od + 0.5f, 0.0f, 1.0f);
                float v = (c * b) * a;
                if (v > bestv) { best = i; bestv = v; }
            }
            g_mission->table()->next_used(&i);
        } while (i < n);
    }
    return best;
}
