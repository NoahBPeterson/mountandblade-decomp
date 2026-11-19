// port/platform/mbp_win.h: the window object behind HWND, shared by window.cpp, dialog.cpp, gdi.cpp, clipboard.cpp.
#pragma once
#include "mbp_internal.h"

namespace mbp {

struct Class {
    std::string name;
    WNDPROC proc = nullptr;
    UINT style = 0;
    int cls_extra = 0, wnd_extra = 0;
    HICON icon = nullptr;
    HCURSOR cursor = nullptr;
    HBRUSH bg = nullptr;
};

// The state a dialog control needs (a combo box, list box, check box, edit, trackbar, tab strip are all this one record).
struct Control {
    struct Item { std::string text; LPARAM data = 0; bool selected = false; };
    std::string text;
    int checked = 0;                 // BST_*
    std::vector<Item> items;         // combo / list items
    int cursel = -1;
    int tmin = 0, tmax = 100, tpos = 0;     // trackbar
    int limit = 0;                   // edit limit
};

const uint32_t WIN_MAGIC = 0x4d42574e;

struct Win {
    uint32_t magic = WIN_MAGIC;
    Class* cls = nullptr;
    WNDPROC proc = nullptr;
    DLGPROC dproc = nullptr;
    bool is_dialog = false, is_control = false, modal = false, ended = false, quit_posted = false;
    INT_PTR dialog_result = 0;
    int dialog_template = 0;
    SDL_Window* sdl = nullptr;
    SDL_WindowID sdl_id = 0;
    LONG style = 0, exstyle = 0;
    LONG_PTR userdata = 0;
    std::vector<unsigned char> extra;
    std::string title;
    Win* parent = nullptr;
    std::vector<Win*> children;
    HMENU menu = nullptr;
    int id = 0;                       // control id inside its dialog
    int x = 0, y = 0, w = 0, h = 0;   // outer == client here (no emulated non-client area)
    bool visible = false, enabled = true, invalid = false, active = false, created = false;
    Control ctl;
    HINSTANCE hinst = nullptr;
    LRESULT msg_result = 0;           // DWL_MSGRESULT
};

inline Win* win_of(HWND h)
{
    Win* w = (Win*)h;
    return (w && w->magic == WIN_MAGIC) ? w : nullptr;
}
Win* find_dialog_control(Win* dlg, int id, bool create);
LRESULT send_to_window(Win* w, UINT msg, WPARAM wp, LPARAM lp);
void post_to_queue(HWND h, UINT msg, WPARAM wp, LPARAM lp);
void destroy_window_tree(Win* w);
Win* main_window();
bool any_window_alive();
void register_window(Win* w);

}   // namespace mbp
