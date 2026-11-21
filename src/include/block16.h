// block16.h: the 0x10-byte record built by the out-of-line constructor 0x4638b0 (terrain_a/game_state.cpp): the default
// colour-ramp block (0, 0.5, 0, 0). GameData holds four of them (the wind anchors' values at +0x13b64).
#ifndef MB_BLOCK16_H
#define MB_BLOCK16_H
#include "./vec.h"

struct Block16 {
    Vec2 p, q;
    Block16();                                  // 0x4638b0
};

#endif // MB_BLOCK16_H
