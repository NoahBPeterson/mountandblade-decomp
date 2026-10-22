// item_ref.h: classes of item_ref.cpp (0x4625d0..0x462ac0): the sound-slot scheduler, the item kind record and
// the (item, modifier) pair that refers to one. Other modules of src/game/ui include this header for ItemKind
// and ItemRef. All names provisional; every offset is read by a function of the modules (cited by address).
#ifndef GAME_UI_ITEM_REF_H
#define GAME_UI_ITEM_REF_H
#include "../tstring.h"
#include "./ui_common.h"

// ---- item type / modifier constants (Module System header_items.py: itp_type_*, imod_*) ----------------------
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};
enum {
    imod_plain = 0, imod_cracked, imod_rusty, imod_bent, imod_chipped, imod_battered, imod_poor, imod_crude, imod_old,
    imod_cheap, imod_fine, imod_well_made, imod_sharp, imod_balanced, imod_tempered, imod_deadly, imod_exquisite,
    imod_masterwork, imod_heavy, imod_strong, imod_powerful, imod_tattered, imod_ragged, imod_rough, imod_sturdy,
    imod_thick, imod_hardened, imod_reinforced, imod_superb, imod_lordly
};
#define itp_food 0x00080000

int __cdecl round_float(float* f);          // 0x432540 (round to nearest: floor(x) + (x - floor(x) > 0.5))

// ---- sound slots (0x4625d0..0x4627f0) --------------------------------------------------------------------
struct snd_channel {                // 0x1c bytes
    char pad[0x1c];
    bool play(int arg);             // 0x4875e0 (thiscall, ret 4)
    char emit(int idx, int zero, int* count, void* vals, int tag);   // 0x4b09e0 (thiscall, ret 0x14; mission_ui.cpp 0x488e90)
};
// One scheduler slot (0x78 bytes): state machine driven by three clock indices into g_ticks. The state and the
// clock indices behave as volatile (the original re-reads them and never CSEs g_ticks[clk]).
struct snd_slot {
    float min_gap;                  // 0x00  seconds between plays (<= -0.1: follow the caller's request)   0x4625d0, 0x462790
    float play_time;                // 0x04  seconds the first channel runs before the second starts        0x4625d0
    float hold_time;                // 0x08  seconds the second channel runs                                0x4625d0
    volatile int state;             // 0x0c  0 idle, 1 first channel playing, 2 second channel playing      0x4625d0
    snd_channel first;              // 0x10                                                                 0x4625d0
    snd_channel second;             // 0x2c                                                                 0x4625d0
    volatile int clk_gap;           // 0x48                                                                 0x4625d0, 0x462790
    char pad4c[4];
    __int64 t_gap;                  // 0x50                                                                 0x4625d0, 0x462790
    volatile int clk_play;          // 0x58                                                                 0x4625d0
    char pad5c[4];
    __int64 t_play;                 // 0x60                                                                 0x4625d0
    volatile int clk_hold;          // 0x68                                                                 0x4625d0
    char pad6c[4];
    __int64 t_hold;                 // 0x70                                                                 0x4625d0
    void update(int arg);           // 0x4625d0
};
struct snd_bank {
    int count;                      // 0     0x462760, 0x462790
    snd_slot* slots;                // 4     0x462760, 0x462790
    void update_all(int arg);       // 0x462760
    void randomize();               // 0x462790
};

// ---- items -------------------------------------------------------------------------------------------------
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18 bytes     0x462820
struct ItemKind {                           // 0x238 bytes (array g_items, 0x5e29d0)
    char pad0[0x44];
    int mesh_count;                         // +0x44   0x462820
    ItemMesh meshes[1];                     // +0x48   0x462820
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                    // +0x1c8 (low byte = itp_type_*)   0x462c80, 0x462da0, 0x462e20
    char pad1cc[0x1d8 - 0x1cc];
    int value;                              // +0x1d8 (price / sort value)      0x462e20, 0x488f80, 0x489d10
    char pad1dc[0x1fc - 0x1dc];
    int f1fc;                               // +0x1fc   0x462960
    char pad200[4];
    int f204;                               // +0x204   0x462a10
    char pad208[0x21c - 0x208];
    float f21c;                             // +0x21c (set to 1.0 for every kind by 0x48a210)
    char pad220[4];
    float f224;                             // +0x224 (set for every kind by 0x48a250)
    char pad228[0x238 - 0x228];
    tstring mesh_name(unsigned __int64 cls, int bit);   // 0x462820
    bool is_ranged();                       // 0x433d90
    bool is_melee();                        // 0x433dc0
};
extern ItemKind* g_items;                   // 0x5e29d0
extern int g_item_count;                    // 0x5e29d4

// An (item, modifier) pair: the unit of every inventory, equipment and loot table (8 bytes).
struct ItemRef {
    int item;                               // +0 index into g_items, -1 = empty        0x462900, 0x463030
    union {
        unsigned m4;                        // +4 as a word (copied whole by 0x463070, 0x463100)
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;   // mod = byte +7 (0x462960, 0x462900)
    };
    tstring mesh_name(unsigned __int64 cls);    // 0x462900
    int stat_1fc();                             // 0x462960
    int stat_204();                             // 0x462a10
    int stat_1ec();                             // 0x434c50 (not in this module)
    void check_slots(int n);                    // 0x489070 (sort helper: scans for the first empty pair; see mission_ui.cpp)
    void sort_sel(int n);                       // 0x489090 descending selection sort by (item, g_sort_val[mod * 5])
};

// weights indexed by a pair's modifier byte (5 entries per modifier, 8 bytes each); sort tie-break
struct SortVal { float v; float pad; };
extern SortVal g_sort_val[];                    // 0xa75f90

#endif // GAME_UI_ITEM_REF_H
