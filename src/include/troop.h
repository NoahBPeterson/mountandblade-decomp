// troop.h: the troop record (Troop, 0x508 bytes; array behind GameData+0x13a00, game_data.h). ONE definition for src/game
// (agent, agent2, agent3, ui, party, script, terrain_b, dialog_manager) and src/rgl (gui_edit.cpp, game_util.cpp). The
// slices and modules called it Troop (agent.h, ui/troop_inv.h, terrain_b/game_data.h), TroopRec (agent2 / agent3 /
// dialog_manager / gui_edit) and ItemDef (game_util.h); every name is this record. TroopRec derives from Troop and only adds the
// members of the language-export side (gui_edit.cpp) and display_name (agent2): the layout is Troop's.
// Where two modules read one offset with different names or types, the fields overlay in anonymous unions of plain
// members (the compiler emits the same loads as for the separate declarations): +0x30 flags as 64 bit unsigned /
// signed or its low dword; +0x80 the per-slot values as eight or twelve floats or as exp / hp / fa4.. ints.
// Offsets cite the function that proves them. Needs ItemRef (src/game/item_ref.h).
#ifndef MB_TROOP_H
#define MB_TROOP_H
#include <stddef.h>
#include "../game/tstring.h"
#include "../game/item_ref.h"

struct Troop {
    tstring name;                           // +0x00 (the first member: 0x51c720 reads the record as a tstring; agent2 / agent3 called it `id`, 0x43a480, 0x4428c0)
    tstring plural;                         // +0x10 (gui_edit.cpp save_language_files 0x4c8ca0)
    tstring key;                            // +0x20 translation key (0x460160, 0x4c62b0; agent2 / agent3 called it `name`)
    union {
        unsigned __int64 flags30;           // +0x30 (bit 0x10: hero / fixed stock)   0x435130, 0x462b60, 0x462c80, 0x462e20
        __int64 flags30s;                   // +0x30 as signed (0x10 = hero, 0x1000000 / 0x4000000 = troop class bits)   0x473a60
        unsigned flags32;                   // +0x30 low dword (low nibble 1 = female, low nibble = class)   0x460160, 0x443870
    };
    char pad38[0x50 - 0x38];
    unsigned f50;                           // +0x50 linked / leader troop index (chain end 0)   0x48a080
    char pad54[4];
    union {
        int f58[2];                         // +0x58 skill slots   0x462c00
        int lvl58;                          // +0x58
    };
    char pad60[4];
    unsigned f64;                           // +0x64 (low byte = alpha / colour id)   0x489370
    unsigned nib[6];                        // +0x68 packed 4-bit skill levels, 8 per word (0x4351a0, 0x437b20); nib[3] bits 12..15 = a skill tier (0x4375b0)
    union {
        float stat[12];                     // +0x80 attribute accumulators (clamped to 0..1000 by add_stat)   0x462c00, 0x4892d0
        float f80[8];                       // +0x80 per-slot / per-class values (0x435150 truncates one, 0x437770 adjusts one, 0x4385e0, 0x43bdb0)
        struct {
            char pad80[0x1c];
            int exp;                        // +0x9c   0x4428c0
            float hp;                       // +0xa0 heroes
            int fa4, fa8, fac;              // +0xa4 (fac += 10 per level)   0x4428c0
        };
    };
    union {
        int fb0;                            // +0xb0 troop size   0x48a080, 0x48a290
        int b0;                             // +0xb0   0x435170
        int level;                          // +0xb0
    };
    ItemRef inv[0x60];                      // +0xb4 inventory (inv_count() slots, at most 0x60)   0x462da0, 0x462e20, 0x463030
    int f3b4;                               // +0x3b4 (clamped at 0 by 0x489340)
    union {
        int f3b8;                           // +0x3b8 (packed by 0x489320)
        unsigned u3b8;                      // +0x3b8 (low 16 = party/side id, bits 16..23 = rule id)
    };
    int used;                               // +0x3bc
    char pad3c0[0x4a4 - 0x3c0];
    union {
        ItemRef equip[10];                  // +0x4a4 equipment slots   0x4630d0, 0x463100, 0x489c50
        struct { char pad4a4[0x40]; int f4e4; };    // +0x4e4 (equip[8].item)
    };
    char pad4f4[0x508 - 0x4f4];

    bool has_flag10();                      // 0x435130
    int stat_int(int i);                    // 0x435150
    int scaled_b0();                        // 0x435170
    int nibble(int i);                      // 0x4351a0 (packed 4-bit field i of the skill words)
    float falloff(float dist, float k);     // 0x4375b0
    void adjust(int i, float k);            // 0x437770
    int inv_count();                        // 0x462b60 number of inventory slots: 90, or 30..96 by skill nibble 12 for heroes
    int slot_value(int item);               // 0x462c00
    bool fits_slot(ItemRef* ref, int slot); // 0x462c80
    bool item_matches(int idx, int slot, unsigned need, unsigned forbid, int t1, int t2, int t3);   // 0x462da0
    int pick(int seed, int slot, unsigned need, unsigned forbid, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x462e20
    int find_free();                        // 0x463030
    bool add(ItemRef* s);                   // 0x463070
    void set_flags_low(int v);              // 0x4892a0 (replaces the low nibble of flags30)
    void add_stat(unsigned i, int amt);     // 0x4892d0
    void set_colors(int a, int b);          // 0x489320
    void sub_3b4(int amt);                  // 0x489340
    void set_alpha(int a);                  // 0x489370
    void unequip(int i);                    // 0x4630d0
    bool equip_best(int seed, int slot, unsigned need, unsigned forbid, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x463100
    // ---- src/game/mission_ui2 (method declarations only: no layout change) ----
    void set_attr(int idx, int val);        // 0x48be70 (attribute idx 0..3 at +0x58.. clamped to 0..100; then the +0xa0 minimum is raised)
    void add_attr(int idx, int delta);      // 0x48bef0 (ret 8; steps by one through set_attr)
    void set_skill(int idx, int val);       // 0x48bf90 (packed 4-bit skill idx in the words at +0x68)
    void add_skill(int id, int delta);      // 0x48c090 (ret 8)
    void sort_inv();                        // 0x48c0c0 (compacts the inventory, then a bubble pass by item value)
    void set_slot(int item, int idx, int mod);   // 0x48c7f0 (ret 0xc)
};

struct TroopRec : Troop {
    tstring* display_name();                // 0x43a480 (the record's own name, or its key when it has none; shared with ItemKind)
    tstring translated();                   // 0x4c62b0
    tstring translated_plural();            // 0x4c6330
    tstring* plural_name();                 // 0x48d6d0
};

#endif // MB_TROOP_H
