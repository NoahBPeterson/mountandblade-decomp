// prop_vec.h: the vector of the mission's 0x1c8-byte scene prop instances (Mission +0x6688: allocator word, first +0x668c, last
// +0x6690, end). ONE definition for src/game/agent4 (Mission::add_placed 0x44f870 pushes through the out-of-line push_back
// 0x440b80), src/game/map_editor (erase 0x531470, called by MapMission::remove_prop 0x5313c0) and src/game/script (the prop
// operations: get 0x438c10, warn_scene_prop_instance 0x49d290 sizes it as (last - first) / 0x1c8). The element is Rec1c8
// (include/rec1c8.h; the script's PropInst view of the same record).
#ifndef GAME_PROP_VEC_H
#define GAME_PROP_VEC_H
#include <vector>
#include "../include/rec1c8.h"

struct PropInst;
struct PropVec : std::vector<Rec1c8> {
    void push_back(const Rec1c8& x);                            // 0x440b80
    iterator erase(iterator where);                             // 0x531470
    PropInst* at(int id);                                      // 0x438c10 (thiscall, ret 4)
    int count() const { return (int)(((char*)_Mylast - (char*)_Myfirst) / 0x1c8); }   // the script's inline size
};

#endif // GAME_PROP_VEC_H
