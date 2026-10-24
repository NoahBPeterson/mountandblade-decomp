// menu_rec.h: the game-menu record of menus.txt (0x60 bytes). ONE definition for src/game/triggers (trigger_vec.cpp: the
// module-file reader 0x4c2b20) and src/game/inventory_panel (module_files.cpp: the loader that allocates the table g_menus
// 0x5e29b0 with the inlined constructor and rejects it with the array destructor 0x4c4f40). The loader's slice view
// named the fields a, b, f20/f24, c, (f38, f3c, f40 + d = the DialogBlock), f54, f58, f5c.
#ifndef MB_GAME_MENU_REC_H
#define MB_GAME_MENU_REC_H
#include "./tstring.h"
#include "../include/dialog_rec.h"

struct menu_item;
struct stream_base;

struct MenuRec {            // module-file game menu (0x4c2b20)
    tstring text;           // +0x00 (underscores become spaces)
    tstring id;             // +0x10
    __int64 flags;          // +0x20
    tstring mesh;           // +0x28
    DialogBlock ops;        // +0x38 operations (items +0x38, count +0x3c, done +0x40, text +0x44)
    int     count;          // +0x54
    menu_item* items;       // +0x58 (zeroed by the inlined constructor in module_files; +0x5c is padding to 0x60)
    MenuRec() : items(0) {}
    void read(stream_base& s);                  // 0x4c2b20 (ret 4)
    void destroy_array(int flags);              // 0x4c4f40 (the vector deleting destructor; flag 3 destroys the elements and frees the cookie-prefixed block)
};

#endif // MB_GAME_MENU_REC_H
