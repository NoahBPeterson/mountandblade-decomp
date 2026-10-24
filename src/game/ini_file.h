// ini_file.h: the ini-file reader (0x4bd4d0..0x4bd740): a fixed table of 0x1000 key / value entries of 0x2c bytes each.
// One definition for config.h, the option panel, the character sheet, the inventory panel and the module-info /
// game-variable tables (globals 0xa18238 g_module_info and 0xa44240 g_game_vars / g_ini / g_config: the same
// 0x2c004-byte object; ctor 0x4c1be0 and dtor 0x4be920 are shared by every view, find 0x4bd5a0 and get_float 0x4bd5e0
// are called on it from char_sheet, inventory_panel and rgl/mesh2).
#ifndef GAME_INI_FILE_H
#define GAME_INI_FILE_H
#include <stdio.h>
#include "./tstring.h"

struct ini_entry {
    tstring key;                                    // +0
    char    pad10[0xc];
    tstring value;                                  // +0x1c
};
struct ini_file {
    ini_entry entries[0x1000];                      // +0
    int       count;                                // +0x2c000
    ini_file();                                     // 0x4c1be0 (src/game/triggers: constructs the 0x1000 key / value string pairs, clears count)
    ~ini_file();                                    // 0x4be920 (destroys them, last pair first)
    void load(FILE* f);                             // 0x4bd630
    int  find(const tstring& key);                  // 0x4bd5a0
    float get_float(const tstring& key);            // 0x4bd5e0
    int  get_int(const tstring& key);               // 0x5093d0 (find, then atoi of the value; src/game/inventory_panel/char_window.cpp)
    int  get_hex(int* idx);                         // 0x4e51e0 (sscanf "%x" of entry *idx; 0 when out of range; src/game/cls_map/progress_bar.cpp)
    float get_float(int* idx);                      // 0x4e5230 (atof of entry *idx; -1.0f when out of range)
    int  hex_field(const tstring& key);             // 0x4e5d30 (find, then "%x" of the entry)
    void parse_line(const char* line, tstring& key, tstring& value);   // 0x4bd4d0
    void write(FILE* f, const tstring& key, const tstring& value);     // 0x4bd740
};

extern ini_file g_game_vars; // 0xa44240
extern ini_file g_module_info; // 0xa18238
#endif // GAME_INI_FILE_H
