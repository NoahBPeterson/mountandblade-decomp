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
#include "rgl.h"

// ---- engine string --------------------------------------------------------------------------------
#include "../tstring.h"

// ---- resource manager (0x5d6fa0) -------------------------------------------------------------------
#include "../rgl_mesh.h"
using rgl::mesh;

#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
extern resmgr g_alloc;                          // 0x5d6fa0

// resource lookup + clone, as the factories do it (the original inlines this twice, giving the two
// tstring temporaries separate stack slots)
__forceinline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return g_alloc.get_object(n)->clone();
}

// ---- language / text tables (0x5e2a18) -------------------------------------------------------------
#include "../../rgl/mesh2/mesh2_common.h"   // LangTables (shared: find 0x4c6010, load 0x4cda90) and its global g_strtable 0x5e2a18

// ---- text log stream (0x5a9f60, see s004bd990) -----------------------------------------------------
struct stream_base {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void w_cstr(const char* s);         // +0x28 (slot 10)
};
extern stream_base g_log;                       // 0x5a9f60

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
    WidgetC* ctor();                            // 0x49deb0 (returns this)
};

// ---- helpers declared elsewhere --------------------------------------------------------------------
void load_params(Widget* w, tstring name);      // 0x4e4c70 (tstring by value)
void* make_medium_button(void* parent, const tstring* text, int a2, int a3, float a4,
                         const tstring* up, const tstring* down, int a5, float a6, float k);  // 0x4dcc50


// ---- globals ---------------------------------------------------------------------------------------
extern int g_no_cam;                            // 0x5a9fb0
extern tstring g_log_path;                      // 0x5a9f64
extern FILE* g_log_file;                        // 0x5a9f90
extern unsigned char g_log_flag94;              // 0x5a9f94
extern unsigned char g_log_flag95;              // 0x5a9f95
extern unsigned char g_log_flag78;              // 0x5a9f78
extern tstring g_path_prefix;                   // 0x8ee3c28
extern char g_path_prefix_chars[];              // 0x8ee3c28
extern tstring g_5d705c;                        // 0x5d705c
extern unsigned char g_5d706c;                  // 0x5d706c
extern unsigned char g_5d706d;                  // 0x5d706d
extern int g_5e29a4;                            // 0x5e29a4
extern int g_5e29c4;                            // 0x5e29c4
extern void* g_font_def;                        // 0x5e2934

struct Startup { void init(); };                // 0x5fe898
struct GuiState { void init_widgets(); };       // 0x5d7080
#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition
struct EditorGame {
    void refresh();
    char pad[0x2f9];
    unsigned char f2f9;                         // +0x2f9
};
extern EditorGame* g_editor_game;               // 0xa739b8

struct EditorApp {
    char pad[0xe8];
    HWND hwnd;                                  // +0xe8
    void refresh_dialog();   // 0x4f6390
};

struct FontDef { void load_fonts(tstring* path); };  // 0x4d9aa0
FontDef* ctor_font_def(FontDef* p);             // 0x4d99d0
void init_mission_triggers();                   // 0x4d0620
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
