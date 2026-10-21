// Slice s0044d000: Agent::choose_action (0x44d000, 3877 bytes). The agent's "what do I do next" decision: fills 15
// candidate scores (one per attack/movement kind 0..14), each with a target id and a position, scales them by
// distance / weapon class / ai class record, randomises them (x0.9..1.1), picks the highest and starts it
// (start_attack, set_goal_*). Names provisional; layouts reuse s004465e0, s0043b5d0, s00435490, s00433cb0.
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;

// troop flags (Module System header_troops)
#define tf_hero             0x00000010i64
#define tf_guarantee_horse  0x01000000i64
#define tf_guarantee_ranged 0x04000000i64

struct ItemKind {
    char pad0[0x238];
    bool is_ranged();                           // 0x433d90
};
extern ItemKind* g_items;                       // 0x5e29d0

struct Troop {
    char pad0[0x30];
    __int64 flags;                              // +0x30
    char pad38[0x508 - 0x38];
    int nibble(int i);                          // 0x4351a0
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                              // +0x13a00 (stride 0x508)
};
extern World* g_world;                          // 0x5e2930

// one ai-class record of a team (0x6c bytes): +0 kind (1/2/3), +4 sub kind, +0x10 anchor point, +0x4c radius
struct ClassRec {
    int a, b;
    char pad8[0x10 - 8];
    float px, py, pz;
    char pad1c[0x4c - 0x1c];
    float radius;
    char pad50[0x6c - 0x50];
};
struct TeamRec {                                // 0x16c bytes, array at mission+0x5280
    int leader;
    ClassRec cls[3];
    int rel[8];                                 // +0x148 (-1 enemy, 1 friend)
    char pad168[4];
};
struct TeamSlot { int a, b, c, d, e, f; };      // 0x18, array at mission+0x622c

struct ModeRec { char pad[0x20]; unsigned flags; char pad2[0x58 - 0x24]; };   // 0x58, array at 0x5e2a00
extern ModeRec* g_modes;                        // 0x5e2a00

struct ItemRef {
    int item;
    unsigned m4;
    int stat_1ec();                             // 0x434c50
};

struct Pair { int a; unsigned b; };

struct Threat { unsigned id; float dist; vector4 dir; };   // 0x18 bytes

struct Agent;
struct AgentTable {
    int count;                                  // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                  // +0x14
    void next_used(unsigned* i);                // 0x43b160
    int first_used();                           // 0x43b1b0
};
struct Mission {
    char pad0[4];
    AgentTable agents;                          // +4
    char pad28[0x5280 - 0x28];
    TeamRec teams[8];                           // +0x5280
    char pad5de0[0x5df0 - 0x5de0];
    int f5df0;                                  // +0x5df0 (index into g_modes)
    char pad5df4[0x622c - 0x5df4];
    TeamSlot slots[8];                          // +0x622c
};
extern Mission* g_mission;                      // 0x5e292c

struct Skel { char pad[0x68]; vector4 f; };     // agent body: forward row at +0x68

struct Agent {
    char pad0[8];
    int id;                                     // +8
    char padc[4];
    int f10;                                    // +0x10
    int f14;                                    // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                    // +0x2c (-1 = on foot)
    int f30;                                    // +0x30
    char pad34[0x50 - 0x34];
    vector4 pos;                                // +0x50
    vector4 dir;                                // +0x60
    char pad70[0x88 - 0x70];
    int troop;                                  // +0x88
    ItemRef f8c;                                // +0x8c
    char pad94[0x130 - 0x94];
    unsigned char flags130;                     // +0x130
    char pad131[0x1c4 - 0x131];
    int team;                                   // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    Pair held[8];                               // +0x1d0
    char pad210[0x22c - 0x210];
    int cur_slot;                               // +0x22c
    char pad230[0x250 - 0x230];
    Skel* body;                                 // +0x250
    char pad254[0x288 - 0x254];
    float f288; int f28c;                           // +0x288
    char pad290[0x2c4 - 0x290];
    unsigned char f2c4;                         // +0x2c4
    char pad2c5[3];
    int mode;                                   // +0x2c8
    int f2cc;                                   // +0x2cc
    vector4 f2d0;                               // +0x2d0
    int f2e0, f2e4, f2e8, f2ec;                 // +0x2e0
    char pad2f0[0x5338 - 0x2f0];
    unsigned f5338;                             // +0x5338
    char pad533c[0x53a0 - 0x533c];
    int f53a0;                                  // +0x53a0
    char pad53a4[0x53ec - 0x53a4];
    Threat threats[16];                         // +0x53ec
    int nthreat;                                // +0x556c
    float f5570;                                // +0x5570
    char pad5574[0x55a4 - 0x5574];
    vector4 f55a4;                              // +0x55a4
    int f55b4;                                  // +0x55b4
    char pad55b8[0x55c0 - 0x55b8];
    int f55c0;                                  // +0x55c0
    char pad55c4[0x55e8 - 0x55c4];
    int f55e8;                                  // +0x55e8
    char pad55ec[0x56d4 - 0x55ec];
    int weapon_mode;                            // +0x56d4
    char pad56d8[0x5830 - 0x56d8];
    int f5830;                                  // +0x5830
    char pad5834[0x5844 - 0x5834];
    int f5844;                                  // +0x5844
    char pad5848[0x5928 - 0x5848];

    bool has_weapon_for(int mode);                                      // 0x437f00
    unsigned pick_threat();                                             // 0x443430
    unsigned pick_random();                                             // 0x43b5d0
    void collect_enemies(void*);                                        // 0x43b300 (ret 4, one unused argument)
    void hand_pos(vector4* out);                                        // 0x43a510
    int update_nav_face();                                              // 0x43cb30
    void set_goal_near(const vector4* center, const float* radius);     // 0x43dd70
    void set_goal_near2(const vector4* center, const float* radius);    // 0x445850
    void set_goal_move(const vector4* p, const float* speed, const int* face, const int* kind);   // 0x445720
    void start_attack(int kind);                                        // 0x4464d0
    void choose_action(int unused);                                     // 0x44d000
    inline Pair cur_pair() const;
};
inline Pair Agent::cur_pair() const
{
    const Pair* p = 0;
    if (cur_slot >= 0) p = &held[cur_slot];
    Pair r;
    r.a = -1;
    if (p) {
        r.a = p->a;
        r.b = p->b;
    }
    return r;
}
#define AG(i) (g_mission->agents.pages[(i) >> 4][(i) & 0xf])

float frand();                                                          // 0x40e400

#define ZERO_S for (int zi = 0; zi < 15; zi++) s[zi] = 0.0f;

// @ 0x44d000
void Agent::choose_action(int unused)
{
    vector4 pv[15];                             // target position per kind
    int ids[15];                                // target id per kind
    ClassRec* cr;
    float s[15];                                // score per kind
    vector4 hp;
    unsigned cand;
    float dist;
    int cls = team;
    if (cls < 0 || cls >= 8) cls = 7;
    int kind = 0;
    if (troop >= 0) {
        if (f2c >= 0)
            kind = 2;
        else
            kind = (g_world->troops[troop].flags & tf_guarantee_ranged) ? 1 : 0;
    }
    cr = &g_mission->teams[cls].cls[kind];

    f55c0 = -1;
    f28c = 0;
    f288 = 1.5f;
    ZERO_S

    if (f10 == 1) {
        if (f55b4 == 1) {
            ZERO_S
            s[1] = 10000.0f;
            pv[1] = f55a4;
        } else if (f5844) {
            s[14] = 10000.0f;
        } else {
            s[13] = 10000.0f;
        }
    } else if (!(f2c4 & 1)) {
        if (f55b4 == 1) {
            s[1] = 10.0f;
            pv[1] = f55a4;
        }
        s[11] = (float)(rand() % 15817) * 9.4834677e-06f + 0.3f;
    } else {
        collect_enemies(&hp);
        cand = pick_random();
        Mission* m = g_mission;
        bool target_mounted = false;
        int held_id = -1;
        dist = 1000.0f;
        if (cand != (unsigned)-1) {
            float dx = m->agents.pages[cand >> 4][cand & 0xf].pos.x - pos.x;
            float dy = m->agents.pages[cand >> 4][cand & 0xf].pos.y - pos.y;
            float dz = m->agents.pages[cand >> 4][cand & 0xf].pos.z - pos.z;
            dist = (float)sqrt(dx * dx + (dy * dy + dz * dz));
            Pair r = m->agents.pages[cand >> 4][cand & 0xf].cur_pair();
            held_id = r.a;
            target_mounted = m->agents.pages[cand >> 4][cand & 0xf].f2c >= 0;
        }
        if (f55b4 == 1) {
            ZERO_S
            s[1] = 10.0f;
            pv[1] = f55a4;
        } else {
            if (nthreat) {
                float t = 5.0f / dist;
                if (t < 0.0f)
                    s[4] = 0.0f;
                else if (t >= 5.0f)
                    s[4] = 5.0f;
                else
                    s[4] = t;
                ids[4] = cand;
                if (!has_weapon_for(1) && !has_weapon_for(3)) s[4] = s[4] * 0.1f;
                if (has_weapon_for(2)) {
                    float t5 = dist * 0.06666667f;
                    if (t5 < 0.0f)
                        s[5] = 0.0f;
                    else if (t5 >= 2.5f)
                        s[5] = 2.5f;
                    else
                        s[5] = t5;
                    float t6 = dist * 0.07692308f;
                    if (!(t6 < 2.0f)) t6 = 2.0f;
                    s[6] = t6 * 1.5f;
                    if (g_world->troops[troop].flags & tf_guarantee_ranged) {
                        s[5] = s[5] + s[5];
                        s[6] = s[6] * 3.0f;
                    }
                    if (f2c != -1) {
                        if (weapon_mode == 2)
                            s[6] = (frand() * 2.0f + 3.5f) * s[6];
                        else if (f5570 < -0.5f)
                            s[6] = (frand() * 5.5f + 6.0f) * s[6];
                    } else {
                        s[6] = 0.0f;
                        if (weapon_mode == 2) s[5] = (frand() + 2.0f) * s[5];
                        if (target_mounted && held_id >= -1 && g_items[held_id].is_ranged())
                            s[5] = s[5] * 2.5f;
                    }
                    ids[5] = cand;
                    ids[6] = cand;
                }
                if (f2c != -1) {
                    s[5] = s[5] * 0.2f;
                    s[4] = 0.0f;
                    if (has_weapon_for(3) || has_weapon_for(1)) {
                        int best = -1;
                        float bests = -100.0f;
                        for (int i = 0; i < nthreat; i++) {
                            float w = 50.0f / (threats[i].dist + 25.0f);
                            if (!(w < 1.0f)) w = 1.0f;
                            int n = AG(threats[i].id).f55e8 - 1;
                            w = w / ((float)(n < 0 ? 0 : n) + 1.0f);
                            float d = dir.y * threats[i].dir.y + (threats[i].dir.z * dir.z + threats[i].dir.x * dir.x);
                            if (0.0f < d) w = (d * d * 20.0f + 1.0f) * w;
                            float sc = ((float)(rand() % 15817) * 6.3223117e-05f + 0.25f) * w * 0.5f;
                            if ((AG(threats[i].id).flags130 & 0x10) || (AG(threats[i].id).flags130 & 0x20))
                                sc = sc * 1.5f;
                            if (bests < sc) {
                                best = i;
                                bests = sc;
                            }
                        }
                        ids[7] = threats[best].id;
                        s[7] = bests;
                    }
                }
            } else {
                if ((g_modes[g_mission->f5df0].flags >> 1) & 1) {
                    for (int i = 0; i < 8; i++) {
                        if (g_mission->teams[team].rel[i] == -1 &&
                            (g_mission->slots[i].b > 0 || g_mission->slots[i].a > 0)) {
                            s[12] = 1000.0f;
                            break;
                        }
                    }
                }
            }
            cls = team;
            if (cls < 0 || cls >= 8) cls = 7;
            int leader = g_mission->teams[cls].leader;
            if (f10 == 0) {
                int k = cr->a;
                if (k == 1 || (k == 2 && leader != id)) {
                    if ((int)cand >= 0) {
                        Agent* o = &AG(cand);
                        float dx = o->pos.x - cr->px;
                        float dy = o->pos.y - cr->py;
                        float dz = o->pos.z - cr->pz;
                        if (cr->radius + cr->radius < (float)sqrt(dx * dx + (dy * dy + dz * dz)) && 10.0f < dist)
                            s[7] = s[7] * 0.0f;
                    }
                    if (k == 2 && leader != id) {
                        hand_pos(&hp);
                        float dx = hp.x - pos.x;
                        float dy = hp.y - pos.y;
                        float dz = hp.z - pos.z;
                        float l = (float)sqrt(dy * dy + (dz * dz + dx * dx)) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[9] = l + 1.0f;
                        pv[1] = AG(leader).pos;
                        pv[9] = AG(leader).pos;
                    } else if (k == 1) {
                        hand_pos(&hp);
                        float dx = hp.x - pos.x;
                        float dy = hp.y - pos.y;
                        float dz = hp.z - pos.z;
                        float l = (float)sqrt(dy * dy + (dz * dz + dx * dx)) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[1] = l + 1.0f;
                        pv[1] = hp;
                    }
                    ids[9] = cand;
                    ids[1] = cand;
                } else if (k == 3) {
                    pv[1] = pos;
                    s[1] = 1.5f;
                    ids[1] = cand;
                }
                if (cr->b == 1 && f2c == -1) {
                    Troop* T = &g_world->troops[troop];
                    if ((T->flags & tf_hero) || (T->flags & tf_guarantee_horse)) {
                        AgentTable* tbl = &g_mission->agents;
                        float best = 10000.0f;
                        int bestid = -1;
                        unsigned it = tbl->first_used();
                        unsigned cnt = tbl->count;
                        if (it < cnt) {
                            do {
                                Agent* a = &AG(it);
                                if (a->f10 == 1 && a->f30 == -1 && a->f5830 == 2) {
                                    int v = a->f8c.stat_1ec();
                                    if (v <= T->nibble(0x18)) {
                                        float dx = a->pos.x - pos.x;
                                        float dy = a->pos.y - pos.y;
                                        float dz = a->pos.z - pos.z;
                                        float dd = (float)sqrt(dy * dy + (dz * dz + dx * dx));
                                        if (dd < best) {
                                            best = dd;
                                            bestid = it;
                                        }
                                    }
                                }
                                tbl->next_used(&it);
                            } while (it < cnt);
                            if (bestid != -1 && best < 60.0f) {
                                s[2] = 2.2f;
                                ids[2] = bestid;
                            }
                        }
                        goto randomise;
                    }
                }
                if (cr->b == 2 && f2c != -1 && 5.0f < dist) s[3] = 50.0f;
            }
        }
    }
randomise:
    for (int i = 0; i < 15; i++) s[i] = ((float)(rand() % 15817) * 1.2644623e-05f + 0.9f) * s[i];
    if (f2e0 == 2) {
        s[5] = s[5] * 0.1f;
        s[6] = s[6] * 0.5f;
    }
    int best = 0;
    for (int i = 1; i < 15; i++)
        if (s[i] > s[best]) best = i;

    int saved = mode;
    start_attack(best);
    f2cc = ids[best];
    f2d0 = pv[best];
    f2e4 = 0;
    f2e8 = -1;
    if (best == 11 || best != 1) {
        if (best == 5 || best == 6) {
            int t = pick_threat();
            if (t >= 0) f2cc = t;
        }
    } else {
        int k = cr->a;
        float zero;
        if (k == 1) {
            zero = 0.0f;
            set_goal_near2(&f2d0, &zero);
        } else {
            bool other = false;
            if (k == 2) {
                int c2 = team;
                if (c2 < 0 || c2 >= 8) c2 = 7;
                if (g_mission->teams[c2].leader != id) {
                    zero = 0.0f;
                    set_goal_near2(&f2d0, &zero);
                    other = true;
                }
            }
            if (!other) {
                if (k == 3 && f55b4 != 1) {
                    if (f53a0 == 0) {
                        vector4 p;
                        p.x = body->f.x + pos.x;
                        p.y = body->f.y + pos.y;
                        p.z = body->f.z + pos.z;
                        int fkind = 2;
                        int face = update_nav_face();
                        float speed = 2.0f;
                        set_goal_move(&p, &speed, &face, &fkind);
                    }
                } else {
                    zero = 0.0f;
                    set_goal_near(&f2d0, &zero);
                }
            }
        }
        f5338 |= 0x20;
        if (f2c < 0 && f10 != 1) {
            if (f55b4 == 0)
                f288 = 1.0f;
            else
                f288 = 1.75f;
        } else {
            f288 = 3.0f;
        }
    }
    f2e0 = 0;
    if (mode != saved) f2ec = 0;
}
