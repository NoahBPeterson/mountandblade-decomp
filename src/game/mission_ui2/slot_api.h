// slot_api.h: classes of slot_api.cpp (0x48e5d0..0x48ef40): the animation slot vtable the overlay animators call, the
// string-record tables, the level grid query and the page table of 0x98-byte records. Names provisional; every offset cites
// the function that reads it. The widget manager's members are in src/game/config/widget_mgr.h, the game data's in
// src/include/game_data.h, the mission's in src/game/mission.h.
#ifndef GAME_MISSION_UI2_SLOT_API_H
#define GAME_MISSION_UI2_SLOT_API_H
#include "../tstring.h"
#include "../strat/gui_manager.h"     // gui_manager (push 0x4334b0)
#include "../string_table.h"          // StringTable::get_string 0x48e8f0, Str::resolve 0x48e8d0

// A widget as the overlay animators see it: the animation slots of its vtable take (seconds, value).
struct AnimVt {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void animate_color(float a, int b);       // +0x10 colour (0x48e650)
    virtual void animate_alpha(float a, int b);       // +0x14 alpha, b << 24 (0x48e6d0)
    virtual void animate_hilight_color(float a, int b);       // +0x18 highlight colour (0x48e750)
    virtual void animate_hilight_alpha(float a, int b);       // +0x1c highlight alpha, b << 24 (0x48e7d0)
    virtual void animate_size(float a, int b);       // +0x20 size (0x48e850)
    virtual void animate_pos(float a, int b);       // +0x24 position (0x48e5d0)
};

// The level (global 0xa732b4) is the engine's rgl::world (src/rgl/scene/world.h): the grid of collision cells at +0x28 and the point locator.
#include "../../rgl/scene/world.h"

// A table of pages of 0x98-byte records: pages at +0x18 (16 records per page).
struct PageTable98 {
    char pad0[0x18];
    void** pages;                           // +0x18
    void* slot(unsigned index);             // 0x48ef20
};

// The message log (global 0x5d7080, the same object as the UI sound host g_ui_sound in ui_widgets.h) and the error reporter.
void mb_error(const char* fmt, ...);        // 0x42cc80 (cdecl)

#endif // GAME_MISSION_UI2_SLOT_API_H
