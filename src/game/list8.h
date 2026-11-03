// list8.h: the VC7.1 std::vector layout of 8-byte elements (allocator word, then first / last / end). ONE definition for
// src/game/char_sheet. write8 0x4e94b0 reads the pointers at +4 / +8 (its callers pass record + 0x4f4 for the troop
// record, 0x4e9850), so the save_records view (first at +0) was missing the allocator word.
#ifndef MB_GAME_LIST8_H
#define MB_GAME_LIST8_H
#include "out_stream.h"
#include "../include/i64_vec.h"                      // I64Vec: the same vector (std::vector<__int64>); read 0x51e0b0, resize 0x493090

struct Elem8 { int a, b; };
struct List8 : I64Vec {                             // vector of 8-byte elements: the saved form of the records (write8)
    __declspec(noinline) void write8(OutStream* out);   // 0x4e94b0 (ret 4)
};
extern List8 g_list_a70480;                         // 0xa70480 (read by Game::load_state 0x51dbd0, written by GameData::save_state 0x4e8f10; defined in party/game.cpp)

#endif // MB_GAME_LIST8_H
