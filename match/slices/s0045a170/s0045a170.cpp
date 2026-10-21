// Slice s0045a170: Agent::swing_trace (0x45a170, 8361 bytes, name provisional): the per-tick melee-swing / missile
// hit test of an agent. It builds the weapon's swept capsule from the hand frames (or the missile's segment),
// collects nearby agents, tests each with the agent's own skeleton capsules (shield block with sound, friendly-fire
// message, damage record via hit_damage / reduce_blow), and when no agent was hit applies the blow to a scene
// prop the probe touched (prop hit points, hit frame, sounds, embedded-missile mesh).
// All names are provisional; every offset comes from this function and the neighbours' verified layouts
// (s00453710 hit_damage, s0043f6d0 capsule_hit, s00454e30 Blow / reduce_blow, s004465e0 AgentTable).
#include <stdlib.h>
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
};
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;   // 0x43a1e0
};
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog {
    void add(tstring msg, int color);               // 0x452c00 (thiscall, ret 0x14)
};
extern MsgLog g_msglog;                             // 0x5d7080

// ---- shapes ----------------------------------------------------------------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;     // mc = scene entity id
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}
};
// vtable 0x54a540 (rgl::capsule, 0x64 bytes); default ctor inlined, copy ctor at 0x40a580
struct capsule : shape_base {
    vector4 v20, v30, v40, v50;
    float radius;
    capsule() {}
    capsule(const capsule& o);                       // 0x40a580
    virtual ~capsule() {}
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void scale(const float* s);
    virtual void transform(const matrix& m);
};
struct contact { vector4 a; vector4 point; int extra; };          // 0x24 bytes
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);    // 0x40bfe0

// ---- items -----------------------------------------------------------------------------------------------
enum { itp_type_shield = 7 };
struct ItemRef {
    int item;
    unsigned m4;
};
struct ItemKind {                                   // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                            // +0x1c8
    char pad1cc[4];
    unsigned cap_lo;                                // +0x1d0 (low dword of itcf_*)
    char pad1d4[0x20c - 0x1d4];
    int f20c;                                       // +0x20c
    char pad210[4];
    int f214;                                       // +0x214
    int f218;                                       // +0x218
    char pad21c[0x238 - 0x21c];
};
extern ItemKind* g_items;                           // 0x5e29d0

// ---- scene props -----------------------------------------------------------------------------------------
struct SoundSet {
    void play(int id);                              // 0x48add0
};
struct PropKind {                                   // 0x78 bytes (array at 0x5e29dc)
    char pad0[0x10];
    unsigned flags;                                 // +0x10
    char pad14[0x6c - 0x14];
    SoundSet snd;                                   // +0x6c
    char pad6c[0x78 - 0x6c - 1];
};
extern PropKind* g_propkinds;                       // 0x5e29dc
struct PropRec {                                    // 0x1c8 bytes (mission +0x668c)
    char pad0[0x18];
    matrix m;                                       // +0x18
    char pad58[0x188 - 0x58];
    int destroyed;                                  // +0x188
    int kind;                                       // +0x18c
    char pad190[0x1c0 - 0x190];
    float hp;                                       // +0x1c0
    char pad1c4[0x1c8 - 0x1c4];
};
struct Vec1c8 {
    int pad; char* first; char* last;
    int size() const;                               // 0x4359e0
};

struct mat3 { vector4 s, f, u; };                   // the three rotation rows of a matrix
struct mesh { char pad[0xcc]; matrix frame; };      // +0xcc: placement frame
struct pos : matrix { matrix relative(const matrix& base) const; };   // 0x4329d0 (hidden return pointer + base)
struct StratEnt {                                   // scene entity
    char pad0[0xc];
    int f0c;                                        // +0xc
    int f10;                                        // +0x10
    char pad14[0x58 - 0x14];
    pos m;                                          // +0x58
    void add_mesh(mesh* m);                         // 0x442110
};
struct Ctx {
    StratEnt* find_entity(int id);                  // 0x42ba50
};

struct GameData {
    char pad0[0x1a1f0];
    int f1a1f0;                                     // +0x1a1f0
    void set_matrix(const matrix* m);               // 0x435650 (Placement)
};
extern GameData* g_game;                            // 0x5e2930

// ---- skeleton / body -------------------------------------------------------------------------------------
struct Z_ { char pad[0xd8]; float fd8; };
struct Skeleton { char pad0[0x40]; Z_* f40; };
struct AgentBody { char pad0[0x304]; Skeleton* skel; };

// ---- the mission -----------------------------------------------------------------------------------------
struct Agent;
struct Timer {
    int clock; int pad4; __int64 start;
    float elapsed() const;                          // 0x432c10
};
struct AgentTable {
    int count;                                      // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                      // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                    // 0x43b160
    int first_used();                               // 0x43b1b0
    Agent* get(unsigned id);                        // 0x43b1f0
};
struct Mission {
    Ctx* scene;                                     // +0
    AgentTable agents;                              // +4
    char pad28[0x5c - 0x28];
    int local_id;                                   // +0x5c
    char pad60[0x53c8 - 0x60];
    int rel[8 * 0x5b];                              // +0x53c8: relation of team a (row stride 0x5b) to team b: 1 friend, -1 enemy
    char pad_after[0x6688 - 0x53c8 - 8 * 0x5b * 4];
    Vec1c8 props;                                   // +0x6688 (first at +0x668c)
};
extern Mission* g_mission;                          // 0x5e292c

extern __int64 g_last_hit_prop;                     // 0xa70448
extern __int64 g_last_hit_damage;                   // 0xa70450
extern matrix g_last_hit_frame;                     // 0xa708d0

// ---- missile record (param 3) ---------------------------------------------------------------------------
struct mesh;
struct Missile {
    char pad0[0x40];
    vector4 dir;                                    // +0x40
    struct Owner { char pad[0x88]; vector4 hit; } *owner;   // +0x50: shooter record, +0x88 = last hit point
    char pad54[0x80 - 0x54];
    int item;                                       // +0x80
    unsigned item_mod;                              // +0x84
    char pad88[0x90 - 0x88];
    char spent;                                     // +0x90
    void make_capsule(capsule* out);                // 0x46b410 (ret 4)
    mesh* make_ranged_mesh();                       // 0x44f6a0
};

// blow record written for the victim (0x58 bytes; see s00454e30 Blow)
struct Blow {
    unsigned flags;                                 // +0
    unsigned attacker;                              // +4
    vector4 pos;                                    // +8
    vector4 dir;                                    // +0x18
    vector4 dir2;                                   // +0x28
    int bone;                                       // +0x38
    float dmg_in;                                   // +0x3c
    float dmg_out;                                  // +0x40
    int absorbed;                                   // +0x44
    int kind;                                       // +0x48
    int item;                                       // +0x4c
    unsigned item_mod;                              // +0x50
    Missile* missile;                               // +0x54
};

struct Agent {
    char pad0[8];
    int id;                                         // +8
    char padc[4];
    int f10;                                        // +0x10 (0 = human, 1 = horse)
    char pad14[0x2c - 0x14];
    int mount;                                      // +0x2c
    int rider;                                      // +0x30
    char pad34[0x50 - 0x34];
    vector4 pos;                                    // +0x50
    vector4 dir;                                    // +0x60
    char pad70[0x100 - 0x70];
    int action;                                     // +0x100
    char pad104[0x1c4 - 0x104];
    int team;                                       // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef held[10];                               // +0x1d0
    char pad220[0x22c - 0x220];
    int slots[5];                                   // +0x22c
    char pad240[0x250 - 0x240];
    AgentBody* body;                                // +0x250
    char pad254[0x55f8 - 0x254];
    int hand_a;                                     // +0x55f8
    char pad55fc[0x5638 - 0x55fc];
    int has_blow;                                   // +0x5638
    Blow last_blow;                                 // +0x563c
    char pad5694[0x5734 - 0x5694];
    matrix fr1;                                     // +0x5734 (swing frame, current)
    matrix fr2;                                     // +0x5774 (swing frame, previous; o.x < -1e7 = none)
    char pad57b4[0x57bc - 0x57b4];
    char hit_once;                                  // +0x57bc
    char prop_done;                                 // +0x57bd
    char pad57be[0x57e0 - 0x57be];
    Timer t57e0;                                    // +0x57e0
    char pad57f0[0x5830 - 0x57f0];
    int state;                                      // +0x5830
    char pad5834[0x5928 - 0x5834];

    ItemRef slot_pair(int idx) const;               // 0x432790 (thiscall, ret 8)
    __forceinline ItemRef slot_pair0() const
    {
        const ItemRef* p = 0;
        int i = slots[0];
        if (i >= 0) p = &held[i];
        ItemRef r;
        r.item = -1;
        if (p) { r.item = p->item; r.m4 = p->m4; }
        return r;
    }
    __forceinline ItemRef slot_pair1() const
    {
        const ItemRef* p = 0;
        int i = slots[1];
        if (i >= 0 && slots[4] == 0) p = &held[i];
        ItemRef r;
        r.item = -1;
        if (p) { r.item = p->item; r.m4 = p->m4; }
        return r;
    }
    __forceinline bool couching() const
    {
        if (action != 0x48) return false;
        return body->skel->f40->fd8 > 0.1f;
    }
    bool charged_0x48() const;                      // 0x432470
    bool is_enemy(const Agent* o) const;            // 0x432350
    bool is_friend(const Agent* o) const;           // 0x4323a0
    bool is_neutral(const Agent* o) const;          // 0x4323f0
    int probe_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag);   // 0x43a720 (ret 0x10)
    void set_default_order();                       // 0x4382e0
    int can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target);   // 0x4406a0 (ret 0x14)
    void play_sound_var(int* kind, float* var);     // 0x443730
    void absorb_missile(Missile* m);                // 0x452650 (ret 4)
    void build_shape(capsule* out);                 // 0x453050 (ret 4)
    void shield_hit(int damage);                    // 0x4532f0
    void hit_damage(int hand, Missile* missile, float* out_dmg, unsigned* out_flag, unsigned victim, float* scale);   // 0x453710
    void reduce_blow(Blow* b);                      // 0x454e30
    bool capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone);   // 0x43f6d0

    int swing_trace(int hand, Missile* missile, Blow* out, float* scale);   // 0x45a170
};

int __cdecl round_float(const float* p);            // 0x432540
static __forceinline int round_inl(const float& x)
{
    float fl = (float)floor((double)x);
    int i = (int)fl;
    return (x - fl <= 0.5f) ? i : i + 1;
}

static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents.pages[id >> 4][id & 0xf];
}

// team relation lookup (mission +0x53c8): 1 friend, -1 enemy; false when either team is out of range
static __forceinline bool rel_is(int a, int b, int v, int* rel)
{
    if (a < 0 || a >= 8 || b < 0 || b >= 8)
        return false;
    *rel = g_mission->rel[a * 0x5b + b];
    return *rel == v;
}
#define SND(who, n) { int sid_ = (n); float var_ = 0.1f; (who)->play_sound_var(&sid_, &var_); }
static __forceinline bool is_friend_rel(int r) { return r == 1; }
static __forceinline float vlen(const vector4& v) { return (float)sqrt(v.y * v.y + (v.z * v.z + v.x * v.x)); }
static __forceinline vector4 vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }
static __forceinline vector4 smul(const float& k, const vector4& v) { return vector4(k * v.x, k * v.y, k * v.z); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x45a170
int Agent::swing_trace(int hand, Missile* missile, Blow* out, float* scale)
{
    capsule sweep;
    ItemRef sp = slot_pair(hand);
    float reach = 0.2f;
    float inner = 0.0f;
    vector4 tA, bA, tB, bB, dT, dB, bBs;
    int steps;
    float len;
    if (missile == 0) {
        if (fr2.o.x < -1e7f)
            return 0;
        if (sp.item >= 0) {
            reach = (float)g_items[sp.item].f20c * 0.01f - 0.01f;
        } else {
            reach = 0.15f;
            inner = 0.0f;
        }
        if (hand_a == 0) {
            float t = reach - 1.0f;
            inner = (0.0f > t) ? 0.0f : t;
        }
        tA = vadd(smul(reach, fr1.f), fr1.o);
        bA = vadd(smul(inner, fr1.f), fr1.o);
        tB = vadd(smul(reach, fr2.f), fr2.o);
        bB = vadd(smul(inner, fr2.f), fr2.o);
        bBs = bB;
        dT = vsub(tA, tB);
        dB = vsub(bA, bB);
        steps = round_inl((len = vlen(dT)) * 20.0f) + 1;
        sweep.v20 = bB;
        sweep.v30 = tA;
        sweep.radius = 0.1f;
    } else {
        missile->make_capsule(&sweep);
        steps = 1;
    }

    int result = 0;
    bool ragdoll = false;
    shape_base* found = 0;
    int probed = 0;
    char can;
    if (!prop_done) {
        capsule pc(sweep);
        pc.radius = 0.0f;
        if (missile == 0 && hand_a != 0) {
            can = 1;
            probed = probe_contact(&pc, &bB, &found, &can);
        } else {
            can = 0;
            probed = probe_contact(&pc, &bB, &found, &can);
        }
    }
    int ncand = 0;
    float best_d2 = 10000.0f;
    int best = -1;
    can = 0;
    bool hit_flag = false;
    float maxdist = 1000.0f;
    if (probed) {
        vector4 d = vsub(bB, sweep.v20);
        maxdist = (float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z));
    }
    unsigned cand[0x400];
    unsigned i;
    if (!prop_done && (missile != 0 || probed == 0)) {
        vector4 sd = vsub(sweep.v20, sweep.v30);
        float limit2 = (sd.x * sd.x + (sd.y * sd.y + sd.z * sd.z)) + 50.0f;
        unsigned end = g_mission->agents.count;
        for (i = g_mission->agents.first_used(); i < end; g_mission->agents.next_used(&i)) {
            if (ncand >= 0x400)
                break;
            if (i != (unsigned)id && i != (unsigned)mount) {
                Agent* o = agent_by_id(i);
                bool ok = o->state == 2;
                if (!ok && is_friend(o) && agent_by_id(i)->state == 3 &&
                    g_mission->agents.get(i)->t57e0.elapsed() < 2.0f)
                    ok = true;
                if (ok) {
                    vector4 dd = vsub(o->pos, sweep.v20);
                    float d2 = dd.x * dd.x + (dd.y * dd.y + dd.z * dd.z);
                    if (d2 < limit2) {
                        cand[ncand] = i;
                        if (d2 < best_d2) {
                            best = ncand;
                            ncand++;
                            best_d2 = d2;
                        } else {
                            ncand++;
                        }
                    }
                }
            }
        }
        if (best >= 0) {
            unsigned t = cand[0];
            cand[0] = cand[best];
            cand[best] = t;
        }
        for (i = 0; (int)i < ncand; i++) {
            Agent* o = agent_by_id(cand[i]);
            bool hit = false;
            if (o->f10 == 0) {
                capsule oc;
                o->build_shape(&oc);
                contact ct;
                vector4 hitpt;
                if (missile != 0) {
                    hit = shape_contact_count(&sweep, &oc, 1, &ct) != 0;
                    hitpt = ct.point;
                } else {
                    sweep.v20 = bBs;
                    sweep.v30 = tA;
                    sweep.radius = 0.9f;
                    if (shape_contact_count(&oc, &sweep, 0, 0) != 0) {
                        sweep.radius = 0.1f;
                        int n = round_inl(len * 10.0f) + 1;
                        for (int j = 0; j < n && !hit; j++) {
                            float t = (float)j / (float)n;
                            vector4 T = vadd(smul(t, dT), tB);
                            vector4 Bp = vadd(smul(t, dB), bBs);
                            vector4 d = vsub(T, Bp);
                            d.normalize();
                            float k = (reach - inner) - 0.15f;
                            sweep.v20 = Bp;
                            sweep.v30 = vadd(smul(k, d), Bp);
                            hit = shape_contact_count(&oc, &sweep, 1, &ct) != 0;
                            hitpt = ct.point;
                        }
                    }
                }
                if (hit) {
                    int rel;
                    if (!rel_is(team, o->team, -1, &rel)) {
                        if (!rel_is(team, o->team, 1, &rel))
                            return 2;
                    }
                    bool fr = rel == 1;
                    if (fr) {
                        if (missile == 0) {
                            vector4 dv(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                            float dl = dv.normalize();
                            if (mount == -1 && o->mount == -1 &&
                                0.8f < dv.x * dir.x + (dv.y * dir.y + dv.z * dir.z) && dl < 1.3f)
                                return 2;
                            goto next;
                        }
                        if (id != g_mission->local_id)
                            return 2;
                    }
                    ItemRef cur = slot_pair0();
                    sp = cur;
                    int twohand = 0;
                    if (cur.item >= 0 && (g_items[cur.item].type_flags & 0x20000) != 0)
                        twohand = 1;
                    can = can_hit(id, cand[i], hand, (int)missile, &hitpt.x) != 0;
                    if (can) {
                        ItemRef os = o->slot_pair1();
                        if (os.item >= 0 && (g_items[os.item].type_flags & 0xff) == itp_type_shield) {
                            float dmg;
                            unsigned fl2[2];
                            hit_damage(hand, missile, &dmg, fl2, o->id, scale);
                            ItemRef ms = slot_pair0();
                            unsigned wf = ms.item < 0 ? 0 : g_items[ms.item].type_flags;
                            if ((wf & 0x4000000) != 0)
                                dmg = dmg + dmg;
                            if (charged_0x48())
                                dmg = dmg + dmg;
                            if (missile != 0 && (g_items[missile->item].cap_lo & 0xff000) != 0x10000)
                                o->absorb_missile(missile);
                            o->shield_hit((int)dmg);
                            if ((g_items[os.item].type_flags & 0x40000) != 0) {
                                if (twohand) { SND(o, 0x71); } else { SND(o, 0x73); }
                            } else {
                                if (twohand) { SND(o, 0x74); } else { SND(o, 0x72); }
                            }
                        } else {
                            ItemRef os0 = o->slot_pair0();
                            if (sp.item == -1) {
                                SND(this, 0x6d);
                            } else if (os0.item >= 0 && (g_items[os0.item].type_flags & 0x40000) != 0) {
                                if (twohand) { SND(this, 0x6e); } else { SND(this, 0x70); }
                            } else {
                                if (twohand) { SND(this, 0x70); } else { SND(this, 0x6f); }
                            }
                        }
                        o->set_default_order();
                        result = 2;
                    } else {
                        sweep.radius = 0.1f;
                        for (int j = 0; j < steps && result == 0; j++) {
                            if (missile == 0 && hand_a != 0) {
                                float t = (float)j / (float)steps;
                                vector4 T = vadd(smul(t, dT), tB);
                                vector4 Bp = vadd(smul(t, dB), bBs);
                                vector4 d = vsub(T, Bp);
                                d.normalize();
                                float k = reach - inner;
                                sweep.v20 = Bp;
                                sweep.v30 = vadd(smul(k, d), Bp);
                            }
                            vector4 hp;
                            vector4 nrm;
                            int bone;
                            result = o->capsule_hit(&sweep, &hp, &nrm, &bone) & 0xff;
                            vector4 hv = vsub(hp, sweep.v20);
                            if (result != 0 && vlen(hv) < maxdist) {
                                hit_flag = true;
                                if (id == g_mission->local_id && is_friend(o)) {
                                    g_msglog.add(*g_strtable.lookup(tstring("ui_you_hit_a_friendly_troop")),
                                                 0xffff0000);
                                }
                                out->flags = 0;
                                out->attacker = id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->id, scale);
                                if (missile != 0) {
                                    out->dir = missile->dir;
                                    out->dir2 = missile->dir;
                                    out->dir2.normalize();
                                    out->item = missile->item;
                                    out->item_mod = missile->item_mod;
                                    out->kind = (g_items[missile->item].f214 >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    out->dir = vsub(o->pos, pos);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].f218 >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].f214 >> 8) & 3;
                                    out->dir2 = dT;
                                    out->dir2.normalize();
                                    out->dir.normalize();
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                    if (couching())
                                        out->flags |= 0x10;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->last_blow.dmg_out)) {
                                    o->last_blow = *out;
                                    o->has_blow = 1;
                                }
                                result = 1;
                            }
                        }
                    }
                }
            } else {
                char skip = missile == 0 ? hit_once : missile->spent;
                if (skip == 0) {
                    int rd = o->rider;
                    if (rd < 0 || !is_neutral(agent_by_id(rd))) {
                        if (missile != 0 || (rd >= 0 && is_enemy(agent_by_id(rd)))) {
                            bool h = false;
                            vector4 hp;
                            vector4 nrm;
                            int bone;
                            if (missile != 0) {
                                h = o->capsule_hit(&sweep, &hp, &nrm, &bone);
                            } else {
                                float l6 = len * 6.6666665f;
                                sweep.radius = 0.1f;
                                int n = round_float(&l6) + 1;
                                for (int j = 0; j < n && !h; j++) {
                                    float t = (float)j / (float)n;
                                    vector4 T = vadd(smul(t, dT), tB);
                                    vector4 Bp = vadd(smul(t, dB), bBs);
                                    vector4 d = vsub(T, Bp);
                                    d.normalize();
                                    float k = reach - inner;
                                    sweep.v20 = Bp;
                                    sweep.v30 = vadd(smul(k, d), Bp);
                                    h = o->capsule_hit(&sweep, &hp, &nrm, &bone);
                                }
                            }
                            if (h) {
                                out->flags = 0;
                                out->attacker = id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->id, scale);
                                if (missile != 0) {
                                    out->dir = missile->dir;
                                    out->dir2 = missile->dir;
                                    out->item = missile->item;
                                    out->item_mod = missile->item_mod;
                                    out->kind = (g_items[missile->item].f214 >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].f218 >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].f214 >> 8) & 3;
                                    out->dir = nrm;
                                    out->dir2 = nrm;
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->last_blow.dmg_out)) {
                                    o->last_blow = *out;
                                    o->has_blow = 1;
                                }
                                if (missile != 0) {
                                    missile->spent = 1;
                                    result = 1;
                                } else {
                                    hit_once = 1;
                                    int r = rand() % 0x3dc9;
                                    *scale = ((float)r * 1.2644623e-05f + 0.4f) * *scale;
                                }
                            }
                        }
                    }
                }
            }
        next:
            if (result != 0)
                break;
        }
    }

    if (!prop_done && !hit_flag && !can && probed != 0) {
        int eid = found->mc;
        if (eid >= 0) {
            StratEnt* e = g_mission->scene->find_entity(eid);
            if (e != 0) {
                int pi = e->f0c;
                e->f10 = 0xc;
                if (pi >= 0 && (unsigned)pi < (unsigned)g_mission->props.size()) {
                    PropRec* pr = &((PropRec*)g_mission->props.first)[pi];
                    prop_done = 1;
                    if (pr->destroyed == 0) {
                        unsigned kf = g_propkinds[pr->kind].flags & 0x8000;
                        if ((kf != 0 && 0.0f < pr->hp) || kf == 0) {
                            float dmg = 0.0f;
                            int flg = 0;
                            if (kf != 0 && 0.0f < pr->hp) {
                                hit_damage(0, missile, &dmg, (unsigned*)&flg, 0xffffffff, scale);
                                pr->hp = pr->hp - dmg;
                            }
                            PropKind* kind = &g_propkinds[pr->kind];
                            g_game->set_matrix(&pr->m);
                            g_game->f1a1f0 = pi;
                            g_last_hit_prop = pi;
                            g_last_hit_damage = (__int64)dmg;
                            matrix frame;
                            frame.o = bB;
                            if (missile != 0) {
                                frame.f = missile->dir;
                                frame.f.normalize();
                                frame.u = vector4(0.0f, 0.0f, 1.0f);
                                frame.orthonormalize();
                                if ((kind->flags & 0x18000) != 0) {
                                    mesh* m = missile->make_ranged_mesh();
                                    if (m != 0) {
                                        vector4 o2 = vsub(frame.o, vmul(frame.f, 0.2f));
                                        matrix fm2;
                                        *(mat3*)&fm2 = *(mat3*)&frame;
                                        fm2.o = o2;
                                        m->frame = e->m.relative(fm2);
                                        e->add_mesh(m);
                                        ragdoll = true;
                                    }
                                }
                            } else {
                                *(mat3*)&frame = *(const mat3*)&fr1;
                            }
                            g_last_hit_frame = frame;
                            kind->snd.play(-0x2a);
                            if ((kind->flags & 0x8000) != 0 && pr->hp <= 0.0f)
                                kind->snd.play(-0x2b);
                        }
                    }
                }
            }
        }
        if (missile != 0) {
            if (ragdoll)
                return 2;
            missile->owner->hit = bB;
            result = 3;
        }
    }
    return result;
}
