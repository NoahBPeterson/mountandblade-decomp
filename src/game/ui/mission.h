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
#include "../key_names.h"

// (the per-team accessors 0x48a9e0..0x48ac50 address mission.h's TeamRec teams[] at +0x527c directly)

// ---- the view: three 16-int vectors and a render-target factory --------------------------------------------
struct ViewObj {                            // interface at +0x1b4 (virtual 23 = +0x5c)
    virtual void d0(); virtual void d1(); virtual void d2(); virtual void d3(); virtual void d4();
    virtual void d5(); virtual void d6(); virtual void d7(); virtual void d8(); virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22();
    virtual void slot23(void* src, void* dst);
};
#include "../../rgl/resource.h"             // resmgr g_resmgr (0x5d6fa0): get_shadow_skin 0x41c420 builds the shadow-map render target

struct Blob16 { int a, b, c, d; };
struct Prop {
    void update(int x);   // 0x467c50 (game/terrain_a: the prop record's per-frame update)
};
struct EmptySlot {
    virtual void slot();   // 0x477b70 (game/terrain_b: the empty slot of the IfaceB-style records)
};
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

    __forceinline void ext_467c50(int x) { ((Prop*)this)->update(x); }              // 0x467c50 (thiscall, one stack argument: Prop::update, terrain_a)
    __forceinline void ext_477b70() { ((EmptySlot*)this)->EmptySlot::slot(); }       // 0x477b70 (EmptySlot::slot, terrain_b: called directly, not through the vtable)
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
    rgl::vector4 first();                                   // 0x489890 (ret 4: hidden return, returns the pointer in eax)
    void set_vec(const int* a, const int* b, const int* c); // 0x4898e0
    void bump(int d);                                       // 0x4899e0
};


// ---- containers -------------------------------------------------------------------------------------------
#include "../../include/map10007.h"                        // MapEntry, Map10007 (lookup 0x48ad00, mission_ui.cpp)
#include "../../include/e68.h"                               // E68 (0x68 bytes; shared with rgl/mesh2/mesh_2.h)
struct Vec68 { char pad4[4]; E68* begin; E68* end; int count(); };    // 0x48ace0
void fill8(int* first, int* last, const int* val);          // 0x48ad70
void move_backward8(__int64* first, __int64* last, __int64* dest);   // 0x48ada0

#endif // GAME_UI_MISSION_H
