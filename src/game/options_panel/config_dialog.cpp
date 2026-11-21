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
// a copy of the table's text that outlives the key (the key is a local of the inlined body)
static __forceinline tstring strtab_text(const char* s) { tstring key(s); return *strtab_entry(key); }


#include "./list_modes.h"   // 0x4cf100 list_modes, ModeList

const float g_width_scale = 0.0023752970155328512f;    // 0x59642c
const float g_height_scale = 0.0020833334419876337f;   // 0x596430

// skinmagic ordinal 9: skins a dialog window with a registered skin class name.

#include "../../include/crt.h"   // canonical: sprintf (0x536f8f)

// 0x4bab70 (config/config.cpp, byte-exact): build the user path of a file name in place. A static function in the
// original, so cl passes `out` in esi (0x4d029b); a local copy with the same body, as in keys_page.cpp.
static void user_path(char* out)                                            // 0x4bab70
{
    char name[0x104];
    char path[0x104];

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
// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
Config g_launcher_cfg = { 1, 50, 0, 100, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 15, 0, 0, 32, 1, 0, 0, 1, 1, 1 };   // 0x5aa0a8
#define LAUNCHER_CFG (&g_launcher_cfg)

const char k_config[] = "rgl_config.txt";        // 0x54b048


// A localised text for a control label: the key is a temporary tstring, so its destructor runs inside the statement
// that uses the text (SetDlgItemTextA / CB_ADDSTRING / SendMessageA), as in the original (0x42bbd8 ctor, lookup,
// label call, 0x42bc40 dtor in each block).
#define LABEL_TEXT(id, s)                        \
    {                                            \
        tstring key(s);                          \
        SetDlgItemTextA(options_hwnd, id, strtab_lookup(key)); \
    }


// @ 0x4cf7b0
void GameApp::fill_video_tab()
{
    // TCM_INSERTITEM's TCITEM (0x4cf7c5 / 0x4cf7cd): mask TCIF_TEXT at +0, pszText at +0xc, iImage -1 at +0x14; the
    // other fields stay as the stack left them. The whole 28-byte struct (lParam too) is in the original's frame (0x60).
    TCITEMA item;
    item.mask = TCIF_TEXT;
    item.iImage = -1;
    options_ctrl = 0;

    HWND tabs = GetDlgItem(options_hwnd, 0x43d);
    {
        tstring key("ui_video");
        const char* text = strtab_lookup(key);
        item.pszText = (LPSTR)text;
    }
    SendMessageA(tabs, TCM_INSERTITEMA, 0, (LPARAM)&item);
    {
        tstring key("ui_audio");
        const char* text = strtab_lookup(key);
        item.pszText = (LPSTR)text;
    }
    SendMessageA(tabs, TCM_INSERTITEMA, 1, (LPARAM)&item);
    {
        tstring key("ui_game");
        const char* text = strtab_lookup(key);
        item.pszText = (LPSTR)text;
    }
    SendMessageA(tabs, TCM_INSERTITEMA, 2, (LPARAM)&item);
    {
        tstring key("ui_advanced");
        const char* text = strtab_lookup(key);
        item.pszText = (LPSTR)text;
    }
    SendMessageA(tabs, TCM_INSERTITEMA, 3, (LPARAM)&item);

    RECT dlg;
    RECT rc;
    GetWindowRect(options_hwnd, &dlg);
    GetWindowRect(tabs, &rc);
    rc.bottom -= 0x3c;
    rc.right -= 10;
    rc.left -= 10;
    rc.top -= 0x3c;
    // the tab's design-unit edge scaled to the dialog's size (480 x 421 units, 0x4cf921..0x4cf995), less the corner
    SetWindowPos(tabs, NULL, rc.left - dlg.left, rc.top - dlg.top,
                 (int)((float)(dlg.right - dlg.left) * (float)rc.right * g_width_scale - (float)rc.left),
                 (int)((float)(dlg.bottom - dlg.top) * (float)rc.bottom * g_height_scale - (float)rc.top), 0x14);

    HWND modes = GetDlgItem(options_hwnd, 0x45f);
    if (modes) {
        SendMessageA(modes, 0x14b, 0, 0);
        std::vector<tstring> list = list_modes();
        for (unsigned i = 0; i < list.size(); i++)
            SendMessageA(modes, 0x14a, i, (LPARAM)list[i].c_str());   // the empty string passes &buf (0x4cfa14)
    }
    {
        tstring key("ui_use_desktop_resolution");
        SendMessageA(modes, 0x14a, 0, (LPARAM)strtab_lookup(key));
    }

    HWND quality = GetDlgItem(options_hwnd, 0x467);
    {
        int i;
        {
            tstring key("ui_m_ultra_high");
            i = SendMessageA(quality, 0x14a, 0, (LPARAM)strtab_lookup(key));
        }
        SendMessageA(quality, 0x151, i, 2);
    }
    {
        int i;
        {
            tstring key("ui_m_high");
            i = SendMessageA(quality, 0x14a, 0, (LPARAM)strtab_lookup(key));
        }
        SendMessageA(quality, 0x151, i, 1);
    }
    {
        int i;
        {
            tstring key("ui_m_low");
            i = SendMessageA(quality, 0x14a, 0, (LPARAM)strtab_lookup(key));
        }
        SendMessageA(quality, 0x151, i, 0);
    }
    {
        int i;
        {
            tstring key("ui_directx_9");
            i = SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14a, 0, (LPARAM)strtab_lookup(key));
        }
        SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x151, i, 1);
    }
    {
        int i;
        {
            tstring key("ui_directx_7");
            i = SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14a, 0, (LPARAM)strtab_lookup(key));
        }
        SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x151, i, 0);
    }

    refresh_multisample_list(1);

    // a copy of the table's string that lives to the end of the function (the key dies before the caption is set)
    tstring caption = strtab_text("ui_configure_caption");
    SendMessageA(options_hwnd, WM_SETTEXT, 0, (LPARAM)caption.c_str());   // an empty string passes &buf (0x4cfc97)

    LABEL_TEXT( 0x43f, "ui_texture_detail");
    LABEL_TEXT( 0x449, "ui_max_frame_rate");
    LABEL_TEXT( 0x460, "ui_screen_size");
    LABEL_TEXT( 0x462, "ui_antialiasing");
    LABEL_TEXT( 0x492, "ui_shadow_quality");
    LABEL_TEXT( 0x493, "ui_enable_vertex_shaders");
    LABEL_TEXT( 0x44b, "ui_create_brf");
    LABEL_TEXT( 0x443, "ui_disable_blood");
    LABEL_TEXT( 0x446, "ui_disable_music");
    LABEL_TEXT( 0x452, "ui_disable_sound");
    LABEL_TEXT( 0x442, "ui_enable_cheats");
    LABEL_TEXT( 0x44c, "ui_enable_edit_mode");
    LABEL_TEXT( 0x44a, "ui_force_single_threading");
    LABEL_TEXT( 0x441, "ui_enable_pixel_shaders");
    LABEL_TEXT( 0x44d, "ui_enable_sound_variation");
    LABEL_TEXT( 0x445, "ui_show_framerate");
    LABEL_TEXT( 0x487, "ui_force_vsync");
    LABEL_TEXT( 0x444, "ui_start_windowed");
    LABEL_TEXT( 2, "ui_cancel2");
    LABEL_TEXT( 1, "ui_ok2");
    LABEL_TEXT( 0x45e, "ui_auto_adjust");

    SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x406, 0, 0x64000a);   // TBM_SETRANGE: the result is dropped (0x4d0216)
    set_active_tab(options_ctrl);                    // options_ctrl is still the 0 set above
}

// @ 0x4d0240
LRESULT GameApp::config_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char text[0x100];
    switch (msg) {
    case WM_INITDIALOG: {
        // the launcher's own option block: read rgl_config.txt, or take the default texture detail (0x4d02dd)
        char path[0x104];
        options_hwnd = hwnd;
        SetSingleDialogSkin(hwnd, "Dialog");
        sprintf(path, k_config);
        user_path(path);
        if (mb_file_exists(path)) {
            FILE* f = mb_open_user_file(k_config, "r");
            LAUNCHER_CFG->load_file(f);
            fclose(f);
        } else
            LAUNCHER_CFG->texture_detail = 0x46;
        g_app_obj.choose_dx_settings(LAUNCHER_CFG, false);   // 0x4d02f3
        fill_video_tab();
        show_options();
        return 1;
    }
    case WM_HSCROLL:                                 // 0x114
    {
        // the band of the quality slider, for the label next to it
        sprintf(text, "%d", (int)SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x400, 0, 0));   // TBM_GETPOS
        SetWindowTextA(GetDlgItem(options_hwnd, 0x450), text);
        return 1;
    }
    case 0x4e:                                       // WM_NOTIFY
    {
        if (LOWORD(wParam) != 0x43d)                 // not the tab control: let Windows have it
            return (LRESULT)DefWindowProcA(hwnd, msg, wParam, lParam);
        HWND tabs = GetDlgItem(options_hwnd, 0x43d);
        set_active_tab((int)SendMessageA(tabs, 0x130b, 0, 0));
        // falls through
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 0x448:                                  // 1096: the maximum frame rate edit box (10..999)
            if (HIWORD(wParam) == 0x200) {           // EN_KILLFOCUS
                BOOL ok;
                UINT v = GetDlgItemInt(options_hwnd, 0x448, &ok, 0);
                if (ok && (int)v >= 10 && (int)v < 1000) {
                    LAUNCHER_CFG->max_framerate = v;
                    return 1;
                }
                SetDlgItemInt(options_hwnd, 0x448, LAUNCHER_CFG->max_framerate, 0);
                return 1;
            }
            break;
        case IDOK:                                   // 1: keep what the dialog shows, write rgl_config.txt
            read_options();
            {
                FILE* f = mb_open_user_file(k_config, "w");
                LAUNCHER_CFG->save(f);
                fclose(f);
            }
            EndDialog(hwnd, 2);
            return 1;
        case IDCANCEL:                               // 2
            EndDialog(hwnd, 2);
            return 1;
        case 0x45e:                                  // 1118: auto-adjust: take the defaults and show them
            read_options();
            LAUNCHER_CFG->first_time = 0;
            choose_dx_settings(LAUNCHER_CFG, true);
            show_options();
            break;
        case 0x45f:                                  // 1119: the Apply button
            if (HIWORD(wParam) == 9) {               // BN_CLICKED
                refresh_multisample_list(0);
                return 1;
            }
            break;
        default:
            break;
        }
        break;
    default:
        return 0;
    }
    return 1;
}