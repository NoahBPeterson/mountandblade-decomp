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
    int sum_stat(unsigned flags);        // 0x4b1cf0 (ret 4)
    int find_or_add(int troop, int type, int flag);        // 0x489620 (ret 0xc)
    __declspec(noinline) void compact();    // 0x532db0
    // ---- script/script_interpreter.cpp (Party's names in the comments) ----
    int max_stacks();                       // 0x4b1700 (always 32; Party::max_4b1700)
    int sum_all();                      // 0x489490 (Party::sum_all)
    int first_flagged();               // 0x463190 (Party::first_flagged)
    int count_tail();              // 0x489510 (Party::count_tail)
    int sum_head();              // 0x4894b0 (Party::sum_head)
    int sum_flagged();             // 0x4894e0 (Party::sum_flagged)
    int max_members();                 // 0x4b22d0 (game script 0x11 may override it; Party::max_members)
    int max_prisoners();                   // 0x4b2330 (game script 0x12 may override it; Party::max_prisoners)
    int add_members(int troop, int count, char prisoners, int a4, int a5);  // 0x48c840 (ret 0x14)
    Stack remove_stack(int index, void* rows);                            // 0x489540 (ret 0xc: hidden return; Party::remove_row spells the out pointer)  // old name of Party::remove_row, removed after the names merge
    void force_add_members(int troop, int count, char prisoners, int a4);   // 0x4b4470 (ret 0x10)
    void place_on_map();                                                // 0x4b2880 (Party::place_on_map)
    void set_name(const tstring& name);                                     // 0x489400 (ret 4)
    void set_id(int icon);                                                // 0x489420 (ret 4; Party::set_id)
    void add(int psys);                                     // 0x4955e0 (ret 4)
};

#endif // GAME_PARTY_X_H
