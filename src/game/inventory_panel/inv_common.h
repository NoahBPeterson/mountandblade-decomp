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
#include "../strat/gui_manager.h"
#include "../ui/mission_ui.h"

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

extern ini_file g_game_vars;                      // 0xa44240 (the option table: key tstring +0, value tstring +0x1c, count +0x2c000)

// ---- classes of other modules whose definitions own methods the game data view calls --------------------------------------
// (the definitions carry these class names; a call through GameData / PartyMgr would name a symbol nothing defines)

// ---- the game data (g_game) with the members these modules call ----------------------------------------------------
struct GameDataV : GameData {
    __forceinline tstring* string_value(tstring* out, int value)   // 0x48e9e0 (ret 8): GameData::string_value (hidden result), built in place in *out
    {
        new (out) tstring(GameData::string_value(value));
        return out;
    }
    char emit_event4(int id, int a, int b, int c);          // 0x512620 (ret 0x10)
    int price(ItemRef* item, int idx, char adjust, char plain, char scaled);   // 0x5129d0 (ret 0x14)
    void clear_hero_ammo(unsigned id);                      // 0x513eb0 (ret 4: zeroes the f10 word of the hero stacks of a party)
    void fill_ids(int* out, int id);                        // 0x5168e0 (ret 8)
    __declspec(noinline) void reset_trade_ids();            // 0x5168b0
    int weekly_cost();                      // 0x504e20 (thiscall, no args: the weekly wage of the player's party)
    __forceinline int sl_query(int kind, int troop, int arg) { return emit_event(kind, troop, arg); }   // 0x435790 (ret 0xc; SoundCtl::emit_event)
};
#define GV ((GameDataV*)g_game)

// a tstring a callee constructs in place (string_value writes into its first argument)
struct tstring_buffer {
    char bytes[sizeof(tstring)];
    tstring* get() { return (tstring*)bytes; }
    void destroy() { get()->~tstring(); }
};

struct NodeA;

// the main menu / screen manager (0xa73980)
#include "../window_mgr.h"                         // g_windows (0xa73980)

#include "../map_flags.h"                             // MapFlags (0x5e291c): the one definition
#define GD_I(off) (*(int*)((char*)g_game + (off)))

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


// ---- helpers every module here uses -------------------------------------------------------------------------------
// PartyMgr::index_of is the party-id hash lookup 0x48ad00 (game/ui/mission.h: Map10007::lookup)
// the party record behind a page-table index (PartyMgr::pages, pages of 16 records of 0x5ac8 bytes)
static inline Party* party_at(PartyMgr* m, int idx)
{
    Party* page = m->pages[idx >> 4];
    return &page[idx & 0xf];
}
static inline Party* party_index(int id)
{
    PartyMgr* m = &g_game->parties;
    return party_at(m, m->lookup(id));
}
#define PARTIES (&g_game->parties)

// the rgl mesh/resource objects reached through the resource manager (0x5d6fa0)

void __cdecl label_refresh(MissionUi* w, tstring name);  // 0x4e4c70 (cdecl, the name by value: sets a freshly built widget up from its ini keys)
#define label_refresh(w, name) label_refresh((MissionUi*)(w), name)

#endif // INV_COMMON_H
