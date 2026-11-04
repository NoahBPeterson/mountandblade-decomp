// str_table.h: the translated-string table (global object g_strtable at 0x5e2a18), the one definition of its class.
//   lookup 0x43a1e0 (thiscall, ret 4): never fails, a shared entry answers a miss
//   find   0x4c6010: null when the key is absent
//   ctor 0x4cda20, load 0x4cda90 (reads language.txt and languages/<lang>/*), release_strings 0x4ca720 (the atexit destructor)
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

// 16-byte hash bucket: the keys / values of one hash slot. The original's std::vector<tstring>: the leading dword is the
// allocator (never touched), first / last / end follow. The ctor, loader and finder address first / last / end directly
// (byte-exact in rgl/mesh2), the lookup uses the vector view v().
struct StrBucket {
    int pad;                                                    // +0x00 (the vector's allocator)
    tstring* myfirst;                                           // +0x04
    tstring* mylast;                                            // +0x08
    tstring* myend;                                             // +0x0c
    unsigned size() const { return myfirst ? (unsigned)(mylast - myfirst) : 0u; }
    std::vector<tstring>& v() { return *(std::vector<tstring>*)this; }
};

// The two hash tables (0xdf3 buckets each: the keys, then the values). 0x4cda20 emits the 0x42bbd0 calls for the two trailing
// strings *after* the bucket zero loops, but a ctor's member construction always precedes its body -- so the tables live in a
// base whose inlined ctor clears them first, and the strings stay members of the derived class.
struct StrTableBase {
    StrBucket keys[0xdf3];                                      // +0x0000 (hash_string 0x433bb0 picks the bucket)
    StrBucket vals[0xdf3];                                      // +0xdf30
    __forceinline StrTableBase()
    {
        int* k = (int*)keys + 2;
        int n = 0xdf3;
        do {
            k[-1] = 0; k[0] = 0; k[1] = 0;
            k += 4;
            n--;
        } while (n != 0);
        int* v = (int*)vals + 2;
        n = 0xdf3;
        do {
            v[-1] = 0; v[0] = 0; v[1] = 0;
            v += 4;
            n--;
        } while (n != 0);
    }
};

struct StrTable : StrTableBase {
    tstring notfound;                                           // +0x1be60 the shared entry answering a miss ("STR NOT FOUND")
    tstring language;                                           // +0x1be70 (0x5fe888) the language of the skin file name 
    StrTable();                                                 // 0x4cda20
    ~StrTable() { release_strings(); }                          // the atexit destructor 0x548470
    tstring* find(const tstring* key);                          // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key);                        // 0x43a1e0 (key -> translated string; ret 4)
    void load(const tstring* root);                             // 0x4cda90 (the path prefix is a tstring: buf, else its inline bytes at +4)
    void release_strings();                                     // 0x4ca720
};

extern StrTable g_strtable; // 0x5e2a18
#endif // GAME_STR_TABLE_H
