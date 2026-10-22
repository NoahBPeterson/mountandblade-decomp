// Slice s005284a0: the mission player-view / aim controller's per-frame update (0x5284a0, 8192 bytes).
//
// It damps the view yaw/pitch (+0x268/+0x26c) from the look input, clamps the pitch, the look speed
// (+0x278) and the field of view (+0x288, 37 .. 90/97 depending on the sheathed flag), builds the view
// frame and hands its forward row to the local agent's look direction (+0x55d8), and (while the game is
// not paused) maps the bound keys into the agent's movement / action bits (+0x55d0), runs the slot
// cycling keys, toggles the drawn-weapon state, and turns the three "action" keys into Blow records for
// the agents under the crosshair.  `this` is the mission view object (same object as s004f7f00's
// MissionObj: +0x268 yaw, +0x2c8 mode, +0x2d4 aim target, +0x2fc snap counter); names are provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- item records ------------------------------------------------------------------------------------
struct ItemKind {                                       // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
};
extern ItemKind* g_items;                               // 0x5e29d0

struct ItemRef {                                        // item index + packed amount/modifier
    int item;
    unsigned m4;
};

// ---- the blow record (0x58 bytes, ctor 0x4353a0) -----------------------------------------------------
struct Blow {
    Blow();                                             // 0x4353a0
    char pad0[4];
    int attacker;                                       // +0x04
    vector4 pos;                                        // +0x08 (position of the agent hit)
    char pad18[0x38 - 0x18];
    int f38;                                            // +0x38
    float damage;                                       // +0x3c
    char pad40[0x48 - 0x40];
    int f48;                                            // +0x48
    char pad4c[0x58 - 0x4c];
};

// ---- the mission / agent objects (layouts reused from s004556d0, s004f7f00) --------------------------
struct Agent;
struct AgentTable {
    int count;                                          // +0x00
    char pad4[0x18 - 4];
    Agent** pages;                                      // +0x18 (first page pointer; mission+0x1c)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
    Agent* get(unsigned id);                            // 0x43b1f0
};

struct Agent {                                          // 0x5928 bytes
    char pad0[4];
    int used;                                           // +0x04
    char pad8[8];
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x2c - 0x18];
    int mount_id;                                       // +0x2c (-1 on foot)
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    vector4 dir;                                        // +0x60 (facing direction)
    char pad70[0x7c - 0x70];
    int f7c;                                            // +0x7c (drawn-weapon state)
    char pad80[0x100 - 0x80];
    int f100;                                           // +0x100
    char pad104[0x118 - 0x104];
    float f118;                                         // +0x118
    char pad11c[0x22c - 0x11c];
    int wield_a;                                        // +0x22c
    char pad230[0x2c8 - 0x230];
    int mode;                                           // +0x2c8 (aim mode)
    int f2cc;                                           // +0x2cc (aim target)
    char pad2d0[0x2d4 - 0x2d0];
    int f2d4;                                           // +0x2d4
    char pad2d8[0x31c - 0x2d8];
    unsigned target_id;                                 // +0x31c
    char pad320[0x55d0 - 0x320];
    unsigned in_flags;                                  // +0x55d0 (movement / action bits)
    char f55d4;                                         // +0x55d4
    char pad55d5[0x55d8 - 0x55d5];
    vector4 dir55d8;                                    // +0x55d8 (look direction)
    char pad55e8[0x55ec - 0x55e8];
    int f55ec;                                          // +0x55ec
    char pad55f0[0x560c - 0x55f0];
    int f560c;                                          // +0x560c
    int f5610;                                          // +0x5610
    int f5614;                                          // +0x5614
    char pad5618[0x5720 - 0x5618];
    int f5720;                                          // +0x5720
    int f5724;                                          // +0x5724
    char pad5728[0x5928 - 0x5728];

    ItemRef slot_pair(int idx);                         // 0x432790 (ret 8)
    bool can_attack(Agent* o);                          // 0x432350 (ret 4)
    bool can_attack2(Agent* o);                         // 0x4323a0 (ret 4)
    void note_blow(Blow* b);                            // 0x457af0 (ret 4)
    void cycle_slot(int slot);                          // 0x44bc30 (ret 4)
    void on_wield_reset();                              // 0x445a20
};

struct Mission {
    char pad0[4];
    AgentTable agents;                                  // +0x04
    char pad20[0x5c - 0x20];
    int selected;                                       // +0x5c (local agent id)
};
extern Mission* g_mission;                              // 0x5e292c

struct Game {
    char pad0[0x139e4];
    int f139e4;                                         // +0x139e4 (weapons sheathed)
    char pad139e8[0x1a090 - 0x139e8];
    int f1a090;                                         // +0x1a090 (mission paused / cutscene)
};
extern Game* g_game;                                    // 0x5e2930

// ---- the view object (`this`) -----------------------------------------------------------------------
struct ViewCtrl {
    char pad0[0x44];
    unsigned char flags44;                              // +0x44 (bit 0: keyboard/analog look input)
    char pad45[0x268 - 0x45];
    float yaw;                                          // +0x268
    float pitch;                                        // +0x26c
    float yaw_rate;                                     // +0x270
    float pitch_rate;                                   // +0x274
    float look_speed;                                   // +0x278 (0.7 .. 10)
    char pad27c[0x288 - 0x27c];
    float fov;                                          // +0x288 (37 .. 90 / 97)
    char pad28c[0x2c8 - 0x28c];
    int mode;                                           // +0x2c8 (aim mode)
    int target;                                         // +0x2cc (aim target)
    char pad2d0[0x2d4 - 0x2d0];
    int aim_valid;                                      // +0x2d4
    char pad2d8[0x2fc - 0x2d8];
    int snap;                                           // +0x2fc (frames left to snap the view)

    void update(float dt);                              // 0x5284a0
    void update_weapon_display();                       // 0x4f8b40
};

// a view of rgl::matrix with the engine's rotation helpers (thiscall, ecx = matrix)
struct MatrixExt {
    void rotate_sf(float* angle);                       // 0x40e770
    void rotate_fu(float* angle);                       // 0x40e570
};
void identity3(matrix* m);                              // 0x40cd60

// ---- key tables -------------------------------------------------------------------------------------
struct KeyTable {                                       // 0xa73730
    char has_free_origin(int key);                      // 0x48ac90 (ret 4)
    char has_origin(int key);                           // 0x48ac50 (ret 4)
};
extern KeyTable g_keys;                                 // 0xa73730
struct KeyNames {                                       // 0xa170c8
    char bound(int action);                             // 0x487900 (ret 4)
};
extern KeyNames g_key_names;                            // 0xa170c8
extern int g_key_down[];                                // 0xa170d4 (per-key down state)

// ---- globals ----------------------------------------------------------------------------------------
extern int g_a17188;                                    // 0xa17188
extern int g_a17128;                                    // 0xa17128
extern int g_a17184;                                    // 0xa17184
extern int g_a1712c;                                    // 0xa1712c
extern int g_a17190;                                    // 0xa17190
extern int g_a1715c;                                    // 0xa1715c
extern int g_a17124;                                    // 0xa17124
extern int g_a17454;                                    // 0xa17454
extern int g_a1717c;                                    // 0xa1717c
extern int g_5a9fbc;                                    // 0x5a9fbc
extern int g_a17148;                                    // 0xa17148
extern int g_a17348;                                    // 0xa17348
extern int g_a171d0;                                    // 0xa171d0
extern int g_a175d0;                                    // 0xa175d0
extern int g_a171d4;                                    // 0xa171d4
extern int g_a175d4;                                    // 0xa175d4
extern int g_a171c8;                                    // 0xa171c8
extern int g_a175c8;                                    // 0xa175c8
extern int g_a17160;                                    // 0xa17160
extern int g_a17560;                                    // 0xa17560
extern int g_a73790;                                    // 0xa73790
extern int g_a73794;                                    // 0xa73794
extern int g_a73798;                                    // 0xa73798
extern int g_a7379c;                                    // 0xa7379c
extern int g_5e299c;                                    // 0x5e299c
extern int g_invert;                                    // 0x5aa01c
extern int g_ctl_mode;                                  // 0x5aa074 (0 = target, 1 = mouse, 2 = mirrored)
extern int g_stick_mode;                                // 0x5aa078
extern float g_step;                                    // 0x5a9c20 (per-clock step, seconds)
extern float g_time_scale;                              // 0x5aa018
extern float g_axis_x;                                  // 0x5d7094 (gamepad x axis, 0..1)
extern float g_axis_y;                                  // 0x5d7098
extern float g_analog_x;                                // 0xa178d4 (mouse dx this frame)
extern float g_analog_y;                                // 0xa178d8

#define AGENT(id) (g_mission->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))

// @ 0x5284a0 ------------------------------------------------------------------------------------------
void ViewCtrl::update(float dt)
{
    if (g_game->f1a090) {
        if (g_a17188) return;
        if (g_a17128) return;
        if (g_a17184) return;
        if (g_a1712c) return;
        if (g_a17190) return;
        if (g_a1715c) return;
        if (g_a17124) return;
        if (g_game->f1a090)
            dt = g_step;
    }
    float k = expf(g_time_scale * 4.0f) * 0.04f;

    // ---- aiming at an agent while a melee weapon (or the fists) is out is slower ---------------------
    if (!g_game->f1a090) {
        if (g_5e299c == 0 || (flags44 & 1)) {
            if (g_mission->selected >= 0) {
                Agent* a = AGENT(g_mission->selected);
                ItemRef w0 = a->slot_pair(0);
                ItemRef w1 = a->slot_pair(1);
                bool hand0;
                if (w0.item < 0) {
                    hand0 = true;
                } else {
                    int t = g_items[w0.item].type_flags & 0xff;
                    hand0 = (t == 2 || t == 3 || t == 4);
                }
                if (((hand0 && a->f560c == 1 && g_ctl_mode > 0 && a->f100 == a->f55ec) ||
                     (w1.item < 0 && a->f5610 > 0 && g_stick_mode > 0 && a->f100 == a->f5614)) &&
                    a->f118 < 0.6f)
                    k *= 0.1f;
            }
        }
    }

    // ---- look input ---------------------------------------------------------------------------------
    float look_x, look_y;
    if (g_5e299c == 0 || (flags44 & 1)) {
        look_x = g_analog_x;
        look_y = g_analog_y;
    } else {
        look_x = 0.0f;
        look_y = 0.0f;
        if (g_axis_x <= 0.01f)
            look_x = dt * -400.0f;
        else if (g_axis_x >= 0.99f)
            look_x = dt * 400.0f;
        if (g_axis_y <= 0.01f)
            look_y = dt * 400.0f;
        if (g_axis_y >= 0.74f)
            look_y = dt * -400.0f;
    }

    // ---- damp the view rates and integrate them into yaw / pitch ------------------------------------
    if (!g_game->f1a090 || g_a17454 != 0) {
        float r = 1.0f - dt * 20.0f;
        if (r < 0.0f)
            r = 0.0f;
        yaw_rate = yaw_rate * r;
        pitch_rate = pitch_rate * r;
        yaw_rate = yaw_rate + look_x * fov * 0.0018f;
        if (g_invert)
            look_y = -look_y;
        pitch_rate = pitch_rate + look_y * fov * 0.0018f;
        yaw = yaw - yaw_rate * k * dt;
        pitch = pitch - pitch_rate * k * dt;
    }

    // ---- clamps -------------------------------------------------------------------------------------
    if (pitch < -0.9519978165626526f)
        pitch = -0.9519978165626526f;
    else if (pitch > 0.9519978165626526f)
        pitch = 0.9519978165626526f;
    if (look_speed < 0.7f)
        look_speed = 0.7f;
    else if (look_speed > 10.0f)
        look_speed = 10.0f;

    if (g_mission->selected < 0)
        return;

    // ---- build the view frame and hand its forward row to the local agent ----------------------------
    Agent* a;
    {
        matrix m;
        identity3(&m);
        ((MatrixExt*)&m)->rotate_sf(&yaw);
        ((MatrixExt*)&m)->rotate_fu(&pitch);

        a = AGENT(g_mission->selected);
        bool looking = (g_a73798 >= 0 && g_key_down[g_a73798] != 0);
        if ((g_a7379c < 0 || g_key_down[g_a7379c] == 0) && !looking)
            a->dir55d8 = m.f;
    }

    if (snap < 5) {
        a->dir55d8 = a->dir;
        yaw = (float)atan2(-a->dir.x, a->dir.y);
        pitch = 0.0f;
        snap++;
    }

    // ---- movement / action bits ---------------------------------------------------------------------
    a->in_flags = 0;
    if (!g_game->f1a090) {
        if (g_a1717c == 0)
            a->in_flags = 0x200000;
        if (g_keys.has_free_origin(5))
            a->in_flags |= 0x100000;

        if (a->mount_id < 0) {
            if (g_keys.has_origin(0))
                a->in_flags |= 1;
            else if (g_keys.has_origin(1))
                a->in_flags |= 2;
            if (g_keys.has_origin(3))
                a->in_flags |= 4;
            else if (g_keys.has_origin(2))
                a->in_flags |= 8;
            if (g_keys.has_free_origin(4) && mode == 2 && aim_valid != 0) {
                a->in_flags |= 0x20000;
                a->target_id = target;
            }
        } else {
            if (g_keys.has_free_origin(0))
                a->in_flags |= 0x80000;
            if (g_keys.has_origin(0))
                a->in_flags |= 0x10;
            else if (g_keys.has_origin(1))
                a->in_flags |= 0x20;
            if (g_keys.has_origin(3))
                a->in_flags |= 0x40;
            else if (g_keys.has_origin(2))
                a->in_flags |= 0x80;
            if (g_keys.has_free_origin(4) && mode == 3)
                a->in_flags |= 0x10000;
            if (g_key_names.bound(0x24) && (g_a17148 != 0 || g_a17348 != 0))
                a->in_flags |= 0x40000;
        }

        if (g_5e299c == 0 || (flags44 & 1)) {
            if (g_keys.has_origin(6))
                a->in_flags |= 0x100;
            if (g_keys.has_free_origin(6))
                a->in_flags |= 0x400;
            if (g_keys.has_origin(7))
                a->in_flags |= 0x200;
        }

        if (g_keys.has_free_origin(8)) {
            a->cycle_slot(0);
        } else if (g_keys.has_free_origin(9)) {
            a->cycle_slot(1);
        } else if (g_keys.has_free_origin(10)) {
            a->wield_a = -1;
            a->on_wield_reset();
        }
    }

    if (g_keys.has_free_origin(0xe)) {
        g_game->f139e4 = (g_game->f139e4 == 0);
        update_weapon_display();
    }
    AGENT(g_mission->selected)->f55d4 = (g_game->f139e4 != 0);

    // ---- field of view ------------------------------------------------------------------------------
    bool zoom_key = (g_a73790 >= 0 && g_key_down[g_a73790] != 0);
    if ((g_a73794 >= 0 && g_key_down[g_a73794] != 0) || zoom_key)
        fov = fov - dt * 350.0f;
    else
        fov = fov + dt * 350.0f;

    float limit = g_game->f139e4 ? 90.0f : 97.0f;
    if (fov < 37.0f)
        fov = 37.0f;
    else if (fov > limit)
        fov = limit;

    // ---- the "use / attack" keys walk the agent table and hit the agent under the crosshair ---------
    if (g_5a9fbc != 0 && (g_a17148 != 0 || g_a17348 != 0) && g_a171d0 != 0 && g_a175d0 == 0) {
        Agent* t = AGENT(g_mission->selected);
        int old = t->f7c;
        t->f7c = (old == 2) ? 1 : 2;
    }

    if (g_5a9fbc != 0 && g_a17148 != 0 && g_key_names.bound(0x3e)) {
        AgentTable* t = &g_mission->agents;
        bool done = false;
        unsigned i = t->first_used();
        while (i < (unsigned)t->count && !done) {
            Agent* o = AGENT(i);
            if (o->f14 == 1 && o->f10 == 0 && AGENT(g_mission->selected)->can_attack(o)) {
                Blow b;
                b.f48 = 2;
                b.f38 = 0;
                b.attacker = g_mission->selected;
                b.pos = AGENT(i)->pos;
                b.damage = 100.0f;
                AGENT(i)->note_blow(&b);
                if (g_a1717c == 0)
                    done = true;
            }
            t->next_used(&i);
        }
    }

    if (g_5a9fbc != 0 && g_a17148 != 0 && g_a171d4 != 0 && g_a175d4 == 0) {
        AgentTable* t = &g_mission->agents;
        bool done = false;
        unsigned i = t->first_used();
        while (i < (unsigned)t->count && !done) {
            Agent* o = AGENT(i);
            if (i != (unsigned)g_mission->selected && o->f14 == 1 && o->f10 == 0 &&
                AGENT(g_mission->selected)->can_attack2(o)) {
                Blow b;
                b.f48 = 2;
                b.f38 = 0;
                b.attacker = g_mission->selected;
                b.pos = AGENT(i)->pos;
                b.damage = 100.0f;
                AGENT(i)->note_blow(&b);
                if (g_a1717c == 0)
                    done = true;
            }
            t->next_used(&i);
        }
    }

    if (g_5a9fbc != 0 && g_a17148 != 0 && g_a171c8 != 0 && g_a175c8 == 0) {
        Blow b;
        b.f48 = 2;
        b.f38 = 0;
        b.attacker = g_mission->selected;
        b.pos = AGENT(g_mission->selected)->pos;
        b.damage = 200.0f;
        AGENT(g_mission->selected)->note_blow(&b);
    }

    if (g_a17148 != 0 || g_a17348 != 0) {
        if (g_a17160 != 0 && g_a17560 == 0 && g_5a9fbc != 0) {
            Agent* t = AGENT(g_mission->selected);
            t->f5724 = t->f5720;
        }
    }
}
