// port/tu/game/mission_ui2/slot_api.h: per-TU prelude for src/game/mission_ui2/slot_api.cpp (portable build only; see the
// port/tu note in port/CMakeLists.txt).
//
// slot_api.cpp defines the explicit specialization Pool<Event>::get (0x48ef20), but battle_rec.h, included first, already
// calls it from an inline member (BattleTable::get), which instantiates the member's declaration. cl accepts the later
// specialization; clang rejects it ("explicit specialization after instantiation", no flag relaxes it). Declaring the
// specialization before any use is what the language wants, and doing it here keeps src/ unchanged.
#pragma once
#include <pool.h>
struct Event;
template<> Event* Pool<Event>::get(unsigned index);
