// str_table.h: the translated-string table (global object g_strtable at 0x5e2a18), the one definition of its class.
//   lookup 0x43a1e0 (thiscall, ret 4): never fails, a shared entry answers a miss
//   find   0x4c6010: null when the key is absent
//   ctor 0x4cda20, load 0x4cda90 (reads language.txt and languages/<lang>/*), ~StrTable 0x4ca720 (the atexit thunk 0x548470 tail-calls it)
// Merged views: the former rgl/mesh2 LangTables (this class: ctor, loader, find, write), agent2 StrTab (the layout, lookup),
// char_sheet/screen_init.h StrTab, options_panel/world_app.cpp StrTable, mission5's StrTable_s* and the partial copies of camera.cpp,
// note_window.cpp, mission_items.cpp, mesh_game.cpp, cls_map.h, gui_edit.cpp.
// Users: src/game/agent3/agent_fx_sound.cpp, src/game/party/game.cpp + party_window.cpp (as g_strtab), the
// script interpreter and dialog_manager (as g_strtable).
#ifndef GAME_STR_TABLE_H
#define GAME_STR_TABLE_H
#include <stdio.h>
#include <vector>
#include "./tstring.h"

// The two hash tables (0xdf3 buckets each: the keys, then the values). 0x4cda20 emits the 0x42bbd0 calls for the two trailing
// strings *after* the bucket constructors, so the tables live in a base and the strings stay members of the derived class.
struct StrTableBase {
    std::vector<tstring> keys[0xdf3];                           // +0x0000 (hash_string 0x433bb0 picks the bucket)
    std::vector<tstring> vals[0xdf3];                           // +0xdf30 (the translation of keys[h][i] is vals[h][i])
};

struct StrTable : StrTableBase {
    tstring notfound;                                           // +0x1be60 the shared entry answering a miss ("STR NOT FOUND")
    tstring language;                                           // +0x1be70 (0x5fe888) the language of the skin file name 
    StrTable();                                                 // 0x4cda20
    ~StrTable();                                                // 0x4ca720: language, notfound, then the base's tables
    tstring* find(const tstring* key);                          // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key);                        // 0x43a1e0 (key -> translated string; ret 4)
    void load(const tstring* root);                             // 0x4cda90 (the path prefix is a tstring: buf, else its inline bytes at +4)
};

extern StrTable g_strtable; // 0x5e2a18
#endif // GAME_STR_TABLE_H
