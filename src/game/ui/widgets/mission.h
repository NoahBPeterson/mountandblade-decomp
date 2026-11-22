// mission.h: the mission record (replay-frame interpolation, per-team counters, combatant slots, origin table
// checks), the mission view (render-target setup), the three-record selector and two small containers of
// widget_base.cpp (0x489860..0x48ae20). Only fields read by these functions are declared (cited by address).
// Names provisional. The slices called both Mission and MissionView `Mission`; their `this` layouts differ
// (view: ints at +0x18, +0xd8, +0x118; mission: counters at +0x621c, frames at +0x674c).
#ifndef GAME_UI_MISSION_H
#define GAME_UI_MISSION_H
#include "engine/math/rgl_math.h"
#include "engine/core/tstring.h"
#include "game/ui/widgets/ui_common.h"

// ---- the mission record is the shared src/game/mission/mission.h (replay frames, per-team counters, per-team class blocks) ----
#include "game/mission/mission.h"
#include "game/input/key_names.h"

// (the per-team accessors 0x48a9e0..0x48ac50 address mission.h's TeamRec teams[] at +0x527c directly)

// ---- the view: three 16-int vectors and a render-target factory --------------------------------------------
#include "engine/resource/resource.h"             // resmgr g_resmgr (0x5d6fa0): get_shadow_skin 0x41c420 builds the shadow-map render target

struct EmptySlot {
    virtual void slot();   // 0x477b70 (game/terrain_b: the empty slot of the IfaceB-style records)
};
#include "game/mission/placed_obj.h"                  // PlacedObj::animate_to 0x4897a0 / set_position 0x489820 (defined in widget_base.cpp)

// (the three-record selector the slices called Sel is TeamRec, ../mission.h: its classes cls[3], members set_v 0x489860,
// first 0x489890, set_vec 0x4898e0, bump 0x4899e0)


#include <vector>
// ---- containers -------------------------------------------------------------------------------------------
#include "game/data/map10007.h"                        // MapEntry, Map10007 (lookup 0x48ad00, widget_base.cpp)
#include "game/data/e68.h"                               // E68 (0x68 bytes; shared with game/editor/poly_mesh_edge_face_ops.h)
struct Vec68 : std::vector<E68> { int count(); };    // 0x48ace0
void fill8(int* first, int* last, const int* val);          // 0x48ad70
void move_backward8(__int64* first, __int64* last, __int64* dest);   // 0x48ada0

#endif // GAME_UI_MISSION_H
