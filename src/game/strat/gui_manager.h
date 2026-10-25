// gui_manager.h: the engine's UI layer (widget input dispatch, the UI root `gui_manager`) and the text-log stream
// used by the reporters in the same object. Functions: src/game/strat/gui_manager.cpp (0x42cb70..0x42d820).
// Layouts recovered from the byte-exact / equivalent functions; each field cites the function that proves it.
// The streams come from src/rgl/stream.h and the math from src/rgl/rgl_math.h (they cannot be mixed with
// match/include/rgl.h, which strat_ext.h needs: rgl::string collides). The three render-layer classes the UI root
// embeds are sized stand-ins here (their real definitions belong to the scene / frustum / mesh modules).
// ALL NAMES PROVISIONAL.
#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H
#include <stdio.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../tstring.h"

using rgl::vector4;

#include "../../include/fx.h"

// sized stand-ins (see above); ctor/dtor addresses are what the gui_manager ctor/dtor call
struct scene {
    char d[0x1d8];
    scene();                        // 0x429740
    ~scene();                       // 0x41f4e0
};
#include "../../include/frustum.h"
using rgl::frustum;                 // 0x41ef60 (ctor, no dtor); the sized stand-in became the shared class
struct mesh {
    char d0[0x10c];
    unsigned mask;                  // +0x10c (reset 0x42d1b0 clears it on two of them)
    char d1[0x260 - 0x110];
    mesh();                         // 0x412a90
    ~mesh();                        // 0x4127f0
};

extern float g_mouse_x;             // 0x5d7094 (mouse_event 0x42cf50)
extern float g_mouse_y;             // 0x5d7098
extern char  g_ui_dirty;            // 0x5d8789 (wheel_step 0x42ce50, handle_event 0x42d170)
extern char  g_error_flag;          // 0x5d7070 (mb_error 0x42cc80)
extern int   g_counter;             // 0x5d7074 (bump_counter 0x42cb70)

// Text log stream (global at 0x5a9f60): a file writer whose flush (slot 26) is 0x42cc10.
struct log_stream : file_writer_base {
    STREAM_SLOTS
    virtual int flush();                        // 0x42cc10
};
extern log_stream g_log;            // 0x5a9f60

void bump_counter();                            // 0x42cb70
void mb_warning(const char* fmt, ...);          // 0x42cb80  vsnprintf + MessageBoxA "RGL WARNING"
__int64 file_length(FILE* f);                   // 0x42cbc0
void mb_error(const char* fmt, ...);            // 0x42cc80  "ERROR:" to g_log, MessageBoxA "RGL ERROR", exit(-1)

struct gui_timer {                  // 16 bytes: clock index, value (gui_manager ctor 0x42d5c0 initialises three)
    int clock;                      // +0   set_delay 0x42cd20
    int pad4;
    __int64 t;                      // +8   set_delay 0x42cd20, reset 0x42d1b0
    gui_timer() { clock = 0; t = g_ticks[0]; }
    void set_delay(float seconds);  // 0x42cd20 (thiscall, ret 4)
    void rewind() { t = g_ticks[clock] - 100000; }   // inline: one second in the past
};

// The UI element base is src/game/widget.h (shared with char_sheet / inventory_panel). The container classes (elem 0x188, overlay
// 0x190: ctor 0x4931a0 zeroes +0x17c..+0x184) add the child vector at +0x178; the base vtable's slots 35 / 36 (refresh_all 0x42cd60,
// wheel_step 0x42ce50) and the input dispatch of this file work on it (this view was named `widget` here).
#include "../widget.h"
struct gui_window : widget {
    std::vector<widget*> children;              // +0x178 (mouse_event, wheel_focus)
    char wants_redraw;                          // +0x188 (handle_event 0x42d170; the vector is 0x10 bytes)

    void wheel_step();                          // 0x42ce50
    void wheel_focus();                         // 0x42cec0
    char mouse_event(unsigned mask);            // 0x42cf50
    char handle_event(unsigned mask);           // 0x42d170
    char refresh_all(unsigned mask);            // 0x42cd60
};

// ---- the UI root object (0xb350 bytes; global instance not seen yet). Layout from reset (0x42d1b0), the
// ctor 0x42d5c0 and the dtor 0x42d3e0. ----
struct gui_obj {                    // polymorphic child object (deleted through vtable slot 0)
    virtual ~gui_obj();
};
struct pair2 {
    int a, b;
    pair2() {}
    pair2(const int& a_, const int& b_) { a = a_; b = b_; }
    pair2& operator=(const pair2& o) { a = o.a; b = o.b; return *this; }
};

struct gui_manager {
    std::vector<gui_obj*> objs;     // +0      reset 0x42d1b0 (deletes the elements, erases the range)
    int f10;                        // +0x10   reset
    pair2 p14;                      // +0x14   ctor 0x42d5c0
    char pad1c[0x24 - 0x1c];
    pair2 p24;                      // +0x24   ctor
    pair2 p2c;                      // +0x2c   ctor
    char pad34[0x44 - 0x34];
    scene x44;                      // +0x44   ctor: scene::scene 0x429740 (0x1d8 bytes)
    scene x21c;                     // +0x21c  ctor: scene::scene
    frustum y3f4;                   // +0x3f4  ctor: frustum::frustum 0x41ef60 (0x120 bytes)
    float f514;                     // +0x514  reset
    gui_timer t0, t1, t2;           // +0x518, +0x528, +0x538   ctor / reset
    char pad548[0x560 - 0x548];
    char f560;                      // +0x560  ctor
    char pad561[3];
    gui_obj* o564;                  // +0x564  set_o564 0x42cd80, reset, dtor 0x42d3e0
    gui_obj* o568;                  // +0x568  set_o568 0x42cdb0
    gui_obj* o56c;                  // +0x56c  reset
    mesh m570;                      // +0x570  ctor: mesh::mesh 0x412a90 (0x260 bytes each)
    tstring lines[0x50];            // +0x7d0  set_o568 / reset (assign("")), ctor loop of tstring::tstring 0x42bbd0
    unsigned char line_wrap[0x50];  // +0xcd0  per-line flag (draw_string 0x4e43e0: the `wrap` argument; rebuild 0x4da900 replays it)
    float line_height[0x50];        // +0xd20  per-line height argument
    int line_arg[0x50];             // +0xe60  per-line int argument
    int fa0, fa4;                   // +0xfa0, +0xfa4  set_o568 / reset
    float fa8;                      // +0xfa8
    char padfac[0xfb0 - 0xfac];
    mesh mfb0;                      // +0xfb0
    gui_obj* o1210; gui_obj* o1214; gui_obj* o1218; gui_obj* o121c;   // +0x1210  dtor
    mesh m1220;                     // +0x1220
    mesh m1480;                     // +0x1480 (reset clears its mask +0x10c)
    int f16e0, f16e4;               // +0x16e0, +0x16e4  reset
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
    mesh meshA[32];                 // +0x1728  ctor loop (0x260 stride)
    mesh meshB[32];                 // +0x6328
    char padaf28[0xb028 - 0xaf28];
    float fb028;                    // +0xb028  ctor
    char padb02c[0xb0ac - 0xb02c];
    char fb0ac;                     // +0xb0ac  reset / ctor
    char padb0ad[0xb0b8 - 0xb0ad];
    struct name_table {             // +0xb0b8: two ints and 32 names; the inline ctor clears the ints after the names
        int fb0b8, fb0bc;
        tstring names[32];
        name_table() { fb0bc = 0; fb0b8 = 0; }
    } nt;
    char padb2c0[0xb340 - 0xb2c0];
    std::vector<tstring> strs;      // +0xb340  (vector: alloc at +0xb340, first +0xb344; ctor zeroes first/last/end)

    gui_manager();                  // 0x42d5c0
    ~gui_manager();                 // 0x42d3e0
    void reset();                   // 0x42d1b0
    void set_o564(gui_obj* p);      // 0x42cd80
    void set_o568(gui_obj* p, bool clear_lines);   // 0x42cdb0
    void draw_string(tstring* s, float height, int arg, char wrap);   // 0x4e43e0 (ret 0x10; src/game/cls_map/progress_bar.cpp)
    void build_scene();             // 0x4e6050 (the frame's scene build: two viewports, the widget walk and the log / frame meshes; src/game/cls_map/progress_bar.cpp)
    void rebuild();                 // 0x4da900 (replays the stored lines into a fresh text mesh; src/game/cls_map)
    void add_root(void* const& window);            // 0x49b120 (ret 4: make `window` the active root window; src/game/cls_map)
};
extern gui_manager g_gui; // 0x5d7080
#endif // GUI_MANAGER_H
