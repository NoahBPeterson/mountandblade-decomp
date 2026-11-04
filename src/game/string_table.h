// string_table.h: the module's script string tables (global 0xa75ea0): the "s_*" strings (+0, count +8) and the "quick strings"
// (+4, count +0xc) of the module files. ONE definition for src/game/script (the interpreter reads the counts and fetches a string
// through get 0x48e8f0) and src/game/inventory_panel (module_files.cpp: load 0x4c46d0 / load_quick 0x4c47d0 fill it). Not the
// translated-string table 0x5e2a18 (str_table.h StrTable).
#ifndef GAME_STRING_TABLE_H
#define GAME_STRING_TABLE_H
#include "./tstring.h"

struct stream_base;

struct StringTable {
    struct Str { tstring name; tstring key; };                  // 0x20 bytes (name +0, key +0x10; load1 reads the key first)
    Str* strings;                       // +0  s_* strings
    Str* quick;                         // +4  quick strings
    int count;                          // +8  (check_string_arg 0x49cda0)
    int quick_count;                    // +0xc
    tstring get_string(int id, int type);      // 0x48e8f0 (thiscall, ret 0xc: hidden return; type 0 = register, 0x16 = quick)
    ~StringTable();                     // 0x4c4640 (the atexit destructor: deletes both arrays; rgl/mesh2/rec_tables.cpp)
    void load1(stream_base* s);         // 0x4c46d0 (rgl/mesh2/rec_tables.cpp)
    void load2(stream_base* s);         // 0x4c47d0
};
extern StringTable g_strings;           // 0xa75ea0

#endif // GAME_STRING_TABLE_H
