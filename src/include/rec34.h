// rec34.h: rec34, the 0x34-byte record of the module-file table loaded through rec34::read 0x4be470 (name at +4, a float at +0x14,
// then the script block DialogBlock at +0x18). ONE definition for src/game/config (config.cpp: read 0x4be470, the table loader)
// and src/rgl/mesh2 (gui_edit.cpp 0x4c48d0 builds the array: tstring ctor at +4, DialogBlock ctor at +0x18 inlined, calls 0x4be470
// on each element, vector deleting destructor 0x4318c0).
#ifndef MB_REC34_H
#define MB_REC34_H
#include "./dialog_rec.h"       // DialogBlock, Elem, tstring

struct rec34 {                                      // 0x34 bytes
    int     f00;                                    // +0x00
    tstring name;                                   // +0x04 (ctor at 0x4c4970)
    float   f14;                                    // +0x14
    DialogBlock body;                               // +0x18 (items +0x18, count +0x1c, done +0x20, text +0x24: 0x4c4980..0x4c498d)
    int read(stream_base* s);                       // 0x4be470
};

struct rec_table {                                  // {records, count} pair of the module-file tables (globals 0xa75eb0, 0xa75f2c, 0x8ed4f58)
    void* records;                                  // +0
    int   count;                                    // +4
    rec_table();                                    // 0x4be520 (game/config/config.cpp; the original is bracketed by WinLicense marker records)
};
struct rec34_table : rec_table {                    // 0x7c bytes: g_scripts 0xa75eb0 (scripts.txt), records are rec34
    int    success;                                 // +0x08 how many records loaded (the extra-triggers argument of Game::load_game_data)
    int    used[28];                                // +0x0c the record index of each of the engine's 28 named scripts (g_script_names, -1 = missing)
    ~rec34_table();                                 // the atexit destructor (rgl/mesh2/gui_edit.cpp): deletes the record array
    rec34& operator[](int i) { return ((rec34*)records)[i]; }
    void load_used();                               // 0x4be560 (game/config/config.cpp)
    void load(stream_base& s);                      // 0x4c48d0 (rgl/mesh2/gui_edit.cpp: reads the count and every record, then load_used)
};
extern rec34_table g_scripts;                       // 0xa75eb0 (defined in rgl/mesh2/gui_edit.cpp; count 0xa75eb4, success 0xa75eb8, used 0xa75ebc)

#endif // MB_REC34_H
