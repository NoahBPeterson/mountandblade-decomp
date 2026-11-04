// slot_array.h: the Module System slot array every scripted record carries (troop, party, faction, scene, party template,
// agent, quest, item): an empty view whose `this` is the slot storage inside the record. ONE definition for
// src/game/script (the interpreter's slot operations) and the agent record (Agent::slot_vars, +0x1c).
#ifndef GAME_SLOT_ARRAY_H
#define GAME_SLOT_ARRAY_H
struct SlotArray {
    __int64 get(const int& slot);                               // 0x48d650 (thiscall, ret 4)
    void set(const int& slot, const __int64& value);            // 0x493d70 (thiscall, ret 8)
};
// a record's slot storage as a real std::vector<__int64> (the party record's, +0x59d8): SlotArray's get / set act on it
#include <vector>
struct SlotVec : std::vector<__int64> {
    __int64 get(const int& slot) { return ((SlotArray*)this)->get(slot); }                    // 0x48d650
    void set(const int& slot, const __int64& value) { ((SlotArray*)this)->set(slot, value); } // 0x493d70
};
#endif // GAME_SLOT_ARRAY_H
