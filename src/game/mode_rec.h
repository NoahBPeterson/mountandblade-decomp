// mode_rec.h: the mission mode table (0x58-byte records, array g_modes at 0x5e2a00, indexed by Mission::mode +0x5df0) and
// the spawn rules (0x4c bytes each) its +0x34 pointer leads to. ONE definition for src/game: agent.cpp reads the flags at
// +0x20 (Mission::mode_flag1 0x435840), terrain_b the rule table (rule_accepts / pick_spawn 0x473a60, 0x467940), terrain_a
// (0x4677a0) the rule record by index, script the template entries (MissionTemplate: entries +0x34, count +0x38).
// Names provisional (the slices called the record ModeRec, ModeRec2 and MissionTemplate).
#ifndef GAME_MODE_REC_H
#define GAME_MODE_REC_H

struct stream_base;

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
struct ModeRec2 {                               // 0x58 bytes
    char pad0[0x20];
    unsigned flags;                             // +0x20 bit 1 = mode_flag1 (0x435840)
    char pad24[0x34 - 0x24];
    SpawnRec* recs;                             // +0x34 -> 0x4c-byte records (0x473a60, 0x4677a0)
    int nrecs;                                  // +0x38 (script: MissionTemplate::num_entries)
    char pad3c[0x58 - 0x3c];
};
typedef ModeRec2 ModeRec;                       // agent.h's name for the same record
extern ModeRec2* g_modes2;                      // 0x5e2a00 (g_modes of agent.h, g_mission_templates of script)

#endif // GAME_MODE_REC_H
