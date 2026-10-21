// Shared declarations of slice s00435490 (provisional names; evidence in comments).
#ifndef S00435490_H
#define S00435490_H
#include "rgl.h"
#include <math.h>
using rgl::vector4;
using rgl::matrix;

// copy of s0042b790/tstring.h (only what this slice uses)
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    char equals(const tstring& o) const;        // 0x42bdd0
};

struct Pair { int a, b; };

// mission object at 0x5e292c (see s00432130): relation matrix, per-team tables
struct TeamSlot { int a, b, c, d, e, f; };       // 0x18
struct Mission {
    char pad0[0x5280];
    int field5280[1];                            // +0x5280, stride 0x16c per team
    char pad5284[0x53c8 - 0x5284];
    int relation[1];                             // +0x53c8, stride 0x16c per team
    char pad53cc[0x5df0 - 0x53cc];
    int f5df0;                                   // +0x5df0 (index into the 0x58-byte table at 0x5e2a00)
    char pad5df4[0x622c - 0x5df4];
    TeamSlot slots[8];                           // +0x622c
    char mode_flag1() const;                     // 0x435840
};
extern Mission* g_mission;                       // 0x5e292c

struct ModeRec { char pad[0x20]; unsigned flags; char pad2[0x58 - 0x24]; };   // 0x58, array at 0x5e2a00
extern ModeRec* g_modes;                         // 0x5e2a00

struct ItemRec {                                 // 0x238, array at 0x5e29d0
    char pad0[0x20];
    tstring name;                                // +0x20
    char pad30[0x1c8 - 0x30];
    unsigned type;                               // +0x1c8
    char pad1cc[4];
    unsigned flags;                              // +0x1d0
    char pad1d4[0x238 - 0x1d4];
};
extern ItemRec* g_items;                         // 0x5e29d0
struct NamedRec20 { tstring name; char pad[0x20 - 0x10]; };             // 0x20, array at 0x5e28f8
extern NamedRec20* g_recs20;                     // 0x5e28f8
struct NamedRec78 { tstring name; char pad[0x78 - 0x10]; };   // 0x78, array at 0x5e29dc
extern NamedRec78* g_recs78;                     // 0x5e29dc

struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; };   // 0x30

// per-slot floats of an agent: stride 0x2c from +0xf8
// 0x34-byte records, array pointer at 0xa75eb0, parallel index array at 0xa75ebc
struct EmitSub {
    char emit(int flags, int zero, int* count, __int64* vals, unsigned tag);   // 0x4b09e0
};
struct Emitter {
    char pad0[4];
    tstring name;                                // +4
    char pad14[0x18 - 0x14];
    EmitSub sub;                                 // +0x18
    char pad19[0x34 - 0x19];
};
extern Emitter* g_emitters;                      // 0xa75eb0
extern int g_emitter_idx[];                      // 0xa75ebc

struct Agent_f8 {
    char pad[0xf8];
    float v[1][0x2c / 4];
    void set_slot(int i, float x);               // 0x4354b0
};

// object with a placement block at +0x13fb4 (provisional)
struct Placement {
    char pad0[0x13fb4];
    vector4 v13fb4;
    vector4 v13fc4;
    char pad13fd4[0x14014 - 0x13fd4];
    matrix m14014;
    void set_matrix(const matrix* m);            // 0x435650
};

// mission-wide clock helpers are free functions
struct SoundCtl {
    char pad0[0x1a068];
    int f1a068;
    tstring name;                                // +0x1a06c
    int f1a07c;
    char emit_event(int id, int a, int b);       // 0x435790
};

struct Agent {
    char pad0[0x4];
    int f4;                                      // +4
    char pad8[0x10 - 8];
    int f10;                                     // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                     // +0x2c
    char pad30[0x80 - 0x30];
    float f80;                                   // +0x80
    char pad84[0xd4 - 0x84];
    int fd4;                                     // +0xd4
    char padd8[0xe8 - 0xd8];
    unsigned fe8;                                // +0xe8
    char padec[0x114 - 0xec];
    unsigned f114;                               // +0x114
    char pad118[0x12c - 0x118];
    TroopDef** f12c;                              // +0x12c
    char pad130[0x1c4 - 0x130];
    int team;                                    // +0x1c4
    char pad1c8[0x2c8 - 0x1c8];
    int f2c8;                                    // +0x2c8
    char pad2cc[0x533c - 0x2cc];
    int f533c;                                   // +0x533c
    char pad5340[0x56c4 - 0x5340];
    int f56c4;                                   // +0x56c4
    float scale_f4() const;                      // 0x4355f0
    float scale_f10() const;                     // 0x435620
    int clamped_team() const;                    // 0x435490
    bool flag_0x400_or_0x114() const;            // 0x4354d0
    Pair slot_pair(int idx) const;               // 0x432790 (s00432130)
    void classify(int key);                      // 0x435520
    void reset_if_enemy_charged();               // 0x435b90
};

// placed object (scene entry, provisional): two placement matrices, kind/id, name, value
struct PlacedObj {
    char pad0[0x18];
    matrix m18;                                  // +0x18
    matrix m58;                                  // +0x58 (unused here)
    matrix m98;                                  // +0x98
    char padd8[0x188 - 0xd8];
    int kind;                                    // +0x188
    int id;                                      // +0x18c
    tstring name;                                // +0x190
    int value;                                   // +0x1a0
    void reset();                                // 0x4352f0
};
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m);   // 0x435860

// vector-like containers: [0] allocator/pad, +4 first, +8 last
struct Vec38 { int pad; char* first; char* last; int size() const; };   // 0x435990
struct Vec2c { int pad; char* first; char* last; int size() const; };   // 0x4359c0
struct Vec1c8 { int pad; char* first; char* last; int size() const; };  // 0x4359e0
struct Triple { int a, b, c; };
#endif

