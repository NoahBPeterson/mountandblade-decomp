// game.h: the game-state object (global g_game, 0x5e2930, 0x1a2b0 bytes) and the records its constructor, reset
// and save-game loaders (game.cpp, 0x51c5a0..0x51ea?0) touch. Only the fields those functions and the party window
// (party_window.cpp) read or write are named; the gaps are padding. Every field cites a function that proves it
// (the offset table is checked at the end of the header: a wrong pad is a compile error).
//
// The same object is `World` in src/game/ui/world.h (dialog_manager.h and terrain_b/game_data.h call it GameData):
// those headers declare other fields of it and cannot be included together with this one (see MIGRATION.md).
// Member names follow include/game_data.h's GameData at the same offsets (main_party, sel_party, map_live, player, reveal_all,
// cnt_killed.., mode, events, parties, attach_*, range_lo/hi, string_count, avg_*, labels, scene_open, emit_name) and, where
// GameData has none, script/script_interpreter.h's GameX, whose names are the script operations that read them (encounter_flags,
// background_presentation, merchandise_modifier_quality, map_conversation_*, scenes / num_scenes ("Number of scenes in file",
// 0x51e240), rest_*, current_menu, jump_*, screen_*, presentation, notes_*, clock, current_hours, cloud/haze_amount,
// message_log, stack_rows, question_answer, background_mesh, menu_tableau, talk_*). The save-game stream is `stream_base` of src/rgl/stream.h: the slices called its
// slots r_int (+0x38 = r_float), r_i64b (+0x3c = r_bool), r_i64 (+0x44 = r_u64), r_int1 (+0x48 = r_i64),
// r_raw (+0x4c = r_int) and r_struct (+0x54 = r_tstr); the field types below follow the slot that fills them.
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
struct GameData;
extern GameData* g_game;                    // 0x5e2930
#define g_game_G (*(Game**)&g_game)         // this header's view of it
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

// A clock index and its start stamp: 16 bytes, the stamp is g_ticks[clock] (0x51cfa0: clocks 1, 1, 1, 0, 1).
struct GameTimer {
    int clock;                              // +0
    int pad4;
    __int64 t;                              // +8   also stored by 0x51cc80 (g_ticks[clock]) and loaded by 0x51de70
    GameTimer(int c)
    {
        clock = c;
        ((int*)&t)[0] = ((int*)&g_ticks[c])[0];
        ((int*)&t)[1] = ((int*)&g_ticks[c])[1];
    }
};


// 0x10-byte record built by the out-of-line constructor 0x4638b0 (four of them at +0x13b64): terrain_a/game_state.h calls it Block16
// (the default colour-ramp block, 0, 0.5, 0, 0), which is the name of the definition.
struct Block16 {
    Vec2 p, q;
    Block16();                              // 0x4638b0
};
typedef Block16 Rec10;

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

struct TrackRec {                           // 0x5c bytes: a game track (Rec5c)
    float x, y, z;                          // +0 position (r_float x3: 0x51e750; rec5c.h names them x, y, z)
    char padc[4];
    float f10;                              // +0x10 (r_float)
    float age;                              // +0x14 (r_float; rec5c.h: age / timer)
    int flags;                              // +0x18 (r_int; rec5c.h: 0x20000000 visible, 0x10000000 battle track)
    char pad1c[0x5c - 0x1c];
    __declspec(noinline) void read(stream_base* s);        // 0x51e750
};
// The vector of event / track records at Game+0x13a60 is a std::vector of the 0x5c-byte record, which other objects name by their own
// record type (E92 in party2/party_2.h, Rec5c in rec5c.h). Its _Buy (0x51d1c0) and resize (0x51e7a0) are out of line in the original:
// the explicit specializations declared here are defined in game.cpp. The two library members resize calls were kept by the linker from
// other objects under those objects' record names (std::vector<E92>::_Insert_n 0x4b54a0, party2/party_2.cpp; the range erase 0x518990,
// inventory_panel/game_tick.h Vec5c::shift_over), so resize reaches them through those names until the three record types are one.
struct Rec5c;                               // src/game/rec5c.h: the 0x5c-byte record (TrackRec is the same record)
struct Vec5c {
    Rec5c** shift_over(Rec5c** out, Rec5c* dst, Rec5c* src);   // 0x518990 (ret 0xc; inventory_panel/game_tick.h)
};
#include "../../include/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0
struct E92 { int v[23]; ~E92() {} };        // 0x5c: the same record as party2/party_2.h E92 (same definition)
// std::vector<E92>::_Insert_n (0x4b54a0, party2/party_2.cpp) is a template member that stays out of line; its explicit specialization is only
// declared.
template<> void std::vector<E92>::_Insert_n(std::vector<E92>::iterator, size_t, const E92&);   // 0x4b54a0
template<> bool std::vector<TrackRec, std::allocator<TrackRec> >::_Buy(size_t);                 // 0x51d1c0 (0x5c-byte elements, max 0x2c8590b)
template<> void std::vector<TrackRec, std::allocator<TrackRec> >::resize(size_t, TrackRec);     // 0x51e7a0 (ret 0x60)

// The Troop record's loader (0x51e540). Troop (ui/troop_inv.h) declares the inventory and equipment; the other
// fields read here are reached by offset.
struct TroopIO : Troop {
    __declspec(noinline) void read(stream_base* s);        // 0x51e540
};

// ---- the event pool (Game +0xcc): Event records (game/event.h, 0x98 bytes at 32 bits) in pages of 16 ---------------------
// 0x43e760 is the _Buy instance every pointer vector shares (identical bodies, folded by the linker); the explicit specialization is only
// declared, so constructing the page list is a call to it, as in the original.
template<> bool std::vector<char*, std::allocator<char*> >::_Buy(size_t);      // 0x43e760
#include "../event.h"                       // Event: the record of the pool at +0xcc (Pool<Event>, include/pool.h)
struct SpriteTable {
    int count;                              // +0     0x51e840
    IntVec free_ids;                        // +4     0x51e840 (pad +4, first +8, last +0xc, end +0x10)
    std::vector<char*> pages;               // +0x14  0x51d220 (_Buy(0)); its _Myfirst (+0x18) is read by 0x51e840 / 0x51ea10
    int f24, f28;                           // +0x24  0x51e840 (r_int at +0x24)
    SpriteTable();                          // 0x51d220
    void alloc_id() { ((Pool<Event>*)this)->alloc(); }        // 0x493700 (Pool<Event>::alloc, world_data/world_data.cpp: the same pool object; ret 0, the id unused)
    __declspec(noinline) void clear();                      // 0x51ea10
    void read(stream_base* s, int arg);                     // 0x51e840 (ret 8)
    Event* at(int i) { return (Event*)pages._Myfirst[(unsigned)i >> 4] + (i & 0xf); }   // the pages are Pool<Event>::alloc's `new Event[16]`
};

// the party manager embedded at Game +0xf8 is PartyMgr (include/party.h): its constructor is 0x51d250 (below), read 0x51eb90, reset_ids 0x51eda0, get 0x51ee50

// ---- the game object -----------------------------------------------------------------------------------
struct Game {
    int kind;                                // +0x0  0x51cc10 (init), 0x51cc80 (-1)
    int f4;                                  // +0x4  0x51dbd0 (r_int)
    tstring title;                           // +0x8  0x51cfa0, 0x51c720 step 2 (the save's name)
    char flag18;                             // +0x18  0x51cc10, 0x51cc80
    char pad19[0x1c - 0x19];
    tstring module;                          // +0x1c  0x51cc10
    rgl::mesh* map_track;                    // +0x2c  0x51cc80 (0), mission5/game_2.cpp build_scene 0x520d60
    rgl::entity* track_entity;               // +0x30  0x520d60
    std::vector<rgl::mesh*> ents;            // +0x34  0x51cfa0 (its three pointers zeroed), the map cell meshes (0x5213d0)
    rgl::mesh* map_obj;                      // +0x44  0x51cc80 (0), 0x520d60
    rgl::vector4 fog1;                       // +0x48  0x520d60 (the sun colour)
    rgl::vector4 fog2;                       // +0x58  (the fog colour)
    rgl::vector4 fog3;                       // +0x68
    int main_party;                          // +0x78  0x51cc10, 0x51cc80 (party id used by the scene loader 0x51eaa0)
    int f7c;                                 // +0x7c  0x51cc80
    int f80;                                 // +0x80  0x51cc10
    int sel_party;                           // +0x84  0x51cc10, 0x51cc80
    char map_live;                           // +0x88  0x51cc10, 0x51cc80
    char pad89[0x8c - 0x89];
    int player;                              // +0x8c  0x502320 (index of the current troop), 0x51dbd0
    int hero_troop;                          // +0x90  0x502320 (compared with a troop index), 0x51cc10 (= 1)
    char f94;                                // +0x94  0x51cc80
    char reveal_all;                         // +0x95  0x51cc80
    char pad96[0x98 - 0x96];
    int cnt_killed;                          // +0x98  0x51cc10, 0x51dbd0 (r_int)
    int cnt_killed_friend;                   // +0x9c  0x51cc10, 0x51dbd0
    int cnt_ko;                              // +0xa0  0x51cc10, 0x51dbd0
    int cnt_ko_friend;                       // +0xa4  0x51cc10, 0x51dbd0
    int encounter_flags;                     // +0xa8  0x51cc80
    int background_presentation;             // +0xac  0x51cc80
    char padb0[0xb4 - 0xb0];
    int fb4;                                 // +0xb4  0x51cc80, 0x51dbd0 (r_int)
    tstring nameb8;                          // +0xb8  0x51cfa0
    int mode;                                // +0xc8  0x51cc80
    SpriteTable events;                      // +0xcc  0x51cfa0 (ctor 0x51d220), 0x51dbd0 (read)
    PartyMgr parties;                        // +0xf8  0x51cfa0 (ctor 0x51d250), 0x51eaa0
    Light* wind;                             // +0x139dc  0x520d60 (the map's sun light; game_tick.h's ColorTarget)
    int f139e0;                              // +0x139e0
    int f139e4;                              // +0x139e4  0x51cc80
    TriggerVec triggers;                     // +0x139e8  0x51cfa0, 0x51dbd0
    SimpleTriggerVec simple;                        // +0x139f4  0x51cfa0, 0x51dbd0
    Troop* troops;                           // +0x13a00  0x502320, 0x51cfa0, 0x51e4c0
    int ntroops;                             // +0x13a04  0x51e4c0 (count limit)
    int f13a08;                              // +0x13a08  0x51cc80
    int attach_party;                        // +0x13a0c  0x51cc80 (-100000), 0x51de70 (r_int)
    int attach_scene;                        // +0x13a10  0x51cc80 (-100000), 0x51de70
    int attach_flip;                         // +0x13a14  0x51cc80
    char f13a18;                             // +0x13a18  0x51cc80
    char pad13a19[0x13a20 - 0x13a19];
    int f13a20;                              // +0x13a20  0x51cc80 (20), 0x51de70
    int f13a24;                              // +0x13a24  0x51cc80
    int f13a28;                              // +0x13a28  0x51cc80
    int f13a2c;                              // +0x13a2c  0x51cc80
    char pad13a30[0x13a34 - 0x13a30];
    int f13a34;                              // +0x13a34  0x51cc80
    float merchandise_modifier_quality;      // +0x13a38  0x51cc80 (1.0)
    char start_map_conversation;             // +0x13a3c  0x51cc80
    char pad13a3d[0x13a40 - 0x13a3d];
    int map_conversation_mode;               // +0x13a40  0x51cc80 (-1)
    int map_conversation_troop;              // +0x13a44  0x51cc80 (-1)
    Faction* factions;                       // +0x13a48  0x51cfa0 (zero), 0x51e2b0
    int n_factions;                          // +0x13a4c  0x51e2b0
    struct SceneFile* scenes;                // +0x13a50  0x51cfa0, 0x51e240 (terrain_b/scene_file.h: the scene records, slots read by load_parties)
    int num_scenes;                          // +0x13a54  0x51e240
    QuestRec* quests;                        // +0x13a58  0x51cfa0, 0x51e100
    int n_quests;                            // +0x13a5c  0x51e100
    std::vector<TrackRec> tracks;            // +0x13a60  0x51cfa0 (ctor), 0x51e6c0
    tstring t13a70;                          // +0x13a70  0x51cfa0, 0x51de70 (r_tstr)
    GameTimer timer13a80;                    // +0x13a80  0x51cfa0 (clock 1); +0x13a88 read by 0x51de70 (r_i64)
    GameTimer timer13a90;                    // +0x13a90  0x51cfa0 (clock 1); +0x13a98 reloaded by 0x51cc80
    char f13aa0;                             // +0x13aa0  0x51cc80, 0x51de70 (r_bool)
    char pad13aa1[0x13aa4 - 0x13aa1];
    int f13aa4;                              // +0x13aa4  0x51cc80
    tstring t13aa8;                          // +0x13aa8  0x51cfa0
    tstring t13ab8;                          // +0x13ab8  0x51cfa0
    char f13ac8;                             // +0x13ac8  0x51cc80
    char pad13ac9[0x13acc - 0x13ac9];
    float f13acc;                            // +0x13acc  0x51cc80 (1.0)
    float rest_period;                       // +0x13ad0  0x51cc80 (-10.0), 0x51de70 (r_float)
    int rest_attackable;                     // +0x13ad4  0x51cc80, 0x51de70
    int rest_interactive;                    // +0x13ad8  0x51cc80, 0x51de70
    int rest_time_speed;                     // +0x13adc  0x51cc80, 0x51de70
    int range_lo;                            // +0x13ae0  0x51cc80 (0), 0x51de70
    int range_hi;                            // +0x13ae4  0x51cc80 (100), 0x51de70
    int f13ae8;                              // +0x13ae8  0x51cc80, 0x51de70
    int current_menu;                        // +0x13aec  0x51cc80, 0x51de70
    int screen_troop;                        // +0x13af0  0x51cc80 (-1)
    int jump_scene;                          // +0x13af4  0x51cc80 (-1), 0x51de70
    int jump_entry;                          // +0x13af8  0x51cc80, 0x51de70
    int jump_mission;                        // +0x13afc  0x51cc80, 0x51de70
    int screen_party;                        // +0x13b00  0x51cc80
    int presentation;                        // +0x13b04  0x51cc80 (-1)
    int notes_type;                          // +0x13b08  0x51cc80 (-1)
    int notes_object;                        // +0x13b0c  0x51cc80 (-1)
    char pad13b10[0x13b18 - 0x13b10];
    GameTimer clock;                         // +0x13b18  0x51cfa0 (clock 1); +0x13b20 read by 0x51de70 (r_i64)
    int current_hours;                       // +0x13b28  0x51de70 (r_int)
    int string_count;                        // +0x13b2c  0x51de70
    int f13b30;                              // +0x13b30  0x51de70
    int f13b34;                              // +0x13b34  0x51de70
    int f13b38;                              // +0x13b38  0x51de70
    float cloud_amount;                      // +0x13b3c  0x51cc80 (0.5), 0x51de70 (r_float)
    float haze_amount;                       // +0x13b40  0x51cc80 (0.5), 0x51de70
    float avg_value;                         // +0x13b44  0x51cc80 (100.0), 0x51de70
    float avg_weight;                        // +0x13b48  0x51cc80 (0), 0x51de70
    float avg_prev_time;                     // +0x13b4c  0x51cc80 (g_ticks[0] * 1e-5)
    tstring message_log;                     // +0x13b50  0x51cfa0, 0x51de70 ("-1" becomes "")
    int f13b60;                              // +0x13b60  0x51cc80, 0x51de70
    Rec10 recs[4];                           // +0x13b64  0x51cfa0 (ctor 0x4638b0 x4)
    char pad13ba4[0x13bcc - 0x13ba4];
    tstring labels[0x32];                    // +0x13bcc  0x51cfa0
    char pad13eec[0x14058 - 0x13eec];
    int f14058;                              // +0x14058  0x51cc80
    char stack_rows[0x1a05c - 0x1405c];      // +0x1405c  0x51eaa0 (not in this module): 0x80 bytes per entry
    char scene_open;                         // +0x1a05c  0x51cfa0 (0), 0x51c720 step 1 (1)
    char pad1a05d[0x1a060 - 0x1a05d];
    ScriptDebugger debugger;                 // +0x1a060  0x51cc80, 0x51cfa0 (enabled, shown cleared; the two strings constructed)
    int map_editor_on;                       // +0x1a090  0x51cc80, 0x51cfa0 (game_data.h: map editor active; restore_defaults 0x4d3a50 clears it)
    int f1a094;                              // +0x1a094  0x51cc80
    int f1a098;                              // +0x1a098  0x51cc80
    char pad1a09c[0x1a0b0 - 0x1a09c];
    int f1a0b0;                              // +0x1a0b0  0x51cc80 (1)
    int f1a0b4;                              // +0x1a0b4  0x51cc80
    int f1a0b8;                              // +0x1a0b8  0x51cc80 (-1)
    int f1a0bc;                              // +0x1a0bc  0x51cc80 (-1)
    int f1a0c0;                              // +0x1a0c0  0x51cc80 (-1)
    int f1a0c4;                              // +0x1a0c4  0x51cc80 (-1)
    char f1a0c8;                             // +0x1a0c8  0x51cc80
    char pad1a0c9[0x1a0cc - 0x1a0c9];
    int f1a0cc;                              // +0x1a0cc  0x51cc80
    int f1a0d0;                              // +0x1a0d0  0x51cc80
    float f1a0d4;                            // +0x1a0d4  0x51cc80 (2.0)
    float f1a0d8;                            // +0x1a0d8  0x51cc80 (2.0)
    float f1a0dc;                            // +0x1a0dc  0x51cc80 (2.0)
    float f1a0e0;                            // +0x1a0e0  0x51cc80 (1.0)
    float f1a0e4;                            // +0x1a0e4  0x51cc80 (1.0)
    float f1a0e8;                            // +0x1a0e8  0x51cc80 (1.0)
    float f1a0ec;                            // +0x1a0ec  0x51cc80 (1.0)
    float f1a0f0;                            // +0x1a0f0  0x51cc80 (1.0)
    float f1a0f4;                            // +0x1a0f4  0x51cc80 (1.0)
    float f1a0f8;                            // +0x1a0f8  0x51cc80 (1.0)
    float f1a0fc;                            // +0x1a0fc  0x51cc80 (1.0)
    float f1a100;                            // +0x1a100  0x51cc80 (1.0)
    int f1a104;                              // +0x1a104  0x51cc80 (copied from the uninitialised fourth element of a local vector: the original assigns a vector4 whose w was never set)
    int f1a108;                              // +0x1a108  0x51cc80
    char pad1a10c[0x1a150 - 0x1a10c];
    GameTimer timer1a150;                    // +0x1a150  0x51cfa0 (clock 0)
    int f1a160;                              // +0x1a160  0x51cc80, 0x51cfa0
    char pad1a164[0x1a1b4 - 0x1a164];
    int f1a1b4;                              // +0x1a1b4  0x51cc80
    std::vector<int> sel;                    // +0x1a1b8  the map editor's selected terrains (GameData::editor.sel): 0x51cc80 frees and clears it
    int f1a1c8;                              // +0x1a1c8  0x51cc80
    int map_editor_dirty;                    // +0x1a1cc  0x51cc80 (restore_defaults 0x4d3a50: set = ask ui_question_save_changes)
    int f1a1d0;                              // +0x1a1d0  0x51cc80
    int f1a1d4;                              // +0x1a1d4  0x51cc80
    int f1a1d8;                              // +0x1a1d8  0x51cc80, 0x51de70 (r_int)
    int f1a1dc;                              // +0x1a1dc  0x51cc80, 0x51de70 (r_int)
    int f1a1e0;                              // +0x1a1e0  0x51cc80, 0x51de70 (r_int)
    int f1a1e4;                              // +0x1a1e4  0x51cc80, 0x51de70 (r_int)
    int f1a1e8;                              // +0x1a1e8  0x51cc80, 0x51de70 (r_int)
    int f1a1ec;                              // +0x1a1ec  0x51cc80, 0x51de70 (r_int)
    EventContext event_ctx;                  // +0x1a1f0  0x51cc80 (prop = -1), 0x502b80 (the party window's item mesh)
    int question_answer;                     // +0x1a208  0x51cc80 (-1)
    int f1a20c;                              // +0x1a20c  0x51cc80
    tstring background_mesh;                 // +0x1a210  0x51cfa0
    int menu_tableau;                        // +0x1a220  0x51cc80 (-1)
    char pad1a224[0x1a238 - 0x1a224];
    int talk_relation_bar;                   // +0x1a238  0x51cc80
    int talk_info_shown;                     // +0x1a23c  0x51cc80
    tstring talk_lines[5];                   // +0x1a240  0x51cc80 (cleared), 0x51cfa0
    char talk_line_set[5];                   // +0x1a290  0x51cc80 (zeroed beside each name)
    char pad1a295[0x1a298 - 0x1a295];
    GameTimer timer1a298;                    // +0x1a298  0x51cfa0 (clock 1)
    rgl::entity* compass;                    // +0x1a2a8  0x520d60
    char pad1a2ac[0x1a2b0 - 0x1a2ac];

    Game();                                 // 0x51cfa0 (the member constructors run in offset order)
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
    const char* module_path(const char* sub);      // 0x4639d0 (ret 4)
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


// ---- layout checks (a wrong pad above is a compile error here) ----
MB_LAYOUT32_ASSERT(chk_game_kind, offsetof(Game, kind) == 0x0);
MB_LAYOUT32_ASSERT(chk_game_f4, offsetof(Game, f4) == 0x4);
MB_LAYOUT32_ASSERT(chk_game_title, offsetof(Game, title) == 0x8);
MB_LAYOUT32_ASSERT(chk_game_flag18, offsetof(Game, flag18) == 0x18);
MB_LAYOUT32_ASSERT(chk_game_module, offsetof(Game, module) == 0x1c);
MB_LAYOUT32_ASSERT(chk_game_map_track, offsetof(Game, map_track) == 0x2c);
MB_LAYOUT32_ASSERT(chk_game_ents, offsetof(Game, ents) == 0x34);
MB_LAYOUT32_ASSERT(chk_game_map_obj, offsetof(Game, map_obj) == 0x44);
MB_LAYOUT32_ASSERT(chk_game_fog3, offsetof(Game, fog3) == 0x68);
MB_LAYOUT32_ASSERT(chk_game_main_party, offsetof(Game, main_party) == 0x78);
MB_LAYOUT32_ASSERT(chk_game_f7c, offsetof(Game, f7c) == 0x7c);
MB_LAYOUT32_ASSERT(chk_game_f80, offsetof(Game, f80) == 0x80);
MB_LAYOUT32_ASSERT(chk_game_sel_party, offsetof(Game, sel_party) == 0x84);
MB_LAYOUT32_ASSERT(chk_game_map_live, offsetof(Game, map_live) == 0x88);
MB_LAYOUT32_ASSERT(chk_game_player, offsetof(Game, player) == 0x8c);
MB_LAYOUT32_ASSERT(chk_game_hero_troop, offsetof(Game, hero_troop) == 0x90);
MB_LAYOUT32_ASSERT(chk_game_f94, offsetof(Game, f94) == 0x94);
MB_LAYOUT32_ASSERT(chk_game_reveal_all, offsetof(Game, reveal_all) == 0x95);
MB_LAYOUT32_ASSERT(chk_game_cnt_killed, offsetof(Game, cnt_killed) == 0x98);
MB_LAYOUT32_ASSERT(chk_game_cnt_killed_friend, offsetof(Game, cnt_killed_friend) == 0x9c);
MB_LAYOUT32_ASSERT(chk_game_cnt_ko, offsetof(Game, cnt_ko) == 0xa0);
MB_LAYOUT32_ASSERT(chk_game_cnt_ko_friend, offsetof(Game, cnt_ko_friend) == 0xa4);
MB_LAYOUT32_ASSERT(chk_game_encounter_flags, offsetof(Game, encounter_flags) == 0xa8);
MB_LAYOUT32_ASSERT(chk_game_background_presentation, offsetof(Game, background_presentation) == 0xac);
MB_LAYOUT32_ASSERT(chk_game_fb4, offsetof(Game, fb4) == 0xb4);
MB_LAYOUT32_ASSERT(chk_game_nameb8, offsetof(Game, nameb8) == 0xb8);
MB_LAYOUT32_ASSERT(chk_game_mode, offsetof(Game, mode) == 0xc8);
MB_LAYOUT32_ASSERT(chk_game_events, offsetof(Game, events) == 0xcc);
MB_LAYOUT32_ASSERT(chk_game_parties, offsetof(Game, parties) == 0xf8);
MB_LAYOUT32_ASSERT(chk_game_wind, offsetof(Game, wind) == 0x139dc);
MB_LAYOUT32_ASSERT(chk_game_compass, offsetof(Game, compass) == 0x1a2a8);
MB_LAYOUT32_ASSERT(chk_game_f139e4, offsetof(Game, f139e4) == 0x139e4);
MB_LAYOUT32_ASSERT(chk_game_triggers, offsetof(Game, triggers) == 0x139e8);
MB_LAYOUT32_ASSERT(chk_game_simple, offsetof(Game, simple) == 0x139f4);
MB_LAYOUT32_ASSERT(chk_game_troops, offsetof(Game, troops) == 0x13a00);
MB_LAYOUT32_ASSERT(chk_game_ntroops, offsetof(Game, ntroops) == 0x13a04);
MB_LAYOUT32_ASSERT(chk_game_f13a08, offsetof(Game, f13a08) == 0x13a08);
MB_LAYOUT32_ASSERT(chk_game_attach_party, offsetof(Game, attach_party) == 0x13a0c);
MB_LAYOUT32_ASSERT(chk_game_attach_scene, offsetof(Game, attach_scene) == 0x13a10);
MB_LAYOUT32_ASSERT(chk_game_attach_flip, offsetof(Game, attach_flip) == 0x13a14);
MB_LAYOUT32_ASSERT(chk_game_f13a18, offsetof(Game, f13a18) == 0x13a18);
MB_LAYOUT32_ASSERT(chk_game_f13a20, offsetof(Game, f13a20) == 0x13a20);
MB_LAYOUT32_ASSERT(chk_game_f13a24, offsetof(Game, f13a24) == 0x13a24);
MB_LAYOUT32_ASSERT(chk_game_f13a28, offsetof(Game, f13a28) == 0x13a28);
MB_LAYOUT32_ASSERT(chk_game_f13a2c, offsetof(Game, f13a2c) == 0x13a2c);
MB_LAYOUT32_ASSERT(chk_game_f13a34, offsetof(Game, f13a34) == 0x13a34);
MB_LAYOUT32_ASSERT(chk_game_merchandise_modifier_quality, offsetof(Game, merchandise_modifier_quality) == 0x13a38);
MB_LAYOUT32_ASSERT(chk_game_start_map_conversation, offsetof(Game, start_map_conversation) == 0x13a3c);
MB_LAYOUT32_ASSERT(chk_game_map_conversation_mode, offsetof(Game, map_conversation_mode) == 0x13a40);
MB_LAYOUT32_ASSERT(chk_game_map_conversation_troop, offsetof(Game, map_conversation_troop) == 0x13a44);
MB_LAYOUT32_ASSERT(chk_game_factions, offsetof(Game, factions) == 0x13a48);
MB_LAYOUT32_ASSERT(chk_game_n_factions, offsetof(Game, n_factions) == 0x13a4c);
MB_LAYOUT32_ASSERT(chk_game_scenes, offsetof(Game, scenes) == 0x13a50);
MB_LAYOUT32_ASSERT(chk_game_num_scenes, offsetof(Game, num_scenes) == 0x13a54);
MB_LAYOUT32_ASSERT(chk_game_quests, offsetof(Game, quests) == 0x13a58);
MB_LAYOUT32_ASSERT(chk_game_n_quests, offsetof(Game, n_quests) == 0x13a5c);
MB_LAYOUT32_ASSERT(chk_game_tracks, offsetof(Game, tracks) == 0x13a60);
MB_LAYOUT32_ASSERT(chk_game_t13a70, offsetof(Game, t13a70) == 0x13a70);
MB_LAYOUT32_ASSERT(chk_game_timer13a80, offsetof(Game, timer13a80) == 0x13a80);
MB_LAYOUT32_ASSERT(chk_game_timer13a90, offsetof(Game, timer13a90) == 0x13a90);
MB_LAYOUT32_ASSERT(chk_game_f13aa0, offsetof(Game, f13aa0) == 0x13aa0);
MB_LAYOUT32_ASSERT(chk_game_f13aa4, offsetof(Game, f13aa4) == 0x13aa4);
MB_LAYOUT32_ASSERT(chk_game_t13aa8, offsetof(Game, t13aa8) == 0x13aa8);
MB_LAYOUT32_ASSERT(chk_game_t13ab8, offsetof(Game, t13ab8) == 0x13ab8);
MB_LAYOUT32_ASSERT(chk_game_f13ac8, offsetof(Game, f13ac8) == 0x13ac8);
MB_LAYOUT32_ASSERT(chk_game_f13acc, offsetof(Game, f13acc) == 0x13acc);
MB_LAYOUT32_ASSERT(chk_game_rest_period, offsetof(Game, rest_period) == 0x13ad0);
MB_LAYOUT32_ASSERT(chk_game_rest_attackable, offsetof(Game, rest_attackable) == 0x13ad4);
MB_LAYOUT32_ASSERT(chk_game_rest_interactive, offsetof(Game, rest_interactive) == 0x13ad8);
MB_LAYOUT32_ASSERT(chk_game_rest_time_speed, offsetof(Game, rest_time_speed) == 0x13adc);
MB_LAYOUT32_ASSERT(chk_game_range_lo, offsetof(Game, range_lo) == 0x13ae0);
MB_LAYOUT32_ASSERT(chk_game_range_hi, offsetof(Game, range_hi) == 0x13ae4);
MB_LAYOUT32_ASSERT(chk_game_f13ae8, offsetof(Game, f13ae8) == 0x13ae8);
MB_LAYOUT32_ASSERT(chk_game_current_menu, offsetof(Game, current_menu) == 0x13aec);
MB_LAYOUT32_ASSERT(chk_game_screen_troop, offsetof(Game, screen_troop) == 0x13af0);
MB_LAYOUT32_ASSERT(chk_game_jump_scene, offsetof(Game, jump_scene) == 0x13af4);
MB_LAYOUT32_ASSERT(chk_game_jump_entry, offsetof(Game, jump_entry) == 0x13af8);
MB_LAYOUT32_ASSERT(chk_game_jump_mission, offsetof(Game, jump_mission) == 0x13afc);
MB_LAYOUT32_ASSERT(chk_game_screen_party, offsetof(Game, screen_party) == 0x13b00);
MB_LAYOUT32_ASSERT(chk_game_presentation, offsetof(Game, presentation) == 0x13b04);
MB_LAYOUT32_ASSERT(chk_game_notes_type, offsetof(Game, notes_type) == 0x13b08);
MB_LAYOUT32_ASSERT(chk_game_notes_object, offsetof(Game, notes_object) == 0x13b0c);
MB_LAYOUT32_ASSERT(chk_game_clock, offsetof(Game, clock) == 0x13b18);
MB_LAYOUT32_ASSERT(chk_game_current_hours, offsetof(Game, current_hours) == 0x13b28);
MB_LAYOUT32_ASSERT(chk_game_string_count, offsetof(Game, string_count) == 0x13b2c);
MB_LAYOUT32_ASSERT(chk_game_f13b30, offsetof(Game, f13b30) == 0x13b30);
MB_LAYOUT32_ASSERT(chk_game_f13b34, offsetof(Game, f13b34) == 0x13b34);
MB_LAYOUT32_ASSERT(chk_game_f13b38, offsetof(Game, f13b38) == 0x13b38);
MB_LAYOUT32_ASSERT(chk_game_cloud_amount, offsetof(Game, cloud_amount) == 0x13b3c);
MB_LAYOUT32_ASSERT(chk_game_haze_amount, offsetof(Game, haze_amount) == 0x13b40);
MB_LAYOUT32_ASSERT(chk_game_avg_value, offsetof(Game, avg_value) == 0x13b44);
MB_LAYOUT32_ASSERT(chk_game_avg_weight, offsetof(Game, avg_weight) == 0x13b48);
MB_LAYOUT32_ASSERT(chk_game_avg_prev_time, offsetof(Game, avg_prev_time) == 0x13b4c);
MB_LAYOUT32_ASSERT(chk_game_message_log, offsetof(Game, message_log) == 0x13b50);
MB_LAYOUT32_ASSERT(chk_game_f13b60, offsetof(Game, f13b60) == 0x13b60);
MB_LAYOUT32_ASSERT(chk_game_recs, offsetof(Game, recs) == 0x13b64);
MB_LAYOUT32_ASSERT(chk_game_labels, offsetof(Game, labels) == 0x13bcc);
MB_LAYOUT32_ASSERT(chk_game_f14058, offsetof(Game, f14058) == 0x14058);
MB_LAYOUT32_ASSERT(chk_game_stack_rows, offsetof(Game, stack_rows) == 0x1405c);
MB_LAYOUT32_ASSERT(chk_game_scene_open, offsetof(Game, scene_open) == 0x1a05c);
MB_LAYOUT32_ASSERT(chk_game_f1a090, offsetof(Game, map_editor_on) == 0x1a090);
MB_LAYOUT32_ASSERT(chk_game_f1a094, offsetof(Game, f1a094) == 0x1a094);
MB_LAYOUT32_ASSERT(chk_game_f1a098, offsetof(Game, f1a098) == 0x1a098);
MB_LAYOUT32_ASSERT(chk_game_f1a0b0, offsetof(Game, f1a0b0) == 0x1a0b0);
MB_LAYOUT32_ASSERT(chk_game_f1a0b4, offsetof(Game, f1a0b4) == 0x1a0b4);
MB_LAYOUT32_ASSERT(chk_game_f1a0b8, offsetof(Game, f1a0b8) == 0x1a0b8);
MB_LAYOUT32_ASSERT(chk_game_f1a0bc, offsetof(Game, f1a0bc) == 0x1a0bc);
MB_LAYOUT32_ASSERT(chk_game_f1a0c0, offsetof(Game, f1a0c0) == 0x1a0c0);
MB_LAYOUT32_ASSERT(chk_game_f1a0c4, offsetof(Game, f1a0c4) == 0x1a0c4);
MB_LAYOUT32_ASSERT(chk_game_f1a0c8, offsetof(Game, f1a0c8) == 0x1a0c8);
MB_LAYOUT32_ASSERT(chk_game_f1a0cc, offsetof(Game, f1a0cc) == 0x1a0cc);
MB_LAYOUT32_ASSERT(chk_game_f1a0d0, offsetof(Game, f1a0d0) == 0x1a0d0);
MB_LAYOUT32_ASSERT(chk_game_f1a0d4, offsetof(Game, f1a0d4) == 0x1a0d4);
MB_LAYOUT32_ASSERT(chk_game_f1a0d8, offsetof(Game, f1a0d8) == 0x1a0d8);
MB_LAYOUT32_ASSERT(chk_game_f1a0dc, offsetof(Game, f1a0dc) == 0x1a0dc);
MB_LAYOUT32_ASSERT(chk_game_f1a0e0, offsetof(Game, f1a0e0) == 0x1a0e0);
MB_LAYOUT32_ASSERT(chk_game_f1a0e4, offsetof(Game, f1a0e4) == 0x1a0e4);
MB_LAYOUT32_ASSERT(chk_game_f1a0e8, offsetof(Game, f1a0e8) == 0x1a0e8);
MB_LAYOUT32_ASSERT(chk_game_f1a0ec, offsetof(Game, f1a0ec) == 0x1a0ec);
MB_LAYOUT32_ASSERT(chk_game_f1a0f0, offsetof(Game, f1a0f0) == 0x1a0f0);
MB_LAYOUT32_ASSERT(chk_game_f1a0f4, offsetof(Game, f1a0f4) == 0x1a0f4);
MB_LAYOUT32_ASSERT(chk_game_f1a0f8, offsetof(Game, f1a0f8) == 0x1a0f8);
MB_LAYOUT32_ASSERT(chk_game_f1a0fc, offsetof(Game, f1a0fc) == 0x1a0fc);
MB_LAYOUT32_ASSERT(chk_game_f1a100, offsetof(Game, f1a100) == 0x1a100);
MB_LAYOUT32_ASSERT(chk_game_f1a104, offsetof(Game, f1a104) == 0x1a104);
MB_LAYOUT32_ASSERT(chk_game_f1a108, offsetof(Game, f1a108) == 0x1a108);
MB_LAYOUT32_ASSERT(chk_game_timer1a150, offsetof(Game, timer1a150) == 0x1a150);
MB_LAYOUT32_ASSERT(chk_game_f1a160, offsetof(Game, f1a160) == 0x1a160);
MB_LAYOUT32_ASSERT(chk_game_f1a1b4, offsetof(Game, f1a1b4) == 0x1a1b4);
MB_LAYOUT32_ASSERT(chk_game_f1a1c8, offsetof(Game, f1a1c8) == 0x1a1c8);
MB_LAYOUT32_ASSERT(chk_game_f1a1cc, offsetof(Game, map_editor_dirty) == 0x1a1cc);
MB_LAYOUT32_ASSERT(chk_game_f1a1d0, offsetof(Game, f1a1d0) == 0x1a1d0);
MB_LAYOUT32_ASSERT(chk_game_f1a1d4, offsetof(Game, f1a1d4) == 0x1a1d4);
MB_LAYOUT32_ASSERT(chk_game_debugger, offsetof(Game, debugger) == 0x1a060);
MB_LAYOUT32_ASSERT(chk_game_sel, offsetof(Game, sel) == 0x1a1b8);
MB_LAYOUT32_ASSERT(chk_game_event_ctx, offsetof(Game, event_ctx) == 0x1a1f0);
MB_LAYOUT32_ASSERT(chk_game_f1a1d8, offsetof(Game, f1a1d8) == 0x1a1d8);
MB_LAYOUT32_ASSERT(chk_game_f1a1dc, offsetof(Game, f1a1dc) == 0x1a1dc);
MB_LAYOUT32_ASSERT(chk_game_f1a1e0, offsetof(Game, f1a1e0) == 0x1a1e0);
MB_LAYOUT32_ASSERT(chk_game_f1a1e4, offsetof(Game, f1a1e4) == 0x1a1e4);
MB_LAYOUT32_ASSERT(chk_game_f1a1e8, offsetof(Game, f1a1e8) == 0x1a1e8);
MB_LAYOUT32_ASSERT(chk_game_f1a1ec, offsetof(Game, f1a1ec) == 0x1a1ec);
MB_LAYOUT32_ASSERT(chk_game_question_answer, offsetof(Game, question_answer) == 0x1a208);
MB_LAYOUT32_ASSERT(chk_game_f1a20c, offsetof(Game, f1a20c) == 0x1a20c);
MB_LAYOUT32_ASSERT(chk_game_background_mesh, offsetof(Game, background_mesh) == 0x1a210);
MB_LAYOUT32_ASSERT(chk_game_menu_tableau, offsetof(Game, menu_tableau) == 0x1a220);
MB_LAYOUT32_ASSERT(chk_game_talk_relation_bar, offsetof(Game, talk_relation_bar) == 0x1a238);
MB_LAYOUT32_ASSERT(chk_game_talk_info_shown, offsetof(Game, talk_info_shown) == 0x1a23c);
MB_LAYOUT32_ASSERT(chk_game_talk_lines, offsetof(Game, talk_lines) == 0x1a240);
MB_LAYOUT32_ASSERT(chk_game_talk_line_set, offsetof(Game, talk_line_set) == 0x1a290);
MB_LAYOUT32_ASSERT(chk_game_timer1a298, offsetof(Game, timer1a298) == 0x1a298);
MB_LAYOUT32_ASSERT(chk_game_size, sizeof(Game) == 0x1a2b0);

#endif // GAME_PARTY_GAME_H
