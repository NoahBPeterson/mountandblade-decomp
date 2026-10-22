// Slice s004ebc10: the frame driver of the 0x4e... screen module (mission/map screen).
//   0x4ebc10 take_screenshot   builds the next free "mb<N>.bmp" path, saves the frame (0x4bb9c0), logs the
//                              translated result message.
//   0x4ebdc0 ~UiRow            destroys one 0x20-byte row (units array 0x4d10f0 + free, tstring at +0).
//   0x4ebe10 append_rows       copy-constructs [first,last) into dest (0x4e42f0), returning the new end.
//   0x4ebe7a unwind_rows       the EH cleanup of append_rows: calls ~UiRow on [saved, current) then rethrows.
//   0x4ebea0 Screen::Screen    ctor of the screen object (1.1 MB: 6 "hint" vector tables, 3 std::vectors,
//                              vec_table 0x417cb0, rgl::mesh, scene, a 0x800-byte matrix array).
//   0x4ebfd0 Screen::process   per-frame mouse/camera update and key-message dispatch.
//   0x4ec460 frame_hook        the engine's end-of-frame callback: screenshot / leave-mission triggers.
// Names and field names are provisional; offsets come from this function and the neighbouring slices.
#include <windows.h>
#include <stdio.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

struct mat4 { vector4 s, f, u, o; };
struct mat3 { vector4 s, f, u; };
mat3* identity3(mat3* out);             // 0x40cd60 (cdecl; returns out; 12 dwords)

struct mesh {                           // 0x260-byte mesh instance, ctor 0x412a90
    char pad[0x260];
    mesh();
};

struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                          // 0x42bbd0
    tstring(const char* s);             // 0x42bbe0
    tstring(const tstring& o);          // 0x42c110
    ~tstring();                         // 0x42bc40
};

struct StrTable {
    tstring* lookup(const tstring& key);                // 0x43a1e0
    tstring* at(int i) { return (tstring*)this + i; }
};
extern StrTable g_strtable;                             // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };    // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                                 // 0x5d7080

bool __cdecl save_screenshot(const char* file_name, int flag);  // 0x4bb9c0 (2-arg cdecl)
void take_screenshot(void);
void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100
void free_rows(void* first, void* last);                // 0x4d10f0
void __cdecl destroy_row_range(void* first, void* model, void* last, void* self);   // 0x4d10f0
void copy_row(void* dst, const void* src);              // 0x4e42f0 (thiscall, ret 8)
void __cdecl __CxxThrowException(void* p, void* info);  // 0x5368e6
// the MSVC 7.1 formulation of _AddressOfReturnAddress
#define _AddressOfReturnAddress() ((void*)((char*)&_AddressOfReturnAddress_ + 4))
int _AddressOfReturnAddress_;

// 0x98-byte "hint" element of the six vector tables (ctor 0x4cd720).
struct Hint {
    char pad0[0x3c];
    tstring name;                       // +0x3c
    char pad4c[0x98 - 0x4c];
    Hint();                             // 0x4cd720
};

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
    void destroy(UiRow* p);                         // 0x4ebdc0
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
struct vec_table {                      // ctor 0x417cb0
    char pad0[0x260];
    vec_table();                        // 0x417cb0
};

// a matrix-array entry of the screen: 3 identity rows + a 4th scale vector (0x40 bytes).
struct xform {
    mat3 m;                             // +0 (identity3 output, 0x40cd60: 12 dwords)
    vector4 ex;                         // +0x30
};
#define MAT_XFORM(d) ((xform*)((char*)*(xform**)((char*)this + 0x4183d4) + (d)))

// ---- the module's string table heads (the 0x8ee3c3c slot read by 0x4ebc10) --------------------------
extern char* g_save_prefix;             // 0x8ee3c3c (the capture-file prefix; null before init)
extern int g_8ee3c38;                   // 0x8ee3c38

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

struct scene {
    char pad0[0x1d8];
    scene();                            // 0x429740
};

// the active overlay (vtable slot +0x8c receives key codes, +0x40 resets it).
struct CurObj;

// one mouse-button record: the table index plus the last click position read from the pair table.
struct ClickRec {
    int idx;                            // +0x0 index into the 0x5e2948 float-pair table
    int pad4;                           // +0x4
    float x;                            // +0x8
    float y;                            // +0xc
    float elapsed();                    // 0x432c10 (seconds since the record was touched)
};

struct Screen {
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

    void key_flag(int code);                                // 0x4c52f0 (records a key in the hint tables)
    void post(int param);                                   // 0x4e6510
    void finish();                                          // 0x4c5330

    Screen();                                               // 0x4ebea0
    void process(int param);                                // 0x4ebfd0
};

// the two inlined std::vector idioms, kept as free helpers so `this` stays in ESI.
static __forceinline int vec_size(Screen* s)
{
    return s->obj_first ? (int)(s->obj_last - s->obj_first) : 0;
}
static __forceinline CurObj* vec_back(Screen* s)
{
    return s->obj_first[vec_size(s) - 1];
}

// every element of the Screen::obj vector exposes the per-frame update at vtable slot +0x8.
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

extern int g_8ee3c38;                   // 0x8ee3c38
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
struct InputRec {
    unsigned ofs;                       // +0x0 (the input object: 0/4/8 accumulate, 0xc/0xd/0xe are keys)
    unsigned data;                      // +0x4 (low byte signed: < 0 means the key went down)
    unsigned time;                      // +0x8
    unsigned seq;                       // +0xc
    unsigned app;                       // +0x10
};

extern float g_5e2948[];                // 0x5e2948 (float pairs: x at [i*2], y at [i*2+1])
extern float g_four_tenths;             // 0x54b3b8  0.4f

extern float zero;                      // 0x549ca8  0.0f
extern float g_one;                     // 0x549cdc  1.0f
extern float g_eps;                     // 0x54a6f0  0.01f
extern float g_ninety_nine;             // 0x54a5cc  0.99f
extern float g_seventy_four;            // 0x5985f4  0.74f

void __cdecl leave_module(char flag);       // 0x4ebb80

// @ 0x4ebc10
// Saves the next free screenshot file, then logs "ui_screenshot_taken_to_value" with the file name, or
// "ui_screenshot_failed" when the save failed.
void __cdecl take_screenshot(void)
{
    char dir[260];
    char name[512];
    WIN32_FIND_DATAA fd;
    char msg[1024];
    tstring* s;
    HANDLE h;

    _sprintf(dir, "", g_save_prefix ? g_save_prefix : (char*)&g_save_prefix);
    screenshots_path(dir);
    _sprintf(name, "%s%s", dir, g_save_prefix ? g_save_prefix : (char*)&g_save_prefix);
    CreateDirectoryA(name, 0);
    for (;;) {
        g_screenshots_counter++;
        mb_snprintf(name, 0x200, "%s%s\\mb%d.bmp", dir,
                    g_save_prefix ? g_save_prefix : (char*)&g_save_prefix,
                    g_screenshots_counter);
        h = FindFirstFileA(name, &fd);
        if (h == (HANDLE)-1)
            break;
        FindClose(h);
    }

    if (save_screenshot(name, 0)) {
        {
            tstring key("ui_screenshot_taken_to_value");
            s = g_strtable.lookup(key);
            _sprintf(msg, s->buf ? s->buf : (char*)&s->buf, name);
        }
        g_msglog.add(tstring(msg), -1);
    } else {
        tstring key("ui_screenshot_failed");
        g_msglog.add(*g_strtable.lookup(key), -1);
    }
}

// 0x4d10f0 as the row destructor uses it: (first, model = this+0x10, last, this).
void __cdecl destroy_row_range(void* first, void* model, void* last, void* self);

// The member at +0x10 is the row's model: [0]=first, [4]=buffer (freed), [8]=last.
// @ 0x4ebdc0
void __stdcall dtor_ui_row(UiRow* param)
{
    UiRow* self = param;
    int* l = &self->pad10;
    if (l[1]) {
        // arg 4 is a distinct 4-arg instantiation ICF-folded with the 2-arg range destructor.
        destroy_row_range((void*)l[1], (void*)l[2], l, *(UiRow* const volatile*)&param);
        ::operator delete((void*)l[1]);
    }
    l[1] = 0;
    l[2] = 0;
    l[3] = 0;
    self->name.~tstring();
}

// @ 0x4ebe10
// VC7.1 <memory> _Uninit_copy: copy-construct [first,last) into raw dest through the row allocator,
// destroying the constructed prefix and rethrowing if a copy ctor throws. The frame slot holding the
// original dest is what lets the compiler's cleanup funclet at 0x4ebe7a walk [next,dest).
UiRow* __cdecl append_rows(UiRow* first, UiRow* last, UiRow* dest, RowAlloc& al)
{
    UiRow* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            al.destroy(next);
        throw;
    }
    return dest;
}

// @ 0x4ebea0
Screen::Screen()
{
    for (int i = 0; i < 5; i++)
        ((Hint*)((char*)this + 0x14))[i].Hint::Hint();
    ((Hint*)((char*)this + 0x30c))->Hint::Hint();
    *(int*)((char*)this + 0x3a8) = 0;
    *(int*)((char*)this + 0x3ac) = 0;
    *(int*)((char*)this + 0x3b0) = 0;
    ((vec_table*)((char*)this + 0x4183d8))->vec_table::vec_table();
    ((mesh*)((char*)this + 0x4183e0))->mesh::mesh();
    ((scene*)((char*)this + 0x418640))->scene::scene();
    *(xform**)((char*)this + 0x4183d4) = new xform[0x20];
    mat3 m;
    xform p;
    p.ex = vector4(0, 0, 0);
    for (int j = 0; j < 0x800; j += 0x40) {
        p.m = *identity3(&m);
        *MAT_XFORM(j) = p;
        xform* d = MAT_XFORM(j);
        d->m.s.w = 0;
        d->m.f.w = 0;
        d->m.u.w = 0;
        d->ex.w = 1.0f;
    }
    *(int*)((char*)this + 0x4183c4) = 0;
    *(int*)((char*)this + 0x4183c0) = 0;
    *(int*)((char*)this + 0x418828) = 0;
}

// @ 0x4ebfd0
// One input pass: resets the top overlay, clamps the mouse delta against the screen edges, drains
// DirectInput (GetDeviceData, re-acquiring while the device is unacquired), maps the raw cursor into the
// 0..1 view rectangle, then dispatches every record to the key-flag path (0x4c52f0) or the active
// overlay (+0x8c), and finally fans the frame out to the screen vector, 0x4e6510 and 0x4c5330.
void Screen::process(int param)
{
    InputRec buf[16];
    unsigned count;
    unsigned i;
    int acc0, acc1;
    ClickRec* cr;

    vec_back(this)->load();

    this->dmx = 0.0f;
    this->dmy = 0.0f;
    if (this->mx <= g_eps) {
        if (g_a178d4 < 0.0f) {
            this->dmx = g_a178d4;
            this->ax = this->ax + g_a178d4;
        }
    }
    else if (this->mx >= g_ninety_nine) {
        if (g_a178d4 > 0.0f) {
            this->dmx = g_a178d4;
            this->ax = this->ax + g_a178d4;
        }
    }
    else {
        this->ax = 0.0f;
    }
    if (this->my <= g_eps) {
        if (g_a178d8 > 0.0f) {
            this->dmy = -g_a178d8;
            this->ay = this->ay - g_a178d8;
        }
    }
    else if (this->my >= g_seventy_four) {
        if (g_a178d8 < 0.0f) {
            this->dmy = -g_a178d8;
            this->ay = this->ay - g_a178d8;
        }
    }
    else {
        this->ay = 0.0f;
    }

    count = 0x10;
    if (g_a170d0) {
        if (g_a170d0->vtbl->GetDeviceData(g_a170d0, 0x14, buf, &count, 0)) {
            while (g_a170d0->vtbl->Acquire(g_a170d0) == (long)0x8007001e)
                ;
        }
    }

    this->prev_cx = this->cur_mx;
    this->prev_cy = this->cur_my;
    GetCursorPos((POINT*)&this->cur_mx);
    this->prev_mx = this->mx;
    this->mx = (float)(this->cur_mx - this->view_w) / (float)this->view_x;
    this->prev_my = this->my;
    this->my = (g_one - (float)(this->cur_my - this->view_y) / (float)this->view_h) * this->f514;

    if (this->cur) {
        this->cur->key(1);
    }
    else if (vec_size(this)) {
        vec_back(this)->key(1);
    }

    acc0 = 0;
    acc1 = 0;
    this->f1710 = 0;
    this->f1708 = 0;

    {
        unsigned* rec = (unsigned*)buf + 1;
        for (i = 0; i < count; i++, rec += 5) {
            unsigned type;
            if (this->f1708)
                break;
            type = rec[-1];
            if (type == 0) {
                acc0 += (int)rec[0];
                continue;
            }
            if (type == 4) {
                acc1 += (int)rec[0];
                continue;
            }
            if (type == 8) {
                this->f1710 += (int)rec[0];
                continue;
            }
            if (acc0 || acc1) {
                if (this->cur) {
                    this->cur->key(1);
                }
                else if (vec_size(this)) {
                    vec_back(this)->key(1);
                }
                acc0 = 0;
                acc1 = 0;
            }
            switch (type) {
            case 0xc:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x10);
                    cr = &this->click3;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x200);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(8);
                }
                break;
            case 0xd:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x40);
                    cr = &this->click2;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x400);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(0x20);
                }
                break;
            case 0xe:
                if ((char)rec[0] < 0) {
                    this->key_flag(0x100);
                    cr = &this->click1;
                    if (cr->elapsed() < g_four_tenths) {
                        this->key_flag(0x800);
                    }
                    else {
                        cr->x = g_5e2948[cr->idx * 2];
                        cr->y = g_5e2948[cr->idx * 2 + 1];
                    }
                }
                else {
                    this->key_flag(0x80);
                }
                break;
            }
        }
    }

    if (!this->f1708 && (acc0 || acc1)) {
        if (this->cur) {
            this->cur->key(1);
        }
        else if (vec_size(this)) {
            vec_back(this)->key(1);
        }
    }

    if (this->cur && this->f1718 > 0)
        this->cur->key(0x1000);

    for (i = 0; i < (unsigned)vec_size(this); i++)
        ((ScreenVT*)this->obj_first[i])->update(param);

    this->post(param);
    if (this->f1709)
        this->finish();
}

// @ 0x4ec460
// The engine's end-of-frame callback. When a screenshot was requested it saves the frame; otherwise, with a
// camera present and a module-leave pending, it leaves the current module.
void __cdecl frame_hook(void)
{
    if ((g_a17148 || g_a17348) && g_a1741c && !g_a1781c)
        take_screenshot();
    else if (g_5a9fb0 && (g_a17148 || g_a17348) && (g_a1717c || g_a171ac) && g_a17190 && !g_a17590)
        leave_module(0);
}
