// gui_manager.h: the engine's UI layer (widget input dispatch, the UI root `gui_manager`) and the text-log stream
// used by the reporters in the same object. Functions: src/game/strat/gui_manager.cpp (0x42cb70..0x42d820).
// Layouts recovered from the byte-exact / equivalent functions; each field cites the function that proves it.
// The streams come from src/rgl/stream.h and the math from src/rgl/rgl_math.h.
//
// gui_manager is THE object at 0x5d7080 (0xb350 bytes, defined once as g_gui in strat/gui_manager.cpp and built by the
// compiler's own initialiser). Every other module's view of it (the message log, the screen / input state, the loading
// meshes, the map label cursor, the sound host, ...) and every label inside its range (g_screen_w, g_gui_dirty,
// g_mission_clock, g_fov_k, g_div, g_mouse_dx, g_strs, g_drag, ...) is a member here. ALL NAMES PROVISIONAL.
#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H
#include <stddef.h>
#include <stdio.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../tstring.h"
#include "../../include/timer.h"
#include "../../include/clock_table.h"

using rgl::vector4;

#include "../../include/fx.h"

#include "../../rgl/mesh.h"       // rgl::mesh (0x260 bytes, ctor 0x412a90) and rgl::scene (0x1d8, ctor 0x429740)
#include "../../include/frustum.h"
using rgl::frustum;                 // 0x41ef60 (ctor, no dtor)
struct MissionUi;                   // ui/mission_ui.h: the widget base (the UI root owns its root widgets through it)

extern char  g_error_flag;          // 0x5d7070 (mb_error 0x42cc80)
extern int   g_gui_counter;         // 0x5d7074 (bump_counter 0x42cb70)

// g_log (0x5a9f60) is log_stream, src/rgl/stream.h.

void bump_counter();                            // 0x42cb70
void mb_warning(const char* fmt, ...);          // 0x42cb80  vsnprintf + MessageBoxA "RGL WARNING"
__int64 file_length(FILE* f);                   // 0x42cbc0
void mb_error(const char* fmt, ...);            // 0x42cc80  "ERROR:" to g_log, MessageBoxA "RGL ERROR", exit(-1)

// gui_timer: include/timer.h
// the UI root's widget list (the vector at +0): push_back 0x49b120 and insert 0x49b170 are the out-of-line instances the
// original calls (config/widget_node.cpp); _Insert_n 0x49b1c0 follows them
struct WidgetVec : std::vector<MissionUi*> {
    void push_back(MissionUi* const& x);                                    // 0x49b120
    iterator insert(iterator where, MissionUi* const& x);                   // 0x49b170
};

struct gui_manager {
    WidgetVec objs;                 // +0      reset 0x42d1b0 (deletes the elements, erases the range); the root widgets
    MissionUi* ui_active;           // +0x10   reset; the slider / widget being dragged (g_ui_active 0x5d7090: ui_d_key, ui_slider_key, ui_widgets_c)
    float mouse[2];                 // +0x14   ctor 0x42d5c0; the pointer position x, y (0x5d7094 / 0x5d7098: g_ui_target, g_look_y; process 0x4ebfd0)
    float dmx, dmy;                 // +0x1c   process: the pointer's step this frame
    union {
        struct { float mouse_dx, mouse_dy; };   // +0x24 g_mouse_dx / g_mouse_dy (0x5d70a4 / 0x5d70a8): the mouse offset accumulated at the screen edge
        struct { int p24a, p24b; };
    };
    float prev_mx, prev_my;         // +0x2c   process: the pointer position before this frame (ctor zeroes both)
    int warp_x, warp_y;             // +0x34, +0x38 the cursor position the editor warps the pointer to (SetCursorPos, mission5/mesh_3.cpp); process: the previous raw cursor
    char pad3c[0x44 - 0x3c];
    rgl::scene x44;                 // +0x44   ctor: scene::scene 0x429740 (0x1d8 bytes)
    rgl::scene x21c;                // +0x21c  ctor: scene::scene
    frustum y3f4;                   // +0x3f4  ctor: frustum::frustum 0x41ef60 (0x120 bytes)
    float aspect;                   // +0x514  reset (0.75); g_div 0x5d7594: the view's height / width (render_shape 0x4ed6f0)
    gui_timer t0, t1, t2;           // +0x518, +0x528, +0x538   ctor / reset; process: the last left / right / middle click
    int screen_w, screen_h;         // +0x548, +0x54c  g_screen_w / g_screen_h (0x5d75c8 / 0x5d75cc): the viewport size (update_view_rect 0x4bace0)
    int view_y;                     // +0x550  update_view_rect 0x4bace0 (the 3D view's origin inside the window)
    int view_x;                     // +0x554
    float view_a, view_b;           // +0x558, +0x55c  g_view_a / g_view_b (0x5d75d8 / 0x5d75dc; agent_hud: g_screen_b / g_screen_a): the view's scale factors
    bool show_cursor;               // +0x560  ctor; g_5d75e0 (GameApp::m_bShowCursorWhenFullscreen)
    char pad561[3];
    rgl::mesh* drag;                // +0x564  set_drag 0x42cd80, reset, dtor 0x42d3e0; g_drag (the mesh dragged by the inventory)
    rgl::mesh* overlay;             // +0x568  set_overlay 0x42cdb0; g_overlay (the text mesh the lines are drawn into)
    rgl::mesh* label_obj;           // +0x56c  reset; g_label_obj (the map label under the cursor)
    rgl::mesh m570;                 // +0x570  ctor: mesh::mesh 0x412a90 (0x260 bytes each)
    tstring lines[0x50];            // +0x7d0  g_strs: set_overlay / reset (assign("")), ctor loop of tstring::tstring 0x42bbd0
    unsigned char line_wrap[0x50];  // +0xcd0  per-line flag (draw_string 0x4e43e0: the `wrap` argument; rebuild 0x4da900 replays it)
    float line_height[0x50];        // +0xd20  per-line height argument
    int line_arg[0x50];             // +0xe60  per-line int argument
    int fa0, fa4;                   // +0xfa0, +0xfa4  g_list0 / g_list1: set_overlay / reset
    float fa8;                      // +0xfa8  g_list2
    char padfac[0xfb0 - 0xfac];
    rgl::mesh mfb0;                 // +0xfb0
    rgl::mesh* corner[4];           // +0x1210 dtor; the loading frame's corner meshes
    rgl::mesh m1220;                // +0x1220 (its mask +0x132c is the font mesh's "on" flag, set_font_mesh 0x49b630)
    rgl::mesh m1480;                // +0x1480 (reset clears its mask +0x10c): the font mesh
    unsigned panel_color, panel_bg; // +0x16e0, +0x16e4  g_panel_color / g_panel_bg: reset, set_overlay
    float vx, vy, vs, f16f4;        // +0x16e8  set_view 0x4c5190: the view x (clamped to [0.05, 0.95]), y, y scale; +0x16f4
    float box_x, box_y;             // +0x16f8, +0x16fc  g_box_x / g_box_y (the map label's offset, map_panel.cpp)
    int sel_ent;                    // +0x1700  g_sel_ent
    union {
        int f1704;                  // +0x1704  g_5d8784 (zeroed with the label object)
        float cursor_y;             // +0x1704  the label text's running vertical cursor (add_label 0x51a880)
    };
    unsigned char dirty;            // +0x1708  g_gui_dirty: set to 1 whenever a window was installed (process 0x4ebfd0: keyboard cursor mode, stop processing)
    char f1709;                     // +0x1709
    char pad170a[2];
    int f170c;                      // +0x170c  g_loaded_sound_count (module_files.cpp) / the UI click sample (play_sound 0x48b2d0)
    int wheel;              // +0x1710  g_mission_clock (0x5d8790): the mouse wheel steps read this frame (process zeroes then sums them; the sliders scale by it)
    char enabled;                   // +0x1714  the message log's enabled byte (reset sets it)
    void set_enabled(bool v) { enabled = v; }      // inline (script op set_show_messages 0x4a30a0)
    char pad1715[3];
    int f1718;                      // +0x1718  g_5d8798: the last keyboard character (-1 when consumed)
    float total_h;                  // +0x171c  summed height of the visible log lines (add 0x452c00)
    int cur;                        // +0x1720  index of the newest log line
    char pad1724[4];
    rgl::mesh meshA[32];            // +0x1728  the log's line texts (ctor loop, 0x260 stride)
    rgl::mesh meshB[32];            // +0x6328  their backgrounds
    float age[32];                  // +0xaf28  seconds since each log line was added (ages past 10 are not drawn)
    float height[32];               // +0xafa8
    float fov_k;                    // +0xb028  g_fov_k (0x5e20a8): ctor sets 1.0
    int color[32];                  // +0xb02c
    char hud_flag;                  // +0xb0ac  g_hud_flag / g_5e212c: reset / ctor clear it, agent_hud sets it
    char padb0ad[0xb0b0 - 0xb0ad];
    int saved_x, saved_y;           // +0xb0b0, +0xb0b4 the cursor position last read by process (GetCursorPos); mission5/mesh_3.cpp
    struct Ring {                   // +0xb0b8: the pending log messages, a 32-entry ring; the inline ctor clears head then count
        int count;                  // +0xb0b8
        int head;                   // +0xb0bc
        tstring names[32];          // +0xb0c0
        int vals[32];               // +0xb2c0
        Ring() { head = 0; count = 0; }
        // inline; the element address comes from the ring's this, which keeps `names` from folding into `(idx + 0xb0c) << 4` (0x4334b0)
        void add(const tstring& s, int v) {
            if (count < 32) {
                int idx = (head + count) % 32;
                names[idx] = s;
                vals[idx] = v;
                count++;
            }
        }
        void pop(tstring& out, int& index);             // 0x4f13f0 (src/game/char_sheet/msg_log_ui.cpp): oldest entry, or "No messages" and -1
    } ring;
    std::vector<tstring> history;   // +0xb340  (vector: alloc at +0xb340, first +0xb344; ctor zeroes first/last/end) the log line history

    gui_manager();                  // 0x42d5c0
    ~gui_manager();                 // 0x42d3e0
    void reset();                   // 0x42d1b0
    void set_drag(rgl::mesh* p);    // 0x42cd80
    void set_overlay(rgl::mesh* p, bool clear_lines);   // 0x42cdb0
    void draw_string(tstring* s, float height, int arg, char wrap);   // 0x4e43e0 (ret 0x10; src/game/cls_map/progress_bar.cpp)
    __forceinline void draw_string(const tstring& s, float height, unsigned color, int wrap)   // the callers' by-reference form
    {
        draw_string((tstring*)&s, height, (int)color, (char)wrap);
    }
    void render_log(rgl::scene* pass, const float* depth);   // 0x4bd170 (thiscall, ret 8; src/game/config/config.cpp: draws the 32 fading log lines)
    __forceinline void draw_string(const tstring* s, float y, int color, char b)   // the callers' const form
    {
        draw_string((tstring*)s, y, color, b);
    }
    void build_scene();             // 0x4e6050 (the frame's scene build: two viewports, the widget walk and the log / frame meshes; src/game/cls_map/progress_bar.cpp)
    void rebuild();                 // 0x4da900 (replays the stored lines into a fresh text mesh; src/game/cls_map)
    void process(int dt_bits);      // 0x4ebfd0 (one input pass; the dt rides in as its bits; src/game/char_sheet/screen.cpp)
    void set_view(const float* v);  // 0x4c5190 (src/rgl/mesh2/gui_edit.cpp: the view state pair)
    void wheel_step(int mask);      // 0x4c52f0 (records a key in the hint tables)
    void clear_selection();         // 0x4c5330
    void set_font_mesh(const tstring* text, int arg);   // 0x49b630 (src/game/party2/camera.cpp)
    void push(const tstring& s, int v);                 // 0x4334b0 (src/rgl/xmlcfg/game_util.cpp: queue a log message on the ring)
    void add(tstring msg, int color);                   // 0x452c00 (thiscall, ret 0x14; src/game/agent4/agent_logic.cpp)
    void reset_geometry();          // 0x4e64b0 (src/game/cls_map/progress_bar.cpp: clear the 32+32 meshes, the ages to 1e6, the heights to 0.1)
    void update(float dt);          // 0x4e6510 (flush the pending text, age the lines, fade the total height)
    void flush();                   // 0x4f1390 (char_sheet/msg_log_ui.cpp): pops the ring into add()
    void set_rect(const float* a, const float* b);      // 0x4dc350 (ret 8; src/game/cls_map/cls_map.cpp: place the loading screen's frame)
    void build_frames();            // 0x50f930 (src/game/inventory_panel/loading_screen.cpp)
    void build_quad();              // 0x50f8a0
    void set_pos(const float* p);                       // 0x51a760 (src/game/inventory_panel/map_panel.cpp)
    void add_label(const tstring* text, float h, int color);   // 0x51a880
    void set_quad(const float* origin, const float* size);     // 0x51aa20
    void play_sound();              // 0x48b2d0 (src/game/ui/ui_widgets_a.cpp: FSOUND_PlaySoundEx on f170c + volume from g_master_volume)
};
typedef char gui_manager_size_check[sizeof(gui_manager) == 0xb350 ? 1 : -1];
typedef char gui_manager_off_check[(offsetof(gui_manager, ui_active) == 0x10 && offsetof(gui_manager, mouse) == 0x14 && offsetof(gui_manager, mouse_dx) == 0x24
    && offsetof(gui_manager, warp_x) == 0x34 && offsetof(gui_manager, x44) == 0x44 && offsetof(gui_manager, aspect) == 0x514
    && offsetof(gui_manager, screen_w) == 0x548 && offsetof(gui_manager, view_a) == 0x558 && offsetof(gui_manager, show_cursor) == 0x560
    && offsetof(gui_manager, drag) == 0x564 && offsetof(gui_manager, m570) == 0x570 && offsetof(gui_manager, lines) == 0x7d0
    && offsetof(gui_manager, fa0) == 0xfa0 && offsetof(gui_manager, mfb0) == 0xfb0 && offsetof(gui_manager, corner) == 0x1210
    && offsetof(gui_manager, m1220) == 0x1220 && offsetof(gui_manager, m1480) == 0x1480 && offsetof(gui_manager, panel_color) == 0x16e0
    && offsetof(gui_manager, box_x) == 0x16f8 && offsetof(gui_manager, sel_ent) == 0x1700 && offsetof(gui_manager, dirty) == 0x1708
    && offsetof(gui_manager, f170c) == 0x170c && offsetof(gui_manager, wheel) == 0x1710 && offsetof(gui_manager, enabled) == 0x1714
    && offsetof(gui_manager, f1718) == 0x1718 && offsetof(gui_manager, meshA) == 0x1728 && offsetof(gui_manager, meshB) == 0x6328
    && offsetof(gui_manager, age) == 0xaf28 && offsetof(gui_manager, fov_k) == 0xb028 && offsetof(gui_manager, hud_flag) == 0xb0ac
    && offsetof(gui_manager, saved_x) == 0xb0b0 && offsetof(gui_manager, ring) == 0xb0b8 && offsetof(gui_manager, history) == 0xb340) ? 1 : -1];
extern gui_manager g_gui;           // 0x5d7080 (defined in strat/gui_manager.cpp)
#endif // GUI_MANAGER_H
