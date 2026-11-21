//-----------------------------------------------------------------------------
// File: editor_dialog.cpp  (0x4fe860..0x500ad0)
//
// Desc: the map editor's terrain (brush) dialog procedure: WM_INITDIALOG, WM_NOTIFY (trackbar notifications) and
//       WM_COMMAND (button, listbox, combo and edit notifications), one 8390-byte function. The editor panel (hwnd at
//       +0x374, HINSTANCE at +0x358) works on the game object's editor state (g_game->editor, include/game_data.h) and on
//       the editor scene g_mission (0x5e292c): its terrain layers, props, nav mesh and marker flags.
//       Names provisional.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include "../../include/layout32.h"
#include <stdio.h>
#include "../../include/image_addrs.h"
#include "../terrain_a/terrain.h"   // Terrain, g_terrain (0xa76630)
#include <windows.h>
#include <commctrl.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"          // rgl::mesh: set_color 0x47af30

// GetDlgItem: User32 exports it as GetDlgItem (IAT 0x54932c); windows.h's A/W-neutral macro maps to it.

char* __cdecl _itoa(int, char*, int);               // 0x5371f7
#include <string.h>        // strcmp: /O2 inlines it (the original compares the digit strings inline, 0x4fed02..0x4fed2b)
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

#include "../../include/game_data.h"                     // GameData g_game (0x5e2930): the editor state is g_game->editor

#include "../mission.h"                                 // Mission g_mission (0x5e292c): the edited scene
#include "../placed_obj.h"                              // PlacedObj: Mission::props (scale, value, the prop's entity)
#include "../../rgl/scene/scene_entity.h"               // rgl::entity: the meshes of a prop / of the edit-mode marker
// the ground-type table (0x8ed4df8: 0x20-byte records, include/ground_type.h) is indexed in place (0x4feaff: j * 0x20 + 0x8ed4df8); its one
// definition is module_load.cpp's g_ground_types, which the ground-spec loader (0x4d0e5b) fills.

// These live in the original's shared float-constant pool (.rdata). Defined without an initialiser they are
// zero in our build, which silently broke every use: g_one = 0 made GuiScreen::process's cursor y negative
// ((-y/h)*f514) so no widget ever hit-tested (menus unclickable), and g_hundred / g_color_unit are the
// 0..100 slider range and the byte->float colour unit.
float g_one = 1.0f;                                 // 0x549cdc   1.0f
float g_hundred = 100.0f;                           // 0x54ab28 100.0f


void restore_defaults();                           // 0x4d3a50
// The callees below take their object in ECX (thiscall). Each is declared as a member of the class that defines it, named as that definition has it
// (the modules that define them name the classes after another of their users, and the linker folds identical bodies), and called on the object the
// original passes: the editor scene's polygon mesh (Mission::nav), the game object's vectors,
// the dialog panel (this).
struct EditorApp { void refresh_dialog(); };                         // 0x4f6390 (editor_ui.cpp)
#include "../../include/timer.h"                 // Timer::set_delay 0x42cd20 (strat/gui_manager.cpp)
INT_PTR __stdcall options_dlg_proc(HWND, UINT, WPARAM, LPARAM);  // 0x4bf5f0

#include "../config/core.h"       // GameApp (hinstance = +0x358, m_hMapEditorDlg = +0x374)

static __forceinline void redraw_all() { g_mission->need_redraw[0] = 1; g_mission->need_redraw[1] = 1; g_mission->need_redraw[2] = 1; }   // the three stores, one block per use
// @ 0x4fe860
LRESULT GameApp::terrain_dialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        m_hMapEditorDlg = hDlg;
        update_editor_checks();
        HWND objs = GetDlgItem(hDlg, 0x46e);                          // looked up once (0x4fe8ba, kept in ebx)
        SendMessageA(objs, CB_RESETCONTENT, 0, 0);                    // 0x46e is a combo box (0x4fe8c8)
        CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->editor.axis + 0x474);
        // the three sliders' ranges (no redraw) and positions from the current values (0x4fe8ff..0x4fe9d7)
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETRANGE, FALSE, MAKELONG(1, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETRANGE, FALSE, MAKELONG(1, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.sx * 100.0f));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.tx * 100.0f));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game->editor.mx);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        g_game->editor.col = rgl::vector4(1.0f, 1.0f, 1.0f);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETPOS, TRUE, 255);
        for (int i = 0; i < g_mission->terrain.n - 2; i++) {
            int j = g_mission->terrain.layers[i].type;
            if (j >= 0) {
                // name table: 0x20-byte records at 0x8ed4df8 starting with a tstring: its c_str() (buf at +4, NULL ->
                // the address of that buf field, 0x4feb0a)
                WPARAM idx = SendMessageA(objs, CB_ADDSTRING, 0, (LPARAM)g_ground_types[j].name.c_str());
                SendMessageA(objs, CB_SETITEMDATA, idx, i);
            }
        }
        SendMessageA(objs, CB_SETCURSEL, 0, 0);
        HWND combo = GetDlgItem(hDlg, 0x431);
        SendMessageA(combo, CB_RESETCONTENT, 0, 0);
        if (combo) {
            WPARAM idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"Scene Props");
            SendMessageA(combo, CB_SETITEMDATA, idx, 0);
            idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"Entry Point");
            SendMessageA(combo, CB_SETITEMDATA, idx, 1);
            idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"Item Kinds");
            SendMessageA(combo, CB_SETITEMDATA, idx, 2);
            idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"Plants");
            SendMessageA(combo, CB_SETITEMDATA, idx, 4);
            idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"Passage");
            SendMessageA(combo, CB_SETITEMDATA, idx, 5);
            g_mission->object_kind = 0;                                // the object kind resets to the first entry (0x4fec16)
            SendMessageA(combo, CB_SETCURSEL, 0, 0);
            refresh_editor_kinds();
            return 1;
        }
        break;
    }
    case WM_NOTIFY:
        switch (LOWORD(wParam)) {
        case 0x4c8:
        case 0x4c9:
        case 0x4ca: {
            int b, g, r;
            r = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_GETPOS, 0, 0);
            g = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_GETPOS, 0, 0);
            b = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_GETPOS, 0, 0);
            char num[64];
            char text[64];
            _itoa(r, num, 10);
            GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
            if (strcmp(num, text) != 0)
                SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, num);
            _itoa(g, num, 10);
            GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
            if (strcmp(num, text) != 0)
                SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, num);
            _itoa(b, num, 10);
            GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
            if (strcmp(num, text) != 0)
                SetDlgItemTextA(m_hMapEditorDlg, 0x4ce, num);
            SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4cb), 0x443, 0, RGB(r, g, b));      // colour in lParam (0x4fee47..0x4fee79)
            g_game->editor.col = rgl::vector4((float)r * (1.0f / 255.0f), (float)g * (1.0f / 255.0f), (float)b * (1.0f / 255.0f));
            return 0;
        }
        case 0x473: {
            char buf[512];
            if (g_game->editor.mode == 2) {
                g_game->editor.sy = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.sy);
            } else if (g_game->editor.mode == 1) {
                g_game->editor.sx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.sx);
            } else {
                g_game->editor.sz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.sz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
            break;
        }
        case 0x477: {
            char buf[512];
            if (g_game->editor.mode == 2) {
                g_game->editor.ty = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.ty);
            } else if (g_game->editor.mode == 1) {
                g_game->editor.tx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.tx);
            } else {
                g_game->editor.tz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game->editor.tz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
            break;
        }
        case 0x472: {
            char buf[512];
            if (g_game->editor.mode == 2) {
                g_game->editor.my = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game->editor.my);
            } else if (g_game->editor.mode == 1) {
                g_game->editor.mx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game->editor.mx);
            } else {
                g_game->editor.mz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game->editor.mz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
            break;
        }
        }
        return 0;
    case WM_COMMAND:
        // WM_COMMAND: LOWORD(wParam) is the control, HIWORD(wParam) the notification code (0x4ff24c reads both from
        // wParam; every case below tested HIWORD(lParam), the control's window handle)
        switch (LOWORD(wParam)) {
        case 0x431:
            if (HIWORD(wParam) == 1) {
                refresh_editor_kinds();
                break;
            }
            break;
        case 2:
            restore_defaults();
            ShowWindow(m_hMapEditorDlg, SW_HIDE);
            break;
        case 0x433:
            if (HIWORD(wParam) == 2) {
                g_game->editor.flag_c8 = 1;
                break;
            } else {
                GetDlgItem(m_hMapEditorDlg, 0x431);                 // looked up and not used (0x4ff323)
                HWND c = GetDlgItem(m_hMapEditorDlg, 0x433);        // the object list is a list box: LB_ messages (0x4ff347)
                g_game->editor.list_sel = (int)SendMessageA(c, LB_GETITEMDATA, SendMessageA(c, LB_GETCURSEL, 0, 0), 0);
                break;
            }
        case 0x438: {
            WORD code = HIWORD(wParam);
            if (code == 2) {
                HWND c = GetDlgItem(hDlg, 0x438);
                int cnt = (int)SendMessageA(c, LB_GETCOUNT, 0, 0);
                for (int i = 0; i < cnt; i++)
                    if (g_game->editor.sel_index != (int)SendMessageA(c, LB_GETITEMDATA, i, 0))
                        SendMessageA(c, LB_SETSEL, 0, i);
                g_game->editor.sel.resize(0);
                if (g_game->editor.sel_index >= 0)
                    g_game->editor.sel.push_back(g_game->editor.sel_index);
                ((EditorApp*)this)->refresh_dialog();
                if (g_game->editor.sel_index >= 0) {
                    g_game->editor.sel_copy = g_mission->props[g_game->editor.sel_index].frame;
                    g_game->editor.timer.set_delay(-2.0f);
                }
                refresh_editor_dialog();
                break;
            }
            if (code == 1 || code == 3) {
                HWND c = GetDlgItem(hDlg, 0x438);
                int sel[1024];
                int cnt = (int)SendMessageA(c, LB_GETSELITEMS, 0x400, (LPARAM)sel);
                g_game->editor.sel_index = -1;
                std::vector<int> picked;                                        // the indices the list box reports
                for (int i = 0; i < cnt; i++) {
                    int v = (int)SendMessageA(c, LB_GETITEMDATA, sel[i], 0);
                    if (v >= 0) {
                        picked.push_back(v);
                        bool found = false;
                        unsigned k = 0;
                        if (g_game->editor.sel.size() > 0) do {
                            if (g_game->editor.sel[k] == v) {
                                found = true;
                                break;
                            }
                            k++;
                        } while (k < g_game->editor.sel.size());
                        if (!found) {
                            g_game->editor.sel.push_back(v);
                            g_game->editor.sel_index = v;
                        }
                    }
                }
                if (picked.size() != g_game->editor.sel.size()) {
                    // drop the selected indices the list box no longer reports (swap with the last, pop)
                    unsigned k = 0;
                    if (g_game->editor.sel.size() > 0) do {
                        bool kept = false;
                        for (unsigned j = 0; j < picked.size(); j++)
                            if (g_game->editor.sel[k] == picked[j])
                                kept = true;
                        if (!kept) {
                            g_game->editor.sel[k] = g_game->editor.sel[g_game->editor.sel.size() - 1];
                            g_game->editor.sel.pop_back();
                            k--;
                        }
                        k++;
                    } while (k < g_game->editor.sel.size());
                }
                if (g_game->editor.sel_index == -1 && g_game->editor.sel.size() > 0)
                    g_game->editor.sel_index = g_game->editor.sel[g_game->editor.sel.size() - 1];
                ((EditorApp*)this)->refresh_dialog();
                break;
            }
            break;
        }
        case 0x434:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x434), BM_GETCHECK, 0, 0); g_game->editor.rt_flag = v == BST_CHECKED; }
            g_game->editor.rt = rgl::vector4(1.0f, 1.0f, 1.0f);
            g_game->editor.rt_extra = 0;
            if (g_game->editor.rt_flag == 0) {
                refresh_editor_dialog();
                break;
            }
            break;
        case 0x43a:
            if (HIWORD(wParam) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43a, &ok, FALSE);
                if (ok != 0 && g_game->editor.sel_index >= 0 && v < 0x40) {
                    g_mission->props[g_game->editor.sel_index].value = v;
                    g_game->editor.dirty = 1;
                    break;
                }
                SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
                break;
            }
            break;
        case 0x46d:
            if (HIWORD(wParam) == 0) {
                g_game->editor.mode = 2;
                update_editor_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game->editor.my);
                sprintf(buf, "%1.0f", (double)g_game->editor.my);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.sy * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.sy);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.ty * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.ty);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                break;
            }
            break;
        case 0x43b:
            if (HIWORD(wParam) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43b, &ok, FALSE);
                if (ok != 0 && g_game->editor.sel_index >= 0 && v < 0x40) {
                    g_mission->props[g_game->editor.sel_index].m1a4 = v;
                    g_game->editor.dirty = 1;
                    break;
                }
                SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
                break;
            }
            break;
        case 0x453:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x453), BM_GETCHECK, 0, 0); g_game->editor.opt_cc = v == BST_CHECKED; }
            break;
        case 0x454:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x454), BM_GETCHECK, 0, 0); g_game->editor.opt_d0 = v == BST_CHECKED; }
            break;
        case 0x46e:
            if (HIWORD(wParam) == 1) {
                HWND lst = GetDlgItem(m_hMapEditorDlg, 0x46e);
                g_mission->object_kind = (int)SendMessageA(lst, CB_GETITEMDATA, SendMessageA(lst, CB_GETCURSEL, 0, 0), 0);
                break;
            }
            break;
        case 0x475:     // return here (not break) with break at the other success sites: cross-jumping then merges these three calls and the shared exit lands after this case
            g_game->editor.axis = 1;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->editor.axis + 0x474);
            return 1;
        case 0x474:
            g_game->editor.axis = 0;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->editor.axis + 0x474);
            return 1;
        case 0x476:
            g_game->editor.axis = 2;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game->editor.axis + 0x474);
            return 1;
        case 0x470:
            if (HIWORD(wParam) == 0) {
                g_game->editor.mode = 1;
                update_editor_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game->editor.mx);
                sprintf(buf, "%1.0f", (double)g_game->editor.mx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.sx * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.sx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.tx * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.tx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                break;
            }
            break;
        case 0x471:
            if (HIWORD(wParam) == 0) {
                g_game->editor.mode = 3;
                update_editor_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game->editor.mz);
                sprintf(buf, "%1.0f", (double)g_game->editor.mz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.sz * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.sz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game->editor.tz * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game->editor.tz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                break;
            }
            break;
        case 0x46f:
            if (HIWORD(wParam) == 0) {
                g_game->editor.mode = 0;
                update_editor_checks();
                break;
            }
            break;
        case 0x489:
            DialogBoxParamA(hinstance, (LPCSTR)0xde, m_hMapEditorDlg, options_dlg_proc, 0);
            break;
        case 0x494:
            if (MessageBoxA(m_hMapEditorDlg, "Are you sure?", "Warning", 1) == 1) {
                g_mission->frame_step();
                break;
            }
            break;
        case 0x491:
        case 0x492:
        case 0x493:
            if (HIWORD(wParam) == 0 && (g_game->editor.sel_index > 0 || g_game->editor.rt_flag != 0)) {
                rgl::vector4 f(1.0f, 1.0f, 1.0f);
                if ((short)wParam == 0x491)
                    f.x = -1.0f;
                else if ((short)wParam == 0x492)
                    f.y = -1.0f;
                else if ((short)wParam == 0x493)
                    f.z = -1.0f;
                if (g_game->editor.rt_flag) {
                    rgl::vector4 old = g_game->editor.rt;
                    g_game->editor.rt.x *= f.x;
                    g_game->editor.rt.y *= f.y;
                    g_game->editor.rt.z *= f.z;
                    for (unsigned i = 0; i < g_mission->edit_obj->meshes.size(); i++) {
                        rgl::vector4 inv(1.0f / old.x, 1.0f / old.y, 1.0f / old.z);
                        g_mission->edit_obj->meshes[i]->set_color(inv);
                        g_mission->edit_obj->meshes[i]->set_color(g_game->editor.rt);
                    }
                    break;
                }
                g_mission->props[g_game->editor.sel_index].scale.x *= f.x;
                g_mission->props[g_game->editor.sel_index].scale.y *= f.y;
                g_mission->props[g_game->editor.sel_index].scale.z *= f.z;
                for (unsigned i = 0; i < g_mission->props[g_game->editor.sel_index].ent->meshes.size(); i++) {
                    g_mission->props[g_game->editor.sel_index].ent->meshes[i]->mirrored = 1 - g_mission->props[g_game->editor.sel_index].ent->meshes[i]->mirrored;
                }
                g_mission->set_object_target(g_game->editor.sel_index);
                g_game->editor.dirty = 1;
                break;
            }
            break;
        case 0x497:
            if (HIWORD(wParam) == 0) {
                g_game->editor.sub_mode = 0;
                update_editor_checks();
                redraw_all();
            break;
            }
            break;
        case 0x4af:
            if (MessageBoxA(m_hMapEditorDlg, "This action will clear all texture and height data of the terrain that are added manually. Are you sure?", "Warning", 1) == 1) {
                g_terrain.init_map(&g_mission->terrain);
                g_game->editor.dirty = 1;
                g_terrain.conv_build_scene();
                break;
            }
            break;
        case 0x49a:
            if (HIWORD(wParam) == 0) {
                g_game->editor.mode = 4;
                update_editor_checks();
                break;
            }
            break;
        case 0x499:
            if (HIWORD(wParam) == 0) {
                g_game->editor.sub_mode = 2;
                update_editor_checks();
                redraw_all();
            break;
            }
            break;
        case 0x498:
            if (HIWORD(wParam) == 0) {
                g_game->editor.sub_mode = 1;
                update_editor_checks();
                redraw_all();
            break;
            }
            break;
        case 0x4a5:
            g_mission->nav.update_vertex_mode();
            g_game->editor.sub_mode = 0;
            update_editor_checks();
            redraw_all();
            break;
        case 0x4a3:
            g_mission->nav.update_edge_mode();
            g_game->editor.sub_mode = 1;
            update_editor_checks();
            redraw_all();
            break;
        case 0x4a4:
            g_mission->nav.update_poly_mode();
            g_game->editor.sub_mode = 2;
            update_editor_checks();
            redraw_all();
            break;
        case 0x49b:
            g_mission->nav.build_second_pass();
            g_game->editor.dirty = 1;
            redraw_all();
            break;
        case 0x4aa:
            if (g_mission->nav.teardown()) {
                g_game->editor.dirty = 1;
                redraw_all();
                break;
            }
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 3 or 4 vertices (or edges that has 3 or 4 different edges) that are not a member of the same face and that are not members of any edges with 2 faces.", "Warning", 0);
            break;
        case 0x4a7:
            if (g_mission->nav.build_first_pass()) {
                g_game->editor.dirty = 1;
                redraw_all();
                break;
            }
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of a face and do not have any edge neighborhood.", "Warning", 0);
            break;
        case 0x4a6:
            g_mission->nav.refresh_selection();
            g_mission->need_redraw[g_game->editor.sub_mode] = 1;
            break;
        case 0x4ad:
            if (g_mission->nav.split_marked_edge()) {
                g_game->editor.dirty = 1;
                redraw_all();
                break;
            }
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
            break;
        case 0x4a8:
            g_mission->nav.finish();
            g_game->editor.dirty = 1;
            redraw_all();
            break;
        case 0x49d:
            if (g_mission->nav.split_edge(-1)) {
                g_game->editor.dirty = 1;
                redraw_all();
            break;
            }
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge that has a neighborhood of 2 triangular faces.", "Warning", 0);
            break;
        case 0x4ac:
            if (g_mission->nav.combine()) {
                g_game->editor.dirty = 1;
                redraw_all();
                break;
            }
                if (g_mission->nav.mode == 0)
                    MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of the same edge or that are members of any edge having a neighborhood to only one face.", "Warning", 0);
                else
                    MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
            break;
        case 0x4ae:
            g_mission->nav.add_quad(&g_game->editor.cam_pos);
            g_game->editor.dirty = 1;
            redraw_all();
            break;
        case 0x4cd:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
                // the edit text is rewritten only when the value had to be corrected (empty, negative or above 255: 0x50078c);
                // an in-range value goes straight to the trackbar (0x5007b3)
                int v = -1;
                if (text[0]) v = atoi(text);
                if (v > 0xff || v < 0) {
                    if (v < 0) v = 0; else v = 0xff;
                    _itoa(v, text, 10);
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text);
                }
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETPOS, TRUE, (LPARAM)v);
                break;
            }
            break;
        case 0x4cf:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
                int v = -1;
                if (text[0]) v = atoi(text);
                if (v > 0xff || v < 0) {
                    if (v < 0) v = 0; else v = 0xff;
                    _itoa(v, text, 10);
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text);
                }
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETPOS, TRUE, (LPARAM)v);
                break;
            }
            break;
        case 0x4ce:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
                int v = -1;
                if (text[0]) v = atoi(text);                  // empty text: -1, which the clamp turns into 0 (0x5008ca jumps straight there)
                if (v < 0) v = 0; else if (v > 0xff) v = 0xff;
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETPOS, TRUE, (LPARAM)v);
                break;
            }
            break;
        }
        break;
    case WM_MOUSEMOVE:
        break;
    default:
        return 0;
    }
    return 1;
}
