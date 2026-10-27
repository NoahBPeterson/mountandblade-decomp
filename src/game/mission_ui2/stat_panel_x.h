// stat_panel_x.h: classes of stat_panel_x.cpp (0x48be00..0x48c330): the placeholder "Event" object. The inventory
// compaction, the attribute / skill setters and the inventory sort are members of ItemRef / Troop (src/game/item_ref.h,
// src/include/troop.h). Names provisional; offsets cite the function that reads them.
#ifndef GAME_MISSION_UI2_STAT_PANEL_X_H
#define GAME_MISSION_UI2_STAT_PANEL_X_H
#include "../tstring.h"

// highest skill rank / tier, read by Troop::set_attr and Troop::set_skill (bound through symbols/slices/stat_panel_x.txt: outside the image)
#include "../skill_cap.h"

// A scripted-event placeholder (vtable 0x5922cc, size 0x94): a name, a position/size record and the two clock readings.
// Constructor 0x48c210; init() is the object's vtable slot 1 (0x5922d0, no direct callers).
struct Event {
    virtual void v00();                     // +0x00 vptr 0x5922cc (0x48c210); slot 0 is not defined in this object
    virtual void init();                    // slot 1 (0x5922d0): 0x48c260
    char pad04[0x10 - 4];
    int f10;                                // +0x10 (0x48c260)
    tstring name;                           // +0x14 "An Event" (0x48c260), "" (0x48c210)
    int f24;                                // +0x24
    float f28;                              // +0x28
    char pad2c[0x34 - 0x2c];
    float f34;                              // +0x34
    float f38;                              // +0x38
    char pad3c[0x40 - 0x3c];
    float f40;                              // +0x40
    float f44;                              // +0x44
    char pad48[0x50 - 0x48];
    int f50;                                // +0x50
    char pad54[0x60 - 0x54];
    int f60;                                // +0x60
    char pad64[4];
    int f68;                                // +0x68 g_ticks[1] low word (0x48c210)
    int f6c;                                // +0x6c g_ticks[1] high word
    char pad70[0x80 - 0x70];
    int f80;                                // +0x80
    char pad84[4];
    int f88;                                // +0x88 g_ticks[0] low word
    int f8c;                                // +0x8c g_ticks[0] high word
    int f90;                                // +0x90
    Event();                                // 0x48c210
    virtual void s_last() {}                 // placeholder: original table 0x5922cc has 3 slots, ours 2
};

#endif // GAME_MISSION_UI2_STAT_PANEL_X_H
