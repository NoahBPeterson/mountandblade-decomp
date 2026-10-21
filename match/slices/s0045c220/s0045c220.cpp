// Slice s0045c220: Agent::update_attack (0x45c220, 5.6 KB), the per-tick attack / reload state machine of one
// agent (state in +0x560c). Melee weapons (and bare hands): wait for the attack button (+0x55d0 bit 0x100), start
// the swing animation, ramp the charge (+0x57c0), release, recover; ranged weapons (bow, crossbow, thrown, pistol,
// musket): draw, hold, release, reload, quiver display, ammo messages. Also tracks the hand matrix
// (+0x5734 -> +0x5774) and the block/parry animations (+0x5610, +0x5614..).
// Names are provisional; offsets come from this function and the neighbouring slices.
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
};
struct StrTable { tstring* lookup(const tstring& key) MB_NOTHROW; };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;            // 0x40e870 (this = left operand)
};

// item types (Module System header_items.py)
enum {
    itp_type_one_handed_wpn = 2, itp_type_two_handed_wpn = 3, itp_type_polearm = 4, itp_type_arrows = 5,
    itp_type_bolts = 6, itp_type_bow = 8, itp_type_crossbow = 9, itp_type_thrown = 10, itp_type_pistol = 0x10,
    itp_type_musket = 0x11
};
#define itp_cant_reload_on_horseback 0x00100000
#define itcf_throw_stone   0x0000000000010000i64
#define itcf_throw_knife   0x0000000000020000i64
#define itcf_throw_axe     0x0000000000030000i64
#define itcf_throw_javelin 0x0000000000040000i64
#define itcf_shoot_musket  0x0000000000080000i64
#define itcf_shoot_mask    0x00000000000ff000i64
#define itcf_reload_pistol 0x0000007000000000i64
#define itcf_reload_musket 0x0000008000000000i64
#define itcf_reload_mask   0x000000f000000000i64

struct SoundRef { void play(int id); };             // 0x48add0 (thiscall, ret 4)

struct ItemKind {                                   // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                            // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    unsigned __int64 flags;                         // +0x1d0 itcf_*
    char pad1d8[0x228 - 0x1d8];
    SoundRef snd;                                   // +0x228
    char pad228[0x238 - 0x228 - sizeof(SoundRef)];
    bool is_ranged();                               // 0x433d90
    int wield_bone();                               // 0x433df0
};
extern ItemKind* g_items;                           // 0x5e29d0

struct ItemRef {                                    // item index + amount/modifier word
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    int max_amount();                               // 0x434b70
    void add_amount(int n);                         // 0x434be0
};

struct Skeleton {
    int count;                                      // +0
    char* elems;                                    // +4 (0x1dc-byte records, matrix at +0x84)
    char pad8[0x50 - 8];
    char done;                                      // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                                  // +0x64
    void update();                                  // 0x422130
    void update_poses();                            // 0x422080
    char* rec(int i);                               // 0x43f260
};
struct AgentBody {
    char pad0[0x58];
    mat4 m;                                         // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                 // +0x304
};

struct GameData {
    char pad0[0x1a1f4];
    int f1a1f4;                                     // +0x1a1f4 (agent id)
    int f1a1f8;                                     // +0x1a1f8
    int f1a1fc;                                     // +0x1a1fc
    int pad1a200;
    int f1a204;                                     // +0x1a204 (wielded slot)
};
extern GameData* g_game;                            // 0x5e2930

struct tstring;
struct Mission {
    char pad0[0x5c];
    int local_id;                                   // +0x5c
    void log_timed(const tstring* s, float seconds);    // 0x453020 (thiscall, ret 8)
};
extern Mission* g_mission;                          // 0x5e292c

extern int g_ctl_mode;                              // 0x5aa074
extern int g_stick_mode;                            // 0x5aa078
extern __int64 g_ticks[];                           // 0x5e2948
extern mat4 g_hand_mat;                             // 0xa708d0

float __cdecl clamp_float(const float* x, const float* lo, const float* hi);   // 0x40e3d0
float __cdecl frand();                              // 0x40e400
int __cdecl ftoi_helper(int x);                     // 0x435440

struct OrderSlot {                                  // 0x2c bytes (see s00437f00)
    int a;                                          // +0
    int b;                                          // +4
    char pad8[0x14 - 8];
    unsigned m14;                                   // +0x14 (low 17 bits = priority)
    float f18;                                      // +0x18
    char pad1c[0x20 - 0x1c];
    float rate;                                     // +0x20
    float f24;                                      // +0x24
    char pad28[0x2c - 0x28];
    bool set(int id, unsigned prio)                 // 0x4378a0 (inlined here)
    {
        if ((prio & 0x1ffff) < (m14 & 0x1ffff) && (a != -1 || b != -1))
            return false;
        b = id;
        m14 = prio;
        if (id == -1)
            m14 = 0;
        return true;
    }
};
struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; };
struct DefList { TroopDef* defs; int count; };
struct Curve { void set_rec_phase(int i, float p); };   // 0x433b20
struct SlotLink {
    char pad[0xcc];
    int m_cc;                                       // +0xcc
    void set_value(float* v);                       // 0x439000 (thiscall, ret 4)
};

// attack query record (three-row placeholder record initialised by 0x4353a0, s00434ad0 "Rows")
struct HitInfo {
    int m0, m4;
    vector4 v8, v18, v28;
    int m38, m3c;
    float dist;                                     // +0x40
    int m44, m48, m4c, m50, m54;
    HitInfo* init();                                // 0x4353a0 (returns this)
};

struct Agent {                                      // 0x5928 bytes
    char pad0[8];
    int id;                                         // +8
    char padc[4];
    int f10;                                        // +0x10
    char pad14[0x2c - 0x14];
    int mount_id;                                   // +0x2c (-1 on foot)
    char pad30[0x8c - 0x30];
    int f8c;                                        // +0x8c
    char pad90[0xd4 - 0x90];
    OrderSlot order[2];                             // +0xd4, +0x100 (+0x100 current animation id, +0x104 target)
    DefList* defs;                                  // +0x12c
    char pad130[0x13c - 0x130];
    int f13c[4];                                    // +0x13c
    SlotLink* links[4];                             // +0x14c
    char pad15c[0x1d0 - 0x15c];
    ItemRef held[11];                               // +0x1d0
    int pad228;
    int wield_a;                                    // +0x22c
    char pad230[0x250 - 0x230];
    AgentBody* body;                                // +0x250
    char pad254[0x55d0 - 0x254];
    unsigned in_flags;                              // +0x55d0
    char pad55d4[0x55ec - 0x55d4];
    int f55ec;                                      // +0x55ec (draw animation)
    int f55f0;                                      // +0x55f0 (release animation)
    int f55f4;                                      // +0x55f4
    int f55f8;                                      // +0x55f8
    char pad55fc[0x560c - 0x55fc];
    int f560c;                                      // +0x560c (state)
    int f5610;                                      // +0x5610
    int f5614, f5618, f561c, f5620;                 // +0x5614
    char pad5624[0x56c4 - 0x5624];
    int f56c4, f56c8;                               // +0x56c4
    char pad56cc[0x5734 - 0x56cc];
    mat4 m5734;                                     // +0x5734
    mat4 m5774;                                     // +0x5774
    float f57b4;                                    // +0x57b4
    char pad57b8[0x57bc - 0x57b8];
    char f57bc, f57bd;                              // +0x57bc
    char pad57be[0x57c0 - 0x57be];
    float f57c0;                                    // +0x57c0
    char pad57c4[0x57f0 - 0x57c4];
    int f57f0;                                      // +0x57f0
    char pad57f4[0x57f8 - 0x57f4];
    __int64 f57f8;                                  // +0x57f8

    ItemRef slot_pair(int idx) const;               // 0x432790 (s00432130; thiscall, ret 8)
    bool charged_0x48() const;                      // 0x432470
    void classify(int key);                         // 0x435520
    float damage_factor(int idx);                   // 0x4385e0
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void update_quiver();                           // 0x43a800
    ItemRef best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880
    int pick_attack_dir(unsigned target_id);        // 0x43f970
    void start_shoot(int slot);                     // 0x43fd10
    void pick_sound_ids(int* a, int* b, int* c);    // 0x440070 (ret 0xc)
    void play_sound_var(int* kind, float* var);     // 0x443730
    void play_hit_sound(int* kind);                 // 0x4435c0
    void play_class_sound(const int* kind);         // 0x4459c0
    int refill_ammo(int count);                     // 0x445a90
    void shoot();                                   // 0x44e2b0
    int swing_check(int a, int b, HitInfo* hit, float* charge);   // 0x45a170 (ret 0x10)
    void update_attack(float dt);                   // 0x45c220

    __forceinline ItemRef cur_pair() const
    {
        const ItemRef* p = 0;
        if (wield_a >= 0) p = &held[wield_a];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    __forceinline ItemRef* cur_ptr()
    {
        ItemRef* p = 0;
        if (wield_a >= 0) p = &held[wield_a];
        return p;
    }
    // 0x4379c0 with slot 1, as cl inlines it when the animation id is a constant (see s004556d0)
    __forceinline void order1(int id, unsigned prio)
    {
        float rate = 1.0f;
        if (id >= -1 && id < defs->count) {
            if (order[1].set(id, prio)) {
                order[1].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
                if (prio & 0x4000000)
                    ((Curve*)0)->set_rec_phase(1, 0.0f);
                int k = wield_a;
                if (k >= 0 && held[k].item >= 0) {
                    if (prio & 0x10000000)
                        links[k]->m_cc = 0;
                    else
                        links[k]->m_cc = 0xffff;
                }
            }
        }
    }
};

#define ACT order[1].a          // +0x100 current animation id
#define NEXTACT order[1].b      // +0x104
#define PRIO1 order[1].m14      // +0x114
#define PROG order[1].f18       // +0x118 animation progress
#define TYPE (ik->type_flags & 0xff)
#define LOG_TIMED(str) { g_mission->log_timed(g_strtable.lookup(tstring(str)), 20.0f); }

// @ 0x45c220
void Agent::update_attack(float dt)
{
    HitInfo hit;
    hit.init();
    if (((int)(order[0].m14 & 0x1ffff) >= 0x20 && order[0].a >= 0 && (defs->defs[order[0].a].flags & 0x400)) ||
        (int)(PRIO1 & 0x1ffff) >= 0x40) {
        f560c = 0;
        f5610 = 0;
        return;
    }
    ItemRef cur = cur_pair();
    Skeleton* sk = body->skel;
    if (!sk->done) {
        if (sk->has_poses)
            sk->update_poses();
        else
            sk->update();
    }

    m5734 = body->m * *(mat4*)(sk->elems + 0x23d8);
    if (f10 != 0)
        goto done;
    {
    int ci = cur.item;
    if (ci < 0) ci = 0;
    ItemKind* ik = &g_items[ci];
    unsigned t0 = TYPE;
    if (t0 == itp_type_one_handed_wpn || t0 == itp_type_two_handed_wpn || t0 == itp_type_polearm || cur.item < 0) {
        // ------------------------------------------------------------------ melee / bare hands
        if (ACT == -1)
            f560c = 0;
        int S = f560c;
        if (S == 0 && f5610 == 0) {
            float t = dt + f57b4;
            f57b4 = t;
            if (t > 1.0f)
                f57b4 = 1.0f;
            if (in_flags & 0x100) {
                bool found = false;
                int i = 0;
                do {
                    if (i >= 10) break;
                    start_shoot(0);
                    if (f55ec != -1) {
                        found = true;
                        f560c = 1;
                        set_order(1, f55ec, 0x0240000a, damage_factor(0));
                        f56c8 = 0;
                        f5610 = 0;
                        classify(0);
                        f57c0 = f57b4;
                    }
                    i++;
                } while (!found);
            }
        } else if (S == 1) {
            if (PROG > 0.99f) {
                float u = dt * 0.8f + f57c0;
                f57c0 = u;
                if (u < 1.0f)
                    f57c0 = dt * 0.6f + u;
                if (f57c0 > 1.3f)
                    f57c0 = 1.3f;
            }
            if (ACT == f55ec && !(in_flags & 0x200)) {
                if (id == g_mission->local_id) {
                    if (in_flags & 0x100) {
                        if (PROG < 0.3f && g_ctl_mode > 0) {
                            if (pick_attack_dir(0) != f55f8) {
                                start_shoot(0);
                                if (f55ec != -1) {
                                    set_order(1, f55ec, 0x0240000a, damage_factor(0));
                                    classify(0);
                                }
                            }
                            goto melee_tail;
                        }
                        goto melee_hold;
                    }
                } else {
                melee_hold:
                    if (in_flags & 0x100)
                        goto melee_tail;
                }
                if (PROG > 0.99f) {
                    if (f55f0 == -1) {
                        f560c = 0;
                    } else {
                        set_order(1, f55f0, 0x0140000a, damage_factor(0));
                        f560c = 2;
                        int kind = 3;
                        play_hit_sound(&kind);
                        if (mount_id == -1 && frand() < (f57c0 - 1.0f) * 3.5f) {
                            if (id == g_mission->local_id || frand() < 0.3f) {
                                kind = 2;
                                play_class_sound(&kind);
                            } else {
                                kind = 3;
                                play_class_sound(&kind);
                            }
                        }
                        m5734.o.x = m5774.o.x = -10000000.0f;
                        f57bc = 0;
                        f57bd = 0;
                        classify(0);
                    }
                }
            } else {
                f57b4 = 0.8f;
                f560c = 6;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
        }
    melee_tail:
        S = f560c;
        if ((S == 2 && ACT == NEXTACT && ACT == f55f0 && (f55f8 == 0 || PROG > 0.1f)) || charged_0x48()) {
            if (charged_0x48()) {
                f55f8 = 0;
                f55f4 = 0x44;
                if (frand() < 0.05f) {
                    f57bc = 0;
                    f57bd = 0;
                }
            }
            if (in_flags & 0x200) {
                f560c = 6;
                f57b4 = 0.8f;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
            if (!charged_0x48() && (!(PROG < 0.85f) || (f55f8 != 0 && !(PROG > 0.2f))))
                goto chk99;
            if (charged_0x48())
                f57c0 = 1.0f;
            int r = swing_check(0, 0, &hit, &f57c0);
            if (r != 0 && charged_0x48())
                order1(0x44, 0x0100007d);
            if (r >= 2) {
                set_order(1, f55f4, 0x1400023, damage_factor(0));
                f560c = 5;
            }
            if ((in_flags & 0x100) && id != g_mission->local_id) {
                f560c = 6;
                f57b4 = 0.8f;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
            if (r != 1) {
                if (r == 2)
                    goto clear_state;
                goto chk99;
            }
            if (hit.dist < 5.0f && cur.item >= 0) {
                set_order(1, f55f4, 0x1400023, damage_factor(0));
                f560c = 5;
            } else {
                f560c = 3;
            }
            goto tail;
        } else {
            if (S == 3)
                goto chk99;
            if (S != 5 && S != 6)
                goto tail;
            if (ACT == -1)
                goto clear_state;
        chk99:
            if (!(PROG > 0.99f))
                goto tail;
        clear_state:
            f560c = 0;
        }
    } else if (ik->is_ranged()) {
        // ------------------------------------------------------------------ bow / crossbow / thrown / guns
        int slot = 0, missing, total;
        best_ammo(-1, &missing, &total, &slot);
        if (TYPE == itp_type_crossbow) {
            if (cur.max_amount() - (cur.m4 & 0xff) == 0) {
                float var = 0.0f;
                links[wield_a]->set_value(&var);
                if (slot >= 0)
                    links[slot]->m_cc = 0;
            } else {
                float var = 100.0f;
                links[wield_a]->set_value(&var);
                if (slot >= 0)
                    links[slot]->m_cc = 0xffff;
            }
        } else if (TYPE == itp_type_bow && links[wield_a] != 0) {
            float var = 0.0f;
            links[wield_a]->set_value(&var);
        }
        int S = f560c;
        if (S == 0) {
            if (!(in_flags & 0x400))
                goto tail;
            if ((TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) &&
                cur.max_amount() - (cur.m4 & 0xff) == 0) {
                if (missing > 0) {
                    f5620 = -1;
                    if (mount_id == -1) {
                        f5620 = 0x7b;
                    } else if (ik->type_flags & itp_cant_reload_on_horseback) {
                        if (id == g_mission->local_id) {
                            g_msglog.add(*g_strtable.lookup(tstring("ui_cant_reload_this_weapon_on_horseback")), -1);
                        }
                    } else {
                        f5620 = 0x7c;
                    }
                    unsigned __int64 rl = ik->flags & itcf_reload_mask;
                    if (rl != 0) {
                        if (rl == itcf_reload_pistol)
                            f5620 = 0x87;
                        else if (rl == itcf_reload_musket)
                            f5620 = 0x8a;
                    }
                    if (f5620 >= 0) {
                        set_order(1, f5620, 0x0140003c, damage_factor(0));
                        update_quiver();
                        f560c = 2;
                    }
                } else if (id == g_mission->local_id) {
                    if (total > 0)
                        LOG_TIMED("ui_no_more_bolts")
                    else
                        LOG_TIMED("ui_you_are_not_carrying_any_bolts")
                }
            } else if (missing <= 0 && TYPE != itp_type_crossbow && TYPE != itp_type_pistol && TYPE != itp_type_musket) {
                if (id == g_mission->local_id) {
                    if (total > 0) {
                        if (TYPE == itp_type_bow)
                            LOG_TIMED("ui_no_more_arrows")
                    } else {
                        if (TYPE == itp_type_bow)
                            LOG_TIMED("ui_you_are_not_carrying_any_arrows")
                        else
                            LOG_TIMED("ui_you_are_not_carrying_any_bolts")
                    }
                }
            } else {
                f57bc = 0;
                f57bd = 0;
                f560c = 1;
                f57f8 = g_ticks[f57f0];
                start_shoot(0);
                if (f55ec != -1) {
                    set_order(1, f55ec, 0x02400002, damage_factor(0));
                    unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                    if (sm == itcf_throw_axe || sm == itcf_throw_knife || sm == itcf_throw_stone ||
                        sm == itcf_shoot_musket || TYPE == itp_type_thrown || TYPE == itp_type_crossbow) {
                        f56c4 = 4;
                        f56c8 = 0;
                    } else {
                        f56c8 = 0;
                        f56c4 = ((TYPE != itp_type_pistol) - 1) & 5;
                    }
                }
            }
        } else if (S == 1) {
            if ((unsigned char)ik->type_flags == itp_type_bow && PROG > 0.3f) {
                if (PROG > 0.46f && links[wield_a] != 0) {
                    float x = (PROG - 0.46f) * 2.5f;
                    float hi = 1.0f;
                    float lo = 0.0f;
                    float var = clamp_float(&x, &lo, &hi) * 100.0f;
                    links[wield_a]->set_value(&var);
                }
                links[slot]->m_cc = 0xffff;
                update_quiver();
            }
            if (in_flags & 0x200) {
                order1(-1, 2);
                if ((unsigned char)ik->type_flags == itp_type_bow && links[slot] != 0) {
                    links[slot]->m_cc = 0;
                    update_quiver();
                }
                goto clear_state;
            }
            if (in_flags & 0x100)
                goto tail;
            if (!(PROG > 0.99f) && ((TYPE != itp_type_crossbow && TYPE != itp_type_musket) || !(PROG > 0.2f)))
                goto tail;
            {
                unsigned rel = 0;
                if (TYPE != itp_type_bow) {
                    if (TYPE == itp_type_thrown) {
                        rel = 0x40001b;
                    } else if (TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) {
                        ItemRef* rp = cur_ptr();
                        if (rp != 0)
                            rp->add_amount(1);
                    }
                }
                set_order(1, f55f0, rel | 0x1000002, damage_factor(0));
            }
            if ((unsigned char)ik->type_flags != itp_type_thrown) {
                shoot();
                if ((unsigned char)ik->type_flags == itp_type_bow)
                    refill_ammo(1);
                if (slot >= 0 && links[slot] != 0)
                    links[slot]->m_cc = 0;
                f560c = 0;
            } else {
                f560c = 3;
            }
            if (TYPE == itp_type_bow) {
                int kind = 0x25;
                float var = 0.1f;
                if (id != g_mission->local_id)
                    play_sound_var(&kind, &var);
                else
                    play_sound_var(&kind, &var);
            } else if (TYPE == itp_type_crossbow) {
                float var = 0.1f;
                int kind = 0x26;
                play_sound_var(&kind, &var);
            } else if (TYPE == itp_type_thrown) {
                unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                int kind;
                float var;
                if (sm == itcf_throw_javelin) {
                    kind = 0x27;
                    var = 0.1f;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_axe) {
                    var = 0.1f;
                    kind = 0x28;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_knife) {
                    var = 0.1f;
                    kind = 0x29;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_stone) {
                    kind = 0x2a;
                    var = 0.1f;
                    play_sound_var(&kind, &var);
                }
            }
            g_game->f1a1f4 = id;
            g_game->f1a1f8 = f8c;
            g_game->f1a204 = wield_a;
            {
                GameData* g = g_game;
                g->f1a1fc = f13c[ftoi_helper(g->f1a204)];
            }
            g_hand_mat = body->m * *(mat4*)(body->skel->rec(ik->wield_bone()));
            ik->snd.play(-0x33);
        } else if (S == 2) {
            if (ACT != f5620) {
                f560c = 0;
            } else if ((in_flags & 0x130200) ||
                       (ACT == 0x7b && PROG < 0.8f && id == g_mission->local_id && (in_flags & 0xf))) {
                order1(-1, 0x3d);
                f560c = 0;
            } else if (PROG > 0.99f) {
                ItemRef* rp = cur_ptr();
                if (rp != 0) {
                    unsigned u = rp->m4 & 0xff;
                    int n = refill_ammo(u);
                    ItemRef* rq = cur_ptr();
                    rq->b.amount = 0;
                    rq->m4 |= u - n;
                }
                update_quiver();
                goto clear_state;
            } else if (links[wield_a] != 0) {
                float lo2 = 0.4f, hi2 = 0.66f;
                if (mount_id != -1) {
                    lo2 = 0.16f;
                    hi2 = 0.5f;
                }
                float x = (PROG - lo2) / (hi2 - lo2);
                float hi = 1.0f;
                float lo = 0.0f;
                float var = clamp_float(&x, &lo, &hi) * 100.0f;
                links[wield_a]->set_value(&var);
            }
        } else if (S == 3) {
            if (ACT == 0x84 || ACT == 0x80 || ACT == 0x82 || ACT == 0x7e) {
                if (PROG > 0.25f) {
                    shoot();
                    refill_ammo(1);
                    f560c = 4;
                    if (wield_a >= 0)
                        links[wield_a]->m_cc = 0;
                }
            } else {
                f560c = 0;
            }
        } else if (S == 4) {
            if (ACT == 0x84 || ACT == 0x80 || ACT == 0x82 || ACT == 0x7e) {
                if (!(PROG > 0.98f))
                    goto tail;
                if (wield_a >= 0)
                    links[wield_a]->m_cc = 0xffff;
                update_quiver();
            }
            goto clear_state;
        }
    }
    }
tail:
    if (in_flags & 0x200) {
        if (f5610 == 0) {
            pick_sound_ids(&f5614, &f5618, &f561c);
            if (f5614 != -1 && (int)(PRIO1 & 0x1ffff) <= 0xf) {
                f5610 = 1;
                f560c = 0;
                float v = damage_factor(0);
                if (slot_pair(1).item >= 0)
                    v = damage_factor(1);
                set_order(1, f5614, 0x0240000e, v);
                f56c8 = 0;
                classify(1);
            }
            goto done;
        }
        if (ACT == f5614 || ACT == f561c || ACT == f5618) {
            if (ACT == f5614 && PROG < 0.99f) {
                if (PROG < 0.5f && g_stick_mode != 0) {
                    int a, b, c;
                    pick_sound_ids(&a, &b, &c);
                    if (a != f5614) {
                        f5618 = b;
                        f5614 = a;
                        f561c = c;
                        float v = damage_factor(0);
                        if (slot_pair(1).item >= 0)
                            v = damage_factor(1);
                        float p = PROG;
                        set_order(1, f5614, 0x0240000e, v);
                        order[1].f24 = p * 0.85f;
                        f560c = 0;
                        goto done;
                    }
                }
            } else {
                set_order(1, f5618, 0xe, 1.0f);
            }
            f560c = 0;
        } else {
            order1(-1, 0xf);
        }
    } else {
        f5610 = 0;
        if (f5614 != -1) {
            if (ACT == f5614 || (ACT == f561c && PROG > 0.2f))
                order1(-1, 0xf);
        }
    }
done:
    m5774 = m5734;
}
