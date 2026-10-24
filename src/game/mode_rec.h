// mode_rec.h: the mission mode table (0x58-byte records, array g_modes at 0x5e2a00, indexed by Mission::mode +0x5df0) and
// the spawn rules (0x4c bytes each) its +0x34 pointer leads to. ONE definition for src/game: agent.cpp reads the flags at
// +0x20 (Mission::mode_flag1 0x435840), terrain_b the rule table (rule_accepts / pick_spawn 0x473a60, 0x467940), terrain_a
// (0x4677a0) the rule record by index, script the template entries (MissionTemplate: entries +0x34, count +0x38).
// Names provisional (the slices called the record ModeRec, ModeRec2 and MissionTemplate: MissionTemplate is the name the module-file
// loader 0x4c2300 (triggers/trigger_vec.cpp), the script interpreter (g_mission_templates) and inventory_panel/mission_ctx.cpp
// use; ModeRec / ModeRec2 stay as typedefs). Proof that the views are one record: the same table 0x5e2a00 (stride 0x58, count
// 0x5e2a04), spawn rules at +0x34 / count at +0x38 (loader, script, terrain_a 0x4677a0), flags at +0x20 (agent
// mode_flag1 0x435840 = the loader's r_int(&f20)), +0x3c (loader's r_int(&f3c) = mission_ctx's `type`).
#ifndef GAME_MODE_REC_H
#define GAME_MODE_REC_H
#include "./tstring.h"

struct stream_base;
struct tstream3;

struct SpawnRec {                               // 0x4c bytes: a mission spawn rule
    int id;                                     // +0x00
    int flags;                                  // +0x04 (see rule_accepts / pick_spawn)
    union {
        unsigned flags2;                        // +0x08
        int override_flags;                     // +0x08 (script mission_tpl_entry_set_override_flags 0x4aa79a)
    };
    union {
        int padc[4];
        struct { int f0c, f10, f14, f18; };     // +0x0c (mesh_slot/mesh_start.cpp 0x484590 reads f14, f18)
    };
    int f1c;                                    // +0x1c
    char pad20[4];
    int num_override_items;                     // +0x24 (script 0x4aa7d0 family)
    int override_items[8];                      // +0x28
    int pad48;
    void load_extras(stream_base& s);           // 0x4bdd50 (triggers/trigger_vec.cpp, the mission_templates loader; config.cpp declares it on psys_record)
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
        int type;                               // inventory_panel/mission_ctx.cpp
    };
    int f40, f44, f48;                          // +0x40
    int f4c;                                    // +0x4c the TriggerVec (triggers/trigger_vec.h: count, data, cur at +0x4c / +0x50 / +0x54); the sound bank for mesh_start
    int f50;                                    // +0x50
    int f54;                                    // +0x54
    MissionTemplate() : spawns(0), f3c(-1), f4c(0), f50(0), f54(0) {}   // the module loader's inline constructor (inventory_panel/module_files.cpp)
    void read(stream_base& s);                  // 0x4c2300 (ret 4)
    void read_trigger_block(tstream3* s);       // 0x4c2300 (the module loader's spelling: inventory_panel/module_files.cpp)  // old name of MissionTemplate::read, removed after the names merge
    void destroy_array(int flags);              // 0x4c3430
};
typedef MissionTemplate ModeRec2;               // the slices' names for the same record
typedef ModeRec2 ModeRec;                       // agent.h's name for the same record
extern ModeRec2* g_modes2;                      // 0x5e2a00 (g_modes of agent.h, g_mission_templates of script)  // old name of g_mission_templates, removed after the names merge

#endif // GAME_MODE_REC_H
