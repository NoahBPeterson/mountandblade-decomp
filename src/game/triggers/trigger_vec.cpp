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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <io.h>
#include <new>
#include "./trigger_vec.h"
#include <richedit.h>
#include "../config/core.h"
#include "../config/config.h"
#include "../party/game.h"
#include "../../rgl/xmlcfg/records.h"     // (listed directly: the resolution audit scans only the source's own includes)
#include "../meta_mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

void __fastcall tstring_ctor(tstring* p);                   // 0x42bbd0 (tstring::tstring, called on raw slots)
void __fastcall tstring_dtor(tstring* p);                   // 0x42bc40
void __cdecl free_mem(void* p);                             // 0x53570d (operator delete[])

// @ 0x4be742
void MetaMission::init()
{
    f014 = 0;
    f144 = 0;
    f138 = 0;
    f010 = 0;
    f008 = -1;
    f000 = -1;
    f004 = -1;
    f13c = -1;
    f140 = -1;
    f024 = 0;
    f01c = 0;
    f00c = -1;
    f154 = -1;
    f158 = -1;
    f15c = -1;
    f03c = 0x400;
    f168 = 0;
    f2c0 = 0;
    f2c4 = 0;
    f2c8 = 0;
    f040 = 0;
}

// @ 0x4be7c0
void code_table::init()
{
    memset(v, -1, sizeof(v));
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
// @ 0x4bed90
void __fastcall core_dtor(Core* self)
{
    CD3DFont* font = self->font;
    *(void**)self = (void*)0x5948e0;
    if (font != 0) {
        font->~CD3DFont();
        operator delete(font);
        self->font = 0;
    }
    {
        tstring* p = (tstring*)(self->module_names + sizeof(self->module_names));
        int n = 0x20;
        do {
            p--;
            p->~tstring();
        } while (--n);
    }
    *(void**)self = (void*)0x549c60;
    ((CD3DEnumeration*)((char*)self + 0xc))->~CD3DEnumeration();
}

// @ 0x4bedf0
void* __fastcall core_deleting_dtor(Core* self, int, unsigned flags)
{
    core_dtor(self);
    if ((flags & 1) != 0)
        ::operator delete(self);
    return self;
}

// ---- options dialog: write the stored values into the controls (0x4bee10), read them back (0x4bf170) -------------------
// @ 0x4bee10
void Core::show_options()
{
    char buf[0x200];

    SendMessageA(GetDlgItem(options_hwnd, 0x43e), 0x405, 1, g_opt_5aa0ac);
    sprintf(buf, "%d", g_opt_5aa0ac);
    SetWindowTextA(GetDlgItem(options_hwnd, 0x450), buf);
    SendMessageA(GetDlgItem(options_hwnd, 0x441), BM_SETCHECK, g_opt_5aa0e0 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x444), BM_SETCHECK, g_opt_5aa0b8 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x445), BM_SETCHECK, g_opt_5aa0b0 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x487), BM_SETCHECK, g_opt_5aa108 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x452), BM_SETCHECK, g_opt_5aa0e8 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x446), BM_SETCHECK, g_opt_5aa0e4 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44d), BM_SETCHECK, g_opt_5aa0d8 == 0, 0);
    SetDlgItemInt(options_hwnd, 0x448, g_opt_5aa0b4, FALSE);
    SendMessageA(GetDlgItem(options_hwnd, 0x442), BM_SETCHECK, g_opt_5aa0cc != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x443), BM_SETCHECK, g_opt_5aa0d0 == 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44c), BM_SETCHECK, g_opt_5aa0c0 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x44a), BM_SETCHECK, g_opt_5aa0c4 != 0, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x467), 0x14e, g_opt_5aa110, 0);
    SendMessageA(GetDlgItem(options_hwnd, 0x468), 0x14e, g_opt_5aa0dc, 0);
    if (g_opt_5aa0f4 == 0) {
        SendMessageA(GetDlgItem(options_hwnd, 0x45f), 0x14e, 0, 0);
    } else {
        char num[0x80];
        tstring s(_itoa(g_opt_5aa0f4, num, 10));
        s.push_back('x');
        {
            tstring h(_itoa(g_opt_5aa0f8, num, 10));
            s.append(h);
        }
        {
            tstring b(g_opt_5aa0fc == 0x10 ? " 16 bits" : " 32 bits");
            s.append(b);
        }
        SendMessageA(GetDlgItem(options_hwnd, 0x45f), 0x14d, 0,
                     (LPARAM)(s.buf ? s.buf : (const char*)&s.buf));
    }
}

// @ 0x4bf170
void Core::read_options()
{
    g_opt_5aa0ac = SendMessageA(GetDlgItem(options_hwnd, 0x43e), WM_USER, 0, 0);
    g_opt_5aa0e0 = SendMessageA(GetDlgItem(options_hwnd, 0x441), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0dc = SendMessageA(GetDlgItem(options_hwnd, 0x468), CB_GETCURSEL, 0, 0) == 1;
    g_opt_5aa0cc = SendMessageA(GetDlgItem(options_hwnd, 0x442), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0d0 = SendMessageA(GetDlgItem(options_hwnd, 0x443), BM_GETCHECK, 0, 0) != 1;
    g_opt_5aa0b8 = SendMessageA(GetDlgItem(options_hwnd, 0x444), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0b0 = SendMessageA(GetDlgItem(options_hwnd, 0x445), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa108 = SendMessageA(GetDlgItem(options_hwnd, 0x487), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0e8 = SendMessageA(GetDlgItem(options_hwnd, 0x452), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0e4 = SendMessageA(GetDlgItem(options_hwnd, 0x446), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0d8 = SendMessageA(GetDlgItem(options_hwnd, 0x44d), BM_GETCHECK, 0, 0) != 1;
    {   // "fps limit" edit box, clamped to 10..999
        BOOL ok;
        UINT v = GetDlgItemInt(options_hwnd, 0x448, &ok, FALSE);
        if (ok != 0) {
            int iv = (int)v;
            if (iv < 10)
                g_opt_5aa0b4 = 10;
            else if (iv >= 1000)
                g_opt_5aa0b4 = 999;
            else
                g_opt_5aa0b4 = iv;
        }
    }
    g_opt_5aa0c0 = SendMessageA(GetDlgItem(options_hwnd, 0x44c), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0c4 = SendMessageA(GetDlgItem(options_hwnd, 0x44a), BM_GETCHECK, 0, 0) == 1;
    if (SendMessageA(GetDlgItem(options_hwnd, 0x45f), CB_GETCURSEL, 0, 0) == 0) {
        g_opt_5aa0f4 = 0;
        g_opt_5aa0f8 = 0;
        int md = mode.Format;
        if (md == 0x17 || md == 0x19 || md == 0x1a || md == 0x18 || md == 0x1e)
            g_opt_5aa0fc = 0x10;
        else
            g_opt_5aa0fc = 0x20;
    } else {
        // the edit box holds "<width>x<height> <bits>"
        char buf[100];
        GetWindowTextA(GetDlgItem(options_hwnd, 0x45f), buf, 100);
        tstring s(buf);
        char cx = 'x';
        int ix = s.find(cx, 0);
        char cs = ' ';
        int isp = s.find(cs, 0);
        g_opt_5aa0f4 = atoi(s.substr(0, ix).c_str());
        g_opt_5aa0f8 = atoi(s.substr(ix + 1, isp).c_str());
        g_opt_5aa0fc = atoi(s.substr(isp + 1, isp + 3).c_str());
    }
    g_opt_5aa100 = SendMessageA(GetDlgItem(options_hwnd, 0x465), CB_GETCURSEL, 0, 0);
    g_opt_5aa100 = SendMessageA(GetDlgItem(options_hwnd, 0x465), CB_GETITEMDATA, g_opt_5aa100, 0);
    g_opt_5aa110 = SendMessageA(GetDlgItem(options_hwnd, 0x467), CB_GETITEMDATA,
                               SendMessageA(GetDlgItem(options_hwnd, 0x467), CB_GETCURSEL, 0, 0), 0);
    int m;
    if (g_opt_5aa0fc == 0x10)
        m = 0x17;
    else if (g_opt_5aa0fc == 0x20)
        m = 0x16;
    else
        m = mode.Format;
    if (g_opt_5aa100 > 1) {
        DWORD q;
        if (m_pD3D->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, (D3DFORMAT)m, FALSE,
                                               g_opt_5aa100 != 2 ? D3DMULTISAMPLE_4_SAMPLES : D3DMULTISAMPLE_2_SAMPLES, &q) >= 0) {
            g_opt_5aa104 = q - 1;
            return;
        }
    }
    g_opt_5aa104 = 0;
}

// @ 0x4bf5f0
INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        g_dlg_options = dlg;
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
            EndDialog((HWND)g_dlg_options, 1);
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
    SendMessageA(GetDlgItem(hDlg, 0x470), BM_SETSTATE, g_config->game_mode == 1, 0);
    SendMessageA(GetDlgItem(hDlg, 0x46d), BM_SETSTATE, g_config->game_mode == 2, 0);
    SendMessageA(GetDlgItem(hDlg, 0x471), BM_SETSTATE, g_config->game_mode == 3, 0);
    SendMessageA(GetDlgItem(hDlg, 0x49a), BM_SETSTATE, g_config->game_mode == 4, 0);
    SendMessageA(GetDlgItem(hDlg, 0x46f), BM_SETSTATE, g_config->game_mode == 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x434), BM_SETCHECK, g_config->check_a098 != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x453), BM_SETCHECK, g_config->check_a0cc != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x454), BM_SETCHECK, g_config->check_a0d0 != 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x497), BM_SETSTATE, g_config->game_mode == 4 && g_config->sub_option == 0, 0);
    SendMessageA(GetDlgItem(hDlg, 0x498), BM_SETSTATE, g_config->game_mode == 4 && g_config->sub_option == 1, 0);
    SendMessageA(GetDlgItem(hDlg, 0x499), BM_SETSTATE, g_config->game_mode == 4 && g_config->sub_option == 2, 0);
    refresh();
}

// ---- the script debugger dialog (a method of the application object: 0x4c3330 jumps here with ecx = g_app_obj) ----------------
// WM_INITDIALOG lays the controls out and fills the script combo from ID_scripts.py; WM_COMMAND handles the Stop / Debug
// toggle (0x4b3), the reset button (2), the index combo (0x46c) and the script combo (0x4bb, which loads module_scripts.py
// without its "#" comment lines into the viewer and colours the quoted strings); WM_SIZING keeps the layout.
// @ 0x4bf870
INT_PTR Core::script_debugger_proc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
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
            g_config->debug_file = (int)f;
            sprintf(msgbuf,
                "File %s could not be found. Please set the correct module system folder by choosing \"Set Debug Mode Folder\" on the menu.", path.c_str());
            MessageBoxA(0, msgbuf, "ERROR", 0);
        } else {
            int n = (int)file_length(f);
            tstring text;
            text.read_from(f, n);
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
                g_config->script_text.assign(buf);
                LRESULT ix = SendMessageA(GetDlgItem(script_dlg, 0x46c), CB_GETCURSEL, 0, 0);
                if (ix >= 0)
                    g_config->script_index = SendMessageA(GetDlgItem(script_dlg, 0x46c), CB_GETITEMDATA, ix, 0);
                else
                    g_config->script_index = 0;
                return 0;
            }
            break;
        case 2:
            g_config->debug_file = 0;
            g_config->settings_text.assign("");
            return 0;
        case 0x4b3:
            g_config->paused = 1 - g_config->paused;
            if (g_config->paused)
                SetDlgItemTextA(script_dlg, 0x4b3, "Stop");
            else
                SetDlgItemTextA(script_dlg, 0x4b3, "Debug");
            return 0;
        case 0x4bb:
            if (HIWORD(wParam) == 9) {
                if (g_config->settings_text.len == 0) {
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
                        g_config->debug_file = (int)f;
                        sprintf(msgbuf,
                            "File %s could not be found. Please set the correct module system folder by choosing \"Set Debug Mode Folder\" on the menu.", path.c_str());
                        MessageBoxA(0, msgbuf, "ERROR", 0);
                        return 1;
                    }
                    int n = (int)file_length(f);
                    tstring text;
                    text.read_from(f, n);
                    fclose(f);
                    // strip the comment lines ("#" up to the next line feed), last to first
                    int end = n - 1;
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
                    g_config->settings_text = text;
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
                g_config->script_text.assign(buf);
                g_config->script_index = 0;
                tstring want("\"");
                {
                    tstring name(buf);
                    want.append(name);
                }
                {
                    tstring q("\"");
                    want.append(q);
                }
                int a = g_config->settings_text.find(want, 0);
                if (a > 0) {
                    char lb = '[';
                    int b = g_config->settings_text.find(lb, a) + 1;
                    char rb = ']';
                    int c = g_config->settings_text.find(rb, b);
                    if (b > 0 && c > 0) {
                        HWND list = GetDlgItem(h, 0x46c);
                        SendMessageA(list, CB_RESETCONTENT, 0, 0);
                        tstring body = g_config->settings_text.substr(b, c);
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
    g_display_wp_one_handed = 1;
    g_display_wp_two_handed = 1;
    g_display_wp_polearms = 1;
    g_display_wp_archery = 1;
    g_display_wp_crossbows = 1;
    g_display_wp_throwing = 1;
    g_display_wp_firearms = 1;
    for (int i = 0; n > 0 && i < n; i++) {
        if (i >= 0 && i < g_module_info.count) {
            msg = g_module_info.entries[i].key;
            val = g_module_info.entries[i].value;
        }
        if (msg.equals("load_resource")) {
            ((stream_base&)g_log).w_cstr("Loading Resource ").w_tstr((rgl::string*)&val).w_cstr("\n");
            g_resmgr_x.load_pack(val, 0);
        }
        else if (msg.equals("load_mod_resource") || msg.equals("load_module_resource")) {
            ((stream_base&)g_log).w_cstr("Loading Module Resource ").w_tstr((rgl::string*)&val).w_cstr("\n");
            g_resmgr_x.load_pack(val, 1);
        }
        else if (msg.equals("time_multiplier")) {
            g_time_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Time Multiplier = ").w_float(&g_time_multiplier).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_cut")) {
            g_armor_soak_factor_against_cut = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_cut = ").w_float(&g_armor_soak_factor_against_cut).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_pierce")) {
            g_armor_soak_factor_against_pierce = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_pierce = ").w_float(&g_armor_soak_factor_against_pierce).w_cstr("\n");
        }
        else if (msg.equals("armor_soak_factor_against_blunt")) {
            g_armor_soak_factor_against_blunt = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_soak_factor_against_blunt = ").w_float(&g_armor_soak_factor_against_blunt).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_cut")) {
            g_armor_reduction_factor_against_cut = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_cut = ").w_float(&g_armor_reduction_factor_against_cut).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_pierce")) {
            g_armor_reduction_factor_against_pierce = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_pierce = ").w_float(&g_armor_reduction_factor_against_pierce).w_cstr("\n");
        }
        else if (msg.equals("armor_reduction_factor_against_blunt")) {
            g_armor_reduction_factor_against_blunt = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting armor_reduction_factor_against_blunt = ").w_float(&g_armor_reduction_factor_against_blunt).w_cstr("\n");
        }
        else if (msg.equals("missile_damage_speed_power")) {
            float x = (float)atof(val.c_str());
            float lo = 2.0f;
            float hi = 1.0f;
            g_missile_damage_speed_power = clamp_float3(x, hi, lo);
            ((stream_base&)g_log).w_cstr("Setting missile_damage_speed_power = ").w_float(&g_missile_damage_speed_power).w_cstr("\n");
        }
        else if (msg.equals("melee_damage_speed_power")) {
            float x = (float)atof(val.c_str());
            float lo = 2.0f;
            float hi = 1.0f;
            g_melee_damage_speed_power = clamp_float3(x, hi, lo);
            ((stream_base&)g_log).w_cstr("Setting melee_damage_speed_power = ").w_float(&g_melee_damage_speed_power).w_cstr("\n");
        }
        else if (msg.equals("damage_interrupt_attack_treshold") || msg.equals("damage_interrupt_attack_threshold")) {
            g_damage_interrupt_attack_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting damage_interrupt_attack_treshold = ").w_float(&g_damage_interrupt_attack_treshold).w_cstr("\n");
        }
        else if (msg.equals("horse_charge_damage_multiplier")) {
            g_horse_charge_damage_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting horse_charge_damage_multiplier = ").w_float(&g_horse_charge_damage_multiplier).w_cstr("\n");
        }
        else if (msg.equals("couched_lance_damage_multiplier")) {
            g_couched_lance_damage_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting couched_lance_damage_multiplier = ").w_float(&g_couched_lance_damage_multiplier).w_cstr("\n");
        }
        else if (msg.equals("seeing_range")) {
            g_seeing_range = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Seeing Range Multiplier = ").w_float(&g_seeing_range).w_cstr("\n");
        }
        else if (msg.equals("track_spotting_multiplier")) {
            g_track_spotting = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Track Spotting Multiplier = ").w_float(&g_track_spotting).w_cstr("\n");
        }
        else if (msg.equals("blood_multiplier")) {
            g_blood_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Blood Multiplier = ").w_float(&g_blood_multiplier).w_cstr("\n");
        }
        else if (msg.equals("skill_leadership_bonus")) {
            g_skill_leadership_bonus = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Skill Leadership Bonus = ").w_int(&g_skill_leadership_bonus).w_cstr("\n");
        }
        else if (msg.equals("skill_prisoner_management_bonus")) {
            g_skill_prisoner_management_bonus = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Skill Prisoner Management Bonus = ").w_int(&g_skill_prisoner_management_bonus).w_cstr("\n");
        }
        else if (msg.equals("base_companion_limit")) {
            g_base_companion_limit = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting Base Companion Limit = ").w_int(&g_base_companion_limit).w_cstr("\n");
        }
        else if (msg.equals("player_xp_multiplier")) {
            float x = (float)atof(val.c_str());
            float lo = 10.0f;
            float hi = 1.0f;
            g_player_xp_multiplier = clamp_float3(x, hi, lo);
            ((stream_base&)g_log).w_cstr("Setting player_xp_multiplier = ").w_float(&g_player_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("hero_xp_multiplier")) {
            g_hero_xp_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting hero_xp_multiplier = ").w_float(&g_hero_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("regulars_xp_multiplier")) {
            g_regulars_xp_multiplier = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting regulars_xp_multiplier = ").w_float(&g_regulars_xp_multiplier).w_cstr("\n");
        }
        else if (msg.equals("hero_wounded_treshold")) {
            g_hero_wounded_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting hero_wounded_treshold = ").w_float(&g_hero_wounded_treshold).w_cstr("\n");
        }
        else if (msg.equals("player_wounded_treshold")) {
            g_player_wounded_treshold = (float)atof(val.c_str());
            ((stream_base&)g_log).w_cstr("Setting player_wounded_treshold = ").w_float(&g_player_wounded_treshold).w_cstr("\n");
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
            g_map_sea_direction = (float)atof(val.c_str());
        }
        else if (msg.equals("map_sea_speed_x")) {
            g_map_sea_speed_x = (float)atof(val.c_str());
        }
        else if (msg.equals("map_sea_speed_y")) {
            g_map_sea_speed_y = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_direction")) {
            g_map_river_direction = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_speed_x")) {
            g_map_river_speed_x = (float)atof(val.c_str());
        }
        else if (msg.equals("map_river_speed_y")) {
            g_map_river_speed_y = (float)atof(val.c_str());
        }
        else if (msg.equals("map_tree_types")) {
            g_map_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_snow_tree_types")) {
            g_map_snow_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_steppe_tree_types")) {
            g_map_steppe_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("map_desert_tree_types")) {
            g_map_desert_tree_types = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_one_handed")) {
            g_display_wp_one_handed = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_two_handed")) {
            g_display_wp_two_handed = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_polearms")) {
            g_display_wp_polearms = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_archery")) {
            g_display_wp_archery = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_crossbows")) {
            g_display_wp_crossbows = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_throwing")) {
            g_display_wp_throwing = atoi(val.c_str());
        }
        else if (msg.equals("display_wp_firearms")) {
            g_display_wp_firearms = atoi(val.c_str());
        }
        else if (msg.equals("module_version")) {
            g_module_version = atoi(val.c_str());
        }
        else if (msg.equals("compatible_module_version")) {
            g_compatible_module_version = atoi(val.c_str());
        }
        else if (msg.equals("scan_module_textures")) {
            g_scan_module_textures = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Scan Module Textures = ").w_int(&g_scan_module_textures).w_cstr("\n");
        }
        else if (msg.equals("scan_module_sounds")) {
            g_scan_module_sounds = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Scan Module Sounds = ").w_int(&g_scan_module_sounds).w_cstr("\n");
        }
        else if (msg.equals("module_name")) {
            g_module_info_name = val;
            ((stream_base&)g_log).w_cstr("Module_name = ").w_tstr((rgl::string*)&g_module_info_name).w_cstr("\n");
        }
        else if (msg.equals("num_hints")) {
            g_num_hints = atoi(val.c_str());
            ((stream_base&)g_log).w_cstr("Num Hints = ").w_int(&g_num_hints).w_cstr("\n");
        }
        else if (msg.equals("starting_year")) {
            g_starting_year = atoi(val.c_str());
        }
        else if (msg.equals("starting_month")) {
            g_starting_month = atoi(val.c_str());
        }
        else if (msg.equals("starting_day")) {
            g_starting_day = atoi(val.c_str());
        }
        else if (msg.equals("enable_quick_battles")) {
            g_xa181a4 = atoi(val.c_str());
        }
        else if (msg.equals("disable_food_slot")) {
            g_xa181a8 = atoi(val.c_str());
        }
        else if (msg.equals("limit_hair_colors")) {
            g_xa181ac = atoi(val.c_str());
        }
        else if (msg.equals("show_faction_color")) {
            g_xa181b0 = atoi(val.c_str());
        }
        else if (msg.equals("show_quest_notes")) {
            g_xa181b4 = atoi(val.c_str());
        }
        else if (msg.equals("show_party_ids_instead_of_names")) {
            g_xa181b8 = atoi(val.c_str()) != 0;
        }
        else if (msg.equals("main_menu_font_color_red")) {
            g_main_menu_font_color_red = atoi(val.c_str());
        }
        else if (msg.equals("main_menu_font_color_green")) {
            g_main_menu_font_color_green = atoi(val.c_str());
        }
        else if (msg.equals("main_menu_font_color_blue")) {
            g_main_menu_font_color_blue = atoi(val.c_str());
        }
        else if (msg.equals("works_with_version_min")) {
            g_works_with_version_min = atoi(val.c_str());
        }
        else if (msg.equals("works_with_version_max")) {
            g_works_with_version_max = atoi(val.c_str());
        }
        else if (n > 1)
            ((stream_base&)g_log).w_cstr("Unrecognized Module_info directive = ").w_tstr((rgl::string*)&msg).w_cstr(". Skipping.\n");
    }
    g_resmgr_x.linearise_meshes();
}

// ---- shader and device-object handlers (virtual overrides of the application object, vtable 0x5948e0) ------------------
// @ 0x4c17c0
HRESULT build_shader()
{
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    tstring path(g_path_prefix);
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
    HRESULT hr = D3DXCreateEffectFromFileA(g_d3d_device, path.c_str(), 0, 0, 0x800, 0, &g_effect, 0);
    if (hr >= 0) {
        g_resmgr_x.reset_effect_sets();
        g_shadow_map.pick_size();
        g_effect->SetFloat("fShadowMapNextPixel", 1.0f / (float)g_shadow_map.w);
        g_effect->SetFloat("fShadowMapSize", (float)g_shadow_map.w);
        float inv = 1.0f / g_gamma;
        D3DXVECTOR4 gv, iv;
        gv.x = g_gamma; gv.y = g_gamma; gv.z = g_gamma; gv.w = g_gamma;
        iv.x = inv; iv.y = inv; iv.z = inv; iv.w = inv;
        g_effect->SetVector("output_gamma", &gv);
        g_effect->SetVector("output_gamma_inv", &iv);
    }
    return hr;
}

// @ 0x4c1940
HRESULT Core::RestoreDeviceObjects()
{
    font->RestoreDeviceObjects();
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    update_view_rect();
    IDirect3DDevice9* dev = m_pd3dDevice;
    g_d3d_device = dev;
    dev->GetDeviceCaps(&g_caps);
    g_renderer_vsync = -1;
    g_renderer_flag = 0;
    if (g_use_shaders) {
        if (build_shader() < 0)
            mb_error("Unable to compile the effect file.");
    }
    g_batch.init_shaders();
    g_resmgr_x.restore(g_effect);
    if (g_d3d_device)
        g_d3d_device->ShowCursor(g_cursor_flag);
    IDirect3DTexture9* tex;
    {
        tstring name("cursor");
        tex = *g_resmgr_x.get_texture(name)->d3d_tex;
    }
    IDirect3DSurface9* surf;
    tex->GetSurfaceLevel(0, &surf);
    g_d3d_device->SetCursorProperties(1, 1, surf);
    if (surf)
        surf->Release();
    return 0;
}

// @ 0x4c1a60
HRESULT Core::InvalidateDeviceObjects()
{
    font->InvalidateDeviceObjects();
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    g_resmgr_x.reload_textures();
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
HRESULT Core::DeleteDeviceObjects()
{
    font->DeleteDeviceObjects();
    g_resmgr_x.invalidate();
    g_batch.release_decls();
    return 0;
}

// @ 0x4c1b00
HRESULT gfx_restore()
{
    g_app_obj.Core::InvalidateDeviceObjects();
    return g_app_obj.Core::RestoreDeviceObjects();
}

// ---- allocator objects of the vectors of 0x2c- and 0x204-byte records ----------------------------------------------------------
// @ 0x4c1b20
void Alloc2c::construct(void* dst, const void* src)
{
    if (dst)
        *(blk2c*)dst = *(const blk2c*)src;
}

// @ 0x4c1b40
void Alloc204::construct(void* dst, const void* src)
{
    if (dst)
        *(blk204*)dst = *(const blk204*)src;
}

// @ 0x4c1b60
// (std::uninitialized_copy; the catch(...) funclet at 0x4c1bcc is the rethrow `push 0; push 0; call _CxxThrowException`)
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* a)
{
    try {
        for (; first != last; first++, dst += sizeof(blk204))
            a->construct(dst, first);
    } catch (...) {
        throw;
    }
    return dst;
}

// ---- the ini table's constructor (two 0x1000-entry tables: 0xa18238 module_info, 0xa44240 game variables) ---------------------
// @ 0x4c1be0
ini_file::ini_file()
{
    count = 0;
}

// ---- trigger and game records -------------------------------------------------------------------------------------------------
struct TimedVal { int id; int pad; __int64 t; };    // a (clock index, reading) pair: trigger_entry +0x20, TriggerRec +0x48 / +0x58 / +0x68

// @ 0x4c1c20
TriggerRec::TriggerRec()
{
    TimedVal* p = (TimedVal*)&f48;
    for (int i = 0; i < 3; i++) {
        p->id = 0;
        p->t = g_ticks[0];
        p++;
    }
    f0c = 0;
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
        int c = cur;
        TriggerRec* r = data + i;
        r->f48 = c; r->f50 = g_ticks[c];
        r->f58 = c; r->f60 = g_ticks[c];
        r->f68 = c; r->f70 = g_ticks[c];
        ((trigger_rec*)r)->read(s, name, i);
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
        int c = cur;
        TimedVal* tv = (TimedVal*)(i * 0x30 + (int)data + 0x20);
        tv->id = c;
        tv->t = g_ticks[c];
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
// @ 0x4c1f20
void loaded_psys_table::clear()
{
    MouseRec* q = (MouseRec*)records;
    if (q != 0)
        delete[] q;
}

// @ 0x4c1f70
void loaded_psys_table::resize(stream_base& s)
{
    s.r_int(&count);
    MouseRec* o = (MouseRec*)records;
    if (o != 0) {
        delete[] o;
        records = 0;
    }
    records = (psys_record*)new MouseRec[count];
    for (int i = 0; i < count; i++)
        records[i].load(s);
    load_used();
}

// @ 0x4c20c0
void MapIcon::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)this);
    s.r_int(&flags);
    s.r_tstr((rgl::string*)&mesh);
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
    s.r_int(&e.s.a);
    e.s.ptr = (int*)new char[e.s.a << 6];
    for (int i = 0; i < e.s.a; i++)
        read_psys_rec(&s, (char*)e.s.ptr + i * 0x40);
    if (e.s.b & 0xff000000)
        e.tail = (unsigned char)((unsigned)e.s.b >> 24);
    return s;
}

// @ 0x4c2200
void TriggerEntryVec::dtor()
{
    RecH* o = data;
    if (o == 0)
        return;
    char* base = (char*)o - 4;
    int n = *(int*)base;
    RecH* e = o + n;
    for (int i = n - 1; i >= 0; i--) {
        e--;
        delete[] e->s.ptr;
        tstring_dtor(&e->s.name);
        tstring_dtor(&e->name);
    }
    delete[] base;
}

// @ 0x4c2250
int __cdecl entry_vec_resize(stream_base* sp, TriggerEntryVec* self)
{
    stream_base& s = *sp;
    TriggerEntryVec* t = self;
    if (t->data != 0)
        ((RecHVdel*)t->data)->vdel(3);
    s.r_int(&t->count);
    int n = t->count;
    int* blk = (int*)operator new[](n * sizeof(RecH) + 4);
    RecH* p;
    if (blk != 0) {
        p = (RecH*)(blk + 1);
        blk[0] = n;
        for (int i = 0; i < n; i++) {
            tstring_ctor(&p[i].name);
            tstring_ctor(&p[i].s.name);
            p[i].s.ptr = 0;
        }
    } else {
        p = 0;
    }
    t->data = p;
    for (int i = 0; i < t->count; i++)
        read_entry(s, t->data[i]);
    return (int)&s;
}

// @ 0x4c2300
void MissionTemplate::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)this);
    s.r_tstr((rgl::string*)&f10);
    s.r_int(&f20);
    s.r_int(&f3c);
    tstring* disp = &display;
    s.r_tstr((rgl::string*)disp);
    s.r_int(&n_spawns);
    spawns = (SpawnRec*)new char[n_spawns * 0x4c];
    for (int i = 0; i < n_spawns; i++)
        spawns[i].load_extras(s);
    _snprintf(buf, 0x200, "Mission Template %s", name.c_str());
    ((TriggerVec*)triggers_raw)->resize(s, buf);
    ((TriggerVec*)triggers_raw)->stamp(2);
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
void* __fastcall dialog_mgr_ctor(void* self)
{
    char* p = (char*)self + 0x30;
    for (int i = 0; i < 0x400; i++, p += 0x1c)
        ((tstring*)p)->tstring::tstring();
    p = (char*)self + 0x7030;
    for (int i = 0; i < 0x400; i++, p += 0x1c)
        ((tstring*)p)->tstring::tstring();
    ((tstring*)((char*)self + 0xe028))->tstring::tstring();
    *(int*)((char*)self + 4) = 0;
    return self;
}

// @ 0x4c2b20
void MenuRec::read(stream_base& s)
{
    char buf[0x200];
    s.r_tstr((rgl::string*)&id);
    s.r_i64(&flags);
    s.r_tstr((rgl::string*)&text);
    text.underscores_to_spaces();
    s.r_tstr((rgl::string*)&mesh);
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
extern "C" short __stdcall FSOUND_Stream_Close_(void* stream);

// @ 0x4c24c0
// (the original brackets the monitor initialisation with two 18-byte WinLicense markers, at 0x4c2542 and 0x4c25c1)
music_player::music_player()
{
    for (int i = 0; i < 0x100; i++)
        tstring_ctor((tstring*)((char*)this + 0x18 + i * 0x20));
    TimedVal* t = (TimedVal*)&w[4];                 // +0x2418 / +0x2420
    t->id = 0; t->t = g_ticks[0];
    t = (TimedVal*)&w[18];                          // +0x2450 / +0x2458
    t->id = 0; t->t = g_ticks[0];
    t = (TimedVal*)&w[22];                          // +0x2460 / +0x2468
    t->id = 0; t->t = g_ticks[0];
    tstring_ctor((tstring*)((char*)this + 0x247c));
    ((int*)this)[1] = 0;
    w[8] = 0;  w[0] = 0;  w[12] = -1;  w[2] = 0;  w[10] = -1;  w[14] = 1000;  w[26] = 0;
    w[9] = 0;  w[1] = 0;  w[13] = -1;  w[3] = 0;  w[11] = -1;  w[15] = 1000;  w[27] = 0;
    *(int*)this = 0;
    w[16] = 0;
    *((char*)this + 0x2478) = 0;
}

// @ 0x4c25e0
// Slot 0 plays the current piece, slot 1 the one being faded in / out. Each slot first waits for its stream to open,
// then watches it until its end; with no piece in slot 0 the track is chosen: a script request wins, else the tracks
// matching the situation `mode` (and the category `mask`) compete by how long ago they were played.
void music_player::update()
{
    char buf[0x200];
    int& mode = *(int*)this;
    int& count = ((int*)this)[1];
    int& mask = w[16];
    if (w[0] != 0 && w[2] == 0) {
        int h = FSOUND_Stream_GetOpenState((void*)w[8]);
        if (h == 0) {
            w[2] = 1;
            w[12] = FSOUND_Stream_Play(-1, (void*)w[8]);
        } else if (h != -2) {
            tstring* nm = (tstring*)((char*)this + 0x18 + 0x20 * w[10]);
            sprintf(buf, "Error: Unable to load %s", nm->c_str());
            ((stream_base&)g_log).w_cstr(buf);
            w[0] = 0;
        }
    } else if (w[0] != 0) {
        int len = FSOUND_Stream_GetLengthMs((void*)w[8]);
        int pos = FSOUND_Stream_GetTime((void*)w[8]);
        int a = g_music_request;
        int b = g_music_request_kind;
        if (a >= 0 && a == w[10] && b > 0) {
            a = -1;
            g_music_request = a;
        }
        bool done = true;
        if (pos < len) {
            int rec = 0x20 * w[10];
            int fl = *(int*)((char*)this + rec + 8) & 0x100;
            if (fl != 0 || a < 0 || b != 2) {
                done = false;
                if (w[26] == 0 && fl == 0) {
                    if (mode == 0 || (*(int*)((char*)this + rec + 0xc) & mode) == 0
                        || (mask != 0 && (*(int*)((char*)this + rec + 0x14) & mask) == 0)
                        || (a >= 0 && b == 1)) {
                        TimedVal* t = (TimedVal*)&w[18];
                        t->t = g_ticks[w[18]];
                        w[26] = 1;
                        *((char*)this + 0x2478) = 1;
                    }
                }
            }
        }
        if (done) {
            FSOUND_Stream_Close_((void*)w[8]);
            w[8] = 0;
            w[0] = 0;
            w[12] = -1;
        }
    } else if (g_music_volume > g_music_volume_floor) {
        int forced = 0;
        int sel = -1;
        int best = -1;
        int a = g_music_request;
        if (a >= 0) {
            g_music_request = -1;
            forced = 1;
            if (a != -1)
                sel = a;
        }
        if (sel < 0 && mode != 0) {
            int cur = w[10];
            if (cur >= 0 && (mode & *(int*)((char*)this + 0x20 * cur + 0xc)) != 0
                && (mask == 0 || (mask & *(int*)((char*)this + 0x20 * cur + 0x14)) != 0))
                best = cur;
            float sum = 0.0f;
            float now = (float)g_ticks[0] * 1e-5f + 1.0f;
            const float* times = (const float*)((char*)this + 0x2008);
            for (int i = 0; i < count; i++) {
                if (i != best && (TRACK(i, 8) & mode) != 0 && (mask == 0 || (TRACK(i, 0x10) & mask) != 0))
                {
                    sum += now - times[i];
                    sum += 500.0f;
                }
            }
            if (sum <= 0.0f) {
                sel = best;
            } else {
                int pick = (int)((float)(rand() % 0x3dc9) * sum * 6.3223117e-05f);
                for (int i = 0; i < count; i++) {
                    if (i != best && (TRACK(i, 8) & mode) != 0 && (mask == 0 || (TRACK(i, 0x10) & mask) != 0)) {
                        pick = (int)((float)pick - ((now - times[i]) + 500.0f));
                        if ((float)pick < 0.0f) {
                            sel = i;
                            break;
                        }
                    }
                }
            }
        }
        if (sel >= 0) {
            if (forced || *((char*)this + 0x2478) != 0 || (TRACK(sel, 8) & 0x80) != 0
                || (rand() % 0x3dc9) * 6.3223117e-05f < 0.01f) {
                int slot = 0;
                *((char*)this + 0x2478) = 0;
                play(sel, &slot);
                if ((TRACK(w[10], 8) & 0x100) != 0)
                    mode = 0;
            }
        }
    }
    // slot 1: the piece being faded
    if (w[1] != 0) {
        if (w[3] == 0) {
            int h = FSOUND_Stream_GetOpenState((void*)w[9]);
            if (h == 0) {
                w[3] = 1;
                w[13] = FSOUND_Stream_Play(-1, (void*)w[9]);
                *(__int64*)&w[24] = g_ticks[w[22]];
            } else if (h != -2) {
                tstring* nm = (tstring*)((char*)this + 0x18 + 0x20 * w[11]);
                sprintf(buf, "Error: Unable to load %s", nm->c_str());
                ((stream_base&)g_log).w_cstr(buf);
                w[1] = 0;
            }
        } else {
            int len = FSOUND_Stream_GetLengthMs((void*)w[9]);
            if (FSOUND_Stream_GetTime((void*)w[9]) < len)
                return;
            FSOUND_Stream_Close_((void*)w[9]);
            w[9] = 0;
            w[1] = 0;
            w[13] = -1;
        }
    } else if (g_music_event >= 0) {
        if (g_music_volume > g_music_volume_floor) {
            int slot = 1;
            play(g_music_event, &slot);
        }
        g_music_event = -1;
    }
}
