// Slice s004556d0: Agent::update_controls (0x4556d0, 8252 bytes), the per-tick movement update of an agent (human or
// mount): damps the lean/turn accumulators, reads the movement input bits (+0x55d0) into the two input axes
// (g_move_fwd / g_move_side), derives the wanted ground velocity (+0xa8/+0xac) from them (skill, slope, stamina,
// terrain), picks the walk/run/strafe animation per gait band and weapon mode (set_order slot 0), runs the mount's
// acceleration / jump / rear-up state machine with its gait bands, handles the jump / horse-hit sounds, steers the
// heading, reacts to the mount's gait animation and finally does the mount / dismount transitions (+0x10000/+0x20000).
// Names are provisional. Not byte-exact: see nonmatching.txt. The slot-1 order requests that cl inlined at constant
// arguments are Agent::set_order_inl / order1 (the out-of-line set_order at 0x4379c0 serves the slot-0 calls).
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- item records ------------------------------------------------------------------------------------------------
struct ItemKind {                                       // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
    bool is_melee();                                    // 0x433dc0
};
extern ItemKind* g_items;                               // 0x5e29d0

struct ItemRef {                                        // item index + packed amount/modifier
    int item;
    unsigned m4;
    int stat_1f4();                                     // 0x434f60
    int stat_1f0b();                                    // 0x434fd0
};

// ---- the world / mission objects ---------------------------------------------------------------------------------
struct Troop {                                          // 0x508 bytes
    char pad0[0x5c];
    int f5c;                                            // +0x5c
    char pad60[0x508 - 0x60];
    int nibble(int i);                                  // 0x4351a0
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                                      // +0x13a00
    bool is_night();                                    // 0x435740 (ecx = g_world, otherwise unused)
};
extern World* g_world;                                  // 0x5e2930

struct ModeRec { char pad[0x20]; unsigned flags; char pad2[0x58 - 0x24]; };   // 0x58, array at 0x5e2a00
extern ModeRec* g_modes;                                // 0x5e2a00

struct Agent;
struct AgentTable { Agent* get(unsigned id); };         // 0x43b1f0 (thiscall on mission+4)
struct Mission {
    char pad0[0x18];
    std::vector<Agent*> pages;                          // +0x18 (page table: id >> 4; 16 agents of 0x5928 bytes; AgentTable lives at +4)
    char pad28[0x5c - 0x28];
    int local_id;                                       // +0x5c
    char pad60[0x5df0 - 0x60];
    int cur_mode;                                       // +0x5df0 (index into g_modes)
    char pad5df4[0x6528 - 0x5df4];
    unsigned char f6528;                                // +0x6528
    char pad6529[0x654c - 0x6529];
    float f654c;                                        // +0x654c
    char pad6550[0x6604 - 0x6550];
    int f6604;                                          // +0x6604
};
extern Mission* g_mission;                              // 0x5e292c
extern __int64 g_ticks[];                               // 0x5e2948 (100 kHz clocks)

struct gui_timer {                                      // 16 bytes
    int clock;
    int pad4;
    __int64 t;
    float elapsed() const;                              // 0x432c10
    void set_delay(float seconds);                      // 0x42cd20
    void restart() { t = g_ticks[clock]; }              // inline
};
struct Timer {                                          // s00432a50: clock, start, active flag at +0x10
    int clock;
    int pad4;
    __int64 start;
    char active;
    void begin(float seconds);                          // 0x432da0
    bool expired();                                     // 0x432de0
};
struct vec2 {
    float x, y;
    void rotate_b(float ang);                           // 0x4327f0
};

struct OrderSlot {                                      // 0x2c bytes (see s00437f00)
    int a;                                              // +0
    int b;                                              // +4
    char pad8[0x14 - 8];
    unsigned m14;                                       // +0x14
    char pad18[0x1c - 0x18];
    float f1c;                                          // +0x1c (+0xf0 in slot 0: 1/gait ratio)
    float rate;                                         // +0x20
    char pad24[0x2c - 0x24];
    bool set(int id, unsigned prio)                     // 0x4378a0 (inlined here)
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
struct pos : matrix {
    vector4 rot_ext(const vector4& p) const;            // 0x40a9c0
};
struct AgentBody {
    char pad0[0x58];
    pos m;                                              // +0x58
    char pad98[0x304 - 0x98];
    Curve* curve;                                       // +0x304
};
struct Obj_a0 { char pad[0xa0]; vector4 d; };           // +0xa0 direction vector
struct SlotLink { char pad[0xcc]; int m_cc; };

struct vec3 {
    float x, y, z;
    vec3() {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    void set(const float& a, const float& b, const float& c) { x = a; y = b; z = c; }
};

struct GaitBand { float lo, hi; };
extern GaitBand g_gait_walk[];                          // 0x590940
extern GaitBand g_gait_mount[];                         // 0x590910
extern float g_move_fwd;                                // 0x5e2478
extern float g_move_side;                               // 0x5e247c

float __cdecl fmax_ref(const float* a, const float* b); // 0x4324d0
float __cdecl clamp_float(const float* x, const float* lo, const float* hi);  // 0x40e3d0
float __cdecl frand();                                  // 0x40e400

struct Agent {                                          // 0x5928 bytes
    char pad0[8];
    unsigned id;                                        // +8
    char padc[4];
    int f10;                                            // +0x10 (1 = mount)
    char pad14[0x2c - 0x14];
    int mount_id;                                       // +0x2c (-1 on foot)
    int rider_id;                                       // +0x30
    int anim34;                                         // +0x34
    int anim38;                                         // +0x38
    unsigned flags3c;                                   // +0x3c
    gui_timer timer40;                                  // +0x40
    char pad50[0x58 - 0x50];
    float f58;                                          // +0x58
    char pad5c[0x60 - 0x5c];
    vector4 dir;                                        // +0x60
    float f70, f74;                                     // +0x70
    char pad78[4];
    int f7c;                                            // +0x7c
    char pad80[0x88 - 0x80];
    int troop;                                          // +0x88
    ItemRef mount_item;                                 // +0x8c
    char pad94[0xa0 - 0x94];
    float fa0;                                          // +0xa0
    char pada4[0xa8 - 0xa4];
    vec2 vel;                                           // +0xa8 (ground velocity: x = sideways, y = forward)
    char padb0[0xb4 - 0xb0];
    float fb4;                                          // +0xb4
    unsigned fb8;                                       // +0xb8
    Obj_a0* fbc;                                        // +0xbc
    char padc0[0xd4 - 0xc0];
    OrderSlot order[2];                                 // +0xd4
    DefList* defs;                                      // +0x12c
    unsigned f130;                                      // +0x130
    char pad134[0x14c - 0x134];
    SlotLink* links[4];                                 // +0x14c
    char pad15c[0x1d0 - 0x15c];
    ItemRef held[11];                                   // +0x1d0
    int pad228;
    int wield_a;                                        // +0x22c
    int wield_b;                                        // +0x230
    char pad234[0x24c - 0x234];
    float f24c;                                         // +0x24c
    AgentBody* body;                                    // +0x250
    char pad254[0x2b0 - 0x254];
    gui_timer timer2b0;                                 // +0x2b0
    float f2c0;                                         // +0x2c0
    unsigned char f2c4;                                 // +0x2c4
    char pad2c5[0x2c8 - 0x2c5];
    int f2c8;                                           // +0x2c8
    char pad2cc[0x31c - 0x2cc];
    unsigned target_id;                                 // +0x31c
    char pad320[0x55d0 - 0x320];
    unsigned in_flags;                                  // +0x55d0
    char f55d4;                                         // +0x55d4
    char pad55d5[0x55d8 - 0x55d5];
    vector4 dir55d8;                                    // +0x55d8
    char pad55e8[0x560c - 0x55e8];
    int f560c;                                          // +0x560c
    int f5610;                                          // +0x5610
    char pad5614[0x5694 - 0x5614];
    vec3 v5694;                                         // +0x5694
    char pad56a0[0x56e8 - 0x56a0];
    float f56e8, f56ec;                                 // +0x56e8
    char pad56f0[0x57c8 - 0x56f0];
    Timer timer57c8;                                    // +0x57c8 (active flag at +0x57d8)
    char pad57e0[0x5800 - 0x57e0];
    gui_timer timer5800;                                // +0x5800
    gui_timer timer5810;                                // +0x5810
    char pad5820[0x5830 - 0x5820];
    int f5830;                                          // +0x5830
    char pad5834[0x5874 - 0x5834];
    int f5874;                                          // +0x5874
    char pad5878[0x5928 - 0x5878];

    ItemRef slot_pair(int idx) const;                   // 0x432790 (thiscall, ret 8)
    bool charged_0x48() const;                          // 0x432470
    float stamina_scale();                              // 0x438180
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void steer(float dt);                               // 0x435f10
    void play_sound_var(int* kind, float* var);         // 0x443730
    void play_hit_sound(int* kind);                     // 0x4435c0
    void play_class_sound(const int* kind);             // 0x4459c0
    void on_wield_reset();                              // 0x445a20
    void restore_pose();                                // 0x4552c0
    unsigned find_talk_target();                        // 0x43b8a0 (fastcall)
    void update_controls(float dt);                     // 0x4556d0

    // 0x4379c0 as cl inlines it (the call sites that pass a constant slot 1)
    __forceinline void set_order_inl(int slot, int id, unsigned prio, float rate)
    {
        if (id >= -1 && id < defs->count) {
            if (order[slot].set(id, prio)) {
                order[slot].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
                if (prio & 0x4000000)
                    body->curve->set_rec_phase(slot, 0.0f);
                if (slot == 0) {
                    if (id >= 0 && (defs->defs[id].flags & 0x400) && order[1].a != -1 &&
                        (prio & 0x1ffff) > (order[1].m14 & 0x1ffff))
                        order[1].set(-1, prio & 0x1ffff);
                } else if (slot == 1) {
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
    }
    __forceinline void order1(int id, unsigned prio) { set_order_inl(1, id, prio, 1.0f); }
};

#define PICK(m2, m3, m1, m0) (mode == 2 ? (m2) : mode == 3 ? (m3) : mode == 1 ? (m1) : (m0))

static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->pages[id >> 4][id & 0xf];
}

// @ 0x4556d0
void Agent::update_controls(float dt)
{
    g_move_fwd = 0.0f;
    g_move_side = 0.0f;
    f56ec = (1.0f - dt * 10.0f) * f56ec;
    f56e8 = (1.0f - dt * 5.0f) * f56e8;
    float t74 = (1.0f - dt * 4.0f) * (f74 - dt * f70 * 0.2f) * 0.9f;
    f74 = t74;
    f70 = t74 + f70;
    if ((order[1].m14 & 0x1ffff) == 0)
        order1(-1, 0);
    if ((int)(order[0].m14 & 0x1ffff) >= 0x40) {
        vec2 z;
        z.x = 0.0f;
        z.y = 0.0f;
        vel = z;
        return;
    }
    vec2 mv;
    float divisor;
    float speed_mul = 1.0f;
    if (flags3c & 1) {
        if ((float)(g_ticks[timer40.clock] - timer40.t) * 1e-05f >= 0.0f) {
            // ---- terrain / night speed multiplier ------------------------------------------------------------------
            Mission* ms = g_mission;
            if (f10 == 0) {
                if (f58 < ms->f654c) {
                    float lo = 0.4f;
                    float v = 1.0f - (ms->f654c - f58) * 0.5f;
                    speed_mul = fmax_ref(&lo, &v);
                } else if (ms->f6604 == 1) {
                    speed_mul = 0.9f;
                }
            } else {
                if (f58 < ms->f654c) {
                    float lo = 0.3f;
                    float v = 1.0f - (ms->f654c - f58) * 0.6f;
                    speed_mul = fmax_ref(&lo, &v);
                } else {
                    if (ms->f6604 == 1)
                        speed_mul = 0.9f;
                    if (g_world->is_night())
                        speed_mul = speed_mul * 0.9f;
                }
            }
            vector4 up(0.0f, 0.0f, 1.0f);
            if (fbc)
                up = fbc->d;

            // ---- input bits -> the two input axes ------------------------------------------------------------------
            unsigned fl = in_flags;
            float side = 0.0f;
            mv.y = 0.0f;
            if (fl & 1) {
                mv.y = 1.0f;
                if (fl & 0x200000)
                    g_move_fwd = 1.0f;
                else
                    g_move_fwd = 0.6f;
            } else if (fl & 2) {
                mv.y = -1.0f;
                g_move_fwd = -0.7f;
                if (!(fl & 0x200000))
                    g_move_fwd = -0.5f;
            }
            if (in_flags & 4) {
                g_move_fwd = g_move_fwd * 0.7f;
                side = 1.0f;
                if (in_flags & 0x200000)
                    g_move_side = 0.9f;
                else
                    g_move_side = 0.55f;
            } else if (in_flags & 8) {
                g_move_fwd = g_move_fwd * 0.7f;
                side = -1.0f;
                g_move_side = -0.9f;
                if (!(in_flags & 0x200000))
                    g_move_side = -0.55f;
            }
            float len2 = mv.y * mv.y + side * side;
            if (len2 > 0.0f) {
                divisor = 1.0f / (float)sqrt(len2);
                float ms = divisor * side;
                float mf = divisor * mv.y;
                if (mf < -0.1f) {
                    ms = ms * 0.7f;
                    mf = mf * 0.7f;
                    if (!(in_flags & 0x200000)) {
                        ms = ms * 0.7f;
                        mf = mf * 0.7f;
                    }
                } else if (!(in_flags & 0x200000)) {
                    ms = ms * 0.6f;
                    mf = mf * 0.6f;
                }
                g_move_side = ms;
                g_move_fwd = mf;
            }

            int mode = 0;
            if (f10 == 0) {
                // ---- on foot: weapon mode, walk animation ----------------------------------------------------------
                {
                    ItemRef a = slot_pair(0);
                    ItemRef b = slot_pair(1);
                    if (a.item >= 0) {
                        ItemKind* ik = &g_items[a.item];
                        mode = 1;
                        unsigned fl;
                        if ((b.item == -1 && (ik->type_flags & 0xff) == 4) || (fl = ik->type_flags, (fl & 0xff) == 9)) {
                            mode = 2;
                        } else if (ik->is_melee() && (fl & 0x200000)) {
                            mode = 3;
                        }
                    }
                }
                if ((order[0].m14 & 0x1ffff) == 0 && mount_id == -1) {
                    int anim;
                    switch (mode) {
                    case 1:
                        anim = 5;
                        break;
                    case 2:
                        if (slot_pair(0).item < 0 || g_items[slot_pair(0).item].type_flags % 256 != 9)
                            anim = 7;
                        else
                            anim = 8;
                        break;
                    case 3:
                        anim = 6;
                        break;
                    default:
                        if (!(f2c4 & 1) && (id != (unsigned)g_mission->local_id ||
                                (!((g_modes[g_mission->cur_mode].flags >> 1) & 1) && !(g_mission->f6528 & 1))))
                            anim = anim34;
                        else
                            anim = 4;
                        break;
                    }
                    set_order(0, anim, 0, 1.0f);
                }
                mv.x = g_move_side;
                mv.y = g_move_fwd;
                divisor = 1.0f;
                float slope = 1.0f;
                vector4 v;
                v.x = mv.x;
                v.y = mv.y;
                v.z = 0.0f;
                v.normalize();
                v = body->m.rot_ext(v);
                v = cross(up, cross(v, up));
                v.normalize();
                if (v.z > 0.01f)
                    slope = 1.0f - v.z * 0.5f;
                if (fa0 > 0.0001f) {
                    slope = (1.0f - fa0 * 0.2f) * slope;
                } else if (fa0 < -0.0001f) {
                    float lo = 0.0f;
                    float vv = fa0 * 0.5f + 0.7f;
                    slope = fmax_ref(&lo, &vv) * slope;
                }
                float mag = (float)sqrt(g_move_fwd * g_move_fwd + g_move_side * g_move_side);
                if (mag > 0.0f) {
                    mv.x = g_move_side * (1.0f / mag);
                    mv.y = (1.0f / mag) * g_move_fwd;
                }
                float lim = stamina_scale() * slope;
                if (mag > speed_mul)
                    mag = speed_mul;
                if (mag < speed_mul * -0.8f)
                    mag = speed_mul * -0.8f;
                if (mag > lim)
                    mag = lim;
                if (mag < lim * -0.8f)
                    mag = lim * -0.8f;
                int off = troop * 0x508;
                Troop* tr = (Troop*)((char*)g_world->troops + off);
                int ath = tr->nibble(0x19);
                float skill = ((60.0f / (f24c + 60.0f)) * (ath * 5.0f + tr->f5c) + 100.0f) * 0.01f;
                v.y = ((mv.y * mag) * 2.8f) * skill;
                if (v.y < 0.0f)
                    v.y = v.y * 0.8f;
                v.x = (((mv.x * mag) * 2.8f) * skill) * 0.95f - vel.x;
                v.y = v.y - vel.y;
                tr = (Troop*)((char*)g_world->troops + off);
                int ath2 = tr->nibble(0x19);
                float step = (((70.0f / (f24c + 70.0f)) * ((25.0f / (f24c + 25.0f)) *
                              ((float)tr->f5c * 0.06666667f + (float)ath2 * 0.2f) + 1.0f)) * dt) * 10.0f;
                float nstep = -step;
                float nstep2 = nstep;
                float nvx = clamp_float(&v.x, &nstep, &step) + vel.x;
                vel.x = nvx;
                float nvy = clamp_float(&v.y, &nstep2, &step) + vel.y;
                vel.y = nvy;
                unsigned band = fb8 & 7;
                float spd = (float)sqrt(vel.y * vel.y + vel.x * vel.x);
                for (;;) {
                    if (spd > g_gait_walk[band].hi)
                        band++;
                    else if (spd < g_gait_walk[band].lo)
                        band--;
                    else
                        break;
                }
                if (band == 2 && nvy < -0.1f)
                    band = 1;
                if (band == 2) {
                    int ath3 = ((Troop*)((char*)g_world->troops + off))->nibble(0x19);
                    divisor = ((float)ath3 + 30.0f) * 0.07f;
                    if (fabs(nvy) * 0.5f > fabs(nvx)) {
                        if (nvy > 0.0f)
                            set_order(0, PICK(0x13, 0x14, 0x12, 0x11), 0x100000, 1.0f);
                        else
                            set_order(0, PICK(0x1f, 0x20, 0x1e, 0x1d), 0x100000, 1.0f);
                    } else {
                        float h = (float)fabs(nvx) * 0.5f;
                        if (nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x17, 0x18, 0x16, 0x15), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x1b, 0x1c, 0x1a, 0x19), 0x100000, 1.0f);
                        } else if (-nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x38, 0x37, 0x36, 0x35), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x34, 0x33, 0x32, 0x31), 0x100000, 1.0f);
                        } else if (nvx > 0.0f) {
                            set_order(0, PICK(0x3c, 0x3b, 0x3a, 0x39), 0x100000, 1.0f);
                        } else {
                            set_order(0, PICK(0x40, 0x3f, 0x3e, 0x3d), 0x100000, 1.0f);
                        }
                    }
                } else if (band == 1) {
                    divisor = 1.5f;
                    if (fabs(nvy) * 0.5f > fabs(nvx)) {
                        if (nvy > 0.0f)
                            set_order(0, PICK(0xb, 0xc, 10, anim38), 0x100000, 1.0f);
                        else
                            set_order(0, PICK(0xf, 0x10, 0xe, 0xd), 0x100000, 1.0f);
                    } else {
                        float h = (float)fabs(nvx) * 0.5f;
                        if (nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x2c, 0x2b, 0x2a, 0x29), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x30, 0x2f, 0x2e, 0x2d), 0x100000, 1.0f);
                        } else if (-nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x38, 0x37, 0x36, 0x35), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x34, 0x33, 0x32, 0x31), 0x100000, 1.0f);
                        } else if (nvx > 0.0f) {
                            set_order(0, PICK(0x24, 0x23, 0x22, 0x21), 0x100000, 1.0f);
                        } else {
                            set_order(0, PICK(0x28, 0x27, 0x26, 0x25), 0x100000, 1.0f);
                        }
                    }
                }
                fb4 = spd / divisor;
                if (fb4 > 0.001f)
                    order[0].f1c = 1.0f / fb4;
                else
                    order[0].f1c = 1.0f;
                fb8 = (fb8 & 0xfffffff8) | band;
            }
            // ---- mount: rider skill, acceleration, gait band ------------------------------------------------------
            int rid = rider_id;
            int rskill = 5;
            float rmul = 1.0f;
            if (rid != -1) {
                rskill = g_world->troops[agent_by_id(rid)->troop].nibble(0x18);
                rmul = ((float)rskill + 25.0f) * 0.037037037f;
            }
            float sgn = 0.0f;
            if (f10 == 1) {
                unsigned fl2 = in_flags;
                if (fl2 & 0x40) {
                    sgn = -1.0f;
                } else if ((char)fl2 < 0) {
                    sgn = 1.0f;
                }
                if (rid >= 0) {
                    if (((AgentTable*)((char*)g_mission + 4))->get(rid)->charged_0x48())
                        sgn = sgn * 0.7f;
                }
                float lim = 4.0f;
                if (vel.y > 4.0f)
                    lim = vel.y;
                int s1 = mount_item.stat_1f0b() + 1;
                float acc = ((((float)s1 / lim) * sgn) * rmul) * dt + f56e8;
                f56e8 = acc;
                f74 = f74 - ((acc * vel.y) * dt) * 0.007f;
                ((vec2*)&dir)->rotate_b(acc * dt);
                if (in_flags & 0x10) {
                    if (f10 == 1) {
                        unsigned bb = fb8;
                        if ((bb & 0x300) == 0) {
                            fb8 = bb | 0x100;
                            int s2 = mount_item.stat_1f0b() - 1;
                            vel.y = ((float)s2 * rmul) * 0.04f;
                        } else if (bb & 0x100) {
                            int s2 = mount_item.stat_1f0b();
                            vel.y = (((18.0f - vel.y) * ((float)s2 * ((float)rskill + 3.0f))) * dt) * 0.0006f + vel.y;
                        } else if (bb & 0x200) {
                            float nf = dt + vel.y;
                            vel.y = nf;
                            if (nf > -0.9f)
                                vel.y = 0.0f;
                        }
                    }
                } else if (in_flags & 0x20) {
                    if (f10 != 0)
                        order1(0x1a1, 0);
                    unsigned bb = fb8;
                    if ((bb & 0x300) == 0) {
                        bb = bb | 0x200;
                        vel.y = -0.9f;
                        fb8 = bb;
                    } else if (bb & 0x200) {
                        vel.y = vel.y - dt * 0.5f;
                    } else if (bb & 0x100) {
                        int s2 = mount_item.stat_1f0b();
                        float nf = vel.y - ((float)s2 * dt) * 0.08f;
                        vel.y = nf;
                        if (nf < 0.9f)
                            vel.y = 0.0f;
                    }
                } else {
                    float nf = (1.0f - dt * 0.01f) * vel.y;
                    vel.y = nf;
                    if (fabs(nf) < 0.9f)
                        vel.y = 0.0f;
                    if (vel.y == 0.0f)
                        fb8 = fb8 & 0xfffffcff;
                }
                dir.normalize();
                int s3 = mount_item.stat_1f4() + 1;
                vector4* fr = &body->m.f;
                float cap = ((float)rskill * 0.02f + 1.0f) * (((float)s3 * speed_mul) * 0.19f);
                vector4 v;
                v.x = vel.y * fr->x;
                v.y = fr->y * vel.y;
                v.z = fr->z * vel.y;
                v = cross(up, cross(v, up));
                v.normalize();
                float lim2;
                if (v.z > 0.01f) {
                    lim2 = (1.0f - v.z * 0.9f) * cap;
                } else {
                    lim2 = cap;
                    if (v.z < -0.27f)
                        lim2 = (1.0f - (-0.27f - v.z) * 1.3f) * cap;
                }
                if (vel.y < -1.0f)
                    vel.y = -1.0f;
                else if (lim2 < vel.y)
                    vel.y = lim2;
                unsigned band = fb8 & 7;
                float div2 = 1.0f;
                if (vel.y > g_gait_mount[band].hi)
                    band++;
                else if (vel.y < g_gait_mount[band].lo)
                    band--;
                if ((int)band < 0)
                    band = 0;
                else if ((int)band > 5)
                    band = 5;
                if (band == 5) {
                    set_order(0, 0x19a, 0x100000, 1.0f);
                    div2 = 5.0f;
                    if (rider_id == g_mission->local_id) {
                        if (timer5810.elapsed() > 1.2f)
                            timer5810.restart();
                    }
                } else if (band == 4) {
                    set_order(0, 0x199, 0x100000, 1.0f);
                    div2 = 3.2f;
                } else if (band == 3) {
                    set_order(0, 0x198, 0x100000, 1.0f);
                    div2 = 2.0f;
                } else if (band == 2) {
                    set_order(0, 0x197, 0x100000, 1.0f);
                    div2 = 1.7f;
                } else if (band == 0) {
                    set_order(0, 0x19b, 0, 1.0f);
                    div2 = 1.0f;
                } else {
                    set_order(0, 0x196, 0, 1.0f);
                    if (timer5810.elapsed() > 1.2f) {
                        if (timer5810.elapsed() > 30.2f || frand() < 0.05f) {
                            float var = 0.0f;
                            int kind = 0x6c;
                            play_sound_var(&kind, &var);
                        }
                        timer5810.restart();
                    }
                }
                float r = vel.y / div2;
                fb4 = r;
                if (r != 0.0f)
                    order[0].f1c = 1.0f / r;
                else
                    order[0].f1c = 1.0f;
                fb8 = (fb8 & 0xfffffff8) | band;
                if (f56e8 < -0.1f) {
                    if (band == 1)
                        set_order(0, 0x19f, 0, 1.0f);
                    order1(0x19f, 0);
                } else if (f56e8 > 0.1f) {
                    if (band == 1)
                        set_order(0, 0x1a0, 0, 1.0f);
                    order1(0x1a0, 0);
                }
            }
            // ---- jump / rear-up ------------------------------------------------------------------------------------
            if ((in_flags & 0x100000) && timer5800.elapsed() >= 0.0f) {
                if (f10 == 0) {
                    if (mount_id == -1) {
                        flags3c = flags3c & 0xfffffffe;
                        { float c5 = v5694.z + 5.0f; v5694.set(v5694.x, v5694.y, c5); }
                        set_order(0, 2, 0x1000002, 1.0f);
                        float var = 0.1f;
                        if (f58 < g_mission->f654c) {
                            int kind = 0x1f;
                            play_sound_var(&kind, &var);
                        } else {
                            int kind = 0x1d;
                            play_sound_var(&kind, &var);
                        }
                        timer5800.set_delay(-2.0f);
                    }
                } else if (vel.y > 3.5f) {
                    flags3c = flags3c & 0xfffffffe;
                    { float c6 = v5694.z + 6.0f; v5694.set(v5694.x, v5694.y, c6); }
                    set_order(0, 0x19d, 0x1000002, 1.0f);
                    float var = 0.1f;
                    if (f58 < g_mission->f654c) {
                        int kind = 0x23;
                        play_sound_var(&kind, &var);
                    } else {
                        int kind = 0x21;
                        play_sound_var(&kind, &var);
                    }
                    timer5800.set_delay(-4.0f);
                }
            }
            if ((in_flags & 0x40000) && f10 != 0) {
                int kind = 4;
                play_hit_sound(&kind);
                set_order(0, 0x19c, 0x100004a, 1.0f);
            }
        }
    }
    // ---- heading ------------------------------------------------------------------------------------------------
    if ((int)id == g_mission->local_id && mount_id == -1) {
        if (f55d4 != 0) {
            dir = dir55d8;
        } else {
            if (f560c != 0 || f5610 != 0 || (in_flags & 0xffdfffff) != 0)
                steer(dt);
        }
    } else if (f10 != 1 && mount_id == -1) {
        steer(dt);
    }
    // ---- riding: mount reactions, mount / dismount --------------------------------------------------------------
    unsigned mid = mount_id;
    if ((int)mid >= 0) {
        Agent* h = agent_by_id(mid);
        switch (h->order[0].a) {
        case 0x197: set_order(0, 0x42, 0, 1.0f); break;
        case 0x198: set_order(0, 0x45, 0, 1.0f); break;
        case 0x199: set_order(0, 0x46, 0, 1.0f); break;
        case 0x19a: set_order(0, 0x47, 0, 1.0f); break;
        case 0x19c: set_order(0, 0x49, 0x1000040, 1.0f); break;
        case 0x19d: set_order(0, 0x4b, 0, 1.0f); break;
        case 0x19e: set_order(0, 0x4c, 0, 1.0f); break;
        case 0x1a0: set_order(0, 0x4e, 0, 1.0f); break;
        case 0x19f: set_order(0, 0x4d, 0, 1.0f); break;
        default:
            order[0].set(0x41, 0);
            break;
        }
        if (f130 & 8) {
            Agent* h2 = agent_by_id(mount_id);
            if (h2->vel.y > 5.5f) {
                order1(0x48, 0);
            } else if (h2->order[0].a == 0x196) {
                order1(0x51, 0);
            } else {
                order1(0x43, 0);
            }
        }
    }
    if ((in_flags & 0x80000) && (fb8 & 0x100) && f10 == 0 && mount_id >= 0 &&
        ((AgentTable*)((char*)g_mission + 4))->get(mount_id)->vel.y < 5.2f)
        set_order(0, 0x4a, 0x1000002, 1.0f);
    unsigned fl = in_flags;
    if ((fl & 0x10000) && mount_id >= 0 && timer57c8.active == 0) {
        timer57c8.begin(2.0f);
        set_order(0, 0x50, 0x1000040, 1.0f);
        order1(-1, 0x1f);
        f560c = 0;
        f5610 = 0;
        agent_by_id(mount_id)->rider_id = -1;
        agent_by_id(mount_id)->f2c8 = 0;
        agent_by_id(mount_id)->f2c0 = 10.0f;
        agent_by_id(mount_id)->timer2b0.restart();
        mount_id = -1;
        restore_pose();
    } else if (!(fl & 0x20000) || mount_id != -1) {
        if ((fl & 0xff000000) && (fl & 0xff000000) == 0x1000000 && f10 == 0 && order[1].a != 0x127 && f5874 < 3) {
            order1(0x127, 0x1000040);
            f5874++;
            int kind = 6;
            play_class_sound(&kind);
        }
    } else {
        unsigned tg;
        if (f7c == 2)
            tg = target_id;
        else
            tg = find_talk_target();
        if ((int)tg >= 0 && agent_by_id(tg)->rider_id == -1) {
            ((AgentTable*)((char*)g_mission + 4))->get(tg)->f5830 = 2;
            ((AgentTable*)((char*)g_mission + 4))->get(tg)->rider_id = id;
            mount_id = tg;
            set_order(0, 0x4f, 0x1000040, 1.0f);
            order1(-1, 0x1f);
            f560c = 0;
            f5610 = 0;
            restore_pose();
            if (wield_a >= 0 && held[wield_a].item >= 0 && (g_items[held[wield_a].item].type_flags & 0x10000000)) {
                wield_a = -1;
                on_wield_reset();
            }
            if (wield_b >= 0 && held[wield_b].item >= 0) {
                unsigned tf = g_items[held[wield_b].item].type_flags;
                if ((tf & 0xff) == 7 && (tf & 0x10000000)) {
                    wield_b = -1;
                    on_wield_reset();
                    order1(0x6d, 0x5000046);
                }
            }
        }
    }
    timer57c8.expired();
}
