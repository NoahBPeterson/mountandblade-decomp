// Slice s004461e0: Agent weapon-mode selection (0x4461e0) and attack-start timing (0x4464d0).
// Class and field names are provisional; layout copied from s00437f00 (Agent/Pair/ItemKind/Game/World).
#include <stdlib.h>

struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x214 - 0x1cc];
    unsigned f214, f218;                        // +0x214, +0x218
    char pad21c[0x238 - 0x21c];
    bool uses_ammo();                           // 0x433d90 (ecx = ItemKind*)
};
extern ItemKind* g_items;                       // 0x5e29d0

struct Pair {
    int a;
    unsigned b;
    int stat_21x(int which);                    // 0x434cb0
};

struct ClassRec { int a; int b; char pad[0x6c - 8]; };
struct TeamRec { ClassRec cls[3]; char pad[0x16c - 0x144]; };
struct Game {
    char pad0[0x528c];
    TeamRec teams[8];                           // +0x528c
};
extern Game* g_game;                            // 0x5e292c

struct Troop {
    char pad0[0x30];
    unsigned __int64 flags;                     // +0x30
    char pad38[0x508 - 0x38];
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                              // +0x13a00 (stride 0x508)
};
extern World* g_world;                          // 0x5e2930

struct Agent {
    char pad0[0x2c];
    int m2c;                                    // +0x2c (-1 = on foot)
    char pad30[0x88 - 0x30];
    int troop;                                  // +0x88
    char pad8c[0x1c4 - 0x8c];
    int team;                                   // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    Pair held[11];                              // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c
    char pad240[0x2c0 - 0x240];
    float attack_delay;                         // +0x2c0
    char pad2c4[0x2c8 - 0x2c4];
    int attack_kind;                            // +0x2c8
    char pad2cc[0x5398 - 0x2cc];
    int f5398;                                  // +0x5398
    char pad539c[0x53bc - 0x539c];
    unsigned f53bc;                             // +0x53bc
    char pad53c0[0x56d4 - 0x53c0];
    int weapon_mode;                            // +0x56d4

    Pair best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880 (s004362e0)
    bool has_weapon_for(int mode);              // 0x437f00
    void refresh_weapon_mode();                 // 0x445a20
    void set_weapon_mode(int mode);             // 0x4461e0
    void start_attack(int kind);                // 0x4464d0
};

// @ 0x4461e0
void Agent::set_weapon_mode(int mode)
{
    if (weapon_mode != mode) {
        int hit = -1;
        int hit2 = -1;
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
        for (int i = 3; i >= 0; i--) {
            Pair* p = &held[i];
            if (p->a >= 0) {
                ItemKind* ik = &g_items[p->a];
                bool ok = ((ik->f218 & 0x300) == 0x200 || p->stat_21x(0) <= 1) &&
                          ((ik->f214 & 0x300) == 0x200 || p->stat_21x(1) <= 1);
                if ((ra == 0 || ok) &&
                    (rb == 0 || ((ik->type_flags & 0xff) != 8 && (ik->type_flags & 0xff) != 9 &&
                                 (ik->type_flags & 0xff) != 10 && (ik->type_flags & 0xff) != 0x10 &&
                                 (ik->type_flags & 0xff) != 0x11)) &&
                    (m2c < 0 || !(ik->type_flags & 0x10000000))) {
                    if (mode == 1) {
                        unsigned fl = ik->type_flags;
                        unsigned type = fl & 0xff;
                        if ((type == 2 || type == 3 || type == 4) && (fl & 0x400000))
                            hit = i;
                    } else if (mode == 2) {
                        if (ik->uses_ammo()) {
                            int missing, total, slot_out;
                            best_ammo(i, &missing, &total, &slot_out);
                            if (missing > 0) {
                                if (!(ik->type_flags & 0x100000) || m2c == -1)
                                    hit = i;
                            }
                        }
                    } else if (mode == 3) {
                        if ((char)ik->type_flags == 4)
                            hit = i;
                    }
                }
                if ((char)ik->type_flags == 7 && (int)(p->b & 0xffff) > 1)
                    hit2 = i;
            }
        }
        if (hit != -1)
            slots[0] = hit;
        else if (mode == 1)
            slots[0] = -1;
        if (hit2 != -1)
            slots[1] = hit2;
        weapon_mode = mode;
        f5398 = 0;
        f53bc |= 0x200;
        refresh_weapon_mode();
    }
}

// @ 0x4464d0
void Agent::start_attack(int kind)
{
    attack_kind = kind;
    float scale = 5.0f;
    switch (kind) {
    case 4:
        if (!has_weapon_for(1)) {
            if (has_weapon_for(3)) {
                set_weapon_mode(3);
                scale = 8.0f;
            } else {
                set_weapon_mode(1);
                scale = 8.0f;
            }
        } else {
            set_weapon_mode(1);
            scale = 8.0f;
        }
        break;
    case 7:
        if (has_weapon_for(3)) {
            set_weapon_mode(3);
            scale = 9.0f;
        } else {
            set_weapon_mode(1);
            scale = 9.0f;
        }
        break;
    case 5:
    case 6:
        set_weapon_mode(2);
        // fall through
    case 11:
    case 13:
    case 14:
        scale = 10.0f;
        break;
    case 8:
        scale = 2.0f;
        break;
    // cases 9, 10 and 12 need their own (empty after optimisation) targets for cl to emit a plain dword jump table
    case 9:
        scale = scale;
        break;
    case 10:
        scale = scale;
        break;
    case 12:
        scale = scale;
        break;
    }
    attack_delay = ((float)(rand() % 0x3dc9) * 3.161156e-05f + 0.75f) * scale;
}
