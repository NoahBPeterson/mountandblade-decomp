// loading_panel.h: the "creating game" screen (0x51c5a0, 0x51c720): a panel that advances a seven-step sequence one
// step per call and shows the step's text. Names provisional (the slices called the class Panel).
#ifndef GAME_PARTY_LOADING_PANEL_H
#define GAME_PARTY_LOADING_PANEL_H
#include "../tstring.h"
#include "../../rgl/stream.h"
#include "./widget_ops.h"

// the save-game reader is the bin_reader of src/rgl/stream.h (0x38 bytes, constructor 0x417c50); the panel calls two functions that the
// folded stream classes share: text_stream::open_read (0x415290) and mem_stream::~mem_stream (0x417c90), both through a cast (see game.cpp).
struct SlotReader;
struct SlotHeader {
    bool read_slot(SlotReader* r);   // 0x4e54e0 (cls_map/progress_bar.cpp: the slot's own header)
};

// header of a save game, read by 0x4e54e0 into the panel at +0x58
struct SaveHeader {
    tstring title;                          // +0x00  (panel +0x58) copied to Game::title    0x51c720 step 2
    tstring hero;                           // +0x10  (panel +0x68) copied to the hero troop's name
    char pad20[0x28 - 0x20];
    int f28;                                // +0x28  (panel +0x80) stored to 0x5e29bc        0x51c720 step 0
    int f2c;                                // +0x2c
    int f30;                                // +0x30
    int f34;                                // +0x34  (panel +0x8c) the value handed to Game::init   0x51c720 step 1
    void read_slot(bin_reader* r) { ((SlotHeader*)this)->read_slot((SlotReader*)r); }     // 0x4e54e0
};

struct LoadingPanel {
    char pad0[4];
    int mode;                               // +0x04  0 = new game, 1 = load a save, 2/3 = other starts   0x51c720
    char pad8[0x30 - 8];
    void* text;                             // +0x30  the status text widget (set_text through WidgetOps +0xa4)   0x51c5a0
    char pad34[0x3c - 0x34];
    int counter;                            // +0x3c  current step    0x51c5a0, 0x51c720
    int f40;                                // +0x40  (unused argument of sub)   0x51c5a0
    bin_reader* save;                       // +0x44  0x51c720
    tstring path;                           // +0x48  save file name (its buffer pointer is read at +0x4c)   0x51c720
    SaveHeader header;                      // +0x58

    void update(int arg);                   // 0x51c5a0 (ret 4; the argument is unused)
    bool sub(int* counter, int* unused, tstring* out);          // 0x51c720 (ret 0xc)
};

#endif // GAME_PARTY_LOADING_PANEL_H
