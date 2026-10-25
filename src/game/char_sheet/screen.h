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
#include "rgl.h"
#include "./cs_common.h"
#include "./cs_mat.h"

using rgl::vector4;

#include "../../include/mat4.h"
using rgl::mat4;
using rgl::mat3;
// (identity3 0x40cd60: cs_mat.h)

#include "../rgl_mesh.h"
using rgl::mesh;

#include "../tstring.h"

#include "../str_table.h"
extern StrTable g_strtable;                             // 0x5e2a18
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                                 // 0x5d7080

bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format);  // 0x4bb9c0 (2-arg cdecl)
void take_screenshot(void);
#include "../../rgl/text_stream.h"                  // mb_snprintf 0x415100
// (0x4d10f0 is item_modifier_array_dtor, 0x4e42f0 allocator<RagModel>::construct: cs_common.h)
void __cdecl __CxxThrowException(void* p, void* info);  // 0x5368e6
// the MSVC 7.1 formulation of _AddressOfReturnAddress
#define _AddressOfReturnAddress() ((void*)((char*)&_AddressOfReturnAddress_ + 4))
int _AddressOfReturnAddress_;

// 0x98-byte "hint" element of the six vector tables (ctor 0x4cd720).
struct Obj720 {                         // (the slice's Hint)
    char pad0[0x3c];
    tstring name;                       // +0x3c
    char pad4c[0x98 - 0x4c];
    Obj720();                           // 0x4cd720
};
typedef Obj720 Hint;

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
    int zero;                           // +0x10 (nulled by ~UiRow)
    int pad14;                          // +0x14 (nulled by ~UiRow)
    int count;                          // +0x18
    int pad1c;                          // +0x1c (nulled by ~UiRow)
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

// a matrix-array entry of the screen: 3 identity rows + a 4th scale vector (0x40 bytes).
struct xform {
    mat3 m;                             // +0 (identity3 output, 0x40cd60: 12 dwords)
    vector4 ex;                         // +0x30
};
#define MAT_XFORM(d) ((xform*)((char*)*(xform**)((char*)this + 0x4183d4) + (d)))

struct Timer {
    float elapsed() const;   // 0x432c10 (seconds since the record was touched; include/timer.h)
};

// ---- the module's string table heads (the 0x8ee3c3c slot read by 0x4ebc10) --------------------------
extern char* g_save_prefix;             // 0x8ee3c3c (the capture-file prefix; null before init)
extern int g_module_name;                   // 0x8ee3c38

// 0x4babf0 is a file-local helper: cl's register convention for `static` functions hands it its out
// pointer in ESI. Defining it here (body shape taken from 0x4babf0) makes the caller emit
// `lea esi,[esp+0x18]; call ...` instead of a normal stack argument.
extern "C" int _sprintf(char* buf, const char* fmt, ...);   // 0x536f8f
extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(void*, int, void*, unsigned long, char*);
__declspec(noinline) static void screenshots_path(char* out)
{
    char home[0x104];
    char path[0x104];
    SHGetFolderPathA(0, 5, 0, 0, home);
    _sprintf(path, "%s\\Mount&Blade Screenshots", home);
    CreateDirectoryA(path, 0);
    _sprintf(path, "%s", out);
    _sprintf(out, "%s\\Mount&Blade Screenshots\\%s", home, path);
}

using rgl::scene;

// the active overlay (vtable slot +0x8c receives key codes, +0x40 resets it).
struct CurObj;

// one mouse-button record: the table index plus the last click position read from the pair table.
struct ClickRec {
    int idx;                            // +0x0 index into the 0x5e2948 float-pair table
    int pad4;                           // +0x4
    float x;                            // +0x8
    float y;                            // +0xc
    // (elapsed 0x432c10, seconds since the record was touched, is Timer's: cs_common.h)
};

struct GuiScreen {
    void* vptr;                         // +0x00
    CurObj** obj_first;                 // +0x04 (std::vector<CurObj*> first/last/end)
    CurObj** obj_last;                  // +0x08
    CurObj** obj_end;                   // +0x0c
    CurObj* cur;                        // +0x10
    float mx;                           // +0x14 cursor x (0..1)
    float my;                           // +0x18 cursor y (0..1)
    float dmx;                          // +0x1c dx this frame
    float dmy;                          // +0x20 dy this frame
    float ax;                           // +0x24 accumulated mouse offset x
    float ay;                           // +0x28 accumulated mouse offset y
    float prev_mx;                      // +0x2c cursor x before this frame
    float prev_my;                      // +0x30 cursor y before this frame
    int prev_cx;                        // +0x34 raw cursor x before GetCursorPos
    int prev_cy;                        // +0x38 raw cursor y before GetCursorPos
    char pad3c[0x3a8 - 0x3c];
    char pad3a8[0x514 - 0x3a8];
    float f514;                         // +0x514 vertical scale
    ClickRec click3;                    // +0x518 (input offset 0xc: last left-click)
    ClickRec click2;                    // +0x528 (input offset 0xd: last right-click)
    ClickRec click1;                    // +0x538 (input offset 0xe: last middle-click)
    int view_x;                         // +0x548
    int view_h;                         // +0x54c
    int view_y;                         // +0x550
    int view_w;                         // +0x554
    char pad558[0x1708 - 0x558];
    char f1708;                         // +0x1708 (keyboard cursor mode: stop processing)
    char f1709;                         // +0x1709
    char pad170a[6];
    int f1710;                          // +0x1710
    int f1714;
    int f1718;                          // +0x1718 (>0 and cur: send 0x1000)
    char pad171c[0xb0b0 - 0x171c];
    int cur_mx;                         // +0xb0b0 (GetCursorPos)
    int cur_my;                         // +0xb0b4

    // (wheel_step 0x4c52f0 and clear_selection 0x4c5330 are gui_view's, post 0x4e6510 is MsgLog::update: cs_common.h / msg_log.h)

    GuiScreen();                                               // 0x4ebea0
    void process(int param);                                // 0x4ebfd0
};

// the two inlined std::vector idioms, kept as free helpers so `this` stays in ESI.
static __forceinline int vec_size(GuiScreen* s)
{
    return s->obj_first ? (int)(s->obj_last - s->obj_first) : 0;
}
static __forceinline CurObj* vec_back(GuiScreen* s)
{
    return s->obj_first[vec_size(s) - 1];
}

// every element of the GuiScreen::obj vector exposes the per-frame update at vtable slot +0x8.
struct ScreenVT {
    virtual void s0(int); virtual void s1(int); virtual void update(int);
};

struct CurObj {
    virtual void c00(int); virtual void c01(int); virtual void c02(int); virtual void c03(int);
    virtual void c04(int); virtual void c05(int); virtual void c06(int); virtual void c07(int);
    virtual void c08(int); virtual void c09(int); virtual void c10(int); virtual void c11(int);
    virtual void c12(int); virtual void c13(int); virtual void c14(int); virtual void c15(int);
    virtual void load();                // +0x40 (slot 16)
    virtual void c16(int); virtual void c17(int); virtual void c18(int); virtual void c19(int);
    virtual void c20(int); virtual void c21(int); virtual void c22(int); virtual void c23(int);
    virtual void c24(int); virtual void c25(int); virtual void c26(int); virtual void c27(int);
    virtual void c28(int); virtual void c29(int); virtual void c30(int); virtual void c31(int);
    virtual void c32(int); virtual void c33(int);
    virtual void key(int code);         // +0x8c (slot 35)
};

struct GameMgr;
extern GameMgr g_game;                  // 0x5e2930

// ---- globals --------------------------------------------------------------------------------------------
extern int g_screenshots_counter;       // 0xa70440

extern int g_module_name;                   // 0x8ee3c38
extern int g_5a9fb0;                    // 0x5a9fb0 (camera enabled)
extern int g_a17148;                    // 0xa17148 (0 when the mission screen owns the input)
extern int g_a17348;                    // 0xa17348
extern int g_a1741c;                    // 0xa1741c (screenshot requested)
extern int g_a1781c;                    // 0xa1781c (screenshot already done)
extern int g_a1717c;                    // 0xa1717c
extern int g_a171ac;                    // 0xa171ac
extern int g_a17190;                    // 0xa17190
extern int g_a17590;                    // 0xa17590
extern float g_a178d4;                  // 0xa178d4 mouse dx
extern float g_a178d8;                  // 0xa178d8 mouse dy

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
extern DiDev* g_a170d0;                 // 0xa170d0 (input device, may be null)

// input records handed back by GetDeviceData (DIDEVICEOBJECTDATA, 0x14 bytes each).
struct DiObjectData {
    unsigned ofs;                       // +0x0 (the input object: 0/4/8 accumulate, 0xc/0xd/0xe are keys)
    unsigned data;                      // +0x4 (low byte signed: < 0 means the key went down)
    unsigned time;                      // +0x8
    unsigned seq;                       // +0xc
    unsigned app;                       // +0x10
};

extern float g_ticks[];                // 0x5e2948 (float pairs: x at [i*2], y at [i*2+1])
extern float g_four_tenths;             // 0x54b3b8  0.4f

extern float zero;                      // 0x549ca8  0.0f
extern float g_one;                     // 0x549cdc  1.0f
extern float g_eps;                     // 0x54a6f0  0.01f
extern float g_ninety_nine;             // 0x54a5cc  0.99f
extern float g_seventy_four;            // 0x5985f4  0.74f

// (leave_module 0x4ebb80 is menu_variables_loaded: cs_common.h)

#endif
