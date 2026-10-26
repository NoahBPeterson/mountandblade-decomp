// game_data.h: the game-data object (GameData, global pointer g_game at 0x5e2930) as far as agent2, agent3, dialog_manager,
// terrain_b, game_util and gui_edit read it. ONE definition for those modules (src/game and src/rgl). The other headers
// that see the same object under another name (World in ui/world.h, Game in party/game.h, GameConfig in config/config.h)
// declare further fields; they cannot be included together with this one because each declares its own g_game.
// Offsets cite the function that reads them. Names that two modules gave to one offset overlay in anonymous unions of
// plain members (f78 / party_id_78, player / player_troop, troops / chars, ntroops / n_chars / f13a04,
// n_factions / f13a4c, levels / slots).
#ifndef MB_GAME_DATA_H
#define MB_GAME_DATA_H
#include "../game/tstring.h"
namespace rgl { struct matrix; }
#include "./troop.h"
#include "./party.h"
#include "./game_recs.h"
namespace rgl { struct mesh; }

// record types of the save-game writer (char_sheet/save_state.h defines them), the party-info record of the map tooltip and the
// serialisation streams: only pointers to them are members / parameters here
struct Rec300;
struct RecB0;
struct Rec138;
struct PartyInfo;
struct OutStream;
struct SaveCtx;

// The 0x494-byte level / scene-object record (table at game+0x13a50).
struct Level494 {
    char pad0[0x10];
    tstring name10;                             // +0x10 the scene object file's name (scene_path 0x463950: "SceneObj\\<name>.sco")
    char pad20[0x30 - 0x20];
    union {
        unsigned flags;                         // +0x30 (0x100: flat level, 0x1: no fog sheet) (0x479e20, 0x47a0c0, 0x4792d0)
        unsigned char flags8;                   // +0x30 as a byte (0x4435c0)
    };
    tstring name34;                             // +0x34 (0x47a800: the prefix find_meshes searches)
    tstring name44;                             // +0x44 (0x479e20: the body resource looked up with get_object60)
    char pad54[0x114 - 0x54];
    struct { int a, b; } pairs[1];              // +0x114 spawn pairs (0x473a60)
    char pad11c[0x494 - 0x11c];
};

struct GameData {
    int version;                                // +0x00 (char_sheet/save_game.cpp 0x4e8e70; save_state.cpp 0x4e8f10 writes +4..)
    int f4;                                     // +0x04 (0x473a60)
    tstring title;                               // +0x08 the save's name (char_sheet/save_game.cpp 0x4e8980)
    int unk18;                                  // +0x18 (char_sheet/save_game.cpp)
    tstring module;                             // +0x1c the module name (module_path 0x4639d0: "Modules\\<name>\\" + sub); also the save slot name: save_game.cpp 0x4e8980 reads its buf at +0x20 (null = the text is inline)
    char pad2c[0x58 - 0x2c];
    float fog_color[4];                          // +0x58 the fog colour, passed by value to the colour packer 0x40e440 (char_sheet/screen_camera.cpp 0x4ece20)
    char pad68[0x78 - 0x68];
    union {
        int f78;                                // +0x78 (default party)   0x4428c0
        int party_id_78;
        int main_party;                         // +0x78 (inventory_panel/map_tooltip.cpp)
        int side_party;                         // +0x78 (char_sheet/screen_init.cpp)
        int cur_party;                          // +0x78   ui/mission_ui.cpp 0x48a290, 0x48a320 (the party the sl_* queries are asked for)
    };
    char pad7c[0x84 - 0x7c];
    union {
        int sel_party;                          // +0x84 the party selected on the map (inventory_panel/map_update.cpp 0x5191e0, map_info.cpp 0x51abd0; char_sheet/screen_camera.cpp 0x4ece20 follows it)
        int selected;
    };
    union {
        char map_live;                          // +0x88 the map is live (0x5191e0, 0x4ece20)
        char f88;
    };
    char pad89[0x8c - 0x89];
    union {
        int player;                             // +0x8c (the player's troop id)   0x460160, 0x460640
        int player_troop;
        int f8c;                                // +0x8c (char_sheet/save_state.cpp: index of the current party)
        int cur_troop;                          // +0x8c   ui/mission_ui.cpp 0x48a080, 0x489c50
    };
    char pad90[0x95 - 0x90];
    char f95;                                   // +0x95 (inventory_panel/map_update.cpp 0x5191e0)
    char pad96[0x98 - 0x96];
    int cnt_killed;                    // +0x98 0x457b40 (kills of the player)
    int cnt_killed_friend;                    // +0x9c 0x457b40
    int cnt_ko;                    // +0xa0 0x457b40 (knock-outs by the player)
    int cnt_ko_friend;                    // +0xa4 0x457b40
    char pada8[0xb0 - 0xa8];
    int fb0;                                    // +0xb0   0x4610b0
    int fb4;                                    // +0xb4 (char_sheet/cs_update.cpp: set from the profile's +0x50; nonzero scales the party value by 0.75, ui/mission_ui.cpp 0x489f40)
    char padb8[0xc8 - 0xb8];
    union {
        int fc8;                                // +0xc8   0x460620, 0x460d80
        int mode;                               // +0xc8 screen mode (0x12 = party screen)   ui/mission_ui.cpp 0x48add0
    };
    char padcc[0xe4 - 0xcc];                    // +0xcc a paged table of 0x98-byte scene records (16 per page); its API is 0x48ef20 slot() (map_info.cpp), see party2/camera.cpp (BattleTable)
    void** agent_pages;                         // +0xe4 its page table: first pointer of the vector at +0xe0 (map_update.cpp 0x5191e0, map_info.cpp 0x48ef20, camera.cpp)
    char pade8[0xf8 - 0xe8];
    PartyMgr parties;                           // +0xf8 .. +0x139d8 (gui_edit.cpp called it `pm`)
    char pad139d8[0x139e4 - 0x139d8];
    int f139e4;                                 // +0x139e4 (0x443aa0)
    char pad139e8[0x13a00 - 0x139e8];
    union {
        Troop* troops;                          // +0x13a00 (stride 0x508)
        TroopRec* chars;                        // +0x13a00   0x460160, 0x4428c0
        Troop* records;                         // +0x13a00 (char_sheet/save_state.cpp 0x4e97e0)
        char* chars_c;                          // +0x13a00 as bytes (inventory_panel/view_open.cpp, char_sheet/facegen*.cpp: records are indexed by 0x508)
    };
    union {
        int ntroops;                            // +0x13a04
        int n_chars;
        int f13a04;                             // (repeat count, mode 0x3000)
        int nrecords;                           // char_sheet/save_state.cpp
    };
    char pad13a08[4];
    int excluded;                               // +0x13a0c record index skipped by sl_roll_other (ui/mission_ui.cpp 0x489ee0)
    char pad13a10[0x13a1c - 0x13a10];
    int f13a1c;                                 // +0x13a1c current repeat index   0x460d80, 0x460de0
    int f13a20;                                 // +0x13a20 (char_sheet/save_state.cpp)
    char pad13a24[0x13a34 - 0x13a24];
    int f13a34;                                 // +0x13a34   0x4428c0
    float f13a38;                               // +0x13a38 item quality for generated loot (0x48cc40: passed to ItemRef::randomize_modifier)
    char pad13a3c[0x13a48 - 0x13a3c];
    union {
        FactionRec* factions;                   // +0x13a48   0x4c8ca0
        Rec300* f13a48;                         // +0x13a48 (char_sheet/save_state.cpp: the 0x300-byte records as its writer sees them)
        float* rel;                             // +0x13a48 (inventory_panel/map_tooltip.cpp: 0xc0 floats per troop)
    };
    union {
        int n_factions;                         // +0x13a4c
        int f13a4c;                             // (repeat count, mode 0x1000)
        int n13a4c;
    };
    union {
        Level494* levels;                       // +0x13a50 (the scene / level table)
        Level494* slots;
        RecB0* party_recs;                      // +0x13a50 (char_sheet/save_state.cpp: the 0x494-byte records, a List8 at +0x474)
    };
    int nparties;                               // +0x13a54 (char_sheet/save_state.cpp write_list_party 0x4e9670)
    union {
        QuestRec* quests;                       // +0x13a58   0x4c8ca0
        Rec138* f13a58;                         // +0x13a58 (char_sheet/save_state.cpp)
    };
    union {
        int n_quests;                           // +0x13a5c
        int n13a5c;
    };
    char pad13a60x[4];                          // +0x13a60 allocator word of the event / fleet vector
    union {
        char* fleet;                            // +0x13a64 first of the 0x5c-byte fleet / battle records (std::vector first; map_update.cpp 0x5191e0, map_info.cpp 0x51abd0, map_tooltip.cpp 0x51c090; party2 push_event 0x4b5fe0)
        char* f13a64;                           // (char_sheet/save_state.cpp write_vec5c 0x4e93f0)
        PartyInfo* party_info;                  // (inventory_panel/map_tooltip.cpp)
    };
    union {
        char* fleet_end;                        // +0x13a68 its last (map_update.cpp)
        char* f13a68;
    };
    char pad13a6c[0x13a70 - 0x13a6c];
    tstring t13a70;                             // +0x13a70 (char_sheet/save_state.cpp write_clock: w_struct)
    char pad13a80[0x13a88 - 0x13a80];
    int f13a88;                                 // +0x13a88 (char_sheet/save_state.cpp: low word of a 64-bit stamp)
    char pad13a8c[0x13aa0 - 0x13a8c];
    int f13aa0;                                 // +0x13aa0 (char_sheet/save_state.cpp)
    char pad13aa4[0x13acc - 0x13aa4];
    float f13acc;                               // +0x13acc scaled zoom (char_sheet/screen_init.cpp 0x4ecce0)
    int f13ad0;                            // +0x13ad0 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13ad4;                            // +0x13ad4 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13ad8;                            // +0x13ad8 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13adc;                            // +0x13adc (char_sheet/save_state.cpp write_clock 0x4e9190)
    union {
        int f13ae0;                        // +0x13ae0 (char_sheet/save_state.cpp write_clock 0x4e9190)
        int range_lo;                      // +0x13ae0 random integer range of the party-template roll   ui/mission_ui.cpp 0x48a150
    };
    union {
        int f13ae4;                        // +0x13ae4 (char_sheet/save_state.cpp write_clock 0x4e9190)
        int range_hi;                      // +0x13ae4
    };
    int f13ae8;                            // +0x13ae8 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13aec;                            // +0x13aec (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13af0;                            // +0x13af0 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13af4;                            // +0x13af4 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13af8;                            // +0x13af8 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int f13afc;                            // +0x13afc (char_sheet/save_state.cpp write_clock 0x4e9190)
    char pad13b00[0x13b08 - 0x13b00];
    int f13b08;                                 // +0x13b08 (inventory_panel/view_open.cpp)
    int f13b0c;                                 // +0x13b0c (inventory_panel/view_open.cpp)
    char pad13b10[0x13b18 - 0x13b10];
    int f13b18;                                 // +0x13b18 tick index (8-byte entries) (0x4792d0)
    char pad13b1c[0x13b20 - 0x13b1c];
    union {
        struct {
            unsigned f13b20;                    // +0x13b20 low word of the mission tick clock
            int f13b24;                         // +0x13b24 high word
        };
        __int64 clock;                          // +0x13b20 (char_sheet/save_game.cpp)
    };
    int f13b28;                            // +0x13b28 (char_sheet/save_state.cpp write_clock)
    union {
        int f13b2c;                        // +0x13b2c (char_sheet/save_state.cpp write_clock)
        int string_count;                  // +0x13b2c the string counter the report pages compare ids against (inventory_panel/note_window.cpp 0x50a1c0)
    };
    int f13b30;                            // +0x13b30 (char_sheet/save_state.cpp write_clock)
    int f13b34;                            // +0x13b34 (char_sheet/save_state.cpp write_clock)
    int f13b38;                            // +0x13b38 (char_sheet/save_state.cpp write_clock)
    union {
        float f13b3c;                           // +0x13b3c (char_sheet/screen_camera.cpp)
        int i13b3c;                             // (char_sheet/save_state.cpp)
    };
    union {
        float f13b40;                           // +0x13b40 (char_sheet/screen_camera.cpp)
        int i13b40;                             // (char_sheet/save_state.cpp)
    };
    union {
        int f13b44;                             // +0x13b44 (char_sheet/save_state.cpp)
        float avg_value;                        // +0x13b44 running party value   ui/mission_ui.cpp 0x48a000
    };
    union {
        int f13b48;                             // +0x13b48 (char_sheet/save_state.cpp)
        float avg_weight;                       // +0x13b48
    };
    float avg_prev_time;                        // +0x13b4c
    tstring f13b50;                              // +0x13b50 (char_sheet/save_state.cpp)
    int f13b60;                                 // +0x13b60 (char_sheet/save_state.cpp)
    float val[4][4];                            // +0x13b64 four wind anchors: four weights / values each (0x471040)
    float pos[4][2];                            // +0x13ba4 their xy positions
    int n_labels;                               // +0x13bc4 (inventory_panel/map_update.cpp 0x5191e0, map_picker.cpp)
    int cur_label;                              // +0x13bc8
    tstring labels[50];                         // +0x13bcc the map label texts (0x10 each)
    int label_target[64];                       // +0x13eec the party id of each label (map_update.cpp)
    char pad13fec[0x1405c - 0x13fec];
    int tbl[64][32];                            // +0x1405c (0x473a60)
    char pad1605c[0x1a060 - 0x1605c];
    int f1a060;                                 // +0x1a060 script dialog requested (inventory_panel/game_app.cpp)
    int f1a064;                                 // +0x1a064 script dialog shown
    int f1a068;                                 // +0x1a068
    char pad1a06c[0x1a090 - 0x1a06c];
    int f1a090;                                 // +0x1a090 map editor active (0x472050)
    char pad1a094[0x1a160 - 0x1a094];
    int f1a160;                                 // +0x1a160 map editor dialog shown (inventory_panel/game_app.cpp)
    char pad1a164[0x1a1b4 - 0x1a164];
    int f1a1b4;                                 // +0x1a1b4 (inventory_panel/game_app.cpp)
    char pad1a1b8[0x1a1bc - 0x1a1b8];
    int* f1a1bc;                                // +0x1a1bc int vector begin (inventory_panel/game_app.cpp)
    int* f1a1c0;                                // +0x1a1c0 int vector end
    char pad1a1c4[0x1a1c8 - 0x1a1c4];
    int f1a1c8;                                 // +0x1a1c8 map editor list needs rebuilding
    char pad1a1cc[0x1a1d8 - 0x1a1cc];
    int f1a1d8;                            // +0x1a1d8 (char_sheet/save_state.cpp write_clock)
    int f1a1dc;                            // +0x1a1dc (char_sheet/save_state.cpp write_clock)
    int f1a1e0;                            // +0x1a1e0 (char_sheet/save_state.cpp write_clock)
    int f1a1e4;                            // +0x1a1e4 (char_sheet/save_state.cpp write_clock)
    int f1a1e8;                            // +0x1a1e8 (char_sheet/save_state.cpp write_clock)
    int f1a1ec;                            // +0x1a1ec (char_sheet/save_state.cpp write_clock)
    int f1a1f0;                    // +0x1a1f0 0x44c210 (Mission::spawn_prop: the sound-event context: prop index)
    int f1a1f4;                    // +0x1a1f4 0x44c210 (agent id)
    int f1a1f8;                    // +0x1a1f8 0x44c210 (its troop)
    int f1a1fc;                    // +0x1a1fc 0x44c210
    int f1a200;                    // +0x1a200 0x44c210 (first mesh of the entity)
    int f1a204;                    // +0x1a204 0x44c210
    char pad1a208[0x1a20c - 0x1a208];
    int f1a20c;                                 // +0x1a20c
    tstring mesh1a210;                          // +0x1a210 the game menu's mesh override (MenuRec::mesh, menus.txt
                                                //         +0x28, copied here by GameScreen::build; constructed by the save
                                                //         block reader Block::read 0x51d100, reset at 0x492f6f)
    char pad1a220[0x1a23c - 0x1a220];
    int f1a23c;                                 // +0x1a23c   0x4610b0

    int skill_level(int troop, int skill);      // 0x437b20 (thiscall, ret 8)  // old name of GameData::skill_level, removed after the names merge
    void step(float dt);                        // 0x427f40 (char_sheet/cs_update.cpp)
    // ---- char_sheet/save_game.cpp, save_state.cpp (the save-game serialiser; the stream is the binary writer OutStream) ----
    void load_header(SaveCtx* h);               // 0x4e8e70 (ret 4)
    int save_state(OutStream* out);             // 0x4e8f10
    void world_free();                          // 0x492f40
    __declspec(noinline) int  write_clock(OutStream* out);               // 0x4e9190
    __declspec(noinline) void write_vec5c(OutStream* out);               // 0x4e93f0
    __declspec(noinline) void write_list138(OutStream* out);             // 0x4e9550
    __declspec(noinline) void write_list_party(OutStream* out);          // 0x4e9670
    __declspec(noinline) void write_list300(OutStream* out);             // 0x4e96c0
    __declspec(noinline) void write_records(OutStream* out);             // 0x4e97e0
    __declspec(noinline) void write_items(OutStream* out);               // 0x4e9790
    void write_missions(OutStream* out); // 0x4e9b50
    float time_of_day();                        // 0x4356e0 (agent5/agent_hud.cpp: the sun pass reads it through the game object; agent/agent.h time_of_day is the same code)
    char emit_event(int id, int a, int b);      // 0x435790 (ret 0xc)
    void set_matrix(const rgl::matrix* m);      // 0x435650 (agent4: sound-event context frame)
    void add_experience(int troop, int amount); // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
    void party_counts(unsigned id, int* extra, int* wounded, int* size);   // 0x51be60 (inventory_panel/map_tooltip.cpp, called by map_info.cpp 0x51abd0: counts a party's heroes and wounded)
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4: the hit points of troop idx are below the agent-low threshold)
    tstring scene_path(int idx);                // 0x463950 (ret 8: hidden return)
    const char* module_path(const char* sub);   // 0x4639d0
    void load_scene(int idx);                   // 0x47fbd0 (mesh_slot/mesh_char.cpp: reads the scene object file of level idx)
    void blend(float* out, float x, float y);   // 0x471040 (wind / sky blend over the anchors)
    void save_language_files(bool translated);  // 0x4c8ca0
    // ---- src/game/party2 (method declarations only; fields at the offsets noted are reached by offset there) ----
    struct face* find_at(const float* p);       // 0x48ea80 (nav-mesh face under a map position; other object)
    void init_event(struct E92* r);             // 0x4b36a0 (ret 4: orient an event record on the map)
    float party_value(unsigned id);             // 0x4b4800 (ret 4)
    void refresh_party(unsigned id);            // 0x4b4700 (ret 4)
    void push_event(const int* pos, unsigned nation);       // 0x4b5fe0 (ret 8: events vector at +0x13a60)
    void push_party_event(int party, const int* pos, const float* heading);   // 0x4b6070 (ret 0xc)
    unsigned color_of(float relation, int flag);           // 0x4b1930 (ret 8: relation colour ramp)
    bool is_night();                            // 0x435740 (the slices' not_afternoon)
    int remove_troops(int party, int troop, int count, char prisoners, int a5);   // 0x48f9f0 (ret 0x14: count removed)
    float relation(int a, int b);               // 0x48ffa0 (ret 8: party relation, -1..1)
    void list_members(int id, unsigned* out, unsigned* n);   // 0x490fa0 (ret 0xc: the party and its attached parties)
    void detach_party(int id); // 0x490a30
    void remove_party(int party);               // 0x4915d0 (ret 4)
    void label_text(rgl::mesh* m, const tstring* text, char flag, int arg, const float* h);   // 0x49baa0 (ret 0x14: lays text out into a label mesh)
    bool deploy_party(int a, int b);            // 0x49f060 (ret 8)
    void add_gold(int gold, int troop);         // 0x49b720 (ret 8: adds to the troop's gold (+0x3b4) and, for the player, logs ui_you_got_value)
    void remove_gold(int gold, int troop);      // 0x49b820 (ret 8: subtracts (clamped at 0) and logs ui_you_lost_value)
    void start_quest(int quest, int giver_troop);   // 0x49b930 (ret 8: stamps a quest record as started by the giver, logs ui_new_quest_taken)
    void fill_parties(int id, struct Party* out);   // 0x48fd60 (ret 8: merges the party and its attached parties into `out`)
    void count_units(int party, int* a, int* b, int* c, int* d);   // 0x490ce0 (ret 0x14)
    // ---- src/game/ui (mission_ui.cpp; the slices called the object World, `sl_query` is emit_event 0x435790) ----
    tstring get_name();                         // 0x489c30 (hidden return pointer, ret 4: the string at +0xe028)
    int count_in(int idx, int id, char flag);   // 0x489c50
    int sl_take(int group, int item, int want, char extras);   // 0x489d10
    int sl_roll_party(int lo, int hi);          // 0x489e70
    int sl_roll_other(int lo, int hi);          // 0x489ee0
    int sl_value_factor();                      // 0x489f40 (truncated to int)
    void sl_update_avg();                       // 0x48a000
    int sl_pick_chain(int leader, char lancer); // 0x48a080 (walks the +0x50 link chain from leader)
    int sl_party_value(int party);              // 0x48a290
    int sl_party_price(int party);              // 0x48a320
    void set_names(int slot, const tstring* a); // 0x48a380
    // ---- src/game/mission_ui2 (method declarations only) ----
    tstring event_string(int id);               // 0x48c470 (ret 8: hidden return; runs emit_event(0xf, id / 6, id), returns the result string 0xa70464)
    void init_slots(int troop, int want);       // 0x48c4c0 (ret 8: frees inventory slots of a troop until `want` are free, then compacts)
    void give_item(int troop, int item, int count);   // 0x48c9b0 (ret 0xc)
    int pick_random_item(int t0, int t1, int t2, int t3);   // 0x48ca30 (ret 0x10)
    void spawn_items(int type, int want, int troop);   // 0x48cc40 (ret 0xc)
    int add_members(int party, int troop, int count, char prisoners);   // 0x48f8f0 (ret 0x10: returns how many members were added)
    void add_template(int party, int tmpl, int reverse);   // 0x48fbd0 (ret 0xc: fills a party from a party template's six slots)
    int battle_root(int party);                 // 0x48fce0 (ret 4: the end of the attached-to chain)
    void collect_attachments(int party, Party* dest);   // 0x48ff70 (ret 8)
    int template_count(int tmpl, int slot, float t);   // 0x48a150 (ret 0xc: member count of a party template slot; rolls between range_lo..range_hi percent of the slot's min..max; the third word is unused: ui/mission_ui.cpp)
    int count_troops(int troop, int party, char prisoners);   // 0x48f790 (ret 0xc: members of `troop` in the party's regular (prisoners 0) or prisoner stacks)
    tstring string_value(int index);            // 0x48e9e0 (ret 8: hidden return; the script string `index` of the current text table, or its fallback)
    void add_inventory(int item, int amount);   // 0x48eb50 (ret 8: the current troop receives an item; a unique item displaces a plain one)
};
extern GameData* g_game;                        // 0x5e2930

#endif // MB_GAME_DATA_H
