// mission_ctx.h: the mission start-up object (global at 0xa73300; the slices called it MissionCtx, Unit, Screen's
// "MissionUi", InventoryPanel and g_panel; trigger_vec.h MetaMission, world_data.h BattleW), the map screen (Screen) and
// the small records of the mission start (code 0x513710..0x516ac0). Layout: every offset cites the function that
// proves it. Names provisional. The game data is read through GameDataV / GD_I (inv_common.h), the mission object
// through MissionV (a view of src/game/mission.h's Mission with the two methods of 0x5143f0 / 0x5144d0).
#ifndef GAME_INVENTORY_PANEL_MISSION_CTX_H
#define GAME_INVENTORY_PANEL_MISSION_CTX_H
#include "./inv_common.h"
#include "../mission.h"

// a menu screen object: vtable (slot 4 = +0x10), state at +0x04, "highlight" counter at +0x18 (0x513710, 0x514d40)
struct MenuWidget {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4();                                  // +0x10
    int f4;                                             // +0x04
    char pad8[0x14 - 0x8];
    int f14;                                            // +0x14 (0x514d40)
    int f18;                                            // +0x18
};
extern MenuWidget* g_w94;                               // 0xa73994
extern MenuWidget* g_w98;                               // 0xa73998
extern MenuWidget* g_wA0;                               // 0xa739a0
extern MenuWidget* g_wA8;                               // 0xa739a8
extern MenuWidget* g_wAC;                               // 0xa739ac
extern MenuWidget* g_wB0;                               // 0xa739b0
extern MenuWidget* g_wB4;                               // 0xa739b4
extern MenuWidget* g_wB8;                               // 0xa739b8
extern MenuWidget* g_wBC;                               // 0xa739bc
extern MenuWidget* g_wC0;                               // 0xa739c0
extern MenuWidget* g_wD8;                               // 0xa739d8
extern MenuWidget* g_wDC;                               // 0xa739dc
extern MenuWidget* g_wE4;                               // 0xa739e4
extern MenuWidget* g_wE8;                               // 0xa739e8
extern MenuWidget* g_wEC;                               // 0xa739ec

// the Mission object with the members of 0x5143f0 / 0x5144d0 (src/game/mission.h lacks them) and the offsets the start-up reads
struct MissionV : Mission {
    void init(int tmpl, int a2, int a3, int a4, struct MissionCtx* ctx, int a6, int a7);   // 0x5143f0 (ret 0x18)
    __declspec(noinline) void init_scene();             // 0x5144d0
    void frame(float a, int b);                          // 0x483f60 (ret 8)
    void start_setup();                                 // 0x484590
};
#define MI(m, off) (*(int*)((char*)(m) + (off)))


// the 0x6c-byte per-class record of a team (ClassInfo in mission.h: its constructor-style reset is 0x5146f0)
struct Lane4 {
    float x, y, z, w;
    Lane4(float a, float b, float c) { x = a; y = b; z = c; }
};
struct Sub3 {
    int f0, f4, f8, fc;                                 // +0x00
    Lane4 a;                                             // +0x10 (20, 20, 20, -)
    Lane4 b;                                             // +0x20 (0, 0, 0, -)
    int f30;                                            // +0x30 (-1)
    Lane4 c;                                             // +0x34 (0, 0.001, 0, -)
    int f44;                                            // +0x44 (1)
    int f48;                                            // +0x48 (1)
    float f4c;                                          // +0x4c (10)
    int f50, f54;                                       // +0x50
    int f58;                                            // +0x58 (2)
    Lane4 d;                                             // +0x5c (20, 20, 20, -)
    void init();                                        // 0x5146f0
};
struct Team {                                           // the team record seen from 4 bytes before TeamRec (mission.h): 0x16c bytes
    int f0;                                             // +0x00 (the previous record's pad168)
    int f4;                                             // +0x04 leader, -1 = none
    Sub3 subs[3];                                       // +0x08 (0x6c each)
    int sel[8];                                         // +0x14c relations
    void init(int idx);                                 // 0x514670
};

// the map screen (`this` of 0x514d40 / 0x515d80)
#include "../campaign_screen.h"            // Screen (the campaign map screen; its Cam is rgl::frustum)

// the mission start-up object (global g_mission_ctx, 0xa73300)
struct MissionCtx {
    int party[2];                                       // +0x00 the two sides' party ids (0x513f80, 0x5143f0, 0x514cd0)
    unsigned f08;                                       // +0x08 packed handle of the detach target (0x513b40)
    int f0c;                                            // +0x0c
    int f10;                                            // +0x10 mission flags
    int mode;                                           // +0x14
    int f18;                                            // +0x18
    int f1c;                                            // +0x1c rating
    char f20;                                           // +0x20
    char f21;                                           // +0x21
    char pad22[2];
    int f24;                                            // +0x24
    char pad28[4];
    int type;                                           // +0x2c mission type (0x514360)
    char f30pad[4];
    char f34, f35;                                      // +0x34 (0x516700)
    char pad36[2];
    int f38;                                            // +0x38
    unsigned f3c;                                       // +0x3c accumulated key mask (0x513b10)
    char f40;                                           // +0x40 dirty flag
    char pad41[3];
    int f44;                                            // +0x44
    char pad48[0x138 - 0x48];
    int side;                                           // +0x138 the page / side displayed
    char pad13c[0x144 - 0x13c];
    int f144;                                           // +0x144
    char pad148[0x154 - 0x148];
    int f154, f158, f15c;                               // +0x154 launch arguments (0x5147e0)
    char pad160[4];
    int f164;                                           // +0x164 flag byte (bit 0 forces a party detach)
    int f168;                                           // +0x168 announced item count
    int f16c, f170;                                     // +0x16c
    char pad174[0x274 - 0x174];
    int f274, f278;                                     // +0x274
    int f27c;                                           // +0x27c
    char pad280[0x2c0 - 0x280];
    int f2c0, f2c4;                                     // +0x2c0
    char f2c8;                                          // +0x2c8
    char pad2c9[3];

    unsigned pop_key();                                 // 0x513b10
    void update_parties();                              // 0x513b40
    void setup(unsigned flags, int delta);              // 0x513f80 (ret 8)
    void pick_template();                               // 0x514360
    void start_scene(int a, int b, int c);              // 0x5147e0 (ret 0xc)
    void build_hero_table(int a, int b, int c, int d);  // 0x514890 (ret 0x10)
    void start_battle(int a, int b, int c, int d);      // 0x514c00 (ret 0x10)
    void start_mission();                               // 0x514cd0
    void init_battle(int id);                           // 0x516250 (ret 4; the slices' Unit constructor)
    void frame(int tick);                               // 0x516370 (ret 4)
    void update_anim();                                 // 0x516700
    void reset_ids(int id);                             // 0x5169e0 (ret 4)
    void select_side(int side);                         // 0x492750 (ret 4)
    void base_init();                                   // 0x4be730
};
extern MissionCtx g_mission_ctx;                        // 0xa73300

// a 2x2 matrix (0x516220)
struct mat2 {
    float m[4];
    mat2(const float* a, const float* b);               // 0x516220
    void rotate_2x2(float angle);                           // 0x4f3b40 (thiscall: pointer in ecx, angle on the stack)
};

#endif // GAME_INVENTORY_PANEL_MISSION_CTX_H
