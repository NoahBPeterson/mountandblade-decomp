// Slice s004465e0: Agent::update_ai (0x4465e0, 10436 bytes), the per-tick AI "order executor" of a mission agent.
// Switches on the agent's current ai mode (+0x2c8) and picks a target / movement goal / weapon mode for it.
// All names are provisional; every offset comes from this function and the neighbours' verified layouts
// (s00437f00 Agent/Pair/ItemKind/World, s0043ac40 AgentTable/Threat, s00440a30 ai_agent.h).
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;


// Bodies the original translation unit could see (header-inline there, kept out of line): cl uses them for
// its side-effect analysis (a call that cannot write memory does not end a CSE'd pages-pointer load).
namespace rgl {
// @ 0x407d60
__declspec(noinline) float vector4::normalize()
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
// @ 0x407ca0
__declspec(noinline) vector4 vector4::operator-(const vector4& o) const { return vector4(x - o.x, y - o.y, z - o.z); }
// @ 0x407c50
__declspec(noinline) float vector4::out_length() const { return length(); }
// @ 0x407d30
__declspec(noinline) float length(const vector4& v) { return v.length(); }
}

// item type ids (Module System header_items itp_type_*), low byte of ItemKind::type_flags
enum { itp_type_bow = 7 };
// troop flag (header_troops tf_guarantee_ranged), 64-bit troop flags at +0x30
#define tf_guarantee_ranged 0x4000000i64

struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8
    char pad1cc[0x1f4 - 0x1cc];
    int f1f4;                                   // +0x1f4
    char pad1f8[0x20c - 0x1f8];
    int f20c;                                   // +0x20c
    char pad210[0x238 - 0x210];
    bool is_ranged();                           // 0x433d90
};
extern ItemKind* g_items;                       // 0x5e29d0

struct Pair { int a; unsigned b; };

struct ClassRec { int a; char pad[0x6c - 4]; };                      // a: 1/2/3 = some ai class (compared with 1, 2, 3)
struct TeamRec { int leader; ClassRec cls[3]; int rel[8]; int pad148; };   // 0x16c bytes, array at mission+0x5280; rel[] = relation row (1 friend, -1 enemy)

struct Troop {
    char pad0[0x30];
    __int64 flags;                              // +0x30
    char pad38[0xb0 - 0x38];
    int fb0;                                    // +0xb0
    char padb4[0x508 - 0xb4];
    int nibble(int i);                          // 0x4351a0
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                              // +0x13a00 (stride 0x508)
};
extern World* g_world;                          // 0x5e2930

struct HeightField {
    void sample(float x, float y, float* out_h, int* out_id);      // 0x433510
};
struct Scene {
    char pad0[0x64];
    HeightField* hf;                            // +0x64
};

struct Agent;
struct AgentTable {
    int count;                                  // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                  // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                // 0x43b160
    int first_used();                           // 0x43b1b0
    Agent* get(unsigned id);                    // 0x43b1f0
    __forceinline Agent* at(unsigned id);       // inline twin of get()
};
struct Mission {
    Scene* scene;                               // +0
    AgentTable agents;                          // +4
    char pad28[0x5c - 0x28];
    int player_id;                              // +0x5c
    char pad60[0x5280 - 0x60];
    TeamRec teams[8];                           // +0x5280
    char pad5de0[0x6554 - 0x5de0];
    char f6554;                                 // +0x6554
};
extern Mission* g_mission;                      // 0x5e292c

struct Skel { char pad[0x58]; vector4 s, f, u; };   // agent body: frame rows from +0x58 (s), +0x68 (f), +0x78 (u)

struct vec2 {
    float x, y;
    float normalize();                          // 0x4367c0
};

struct Threat { int id; float dist; vector4 dir; };   // 0x18 bytes

struct Agent {
    char pad0[4];
    int used;                                   // +4
    int id;                                     // +8
    char padc[4];
    int f10;                                    // +0x10
    int f14;                                    // +0x14 (1 = alive)
    char pad18[0x2c - 0x18];
    int f2c;                                    // +0x2c (-1 = on foot)
    char pad30[0x50 - 0x30];
    vector4 pos;                                // +0x50
    vector4 dir;                                // +0x60
    char pad70[0x88 - 0x70];
    int troop;                                  // +0x88
    char pad8c[0xac - 0x8c];
    float fac;                                  // +0xac
    char padb0[0xbc - 0xb0];
    int* fbc;                                   // +0xbc
    char padc0[0x1c4 - 0xc0];
    int team;                                   // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    Pair held[11];                              // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c (slots[0] = current held item; slots[1] at +0x230, slots[4] at +0x23c)
    char pad240[0x250 - 0x240];
    Skel* body;                                 // +0x250
    char pad254[0x27c - 0x254];
    int f27c, f280, f284;                       // +0x27c
    float f288, f28c;                           // +0x288
    char pad290[0x294 - 0x290];
    float f294, f298, f29c;                     // +0x294
    char pad2a0[0x2c0 - 0x2a0];
    float f2c0;                                 // +0x2c0
    unsigned char f2c4;                         // +0x2c4
    char pad2c5[3];
    int mode;                                   // +0x2c8
    int f2cc;                                   // +0x2cc
    vector4 f2d0;                               // +0x2d0
    char pad2e0[0x2e4 - 0x2e0];
    int f2e4, f2e8;                             // +0x2e4
    char pad2ec[0x310 - 0x2ec];
    int f310;                                   // +0x310
    float f314;                                 // +0x314
    int f318;                                   // +0x318
    int target;                                 // +0x31c
    char pad320[0x5324 - 0x320];
    int f5324;                                  // +0x5324
    char pad5328[4];
    int f532c;                                  // +0x532c
    char f5330;                                 // +0x5330
    char pad5331[7];
    unsigned f5338;                             // +0x5338
    int f533c;                                  // +0x533c
    char pad5340[0x5398 - 0x5340];
    int f5398;                                  // +0x5398
    char pad539c[0x53ec - 0x539c];
    Threat threats[16];                         // +0x53ec
    int nthreat;                                // +0x556c
    char pad5570[0x55a4 - 0x5570];
    vector4 f55a4;                              // +0x55a4
    int f55b4, f55b8;                           // +0x55b4
    char pad55bc[4];
    int f55c0;                                  // +0x55c0
    char pad55c4[0x56b4 - 0x55c4];
    vector4 vel;                                // +0x56b4
    char pad56c4[0x56d4 - 0x56c4];
    int weapon_mode;                            // +0x56d4
    char pad56d8[0x5928 - 0x56d8];

    // callees
    bool is_enemy(const Agent* o) const;                                // 0x432350
    Pair slot_pair(int idx) const;                                      // 0x432790
    bool f100_is_0x48_and_charged() const;                              // 0x432470
    bool line_of_sight(unsigned id, void* ignore);                      // 0x440e30
    int fire_check(unsigned id, float* out_blocker);                      // 0x441020
    void reset_act();                                                   // 0x432080
    unsigned pick_shout_target(int flags, float maxdist);               // 0x443270
    unsigned pick_threat();                                             // 0x443430
    void reset_if_enemy_charged();                                      // 0x435b90
    bool has_weapon_for(int mode);                                      // 0x437f00
    unsigned pick_target();                                             // 0x43b4d0
    unsigned pick_random();                                             // 0x43b5d0
    void collect_enemies(void*);                                        // 0x43b300 (ret 4, one unused argument)
    void hand_pos(vector4* out);                                        // 0x43a510
    int update_nav_face();                                              // 0x43cb30
    void set_goal_agent(unsigned id);                                   // 0x43db90
    void set_goal_near(const vector4* center, const float* radius);     // 0x43dd70
    void set_goal_near2(const vector4* center, const float* radius);    // 0x445850
    void set_goal_move(const vector4* p, const float* speed, const int* face, const int* kind);   // 0x445720
    void set_weapon_mode(int mode);                                     // 0x4461e0
    void update_ai();                                                   // 0x4465e0

    // inlined helpers
    __forceinline ClassRec* class_rec();
    __forceinline Pair cur_pair() const;
    __forceinline Pair slot_pair_inl(int idx) const;
    void tail();
};
__forceinline Agent* AgentTable::at(unsigned id) { return &pages[id >> 4][id & 0xf]; }
// @ 0x43b1f0
// @ 0x432350 (header-inline in the original TU: cl sees it is pure, so x87 temporaries survive the call)
__declspec(noinline) bool Agent::is_enemy(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->teams[team].rel[o->team] == -1;
    return false;
}
// @ 0x43b160 / 0x43b1b0: header-inline in the original TU too (visible, so cl knows they leave g_mission alone)
__declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    unsigned v = ++*i;
    if ((int)v < count) {
        do {
            if (pages[v >> 4][v & 0xf].used != 0) break;
        } while ((int)(*i = ++v) < count);
    }
}
__declspec(noinline) int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].used != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
// inline vector helper of the original TU (rgl's operator- is out-of-line elsewhere)
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
#define AG(i) (*g_mission->agents.at(i))

// 0x432350-adjacent: team field 0x5280 of an agent's team (cdecl, arg on the stack)
int __cdecl team_leader(const Agent* a);                                // 0x435b60
float __cdecl hit_chance(const Pair& who, Agent* a);                    // 0x438cb0
float frand();                                                          // 0x40e400
float clamp(const float* x, const float* lo, const float* hi);          // 0x40e3d0
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
extern float g_dt_scale;                                                // 0x5a9bf0
extern int g_int_5aa04c;                                                // 0x5aa04c
extern unsigned g_flags_5e2470;                                         // 0x5e2470
extern int g_player_agent;                                              // 0x8ed50dc

__forceinline ClassRec* Agent::class_rec()
{
    int cls = team;
    if (cls < 0 || cls >= 8) cls = 7;
    int kind = 0;
    if (troop >= 0) {
        if (f2c >= 0)
            kind = 2;
        else
            kind = (g_world->troops[troop].flags & tf_guarantee_ranged) ? 1 : 0;
    }
    return &g_mission->teams[cls].cls[kind];
}
__forceinline Pair Agent::cur_pair() const
{
    const Pair* p = 0;
    if (slots[0] >= 0) p = &held[slots[0]];
    Pair r;
    r.a = -1;
    if (p) {
        r.a = p->a;
        r.b = p->b;
    }
    return r;
}
__forceinline Pair Agent::slot_pair_inl(int idx) const
{
    const Pair* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &held[i];
    Pair r;
    r.a = -1;
    if (p) {
        r.a = p->a;
        r.b = p->b;
    }
    return r;
}

// @ 0x4465e0
void Agent::update_ai()
{
    ClassRec* cr = class_rec();

    int prev = -1;
    if (f55c0 >= 0 && AG(f55c0).f14 == 1) {
        Agent* o = &AG(f55c0);
        vector4 d;
        d.x = o->pos.x - pos.x;
        d.y = o->pos.y - pos.y;
        d.z = o->pos.z - pos.z;
        float len = d.normalize();
        bool far_ = true;
        if (target >= 0) {
            const vector4& cp = AG(target).pos;
            vector4 dd = vsub(cp, pos);
            far_ = !(sqrt(dd.y * dd.y + dd.z * dd.z + dd.x * dd.x) <= len + 1.0f);
        }
        if (far_ && is_enemy(o))
            target = f55c0;
        prev = f55c0;
    }

    bool flag_a = false;        // local_81
    bool flag_b = false;        // local_5d
    int mode_ = mode;

    if (mode_ == 1) {
        if (!(f2c4 & 1)) {
            f533c = 0;
            f5338 |= 0x20;
        } else {
            unsigned t = target;
            if (t == 0xffffffff || AG(t).f14 != 1 || frand() < 0.1f)
                flag_a = true;
            t = target;
            if ((int)t >= 0) {
                bool ranged = false;
                const vector4& op = AG(t).pos;
                vector4 dd = vsub(pos, op);
                float dist = (float)sqrt(dd.y * dd.y + dd.z * dd.z + dd.x * dd.x);
                Pair p = AG(t).cur_pair();
                if (p.a >= 0 && g_items[p.a].is_ranged())
                    ranged = true;
                if (f2c >= 0 || vel.length_squared() < 0.7f || dist < 15.0f ||
                    (dist < 100.0f && weapon_mode == 2) || ranged) {
                    f533c = 2;
                    Pair c = cur_pair();
                    if (c.a >= 0 && g_items[c.a].is_ranged())
                        f533c = 0x22;
                }
            }
            flag_b = true;
            f5338 |= 0x20;
        }
    } else if (mode_ == 4) {
        if (prev == -1 || target == -1 || AG(target).f14 != 1)
            target = pick_shout_target(0x26, 300.0f);
        if (target == -1) {
            reset_if_enemy_charged();
            f533c = 0;
        } else if (AG(target).f14 != 1) {
            f2c0 = 0.0f;
        } else {
            vector4 w, q;
            const vector4& op = AG(target).pos;
            w.x = op.x - pos.x;
            w.y = op.y - pos.y;
            w.z = op.z - pos.z;
            float dist = w.normalize();
            bool ranged = false;
            {
                Pair sp = AG(target).slot_pair(0);
                if (sp.a >= 0 && g_items[sp.a].is_ranged())
                    ranged = true;
            }
            f533c = 2;
            if (!ranged && dist > 15.0f && f2c < 0)
                f533c = 0;
            if (f314 < 0.0f)
                f314 = 0.0f;
            else if (f314 > 30.0f)
                f314 = 30.0f;
            vector4 acc[2];
            float wsum[2];
            q.x = q.y = q.z = 0.0f;
            acc[0] = q;
            acc[1] = q;
            wsum[0] = wsum[1] = 0.0f;
            float t98 = 0.0f;                       // cnt, later k and the clamp argument (one address-taken scratch)
            int best = -1;
            unsigned it = g_mission->agents.first_used();
            if (it < (unsigned)g_mission->agents.count) {
                do {
                    Agent* a = &AG(it);
                    if (a->f10 == 0 && a->id != target && a->f14 == 1 && a->id != id) {
                        vector4 dv = a->pos - pos;
                        float len = dv.normalize();
                        float k = 2.0f / (2.0f + len);
                        float w2 = k * k;
                        int idx = 0;
                        if (is_enemy(a)) {
                            idx = 1;
                            if (len < 5.0f) {
                                t98 += 1.0f;
                                best = it;
                            }
                        }
                        wsum[idx] = w2 + wsum[idx];
                        q.z = dv.z * w2;
                        float* ap = &acc[idx].x;
                        ap[0] = dv.x * w2 + ap[0];
                        ap[1] = dv.y * w2 + ap[1];
                        ap[2] = q.z + ap[2];
                    }
                    g_mission->agents.next_used(&it);
                } while (it < (unsigned)g_mission->agents.count);
            }
            if (dist > 10.0f && best >= 0) {
                const vector4& bp = AG(best).pos;
                q.x = bp.x - pos.x;
                q.y = bp.y - pos.y;
                q.z = bp.z - pos.z;
                q.normalize();
                if (q.x * f294 + (q.y * f298 + q.z * f29c) > -0.3f)
                    target = best;
            }
            bool close_ = false;
            if (cr->a == 1) {
                vector4 d;
                hand_pos(&q);
                d.x = pos.x - q.x;
                d.y = pos.y - q.y;
                d.z = pos.z - q.z;
                float len = d.normalize();
                if (len > 5.0f) {
                    mode = 1;
                    float r = 0.2f;
                    set_goal_near2(&q, &r);
                    close_ = true;
                }
            }
            if (cr->a == 2) {
                int t2 = team;
                if (t2 < 0 || t2 >= 8) t2 = 7;
                if (g_mission->teams[t2].leader != id) {
                    vector4 d;
                    hand_pos(&q);
                    d.x = pos.x - q.x;
                    d.y = pos.y - q.y;
                    d.z = pos.z - q.z;
                    float len = d.normalize();
                    if (len > 8.0f) {
                        mode = 9;
                        goto L_tail4;
                    }
                }
            }
            if (!close_) {
                if (f2e4 == 0) {
                    vector4 d;
                    float diff = wsum[1] - wsum[0];
                    if (diff < 0.05f)
                        diff = 0.05f;
                    else if (diff >= 0.3f)
                        diff = 0.3f;
                    int r = rand() % 0x3dc9;
                    int hf = f55b8;
                    float push = (((float)r * (1.0f / 15817.0f)) * diff * g_dt_scale) * 20.0f;
                    if (hf == 0)
                        push = ((float)g_int_5aa04c * 0.5f + (40.0f - (float)g_world->troops[troop].fb0) * 0.012f) + push;
                    push = push + f314;
                    f314 = push;
                    if (push < 0.0f)
                        f314 = 0.0f;
                    else if (push > 11.0f)
                        f314 = 11.0f;
                    if (f55b4 == 1)
                        f314 = 0.0f;
                    {
                        Pair cp = AG(target).cur_pair();
                        if ((cp.a < 0 || !g_items[cp.a].is_ranged()) && f314 >= 10.0f) {
                            f314 = 10.0f;
                            if (hf == 0)
                                f2e4 = 1;
                        }
                    }
                    if (f2e8 != 0 || frand() < 0.3f - t98 * 0.05f) {
                        f2e8 = f2e4;
                        if (f55b4 == 1) {
                            float r2 = 0.2f;
                            set_goal_near(&f55a4, &r2);
                        } else {
                            int fid = -1;
                            if (g_mission->scene->hf)
                                g_mission->scene->hf->sample(pos.x, pos.y, wsum, &fid);
                            if (dist < 4.0f ||
                                (g_mission->scene->hf && (fbc == 0 || fbc[1] != fid))) {
                                set_goal_agent(target);
                            } else {
                                float r30 = 30.0f;
                                float m = fmin_ref(&dist, &r30);
                                float fr = frand();
                                AgentTable* tb = &g_mission->agents;
                                int u = target;
                                t98 = ((fr + 2.0f) * m) * 0.2f;
                                const vector4* pv = &tb->get(u)->vel;
                                d.x = t98 * pv->x;
                                d.y = t98 * pv->y;
                                d.z = t98 * pv->z;
                                pv = &tb->get(u)->vel;
                                q = *pv;
                                q.normalize();
                                float f = q.y * w.y + (q.z * w.z + q.x * w.x) + 1.0f;
                                w.x = d.x * f;
                                w.y = d.y * f;
                                w.z = d.z * f;
                                const vector4* pp = &tb->get(u)->pos;
                                q.x = w.x + pp->x;
                                q.y = w.y + pp->y;
                                q.z = w.z + pp->z;
                                t98 = dist * 0.3f - 0.6f;
                                float hi = 20.0f;
                                float lo = 0.0f;
                                d = q;
                                float c = clamp(&t98, &lo, &hi);
                                if (f55b8 != 0 && c > 0.5f)
                                    c = 0.5f;
                                int ms = (int)(c * 0.2f + 4.0f);
                                int face = tb->get(u)->update_nav_face();
                                set_goal_move(&d, &c, &face, &ms);
                            }
                        }
                    }
                } else if (f2e4 == 1) {
                    vector4 d;
                    float val = wsum[0] * 1.3f - wsum[1];
                    float hi = 0.3f;
                    float lo = 0.01f;
                    float c = clamp(&val, &lo, &hi);
                    float fr = frand();
                    float nv = f314 - ((fr * c) * g_dt_scale) * 20.0f;
                    f314 = nv;
                    if (nv < 0.0f)
                        f314 = 0.0f;
                    else if (nv > 11.0f)
                        f314 = 11.0f;
                    bool rg = false;
                    {
                        Pair sp2 = AG(target).slot_pair(0);
                        if (sp2.a >= 0 && g_items[sp2.a].is_ranged())
                            rg = true;
                    }
                    if (f314 < 0.0f || rg) {
                        f314 = 0.0f;
                        f2e4 = 0;
                    }
                    if (f2e8 != 1) {
                        f2e8 = f2e4;
                        float z = acc[0].z * 4.0f;
                        q.x = acc[0].x * 4.0f - acc[1].x;
                        q.y = acc[0].y * 4.0f - acc[1].y;
                        q.z = z - acc[1].z;
                        q.normalize();
                        float r16 = 16.0f;
                        q.z = q.z * 15.0f;
                        d.x = q.x * 15.0f + pos.x;
                        d.y = q.y * 15.0f + pos.y;
                        d.z = q.z + pos.z;
                        set_goal_near(&d, &r16);
                    }
                }
            }
        }
    L_tail4:
        {
            unsigned fl = f5338;
            f5338 = fl | 0x20;
            float s = 0.5f;
            Pair cp = cur_pair();
            if (cp.a != -1)
                s = (float)g_items[cp.a].f20c * 0.01f + 0.9f;
            if (fl & 2) {
                f288 = s;
                f28c = s - 0.4f;
            } else {
                f288 = 0.5f;
                f28c = 0.2f;
            }
        }
    } else if (mode_ == 5 && f5398 != 1 && f5398 != 3 && f5398 != 4) {
        float chance = hit_chance(slot_pair(0), this);
        vector4 d, q, w;
        float t80 = 500.0f;
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            d = g_mission->agents.get(e)->pos - pos;
            near_ = d.normalize();
        }
        if (target >= 0 && g_mission->agents.get(target)->f14 != 1)
            target = -1;
        if (target == -1) {
            int c = f2cc;
            if (c >= 0 && g_mission->agents.get(c)->f14 == 1)
                target = c;
        }
        int tg = target;
        float cur;
        if (tg >= 0) {
            d = g_mission->agents.get(tg)->pos - pos;
            cur = d.normalize();
        } else {
            cur = t80;
        }
        if ((near_ < 8.0f && cur > near_ + near_) || tg == -1)
            target = e;
        float wsum[2];
        if (frand() < 0.2f || target == -1 || g_mission->agents.get(target)->f14 != 1) {
            collect_enemies(wsum);
            if (nthreat > 0) {
                int c = (threats[0].dist < 3.5f && g_mission->agents.get(threats[0].id)->f14 == 1) ? threats[0].id : pick_threat();
                if (c >= 0) {
                    f2cc = c;
                    target = c;
                }
            }
        }
        int t3 = target;
        if (t3 >= 0 && g_mission->agents.get(t3)->f14 == 1) {
            Agent* ta = g_mission->agents.get(t3);
            f533c = 0x22;
            d = ta->pos - pos;
            t80 = d.normalize();
            int bestid = -1;
            bool close99 = false;
            if (frand() + 1.5f > t80)
                f2c0 = 0.0f;
            float one = 1.0f;
            t80 = (ta->pos.z - pos.z) * 0.7f + t80;
            float dd = fmax_ref(&t80, &one);
            if (cr->a == 1 || (cr->a == 2 && team_leader(this) != id)) {
                close99 = true;
                hand_pos(&w);
                q.x = pos.x - w.x;
                q.y = pos.y - w.y;
                q.z = pos.z - w.z;
                wsum[0] = q.normalize();
            }
            t80 = chance * 0.7f;
            one = 30.0f;
            if ((fmin_ref(&one, &t80) < dd && !close99 && cr->a != 3) ||
                (f55b8 != 0 && !close99 && cr->a != 3)) {
                q = ta->pos;
                bestid = ta->id;
            } else {
                if (line_of_sight(ta->id, 0) && fire_check(ta->id, &t80) == 1 && (!close99 || wsum[0] < 3.0f)) {
                    reset_act();
                    goto L_a79;
                }
                if (close99) {
                    q = w;
                } else {
                    d.z = d.z * 0.7f;
                    q.x = d.x * 0.7f + pos.x;
                    q.y = d.y * 0.7f + pos.y;
                    q.z = d.z + pos.z;
                    q.w = w.w;
                    w.x = q.x;
                    w.y = q.y;
                    w.z = q.z;
                }
            }
            if (frand() < 0.3f) {
                if (close99) {
                    wsum[0] = 2.0f;
                    set_goal_near2(&q, &wsum[0]);
                } else if (bestid >= 0) {
                    int two = 2;
                    int face = g_mission->agents.get(bestid)->update_nav_face();
                    one = 1.0f;
                    set_goal_move(&q, &one, &face, &two);
                } else {
                    wsum[0] = 1.8f;
                    set_goal_near(&q, &wsum[0]);
                }
            }
        }
    L_a79:
        f5338 |= 0x20;
        f288 = 1.5f;
    } else if (mode_ == 6) {
        vector4 d;
        hit_chance(cur_pair(), this);
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            d.x = AG(e).pos.x - pos.x;
            d.y = AG(e).pos.y - pos.y;
            d.z = AG(e).pos.z - pos.z;
            near_ = d.normalize();
        }
        float cur;
        if (target >= 0) {
            d.x = AG(target).pos.x - pos.x;
            d.y = AG(target).pos.y - pos.y;
            d.z = AG(target).pos.z - pos.z;
            cur = d.normalize();
        } else {
            cur = 500.0f;
        }
        if (near_ < 8.0f && cur > near_ + near_)
            target = e;
        int r = rand() % 0x3dc9;
        float wsum[2];
        int c;
        if (((float)r * (1.0f / 15817.0f) < 0.2f || target == -1 ||
             g_mission->agents.get(target)->f14 != 1) &&
            (collect_enemies(wsum), nthreat > 0 && (c = (int)pick_threat()) >= 0)) {
            f2cc = c;
            target = c;
        }
        if (target == -1)
            target = f2cc;
        if (f2c == -1) {
            f2c0 = 0.0f;
            return;
        }
        if (AG(target).f14 != 1) {
            target = -1;
            f2c0 = 0.0f;
        } else {
            f533c = 0x22;
            int c3 = cr->a;
            if (c3 != 3) {
                if (c3 == 1) {
                    if (frand() < 0.2f) {
                        vector4 h;
                        hand_pos(&h);
                        float r2 = 2.0f;
                        set_goal_near2(&h, &r2);
                    }
                } else if (c3 == 2) {
                    if (frand() < 0.2f) {
                        vector4 h;
                        hand_pos(&h);
                        float r2 = 2.0f;
                        set_goal_near2(&h, &r2);
                    }
                } else {
                    if (frand() < 0.5f) {
                        Agent* ta = g_mission->agents.get(target);
                        vec2 v;
                        v.x = ta->pos.x - pos.x;
                        v.y = ta->pos.y - pos.y;
                        v.normalize();
                        vector4 gp;
                        float a = v.y * 13.0f;
                        float b = -v.x * 13.0f;
                        float cc = v.x * -20.0f;
                        float dd = v.y * -20.0f + ta->pos.y;
                        float r10 = 10.0f;
                        gp.x = (cc + ta->pos.x) + a;
                        gp.y = dd + b;
                        gp.z = pos.z + 100.0f;
                        set_goal_near(&gp, &r10);
                    }
                }
            }
        }
        f5338 |= 0x20;
        f288 = 1.5f;
    } else if (mode_ == 8) {
        flag_a = true;
        if (f2c == -1) {
            f2c0 = 0.0f;
            return;
        }
        if (cr->a != 3 &&
            (!(f5338 & 1) || g_mission->agents.get(f2c)->fac < 1.0f || (f27c != f280 && f27c >= 0))) {
            vector4 gp;
            const vector4* gpp;
            float rr;
            if (g_mission->agents.get(f2c)->fac > 2.0f) {
                gpp = &gp;
                rr = 15.0f;
                gp.z = dir.z * 25.0f;
                gp.x = dir.x * 25.0f + pos.x;
                gp.y = dir.y * 25.0f + pos.y;
                gp.z = gp.z + pos.z;
            } else {
                gpp = &pos;
                rr = 30.0f;
            }
            set_goal_near(gpp, &rr);
        }
        if (f5330 != 0) {
            float r50 = 50.0f;
            set_goal_near(&pos, &r50);
        }
    } else if (mode_ == 7) {
        if (f2c == -1) {
            f2c0 = 0.0f;
            return;
        }
        target = pick_shout_target(0x1026, 150.0f);
        if (target == -1)
            target = f2cc;
        unsigned u = target;
        Agent* ta = g_mission->agents.get(u);
        vec2 v;
        v.x = ta->pos.x - pos.x;
        v.y = ta->pos.y - pos.y;
        float dist = v.normalize();
        float dotv = v.x * dir.x + v.y * dir.y;
        if (((dist < 10.0f && dotv < 0.0f) &&
             (g_mission->agents.get(u)->f2c == -1 || frand() < 0.2f)) ||
            (dist < 2.0f && fac < 1.0f)) {
            mode = 8;
            f2c0 = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        } else if (weapon_mode == 3 && dotv < 0.3f && dist < 10.0f) {
            mode = 8;
            f2c0 = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        }
        f318 = target;
        set_goal_agent(target);
        if (cr->a != 3) {
            if ((fac > 2.0f && g_mission->agents.get(target)->f2c == -1 && dist < 11.0f) ||
                (f100_is_0x48_and_charged() && dotv > 0.2f)) {
                set_goal_agent(target);
                f2e4 = 0;
            } else if (f2e4 != 1 || frand() < 0.5f) {
                float s = dist + 10.0f;
                f2e4 = 1;
                float r30 = 30.0f;
                float m = fmin_ref(&s, &r30);
                float fr = frand();
                unsigned u2 = target;
                float k = ((fr + 2.0f) * m) * 0.2f;
                vector4 q, w, d, r4;
                Agent* t1 = g_mission->agents.get(u2);
                q.x = k * t1->vel.x;
                q.y = k * t1->vel.y;
                q.z = k * t1->vel.z;
                t1 = g_mission->agents.get(u2);
                w.x = q.x + t1->pos.x;
                w.y = q.y + t1->pos.y;
                w.z = q.z + t1->pos.z;
                t1 = g_mission->agents.get(u2);
                r4 = t1->vel;
                r4.normalize();
                float f = r4.y * v.y + r4.x * v.x;
                if (f < 0.0f) {
                    f = f + 1.0f;
                    d.x = q.x * f;
                    d.y = q.y * f;
                    d.z = q.z * f;
                    t1 = g_mission->agents.get(u2);
                    q.x = d.x + t1->pos.x;
                    q.y = d.y + t1->pos.y;
                    q.z = d.z + t1->pos.z;
                    w.w = q.w;
                    w.x = q.x;
                    w.y = q.y;
                    w.z = q.z;
                }
                float val = dist * 0.35f - 0.7f;
                float hi = 25.0f;
                float lo = 0.0f;
                float c = clamp(&val, &lo, &hi);
                set_goal_near(&w, &c);
            }
        }
        unsigned fl = f5338;
        f5338 = fl | 0x20;
        if (f5338 & 2)
            f5338 = fl | 0xa0;
        if (dist < 35.0f)
            f5338 |= 0x200;
        f533c = 2;
    } else if (mode_ == 9) {
        flag_b = true;
        vector4 q, d;
        hand_pos(&q);
        f2d0 = q;
        Pair sp = slot_pair(0);
        d.x = q.x - pos.x;
        d.y = q.y - pos.y;
        d.z = q.z - pos.z;
        float dist = d.normalize();
        float m = 1000.0f;
        bool b99 = false;
        if (sp.a < 0 || !g_items[sp.a].is_ranged()) {
            if (target == -1 || g_mission->agents.get(target)->f14 != 1 || frand() < 0.2f)
                target = pick_threat();
            b99 = true;
        } else if (target == -1 || g_mission->agents.get(target)->f14 != 1 || frand() < 0.2f) {
            target = pick_random();
        }
        int tg = target;
        if (tg >= 0) {
            d = g_mission->agents.get(tg)->pos - pos;
            m = d.normalize();
        }
        if (!b99 && tg >= 0 && dist < 7.0f && m < 4.0f) {
            set_goal_agent(tg);
        } else {
            float r2 = 0.2f;
            set_goal_near2(&f2d0, &r2);
            f5338 |= 0x20;
            if (f2c >= 0)
                f288 = 3.5f;
            else
                f288 = 2.5f;
        }
    } else if (mode_ == 0xb) {
        int p = g_mission->player_id;
        if (p >= 0) {
            Agent* pa = g_mission->agents.get(p);
            if (rgl::length(pos - pa->pos) < 1.2f) {
                target = p;
                f533c = 2;
                goto done;
            }
        }
        f533c = 0;
        target = -1;
    } else if (mode_ == 0xd) {
        f533c = 0;
        target = -1;
        f288 = 3.5f;
        if (frand() < 0.0005f || prev >= 0) {
            vector4 v;
            v.z = dir.z * 10.0f;
            float r15 = 15.0f;
            v.x = dir.x * 10.0f + pos.x;
            v.y = dir.y * 10.0f + pos.y;
            v.z = v.z + pos.z;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 0xe) {
        f533c = 0;
        target = -1;
        f288 = 3.0f;
        if (frand() < 0.3f) {
            vector4 v;
            v.z = dir.z * 20.0f;
            float r15 = 15.0f;
            v.x = dir.x * 20.0f + pos.x;
            v.y = dir.y * 20.0f + pos.y;
            v.z = v.z + pos.z;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 3) {
        reset_act();
        if (fac < 0.5f && f2c != -1)
            g_flags_5e2470 |= 0x10000;
    } else if (mode_ == 2) {
        if (f2c == -1) {
            int c = f2cc;
            if (c != -1 && g_mission->agents.get(c)->f14 == 1) {
                Agent* ca = g_mission->agents.get(c);
                vector4 d = ca->pos - pos;
                float l = d.out_length();
                vector4 w, q;
                if (l > 0.99f) {
                    flag_a = true;
                    Skel* b = ca->body;
                    if (l > 2.0f) {
                        float sz = b->s.z;
                        w.x = ca->pos.x - b->s.x;
                        w.y = ca->pos.y - b->s.y;
                        w.z = ca->pos.z;
                        f5338 = 0x21;
                        w.z = w.z - sz;
                    } else {
                        float sz = b->s.z * 0.2f;
                        w.x = ca->pos.x - b->s.x * 0.2f;
                        w.y = ca->pos.y - b->s.y * 0.2f;
                        w.z = ca->pos.z;
                        f5338 = 1;
                        w.z = w.z - sz;
                    }
                    q = w;
                    float r0 = 0.0f;
                    set_goal_near(&q, &r0);
                } else {
                    g_flags_5e2470 |= 0x20000;
                    flag_a = false;
                    target = c;
                    f533c = 2;
                }
                f288 = 0.15f;
            }
        } else if (f2c0 > 1.0f) {
            f2c0 = 1.0f;
        }
    }

done:
    if (g_mission->f6554 != 0 && id == g_player_agent) {
        mode = 0;
        flag_a = false;
        target = g_mission->player_id;
        f533c = 2;
    }
    if (mode == 0) {
        f5338 = 0;
        f5330 = 0;
        f532c = 0;
        f5324 = 0;
        f284 = 0;
        f533c = 0;
    } else if (mode == 0xc) {
        f5338 = 0;
        f5330 = 0;
        f532c = 0;
        f5324 = 0;
        f284 = 0;
        f533c = 0;
        target = -1;
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.3f) {
            target = pick_shout_target(0x26, 300.0f);
            if (target >= 0)
                f2c0 = 0.0f;
        }
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.6f) {
            g_flags_5e2470 = g_flags_5e2470 & 0xffffff;
            g_flags_5e2470 = g_flags_5e2470 | 0x1000000;
        }
    }

    Pair cp = cur_pair();
    if (flag_a && f10 != 1) {
        float range = 30.0f;
        if (cp.a >= 0 && g_items[cp.a].is_ranged())
            range = hit_chance(cp, this);
        int n = nthreat;
        float best = 0.0f;
        int bestid = -1;
        if (n > 0)
            bestid = threats[0].id;
        if (cp.a >= 0 && g_items[cp.a].is_ranged()) {
            bestid = pick_threat();
        } else if (n > 0) {
            for (int i = 0; i < nthreat; i++) {
                Threat* t = &threats[i];
                float score = -0.01f;
                Agent* ta = &AG((unsigned)t->id);
                if (ta->f14 == 1) {
                    vector4 d;
                    d.x = ta->pos.x - pos.x;
                    d.y = ta->pos.y - pos.y;
                    d.z = ta->pos.z - pos.z;
                    t->dir = d;
                    float len = t->dir.normalize();
                    t->dist = len;
                    if (len < range) {
                        Skel* b = body;
                        score = 10.0f / (len + 1.0f) +
                                (b->f.z * t->dir.z + b->f.y * t->dir.y + b->f.x * t->dir.x) * 0.1f;
                    } else {
                        if (f2c == -1) {
                            Pair p1 = slot_pair_inl(1);
                            if (p1.a >= 0) {
                                Pair p1b = slot_pair(1);
                                if ((char)g_items[p1b.a].type_flags == itp_type_bow) {
                                    Pair p0 = ta->slot_pair(0);
                                    if (p0.a >= 0) {
                                        ItemKind* ik = &g_items[p0.a];
                                        if (ik->is_ranged() && (float)ik->f1f4 * 2.1f > t->dist) {
                                            float inv = 1.0f / (t->dist + 10.0f);
                                            float one = 1.0f;
                                            score = fmin_ref(&one, &inv) * 2.5f;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (t->id == target)
                        score = score * 1.5f;
                    if (t->id == (int)prev)
                        score = score + score;
                }
                if (score > best) {
                    best = score;
                    bestid = t->id;
                }
            }
        }
        if (bestid < 0)
            target = -1;
        else
            target = bestid;
        f533c = (target == -1) ? 0 : 2;
    }
    if (flag_b && f10 == 0 && target >= 0) {
        vector4 ta_ = vadd(AG(target).vel, AG(target).pos);
        vector4 tb_ = vadd(vel, pos);
        vector4 d = vsub(ta_, tb_);
        float dist = d.normalize();
        float range = 3.0f;
        if (f2c >= 0 && g_world->troops[troop].nibble(0x17) < 2)
            range = 15.0f;
        if (dist > range && has_weapon_for(2)) {
            set_weapon_mode(2);
        } else if (f2c < 0 || !(vel.length() > 0.1f) ||
                   !has_weapon_for(3)) {
            if (has_weapon_for(1))
                set_weapon_mode(1);
        } else {
            set_weapon_mode(3);
        }
    }
    f310 = 0;
}
