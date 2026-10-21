// Slice s00437f00: Agent combat-stat helpers (weapon-class query, fatigue / draw-speed / damage factors, ammo stack
// reset, default order slot, 2-D signed angle). Class and field names are provisional; layouts from s004362e0,
// s00434ad0 and s004375b0.
#include <math.h>
#include "rgl.h"

// item kind record (0x238 bytes, array at 0x5e29d0)
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x1f4 - 0x1cc];
    int f1f4;                                   // +0x1f4
    char pad1f8[0x214 - 0x1f8];
    unsigned f214, f218;                        // +0x214, +0x218
    char pad21c[0x238 - 0x21c];
};
extern ItemKind* g_items;                       // 0x5e29d0

enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_pistol = 16,
    itp_type_musket, itp_type_bullets, itp_type_animal
};

// item index + packed amount word (s00434ad0 ItemRef)
struct Pair {
    int a;
    unsigned b;
    int stat_1e8();                             // 0x434df0
    int stat_1ec();                             // 0x434c50
    int stat_1f0a();                            // 0x434d70
    int stat_21x(int which);                    // 0x434cb0
    inline void reduce(int n);                  // 0x437510 (inlined here with n == 0)
};

inline void Pair::reduce(int n)
{
    if (a > 0) {
        unsigned t = g_items[a].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts || t == itp_type_pistol ||
            t == itp_type_musket || t == itp_type_crossbow || t == itp_type_bullets) {
            if (n <= 0) {
                ((unsigned char*)&b)[0] = 0;
                return;
            }
            unsigned v = (b & 0xff) - n;
            ((unsigned char*)&b)[0] = 0;
            b = b | (v & ((int)v < 0) - 1);
        } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
            unsigned v = stat_1e8();
            *(short*)&b = 0;
            b |= v;
        }
    }
}

// per-team, per-class record of the game options (stride 0x6c; teams 0x16c apart) and the game object
struct ClassRec { int a; int b; char pad[0x6c - 8]; };
struct TeamRec { ClassRec cls[3]; char pad[0x16c - 0x144]; };
struct Game {
    char pad0[0x5c];
    int f5c;                                    // +0x5c
    char pad60[0x528c - 0x60];
    TeamRec teams[8];                           // +0x528c
    char pad5dec[0x6604 - (0x528c + 8 * 0x16c)];
    int f6604;                                  // +0x6604
};
extern Game* g_game;                            // 0x5e292c
extern int g_agent_count;                       // 0x5aa050

// troop record (0x508 bytes); skill levels are queried through the world (0x437b20)
struct Troop {
    char pad0[0x30];
    unsigned __int64 flags;                     // +0x30
    char pad38[0x58 - 0x38];
    int lvl58;                                  // +0x58
    char pad5c[0x80 - 0x5c];
    float f80[8];                               // +0x80
    char pad_a0[0x508 - 0xa0];
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                              // +0x13a00 (stride 0x508)
    int skill_level(int agent, int skill);      // 0x437b20 (s004375b0)
};
extern World* g_world;                          // 0x5e2930

struct OrderSlot {
    int a;                                      // +0
    int b;                                      // +4
    char pad8[0x14 - 8];
    unsigned m14;
    char pad18[0x20 - 0x18];
    float rate;                                 // +0x20
    char pad24[0x2c - 0x24];
    bool set(int id, unsigned prio);            // 0x4378a0
};
struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; };
struct DefList { TroopDef* defs; int count; };
struct Curve { void set_rec_phase(int i, float p); };   // 0x433b20
struct CurveOwner { char pad[0x304]; Curve* curve; };
struct SlotLink { char pad[0xcc]; int m_cc; };

struct vec2 { float x, y; };

struct Agent {
    char pad0[8];
    int side;                                   // +8
    char padc[0x10 - 0xc];
    int m10;                                    // +0x10
    char pad14[0x2c - 0x14];
    int m2c;                                    // +0x2c (-1 = on foot)
    char pad30[0x88 - 0x30];
    int troop;                                  // +0x88
    char pad8c[0xd4 - 0x8c];
    OrderSlot oslots[2];                        // +0xd4
    DefList* defs;                              // +0x12c
    char pad130[0x14c - 0x130];
    SlotLink* links[4];                         // +0x14c
    char pad15c[0x1c4 - 0x15c];
    int team;                                   // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    Pair held[11];                              // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c (slots[0] = current held item, slots[1] at +0x230, slots[4] at +0x23c)
    char pad240[0x24c - 0x240];
    float f24c;                                 // +0x24c
    CurveOwner* owner;                          // +0x250
    char pad254[0x561c - 0x254];
    int f561c;                                  // +0x561c

    __declspec(noinline) Pair slot_pair(int idx) const;   // 0x432790 (s00432130)
    inline Pair slot_pair_inl(int idx) const;
    inline Pair cur_pair() const;
    Pair best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880 (s004362e0)
    bool has_weapon_for(int mode);              // 0x437f00
    float stamina_scale();                      // 0x438180
    void set_default_order();                   // 0x4382e0
    inline void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0 (s004375b0), inlined here
    float attack_speed();                       // 0x438360
    void clear_ammo(int unused);                // 0x438520
    float damage_factor(int idx);               // 0x4385e0
};

inline Pair Agent::slot_pair_inl(int idx) const
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
inline Pair Agent::cur_pair() const
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

// @ 0x437f00
bool Agent::has_weapon_for(int mode)
{
    bool found = false;
    int t1 = team;
    if (t1 < 0 || t1 >= 8) t1 = 7;
    int tr = troop;
    int k1 = 0;
    if (tr >= 0) {
        if (m2c >= 0)
            k1 = 2;
        else
            k1 = (g_world->troops[tr].flags & 0x4000000) ? 1 : 0;
    }
    int ra = g_game->teams[t1].cls[k1].a;
    int t2 = team;
    if (t2 < 0 || t2 >= 8) t2 = 7;
    int k2 = 0;
    if (tr >= 0) {
        if (m2c >= 0)
            k2 = 2;
        else
            k2 = (g_world->troops[tr].flags & 0x4000000) ? 1 : 0;
    }
    int rb = g_game->teams[t2].cls[k2].b;
    for (int i = 0; i < 4; i++) {
        if (found) return found;
        Pair* p = &held[i];
        if (p->a >= 0) {
            ItemKind* ik = &g_items[p->a];
            unsigned fl = ik->type_flags;
            unsigned type = fl & 0xff;
            bool ok = ((ik->f218 & 0x300) == 0x200 || p->stat_21x(0) <= 1) &&
                      ((ik->f214 & 0x300) == 0x200 || p->stat_21x(1) <= 1);
            if ((ra == 0 || ok) &&
                (rb == 0 || (type != 8 && type != 9 && type != 10 && type != 0x10 && type != 0x11))) {
                if (mode == 1) {
                    if ((type == 2 || type == 3 || type == 4) && (fl & 0x400000))
                        found = true;
                } else if (mode == 2) {
                    if (type == 8 || type == 9 || type == 10 || type == 0x10 || type == 0x11) {
                        int missing, total, slot_out;
                        best_ammo(i, &missing, &total, &slot_out);
                        if (missing > 0) {
                            if (type != 9 || !(ik->type_flags & 0x100000) || m2c == -1)
                                found = true;
                        }
                    }
                } else if (mode == 3) {
                    if (type == 4)
                        found = true;
                }
            }
        }
    }
    return found;
}

// @ 0x438180
float Agent::stamina_scale()
{
    if (m10 != 1) {
        float r = 1.0f - f24c * 0.11f / (g_world->troops[troop].lvl58 + 9);
        if (0.0f > r) return 0.0f;
        return r;
    }
    return 1.0f;
}

// @ 0x4381e0
float signed_angle(const vec2* a, const vec2* b)
{
    vec2 u = *a;
    float l = (float)sqrt(u.x * u.x + u.y * u.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        u.x = inv * u.x;
        u.y = inv * u.y;
    } else {
        u.x = 0.0f;
        u.y = 1.0f;
    }
    vec2 bb = *b;
    vec2 r;
    r.x = bb.x * u.y - bb.y * u.x;
    r.y = bb.y * u.y + bb.x * u.x;
    float l2 = (float)sqrt(r.y * r.y + r.x * r.x);
    if (l2 > 0.0f) {
        float inv = 1.0f / l2;
        r.x = inv * r.x;
        r.y = inv * r.y;
    } else {
        r.x = 0.0f;
        r.y = 1.0f;
    }
    return (float)atan2(-r.x, r.y);
}

inline void Agent::set_order(int slot, int id, unsigned prio, float rate)
{
    if (id >= -1 && id < defs->count) {
        if (oslots[slot].set(id, prio)) {
            oslots[slot].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
            if (prio & 0x4000000)
                owner->curve->set_rec_phase(slot, 0.0f);
            if (slot == 0) {
                if (id >= 0 && (defs->defs[id].flags & 0x400) && oslots[1].a != -1 &&
                    (prio & 0x1ffff) > (oslots[1].m14 & 0x1ffff))
                    oslots[1].set(-1, prio & 0x1ffff);
            } else if (slot == 1) {
                int k = slots[0];
                if (k >= 0 && held[k].a >= 0) {
                    if (prio & 0x10000000)
                        links[k]->m_cc = 0;
                    else
                        links[k]->m_cc = 0xffff;
                }
            }
        }
    }
}

// @ 0x4382e0
void Agent::set_default_order()
{
    set_order(1, f561c, 0x500000f, 1.0f);
}

// @ 0x438360
float Agent::attack_speed()
{
    float f = 1.0f;
    float base = 10.0f;
    Pair cur = cur_pair();
    if (cur.a >= 0) {
        ItemKind* ik = &g_items[cur.a];
        base = (float)ik->f1f4;
        unsigned type = ik->type_flags & 0xff;
        if (type == 8) {
            int lvl = g_world->skill_level(troop, 0x21);
            int v = cur.stat_1ec() + 4;
            if (v >= lvl) v = lvl;
            f = (float)v * 0.12f + 1.0f;
            if (m2c != -1)
                f = ((float)g_world->skill_level(troop, 0x17) * 0.019 + 0.8) * f;
            if (g_game->f6604 == 1)
                return (float)sqrt(f * 0.9f) * base * 1.2f;
        } else if (type == 10) {
            f = (float)g_world->skill_level(troop, 0x22) * 0.1f + 1.0f;
            if (m2c != -1)
                return (float)sqrt(((float)g_world->skill_level(troop, 0x17) * 0.019 + 0.8) * f) * base * 1.2f;
        } else if (type == 9) {
            if (g_game->f6604 == 1) f = 0.75f;
        }
    }
    return (float)sqrt(f) * base * 1.2f;
}

// @ 0x438520
void Agent::clear_ammo(int unused)
{
    for (int i = 0; i < 4; i++) {
        Pair* p = &held[i];
        if (held[i].a >= 0) {
            unsigned t = g_items[held[i].a].type_flags & 0xff;
            if (t == itp_type_arrows || t == itp_type_bolts || t == itp_type_bullets || t == itp_type_thrown) {
                if (p->a > 0)
                    p->reduce(0);
            }
        }
    }
}

// The slot-class lookup 0x4350c0 uses cl's register convention for same-TU statics (item in EAX, flag on the
// stack): reproduced by defining it here as a static (copy of the body from s00434ad0).
// @ 0x4350c0
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == itp_type_bow) return 3;
    if (t == itp_type_crossbow) return 4;
    if (t == itp_type_thrown) return 5;
    if (t == itp_type_pistol || t == itp_type_musket) return 6;
    if (t == itp_type_polearm) return 2;
    if (t == itp_type_one_handed_wpn) return 0;
    return flag == 0;
}

// @ 0x4385e0
float Agent::damage_factor(int idx)
{
    float r = 1.0f;
    Pair sel = slot_pair(idx);
    int item = sel.a;
    if (item >= 0) {
        r = (float)sel.stat_1f0a() * 0.01f;
        unsigned flags = g_items[item].type_flags;
        unsigned type = flags & 0xff;
        ItemKind* ik = &g_items[item];

        if (type == 2 || type == 3 || type == 4 || type == 8 || type == 9 || type == 10 || type == 0x10 ||
            type == 0x11) {
            Pair o = slot_pair_inl(1);
            int cls = slot_class_impl(item, o.a >= 0);
            int v = (int)g_world->troops[troop].f80[cls + 0];
            float frac = 0.07f;
            if ((char)ik->type_flags == 8) frac = 0.11f;
            float vv = (float)v * 0.01f;
            r = ((1.0f - frac) + vv * frac) * r;
        }
        if (type == 3 || type == 4) {
            Pair o = slot_pair_inl(1);
            if (o.a >= 0 || m2c >= 0) {
                r = r - 0.15f;
                if (flags & 0x200000) r = r - 0.05f;
            }
        } else if (type == 7) {
            int lvl = g_world->skill_level(troop, 0x1a);
            r = ((float)lvl * 0.25f + 1.0f) * r;
            if (side == g_game->f5c) {
                if (g_agent_count > 1)
                    r = (((float)g_agent_count * 0.7f + 9.3f) * r) * 0.07f;
            } else {
                r = ((float)g_agent_count * 0.07f + 0.93f) * r;
            }
        }
        unsigned t2 = ik->type_flags & 0xff;
        if ((t2 == 2 || t2 == 3 || t2 == 4) && (side != g_game->f5c || g_agent_count > 1))
            r = ((float)g_agent_count * 0.07f + 0.93f) * r;
    }
    return r;
}

// @ 0x432790 (same body as in s00432130; defined here so cl knows it leaves the x87 stack alone, which lets
// damage_factor keep its 1.0 result on the FPU stack across the call)
Pair Agent::slot_pair(int idx) const
{
    const Pair* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &held[i];
    Pair r;
    r.a = -1;
    if (p) r = *p;
    return r;
}
