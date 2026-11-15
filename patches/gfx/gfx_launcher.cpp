// patches/gfx/gfx_launcher.cpp: the gfx settings on the launcher's Configure dialog (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose: the dialog's Video tab gets four more rows, below Shadow Quality:
//   Anisotropic Filtering   combo: Off, 2x, 4x, 8x, 16x                          (gfx_anisotropy)
//   Render Scale            combo: 50% 67% 75% 85% 100% 125% 150% 200%          (gfx_render_scale)
//   AMD FSR 1 ...           checkbox                                             (gfx_fsr)
//   FSR Sharpness           combo: 0.0 (sharpest), 0.2, 0.5, 1.0, 2.0 (softest)  (gfx_fsr_sharpness)
// A value from rgl_config.txt that is not a preset gets its own entry, so opening and closing the dialog keeps it.
//
// How. The dialog is template 0xc1 (resources/RT_DIALOG/193_1024.bin), run by GameApp::msg_proc (0x4d4140) with
// DialogBoxParamA and procedure wndproc_thunk (0x4d4130 -> GameApp::config_proc 0x4d0240). A static constructor swaps
// our DialogBoxParamA import slot (devfast/iat_hook.h); for template 0xc1 the replacement passes its own procedure,
// which calls the game's and then:
//   WM_INITDIALOG    grows the dialog (and its tab control) by the four rows, moves OK / Auto Adjust / Cancel down, and
//                    creates the controls with CreateWindowEx in the template's own units (MapDialogRect) and font;
//   WM_NOTIFY        (the tab control) shows them on the Video tab only, as OptionsDlg::set_active_tab 0x4b9790 does
//                    for the template's own controls;
//   IDOK             before the game's handler, which writes rgl_config.txt through rgl_config::save (0x415fb0,
//                    gfx_config.cpp): the controls go to g_gfx_saved and gfx_apply_saved (the device does not exist yet
//                    when the launcher runs, so the game starts with them).
// The template itself is unchanged, so the faithful build's resources stay as they are; src/ is not touched.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "../devfast/iat_hook.h"
#include "./gfx.h"

typedef INT_PTR (WINAPI* dialog_box_fn)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
static dialog_box_fn s_dialog_box;
static DLGPROC s_game_proc;

enum {
    CONFIG_TEMPLATE = 0xc1,
    ID_TABS = 0x43d,                            // the SysTabControl32
    ID_ANISO_LABEL = 0x7a00, ID_ANISO, ID_SCALE_LABEL, ID_SCALE, ID_FSR, ID_SHARP_LABEL, ID_SHARP,
    ID_FIRST = ID_ANISO_LABEL, ID_LAST = ID_SHARP,
    // the template's rows (dialog units): labels at x 20 (104 wide), combos at x 126 (95 wide) 4 units above their
    // label, 17 units apart; Shadow Quality, the last, is at y 195
    ROW0 = 212, ROW_STEP = 17, GROW = 4 * ROW_STEP,
};

static HWND make(HWND dlg, const char* cls, const char* text, DWORD style, int x, int y, int w, int h, int id)
{
    RECT r = { x, y, x + w, y + h };
    MapDialogRect(dlg, &r);
    HWND c = CreateWindowExA(0, cls, text, WS_CHILD | WS_VISIBLE | style, r.left, r.top, r.right - r.left,
                             r.bottom - r.top, dlg, (HMENU)(INT_PTR)id, GetModuleHandleA(0), 0);
    SendMessageA(c, WM_SETFONT, SendMessageA(dlg, WM_GETFONT, 0, 0), FALSE);
    return c;
}

static void add_item(HWND combo, const char* text, int value)
{
    int i = (int)SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)text);
    SendMessageA(combo, CB_SETITEMDATA, i, value);
}

// a combo of presets, with `cur` selected: added in order (inserted, if it is not one of them)
static void fill_combo(HWND combo, const int* presets, int n, int cur, const char* fmt, int div)
{
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (!found && cur < presets[i]) {
            char t[32];
            sprintf(t, fmt, div ? (double)cur / div : (double)cur);
            add_item(combo, t, cur);
            found = true;
        }
        if (presets[i] == cur)
            found = true;
        char t[32];
        if (presets == g_gfx_aniso_levels && presets[i] == 0)
            sprintf(t, "Off");
        else if (presets == g_gfx_sharp_presets && i == 0)
            sprintf(t, "%.2f (sharpest)", presets[i] / 100.0);
        else if (presets == g_gfx_sharp_presets && i == n - 1)
            sprintf(t, "%.2f (softest)", presets[i] / 100.0);
        else
            sprintf(t, fmt, div ? (double)presets[i] / div : (double)presets[i]);
        add_item(combo, t, presets[i]);
    }
    if (!found) {
        char t[32];
        sprintf(t, fmt, div ? (double)cur / div : (double)cur);
        add_item(combo, t, cur);
    }
    int count = (int)SendMessageA(combo, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++)
        if ((int)SendMessageA(combo, CB_GETITEMDATA, i, 0) == cur)
            SendMessageA(combo, CB_SETCURSEL, i, 0);
}

static int combo_value(HWND dlg, int id, int fallback)
{
    HWND c = GetDlgItem(dlg, id);
    int i = (int)SendMessageA(c, CB_GETCURSEL, 0, 0);
    return i < 0 ? fallback : (int)SendMessageA(c, CB_GETITEMDATA, i, 0);
}

// a child of the dialog, moved down by dy pixels (and, with grow, made dy taller instead)
static void shift(HWND dlg, int id, int dy, bool grow)
{
    HWND c = GetDlgItem(dlg, id);
    if (!c)
        return;
    RECT r;
    GetWindowRect(c, &r);
    MapWindowPoints(0, dlg, (POINT*)&r, 2);
    if (grow)
        SetWindowPos(c, 0, 0, 0, r.right - r.left, r.bottom - r.top + dy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    else
        SetWindowPos(c, 0, r.left, r.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void add_controls(HWND dlg)
{
    RECT g = { 0, 0, 0, GROW };
    MapDialogRect(dlg, &g);
    int dy = g.bottom;
    RECT w;
    GetWindowRect(dlg, &w);
    SetWindowPos(dlg, 0, 0, 0, w.right - w.left, w.bottom - w.top + dy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    shift(dlg, IDOK, dy, false);
    shift(dlg, IDCANCEL, dy, false);
    shift(dlg, 0x45e, dy, false);              // Auto Adjust
    shift(dlg, ID_TABS, dy, true);

    const gfx_settings& s = g_gfx_saved;
    const DWORD combo = CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP;
    int y = ROW0;
    make(dlg, "STATIC", "Anisotropic Filtering:", SS_LEFT, 20, y, 104, 8, ID_ANISO_LABEL);
    fill_combo(make(dlg, "COMBOBOX", "", combo, 126, y - 4, 95, 113, ID_ANISO), g_gfx_aniso_levels, 5, s.anisotropy,
               "%.0fx", 0);
    y += ROW_STEP;
    make(dlg, "STATIC", "Render Scale (3D scene):", SS_LEFT, 20, y, 104, 8, ID_SCALE_LABEL);
    fill_combo(make(dlg, "COMBOBOX", "", combo, 126, y - 4, 95, 113, ID_SCALE), g_gfx_scale_presets, 8,
               (int)floor(s.render_scale * 100.0f + 0.5f), "%.0f%%", 0);
    y += ROW_STEP;
    HWND fsr = make(dlg, "BUTTON", "AMD FSR 1 (upscale below 100%, sharpen at and above)", BS_AUTOCHECKBOX | WS_TABSTOP,
                    20, y, 228, 8, ID_FSR);
    SendMessageA(fsr, BM_SETCHECK, s.fsr ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_STEP;
    make(dlg, "STATIC", "FSR Sharpness:", SS_LEFT, 20, y, 104, 8, ID_SHARP_LABEL);
    fill_combo(make(dlg, "COMBOBOX", "", combo, 126, y - 4, 95, 113, ID_SHARP), g_gfx_sharp_presets, 5,
               (int)floor(s.fsr_sharpness * 100.0f + 0.5f), "%.2f", 100);
    EnableWindow(GetDlgItem(dlg, ID_SHARP), s.fsr);
    EnableWindow(GetDlgItem(dlg, ID_SHARP_LABEL), s.fsr);
}

static void show_for_tab(HWND dlg)
{
    int tab = (int)SendMessageA(GetDlgItem(dlg, ID_TABS), 0x130b, 0, 0);
    for (int id = ID_FIRST; id <= ID_LAST; id++)
        ShowWindow(GetDlgItem(dlg, id), tab == 0 ? SW_SHOW : SW_HIDE);
}

static void read_controls(HWND dlg)
{
    gfx_settings& s = g_gfx_saved;
    s.anisotropy = combo_value(dlg, ID_ANISO, s.anisotropy);
    s.render_scale = combo_value(dlg, ID_SCALE, (int)floor(s.render_scale * 100.0f + 0.5f)) / 100.0f;
    s.fsr = SendMessageA(GetDlgItem(dlg, ID_FSR), BM_GETCHECK, 0, 0) == BST_CHECKED;
    s.fsr_sharpness = combo_value(dlg, ID_SHARP, (int)floor(s.fsr_sharpness * 100.0f + 0.5f)) / 100.0f;
    gfx_apply_saved();
}

static INT_PTR CALLBACK config_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_COMMAND && LOWORD(wp) == IDOK)
        read_controls(dlg);                     // first: the game's handler saves and ends the dialog
    INT_PTR r = s_game_proc(dlg, msg, wp, lp);
    if (msg == WM_INITDIALOG) {
        add_controls(dlg);
        show_for_tab(dlg);
    } else if (msg == WM_NOTIFY && LOWORD(wp) == ID_TABS) {
        show_for_tab(dlg);
    } else if (msg == WM_COMMAND && LOWORD(wp) == ID_FSR && HIWORD(wp) == BN_CLICKED) {
        BOOL on = SendMessageA(GetDlgItem(dlg, ID_FSR), BM_GETCHECK, 0, 0) == BST_CHECKED;
        EnableWindow(GetDlgItem(dlg, ID_SHARP), on);
        EnableWindow(GetDlgItem(dlg, ID_SHARP_LABEL), on);
    }
    return r;
}

static INT_PTR WINAPI hook_dialog_box(HINSTANCE inst, LPCSTR tmpl, HWND parent, DLGPROC proc, LPARAM lp)
{
    if (((UINT_PTR)tmpl >> 16) == 0 && (UINT_PTR)tmpl == CONFIG_TEMPLATE && proc) {
        s_game_proc = proc;
        return s_dialog_box(inst, tmpl, parent, config_proc, lp);
    }
    return s_dialog_box(inst, tmpl, parent, proc, lp);
}

static struct gfx_launcher_init {
    gfx_launcher_init()
    {
        gfx_load_settings();
        s_dialog_box = (dialog_box_fn)iat_hook("user32.dll", "DialogBoxParamA", (void*)hook_dialog_box);
        gfx_log("DialogBoxParamA hook: %s\n", s_dialog_box ? "set" : "IMPORT NOT FOUND");
    }
} s_gfx_launcher_init;
