// port/platform/dialog.cpp: dialogs and their controls as virtual objects.
// The game's dialogs (the launcher, the options pages, the script / map-editor dialogs) are Win32 dialog templates in the
// original .exe's resources; those resources are not part of this build. A dialog here is a window object whose controls
// exist on demand (GetDlgItem(id) creates the control the first time an id is used) and keep the state a dialog procedure
// reads back: text, check state, combo / list items with item data and selection, trackbar range and position, tab items.
// DialogBoxParamA runs the dialog procedure's WM_INITDIALOG, drains its posted messages, and, if nothing ends the dialog,
// answers it the way the user's default choice would: the launcher (template 0xaa) presses "Play" (0x410, which returns
// 0x410 and starts the game); every other dialog is cancelled (IDCANCEL).
#include "mbp_win.h"
#include <algorithm>
#include <ctype.h>

using namespace mbp;

namespace {
std::string tc_text(LPARAM lp) { return lp ? ansi_to_utf8((const char*)lp) : std::string(); }
int copy_out(const std::string& utf8, char* dst)
{
    std::string a = utf8_to_ansi(utf8.c_str());
    if (dst) memcpy(dst, a.c_str(), a.size() + 1);
    return (int)a.size();
}
std::string lowered(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
}

namespace mbp {
Win* find_dialog_control(Win* dlg, int id, bool create)
{
    for (Win* c : dlg->children) if (c->id == id) return c;
    if (!create) return nullptr;
    Win* c = new Win;
    c->is_control = true; c->id = id; c->parent = dlg; c->visible = true; c->created = true;
    dlg->children.push_back(c);
    register_window(c);
    return c;
}
bool is_virtual_window(HWND h) { Win* w = win_of(h); return w && !w->sdl; }

LRESULT control_message(HWND h, UINT msg, WPARAM wp, LPARAM lp, bool* handled)
{
    Win* w = win_of(h);
    Control& c = w->ctl;
    *handled = true;
    auto idx_ok = [&](LONG_PTR i) { return i >= 0 && (size_t)i < c.items.size(); };
    switch (msg) {
    case WM_SETTEXT: c.text = tc_text(lp); w->title = c.text; return TRUE;
    case 0x000D: if (wp == 0) return 0; { std::string a = utf8_to_ansi(c.text.c_str()); size_t n = std::min<size_t>(a.size(), wp - 1); memcpy((char*)lp, a.data(), n); ((char*)lp)[n] = 0; return (LRESULT)n; }
    case 0x000E: return (LRESULT)utf8_to_ansi(c.text.c_str()).size();
    case 0x0030: return 0;                                              // WM_SETFONT: presentation only
    case BM_GETCHECK: return c.checked;
    case BM_SETCHECK: c.checked = (int)wp; return 0;
    case BM_GETSTATE: return c.checked ? BST_CHECKED : 0;
    case BM_SETSTATE: return 0;
    case EM_SETLIMITTEXT: c.limit = (int)wp; return 0;
    // combo box + list box share the item model
    case CB_ADDSTRING: case LB_ADDSTRING: { Control::Item it; it.text = tc_text(lp); c.items.push_back(it); return (LRESULT)c.items.size() - 1; }
    case CB_INSERTSTRING: case 0x181: {
        Control::Item it; it.text = tc_text(lp);
        size_t at = (wp == (WPARAM)-1 || wp > c.items.size()) ? c.items.size() : wp;
        c.items.insert(c.items.begin() + (long)at, it);
        if (c.cursel >= (int)at) ++c.cursel;
        return (LRESULT)at;
    }
    case CB_DELETESTRING: case 0x182:
        if (!idx_ok((LONG_PTR)wp)) return CB_ERR;
        c.items.erase(c.items.begin() + (long)wp);
        if (c.cursel == (int)wp) c.cursel = -1; else if (c.cursel > (int)wp) --c.cursel;
        return (LRESULT)c.items.size();
    case CB_GETCOUNT: case LB_GETCOUNT: return (LRESULT)c.items.size();
    case CB_GETCURSEL: case LB_GETCURSEL: return c.cursel < 0 ? CB_ERR : c.cursel;
    case CB_SETCURSEL: case LB_SETCURSEL:
        for (auto& it : c.items) it.selected = false;
        if (!idx_ok((LONG_PTR)(int)wp)) { c.cursel = -1; return CB_ERR; }
        c.cursel = (int)wp; c.items[(size_t)c.cursel].selected = true;
        return (LRESULT)c.cursel;
    case CB_GETLBTEXT: case 0x189:
        if (!idx_ok((LONG_PTR)wp)) return CB_ERR;
        return copy_out(c.items[wp].text, (char*)lp);
    case CB_GETLBTEXTLEN: case 0x18A:
        if (!idx_ok((LONG_PTR)wp)) return CB_ERR;
        return (LRESULT)utf8_to_ansi(c.items[wp].text.c_str()).size();
    case CB_RESETCONTENT: case LB_RESETCONTENT: c.items.clear(); c.cursel = -1; return 0;
    case CB_GETITEMDATA: case LB_GETITEMDATA: return idx_ok((LONG_PTR)(int)wp) ? c.items[(size_t)(int)wp].data : CB_ERR;
    case CB_SETITEMDATA: case LB_SETITEMDATA:
        if (!idx_ok((LONG_PTR)wp)) return CB_ERR;
        c.items[wp].data = lp;
        return 0;
    case 0x158: case 0x1A2: case 0x14C: case 0x18F: {                     // CB_FINDSTRINGEXACT / LB_FINDSTRINGEXACT / CB_FINDSTRING / LB_FINDSTRING
        std::string key = lowered(tc_text(lp));
        bool exact = msg == 0x158 || msg == 0x1A2;
        size_t n = c.items.size();
        if (n == 0) return CB_ERR;
        size_t start = (wp == (WPARAM)-1) ? 0 : (wp + 1) % n;
        for (size_t k = 0; k < n; ++k) {
            size_t i = (start + k) % n;
            std::string t = lowered(c.items[i].text);
            if (exact ? t == key : t.compare(0, key.size(), key) == 0) return (LRESULT)i;
        }
        return CB_ERR;
    }
    case LB_SETSEL: {
        if (lp == -1) { for (auto& it : c.items) it.selected = wp != 0; return 0; }
        if (!idx_ok(lp)) return LB_ERR;
        c.items[(size_t)lp].selected = wp != 0;
        return 0;
    }
    case LB_GETSEL: return idx_ok((LONG_PTR)wp) ? (c.items[wp].selected ? 1 : 0) : LB_ERR;
    case LB_GETSELITEMS: {
        int* out = (int*)lp;
        int n = 0;
        for (size_t i = 0; i < c.items.size() && n < (int)wp; ++i) if (c.items[i].selected) out[n++] = (int)i;
        return n;
    }
    // trackbar
    case 0x400: return c.tpos;                                           // TBM_GETPOS
    case 0x401: return c.tmin;
    case 0x402: return c.tmax;
    case 0x405: c.tpos = std::max(c.tmin, std::min(c.tmax, (int)lp)); return 0;
    case 0x406: c.tmin = (int)(short)LOWORD(lp); c.tmax = (int)(short)HIWORD(lp); c.tpos = std::max(c.tmin, std::min(c.tmax, c.tpos)); return 0;
    case 0x407: c.tmin = (int)lp; return 0;
    case 0x408: c.tmax = (int)lp; return 0;
    case 0x414: return 0;                                                // TBM_SETTICFREQ
    // tab strip
    case 0x130B: return c.cursel < 0 ? 0 : c.cursel;                     // TCM_GETCURSEL
    case 0x130C: { int prev = c.cursel < 0 ? 0 : c.cursel; if (idx_ok((LONG_PTR)(int)wp)) c.cursel = (int)wp; return prev; }   // TCM_SETCURSEL
    case 0x1307: {                                                       // TCM_INSERTITEMA
        const TCITEMA* t = (const TCITEMA*)lp;
        Control::Item it;
        if (t && (t->mask & TCIF_TEXT)) it.text = tc_text((LPARAM)t->pszText);
        if (t && (t->mask & TCIF_PARAM)) it.data = t->lParam;
        size_t at = wp > c.items.size() ? c.items.size() : wp;
        c.items.insert(c.items.begin() + (long)at, it);
        if (c.cursel < 0) c.cursel = 0;
        return (LRESULT)at;
    }
    default: *handled = false; return 0;
    }
}
}   // namespace mbp

extern "C" {
HWND WINAPI GetDlgItem(HWND d, int id)
{
    Win* w = win_of(d);
    if (!w) return nullptr;
    return (HWND)find_dialog_control(w, id, true);
}
BOOL WINAPI SetDlgItemTextA(HWND d, int id, LPCSTR s) { HWND c = GetDlgItem(d, id); return c ? (BOOL)SendMessageA(c, WM_SETTEXT, 0, (LPARAM)s) : FALSE; }
UINT WINAPI GetDlgItemTextA(HWND d, int id, LPSTR buf, int max)
{
    HWND c = GetDlgItem(d, id);
    if (!c || max <= 0) return 0;
    return (UINT)SendMessageA(c, 0x000D, (WPARAM)max, (LPARAM)buf);
}
BOOL WINAPI SetDlgItemInt(HWND d, int id, UINT v, BOOL sgn)
{
    char b[32];
    snprintf(b, sizeof b, sgn ? "%d" : "%u", sgn ? (int)v : v);
    return SetDlgItemTextA(d, id, b);
}
UINT WINAPI GetDlgItemInt(HWND d, int id, BOOL* ok, BOOL sgn)
{
    char b[64] = {0};
    GetDlgItemTextA(d, id, b, sizeof b);
    char* end = nullptr;
    long long v = sgn ? strtoll(b, &end, 10) : (long long)strtoull(b, &end, 10);
    bool good = end && end != b && *end == 0;
    if (ok) *ok = good;
    return good ? (UINT)v : 0;
}
BOOL WINAPI CheckRadioButton(HWND d, int first, int last, int check)
{
    if (!win_of(d)) return FALSE;
    for (int i = first; i <= last; ++i) SendMessageA(GetDlgItem(d, i), BM_SETCHECK, i == check ? BST_CHECKED : BST_UNCHECKED, 0);
    return TRUE;
}
UINT WINAPI IsDlgButtonChecked(HWND d, int id) { HWND c = GetDlgItem(d, id); return c ? (UINT)SendMessageA(c, BM_GETCHECK, 0, 0) : 0; }
BOOL WINAPI CheckDlgButton(HWND d, int id, UINT st) { HWND c = GetDlgItem(d, id); if (!c) return FALSE; SendMessageA(c, BM_SETCHECK, st, 0); return TRUE; }
LRESULT WINAPI SendDlgItemMessageA(HWND d, int id, UINT m, WPARAM wp, LPARAM lp) { HWND c = GetDlgItem(d, id); return c ? SendMessageA(c, m, wp, lp) : 0; }

BOOL WINAPI EndDialog(HWND h, INT_PTR result)
{
    Win* w = win_of(h);
    if (!w || !w->is_dialog) return FALSE;
    w->ended = true;
    w->dialog_result = result;
    return TRUE;
}

static Win* new_dialog(LPCSTR templ, HWND parent, DLGPROC proc, bool modal)
{
    mbp_sdl_init();
    Win* d = new Win;
    d->is_dialog = true; d->modal = modal; d->dproc = proc; d->parent = win_of(parent); d->visible = true; d->created = true;
    d->dialog_template = (uintptr_t)templ < 0x10000 ? (int)(uintptr_t)templ : 0;
    register_window(d);
    return d;
}

INT_PTR WINAPI DialogBoxParamA(HINSTANCE, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    if (!proc) { set_last_error(87); return -1; }
    Win* d = new_dialog(templ, parent, proc, true);
    proc((HWND)d, WM_INITDIALOG, 0, param);
    int attempt = 0;
    int default_id = d->dialog_template == 0xaa ? 0x410 : IDCANCEL;
    for (;;) {
        MSG m;
        while (!d->ended && PeekMessageA(&m, nullptr, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); d->ended = true; d->dialog_result = -1; break; }
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
        if (d->ended) break;
        if (attempt == 0) {
            attempt = 1;
            PostMessageA((HWND)d, WM_COMMAND, MAKEWPARAM(default_id, BN_CLICKED), (LPARAM)GetDlgItem((HWND)d, default_id));
        } else if (attempt == 1 && default_id != IDCANCEL) {
            attempt = 2;
            PostMessageA((HWND)d, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)GetDlgItem((HWND)d, IDCANCEL));
        } else {
            d->ended = true; d->dialog_result = IDCANCEL;
        }
    }
    INT_PTR r = d->dialog_result;
    d->modal = false;
    DestroyWindow((HWND)d);
    return r;
}
HWND WINAPI CreateDialogParamA(HINSTANCE, LPCSTR templ, HWND parent, DLGPROC proc, LPARAM param)
{
    if (!proc) { set_last_error(87); return nullptr; }
    Win* d = new_dialog(templ, parent, proc, false);
    proc((HWND)d, WM_INITDIALOG, 0, param);
    return (HWND)d;
}
}
