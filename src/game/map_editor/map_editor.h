// map_editor.h: the map editor's per-frame update and the prop-list maintenance next to it (code 0x5305b0..0x531590,
// TU 118). Names provisional. The classes are the canonical ones:
//   MapEditorDialog   src/rgl/mesh2/gui_edit.h (the object at 0x8ee3c48; this module added cam_x/y/z, picked, f2f8, f2f9
//                     and the members update, add_selected, select_entity, handle_input there)
//   GameData          src/include/game_data.h (g_game 0x5e2930); the editor fields it lacks are read through the
//                     accessors below (offsets proven by 0x5305b0)
//   Mission           src/game/mission.h (g_mission 0x5e292c); MapMission adds remove_prop and the typed views of
//                     the ints at +0x65d0 / +0x65d8 / +0x65dc
//   PlacedObj         src/game/placed_obj.h (the 0x1c8-byte record the map editor edits)
//   Rec1c8            src/include/rec1c8.h (the same record as Mission::props holds it)
#ifndef GAME_MAP_EDITOR_MAP_EDITOR_H
#define GAME_MAP_EDITOR_MAP_EDITOR_H
#include <windows.h>
#include <vector>
#include "../../rgl/mesh2/gui_edit.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"
#include "../../include/mat4.h"
#include "../../include/game_data.h"
#include "../../include/rec1c8.h"
#include "../mission.h"
#include "../placed_obj.h"
#include "../int_vec.h"      // IntVec: push_back with the inlined fast path and the library insert_n (0x43e7b0), as 0x5305b0 uses it

using rgl::matrix;
using rgl::vector4;

// ---- engine globals (the input state words of the keyboard object; the editor's modifier / action keys) --------------
extern int g_a17458;                    // 0xa17458
extern int g_a17858;                    // 0xa17858
extern int g_a17148;                    // 0xa17148  select-all modifier
extern int g_a17348;                    // 0xa17348
extern int g_a1714c;                    // 0xa1714c
extern int g_a1754c;                    // 0xa1754c
extern int g_a17420;                    // 0xa17420  delete key
extern int g_a17820;                    // 0xa17820
extern int g_a1712c;                    // 0xa1712c
extern int g_a1752c;                    // 0xa1752c
extern HWND g_editor_hwnd;              // 0x8ee3d30

// pos3: a frame with the engine's "advance along forward" helper (matrix::move_forward, 0x531380). The multiplication
// is rgl::mat4::operator* (0x40e870), the relative transform pos::relative (0x4329d0).
struct pos3 : rgl::matrix {
    // (the multiplication is rgl::matrix::operator*, 0x40e870)
    void move_forward(const float& d);              // 0x531380 (ret 4): o += f * d
};

// ---- scene objects the editor touches through pointers ------------------------------------------------------------
// the per-record scene object (PlacedObj +0x1b4, Rec1c8 +0x1b4) is an rgl::entity (src/rgl/scene/scene_entity.h): the frame
// at +0x58 (local), the flags at +0x140 / +0x141, vtable slot 0x5c = set_local_scaled_dirty(matrix*, vector4*)
inline rgl::entity* ent_of(PlacedObj* r) { return (rgl::entity*)r->m1b4; }

// the current editor entity (0xa739b8)
#include "../editor_entity.h"
extern EditorEntity* g_editor;                                  // 0xa739b8

// ---- Mission: the editor's view (0x5305b0) ---------------------------------------------------------------------------
#include "../prop_vec.h"      // PropVec: the one definition (agent4, map_editor, script)

struct MapMission : Mission {
    rgl::entity* cursor() { return (rgl::entity*)f65d0; }       // +0x65d0 the selection cursor object
    rgl::mesh* mesh_a() { return (rgl::mesh*)f65d8; }           // +0x65d8
    rgl::entity* frame_b() { return (rgl::entity*)f65dc; }      // +0x65dc
    void remove_prop(int index);                                // 0x5313c0
};
inline MapMission* g_map_mission() { return (MapMission*)g_mission; }

// ---- GameData: the editor fields (0x5305b0) ---------------------------------------------------------------------------
inline matrix& ed_frame(GameData* g) { return *(matrix*)((char*)g + 0x1a164); }          // +0x1a164 the editor camera frame
inline vector4& ed_pos(GameData* g) { return *(vector4*)((char*)g + 0x1a1a4); }          // +0x1a1a4 the editor camera position
inline int& ed_selected(GameData* g) { return *(int*)((char*)g + 0x1a1b4); }             // +0x1a1b4 the selected record
inline std::vector<int>& ed_sel_list(GameData* g) { return *(std::vector<int>*)((char*)g + 0x1a1b8); }   // +0x1a1b8 multi-selection
inline int& ed_dirty_1c8(GameData* g) { return *(int*)((char*)g + 0x1a1c8); }            // +0x1a1c8
inline int& ed_dirty_1cc(GameData* g) { return *(int*)((char*)g + 0x1a1cc); }            // +0x1a1cc

#endif // GAME_MAP_EDITOR_MAP_EDITOR_H
