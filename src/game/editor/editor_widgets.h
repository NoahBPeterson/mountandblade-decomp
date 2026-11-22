#ifndef EDITOR_UI_H
#define EDITOR_UI_H
// clang-format off
// Slice s004f5d20: the last block of the mission/options UI widget factories (the arrow buttons, the
// horizontal slider and the scroll bar), the map editor's dialog refresh hook (0x4f6390), the engine
// start-up sequence (0x4f63c0) and the tail of a std::vector<T> implementation for a 0x20-byte record
// (0x4f6360 insert-n, 0x4f65f0 construct-n with its catch funclet at 0x4f6658, 0x4f6680 destroy-range).
//
// MissionUi slot offsets (+0x58/+0x64/+0x6c/+0x70/+0x80/+0xa4/+0xc8), the tstring layout and the
// resource-manager / language-table / log-stream globals follow the verified neighbouring slices
// (s004dfd50, s004dc630, s004e05c0, s0049e780, s004bd990).  Names are provisional.
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "engine/math/rgl_math.h"

// ---- engine string --------------------------------------------------------------------------------
#include "engine/core/tstring.h"

// ---- resource manager (0x5d6fa0) -------------------------------------------------------------------
#include "engine/render/mesh.h"
using rgl::mesh;

#include "engine/resource/res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules

// resource lookup + clone, as the factories do it (the original inlines this twice, giving the two
// tstring temporaries separate stack slots)
__forceinline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return (mesh*)((rgl::mesh*)g_resmgr.get_object(&n))->clone();
}

// ---- language / text tables (0x5e2a18) -------------------------------------------------------------
#include "game/editor/mesh2_common.h"   // LangTables (shared: find 0x4c6010, load 0x4cda90) and its global g_strtable 0x5e2a18

// ---- text log stream (0x5a9f60, see s004bd990) -----------------------------------------------------
#include "engine/io/stream.h"                     // text_stream: name, autoflush, fp, failed, is_open

// ---- widgets ---------------------------------------------------------------------------------------
#include "game/ui/widgets/ui_widget.h"                 // MissionUi (the UI widget interface)

// ---- helpers declared elsewhere --------------------------------------------------------------------
#include "game/ui/screens/character_sheet/cs_common.h"                        // label_refresh(MissionUi*, tstring) 0x4e4c70, MissionUi, make_scrollbar / make_editor_list
#include "engine/render/font.h"                            // Font (ctor 0x4d99d0, load 0x4d9aa0: the original hands it a tstring)
#include "engine/render/render_batch.h"                        // render_batch, g_batch (0x5fe898): init_formats 0x4bc5b0 starts the engine


// ---- globals ---------------------------------------------------------------------------------------
extern char g_path_prefix_chars[];              // 0x8ee3c28
#include "game/common/game_globals.h"

struct GuiState {
    void init_widgets();   // 0x5d7080
};
#include "game/ui/window_mgr.h"                         // g_windows (0xa73980)

void init_mission_trigger_records();                   // 0x4d0620
void init_mission_trigger_records(); // 0x4d0620

#endif
