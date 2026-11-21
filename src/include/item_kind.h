// item_kind.h: the item definition record (ItemKind, 0x238 bytes, array g_items) with its mesh variants (ItemMesh) and the
// Module System item constants (itp_type_*, imod_*). ONE definition for src/game (agent/item_kind.cpp, agent.cpp, ui,
// party, terrain_a, script) and src/rgl/mesh2/gui_edit.cpp (the language export). The record is seen from three sides:
//   +0x00 / +0x10 / +0x20  the three names (gui_edit.cpp: save_language_files 0x4c8ca0, display_name 0x43a480; +0x20 is
//                          the key item_kind.cpp compares in find_item 0x434ad0 and falls back to in 0x433bf0);
//   +0x44.. +0x1c8..       the mesh table, type and capability flags and the modifier-adjusted stat bases (item_kind.cpp,
//                          agent.cpp);
//   +0x1d8 .. +0x228       the value, shop and sound words the ui modules read (item_ref.cpp, mission_ui.cpp).
// Every offset below is read by a function of one of those modules (cited); the rest is padding. The stat names are the
// module system's (header_items.py: weight, abundance, head_armor, ..., thrust_damage, swing_damage).
// Needs nothing from rgl: the carry frame is an rgl::matrix.
#ifndef MB_ITEM_KIND_H
#define MB_ITEM_KIND_H
#include <vector>
#include "../game/tstring.h"
#include "../game/slot_array.h"
#include "../game/trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list

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

namespace rgl { struct mesh; }
namespace rgl { struct matrix; }
struct stream_base;

// one mesh variant of an item: name + 64-bit flags (top nibble = class, low bits = modifier mask)
struct ItemMesh { tstring name; __int64 flags; };      // 0x18 (0x433bf0, 0x433cb0, 0x462820)

struct ItemKind {                       // 0x238 bytes
    tstring name;                       // +0x00 (gui_edit.cpp: the record's name in the language export, 0x4c62b0)
    tstring plural;                     // +0x10 (0x4c6330)
    tstring key;                        // +0x20 (0x433bf0 fallback name, 0x434ad0 find_item compares it; 0x43a480 display name)
    SlotArray slots;                    // +0x30 the item's slots (item_set_slot 0x4a0ada; char_sheet/save_state.cpp writes them)
    float f40;                          // +0x40 (1.0f: written by ItemKind() 0x4cdf20 and read 0x4c8110)
    int mesh_count;                     // +0x44 (0x433bf0, 0x433cb0, 0x462820)
    ItemMesh meshes[16];                // +0x48 entries of 0x18 (0x433bf0, 0x433cb0, 0x462820)
    unsigned type_flags;                // +0x1c8 low byte = itp_type_*, the rest itp_* flags (0x433d90, 0x433dc0, 0x433df0, 0x4350c0, 0x462c80, 0x462da0)
    unsigned char type() const { return (unsigned char)type_flags; }   // the low byte: itp_type_*
    unsigned __int64 flags;             // +0x1d0 itcf_* capability flags, 64 bit (0x433e40, 0x4362e0, 0x435520)
    int value;                          // +0x1d8 price / sort value (0x462e20, 0x488f80, 0x489d10)
    unsigned __int64 modifiers;         // +0x1e0 the imod_* bits the item may take (read 0x4c8110 reads them as one word; 0x48c5c0 tests them against g_bit[imod])
    // the stats, in the module system's names; read 0x4c8110 reads them in the order weight, abundance, head / body / leg
    // armour, difficulty, hit points, speed rating, missile speed, weapon length, max ammo, thrust / swing damage
    int hit_points;                     // +0x1e8 (ItemRef::hit_points 0x434df0: a horse's or shield's, 100 for a horse with none)
    int difficulty;                     // +0x1ec (ItemRef::difficulty 0x434c50: the skill the item requires)
    int spd_rtng;                       // +0x1f0 speed rating, a horse's maneuver (ItemRef::speed_rating 0x434d70, horse_maneuver 0x434fd0)
    int shoot_speed;                    // +0x1f4 missile speed, a horse's speed (ItemRef::horse_speed 0x434f60)
    int f1f8;                           // +0x1f8 (read and written by nothing we have)
    int head_armor;                     // +0x1fc (ItemRef::head_armor 0x462960)
    int body_armor;                     // +0x200 (ItemRef::body_armor 0x434eb0)
    int leg_armor;                      // +0x204 (ItemRef::leg_armor 0x462a10; ItemRef::accuracy 0x435090 reads it raw)
    float weight;                       // +0x208 (0x436050; inventory_panel.cpp item_weight, party_2.cpp sums it)
    int weapon_length;                  // +0x20c (0x433e40)
    int max_ammo;                       // +0x210 the stack size (ItemRef::max_amount 0x434b70)
    int thrust_damage, swing_damage;   // +0x214 / +0x218 low byte = damage, bits 8..9 = damage type (ItemRef::damage 0x434cb0;
                                        // thrust_damage is a horse's charge, ItemRef::horse_charge 0x435040)
    float price_rate;                   // +0x21c (1.0 for every kind by 0x48a210; set_price_rate_for_item 0x4a3e87)
    int abundance;                      // +0x220 (0x48cc40: loot weight, times merchandise_probability)
    float merchandise_probability;      // +0x224 (set for every kind by 0x48a250; set_item_probability_in_merchandise 0x4a56ef)
    SimpleTriggerVec sound;                    // +0x228 the item's sound set (party_window.cpp 0x502b80; update 0x48add0, ~SimpleTriggerVec 0x4317b0)
    int f234;                           // +0x234

    ItemKind();                                             // 0x4cdf20 (inventory_panel/module_files.cpp: g_items = new ItemKind[count])
    ~ItemKind();                                            // 0x4c8a40
    void read(stream_base* s, int version);                 // 0x4c8110 (rgl/mesh2/gui_edit.cpp; module_files.cpp: the item_kinds.txt loader; ret 8)
    tstring mesh_name_for(int bit);                         // 0x433bf0
    rgl::mesh* mesh_for(unsigned __int64 cls, int bit);       // 0x433cb0
    bool is_ranged();                                       // 0x433d90
    bool is_melee();                                        // 0x433dc0
    int ammo_skill();                                       // 0x433df0
    void get_carry_pos(rgl::matrix* out, int* bone_flags, int* bone);   // 0x433e40
    tstring mesh_name(unsigned __int64 cls, int bit);       // 0x462820 (ui/item_ref.cpp)
    tstring* display_name();                                // 0x43a480 (the record's own name, or itself when it has none)
    tstring translated();                                   // 0x4c62b0 (rgl/mesh2/gui_edit.cpp)
    tstring translated_plural();                            // 0x4c6330
    const tstring* plural_name();                           // gui_edit.cpp: suffix_or(key, "_pl", plural)
    // the same lookup out of line: the script's call (str_store_item_name_plural) goes to Troop::plural_name 0x48d6d0, which
    // the linker folded with this one; a symbol of its own keeps cl from merging the item and troop name tails past the call,
    // which the original does not (str_store_troop_name_plural jumps into str_store_troop_name_by_count)
    tstring* find_plural();                                 // 0x48d6d0 (folded with Troop::plural_name)
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_kind_count;           // 0x5e29d4

int find_item(const tstring* name);     // 0x434ad0 (cdecl): index of the item named `name`, 0 when absent

#endif // MB_ITEM_KIND_H
