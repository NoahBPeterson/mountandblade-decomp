// list8.h: the VC7.1 std::vector layout of 8-byte elements (allocator word, then first / last / end). ONE definition for
// src/game/char_sheet. write8 0x4e94b0 reads the pointers at +4 / +8 (its callers pass record + 0x4f4 for the troop
// record, 0x4e9850), so the save_records view (first at +0) was missing the allocator word.
#ifndef MB_GAME_LIST8_H
#define MB_GAME_LIST8_H
#include "out_stream.h"

struct Elem8 { int a, b; };
struct List8 {                                      // vector of 8-byte elements
    int   pad0;                                     // +0x0 (allocator)
    Elem8* begin;                                   // +0x4
    Elem8* end;                                     // +0x8
    Elem8* endcap;                                  // +0xc
    __declspec(noinline) void write8(OutStream* out);   // 0x4e94b0 (ret 4)
};

#endif // MB_GAME_LIST8_H
