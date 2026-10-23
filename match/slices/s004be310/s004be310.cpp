// Slice s004be310: module-script / clip record helper layer - the record readers,
// constructors, destructors and the generic copy/fill helpers (STL-instantiated loops)
// shared by the module script tables. ALL NAMES PROVISIONAL.
// Flags: default /O2 /MT /Gy /TP (no EH frames in the original).
#include <windows.h>
#include <stdio.h>
#include "rgl.h"

void mb_error(const char* fmt, ...);                                   // 0x42cc80
extern "C" int __cdecl mb_sprintf(char* buf, unsigned size, const char* fmt, ...);  // 0x535717
void __cdecl free_mem(void* p);                                         // 0x53570d (operator delete[])

// ---- tstring (16-byte rgl string) ------------------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
    char equals(const char* s) const;           // 0x42bd10
    bool differs(const char* s) const;          // 0x42bdb0
    void underscores_to_spaces();               // 0x42bf80
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- stream --------------------------------------------------------------------------------
// vtable offsets that matter here: +0x38 r_float, +0x40 r_uint, +0x48 r_i64,
// +0x4c r_int, +0x54 r_tstr (same physical layout as s0045ff30; names provisional).
struct stream_base {
    virtual void s00() = 0;
    virtual void s01() = 0;
    virtual void s02() = 0;
    virtual void s03() = 0;
    virtual void s04() = 0;
    virtual void s05() = 0;
    virtual void s06() = 0;
    virtual void s07() = 0;
    virtual void s08() = 0;
    virtual void s09() = 0;
    virtual void s0a() = 0;
    virtual void s0b() = 0;
    virtual void s0c() = 0;
    virtual void s0d() = 0;
    virtual void r_float(float* v) = 0;         // +0x38
    virtual void s0f() = 0;                     // +0x3c
    virtual void r_uint(unsigned* v) = 0;       // +0x40
    virtual void s11() = 0;                     // +0x44
    virtual void r_i64(__int64* v) = 0;         // +0x48
    virtual void r_int(int* v) = 0;             // +0x4c
    virtual void s14() = 0;                     // +0x50
    virtual void r_tstr(tstring* v) = 0;        // +0x54
    virtual stream_base& w_cstr(const char* s) = 0;  // 10 (+0x28), used via g_log
};

// Text log stream (global object 0x5a9f60). Only slot 10 (+0x28, w_cstr) is used here.
// Abstract base + bodyless override keep cl from devirtualising the call: it emits a
// real indirect call through the vtable slot (+0x28), as in the original.
struct log_base {
    virtual void p00() = 0;
    virtual void p01() = 0;
    virtual void p02() = 0;
    virtual void p03() = 0;
    virtual void p04() = 0;
    virtual void p05() = 0;
    virtual void p06() = 0;
    virtual void p07() = 0;
    virtual void p08() = 0;
    virtual void p09() = 0;
    virtual log_base& w_cstr(const char* s) = 0;     // slot 10 (+0x28)
};
struct log_stream : log_base {
    virtual void p00();
    virtual void p01();
    virtual void p02();
    virtual void p03();
    virtual void p04();
    virtual void p05();
    virtual void p06();
    virtual void p07();
    virtual void p08();
    virtual void p09();
    virtual log_base& w_cstr(const char* s);
};
extern log_stream g_log;                            // 0x5a9f60
extern const char g_eol[];                          // 0x54b118

// Resource-manager object (0x5d6fa0): only the material lookup is used here.
struct resource_manager { int pad[33]; void* get_material(tstring* name); };  // 0x418a70
extern resource_manager g_resmgr;                   // 0x5d6fa0

// ---- block (0x1c bytes) + its reader 0x4607e0 ----------------------------------------------
struct BlockElem {
    unsigned id;            // +0x00
    int      pad04;
    int      n;             // +0x08
    int      pad0c;
    __int64  vals[16];      // +0x10
};
struct Block {
    BlockElem* items;       // +0x00
    int        count;       // +0x04
    char       done;        // +0x08
    tstring    text;        // +0x0c
    Block() { count = 0; items = 0; done = 0; }
    void read(stream_base* s);                  // 0x4607e0
};

// ---- menu item (0x68 bytes) -----------------------------------------------------------------
struct menu_item {
    tstring name;           // +0x00
    tstring f10;            // +0x10
    tstring f20;            // +0x20
    Block   cond;           // +0x30
    Block   conseq;         // +0x4c
    void read(stream_base* s, const tstring* menu);  // 0x4be310
    menu_item();                                    // 0x4be420
};

// ---- script record (0x34 bytes) -------------------------------------------------------------
struct rec34 {
    int     f00;            // +0x00
    tstring name;           // +0x04
    float   f14;            // +0x14
    Block   body;           // +0x18
    int read(stream_base* s);                   // 0x4be470
};

// ---- small record (0x20 bytes) --------------------------------------------------------------
struct rec20 {
    tstring name;           // +0x00
    void*   mat;            // +0x10
    void*   mat2;           // +0x14
    float   f18;            // +0x18
    int     f1c;            // +0x1c
    void read(stream_base* s);                  // 0x4be600
};

// ---- clip table (0x14c bytes): 13 slots of 0x18 bytes at +0x0c ---------------------------------
void __fastcall tstring_ctor(tstring* p);                       // 0x42bbd0
void __fastcall tstring_dtor(tstring* p);                       // 0x42bc40
struct raw_tstring {         // same layout as tstring, but plain data (no implicit ctor)
    int   pos;              // +0x00
    char* buf;              // +0x04
    int   cap;              // +0x08
    int   len;              // +0x0c
};
struct clip_slot {
    raw_tstring name;       // +0x00
    int     v10;            // +0x10
    char*   v14;            // +0x14
    clip_slot();                                // 0x4be720
};
struct clip_slot_raw {       // layout-identical POD used by clip_table's array
    raw_tstring name;       // +0x00
    int     v10;            // +0x10
    char*   v14;            // +0x14
};
struct clip_table {
    int       v00;          // +0x00
    int       v04;          // +0x04
    int       v08;          // +0x08
    clip_slot_raw slots[13];    // +0x0c
    int       v144;         // +0x144
    int       v148;         // +0x148
    clip_table();                               // 0x4be6b0
    ~clip_table();                              // 0x4be6f0
};

// ---- meta mission object (>= 0x2c9 bytes) ---------------------------------------------------
struct meta_mission {
    int   f000;             // +0x000
    int   f004;             // +0x004
    int   f008;             // +0x008
    int   f00c;             // +0x00c
    int   f010;             // +0x010
    int   f014;             // +0x014
    int   f018;             // +0x018
    int   f01c;             // +0x01c
    int   f020;             // +0x020
    int   f024;             // +0x024
    char  pad028[0x14];
    int   f03c;             // +0x03c
    char  f040;             // +0x040
    char  pad041[0xf7];
    int   f138;             // +0x138
    int   f13c;             // +0x13c
    int   f140;             // +0x140
    int   f144;             // +0x144
    int   f148;             // +0x148
    int   pad14c;
    int   f150;
    int   f154;             // +0x154
    int   f158;             // +0x158
    int   f15c;             // +0x15c
    char  pad160[8];
    int   f168;             // +0x168
    char  pad16c[0x154];
    int   f2c0;             // +0x2c0
    int   f2c4;             // +0x2c4
    char  f2c8;             // +0x2c8
    void init();   // 0x4be730 (WinLicense marker jmp) / body 0x4be742; void, not a ctor
};

// ---------- implementations ------------------------------------------------------------------

// @ 0x4be310
void menu_item::read(stream_base* s, const tstring* menu)
{
    char buf[0x200];
    s->r_tstr(&f20);
    cond.read(s);
    mb_sprintf(buf, 0x200, "menu %s item %s condition", menu->c_str(), f20.c_str());
    tstring tmp(buf);
    cond.text = tmp;
    s->r_tstr(&name);
    name.underscores_to_spaces();
    conseq.read(s);
    mb_sprintf(buf, 0x200, "menu %s item %s consequence", f20.c_str(), menu->c_str());
    tstring tmp2(buf);
    conseq.text = tmp2;
    s->r_tstr(&f10);
    f10.underscores_to_spaces();
}

// @ 0x4be420
menu_item::menu_item()
{
}

// @ 0x4be470
int rec34::read(stream_base* s)
{
    char buf[0x200];
    s->r_tstr(&name);
    s->r_float(&f14);
    body.read(s);
    mb_sprintf(buf, 0x200, "script: %s", name.c_str());
    body.text = buf;
    return f14 >= 0.0f;
}

// 0x5a9ea0: the 28 script-code names.
extern const char* const g_script_names[];      // 0x5a9ea0

// ---- name table load_used ---------------------------------------------------------------------
struct rec34_table {
    rec34* records;         // +0x00
    int    count;           // +0x04
    int    success;         // +0x08
    int    used[28];        // +0x0c
    void load_used();                           // 0x4be560
};

// @ 0x4be560
void rec34_table::load_used()
{
    for (int k = 0; k < 28; ++k) {
        int found = -1;
        int i = 0;
        for (; found == -1 && i < count; ++i) {
            if (records[i].name.equals(g_script_names[k]))
                found = i;
        }
        if (found == -1)
            ((log_base&)g_log).w_cstr("WARNING: UNABLE TO MAP GAME SCRIPT CODE: ").w_cstr(g_script_names[k]).w_cstr(g_eol);
        used[k] = found;
    }
}

// @ 0x4be600
void rec20::read(stream_base* s)
{
    s->r_tstr(&name);
    s->r_int(&f1c);
    tstring tmp;
    s->r_tstr(&tmp);
    mat = g_resmgr.get_material(&tmp);
    s->r_float(&f18);
    mat2 = 0;
    tstring tmp2;
    s->r_tstr(&tmp2);
    if (tmp2.differs("none"))
        mat2 = g_resmgr.get_material(&tmp2);
}

// @ 0x4be6b0
clip_table::clip_table()
{
    clip_slot_raw* p = slots;
    int n = 13;
    do {
        tstring_ctor((tstring*)&p->name);                       // 0x42bbd0
        p->v14 = 0;
        ++p;
    } while (--n);
    v04 = -100;
    v08 = -100;
    v00 = 0;
    v144 = 0;
    v148 = 0;
}

// @ 0x4be6f0
clip_table::~clip_table()
{
    clip_slot_raw* p = slots + 13;
    int n = 13;
    do {
        --p;
        if (p->v14)
            free_mem(p->v14);
        tstring_dtor((tstring*)&p->name);                       // 0x42bc40
    } while (--n);
}

// @ 0x4be720
clip_slot::clip_slot()
{
    tstring_ctor((tstring*)&name);                              // 0x42bbd0
}

// @ 0x4be730
void meta_mission::init()
{
    f014 = 0;
    f144 = 0;
    f138 = 0;
    f010 = 0;
    f008 = -1;
    f000 = -1;
    f004 = -1;
    f13c = -1;
    f140 = -1;
    f024 = 0;
    f01c = 0;
    f00c = -1;
    f154 = -1;
    f158 = -1;
    f15c = -1;
    f03c = 0x400;
    f168 = 0;
    f2c0 = 0;
    f2c4 = 0;
    f2c8 = 0;
    f040 = 0;
}

// ---- code table (0x4a ints) --------------------------------------------------------------------
struct code_table {
    int v[0x4a];                                                // +0x00
    void init();                                                // 0x4be7c0 (was: ctor)
};

// @ 0x4be7c0
void code_table::init()
{
    memset(v, -1, sizeof(v));
    v[0x00 / 4] = 0x11; v[0x08 / 4] = 0x1f; v[0x10 / 4] = 0x1e;
    v[0x18 / 4] = 0x20; v[0x20 / 4] = 0x21; v[0x28 / 4] = 0x39;
    v[0x30 / 4] = 0xe0; v[0x38 / 4] = 0xe1; v[0x40 / 4] = 0xee;
    v[0x48 / 4] = 0xef; v[0x50 / 4] = 0x2b; v[0x78 / 4] = 0x2e;
    v[0x80 / 4] = 0x17; v[0x88 / 4] = 0x19; v[0x90 / 4] = 0x10;
    v[0x98 / 4] = 0x26; v[0x58 / 4] = 0x0f; v[0x60 / 4] = 0x2a;
    v[0x68 / 4] = 0x29; v[0x70 / 4] = 0x13; v[0xa0 / 4] = 0x0e;
    v[0xa8 / 4] = 0x3b; v[0xb0 / 4] = 0x3c; v[0xb8 / 4] = 0x3d;
    v[0xc0 / 4] = 0x3e; v[0xc8 / 4] = 0x3f; v[0xd0 / 4] = 0x40;
    v[0xd8 / 4] = 0x41; v[0xe0 / 4] = 0x42; v[0xe8 / 4] = 0x43;
    v[0xf0 / 4] = 0x57; v[0xf8 / 4] = 2;    v[0x100 / 4] = 3;
    v[0x108 / 4] = 4;   v[0x110 / 4] = 5;   v[0x118 / 4] = 6;
    v[0x120 / 4] = 0x58;
}

// ---- big table (0x1000 x 0x2c elements) ---------------------------------------------------------
struct elem2c8 {              // layout-identical POD twin of the real element
    raw_tstring a;           // +0x00
    char    pad10[0x0c];
    raw_tstring b;           // +0x1c
};
struct big_table {
    elem2c8 table[0x1000];
    ~big_table();                                               // 0x4be920
};

// @ 0x4be920
big_table::~big_table()
{
    for (int i = 0x1000; i-- > 0; ) {
        tstring_dtor((tstring*)&table[i].b);                    // 0x42bc40
        tstring_dtor((tstring*)&table[i].a);                    // 0x42bc40
    }
}

// ---- vector-like containers ---------------------------------------------------------------------
struct vec60 {
    int    n;               // +0x00
    char*  begin;           // +0x04
    char*  end;             // +0x08
    int size() const;                                           // 0x4be950
    int size_2c8() const;                                       // 0x4be970
    int size_2c8b() const;                                      // 0x4be9a0
};

// @ 0x4be950
int vec60::size() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0x50);
}

// @ 0x4be970
int vec60::size_2c8() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0x1f8);
}

// @ 0x4be9a0
int vec60::size_2c8b() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0xb0);
}

// ---- aggregate ctor (0x4be9c0): two pointers + 8-dword copy -------------------------------------
struct clip_pair {
    void* a;                // +0x00
    void* b;                // +0x04
    int   d[8];             // +0x08
    clip_pair(void* a_, void* b_, const int* src);              // 0x4be9c0
};

// @ 0x4be9c0
clip_pair::clip_pair(void* a_, void* b_, const int* src)
{
    a = a_;
    b = b_;
    memcpy(d, src, sizeof(d));
}

// ---- generic copy / fill helpers ----------------------------------------------------------------
struct rec2c {
    int v[0xb];             // 0x2c bytes
};
struct rec204 {
    int v[0x81];            // 0x204 bytes
};

// @ 0x4be9f0
rec2c* copy_fwd_2c(rec2c* first, rec2c* last, rec2c* dst)
{
    for (; first != last; ++first, ++dst)
        *dst = *first;
    return dst;
}

// @ 0x4bea20
void fill_2c(rec2c* first, rec2c* last, const rec2c* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4bea50
rec2c* copy_back_2c(rec2c* first, rec2c* last, rec2c* dst)
{
    for (; first != last; ) {
        --last;
        --dst;
        *dst = *last;
    }
    return dst;
}

// @ 0x4bea80
rec204* copy_fwd_204(rec204* first, rec204* last, rec204* dst)
{
    for (; first != last; ++first, ++dst)
        *dst = *first;
    return dst;
}

// @ 0x4beac0
void fill_204(rec204* first, rec204* last, const rec204* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4beaf0
rec204* copy_back_204(rec204* first, rec204* last, rec204* dst)
{
    for (; first != last; ) {
        --last;
        --dst;
        *dst = *last;
    }
    return dst;
}

// @ 0x4beb30
void fill_int(int* first, int* last, const int* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4beb50
int* copy_back_int(int* first, int* last, int* dst)
{
    int* p = last;
    while (first != p) {
        --p;
        --dst;
        *dst = *p;
    }
    return dst;
}
