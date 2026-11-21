// map_editor.h: the map editor's per-frame update and the prop-list maintenance next to it (code 0x5305b0..0x531590,
// TU 118). Names provisional. The classes are the canonical ones:
//   MissionBlocker    src/game/options_panel/mission_item_recs.h (the mission screen: update_editor 0x5305b0 and
//                     add_selected 0x5314e0 are its members; the picked record is sel_target, the camera k)
//   GameData          src/include/game_data.h (g_game 0x5e2930); the editor fields it lacks are read through the
//                     accessors below (offsets proven by 0x5305b0)
//   Mission           src/game/mission.h (g_mission 0x5e292c); MapMission adds remove_prop and the typed views of
//                     the ints at +0x65d0 / +0x65d8 / +0x65dc
//   PlacedObj         src/game/placed_obj.h (the 0x1c8-byte record the map editor edits)
//   PlacedObj            src/game/placed_obj.h (the same record as Mission::props holds it)
#ifndef GAME_MAP_EDITOR_MAP_EDITOR_H
#define GAME_MAP_EDITOR_MAP_EDITOR_H
#include <windows.h>
#include <vector>
#include "../../rgl/mesh2/gui_edit.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"
#include "../../include/mat4.h"
#include "../../include/game_data.h"
#include "../placed_obj.h"
#include "../mission.h"
#include "../placed_obj.h"
#include <vector>
#include "../key_names.h"
#include "../window_mgr.h"

using rgl::matrix;
using rgl::vector4;

// ---- engine globals (the input state words of the keyboard object; the editor's modifier / action keys) --------------

// pos3: a frame with the engine's "advance along forward" helper (matrix::move_forward, 0x531380). The multiplication
// is rgl::mat4::operator* (0x40e870), the relative transform pos::relative (0x4329d0).
struct pos3 : rgl::matrix {
    // (the multiplication is rgl::matrix::operator*, 0x40e870)
    void move_forward(const float& d);              // 0x531380 (ret 4): o += f * d
};

// ---- scene objects the editor touches through pointers ------------------------------------------------------------
// the per-record scene object (PlacedObj +0x1b4, PlacedObj +0x1b4) is an rgl::entity (src/rgl/scene/scene_entity.h): the frame
// at +0x58 (local), the flags at +0x140 / +0x141, vtable slot 0x5c = set_local_scaled_dirty(matrix*, vector4*)
inline rgl::entity* ent_of(PlacedObj* r) { return (rgl::entity*)r->ent; }

// the current editor entity (0xa739b8)
#include "../options_panel/mission_item_recs.h"   // MissionBlocker (slot 14): pick_entity 0x4ed8d0, f2f9

// ---- Mission: the editor's view (0x5305b0) ---------------------------------------------------------------------------
#include "../prop_vec.h"      // PropVec: the one definition (agent4, map_editor, script)
#include "../key_names.h"      // g_keyboard (0xa170c8)
#include "../ui_globals.h"      // UI / core globals

struct MapMission : Mission {
    void remove_prop(int index);                                // 0x5313c0
};
inline MapMission* g_map_mission() { return (MapMission*)g_mission; }

// the editor fields of 0x5305b0 (camera frame and position, selection, dirty flags) are GameData::editor (include/game_data.h)

#endif // GAME_MAP_EDITOR_MAP_EDITOR_H
