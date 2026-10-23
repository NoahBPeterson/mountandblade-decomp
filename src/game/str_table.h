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
};

#endif // GAME_STR_TABLE_H
