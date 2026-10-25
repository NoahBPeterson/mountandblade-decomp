// inv_common.h: what the modules of src/game/inventory_panel share (0x502cc0..0x51c5a0, the T-regime run "tu112").
// Canonical headers are included, never redefined. Where the canonical class lacks a method or a field these modules
// read, the access goes through a pure-addition VIEW declared here:
//   * GameDataV  : GameData (src/include/game_data.h) + the methods of 0x504e20, 0x48a290, ... and offset accessors;
//   * PsOps/...  : a pure interface laid over the widget family (MissionUi, src/game/ui/mission_ui.h): the vtable
//                  slots these modules call, with the signature each call site shows (same idea as party/widget_ops.h).
// Every offset cites a function of this region (slice-era notes are kept in MIGRATION.md).
#ifndef INV_COMMON_H
#define INV_COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include <vector>
#include "../config/config.h"
#include "../tstring.h"
#include "../str_table.h"
#include "../item_ref.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"
#include "../ui/mission_ui.h"

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

extern StrTable g_strtable;                 // 0x5e2a18
extern ini_file g_game_vars;                      // 0xa44240 (the option table: key tstring +0, value tstring +0x1c, count +0x2c000)
extern int g_store0;                        // 0xa70458 (first out-slot of GameData::emit_event / sl_query)
extern int g_store1;                        // 0xa7045c

// ---- the game data (g_game) with the members these modules call ----------------------------------------------------
struct GameDataV : GameData {
    tstring* string_value(tstring* out, int value);   // 0x48e9e0 (ret 8; GameData::string_value with the hidden result pointer spelled out)
    char emit_event4(int id, int a, int b, int c);          // 0x512620 (ret 0x10)
    int price(ItemRef* item, int idx, char adjust, char plain, char scaled);   // 0x5129d0 (ret 0x14)
    void clear_hero_ammo(unsigned id);                      // 0x513eb0 (ret 4: zeroes the f10 word of the hero stacks of a party)
    int gate_rating(unsigned p1, unsigned p2, int p3);      // 0x491b20 (ret 0xc)
    int token_lookup(int party, int a, int b, int c);            // 0x491f20 (ret 0x10)
    void fill_ids(int* out, int id);                        // 0x5168e0 (ret 8)
    __declspec(noinline) void reset_trade_ids();            // 0x5168b0
    void attach_party_center(const float* pt, int a, int b);   // 0x49e9c0 (ret 0xc)
    void tick(float dt);                                 // 0x516ac0 (ret 4)
    int weekly_cost();                      // 0x504e20 (thiscall, no args: the weekly wage of the player's party)
    int sl_party_value(int party);          // 0x48a290 (ret 4)
    int sl_party_price(int party);          // 0x48a320 (ret 4)
    int sl_query(int kind, int troop, int arg);   // 0x435790 (ret 0xc; the same function as GameData::emit_event, int-returning view)
};
#define GV ((GameDataV*)g_game)

// a tstring a callee constructs in place (string_value writes into its first argument)
struct tstring_buffer {
    char bytes[sizeof(tstring)];
    tstring* get() { return (tstring*)bytes; }
    void destroy() { get()->~tstring(); }
};

// the UI root object (0x5d7080)
struct GuiRoot {
    void reset();                                   // 0x42d1b0
    void play_sound();                                    // 0x48b2d0
    void set_o564(int flag);                          // 0x42cd80 (ret 4)
    void add_root(MissionUi** w);                 // 0x49b120 (ret 4)
    void set_o568(void* o, bool clear_lines);       // 0x42cdb0 (ret 8)
    void set_view(float* pt);                       // 0x4c5190 (ret 4)
    void draw_string(const tstring* s, float y, int color, char b);   // 0x4e43e0 (ret 0x10)
};
extern GuiRoot g_gui_root;                          // 0x5d7080

// the main menu / screen manager (0xa73980)
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

#include "../map_flags.h"                             // MapFlags (0x5e291c): the one definition
#define GD_I(off) (*(int*)((char*)g_game + (off)))
#define GD_F(off) (*(float*)((char*)g_game + (off)))
#define GD_C(off) (*(char*)((char*)g_game + (off)))

// ---- the inline ini lookups the callers expand (0x4bd5e0 inlined) ---------------------------------------------------
// find() then atof of the value text (0x4bd5a0)
static __forceinline float ini_float(const tstring& key)
{
    float ret = 0.0f;
    int idx = g_game_vars.find(key);
    if (idx >= 0)
        ret = (float)atof(g_game_vars.entries[idx].value.c_str());
    return ret;
}

// ---- widget interface views -----------------------------------------------------------------------------------------
// 41 filler slots (+0x00..+0xa0) shared by the views below
struct PsFill {
    virtual void f00(); virtual void f04(); virtual void f08(); virtual void f0c(); virtual void f10(); virtual void f14();
    virtual void f18(); virtual void f1c(); virtual void f20(); virtual void f24(); virtual void f28(); virtual void f2c();
    virtual void f30(); virtual void f34(); virtual void f38(); virtual void f3c(); virtual void f40();
    virtual char clicked_on(int which);             // +0x44 (0x50fe20, 0x5108d0, 0x512c80)
    virtual void set_key_flags(int flags);          // +0x48 (0x50fe20)
    virtual void f4c(); virtual void f50(); virtual void f54();
    virtual void set_pos(float* xy);                // +0x58 (0x502cc0, 0x504ef0, 0x504ba0, 0x5059c0)
    virtual void f5c(); virtual void f60();
    virtual void set_size(float* xy);               // +0x64
    virtual void f68(); virtual void f6c(); virtual void f70(); virtual void f74(); virtual void f78(); virtual void f7c();
    virtual void add_image(int layer, void* mesh);  // +0x80 (0x502cc0, 0x507310, 0x5063e0)
    virtual void f84(); virtual void f88(); virtual void f8c(); virtual void f90(); virtual void f94();
    virtual void set_colour(int* rgb);              // +0x98 (0x503ed0, 0x5063e0)
    virtual void f9c(); virtual void fa0();
};
struct PsOps : PsFill {
    virtual void set_text(float* out, const tstring* s);        // +0xa4 (the text size is written to out; 0x488b50)
    virtual void fa8();
    virtual void set_label(const tstring* text, const float* pos);   // +0xac (0x5059c0, 0x5063e0)
    virtual void clear();                           // +0xb0 (0x503ed0)
    virtual void act_b4();                          // +0xb4
    virtual void enable();                          // +0xb8
    virtual void act_bc();                          // +0xbc
    virtual void fc0(); virtual void fc4();
    virtual void add_child(void* child);            // +0xc8
};
#include "../wret.h"
struct PsOpsW : PsFill {                            // the same widget, slot +0xa4 as the money label calls it (0x504d50)
    virtual WRet set_text_w(const tstring& s);      // +0xa4
};
#define PSO(w) ((PsOps*)(w))

// ---- helpers every module here uses -------------------------------------------------------------------------------
// the party record behind a page-table index (PartyMgr::pages, pages of 16 records of 0x5ac8 bytes)
static inline Party* party_at(PartyMgr* m, int idx)
{
    Party* page = m->pages._Myfirst[idx >> 4];
    return &page[idx & 0xf];
}
static inline Party* party_index(int id)
{
    PartyMgr* m = &g_game->parties;
    return party_at(m, m->index_of(id));
}
#define PARTIES (&g_game->parties)

// the rgl mesh/resource objects reached through the resource manager (0x5d6fa0)
#include "../res_mesh.h"                       // ResMesh (was RMesh), ResMgr (was RResMgr), ObjTable: the one definition
extern ResMgr g_resmgr_inv;                        // 0x5d6fa0

void __cdecl label_refresh(void* w, tstring name);  // 0x4e4c70 (cdecl, the name by value: sets a freshly built widget up from its ini keys)

#endif // INV_COMMON_H
