// slot_api.h: classes of slot_api.cpp (0x48e5d0..0x48ef40): the animation slot vtable the overlay animators call, the
// string-record tables, the level grid query and the page table of 0x98-byte records. Names provisional; every offset cites
// the function that reads it. The widget manager's members are in src/game/config/widget_mgr.h, the game data's in
// src/include/game_data.h, the mission's in src/game/mission.h.
#ifndef GAME_MISSION_UI2_SLOT_API_H
#define GAME_MISSION_UI2_SLOT_API_H
#include "../tstring.h"

// A widget as the overlay animators see it: the animation slots of its vtable take (seconds, value).
struct AnimVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(float a, int b);       // +0x10 colour (0x48e650)
    virtual void v14(float a, int b);       // +0x14 alpha, b << 24 (0x48e6d0)
    virtual void v18(float a, int b);       // +0x18 highlight colour (0x48e750)
    virtual void v1c(float a, int b);       // +0x1c highlight alpha, b << 24 (0x48e7d0)
    virtual void v20(float a, int b);       // +0x20 size (0x48e850)
    virtual void v24(float a, int b);       // +0x24 position (0x48e5d0)
};

// A 0x20-byte string record: the name at +0, the translation key at +0x10.
struct NameRec {
    tstring name;                           // +0
    tstring key;                            // +0x10
    tstring* resolve();                     // 0x48e8d0
};

// The string tables (global 0xa75ea0): two arrays of NameRec with their own counts.
struct NameTable {
    NameRec* base;                          // +0   named strings (0x48e8f0, kinds other than 0 / 0x16)
    NameRec* base2;                         // +4   quick strings (kind 0x16)
    int count;                              // +8
    int count2;                             // +0xc
    tstring get_string(int index, int kind);    // 0x48e8f0 (ret 0xc: hidden return)
};

// The level (global 0xa732b4): the grid of collision cells (src/game/cell.h) at +0x28 and the point locator.
struct cell;
struct LevelGrid {
    char pad0[0x28];
    cell** cells;                           // +0x28 (0x48ea80: cells[row] + col * 0x34)
    void locate(const float* p, int* out);  // 0x41ed00 (grid cell of a point)
};

// A table of pages of 0x98-byte records: pages at +0x18 (16 records per page).
struct PageTable98 {
    char pad0[0x18];
    void** pages;                           // +0x18
    void* slot(unsigned index);             // 0x48ef20
};

// The message log (global 0x5d7080, the same object as the UI sound host g_ui_sound in ui_widgets.h) and the error reporter.
struct MsgLogV {
    void push(const tstring& s, int colour);    // 0x4334b0
};
extern MsgLogV g_msglog;                    // 0x5d7080
void mb_error(const char* fmt, ...);        // 0x42cc80 (cdecl)

#endif // GAME_MISSION_UI2_SLOT_API_H
