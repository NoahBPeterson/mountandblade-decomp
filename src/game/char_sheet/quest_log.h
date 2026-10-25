#ifndef QUEST_LOG_H
#define QUEST_LOG_H
// Slice s004ee230: the quest log window (the singleton lives at 0x5d7080).
//   0x4ee230  build          - the window, its two panels, the tab texts, the quest rows, the scrollbar and the list
//   0x4ee840  refresh        - refills indices[] (+0x4c) / count (+0x24c) with the quests of the current tab
//   0x4ee910  make_scrollbar - 0x188-byte scrollbar widget
//   0x4eea90  make_quest_list - 0x4a8-byte quest list widget
//
// Match status: none of the four functions is byte-exact; all four are complete. What the disassembly
// pins down, and what makes the bytes unreachable with cl 13.10:
//  - 0x4ee230 is framed like a /Gh function: `and esp,-8; sub esp,0x248` with `this` taken from ecx into
//    esi before the first call, plus a `xor ebx,ebx` zero register that is reused for every 0 pointer.
//    cl 13.10 emits that frame only for functions it must inline-protect; the same body written as a
//    member compiles to a plain `sub esp` frame with `mov ebp,ecx`.
//  - every `new` in the slice uses the EH form of operator new (??2@YAPAXI@Z) followed by
//    `mov ecx,eax / call ctor`; `new T` from source resolves to `push eax / call eax-ctor / add esp,4`.
//  - the widget's later slots (+0x80 add_item, +0x98/+0x9c values, +0xa4 set_text, +0xc8 add_child) are
//    called with `add eax,<imm>` then an indirect call, an encoding the compiler does not select here.
// Class, member and function names are provisional.
#include <stdio.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
#include "../tstring.h"
#include "../rgl_mesh.h"
using rgl::mesh;
#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
#include "./cs_common.h"                        // widget constructors, gui_manager, NodeVec, MissionPanel, new_medium_button, ...

void __cdecl mb_error(const char* fmt, ...);            // 0x42cc80
#include "../../rgl/text_stream.h"                // mb_snprintf 0x415100
#include "../str_table.h"
extern StrTable g_strtable;
extern resmgr g_resmgr;

// ---- constants ----------------------------------------------------------------------------------------------
extern const float g_54ab1c;            // 0x54ab1c  0.04f
extern const float g_54b4d0;            // 0x54b4d0  0.56f
extern const float g_591d54;            // 0x591d54  0.45f
extern const float g_5987e4;            // 0x5987e4  1.0f
extern const float g_590b70;            // 0x590b70  1.26f
extern const float g_592758;            // 0x592758  0.48f
extern const float g_592754;            // 0x592754  0.155f

// ---- the gui widget ------------------------------------------------------------------------------------------
// The base supplies slots up to set_pos (+0x58) / set_size (+0x64); the concrete window classes insert their own
// slots in between, so the later slots are reached through the raw vtable.
#include "../widget.h"                        // widget (the 0x178-byte UI element; shared with facegen_init.h, view_open.cpp, widget_text.cpp, strat/gui_manager.h)

// slots addressed by byte offset: ((void**)self[0])[off / 4]
#define VCALL0(self, off)               ((void (*)(void*))((void**)*(void***)(self))[(off) / 4])(self)
#define VCALL1(self, off, a)            ((void (*)(void*, int))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALLP(self, off, a)            ((void (*)(void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALL2(self, off, a, b)         ((void (*)(void*, const void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a), (b))

// (the constructors 0x48ae20 MissionUi, 0x4931a0 ContainerW::construct, 0x48b890 UiD, 0x49deb0 c49deb0::Camera, draw 0x488d50, 0x49b120 NodeVec::push_back,
// new_medium_button 0x4dce70 are cs_common.h's)
mesh* make_background(float* col, int rgb);              // 0x4de320

// ---- the quest database (g_game = 0x5e2930) ----------------------------------------------------------------
extern int g_game[];                                    // 0x5e2930
extern char g_frame_flag;                               // 0x5d8788
#define QUEST_COUNT (*(int*)((char*)g_game + 0x13a5c))
#define QUESTS      ((char*)(*(int*)((char*)g_game + 0x13a58)))
#define QUEST(i)    (QUESTS + (i) * 0x138)

// ---- the quest log window ----------------------------------------------------------------------------------
struct quest_window {
    char pad000[4];
    int mode;                           // +0x04  0 = active quests, else finished
    char pad008[0x2c - 8];
    widget* window;                     // +0x2c
    widget* done;                       // +0x30  the "done" text
    widget* active_tab;                 // +0x34
    widget* finished_tab;               // +0x38
    widget* list;                       // +0x3c
    widget* frame;                      // +0x40  the quest row frame
    widget* panel1;                     // +0x44
    widget* panel2;                     // +0x48
    int indices[0x80];                  // +0x4c
    int count;                          // +0x24c
    int f250;                           // +0x250
    widget* rows[0x80];                 // +0x254

    void __fastcall build();            // 0x4ee230
    void __fastcall refresh();          // 0x4ee840
};

#endif
