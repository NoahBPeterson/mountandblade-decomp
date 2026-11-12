//-----------------------------------------------------------------------------
// File: gui_manager.cpp  (0x42cb70..0x42d820)
//
// Desc: the engine's reporting helpers (warning / error message boxes, text-log flush, file length), the UI layer
//       (widget wheel / mouse dispatch, the UI root gui_manager: constructor, destructor, reset) and the VC7.1
//       std::vector<tstring> / <vector4> helpers it instantiates. 0x42d820 starts the TinyXML objects (src/tinyxml).
//       Functions in ascending address order. Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../tstring.h"
#include "../../rgl/stream.h"
#include "./gui_manager.h"
#include "../ui/mission_ui.h"                  // MissionUi: the root widgets reset deletes
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/mat4.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (docs/standalone_data.md) ----
char g_error_flag;                               // 0x5d7070 (.bss) set by mb_error before the message box
int g_gui_counter;                               // 0x5d7074 (.bss) bumped by bump_counter
gui_manager g_gui;                               // 0x5d7080 (.bss, 0xb350 bytes; built by the compiler's initialiser gui_manager::gui_manager, destroyed by its atexit)

// @ 0x42cb70
void bump_counter() { g_gui_counter++; }

// @ 0x42cb80
void mb_warning(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, 512, fmt, ap);
    MessageBoxA(0, buf, "RGL WARNING", MB_ICONHAND);
}

// @ 0x42cbc0
__int64 file_length(FILE* f)
{
    __int64 pos = ftell(f);
    fseek(f, 0, SEEK_END);
    __int64 end = ftell(f);
    fseek(f, (long)pos, SEEK_SET);
    return end;
}

// @ 0x42cc10
int log_stream::flush()
{
    int r = 0;
    if (is_open) {
        int n = buf.len;
        if (flushed < n) {
            int bad = 0;
            const char* p = buf.c_str();
            size_t w = fwrite(flushed + p, 1, n - flushed, fp);
            if (w != buf.len - flushed)
                bad = 1;
            flushed = buf.len;
            fflush(fp);
            r = bad;
        }
    }
    return r;
}

// @ 0x42cc80
void mb_error(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, 512, fmt, ap);
    stream_base& log = g_log;
    log.w_cstr("ERROR:");
    log.w_cstr(buf);
    g_log.flush();
    g_error_flag = 1;
    MessageBoxA(0, buf, "RGL ERROR", MB_ICONHAND);
    printf("Error: %s", buf);
    fflush(stdout);
    exit(-1);
}

// @ 0x42cd20
void Timer::set_delay(float seconds)
{
    start = g_ticks[clock] - (int)(seconds * 100000.0f);
}

// @ 0x42cd80
void gui_manager::set_drag(rgl::mesh* p)
{
    if (drag) {
        delete drag;
        drag = p;
        return;
    }
    drag = p;
}

// @ 0x42cdb0
void gui_manager::set_overlay(rgl::mesh* p, bool clear_lines)
{
    if (overlay) delete overlay;
    if (p == 0) {
        panel_color = -1;
        panel_bg = 0xff101010;
    }
    if (clear_lines) {
        for (int i = 0; i < 0x50; i++)
            lines[i].assign("");
        fa4 = 0;
        fa8 = -0.03f;
    }
    overlay = p;
    fa0 = 0;
    sel_ent = 0;
}

// 0x42cd60 MissionUi::key, 0x42ce50 MissionUi::refresh, 0x42cec0 ContainerBase::refresh, 0x42cf50 ContainerBase::key and
// 0x42d170 NodeA::key (the slices' gui_window::refresh_all / wheel_step / wheel_focus / mouse_event / handle_event) are
// defined once, as the widget classes' own virtuals, in ui/widget_slots.cpp.

// @ 0x42d1b0
void gui_manager::reset()
{
    enabled = 1;
    hud_flag = 0;
    aspect = 0.75f;
    ui_active = 0;
    t0.rewind();
    t1.rewind();
    t2.rewind();
    for (unsigned i = 0; i < objs.size(); i++) {
        if (objs[i]) {
            delete objs[i];
            objs[i] = 0;
        }
    }
    objs.erase(objs.begin(), objs.end());
    if (drag) delete drag;
    drag = 0;
    if (overlay) delete overlay;
    panel_color = -1;
    panel_bg = 0xff101010;
    for (int i = 0; i < 0x50; i++)
        lines[i].assign("");
    fa4 = 0;
    fa8 = -0.03f;
    fa0 = 0;
    overlay = 0;
    sel_ent = 0;
    if (label_obj) delete label_obj;
    label_obj = 0;
    f1704 = 0;
    f1709 = 0;
    m1480.mask = 0;
    m1220.mask = 0;
}

// 0x42d360 std::_Destroy(tstring*, tstring*) and 0x42d390 std::vector<tstring>::_Tidy: instantiated by ~gui_manager
// (and by the dtor of its `history` member).

// @ 0x42d3e0
gui_manager::~gui_manager()
{
    reset();
    if (drag) { delete drag; drag = 0; }
    if (overlay) { delete overlay; overlay = 0; }
    if (label_obj) { delete label_obj; label_obj = 0; }
    if (corner[0]) { delete corner[0]; corner[0] = 0; }
    if (corner[1]) { delete corner[1]; corner[1] = 0; }
    if (corner[2]) { delete corner[2]; corner[2] = 0; }
    if (corner[3]) { delete corner[3]; corner[3] = 0; }
}

// 0x42d570 std::vector<rgl::vector4>::_Buy (the only member of this instance the linker kept)
template class std::vector<vector4>;

static __forceinline vec2 gm_vec2(float x, float y) { vec2 r; r.x = x; r.y = y; return r; }

// @ 0x42d5c0
gui_manager::gui_manager()
{
    WL_MARKER(12)
    reset();
    panel_color = f1718 = -1;
    *(vec2*)&p24a = gm_vec2(0.0f, 0.0f);
    *(vec2*)&p24a = gm_vec2(0.0f, 0.0f);
    show_cursor = 1;
    drag = 0;
    overlay = 0;
    label_obj = 0;
    fa0 = 0;
    fa4 = 0;
    fa8 = -0.03f;
    corner[0] = 0;
    corner[1] = 0;
    corner[2] = 0;
    corner[3] = 0;
    panel_bg = 0xff101010;
    *(vec2*)&mouse[0] = gm_vec2(0.0f, 0.0f);    // after panel_bg: mouse.y's store is then lexically behind it (equal-priority stores issue in lexical order)
    *(vec2*)&prev_mx = gm_vec2(0.0f, 0.0f);
    f1709 = 0;
    f170c = 0;
    hud_flag = 0;
    fov_k = 1.0f;
    wheel = 0;
    WL_MARKER(13)
}
