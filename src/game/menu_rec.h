// menu_rec.h: the game-menu record of menus.txt (0x60 bytes). ONE definition for src/game/triggers (trigger_vec.cpp: the
// module-file reader 0x4c2b20) and src/game/inventory_panel (module_files.cpp: the loader that allocates the table g_menus
// 0x5e29b0 with the inlined constructor and rejects it with the array destructor 0x4c4f40). The loader's slice view
// named the fields a, b, f20/f24, c, (f38, f3c, f40 + d = the DialogBlock), f54, f58, f5c.
#ifndef MB_GAME_MENU_REC_H
#define MB_GAME_MENU_REC_H
#include "./tstring.h"
#include "../include/dialog_rec.h"

struct stream_base;

// an entry of a game menu (0x68 bytes): its text, id and the translation key, the condition and the consequence blocks
struct menu_item {                                  // 0x68 bytes (config/config.cpp)
    tstring name;                                   // +0x00
    tstring text;                                   // +0x10 the door text (its translation key is key + "_door")
    tstring key;                                    // +0x20 the translation key
    DialogBlock   cond;                                   // +0x30
    DialogBlock   conseq;                                 // +0x4c
    void read(stream_base* s, const tstring* menu); // 0x4be310
    menu_item();                                    // 0x4be420
    tstring translated();                           // 0x4c62b0 (TroopRec::translated's body, folded: rgl/mesh2/gui_edit.cpp)
    tstring translated_door();                      // 0x4c6d00 (rgl/mesh2/gui_edit.cpp)
    tstring* door_text();                           // 0x4c6ca0 (rgl/mesh2/gui_edit.cpp): the table's text for key + "_door", else &text
};

struct MenuRec {            // module-file game menu (0x4c2b20)
    tstring text;           // +0x00 (underscores become spaces)
    tstring id;             // +0x10
    __int64 flags;          // +0x20
    tstring mesh;           // +0x28
    DialogBlock ops;        // +0x38 operations (items +0x38, count +0x3c, done +0x40, text +0x44)
    int     count;          // +0x54
    menu_item* items;       // +0x58 (zeroed by the inlined constructor in module_files; +0x5c is padding to 0x60)
    MenuRec() : items(0) {}
    // inline: `delete[] g_menus` (cls_map.cpp FinalCleanup, module_files.cpp) emits it into the vector deleting destructor
    // 0x4c4f40; the items go through menu_item's vector deleting destructor 0x431810 (implicit ~menu_item 0x431660)
    ~MenuRec() { delete[] items; }
    void read(stream_base& s);                  // 0x4c2b20 (ret 4)
};

extern MenuRec* g_menus; // 0x5e29b0
#endif // MB_GAME_MENU_REC_H
