//-----------------------------------------------------------------------------
// File: trigger_vec.cpp  (0x4be742..0x4c2c50)
//
// Desc: the meta-mission and script-code tables, the vector copy helpers, the registry / module-folder helpers, the
//       application object's destructor, options dialog and script debugger dialog, the module_info.txt parser,
//       the shader / device-object handlers, the module-file record vectors (trigger, game, mouse records and the
//       trigger entries), the dialog table constructor, the music monitor and the loadable-record readers.
//       Names provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: see MIGRATION.md. No forward call proves a boundary anywhere in 0x4be742..0x4c2c50 (the one forward
// call, 0x4be730 -> 0x4be742, is the jump over a WinLicense marker), so the four map TUs (trigger_vec, game_vec,
// trigger_entry_vec, big_obj) are one module.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <io.h>
#include <new>
#include "../../rgl/rgl_string.h"      // rgl::string::read_from (0x4151e0)
#include "./trigger_vec.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../game_globals.h"      // canonical declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../trigger_vecs.h"           // (listed directly: the resolution audit reads only the source's own includes)
#include <richedit.h>
#include "../config/core.h"
#include "../config/config.h"
#include "../emit.h"                 // EmitTable, g_emit_defs (0x8ed5090)
#include "../party/game.h"
#include "../../rgl/xmlcfg/records.h"     // (listed directly: the resolution audit scans only the source's own includes)
#include "../inventory_panel/mission_ctx_rec.h"      // MissionCtx (init 0x4be730; the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../menu_rec.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../rgl/options.h"   // g_options 0x5aa008

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_compatible_module_version;                            // 0x5e29b8
#include "../shared_vars.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_display_wp[7];   // 0x5e2900
ID3DXEffect* g_effect;   // 0x5e293c
#include "../../include/render_flags.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----




// ---- data this module owns (docs/standalone_data.md) ----
// The module settings object (module_settings.h): module_info.txt / game_variables.txt are read into it below.
ModuleSettings g_module_settings;                // 0xa180f8 (.bss, built by the dynamic initialiser below)
tstring g_module_name;                           // 0x8ee3c38 (.bss) the selected module (dynamic initialiser: tstring ctor + atexit dtor)

// @ 0x547bc0   the defaults before module_info.txt / game_variables.txt are read
ModuleSettings::ModuleSettings()
{
    f0d8 = 10000;  f0dc = 10000;  f0e4 = 10000;  f0ec = 10000;
    f0f0 = 10000;  f0f4 = 10000;  f0f8 = 10000;  f0fc = 10000;
    f0e0 = 200000;
    f0e8 = 0x5399;
    main_menu_font_color_red = 0;  main_menu_font_color_green = 0;  main_menu_font_color_blue = 0;
    works_with_version_min = 0;
    works_with_version_max = 1000000;
    scan_module_textures = 0;  scan_module_sounds = 0;
    time_multiplier = 0.15f;
    seeing_range = 6.5f;
    track_spotting = 0.8f;
    blood_multiplier = 1.0f;
    skill_leadership_bonus = 5;  skill_prisoner_management_bonus = 5;  base_companion_limit = 5;
    player_xp_multiplier = 1.0f;  hero_xp_multiplier = 1.0f;  regulars_xp_multiplier = 1.0f;
    hero_wounded_treshold = 15.0f;
    player_wounded_treshold = 5.0f;
    num_hints = 0;
    map_sea_direction = 0.0f;  map_sea_speed_x = 0.0f;  map_sea_speed_y = 0.0f;
    map_river_direction = 0.0f;  map_river_speed_x = 0.0f;  map_river_speed_y = 0.0f;
    map_tree_types = 0;  map_snow_tree_types = 0;  map_steppe_tree_types = 0;  map_desert_tree_types = 0;
    starting_year = 1200;  starting_month = 2;  starting_day = 12;
    damage_interrupt_attack_treshold = 0.0f;
    armor_soak_factor[0] = 1.0f;  armor_soak_factor[1] = 0.5f;  armor_soak_factor[2] = 0.5f;
    armor_reduction_factor[0] = 0.0f;  armor_reduction_factor[1] = 0.0f;  armor_reduction_factor[2] = 0.0f;
    missile_damage_speed_power = 1.0f;  melee_damage_speed_power = 1.0f;
    horse_charge_damage_multiplier = 1.0f;  couched_lance_damage_multiplier = 1.0f;
    enable_quick_battles = 0;  disable_food_slot = 0;  limit_hair_colors = 0;  show_faction_color = 0;  show_quest_notes = 0;
    show_party_ids_instead_of_names = 0;
}

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_module_version;   // 0x5e29c0
float g_map_min_y = -50.0f;   // 0x5aa124
float g_map_min_x = -50.0f;   // 0x5aa120
float g_map_max_y = 50.0f;   // 0x5aa11c
float g_map_max_x = 50.0f;   // 0x5aa118


MissionCtx g_mission_ctx;   // 0xa73300 (.bss, 0x2cc bytes; constructed by the compiler's static initializer, the original's 0x547e40, which calls MissionCtx::init below)

void __cdecl operator delete[](void* p);                             // 0x53570d (operator delete[]; crt.h cannot be included here: its ftol2 clashes with the declaration in the included headers)

// @ 0x4be730
void MissionCtx::init()
{
    WL_MARKER(12)
    mode = 0;
    f144 = 0;
    side = 0;
    f10 = 0;
    f08 = -1;
    ids[0] = -1;
    ids[1] = -1;
    f13c = -1;
    f140 = -1;
    f24 = 0;
    f1c = 0;
    f0c = -1;
    f154 = -1;
    f158 = -1;
    f15c = -1;
    f3c = 0x400;
    f168 = 0;
    scene_counter = 0;
    passage_menu = 0;
    f2c8 = 0;
    f40 = 0;
    WL_MARKER(13)
}

// @ 0x4be7c0
void GameKeys::set_defaults()
{
    int* v = &entry[0][0];
    memset(v, -1, sizeof entry);
    v[0x00 / 4] = 0x11; v[0x08 / 4] = 0x1f; v[0x10 / 4] = 0x1e;
    v[0x18 / 4] = 0x20; v[0x20 / 4] = 0x21; v[0x28 / 4] = 0x39;
    v[0x30 / 4] = 0xe0; v[0x38 / 4] = 0xe1; v[0x40 / 4] = 0xee;
    v[0x48 / 4] = 0xef; v[0x50 / 4] = 0x2b; v[0x78 / 4] = 0x2e;
    v[0x80 / 4] = 0x17; v[0x88 / 4] = 0x19; v[0x90 / 4] = 0x10;
    v[0x98 / 4] = 0x26; v[0x58 / 4] = 0x0f; v[0x60 / 4] = 0x2a;
    v[0x68 / 4] = 0x29; v[0x70 / 4] = 0x13; v[0xa0 / 4] = 0x0e;
    v[0xa8 / 4] = 0x3b; v[0xb0 / 4] = 0x3c; v[0xb8 / 4] = 0x3d;
    v[0xc0 / 4] = 0x3e; v[0xc8 / 4] = 0x3f; v[0xd0 / 4] = 0x40;
    v[0xd8 / 4] = 0x41; v[0xe0 / 4] = 0x42; v[0xe8 / 4] = 0x43;
    v[0xf0 / 4] = 0x57; v[0xf8 / 4] = 2;    v[0x100 / 4] = 3;
    v[0x108 / 4] = 4;   v[0x110 / 4] = 5;   v[0x118 / 4] = 6;
    v[0x120 / 4] = 0x58;
}

// @ 0x4be920
ini_file::~ini_file()
{
}

// @ 0x4be950
int vec_e50::size() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0x50);
}

// @ 0x4be970
int vec_e1f8::size() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0x1f8);
}

// @ 0x4be9a0
int vec_eb0::size() const
{
    if (!begin)
        return 0;
    return (int)((end - begin) / 0xb0);
}

// @ 0x4be9c0
clip_pair::clip_pair(void* a_, void* b_, const int* src)
{
    a = a_;
    b = b_;
    memcpy(d, src, sizeof(d));
}

// @ 0x4be9f0
blk2c* copy_fwd_2c(blk2c* first, blk2c* last, blk2c* dst)
{
    for (; first != last; ++first, ++dst)
        *dst = *first;
    return dst;
}

// @ 0x4bea20
void fill_2c(blk2c* first, blk2c* last, const blk2c* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4bea50
blk2c* copy_back_2c(blk2c* first, blk2c* last, blk2c* dst)
{
    for (; first != last; ) {
        --last;
        --dst;
        *dst = *last;
    }
    return dst;
}

// @ 0x4bea80
blk204* copy_fwd_204(blk204* first, blk204* last, blk204* dst)
{
    for (; first != last; ++first, ++dst)
        *dst = *first;
    return dst;
}

// @ 0x4beac0
void fill_204(blk204* first, blk204* last, const blk204* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4beaf0
blk204* copy_back_204(blk204* first, blk204* last, blk204* dst)
{
    for (; first != last; ) {
        --last;
        --dst;
        *dst = *last;
    }
    return dst;
}

// @ 0x4beb30
void fill_int(int* first, int* last, const int* val)
{
    for (; first != last; ++first)
        *first = *val;
}

// @ 0x4beb50
int* copy_back_int(int* first, int* last, int* dst)
{
    int* p = last;
    while (first != p) {
        --p;
        --dst;
        *dst = *p;
    }
    return dst;
}

// ---- registry / path helpers ---------------------------------------------------------------------------------------------
// @ 0x4beb80
void keys_write_value(HKEY* key, LPCSTR name, const tstring* v)
{
    if (*key != (HKEY)0) {
        int len = v->len;
        const char* data = v->buf ? v->buf : (const char*)v + 4;
        RegSetValueExA(*key, name, 0, 1 /*REG_SZ*/, (const BYTE*)data, len + 1);
    }
}

// @ 0x4bebc0
bool module_ini_exists(const char* module)
{
    bool found = false;
    tstring path("Modules\\");
    {
        tstring name(module);
        path.append(name);
    }
    {
        tstring ini("\\module.ini");
        path.append(ini);
    }
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path.buf ? path.buf : (const char*)&path.buf, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
        found = true;
    }
    return found;
}

// @ 0x4bec60
void scan_module_folders(tstring* out, int* count)
{
    *count = 0;
    _finddata_t findbuf;
    tstring names[0x20];

    int h = _findfirst("Modules\\*", &findbuf);
    if ((findbuf.attrib & 0x10) != 0 && module_ini_exists(findbuf.name)) {
        names[*count].assign(findbuf.name);
        (*count)++;
    }
    if (h != -1) {
        while (_findnext(h, &findbuf) == 0) {
            if (*count >= 0x20)
                break;
            if (module_ini_exists(findbuf.name)) {
                names[*count].assign(findbuf.name);
                (*count)++;
            }
        }
    }
    _findclose(h);

    for (int i = 0; i < *count; i++)
        out[i] = names[(*count - i) - 1];
}

// ---- the application object's destructor (vtable 0x5948e0, base CD3DApplication vtable 0x549c60) ---------------------------
// @ 0x4bed90   (the scalar deleting destructor 0x4bedf0 is the compiler's)
GameApp::~GameApp()
{
    CD3DFont* f = font;
    if (f != 0) {
        f->~CD3DFont();
        operator delete(f);
        font = 0;
    }
}

// ---- options dialog: write the stored values into the controls (0x4bee10), read them back (0x4bf170) -------------------
// @ 0x4bee10
void GameApp::show_options()
{
    char buf[0x200];

    SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x405, 1, g_launcher_cfg.texture_detail);
    sprintf(buf, "%d", g_launcher_cfg.texture_detail);
    SetWindowTextA(GetDlgItem(options_hwnd, 0x450), buf);
    SendMessageA(GetDlgItem(options_hwnd, 0x441), BM_SETCHECK, g_launcher_cfg.use_pixel_shaders != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x444), BM_SETCHECK, g_launcher_cfg.start_windowed != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x445), BM_SETCHECK, g_launcher_cfg.show_framerate != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x487), BM_SETCHECK, g_launcher_cfg.force_vsync != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x452), BM_SETCHECK, g_launcher_cfg.disable_sound != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x446), BM_SETCHECK, g_launcher_cfg.disable_music != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44d), BM_SETCHECK, g_launcher_cfg.disable_frequency_variation == 0, 0);
    SetDlgItemInt(options_hwnd, 0x448, g_launcher_cfg.max_framerate, FALSE);
    SendMessageA(GetDlgItem(options_hwnd, 0x442), BM_SETCHECK, g_launcher_cfg.cheat_mode != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x443), BM_SETCHECK, g_launcher_cfg.enable_blood == 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44c), BM_SETCHECK, g_launcher_cfg.enable_edit_mode != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44a), BM_SETCHECK, g_launcher_cfg.force_single_threading != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x467), 0x14e, g_launcher_cfg.shadowmap_quality, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14e, g_launcher_cfg.use_vertex_shaders, 0);
    if (g_launcher_cfg.display_width == 0) {
        SendMessageA(GetDlgItem(options_hwnd, 0x45f), 0x14e, 0, 0);
    } else {
        char num[0x80];
        tstring s(_itoa(g_launcher_cfg.display_width, num, 10));
        s.push_back('x');
        {
            tstring h(_itoa(g_launcher_cfg.display_height, num, 10));
            s.append(h);
        }
        {
            tstring b(g_launcher_cfg.display_bits == 0x10 ? " 16 bits" : " 32 bits");
            s.append(b);
        }
        SendMessageA(GetDlgItem(options_hwnd, 0x45f), 0x14d, 0,
                     (LPARAM)(s.buf ? s.buf : (const char*)&s.buf));
    }
}

// @ 0x4bf170
void GameApp::read_options()
{
    BOOL ok;
    DWORD q;
    g_launcher_cfg.texture_detail = SendMessageA(GetDlgItem(options_hwnd, 0x43e), WM_USER, 0, 0);
    g_launcher_cfg.use_pixel_shaders = SendMessageA(GetDlgItem(options_hwnd, 0x441), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.use_vertex_shaders = SendMessageA(GetDlgItem(options_hwnd, 0x468), CB_GETCURSEL, 0, 0) == 1;
    g_launcher_cfg.cheat_mode = SendMessageA(GetDlgItem(options_hwnd, 0x442), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.enable_blood = SendMessageA(GetDlgItem(options_hwnd, 0x443), BM_GETCHECK, 0, 0) != 1;
    g_launcher_cfg.start_windowed = SendMessageA(GetDlgItem(options_hwnd, 0x444), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.show_framerate = SendMessageA(GetDlgItem(options_hwnd, 0x445), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.force_vsync = SendMessageA(GetDlgItem(options_hwnd, 0x487), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.disable_sound = SendMessageA(GetDlgItem(options_hwnd, 0x452), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.disable_music = SendMessageA(GetDlgItem(options_hwnd, 0x446), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.disable_frequency_variation = SendMessageA(GetDlgItem(options_hwnd, 0x44d), BM_GETCHECK, 0, 0) != 1;
    {   // "fps limit" edit box, clamped to 10..999
        UINT v = GetDlgItemInt(options_hwnd, 0x448, &ok, FALSE);
        if (ok != 0) {
            int iv = (int)v;
            if (iv < 10)
                g_launcher_cfg.max_framerate = 10;
            else if (iv >= 1000)
                g_launcher_cfg.max_framerate = 999;
            else
                g_launcher_cfg.max_framerate = iv;
        }
    }
    g_launcher_cfg.enable_edit_mode = SendMessageA(GetDlgItem(options_hwnd, 0x44c), BM_GETCHECK, 0, 0) == 1;
    g_launcher_cfg.force_single_threading = SendMessageA(GetDlgItem(options_hwnd, 0x44a), BM_GETCHECK, 0, 0) == 1;
    if (SendMessageA(GetDlgItem(options_hwnd, 0x45f), CB_GETCURSEL, 0, 0) == 0) {
        g_launcher_cfg.display_width = 0;
        g_launcher_cfg.display_height = 0;
        int md = mode.Format;
        if (md == 0x17 || md == 0x19 || md == 0x1a || md == 0x18 || md == 0x1e)
            g_launcher_cfg.display_bits = 0x10;
        else
            g_launcher_cfg.display_bits = 0x20;
    } else {
        // the edit box holds "<width>x<height> <bits>"
        char buf[0x80];
        GetWindowTextA(GetDlgItem(options_hwnd, 0x45f), buf, 100);
        tstring s(buf);
        int ix = s.find('x', 0);
        int isp = s.find(' ', 0);
        g_launcher_cfg.display_width = atoi(s.substr(0, ix).c_str());
        g_launcher_cfg.display_height = atoi(s.substr(ix + 1, isp).c_str());
        g_launcher_cfg.display_bits = atoi(s.substr(isp + 1, isp + 3).c_str());
    }
    g_launcher_cfg.antialiasing = SendMessageA(GetDlgItem(options_hwnd, 0x465), CB_GETITEMDATA,
                                     SendMessageA(GetDlgItem(options_hwnd, 0x465), CB_GETCURSEL, 0, 0), 0);   // (one store, 0x4bf53a)
    g_launcher_cfg.shadowmap_quality = SendMessageA(GetDlgItem(options_hwnd, 0x467), CB_GETITEMDATA,
                               SendMessageA(GetDlgItem(options_hwnd, 0x467), CB_GETCURSEL, 0, 0), 0);
    int m;
    if (g_launcher_cfg.display_bits == 0x10)
        m = 0x17;
    else if (g_launcher_cfg.display_bits == 0x20)
        m = 0x16;
    else
        m = mode.Format;
    if (g_launcher_cfg.antialiasing > 1) {
        if (m_pD3D->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, (D3DFORMAT)m, FALSE,
                                               g_launcher_cfg.antialiasing != 2 ? D3DMULTISAMPLE_4_SAMPLES : D3DMULTISAMPLE_2_SAMPLES, &q) >= 0) {
            g_launcher_cfg.sample_quality = q - 1;
            return;
        }
    }
    g_launcher_cfg.sample_quality = 0;
}

// @ 0x4bf5f0
INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        g_app_obj.dlg_options = dlg;
        SetDlgItemTextA(dlg, 0x488,
    "Common Keys\r\n"
        "-----------------\r\n"
        "Left Mouse Button: While pressed, mouse movements rotate\r\n"
        "the camera.\r\n"
        "H: Hides/unhides the highlights and user interface objects\r\n"
        "(Good for taking a screenshot).\r\n"
        "CTRL + Any movement key: Speeds up the camera\r\n"
        "movements, slows down the object movements.\r\n"
        "\r\n"
        "Edit Objects Mode\r\n"
        "----------------------\r\n"
        "Right Mouse Button: Selects objects.\r\n"
        "CTRL + Right Mouse Button: Selects multiple objects.\r\n"
        "Double Click on Scene Objects list: Selects the clicked\r\n"
        "object and moves the camera towards it.\r\n"
        "A,S,D,W: Moves the camera.\r\n"
        "C,F: Increases/decreases the height of the camera.\r\n"
        "G: While pressed, mouse movements move the selected\r\n"
        "object(s) parallel to the ground.\r\n"
        "T: While pressed, mouse movements increase/decrease the\r\n"
        "height of the selected object(s).\r\n"
        "X,Y,Z: While pressed, mouse movements rotate the selected\r\n"
        "object(s) with respect to the object's X, Y and Z axis.\r\n"
        "U: While pressed, mouse movements rotate the selected\r\n"
        "object(s) with respect to the \"Up\" axis of the world.\r\n"
        "R: Resets the selected object's rotation.\r\n"
        "B: Scale selected object.\r\n"
        "B + X: Scale selected object along X axis.\r\n"
        "B + Y: Scale selected object along Y axis.\r\n"
        "B + Z: Scale selected object along Z axis.\r\n"
        "Alt + B: Resets the selected object's scale.\r\n"
        "Delete: Deletes selected object(s).\r\n"
        "Space: Enables add object mode.\r\n"
        "\r\n"
        "Add Object Mode\r\n"
        "-------------------\r\n"
        "Right Mouse Button: Adds current object to scene.\r\n"
        "Space: Disables add object mode.\r\n"
        "T, U, X, Y, Z, R and B can also be used in this mode.\r\n"
        "\r\n"
        "Ground Elevate and Ground Paint Mode\r\n"
        "-----------------------------------------------\r\n"
        "Right Mouse Button: Elevates the ground up or down in\r\n"
        "\"Ground Elevate\" mode and paints the ground in \"Ground\r\n"
        "Paint\" mode.\r\n"
        "Midle Mouse Button: Clears the elevation in \"Ground Elevate\"\r\n"
        "mode and clears the paints in \"Ground Paint\" mode.\r\n"
        "\r\n"
        "Edit AI Mesh Mode\r\n"
        "---------------------\r\n"
        "Right Mouse Button: Selects AI mesh objects.\r\n"
        "CTRL + Right Mouse Button: Selects multiple AI mesh objects.\r\n"
        "1: Activates Vertex Editing Mode\r\n"
        "2: Activates Edge Editing Mode\r\n"
        "3: Activates Face Editing Mode\r\n"
        "G, T, X, Y, Z, B and Delete can also be used in this mode.\r\n"
        "\r\n"
        "Additional Help\r\n"
        "------------------\r\n"
        "- You can save your changes only when you leave the edit\r\n"
        "mode.\r\n"
        "\r\n"
        "- You can not undo your works, you can only discard changes\r\n"
        "while leaving the edit mode. So you should save your work\r\n"
        "occasionally by leaving the edit mode and re-entering it.\r\n"
        "\r\n"
        "- Scene files are located under [Current Module]\\SceneObj\r\n"
        "foler. When you save your changes, the related scene file\r\n"
        "under this folder will be updated.\r\n"
        "\r\n"
        "- Create AI Mesh button works only on outdoor scenes.");
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:
            EndDialog((HWND)g_app_obj.dlg_options, 1);
            break;
        }
        break;
    }
    return FALSE;
}

// @ 0x4bf640
// (the original sets the first five buttons with BM_SETSTATE, 0x474 / 0x497..0x499 as well)
void CreateGameOptionsPage::update_checks()
{
    SendMessageA(GetDlgItem(hDlg, 0x470), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 1, 0);
    SendMessageA(GetDlgItem(hDlg, 0x46d), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 2, 0);
    SendMessageA(GetDlgItem(hDlg, 0x471), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 3, 0);
    SendMessageA(GetDlgItem(hDlg, 0x49a), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 4, 0);
    SendMessageA(GetDlgItem(hDlg, 0x46f), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x434), BM_SETCHECK, ((GameConfig*)g_game_G)->check_a098 != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x453), BM_SETCHECK, ((GameConfig*)g_game_G)->check_a0cc != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x454), BM_SETCHECK, ((GameConfig*)g_game_G)->check_a0d0 != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x497), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 4 && ((GameConfig*)g_game_G)->sub_option == 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x498), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 4 && ((GameConfig*)g_game_G)->sub_option == 1, 0);
    SendMessageA(GetDlgItem(hDlg, 0x499), BM_SETSTATE, ((GameConfig*)g_game_G)->game_mode == 4 && ((GameConfig*)g_game_G)->sub_option == 2, 0);
    refresh();
}

// ---- the script debugger dialog (a method of the application object: 0x4c3330 jumps here with ecx = g_app_obj) ----------------
// WM_INITDIALOG lays the controls out and fills the script combo from ID_scripts.py; WM_COMMAND handles the Stop / Debug
// toggle (0x4b3), the reset button (2), the index combo (0x46c) and the script combo (0x4bb, which loads module_scripts.py
// without its "#" comment lines into the viewer and colours the quoted strings); WM_SIZING keeps the layout.
// @ 0x4bf870
INT_PTR GameApp::script_debugger_proc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char msgbuf[0x1000];
    char buf[0x400];
    switch (msg) {
    case WM_INITDIALOG: {
        script_dlg = h;
        RECT rc, br;
        GetWindowRect(h, &rc);
        SetWindowPos(GetDlgItem(script_dlg, 0x4b0), 0, rc.left + 10, rc.top + 0x78,
                     (rc.right - rc.left) - 0x1e, (rc.bottom - rc.top) - 0xf5, 0x14);
        GetWindowRect(GetDlgItem(script_dlg, 2), &br);
        SetWindowPos(GetDlgItem(script_dlg, 2), 0,
                     (((rc.right - rc.left) / 2) - br.right) + br.left + rc.left,
                     rc.bottom - 0x73, br.right - br.left, br.bottom - br.top, 0x14);
        SetDlgItemTextA(script_dlg, 0x4b0, "");
        SetDlgItemTextA(script_dlg, 0x4b4,
            "When the game freezes, press 1 to process the next line, 2 to step into the currently called script, 3 to step out of the current script or to proceed to the next occurence of the line that we want the debugger to stop at, 4 to process the whole script, and 5 to leave the debug mode.");
        tstring path;
        HKEY key;
        open_keys_key(&key);
        if (key != (HKEY)0) {
            read_key_string(&key, "module_system_folder", &path);
            if (key != (HKEY)0)
                RegCloseKey(key);
        }
        {
            tstring name("ID_scripts.py");
            path.append(name);
        }
        FILE* f = fopen(path.c_str(), "r");
        if (f == 0) {
            ((GameConfig*)g_game_G)->debug_file = (int)f;
            sprintf(msgbuf,
                "File %s could not be found. Please set the correct module system folder by choosing \"Set Debug Mode Folder\" on the menu.", path.c_str());
            MessageBoxA(0, msgbuf, "ERROR", 0);
        } else {
            int n = (int)file_length(f);
            tstring text;
            (&text)->read_from(f, n);       // rgl::string is tstring (same 16 bytes): rgl_string.h read_from 0x4151e0
            fclose(f);
            HWND combo = GetDlgItem(h, 0x4bb);
            SendMessageA(combo, CB_RESETCONTENT, 0, 0);
            int i;
            {
                tstring pfx("script_");
                i = text.find(pfx, 0) + 7;
            }
            char sp = ' ';
            int e = text.find(sp, i);
            while (i >= 7 && e >= 0) {
                SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)text.substr(i, e).c_str());
                {
                    tstring pfx("script_");
                    i = text.find(pfx, e) + 7;
                }
                e = text.find(sp, i);
            }
        }
        return 1;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 0x46c:
            if (HIWORD(wParam) == 9) {
                SendMessageA(GetDlgItem(script_dlg, 0x4bb), CB_GETLBTEXT,
                             SendMessageA(GetDlgItem(script_dlg, 0x4bb), CB_GETCURSEL, 0, 0), (LPARAM)buf);
                ((GameConfig*)g_game_G)->script_text.assign(buf);
                LRESULT ix = SendMessageA(GetDlgItem(script_dlg, 0x46c), CB_GETCURSEL, 0, 0);
                if (ix >= 0)
                    ((GameConfig*)g_game_G)->script_index = SendMessageA(GetDlgItem(script_dlg, 0x46c), CB_GETITEMDATA, ix, 0);
                else
                    ((GameConfig*)g_game_G)->script_index = 0;
                return 0;
            }
            break;
        case 2:
            ((GameConfig*)g_game_G)->debug_file = 0;
            ((GameConfig*)g_game_G)->settings_text.assign("");
            return 0;
        case 0x4b3:
            ((GameConfig*)g_game_G)->paused = 1 - ((GameConfig*)g_game_G)->paused;
            if (((GameConfig*)g_game_G)->paused)
                SetDlgItemTextA(script_dlg, 0x4b3, "Stop");
            else
                SetDlgItemTextA(script_dlg, 0x4b3, "Debug");
            return 0;
        case 0x4bb:
            if (HIWORD(wParam) == 9) {
                if (((GameConfig*)g_game_G)->settings_text.len == 0) {
                    tstring path;
                    HKEY key;
                    open_keys_key(&key);
                    if (key != (HKEY)0) {
                        read_key_string(&key, "module_system_folder", &path);
                        if (key != (HKEY)0)
                            RegCloseKey(key);
                    }
                    {
                        tstring name("module_scripts.py");
                        path.append(name);
                    }
                    FILE* f = fopen(path.c_str(), "r");
                    if (f == 0) {
                        ((GameConfig*)g_game_G)->debug_file = (int)f;
                        sprintf(msgbuf,
                            "File %s could not be found. Please set the correct module system folder by choosing \"Set Debug Mode Folder\" on the menu.", path.c_str());
                        MessageBoxA(0, msgbuf, "ERROR", 0);
                        return 1;
                    }
                    int n = (int)file_length(f);
                    tstring text;
                    (&text)->read_from(f, n);       // rgl::string is tstring (same 16 bytes): rgl_string.h read_from 0x4151e0
                    fclose(f);
                    // strip the comment lines ("#" up to the next line feed), last to first
                    int end = text.len - 1;       // (0x4bfc55: the string's length, not the file's: the text-mode read drops the CRs)
                    for (;;) {
                        char hash = '#';
                        end = text.rfind(hash, 0, end);
                        if (end < 0)
                            break;
                        char lf = '\n';
                        int q = text.find(lf, end);
                        if (q < 0)
                            break;
                        text.erase_range(end, q);
                    }
                    ((GameConfig*)g_game_G)->settings_text = text;
                    SetDlgItemTextA(script_dlg, 0x4b0, text.c_str());
                    tstring quote("\"");
                    FINDTEXTEXA ft;
                    ft.chrg.cpMin = 0;
                    ft.chrg.cpMax = 0xfa000;
                    ft.lpstrText = (LPSTR)quote.c_str();
                    int a = (int)SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
                    CHARFORMAT2A cf;
                    cf.cbSize = 0x3c;
                    cf.dwMask = CFM_SIZE | CFM_FACE;
                    sprintf(cf.szFaceName, "Courier New");
                    cf.yHeight = 0xdc;
                    SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
                    if (a >= 0) {
                        do {
                            ft.chrg.cpMin = a + 1;
                            int r = (int)SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
                            ft.chrg.cpMin = r + 1;
                            if (r < 0)
                                break;
                            CHARRANGE sel;
                            sel.cpMin = a;
                            sel.cpMax = r + 1;
                            SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_EXSETSEL, 0, (LPARAM)&sel);
                            cf.cbSize = 0x3c;
                            cf.dwMask = CFM_COLOR;
                            cf.dwEffects = 0;
                            cf.crTextColor = 0xc800;
                            SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            a = (int)SendMessageA(GetDlgItem(script_dlg, 0x4b0), EM_FINDTEXTEX, FR_DOWN, (LPARAM)&ft);
                        } while (a >= 0);
                    }
                }
                SendMessageA(GetDlgItem(script_dlg, 0x4bb), CB_GETLBTEXT,
                             SendMessageA(GetDlgItem(script_dlg, 0x4bb), CB_GETCURSEL, 0, 0), (LPARAM)buf);
                ((GameConfig*)g_game_G)->script_text.assign(buf);
                ((GameConfig*)g_game_G)->script_index = 0;
                tstring want("\"");
                {
                    tstring name(buf);
                    want.append(name);
                }
                {
                    tstring q("\"");
                    want.append(q);
                }
                int a = ((GameConfig*)g_game_G)->settings_text.find(want, 0);
                if (a > 0) {
                    char lb = '[';
                    int b = ((GameConfig*)g_game_G)->settings_text.find(lb, a) + 1;
                    char rb = ']';
                    int c = ((GameConfig*)g_game_G)->settings_text.find(rb, b);
                    if (b > 0 && c > 0) {
                        HWND list = GetDlgItem(h, 0x46c);
                        SendMessageA(list, CB_RESETCONTENT, 0, 0);
                        tstring body = ((GameConfig*)g_game_G)->settings_text.substr(b, c);
                        int j = 0, k = 0, idx = 0;
                        while (k >= 0) {
                            char lp = '(';
                            j = body.find(lp, j);
                            char rp = ')';
                            k = body.find(rp, j) + 1;
                            if (j < 0)
                                break;
                            if (k >= 0) {
                                LRESULT r = SendMessageA(list, CB_INSERTSTRING, idx, (LPARAM)body.substr(j, k).c_str());
                                SendMessageA(list, CB_SETITEMDATA, r, idx);
                                j = k;
                                idx++;
                            }
                        }
                    }
                }
            }
            break;
        }
        break;
    case WM_SIZING: {
        RECT* r = (RECT*)lParam;
        int left = r->left, top = r->top, right = r->right, bottom = r->bottom;
        RECT br;
        SetWindowPos(GetDlgItem(script_dlg, 0x4b0), 0, left + 10, top + 0x78,
                     (right - left) - 0x1e, (bottom - top) - 0xf5, 0x14);
        GetWindowRect(GetDlgItem(script_dlg, 2), &br);
        SetWindowPos(GetDlgItem(script_dlg, 2), 0, (((right - left) / 2) - br.right) + br.left + left,
                     bottom - 0x73, br.right - br.left, br.bottom - br.top, 0x14);
        return 1;
    }
    }
    return 0;
}

// ---- the module folder dialog (0x4c03b0; the thunk 0x4c3340 jumps to it with ecx = g_app_obj, which it ignores) -------------
// @ 0x4c03b0
INT_PTR CALLBACK folder_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char buf[0x1000];
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: {                          // OK: store the folder
            GetDlgItemTextA(dlg, 0x4b8, buf, 0x1000);
            tstring folder(buf);
            if (folder.buf[folder.len - 1] != '\\')
                folder.push_back('\\');
            HKEY key;
            open_keys_key(&key);
            if (key != (HKEY)0) {
                keys_write_value(&key, "module_system_folder", &folder);
                if (key != (HKEY)0)
                    RegCloseKey(key);
            }
            EndDialog(dlg, 2);
            break;
        }
        case 2:                            // Cancel
            EndDialog(dlg, 2);
            break;
        }
        break;
    case WM_INITDIALOG: {
        tstring folder;
        HKEY key;
        open_keys_key(&key);
        if (key != (HKEY)0) {
            read_key_string(&key, "module_system_folder", &folder);
            if (key != (HKEY)0)
                RegCloseKey(key);
        }
        SetDlgItemTextA(dlg, 0x4b8, folder.c_str());
        return TRUE;
    }
    }
    return FALSE;
}

// @ 0x4c0520
void load_game_variables()
{
    tstring path("Modules\\");
    path.append(g_module_name);
    {
        tstring name("\\game_variables.txt");
        path.append(name);
    }
    FILE* f = fopen(path.c_str(), "r");
    if (f != 0) {
        g_game_vars.load(f);
        fclose(f);
    }
}

// ---- module_info.txt: one directive per entry of the table at 0xa18238 ---------------------------------------------------------
// @ 0x4c05b0
void parse_module_info()
{
    int n = g_module_info.count;
    tstring msg, val;
    g_display_wp[0] = 1;
    g_display_wp[1] = 1;
    g_display_wp[2] = 1;
    g_display_wp[3] = 1;
    g_display_wp[4] = 1;
    g_display_wp[5] = 1;
    g_display_wp[6] = 1;
    for (int i = 0; n > 0 && i < n; i++) {
        if (i >= 0 && i < g_module_info.count) {
            msg = g_module_info.entries[i].key;
            val = g_module_info.entries[i].value;
        }
        if (msg.equals("load_resource")) {
            ((stream_base&)g_log).w_cstr("Loading Resource ").w_tstr(&val).w_cstr("\n");
            g_resmgr.load_pack(val, 0);
        }
        else if (msg.equals("load_mod_resource") || msg.equals("load_module_resource")) {
            ((stream_base&)g_log).w_cstr("Loading Module Resource ").w_tstr(&val).w_cstr("\n");
            g_resmgr.load_pack(val, 1);
        }
        else if (msg.equals("time_multiplier")) {
            g_module_settings.time_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Time Multiplier = ").w_float(&g_module_settings.time_multiplier).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_cut")) {
            g_module_settings.armor_soak_factor[0] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_cut = ").w_float(&g_module_settings.armor_soak_factor[0]).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_pierce")) {
            g_module_settings.armor_soak_factor[1] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_pierce = ").w_float(&g_module_settings.armor_soak_factor[1]).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_blunt")) {
            g_module_settings.armor_soak_factor[2] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_blunt = ").w_float(&g_module_settings.armor_soak_factor[2]).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_cut")) {
            g_module_settings.armor_reduction_factor[0] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_cut = ").w_float(&g_module_settings.armor_reduction_factor[0]).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_pierce")) {
            g_module_settings.armor_reduction_factor[1] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_pierce = ").w_float(&g_module_settings.armor_reduction_factor[1]).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_blunt")) {
            g_module_settings.armor_reduction_factor[2] = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_blunt = ").w_float(&g_module_settings.armor_reduction_factor[2]).w_cstr("\n");
        }
        else if (msg.equals("missile_damage_speed_power")) {
            float x = (float)atof(val.c_str());
            float lo = 2.0f;
            float hi = 1.0f;
            g_module_settings.missile_damage_speed_power = clamp(&x, &hi, &lo);
            ((stream_base&)g_log).w_cstr("Setting missile_damage_speed_power = ").w_float(&g_module_settings.missile_damage_speed_power).w_cstr("\n");
        }
        else if (msg.equals("melee_damage_speed_power")) {
            float x = (float)atof(val.c_str());
            float lo = 2.0f;
            float hi = 1.0f;
            g_module_settings.melee_damage_speed_power = clamp(&x, &hi, &lo);
            ((stream_base&)g_log).w_cstr("Setting melee_damage_speed_power = ").w_float(&g_module_settings.melee_damage_speed_power).w_cstr("\n");
        }
        else if (msg.equals("damage_interrupt_attack_treshold") || msg.equals("damage_interrupt_attack_threshold")) {
            g_module_settings.damage_interrupt_attack_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting damage_interrupt_attack_treshold = ").w_float(&g_module_settings.damage_interrupt_attack_treshold).w_cstr("\n");
        }
        else if (msg.equals("horse_charge_damage_multiplier")) {
            g_module_settings.horse_charge_damage_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting horse_charge_damage_multiplier = ").w_float(&g_module_settings.horse_charge_damage_multiplier).w_cstr("\n");
        }
        else if (msg.equals("couched_lance_damage_multiplier")) {
            g_module_settings.couched_lance_damage_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting couched_lance_damage_multiplier = ").w_float(&g_module_settings.couched_lance_damage_multiplier).w_cstr("\n");
        }
        else if (msg.equals("seeing_range")) {
            g_module_settings.seeing_range = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Seeing Range Multiplier = ").w_float(&g_module_settings.seeing_range).w_cstr("\n");
        }
        else if (msg.equals("track_spotting_multiplier")) {
            g_module_settings.track_spotting = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Track Spotting Multiplier = ").w_float(&g_module_settings.track_spotting).w_cstr("\n");
        }
        else if (msg.equals("blood_multiplier")) {
            g_module_settings.blood_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Blood Multiplier = ").w_float(&g_module_settings.blood_multiplier).w_cstr("\n");
        }
        else if (msg.equals("skill_leadership_bonus")) {
            g_module_settings.skill_leadership_bonus = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Skill Leadership Bonus = ").w_int(&g_module_settings.skill_leadership_bonus).w_cstr("\n");
        }
        else if (msg.equals("skill_prisoner_management_bonus")) {
            g_module_settings.skill_prisoner_management_bonus = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Skill Prisoner Management Bonus = ").w_int(&g_module_settings.skill_prisoner_management_bonus).w_cstr("\n");
        }
        else if (msg.equals("base_companion_limit")) {
            g_module_settings.base_companion_limit = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Base Companion Limit = ").w_int(&g_module_settings.base_companion_limit).w_cstr("\n");
        }
        else if (msg.equals("player_xp_multiplier")) {
            float x = (float)atof(val.c_str());
            float lo = 10.0f;
            float hi = 1.0f;
            g_module_settings.player_xp_multiplier = clamp(&x, &hi, &lo);
            ((stream_base&)g_log).w_cstr("Setting player_xp_multiplier = ").w_float(&g_module_settings.player_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("hero_xp_multiplier")) {
            g_module_settings.hero_xp_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting hero_xp_multiplier = ").w_float(&g_module_settings.hero_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("regulars_xp_multiplier")) {
            g_module_settings.regulars_xp_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting regulars_xp_multiplier = ").w_float(&g_module_settings.regulars_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("hero_wounded_treshold")) {
            g_module_settings.hero_wounded_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting hero_wounded_treshold = ").w_float(&g_module_settings.hero_wounded_treshold).w_cstr("\n");
        }
        else if (msg.equals("player_wounded_treshold")) {
            g_module_settings.player_wounded_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting player_wounded_treshold = ").w_float(&g_module_settings.player_wounded_treshold).w_cstr("\n");
        }
        else if (msg.equals("map_min_x")) {
            g_map_min_x = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Map Min X = ").w_float(&g_map_min_x).w_cstr("\n");
        }
        else if (msg.equals("map_min_y")) {
            g_map_min_y = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Map Min Y = ").w_float(&g_map_min_y).w_cstr("\n");
        }
        else if (msg.equals("map_max_x")) {
            g_map_max_x = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Map Max X = ").w_float(&g_map_max_x).w_cstr("\n");
        }
        else if (msg.equals("map_max_y")) {
            g_map_max_y = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Map Max Y = ").w_float(&g_map_max_y).w_cstr("\n");
        }
        else if (msg.equals("map_sea_direction")) {
            g_module_settings.map_sea_direction = (float)atof(val.c_str());
        }
        else if (msg.equals("map_sea_speed_x")) {
            g_module_settings.map_sea_speed_x = (float)atof(val.c_str());
        }
        else if (msg.equals("map_sea_speed_y")) {
            g_module_settings.map_sea_speed_y = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_direction")) {
            g_module_settings.map_river_direction = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_speed_x")) {
            g_module_settings.map_river_speed_x = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_speed_y")) {
            g_module_settings.map_river_speed_y = (float)atof(val.c_str());
        }
        else if (msg.equals("map_tree_types")) {
            g_module_settings.map_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_snow_tree_types")) {
            g_module_settings.map_snow_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_steppe_tree_types")) {
            g_module_settings.map_steppe_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_desert_tree_types")) {
            g_module_settings.map_desert_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_one_handed")) {
            g_display_wp[0] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_two_handed")) {
            g_display_wp[1] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_polearms")) {
            g_display_wp[2] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_archery")) {
            g_display_wp[3] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_crossbows")) {
            g_display_wp[4] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_throwing")) {
            g_display_wp[5] = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_firearms")) {
            g_display_wp[6] = atoi(val.c_str());
        }
        else if (msg.equals("module_version")) {
            g_module_version = atoi(val.c_str());
        }
        else if (msg.equals("compatible_module_version")) {
            g_compatible_module_version = atoi(val.c_str());
        }
        else if (msg.equals("scan_module_textures")) {
            g_module_settings.scan_module_textures = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Scan Module Textures = ").w_int(&g_module_settings.scan_module_textures).w_cstr("\n");
        }
        else if (msg.equals("scan_module_sounds")) {
            g_module_settings.scan_module_sounds = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Scan Module Sounds = ").w_int(&g_module_settings.scan_module_sounds).w_cstr("\n");
        }
        else if (msg.equals("module_name")) {
            g_module_settings.module_name = val;
            ((stream_base&)g_log).w_cstr("Module_name = ").w_tstr(&g_module_settings.module_name).w_cstr("\n");
        }
        else if (msg.equals("num_hints")) {
            g_module_settings.num_hints = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Num Hints = ").w_int(&g_module_settings.num_hints).w_cstr("\n");
        }
        else if (msg.equals("starting_year")) {
            g_module_settings.starting_year = atoi(val.c_str());
        }
        else if (msg.equals("starting_month")) {
            g_module_settings.starting_month = atoi(val.c_str());
        }
        else if (msg.equals("starting_day")) {
            g_module_settings.starting_day = atoi(val.c_str());
        }
        else if (msg.equals("enable_quick_battles")) {
            g_module_settings.enable_quick_battles = atoi(val.c_str());
        }
        else if (msg.equals("disable_food_slot")) {
            g_module_settings.disable_food_slot = atoi(val.c_str());
        }
        else if (msg.equals("limit_hair_colors")) {
            g_module_settings.limit_hair_colors = atoi(val.c_str());
        }
        else if (msg.equals("show_faction_color")) {
            g_module_settings.show_faction_color = atoi(val.c_str());
        }
        else if (msg.equals("show_quest_notes")) {
            g_module_settings.show_quest_notes = atoi(val.c_str());
        }
        else if (msg.equals("show_party_ids_instead_of_names")) {
            g_module_settings.show_party_ids_instead_of_names = atoi(val.c_str()) != 0;
        }
        else if (msg.equals("main_menu_font_color_red")) {
            g_module_settings.main_menu_font_color_red = atoi(val.c_str());
        }
        else if (msg.equals("main_menu_font_color_green")) {
            g_module_settings.main_menu_font_color_green = atoi(val.c_str());
        }
        else if (msg.equals("main_menu_font_color_blue")) {
            g_module_settings.main_menu_font_color_blue = atoi(val.c_str());
        }
        else if (msg.equals("works_with_version_min")) {
            g_module_settings.works_with_version_min = atoi(val.c_str());
        }
        else if (msg.equals("works_with_version_max")) {
            g_module_settings.works_with_version_max = atoi(val.c_str());
        }
        else if (msg.len > 1)      // (0x4c16d4: cmp [msg.len], 1; not the entry count)
            ((stream_base&)g_log).w_cstr("Unrecognized Module_info directive = ").w_tstr(&msg).w_cstr(". Skipping.\n");
    }
    g_resmgr.linearise_meshes();
}

// ---- shader and device-object handlers (virtual overrides of the application object, vtable 0x5948e0) ------------------
// @ 0x4c17c0
HRESULT build_shader()
{
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    tstring path(g_module_path);
    {
        tstring name("mb.fx");
        path.append(name);
    }
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE)
        FindClose(h);
    else
        path.assign("mb.fxo");
    HRESULT hr = D3DXCreateEffectFromFileA(g_render.dev, path.c_str(), 0, 0, 0x800, 0, &g_effect, 0);
    if (hr >= 0) {
        g_resmgr.reset_effect_sets();
        g_shadow_map.pick_size();
        g_effect->SetFloat("fShadowMapNextPixel", 1.0f / (float)g_shadow_map.w);
        g_effect->SetFloat("fShadowMapSize", (float)g_shadow_map.w);
        float inv = 1.0f / (*(float*)((char*)&g_options + 0x80));
        D3DXVECTOR4 gv, iv;
        gv.x = (*(float*)((char*)&g_options + 0x80)); gv.y = (*(float*)((char*)&g_options + 0x80)); gv.z = (*(float*)((char*)&g_options + 0x80)); gv.w = (*(float*)((char*)&g_options + 0x80));
        iv.x = inv; iv.y = inv; iv.z = inv; iv.w = inv;
        g_effect->SetVector("output_gamma", &gv);
        g_effect->SetVector("output_gamma_inv", &iv);
    }
    return hr;
}

// @ 0x4c1940
HRESULT GameApp::RestoreDeviceObjects()
{
    font->RestoreDeviceObjects();
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    update_view_rect();
    IDirect3DDevice9* dev = m_pd3dDevice;
    g_render.dev = dev;
    dev->GetDeviceCaps(&g_render.caps);
    g_render.vsync = -1;
    g_render.pad504 = 0;
    if (g_use_shaders) {
        if (build_shader() < 0)
            mb_error("Unable to compile the effect file.");
    }
    g_batch.init_shaders();
    g_resmgr.restore(g_effect);
    if (g_render.dev)
        g_render.dev->ShowCursor(((unsigned char)g_gui.show_cursor));
    IDirect3DTexture9* tex;
    {
        tstring name("cursor");
        tex = *g_resmgr.get_texture(name)->d3d_tex;
    }
    IDirect3DSurface9* surf;
    tex->GetSurfaceLevel(0, &surf);
    g_render.dev->SetCursorProperties(1, 1, surf);
    if (surf)
        surf->Release();
    return 0;
}

// @ 0x4c1a60
HRESULT GameApp::InvalidateDeviceObjects()
{
    font->InvalidateDeviceObjects();
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    g_resmgr.reload_textures();
    if (g_shadow_map.surf != 0) {
        g_shadow_map.surf->Release();
        g_shadow_map.surf = 0;
    }
    for (buffer_pair* p = g_batch.vf; (int)p < (int)&g_batch.ib; p++)
        p->release();
    g_batch.ib.release();
    return 0;
}

// @ 0x4c1ad0
HRESULT GameApp::DeleteDeviceObjects()
{
    font->DeleteDeviceObjects();
    g_resmgr.invalidate();
    g_batch.release_decls();
    return 0;
}

// @ 0x4c1b00
HRESULT gfx_restore()
{
    g_app_obj.GameApp::InvalidateDeviceObjects();
    return g_app_obj.GameApp::RestoreDeviceObjects();
}

// ---- allocator objects of the vectors of 0x2c- and 0x204-byte records ----------------------------------------------------------
// @ 0x4c1b20
void Alloc2c::construct(void* dst, const void* src)
{
    if (dst)
        *(blk2c*)dst = *(const blk2c*)src;
}

// @ 0x4c1b60
// (std::uninitialized_copy with the empty iterator-tag class passed as a 5th argument, which is what keeps first/dst in esi/edi; the catch(...) funclet at 0x4c1bcc is the rethrow `push 0; push 0; call _CxxThrowException`)
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* a, ptr_tag tag)
{
    char* s = dst;
    try {
        for (; first != last; dst += sizeof(blk204), ++first) a->construct(dst, first);
    } catch (...) {
        for (; s != dst; s += sizeof(blk204)) ;
        throw;
    }
    return dst;
}

// (Alloc204::construct 0x4c1b40 is in alloc204.cpp: the original compiled it outside this object, so uninit_copy_204 and
// copy_uninit_204 call it without seeing its body)

// ---- the ini table's constructor (two 0x1000-entry tables: 0xa18238 module_info, 0xa44240 game variables) ---------------------
// @ 0x4c1be0
ini_file::ini_file()
{
    count = 0;
}

// the two tables, one definition each (constructed by the compiler: ini_file::ini_file above, ~ini_file 0x4be920 at exit; these replace
// static_init's stubs 0x547da0 / 0x547dc0 and 0x5481a0 / 0x5481d0)
ini_file g_module_info;     // 0xa18238 module.ini
ini_file g_game_vars;       // 0xa44240 game_variables.txt

// ---- trigger and game records -------------------------------------------------------------------------------------------------
struct TimedVal { int id; int pad; __int64 t; void reset() { id = 0; t = g_ticks[0]; } void set(int c) { id = c; t = g_ticks[c]; } };    // a (clock index, reading) pair: trigger_entry +0x20, TriggerRec +0x48 / +0x58 / +0x68

// @ 0x4c1c20
TriggerRec::TriggerRec()
{
    TimedVal* p = (TimedVal*)&f48;
    p[0].reset();
    p[1].reset();
    p[2].reset();
    f0c = 0;
}

static inline void stamp_rec(TriggerRec& r, int c)
{
    ((TimedVal*)&r.f48)->set(c);
    ((TimedVal*)&r.f58)->set(c);
    ((TimedVal*)&r.f68)->set(c);
}

// @ 0x4c1c90
void TriggerVec::resize(stream_base& s, const char* name)
{
    s.r_int(&count);
    if (data != 0) {
        delete[] data;
        data = 0;
    }
    data = new TriggerRec[count];
    for (int i = 0; i < count; i++) {
        stamp_rec(data[i], cur);
        ((trigger_rec*)&data[i])->read(s, name, i);
    }
}

// @ 0x4c1d70
void TriggerVec::stamp(int c)
{
    for (int i = 0; i < count; i++) {
        TriggerRec* r = data + i;
        r->f48 = c; r->f50 = g_ticks[c];
        r->f58 = c; r->f60 = g_ticks[c];
        r->f68 = c; r->f70 = g_ticks[c];
    }
}

// @ 0x4c1de0
trigger_entry::trigger_entry()
{
    TimedVal* tv = (TimedVal*)&clock;
    tv->id = 0;
    tv->t = g_ticks[0];
}

// @ 0x4c1e10
int GameVec::resize(stream_base& s, const char* name, int extra)
{
    s.r_int(&count);
    count += extra;
    if (count > 0) {
        if (data != 0) {
            delete[] data;
            data = 0;
        }
        data = new trigger_entry[count];
    }
    for (int i = 0; i < count; i++) {
        trigger_entry& e = data[i];
        TimedVal* tv = (TimedVal*)&e.clock;
        tv->set(cur);
        if (i < count - extra)
            data[i].read(s, name, i);
    }
    return count - extra;
}

// @ 0x4c1ee0
void GameVec::stamp(int c)
{
    for (int i = 0; i < count; i++) {
        char* b = (char*)data + i * 0x30;
        TimedVal* tv = (TimedVal*)(b + 0x20);
        tv->id = c;
        tv->t = g_ticks[c];
    }
}

// ---- particle-system record table (loaded_psys_table, seen through MouseRec for the inline destructor / constructor) ---------
EmitTable g_emit_defs;                               // 0x8ed5090 (.bss; the compiler registers the destructor with atexit: ~EmitTable 0x4c1f20)

// @ 0x4c1f20
EmitTable::~EmitTable()
{
    MouseRec* q = (MouseRec*)defs;
    if (q != 0)
        delete[] q;
}

// @ 0x4c1f70
void EmitTable::resize(stream_base& s)
{
    s.r_int(&count);
    MouseRec* o = (MouseRec*)defs;
    if (o != 0) {
        delete[] o;
        defs = 0;
    }
    defs = (EmitDef*)new MouseRec[count];
    for (int i = 0; i < count; i++)
        ((psys_record*)defs)[i].load(s);
    load_used();
}

// @ 0x4c20c0
void MapIcon::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)this);
    s.r_int(&flags);
    s.r_tstr(&mesh);
    s.r_float(&f24);
    s.r_int(&f28);
    s.r_float(&f2c);
    s.r_float(&f30);
    s.r_float(&f34);
    _snprintf(buf, 0x200, "Map icon %s", name.c_str());
    triggers.resize(s, buf, 0);
    triggers.stamp(2);
}

// ---- trigger entries ---------------------------------------------------------------------------------------------------------------
// @ 0x4c2170
stream_base& read_entry(stream_base& s, RecH& e)
{
    e.tail = 0x64;
    s.r_tstr((rgl::string*)&e);
    s.r_uint((unsigned*)&e.s.b);
    int& n = e.s.a;
    s.r_int(&n);
    struct Blk40 { char b[0x40]; };
    e.s.ptr = (int*)new Blk40[n];
    for (int i = 0; i < n; i++)
        psys_record::read(s, (char*)&((Blk40*)e.s.ptr)[i]);
    if (e.s.b & 0xff000000)
        e.tail = ((unsigned char*)&e.s.b)[3];
    return s;
}

// @ 0x4c2200
AnimSet g_anim_set;                                     // 0xa76628 (.bss; no constructor: the compiler registers only the destructor)

AnimSet::~AnimSet()
{
    RecH* o = data;
    if (o == 0)
        return;
    int n = ((int*)o)[-1];
    RecH* e = o + n;
    for (int i = n - 1; i >= 0; i--) {
        e--;
        delete[] e->s.ptr;
        e->s.name.~tstring();
        e->name.~tstring();
    }
    delete[] ((char*)o - 4);
}

// @ 0x4c2250
int __cdecl entry_vec_resize(stream_base* sp, AnimSet* self)
{
    stream_base& s = *sp;
    AnimSet* t = self;
    delete[] t->data;    // 0x4315d0 (??_ERecH)
    s.r_int(&t->count);
    t->data = new RecH[t->count];
    for (int i = 0; i < t->count; i++)
        read_entry(s, t->data[i]);
    return (int)&s;
}

// @ 0x4c2300
void MissionTemplate::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)this);
    s.r_tstr(&f10);
    s.r_int(&f20);
    s.r_int(&f3c);
    tstring* disp = &display;
    s.r_tstr(disp);
    s.r_int(&n_spawns);
    spawns = (SpawnRec*)new char[n_spawns * 0x4c];
    for (int i = 0; i < n_spawns; i++)
        spawns[i].load_extras(s);
    _snprintf(buf, 0x200, "Mission Template %s", name.c_str());
    ((TriggerVec*)&f4c)->resize(s, buf);
    ((TriggerVec*)&f4c)->stamp(2);
    disp->underscores_to_spaces();
}

// @ 0x4c23f0
void PresentationRec::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)this);
    s.r_int(&flags);
    s.r_int(&f20);
    _snprintf(buf, 0x200, "Presentation %s", name.c_str());
    triggers.resize(s, buf, 0);
    triggers.stamp(2);
}

// @ 0x4c2470
// (the dialog manager, ../dialog_manager.h DialogMgr; that header cannot be mixed with the stream.h-based ones, so the
//  constructor works on the raw object: 0x400 + 0x400 lines of 0x1c bytes whose text sits at +0xc, cur_text at +0xe028)
DialogMgr::DialogMgr()
{
    recs = 0;
}

// @ 0x4c2b20
void MenuRec::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr(&id);
    s.r_i64(&flags);
    s.r_tstr(&text);
    text.underscores_to_spaces();
    s.r_tstr(&mesh);
    ops.read(&s);
    _snprintf(buf, 0x200, "Game menu %s", id.c_str());
    {
        tstring tmp(buf);
        ops.text = tmp;
    }
    s.r_int(&count);
    if (count != 0)
        items = new menu_item[count];
    for (int i = 0; i < count; i++)
        items[i].read(&s, &id);
}

// ---- the music monitor (global object, 0x2474 bytes: 0x100 track records of 0x20 bytes from +0, play state from +0x2008) --------
// Track i: flags +8 (tested against the mode of the situation), +0xc (category mask), +0x10 / +0x14 (second mask), the
// name tstring at +0x18; start time of the last play at +0x2008 + 4 * i (config.h music_player::play stamps it).
// Two stream slots: w[0..] = (active, ended, stream, track, result) per slot as music_player::play sets them.
#define TRACK(i, off) (*(int*)((char*)this + 0x20 * (i) + (off)))
extern "C" short __stdcall FSOUND_Stream_Close(void* stream);
float frand();                                       // 0x40e400 (rand() % 15817 * (1 / 15817), in st0)

music_player g_music;                                // 0xa73a10 (.bss; constructed by the compiler: music_player::music_player 0x4c24c0)

music_player::~music_player()
{
    ((tstring*)&root)->~tstring();
    for (int i = 0x100; i; i--)
        ((tstring*)((char*)this + 0x18 + (i - 1) * 0x20))->~tstring();
}

// @ 0x4c24c0
// (the original brackets the monitor initialisation with two 18-byte WinLicense markers, at 0x4c2542 and 0x4c25c1)
music_player::music_player()
{
    WL_MARKER(12)
    for (int i = 0; i < 0x100; i++)
        ((tstring*)((char*)this + 0x18 + i * 0x20))->tstring::tstring();
    t2418.clock = 0; t2418.start = g_ticks[0];
    t2450.clock = 0; t2450.start = g_ticks[0];
    t2460.clock = 0; t2460.start = g_ticks[0];
    ((tstring*)&root)->tstring::tstring();
    track_count = 0;
    stream[0] = 0;  active[0] = 0;  channel[0] = -1;  opened[0] = 0;  track[0] = -1;  volume[0] = 1000;  fading[0] = 0;
    stream[1] = 0;  active[1] = 0;  channel[1] = -1;  opened[1] = 0;  track[1] = -1;  volume[1] = 1000;  fading[1] = 0;
    situation = 0;
    mask = 0;
    new_piece = 0;
    WL_MARKER(13)
}

// @ 0x4c25e0
// Slot 0 plays the current piece, slot 1 the one being faded in / out. Each slot first waits for its stream to open,
// then watches it until its end; with no piece in slot 0 the track is chosen: a script request wins, else the tracks
// matching the situation `mode` (and the category `mask`) compete by how long ago they were played.
void music_player::advance_piece()
{
    char buf[0x200];
    int& mode = situation;
    int& count = track_count;
    if (active[0] != 0 && opened[0] == 0) {
        int h = FSOUND_Stream_GetOpenState(stream[0]);
        if (h == 0) {
            opened[0] = 1;
            channel[0] = FSOUND_Stream_Play(-1, stream[0]);
        } else if (h != -2) {
            tstring* nm = (tstring*)((char*)this + 0x18 + 0x20 * track[0]);
            sprintf(buf, "Error: Unable to load %s", nm->c_str());
            ((stream_base&)g_log).w_cstr(buf);
            active[0] = 0;
        }
    } else if (active[0] != 0) {
        int len = FSOUND_Stream_GetLengthMs(stream[0]);
        int pos = FSOUND_Stream_GetTime(stream[0]);
        int a = g_script.next_track;
        int b = g_script.next_track_options;
        if (a >= 0 && a == track[0] && b > 0) {
            a = -1;
            g_script.next_track = a;
        }
        bool done = true;
        if (pos < len) {
            int rec = 0x20 * track[0];
            int fl = *(int*)((char*)this + rec + 8) & 0x100;
            if (fl != 0 || a < 0 || b != 2) {
                done = false;
                if (fading[0] == 0 && fl == 0) {
                    if (mode == 0 || (*(int*)((char*)this + rec + 0xc) & mode) == 0
                        || (mask != 0 && (*(int*)((char*)this + rec + 0x14) & mask) == 0)
                        || (a >= 0 && b == 1)) {
                        t2450.start = g_ticks[t2450.clock];
                        fading[0] = 1;
                        new_piece = 1;
                    }
                }
            }
        }
        if (done) {
            FSOUND_Stream_Close(stream[0]);
            stream[0] = 0;
            active[0] = 0;
            channel[0] = -1;
        }
    } else if (g_options.music_volume > 0.02f) {
        bool forced = false;
        int sel = -1;
        int best = -1;
        int a = g_script.next_track;
        if (a >= 0) {
            g_script.next_track = -1;
            forced = true;
            if (a != -1)
                sel = a;
        }
        if (sel < 0 && mode != 0) {
            int cur = track[0];
            if (cur >= 0 && (mode & *(int*)((char*)this + 0x20 * cur + 0xc)) != 0
                && (mask == 0 || (mask & *(int*)((char*)this + 0x20 * cur + 0x14)) != 0))
                best = cur;
            float sum = 0.0f;
            float now = (float)g_ticks[0] * 1e-5f + 1.0f;
            const float* times = start;
            int i = 0;
            if (count > 0) do {
                if (i != best && mode != 0 && (TRACK(i, 8) & mode) != 0 && (mask == 0 || (TRACK(i, 0x10) & mask) != 0))
                {
                    sum += now - times[i];
                    sum += 500.0f;
                }
            } while (++i < count);
            // 0x4c2888 fcomp 0; test ah,0x41; jne: only an ordered sum > 0 draws (a NaN sum keeps `best`)
            if (sum > 0.0f) {
                int pick = (int)((float)(rand() % 0x3dc9) * sum * 6.3223117e-05f);
                i = 0;
                if (count > 0) do {
                    if (i != best && mode != 0 && (TRACK(i, 8) & mode) != 0 && (mask == 0 || (TRACK(i, 0x10) & mask) != 0)) {
                        pick = (int)((float)pick - ((now - times[i]) + 500.0f));
                        if ((float)pick < 0.0f) {
                            sel = i;
                            break;
                        }
                    }
                } while (++i < count);
            } else {
                sel = best;
            }
        }
        if (sel >= 0) {
            if (forced || new_piece != 0 || (TRACK(sel, 8) & 0x80) != 0
                || frand() < 0.01f) {   // 0x4c29dc: frand() (0x40e400), compared with 0.01f (0x54a6f0)
                int slot = 0;
                new_piece = 0;
                play(sel, &slot);
                if ((TRACK(track[0], 8) & 0x100) != 0)
                    mode = 0;
            }
        }
    }
    // slot 1: the piece being faded
    if (active[1] != 0 && opened[1] == 0) {
        int h = FSOUND_Stream_GetOpenState(stream[1]);
        if (h == 0) {
            opened[1] = 1;
            channel[1] = FSOUND_Stream_Play(-1, stream[1]);
            t2460.start = g_ticks[t2460.clock];
        } else if (h != -2) {
            tstring* nm = (tstring*)((char*)this + 0x18 + 0x20 * track[1]);
            sprintf(buf, "Error: Unable to load %s", nm->c_str());
            ((stream_base&)g_log).w_cstr(buf);
            active[1] = 0;
        }
    } else if (active[1] != 0) {
        int len = FSOUND_Stream_GetLengthMs(stream[1]);
        if (FSOUND_Stream_GetTime(stream[1]) < len)
            return;
        FSOUND_Stream_Close(stream[1]);
        stream[1] = 0;
        active[1] = 0;
        channel[1] = -1;
    } else if (g_script.cue_track >= 0) {
        if (g_options.music_volume > 0.02f) {
            int slot = 1;
            play(g_script.cue_track, &slot);
        }
        g_script.cue_track = -1;
    }
}
