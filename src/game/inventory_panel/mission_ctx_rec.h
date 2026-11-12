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
    int f1c;                                            // +0x1c the battle rating / advantage (-32..6; script ops get/set_battle_advantage, refresh_rating 0x492470)
    char f20;                                           // +0x20
    char f21;                                           // +0x21
    char pad22[2];
    int f24;                                            // +0x24
    int tier;                                           // +0x28 (refresh_rating 0x492470: the active side's strength class 0..3)
    int type;                                           // +0x2c mission type (0x514360)
    int f30;                                            // +0x30 (setup_entry 0x531600)
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
    int f13c;                                           // +0x13c the party of page 1 (the diary reads it as ids[page + ..])
    int f140;                                           // +0x140 the party of page 0
    int f144;                                           // +0x144
    char pad148[0x154 - 0x148];
    int f154, f158, f15c;                               // +0x154 launch arguments (0x5147e0)
    int f160;                                           // +0x160 (setup_entry 0x531600)
    int f164;                                           // +0x164 battle flags (bit 0 forces a party detach; script op battle_set_flag)
    int f168;                                           // +0x168 announced item count
    int f16c, f170;                                     // +0x16c
    char pad174[0x274 - 0x174];
    int f274, f278;                                     // +0x274
    int f27c;                                           // +0x27c
    char pad280[0x2c0 - 0x280];
    int scene_counter;                                           // +0x2c0
    int passage_menu;                                   // +0x2c4 passage menu (set_passage_menu 0x4a55f3; 0 or less: no menu to close)
    union {
    char f2c8;                                          // +0x2c8 party battle mode (script op set_party_battle_mode 0x4a2ad0 stores a byte)
    int f2c8_word;                                      // +0x2c8 the same word (the world map's frame sets it as a dword)
    };

    MissionCtx() { init(); }                            // the original's static initializer 0x547e40 calls init
    ~MissionCtx() {}                                    // (empty: the original registers an empty atexit function, 0x548200)
    void init();                                        // 0x4be742 (MetaMission::init, entered through the WinLicense marker at 0x4be730)
    void setup_entry(struct MenuEntry* e);              // 0x531600
    int weight(unsigned flags);                         // 0x5316f0
    void refresh_rating();                              // 0x492470
    unsigned pop_key();                                 // 0x513b10
    void update_parties();                              // 0x513b40
    int count_hero_items();                             // 0x513e10 (never reads this)
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
    void tick();                                        // 0x5295a0 (mission_5.cpp)
    void refresh_markers();                             // 0x529600 (mission_5.cpp)
    void reset_party_heroes(int party_id);              // 0x529670 (mission_5.cpp; this unused)
    void select_side(int side);                         // 0x492750 (ret 4; world_data.cpp)
};
extern MissionCtx g_mission_ctx;                        // 0xa73300 (defined in triggers/trigger_vec.cpp)
// a free inline helper taking the object by reference: Operation::execute 0x49fa10 first-references fewer global
// field views than direct member stores would make (the global symbol-key count fixes g_visitor_scene's sort order)
inline void set_party_battle_mode(MissionCtx& c) { c.f2c8 = 1; }   // script op set_party_battle_mode 0x4a2ad0

#endif
