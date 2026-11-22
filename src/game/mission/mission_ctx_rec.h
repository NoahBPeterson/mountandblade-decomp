// mission_ctx_rec.h: the mission start-up object MissionCtx (global g_mission_ctx, 0xa73300), the one definition. No includes:
// mission_context.h (inventory_panel) adds the methods' inline bodies; winmain_and_menu_screens.h includes this header directly because its own
// local member-only classes (StrTable, resmgr, NodeVec) clash with inv_common.h, which mission_context.h pulls in.
#ifndef GAME_INVENTORY_PANEL_MISSION_CTX_REC_H
#define GAME_INVENTORY_PANEL_MISSION_CTX_REC_H
// the mission start-up object (global g_mission_ctx, 0xa73300)
struct MissionCtx {
    union {
    int ids[0x12];                                      // +0x00 the same words as a table (the diary reads the id of side n as ids[n - 1], of a page as ids[page])
    struct {
    int party[2];                                       // +0x00 the two sides' party ids (0x513f80, 0x5143f0, 0x514cd0)
    unsigned battle_event;                              // +0x08 packed handle of the battle's map event (Event pool: update_parties 0x513b40 detaches it, init_battle 0x516250 sets it)
    int f0c;                                            // +0x0c
    int mission_flags;                                       // +0x10 mission flags (0x10: no side chosen yet, select_side 0x492750 / frame 0x516370)
    int mode;                                           // +0x14
    int f18;                                            // +0x18
    int advantage;                                      // +0x1c the battle rating / advantage (-32..6; script ops get/set_battle_advantage, refresh_rating 0x492470)
    char f20;                                           // +0x20
    char f21;                                           // +0x21
    char pad22[2];
    int player_side;                                    // +0x24 the side the player fights on (0 / 1; 2 = not chosen: frame 0x516370 picks it; select_side 0x492750)
    int tier;                                           // +0x28 (refresh_rating 0x492470: the active side's strength class 0..3)
    int type;                                           // +0x2c mission type (0x514360)
    int menu_value;                                     // +0x30 the value of the menu entry chosen (setup_entry 0x531600)
    char f34, f35;                                      // +0x34 (0x516700)
    char pad36[2];
    int f38;                                            // +0x38
    unsigned key_mask;                                  // +0x3c accumulated key mask (pop_key 0x513b10)
    char dirty;                                         // +0x40 dirty flag (update_parties 0x513b40)
    char pad41[3];
    int template_type;                                  // +0x44 the mission type the template was picked for (pick_template 0x514360)
    };
    };
    int grid[0x38];                                     // +0x48 the unit indices of the diary grid (cols x depth x rows; winmain_and_menu_screens.cpp fill_mission_grid 0x4db090)
    int cols, depth, rows;                              // +0x128 grid dimensions
    char pad134[4];
    int side;                                           // +0x138 the page / side displayed
    int meet_party;                                     // +0x13c the party whose leader the player meets (inventory_panel.cpp: build_hero_table of its first troop; the diary reads it as ids[page + ..])
    int meet_troop;                                     // +0x140 the troop the player meets (inventory_panel.cpp: build_hero_table(meet_troop, ...))
    int side_chosen;                                    // +0x144 the side is decided (select_side 0x492750, init_battle 0x516250, frame 0x516370 refreshes the rating then)
    char pad148[0x154 - 0x148];
    int mission_template;                               // +0x154 the mission template launched (start_scene 0x5147e0; tick 0x5295a0 reads its flags)
    int f158, f15c;                                     // +0x158 the other two launch arguments (Mission::init a2 / a3)
    int option;                                         // +0x160 the mission-type option drawn (setup_entry 0x531600, -1 = none)
    int battle_flags;                                   // +0x164 battle flags (bit 0 forces a party detach; script op battle_set_flag)
    int upgradable;                                     // +0x168 the troops ready to upgrade already announced (update_parties 0x513b40: "ui_some_soldiers_are_ready_to_upgrade")
    int f16c, f170;                                     // +0x16c
    char pad174[0x274 - 0x174];
    int f274, f278;                                     // +0x274
    int advantage_delta;                                // +0x27c the change of advantage by the last setup (setup 0x513f80)
    char pad280[0x2c0 - 0x280];
    int scene_counter;                                           // +0x2c0
    int passage_menu;                                   // +0x2c4 passage menu (set_passage_menu 0x4a55f3; 0 or less: no menu to close)
    union {
    char party_battle_mode;                             // +0x2c8 party battle mode (script op set_party_battle_mode 0x4a2ad0 stores a byte)
    int party_battle_mode_word;                         // +0x2c8 the same word (the world map's frame sets it as a dword)
    };

    MissionCtx() { init(); }                            // the original's static initializer 0x547e40 calls init
    ~MissionCtx() {}                                    // (empty: the original registers an empty atexit function, 0x548200)
    void init();                                        // 0x4be730 (MetaMission::init: the WinLicense marker entry; its jmp reaches the body at 0x4be742)
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
    void select_side(int side);                         // 0x492750 (ret 4; game_data_party_quest.cpp)
};
extern MissionCtx g_mission_ctx;                        // 0xa73300 (defined in game/app/options_dialog_and_module_tables.cpp)
// a free inline helper taking the object by reference: Operation::execute 0x49fa10 first-references fewer global
// field views than direct member stores would make (the global symbol-key count fixes g_visitor_scene's sort order)
inline void set_party_battle_mode(MissionCtx& c) { c.party_battle_mode = 1; }   // script op set_party_battle_mode 0x4a2ad0

#endif
