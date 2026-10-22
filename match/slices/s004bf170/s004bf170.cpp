// Slice s004bf170: editor "options" dialog handlers (0x4bf170..0x4bf640). Part of the editor panel class
// (the same class as 0x4b9e70: hwnd at +0x374 / +0x380, a D3D-ish object at +0xf4 whose vtable slot 11 is
// called, an int at +0x59c). Names are provisional; layouts are only the offsets the code touches.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "tstring.h"

// GetDlgItemA: windows.h only provides the A/W-neutral GetDlgItem macro, so declare the A-suffixed import
// (the original reaches it through its import table at 0x54932c).
extern "C" __declspec(dllimport) HWND __stdcall GetDlgItemA(HWND, int);   // 0x54932c

long __cdecl _atol(const char*);                    // 0x53734d
#define atol _atol

// The renderer/device object the editor calls through at this+0xf4 (slot 0x2c).
struct EditorGfx {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10();
    // (this, int a, int b, int c, void* d, int e)
    virtual long v11(int a, int b, int c, void* d, int e);   // vtable slot 0x2c
};

// The editor panel (offset-only knowledge).
struct EditorPanel {
    char pad0[0xf4];
    EditorGfx* gfx;             // +0xf4
    char padf8[0x374 - 0xf8];
    HWND view_hwnd;             // +0x374
    char pad378[0x380 - 0x378];
    HWND hwnd;                  // +0x380  the options dialog
    char pad384[0x59c - 0x384];
    int  mode;                  // +0x59c
    void panel_refresh();       // 0x4b9e70
    void read_options();        // 0x4bf170
    void update_option_checks();// 0x4bf640
};

enum {  // control ids of the options dialog
    IDC_OPT_1 = 0x43e, IDC_OPT_441 = 0x441, IDC_OPT_442 = 0x442, IDC_OPT_443 = 0x443,
    IDC_OPT_444 = 0x444, IDC_OPT_445 = 0x445, IDC_OPT_446 = 0x446, IDC_OPT_448 = 0x448,
    IDC_OPT_44a = 0x44a, IDC_OPT_44c = 0x44c, IDC_OPT_44d = 0x44d, IDC_OPT_452 = 0x452,
    IDC_OPT_454 = 0x454, IDC_OPT_45f = 0x45f, IDC_OPT_465 = 0x465, IDC_OPT_467 = 0x467,
    IDC_OPT_468 = 0x468, IDC_OPT_487 = 0x487,
};

int  g_opt_5aa0ac;      // 0x5aa0ac  first combo's selection
int  g_opt_5aa0b0;      // 0x5aa0b0
int  g_opt_5aa0b4;      // 0x5aa0b4  fps limit (10..999)
int  g_opt_5aa0b8;      // 0x5aa0b8
int  g_opt_5aa0c0;      // 0x5aa0c0
int  g_opt_5aa0c4;      // 0x5aa0c4
int  g_opt_5aa0cc;      // 0x5aa0cc
int  g_opt_5aa0d0;      // 0x5aa0d0
int  g_opt_5aa0d8;      // 0x5aa0d8
int  g_opt_5aa0dc;      // 0x5aa0dc
int  g_opt_5aa0e0;      // 0x5aa0e0
int  g_opt_5aa0e4;      // 0x5aa0e4
int  g_opt_5aa0e8;      // 0x5aa0e8
int  g_opt_5aa0f4;      // 0x5aa0f4  resolution width
int  g_opt_5aa0f8;      // 0x5aa0f8  resolution height
int  g_opt_5aa0fc;      // 0x5aa0fc  colour depth (0x10 / 0x20)
int  g_opt_5aa100;      // 0x5aa100  windowed combo state
int  g_opt_5aa104;      // 0x5aa104  selected adapter/mode index
int  g_opt_5aa108;      // 0x5aa108
int  g_opt_5aa110;      // 0x5aa110  second combo state

void* g_editor_state;   // 0x5e2930  global editor state object
#define EV(off) (*(int*)((char*)g_editor_state + (off)))

void* g_dlg_options;    // 0x8ee3fc4  (HWND; kept untyped so the call reloads it into eax)
#define GHDL(p) ((HWND)(p))     // options dialog window (also written by the dialog proc)
const char* const s_options_help_text = "help";   // 0x593af0  the editor's key-help text

// @ 0x4bf170
void EditorPanel::read_options()
{
    g_opt_5aa0ac = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_1), WM_USER, 0, 0);
    g_opt_5aa0e0 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_441), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0dc = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_468), CB_GETCURSEL, 0, 0) == 1;
    g_opt_5aa0cc = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_442), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0d0 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_443), BM_GETCHECK, 0, 0) != 1;
    g_opt_5aa0b8 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_444), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0b0 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_445), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa108 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_487), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0e8 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_452), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0e4 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_446), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0d8 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_44d), BM_GETCHECK, 0, 0) != 1;
    {   // "fps limit" edit box, clamped to 10..999
        BOOL ok;
        UINT v = GetDlgItemInt(hwnd, IDC_OPT_448, &ok, FALSE);
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
    g_opt_5aa0c0 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_44c), BM_GETCHECK, 0, 0) == 1;
    g_opt_5aa0c4 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_44a), BM_GETCHECK, 0, 0) == 1;
    if (SendMessageA(GetDlgItemA(hwnd, IDC_OPT_45f), CB_GETCURSEL, 0, 0) == 0) {
        g_opt_5aa0f4 = 0;
        g_opt_5aa0f8 = 0;
        int md = mode;
        if (md == 0x17 || md == 0x19 || md == 0x1a || md == 0x18 || md == 0x1e)
            g_opt_5aa0fc = 0x10;
        else
            g_opt_5aa0fc = 0x20;
    } else {
        // the edit box holds "<width>x<height> <bits>"
        char buf[100];
        GetWindowTextA(GetDlgItemA(hwnd, IDC_OPT_45f), buf, 100);
        tstring s(buf);
        char sep[2];
        int ix; int isp;
        sep[0] = 'x';
        ix = s.find(sep[0], 0);
        sep[1] = ' ';
        isp = s.find(sep[1], 0);
        g_opt_5aa0f4 = atol(s.substr(0, ix).buf);
        g_opt_5aa0f8 = atol(s.substr(ix + 1, isp).buf);
        g_opt_5aa0fc = atol(s.substr(isp + 1, isp + 3).buf);
    }
    g_opt_5aa100 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_465), CB_GETCURSEL, 0, 0);
    g_opt_5aa100 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_465), CB_GETITEMDATA, g_opt_5aa100, 0);
    g_opt_5aa110 = SendMessageA(GetDlgItemA(hwnd, IDC_OPT_467), CB_GETITEMDATA,
                               SendMessageA(GetDlgItemA(hwnd, IDC_OPT_467), CB_GETCURSEL, 0, 0), 0);
    int m;
    if (g_opt_5aa0fc == 0x10)
        m = 0x17;
    else if (g_opt_5aa0fc == 0x20)
        m = 0x16;
    else
        m = mode;
    if (g_opt_5aa100 > 1) {
        int r;
        if (gfx->v11(0, 1, m, 0, g_opt_5aa100 != 2 ? 4 : 2) >= 0) {
            g_opt_5aa104 = r - 1;
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
        SetDlgItemTextA(dlg, 0x488, s_options_help_text);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) - 1 == 0)
            EndDialog(GHDL(g_dlg_options), 1);
        break;
    }
    return FALSE;
}

// @ 0x4bf640
void EditorPanel::update_option_checks()
{
    int *v = (int*)g_editor_state;   // 0x5e2930
    HWND d = view_hwnd;         // +0x374
    // (the original reloads g_editor_state for every control, exactly as written)
    SendMessageA(GetDlgItemA(d, 0x470), BM_SETCHECK, v[0x1a094 / 4] == 1, 0);
    SendMessageA(GetDlgItemA(d, 0x46d), BM_SETCHECK, v[0x1a094 / 4] == 2, 0);
    SendMessageA(GetDlgItemA(d, 0x471), BM_SETCHECK, v[0x1a094 / 4] == 3, 0);
    SendMessageA(GetDlgItemA(d, 0x49a), BM_SETCHECK, v[0x1a094 / 4] == 4, 0);
    SendMessageA(GetDlgItemA(d, 0x46f), BM_SETCHECK, v[0x1a094 / 4] == 0, 0);
    SendMessageA(GetDlgItemA(d, 0x434), BM_SETCHECK, v[0x1a098 / 4] != 0, 0);
    SendMessageA(GetDlgItemA(d, 0x453), BM_SETCHECK, v[0x1a0cc / 4] != 0, 0);
    SendMessageA(GetDlgItemA(d, 0x454), BM_SETCHECK, v[0x1a0d0 / 4] != 0, 0);
    SendMessageA(GetDlgItemA(d, 0x497), BM_SETCHECK,
                 v[0x1a094 / 4] == 4 && v[0x1a1d0 / 4] == 0, 0);
    SendMessageA(GetDlgItemA(d, 0x498), BM_SETCHECK,
                 v[0x1a094 / 4] == 4 && v[0x1a1d0 / 4] == 1, 0);
    SendMessageA(GetDlgItemA(d, 0x499), BM_SETCHECK,
                 v[0x1a094 / 4] == 4 && v[0x1a1d0 / 4] == 2, 0);
    panel_refresh();
}
