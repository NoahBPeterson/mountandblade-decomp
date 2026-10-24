// mode_rec.h: the mission mode table (0x58-byte records, array g_modes at 0x5e2a00, indexed by Mission::mode +0x5df0) and
// the spawn rules (0x4c bytes each) its +0x34 pointer leads to. ONE definition for src/game: agent.cpp reads the flags at
// +0x20 (Mission::mode_flag1 0x435840), terrain_b the rule table (rule_accepts / pick_spawn 0x473a60, 0x467940), terrain_a
// (0x4677a0) the rule record by index, script the template entries (MissionTemplate: entries +0x34, count +0x38).
// Names provisional (the slices called the record ModeRec, ModeRec2 and MissionTemplate).
#ifndef GAME_MODE_REC_H
#define GAME_MODE_REC_H

#include "tstring.h"

struct stream_base;
struct tstream3;

struct SpawnRec {                               // 0x4c bytes: a mission spawn rule
    int id;                                     // +0x00
    int flags;                                  // +0x04 (see rule_accepts / pick_spawn)
    unsigned flags2;                            // +0x08
    union {
        int padc[4];
        struct { int f0c, f10, f14, f18; };     // +0x0c (mesh_slot/mesh_start.cpp 0x484590 reads f14, f18)
    };
    int f1c;                                    // +0x1c
    char pad20[0x4c - 0x20];
    void load_extras(stream_base& s);           // 0x4bdd50 (triggers/trigger_vec.cpp, the mission_templates loader; config.cpp declares it on psys_record)
};
// The record as the module loader (inventory_panel/module_files.cpp, load_module_files 0x50d5e0: `new ModeRec[n]`, the inline
// constructor, read_trigger_block 0x4c2300, array delete 0x4c3430) builds it, merged with the views of the other TUs: the
// loader's tstrings at +0 / +0x10 / +0x24 and the ints from +0x34 on (f34 = recs, f38 = nrecs, f3c = -1 at start, f4c..f54 =
// the triggers vector of trigger_vec.h's MissionTemplate: count / data / cursor at +0x4c / +0x50 / +0x54). The users that
// only dereference g_modes see the same offsets.
struct ModeRec2 {                               // 0x58 bytes
    tstring name;                               // +0x00
    tstring f10;                                // +0x10
    unsigned flags;                             // +0x20 bit 1 = mode_flag1 (0x435840)
    tstring display;                            // +0x24
    SpawnRec* recs;                             // +0x34 -> 0x4c-byte records (0x473a60, 0x4677a0)
    int nrecs;                                  // +0x38 (script: MissionTemplate::num_entries)
    int f3c;                                    // +0x3c (-1 at start)
    int f40, f44, f48;                          // +0x40
    int f4c;                                    // +0x4c
    int f50;                                    // +0x50
    int f54;                                    // +0x54
    ModeRec2() : recs(0), f3c(-1), f4c(0), f50(0), f54(0) {}
    void read_trigger_block(tstream3* s);       // 0x4c2300
    void destroy_array(int flags);              // 0x4c3430
};
typedef ModeRec2 ModeRec;                       // agent.h's name for the same record
extern ModeRec2* g_modes2;                      // 0x5e2a00 (g_modes of agent.h, g_mission_templates of script)

#endif // GAME_MODE_REC_H
