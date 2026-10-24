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

void __cdecl mb_error(const tstring* s);                // 0x42cc80
void __cdecl mb_snprintf(char* buf, int size, const char* fmt, ...);   // 0x415100
void gui_reset();                                       // 0x42d1b0 gui_manager::reset (0x5d7080)
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
struct widget {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);               // +0x58
    virtual void v23(); virtual void v24();
    virtual void set_size(const float* s);              // +0x64
    virtual void v26();
    virtual void set_color(const float* c);             // +0x6c
    virtual void v28();
    virtual void set_flags(const int* f);               // +0x74
    virtual void v30();
    virtual void add_item(int slot, mesh* m);           // +0x7c
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36();
    virtual void v37();
    virtual void set_value_a(int* v);                   // +0x98
    virtual void set_value_b(int* v);                   // +0x9c
    virtual void v40();
    virtual void set_text(tstring* s, const float* p);  // +0xa4
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45(); virtual void v46();
    virtual void v47(); virtual void v48(); virtual void v49();
    virtual void add_child(widget* c);                  // +0xc8
};

// slots addressed by byte offset: ((void**)self[0])[off / 4]
#define VCALL0(self, off)               ((void (*)(void*))((void**)*(void***)(self))[(off) / 4])(self)
#define VCALL1(self, off, a)            ((void (*)(void*, int))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALLP(self, off, a)            ((void (*)(void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a))
#define VCALL2(self, off, a, b)         ((void (*)(void*, const void*, const void*))((void**)*(void***)(self))[(off) / 4])(self, (a), (b))

widget* __fastcall ctor_panel(widget* mem);                        // 0x48ae20 (in place, returns this)
widget* __fastcall ctor_window(widget* mem);                       // 0x4931a0
widget* __fastcall ctor_scrollbar(widget* mem);                    // 0x48b890
widget* __fastcall ctor_list(widget* mem);                         // 0x49deb0
void __fastcall list_setup(widget* w);                             // 0x488d50
void gui_set_window(widget* w);                         // 0x49b120
widget* make_text(widget* parent, tstring* s, float w, float h, float x, float y);   // 0x4dce70
mesh* make_color(void* col, unsigned rgb);              // 0x4de320

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
