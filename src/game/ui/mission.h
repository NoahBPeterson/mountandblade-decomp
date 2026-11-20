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
#include "../../rgl/resource.h"             // resmgr g_resmgr (0x5d6fa0): get_shadow_skin 0x41c420 builds the shadow-map render target

struct Blob16 { int a, b, c, d; };
struct EmptySlot {
    virtual void slot();   // 0x477b70 (game/terrain_b: the empty slot of the IfaceB-style records)
};
#include "../placed_obj.h"                  // PlacedObj::animate_to 0x4897a0 / set_position 0x489820 (defined in mission_ui.cpp)

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


#include <vector>
// ---- containers -------------------------------------------------------------------------------------------
#include "../../include/map10007.h"                        // MapEntry, Map10007 (lookup 0x48ad00, mission_ui.cpp)
#include "../../include/e68.h"                               // E68 (0x68 bytes; shared with rgl/mesh2/mesh_2.h)
struct Vec68 : std::vector<E68> { int count(); };    // 0x48ace0
void fill8(int* first, int* last, const int* val);          // 0x48ad70
void move_backward8(__int64* first, __int64* last, __int64* dest);   // 0x48ada0

#endif // GAME_UI_MISSION_H
