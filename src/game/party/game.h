// game.h: the game-state object (global g_game, 0x5e2930, 0x1a2b0 bytes) and the records its constructor, reset
// and save-game loaders (game.cpp, 0x51c5a0..0x51ea?0) touch. Only the fields those functions and the party window
// (party_window.cpp) read or write are named; the gaps are padding. Every field cites a function that proves it
// (the offset table is checked at the end of the header: a wrong pad is a compile error).
//
// The same object is `World` in src/game/ui/world.h (dialog_manager.h and terrain_b/game_data.h call it GameData):
// those headers declare other fields of it and cannot be included together with this one (see MIGRATION.md).
// Names are provisional. The save-game stream is `stream_base` of src/rgl/stream.h: the slices called its
// slots r_int (+0x38 = r_float), r_i64b (+0x3c = r_bool), r_i64 (+0x44 = r_u64), r_int1 (+0x48 = r_i64),
// r_raw (+0x4c = r_int) and r_struct (+0x54 = r_tstr); the field types below follow the slot that fills them.
#ifndef GAME_PARTY_GAME_H
#define GAME_PARTY_GAME_H
#include <stddef.h>
#include <new>
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_party_template_count
#include "../../include/dialog_rec.h"      // DialogBlock (TriggerRec)
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../ui/ui_common.h"
#include "../ui/troop_inv.h"

// ---- globals -----------------------------------------------------------------------------------------
// the second element of the clock table g_ticks (ui_common.h, 0x5e2948) as two dwords: the original reads and
// writes the halves separately (0x51cfa0 copies lo then hi; 0x51dbd0 stores them from the loaded value)
#include "../../include/fx.h"            // g_ticks 0x5e2948
extern int g_tick1_hi;                      // 0x5e2954
extern float g_tick_scale;                  // 0x54a6d0 1e-5f (0x51cc80: g_ticks[0] * scale)
extern char g_new_game_done;                // 0xa73090 (0x51d2b0 returns at once when set; 0x51c720 step 6 sets it)

inline tstring& troop_name(Troop* t) { return *(tstring*)t; }      // the name is the first member (0x51c720)
inline rgl::string* rs(tstring* t) { return (rgl::string*)t; }     // the stream's tstring slot takes rgl::string*

// ---- small members with inline constructors (their store order is the constructor's) -----------------------
struct Zero3 {                              // +0x38  0x51cfa0 (three zero stores)
    int a, b, c;
    Zero3() { a = 0; b = 0; c = 0; }
};

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

struct Buf3 {                               // +0x1a1bc: a heap block and two more words (three zero stores)
    void* p;
    int b, c;
    Buf3() { p = 0; b = 0; c = 0; }
};

// 0x10-byte record built by the out-of-line constructor 0x4638b0 (four of them at +0x13b64)
struct Rec10 {
    char b[0x10];
    Rec10();                                // 0x4638b0
};

#include "../../include/i64_vec.h"          // I64Vec: vector of 8-byte integers (std::vector<__int64>); resize is the out-of-line instance 0x493090

#include "../int_vec.h"

// ---- records the loaders read ------------------------------------------------------------------------
// eight names, eight ints and one shared int: read by read_soa (0x51e1f0); 0xa4 bytes
struct SoaBlock {
    int vals[8];                            // +0     0x51e1f0 (r_int at base + i*4)
    tstring names[8];                       // +0x20  0x51e1f0 (r_tstr at base + 0x20 + i*0x10)
    int total;                              // +0xa0  0x51e1f0 (read eight times)
};
stream_base* read_soa(stream_base* s, SoaBlock* b);        // 0x51e1f0 (cdecl, returns the stream)

struct TriggerRec {                         // 0x78 bytes: element of the trigger list (+0x139e8); the module-file trigger record
    float f0, f4, f8;                       // +0x00  (the vtable +0x38 reads of trigger_rec::read 0x4bd830, src/game/config)
    int f0c;                                // +0x0c  0x51ddc0 (r_int), 0x4c1c20 (cleared last)
    DialogBlock cond;                       // +0x10  condition block: items +0x10, count +0x14, done +0x18, text +0x1c (ctor 0x4c1c20)
    DialogBlock cons;                       // +0x2c  consequence block: items +0x2c, count +0x30, done +0x34, text +0x38
    int f48; int pad4c; __int64 f50;        // +0x48  (clock index, 64-bit reading) pair: f50 is read by 0x51ddc0 (r_i64), stamped by 0x4c1d70
    int f58; int pad5c; __int64 f60;        // +0x58
    int f68; int pad6c; __int64 f70;        // +0x68
    TriggerRec();                           // 0x4c1c20 (src/game/triggers/trigger_vec.cpp)
    __declspec(noinline) void read(stream_base* s);        // 0x51ddc0
};
struct TriggerTag {                         // Game +0x139e8: capacity, array, constant 1 (inline ctor stores +4, +8, +0)
    int count;                              // +0   0x51dd60 (the file's count may not exceed it)
    TriggerRec* array;                      // +4
    int one;                                // +8   0x51cfa0
    TriggerTag() { array = 0; one = 1; count = 0; }
    void read(stream_base* s);              // 0x51dd60
};
struct SimpleRec { char pad0[0x28]; __int64 f28; };        // 0x30 bytes, +0x28 read by 0x51de00 (r_i64)
struct SimpleTag {                          // Game +0x139f4 (inline ctor stores +0, +4, +8)
    int count;                              // +0   0x51de00
    SimpleRec* array;                       // +4
    int one;                                // +8   0x51cfa0
    SimpleTag() { count = 0; array = 0; one = 1; }
    void read(stream_base* s);              // 0x51de00
    void arm();                             // 0x51f7c0 (src/game/mission5: re-arms the 0x30-byte delay items, timer at +0x20)
};

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
    float f0, f4, f8;                       // +0 (r_float x3: 0x51e750)
    char padc[4];
    float f10, f14;                         // +0x10, +0x14 (r_float)
    int f18;                                // +0x18 (r_int)
    char pad1c[0x5c - 0x1c];
    __declspec(noinline) void read(stream_base* s);        // 0x51e750
};
struct TrackVec {                           // Game +0x13a60 (std::vector<TrackRec>)
    int pad0;                               // +0
    TrackRec* _Myfirst;                     // +4
    TrackRec* _Mylast;                      // +8
    TrackRec* _Myend;                       // +0xc
    TrackVec() { buy(0); }
    __declspec(noinline) bool buy(unsigned count);            // 0x51d1c0 (_Buy: 0x5c-byte elements, max 0x2c8590b)
    void _Xlen() const;                                       // 0x43e2a0
    int size() const { if (_Myfirst == 0) return 0; return (int)(_Mylast - _Myfirst); }
    __declspec(noinline) void resize(unsigned n, TrackRec val);     // 0x51e7a0 (ret 0x60)
    void insert_n(TrackRec* where, unsigned count, const TrackRec* val);   // 0x4b54a0
    void shift_over(TrackRec** out, TrackRec* first, TrackRec* last);      // 0x518990
};

// The Troop record's loader (0x51e540). Troop (ui/troop_inv.h) declares the inventory and equipment; the other
// fields read here are reached by offset.
struct TroopIO : Troop {
    __declspec(noinline) void read(stream_base* s);        // 0x51e540
};

// ---- the sprite/object pool (Game +0xcc): 0x98-byte elements in pages of 16 ---------------------------------
struct SpriteRec {                          // 0x98 bytes
    char pad0[4];
    int live;                               // +4     0x51e840 / 0x51ea10
    char pad8[4];
    int f0c;                                // +0xc   0x51e840 (r_int)
    int f10;                                // +0x10  0x51e970
    tstring name14;                         // +0x14  0x51e970 (r_tstr)
    float v24[2];                           // +0x24  0x51e970 (r_float x2)
    float v2c[2];                           // +0x2c
    float v34[2];                           // +0x34
    char pad3c[0x54 - 0x3c];
    int f54, f58;                           // +0x54  0x51e970 (r_int)
    char pad5c[0x68 - 0x5c];
    __int64 f68;                            // +0x68  0x51e970 (r_i64)
    float f70;                              // +0x70  0x51e970 (r_float)
    char pad74[0x98 - 0x74];
    __declspec(noinline) void read(stream_base* s, int arg);   // 0x51e970 (ret 8)
};
struct PtrVec {                             // std::vector<char*> (the page list)
    int pad0;
    char** _Myfirst;                        // +4
    char** _Mylast;                         // +8
    char** _Myend;                          // +0xc
    PtrVec() { _Buy(0); }
    bool _Buy(unsigned n);                   // 0x43e760 (_Buy)
};
struct SpriteTable {
    int count;                              // +0     0x51e840
    IntVec free_ids;                        // +4     0x51e840 (pad +4, first +8, last +0xc, end +0x10)
    PtrVec pages;                           // +0x14  0x51d220 (_Buy(0)); its _Myfirst (+0x18) is read by 0x51e840 / 0x51ea10
    int f24, f28;                           // +0x24  0x51e840 (r_int at +0x24)
    SpriteTable();                          // 0x51d220
    int alloc_id();                         // 0x493700
    __declspec(noinline) void clear();                      // 0x51ea10
    void read(stream_base* s, int arg);                     // 0x51e840 (ret 8)
    SpriteRec* at(int i) { return (SpriteRec*)(pages._Myfirst[(unsigned)i >> 4] + (i & 0xf) * 0x98); }
};

// ---- the scene object (Game +0xf8, up to +0x139e4): only what the constructor 0x51d250 touches ------------------
struct PageVec {                            // +0x138cc (was SceneList; world_data.h SceneList is another object): a vector built by 0x4fd490(0); its first pointer (+0x138d0) is read by 0x51eaa0
    char b[0x10];
    PageVec(int n);                         // 0x4fd490 (ret 4)
};
struct Scene {
    int cells[0x2717 * 2];                  // +0       0x51d250 (pairs of -1)
    int f138b8;                             // +0x138b8 0x51d250
    char pad138bc[4];
    Zero3 f138c0;                           // +0x138c0 0x51d250
    PageVec list;                           // +0x138cc
    int f138dc, f138e0;                     // +0x138dc 0x51d250
    char pad138e4[0x138ec - 0x138e4];
    Scene();                                // 0x51d250
    // ---- src/game/mission5/troop_rec.cpp (method declarations only: no layout change) ----
    int index_of(int id);                     // 0x48ad00 (ret 4: id -> record index through the hash at +0)
    void grow();                        // 0x493830 (grows the party table by one page of 16 records)
    __declspec(noinline) void clear();      // 0x51eda0
    __declspec(noinline) void read(stream_base* s, int flag);   // 0x51eb90 (ret 8)
    __declspec(noinline) struct PartyRecIO* get(int id);        // 0x51ee50 (ret 4)
};

// ---- the game object -----------------------------------------------------------------------------------
struct Game {
    int kind;                                // +0x0  0x51cc10 (init), 0x51cc80 (-1)
    int f4;                                  // +0x4  0x51dbd0 (r_int)
    tstring title;                           // +0x8  0x51cfa0, 0x51c720 step 2 (the save's name)
    char flag18;                             // +0x18  0x51cc10, 0x51cc80
    char pad19[0x1c - 0x19];
    tstring name1c;                          // +0x1c  0x51cc10
    int f2c;                                 // +0x2c  0x51cc80
    char pad30[0x38 - 0x30];
    Zero3 f38;                               // +0x38  0x51cfa0 (three zero stores)
    int f44;                                 // +0x44  0x51cc80
    char pad48[0x78 - 0x48];
    int f78;                                 // +0x78  0x51cc10, 0x51cc80 (party id used by the scene loader 0x51eaa0)
    int f7c;                                 // +0x7c  0x51cc80
    int f80;                                 // +0x80  0x51cc10
    int f84;                                 // +0x84  0x51cc10, 0x51cc80
    char f88;                                // +0x88  0x51cc10, 0x51cc80
    char pad89[0x8c - 0x89];
    int cur_troop;                           // +0x8c  0x502320 (index of the current troop), 0x51dbd0
    int hero_troop;                          // +0x90  0x502320 (compared with a troop index), 0x51cc10 (= 1)
    char f94;                                // +0x94  0x51cc80
    char f95;                                // +0x95  0x51cc80
    char pad96[0x98 - 0x96];
    int f98;                                 // +0x98  0x51cc10, 0x51dbd0 (r_int)
    int f9c;                                 // +0x9c  0x51cc10, 0x51dbd0
    int fa0;                                 // +0xa0  0x51cc10, 0x51dbd0
    int fa4;                                 // +0xa4  0x51cc10, 0x51dbd0
    int fa8;                                 // +0xa8  0x51cc80
    int fac;                                 // +0xac  0x51cc80
    char padb0[0xb4 - 0xb0];
    int fb4;                                 // +0xb4  0x51cc80, 0x51dbd0 (r_int)
    tstring nameb8;                          // +0xb8  0x51cfa0
    int fc8;                                 // +0xc8  0x51cc80
    SpriteTable sprites;                     // +0xcc  0x51cfa0 (ctor 0x51d220), 0x51dbd0 (read)
    Scene scene;                             // +0xf8  0x51cfa0 (ctor 0x51d250), 0x51eaa0
    int f139e4;                              // +0x139e4  0x51cc80
    TriggerTag triggers;                     // +0x139e8  0x51cfa0, 0x51dbd0
    SimpleTag simple;                        // +0x139f4  0x51cfa0, 0x51dbd0
    Troop* troops;                           // +0x13a00  0x502320, 0x51cfa0, 0x51e4c0
    int ntroops;                             // +0x13a04  0x51e4c0 (count limit)
    int f13a08;                              // +0x13a08  0x51cc80
    int f13a0c;                              // +0x13a0c  0x51cc80 (-100000), 0x51de70 (r_int)
    int f13a10;                              // +0x13a10  0x51cc80 (-100000), 0x51de70
    int f13a14;                              // +0x13a14  0x51cc80
    char f13a18;                             // +0x13a18  0x51cc80
    char pad13a19[0x13a20 - 0x13a19];
    int f13a20;                              // +0x13a20  0x51cc80 (20), 0x51de70
    int f13a24;                              // +0x13a24  0x51cc80
    int f13a28;                              // +0x13a28  0x51cc80
    int f13a2c;                              // +0x13a2c  0x51cc80
    char pad13a30[0x13a34 - 0x13a30];
    int f13a34;                              // +0x13a34  0x51cc80
    float f13a38;                            // +0x13a38  0x51cc80 (1.0)
    char f13a3c;                             // +0x13a3c  0x51cc80
    char pad13a3d[0x13a40 - 0x13a3d];
    int f13a40;                              // +0x13a40  0x51cc80 (-1)
    int f13a44;                              // +0x13a44  0x51cc80 (-1)
    Faction* factions;                       // +0x13a48  0x51cfa0 (zero), 0x51e2b0
    int nfactions;                           // +0x13a4c  0x51e2b0
    char* parties;                           // +0x13a50  0x51cfa0, 0x51e240 (0x494-byte records, a vector at +0x474)
    int nparties;                            // +0x13a54  0x51e240
    QuestRec* quests;                        // +0x13a58  0x51cfa0, 0x51e100
    int nquests;                             // +0x13a5c  0x51e100
    TrackVec tracks;                         // +0x13a60  0x51cfa0 (ctor), 0x51e6c0
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
    float f13ad0;                            // +0x13ad0  0x51cc80 (-10.0), 0x51de70 (r_float)
    int f13ad4;                              // +0x13ad4  0x51cc80, 0x51de70
    int f13ad8;                              // +0x13ad8  0x51cc80, 0x51de70
    int f13adc;                              // +0x13adc  0x51cc80, 0x51de70
    int f13ae0;                              // +0x13ae0  0x51cc80 (0), 0x51de70
    int f13ae4;                              // +0x13ae4  0x51cc80 (100), 0x51de70
    int f13ae8;                              // +0x13ae8  0x51cc80, 0x51de70
    int f13aec;                              // +0x13aec  0x51cc80, 0x51de70
    int f13af0;                              // +0x13af0  0x51cc80 (-1)
    int f13af4;                              // +0x13af4  0x51cc80 (-1), 0x51de70
    int f13af8;                              // +0x13af8  0x51cc80, 0x51de70
    int f13afc;                              // +0x13afc  0x51cc80, 0x51de70
    int f13b00;                              // +0x13b00  0x51cc80
    int f13b04;                              // +0x13b04  0x51cc80 (-1)
    int f13b08;                              // +0x13b08  0x51cc80 (-1)
    int f13b0c;                              // +0x13b0c  0x51cc80 (-1)
    char pad13b10[0x13b18 - 0x13b10];
    GameTimer timer13b18;                    // +0x13b18  0x51cfa0 (clock 1); +0x13b20 read by 0x51de70 (r_i64)
    int f13b28;                              // +0x13b28  0x51de70 (r_int)
    int f13b2c;                              // +0x13b2c  0x51de70
    int f13b30;                              // +0x13b30  0x51de70
    int f13b34;                              // +0x13b34  0x51de70
    int f13b38;                              // +0x13b38  0x51de70
    float f13b3c;                            // +0x13b3c  0x51cc80 (0.5), 0x51de70 (r_float)
    float f13b40;                            // +0x13b40  0x51cc80 (0.5), 0x51de70
    float f13b44;                            // +0x13b44  0x51cc80 (100.0), 0x51de70
    float f13b48;                            // +0x13b48  0x51cc80 (0), 0x51de70
    float f13b4c;                            // +0x13b4c  0x51cc80 (g_ticks[0] * 1e-5)
    tstring t13b50;                          // +0x13b50  0x51cfa0, 0x51de70 ("-1" becomes "")
    int f13b60;                              // +0x13b60  0x51cc80, 0x51de70
    Rec10 recs[4];                           // +0x13b64  0x51cfa0 (ctor 0x4638b0 x4)
    char pad13ba4[0x13bcc - 0x13ba4];
    tstring names13bcc[0x32];                // +0x13bcc  0x51cfa0
    char pad13eec[0x14058 - 0x13eec];
    int f14058;                              // +0x14058  0x51cc80
    char scene_parties[0x1a05c - 0x1405c];   // +0x1405c  0x51eaa0 (not in this module): 0x80 bytes per entry
    char f1a05c;                             // +0x1a05c  0x51cfa0 (0), 0x51c720 step 1 (1)
    char pad1a05d[0x1a060 - 0x1a05d];
    int f1a060;                              // +0x1a060  0x51cc80, 0x51cfa0
    int f1a064;                              // +0x1a064  0x51cc80, 0x51cfa0
    char pad1a068[0x1a06c - 0x1a068];
    tstring t1a06c;                          // +0x1a06c  0x51cfa0
    char pad1a07c[0x1a080 - 0x1a07c];
    tstring t1a080;                          // +0x1a080  0x51cfa0
    int f1a090;                              // +0x1a090  0x51cc80, 0x51cfa0
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
    char pad1a1b8[0x1a1bc - 0x1a1b8];
    Buf3 buf1a1bc;                           // +0x1a1bc  0x51cc80 (freed and cleared), 0x51cfa0 (three zero stores)
    int f1a1c8;                              // +0x1a1c8  0x51cc80
    int f1a1cc;                              // +0x1a1cc  0x51cc80
    int f1a1d0;                              // +0x1a1d0  0x51cc80
    int f1a1d4;                              // +0x1a1d4  0x51cc80
    int f1a1d8;                              // +0x1a1d8  0x51cc80, 0x51de70 (r_int)
    int f1a1dc;                              // +0x1a1dc  0x51cc80, 0x51de70 (r_int)
    int f1a1e0;                              // +0x1a1e0  0x51cc80, 0x51de70 (r_int)
    int f1a1e4;                              // +0x1a1e4  0x51cc80, 0x51de70 (r_int)
    int f1a1e8;                              // +0x1a1e8  0x51cc80, 0x51de70 (r_int)
    int f1a1ec;                              // +0x1a1ec  0x51cc80, 0x51de70 (r_int)
    int f1a1f0;                              // +0x1a1f0  0x51cc80 (-1)
    int f1a1f4;                              // +0x1a1f4  0x502b80 (spawn state of the party window)
    int f1a1f8;                              // +0x1a1f8  0x502b80
    int f1a1fc;                              // +0x1a1fc  0x502b80
    int f1a200;                              // +0x1a200  0x502b80
    int f1a204;                              // +0x1a204  0x502b80
    int f1a208;                              // +0x1a208  0x51cc80 (-1)
    int f1a20c;                              // +0x1a20c  0x51cc80
    tstring t1a210;                          // +0x1a210  0x51cfa0
    int f1a220;                              // +0x1a220  0x51cc80 (-1)
    char pad1a224[0x1a238 - 0x1a224];
    int f1a238;                              // +0x1a238  0x51cc80
    int f1a23c;                              // +0x1a23c  0x51cc80
    tstring names1a240[5];                   // +0x1a240  0x51cc80 (cleared), 0x51cfa0
    char flags1a290[5];                      // +0x1a290  0x51cc80 (zeroed beside each name)
    char pad1a295[0x1a298 - 0x1a295];
    GameTimer timer1a298;                    // +0x1a298  0x51cfa0 (clock 1)
    char pad1a2a8[0x1a2b0 - 0x1a2a8];

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
    void load_game_data(int extra_triggers);   // 0x5236f0 (ret 4: opens the Data\*.txt tables in turn)
};
extern Game* g_game;                        // 0x5e2930
extern I64Vec g_list_a70480;                // 0xa70480 (read first by 0x51dbd0 after the clock)
extern PartyTemplate* g_party_templates;    // 0x5e29f8 (0xb0-byte records)
extern int g_party_template_count;             // 0x5e29fc
extern char g_troop_extra[];                // 0x8ee3138 (42 entries of 0x44 bytes, up to 0x8ee3c60; the first dword of each is saved)

// ---- layout checks (a wrong pad above is a compile error here) ----
typedef char chk_game_kind[offsetof(Game, kind) == 0x0 ? 1 : -1];
typedef char chk_game_f4[offsetof(Game, f4) == 0x4 ? 1 : -1];
typedef char chk_game_title[offsetof(Game, title) == 0x8 ? 1 : -1];
typedef char chk_game_flag18[offsetof(Game, flag18) == 0x18 ? 1 : -1];
typedef char chk_game_name1c[offsetof(Game, name1c) == 0x1c ? 1 : -1];
typedef char chk_game_f2c[offsetof(Game, f2c) == 0x2c ? 1 : -1];
typedef char chk_game_f38[offsetof(Game, f38) == 0x38 ? 1 : -1];
typedef char chk_game_f44[offsetof(Game, f44) == 0x44 ? 1 : -1];
typedef char chk_game_f78[offsetof(Game, f78) == 0x78 ? 1 : -1];
typedef char chk_game_f7c[offsetof(Game, f7c) == 0x7c ? 1 : -1];
typedef char chk_game_f80[offsetof(Game, f80) == 0x80 ? 1 : -1];
typedef char chk_game_f84[offsetof(Game, f84) == 0x84 ? 1 : -1];
typedef char chk_game_f88[offsetof(Game, f88) == 0x88 ? 1 : -1];
typedef char chk_game_cur_troop[offsetof(Game, cur_troop) == 0x8c ? 1 : -1];
typedef char chk_game_hero_troop[offsetof(Game, hero_troop) == 0x90 ? 1 : -1];
typedef char chk_game_f94[offsetof(Game, f94) == 0x94 ? 1 : -1];
typedef char chk_game_f95[offsetof(Game, f95) == 0x95 ? 1 : -1];
typedef char chk_game_f98[offsetof(Game, f98) == 0x98 ? 1 : -1];
typedef char chk_game_f9c[offsetof(Game, f9c) == 0x9c ? 1 : -1];
typedef char chk_game_fa0[offsetof(Game, fa0) == 0xa0 ? 1 : -1];
typedef char chk_game_fa4[offsetof(Game, fa4) == 0xa4 ? 1 : -1];
typedef char chk_game_fa8[offsetof(Game, fa8) == 0xa8 ? 1 : -1];
typedef char chk_game_fac[offsetof(Game, fac) == 0xac ? 1 : -1];
typedef char chk_game_fb4[offsetof(Game, fb4) == 0xb4 ? 1 : -1];
typedef char chk_game_nameb8[offsetof(Game, nameb8) == 0xb8 ? 1 : -1];
typedef char chk_game_fc8[offsetof(Game, fc8) == 0xc8 ? 1 : -1];
typedef char chk_game_sprites[offsetof(Game, sprites) == 0xcc ? 1 : -1];
typedef char chk_game_scene[offsetof(Game, scene) == 0xf8 ? 1 : -1];
typedef char chk_game_f139e4[offsetof(Game, f139e4) == 0x139e4 ? 1 : -1];
typedef char chk_game_triggers[offsetof(Game, triggers) == 0x139e8 ? 1 : -1];
typedef char chk_game_simple[offsetof(Game, simple) == 0x139f4 ? 1 : -1];
typedef char chk_game_troops[offsetof(Game, troops) == 0x13a00 ? 1 : -1];
typedef char chk_game_ntroops[offsetof(Game, ntroops) == 0x13a04 ? 1 : -1];
typedef char chk_game_f13a08[offsetof(Game, f13a08) == 0x13a08 ? 1 : -1];
typedef char chk_game_f13a0c[offsetof(Game, f13a0c) == 0x13a0c ? 1 : -1];
typedef char chk_game_f13a10[offsetof(Game, f13a10) == 0x13a10 ? 1 : -1];
typedef char chk_game_f13a14[offsetof(Game, f13a14) == 0x13a14 ? 1 : -1];
typedef char chk_game_f13a18[offsetof(Game, f13a18) == 0x13a18 ? 1 : -1];
typedef char chk_game_f13a20[offsetof(Game, f13a20) == 0x13a20 ? 1 : -1];
typedef char chk_game_f13a24[offsetof(Game, f13a24) == 0x13a24 ? 1 : -1];
typedef char chk_game_f13a28[offsetof(Game, f13a28) == 0x13a28 ? 1 : -1];
typedef char chk_game_f13a2c[offsetof(Game, f13a2c) == 0x13a2c ? 1 : -1];
typedef char chk_game_f13a34[offsetof(Game, f13a34) == 0x13a34 ? 1 : -1];
typedef char chk_game_f13a38[offsetof(Game, f13a38) == 0x13a38 ? 1 : -1];
typedef char chk_game_f13a3c[offsetof(Game, f13a3c) == 0x13a3c ? 1 : -1];
typedef char chk_game_f13a40[offsetof(Game, f13a40) == 0x13a40 ? 1 : -1];
typedef char chk_game_f13a44[offsetof(Game, f13a44) == 0x13a44 ? 1 : -1];
typedef char chk_game_factions[offsetof(Game, factions) == 0x13a48 ? 1 : -1];
typedef char chk_game_nfactions[offsetof(Game, nfactions) == 0x13a4c ? 1 : -1];
typedef char chk_game_parties[offsetof(Game, parties) == 0x13a50 ? 1 : -1];
typedef char chk_game_nparties[offsetof(Game, nparties) == 0x13a54 ? 1 : -1];
typedef char chk_game_quests[offsetof(Game, quests) == 0x13a58 ? 1 : -1];
typedef char chk_game_nquests[offsetof(Game, nquests) == 0x13a5c ? 1 : -1];
typedef char chk_game_tracks[offsetof(Game, tracks) == 0x13a60 ? 1 : -1];
typedef char chk_game_t13a70[offsetof(Game, t13a70) == 0x13a70 ? 1 : -1];
typedef char chk_game_timer13a80[offsetof(Game, timer13a80) == 0x13a80 ? 1 : -1];
typedef char chk_game_timer13a90[offsetof(Game, timer13a90) == 0x13a90 ? 1 : -1];
typedef char chk_game_f13aa0[offsetof(Game, f13aa0) == 0x13aa0 ? 1 : -1];
typedef char chk_game_f13aa4[offsetof(Game, f13aa4) == 0x13aa4 ? 1 : -1];
typedef char chk_game_t13aa8[offsetof(Game, t13aa8) == 0x13aa8 ? 1 : -1];
typedef char chk_game_t13ab8[offsetof(Game, t13ab8) == 0x13ab8 ? 1 : -1];
typedef char chk_game_f13ac8[offsetof(Game, f13ac8) == 0x13ac8 ? 1 : -1];
typedef char chk_game_f13acc[offsetof(Game, f13acc) == 0x13acc ? 1 : -1];
typedef char chk_game_f13ad0[offsetof(Game, f13ad0) == 0x13ad0 ? 1 : -1];
typedef char chk_game_f13ad4[offsetof(Game, f13ad4) == 0x13ad4 ? 1 : -1];
typedef char chk_game_f13ad8[offsetof(Game, f13ad8) == 0x13ad8 ? 1 : -1];
typedef char chk_game_f13adc[offsetof(Game, f13adc) == 0x13adc ? 1 : -1];
typedef char chk_game_f13ae0[offsetof(Game, f13ae0) == 0x13ae0 ? 1 : -1];
typedef char chk_game_f13ae4[offsetof(Game, f13ae4) == 0x13ae4 ? 1 : -1];
typedef char chk_game_f13ae8[offsetof(Game, f13ae8) == 0x13ae8 ? 1 : -1];
typedef char chk_game_f13aec[offsetof(Game, f13aec) == 0x13aec ? 1 : -1];
typedef char chk_game_f13af0[offsetof(Game, f13af0) == 0x13af0 ? 1 : -1];
typedef char chk_game_f13af4[offsetof(Game, f13af4) == 0x13af4 ? 1 : -1];
typedef char chk_game_f13af8[offsetof(Game, f13af8) == 0x13af8 ? 1 : -1];
typedef char chk_game_f13afc[offsetof(Game, f13afc) == 0x13afc ? 1 : -1];
typedef char chk_game_f13b00[offsetof(Game, f13b00) == 0x13b00 ? 1 : -1];
typedef char chk_game_f13b04[offsetof(Game, f13b04) == 0x13b04 ? 1 : -1];
typedef char chk_game_f13b08[offsetof(Game, f13b08) == 0x13b08 ? 1 : -1];
typedef char chk_game_f13b0c[offsetof(Game, f13b0c) == 0x13b0c ? 1 : -1];
typedef char chk_game_timer13b18[offsetof(Game, timer13b18) == 0x13b18 ? 1 : -1];
typedef char chk_game_f13b28[offsetof(Game, f13b28) == 0x13b28 ? 1 : -1];
typedef char chk_game_f13b2c[offsetof(Game, f13b2c) == 0x13b2c ? 1 : -1];
typedef char chk_game_f13b30[offsetof(Game, f13b30) == 0x13b30 ? 1 : -1];
typedef char chk_game_f13b34[offsetof(Game, f13b34) == 0x13b34 ? 1 : -1];
typedef char chk_game_f13b38[offsetof(Game, f13b38) == 0x13b38 ? 1 : -1];
typedef char chk_game_f13b3c[offsetof(Game, f13b3c) == 0x13b3c ? 1 : -1];
typedef char chk_game_f13b40[offsetof(Game, f13b40) == 0x13b40 ? 1 : -1];
typedef char chk_game_f13b44[offsetof(Game, f13b44) == 0x13b44 ? 1 : -1];
typedef char chk_game_f13b48[offsetof(Game, f13b48) == 0x13b48 ? 1 : -1];
typedef char chk_game_f13b4c[offsetof(Game, f13b4c) == 0x13b4c ? 1 : -1];
typedef char chk_game_t13b50[offsetof(Game, t13b50) == 0x13b50 ? 1 : -1];
typedef char chk_game_f13b60[offsetof(Game, f13b60) == 0x13b60 ? 1 : -1];
typedef char chk_game_recs[offsetof(Game, recs) == 0x13b64 ? 1 : -1];
typedef char chk_game_names13bcc[offsetof(Game, names13bcc) == 0x13bcc ? 1 : -1];
typedef char chk_game_f14058[offsetof(Game, f14058) == 0x14058 ? 1 : -1];
typedef char chk_game_scene_parties[offsetof(Game, scene_parties) == 0x1405c ? 1 : -1];
typedef char chk_game_f1a05c[offsetof(Game, f1a05c) == 0x1a05c ? 1 : -1];
typedef char chk_game_f1a060[offsetof(Game, f1a060) == 0x1a060 ? 1 : -1];
typedef char chk_game_f1a064[offsetof(Game, f1a064) == 0x1a064 ? 1 : -1];
typedef char chk_game_t1a06c[offsetof(Game, t1a06c) == 0x1a06c ? 1 : -1];
typedef char chk_game_t1a080[offsetof(Game, t1a080) == 0x1a080 ? 1 : -1];
typedef char chk_game_f1a090[offsetof(Game, f1a090) == 0x1a090 ? 1 : -1];
typedef char chk_game_f1a094[offsetof(Game, f1a094) == 0x1a094 ? 1 : -1];
typedef char chk_game_f1a098[offsetof(Game, f1a098) == 0x1a098 ? 1 : -1];
typedef char chk_game_f1a0b0[offsetof(Game, f1a0b0) == 0x1a0b0 ? 1 : -1];
typedef char chk_game_f1a0b4[offsetof(Game, f1a0b4) == 0x1a0b4 ? 1 : -1];
typedef char chk_game_f1a0b8[offsetof(Game, f1a0b8) == 0x1a0b8 ? 1 : -1];
typedef char chk_game_f1a0bc[offsetof(Game, f1a0bc) == 0x1a0bc ? 1 : -1];
typedef char chk_game_f1a0c0[offsetof(Game, f1a0c0) == 0x1a0c0 ? 1 : -1];
typedef char chk_game_f1a0c4[offsetof(Game, f1a0c4) == 0x1a0c4 ? 1 : -1];
typedef char chk_game_f1a0c8[offsetof(Game, f1a0c8) == 0x1a0c8 ? 1 : -1];
typedef char chk_game_f1a0cc[offsetof(Game, f1a0cc) == 0x1a0cc ? 1 : -1];
typedef char chk_game_f1a0d0[offsetof(Game, f1a0d0) == 0x1a0d0 ? 1 : -1];
typedef char chk_game_f1a0d4[offsetof(Game, f1a0d4) == 0x1a0d4 ? 1 : -1];
typedef char chk_game_f1a0d8[offsetof(Game, f1a0d8) == 0x1a0d8 ? 1 : -1];
typedef char chk_game_f1a0dc[offsetof(Game, f1a0dc) == 0x1a0dc ? 1 : -1];
typedef char chk_game_f1a0e0[offsetof(Game, f1a0e0) == 0x1a0e0 ? 1 : -1];
typedef char chk_game_f1a0e4[offsetof(Game, f1a0e4) == 0x1a0e4 ? 1 : -1];
typedef char chk_game_f1a0e8[offsetof(Game, f1a0e8) == 0x1a0e8 ? 1 : -1];
typedef char chk_game_f1a0ec[offsetof(Game, f1a0ec) == 0x1a0ec ? 1 : -1];
typedef char chk_game_f1a0f0[offsetof(Game, f1a0f0) == 0x1a0f0 ? 1 : -1];
typedef char chk_game_f1a0f4[offsetof(Game, f1a0f4) == 0x1a0f4 ? 1 : -1];
typedef char chk_game_f1a0f8[offsetof(Game, f1a0f8) == 0x1a0f8 ? 1 : -1];
typedef char chk_game_f1a0fc[offsetof(Game, f1a0fc) == 0x1a0fc ? 1 : -1];
typedef char chk_game_f1a100[offsetof(Game, f1a100) == 0x1a100 ? 1 : -1];
typedef char chk_game_f1a104[offsetof(Game, f1a104) == 0x1a104 ? 1 : -1];
typedef char chk_game_f1a108[offsetof(Game, f1a108) == 0x1a108 ? 1 : -1];
typedef char chk_game_timer1a150[offsetof(Game, timer1a150) == 0x1a150 ? 1 : -1];
typedef char chk_game_f1a160[offsetof(Game, f1a160) == 0x1a160 ? 1 : -1];
typedef char chk_game_f1a1b4[offsetof(Game, f1a1b4) == 0x1a1b4 ? 1 : -1];
typedef char chk_game_buf1a1bc[offsetof(Game, buf1a1bc) == 0x1a1bc ? 1 : -1];
typedef char chk_game_f1a1c8[offsetof(Game, f1a1c8) == 0x1a1c8 ? 1 : -1];
typedef char chk_game_f1a1cc[offsetof(Game, f1a1cc) == 0x1a1cc ? 1 : -1];
typedef char chk_game_f1a1d0[offsetof(Game, f1a1d0) == 0x1a1d0 ? 1 : -1];
typedef char chk_game_f1a1d4[offsetof(Game, f1a1d4) == 0x1a1d4 ? 1 : -1];
typedef char chk_game_f1a1d8[offsetof(Game, f1a1d8) == 0x1a1d8 ? 1 : -1];
typedef char chk_game_f1a1dc[offsetof(Game, f1a1dc) == 0x1a1dc ? 1 : -1];
typedef char chk_game_f1a1e0[offsetof(Game, f1a1e0) == 0x1a1e0 ? 1 : -1];
typedef char chk_game_f1a1e4[offsetof(Game, f1a1e4) == 0x1a1e4 ? 1 : -1];
typedef char chk_game_f1a1e8[offsetof(Game, f1a1e8) == 0x1a1e8 ? 1 : -1];
typedef char chk_game_f1a1ec[offsetof(Game, f1a1ec) == 0x1a1ec ? 1 : -1];
typedef char chk_game_f1a1f0[offsetof(Game, f1a1f0) == 0x1a1f0 ? 1 : -1];
typedef char chk_game_f1a1f4[offsetof(Game, f1a1f4) == 0x1a1f4 ? 1 : -1];
typedef char chk_game_f1a1f8[offsetof(Game, f1a1f8) == 0x1a1f8 ? 1 : -1];
typedef char chk_game_f1a1fc[offsetof(Game, f1a1fc) == 0x1a1fc ? 1 : -1];
typedef char chk_game_f1a200[offsetof(Game, f1a200) == 0x1a200 ? 1 : -1];
typedef char chk_game_f1a204[offsetof(Game, f1a204) == 0x1a204 ? 1 : -1];
typedef char chk_game_f1a208[offsetof(Game, f1a208) == 0x1a208 ? 1 : -1];
typedef char chk_game_f1a20c[offsetof(Game, f1a20c) == 0x1a20c ? 1 : -1];
typedef char chk_game_t1a210[offsetof(Game, t1a210) == 0x1a210 ? 1 : -1];
typedef char chk_game_f1a220[offsetof(Game, f1a220) == 0x1a220 ? 1 : -1];
typedef char chk_game_f1a238[offsetof(Game, f1a238) == 0x1a238 ? 1 : -1];
typedef char chk_game_f1a23c[offsetof(Game, f1a23c) == 0x1a23c ? 1 : -1];
typedef char chk_game_names1a240[offsetof(Game, names1a240) == 0x1a240 ? 1 : -1];
typedef char chk_game_flags1a290[offsetof(Game, flags1a290) == 0x1a290 ? 1 : -1];
typedef char chk_game_timer1a298[offsetof(Game, timer1a298) == 0x1a298 ? 1 : -1];
typedef char chk_game_size[sizeof(Game) == 0x1a2b0 ? 1 : -1];

#endif // GAME_PARTY_GAME_H
