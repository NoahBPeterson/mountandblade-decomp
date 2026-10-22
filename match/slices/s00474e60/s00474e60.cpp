// Slice s00474e60: Mission::update_team_stats (provisional name), the once-per-1.1s battle bookkeeping that
// builds, per team and per troop class (infantry / archers / cavalry), centroid, headcount and approach direction.
#include <vector>
#include <math.h>
#include "rgl.h"
using rgl::vector4;

extern __int64 g_ticks[];      // 0x5e2948: 64-bit counters, 100 kHz units
extern float g_eps_small;      // 0x591844: 1e-11
extern float g_max_dist;       // 0x590b58: 400.0

struct Agent {                                          // 0x5928 bytes (fields used here)
    char pad0[4];
    int used;                                           // +4 (0 = free slot)
    char pad8[0x10 - 8];
    int f10;                                            // +0x10
    int f14;                                            // +0x14 (1 = alive?)
    char pad18[0x50 - 0x18];
    vector4 pos;                                        // +0x50
    char pad60[0x1c4 - 0x60];
    int team;                                           // +0x1c4
    char pad1c8[4];
    int head_count;                                     // +0x1cc
    char pad1d0[0x31c - 0x1d0];
    int f31c;                                           // +0x31c (id of another agent)
    char pad320[0x5340 - 0x320];
    float f5340;                                        // +0x5340
    float f5344;                                        // +0x5344
    char pad5348[0x55e8 - 0x5348];
    int f55e8;                                          // +0x55e8
    char pad55ec[0x56b4 - 0x55ec];
    vector4 v56b4;                                      // +0x56b4
    char pad56c4[0x5928 - 0x56c4];

    int troop_class();                                  // 0x433140 (Actor::hand_kind elsewhere: 0 infantry, 1 archer, 2 cavalry)
    int update_nav_face();                              // 0x43cb30
};

struct ClassInfo {                                      // 0x6c bytes, per team per troop class
    int kind;                                           // +0x00 (2 = tracked)
    char pad4[0xc];
    vector4 pos;                                        // +0x10
    vector4 target;                                     // +0x20
    int nav;                                            // +0x30
    vector4 dir;                                        // +0x34
    char pad44[0x4c - 0x44];
    float half;                                         // +0x4c
    float count;                                        // +0x50
    float total;                                        // +0x54
    char pad58[4];
    vector4 sum;                                        // +0x5c
};
struct Team {                                           // 0x16c bytes
    unsigned leader;                                    // +0x00 agent id, -1 = none
    ClassInfo cls[3];                                   // +0x04
    int relation[8];                                    // +0x148
    int pad168;
};

struct AgentTable {                                     // mission+4
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    __forceinline unsigned first_used()                 // 0x43b1b0 (inlined here)
    {
        unsigned i = 0;
        if (0 < count) {
            do {
                if (pages[i >> 4][i & 0xf].used != 0) break;
                i++;
            } while ((int)i < count);
        }
        return i;
    }
};

struct Timer {
    int clock;                                          // +0 index into g_ticks
    int pad4;
    __int64 start;                                      // +8
    char active;                                        // +0x10
    float elapsed() const { return (float)(g_ticks[clock] - start) * 1e-5f; }
};

struct Mission {
    int scene;                                          // +0
    AgentTable agents;                                  // +4
    char pad28[0x5280 - 0x28];
    Team teams[8];                                      // +0x5280
    Timer tm;                                           // +0x5de0

    void update_team_stats();                           // 0x474e60 (fastcall)
};

#define FIRST_USED(m, i) unsigned i = (m)->agents.first_used();
#define AGENT(m, id) ((m)->agents.pages[(id) >> 4][(id) & 0xf])

// Inline vector4 arithmetic as the game's header implements it: 3-component results,
// leaving the fourth lane uninitialized (whole-struct copies carry it along).
static __forceinline vector4 vscale(const vector4& a, const float& s)
{
    return vector4(a.x * s, a.y * s, a.z * s);
}
static __forceinline vector4 vsum(const vector4& a, const vector4& b)
{
    return vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}
static __forceinline float normalize_inline(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv;
        v.y = v.y * inv;
        v.z = v.z * inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}

// @ 0x474e60
void Mission::update_team_stats()
{
    int t, k;
    for (t = 0; t < 8; t++) {
        Team& tm_ = teams[t];
        if ((int)tm_.leader >= 0) {
            for (k = 0; k < 3; k++) {
                ClassInfo& c = tm_.cls[k];
                if (c.kind == 2) {
                    Agent& a = AGENT(this, tm_.leader);
                    c.pos = a.pos;
                    c.nav = AGENT(this, tm_.leader).update_nav_face();
                    c.target = AGENT(this, tm_.leader).v56b4;
                }
            }
        }
    }
    if (tm.elapsed() >= 0.0f) {
        tm.start = g_ticks[tm.clock] + 110000;
        vector4 zero(0.0f, 0.0f, 0.0f);
        for (t = 0; t < 8; t++)
            for (k = 0; k < 3; k++) {
                teams[t].cls[k].count = 0.0f;
                teams[t].cls[k].sum = zero;
            }

        FIRST_USED(this, i)
        while (i < (unsigned)agents.count) {
            Agent& a = AGENT(this, i);
            if (a.f14 == 1 && a.f10 == 0) {
                int tid = a.team;
                if (tid < 0 || tid >= 8) tid = 7;
                int cl = a.troop_class();
                a.head_count = (int)(teams[tid].cls[cl].count > 0.0f ? teams[tid].cls[cl].count : teams[tid].cls[cl].count - 0.99999988f);
                if (tid >= 0 && tid < 8 && cl >= 0 && cl < 3) {
                    teams[tid].cls[cl].count = teams[tid].cls[cl].count + 1.0f;
                    teams[tid].cls[cl].sum.x = a.pos.x + teams[tid].cls[cl].sum.x;
                    teams[tid].cls[cl].sum.y = a.pos.y + teams[tid].cls[cl].sum.y;
                    teams[tid].cls[cl].sum.z = a.pos.z + teams[tid].cls[cl].sum.z;
                }
            }
            agents.next_used(&i);
        }

        for (t = 0; t < 8; t++) {
            Team& tm_ = teams[t];
            for (k = 0; k < 3; k++) {
                float n = tm_.cls[k].count;
                for (int j = 0; j < 3; j++) {
                    if (j != k) {
                        float dx = tm_.cls[k].pos.x - tm_.cls[j].pos.x;
                        float dy = tm_.cls[k].pos.y - tm_.cls[j].pos.y;
                        float dz = tm_.cls[k].pos.z - tm_.cls[j].pos.z;
                        if (sqrt(dx * dx + (dy * dy + dz * dz)) < 1.0f)
                            n = n + tm_.cls[j].count;
                    }
                }
                tm_.cls[k].half = n * 0.5f;
                tm_.cls[k].total = n;
            }
            if ((int)tm_.leader >= 0 && AGENT(this, tm_.leader).f14 != 1) {
                bool found = false;
                FIRST_USED(this, i2)
                while (i2 < (unsigned)agents.count) {
                    Agent& a = AGENT(this, i2);
                    if (!found && a.f14 == 1 && a.f10 != 1) {
                        int tid = a.team;
                        if (tid < 0 || tid >= 8) tid = 7;
                        if (tid == t) {
                            found = true;
                            tm_.leader = i2;
                        }
                    }
                    agents.next_used(&i2);
                }
            }
        }

        for (t = 0; t < 8; t++) {
            for (k = 0; k < 3; k++) {
                float inv = 1.0f / teams[t].cls[k].count;
                teams[t].cls[k].sum.x = inv * teams[t].cls[k].sum.x;
                teams[t].cls[k].sum.y = inv * teams[t].cls[k].sum.y;
                teams[t].cls[k].sum.z = inv * teams[t].cls[k].sum.z;
            }
        }

        {
            FIRST_USED(this, i3)
            while (i3 < (unsigned)agents.count) {
                Agent& a = AGENT(this, i3);
                if (a.f14 == 1 && a.f10 != 1) {
                    int tid = a.team;
                    if (tid < 0 || tid >= 8) tid = 7;
                    int cl = a.troop_class();
                    if (tid >= 0) {
                        a.f5340 = (float)a.head_count - teams[tid].cls[cl].count * 0.5f;
                        if (tid == 2) a.f5344 = 1.2f;
                        else if (tid == 1) a.f5344 = 0.0f;
                        else if (tid == 0) a.f5344 = -0.7f;
                    }
                }
                agents.next_used(&i3);
            }
        }

        for (t = 0; t < 8; t++) {
            Team& tm_ = teams[t];
            if ((int)tm_.leader >= 0) {
                for (k = 0; k < 3; k++) {
                    ClassInfo& c = tm_.cls[k];
                    vector4 v;
                    v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
                    FIRST_USED(this, i4)
                    while (i4 < (unsigned)agents.count) {
                        Agent& a = AGENT(this, i4);
                        if (a.f14 == 1 && a.f10 != 1) {
                            int tid = a.team;
                            if (tid < 0 || tid >= 8) tid = 7;
                            a.troop_class();
                            if (teams[t].relation[tid] == -1) {
                                vector4 d;
                                d.x = a.pos.x - c.pos.x;
                                d.y = a.pos.y - c.pos.y;
                                d.z = a.pos.z - c.pos.z;
                                float len = normalize_inline(d);
                                if (len < 2.0f) len = 2.0f;
                                else if (len > g_max_dist) len = g_max_dist;
                                float s = 1.0f / len;
                                volatile float tx = d.x * s;   // original spills this term to a
                                v.x = v.x + tx;                // float slot, rounding it before the add
                                v.y = v.y + d.y * s;
                                v.z = v.z + d.z * s;
                            }
                        }
                        agents.next_used(&i4);
                    }
                    v.z = 0.0f;
                    if (v.length() > g_eps_small) {
                        normalize_inline(v);
                        if (c.dir.length_squared() < 0.5f) {
                            c.dir = v;
                        } else {
                            c.dir = vsum(vscale(c.dir, 0.8f), vscale(v, 0.2f));
                            normalize_inline(c.dir);
                        }
                    }
                }
            }
        }

        {
            FIRST_USED(this, i5)
            while (i5 < (unsigned)agents.count) {
                AGENT(this, i5).f55e8 = 0;
                agents.next_used(&i5);
            }
        }
        {
            FIRST_USED(this, i6)
            while (i6 < (unsigned)agents.count) {
                int o = AGENT(this, i6).f31c;
                if (o >= 0 && o < agents.count && AGENT(this, o).used != 0)
                    AGENT(this, o).f55e8++;
                agents.next_used(&i6);
            }
        }
    }
}
