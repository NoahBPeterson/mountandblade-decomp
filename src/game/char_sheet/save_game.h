#ifndef SAVE_GAME_H
#define SAVE_GAME_H
// Slice s004e85d0 (batch ds1): world-map screen init/frame/buttons and the save-file helpers it calls.
// Layouts follow the verified neighbouring slices (s004d5410, s004e2ea0, s00477ea0).
#include "../../include/image_addrs.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "../../rgl/rgl_math.h"

#define MB_NOTHROW throw()

// ---- tstring (see s00467bf0) ----------------------------------------------------------------------------
#include "../tstring.h"
#include "../str_table.h"

#include "../strat/gui_manager.h"

#include "../../rgl/stream.h"
#define LOG(s) ((stream_base&)g_log).w_cstr(s)                 // 0x5a9f60 (virtual call through a stream_base reference)

bool print_and_remove(const char* path);                // 0x539023
int  rename_file(char* from, const char* to);           // 0x538ff5
extern "C" void __cdecl crt_free(void* p);              // 0x537168
extern "C" int _fltused = 0;
void save_path(char* buf);                               // 0x4dd7b0
int  open_write(void* stream, char* path);         // 0x415320

// ---- widgets --------------------------------------------------------------------------------------------
#include "../ui_widget.h"                 // MissionUi (the UI widget interface)

#include "../window_mgr.h"                         // g_windows (0xa73980)

// ---- serialisation streams ------------------------------------------------------------------------------
#include "../../rgl/stream.h"
#include "../../include/troop.h"          // the 0x508-byte record the slice called GameRecord

struct SaveWriter {
    stream_base* vtbl;                                    // +0x00
    char       pad04[0x14 - 0x04];
    char       failed;                                  // +0x14
    char       pad15[0x2c - 0x15];
    tstring    slot;                                    // +0x2c
    char       pad3c[0x40 - 0x3c];
    char       closed;                                  // +0x40
    char       released;                                // +0x41
};
void copy_ctor(void* dst, const void* src);             // 0x42c110

// ---- the game object (see s00473a60) ---------------------------------------------------------------------
struct SaveCtx;
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)
#pragma pack(push, 4)
#pragma pack(pop)
#include "../../include/clock_table.h"   // g_ticks (0x5e2948)
extern float g_54a6d0;                                  // 0x54a6d0
extern int   g_59811c;                                  // 0x59811c ("WRDR")
extern char  g_598120;                                  // 0x598120
#include "../game_globals.h"

struct SaveCtx {
    tstring title;                                      // +0x0
    tstring slot;                                       // +0x10
    int     flag;                                       // +0x20
    float   stamp;                                      // +0x24
    int     pad28[3];                                   // +0x28 (unused by the writer and the header reader)
    int write(stream_base* out);                          // 0x4e8d70 (ret 4)
};

// ---- the map screen -------------------------------------------------------------------------------------
#include "../presentation.h"
#include "../../rgl/text_stream.h"
struct MapMesh {
    char build(void* grid, int mode);                   // 0x4785e0 (ret 8)
    void polygon_to_mesh(int v);                                     // 0x4e75a0 (ret 4)
    void build_ribbons(int v);                                     // 0x4e6a70 (ret 4)
    void build_spikes(int v);                                     // 0x4e65e0 (ret 4)
};

// ---- callees --------------------------------------------------------------------------------------------
#include "../terrain_desc.h"
int  save_game(int arg, tstring* name);                 // 0x4e8980
#include "../slot.h"                                // Slot, g_keyboard (0xa170c8)
#include "../../include/render_flags.h"
void log_save_path();                                   // 0x4e8df0

extern int      g_use_shaders;                          // 0x5e28fc

#include "../window_mgr.h"                              // the save-game menu screens are slots of g_windows: 18 (0xa739c8), 21 (0xa739d4)

#endif
