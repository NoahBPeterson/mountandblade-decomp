#ifndef EDITOR_UI_H
#define EDITOR_UI_H
// clang-format off
// Slice s004f5d20: the last block of the mission/options UI widget factories (the arrow buttons, the
// horizontal slider and the scroll bar), the map editor's dialog refresh hook (0x4f6390), the engine
// start-up sequence (0x4f63c0) and the tail of a std::vector<T> implementation for a 0x20-byte record
// (0x4f6360 insert-n, 0x4f65f0 construct-n with its catch funclet at 0x4f6658, 0x4f6680 destroy-range).
//
// Widget slot offsets (+0x58/+0x64/+0x6c/+0x70/+0x80/+0xa4/+0xc8), the tstring layout and the
// resource-manager / language-table / log-stream globals follow the verified neighbouring slices
// (s004dfd50, s004dc630, s004e05c0, s0049e780, s004bd990).  Names are provisional.
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "../../rgl/rgl_math.h"

// ---- engine string --------------------------------------------------------------------------------
#include "../tstring.h"

// ---- resource manager (0x5d6fa0) -------------------------------------------------------------------
#include "../../rgl/mesh.h"
using rgl::mesh;

#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
extern resmgr g_alloc;                          // 0x5d6fa0

// resource lookup + clone, as the factories do it (the original inlines this twice, giving the two
// tstring temporaries separate stack slots)
__forceinline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return (mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&n))->clone();
}

// ---- language / text tables (0x5e2a18) -------------------------------------------------------------
#include "../../rgl/mesh2/mesh2_common.h"   // LangTables (shared: find 0x4c6010, load 0x4cda90) and its global g_strtable 0x5e2a18

// ---- text log stream (0x5a9f60, see s004bd990) -----------------------------------------------------
#include "../../rgl/stream.h"                     // text_stream: name, autoflush, fp, failed, is_open
extern text_stream g_log;                         // 0x5a9f60 the engine text log (include/rgl_globals.h declares it too; that header pulls the world classes in)

// ---- widgets ---------------------------------------------------------------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

// the plain 0x178-byte widget: only the base ctor runs
struct WidgetA : Widget {
    char pad[0x178 - 4];
};

// the 0x188-byte slider / scrollbar widgets (vptr 0x591fb0 / 0x592140)
struct WidgetB : Widget {
    char pad0[0x168 - 4];
    float f168;                                 // +0x168
    char pad1[0x178 - 0x16c];
    int f178;                                   // +0x178
    float f17c;                                 // +0x17c
    int f180;                                   // +0x180
    int f184;                                   // +0x184
};

// the 0x4a8-byte editor list widget (ctor 0x49deb0)
struct WidgetC : Widget {
    char pad0[0x104 - 4];
    int f104;                                   // +0x104
    char pad1[0x484 - 0x108];
    int f484;                                   // +0x484
    char pad2[0x490 - 0x488];
    int f490;                                   // +0x490
    // (the constructor 0x49deb0 is c49deb0::Camera::Camera, below)
};

// ---- helpers declared elsewhere --------------------------------------------------------------------
#include "./cs_common.h"                        // label_refresh(MissionUi*, tstring) 0x4e4c70, MissionUi, make_scrollbar / make_editor_list
struct Font {
    Font();   // 0x4d99d0 (FontDef's constructor)
    void load(const char* path);   // 0x4d9aa0 (the original hands it a tstring)
};
// render_batch.h cannot be included next to the rgl_mesh.h universe this TU lives in (rgl::mesh / rgl::scene are declared twice): the one member the start-up calls
struct render_batch {
    void init_formats();   // 0x4bc5b0 (src/game/render_batch.h: the class of g_batch, 0x5fe898)
};
extern render_batch g_batch;   // 0x5fe898 (src/game/render_batch.h; defined in config.cpp)
void* make_medium_button(void* parent, const tstring* text, int a2, int a3, float a4,
                         const tstring* up, const tstring* down, int a5, float a6, float k);  // 0x4dcc50


// ---- globals ---------------------------------------------------------------------------------------
extern FILE* g_log_file;                        // 0x5a9f90
extern unsigned char g_log_flag94;              // 0x5a9f94
extern unsigned char g_log_flag95;              // 0x5a9f95
extern tstring g_path_prefix;                   // 0x8ee3c28
extern tstring g_5d705c;                        // 0x5d705c (resmgr::cur, +0xbc of the resource manager g_alloc 0x5d6fa0)
extern unsigned char g_5d706c;                  // 0x5d706c (the resource manager's +0xcc)
extern unsigned char g_5d706d;                  // 0x5d706d (+0xcd)
extern char g_path_prefix_chars[];              // 0x8ee3c28
#include "../game_globals.h"

struct GuiState {
    void init_widgets();   // 0x5d7080
};
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition
struct EditorGame {
    char pad[0x2f9];
    unsigned char f2f9;                         // +0x2f9
};

struct EditorApp {
    char pad[0xe8];
    HWND hwnd;                                  // +0xe8
    void refresh_dialog();   // 0x4f6390
};

struct FontDef {};                              // the font object at g_font: Font

void init_mission_trigger_records();                   // 0x4d0620
void init_mission_trigger_records(); // 0x4d0620

// ---- per-row text table of the mission-UI widget ----------------------------------------------------
struct TextEntry {                              // 0x34 bytes
    char pad0[0x10];
    tstring text;                               // +0x10
    char pad1[0x34 - 0x20];
};
struct TextRow {                                // 0x1f8 bytes
    char pad0[0x3c];
    int count;                                  // +0x3c
    TextEntry* entries;                         // +0x40
    char pad1[0x1f8 - 0x44];
};

#endif
