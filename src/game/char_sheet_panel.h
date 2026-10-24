// char_sheet_panel.h: the character sheet panel (CharSheet; the window 0x4ef420 / 0x4eeb10 / 0x4ef7d0 / 0x4ef9f0 / 0x4f0820 / 0x4f06f0 act
// on and the opener 0x507400 builds). ONE definition for char_sheet (cs_update, cs_troop, cs_detail, cs_panels, cs_weapons: each
// slice saw the part of the layout its functions touch) and inventory_panel/char_sheet.cpp (the opener, which typed the widget
// rows CharSheetWidget, a row object with its own data). Offsets agree wherever the views overlap (+0x14 troop index, +0x2d0 row
// count, +0x2d4 selected row, +0x2d8 page, +0x42c.. buttons, +0x478/+0x494 proficiency rows, +0x4b0.. panels, +0x52c face page);
// the anonymous unions keep every slice's spelling of the same slot. The widget rows are Widget pointers (src/game/ui_widget.h);
// the opener's rows are CharSheetWidget pointers (forward declared here, completed by its TU).
// Needs the includer's tstring.
#ifndef MB_GAME_CHAR_SHEET_PANEL_H
#define MB_GAME_CHAR_SHEET_PANEL_H
#include "./ui_widget.h"

struct CharSheetWidget;

struct CharSheet {
    int vptr;                                       // +0x00
    union { int f_4; int flag4; };                  // +0x04 (flag4: a record was supplied by the caller)
    union { int f_8; int flag8; };                  // +0x08 (flag8: already initialised once)
    char pad0c[0x14 - 0xc];
    union { int f_14; int player; };                // +0x14 (edited troop index = the character shown)
    int f_18;                                       // +0x18 (sound / message id)
    char pad1c[0x2c - 0x1c];
    CharSheetWidget* window;                        // +0x2c (the 0x190-byte panel)
    union {
        Widget* f_30[10];                           // +0x30 (skill-row number labels)
        CharSheetWidget* skill_row[7];              // +0x30 (num_skills rows actually used)
    };
    char pad58[0xd8 - 0x58];
    Widget* label[0x2a];                            // +0xd8 (rank text per skill row)
    union {
        Widget* f_180[10];                          // +0x180 (skill rows)
        Widget* btn[0x2a];                          // +0x180 (upgrade button per row)
    };
    union {
        int f_228[10];                              // +0x228 (skill id of each skill row)
        int skill_id[0x2a];
    };
    union { int f_2d0; int num_skills; };           // +0x2d0 (number of skill rows)
    union { int f_2d4; int f2d4; };                 // +0x2d4 (selected row, -1 = none)
    union { int f_2d8; int f2d8; };                 // +0x2d8 (page: 0 attributes, 1 skills, 2 profs, 3 face)
    union { int f_2dc; int mode; };                 // +0x2dc (done state: 0 idle, 1 waiting / 0 = new game, 1 = tutorial)
    char pad2e0[0x2e4 - 0x2e0];
    int f_2e4;                                      // +0x2e4 (the world stepped each frame)
    char pad2e8[0x42c - 0x2e8];
    union { Widget* f_42c; CharSheetWidget* done; };        // +0x42c (done/cancel buttons)
    union { Widget* f_430; CharSheetWidget* reset; };       // +0x430 (skills page panel)
    union { Widget* f_434; CharSheetWidget* explanation; }; // +0x434 (description box)
    char pad438[0x440 - 0x438];
    union { Widget* f_440; CharSheetWidget* stats; };       // +0x440 (back to the army button)
    union { Widget* f_444; CharSheetWidget* next; };        // +0x444 (next/previous troop)
    union { Widget* f_448; CharSheetWidget* prev; };        // +0x448
    union {
        Widget* f_44c[4];                           // +0x44c (attribute left buttons)
        Widget* skill_pos[4];
        CharSheetWidget* attrib_row[4];
    };
    union {
        Widget* f_45c[4];                           // +0x45c (attribute right buttons)
        Widget* skill_val[4];
    };
    union { Widget* f_46c; CharSheetWidget* weapon_title; };  // +0x46c
    CharSheetWidget* attrib_title;                  // +0x470
    CharSheetWidget* skill_title;                   // +0x474
    union {
        Widget* f_478[7];                           // +0x478 (proficiency number labels)
        Widget* labels[7];                          // +0x478 (caption line per weapon class)
        CharSheetWidget* weapon_row[7];
    };
    union {
        Widget* f_494[7];                           // +0x494 (proficiency rows)
        Widget* rows[7];                            // +0x494 (one widget per weapon class)
    };
    union { Widget* level_at; CharSheetWidget* attribs; };  // +0x4b0 (next-level / level / attribute lines)
    union { Widget* xp_bar; CharSheetWidget* general; };    // +0x4b4 (experience bar)
    union { Widget* f_4b8; CharSheetWidget* skill_desc; };  // +0x4b8 (title label)
    union { Widget* f_4bc; CharSheetWidget* skills_outer; };// +0x4bc (scroll/slider bar)
    CharSheetWidget* skills;                        // +0x4c0
    union { Widget* f_4c4; CharSheetWidget* name; };// +0x4c4 (portrait / name)
    union { char f_4c8; char f4c8; };               // +0x4c8 (skills visible)
    char pad4c9[0x4cc - 0x4c9];
    Widget* f_4cc;                                  // +0x4cc (edit face button)
    char pad4d0[0x4d4 - 0x4d0];
    union { char f_4d4; char f4d4; };               // +0x4d4 (skills page / dirty flag)
    char pad4d5[0x4d8 - 0x4d5];
    int f_4d8, f_4dc, f_4e0, f_4e4, f_4e8, f_4ec, f_4f0, f_4f4, f_4f8, f_4fc;
    int f_500;
    int f_504;                                      // +0x504 (level)
    int f_508;                                      // +0x508 (skill points)
    int f_50c;                                      // +0x50c (strength)
    int f_510;                                      // +0x510 (agility)
    int f_514, f_518, f_51c, f_520, f_524, f_528;   // +0x514 (skill values, see s004eeb10)
    union { char f_52c; char f52c; };               // +0x52c (face page visible)

    void update(float dt);                          // 0x4eeb10
    void apply_to_troop();                          // 0x4ef420
    void update_skill_rows();                       // 0x4ef7d0
    void update_detail();                           // 0x4ef9f0
    void update_texts();                            // 0x4f0820  // old name of CharSheet::refresh_panels, removed after the names merge
    void update_weapons();                          // 0x4f06f0
    void refresh_panels();                          // 0x4f0820 (cs_update's name for update_texts)
    void refresh_lists();                           // 0x4f06f0 (cs_update's name for update_weapons)  // old name of CharSheet::update_weapons, removed after the names merge
    void init();                                    // 0x507400
};

#endif // MB_GAME_CHAR_SHEET_PANEL_H
