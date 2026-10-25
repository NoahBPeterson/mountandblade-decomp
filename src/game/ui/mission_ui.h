// mission_ui.h: the mission-UI widget base (vtable 0x591c80) and the small helper classes whose members sit
// next to it in mission_ui.cpp (0x488560..0x48b2d0). Names provisional.
//
// MissionUi is ONE class. The slices called it `Effect` (s00487c40, s004885e0: the six track setters, flag
// accessors), `MissionUi` (s00488b20, s0048add0: text placement, ctor, flag setters) and `Buffer` (s004875a0:
// its destructor 0x487c00 stores the same vtable 0x591c80); renderer.h calls the leading part `envelope`
// (0x4624c0, 0x462540 write +0xdc..+0x100). Every member below reads a field of the one layout; the offset
// comments cite a function that proves each.
//
// The slices also put members of other objects under `MissionUi` because they sit between its members; the
// disassembly shows other `this` types (see MIGRATION.md): MissionPanel (draw), ElemList (call0 / call1),
// PairStack (push), TimerChild (tick), ItemRef (sort helpers), Troop (0x4892a0..0x489370).
#ifndef GAME_UI_MISSION_UI_H
#define GAME_UI_MISSION_UI_H
#include "../tstring.h"
#include "./ui_common.h"
#include "./item_ref.h"
#include "../../rgl/mesh.h"

extern float g_tick_scale;                  // 0x54a6d0 1e-5f (the tick table's scale; loaded with fmul from the image)

struct MissionUi {
    // vtable 0x591c80, 50 slots. Names where a caller shows the signature; the rest are placeholders.
    virtual void d00(int release);          // +0x00 deleting destructor (called with push 1 by 0x488ad0)
    virtual bool d01();                     // +0x04 "is active" (tested by 0x488720)
    virtual void d02(); virtual void d03(); virtual void d04(); virtual void d05(); virtual void d06();
    virtual void d07(); virtual void d08(); virtual void d09(); virtual void d10(); virtual void d11();
    virtual void d12(); virtual void d13(); virtual void d14(); virtual void d15(); virtual void d16();
    virtual void d17();
    virtual void set_key_flags(int f);      // +0x48 (0x48b200, 0x48b5a0)
    virtual void d19(); virtual void d20(); virtual void d21();
    virtual void d22(void* p);              // +0x58 position/offset pair (0x487c40)
    virtual void d23(MissionUi* src);       // +0x5c "child moved" notification, passed this (0x488970)
    virtual void d24();
    virtual void set_xy(float* v);          // +0x64 (0x488b50, 0x48b090)
    virtual void d26();
    virtual void set_size(float* v);        // +0x6c (0x48b3a0)
    virtual void set_offset(float* v);      // +0x70 (0x48b3a0)
    virtual void d29(); virtual void d30(); virtual void d31();
    virtual void d32(int a, int b);         // +0x80 (0x488aa0)
    virtual void d33(int a);                // +0x84 (0x488aa0)
    virtual void d34(void* a);              // +0x88 (0x488ad0)
    virtual void d35();
    virtual void refresh();                 // +0x90 (0x48b200)
    virtual void get_xy(float* v);          // +0x94 (0x488b30, 0x488a40)
    virtual void set_color(unsigned* c);    // +0x98 (0x487c40)
    virtual void d39(void* p);              // +0x9c (0x487c40)
    virtual void d40();
    virtual void get_text_size(tstring* s, float* out);   // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48();
    virtual void update_children();         // +0xc4 (0x48b7f0; takes no argument here)

    // six (clock index, start stamp) track pairs; the clock index selects an entry of g_ticks
    int clock0; __int64 t0;                 // +0x08 / +0x10  0x488560, 0x48ae20
    int clock1; __int64 t1;                 // +0x18 / +0x20  0x4885a0
    int clock2; __int64 t2;                 // +0x28 / +0x30  0x4885e0
    int clock3; __int64 t3;                 // +0x38 / +0x40  0x488620
    int clock4; __int64 t4;                 // +0x48 / +0x50  0x488660
    int clock5; __int64 t5;                 // +0x58 / +0x60  0x4886c0
    float periodA, periodB, periodC, periodD, periodE, periodF;   // +0x68..+0x7c  0x488560..0x4886c0 (default -1.0: 0x48ae20)
    float f80, f84;                         // +0x80 0x4886c0
    float f88, f8c;                         // +0x88 0x488660
    int valA, valB, valC, valD;             // +0x90..+0x9c  0x488560..0x488620
    float a0, a4, a8, ac;                   // +0xa0..+0xac  0x488660, 0x4886c0
    int srcA, srcB, srcC, srcD;             // +0xb0..+0xbc  0x488560..0x488620
    unsigned char c0, c1, c2, c3, c4, c5;   // +0xc0..+0xc5 track "restart" flags  0x488560..0x4886c0, 0x48ae20
    unsigned char padc6[2];
    unsigned fC8, fCC, fD0, fD4;            // +0xc8..+0xd4  0x4888d0..0x488950
    float fD8, fDC;                         // +0xd8 (1.0), +0xdc  0x48ae20, 0x488cd0
    union {
        struct { float fE0, fE4, fE8, fEC; };   // +0xe0.. 0x4886c0, 0x488970, 0x488cd0 (fE8: byte flag there, float at 0x488970; fEC smoothing)
        float e[4];                         // indexed by the slider index (0x48b5a0)
    };
    float screen[4];                        // +0xf0 text-space size / base  0x488b50, 0x4889b0 (fF0,fF4), 0x4889e0 (fF8,fFC), 0x48b4f0
    int f100;                               // +0x100 (1)  0x48ae20
    unsigned f104;                          // +0x104 flag word (bit 0 hidden, bit 1 tint)  0x488720, 0x488c90..0x488cc0
    rgl::mesh* tracks[5];                   // +0x108 child meshes (vtable 0x54a6f4); [4] is the primary one  0x488720, 0x488ad0, 0x48b4f0
    unsigned char b11c[5];                  // +0x11c..0x120 per-child "advance" flags  0x488720, 0x48ae20; [4] = 0x120 set by 0x488b50
    unsigned char b121;                     // +0x121  0x488720, 0x48b180
    unsigned char b122;                     // +0x122  0x48ae20
    unsigned char pad123;
    int f124;                               // +0x124  0x488b30
    tstring name;                           // +0x128  0x488b50, 0x488c70
    float f138, f13c, f140, f144;           // +0x138..+0x144 text metrics  0x488a10, 0x488b50, 0x48b090
    unsigned f148;                          // +0x148 option flags  0x488a40, 0x488b50
    unsigned color14c, color150;            // +0x14c / +0x150  0x488720, 0x488b10, 0x488b20
    MissionUi* p154;                        // +0x154 forwarding target  0x488970
    int f158;                               // +0x158  0x48ae20
    int v15c;                               // +0x15c  0x488890
    int f160;                               // +0x160
    int f164;                               // +0x164 (1)
    float v168;                             // +0x168 value / current  0x4888b0
    float f16c;                             // +0x16c range min
    float f170;                             // +0x170 range max (1.0)  0x48ae20
    int f174;                               // +0x174

    MissionUi();                                        // 0x48ae20
    void set_a(float a, int b);                         // 0x488560
    void set_b(float a, int b);                         // 0x4885a0
    void set_c(float a, int b);                         // 0x4885e0
    void set_d(float a, int b);                         // 0x488620
    void set_e(float a, float* p);                      // 0x488660
    void set_f(float a, float* p);                      // 0x4886c0
    void render(rgl::scene* sc, float* count, int c, int d);   // 0x488720 per-child draw loop (ret 0x10)
    void set15c(int v);                                 // 0x488890
    int  get15c();                                      // 0x4888a0
    void set168(int v);                                 // 0x4888b0
    float get168();                                     // 0x4888c0
    bool test_c8(int m);                                // 0x4888d0
    void clear_c8();                                    // 0x4888e0
    bool clear_mask(int m);                             // 0x4888f0
    void or_flags(int m);                               // 0x488910
    void set_d0(int v);                                 // 0x488930
    void set_d4(int v);                                 // 0x488940
    void or_d4(int m);                                  // 0x488950
    void move_a(float* p);                              // 0x488970
    void move_f0(float* p);                             // 0x4889b0
    void move_f8(float* p);                             // 0x4889e0
    void set138(float* p);                              // 0x488a10
    void set148(float* p);                              // 0x488a40
    void or148(float* p);                               // 0x488a70
    void run(int a, int b);                             // 0x488aa0
    void set_track(int i, rgl::mesh* p);                // 0x488ad0
    void set14c(float* p);                              // 0x488b10
    void set_target(void* p);                           // 0x488b20
    void set_pos(int p);                                // 0x488b30
    void set_text(float* out, const tstring* s);   // 0x488b50
    void put(tstring* out);                             // 0x488c70
    void hide();                                   // 0x488c90
    void show();                                 // 0x488ca0
    void flag2_set();                                   // 0x488cb0
    void flag2_clear();                                 // 0x488cc0
    void set_fog(const float* ref, const float* lo, const float* hi);   // 0x488cd0
    void set_text_pos(const float* p);                  // 0x48b090
    char always1();                                     // 0x48b170
    void flag2();                                       // 0x48b180
    void flag1();                                       // 0x48b1a0
    void* make_text(void* out, const tstring* s);       // 0x48b3a0 (in ui_widgets_a.cpp)
    void animate(float dt);                             // 0x487c40 (not in this module)
};

// A panel of two child widgets drawn through MissionUi's virtuals; offsets from 0x488d50 (draw). It shares the
// base's screen[] (+0xf0); the slice's `MissionUi` carried these fields at +0x480.
// A child drawn by the panel: its own interface (slots 0xb0 / 0xb4 begin / end drawing, 0xc4 takes a float).
struct DrawObj {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40(); virtual void s41(); virtual void s42(); virtual void s43();
    virtual void draw_begin();              // +0xb0
    virtual void draw_end();                // +0xb4
    virtual void s46(); virtual void s47(); virtual void s48();
    virtual void set_fill(float f);         // +0xc4
};
struct MissionPanel : MissionUi {
    char pad178[0x480 - 0x178];
    DrawObj* slots[2];                      // +0x480 child objects
    int slot_on[2];                         // +0x488 drawn this frame
    MissionUi* host;                        // +0x490 the widget whose screen[] bounds the children
    void draw();                            // 0x488d50
};

// A list of 0x1dc-byte effect elements (FxElem, vtable 0x54b454) at +4; 0x488e20 / 0x488e40 forward to the
// element's virtuals 0 and 1 (slot 0 is FxElem's destructor entry, called with an int argument).
struct ElemVt { virtual void s0(int a); virtual void s1(int a); char pad[0x1dc - 4]; };
struct ElemList {
    char pad0[4];
    ElemVt* elems;                          // +4
    void call0(int i, int a);               // 0x488e20
    void call1(int i, int a);               // 0x488e40
};

// A stack of up to 0x80 (a, b) int pairs at +0 with its count at +0x400 (0x488e60).
struct PairStack {
    int pairs[0x100];                       // +0
    int count;                              // +0x400
    void push(int a, int b);                // 0x488e60
};

// A timed voice: a period, one sound channel (0x1c bytes) and a clock; 0x30 bytes (0x488e90, 0x48add0).
struct TimerChild {
    float period;                           // +0x00 seconds between plays; < -1: play when round(period) == the request
    snd_channel ch;                         // +0x04
    volatile int clock;                     // +0x20 index into g_ticks (re-read, never CSE'd: 0x488e90)
    int pad24;
    __int64 last;                           // +0x28 last play time
    void tick(int idx);                     // 0x488e90
};
#include "./timer_list.h"                 // TimerList: count +0, items +4, update 0x48add0

// free functions of the item-pair sort and the speed table
int __fastcall sort_value(ItemRef* e);                              // 0x488f80
float __stdcall speed_scale(const float* base, const float* ref, float r);   // 0x488ff0
void __fastcall clear_slots(ItemRef* e);                            // 0x489060 (sets the first 0x60 items to -1)

#endif // GAME_UI_MISSION_UI_H
