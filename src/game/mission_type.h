// mission_type.h: the mission-type table g_mission_types (0xa735d0): 11 records of 0x20 bytes read from Data\mission_types.txt
// (rec_tables.cpp Blk2dd0::read 0x4c2dd0): a name, two ints, and an owned array of options (name + flags + setup argument,
// 0x18 bytes each). The table is a global array: cl builds it with the vector constructor iterator over mission_type::mission_type
// (the name's tstring constructor) and destroys it at exit with ~mission_type (what the dynamic initialiser 0x547e60 and the
// atexit destructor 0x548360 did in the original); the module loader (options_panel/module_load.cpp) defines it.
#ifndef GAME_MISSION_TYPE_H
#define GAME_MISSION_TYPE_H
#include "./tstring.h"

struct MissionOpt {                         // 0x18 bytes: one start option of a mission type
    tstring name;                           // +0x00
    int flags;                              // +0x10 (MenuCtl::weight 0x5316f0 takes it)
    int setup_arg;                          // +0x14 (MissionCtx::setup 0x513f80)
};
struct mission_type {                       // 0x20 bytes
    tstring name;                           // +0x00
    int f10;                                // +0x10 (map_editor/exchange_screens.cpp 0x531600: the template for ids <= 6)
    int f14;                                // +0x14
    int count;                              // +0x18 number of options
    MissionOpt* list;                       // +0x1c array-new'd options
    ~mission_type() { if (count != 0) delete[] list; }
};
extern mission_type g_mission_types[11];    // 0xa735d0 .. 0xa73730 (defined in options_panel/module_load.cpp)

#endif // GAME_MISSION_TYPE_H
