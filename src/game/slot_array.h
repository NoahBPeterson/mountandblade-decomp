// slot_array.h: the Module System slot array every scripted record carries (troop, party, faction, scene, party template,
// item, agent, quest) and the script variables: a std::vector<__int64> (VC7.1 layout: allocator word, first, last, end) with
// the game's members. ONE definition for every record header (troop.h, party.h, game_recs.h, item_kind.h, agent_obj.h,
// scene_file.h, script_state.h) and the modules that define the members.
#ifndef GAME_SLOT_ARRAY_H
#define GAME_SLOT_ARRAY_H
#include <vector>
struct stream_base;
struct stream_base;
struct SlotArray : std::vector<__int64> {
    __int64 get(const int& slot);                               // 0x48d650 (mission_ui2/mission_3.cpp; thiscall, ret 4: 0 past the end)
    void set(const int& slot, const __int64& value);            // 0x493d70 (world_data/world_data.cpp; thiscall, ret 8: grows to slot + 1)
    void grow(int* n);                                          // 0x493c40 (world_data/world_data.cpp; ret 4: at least *n slots, rounded up to a power of two >= 16, the new ones 0)
    void read(stream_base* s);                                  // 0x51e0b0 (party/game.cpp; the save game's count and slots)
    void write(stream_base* out);                                 // 0x4e94b0 (char_sheet/save_state.cpp; ret 4)
    void tidy() { _Tidy(); }                                    // frees the storage (Agent::reset 0x472f90 expands it)
};
#endif // GAME_SLOT_ARRAY_H
