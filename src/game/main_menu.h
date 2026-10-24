// main_menu.h: the main-menu / message-box record at 0xa73980 (g_main_menu): +0x04 "open" (the page selector), +0x1c title,
// +0x4c / +0x50 selection state, +0x54 the slot-name string of the save dialog, +0x68 header, +0x78 / +0x88 the answers.
// ONE definition for char_sheet (save_panel, cam_view, facegen_misc: the slices' main_menu) and options_panel (options_panel.h:
// the slice's MenuRec, "g_main_menu.rec"): the offsets agree wherever the views overlap, and both call the same methods
// (reset 0x494260, choose 0x495700; setup 0x48a3a0 / ask_yes_no 0x48a3d0 of the options view are the same record's).
#ifndef MB_GAME_MAIN_MENU_H
#define MB_GAME_MAIN_MENU_H
#include "./tstring.h"

struct main_menu {
    char pad0[4];
    int  open;                                                  // +0x04
    char pad08[0x1c - 0x08];
    tstring title;                                              // +0x1c
    char pad2c[0x4c - 0x2c];
    int  choice;                                                // +0x4c
    int  v50;                                                   // +0x50
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
    void ask_yes_no(const tstring* text, const tstring* yes, const tstring* no, int* result);  // 0x48a3d0 (ret 0x10)
    void reset(int a);                                          // 0x494260 (ret 4)
    void choose(int option);                                    // 0x495700 (ret 4)
};

#endif // MB_GAME_MAIN_MENU_H
