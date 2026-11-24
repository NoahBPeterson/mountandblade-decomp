// port/platform/window.cpp: USER32 windows, the message loop and the cursor over SDL3.
//   RegisterClass / CreateWindowEx  -> SDL_Window (bordered / borderless / resizable from the Win32 style bits)
//   PeekMessage / GetMessage        -> SDL_PollEvent / SDL_WaitEventTimeout, translated to WM_* messages in a queue
//   DispatchMessage / DefWindowProc -> the registered WNDPROC; DefWindowProc implements the default handling
//   ShowCursor / SetCursor / ClipCursor / GetCursorPos / SetCursorPos -> SDL cursor, mouse grab, warp
// The non-client area is not emulated: the window's client area is the SDL window, so AdjustWindowRect is the identity
// and the frame / caption / menu metrics are 0 (the OS draws the decorations outside what the game sees). Coordinates
// are SDL window points (== pixels unless MBP_HIGHDPI=1 asks for a high-density drawable).
#include "mbp_win.h"
#if !defined(_WIN32)
#include <execinfo.h>       // MessageBoxA's MB_CRASH_BT stack (Windows: main.cpp's unhandled-exception filter)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <set>
#include <algorithm>
#include <ctype.h>

using namespace mbp;

namespace {
typedef void (CALLBACK* TIMERPROC_)(HWND, UINT, UINT_PTR, DWORD);
struct Timer { HWND hwnd; UINT_PTR id; UINT ms; Uint64 next; TIMERPROC_ proc; };
struct CursorObj { int sys; SDL_Cursor* cur; };
struct MenuObj { std::map<UINT, UINT> flags; };
struct CreateStruct { LPVOID lpCreateParams; HINSTANCE hInstance; HMENU hMenu; HWND hwndParent; int cy, cx, y, x; LONG style; LPCSTR lpszName, lpszClass; DWORD dwExStyle; };

std::recursive_mutex g_mu;                // guards the queue, the window set and the timers (PostMessage may come from a thread)
std::map<std::string, Class*>& g_classes() { static std::map<std::string, Class*>* v = new std::map<std::string, Class*>(); return *v; }   // constructed on first use: src/'s static initializers (GameApp, world_app.cpp) call in before this file's would run
std::set<Win*>& g_wins() { static std::set<Win*>* v = new std::set<Win*>(); return *v; }   // constructed on first use: src/'s static initializers (GameApp, world_app.cpp) call in before this file's would run
std::vector<Win*> g_order;                // creation order; the first live top-level window is the "main" window
std::deque<MSG>& g_queue() { static std::deque<MSG>* v = new std::deque<MSG>(); return *v; }   // constructed on first use: src/'s static initializers (GameApp, world_app.cpp) call in before this file's would run
std::vector<Timer> g_timers;
UINT_PTR g_next_timer = 1;
bool g_quit = false;
int g_quit_code = 0;
std::thread::id g_main_thread;
std::once_flag g_init_once;
bool g_init_ok = false;
int g_cursor_count = 0;                   // ShowCursor display counter
bool g_cursor_shape_null = false;         // SetCursor(NULL)
bool g_device_cursor = false;             // the D3D device cursor (gfx lane) overrides SetCursor(NULL)
HCURSOR g_cur_handle = nullptr;
bool g_virt_valid = false;                // GetCursorPos answers g_virt until the next real mouse event
int g_virt_x = 0, g_virt_y = 0;
int g_warp_x = 0, g_warp_y = 0, g_warp_pending = 0;
// Relative mouse mode: while the pointer is hidden (the game's mission view) SDL delivers raw motion that continues past the
// window edge, as DirectInput does on Windows (an absolute pointer stops at the edge and so did the camera's turning).
// The hidden pointer is then virtual: g_rel_x / g_rel_y (screen points, clamped to the clip rectangle) answer GetCursorPos.
bool g_rel = false;
void cur_global(float& x, float& y);
void rel_clamp();
double g_rel_x = 0, g_rel_y = 0;
RECT g_clip = {0, 0, 0, 0};
bool g_clip_on = false;
bool g_clip_is_window = false;            // the clip rectangle was the window's own: it follows the window's size and position
void clip_apply(Win* w);
Uint32 g_wake = 0;
CursorObj g_cursors[8];
HINSTANCE__ g_icon_obj;
HWND g_capture_win = nullptr;

const char* kControlClasses[] = {"button", "edit", "static", "combobox", "listbox", "msctls_trackbar32", "systabcontrol32",
                                 "richedit20a", "richedit20w", "richedit", "scrollbar"};

std::string lower(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
bool on_main() { return std::this_thread::get_id() == g_main_thread; }

void init_once()
{
    g_main_thread = std::this_thread::get_id();
    SDL_SetHint("SDL_MOUSE_FOCUS_CLICKTHROUGH", "1");
    SDL_SetHint("SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK", "0");
    SDL_SetHint("SDL_VIDEO_ALLOW_SCREENSAVER", "0");
    // macOS: fullscreen covers the whole display (the notch band too) instead of a Space below the menu bar; no effect elsewhere
    SDL_SetHint("SDL_VIDEO_MAC_FULLSCREEN_SPACES", "0");
    SDL_SetHint("SDL_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY", "0");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "mb: SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    g_wake = SDL_RegisterEvents(1);
    g_init_ok = true;
}

Win* window_by_sdl(SDL_WindowID id)
{
    for (Win* w : g_wins()) if (w->sdl && w->sdl_id == id) return w;
    return nullptr;
}
bool contains(Win* top, Win* w)
{
    for (; w; w = w->parent) if (w == top) return true;
    return false;
}
DWORD tick() { return (DWORD)SDL_GetTicks(); }

void queue_msg(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    MSG msg;
    memset(&msg, 0, sizeof msg);
    msg.hwnd = h; msg.message = m; msg.wParam = wp; msg.lParam = lp; msg.time = tick();
    float fx = 0, fy = 0;
    cur_global(fx, fy);
    msg.pt.x = (LONG)fx; msg.pt.y = (LONG)fy;
    std::lock_guard<std::recursive_mutex> g(g_mu);
    g_queue().push_back(msg);
}

// the pointer in screen points: the virtual one in relative mode, else the real one
void cur_global(float& x, float& y)
{
    if (g_rel) { x = (float)g_rel_x; y = (float)g_rel_y; return; }
    SDL_GetGlobalMouseState(&x, &y);
}

// keep the virtual pointer inside the clip rectangle (ClipCursor), else the main window
void rel_clamp()
{
    double l, t, r, b;
    if (g_clip_on) {
        l = g_clip.left; t = g_clip.top; r = g_clip.right - 1; b = g_clip.bottom - 1;
    } else {
        Win* w = main_window();
        if (!w) return;
        l = w->x; t = w->y; r = w->x + w->w - 1; b = w->y + w->h - 1;
    }
    if (g_rel_x < l) g_rel_x = l; else if (g_rel_x > r) g_rel_x = r;
    if (g_rel_y < t) g_rel_y = t; else if (g_rel_y > b) g_rel_y = b;
}

void set_relative(bool on)
{
    if (on == g_rel) return;
    Win* w = main_window();
    if (!w || !w->sdl) return;
    if (on) {
        float x = 0, y = 0;
        if (g_virt_valid) { x = (float)g_virt_x; y = (float)g_virt_y; }
        else SDL_GetGlobalMouseState(&x, &y);
        g_rel_x = x; g_rel_y = y;
        g_rel = true;
        SDL_SetWindowRelativeMouseMode(w->sdl, true);
    } else {
        g_rel = false;
        SDL_SetWindowRelativeMouseMode(w->sdl, false);
        g_warp_x = (int)g_rel_x; g_warp_y = (int)g_rel_y; g_warp_pending = 1;
        g_virt_x = g_warp_x; g_virt_y = g_warp_y; g_virt_valid = true;
        SDL_WarpMouseGlobal((float)g_rel_x, (float)g_rel_y);
    }
}

void apply_cursor_visibility()
{
    bool vis = g_cursor_count >= 0 && (!g_cursor_shape_null || g_device_cursor);
    if (vis) SDL_ShowCursor(); else SDL_HideCursor();
    set_relative(!vis);
}

WPARAM mouse_keys()
{
    unsigned b = mouse_button_mask();
    WPARAM w = 0;
    if (b & 1) w |= 1;
    if (b & 4) w |= 2;
    if (b & 2) w |= 0x10;
    if (key_is_down(SDL_SCANCODE_LSHIFT) || key_is_down(SDL_SCANCODE_RSHIFT)) w |= 4;
    if (key_is_down(SDL_SCANCODE_LCTRL) || key_is_down(SDL_SCANCODE_RCTRL)) w |= 8;
    return w;
}

void queue_size(Win* w)
{
    SDL_WindowFlags f = SDL_GetWindowFlags(w->sdl);
    WPARAM t = (f & SDL_WINDOW_MINIMIZED) ? SIZE_MINIMIZED : (f & SDL_WINDOW_MAXIMIZED) ? SIZE_MAXIMIZED : SIZE_RESTORED;
    queue_msg((HWND)w, WM_SIZE, t, MAKELPARAM(w->w, w->h));
}

void handle_event(const SDL_Event& ev0)
{
    SDL_Event e = ev0;
    switch (e.type) {
    case SDL_EVENT_QUIT: {
        Win* m = main_window();
        if (m) queue_msg((HWND)m, WM_CLOSE, 0, 0);
        else { g_quit = true; }
        break;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (Win* w = window_by_sdl(e.window.windowID)) queue_msg((HWND)w, WM_CLOSE, 0, 0);
        break;
    case SDL_EVENT_WINDOW_RESIZED: {
        Win* w = window_by_sdl(e.window.windowID);
        if (!w) break;
        w->w = e.window.data1; w->h = e.window.data2;
        if (g_clip_is_window && w == main_window()) clip_apply(w);
        queue_size(w);
        w->invalid = true;
        break;
    }
    case SDL_EVENT_WINDOW_MINIMIZED: case SDL_EVENT_WINDOW_MAXIMIZED: case SDL_EVENT_WINDOW_RESTORED: {
        Win* w = window_by_sdl(e.window.windowID);
        if (w) { queue_size(w); w->invalid = true; }
        break;
    }
    case SDL_EVENT_WINDOW_MOVED: {
        Win* w = window_by_sdl(e.window.windowID);
        if (!w) break;
        w->x = e.window.data1; w->y = e.window.data2;
        if (g_clip_is_window && w == main_window()) clip_apply(w);
        queue_msg((HWND)w, WM_MOVE, 0, MAKELPARAM(w->x, w->y));
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_GAINED: {
        Win* w = window_by_sdl(e.window.windowID);
        if (!w) break;
        w->active = true;
        queue_msg((HWND)w, WM_ACTIVATEAPP, 1, 0);
        queue_msg((HWND)w, WM_ACTIVATE, 1 /*WA_ACTIVE*/, 0);
        queue_msg((HWND)w, WM_SETFOCUS, 0, 0);
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST: {
        Win* w = window_by_sdl(e.window.windowID);
        on_sdl_event_for_input(e);
        if (!w) break;
        w->active = false;
        queue_msg((HWND)w, WM_ACTIVATE, 0 /*WA_INACTIVE*/, 0);
        queue_msg((HWND)w, WM_KILLFOCUS, 0, 0);
        queue_msg((HWND)w, WM_ACTIVATEAPP, 0, 0);
        break;
    }
    case SDL_EVENT_WINDOW_EXPOSED: case SDL_EVENT_WINDOW_SHOWN:
        if (Win* w = window_by_sdl(e.window.windowID)) w->invalid = true;
        break;
    case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: {
        Win* w = window_by_sdl(e.key.windowID);
        on_sdl_event_for_input(e);
        if (!w) break;
        bool ext = false;
        int vk = keymap_vk_for_scancode(e.key.scancode, &ext);
        if (!vk) break;
        int dik = keymap_dik_for_scancode(e.key.scancode);
        bool down = e.key.down;
        LPARAM lp = 1 | ((LPARAM)(dik & 0x7F) << 16) | ((LPARAM)(ext ? 1 : 0) << 24);
        bool alt = (e.key.mod & SDL_KMOD_ALT) != 0;
        if (alt) lp |= (LPARAM)1 << 29;
        if (e.key.repeat) lp |= (LPARAM)1 << 30;
        if (!down) lp |= (LPARAM)1 << 30 | ((LPARAM)1 << 31);
        bool sys = alt || vk == 0x12 || vk == 0x79 /*F10*/;
        queue_msg((HWND)w, down ? (sys ? WM_SYSKEYDOWN : WM_KEYDOWN) : (sys ? WM_SYSKEYUP : WM_KEYUP), (WPARAM)vk, (LPARAM)(int32_t)(uint32_t)lp);
        break;
    }
    case SDL_EVENT_TEXT_INPUT: {
        Win* w = window_by_sdl(e.text.windowID);
        if (!w || !e.text.text) break;
        std::string ansi = utf8_to_ansi(e.text.text);
        for (unsigned char c : ansi)
            if (c >= 0x20 && c != 0x7F) queue_msg((HWND)w, WM_CHAR, c, 1);
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        Win* w = window_by_sdl(e.motion.windowID);
        if (g_rel) {
            g_rel_x += e.motion.xrel; g_rel_y += e.motion.yrel;
            rel_clamp();
            e.motion.x = (float)(g_rel_x - (w ? w->x : 0)); e.motion.y = (float)(g_rel_y - (w ? w->y : 0));
        }
        int gx = (int)e.motion.x + (w ? w->x : 0), gy = (int)e.motion.y + (w ? w->y : 0);
        if (g_rel) {
            g_virt_valid = false;
        } else if (g_warp_pending > 0 && abs(gx - g_warp_x) <= 1 && abs(gy - g_warp_y) <= 1) {
            --g_warp_pending;                 // the echo of SetCursorPos: not physical movement
            e.motion.xrel = 0; e.motion.yrel = 0;
        } else {
            g_virt_valid = false;
        }
        on_sdl_event_for_input(e);
        if (w) {
            queue_msg((HWND)w, WM_SETCURSOR, (WPARAM)w, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
            queue_msg((HWND)w, WM_MOUSEMOVE, mouse_keys(), MAKELPARAM((short)(int)e.motion.x, (short)(int)e.motion.y));
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: {
        Win* w = window_by_sdl(e.button.windowID);
        if (g_rel && w) { e.button.x = (float)(g_rel_x - w->x); e.button.y = (float)(g_rel_y - w->y); }
        on_sdl_event_for_input(e);
        if (!w) break;
        bool d = e.button.down;
        UINT m = 0;
        switch (e.button.button) {
        case SDL_BUTTON_LEFT: m = d ? WM_LBUTTONDOWN : WM_LBUTTONUP; break;
        case SDL_BUTTON_RIGHT: m = d ? WM_RBUTTONDOWN : WM_RBUTTONUP; break;
        case SDL_BUTTON_MIDDLE: m = d ? 0x207 : 0x208; break;
        case SDL_BUTTON_X1: case SDL_BUTTON_X2: m = d ? 0x20B : 0x20C; break;
        }
        if (!m) break;
        WPARAM keys = mouse_keys();
        if (e.button.button >= SDL_BUTTON_X1) keys |= (WPARAM)(e.button.button == SDL_BUTTON_X1 ? 1 : 2) << 16;
        if (d && e.button.clicks == 2 && w->cls && (w->cls->style & CS_DBLCLKS)) m += 2;    // WM_xBUTTONDBLCLK
        queue_msg((HWND)w, m, keys, MAKELPARAM((short)(int)e.button.x, (short)(int)e.button.y));
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        Win* w = window_by_sdl(e.wheel.windowID);
        on_sdl_event_for_input(e);
        if (!w) break;
        float dir = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        float gx = 0, gy = 0;
        cur_global(gx, gy);
        LPARAM lp = MAKELPARAM((short)(int)gx, (short)(int)gy);
        if (e.wheel.y != 0) queue_msg((HWND)w, 0x20A, ((WPARAM)(unsigned short)(short)(int)(e.wheel.y * dir * 120) << 16) | (mouse_keys() & 0xFFFF), lp);
        if (e.wheel.x != 0) queue_msg((HWND)w, 0x20E, ((WPARAM)(unsigned short)(short)(int)(e.wheel.x * dir * 120) << 16) | (mouse_keys() & 0xFFFF), lp);
        break;
    }
    default:
        break;
    }
}

void fire_timers()
{
    Uint64 now = SDL_GetTicks();
    std::lock_guard<std::recursive_mutex> g(g_mu);
    for (Timer& t : g_timers) {
        if (now >= t.next) {
            bool queued = false;
            for (const MSG& m : g_queue()) if (m.message == WM_TIMER && m.hwnd == t.hwnd && m.wParam == t.id) { queued = true; break; }
            if (!queued) {
                MSG msg; memset(&msg, 0, sizeof msg);
                msg.hwnd = t.hwnd; msg.message = WM_TIMER; msg.wParam = t.id; msg.lParam = (LPARAM)t.proc; msg.time = (DWORD)now;
                g_queue().push_back(msg);
            }
            t.next = now + (t.ms ? t.ms : 1);
        }
    }
}

bool match_filter(const MSG& m, HWND hwnd, UINT lo, UINT hi)
{
    if (hwnd) {
        Win* top = win_of(hwnd);
        Win* w = win_of(m.hwnd);
        if (!top || !w || !contains(top, w)) return false;
    }
    if (lo || hi) {
        if (m.message < lo || (hi && m.message > hi)) return false;
    }
    return true;
}

// next message for (hwnd, lo, hi): queued, then WM_QUIT, then timers, then WM_PAINT
bool take_message(MSG* out, HWND hwnd, UINT lo, UINT hi, bool remove)
{
    fire_timers();
    std::lock_guard<std::recursive_mutex> g(g_mu);
    for (auto it = g_queue().begin(); it != g_queue().end(); ++it) {
        if (!match_filter(*it, hwnd, lo, hi)) continue;
        *out = *it;
        if (remove) g_queue().erase(it);
        return true;
    }
    if (g_quit && !hwnd && (!lo || (lo <= WM_QUIT && (!hi || hi >= WM_QUIT)))) {
        memset(out, 0, sizeof *out);
        out->message = WM_QUIT; out->wParam = (WPARAM)g_quit_code; out->time = tick();
        if (remove) g_quit = false;
        return true;
    }
    if (!lo || (lo <= WM_PAINT && (!hi || hi >= WM_PAINT))) {
        for (Win* w : g_order) {
            if (!w->invalid || !w->visible || !w->sdl) continue;
            if (hwnd && !contains(win_of(hwnd), w)) continue;
            memset(out, 0, sizeof *out);
            out->hwnd = (HWND)w; out->message = WM_PAINT; out->time = tick();
            if (remove) w->invalid = false;
            return true;
        }
    }
    return false;
}

void drain_sdl()
{
    if (!g_init_ok || !on_main()) return;
    SDL_Event e;
    while (SDL_PollEvent(&e)) handle_event(e);
}

LRESULT def_proc(Win* w, UINT m, WPARAM wp, LPARAM lp, bool wide)
{
    switch (m) {
    case WM_NCCREATE: return TRUE;
    case WM_CLOSE: DestroyWindow((HWND)w); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_NCHITTEST: return HTCLIENT;
    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT) {
            if (w->cls && w->cls->cursor) SetCursor(w->cls->cursor);
            else SetCursor(LoadCursorA(nullptr, IDC_ARROW));
        }
        return 0;
    }
    case WM_SETTEXT: {
        w->title = wide ? wide_to_utf8((const wchar_t*)lp) : ansi_to_utf8((const char*)lp);
        if (w->sdl) SDL_SetWindowTitle(w->sdl, w->title.c_str());
        w->ctl.text = w->title;
        return TRUE;
    }
    case 0x000D: {       // WM_GETTEXT
        std::string a = utf8_to_ansi(w->title.c_str());
        if (wp == 0) return 0;
        size_t n = std::min<size_t>(a.size(), wp - 1);
        memcpy((char*)lp, a.data(), n);
        ((char*)lp)[n] = 0;
        return (LRESULT)n;
    }
    case 0x000E: return (LRESULT)utf8_to_ansi(w->title.c_str()).size();     // WM_GETTEXTLENGTH
    case WM_PAINT: w->invalid = false; return 0;
    case WM_SYSCOMMAND:
        switch (wp & 0xFFF0) {
        case 0xF060: queue_msg((HWND)w, WM_CLOSE, 0, 0); break;                // SC_CLOSE
        case 0xF020: if (w->sdl) SDL_MinimizeWindow(w->sdl); break;            // SC_MINIMIZE
        case SC_MAXIMIZE: if (w->sdl) SDL_MaximizeWindow(w->sdl); break;
        case 0xF120: if (w->sdl) SDL_RestoreWindow(w->sdl); break;             // SC_RESTORE
        }
        return 0;
    case WM_SYSKEYDOWN:
        if (wp == 0x73 /*VK_F4*/) { SendMessageA((HWND)w, WM_SYSCOMMAND, 0xF060, 0); return 0; }
        return 0;
    case WM_POWERBROADCAST: return TRUE;
    case 0x0083: return 0;                                                      // WM_NCCALCSIZE
    default: return 0;
    }
}

// outer == client; window styles to SDL
void apply_style(Win* w, LONG style)
{
    w->style = style;
    if (!w->sdl) return;
    bool popup = (style & WS_POPUP) != 0 || !(style & WS_CAPTION);
    SDL_SetWindowBordered(w->sdl, !popup);
    SDL_SetWindowResizable(w->sdl, (style & WS_THICKFRAME) != 0);
}

HWND create_window(DWORD ex, const std::string& clsname, const std::string& title, DWORD style, int X, int Y, int W, int H,
                   HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    std::call_once(g_init_once, init_once);
    std::string lc = lower(clsname);
    Class* cls = nullptr;
    bool builtin = false;
    {
        std::lock_guard<std::recursive_mutex> g(g_mu);
        auto it = g_classes().find(lc);
        if (it != g_classes().end()) cls = it->second;
    }
    for (const char* b : kControlClasses) if (lc == b) builtin = true;
    if (!cls && !builtin) { set_last_error(1407); return nullptr; }      // ERROR_CANNOT_FIND_WND_CLASS

    Win* w = new Win;
    w->cls = cls; w->proc = cls ? cls->proc : nullptr;
    w->style = (LONG)style; w->exstyle = (LONG)ex; w->title = title; w->menu = menu; w->hinst = inst;
    w->parent = win_of(parent);
    if (cls) w->extra.assign((size_t)std::max(cls->wnd_extra, 0), 0);
    w->enabled = !(style & WS_DISABLED);
    w->ctl.text = title;
    bool child = (style & WS_CHILD) != 0 || builtin;
    if (W == CW_USEDEFAULT || W <= 0) W = 800;
    if (H == CW_USEDEFAULT || H <= 0) H = 600;
    w->w = W; w->h = H; w->x = X == CW_USEDEFAULT ? 0 : X; w->y = Y == CW_USEDEFAULT ? 0 : Y;
    if (child) {
        w->is_control = builtin;
        if (w->parent) w->parent->children.push_back(w);
    } else {
        if (!g_init_ok) { delete w; set_last_error(1400); return nullptr; }
        SDL_PropertiesID p = SDL_CreateProperties();
        SDL_SetStringProperty(p, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title.c_str());
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, W);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, H);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_X_NUMBER, X == CW_USEDEFAULT ? SDL_WINDOWPOS_CENTERED : X);
        SDL_SetNumberProperty(p, SDL_PROP_WINDOW_CREATE_Y_NUMBER, Y == CW_USEDEFAULT ? SDL_WINDOWPOS_CENTERED : Y);
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, (style & WS_POPUP) || !(style & WS_CAPTION));
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, (style & WS_THICKFRAME) != 0);
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_ALWAYS_ON_TOP_BOOLEAN, (ex & WS_EX_TOPMOST) != 0);
        const char* hd = getenv("MBP_HIGHDPI");
        SDL_SetBooleanProperty(p, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, hd && *hd == '1');
        w->sdl = SDL_CreateWindowWithProperties(p);
        SDL_DestroyProperties(p);
        if (!w->sdl) {
            fprintf(stderr, "mb: SDL_CreateWindow failed: %s\n", SDL_GetError());
            delete w;
            set_last_error(1400);
            return nullptr;
        }
        w->sdl_id = SDL_GetWindowID(w->sdl);
        SDL_GetWindowPosition(w->sdl, &w->x, &w->y);
        SDL_GetWindowSize(w->sdl, &w->w, &w->h);
        SDL_StartTextInput(w->sdl);
    }
    {
        std::lock_guard<std::recursive_mutex> g(g_mu);
        g_wins().insert(w);
        g_order.push_back(w);
    }
    // WM_NCCREATE / WM_CREATE: lpCreateParams reaches the WNDPROC exactly as on Windows
    CreateStruct cs = {param, inst, menu, parent, H, W, Y, X, (LONG)style, title.c_str(), clsname.c_str(), ex};
    if (!w->is_control) {
        LRESULT r = SendMessageA((HWND)w, WM_NCCREATE, 0, (LPARAM)&cs);
        if (!r) { DestroyWindow((HWND)w); return nullptr; }
        r = SendMessageA((HWND)w, WM_CREATE, 0, (LPARAM)&cs);
        if (r == -1) { DestroyWindow((HWND)w); return nullptr; }
    }
    w->created = true;
    if (w->sdl) {
        queue_msg((HWND)w, WM_SIZE, SIZE_RESTORED, MAKELPARAM(w->w, w->h));
        queue_msg((HWND)w, WM_MOVE, 0, MAKELPARAM(w->x, w->y));
        if (style & WS_VISIBLE) ShowWindow((HWND)w, SW_SHOWNORMAL);
    } else {
        w->visible = (style & WS_VISIBLE) != 0;
    }
    return (HWND)w;
}

std::string class_name_of(const void* name, bool wide)
{
    if ((uintptr_t)name < 0x10000) return "#" + std::to_string((uintptr_t)name);
    return wide ? wide_to_utf8((const wchar_t*)name) : std::string((const char*)name);
}
ATOM register_class(const std::string& name, WNDPROC proc, UINT style, int ce, int we, HICON icon, HCURSOR cur, HBRUSH bg)
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    std::string k = lower(name);
    if (g_classes().count(k)) { set_last_error(1410); return 0; }      // ERROR_CLASS_ALREADY_EXISTS
    Class* c = new Class;
    c->name = name; c->proc = proc; c->style = style; c->cls_extra = ce; c->wnd_extra = we; c->icon = icon; c->cursor = cur; c->bg = bg;
    g_classes()[k] = c;
    return (ATOM)(0xC000 + g_classes().size());
}
}   // namespace

// =====================================================================================================================
// internal API (mbp_win.h / mb_sdl.h)
// =====================================================================================================================
namespace mbp {
void register_window(Win* w)
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    g_wins().insert(w);
    g_order.push_back(w);
}
Win* main_window()
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    for (Win* w : g_order) if (w->sdl && g_wins().count(w)) return w;
    return nullptr;
}
bool any_window_alive() { std::lock_guard<std::recursive_mutex> g(g_mu); return main_window() != nullptr; }
HWND window_from_sdl_id(SDL_WindowID id) { std::lock_guard<std::recursive_mutex> g(g_mu); return (HWND)window_by_sdl(id); }
void post_to_queue(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    queue_msg(h, m, wp, lp);
    if (!on_main() && g_init_ok) {                    // wake a GetMessage blocked in SDL_WaitEvent
        SDL_Event e; SDL_zero(e); e.type = g_wake;
        SDL_PushEvent(&e);
    }
}
LRESULT send_to_window(Win* w, UINT msg, WPARAM wp, LPARAM lp)
{
    if (w->is_control) {
        bool handled = false;
        LRESULT r = control_message((HWND)w, msg, wp, lp, &handled);
        if (handled) return r;
        return def_proc(w, msg, wp, lp, false);
    }
    if (w->is_dialog) {
        if (w->dproc) return (LRESULT)w->dproc((HWND)w, msg, wp, lp);
        return 0;
    }
    if (w->proc) return w->proc((HWND)w, msg, wp, lp);
    return def_proc(w, msg, wp, lp, false);
}
void destroy_window_tree(Win* w)
{
    for (size_t i = 0; i < w->children.size(); ++i) destroy_window_tree(w->children[i]);
    w->children.clear();
    {
        std::lock_guard<std::recursive_mutex> g(g_mu);
        g_wins().erase(w);
        g_order.erase(std::remove(g_order.begin(), g_order.end(), w), g_order.end());
        g_queue().erase(std::remove_if(g_queue().begin(), g_queue().end(), [&](const MSG& m) { return m.hwnd == (HWND)w; }), g_queue().end());
        g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(), [&](const Timer& t) { return t.hwnd == (HWND)w; }), g_timers.end());
    }
    if (w->parent) {
        auto& c = w->parent->children;
        c.erase(std::remove(c.begin(), c.end(), w), c.end());
    }
    if (w->sdl) SDL_DestroyWindow(w->sdl);
    w->sdl = nullptr;
    w->magic = 0;
    // The Win object itself is kept: a stale HWND must fail validation (magic 0) instead of touching freed memory.
}
}   // namespace mbp

extern "C" {
bool mbp_sdl_init(void)
{
    std::call_once(g_init_once, init_once);
    return g_init_ok;
}
void mbp_pump_events(void) { drain_sdl(); }
SDL_Window* mbp_sdl_window(HWND__* h)
{
    Win* w = win_of((HWND)h);
    return w ? w->sdl : nullptr;
}
HWND__* mbp_hwnd_from_sdl(SDL_Window* s)
{
    if (!s) return nullptr;
    return (HWND__*)window_from_sdl_id(SDL_GetWindowID(s));
}
void mbp_set_device_cursor(bool visible)
{
    g_device_cursor = visible;
    mbp_sdl_init();
    apply_cursor_visibility();
}

// ---- classes / windows -----------------------------------------------------------------------------------------------
ATOM WINAPI RegisterClassA(const WNDCLASSA* c)
{
    return register_class(class_name_of(c->lpszClassName, false), c->lpfnWndProc, c->style, c->cbClsExtra, c->cbWndExtra, c->hIcon, c->hCursor, c->hbrBackground);
}
ATOM WINAPI RegisterClassExW(const WNDCLASSEXW* c)
{
    return register_class(class_name_of(c->lpszClassName, true), c->lpfnWndProc, c->style, c->cbClsExtra, c->cbWndExtra, c->hIcon, c->hCursor, c->hbrBackground);
}
HWND WINAPI CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR title, DWORD style, int X, int Y, int W, int H, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    return create_window(ex, class_name_of(cls, false), title ? ansi_to_utf8(title) : "", style, X, Y, W, H, parent, menu, inst, param);
}
HWND WINAPI CreateWindowExW(DWORD ex, LPCWSTR cls, LPCWSTR title, DWORD style, int X, int Y, int W, int H, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    // src/platform/dxframework/d3dapp.cpp passes its 8-bit title ("Mount&Blade") through this wide entry point (a cast, as in the
    // original, where Windows then shows it misread as UTF-16). A UTF-32 title never has a first unit above U+10FFFF, but
    // four ASCII bytes read as one always do, so that identifies the narrow string. UTF-16 (mingw's 2-byte wchar_t): the
    // cast packs two ASCII characters per unit (0x6F4D for "Mo"), while a real L"..." ASCII title keeps the high bytes zero,
    // so any non-ASCII unit before the terminator identifies the narrow string there.
    std::string t;
    if (title) {
        bool narrow = false;
        if (sizeof(wchar_t) == 2) {
            for (const wchar_t* u = title; *u; ++u) if ((unsigned)*u > 0xFFu) { narrow = true; break; }
        } else {
            narrow = (uint32_t)title[0] > 0x10FFFFu;
        }
        if (narrow) t = ansi_to_utf8((const char*)title);
        else t = wide_to_utf8(title);
    }
    return create_window(ex, class_name_of(cls, true), t, style, X, Y, W, H, parent, menu, inst, param);
}
BOOL WINAPI DestroyWindow(HWND h)
{
    Win* w = win_of(h);
    if (!w) { set_last_error(1400); return FALSE; }
    if (w->is_dialog && w->modal) { w->ended = true; }
    if (!w->is_control) {
        SendMessageA(h, WM_DESTROY, 0, 0);
        SendMessageA(h, 0x82 /*WM_NCDESTROY*/, 0, 0);
    }
    destroy_window_tree(w);
    return TRUE;
}
BOOL WINAPI IsWindow(HWND h) { return win_of(h) != nullptr; }
BOOL WINAPI ShowWindow(HWND h, int cmd)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    bool was = w->visible;
    if (w->sdl) {
        switch (cmd) {
        case SW_HIDE: SDL_HideWindow(w->sdl); w->visible = false; break;
        case SW_SHOWMAXIMIZED: SDL_ShowWindow(w->sdl); SDL_MaximizeWindow(w->sdl); w->visible = true; break;
        case SW_SHOWMINIMIZED: case SW_MINIMIZE: SDL_MinimizeWindow(w->sdl); w->visible = true; break;
        case SW_RESTORE: SDL_ShowWindow(w->sdl); SDL_RestoreWindow(w->sdl); w->visible = true; break;
        default: SDL_ShowWindow(w->sdl); SDL_RaiseWindow(w->sdl); w->visible = true; break;
        }
        if (w->visible) w->invalid = true;
    } else {
        w->visible = cmd != SW_HIDE;
    }
    return was;
}
BOOL WINAPI UpdateWindow(HWND h)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    if (w->invalid && w->visible) {
        w->invalid = false;
        SendMessageA(h, WM_PAINT, 0, 0);
    }
    return TRUE;
}
BOOL WINAPI EnableWindow(HWND h, BOOL en)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    bool was = !w->enabled;
    w->enabled = en != 0;
    return was;
}
BOOL WINAPI IsIconic(HWND h) { Win* w = win_of(h); return w && w->sdl && (SDL_GetWindowFlags(w->sdl) & SDL_WINDOW_MINIMIZED); }
BOOL WINAPI IsZoomed(HWND h) { Win* w = win_of(h); return w && w->sdl && (SDL_GetWindowFlags(w->sdl) & SDL_WINDOW_MAXIMIZED); }
HWND WINAPI SetFocus(HWND h)
{
    Win* w = win_of(h);
    HWND prev = GetFocus();
    if (w && w->sdl) SDL_RaiseWindow(w->sdl);
    return prev;
}
HWND WINAPI GetFocus()
{
    if (!g_init_ok) return nullptr;
    SDL_Window* f = SDL_GetKeyboardFocus();
    return f ? (HWND)window_from_sdl_id(SDL_GetWindowID(f)) : nullptr;
}
HWND WINAPI SetActiveWindow(HWND h) { return SetFocus(h); }
BOOL WINAPI SetForegroundWindow(HWND h) { Win* w = win_of(h); if (!w || !w->sdl) return FALSE; SDL_RaiseWindow(w->sdl); return TRUE; }
HWND WINAPI GetForegroundWindow() { return GetFocus(); }
HWND WINAPI GetParent(HWND h) { Win* w = win_of(h); return w ? (HWND)w->parent : nullptr; }

BOOL WINAPI GetWindowRect(HWND h, LPRECT r)
{
    Win* w = win_of(h);
    if (!w) { memset(r, 0, sizeof *r); return FALSE; }
    r->left = w->x; r->top = w->y; r->right = w->x + w->w; r->bottom = w->y + w->h;
    return TRUE;
}
BOOL WINAPI GetClientRect(HWND h, LPRECT r)
{
    Win* w = win_of(h);
    if (!w) { memset(r, 0, sizeof *r); return FALSE; }
    r->left = 0; r->top = 0; r->right = w->w; r->bottom = w->h;
    return TRUE;
}
BOOL WINAPI SetWindowPos(HWND h, HWND after, int X, int Y, int cx, int cy, UINT flags)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    if (!(flags & SWP_NOMOVE)) { w->x = X; w->y = Y; if (w->sdl) SDL_SetWindowPosition(w->sdl, X, Y); }
    if (!(flags & SWP_NOSIZE) && cx > 0 && cy > 0) {
        w->w = cx; w->h = cy;
        if (w->sdl) SDL_SetWindowSize(w->sdl, cx, cy);
    }
    if (w->sdl && !(flags & SWP_NOZORDER)) {
        if (after == HWND_TOPMOST) SDL_SetWindowAlwaysOnTop(w->sdl, true);
        else if (after == HWND_NOTOPMOST || after == HWND_TOP) SDL_SetWindowAlwaysOnTop(w->sdl, false);
    }
    if (flags & SWP_FRAMECHANGED) apply_style(w, w->style);
    if (flags & SWP_SHOWWINDOW) ShowWindow(h, SW_SHOW);
    if (flags & SWP_HIDEWINDOW) ShowWindow(h, SW_HIDE);
    if (!(flags & SWP_NOREDRAW)) w->invalid = true;
    return TRUE;
}
BOOL WINAPI MoveWindow(HWND h, int X, int Y, int W, int H, BOOL repaint)
{
    return SetWindowPos(h, nullptr, X, Y, W, H, SWP_NOZORDER | (repaint ? 0 : SWP_NOREDRAW));
}
BOOL WINAPI AdjustWindowRect(LPRECT, DWORD, BOOL) { return TRUE; }       // no emulated non-client area: outer == client
BOOL WINAPI AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD) { return TRUE; }
BOOL WINAPI GetWindowPlacement(HWND h, WINDOWPLACEMENT* p)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    p->length = sizeof *p;
    p->flags = 0;
    p->showCmd = !w->visible ? SW_HIDE : IsZoomed(h) ? SW_SHOWMAXIMIZED : IsIconic(h) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL;
    p->ptMinPosition.x = p->ptMinPosition.y = -1; p->ptMaxPosition.x = p->ptMaxPosition.y = -1;
    GetWindowRect(h, &p->rcNormalPosition);
    return TRUE;
}
BOOL WINAPI SetWindowPlacement(HWND h, const WINDOWPLACEMENT* p)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    SetWindowPos(h, nullptr, p->rcNormalPosition.left, p->rcNormalPosition.top, p->rcNormalPosition.right - p->rcNormalPosition.left,
                 p->rcNormalPosition.bottom - p->rcNormalPosition.top, SWP_NOZORDER);
    ShowWindow(h, (int)p->showCmd);
    return TRUE;
}
BOOL WINAPI ClientToScreen(HWND h, LPPOINT pt) { Win* w = win_of(h); if (!w) return FALSE; pt->x += w->x; pt->y += w->y; return TRUE; }
BOOL WINAPI ScreenToClient(HWND h, LPPOINT pt) { Win* w = win_of(h); if (!w) return FALSE; pt->x -= w->x; pt->y -= w->y; return TRUE; }

LONG WINAPI GetWindowLongA(HWND h, int i)
{
    Win* w = win_of(h);
    if (!w) return 0;
    switch (i) {
    case GWL_STYLE: return w->style;
    case GWL_EXSTYLE: return w->exstyle;
    case GWL_USERDATA: return (LONG)w->userdata;
    case -4: return (LONG)(intptr_t)w->proc;
    case -6: return (LONG)(intptr_t)w->hinst;
    case -12: return w->id;
    }
    if (i >= 0 && (size_t)i + 4 <= w->extra.size()) { LONG v; memcpy(&v, &w->extra[(size_t)i], 4); return v; }
    return 0;
}
LONG WINAPI SetWindowLongA(HWND h, int i, LONG v)
{
    Win* w = win_of(h);
    if (!w) return 0;
    LONG old = GetWindowLongA(h, i);
    switch (i) {
    case GWL_STYLE: apply_style(w, v); break;
    case GWL_EXSTYLE: w->exstyle = v; if (w->sdl) SDL_SetWindowAlwaysOnTop(w->sdl, (v & WS_EX_TOPMOST) != 0); break;
    case GWL_USERDATA: w->userdata = v; break;
    case -4: w->proc = (WNDPROC)(intptr_t)v; break;
    case -12: w->id = v; break;
    default: if (i >= 0 && (size_t)i + 4 <= w->extra.size()) memcpy(&w->extra[(size_t)i], &v, 4);
    }
    return old;
}
LONG WINAPI GetWindowLongW(HWND h, int i) { return GetWindowLongA(h, i); }
LONG WINAPI SetWindowLongW(HWND h, int i, LONG v) { return SetWindowLongA(h, i, v); }
DWORD WINAPI GetClassLongA(HWND h, int i)
{
    Win* w = win_of(h);
    if (!w || !w->cls) return 0;
    if (i == GCL_HCURSOR) return (DWORD)(uintptr_t)w->cls->cursor;
    return 0;
}
DWORD WINAPI SetClassLongA(HWND h, int i, LONG v)
{
    Win* w = win_of(h);
    if (!w || !w->cls) return 0;
    if (i == GCL_HCURSOR) { DWORD o = (DWORD)(uintptr_t)w->cls->cursor; w->cls->cursor = (HCURSOR)(intptr_t)v; return o; }
    return 0;
}
BOOL WINAPI SetWindowTextA(HWND h, LPCSTR s) { return (BOOL)SendMessageA(h, WM_SETTEXT, 0, (LPARAM)s); }
BOOL WINAPI SetWindowTextW(HWND h, LPCWSTR s)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    return (BOOL)def_proc(w, WM_SETTEXT, 0, (LPARAM)s, true);
}
int WINAPI GetWindowTextA(HWND h, LPSTR buf, int n)
{
    Win* w = win_of(h);
    if (!w || n <= 0) return 0;
    return (int)def_proc(w, 0x000D, (WPARAM)n, (LPARAM)buf, false);
}
BOOL WINAPI InvalidateRect(HWND h, const RECT*, BOOL) { Win* w = win_of(h); if (!w) return FALSE; w->invalid = true; return TRUE; }
BOOL WINAPI RedrawWindow(HWND h, const RECT*, HRGN, UINT flags)
{
    Win* w = win_of(h);
    if (!w) return FALSE;
    if (flags & RDW_INVALIDATE) w->invalid = true;
    if (flags & RDW_UPDATENOW) UpdateWindow(h);
    return TRUE;
}

// ---- messages ------------------------------------------------------------------------------------------------------------
BOOL WINAPI PeekMessageA(LPMSG m, HWND h, UINT lo, UINT hi, UINT rm)
{
    std::call_once(g_init_once, init_once);
    drain_sdl();
    return take_message(m, h, lo, hi, (rm & PM_REMOVE) != 0);
}
BOOL WINAPI PeekMessageW(LPMSG m, HWND h, UINT lo, UINT hi, UINT rm) { return PeekMessageA(m, h, lo, hi, rm); }
BOOL WINAPI GetMessageA(LPMSG m, HWND h, UINT lo, UINT hi)
{
    std::call_once(g_init_once, init_once);
    for (;;) {
        drain_sdl();
        if (take_message(m, h, lo, hi, true)) return m->message != WM_QUIT;
        // block until an event, a posted message or the next timer
        int wait_ms = -1;
        {
            std::lock_guard<std::recursive_mutex> g(g_mu);
            Uint64 now = SDL_GetTicks();
            for (const Timer& t : g_timers) {
                int d = t.next > now ? (int)(t.next - now) : 0;
                if (wait_ms < 0 || d < wait_ms) wait_ms = d;
            }
        }
        if (!g_init_ok || !on_main()) { Sleep(wait_ms < 0 ? 10 : (DWORD)wait_ms); continue; }
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, wait_ms < 0 ? 100 : wait_ms)) handle_event(e);
    }
}
BOOL WINAPI GetMessageW(LPMSG m, HWND h, UINT lo, UINT hi) { return GetMessageA(m, h, lo, hi); }
BOOL WINAPI TranslateMessage(const MSG* m)
{
    // Windows turns WM_KEYDOWN into WM_CHAR; here printable characters arrive from SDL text input, so only the control
    // characters (Enter, Tab, Backspace, Escape, Ctrl+letter) are produced from the key message.
    if (m->message != WM_KEYDOWN && m->message != WM_SYSKEYDOWN) return FALSE;
    int c = 0;
    bool ctrl = key_is_down(SDL_SCANCODE_LCTRL) || key_is_down(SDL_SCANCODE_RCTRL);
    switch (m->wParam) {
    case 0x0D: c = '\r'; break;
    case 0x09: c = '\t'; break;
    case 0x08: c = '\b'; break;
    case 0x1B: c = 0x1B; break;
    default:
        if (ctrl && m->wParam >= 'A' && m->wParam <= 'Z') c = (int)m->wParam - 'A' + 1;
    }
    if (!c) return FALSE;
    queue_msg(m->hwnd, m->message == WM_SYSKEYDOWN ? WM_SYSCHAR : WM_CHAR, (WPARAM)c, m->lParam & 0x3FFFFFFF);
    return TRUE;
}
LRESULT WINAPI DispatchMessageA(const MSG* m)
{
    if (m->message == WM_TIMER && m->lParam) {
        ((TIMERPROC_)m->lParam)(m->hwnd, WM_TIMER, (UINT_PTR)m->wParam, m->time);
        return 0;
    }
    Win* w = win_of(m->hwnd);
    if (!w) return 0;
    return send_to_window(w, m->message, m->wParam, m->lParam);
}
LRESULT WINAPI DispatchMessageW(const MSG* m) { return DispatchMessageA(m); }
LRESULT WINAPI SendMessageA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Win* w = win_of(h);
    if (!w) return 0;
    return send_to_window(w, msg, wp, lp);
}
BOOL WINAPI PostMessageA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    if (h && !win_of(h)) { set_last_error(1400); return FALSE; }
    post_to_queue(h, msg, wp, lp);
    return TRUE;
}
void WINAPI PostQuitMessage(int code)
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    g_quit = true;
    g_quit_code = code;
}
LRESULT WINAPI DefWindowProcA(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    Win* w = win_of(h);
    return w ? def_proc(w, m, wp, lp, false) : 0;
}
LRESULT WINAPI DefWindowProcW(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    Win* w = win_of(h);
    return w ? def_proc(w, m, wp, lp, true) : 0;
}
LRESULT WINAPI CallWindowProcA(WNDPROC p, HWND h, UINT m, WPARAM wp, LPARAM lp) { return p ? p(h, m, wp, lp) : DefWindowProcA(h, m, wp, lp); }
UINT WINAPI SetTimer(HWND h, UINT_PTR id, UINT ms, void* proc)
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    if (!h) id = g_next_timer++;
    for (Timer& t : g_timers)
        if (t.hwnd == h && t.id == id) { t.ms = ms; t.next = SDL_GetTicks() + ms; t.proc = (TIMERPROC_)proc; return (UINT)id; }
    g_timers.push_back(Timer{h, id, ms, SDL_GetTicks() + ms, (TIMERPROC_)proc});
    return (UINT)id;
}
BOOL WINAPI KillTimer(HWND h, UINT_PTR id)
{
    std::lock_guard<std::recursive_mutex> g(g_mu);
    size_t n = g_timers.size();
    g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(), [&](const Timer& t) { return t.hwnd == h && t.id == id; }), g_timers.end());
    g_queue().erase(std::remove_if(g_queue().begin(), g_queue().end(), [&](const MSG& m) { return m.message == WM_TIMER && m.hwnd == h && m.wParam == id; }), g_queue().end());
    return g_timers.size() != n;
}

// ---- accelerators, menus, icons, cursors, images ---------------------------------------------------------------------------
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE, LPCSTR) { static HINSTANCE__ table; return (HACCEL)&table; }   // an empty table: nothing translates
BOOL WINAPI DestroyAcceleratorTable(HACCEL) { return TRUE; }
int WINAPI TranslateAcceleratorA(HWND, HACCEL, LPMSG) { return 0; }
int WINAPI TranslateAcceleratorW(HWND, HACCEL, LPMSG) { return 0; }
HMENU WINAPI LoadMenuA(HINSTANCE, LPCSTR) { return (HMENU)new MenuObj; }     // the .exe's menu resource is not available: an empty menu
HMENU WINAPI GetMenu(HWND h) { Win* w = win_of(h); return w ? w->menu : nullptr; }
BOOL WINAPI SetMenu(HWND h, HMENU m) { Win* w = win_of(h); if (!w) return FALSE; w->menu = m; return TRUE; }
DWORD WINAPI CheckMenuItem(HMENU m, UINT id, UINT flags)
{
    MenuObj* o = (MenuObj*)m;
    if (!o) return (DWORD)-1;
    auto it = o->flags.find(id);
    if (it == o->flags.end()) return (DWORD)-1;
    DWORD prev = it->second & MF_CHECKED;
    it->second = (it->second & ~(UINT)MF_CHECKED) | (flags & MF_CHECKED);
    return prev;
}
BOOL WINAPI EnableMenuItem(HMENU m, UINT id, UINT flags)
{
    MenuObj* o = (MenuObj*)m;
    if (!o) return (BOOL)-1;
    auto it = o->flags.find(id);
    if (it == o->flags.end()) return (BOOL)-1;
    BOOL prev = it->second & MF_GRAYED;
    it->second = (it->second & ~(UINT)MF_GRAYED) | (flags & MF_GRAYED);
    return prev;
}
BOOL WINAPI DestroyMenu(HMENU m) { delete (MenuObj*)m; return TRUE; }
HICON WINAPI LoadIconA(HINSTANCE, LPCSTR) { return (HICON)&g_icon_obj; }
HCURSOR WINAPI LoadCursorA(HINSTANCE, LPCSTR name)
{
    uintptr_t id = (uintptr_t)name;
    if (id >= 0x10000) id = 32512;
    SDL_SystemCursor sys;
    int slot;
    switch (id) {
    case 32513: sys = SDL_SYSTEM_CURSOR_TEXT; slot = 1; break;
    case 32514: sys = SDL_SYSTEM_CURSOR_WAIT; slot = 2; break;
    case 32515: sys = SDL_SYSTEM_CURSOR_CROSSHAIR; slot = 3; break;
    case 32646: sys = SDL_SYSTEM_CURSOR_MOVE; slot = 4; break;
    case 32649: sys = SDL_SYSTEM_CURSOR_POINTER; slot = 5; break;
    case 32648: sys = SDL_SYSTEM_CURSOR_NOT_ALLOWED; slot = 6; break;
    default: sys = SDL_SYSTEM_CURSOR_DEFAULT; slot = 0; break;
    }
    if (!g_cursors[slot].cur && mbp_sdl_init()) g_cursors[slot].cur = SDL_CreateSystemCursor(sys);
    g_cursors[slot].sys = (int)id;
    return (HCURSOR)&g_cursors[slot];
}
HCURSOR WINAPI LoadCursorW(HINSTANCE i, LPCWSTR n) { return LoadCursorA(i, (LPCSTR)n); }
HCURSOR WINAPI SetCursor(HCURSOR c)
{
    HCURSOR prev = g_cur_handle;
    g_cur_handle = c;
    if (!mbp_sdl_init()) return prev;
    if (!c) g_cursor_shape_null = true;
    else {
        g_cursor_shape_null = false;
        CursorObj* o = (CursorObj*)c;
        if (o->cur) SDL_SetCursor(o->cur);
    }
    apply_cursor_visibility();
    return prev;
}
int WINAPI ShowCursor(BOOL show)
{
    g_cursor_count += show ? 1 : -1;
    if (mbp_sdl_init()) apply_cursor_visibility();
    return g_cursor_count;
}
BOOL WINAPI GetCursorPos(LPPOINT p)
{
    if (g_rel) { p->x = (LONG)g_rel_x; p->y = (LONG)g_rel_y; return TRUE; }
    if (g_virt_valid) { p->x = g_virt_x; p->y = g_virt_y; return TRUE; }
    if (!mbp_sdl_init()) return FALSE;
    float x = 0, y = 0;
    SDL_GetGlobalMouseState(&x, &y);
    p->x = (LONG)SDL_floorf(x); p->y = (LONG)SDL_floorf(y);
    return TRUE;
}
BOOL WINAPI SetCursorPos(int X, int Y)
{
    if (!mbp_sdl_init()) return FALSE;
    if (g_rel) { g_rel_x = X; g_rel_y = Y; rel_clamp(); return TRUE; }
    g_warp_x = X; g_warp_y = Y; g_warp_pending = 1;
    g_virt_x = X; g_virt_y = Y; g_virt_valid = true;
    return SDL_WarpMouseGlobal((float)X, (float)Y);
}
// SDL's mouse rectangle (window-relative) and the virtual pointer's clamp for the current g_clip
namespace {
void clip_apply(Win* w)
{
    if (!w || !w->sdl) return;
    if (g_clip_is_window) { g_clip.left = w->x; g_clip.top = w->y; g_clip.right = w->x + w->w; g_clip.bottom = w->y + w->h; }
    SDL_Rect rel = {g_clip.left - w->x, g_clip.top - w->y, g_clip.right - g_clip.left, g_clip.bottom - g_clip.top};
    SDL_SetWindowMouseRect(w->sdl, &rel);
    SDL_SetWindowMouseGrab(w->sdl, true);
    if (g_rel) rel_clamp();
}
}
BOOL WINAPI ClipCursor(const RECT* r)
{
    if (!mbp_sdl_init()) return FALSE;
    Win* w = win_of(GetFocus());
    if (!w) w = main_window();
    if (!w || !w->sdl) return FALSE;
    g_clip_on = r != nullptr;
    g_clip_is_window = false;
    if (r) g_clip = *r;
    if (!r) {
        SDL_SetWindowMouseRect(w->sdl, nullptr);
        SDL_SetWindowMouseGrab(w->sdl, false);
        return TRUE;
    }
    // d3dapp clips to GetWindowRect right after CreateDevice, before an asynchronous SDL fullscreen switch has resized the window
    // (2026-11-21: 1728x971 clip, window 1728x1117 afterwards: the pointer stopped above the bottom buttons): a clip equal to
    // the window's rectangle at the call is the window, whatever size it takes next
    g_clip_is_window = r->left == w->x && r->top == w->y && r->right == w->x + w->w && r->bottom == w->y + w->h;
    clip_apply(w);
    return TRUE;
}
SHORT WINAPI GetKeyState(int vk)
{
    if (!mbp_sdl_init()) return 0;
    if (vk >= 1 && vk <= 6) {
        unsigned b = mouse_button_mask();
        bool d = (vk == 1 && (b & 1)) || (vk == 2 && (b & 4)) || (vk == 4 && (b & 2));
        return d ? (SHORT)0x8000 : 0;
    }
    SDL_Scancode sc[2] = {keymap_scancode_for_vk(vk), SDL_SCANCODE_UNKNOWN};
    if (vk == 0x10) sc[1] = SDL_SCANCODE_RSHIFT;
    if (vk == 0x11) sc[1] = SDL_SCANCODE_RCTRL;
    if (vk == 0x12) sc[1] = SDL_SCANCODE_RALT;
    if (vk == 0xA0) sc[0] = SDL_SCANCODE_LSHIFT;
    if (vk == 0xA1) sc[0] = SDL_SCANCODE_RSHIFT;
    if (vk == 0xA2) sc[0] = SDL_SCANCODE_LCTRL;
    if (vk == 0xA3) sc[0] = SDL_SCANCODE_RCTRL;
    if (vk == 0xA4) sc[0] = SDL_SCANCODE_LALT;
    if (vk == 0xA5) sc[0] = SDL_SCANCODE_RALT;
    for (SDL_Scancode s : sc) if (s != SDL_SCANCODE_UNKNOWN && key_is_down(s)) return (SHORT)0x8000;
    SHORT toggled = 0;
    if (vk == 0x14 && (SDL_GetModState() & SDL_KMOD_CAPS)) toggled = 1;
    if (vk == 0x90 && (SDL_GetModState() & SDL_KMOD_NUM)) toggled = 1;
    return toggled;
}
SHORT WINAPI GetAsyncKeyState(int vk) { return GetKeyState(vk); }

// ---- metrics / monitors ------------------------------------------------------------------------------------------------------
static bool display_rects(SDL_DisplayID d, SDL_Rect* bounds, SDL_Rect* usable)
{
    if (!mbp_sdl_init()) return false;
    if (!d) d = SDL_GetPrimaryDisplay();
    SDL_Rect b = {0, 0, 1024, 768}, u;
    if (d) SDL_GetDisplayBounds(d, &b);
    u = b;
    if (d) SDL_GetDisplayUsableBounds(d, &u);
    if (bounds) *bounds = b;
    if (usable) *usable = u;
    return d != 0;
}
int WINAPI GetSystemMetrics(int i)
{
    SDL_Rect b, u;
    switch (i) {
    case SM_CXSCREEN: display_rects(0, &b, &u); return b.w;
    case SM_CYSCREEN: display_rects(0, &b, &u); return b.h;
    case SM_CXMAXIMIZED: display_rects(0, &b, &u); return u.w;
    case SM_CYMAXIMIZED: display_rects(0, &b, &u); return u.h - 28;      // the title bar the OS draws above the client area
    case SM_CYCAPTION: case SM_CXFRAME: case SM_CYFRAME: case SM_CYMENU: return 0;     // no emulated non-client area
    case 5: case 6: return 0;                 // SM_CXBORDER / SM_CYBORDER
    case 2: case 3: return 17;                // scroll bar arrows
    case 11: case 12: case 13: case 14: return 32;   // icon / cursor sizes
    case 49: case 50: return 16;
    case 19: return 1;                        // SM_MOUSEPRESENT
    case 23: return 0;                        // SM_SWAPBUTTON
    case 28: return 136;  case 29: return 39; // SM_CXMIN / SM_CYMIN
    case 43: return 3;                        // SM_CMOUSEBUTTONS
    case 80: { if (!mbp_sdl_init()) return 1; int n = 0; SDL_DisplayID* d = SDL_GetDisplays(&n); SDL_free(d); return n < 1 ? 1 : n; }
    default: return 0;
    }
}
BOOL WINAPI SystemParametersInfoA(UINT action, UINT uiParam, PVOID pv, UINT)
{
    SDL_Rect b, u;
    switch (action) {
    case 0x0030: {                            // SPI_GETWORKAREA
        if (!pv) return FALSE;
        display_rects(0, &b, &u);
        RECT* r = (RECT*)pv;
        r->left = u.x; r->top = u.y; r->right = u.x + u.w; r->bottom = u.y + u.h;
        return TRUE;
    }
    case 0x0010: if (pv) *(BOOL*)pv = FALSE; return TRUE;      // SPI_GETSCREENSAVEACTIVE: SDL leaves the screensaver off
    case 0x0011: return TRUE;                                  // SPI_SETSCREENSAVEACTIVE
    case 0x0070: if (pv) *(BOOL*)pv = FALSE; return TRUE;      // SPI_GETSCREENSAVERRUNNING
    default: set_last_error(87); return FALSE;
    }
    (void)uiParam;
}
HMONITOR WINAPI MonitorFromWindow(HWND h, DWORD)
{
    Win* w = win_of(h);
    if (!mbp_sdl_init()) return nullptr;
    SDL_DisplayID d = (w && w->sdl) ? SDL_GetDisplayForWindow(w->sdl) : SDL_GetPrimaryDisplay();
    if (!d) d = SDL_GetPrimaryDisplay();
    return (HMONITOR)(uintptr_t)d;
}
BOOL WINAPI GetMonitorInfoA(HMONITOR m, LPMONITORINFO mi)
{
    if (!mi || mi->cbSize < sizeof(MONITORINFO)) return FALSE;
    SDL_Rect b, u;
    SDL_DisplayID d = (SDL_DisplayID)(uintptr_t)m;
    if (!display_rects(d, &b, &u)) return FALSE;
    mi->rcMonitor = RECT{b.x, b.y, b.x + b.w, b.y + b.h};
    mi->rcWork = RECT{u.x, u.y, u.x + u.w, u.y + u.h};
    mi->dwFlags = (d == SDL_GetPrimaryDisplay()) ? 1 : 0;
    return TRUE;
}

// ---- rectangles ----------------------------------------------------------------------------------------------------------------
BOOL WINAPI SetRect(LPRECT r, int l, int t, int rt, int b)
{
    if (!r) return FALSE;
    r->left = l; r->top = t; r->right = rt; r->bottom = b;
    return TRUE;
}
BOOL WINAPI SetRectEmpty(LPRECT r) { if (!r) return FALSE; memset(r, 0, sizeof *r); return TRUE; }
BOOL WINAPI PtInRect(const RECT* r, POINT p) { return p.x >= r->left && p.x < r->right && p.y >= r->top && p.y < r->bottom; }
BOOL WINAPI IntersectRect(LPRECT d, const RECT* a, const RECT* b)
{
    LONG l = std::max(a->left, b->left), t = std::max(a->top, b->top), r = std::min(a->right, b->right), bt = std::min(a->bottom, b->bottom);
    if (l >= r || t >= bt) { memset(d, 0, sizeof *d); return FALSE; }
    d->left = l; d->top = t; d->right = r; d->bottom = bt;
    return TRUE;
}

// ---- message boxes ----------------------------------------------------------------------------------------------------------------
static int message_box(HWND parent, const std::string& text, const std::string& caption, UINT type)
{
    unsigned kind = type & 0xF;
    int first = kind == 4 ? IDYES : IDOK;                                  // the default answer when there is no display
    const char* names[3] = {"OK", nullptr, nullptr};
    int ids[3] = {IDOK, 0, 0};
    int n = 1;
    switch (kind) {
    case 1: names[0] = "OK"; names[1] = "Cancel"; ids[1] = IDCANCEL; n = 2; break;
    case 3: names[0] = "Yes"; names[1] = "No"; names[2] = "Cancel"; ids[0] = IDYES; ids[1] = IDNO; ids[2] = IDCANCEL; n = 3; break;
    case 4: names[0] = "Yes"; names[1] = "No"; ids[0] = IDYES; ids[1] = IDNO; n = 2; break;
    case 5: names[0] = "Retry"; names[1] = "Cancel"; ids[0] = 4; ids[1] = IDCANCEL; n = 2; break;
    default: break;
    }
    fprintf(stderr, "mb: MessageBox [%s] %s\n", caption.c_str(), text.c_str());
    const char* nd = getenv("MBP_NO_DIALOGS");
    if ((nd && *nd == '1') || !mbp_sdl_init() || !on_main()) return first;
    SDL_MessageBoxButtonData btn[3];
    for (int i = 0; i < n; ++i) { btn[i].flags = i == 0 ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : 0; btn[i].buttonID = ids[i]; btn[i].text = names[i]; }
    if (n >= 2) btn[n - 1].flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    SDL_MessageBoxFlags fl = SDL_MESSAGEBOX_INFORMATION;
    switch (type & 0xF0) { case 0x10: fl = SDL_MESSAGEBOX_ERROR; break; case 0x30: fl = SDL_MESSAGEBOX_WARNING; break; default: break; }
    SDL_MessageBoxData d;
    memset(&d, 0, sizeof d);
    d.flags = fl;
    Win* pw = win_of(parent);
    d.window = pw ? pw->sdl : nullptr;
    d.title = caption.c_str();
    d.message = text.c_str();
    d.numbuttons = n;
    d.buttons = btn;
    int pressed = 0;
    if (!SDL_ShowMessageBox(&d, &pressed)) return first;
    return pressed ? pressed : (n >= 2 ? ids[n - 1] : IDOK);
}
int WINAPI MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT type)
{
    if (getenv("MB_CRASH_BT")) {                // MB_CRASH_BT: a message box (mb_error's "RGL ERROR") also logs its text and caller
        fprintf(stderr, "MessageBoxA [%s]: %s\n", cap ? cap : "", text ? text : "");
#if !defined(_WIN32)
        void* frames[48];
        backtrace_symbols_fd(frames, backtrace(frames, 48), 2);
#endif
    }
    return message_box(h, text ? ansi_to_utf8(text) : "", cap ? ansi_to_utf8(cap) : "Error", type);
}
int WINAPI MessageBoxW(HWND h, LPCWSTR text, LPCWSTR cap, UINT type)
{
    return message_box(h, text ? wide_to_utf8(text) : "", cap ? wide_to_utf8(cap) : "Error", type);
}
void WINAPI InitCommonControls(void) {}      // the common-control classes are the virtual dialog controls (dialog.cpp)
}   // extern "C"
