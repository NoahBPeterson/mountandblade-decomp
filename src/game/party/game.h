// game.h: the game object's constructor, reset and loaders (game.cpp, 0x51c5a0..0x51ea?0; the map builder game_2.cpp and the
// new-game start-up troop_rec.cpp) and the records they touch. The object itself, with every member named, is GameData
// (src/include/game_data.h, global g_game 0x5e2930, 0x1a2b0 bytes); `Game` is GameData with the member functions of those
// modules (a class without data members of its own: g_game is a Game, reached through static_cast<Game*>(g_game)). The save-game
// stream is `stream_base` of src/rgl/stream.h: the slices called its slots r_int (+0x38 = r_float), r_i64b (+0x3c = r_bool),
// r_i64 (+0x44 = r_u64), r_int1 (+0x48 = r_i64), r_raw (+0x4c = r_int) and r_struct (+0x54 = r_tstr); the field types of
// GameData follow the slot that fills them.
#ifndef GAME_PARTY_GAME_H
#define GAME_PARTY_GAME_H
#include <stddef.h>
#include "../../include/layout32.h"
#include <new>
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_party_template_count
#include "../../include/dialog_rec.h"      // DialogBlock (TriggerRec)
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../ui/ui_common.h"
#include "../ui/troop_inv.h"
#include "../../include/party.h"      // PartyMgr (Game::scene)
#include "../../include/game_parts.h"   // ScriptDebugger, EventContext

// the one global g_game (0x5e2930) is declared by include/game_data.h as GameData*: this header's class Game is the same object (see above)
#include "../../include/game_data.h"      // GameData, g_game (0x5e2930)
// ---- globals -----------------------------------------------------------------------------------------
// the second element of the clock table g_ticks (ui_common.h, 0x5e2948) as two dwords: the original reads and
// writes the halves separately (0x51cfa0 copies lo then hi; 0x51dbd0 stores them from the loaded value)
#include "../../include/fx.h"            // g_ticks 0x5e2948

inline tstring& troop_name(Troop* t) { return *(tstring*)t; }      // the name is the first member (0x51c720)
inline rgl::string* rs(tstring* t) { return t; }     // the stream's tstring slot takes rgl::string*

// ---- small members with inline constructors (their store order is the constructor's) -----------------------
namespace rgl { struct mesh; struct entity; }
struct Light;                               // game/light.h
struct skin;                                // rgl/material.h

#include "../slot_array.h"                 // SlotArray: the records' slots (read 0x51e0b0); resize is the out-of-line instance 0x493090

#include "../int_vec.h"

// ---- records the loaders read ------------------------------------------------------------------------
#include "../../include/note_block.h"     // SoaBlock: the notes block read_soa reads
stream_base* read_soa(stream_base* s, SoaBlock* b);        // 0x51e1f0 (cdecl, returns the stream)

#include "../../include/trigger_rec.h"     // TriggerRec: the module trigger (0x78 bytes; element of the trigger list +0x139e8)
#include "../../include/trigger_entry.h"
#include "../trigger_vecs.h"   // TriggerVec, SimpleTriggerVec (the game's two trigger lists)

#include "../../include/game_recs.h"        // QuestRec (0x138 bytes, read 0x51e170), FactionRec, PartyTemplate (0xb0 bytes, read 0x51e410): the shared record classes

struct Faction : FactionRec {               // 0x300 bytes: element of Game::factions (Rec300 in the slices); the layout is FactionRec's
    __declspec(noinline) void read(stream_base* s, int n);     // 0x51e320 (ret 8)
};


// 0x60 item pairs read pair by pair (the Troop inventory, Troop +0xb4)
struct ItemPairs {                          // 0x300 bytes
    ItemRef pair[0x60];
    __declspec(noinline) void read(stream_base* s);        // 0x51e690
};

#include "../track_rec.h"                  // TrackRec: the 0x5c-byte map track record (the vector at +0x13a60)
// The vector of event / track records at Game+0x13a60 is a std::vector of the 0x5c-byte record, which other objects name by their own
// record type (E92 in party2/party_2.h). Its _Buy (0x51d1c0) and resize (0x51e7a0) are out of line in the original:
// the explicit specializations declared here are defined in game.cpp. The two library members resize calls were kept by the linker from
// other objects: std::vector<E92>::_Insert_n 0x4b54a0 (party2/party_2.cpp, under its record name E92 until that is TrackRec too)
// and the range erase 0x518990 (std::vector<TrackRec>::erase, inventory_panel/party_tracks.cpp).
#include "../../include/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0
struct E92 { int v[23]; ~E92() {} };        // 0x5c: the same record as party2/party_2.h E92 (same definition)
// std::vector<E92>::_Insert_n (0x4b54a0, party2/party_2.cpp) is a template member that stays out of line; its explicit specialization is only
// declared.
template<> void std::vector<E92>::_Insert_n(std::vector<E92>::iterator, size_t, const E92&);   // 0x4b54a0
template<> bool std::vector<TrackRec, std::allocator<TrackRec> >::_Buy(size_t);                 // 0x51d1c0 (0x5c-byte elements, max 0x2c8590b)
template<> void std::vector<TrackRec, std::allocator<TrackRec> >::resize(size_t, TrackRec);     // 0x51e7a0 (ret 0x60)
template<> std::vector<TrackRec, std::allocator<TrackRec> >::iterator std::vector<TrackRec, std::allocator<TrackRec> >::erase(std::vector<TrackRec, std::allocator<TrackRec> >::iterator, std::vector<TrackRec, std::allocator<TrackRec> >::iterator);   // 0x518990 (party_tracks.cpp)

// The Troop record's loader (0x51e540). Troop (ui/troop_inv.h) declares the inventory and equipment; the other
// fields read here are reached by offset.
struct TroopIO : Troop {
    __declspec(noinline) void read(stream_base* s);        // 0x51e540
};

// ---- the event pool (GameData::events): Event records (game/event.h, 0x98 bytes at 32 bits) in pages of 16 ---------------------
// 0x43e760 is the _Buy instance every pointer vector shares (identical bodies, folded by the linker); the explicit specialization is only
// declared, so constructing the page list is a call to it, as in the original.
template<> bool std::vector<Event*, std::allocator<Event*> >::_Buy(size_t);   // 0x43e760 (the event pool's page list)
#include "../event.h"                       // Event: the record of the pool at +0xcc (Pool<Event>, include/pool.h)
// (SpriteTable, the pool's constructor, reset and loader, is declared in game_data.h)

// the party manager embedded at Game +0xf8 is PartyMgr (include/party.h): its constructor is 0x51d250 (below), read 0x51eb90, reset_ids 0x51eda0, get 0x51ee50

// ---- the game object -----------------------------------------------------------------------------------
struct Game : GameData {
    Game();                                 // 0x51cfa0 (the member constructors run in offset order: GameData::GameData)
    void init(const tstring& name, int value, char flag);  // 0x51cc10
    void reset();                           // 0x51cc80
    void init_map();                        // 0x51d2b0 (uses the map globals, not this)
    void load_state(stream_base* s);        // 0x51dbd0
    void load_clock(stream_base* s);        // 0x51de70
    void load_quests(stream_base* s);       // 0x51e100
    void load_parties(stream_base* s);      // 0x51e240
    void load_factions(stream_base* s);     // 0x51e2b0
    void load_party_templates(stream_base* s);     // 0x51e3a0
    void load_items(stream_base* s);        // 0x51e450
    void load_troops(stream_base* s);       // 0x51e4c0
    void load_tracks(stream_base* s);       // 0x51e6c0
    void load_scene_parties(stream_base* s);       // 0x51eaa0
    // ---- src/game/mission5 (method declarations only: no layout change) ----
    void start_new_game();                  // 0x51f1a0 (the "GameStep4" of the new-game sequence)
    void more_start();                      // 0x51f800
    void build_map();                       // 0x520b20 (loads map.txt into the global map object)
    void build_scene();                     // 0x520d60
    __declspec(noinline) void release_orders();   // 0x5213a0 (destroys the 0x5c-byte orders of the array at +0x13a64)
    void build_map_trees();                 // 0x5213d0 (rebuilds the campaign-map tree meshes)
    // the map builder's members whose bodies never read `this`: the call sites load ecx (0x5217a2, 0x522ffe), both end in ret N
    __declspec(noinline) skin* terrain_material(int type);           // 0x523390
    __declspec(noinline) void scan_arrays(void* a, void* unused);    // 0x523630
    void load_game_data(int extra_triggers);   // 0x5236f0 (ret 4: opens the Data\*.txt tables in turn)
};
extern PartyTemplate* g_party_templates;    // 0x5e29f8 (0xb0-byte records)
extern int g_party_template_count;             // 0x5e29fc


MB_LAYOUT32_ASSERT(chk_game_size, sizeof(Game) == 0x1a2b0);

#endif // GAME_PARTY_GAME_H
