// Slice s0042c9b0: tstring split, the engine's warning/error reporters, text-log flush, a timer helper and the
// UI layer (widget input dispatch, gui_manager reset). Names provisional.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "gui.h"

extern char g_error_flag;           // 0x5d7070
extern int  g_counter;              // 0x5d7074
extern float g_mouse_x;             // 0x5d7094
extern float g_mouse_y;             // 0x5d7098
extern char g_ui_dirty;             // 0x5d8789
extern __int64 g_time[];            // 0x5e2948

// Text log stream (global at 0x5a9f60): see s00418a70. Slot 10 (0x28) is w_cstr.
struct log_stream : bin_writer_base {
    BIN_STREAM_SLOTS
    virtual int flush();
};
extern log_stream g_log;            // 0x5a9f60

// @ 0x42c9b0
tstring* tstring::split(const char& sep, int& count) const
{
    int n = 0;
    for (int i = 0; i < len; i++)
        if (buf[i] == sep) n++;
    count = n + 1;
    tstring* out = new tstring[n + 1];
    int k = 0;
    int start = 0;
    for (int i = 0; i < len; i++) {
        if (buf[i] == sep) {
            out[k] = substr(start, i);
            k++;
            start = i + 1;
        }
    }
    out[k] = substr(start, len);
    return out;
}

// @ 0x42cb70
void bump_counter() { g_counter++; }

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
void gui_timer::set_delay(float seconds)
{
    t = g_time[clock] - (int)(seconds * 100000.0f);
}

// @ 0x42cd60
char widget::refresh_all(unsigned mask)
{
    on_input(mask);
    refresh();
    return 0;
}

// @ 0x42cd80
void gui_manager::set_o564(gui_obj* p)
{
    if (o564) {
        delete o564;
        o564 = p;
        return;
    }
    o564 = p;
}

// @ 0x42cdb0
void gui_manager::set_o568(gui_obj* p, bool clear_lines)
{
    if (o568) delete o568;
    if (p == 0) {
        f16e0 = -1;
        f16e4 = 0xff101010;
    }
    if (clear_lines) {
        for (int i = 0; i < 0x50; i++)
            lines[i].assign("");
        fa4 = 0;
        fa8 = -0.03f;
    }
    o568 = p;
    fa0 = 0;
    f1700 = 0;
}

// @ 0x42ce50
void widget::wheel_step()
{
    if (pending & enabled_mask) {
        int w = wheel;
        if (w & 1) sel = sel + 1;
        else if (w & 2) sel = sel - 1;
        if (sel < sel_lo) sel = sel_lo;
        else if (sel > sel_hi) sel = sel_hi;
        if (w & 3) g_ui_dirty = 1;
    }
    pending = 0;
}

// @ 0x42cec0
void widget::wheel_focus()
{
    if ((pending & enabled_mask) && (wheel & 4) && focus) {
        unsigned top = (unsigned)-1;
        for (unsigned i = 0; i < children.size(); i++) {
            if (children[i]->hit_test(0x10000))
                top = i;
        }
        if ((int)top >= 0)
            focus->set_focus(top);
        focus->on_event(0x10000);
    }
    wheel_step();
}

// @ 0x42cf50
char widget::mouse_event(unsigned mask)
{
    char r = 0;
    if (flags & 3) return 0;
    if (mask & 7) {
        for (std::vector<widget*>::iterator it = children.begin(); it != children.end(); ++it) {
            widget* c = *it;
            if ((c->flags & 3) == 0) {
                bool inside = false;
                if (c->x_lo + c->x0 <= g_mouse_x && g_mouse_x < c->x1 + c->x0 &&
                    c->y_lo + c->y0 <= g_mouse_y && g_mouse_y < c->y1 + c->y0)
                    inside = true;
                if (!(mask & 4) && inside) {
                    if (c->hover) {
                        c->on_event(1);
                    } else {
                        c->on_event(2);
                        (*it)->hover = 1;
                    }
                } else if (c->hover) {
                    c->on_event(4);
                    (*it)->hover = 0;
                }
            }
        }
    } else {
        for (int i = (int)children.size() - 1; i >= 0 && !r; i--) {
            widget* c = children[i];
            r = 0;
            if ((c->flags & 3) == 0) {
                if (c->x0 + c->x_lo <= g_mouse_x && g_mouse_x < c->x0 + c->x1 &&
                    c->y0 + c->y_lo <= g_mouse_y && g_mouse_y < c->y0 + c->y1)
                    r = c->on_event(mask);
            }
        }
    }
    on_input(mask);
    refresh();
    return r;
}

// @ 0x42d170
char widget::handle_event(unsigned mask)
{
    if (mouse_event(mask)) return 1;
    if (wants_redraw && (mask & 0x10)) {
        g_ui_dirty = 1;
        return 1;
    }
    return 0;
}

// @ 0x42d1b0
void gui_manager::reset()
{
    f1714 = 1;
    fb0ac = 0;
    f514 = 0.75f;
    f10 = 0;
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
    if (o564) delete o564;
    o564 = 0;
    if (o568) delete o568;
    f16e0 = -1;
    f16e4 = 0xff101010;
    for (int i = 0; i < 0x50; i++)
        lines[i].assign("");
    fa4 = 0;
    fa8 = -0.03f;
    fa0 = 0;
    o568 = 0;
    f1700 = 0;
    if (o56c) delete o56c;
    o56c = 0;
    f1704 = 0;
    f1709 = 0;
    m1480.f10c = 0;
    m1220.f10c = 0;
}
