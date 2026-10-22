// world.h: the world object (global 0x5e2930; dialog_manager.h calls the same object GameData) and the small
// records next to its members in mission_ui.cpp. Only the fields read by 0x489c30..0x48a3d0 are declared; the
// layouts come from those functions (cited). Names provisional.
#ifndef GAME_UI_WORLD_H
#define GAME_UI_WORLD_H
#include <new>
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "./troop_inv.h"

struct PartyRec {                           // 0x138 bytes, table g_party_recs (0x5e2934)
    char pad0[0x50];
    unsigned f50_0 : 1;                     // +0x50 (bit 1 is tested as a bit-field: shr + test)   0x489e70
    unsigned f50_active : 1;
    unsigned f50_rest : 30;
    char pad54[0x64 - 0x54];
    unsigned f64_lo : 8;                    // +0x64 (bit 8 tested)    0x489e70
    unsigned f64_flag : 1;
    unsigned f64_rest : 23;
};

struct World {
    char pad0[0x78];
    int cur_party;                          // +0x78    0x48a290
    char pad7c[0x8c - 0x7c];
    int cur_troop;                          // +0x8c    0x48a080
    char pad90[0xb4 - 0x90];
    int fb4;                                // +0xb4 (nonzero scales the party value by 0.75)   0x489f40
    char padb8[0xc8 - 0xb8];
    int mode;                               // +0xc8 screen mode (0x12 = party screen)   0x48add0
    char padcc[0xe028 - 0xcc];
    tstring name;                           // +0xe028  0x489c30
    char pade038[0x13a00 - 0xe038];
    Troop* troops;                          // +0x13a00 table of 0x508-byte troop records   0x489c50, 0x489d10, 0x48a080
    char pad13a04[0x13a0c - 0x13a04];
    int excluded;                           // +0x13a0c record index skipped by sl_roll_other (0x489ee0)
    char pad13a10[0x13a58 - 0x13a10];
    char* party_recs;                       // +0x13a58 PartyRec table   0x489e70
    char pad13a5c[0x13ae0 - 0x13a5c];
    int range_lo;                           // +0x13ae0 random integer range of the party roll   0x48a150
    int range_hi;                           // +0x13ae4
    char pad13ae8[0x13b44 - 0x13ae8];
    float avg_value;                        // +0x13b44 running party value   0x48a000
    float avg_weight;                       // +0x13b48
    float avg_prev_time;                    // +0x13b4c

    tstring get_name();                                     // 0x489c30 (hidden return pointer, ret 4)
    int count_in(int idx, int id, char flag);               // 0x489c50
    bool sl_query(int kind, int troop, int arg);            // 0x435790 (thiscall, ret 0xc; not in this module)
    int sl_take(int group, int item, int want, char extras);   // 0x489d10
    int sl_roll_party(int lo, int hi);                      // 0x489e70
    int sl_roll_other(int lo, int hi);                      // 0x489ee0
    int sl_value_factor();                                  // 0x489f40 (truncated to int)
    void sl_update_avg();                                   // 0x48a000
    int sl_pick_chain(int leader, char lancer);             // 0x48a080 (walks the +0x50 link chain from leader)
    int sl_cell_coord(int party, int cell, int unused);     // 0x48a150 (ret 0xc: the third word is unused)
    int sl_party_value(int party);                          // 0x48a290
    int sl_party_price(int party);                          // 0x48a320
    void set_names(int slot, const tstring* a);             // 0x48a380
};
extern World* g_world;                      // 0x5e2930 (the pointer; ECX of every World member)

extern char* g_cell_table;                  // 0x5e29f8 table of 0xb0-byte party cell records (0x48a150)
extern int g_difficulty;                    // 0x5aa04c
extern int g_agent_count;                   // 0x5aa050
extern int g_int_5aa054;                    // 0x5aa054
extern int g_int_5aa058;                    // 0x5aa058
extern int g_stick_mode;                    // 0x5aa078
extern float g_speed_base;                  // 0x5aa07c (1.0f)
extern int g_store0;                        // 0xa70458 first out-slot of sl_query
extern int g_store1;                        // 0xa7045c

// two names and counters (0x48a3a0, 0x48a3d0)
struct NamedRec {
    int f00, f04;
    char pad08[0x1c - 0x08];
    tstring name1c;                         // +0x1c
    int f2c, f30, f34, f38, f3c, f40, f44, f48;
    int f4c;                                // +0x4c
    int f50;                                // +0x50
    char pad54[0x68 - 0x54];
    tstring name68;                         // +0x68
    tstring name78;                         // +0x78
    tstring name88;                         // +0x88
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0
    void setup2(const tstring* a, const tstring* b, const tstring* c, int v);       // 0x48a3d0
};

// a record with a name at +0xc and a selector at +0x1c4 (0x4893e0, 0x489400, 0x489a90)
struct RecA {
    int f0, f4, f8;
    tstring name;                           // +0x0c
    char flag1c;                            // +0x1c
    char pad1d[0x1c4 - 0x1d];
    int sel;                                // +0x1c4 (0..7)
    void reset();                           // 0x4893e0
    void set_name(const tstring* s);        // 0x489400
    void set_sel(const int* src);           // 0x489a90
};

struct Named16 {                            // 16-byte name + flag (0x489ab0)
    tstring name;                           // +0x00
    char flag10;                            // +0x10
    void set(const tstring* s);             // 0x489ab0
};

void sl_item_flag_all();                        // 0x48a210
void __stdcall sl_item_set_all(float v);        // 0x48a250

#endif // GAME_UI_WORLD_H
