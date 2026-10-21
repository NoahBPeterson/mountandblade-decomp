// Slice s0043bdb0: one large function (names provisional): the per-tick update of a ranged agent's aim error.
// From the wielded item (bow, crossbow, thrown, pistol, musket) and the shooter's skills / proficiency it derives a
// target aim error: a base term from the item's accuracy stat, a draw/reload-time term (noise-modulated), the
// agent's (or its mount's) speed, and how fast the agent is turning (facing delta / step). It then eases the
// stored value at +0x5634 towards the target at a rate that depends on the frame step and on `active`.
#include <math.h>
#include "rgl.h"
using rgl::vector4;

// Module System item type ids (header_items.py itp_type_*) and skill ids (header_skills.py skl_*)
enum { itp_type_bow = 8, itp_type_crossbow = 9, itp_type_thrown = 10, itp_type_pistol = 16, itp_type_musket = 17 };
enum { skl_horse_archery = 23, skl_riding = 24, skl_power_draw = 33, skl_power_throw = 34 };

// item reference: item index + packed amount/modifier (s00434ad0, verified there)
struct ItemRef {
    int item;
    unsigned m4;
    int max_amount();                   // 0x434b70
    int stat_1ec();                     // 0x434c50
    int stat_21x(int which);            // 0x434cb0
    int stat_204();                     // 0x435090
};
// item kind record (0x238 bytes, array at 0x5e29d0)
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                // +0x1c8
    char pad1cc[0x204 - 0x1cc];
    int f204;                           // +0x204 (accuracy stat)
    char pad208[0x238 - 0x208];
};
extern ItemKind* g_items;               // 0x5e29d0

struct CharRec {                        // troop record, 0x508 bytes
    char pad0[0x8c];
    float prof_archery, prof_crossbow, prof_throwing, prof_firearm;   // +0x8c: wpt_archery..wpt_firearm
    char pad9c[0x508 - 0x9c];
};
struct GameData {
    char pad[0x13a00];
    CharRec* chars;                     // +0x13a00
    int skill(int troop, int id);       // 0x437b20 (thiscall, ret 8)
};
extern GameData* g_game;                // 0x5e2930

struct Vec2 { float x, y; };
struct AgentRec { char pad[0xa8]; Vec2 vel; };   // an agent as seen through the table
struct AgentTable {
    AgentRec* get(unsigned i);          // 0x43b1f0 (thiscall, ret 4)
};
struct Mission { int f0; AgentTable tbl; };
extern Mission* g_mission;              // 0x5e292c

extern __int64 g_ticks[];               // 0x5e2948 (clock table, 8 bytes per clock)
extern float g_step;                    // 0x5a9c20

struct Timer {
    int clock;
    int pad4;
    __int64 start;
    float elapsed();                    // 0x432c10 (thiscall)
};

void __cdecl noise_gradient(float scale, const float* in, float* out);   // 0x414ae0
float __cdecl fmin_ref(const float* a, const float* b);              // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);              // 0x4324d0

struct Agent {
    char pad0[0x2c];
    int mount;                          // +0x2c (mount / rider agent index, -1 none)
    char pad30[0x88 - 0x30];
    int troop;                          // +0x88
    char pad8c[0xa8 - 0x8c];
    Vec2 vel;                           // +0xa8
    char padb0[0x1d0 - 0xb0];
    ItemRef slots[8];                   // +0x1d0
    char pad210[0x22c - 0x210];
    int cur_slot;                       // +0x22c
    char pad230[0x55d8 - 0x230];
    vector4 facing;                     // +0x55d8
    char pad55e8[0x560c - 0x55e8];
    int active;                         // +0x560c
    char pad5610[0x5624 - 0x5610];
    vector4 prev_facing;                // +0x5624
    float aim_error;                    // +0x5634
    char pad5638[0x57f0 - 0x5638];
    Timer timer;                        // +0x57f0
    float draw_speed(int hand);         // 0x4385e0 (thiscall)
    void update_aim_error();
};

// Notes on the spelling (all found by diffing against the original; none changes behaviour):
//  - `(0.0f > x) ? 0.0f : x` is the inlined max(0, x) the original used for the shot-error floor; the second
//    `if (acc < 0.0f)` is real source too (the original tests twice).
//  - the speed term is `spd * 0.0f` in the original (a tuning constant that is zero in 1.011).
//  - `sp` (mount-less speed) is a float: the original compares/adds it with single-precision constants.
//  - tm is Timer::elapsed() inlined at its first use; the later call in the crossbow/firearm arm stays a call.
// @ 0x43bdb0
void Agent::update_aim_error()
{
    float v = 0.0f;
    ItemRef* r = 0;
    if (cur_slot >= 0) r = &slots[cur_slot];
    ItemRef tmp;
    tmp.item = -1;
    if (r) tmp = *r;
    ItemRef cur = tmp;
    int item = cur.item;
    if (item >= 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_bow || t == itp_type_crossbow || t == itp_type_thrown || t == itp_type_pistol || t == itp_type_musket) {
            float tm = (float)(g_ticks[timer.clock] - timer.start) * 1e-5f;
            float a, b, nz;
            {
                vector4 p, n;
                p.x = tm; p.y = a = tm + 3.0f; p.z = b = tm * 1.5f;
                noise_gradient(0.7f, &p.x, &n.x);
                nz = (n.y + n.x) * 1.9f + 1.4f;
            }
            ItemKind* ik = &g_items[item];
            vector4 d;
            d.x = facing.x - prev_facing.x;
            d.y = facing.y - prev_facing.y;
            d.z = facing.z - prev_facing.z;
            float spd = 0.0f;
            if (g_step > 0.0f) spd = d.length() / g_step;
            spd = spd * 0.0f;
            if (spd > 1.0f) spd = 1.0f;
            prev_facing = facing;
            unsigned t2 = ik->type_flags & 0xff;
            if (t2 == itp_type_bow) {
                int tr = troop;
                float sk = (float)g_game->skill(tr, skl_power_draw) * 25.0f + (int)g_game->chars[tr].prof_archery;
                float p21 = (float)cur.stat_21x(1) * 8.0f;
                float w = (float)cur.stat_1ec() * 40.0f + p21 + 50.0f;
                int c = g_items[item].f204;
                if (c == 0) c = 99;
                float cf = (float)c * 0.01f;
                float acc0 = ((w + 50.0f) - sk) * 6e-5f;
                float acc = (0.0f > acc0) ? 0.0f : acc0;
                if (acc < 0.0f) acc = 0.0f;
                if (active) {
                    int ag = mount;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->tbl;
                        if (1.0f < fabs(tb->get(ag)->vel.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel.y) + 3.0f - (sk + 200.0f) * (((float)g_game->skill(tr, skl_riding) + 5.0f) * (float)g_game->skill(troop, skl_horse_archery)) * 0.0004f) * 0.008f;
                            float zero = 0.0f;
                            float m = 4.0f / ((float)g_game->skill(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel.y * vel.y + vel.x * vel.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 200.0f)) * nz * 7.0f + acc);
                    }
                    float lvl = 1.25f / draw_speed(0);
                    float g = (sk - w * 0.5f) * 0.01f;
                    g = (0.3f > g) ? 0.3f : g;
                    float thr = g + lvl;
                    v = acc;
                    if (tm < lvl) {
                        float zero = 0.0f;
                        float x = (lvl - tm) / lvl - sk * 0.001f;
                        v = fmax_ref(&zero, &x) + acc;
                    } else if (thr < tm) {
                        vector4 q, n; q.x = tm; q.y = a; q.z = b;
                        noise_gradient(0.4f, &q.x, &n.x);
                        float e1 = (tm - thr) * 0.4f;
                        float e2 = 1.0f;
                        float w2 = (float)fabs(((n.y + n.x) + 1.0f) * 0.5f);
                        v = (w * 0.0006f) * (w2 * fmin_ref(&e1, &e2)) + acc;
                    }
                    float r2 = (100.0f / (sk + 100.0f)) * spd - sk * 0.002f;
                    r2 = (0.0f > r2) ? 0.0f : r2;
                    v = (1.0f - cf) * 0.08f + (r2 * 0.2f + v);
                } else {
                    v = 0.0;
                }
            } else if (t2 != itp_type_crossbow && t2 != itp_type_pistol && t2 != itp_type_musket) {
                int tr = troop;
                float sk = (float)g_game->skill(tr, skl_power_throw) * 20.0f + (int)g_game->chars[tr].prof_throwing;
                float cf = (float)cur.stat_204() * 0.01f;
                if (active) {
                    float p21 = (float)cur.stat_21x(1);
                    float fv = (float)cur.stat_1ec() * 5.0f + p21;
                    float zero = 0.0f;
                    float x = ((fv + 30.0f) * 8.0f - sk) * 0.00012f;
                    float acc = fmax_ref(&zero, &x);
                    int ag = mount;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->tbl;
                        if (1.0f < fabs(tb->get(ag)->vel.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel.y) + 5.0f - (sk + 200.0f) * (((float)g_game->skill(tr, skl_riding) + 5.0f) * (float)g_game->skill(troop, skl_horse_archery)) * 0.0004f) * 0.004f;
                            float zero2 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero2, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel.y * vel.y + vel.x * vel.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 50.0f)) * nz + acc);
                    }
                    float zero3 = 0.0f;
                    float y = (100.0f / (sk + 100.0f)) * spd - sk * 0.0007f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero3, &y) * 0.3f + acc);
                } else {
                    v = 0.0;
                }
            } else {
                float cf = (float)cur.stat_204() * 0.01f;
                float A, B;
                if (t2 == itp_type_crossbow) {
                    A = (float)(int)g_game->chars[troop].prof_crossbow;
                    B = (float)cur.stat_21x(1);
                } else if (t2 == itp_type_pistol || t2 == itp_type_musket) {
                    A = (float)(int)g_game->chars[troop].prof_firearm;
                    B = (float)cur.stat_21x(1);
                }
                if (active && cur.max_amount() != (int)(cur.m4 & 0xff)) {
                    float zero = 0.0f;
                    float p1 = ((B + B + 100.0f) - A) * 0.00015f;
                    float q = A + 100.0f;
                    float p2 = 0.7f - timer.elapsed() * q * 0.01f;
                    float zero2 = 0.0f;
                    float r1 = fmax_ref(&zero, &p1);
                    float acc = fmax_ref(&zero2, &p2) + r1;
                    int ag = mount;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->tbl;
                        if (1.0f < fabs(tb->get(ag)->vel.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel.y) + 5.0f - (A + 150.0f) * (((float)g_game->skill(troop, skl_riding) + 5.0f) * ((float)g_game->skill(troop, skl_horse_archery) + 2.0f)) * 0.0004f) * 0.005f;
                            float zero3 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero3, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel.y * vel.y + vel.x * vel.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / q) * nz * 3.0f + acc);
                    }
                    float zero4 = 0.0f;
                    float y = (100.0f / q) * spd - A * 0.004f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero4, &y) * 0.15f + acc);
                } else {
                    v = 0.0;
                }
            }
        }
    }
    double rate = g_step * 5.0f;
    if (!active) rate = g_step;
    if (aim_error < v) rate = rate + rate;
    if (rate > 1.0) rate = 1.0f;
    aim_error = (float)(rate * v + (1.0 - rate) * aim_error);
}
