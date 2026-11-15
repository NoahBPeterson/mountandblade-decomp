// exchange_screens.h: the screen objects of code 0x531590..0x5342a6 (TU 119 party_3 + TU 120 screen): the mission start
// menus, the party exchange window and the screens whose first virtual is the per-frame hook. Names provisional.
//
// Every class here has its own vtable: the hooks sit in the first virtual slot of the vtables 0x595810, 0x595858,
// 0x59587c, 0x595a08, 0x59619c, 0x5961cc, 0x5961f0 and 0x596214 (found by scanning .rdata for the function addresses),
// so the slices' "Menu", "Mission", "Screen", "GameObj" were several different classes. Canonical classes used:
//   GameData (src/include/game_data.h; the offsets it lacks are read through GI/GF below), Party / PartyMgr / Stack
//   (src/include/party.h), Troop (src/include/troop.h), MsgLog (src/include/msg_log.h), and from
//   src/game/options_panel/hud.h: MissionUi (used() = vtable +0x44, set_key_flags() = +0x48), ValueSource, MainMenu,
//   MissionSource (the item source embedded at +0x30 of the screens).
// The classes below that are views of those (PartyX, PartyMgrX, GameDataX) only add member declarations of this region.
#ifndef GAME_MAP_EDITOR_PARTY_SCREEN_H
#define GAME_MAP_EDITOR_PARTY_SCREEN_H
#include "../../include/image_addrs.h"
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include "../options_panel/hud.h"
#include "../screen_records.h"          // MenuEntry, Rec118 / Rec34 (the mission start menus)
#include "../party_screen_class.h"      // PartyScreen: the party exchange window (slot 3 is update 0x531800)
#include "../options_panel/mission_item_recs.h"   // ItemA..D, MissionBlocker: the trigger-record screens whose slots 2 / 3 live in this file
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../ui/map_list.h"                // MWidget: the notes window's text lists (ChoiceList)
#include "../str_table.h"
#include "../../include/dialog_rec.h"

// ---- GameData offsets this region reads that game_data.h does not name (each cites a function of this region) ------------
#define GI(off) (*(int*)((char*)g_game + (off)))
#define GF(off) (*(float*)((char*)g_game + (off)))

#include "../party_x.h"                   // PartyX : Party: the one definition (map_editor and script)
#include "../key_names.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../trial_mode.h"

// the canonical classes the exchange screens call
struct note_window {
    void build(int a, int b);   // 0x50a1c0 (ret 8)
};
typedef Rec58 LogScreen;   // the game-log window (screen_records.h; build 0x4fc820)
namespace std {
template<> void vector<int, allocator<int> >::push_back(const int& x);        // 0x440b30
}
namespace std {
template<> void vector<int, allocator<int> >::resize(size_type n, int x);     // 0x43f310
}

inline PartyMgr* party_mgr() { return &g_game->parties; }

// the party at index `id` of the manager (PartyMgr::pages is a std::vector<Party*>: operator[] gives the original's lea + load)
inline PartyX* party_at(PartyMgr* m, int id)
{
    Party* page = m->pages[id >> 4];
    return (PartyX*)&page[id & 0xf];
}

inline PartyX* party_index(int id)
{
    PartyMgr* m = party_mgr();
    int idx = m->lookup(id);   // 0x48ad00
    return party_at(m, idx);
}

// the same lookup through a game-data pointer that is not g_game (update_party_sprites runs with this = the game data)
inline PartyX* party_index(GameData* g, int id)
{
    PartyMgr* m = &g->parties;
    int idx = m->lookup(id);   // 0x48ad00
    return party_at(m, idx);
}

struct GameDataX : GameData {               // the game object with the members this region calls
    // (sl_party_price 0x48a320 is GameData's)
    __declspec(noinline) void update_party_sprites();            // 0x532de0
    // (add_sprite 0x49ebd0 is GameData's)
};
inline GameDataX* gx() { return (GameDataX*)g_game; }

// the troop's second upgrade (read through a call: the frame update's register choice depends on it; 0x531800)
inline int second_upgrade(Troop* t) { return t->upgrade_troop[1]; }


// ---- small objects ----------------------------------------------------------------------------------------------------
#include "../map_flags.h"                            // MapFlags, g_map_flags (0x5e291c): the one definition (defined in inventory_panel/mission_ctx.cpp)
#include "../window_mgr.h"      // g_windows (0xa73980), slot_as<T>(i): the screens are its slots
#include "../key_names.h"      // g_keyboard (0xa170c8)

#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): setup_entry 0x531600, weight 0x5316f0

// mission template tables (records at 0xa735d0, 0x20 bytes each)
#include "../mission_type.h"                    // mission_type g_mission_types (0xa735d0): the start options of the mission menus

// (the mission start menus, vtables 0x595804 and 0x59584c, are the records Rec118 and Rec34 of screen_records.h)

// ---- the party exchange window (vtable 0x59587c) ---------------------------------------------------------------------
struct TargetW : MissionUi {                // a scrolling list (Camera, party2/camera_class.h: follow[1] at +0x484 is its scroll bar, the value source)
    char pad178[0x484 - 0x178];
    ValueSource* src;                       // +0x484 (Camera::follow[1]; hud.h's view of the scroll bar UiD)
};

typedef PartyScreen XferWin;       // the party exchange window is the party screen (party_screen_class.h; slot 3 is update 0x531800)

// ---- the screens with raw layouts -------------------------------------------------------------------------------------
struct ChoiceList : MWidget {                // the notes window's text lists (ui/map_list.h MWidget, vtable 0x5995c0): the spot index sel (+0x1cc) into list_c / list_d
    __declspec(noinline) void get3(int* a, int* b, int* c);      // 0x533a60 (ret 0xc)
    std::vector<int>& pages() { return *(std::vector<int>*)&list_c; }   // +0x198 the page of each spot (an IntList: VC7.1 std::vector<int>)
    std::vector<int>& args() { return *(std::vector<int>*)&list_d; }    // +0x1a8 its argument
};

struct VecIter {                            // VC7.1 vector<int>::iterator + n, in the out-parameter form
    int* ptr;                               // +0x00
    __declspec(noinline) int** advance(int** out, int n);        // 0x534160 (ret 8)
};

// (the screens whose slot 3 is in this file are the records ItemA / ItemB / ItemC / ItemD of options_panel/mission_item_recs.h)

// ---- globals -----------------------------------------------------------------------------------------------------------






extern float g_k59c48c;                     // 0x59c48c (weight ramp constant)
extern const float g_half;                        // 0x549d70 (0.5f)
extern float g_033;                         // 0x54b894 (0.33f)
#include "../game_keys.h"                    // GameKeys g_game_keys (0xa73730)
#define STR_TABLE (&g_strtable)     // the localized string table (str_table.h)

#endif // GAME_MAP_EDITOR_PARTY_SCREEN_H
