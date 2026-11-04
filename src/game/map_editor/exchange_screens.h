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
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/dialog_rec.h"

// ---- GameData offsets this region reads that game_data.h does not name (each cites a function of this region) ------------
#define GI(off) (*(int*)((char*)g_game + (off)))
#define GF(off) (*(float*)((char*)g_game + (off)))

#include "../party_x.h"                   // PartyX : Party: the one definition (map_editor and script)
#include "../key_names.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

struct PartyMgrX : PartyMgr {};
// the party-by-id getter 0x51ee50 (Scene::get(int), ret 4) and the other canonical classes the exchange screens call
struct PartyRecIO;
struct Scene {
    PartyRecIO* get(int id);   // 0x51ee50 (the party manager's by-id getter)
};
namespace c49e780 {
struct Party {
    int add_sprite(int a, int b, int kind);   // 0x49ebd0 (ret 0xc)
};
}
struct Map10007 {
    int lookup(unsigned id);   // 0x48ad00 (ui/mission.h: the id hash table that opens the party manager)
};
struct SoundCtl {
    char emit_event(int id, int a, int b);   // 0x435790 (agent/agent.h)
};
struct PartyScreen {
    void refresh_selection();   // 0x5063e0
    void refresh_party_members(int);   // 0x503ed0
    void refresh_morale();   // 0x503a50
};
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

inline PartyMgrX* party_mgr() { return (PartyMgrX*)&g_game->parties; }

// the party at index `id` of the manager (PartyMgr::pages is a std::vector<Party*>: operator[] gives the original's lea + load)
inline PartyX* party_at(PartyMgrX* m, int id)
{
    Party* page = m->pages[id >> 4];
    return (PartyX*)&page[id & 0xf];
}

inline PartyX* party_index(int id)
{
    PartyMgrX* m = party_mgr();
    int idx = ((Map10007*)(void*)m)->lookup(id);   // 0x48ad00
    return party_at(m, idx);
}

// the same lookup through a game-data pointer that is not g_game (update_party_sprites runs with this = the game data)
inline PartyX* party_index(GameData* g, int id)
{
    PartyMgrX* m = (PartyMgrX*)&g->parties;
    int idx = ((Map10007*)(void*)m)->lookup(id);   // 0x48ad00
    return party_at(m, idx);
}

struct GameDataX : GameData {               // the game object with the members this region calls
    // (sl_party_price 0x48a320 is GameData's)
    __declspec(noinline) void update_party_sprites();            // 0x532de0
    // (add_sprite 0x49ebd0 is c49e780::Party's)
};
inline GameDataX* gx() { return (GameDataX*)g_game; }

// 0x54: the second linked troop of a troop record (Troop::pad54; 0x531800)
inline int troop_f54(Troop* t) { return *(int*)((char*)t + 0x54); }

// ---- small objects ----------------------------------------------------------------------------------------------------
#include "../map_flags.h"                            // MapFlags, g_map_flags (0x5e291c): the one definition (defined in inventory_panel/mission_ctx.cpp)
#include "../window_mgr.h"      // g_windows (0xa73980), slot_as<T>(i): the screens are its slots
#include "../key_names.h"      // g_keyboard (0xa170c8)
extern MsgLog g_gui;                             // 0x5d7080

#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): setup_entry 0x531600, weight 0x5316f0

// one menu option (8 bytes, array at Menu+0x94)

// the screens' option records (slots of g_windows ...): id +4, +0x10/+0x14 arguments, +0x18 the option to return to
struct MenuItem {
    char pad0[4];
    int f4;                                 // +0x04
    char pad8[0x10 - 8];
    int f10;                                // +0x10
    int f14;                                // +0x14
    int f18;                                // +0x18
};

// mission template tables (records at 0xa735d0, 0x20 bytes each)
#include "../mission_type.h"                    // mission_type g_mission_types (0xa735d0): the start options of the mission menus

// (the mission start menus, vtables 0x595804 and 0x59584c, are the records Rec118 and Rec34 of screen_records.h)

// ---- the party exchange window (vtable 0x59587c) ---------------------------------------------------------------------
struct TargetW {                            // a scroll target (a list widget): the value source behind +0x484
    char pad[0xf4];
    float f_f4;                             // +0xf4
    char pad2[0x484 - 0xf8];
    ValueSource* src;                       // +0x484 (hud.h: the value source behind the widget)
};

typedef C4c3160 XferWin;       // the party exchange window: the record of rgl/mesh2/rec_tables.h (slot 3 is update 0x531800)

// ---- the screens with raw layouts -------------------------------------------------------------------------------------
struct ChoiceList : MissionUi {                // a choice list (was ListWidget; not agent5's 0x4a8-byte ListWidget, ctor 0x49deb0, whose +0x188 holds a scene): current index +0x1cc into the arrays at +0x19c / +0x1ac
    __declspec(noinline) void get3(int* a, int* b, int* c);      // 0x533a60 (ret 0xc)
};

struct OutIntVec : IntVec {                 // a std::vector<int> embedded in a screen (src/game/int_vec.h) whose push_back the original calls out of line (std::vector<int>::push_back 0x440b30, ret 4)
};

struct VecIter {                            // VC7.1 vector<int>::iterator + n, in the out-parameter form
    int* ptr;                               // +0x00
    __declspec(noinline) int** advance(int** out, int n);        // 0x534160 (ret 8)
};

struct ScreenBase {                         // what the raw-layout screens share: +0x18 the option to return to
    char pad0[0x18];
    int f18;                                // +0x18
};
struct TickScreen : ScreenBase {            // vtable 0x595a08
    // (build(a, b) 0x50a1c0 is note_window's, build(arg) 0x4fc820 is LogScreen's)
};
struct EventScreen : ScreenBase {           // vtable 0x59619c
    void on_event(int code);                // 0x533070 (ret 4)
};
struct InputScreen : TickScreen {           // vtable 0x5961cc (shares the animation helpers)
    void on_input(int code);                // 0x533aa0 (ret 4)
};
struct SourceScreen : ScreenBase {          // vtables 0x5961f0 / 0x596214: +0x30 the item source, +0x58 the repeat interval
    char pad1c[0x30 - 0x1c];
    MissionSource src;                      // +0x30
    float interval;                         // +0x58
    void hook_a(int arg);                   // 0x534180 (vtable 0x5961f0 slot 0)
    void hook_b(int arg);                   // 0x534230 (vtable 0x596214 slot 0)
};

// ---- globals -----------------------------------------------------------------------------------------------------------





extern int g_trial_mode;                    // 0x5e29c4
extern int g_a735c4;                        // 0xa735c4

extern int g_a7333c;                        // 0xa7333c
extern int* g_menu_data;                    // 0xa739c0
extern float g_k59c48c;                     // 0x59c48c (weight ramp constant)
extern float g_half;                        // 0x549d70 (0.5f)
extern float g_033;                         // 0x54b894 (0.33f)
extern MenuItem* g_a739a0;                  // 0xa739a0
extern MenuItem* g_a739b0;                  // 0xa739b0
extern MenuItem* g_a739b4;                  // 0xa739b4
extern MenuItem* g_a739bc;                  // 0xa739bc
extern MenuItem* g_a739e4;                  // 0xa739e4
extern MenuItem* g_a739e8;                  // 0xa739e8
extern MenuItem* g_a739ec;                  // 0xa739ec
struct KeyDown {};                          // the key state object at 0xa73730: OriginPairs
struct OriginPairs {
    bool clicked(int key);   // 0x48ac90 (ret 4)
};
extern KeyDown g_game_keys;                    // 0xa73730
#define STR_TABLE (&g_strtable)     // the localized string table (str_table.h)

#endif // GAME_MAP_EDITOR_PARTY_SCREEN_H
