// options_panel.h: the options / character panel (0x4d7370..0x4d92c0): control-key names, the faction report, the
// character import / export flow and the character file writer / reader. Names provisional; layouts derived from
// the disassembly of the functions cited per field. The game object, the troop and faction records are the
// canonical Game / Troop / Faction (../party/game.h, ../../include/troop.h); the members those headers lack are
// reached through GameIo (a view of the same object with the two member functions of this module) and offsets.
#ifndef OPTIONS_PANEL_OPTIONS_PANEL_H
#define OPTIONS_PANEL_OPTIONS_PANEL_H
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "../tstring.h"
#include "../party/game.h"
#include "../../include/troop.h"

extern const float RENOWN_SCALE;                                    // 0x54ab28 (100.0f)

// the game's localized-string table (0x5e2a18): lookup by a scratch tstring holding the text key
#include "../str_table.h"

#include "../slot.h"                                // Slot, g_keyboard (0xa170c8): key_name 0x4d7610

// a ui control: +0x2c set_cursor(bool), +0x44 is_key_down(int)
struct ui_control {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void set_cursor(int show);                              // +0x2c
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40();
    virtual char is_key_down(int key);                              // +0x44
};

// set_text returns an 8-byte object through a hidden pointer that no caller destroys (a non-POD pair)
#include "../wret.h"
// the panel's text store
struct text_store {
    virtual void t00(); virtual void t04(); virtual void t08(); virtual void t0c();
    virtual void t10(); virtual void t14(); virtual void t18(); virtual void t1c();
    virtual void t20(); virtual void t24(); virtual void t28(); virtual void t2c();
    virtual void t30(); virtual void t34(); virtual void t38(); virtual void t3c();
    virtual void t40(); virtual void t44(); virtual void t48(); virtual void t4c();
    virtual void t50(); virtual void t54(); virtual void t58(); virtual void t5c();
    virtual void t60(); virtual void t64(); virtual void t68(); virtual void t6c();
    virtual void t70(); virtual void t74(); virtual void t78(); virtual void t7c();
    virtual void t80(); virtual void t84(); virtual void t88(); virtual void t8c();
    virtual void t90(); virtual void t94(); virtual void t98(); virtual void t9c();
    virtual void ta0();
    virtual WRet set_text(const tstring& s);                        // +0xa4
    virtual void ta8();
    virtual void add_text(tstring* text, float* pos);               // +0xac
};

#include "../main_menu.h"                                   // main_menu (the slice called it MenuRec: g_main_menu.rec, 0xa73980)
// g_main_menu (0xa73980, `rec` at +0 is the dialog box record): main_menu, ../main_menu.h

bool mb_file_exists(const char* path);                              // 0x415120

// the character file is read into the shared ini table (0x2c-byte entries: key +0, value +0x1c; ctor 0x4c1be0, load 0x4bd630, dtor 0x4be920)
#include "../ini_file.h"

// the export file object: the text log's classes. 0x595618 is log_stream (src/rgl/stream.h, g_log's class); 0x595990 (outfile) is the same with the read slots
// returning *this. Slot 10 (+0x28) writes a text line (0x4d86d0).
#include "../../rgl/stream.h"
// 0x595990 (outfile): the log stream's table, 27 slots like the others
struct outfile : log_stream {
    outfile(const char* p) { ((tstring&)name).assign(p); }
    virtual stream_base& r_float(float* v);                         // 0x415870
    virtual stream_base& r_bool(char* v);                           // 0x415870
    virtual stream_base& r_uint(unsigned* v);                       // 0x415870
    virtual stream_base& r_u64(unsigned __int64* v);                // 0x415870
    virtual stream_base& r_i64(__int64* v);                         // 0x415870
    virtual stream_base& r_int(int* v);                             // 0x415870
    virtual stream_base& r_tstr(rgl::string* v);                    // 0x415870
    virtual int flush();                                            // 0x42cc10
};

#define MODE_RA " RA"                                               // 0x595990
#define MODE_R  "r"                                                 // 0x54a8fc
#define MODE_W  "w"                                                 // 0x54b058

// the game object with this module's two member functions (a view of Game; no data)
struct GameIo : Game {
    void export_character(outfile* f);                              // 0x4d86d0 (ret 4)
    void import_apply(ini_file* table);                             // 0x4d8c20 (ret 4)
};

// the options panel (offsets read by 0x4d7370 .. 0x4d9260). The first 0x38 bytes are the control-key table of the
// page (four bindable keys at +0, +8, +0x10, +0x18, each followed by its button word).
struct options_panel {
    int  key_menu;                                                  // +0x00
    int  menu_open;                                                 // +0x04
    int  key_prev;                                                  // +0x08
    char pad0c[4];                                                  // +0x0c
    int  key_select;                                                // +0x10
    char pad14[4];                                                  // +0x14
    union {
        int  key_next;                                              // +0x18 (0x4d7370)
        int  menu_choice;                                           // +0x18 the main-menu option the page returns to (0x4d7d90, 0x4d8000)
    };
    char pad1c[0x2c - 0x1c];
    text_store* texts;                                              // +0x2c
    ui_control* menu_keys;                                          // +0x30
    ui_control* import_key;                                         // +0x34
    ui_control* cursor_control;                                     // +0x38
    ui_control* refresh_key;                                        // +0x3c
    int  export_pending;                                            // +0x40
    int  import_pending;                                            // +0x44

    void key_name(int which, tstring* out);                         // 0x4d7370
    void key_binding_name(int slot, tstring* out, int* translated);  // 0x4d7d10
    __declspec(noinline) void update_cursor();                      // 0x4d7de0
    __declspec(noinline) void refresh_texts();                      // 0x4d7e00
    __declspec(noinline) void draw_faction_report();                // 0x4d7e50
    void export_path(char* out);                                    // 0x4d9260 (ret 4)
};
#endif
