// Shared declarations of slice s00433cb0 (provisional names; evidence in comments).
#ifndef S00433CB0_H
#define S00433CB0_H

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
};
struct res_obj;
struct resmgr { res_obj* get_object(const tstring* name); };   // 0x419540 (global object at 0x5d6fa0)
extern resmgr g_resmgr;                                        // 0x5d6fa0

// item type ids (Module System header_items.py itp_type_*), stored in the low byte of ItemKind::type_flags
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};

// item kind record (0x238 bytes, array at 0x5e29d0): name +0x20, mesh list (count +0x44, entries of 0x18 from +0x48)
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {
    char pad0[0x44];
    int mesh_count;                     // +0x44
    ItemMesh meshes[1];                 // +0x48
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 (low byte = itp_type_*)
    res_obj* mesh_for(unsigned __int64 cls, int bit);    // 0x433cb0
    bool is_ranged();                                    // 0x433d90
    bool is_melee();                                     // 0x433dc0
    int ammo_skill();                                    // 0x433df0
};
#endif
