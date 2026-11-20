// event_pool.h: GameData::events (Pool<Event>, include/pool.h) with the map tick's cursor over it: next 0x517960 and
// first 0x5179b0 (inventory_panel/game_tick.cpp), thiscall members whose `this` is the pool. EventPool adds only these two
// members to Pool<Event> (no data), so the pool is reached as one with static_cast. They are not declared in Pool<T> itself:
// two more members there change cl 13.10's code for PartyScreen::setup_skills 0x5063e0 (party_screen.cpp).
#ifndef GAME_EVENT_POOL_H
#define GAME_EVENT_POOL_H
#include "../include/pool.h"
#include "./event.h"

struct EventPool : Pool<Event> {
    void next(int* index);                      // 0x517960 (ret 4): the next record in use after *index
    int first();                                // 0x5179b0: the first record in use (count when none)
};
inline EventPool& event_pool(Pool<Event>& p) { return static_cast<EventPool&>(p); }

#endif // GAME_EVENT_POOL_H
