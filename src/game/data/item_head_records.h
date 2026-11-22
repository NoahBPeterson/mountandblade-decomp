// item_head_records.h: the 0x60-byte "item" records constructed at 0x4ce870 / 0x4ce8d0 (src/game/data/item_head_records.cpp).
// Both are head_rec objects (src/game/data/head_rec.h: the nine real slots of vtable 0x5947d8) whose own vtable replaces
// the base one; the family shares one deleting destructor (0x4ce930, ICF). ALL NAMES PROVISIONAL except where the
// image's table names the target.
//
// Tables (dumped slot by slot):
//   0x596124 ItemRec   1 0x4c6e00 reset_gui | 2 0x4dcef0 MainMenuScreen::create | 3 0x4d5290 Hud::handle_buttons
//                      7 0x48b170 always1
//   0x596148 ItemRec2  1 0x4c6e00 reset_gui | 2 0x4dda40 EscapeMenu::create    | 3 0x4e87d0 MapScreen::buttons
//                      7 0x48b170 always1
// Slots 2 and 3 override init_screen / frame_update (forwarding stubs in winmain_and_menu_screens.cpp, mission_hud_buttons.cpp, save_game.cpp).
// Slots 3/5/6, 8 and the empty slots 1/2/4 are the base's unless overridden above.
//
// TESTABILITY: nothing in the binary constructs these records - no reference to 0x4ce870 / 0x4ce8d0 exists, and each
// vtable is referenced only by its own constructor's vptr store - so their equivalence rows compare vtable bytes,
// not executed behaviour (docs/Papercuts.md).
#ifndef GAME_TRIGGERS_ITEM_REC_H
#define GAME_TRIGGERS_ITEM_REC_H
#include "game/data/head_rec.h"
#include "engine/core/timer.h"
struct ContainerBase;
struct MissionUi;


#include "game/data/presentation.h"                // widget_mgr: the presentation at +0x30

struct ItemRec : head_rec {                     // vtable 0x596124 (the main menu)
    ContainerBase* win;                         // +0x2c the window
    widget_mgr pres;                            // +0x30 the menu's presentation (prsnt_game_start: init_screen 0x4dcef0)
    MissionUi* row_tutorial;                    // +0x60 the main menu's rows (frame_update 0x4d5290 tests their used flags)
    MissionUi* row_new_game;                    // +0x64
    MissionUi* row_restore;                     // +0x68
    MissionUi* row_custom;                      // +0x6c (may be null)
    MissionUi* row_options;                     // +0x70
    MissionUi* row_credits;                     // +0x74
    MissionUi* row_exit;                        // +0x78
    MissionUi* version;                         // +0x7c
    MissionUi* logo;                            // +0x80
    MissionUi* pic;                             // +0x84
    int     first_free;                         // +0x88
    char    pad8c[0x90 - 0x8c];
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui        (editor_selection_and_translation.cpp)
    virtual void init_screen();                // slot 2  0x4dcef0 MainMenuScreen::create (winmain_and_menu_screens.cpp)
    virtual void frame_update(float);         // slot 3  0x4d5290 Hud::handle_buttons    (mission_hud_buttons.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    ItemRec();                                  // 0x4ce870
};

struct ItemRec2 : head_rec {                    // vtable 0x596148 (the escape menu)
    ContainerBase* win;                         // +0x2c the window
    widget_mgr pres;                            // +0x30 the menu's presentation (prsnt_game_escape: init_screen 0x4dda40)
    MissionUi* row0;                            // +0x60
    MissionUi* row1;                            // +0x64
    MissionUi* row2;                            // +0x68
    MissionUi* row3;                            // +0x6c
    MissionUi* row4;                            // +0x70
    MissionUi* row5;                            // +0x74
    MissionUi* logo;                            // +0x78
    char pad7c[0x80 - 0x7c];
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui        (editor_selection_and_translation.cpp)
    virtual void init_screen();                // slot 2  0x4dda40 EscapeMenu::create    (winmain_and_menu_screens.cpp)
    virtual void frame_update(float);         // slot 3  0x4e87d0 MapScreen::buttons    (save_game.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170
    ItemRec2();                                 // 0x4ce8d0
};
#endif
