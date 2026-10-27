// item_rec.h: the two 0x60-byte "item" records constructed at 0x4ce870 / 0x4ce8d0 (src/game/triggers/item_rec.cpp). Both are
// head_rec objects (../../rgl/mesh2/head_rec.h: vtable 0x5947d8, status word 0x63 at +8, name tstring at +0x1c) whose own
// vtable (0x596124 / 0x596148) replaces the base one; they share one deleting destructor (0x4ce930, ICF). ALL NAMES PROVISIONAL.
#ifndef GAME_TRIGGERS_ITEM_REC_H
#define GAME_TRIGGERS_ITEM_REC_H
#include "../../rgl/mesh2/head_rec.h"

extern __int64 g_ticks[];                       // 0x5e2948 clock table, 1e-5 s ticks

// The owned block at +0x30 (three pointers, cleared by the constructors, released by the destructor).
struct owned_block {
    int f0;                                     // +0x30 (never initialised)
    void* begin;                                // +0x34 0x4ce870
    void* end;                                  // +0x38
    void* cap;                                  // +0x3c
    ~owned_block();                             // 0x4c8bf0 (src/rgl/mesh2/gui_edit.cpp conv_clear: releases the block)
};

// head_rec (../../rgl/mesh2/head_rec.h) as the item records see it: with the virtual destructor of vtable 0x5947d8 slot 0
// (0x4c4c90), whose inline body (name dtor, base vptr store) ends the derived deleting destructor 0x4ce930. mesh2's head_rec
// only models the vptr store (and its constructor stores +4 before the derived vptr, the item constructors after), so the
// exact bytes of these three functions need this variant. Same layout as head_rec.
struct item_head {
    virtual ~item_head() {}                     // 0x4c4c90 (symbols/slices/triggers.txt maps ??_7item_head to 0x5947d8)
    // 0x5947d8 has seven slots (entry 7 is the next table's first dword: declaring it made the engine call a CRT
    // compare). Same table as head_rec.
    virtual void s1() {}
    virtual void s2() {}
    virtual void s3() {}
    virtual void s4() {}
    virtual void s5() {}
    virtual void s6() {}
    virtual void s7() {}
    virtual void s8() {}
    int w04;                                    // +4
    int w08;                                    // +8 0x63
    char pad0c[0x10];
    tstring name;                               // +0x1c
    item_head() : name() { w08 = 0x63; }
};

struct ItemRec : item_head {                     // vtable 0x596124
    char pad2c[4];                              // +0x2c
    owned_block owned;                          // +0x30
    int f40;                                    // +0x40 -1
    int pad44;
    int clock;                                  // +0x48 clock index (0)
    int pad4c;
    __int64 stamp;                              // +0x50 g_ticks[0]
    float f58;                                  // +0x58 1.0f
    int f5c;                                    // +0x5c 0
    ItemRec();                                  // 0x4ce870
};
struct ItemRec2 : item_head {                    // vtable 0x596148
    char pad2c[4];
    owned_block owned;
    int f40;
    int pad44;
    int clock;
    int pad4c;
    __int64 stamp;
    float f58;
    int f5c;
    ItemRec2();                                 // 0x4ce8d0
};
#endif
