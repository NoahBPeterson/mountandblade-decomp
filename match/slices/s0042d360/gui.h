// gui.h: the engine's UI layer (provisional names). Layouts recovered from the byte-exact/equivalent
// functions at 0x42c9b0..0x42d5c0 (s0042c9b0, s0042d360). Copied into each slice directory that uses it.
#ifndef MB_GUI_H
#define MB_GUI_H
#include <vector>
#include "mbbin.h"

extern __int64 g_time[];            // 0x5e2948 table of 64-bit clocks (1e-5 s units)
extern float g_mouse_x;             // 0x5d7094
extern float g_mouse_y;             // 0x5d7098
extern char  g_ui_dirty;            // 0x5d8789
extern char  g_error_flag;          // 0x5d7070
extern int   g_counter;             // 0x5d7074

// 16-byte tstring is in mbbin.h.
struct gui_timer {                  // 16 bytes: clock index, value
    int clock;                      // +0
    int pad4;
    __int64 t;                      // +8
    gui_timer() { clock = 0; t = g_time[0]; }
    void set_delay(float seconds);  // 0x42cd20 (thiscall, ret 4)
    void rewind() { t = g_time[clock] - 100000; }   // inline: one second in the past
};

// Base of the UI controls (vtable slot numbers from the call sites). Size > 0x189.
struct widget {
    virtual ~widget();                          // 0
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8();
    virtual void s9(); virtual void s10();
    virtual void set_focus(int index);          // 11 (0x2c)
    virtual void s12(); virtual void s13(); virtual void s14();
    virtual char hit_test(unsigned mask);       // 15 (0x3c)
    virtual void s16(); virtual void s17();
    virtual void on_input(unsigned mask);       // 18 (0x48)
    virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24(); virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28();
    virtual void s29(); virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33();
    virtual void s34();
    virtual char on_event(unsigned mask);       // 35 (0x8c)
    virtual void refresh();                     // 36 (0x90)
    char pad4[0xcc - 4];
    int pending;                                // +0xcc
    int enabled_mask;                           // +0xd0
    int wheel;                                  // +0xd4 bit0 up, bit1 down, bit2 focus-on-top
    char padd8[0xe0 - 0xd8];
    float x0, y0;                               // +0xe0, +0xe4
    char pade8[0xf0 - 0xe8];
    float x1, y1, x_lo, y_lo;                   // +0xf0, +0xf4, +0xf8, +0xfc
    char pad100[0x104 - 0x100];
    unsigned flags;                             // +0x104 (bits 0-1: hidden/disabled)
    char pad108[0x121 - 0x108];
    char hover;                                 // +0x121
    char pad122[0x158 - 0x122];
    widget* focus;                              // +0x158
    int sel;                                    // +0x15c
    int sel_lo, sel_hi;                         // +0x160, +0x164
    char pad168[0x178 - 0x168];
    std::vector<widget*> children;              // +0x178
    char wants_redraw;                          // +0x188 (declared here: vector is 0x10 bytes, ends at 0x188)

    void wheel_step();                          // 0x42ce50
    void wheel_focus();                         // 0x42cec0
    char mouse_event(unsigned mask);            // 0x42cf50
    char handle_event(unsigned mask);           // 0x42d170
    char refresh_all(unsigned mask);            // 0x42cd60
};

// ---- the UI root object (0x1720+ bytes; global instance not seen yet). Layout from reset (0x42d1b0), the
// ctor 0x42d5c0 and the dtor 0x42d3e0. ----
struct gui_obj {                    // polymorphic child object (deleted through vtable slot 0)
    virtual ~gui_obj();
};
struct tstring_vec {                // std::vector<tstring> layout (alloc, first, last, end)
    int alloc;
    tstring* first;
    tstring* last;
    tstring* end;
    tstring_vec() { first = 0; last = 0; end = 0; }
    ~tstring_vec() { tidy(); }
    void tidy();                    // 0x42d390
};
struct pair2 {
    int a, b;
    pair2() {}
    pair2(const int& a_, const int& b_) { a = a_; b = b_; }
    pair2& operator=(const pair2& o) { a = o.a; b = o.b; return *this; }
};
struct blob_x {
    char d[0x1d8];
    blob_x();                       // 0x429740
    ~blob_x();                      // 0x41f4e0
};
struct blob_y {
    char d[0x120];
    blob_y();                       // 0x41ef60 (no dtor)
};
struct mesh {                       // see s00412a90/mesh.h
    char d0[0x10c]; int f10c; char d1[0x260 - 0x110];
    mesh();                         // 0x412a90
    ~mesh();                        // 0x4127f0
};

struct gui_manager {
    std::vector<gui_obj*> objs;     // +0
    int f10;                        // +0x10
    pair2 p14;                      // +0x14
    char pad1c[0x24 - 0x1c];
    pair2 p24;                      // +0x24
    pair2 p2c;                      // +0x2c
    char pad34[0x44 - 0x34];
    blob_x x44;                     // +0x44
    blob_x x21c;                    // +0x21c
    blob_y y3f4;                    // +0x3f4
    float f514;                     // +0x514
    gui_timer t0, t1, t2;           // +0x518, +0x528, +0x538
    char pad548[0x560 - 0x548];
    char f560;                      // +0x560
    char pad561[3];
    gui_obj* o564;                  // +0x564
    gui_obj* o568;                  // +0x568
    gui_obj* o56c;                  // +0x56c
    mesh m570;                      // +0x570
    tstring lines[0x50];            // +0x7d0
    char padcd0[0xfa0 - 0xcd0];
    int fa0, fa4;                   // +0xfa0, +0xfa4
    float fa8;                      // +0xfa8
    char padfac[0xfb0 - 0xfac];
    mesh mfb0;                      // +0xfb0
    gui_obj* o1210; gui_obj* o1214; gui_obj* o1218; gui_obj* o121c;   // +0x1210
    mesh m1220;                     // +0x1220
    mesh m1480;                     // +0x1480
    int f16e0, f16e4;               // +0x16e0, +0x16e4
    char pad16e8[0x1700 - 0x16e8];
    int f1700, f1704;               // +0x1700, +0x1704
    char pad1708;
    char f1709;                     // +0x1709
    char pad170a[2];
    int f170c, f1710;               // +0x170c, +0x1710
    char f1714;                     // +0x1714
    char pad1715[3];
    int f1718;                      // +0x1718
    char pad171c[0x1728 - 0x171c];
    mesh meshA[32];                 // +0x1728
    mesh meshB[32];                 // +0x6328
    char padaf28[0xb028 - 0xaf28];
    float fb028;                    // +0xb028
    char padb02c[0xb0ac - 0xb02c];
    char fb0ac;                     // +0xb0ac
    char padb0ad[0xb0b8 - 0xb0ad];
    struct name_table {             // +0xb0b8: two ints and 32 names; the inline ctor clears the ints after the names
        int fb0b8, fb0bc;
        tstring names[32];
        name_table() { fb0bc = 0; fb0b8 = 0; }
    } nt;
    char padb2c0[0xb340 - 0xb2c0];
    tstring_vec strs;               // +0xb340

    gui_manager();                  // 0x42d5c0
    ~gui_manager();                 // 0x42d3e0
    void reset();                   // 0x42d1b0
    void set_o564(gui_obj* p);      // 0x42cd80
    void set_o568(gui_obj* p, bool clear_lines);   // 0x42cdb0
};
#endif
