// config_table.h: the settings table (the ini file object, global 0xa44240, 0x1000 entries of 0x2c bytes: key tstring at +0,
// value tstring at +0x1c; src/game/config/config.h calls it ini_file, rec_tables.h ini_file_x). ONE definition of the callers'
// view for src/game/party (party_window.h), src/game/char_sheet (cs_panels.h, conv_open.h) and src/game/inventory_panel
// (char_sheet.cpp): the members the callers reach (find 0x4bd5a0, get_float 0x4bd5e0, get_int 0x5093d0) and the entries
// read from 0xa4425c (the value tstring of entry 0: +4 holds the text pointer, null when the field itself holds the text).
#ifndef GAME_CONFIG_TABLE_H
#define GAME_CONFIG_TABLE_H
#include "./tstring.h"

struct ConfigEntry {                        // 0x2c bytes, table at 0xa4425c
    int f0;
    char* value;                            // +4 (null: the field itself holds the text)  0x501a20
    char pad8[0x2c - 8];
};
struct ConfigTable {
    int find(const tstring& key);           // 0x4bd5a0 (ret 4): the entry index, or -1
    float get_float(const tstring& key);    // 0x4bd5e0 (ret 4)
    int get_int(const tstring& key);        // 0x5093d0 (ret 4)
};
extern ConfigTable g_config;                // 0xa44240  // old name of g_game_vars, removed after the names merge
#define CONFIG_ENTRIES ((ConfigEntry*)0xa4425c)

#endif // GAME_CONFIG_TABLE_H
