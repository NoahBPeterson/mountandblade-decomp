//-----------------------------------------------------------------------------
// File: editor_dialog.cpp  (0x4fe860..0x500ad0)
//
// Desc: the map editor's terrain (brush) dialog procedure: WM_INITDIALOG, WM_NOTIFY (trackbar notifications) and
//       WM_COMMAND (button, listbox, combo and edit notifications), one 8390-byte function. The editor panel (hwnd at
//       +0x374, HINSTANCE at +0x358), the game object g_game (0x5e2930) and the editor scene g_editor (0x5e292c) are
//       seen through the local views EditorGameView / EditorSceneView / EditorTerrainView: the canonical GameState
//       (terrain_a/game_state.h), Terrain (terrain_a/terrain.h) and the other g_game views cover different offsets of
//       the same objects and are not compatible with the rgl.h world this function needs.
//       Names provisional.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"

// GetDlgItem: User32 exports it as GetDlgItem (IAT 0x54932c); windows.h's A/W-neutral macro maps to it.

long __cdecl _atol(const char*);                    // 0x53734d
char* __cdecl _itoa(int, char*, int);               // 0x5371f7
int  __cdecl _stricmp(const char*, const char*);    // 0x536e9b
int  __cdecl _sprintf(char*, const char*, ...);     // 0x536f8f

struct EditorSceneView;                                 // the editor's scene/document (0x5e292c)
struct EditorGameView;                                   // the game object (0x5e2930)

EditorSceneView* g_editor;                              // 0x5e292c
EditorGameView*   g_game;                                // 0x5e2930
char** g_object_names;                              // 0x8ed4df8: 0x20-byte records, name pointer at +4

float g_one;                                        // 0x549cdc   1.0f
float g_hundred;                                    // 0x54ab28 100.0f
float g_color_unit;                                 // 0x54a6c4   1/255


int  size();                                   // 0x40f010
void pop_back_if(int i);                               // 0x438bf0
void resize(int a, int b);                      // 0x43f310
void push_back_out();                                 // 0x440b30
void set_delay();                              // 0x42cd20
void brush_apply_end();                             // 0x41f4b0
void frame_step();                            // 0x4e8720
void restore_defaults();                           // 0x4d3a50
void refresh();                            // 0x4c77c0
void refresh_dialog();                              // 0x4f6390
void set_color(float x, float y, float z, float w);     // 0x47af30
void init();                        // 0x4e85d0
void conv_build_scene();                        // 0x4c8630
void set_object_target();                             // 0x4c6dc0
char split_edge();                            // 0x4cb3a0
void build_second_pass();                                  // 0x4d1880
void update_edge_mode();                                  // 0x4c5b60
void update_poly_mode();                                  // 0x4c5c80
void update_vertex_mode();                                  // 0x4c5a20
void refresh_selection();                                  // 0x4c5dc0
char build_first_pass();                            // 0x4d1410
void finish();                                  // 0x4d22d0
char teardown();                            // 0x4d9e80
char combine();                            // 0x4caa40
char split_marked_edge();                            // 0x4d27d0
void add_quad();                                  // 0x4d2df0
INT_PTR __stdcall options_dlg_proc(HWND, UINT, WPARAM, LPARAM);  // 0x4bf5f0

struct EditorGameView {
    char pad0[0x1a094];
    unsigned mode;          // +0x1a094  0..4: active transform mode (0 = default/brush)
    int   rt_flag;          // +0x1a098  "scale everything" toggle
    float rt_x;             // +0x1a09c
    float rt_y;             // +0x1a0a0
    float rt_z;             // +0x1a0a4
    float rt_w;             // +0x1a0a8
    int   rt_extra;         // +0x1a0ac
    int   combo_sel;        // +0x1a0bc
    char  flag_c8;          // +0x1a0c8
    int   opt_cc;           // +0x1a0cc
    int   opt_d0;           // +0x1a0d0
    float mx, my, mz;       // +0x1a0d4  slider 1: [mode 1], [mode 2], [default]
    float sx, sy, sz;       // +0x1a0e0  slider 2
    float tx, ty, tz;       // +0x1a0ec  slider 3
    float col_r, col_g, col_b, col_a;   // +0x1a0f8
    int   axis;             // +0x1a108  0/1/2 (radio buttons 0x474..0x476)
    char  pad10c[0x1a1b4 - 0x1a10c];
    int   sel_copy[16];     // +0x1a10c  copy of a terrain's +0x18..+0x58
    int   sel_index;        // +0x1a1b4  selected terrain (-1 = none)
    int*  sel_array;        // +0x1a1bc  selected terrain indices, sel_count() long
    int   dirty;            // +0x1a1cc
    unsigned sub_mode;      // +0x1a1d0
};

struct EditorTerrainView {            // 0x1c8 bytes
    char  pad0[0x158];
    float rx, ry, rz;       // +0x158  scale components
    char  pad164[0x1a0 - 0x164];
    unsigned field_1a0;     // +0x1a0
    unsigned field_1a4;     // +0x1a4
    char  pad1a8[0x1b4 - 0x1a8];
    int   obj;              // +0x1b4
};

struct EditorSceneView {
    char  pad0[0x63d0];
    int   brush_count;      // +0x63d0
    char  pad63d4[0x63ec - 0x63d4];
    int   brush_first;      // +0x63ec  brush_count records, 0x18 apart (index at +0)
    char  pad63f0[0x668c - 0x63f0];
    EditorTerrainView* world;         // +0x668c  array of 0x1c8-byte terrain objects
    char  pad6690[0x6710 - 0x6690];
    int   sel_count2;       // +0x6710
    char  pad6714[0x6744 - 0x6714];
    char  need_redraw[3];   // +0x6744..0x6746  per-sub-mode redraw flags
};

static inline EditorTerrainView* terrain(int i) { return (EditorTerrainView*)((char*)g_editor->world + i * 0x1c8); }

#include "../editor_panel.h"       // EditorPanel : GameApp (hinst = m_hInstance +0x358, m_hMapEditorDlg = m_hMapEditorDlg +0x374)

// @ 0x4fe860
LRESULT EditorPanel::terrain_dialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND) {
        goto command;
    }
    if (msg <= WM_COMMAND) {
        if (msg == WM_NOTIFY) {
            switch (wParam & 0xffff) {
            case 0x472: {
                LRESULT v = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                float f = (float)v;
                double d;
                if (g_game->mode == 2) {
                    g_game->my = f;
                    d = (double)g_game->my;
                } else if (g_game->mode == 1) {
                    g_game->mx = f;
                    d = (double)g_game->mx;
                } else {
                    g_game->mz = f;
                    d = (double)g_game->mz;
                }
                char buf[512];
                _sprintf(buf, "%1.0f", d);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                return 0;
            }
            case 0x473: {
                LRESULT v = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0);
                float f = (float)v * 0.01f;
                double d;
                if (g_game->mode == 2) {
                    g_game->sy = f;
                    d = (double)g_game->sy;
                } else if (g_game->mode == 1) {
                    g_game->sx = f;
                    d = (double)g_game->sx;
                } else {
                    g_game->sz = f;
                    d = (double)g_game->sz;
                }
                char buf[512];
                _sprintf(buf, "%1.2f", d);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                return 0;
            }
            case 0x477: {
                LRESULT v = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0);
                float f = (float)v * 0.01f;
                double d;
                if (g_game->mode == 2) {
                    g_game->ty = f;
                    d = (double)g_game->ty;
                } else if (g_game->mode == 1) {
                    g_game->tx = f;
                    d = (double)g_game->tx;
                } else {
                    g_game->tz = f;
                    d = (double)g_game->tz;
                }
                char buf[512];
                _sprintf(buf, "%1.2f", d);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                return 0;
            }
            case 0x4c8:
            case 0x4c9:
            case 0x4ca: {
                int r = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_GETPOS, 0, 0);
                int g = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_GETPOS, 0, 0);
                int b = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_GETPOS, 0, 0);
                char num[64];
                char text[64];
                _itoa(r, num, 10);
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
                if (_stricmp(num, text) != 0)
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, num);
                _itoa(g, num, 10);
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
                if (_stricmp(num, text) != 0)
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, num);
                _itoa(b, num, 10);
                GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
                if (_stricmp(num, text) != 0)
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4ce, num);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4cb), 0x443,
                             (WPARAM)(((b & 0xff) << 16) | (r & 0xff) | ((g & 0xff) << 8)), 0);
                g_game->col_r = (float)r * g_color_unit;
                g_game->col_g = (float)g * g_color_unit;
                g_game->col_b = (float)b * g_color_unit;
                g_game->col_a = 0.0f;
                return 0;
            }
            }
        }
        if (msg == WM_MOUSEMOVE)
            return 1;
        if (msg != WM_INITDIALOG)
            return 0;
        m_hMapEditorDlg = hDlg;
        update_checks();
        SendMessageA(GetDlgItem(hDlg, 0x46e), LB_RESETCONTENT, 0, 0);
        CheckRadioButton(hDlg, 0x474, 0x476, g_game->axis + 0x474);
        SendMessageA(GetDlgItem(hDlg, 0x473), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessageA(GetDlgItem(hDlg, 0x477), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessageA(GetDlgItem(hDlg, 0x472), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessageA(GetDlgItem(hDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_one));
        SendMessageA(GetDlgItem(hDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_one));
        SendMessageA(GetDlgItem(hDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)(g_one));
        SendMessageA(GetDlgItem(hDlg, 0x4c8), TBM_SETRANGE, TRUE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(hDlg, 0x4c9), TBM_SETRANGE, TRUE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(hDlg, 0x4ca), TBM_SETRANGE, TRUE, MAKELONG(0, 255));
        g_game->col_r = g_one;
        g_game->col_g = g_one;
        g_game->col_b = g_one;
        g_game->col_a = 0.0f;
        SendMessageA(GetDlgItem(hDlg, 0x4c8), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(hDlg, 0x4c9), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(hDlg, 0x4ca), TBM_SETPOS, TRUE, 255);
        int n = 0;
        for (int i = 0; i < g_editor->brush_count - 2; i++) {
            int j = *(int*)((char*)g_editor + i * 0x18 + 0x63ec);
            if (j >= 0) {
                // name table: 0x20-byte records at 0x8ed4df8, the name pointer at +4 (NULL -> the record itself)
                const char* name = g_object_names[j * 8 + 1];
                if (name == 0)
                    name = (const char*)&g_object_names[j * 8];
                WPARAM idx = SendMessageA(GetDlgItem(hDlg, 0x46e), LB_ADDSTRING, 0, (LPARAM)name);
                SendMessageA(GetDlgItem(hDlg, 0x46e), LB_SETITEMDATA, idx, n);
            }
            n++;
        }
        SendMessageA(GetDlgItem(hDlg, 0x46e), LB_SETCURSEL, 0, 0);
        HWND combo = GetDlgItem(hDlg, 0x431);
        SendMessageA(combo, CB_RESETCONTENT, 0, 0);
        if (combo == 0)
            return 1;
        WPARAM idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)0x5999c0);
        SendMessageA(combo, CB_SETITEMDATA, idx, 0);
        idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)0x595e74);
        SendMessageA(combo, CB_SETITEMDATA, idx, 1);
        idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)0x5999b4);
        SendMessageA(combo, CB_SETITEMDATA, idx, 2);
        idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)0x5999ac);
        SendMessageA(combo, CB_SETITEMDATA, idx, 4);
        idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)0x595e6c);
        SendMessageA(combo, CB_SETITEMDATA, idx, 5);
        SendMessageA(combo, CB_SETCURSEL, 0, 0);
        update_object_type();
        return 1;
    }
    switch (wParam & 0xffff) {
    case 2:
        restore_defaults();
        ShowWindow(m_hMapEditorDlg, SW_HIDE);
        return 1;
    case 0x433:
        if (HIWORD(lParam) != 2) {
            HWND c = GetDlgItem(m_hMapEditorDlg, 0x433);
            g_game->combo_sel = (int)SendMessageA(c, CB_GETITEMDATA, SendMessageA(c, CB_GETCURSEL, 0, 0), 0);
            return 1;
        }
        g_game->flag_c8 = 1;
        return 1;
    case 0x434: {
        LRESULT v = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x434), BM_GETCHECK, 0, 0);
        g_game->rt_flag = (v == 1);
        g_game->rt_x = g_one;
        g_game->rt_y = g_one;
        g_game->rt_z = g_one;
        g_game->rt_w = 0.0f;
        g_game->rt_extra = 0;
        if (g_game->rt_flag == 0)
            refresh();
        return 1;
    }
    case 0x438: {
        int code = (short)HIWORD(lParam);
        if (code == 2) {
            HWND c = GetDlgItem(hDlg, 0x438);
            LRESULT cnt = SendMessageA(c, LB_GETCOUNT, 0, 0);
            for (int i = 0; i < (int)cnt; i++)
                if (g_game->sel_index != (int)SendMessageA(c, LB_GETITEMDATA, i, 0))
                    SendMessageA(c, LB_SETSEL, 0, i);
            resize(0, 0);
            if (g_game->sel_index >= 0)
                push_back_out();
            refresh_dialog();
            if (g_game->sel_index >= 0) {
                memcpy(g_game->sel_copy, (char*)terrain(g_game->sel_index) + 0x18, 0x40);
                set_delay();
            }
            refresh();
            return 1;
        }
        if (code == 1 || code == 3) {
            HWND c = GetDlgItem(hDlg, 0x438);
            int sel[1024];
            int cnt = (int)SendMessageA(c, LB_GETSELITEMS, 0x400, (LPARAM)sel);
            g_game->sel_index = -1;
            for (int i = 0; i < cnt; i++) {
                int v = (int)SendMessageA(c, LB_GETITEMDATA, sel[i], 0);
                if (v >= 0) {
                    push_back_out();
                    int found = 0;
                    for (int k = 0; k < size(); k++) {
                        if (g_game->sel_array[k] == v) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        push_back_out();
                        g_game->sel_index = v;
                    }
                }
            }
            for (int k = size(); k > 1; k--) {
                g_game->sel_array[0] = g_game->sel_array[k - 1];
                pop_back_if(0);
            }
            if (g_game->sel_index == -1 && size() != 0)
                g_game->sel_index = g_game->sel_array[size() - 1];
            refresh_dialog();
            brush_apply_end();
        }
        return 1;
    }
    case 0x43a:
        if (HIWORD(lParam) != 0x200)
            return 1;
        {
            BOOL ok;
            UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43a, &ok, FALSE);
            if (ok != 0 && g_game->sel_index >= 0 && v < 0x40) {
                terrain(g_game->sel_index)->field_1a0 = v;
                g_game->dirty = 1;
                return 1;
            }
            SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
        }
        return 1;
    case 0x43b:
        if (HIWORD(lParam) != 0x200)
            return 1;
        {
            BOOL ok;
            UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43b, &ok, FALSE);
            if (ok != 0 && g_game->sel_index >= 0 && v < 0x40) {
                terrain(g_game->sel_index)->field_1a4 = v;
                g_game->dirty = 1;
                return 1;
            }
            SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
        }
        return 1;
    case 0x453:
        g_game->opt_cc = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x453), BM_GETCHECK, 0, 0) == 1;
        return 1;
    case 0x454:
        g_game->opt_d0 = SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x454), BM_GETCHECK, 0, 0) == 1;
        return 1;
    case 0x431:
        if (HIWORD(lParam) != 1)
            return 1;
        update_object_type();
        return 1;
    case 0x46d:
        if (HIWORD(lParam) != 0)
            return 1;
        g_game->mode = 2;
        update_checks();
        {
            float v1 = g_game->my, v2 = g_game->sy, v3 = g_game->ty;
            char buf[512];
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)(v1));
            _sprintf(buf, "%1.0f", (double)v1);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(v2 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v2);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(v3 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v3);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
        }
        return 1;
    case 0x46f:
        if (HIWORD(lParam) != 0)
            return 1;
        g_game->mode = 0;
        update_checks();
        return 1;
    case 0x470:
        if (HIWORD(lParam) != 0)
            return 1;
        g_game->mode = 1;
        update_checks();
        {
            float v1 = g_game->mx, v2 = g_game->sx, v3 = g_game->tx;
            char buf[512];
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)(v1));
            _sprintf(buf, "%1.0f", (double)v1);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(v2 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v2);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(v3 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v3);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
        }
        return 1;
    case 0x471:
        if (HIWORD(lParam) != 0)
            return 1;
        g_game->mode = 3;
        update_checks();
        {
            float v1 = g_game->mz, v2 = g_game->sz, v3 = g_game->tz;
            char buf[512];
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)(v1));
            _sprintf(buf, "%1.0f", (double)v1);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(v2 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v2);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(v3 * g_hundred));
            _sprintf(buf, "%1.2f", (double)v3);
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
        }
        return 1;
    case 0x474:
        g_game->axis = 0;
        CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->axis + 0x474);
        return 1;
    case 0x475:
        g_game->axis = 1;
        CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->axis + 0x474);
        return 1;
    case 0x476:
        g_game->axis = 2;
        CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->axis + 0x474);
        return 1;
    case 0x489:
        DialogBoxParamA(m_hInstance, (LPCSTR)0xde, m_hMapEditorDlg, options_dlg_proc, 0);
        return 1;
    case 0x491:
    case 0x492:
    case 0x493:
        if (HIWORD(lParam) != 0)
            return 1;
        if (g_game->sel_index < 1 && g_game->rt_flag == 0)
            return 1;
        {
            float fx = g_one;
            float fy = g_one;
            float fz = g_one;
            if ((short)wParam == 0x491)
                fx = -g_one;
            else if ((short)wParam == 0x492)
                fy = -g_one;
            else if ((short)wParam == 0x493)
                fz = -g_one;
            if (g_game->rt_flag) {
                float ox = g_game->rt_x;
                float oy = g_game->rt_y;
                float oz = g_game->rt_z;
                float ow = g_game->rt_w;
                g_game->rt_x = fx * g_game->rt_x;
                g_game->rt_y = fy * g_game->rt_y;
                g_game->rt_z = fz * g_game->rt_z;
                if (size() == 0)
                    return 1;
                float invx = g_one / ox;
                float invy = g_one / oy;
                float invz = g_one / oz;
                int i = 0;
                do {
                    set_color(invx, invy, invz, ow);
                    set_color(g_game->rt_x, g_game->rt_y, g_game->rt_z, g_game->rt_w);
                    i++;
                } while (i < size());
                return 1;
            }
            EditorTerrainView* t = terrain(g_game->sel_index);
            t->rx = fx * t->rx;
            t->ry = fy * t->ry;
            t->rz = fz * t->rz;
            for (int i = 0; i < size(); i++) {
                EditorTerrainView* ti = terrain(g_game->sel_index);
                *(int*)(*(int*)(*(int*)(ti->obj + 0x11c) + i * 4) + 0x4c) =
                    1 - *(int*)(*(int*)(*(int*)(ti->obj + 0x11c) + i * 4) + 0x4c);
            }
            set_object_target();
            g_game->dirty = 1;
        }
        return 1;
    case 0x494:
        if (MessageBoxA(m_hMapEditorDlg, "Are you sure?", "Warning", 1) != 1)
            return 1;
        frame_step();
        return 1;
    case 0x46e:
        if (HIWORD(lParam) != 1)
            return 1;
        {
            HWND lst = GetDlgItem(m_hMapEditorDlg, 0x46e);
            *(int*)((char*)g_editor + 0x651c) = (int)SendMessageA(lst, LB_GETITEMDATA,
                                                  SendMessageA(lst, LB_GETCURSEL, 0, 0), 0);
        }
        return 1;
    case 0x497:
        if (HIWORD(lParam) != 0)
            return 1;
        g_game->sub_mode = 0;
        update_checks();
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x498:
        if (HIWORD(lParam) == 0) {
            g_game->sub_mode = 1;
            update_checks();
            g_game->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
        }
        return 1;
    case 0x499:
        if (HIWORD(lParam) == 0) {
            g_game->sub_mode = 2;
            update_checks();
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
        }
        return 1;
    case 0x49a:
        if (HIWORD(lParam) == 0) {
            g_game->mode = 4;
            update_checks();
        }
        return 1;
    case 0x49b:
        build_second_pass();
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x49d:
        if (!split_edge()) {
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge that has a neighborhood of 2 triangular faces.", "Warning", 0);
            return 1;
        }
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4a3:
        update_edge_mode();
        g_game->sub_mode = 1;
        update_checks();
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4a4:
        update_poly_mode();
        g_game->sub_mode = 2;
        update_checks();
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4a5:
        update_vertex_mode();
        g_game->sub_mode = 0;
        update_checks();
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4a6:
        refresh_selection();
        g_editor->need_redraw[g_game->sub_mode] = 1;
        return 1;
    case 0x4a7:
        if (!build_first_pass()) {
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of a face and do not have any edge neighborhood.", "Warning", 0);
            return 1;
        }
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4a8:
        finish();
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4aa:
        if (!teardown()) {
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 3 or 4 vertices (or edges that has 3 or 4 different edges) that are not a member of the same face and that are not members of any edges with 2 faces.", "Warning", 0);
            return 1;
        }
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4ac:
        if (!combine()) {
            if (g_editor->sel_count2 != 0)
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
            else
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of the same edge or that are members of any edge having a neighborhood to only one face.", "Warning", 0);
            return 1;
        }
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4ad:
        if (!split_marked_edge()) {
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
            return 1;
        }
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4ae:
        add_quad();
        g_game->dirty = 1;
        g_editor->need_redraw[0] = 1;
        g_editor->need_redraw[1] = 1;
        g_editor->need_redraw[2] = 1;
        return 1;
    case 0x4af:
        if (MessageBoxA(m_hMapEditorDlg, "This action will clear all texture and height data of the terrain that are added manually. Are you sure?", "Warning", 1) == 1) {
            init();
            g_game->dirty = 1;
            conv_build_scene();
        }
        return 1;
    case 0x4c8:
    case 0x4c9:
    case 0x4ca: {
        int r = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_GETPOS, 0, 0);
        int g = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_GETPOS, 0, 0);
        int b = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_GETPOS, 0, 0);
        char num[64];
        char text[64];
        _itoa(r, num, 10);
        GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
        if (_stricmp(num, text) != 0)
            SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, num);
        _itoa(g, num, 10);
        GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
        if (_stricmp(num, text) != 0)
            SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, num);
        _itoa(b, num, 10);
        GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
        if (_stricmp(num, text) != 0)
            SetDlgItemTextA(m_hMapEditorDlg, 0x4ce, num);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4cb), 0x443,
                     (WPARAM)(((b & 0xff) << 16) | (r & 0xff) | ((g & 0xff) << 8)), 0);
        g_game->col_r = (float)r * g_color_unit;
        g_game->col_g = (float)g * g_color_unit;
        g_game->col_b = (float)b * g_color_unit;
        g_game->col_a = 0.0f;
        return 1;
    }
    case 0x4cd: {
        if (HIWORD(lParam) != 0x400)
            return 1;
        char text[0x40];
        long v;
        GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
        if (text[0] == 0)
            v = 0;
        else {
            v = _atol(text);
            if (v < 0)
                v = 0;
            else if (v > 0xff)
                v = 0xff;
        }
        _itoa(v, text, 10);
        SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETPOS, TRUE, (LPARAM)v);
        return 1;
    }
    case 0x4cf: {
        if (HIWORD(lParam) != 0x400)
            return 1;
        char text[0x40];
        long v;
        GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
        if (text[0] == 0)
            v = 0;
        else {
            v = _atol(text);
            if (v < 0)
                v = 0;
            else if (v > 0xff)
                v = 0xff;
        }
        _itoa(v, text, 10);
        SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETPOS, TRUE, (LPARAM)v);
        return 1;
    }
    case 0x4ce: {
        if (HIWORD(lParam) != 0x400)
            return 1;
        char text[0x40];
        long v;
        GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
        if (text[0] == 0 || (v = _atol(text)) < 0)
            v = 0;
        else if (v > 0xff)
            v = 0xff;
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETPOS, TRUE, (LPARAM)v);
        return 1;
    }
    }
command:
    return 1;
}
