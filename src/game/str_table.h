// str_table.h: the translated-string table (global object 0x5e2a18).
//   lookup 0x43a1e0 (thiscall, ret 4): never fails, a shared entry answers a miss
//   find   0x4c6010: null when the key is absent
// Merged views: agent2 StrTab (the layout, lookup), char_sheet/screen_init.h StrTab (lookup spelled find), options_panel/world_app.cpp StrTab.
// Users: src/game/agent3/agent_fx_sound.cpp, src/game/party/game.cpp + party_window.cpp (as g_strtab), the
// script interpreter and dialog_manager (as g_strtable).
#ifndef GAME_STR_TABLE_H
#define GAME_STR_TABLE_H
#include <vector>
#include "./tstring.h"

// 16-byte hash bucket: the keys / values of one hash slot (std::vector<tstring>)
struct StrBucket { std::vector<tstring> v; };

struct StrTable {
    // layout (agent2/agent_stats.cpp defines lookup 0x43a1e0 on it: hash_string 0x433bb0 picks the bucket)
    StrBucket keys[0xdf3];                              // +0x0000 (0x43a1e0)
    StrBucket vals[0xdf3];                              // +0xdf30
    tstring notfound;                                   // +0x1be60 the shared entry answering a miss
    tstring* lookup(const tstring* key);                // 0x43a1e0 (key -> translated string; ret 4)
    tstring* find(const tstring* key);                  // 0x4c6010 (null when absent)
    // the reference spelling of lookup (a caller that has the key as an object): one thiscall, one pointer argument, so the call
    // sites compile identically; only the C++ mangled name differs (it never reaches the image). find has no such twin: a second
    // overload of it made the resolution audit bind options_panel's find calls to the wrong function.
    tstring* lookup(const tstring& key);                // 0x43a1e0
};

#endif // GAME_STR_TABLE_H
