// party.h: the party record (0x5ac8 bytes): a list of up to 64 troop stacks plus a 64-bit flag word. Members live
// in two modules: troop_inv.cpp (first_flagged 0x463190) and mission_ui.cpp (0x489420..0x489620). Names provisional.
// Layout from 0x489420 (flags at +0xa0), 0x489490..0x489540 (stacks at +0xa8, stride 0x20, count at +0x8a8),
// 0x489540 (dirty byte at +0x8b4, trailer fields at stack+0x88/+0x8c), 0x463190 (stack flags at +0xb0 = stack 0 + 8).
#ifndef GAME_UI_PARTY_H
#define GAME_UI_PARTY_H

struct Stack20 {                            // 0x20 bytes
    int count;                              // +0x00 number of troops in the stack   0x489490
    int troop;                              // +0x04 troop index                     0x489620
    int flags;                              // +0x08 bit 0: prisoner stack           0x4894b0, 0x4894e0, 0x489510, 0x463190
    int f0c, f10, f14, f18, f1c;
};

struct Party {
    char pad0[0xa0];
    int flags_lo;                           // +0xa0 (64-bit flag word; low byte = id)  0x489420, 0x489460 (0x3000 = size class)
    int flags_hi;                           // +0xa4
    Stack20 stacks[64];                     // +0xa8
    int nstacks;                            // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                             // +0x8b4
    char pad8b5[0x5ac8 - 0x8b5];

    int first_flagged();                                     // 0x463190 (index of the first prisoner stack, else the count)
    void set_id(int v);                                      // 0x489420
    float scale();                                           // 0x489460
    int sum_all();                                           // 0x489490
    int sum_head();                                          // 0x4894b0
    int sum_flagged();                                       // 0x4894e0
    int count_tail();                                        // 0x489510
    int* remove_row(int* out, int index, int* arr);          // 0x489540
    int find_or_add(unsigned troop, unsigned flag, char force);   // 0x489620

    int max_members();                      // 0x4b22d0 (not in this module)
    int max_prisoners();                    // 0x4b2330 (not in this module)
    int max_4b1700();                       // 0x4b1700 (not in this module)
};

#endif // GAME_UI_PARTY_H
