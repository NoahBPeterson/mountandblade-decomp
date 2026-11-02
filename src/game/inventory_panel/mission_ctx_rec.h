// mission_ctx_rec.h: the mission start-up object MissionCtx (global g_mission_ctx, 0xa73300), the one definition. No includes:
// mission_ctx.h (inventory_panel) adds the methods' inline bodies; cls_map.h includes this header directly because its own
// local member-only classes (StrTable, resmgr, NodeVec) clash with inv_common.h, which mission_ctx.h pulls in.
#ifndef GAME_INVENTORY_PANEL_MISSION_CTX_REC_H
#define GAME_INVENTORY_PANEL_MISSION_CTX_REC_H
// the mission start-up object (global g_mission_ctx, 0xa73300)
struct MissionCtx {
    union {
    int ids[0x12];                                      // +0x00 the same words as a table (the diary reads the id of side n as ids[n - 1], of a page as ids[page])
    struct {
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
    };
    };
    int grid[0x38];                                     // +0x48 the unit indices of the diary grid (cols x depth x rows; cls_map.cpp fill_mission_grid 0x4db090)
    int cols, depth, rows;                              // +0x128 grid dimensions
    char pad134[4];
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
    void select_side(int side);                         // 0x492750 (ret 4; defined inline in mission_ctx.h as BattleW::select_side)
};
extern MissionCtx g_mission_ctx;                        // 0xa73300

#endif
