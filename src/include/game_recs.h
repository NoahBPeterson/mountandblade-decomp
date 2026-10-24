// game_recs.h: the Module System records of the game data that both the game and the language export read: quests
// (QuestRec, 0x138 bytes, array behind GameData+0x13a58), factions (FactionRec, 0x300 bytes, GameData+0x13a48) and party
// templates (PartyTemplate, 0xb0 bytes, table g_party_templates 0x5e29f8). ONE definition for src/game/script,
// src/game/party, src/game/ui (world.h called the quest record PartyRec) and src/rgl/mesh2 (gui_edit.cpp).
// Layout: the names at +0x00 / +0x10 / key (gui_edit.cpp save_language_files 0x4c8ca0), the scripted fields
// (script_interpreter.cpp, cited), the save-game loaders (game.cpp 0x51e170, 0x51e320, 0x51e410). The slot array and the
// 64-bit vector share their bytes (anonymous unions of plain members); likewise the flag word and its bit-field view.
#ifndef MB_GAME_RECS_H
#define MB_GAME_RECS_H
#include "../game/tstring.h"
#include "../game/slot_array.h"
#include "./i64_vec.h"
struct stream_base;

struct QuestRec {                           // 0x138 bytes
    tstring name;                           // +0x00   0x4c63f0
    tstring desc;                           // +0x10   0x4c6470 (gui_edit.cpp called it `text`)
    tstring giver;                          // +0x20 (setup_quest_giver 0x4a5530; game.cpp 0x51e170 r_tstr)
    tstring title;                          // +0x30 expanded (setup_quest_text 0x4a543e)
    tstring text;                           // +0x40
    union {
        unsigned flags2;                    // +0x50 bit 1 tested by store_partner_quest (0x4ad370)
        struct {
            unsigned f50_0 : 1;             // +0x50 (bit 1 is tested as a bit-field: shr + test)   0x489e70
            unsigned f50_active : 1;
            unsigned f50_rest : 30;
        };
    };
    tstring key;                            // +0x54
    union {
        unsigned flags;                     // +0x64 bit 8 active, 9 finished, 10 succeeded, 11 failed, 12 concluded (0x4a062d)
        int f64;                            // +0x64   0x51e170 (r_int)
        struct {
            unsigned f64_lo : 8;            // +0x64 (bit 8 tested)    0x489e70
            unsigned f64_flag : 1;
            unsigned f64_rest : 23;
        };
    };
    int note_times[8];                      // +0x68 (add_quest_note_from_dialog 0x4a3753; SoaBlock::vals, read_soa 0x51e1f0)
    tstring notes[8];                       // +0x88 (0x4a3691; SoaBlock::names)
    int note_tableau;                       // +0x108 (add_quest_note_tableau_mesh 0x4a31f9; SoaBlock::total)
    int troop;                              // +0x10c (store_quest_troop 0x4ad55e)
    int item;                               // +0x110 (store_quest_item 0x4ad51e)
    int number;                             // +0x114 (store_quest_number 0x4ad500)
    int giver_troop;                        // +0x118 (store_partner_quest 0x4ad37f)
    float f11c;                             // +0x11c (r_float)   0x51e170
    char pad120[0x128 - 0x120];
    union {
        I64Vec list;                        // +0x128   0x51e170
        SlotArray slots;                    // +0x128 (quest_set_slot 0x4a0a68)
    };
    tstring* raw_title();                   // 0x48d750
    tstring* raw_text();                    // 0x48d730
    void set_progression(int value);        // 0x489370 (ret 4)
    void read(stream_base* s);              // 0x51e170
    tstring translated();                   // 0x4c63f0
    tstring translated_text();              // 0x4c6470
    const tstring* text_name();             // gui_edit.h: suffix_or(key, "_text", desc)
};

struct FactionRec {                         // 0x300 bytes
    tstring name;                           // +0x00   0x51e320 (r_tstr)
    union {
        bool flag;                          // +0x10
        char f10;                           // +0x10   (r_bool)
    };
    char pad11[3];
    tstring key;                            // +0x14   0x4c6860
    int f24;                                // +0x24   set = no tag drawn (inventory_panel/map_info.cpp 0x51abd0)
    union {
        unsigned color;                     // +0x28 (faction_set_color 0x4a5262)
        unsigned f28;                       // +0x28   (r_uint)
    };
    union {
        float relations[128];               // +0x2c -1..1 (set_relation 0x4a4ef7)
        float rel[129];                     // +0x2c   one float per faction (r_float, n of them)
    };
    int f230;                               // +0x230  (r_int)
    char pad234[0x24c - 0x234];
    union {
        I64Vec list;                        // +0x24c
        SlotArray slots;                    // +0x24c (faction_set_slot 0x4a088d)
    };
    int note_times[8];                      // +0x25c (add_faction_note_from_dialog 0x4a34a8)
    tstring notes[8];                       // +0x27c (0x4a33e4)
    int note_tableau;                       // +0x2fc (add_faction_note_tableau_mesh 0x4a3152)
    void set_name(const tstring& name);     // 0x489ab0 (ret 4)
    tstring* display_name();                // 0x48db00
    tstring translated();                   // 0x4c6860
};

struct PartyTemplate {                      // 0xb0 bytes
    tstring name;                           // +0x00
    tstring key;                            // +0x10
    char pad20[0x34 - 0x20];
    union {
        int f34;                            // +0x34  0x51e410 (r_int)
        int num_created;                    // +0x34 (store_num_parties_created 0x4ad83a)
    };
    union {
        int f38;                            // +0x38
        int num_destroyed;
    };
    union {
        int f3c;                            // +0x3c
        int num_destroyed_by_player;
    };
    char pad40[0xa0 - 0x40];
    union {
        I64Vec list;                        // +0xa0
        SlotArray slots;                    // +0xa0 (party_template_set_slot 0x4a097b)
    };
    __declspec(noinline) void read(stream_base* s);        // 0x51e410
    tstring translated();                   // 0x4c6080
};

#endif // MB_GAME_RECS_H
