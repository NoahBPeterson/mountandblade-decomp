// item_mod.h: the item modifier table (Data\item_modifiers.txt, 43 records of 0x28 bytes at 0xa75f70).
// Read by load_data_files 0x4d0c20; gui_edit exports it, inventory_panel formats item names with it, item_ref sorts by it.
#ifndef MB_ITEM_MOD_H
#define MB_ITEM_MOD_H
#include "../game/tstring.h"

struct item_mod {
    tstring name;       // +0     (underscores shown as spaces)
    tstring key;        // +0x10  read first
    float price;        // +0x20
    float rarity;       // +0x24
};
extern item_mod g_item_mods[43];     // 0xa75f70

#endif // MB_ITEM_MOD_H
