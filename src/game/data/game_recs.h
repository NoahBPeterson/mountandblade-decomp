// game_recs.h: the Module System records of the game data that both the game and the language export read: quests
// (QuestRec, 0x138 bytes, array behind GameData+0x13a58), factions (FactionRec, 0x300 bytes, GameData+0x13a48) and party
// templates (PartyTemplate, 0xb0 bytes, table g_party_templates 0x5e29f8). ONE definition for src/game/script,
// src/game/party, src/game/ui (world.h called the quest record PartyRec) and src/game/editor (editor_selection_and_translation.cpp).
// Layout: the names at +0x00 / +0x10 / key (editor_selection_and_translation.cpp save_language_files 0x4c8ca0), the scripted fields
// (script_interpreter.cpp, cited), the save-game loaders (game_state_load.cpp 0x51e170, 0x51e320, 0x51e410). The slot array and the
// 64-bit vector share their bytes (anonymous unions of plain members); likewise the flag word and its bit-field view.
#ifndef MB_GAME_RECS_H
#define MB_GAME_RECS_H
#include <vector>
#include "engine/core/tstring.h"
#include "game/ui/note_block.h"
#include "game/party/slot_array.h"
struct stream_base;

struct QuestRec {                           // 0x138 bytes
    tstring name;                           // +0x00   0x4c63f0
    tstring desc;                           // +0x10   0x4c6470 (editor_selection_and_translation.cpp called it `text`)
    tstring giver;                          // +0x20 (setup_quest_giver 0x4a5530; game_state_load.cpp 0x51e170 r_tstr)
    tstring title;                          // +0x30 expanded (setup_quest_text 0x4a543e)
    tstring text;                           // +0x40
    union {
        unsigned flags2;                    // +0x50 bit 1 tested by store_partner_quest (0x4ad370)
        int f50;                            // +0x50 the qf_* flags as quests.txt reads them (read_file 0x524690)
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
    SoaBlock notes;                     // +0x68 the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    int troop;                              // +0x10c (store_quest_troop 0x4ad55e)
    int item;                               // +0x110 (store_quest_item 0x4ad51e)
    int number;                             // +0x114 (store_quest_number 0x4ad500)
    int giver_troop;                        // +0x118 (store_partner_quest 0x4ad37f)
    float f11c;                             // +0x11c (r_float)   0x51e170
    int f120, f124;                         // +0x120 neither QuestRec::read 0x51e170 nor write 0x4e95a0 nor any src/ function touches them (the loader's names)
    SlotArray slots;                        // +0x128 (quest_set_slot 0x4a0a68; read 0x51e170)
    QuestRec();                             // 0x524860 (game/campaign/map_builder_and_data_load.cpp)
    ~QuestRec();                            // 0x431b10 (game/data/record_destructors.cpp)
    void read_file(stream_base& s);         // 0x524690 (one quests.txt record)
    tstring* raw_title();                   // 0x48d750
    tstring* raw_text();                    // 0x48d730
    void set_alpha(int value);              // 0x489370 (ret 4: the low byte of +0x64; ICF-folded with Troop::set_alpha, game/ui/widgets/widget_base.cpp)
    void read(stream_base* s);              // 0x51e170
    __declspec(noinline) void write(struct stream_base* out);   // 0x4e95a0 (ret 4; game/save/save_state.cpp)
    tstring translated();                   // 0x4c63f0
    tstring translated_text();              // 0x4c6470
    const tstring* text_name();             // editor_selection_and_translation.h: suffix_or(key, "_text", desc)
};

struct FactionRec {                         // 0x300 bytes
    tstring name;                           // +0x00   0x51e320 (r_tstr)
    union {
        bool flag;                          // +0x10
        char f10;                           // +0x10   (r_bool)
    };
    tstring key;                            // +0x14   0x4c6860
    int f24;                                // +0x24   set = no tag drawn (game/campaign/map_panel.cpp 0x51abd0)
    union {
        unsigned color;                     // +0x28 (faction_set_color 0x4a5262)
        unsigned f28;                       // +0x28   (r_uint)
    };
    float relations[128];                   // +0x2c -1..1 (set_relation 0x4a4ef7; factions.txt reads one float per faction)
    int nranks;                             // +0x22c the number of rank names (read_file 0x524a70)
    int f230;                               // +0x230  (r_int)
    std::vector<tstring> ranks;             // +0x234 the rank names
    float f244;                             // +0x244
    float max_rating;                       // +0x248 the maximum player rating / 100
    SlotArray slots;                        // +0x24c (faction_set_slot 0x4a088d)
    SoaBlock notes;                     // +0x25c the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    FactionRec();                           // 0x5249f0 (game/campaign/map_builder_and_data_load.cpp)
    ~FactionRec();                          // 0x432010 (game/data/record_destructors.cpp)
    void read_file(stream_base& s, int count);   // 0x524a70 (one factions.txt record; count = the number of factions)
    void set(const tstring& name);     // 0x489ab0 (ret 4)
    __declspec(noinline) void write300(struct stream_base* out, int n);   // 0x4e9710 (ret 8; game/save/save_state.cpp)
    tstring* display_name();                // 0x48db00
    tstring translated();                   // 0x4c6860
};

struct PartyTemplate {                      // 0xb0 bytes
    tstring name;                           // +0x00
    tstring key;                            // +0x10
    union {
        struct { int f20, f24; };           // +0x20 copied into a new party's flags (+0xa0, +0xa4: GameData::add_sprite 0x49ebd0)
        unsigned __int64 flags;             // +0x20 the pf_* flags (read 0x4bdff0)
    };
    int f28, f2c, f30;                      // +0x28 the menu, faction, personality (a new party's +0x90, +0x70, +0x74)
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
    struct Stack { int troop, min, max, flags; };   // a stack of the template: troop (-1 = none), count range, pmf_* flags
    Stack stacks[6];                        // +0x40 (0x4bdff0)
    SlotArray slots;                        // +0xa0 (party_template_set_slot 0x4a097b)
    __declspec(noinline) void read(stream_base* s);        // 0x51e410 (the save game's counters)
    __declspec(noinline) void write(struct stream_base* out);   // 0x4e9510 (ret 4; game/save/save_state.cpp)
    void load(stream_base& s);              // 0x4bdff0 (party_templates.txt: game/config/config.cpp)
    tstring translated();                   // 0x4c6080
};
extern PartyTemplate* g_party_templates;    // 0x5e29f8 the party-template table (g_party_template_count rows of 0xb0 bytes; allocated by the module loader, game/data/module_files_loader.cpp)

#endif // MB_GAME_RECS_H
