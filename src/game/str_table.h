// str_table.h: the translated-string table (global object 0x5e2a18).
//   lookup 0x43a1e0 (thiscall, ret 4): never fails, a shared entry answers a miss
//   find   0x4c6010: null when the key is absent
// Users: src/game/agent3/agent_fx_sound.cpp, src/game/party/game.cpp + party_window.cpp (as g_strtab), the
// script interpreter and dialog_manager (as g_strtable).
#ifndef GAME_STR_TABLE_H
#define GAME_STR_TABLE_H
#include "./tstring.h"

struct StrTable {
    tstring* lookup(const tstring* key);                // 0x43a1e0 (key -> translated string; ret 4)
    tstring* find(const tstring* key);                  // 0x4c6010 (null when absent)
    // the reference spelling of lookup (a caller that has the key as an object): one thiscall, one pointer argument, so the call
    // sites compile identically; only the C++ mangled name differs (it never reaches the image). find has no such twin: a second
    // overload of it made the resolution audit bind options_panel's find calls to the wrong function.
    tstring* lookup(const tstring& key);                // 0x43a1e0
};

#endif // GAME_STR_TABLE_H
