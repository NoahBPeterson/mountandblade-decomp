// item_rec.h: the 0x60-byte "item" records constructed at 0x4ce870 / 0x4ce8d0 (src/game/triggers/item_rec.cpp).
// Both are head_rec objects (src/game/head_rec.h: the nine real slots of vtable 0x5947d8) whose own vtable replaces
// the base one; the family shares one deleting destructor (0x4ce930, ICF). ALL NAMES PROVISIONAL except where the
// image's table names the target.
//
// Tables (dumped slot by slot):
//   0x596124 ItemRec   1 0x4c6e00 reset_gui | 2 0x4dcef0 MainMenuScreen::create | 3 0x4d5290 Hud::handle_buttons
//                      7 0x48b170 always1
//   0x596148 ItemRec2  1 0x4c6e00 reset_gui | 2 0x4dda40 EscapeMenu::create    | 3 0x4e87d0 MapScreen::buttons
//                      7 0x48b170 always1
// Slots 3/5/6, 8 and the empty slots 1/2/4 are the base's unless overridden above.
//
// TESTABILITY: nothing in the binary constructs these records - no reference to 0x4ce870 / 0x4ce8d0 exists, and each
// vtable is referenced only by its own constructor's vptr store - so their equivalence rows compare vtable bytes,
// not executed behaviour (docs/Papercuts.md).
#ifndef GAME_TRIGGERS_ITEM_REC_H
#define GAME_TRIGGERS_ITEM_REC_H
#include "../head_rec.h"

extern __int64 g_ticks[];                       // 0x5e2948 clock table, 1e-5 s ticks

// The owned block at +0x30 (three pointers, cleared by the constructors, released by the destructor).
struct owned_block {
    int f0;                                     // +0x30 (never initialised)
    void* begin;                                // +0x34 0x4ce870
    void* end;                                  // +0x38
    void* cap;                                  // +0x3c
    ~owned_block();                             // 0x4c8bf0 (src/rgl/mesh2/gui_edit.cpp conv_clear: releases the block)
};

struct ItemRec : head_rec {                     // vtable 0x596124
    char pad2c[4];                              // +0x2c
    owned_block owned;                          // +0x30
    int f40;                                    // +0x40 -1
    int pad44;
    int clock;                                  // +0x48 clock index (0)
    int pad4c;
    __int64 stamp;                              // +0x50 g_ticks[0]
    float f58;                                  // +0x58 1.0f
    int f5c;                                    // +0x5c 0
    virtual void slot_empty_1();                // slot 1  0x4c6e00 reset_gui        (gui_edit.cpp)
    virtual void screen_build();                // slot 2  0x4dcef0 MainMenuScreen::create (cls_map.cpp)
    virtual void screen_buttons();              // slot 3  0x4d5290 Hud::handle_buttons    (hud.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    ItemRec();                                  // 0x4ce870
};

struct ItemRec2 : head_rec {                    // vtable 0x596148
    char pad2c[4];
    owned_block owned;
    int f40;
    int pad44;
    int clock;
    int pad4c;
    __int64 stamp;
    float f58;
    int f5c;
    virtual void slot_empty_1();                // slot 1  0x4c6e00 reset_gui        (gui_edit.cpp)
    virtual void screen_build();                // slot 2  0x4dda40 EscapeMenu::create    (cls_map.cpp)
    virtual void screen_buttons();              // slot 3  0x4e87d0 MapScreen::buttons    (save_game.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170
    ItemRec2();                                 // 0x4ce8d0
};
#endif
