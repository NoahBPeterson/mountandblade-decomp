// Slice s00444370: Agent::on_wield_changed (0x444370, 2117 bytes) plus the 0x38-byte E56 uninitialized-fill helper
// (std::_Uninit_fill_n with its catch funclet 0x444c28). When the wielded item of either hand changed it picks the
// draw/stow animation and the two sound effects from the item's carry position (a 64-bit switch on itcf_carry_*),
// starts the animation, plays the sounds (throttled by a time table) and recomputes the agent's weapon-class flags.
// All names provisional; layouts copied from s00443aa0 / s004417d0 / s00437f00 / s004375b0.
#include <vector>
#include <new>
#include <stdlib.h>
#include "rgl.h"
using rgl::vector4;

// ---- item records -------------------------------------------------------------------------------------------------
struct item_kind {
    char pad0[0x1c8];
    union { unsigned type_flags; unsigned char type; };      // 0x1c8: low byte = itp_type_*
    char pad1cc[4];
    unsigned __int64 capability_flags;                       // 0x1d0 itcf_*
    char pad1d8[0x238 - 0x1d8];
};
extern item_kind* g_items;                                   // 0x5e29d0

#define itp_type_one_handed_wpn  2
#define itp_type_two_handed_wpn  3
#define itp_type_polearm         4
#define itp_type_shield          7
#define itp_type_bow             8
#define itp_type_crossbow        9
#define itp_type_thrown          10
#define itp_two_handed           0x200000

#define itcf_carry_mask                      0x7f0000000i64
#define itcf_carry_sword_left_hip            0x10000000i64
#define itcf_carry_axe_left_hip              0x20000000i64
#define itcf_carry_dagger_front_left         0x30000000i64
#define itcf_carry_dagger_front_right        0x40000000i64
#define itcf_carry_quiver_front_right        0x50000000i64
#define itcf_carry_quiver_right_vertical     0x70000000i64
#define itcf_carry_revolver_right            0x90000000i64
#define itcf_carry_pistol_front_left         0xa0000000i64
#define itcf_carry_bowcase_left              0xb0000000i64
#define itcf_carry_mace_left_hip             0xc0000000i64
#define itcf_carry_axe_back                  0x100000000i64
#define itcf_carry_sword_back                0x110000000i64
#define itcf_carry_crossbow_back             0x150000000i64
#define itcf_carry_spear                     0x170000000i64
#define itcf_carry_katana                    0x210000000i64
#define itcf_carry_wakizashi                 0x220000000i64

// ---- sound system (see s00443270) ---------------------------------------------------------------------------------
struct SoundSystem {
    int play(int sound, int priority, const vector4* pos, float pitch, float* out);     // 0x442650 (thiscall, ret 0x14)
};
extern SoundSystem g_sound;                                  // 0x5aa128
extern int g_sound_ids[];                                    // 0x5aa178
extern __int64 g_times[];                                    // 0x5e2948

struct Mission {
    char pad0[0x62e8];
    int time_idx;                                            // +0x62e8
    char pad62ec[4];
    __int64 time_base;                                       // +0x62f0
};
extern Mission* g_mission;                                   // 0x5e292c

// ---- agent parts --------------------------------------------------------------------------------------------------
struct OrderSlot {                                           // 0x2c bytes (see s00437f00)
    int a;                                                   // +0
    int b;                                                   // +4
    char pad8[0x14 - 8];
    unsigned m14;                                            // +0x14
    char pad18[0x20 - 0x18];
    float rate;                                              // +0x20
    char pad24[0x2c - 0x24];
    bool set(int id, unsigned prio)                          // 0x4378a0 (out of line elsewhere; cl inlines two of the three calls here)
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
struct DefList { void* defs; int count; };
struct Curve { void set_rec_phase(int i, float p); };        // 0x433b20
struct CurveOwner { char pad[0x304]; Curve* curve; };
struct Mesh { char pad[0xcc]; unsigned vis; };

struct ItemRef { int item; unsigned m4; };
struct Agent {
    char pad0[0x50];
    vector4 pos;                                             // +0x50
    char pad60[0xd4 - 0x60];
    OrderSlot order[2];                                      // +0xd4
    DefList* defs;                                           // +0x12c
    int f130;                                                // +0x130
    char pad134[0x14c - 0x134];
    Mesh* A[4];                                              // +0x14c
    char pad15c[0x1d0 - 0x15c];
    ItemRef slots[8];                                        // +0x1d0
    char pad210[0x22c - 0x210];
    int cur[2];                                              // +0x22c
    int prev[2];                                             // +0x234
    int f23c;                                                // +0x23c
    char pad240[0x250 - 0x240];
    CurveOwner* body;                                        // +0x250
    char pad254[0x560c - 0x254];
    int f560c, f5610;                                        // +0x560c
    char pad5614[0x56c4 - 0x5614];
    int f56c4;                                               // +0x56c4
    char pad56c8[0x5928 - 0x56c8];

    void on_wield_changed();                                 // 0x444370
};

// @ 0x444370
void Agent::on_wield_changed()
{
    int anim = -1;
    int anim2 = -1;
    int snd2 = -1;
    int snd1 = -1;
    int j;
    for (j = 0; j < 2; j++) {
        int c = cur[j];
        int p = prev[j];
        if (c != p) {
            int item;
            if (c < 0 || (item = slots[c].item) < 0 || (j != 0 && f23c != 0)) {
                if (j != 0) {
                    if (p >= 0 && c < 0 && f23c == 0) {
                        int pi = slots[p].item;
                        anim2 = 0x6d;
                        if (pi >= 0) {
                            item_kind* ik = &g_items[pi];
                            unsigned t = ik->type_flags & 0xff;
                            if (t == itp_type_bow) {
                                anim2 = ((ik->capability_flags & itcf_carry_mask) == itcf_carry_bowcase_left) ? 0x71 : 0x6f;
                                snd1 = 0x55;
                            } else if (t == itp_type_shield) {
                                anim2 = 0x6d;
                                snd1 = 0x57;
                            }
                        }
                
                    }
                } else {
                    if (p >= 0 && c < 0) {
                        int pi = slots[p].item;
                        anim = 0x53;
                        if (pi >= 0) {
                            switch (g_items[pi].capability_flags & itcf_carry_mask) {
                            case itcf_carry_mace_left_hip:
                            case itcf_carry_axe_left_hip:        anim = 0x59; snd2 = 0x49; break;
                            case itcf_carry_dagger_front_right:  anim = 0x61; snd2 = 0x53; break;
                            case itcf_carry_sword_left_hip:      anim = 0x55; snd2 = 0x45; break;
                            case itcf_carry_dagger_front_left:   anim = 0x5f; snd2 = 0x53; break;
                            case itcf_carry_revolver_right:      anim = 0x65; snd2 = 0x51; break;
                            case itcf_carry_pistol_front_left:   anim = 0x67; snd2 = 0x51; break;
                            case itcf_carry_spear:               anim = 0x5d; snd2 = 0x4d; break;
                            case itcf_carry_axe_back:            anim = 0x63; snd2 = 0x4b; break;
                            case itcf_carry_sword_back:          anim = 0x57; snd2 = 0x47; break;
                            case itcf_carry_crossbow_back:       anim = 0x5b; snd2 = 0x4f; break;
                            case itcf_carry_katana:              anim = 0x69; snd2 = 0x45; break;
                            case itcf_carry_wakizashi:           anim = 0x6b; snd2 = 0x53; break;
                            default:                             anim = 0x53; break;
                            }
                        }
                    }
                }
            } else {
                item_kind* ik = &g_items[item];
                unsigned __int64 carry = ik->capability_flags & itcf_carry_mask;
                unsigned t = ik->type_flags & 0xff;
                if (t == itp_type_bow) {
                    anim2 = (carry == itcf_carry_bowcase_left) ? 0x70 : 0x6e;
                    snd1 = 0x54;
                } else if (t == itp_type_shield) {
                    anim2 = 0x6c;
                    snd1 = 0x56;
                } else if (j == 0) {
                    switch (carry) {
                    case itcf_carry_mace_left_hip:
                    case itcf_carry_axe_left_hip:        anim = 0x58; snd2 = 0x48; break;
                    case itcf_carry_dagger_front_right:  anim = 0x60; snd2 = 0x52; break;
                    case itcf_carry_sword_left_hip:      anim = 0x54; snd2 = 0x44; break;
                    case itcf_carry_dagger_front_left:   anim = 0x5e; snd2 = 0x52; break;
                    case itcf_carry_revolver_right:      anim = 100;  snd2 = 0x50; break;
                    case itcf_carry_pistol_front_left:   anim = 0x66; snd2 = 0x50; break;
                    case itcf_carry_spear:               anim = 0x5c; snd2 = 0x4c; break;
                    case itcf_carry_axe_back:            anim = 0x62; snd2 = 0x4a; break;
                    case itcf_carry_sword_back:          anim = 0x56; snd2 = 0x46; break;
                    case itcf_carry_crossbow_back:       anim = 0x5a; snd2 = 0x4e; break;
                    case itcf_carry_katana:              anim = 0x68; snd2 = 0x44; break;
                    case itcf_carry_wakizashi:           anim = 0x6a; snd2 = 0x52; break;
                    default:
                        if (t == itp_type_polearm) { anim = 0x5c; snd2 = 0x4c; }
                        else { anim = 0x52; snd2 = 0x58; }
                        break;
                    }
                } else {
                    anim2 = 0x6c;
                }
            }
        }
        prev[j] = c;
    }
    if (f23c != 0)
        prev[1] = -1;
    if (order[1].a == 0x7b && defs->count > -1 && order[1].set(-1, 0x46)) {
        order[1].rate = 1.0f;
        int c0 = cur[0];
        if (c0 >= 0 && slots[c0].item >= 0)
            A[c0]->vis = 0xffff;
    }
    if (anim != -1) {
        if (anim < -1 || anim >= defs->count || !order[1].set(anim, 0x5000046))
            goto skip;
    } else {
        if (anim2 == -1 || anim2 < -1 || anim2 >= defs->count || !order[1].set(anim2, 0x5000046))
            goto skip;
    }
    order[1].rate = 1.0f;
    body->curve->set_rec_phase(1, 0.0f);
    {
        int c0 = cur[0];
        if (c0 >= 0 && slots[c0].item >= 0)
            A[c0]->vis = 0xffff;
    }
skip:
    if (snd1 != -1 && (float)(g_times[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound.play(g_sound_ids[snd1], 0, &pos, 1.0f, 0);
    if (snd2 != -1 && (float)(g_times[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound.play(g_sound_ids[snd2], 0, &pos, 1.0f, 0);
    f130 = 0;
    for (j = 0; j < 2; j++) {
        int s = cur[j];
        if (s >= 0 && slots[s].item >= 0 && (j == 0 || f23c == 0)) {
            item_kind* ik = &g_items[slots[s].item];
            unsigned fl = ik->type_flags;
            unsigned t = fl & 0xff;
            if (t == itp_type_bow) {
                f130 = 0x10;
                f56c4 = 0;
            } else if (t == itp_type_crossbow || t == itp_type_thrown) {
                f130 = 0x20;
                f56c4 = 4;
            } else if (t == itp_type_two_handed_wpn && (fl & itp_two_handed)) {
                f130 = 4;
            } else if (t == itp_type_shield) {
                f130 |= 0x1000;
            } else if (t == itp_type_polearm) {
                f130 |= 8;
            } else if (t == itp_type_one_handed_wpn || t == itp_type_two_handed_wpn) {
                f130 |= 2;
            }
        }
    }
    f560c = 0;
    f5610 = 0;
}

// ---- std::_Uninit_fill_n for the 0x38-byte E56 record (see s004417d0) -----------------------------------------------
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};
struct alloc_e56 {
    __declspec(noinline) void construct(E56* p, const E56& v);      // 0x442060
    void destroy(E56* p);                                           // 0x43e150
};

// @ 0x444bc0 (+ catch-all funclet 0x444c28: destroys [next, first), then `throw;`)
void __cdecl uninit_fill_n_e56(E56* first, unsigned n, const E56& val, alloc_e56& al, int tag)
{
    E56* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy(next);
        throw;
    }
}
// @ 0x444c28
// (catch funclet of uninit_fill_n_e56)
