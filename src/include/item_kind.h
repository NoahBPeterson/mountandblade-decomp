// item_kind.h: the item definition record (ItemKind, 0x238 bytes, array g_items) with its mesh variants (ItemMesh) and the
// Module System item constants (itp_type_*, imod_*). ONE definition for src/game (agent/item_kind.cpp, agent.cpp, ui,
// party, terrain_a, script) and src/rgl/mesh2/gui_edit.cpp (the language export). The record is seen from three sides:
//   +0x00 / +0x10 / +0x20  the three names (gui_edit.cpp: save_language_files 0x4c8ca0, display_name 0x43a480; +0x20 is
//                          the key item_kind.cpp compares in find_item 0x434ad0 and falls back to in 0x433bf0);
//   +0x44.. +0x1c8..       the mesh table, type and capability flags and the modifier-adjusted stat bases (item_kind.cpp,
//                          agent.cpp);
//   +0x1d8 .. +0x228       the value, shop and sound words the ui modules read (item_ref.cpp, mission_ui.cpp).
// Every offset below is read by a function of one of those modules (cited); the rest is padding. Names provisional.
// Needs nothing from rgl: the carry-frame helper (pos2, identity_pos) lives in src/game/agent/item_kind.h.
#ifndef MB_ITEM_KIND_H
#define MB_ITEM_KIND_H
#include "../game/tstring.h"

// item type ids (Module System header_items.py itp_type_*), stored in the low byte of ItemKind::type_flags
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};
#define itp_food 0x00080000

// item modifiers (Module System header_item_modifiers.py)
enum {
    imod_plain = 0, imod_cracked, imod_rusty, imod_bent, imod_chipped, imod_battered, imod_poor, imod_crude, imod_old,
    imod_cheap, imod_fine, imod_well_made, imod_sharp, imod_balanced, imod_tempered, imod_deadly, imod_exquisite,
    imod_masterwork, imod_heavy, imod_strong, imod_powerful, imod_tattered, imod_ragged, imod_rough, imod_sturdy,
    imod_thick, imod_hardened, imod_reinforced, imod_superb, imod_lordly, imod_lame, imod_swaybacked, imod_stubborn,
    imod_timid, imod_meek, imod_spirited, imod_champion, imod_fresh, imod_day_old, imod_two_day_old, imod_smelling,
    imod_rotten, imod_large_bag
};

struct res_obj;
struct pos2;

// one mesh variant of an item: name + 64-bit flags (top nibble = class, low bits = modifier mask)
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18 (0x433bf0, 0x433cb0, 0x462820)

struct ItemKind {                       // 0x238 bytes
    tstring name;                       // +0x00 (gui_edit.cpp: the record's name in the language export, 0x4c62b0)
    tstring plural;                     // +0x10 (0x4c6330)
    tstring key;                        // +0x20 (0x433bf0 fallback name, 0x434ad0 find_item compares it; 0x43a480 display name)
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44 (0x433bf0, 0x433cb0, 0x462820)
    ItemMesh meshes[1];                 // +0x48 entries of 0x18 (0x433bf0, 0x433cb0, 0x462820)
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 low byte = itp_type_* (0x433d90, 0x433dc0, 0x433df0, 0x4350c0, 0x462c80, 0x462da0)
    char pad1cc[4];
    unsigned __int64 flags;             // +0x1d0 itcf_* capability flags, 64 bit (0x433e40, 0x4362e0, 0x435520)
    int value;                          // +0x1d8 price / sort value (0x462e20, 0x488f80, 0x489d10)
    char pad1dc[0x1e8 - 0x1dc];
    int f1e8, f1ec, f1f0, f1f4;         // +0x1e8 stat bases adjusted by the modifier (0x434c50, 0x434d70, 0x434df0, 0x434f60)
    char pad1f8[4];
    int f1fc;                           // +0x1fc (0x462960)
    int f200, f204;                     // +0x200 (0x434eb0, 0x435090, 0x462a10)
    float f208;                         // +0x208 value (0x436050; party_window.cpp reads it as the base armour value)
    int weight;                         // +0x20c (0x433e40)
    int f210;                           // +0x210 stack size (0x434b70)
    unsigned f214, f218;                // +0x214 low byte used (0x435040, 0x434cb0)
    float f21c;                         // +0x21c (set to 1.0 for every kind by 0x48a210)
    char pad220[4];
    float f224;                         // +0x224 (set for every kind by 0x48a250)
    char pad228[0x238 - 0x228];         // +0x228 the item's sound sub-object (party_window.cpp item_sound)

    tstring mesh_name_for(int bit);                         // 0x433bf0
    res_obj* mesh_for(unsigned __int64 cls, int bit);       // 0x433cb0
    bool is_ranged();                                       // 0x433d90
    bool is_melee();                                        // 0x433dc0
    int ammo_skill();                                       // 0x433df0
    void get_carry_pos(pos2* out, int* bone_flags, int* bone);   // 0x433e40
    tstring mesh_name(unsigned __int64 cls, int bit);       // 0x462820 (ui/item_ref.cpp)
    tstring* display_name();                                // 0x43a480 (the record's own name, or itself when it has none)
    tstring translated();                                   // 0x4c62b0 (rgl/mesh2/gui_edit.cpp)
    tstring translated_plural();                            // 0x4c6330
    const tstring* plural_name();                           // gui_edit.cpp: suffix_or(key, "_pl", plural)
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_kind_count;           // 0x5e29d4

int find_item(const tstring* name);     // 0x434ad0 (cdecl): index of the item named `name`, 0 when absent

#endif // MB_ITEM_KIND_H
