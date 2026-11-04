#ifndef SCREEN_H
#define SCREEN_H
// Slice s004ebc10: the frame driver of the 0x4e... screen module (mission/map screen).
//   0x4ebc10 take_screenshot   builds the next free "mb<N>.bmp" path, saves the frame (0x4bb9c0), logs the
//                              translated result message.
//   0x4ebdc0 ~UiRow            destroys one 0x20-byte row (units array 0x4d10f0 + free, tstring at +0).
//   0x4ebe10 append_rows       copy-constructs [first,last) into dest (0x4e42f0), returning the new end.
//   0x4ebe7a unwind_rows       the EH cleanup of append_rows: calls ~UiRow on [saved, current) then rethrows.
//   0x4ebea0 GuiScreen::GuiScreen    ctor of the screen object (1.1 MB: 6 "hint" vector tables, 3 std::vectors,
//                              vec_table 0x417cb0, rgl::mesh, scene, a 0x800-byte matrix array).
//   0x4ebfd0 GuiScreen::process   per-frame mouse/camera update and key-message dispatch.
//   0x4ec460 frame_hook        the engine's end-of-frame callback: screenshot / leave-mission triggers.
// Names and field names are provisional; offsets come from this function and the neighbouring slices.
#include <windows.h>
#include <stdio.h>
#include <new>
#include <d3d9.h>
#include <d3dx9.h>
#include "../../rgl/rgl_math.h"
#include "./cs_common.h"
#include "./cs_mat.h"

using rgl::vector4;

#include "../../include/mat4.h"
using rgl::mat4;
using rgl::mat3;
// (identity3 0x40cd60: cs_mat.h)

#include "../../rgl/mesh.h"
using rgl::mesh;

#include "../tstring.h"

#include "../str_table.h"
#include "../key_names.h"
#include "../str_table.h"                          // StrTable g_strtable (0x5e2a18)
#include "../strat/gui_manager.h"

bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format);  // 0x4bb9c0 (2-arg cdecl)
void take_screenshot(void);
#include "../../rgl/text_stream.h"                  // mb_snprintf 0x415100
#include "../../include/crt.h"
#include "../float_consts.h"
// (0x4d10f0 is item_modifier_array_dtor, 0x4e42f0 allocator<RagModel>::construct: cs_common.h)
// the MSVC 7.1 formulation of _AddressOfReturnAddress
#define _AddressOfReturnAddress() ((void*)((char*)&_AddressOfReturnAddress_ + 4))
int _AddressOfReturnAddress_;

// 0x20-byte row of the screen's unit list.
struct UiRow {
    tstring name;                       // +0
    int pad10;
    int pad14;
    int pad18;
    int pad1c;
    UiRow* build(const UiRow* src) throw(...);   // 0x4e42f0 (thiscall, ret 8: copy-constructs from src)
                                          // returns `this`, so the loop's iterator update is one reg)
    void destroy();                     // 0x4e4310 (thiscall: the tstring destructor body)
    ~UiRow();                           // 0x4ebdc0
};

// the vector's allocator (an empty class): both members are the out-of-line template instantiations.
struct RowAlloc {
    void construct(UiRow* dst, const UiRow& src);   // 0x4e42f0
    // (destroy 0x4ebdc0 is the free stdcall function of screen.cpp)
};

// the row's 0x10-byte model record (reached as this+0x10) and its deallocator 0x4d10f0: `zero` is the
// first pointer of a std::vector-like member, `count` a tstring-like pointer freed with free().
struct RowModel {
    int zero;                           // row+0x10 (nulled by ~UiRow)
    int pad14;                          // row+0x14 (nulled by ~UiRow)
    int count;                          // row+0x18
    int pad1c;                          // row+0x1c (nulled by ~UiRow)
    void __fastcall erase_all(int unused);
};

void __fastcall RowModel::erase_all(int unused)   // 0x4d10f0
{
    if (zero != pad1c) {
        if (count) {
            free((void*)count);
            count = 0;
        }
        pad14 = 0;
        zero = 0;
        ((tstring*)((char*)this - 0x10))->~tstring();
    }
}
#include "../../include/vec_table.h"     // vec_table (8 bytes: data, count; ctor 0x417cb0). It was a 0x260-byte stand-in here.
#include "../../include/clock_table.h"
#include "../ui_globals.h"      // UI / core globals

#include "../../include/timer.h"   // Timer (elapsed 0x432c10: seconds since the record was touched)


// 0x4babf0 is a file-local helper: cl's register convention for `static` functions hands it its out
// pointer in ESI. Defining it here (body shape taken from 0x4babf0) makes the caller emit
// `lea esi,[esp+0x18]; call ...` instead of a normal stack argument.
extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(void*, int, void*, unsigned long, char*);
__declspec(noinline) static void screenshots_path(char* out)
{
    char home[0x104];
    char path[0x104];
    SHGetFolderPathA(0, 5, 0, 0, home);
    sprintf(path, "%s\\Mount&Blade Screenshots", home);
    CreateDirectoryA(path, 0);
    sprintf(path, "%s", out);
    sprintf(out, "%s\\Mount&Blade Screenshots\\%s", home, path);
}

using rgl::scene;

struct GameMgr;

// ---- globals --------------------------------------------------------------------------------------------







// the DirectInput device at 0xa170d0 is a C-style COM object: its vtable holds function pointers that
// take the device as their first (stack) argument.  Only Acquire (+0x1c) and GetDeviceData (+0x28) are used.
struct DiVtbl {
    void* d00; void* d01; void* d02; void* d03; void* d04; void* d05; void* d06;
    long (__stdcall* Acquire)(void* self);                                          // +0x1c
    void* d08; void* d09;
    long (__stdcall* GetDeviceData)(void* self, unsigned size, void* buf,
                                    unsigned* count, unsigned flags);                // +0x28
};
struct DiDev { DiVtbl* vtbl; };


// input records handed back by GetDeviceData (DIDEVICEOBJECTDATA, 0x14 bytes each).
struct DiObjectData {
    unsigned ofs;                       // +0x0 (the input object: 0/4/8 accumulate, 0xc/0xd/0xe are keys)
    unsigned data;                      // +0x4 (low byte signed: < 0 means the key went down)
    unsigned time;                      // +0x8
    unsigned seq;                       // +0xc
    unsigned app;                       // +0x10
};

#include "../../include/clock_table.h"   // g_ticks (0x5e2948), read as float pairs by the cast
extern float g_four_tenths;             // 0x54b3b8  0.4f

extern float zero;                      // 0x549ca8  0.0f
extern float g_eps;                     // 0x54a6f0  0.01f
extern float g_ninety_nine;             // 0x54a5cc  0.99f
extern float g_seventy_four;            // 0x5985f4  0.74f

// (leave_module 0x4ebb80 is menu_variables_loaded: cs_common.h)

#endif
