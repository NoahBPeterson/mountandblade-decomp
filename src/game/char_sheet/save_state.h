#ifndef SAVE_STATE_H
#define SAVE_STATE_H
// Slice s004e8f10 (batch ds1): the game-state serialiser. Game::save_state (0x4e8f10) walks the whole game
// object in a fixed order and hands each block to the binary writer; the rest of the range is the family of
// per-block writers it (and the next slice) uses: the 8-byte element vector writer (0x4e94b0), the
// count+pointer+stride lists (0x4e90d0 / 0x4e9150 / 0x4e9550 / 0x4e9670 / 0x4e96c0 / 0x4e9790), the fixed
// arrays (0x4e9510 / 0x4e95a0 / 0x4e97e0) and the struct-of-arrays helper (0x4e9620).
// Every offset below is read off the disassembly of this range; names are provisional.
// The stream is the binary writer of the save files: slot 0xc writes an int, 0x10 writes a pointer-sized
// int, 0x14 writes a 64-bit value, 0x18 writes an 8-byte element, 0x1c writes a count, 0x20 writes raw bytes,
// 0x28 writes a C string and 0x2c writes a nested structure. Every call returns the stream (eax).
#include <windows.h>
#include <new>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- tstring (see s0042aaf0 / s004e2ea0) ----------------------------------------------------------------
#include "../tstring.h"

// ---- the writer: src/game/out_stream.h ---------------------------------------------------------------------
#include "../out_stream.h"
extern __int64 g_time1;                             // 0x5e2950 (the 64-bit clock counter)

// the record the scalar block writes: src/game/rec5c.h; the 8-byte element vector: src/game/list8.h; the 0x508-byte
// game record (GameRecord in the slices) is the troop record, src/include/troop.h
#include "../rec5c.h"
#include "../list8.h"
#include "../../include/troop.h"
void write_vec8(List8* v, OutStream* out);    // the alias the other slices call

struct RecB0 {                                      // stride 0xb0 (the 0x5e29f8 array)
    char  pad0[0x34];
    int   f34, f38, f3c;
    char  pad40[0xa0 - 0x40];
    List8 f_a0;                                     // +0xa0
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e9510 (ret 4)
};

struct Rec138 {                                     // stride 0x138 (at +0x13a58)
    char  pad0[0x20];
    char  f20[0x10];
    char  f30[0x10];
    char  f40[0x24];
    int   f64;
    char  pad68[0x114 - 0x68];
    int   f114, f118, f11c;
    char  pad120[0x128 - 0x120];
    List8 f128;                                     // +0x128
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e95a0 (ret 4)
};

struct Rec300 {                                     // stride 0x300 (at +0x13a48)
    char  pad0[0x10];
    int   f10;
    char  pad14[0x28 - 0x14];
    int   f28;                                      // +0x28 (count)
    char  f2c[0x230 - 0x2c];                        // +0x2c (n ints)
    int   f230;
    char  pad234[0x24c - 0x234];
    List8 f24c;                                     // +0x24c
    char  f25c[0xa4];                               // +0x25c (the struct-of-arrays base)
    __declspec(noinline) void  write300(OutStream* out, int n);          // 0x4e9710 (ret 8)
};

#include "../../include/rec78.h"

struct Rec78Tag {                                   // count +0x0, array +0x4
    int   count;
    Rec78* array;
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e90d0 (ret 4)
};

struct Rec30Tag {                                   // count +0x0, array +0x4, stride 0x30
    int   count;
    char* array;
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e9150 (ret 4)
};

// ---- the game object (see s00473a60 / s004e85d0) ---------------------------------------------------------

#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)
#pragma pack(push, 4)
struct Table {                                      // the table at Game+0xcc
    void write_table(OutStream* out);                     // 0x4e99d0 (save_records.cpp)
};
struct OutRecList {
    void write_missions(OutStream* out);   // 0x4e9b50 (save_records.cpp: the game object's mission records)
};
#pragma pack(pop)


extern List8 g_list_a70480;                         // 0xa70480
extern RecB0* g_party_templates;                               // 0x5e29f8 (array of 0xb0-byte records)
#include "../game_globals.h"
#include "../../include/item_kind.h"
extern ItemKind* g_items;                           // 0x5e29d0 (array of 0x238-byte item kinds)


// ---- the sub-writers ------------------------------------------------------------------------------------
OutStream* write_soa(OutStream* out, char* base);   // 0x4e9620 (free, two stack args)

#endif
