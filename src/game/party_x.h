// party_x.h: the party record (Party, include/party.h) with the members the map editor's exchange screens and the script
// interpreter call under their own names. ONE definition of PartyX for src/game/map_editor (exchange_screens.h: the economy members
// of 0x4b1700 .. 0x532db0) and src/game/script (script_interpreter.h: the Module System operations). No fields: every field the
// script names (faction, morale, battle, template_id, ai_behavior, flags, carry_gold, notes, slots, ...) is an alias in an anonymous
// union of Party at the offset the interpreter reads it (party.h cites the script operations). The member functions below are the
// script's names for functions Party declares under other names (same address), plus the ones Party does not declare.
#ifndef GAME_PARTY_X_H
#define GAME_PARTY_X_H
#include "../include/party.h"

struct PartyX : Party {
    // ---- map_editor/exchange_screens.cpp ----
    void fn_4b1d80();                       // 0x4b1d80 (Party::add_xp_to_stack in src/game/party2: here its caller passes no argument)  // old name of Party::add_xp_to_stack, removed after the names merge
    int ext_4b1700();                       // 0x4b1700  // old name of Party::max_stacks, removed after the names merge
    int total_value(unsigned flags);        // 0x4b1cf0 (ret 4)  // old name of Party::sum_stat, removed after the names merge
    int find_row(int troop, int type, int flag);        // 0x489620 (ret 0xc)  // old name of Party::find_or_add, removed after the names merge
    int add_row(int troop, int a, int b, int c, int d); // 0x48c840 (ret 0x14)  // old name of Party::add_members, removed after the names merge
    __declspec(noinline) void compact();    // 0x532db0
    // ---- script/script_interpreter.cpp (Party's names in the comments) ----
    int max_stacks();                       // 0x4b1700 (always 32; Party::max_4b1700)
    int num_members();                      // 0x489490 (Party::sum_all)  // old name of Party::sum_all, removed after the names merge
    int num_regular_stacks();               // 0x463190 (Party::first_flagged)  // old name of Party::first_flagged, removed after the names merge
    int num_prisoner_stacks();              // 0x489510 (Party::count_tail)  // old name of Party::count_tail, removed after the names merge
    int num_regular_members();              // 0x4894b0 (Party::sum_head)  // old name of Party::sum_head, removed after the names merge
    int num_prisoner_members();             // 0x4894e0 (Party::sum_flagged)  // old name of Party::sum_flagged, removed after the names merge
    int party_size_limit();                 // 0x4b22d0 (game script 0x11 may override it; Party::max_members)  // old name of Party::max_members, removed after the names merge
    int prisoner_limit();                   // 0x4b2330 (game script 0x12 may override it; Party::max_prisoners)  // old name of Party::max_prisoners, removed after the names merge
    int add_members(int troop, int count, char prisoners, int a4, int a5);  // 0x48c840 (ret 0x14)
    Stack remove_stack(int index, void* rows);                            // 0x489540 (ret 0xc: hidden return; Party::remove_row spells the out pointer)  // old name of Party::remove_row, removed after the names merge
    void force_add_members(int troop, int count, char prisoners, int a4);   // 0x4b4470 (ret 0x10)
    void position_changed();                                                // 0x4b2880 (Party::place_on_map)  // old name of Party::place_on_map, removed after the names merge
    void set_name(const tstring& name);                                     // 0x489400 (ret 4)
    void set_icon(int icon);                                                // 0x489420 (ret 4; Party::set_id)  // old name of Party::set_id, removed after the names merge
    void add_particle_system(int psys);                                     // 0x4955e0 (ret 4)  // old name of EmitterCtl::add, removed after the names merge
};

#endif // GAME_PARTY_X_H
