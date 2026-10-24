// main_menu.h: the screen / menu manager object at 0xa73980 (src/game/world_data/world_data.h calls it WindowMgrW: 30 screen
// objects, the open-screen deque at +0x78; script_interpreter.h WindowMgr; options_panel.h OptMainMenu). ONE definition of the
// callers' `MainMenu` view for src/game/party, char_sheet, inventory_panel, options_panel and the other modules that call into it:
//   choose 0x495700 (thiscall, ret 4: pick a menu option / leave the screen state), reset 0x494260 (ret 4: open a screen),
//   set_texts 0x48a3a0 (ret 8) and ask 0x48a3d0 (ret 0x10) on the record the first dword points to (the dialog box).
// Fields: f0 (+0: the dialog box pointer, a raw int for the callers that cast it), f4 (+4: the open flag of the box record when the
// callers view the box itself), sel (+0x18), f4c / f50 (+0x4c, +0x50). The text parameters are untyped pointers: the option pages
// build their tstring from rgl::string, the other modules from src/game/tstring.h; any tstring* converts implicitly.
// The cls_map main-menu SCREEN (create 0x4dcef0) is a different object and is named MainMenuScreen there; the tutorial menu record of
// inventory_panel/char_sheet.cpp (+0x1c / +0x68 texts) is TutorialMenuRec.
#ifndef GAME_MAIN_MENU_H
#define GAME_MAIN_MENU_H

struct MainMenu {
    int f0;                                         // +0x00 (the dialog box record pointer)
    int f4;                                         // +0x04
    char pad8[0x18 - 8];
    int sel;                                        // +0x18
    char pad1c[0x4c - 0x1c];
    int f4c;                                        // +0x4c
    int f50;                                        // +0x50
    void ask(const void* question, const void* yes, const void* no, int* result);   // 0x48a3d0 (ret 0x10)
    void set_texts(const void* explanation, const void* name);                       // 0x48a3a0 (ret 8)
    void reset(int n);                              // 0x494260 (ret 4)
    void choose(int option);                        // 0x495700 (ret 4)
};
extern MainMenu g_main_menu;                        // 0xa73980

#endif // GAME_MAIN_MENU_H
