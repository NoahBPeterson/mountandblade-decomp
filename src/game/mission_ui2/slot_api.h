// slot_api.h: classes of slot_api.cpp (0x48e5d0..0x48ef40): the animation slot vtable the overlay animators call, the
// string-record tables, the level grid query and the page table of 0x98-byte records. Names provisional; every offset cites
// the function that reads it. The widget manager's members are in src/game/config/widget_mgr.h, the game data's in
// src/include/game_data.h, the mission's in src/game/mission.h.
#ifndef GAME_MISSION_UI2_SLOT_API_H
#define GAME_MISSION_UI2_SLOT_API_H
#include "../tstring.h"
#include "../strat/gui_manager.h"     // gui_manager (push 0x4334b0)
#include "../string_table.h"          // StringTable::get_string 0x48e8f0, Str::resolve 0x48e8d0


// The level (global 0xa732b4) is the engine's rgl::world (src/rgl/scene/world.h): the grid of collision cells at +0x28 and the point locator.
#include "../../rgl/scene/world.h"

// The message log (global 0x5d7080, the same object as the UI sound host g_ui_sound in ui_widgets.h) and the error reporter.
void mb_error(const char* fmt, ...);        // 0x42cc80 (cdecl)

#endif // GAME_MISSION_UI2_SLOT_API_H
