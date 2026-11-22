// mode_rec.h: the mission mode table (0x58-byte records, array g_modes at 0x5e2a00, indexed by Mission::mode +0x5df0) and
// the spawn rules (0x4c bytes each) its +0x34 pointer leads to. ONE definition for src/game: agent_queries.cpp reads the flags at
// +0x20 (Mission::mode_flag1 0x435840), terrain_b the rule table (rule_accepts / pick_spawn 0x473a60, 0x467940), terrain_a
// (0x4677a0) the rule record by index, script the template entries (MissionTemplate: entries +0x34, count +0x38).
// Names provisional (the slices called the record ModeRec, ModeRec2 and MissionTemplate: MissionTemplate is the name the module-file
// loader 0x4c2300 (game/app/options_dialog_and_module_tables.cpp), the script interpreter (g_mission_templates) and game/mission/mission_context.cpp
// use; ModeRec / ModeRec2 stay as typedefs). Proof that the views are one record: the same table 0x5e2a00 (stride 0x58, count
// 0x5e2a04), spawn rules at +0x34 / count at +0x38 (loader, script, terrain_a 0x4677a0), flags at +0x20 (agent
// mode_flag1 0x435840 = the loader's r_int(&f20)), +0x3c (loader's r_int(&f3c) = mission_ctx's `type`).
#ifndef GAME_MODE_REC_H
#define GAME_MODE_REC_H
#include "engine/core/tstring.h"
#include "game/data/trigger_vecs.h"                    // TriggerVec: the template's triggers at +0x4c
#include "game/data/trigger_rec.h"            // TriggerRec: ~MissionTemplate deletes the trigger records

struct stream_base;
struct tstream3;

struct SpawnRec {                               // 0x4c bytes: a mission spawn rule
    int id;                                     // +0x00
    int flags;                                  // +0x04 (see rule_accepts / pick_spawn)
    union {
        int flags2;                             // +0x08
        int override_flags;                     // +0x08 (script mission_tpl_entry_set_override_flags 0x4aa79a)
    };
    union {
        int padc[4];
        struct { int f0c, f10, f14, f18; };     // +0x0c (game/mission/mission_start_setup.cpp 0x484590 reads f14, f18)
    };
    int f1c;                                    // +0x1c
    int f20;                                    // +0x20
    int num_override_items;                     // +0x24 (script 0x4aa7d0 family)
    int override_items[8];                      // +0x28
    int pad48;
    void load_extras(stream_base& s);           // 0x4bdd50 (config/config.cpp; the mission_templates loader, game/app/options_dialog_and_module_tables.cpp, calls it)
};
struct MissionTemplate {                        // 0x58 bytes: a mission mode / template
    tstring name;                               // +0x00 (0x4c2300)
    tstring f10;                                // +0x10
    union {
        int f20;                                // +0x20 (0x4c2300 r_int)
        unsigned flags;                         // +0x20 bit 1 = mode_flag1 (0x435840)
    };
    tstring display;                            // +0x24 (underscores become spaces)
    union {
        SpawnRec* spawns;                       // +0x34 -> 0x4c-byte records (0x473a60, 0x4677a0, loader 0x4c2300)
        SpawnRec* recs;
        SpawnRec* entries;                      // script
    };
    union {
        int n_spawns;                           // +0x38
        int nrecs;
        int num_entries;                        // script
    };
    union {
        int f3c;                                // +0x3c (0x4c2300 r_int)
        int type;                               // game/mission/mission_context.cpp
    };
    int f40, f44, f48;                          // +0x40
    TriggerVec triggers;                        // +0x4c count, data, cur (read 0x4c1c90); mesh_start / mesh_game call it the sound bank
    // the module loader's inline constructor (game/data/module_files_loader.cpp): the trigger vector's constructor runs first
    MissionTemplate() { spawns = 0; f3c = -1; }
    // inline: `delete[] g_mission_templates` (winmain_and_menu_screens.cpp FinalCleanup, module_files_loader.cpp) emits it into the vector deleting
    // destructor 0x4c3430, whose array path deletes the triggers through ~TriggerRec 0x431440 and its scalar path through 0x431480
    ~MissionTemplate() { if (spawns) delete[] spawns; delete[] triggers.data; }
    void read(stream_base& s);                  // 0x4c2300 (ret 4)
};
typedef MissionTemplate ModeRec2;               // the slices' names for the same record
typedef ModeRec2 ModeRec;                       // agent_queries.h's name for the same record
extern MissionTemplate* g_mission_templates;                // 0x5e2a00 mission templates (0x58-byte records, count 0x5e2a04; g_modes of agent_queries.h)

#endif // GAME_MODE_REC_H
