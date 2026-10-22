// item_kind.h: the item definition record (ItemKind, 0x238 bytes, array g_items) and the item-type constants
// shared by item_kind.cpp (0x433bf0..0x434b20) and agent.cpp. One header for the class; every offset below is
// read by a function of this directory (cited); the rest is padding. Names provisional.
#ifndef AGENT_ITEM_KIND_H
#define AGENT_ITEM_KIND_H
#include "rgl.h"
#include "../tstring.h"

using rgl::vector4;

// item type ids (Module System header_items.py itp_type_*), stored in the low byte of ItemKind::type_flags
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};

// the resource manager's object lookup (0x419540, ret 4) as item_kind.cpp uses it; the class itself is migrated
// with the 0x419420 object (src/rgl/resource.h does not declare get_object yet, and defines g_resmgr with its own
// type, so that header cannot be included here).
struct res_obj;
struct resmgr { res_obj* get_object(const tstring* name); };   // 0x419540
extern resmgr g_resmgr;                                         // 0x5d6fa0

// local frame of a carried item: matrix plus the three rotation members of 0x40e570 / 0x40e670 / 0x40e770
// (rgl::matrix::rotate_about_s / _f / _u in src/rgl/rgl_math.h, which cannot be included together with rgl.h)
struct pos2 : rgl::matrix {
    void rotate_fu(const float* angle);     // 0x40e570 rotates rows f and u
    void rotate_su(const float* angle);     // 0x40e670 rotates rows s and u
    void rotate_sf(const float* angle);     // 0x40e770 rotates rows s and f
};
pos2 identity_pos();                        // 0x40cd00 (cdecl, hidden return pointer: identity rotation, zero origin)

// one mesh variant of an item: name + 64-bit flags (top nibble = class, low bits = modifier mask)
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18 (0x433bf0, 0x433cb0)

struct ItemKind {
    char pad0[0x20];
    tstring name;                       // +0x20 (0x433bf0 fallback name, 0x434ad0 find_item compares it)
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44 (0x433bf0, 0x433cb0)
    ItemMesh meshes[1];                 // +0x48 entries of 0x18 (0x433bf0, 0x433cb0)
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 low byte = itp_type_* (0x433d90, 0x433dc0, 0x433df0, 0x4350c0)
    char pad1cc[4];
    unsigned __int64 flags;             // +0x1d0 itcf_* capability flags, 64 bit (0x433e40, 0x4362e0, 0x435520)
    char pad1d8[0x1e8 - 0x1d8];
    int f1e8, f1ec, f1f0, f1f4;         // +0x1e8 stat bases adjusted by the modifier (0x434c50, 0x434d70, 0x434df0, 0x434f60)
    char pad1f8[0x200 - 0x1f8];
    int f200, f204;                     // +0x200 (0x434eb0, 0x435090)
    float f208;                         // +0x208 value (0x436050)
    int weight;                         // +0x20c (0x433e40)
    int f210;                           // +0x210 stack size (0x434b70)
    unsigned f214, f218;                // +0x214 low byte used (0x435040, 0x434cb0)
    char pad21c[0x238 - 0x21c];

    tstring mesh_name_for(int bit);                         // 0x433bf0
    res_obj* mesh_for(unsigned __int64 cls, int bit);       // 0x433cb0
    bool is_ranged();                                       // 0x433d90
    bool is_melee();                                        // 0x433dc0
    int ammo_skill();                                       // 0x433df0
    void get_carry_pos(pos2* out, int* bone_flags, int* bone);   // 0x433e40
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_kind_count;                // 0x5e29d4

int find_item(const tstring* name);     // 0x434ad0 (cdecl): index of the item named `name`, 0 when absent

#endif // AGENT_ITEM_KIND_H
