// mission.h: the mission record (replay-frame interpolation, per-team counters, combatant slots, origin table
// checks), the mission view (render-target setup), the three-record selector and two small containers of
// mission_ui.cpp (0x489860..0x48ae20). Only fields read by these functions are declared (cited by address).
// Names provisional. The slices called both Mission and MissionView `Mission`; their `this` layouts differ
// (view: ints at +0x18, +0xd8, +0x118; mission: counters at +0x621c, frames at +0x674c).
#ifndef GAME_UI_MISSION_H
#define GAME_UI_MISSION_H
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "./ui_common.h"

// ---- the mission record is the shared src/game/mission.h (replay frames, per-team counters, per-team class blocks) ----
#include "../mission.h"

// ---- combatant record seen from +0x527c: the 0x16c-byte TeamRec (mission.h, at +0x5280) with its three ClassInfo blocks, as the
// ui accessors address it (the displacements of their code are relative to 0x527c, so they cannot go through TeamRec) -------
struct Blk {
    char pad0[8];
    int k8;                                 // +0x08 = ClassInfo::kind (switch in active_kind 0x48aac0)
    int kc;                                 // +0x0c = ClassInfo::k4 (slot_mode 0x48ab30)
    int k10;                                // +0x10 = ClassInfo::a (slot_kind_a 0x48ab90)
    int k14;                                // +0x14 = ClassInfo::b (slot_kind_b 0x48abf0)
    char pad18[0x4c - 0x18];
    int use;                                // +0x4c = ClassInfo::use (0x48a9e0..0x48ac50)
    int m4;                                 // +0x50 = ClassInfo::saved_use (0x48a9e0, 0x48aa20)
    char pad54[0x6c - 0x54];
};
struct CombatRec {
    Blk b[3];
    char pad144[0x16c - 0x144];
};

// combatant-origin id pairs, indexed by combatant (0x48ac50, 0x48ac90). The slices lumped these two accessors into `Mission`;
// `this` is an array of {a, b} pairs, not the mission object (whose +0 is the scene pointer).
struct OriginPairs {
    struct Pair { int a, b; } slots[1];
    bool has_origin(int i);                 // 0x48ac50
    bool has_free_origin(int i);            // 0x48ac90
};
extern void* g_origin_table[];              // 0xa170d4 object per origin id
extern int g_origin_state[];                // 0xa174d4 0 = origin idle / free

// ---- the view: three 16-int vectors and a render-target factory --------------------------------------------
struct ViewObj {                            // interface at +0x1b4 (virtual 23 = +0x5c)
    virtual void d0(); virtual void d1(); virtual void d2(); virtual void d3(); virtual void d4();
    virtual void d5(); virtual void d6(); virtual void d7(); virtual void d8(); virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22();
    virtual void slot23(void* src, void* dst);
};
struct Allocator { char* make(int a, int b, int c, int d); };   // 0x5d6fa0 resource manager (ECX = &g_alloc)
extern Allocator g_alloc;                   // 0x5d6fa0
extern int g_day;                           // 0x5a9f9c (day / quality threshold: 0x19, 0x3c)

struct V4 { int a, b, c, d; };
struct MissionView {
    char pad0[0x18];
    int v18[16];                            // +0x18 (width, height, then the quarter size at +0x20 / +0x24)  0x489ad0
    char pad58[0x64 - 0x58];
    int* p64;                               // +0x64   0x489b20
    char pad68[0xd8 - 0x68];
    int vd8[16];                            // +0xd8   0x4897a0
    int v118[16];                           // +0x118
    int f158;                               // +0x158
    char pad15c[0x168 - 0x15c];
    int f168;                               // +0x168
    int f16c;                               // +0x16c
    int f170;                               // +0x170 clock index
    char pad174[0x178 - 0x174];
    __int64 f178;                           // +0x178 stamp
    int f180;                               // +0x180
    char pad184[0x1b4 - 0x184];
    ViewObj* obj1b4;                        // +0x1b4

    void ext_467c50(int);                   // 0x467c50 (thiscall, one stack argument)
    void ext_477b70();                      // 0x477b70
    void setup(void* src, int* b, int* c);  // 0x4897a0
    void set(void* src);                    // 0x489820
    void quarter();                         // 0x489ad0
    void* make(int* p);                     // 0x489b20
};

// ---- three-record selector (0x6c bytes each) ---------------------------------------------------------------
struct SelRec {
    char pad0[0xc];
    int x;                                  // +0x0c   0x489860
    char pad10[0x18 - 0x10];
    int a[4];                               // +0x18   0x489890, 0x4898e0
    int c[4];                               // +0x28
    int b;                                  // +0x38
    char pad3c[0x4c - 0x3c];
    int present;                            // +0x4c
    char pad50[0x60 - 0x50];
    int counter;                            // +0x60   0x4899e0
    char pad64[0x6c - 0x64];
};
struct Sel {
    SelRec rec[3];
    void set_v(int v);                                      // 0x489860
    void first(int* out);                                   // 0x489890
    void set_vec(const int* a, const int* b, const int* c); // 0x4898e0
    void bump(int d);                                       // 0x4899e0
};

// ---- 30 pointers to int tables (0x489be0) ------------------------------------------------------------------
struct Ptr30 {
    int* p[30];
    void clear_all();                       // 0x489be0
};

// ---- containers -------------------------------------------------------------------------------------------
struct MapEntry { unsigned key; int value; };
struct Map10007 { MapEntry tab[0x2717]; int lookup(unsigned id); };   // 0x48ad00
#include "../../include/e68.h"                               // E68 (0x68 bytes; shared with rgl/mesh2/mesh_2.h)
struct Vec68 { char pad4[4]; E68* begin; E68* end; int count(); };    // 0x48ace0
void fill8(int* first, int* last, const int* val);          // 0x48ad70
void move_backward8(__int64* first, __int64* last, __int64* dest);   // 0x48ada0

#endif // GAME_UI_MISSION_H
