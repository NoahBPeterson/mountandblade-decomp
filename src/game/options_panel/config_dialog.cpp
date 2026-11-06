// config_dialog.cpp: the launcher configuration dialog (0x4cf100..0x4d0620): the display-mode list, the tab fill and the
// dialog procedure (rgl_config.txt). Function names FUN_* are the slices' provisional names.
// clang-format off
#include "../../include/image_addrs.h"
#include "../config/options_dlg.h"     // OptionsDlg (first: it selects WIN32_LEAN_AND_MEAN and pulls winsock2.h)
#include <windows.h>
#include <shlobj.h>                  // SHGetFolderPathA (user_path)
#include <d3d9.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <new>
#include "../tstring.h"
#include "../config/config.h"          // Config, GameApp (the dialog's object is g_app_obj, 0x8ee3c48)
#include "../../rgl/mesh2/gui_edit.h"  // MapDialog
#include "../../rgl/text_stream.h"     // mb_open_user_file
#include "../../include/skinmagic.h"

// 0x43a1e0: the string table's lookup (thiscall on g_strtable 0x5e2a18, key -> localized text)
// (g_strtable is mesh2_common.h's LangTables, through gui_edit.h)
static __forceinline tstring* strtab_entry(const tstring& key) { return g_strtable.lookup(const_cast<tstring&>(key)); }
static __forceinline const char* strtab_lookup(const tstring& key) { return strtab_entry(key)->c_str(); }


#include "./list_modes.h"   // list_modes 0x4cf100, ModeList

const float g_width_scale = 0.0023752970155328512f;    // 0x59642c
const float g_height_scale = 0.0020833334419876337f;   // 0x596430

// skinmagic ordinal 9: skins a dialog window with a registered skin class name.

#include "../../include/crt.h"   // canonical: sprintf (0x536f8f)

// 0x4bab70 (config/config.cpp, byte-exact): build the user path of a file name in place. A static function in the
// original, so cl passes `out` in esi (0x4d029b); a local copy with the same body, as in keys_page.cpp.
static void user_path(char* out)                                            // 0x4bab70
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x1a, 0, 0, path);              // CSIDL_APPDATA
    sprintf(name, "%s\\Mount&Blade", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade\\%s", path, name);
}
bool mb_file_exists(const char* path);           // 0x415120
// The dialog procedure runs with this == g_app_obj (0x8ee3c48; Core): 0x4d0240 loads ecx with that address before
// 0x4b9b90 and passes the same register (edi) to 0x4bee10 / 0x4bf170 / 0x4c6f90 / 0x4b9790. Core, OptionsDlg and
// MapDialog all keep the dialog window at +0x380 (Core::options_hwnd, OptionsDlg::hwnd, MapDialog::hwnd); the code at
// 0x4b9790 / 0x4c6f90 serves the dialog through those views, so the calls go through the canonical classes.
// The launcher's option block is g_launcher_cfg, the Config at 0x5aa0a8 (+4 = texture_detail, the first combo's selection).
// rgl_config::save (0x415fb0): src/rgl/resource_manager.h declares it, but that header's g_log clashes with config.h's
// (strat/gui_manager.h), so the one-member class is repeated verbatim.
struct rgl_config { void save(FILE* f); };
// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
Config g_launcher_cfg = { 1, 50, 0, 100, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 15, 0, 0, 32, 1, 0, 0, 1, 1, 1 };   // 0x5aa0a8
#define LAUNCHER_CFG (&g_launcher_cfg)

const char k_config[] = "rgl_config.txt";        // 0x54b048


// A table key whose object lives for the full expression that uses it: the key is freed after the call the text is
// passed to (0x4cfa4f.. CB_ADDSTRING / SetDlgItemTextA, then ~tstring), and before the next statement.
#define KEY_TEXT(s) ((char*)strtab_lookup(tstring(s)))

// @ 0x4cf7b0
void GameApp::fill_video_tab()
{
    // TCM_INSERTITEM's TCITEM (0x4cf7c5 / 0x4cf7cd): mask TCIF_TEXT at +0, pszText at +0xc, iImage -1 at +0x14; the
    // other fields stay as the stack left them
    int item[6];
    item[0] = 1;
    item[5] = -1;
    options_ctrl = 0;

    HWND tabs = GetDlgItem(options_hwnd, 0x43d);
    item[3] = (int)KEY_TEXT("ui_video");
    SendMessageA(tabs, 0x1307, 0, (LPARAM)item);
    item[3] = (int)KEY_TEXT("ui_audio");
    SendMessageA(tabs, 0x1307, 1, (LPARAM)item);
    item[3] = (int)KEY_TEXT("ui_game");
    SendMessageA(tabs, 0x1307, 2, (LPARAM)item);
    item[3] = (int)KEY_TEXT("ui_advanced");
    SendMessageA(tabs, 0x1307, 3, (LPARAM)item);

    RECT dlg;
    RECT rc;
    GetWindowRect(options_hwnd, &dlg);
    GetWindowRect(tabs, &rc);
    rc.top -= 0x3c;
    rc.left -= 10;
    int top = rc.top;
    int left = rc.left;
    // the tab's design-unit edge scaled to the dialog's size (480 x 421 units, 0x4cf921..0x4cf995), less the corner
    int dh = dlg.bottom - dlg.top;
    int dw = dlg.right - dlg.left;
    int h = (int)((float)dh * (float)(rc.bottom - 0x3c) * g_height_scale - (float)top);
    int w = (int)((float)dw * (float)(rc.right - 10) * g_width_scale - (float)left);
    SetWindowPos(tabs, NULL, left - dlg.left, top - dlg.top, w, h, 0x14);

    HWND modes = GetDlgItem(options_hwnd, 0x45f);
    if (modes) {
        SendMessageA(modes, 0x14b, 0, 0);
        std::vector<tstring> list = list_modes();
        for (unsigned i = 0; i < list.size(); i++)
            SendMessageA(modes, 0x14a, i, (LPARAM)list[i].c_str());   // the empty string passes &buf (0x4cfa14)
    }
    SendMessageA(modes, 0x14a, 0, (LPARAM)KEY_TEXT("ui_use_desktop_resolution"));

    HWND quality = GetDlgItem(options_hwnd, 0x467);
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)KEY_TEXT("ui_m_ultra_high"));
        SendMessageA(quality, 0x151, i, 2);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)KEY_TEXT("ui_m_high"));
        SendMessageA(quality, 0x151, i, 1);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)KEY_TEXT("ui_m_low"));
        SendMessageA(quality, 0x151, i, 0);
    }
    {
        int i = SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14a, 0, (LPARAM)KEY_TEXT("ui_directx_9"));
        SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x151, i, 1);
    }
    {
        int i = SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14a, 0, (LPARAM)KEY_TEXT("ui_directx_7"));
        SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x151, i, 0);
    }

    ((MapDialog*)this)->refresh_list(1);

    // a copy of the table's string that lives to the end of the function; the key dies with its statement
    tstring caption(*strtab_entry(tstring("ui_configure_caption")));
    {
        char* p = (char*)caption.c_str();      // an empty string passes &buf (0x4cfc97)
        SendMessageA(options_hwnd, 0xc, 0, (LPARAM)p);
    }

    SetDlgItemTextA(options_hwnd, 0x43f, KEY_TEXT("ui_texture_detail"));
    SetDlgItemTextA(options_hwnd, 0x449, KEY_TEXT("ui_max_frame_rate"));
    SetDlgItemTextA(options_hwnd, 0x460, KEY_TEXT("ui_screen_size"));
    SetDlgItemTextA(options_hwnd, 0x462, KEY_TEXT("ui_antialiasing"));
    SetDlgItemTextA(options_hwnd, 0x492, KEY_TEXT("ui_shadow_quality"));
    SetDlgItemTextA(options_hwnd, 0x493, KEY_TEXT("ui_enable_vertex_shaders"));
    SetDlgItemTextA(options_hwnd, 0x44b, KEY_TEXT("ui_create_brf"));
    SetDlgItemTextA(options_hwnd, 0x443, KEY_TEXT("ui_disable_blood"));
    SetDlgItemTextA(options_hwnd, 0x446, KEY_TEXT("ui_disable_music"));
    SetDlgItemTextA(options_hwnd, 0x452, KEY_TEXT("ui_disable_sound"));
    SetDlgItemTextA(options_hwnd, 0x442, KEY_TEXT("ui_enable_cheats"));
    SetDlgItemTextA(options_hwnd, 0x44c, KEY_TEXT("ui_enable_edit_mode"));
    SetDlgItemTextA(options_hwnd, 0x44a, KEY_TEXT("ui_force_single_threading"));
    SetDlgItemTextA(options_hwnd, 0x441, KEY_TEXT("ui_enable_pixel_shaders"));
    SetDlgItemTextA(options_hwnd, 0x44d, KEY_TEXT("ui_enable_sound_variation"));
    SetDlgItemTextA(options_hwnd, 0x445, KEY_TEXT("ui_show_framerate"));
    SetDlgItemTextA(options_hwnd, 0x487, KEY_TEXT("ui_force_vsync"));
    SetDlgItemTextA(options_hwnd, 0x444, KEY_TEXT("ui_start_windowed"));
    SetDlgItemTextA(options_hwnd, 2, KEY_TEXT("ui_cancel2"));
    SetDlgItemTextA(options_hwnd, 1, KEY_TEXT("ui_ok2"));
    SetDlgItemTextA(options_hwnd, 0x45e, KEY_TEXT("ui_auto_adjust"));

    SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x406, 0, 0x64000a);   // TBM_SETRANGE: the result is dropped (0x4d0216)
    ((OptionsDlg*)this)->set_active_tab(options_ctrl);                    // options_ctrl is still the 0 set above
}

// @ 0x4d0240
LRESULT GameApp::config_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        options_hwnd = hwnd;
        SetSingleDialogSkin(hwnd, "Dialog");
        char path[MAX_PATH];
        sprintf(path, k_config);          // the file name is the format string itself (0x4d0295)
        user_path(path);
        if (mb_file_exists(path)) {
            FILE* f = mb_open_user_file(k_config, "r");
            LAUNCHER_CFG->load_file(f);
            fclose(f);
        } else {
            g_launcher_cfg.texture_detail = 0x46;
        }
        ((OptionsDlg*)&g_app_obj)->choose_dx_settings(LAUNCHER_CFG, false);
        fill_video_tab();
        show_options();
        return 1;
    case 0x114: {
        int v = (int)SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x400, 0, 0);
        char text[256];
        sprintf(text, "%d", v);
        SetWindowTextA(GetDlgItem(options_hwnd, 0x450), text);
        return 1;
    }
    case WM_NOTIFY:             // 0x4e: only the tab control (0x43d, compared as a 16-bit word) is handled
        if (LOWORD(wParam) != 0x43d)
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        {
            HWND tabs = GetDlgItem(options_hwnd, 0x43d);
            ((OptionsDlg*)this)->set_active_tab((int)SendMessageA(tabs, 0x130b, 0, 0));
        }
        // falls through to the control dispatch: a tab notification also runs it
    case WM_COMMAND:            // 0x111 enters the dispatch directly (4d03d1)
        switch (LOWORD(wParam)) {
        case 1: {               // ok: write the config back
            read_options();
            FILE* f = mb_open_user_file(k_config, "w");
            ((rgl_config*)LAUNCHER_CFG)->save(f);
            fclose(f);
            EndDialog(hwnd, 2);
            return 1;
        }
        case 2:                 // cancel
            EndDialog(hwnd, 2);
            return 1;
        case 0x448:             // refresh-rate edit: keep 10..999, else restore
            if (HIWORD(wParam) == 0x200) {
                BOOL ok;
                int v = (int)GetDlgItemInt(options_hwnd, 0x448, &ok, 0);   // signed compares (jl / jge)
                if (ok && v >= 10 && v < 1000) {
                    g_launcher_cfg.max_framerate = v;
                    return 1;
                }
                SetDlgItemInt(options_hwnd, 0x448, g_launcher_cfg.max_framerate, 0);
                return 1;
            }
            break;
        case 0x45e:             // auto adjust: re-detect and re-read the config
            read_options();
            g_launcher_cfg.first_time = 0;
            ((OptionsDlg*)this)->choose_dx_settings(LAUNCHER_CFG, true);
            show_options();
            break;
        case 0x45f:             // display-mode list changed
            if (HIWORD(wParam) == 9) {
                ((MapDialog*)this)->refresh_list(0);
                return 1;
            }
            break;
        }
        return 1;
    }
    return 0;
}

// clang-format on
