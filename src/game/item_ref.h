// item_ref.h: the (item, modifier) pair that refers to an item definition: the unit of every inventory, equipment slot and
// loot table (ItemRef, 8 bytes), with the modifier-adjusted stat accessors of src/game/agent/agent.cpp (0x434b20..0x435090,
// 0x437510), the ui helpers of src/game/ui (0x462900.., 0x489070..) and the script editors (0x48c5a0..). ONE definition for
// src/game: the slices called it `Pair` (s00435490, s00435c10, s004362e0), `ItemRef` and, where a module added members to
// the shared one, `ItemRefX`.
// Layout: item index + packed word (amount in the low byte, proficiency in the low 16 bits for weapons, modifier in
// byte 3 = byte 7 of the pair). Returned by value through a hidden pointer (0x4351f0, 0x432790).
#ifndef GAME_ITEM_REF_H
#define GAME_ITEM_REF_H
#include "./tstring.h"
#include "../include/item_kind.h"

struct ItemRef {
    int item;                           // +0 index into g_items, -1 = none (0x434b70, 0x435fc0, 0x462900, 0x463030)
    union {
        unsigned m4;                    // +4 as a word (0x434be0: amount in the low byte; copied whole by 0x463070, 0x463100)
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;   // mod: byte 7 (0x434c50, 0x462960)
    };
    tstring mesh_name();                // 0x434b20
    tstring mesh_name(unsigned __int64 cls);    // 0x462900 (ui/item_ref.cpp)
    int max_amount() const;             // 0x434b70
    void add_amount(int n);             // 0x434be0
    int room_left();                    // 0x434c30
    int stat_1ec();                     // 0x434c50
    int stat_21x(int which);            // 0x434cb0
    int stat_1f0a();                    // 0x434d70
    int stat_1e8();                     // 0x434df0
    int stat_200();                     // 0x434eb0
    int stat_1f4();                     // 0x434f60
    int stat_1f0b();                    // 0x434fd0
    int stat_214();                     // 0x435040
    int stat_204();                     // 0x435090 (agent.cpp), 0x462a10 (ui/item_ref.cpp)
    int stat_1fc();                     // 0x462960 (ui/item_ref.cpp)
    void reduce(int n);                 // 0x437510 (0x438520 inlines its body with n == 0)
    void set_modifier(int k);           // 0x438bc0 (stores the modifier in the top byte, then reduce(0))
    void set_item(int item);            // 0x48c5a0 (ret 4)
    void randomize_modifier(float quality);   // 0x48c5c0 (ret 4)
    void check_slots(int n);            // 0x489070 (sort helper: scans for the first empty pair; see mission_ui.cpp)
    void sort_sel(int n);               // 0x489090 descending selection sort by (item, g_sort_val[mod * 5])
    void compact(int n);                // 0x48be00 (this = the first of n refs, n <= 0 means 0x60: moves the used refs to the front, then sort_sel)
};

#endif // GAME_ITEM_REF_H
