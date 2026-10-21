// Shared declarations of slice s00434ad0 (provisional names; evidence in comments).
#ifndef S00434AD0_H
#define S00434AD0_H
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    bool equals(const tstring& o) const;        // 0x42bdd0
};

// item type ids (Module System header_items.py itp_type_*), low byte of ItemKind::type_flags
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};
// item modifiers (Module System header_item_modifiers.py)
enum {
    imod_plain = 0, imod_cracked, imod_rusty, imod_bent, imod_chipped, imod_battered, imod_poor, imod_crude, imod_old,
    imod_cheap, imod_fine, imod_well_made, imod_sharp, imod_balanced, imod_tempered, imod_deadly, imod_exquisite,
    imod_masterwork, imod_heavy, imod_strong, imod_powerful, imod_tattered, imod_ragged, imod_rough, imod_sturdy,
    imod_thick, imod_hardened, imod_reinforced, imod_superb, imod_lordly, imod_lame, imod_swaybacked, imod_stubborn,
    imod_timid, imod_meek, imod_spirited, imod_champion, imod_fresh, imod_day_old, imod_two_day_old, imod_smelling,
    imod_rotten, imod_large_bag
};

// item kind record (0x238 bytes, array pointer at 0x5e29d0, count at 0x5e29d4)
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {
    char pad0[0x20];
    tstring name;                       // +0x20
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44
    ItemMesh meshes[1];                 // +0x48
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x1e8 - 0x1cc];
    int f1e8, f1ec, f1f0, f1f4;         // +0x1e8 (stat bases adjusted by the item modifier)
    char pad1f8[0x200 - 0x1f8];
    int f200, f204;                     // +0x200
    char pad208[0x210 - 0x208];
    int f210;                           // +0x210
    unsigned f214, f218;                // +0x214 (low byte used)
    char pad21c[0x238 - 0x21c];
    tstring mesh_name_for(int bit);     // 0x433bf0 (s004333c0)
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_count;                // 0x5e29d4
struct Rec20 { tstring name; char pad[0x10]; };      // 0x20-byte named record (table at 0x5e28f8, count 0x5e29e8)
struct Rec78 { tstring name; char pad[0x68]; };      // 0x78-byte named record (table at 0x5e29dc, count 0x5e29e0)
extern Rec20* g_tbl20;                  // 0x5e28f8
extern int g_tbl20_count;               // 0x5e29e8
extern Rec78* g_tbl78;                  // 0x5e29dc
extern int g_tbl78_count;               // 0x5e29e0

struct pos : matrix {};
pos* identity_pos(pos* out);                        // 0x40cd00 (cdecl; returns out)

// reference to an item with modifier: item index, amount (low byte of m4) and modifier (byte 7)
struct ItemRef {
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    tstring mesh_name();                // 0x434b20
    int max_amount();                   // 0x434b70
    void add_amount(int n);             // 0x434be0
    int room_left();                    // 0x434c30
    int stat_1ec();                     // 0x434c50
    unsigned stat_21x(int which);       // 0x434cb0
    int stat_1f0a();                    // 0x434d70
    int stat_1e8();                     // 0x434df0
    int stat_200();                     // 0x434eb0
    int stat_1f4();                     // 0x434f60
    int stat_1f0b();                    // 0x434fd0
    int stat_214();                     // 0x435040
    int stat_204();                     // 0x435090
};
int round_float(float* f);                          // 0x432540 (cdecl): rounds a float to an int
int find_item(const tstring* name);                 // 0x434ad0 (cdecl)
int find_in_table20(const tstring* name);           // 0x435260
int find_in_table78(const tstring* name);           // 0x4352a0
int ftoi_helper(int x);                             // 0x435440
int slot_class(int item, int flag);                 // 0x4350c0 (register convention)


// per-unit record (names provisional; only the touched fields are laid out)
struct Agent {
    char pad0[0x10];
    int m10;                            // +0x10
    int m14;                            // +0x14
    char pad18[0x30 - 0x18];
    unsigned __int64 flags30;           // +0x30
    char pad38[0x68 - 0x38];
    unsigned nib[6];                    // +0x68 (packed 4-bit values, 8 per word)
    union {
        float stat[8];                  // +0x80
        struct { ItemRef a, b; } slots; // +0x80 (two item references)
    };
    char padA0[0xb0 - 0xa0];
    int b0;                             // +0xb0
    char padB4[0xd8 - 0xb4];
    pos md8;                            // +0xd8
    char pad118[0x158 - 0x118];
    vector4 v158;                       // +0x158
    int m168;                           // +0x168
    float f16c;                         // +0x16c
    char pad170[0x188 - 0x170];
    int m188, m18c;                     // +0x188
    char pad190[0x1a0 - 0x190];
    int m1a0, m1a4;                     // +0x1a0
    char pad1a8[0x1b0 - 0x1a8];
    int m1b0, m1b4, m1b8;               // +0x1b0
    bool has_flag10();                  // 0x435130
    int stat_int(int i);                // 0x435150
    int scaled_b0();                    // 0x435170
    int nibble(int i);                  // 0x4351a0
    ItemRef ranged_ref();               // 0x4351f0
    void reset();                       // 0x4352f0
};
extern char g_skill_tbl[];              // 0x8ee3100 records of 0x44 bytes, cap at +0x34

// three-row placeholder record initialised by 0x4353a0
struct Rows {
    int m0, m4;
    vector4 v8, v18, v28;
    int m38, m3c, m40, m44, m48, m4c, m50, m54;
    Rows* init();                       // 0x4353a0 (returns this)
};
#endif
