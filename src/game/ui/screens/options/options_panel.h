// options_panel.h: the options / character panel (0x4d7370..0x4d92c0): control-key names, the faction report, the
// character import / export flow and the character file writer / reader. Names provisional; layouts derived from
// the disassembly of the functions cited per field. The game object, the troop and faction records are the
// canonical Game / Troop / Faction (../game/campaign/game_state_load.h, ../../game/data/troop.h); the members those headers lack are
// reached through GameIo (a view of the same object with the two member functions of this module) and offsets.
#ifndef OPTIONS_PANEL_OPTIONS_PANEL_H
#define OPTIONS_PANEL_OPTIONS_PANEL_H
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "engine/core/tstring.h"
#include "game/campaign/game_state_load.h"
#include "game/data/troop.h"

extern const float RENOWN_SCALE;                                    // 0x54ab28 (100.0f)

// the game's localized-string table (0x5e2a18): lookup by a scratch tstring holding the text key
#include "game/data/str_table.h"

#include "game/input/slot.h"                                // Slot, g_keyboard (0xa170c8): key_name 0x4d7610

// a ui control: +0x2c set_cursor(bool), +0x44 is_key_down(int)
#include "game/ui/widgets/widget_base.h"                              // MissionUi: the panel's widgets

#include "game/ui/window_mgr.h"                                   // main_menu (the slice called it MenuRec: g_main_menu.rec, 0xa73980)
// g_main_menu (0xa73980, `rec` at +0 is the dialog box record): main_menu, ../main_menu.h

bool mb_file_exists(const char* path);                              // 0x415120

// the character file is read into the shared ini table (0x2c-byte entries: key +0, value +0x1c; ctor 0x4c1be0, load 0x4bd630, dtor 0x4be920)
#include "game/config/ini_file.h"

// the export file object is engine/io/stream.h's outfile (0x595990: log_stream's table with the read slots returning *this). Slot 10 (+0x28) writes a text line (0x4d86d0).
#include "engine/io/stream.h"

#define MODE_RA " RA"                                               // 0x595990
#define MODE_R  "r"                                                 // 0x54a8fc
#define MODE_W  "w"                                                 // 0x54b058

// the game object with this module's two member functions (GameData itself, no data of its own)
struct GameIo : GameData {
    void export_character(outfile* f);                              // 0x4d86d0 (ret 4)
    void import_apply(ini_file* table);                             // 0x4d8c20 (ret 4)
};

// (the faction report's members are Rec3c's: update_cursor 0x4d7de0, refresh_texts 0x4d7e00, draw_faction_report 0x4d7e50;
// export_path 0x4d9260 is Rec48's: ../screen_records.h. The control-key table is GameKeys, game/input/game_keys.h.)
#endif
