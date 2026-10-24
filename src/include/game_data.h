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

// The 0x494-byte level / scene-object record (table at game+0x13a50).
struct Level494 {
    char pad0[0x30];
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
    char pad0[4];
    int f4;                                     // +0x04 (0x473a60)
    char pad8[0x78 - 8];
    union {
        int f78;                                // +0x78 (default party)   0x4428c0
        int party_id_78;
    };
    char pad7c[0x84 - 0x7c];
    int sel_party;                              // +0x84 the party selected on the map (inventory_panel/map_update.cpp 0x5191e0, map_info.cpp 0x51abd0; char_sheet/screen_camera.cpp 0x4ece20 follows it)
    char map_live;                              // +0x88 the map is live (0x5191e0, 0x4ece20)
    char pad89[0x8c - 0x89];
    union {
        int player;                             // +0x8c (the player's troop id)   0x460160, 0x460640
        int player_troop;
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
    char padb4[0xc8 - 0xb4];
    int fc8;                                    // +0xc8   0x460620, 0x460d80
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
    };
    union {
        int ntroops;                            // +0x13a04
        int n_chars;
        int f13a04;                             // (repeat count, mode 0x3000)
    };
    char pad13a08[0x13a1c - 0x13a08];
    int f13a1c;                                 // +0x13a1c current repeat index   0x460d80, 0x460de0
    char pad13a20[0x13a34 - 0x13a20];
    int f13a34;                                 // +0x13a34   0x4428c0
    float f13a38;                               // +0x13a38 item quality for generated loot (0x48cc40: passed to ItemRef::randomize_modifier)
    char pad13a3c[0x13a48 - 0x13a3c];
    FactionRec* factions;                       // +0x13a48   0x4c8ca0
    union {
        int n_factions;                         // +0x13a4c
        int f13a4c;                             // (repeat count, mode 0x1000)
    };
    union {
        Level494* levels;                       // +0x13a50 (the scene / level table)
        Level494* slots;
    };
    char pad13a54[4];
    QuestRec* quests;                           // +0x13a58   0x4c8ca0
    int n_quests;                               // +0x13a5c
    char pad13a60[4];                           // +0x13a60 allocator word of the event / fleet vector
    char* fleet;                                // +0x13a64 first of the 0x5c-byte fleet / battle records (std::vector first; map_update.cpp 0x5191e0, map_info.cpp 0x51abd0, map_tooltip.cpp 0x51c090; party2 push_event 0x4b5fe0)
    char* fleet_end;                            // +0x13a68 its last (map_update.cpp)
    char pad13a6c[0x13b18 - 0x13a6c];
    int f13b18;                                 // +0x13b18 tick index (8-byte entries) (0x4792d0)
    char pad13b1c[4];
    unsigned f13b20;                            // +0x13b20 low word of the mission tick clock
    int f13b24;                                 // +0x13b24 high word
    char pad13b28[0x13b64 - 0x13b28];
    float val[4][4];                            // +0x13b64 four wind anchors: four weights / values each (0x471040)
    float pos[4][2];                            // +0x13ba4 their xy positions
    int n_labels;                               // +0x13bc4 (inventory_panel/map_update.cpp 0x5191e0, map_picker.cpp)
    int cur_label;                              // +0x13bc8
    tstring labels[50];                         // +0x13bcc the map label texts (0x10 each)
    int label_target[64];                       // +0x13eec the party id of each label (map_update.cpp)
    char pad13fec[0x1405c - 0x13fec];
    int tbl[64][32];                            // +0x1405c (0x473a60)
    char pad1605c[0x1a090 - (0x1405c + 64 * 32 * 4)];
    int f1a090;                                 // +0x1a090 (0x472050)
    char pad1a094[0x1a1f0 - 0x1a094];
    int f1a1f0;                    // +0x1a1f0 0x44c210 (Mission::spawn_prop: the sound-event context: prop index)
    int f1a1f4;                    // +0x1a1f4 0x44c210 (agent id)
    int f1a1f8;                    // +0x1a1f8 0x44c210 (its troop)
    int f1a1fc;                    // +0x1a1fc 0x44c210
    int f1a200;                    // +0x1a200 0x44c210 (first mesh of the entity)
    int f1a204;                    // +0x1a204 0x44c210
    char pad1a208[0x1a20c - 0x1a208];
    int f1a20c;                                 // +0x1a20c
    char pad1a210[0x1a23c - 0x1a210];
    int f1a23c;                                 // +0x1a23c   0x4610b0

    int skill_level(int troop, int skill);      // 0x437b20 (thiscall, ret 8)
    int skill(int troop, int id);               // 0x437b20 (thiscall, ret 8) (0x4406a0)
    char emit_event(int id, int a, int b);      // 0x435790 (ret 0xc)
    void set_matrix(const rgl::matrix* m);      // 0x435650 (agent4: sound-event context frame)
    void add_experience(int troop, int amount); // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
    void party_counts(int id, int* extra, int* wounded, int* size);   // 0x51be60 (inventory_panel/map_tooltip.cpp, called by map_info.cpp 0x51abd0: counts a party's heroes and wounded)
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4)
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
    void on_member_killed(int id);              // 0x490a30 (ret 4)
    void remove_party(int party);               // 0x4915d0 (ret 4)
    void label_text(rgl::mesh* m, const tstring* text, char flag, int arg, const float* h);   // 0x49baa0 (ret 0x14: lays text out into a label mesh)
    bool deploy_party(int a, int b);            // 0x49f060 (ret 8)
    void add_gold(int gold, int troop);         // 0x49b720 (ret 8: adds to the troop's gold (+0x3b4) and, for the player, logs ui_you_got_value)
    void remove_gold(int gold, int troop);      // 0x49b820 (ret 8: subtracts (clamped at 0) and logs ui_you_lost_value)
    void start_quest(int quest, int giver_troop);   // 0x49b930 (ret 8: stamps a quest record as started by the giver, logs ui_new_quest_taken)
    void fill_parties(int id, struct Party* out);   // 0x48fd60 (ret 8: merges the party and its attached parties into `out`)
    void count_units(int party, int* a, int* b, int* c, int* d);   // 0x490ce0 (ret 0x14)
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
    int template_count(int tmpl, int slot, float t);   // 0x48a150 (ret 0xc: member count of a party template slot at fraction t)
    int count_troops(int troop, int party, char prisoners);   // 0x48f790 (ret 0xc: members of `troop` in the party's regular (prisoners 0) or prisoner stacks)
    tstring string_value(int index);            // 0x48e9e0 (ret 8: hidden return; the script string `index` of the current text table, or its fallback)
    void add_inventory(int item, int amount);   // 0x48eb50 (ret 8: the current troop receives an item; a unique item displaces a plain one)
};
extern GameData* g_game;                        // 0x5e2930

#endif // MB_GAME_DATA_H
