// main_menu.h: the screen / menu manager object at 0xa73980 (g_main_menu; the main-menu / message-box record). ONE definition for
// src/game/party, char_sheet, inventory_panel, options_panel and the other modules that call into it (the callers' views were
// named MainMenu, main_menu, MenuRec, WindowMgrW, WindowMgr and OptMainMenu; world_data.h calls it WindowMgrW: 30 screen objects,
// the open-screen deque at +0x78). The record: +0x00 the dialog box pointer (a raw int for the callers that cast it), +0x04 "open"
// (the page selector / the open flag of the box record), +0x18 sel, +0x1c title, +0x4c / +0x50 selection state, +0x54 the slot-name
// string of the save dialog, +0x68 header, +0x78 / +0x88 the answers.
//   choose 0x495700 (thiscall, ret 4: pick a menu option / leave the screen state), reset 0x494260 (ret 4: open a screen),
//   set_texts / setup 0x48a3a0 (ret 8) and ask / ask_yes_no 0x48a3d0 (ret 0x10) on the dialog box record.
// The method families are spelled twice on purpose: the typed ones (setup, ask_yes_no, taking src/game/tstring.h strings; setup is
// inline) and the untyped ones (set_texts, ask: the option pages build their tstring from rgl::string; any tstring* converts).
// The cls_map main-menu SCREEN (create 0x4dcef0) is a different object and is named MainMenuScreen there; the tutorial menu record of
// inventory_panel/char_sheet.cpp (+0x1c / +0x68 texts) is TutorialMenuRec.
#ifndef MB_GAME_MAIN_MENU_H
#define MB_GAME_MAIN_MENU_H
#include "./tstring.h"   // tstring complete (the options_panel TUs' former typedef of rgl::string had the same 16-byte layout)

struct main_menu {
    union {
        int f0;                                                 // +0x00 the dialog box record pointer as a raw int
        main_menu* rec;                                         // +0x00 the same pointer (options_panel's OptMainMenu::rec; the record's methods are called through it)
        char pad0[4];
    };
    union {
        int  open;                                              // +0x04
        int  f4;
    };
    char pad08[0x18 - 0x08];
    int  sel;                                                   // +0x18
    tstring title;                                              // +0x1c
    char pad2c[0x4c - 0x2c];
    union {
        int  choice;                                            // +0x4c
        int  f4c;
    };
    union {
        int  v50;                                               // +0x50
        int  f50;
    };
    char pad54[0x68 - 0x54];
    tstring header;                                             // +0x68
    tstring answer_yes;                                         // +0x78
    tstring answer_no;                                          // +0x88
    void setup(const tstring* hdr, const tstring* text)         // 0x48a3a0 (ret 8)
    {
        header = *hdr;
        title = *text;
        choice = 0;
        v50 = 0;
        open = 0;
    }
    void ask_yes_no(const tstring* text, const tstring* yes, const tstring* no, int* result);  // 0x48a3d0 (ret 0x10)  // old name of NamedRec::setup2, removed after the names merge
    void ask(const void* question, const void* yes, const void* no, int* result);   // 0x48a3d0 (ret 0x10)  // old name of NamedRec::setup2, removed after the names merge
    void reset(int n);                                          // 0x494260 (ret 4)  // old name of WindowMgrW::open, removed after the names merge
    void set_state(int option);                                    // 0x495700 (ret 4)
};
typedef main_menu MainMenu;
extern main_menu g_main_menu;                                   // 0xa73980  // old name of g_windows, removed after the names merge

#endif // MB_GAME_MAIN_MENU_H
