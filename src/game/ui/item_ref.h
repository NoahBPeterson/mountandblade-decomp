// item_ref.h: classes of item_ref.cpp (0x4625d0..0x462ac0): the sound-slot scheduler, the item kind record and
// the (item, modifier) pair that refers to one. Other modules of src/game/ui include this header for ItemKind
// and ItemRef. All names provisional; every offset is read by a function of the modules (cited by address).
#ifndef GAME_UI_ITEM_REF_H
#define GAME_UI_ITEM_REF_H
#include "../tstring.h"
#include "./ui_common.h"

// item type / modifier constants (itp_*, imod_*), ItemMesh, ItemKind and ItemRef are the shared headers' (g_items 0x5e29d0)
#include "../../include/item_kind.h"
#include "../item_ref.h"
extern int g_item_count;                    // 0x5e29d4 (g_item_kind_count of src/include/item_kind.h)

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

// weights indexed by a pair's modifier byte (5 entries per modifier, 8 bytes each); sort tie-break
struct SortVal { float v; float pad; };
extern SortVal g_sort_val[];                    // 0xa75f90

#endif // GAME_UI_ITEM_REF_H
