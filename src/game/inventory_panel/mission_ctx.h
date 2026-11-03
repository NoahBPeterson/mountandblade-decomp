// mission_ctx.h: the mission start-up object (global at 0xa73300; the slices called it MissionCtx, Unit, Screen's
// "MissionUi", InventoryPanel and g_panel; trigger_vec.h MetaMission, world_data.h BattleW), the map screen (Screen) and
// the small records of the mission start (code 0x513710..0x516ac0). Layout: every offset cites the function that
// proves it. Names provisional. The game data is read through GameDataV / GD_I (inv_common.h), the mission object
// through MissionV (a view of src/game/mission.h's Mission with the two methods of 0x5143f0 / 0x5144d0).
#ifndef GAME_INVENTORY_PANEL_MISSION_CTX_H
#define GAME_INVENTORY_PANEL_MISSION_CTX_H
#include "./inv_common.h"
#include "../mission.h"
#include "../meta_mission.h"                           // MetaMission: the object at 0xa73300 that g_mission_ctx views (init 0x4be742)
#include "../main_menu.h"

// a menu screen object: vtable (slot 4 = +0x10), state at +0x04, "highlight" counter at +0x18 (0x513710, 0x514d40)
struct MenuWidget {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void v4();                                  // +0x10
    int f4;                                             // +0x04
    char pad8[0x14 - 0x8];
    int f14;                                            // +0x14 (0x514d40)
    int f18;                                            // +0x18
};
extern MenuWidget* g_wA0;                               // 0xa739a0
extern MenuWidget* g_wAC;                               // 0xa739ac
extern MenuWidget* g_wB0;                               // 0xa739b0
extern MenuWidget* g_wB4;                               // 0xa739b4
extern MenuWidget* g_wBC;                               // 0xa739bc
extern MenuWidget* g_wC0;                               // 0xa739c0
extern MenuWidget* g_wDC;                               // 0xa739dc
extern MenuWidget* g_wE4;                               // 0xa739e4
extern MenuWidget* g_wE8;                               // 0xa739e8
extern MenuWidget* g_wEC;                               // 0xa739ec

// the Mission object with the members of 0x5143f0 / 0x5144d0 (src/game/mission.h lacks them) and the offsets the start-up reads
struct MissionG {
    void frame(float dt, bool spawn);   // 0x483f60 (ret 8; game/mesh_slot mesh_game.cpp: Mission::frame's definition)
};
struct MissionV : Mission {
    void init(int tmpl, int a2, int a3, int a4, struct MissionCtx* ctx, int a6, int a7);   // 0x5143f0 (ret 0x18)
    __declspec(noinline) void init_scene();             // 0x5144d0
    __forceinline void frame(float a, int b) { ((MissionG*)this)->frame(a, b != 0); }   // 0x483f60 (ret 8)
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

// the mission start-up object (global g_mission_ctx, 0xa73300): the one definition is mission_ctx_rec.h (no includes), so that
// modules whose local views clash with inv_common.h can use the same class
#include "./mission_ctx_rec.h"
__forceinline void MissionCtx::select_side(int side) { ((BattleW*)this)->select_side(side); }   // 0x492750 (ret 4)

struct rot2 { float m[4]; void rotate_2x2(float angle); };  // 0x4f3b40 (game/char_sheet/terrain_page_input: thiscall, ret 4)
// a 2x2 matrix (0x516220)
struct mat2 {
    float m[4];
    mat2(const float* a, const float* b);               // 0x516220
    __forceinline void rotate_2x2(float angle) { ((rot2*)m)->rotate_2x2(angle); }   // 0x4f3b40
};

#endif // GAME_INVENTORY_PANEL_MISSION_CTX_H
