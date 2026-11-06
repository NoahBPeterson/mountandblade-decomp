// owned_block.h: the block ItemRec and the menu records hold at +0x30 (three pointers, cleared by the constructors, released
// by the destructor 0x4c8bf0, rgl/mesh2/gui_edit.cpp).
#ifndef INCLUDE_OWNED_BLOCK_H
#define INCLUDE_OWNED_BLOCK_H

struct owned_block {
    int f0;                                     // ItemRec+0x30 (never initialised)
    void* begin;                                // ItemRec+0x34 0x4ce870
    void* end;                                  // ItemRec+0x38
    void* cap;                                  // ItemRec+0x3c
    ~owned_block();                             // 0x4c8bf0
    void zero() { begin = 0; end = 0; cap = 0; }   // inline, as the constructors clear it (0x4ce870, 0x4ceb30): the stores stay after the vptr
};

#endif // INCLUDE_OWNED_BLOCK_H
