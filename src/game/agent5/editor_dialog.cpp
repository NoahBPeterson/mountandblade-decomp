//-----------------------------------------------------------------------------
// File: editor_dialog.cpp  (0x4fe860..0x500ad0)
//
// Desc: the map editor's terrain (brush) dialog procedure: WM_INITDIALOG, WM_NOTIFY (trackbar notifications) and
//       WM_COMMAND (button, listbox, combo and edit notifications), one 8390-byte function. The editor panel (hwnd at
//       +0x374, HINSTANCE at +0x358), the game object g_game_ed (0x5e2930) and the editor scene g_editor (0x5e292c) are
//       seen through the local views EditorGameView / EditorSceneView / EditorTerrainView: the canonical GameState
//       (terrain_a/game_state.h), Terrain (terrain_a/terrain.h) and the other g_game_ed views cover different offsets of
//       the same objects and are not compatible with the rgl.h world this function needs.
//       Names provisional.
//-----------------------------------------------------------------------------
#include <stdlib.h>
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

struct EditorSceneView;                                 // the editor's scene/document (0x5e292c)
struct EditorGameView;                                   // the game object (0x5e2930)
struct GameData;
extern GameData* g_game;                                 // 0x5e2930 (game_data.h; its ptr_vec clashes with this file's)
#define g_game_ed ((EditorGameView*)g_game)       // the editor's view of it

struct Mission;
extern Mission* g_mission;                              // 0x5e292c (mission.h; its ptr_vec clashes with this file's)
#define g_editor ((EditorSceneView*)g_mission)   // the editor's view of the mission object g_mission (0x5e292c)
// the ground-type table (0x8ed4df8: 0x20-byte records, include/ground_type.h) is indexed in place (0x4feaff: j * 0x20 + 0x8ed4df8); its one
// definition is module_load.cpp's g_ground_types, which the ground-spec loader (0x4d0e5b) fills.

// These live in the original's shared float-constant pool (.rdata). Defined without an initialiser they are
// zero in our build, which silently broke every use: g_one = 0 made GuiScreen::process's cursor y negative
// ((-y/h)*f514) so no widget ever hit-tested (menus unclickable), and g_hundred / g_color_unit are the
// 0..100 slider range and the byte->float colour unit.
float g_one = 1.0f;                                 // 0x549cdc   1.0f
float g_hundred = 100.0f;                           // 0x54ab28 100.0f
float g_color_unit = 1.0f / 255.0f;                 // 0x54a6c4   1/255


void restore_defaults();                           // 0x4d3a50
// The callees below take their object in ECX (thiscall). Each is declared as a member of the class that defines it, named as that definition has it
// (the modules that define them name the classes after another of their users, and the linker folds identical bodies), and called on the object the
// original passes: the editor scene's edit object at g_editor + 0x6698 (EditMesh and graph_obj are two names for it), the game object's vectors,
// the dialog panel (this).
struct EditMesh {                                     // mesh_edit.cpp
    void add_quad(const rgl::vector4* p);             // 0x4d2df0
    bool build_first_pass();                          // 0x4d1410
    void build_second_pass();                         // 0x4d1880
    void finish();                                    // 0x4d22d0
    bool split_marked_edge();                         // 0x4d27d0
};
struct graph_obj {                                    // mesh_2.cpp / gui_edit.cpp
    bool combine();                                   // 0x4caa40
    bool split_edge(int edge);                        // 0x4cb3a0
    void refresh_selection();                         // 0x4c5dc0
    void update_edge_mode();                          // 0x4c5b60
    void update_poly_mode();                          // 0x4c5c80
    void update_vertex_mode();                        // 0x4c5a20
};
struct TearObj { bool teardown(); };                  // 0x4d9e80 (cls_map.cpp)
struct MapScreen { void frame_step(); };      // 0x4e8720 (progress_bar.cpp)
struct MapDialog { void set_object_target(int sel); };               // 0x4c6dc0 (gui_edit.cpp)
struct MapEditorDialog { void refresh(); void update_object_type(); };   // 0x4c77c0 / 0x4c7bf0 (gui_edit.cpp): the dialog procedure's own object
struct EditorApp { void refresh_dialog(); };                         // 0x4f6390 (editor_ui.cpp)
struct CreateGameOptionsPage { void update_checks(); };              // 0x4bf640 (trigger_vec.cpp)
#include "../../include/timer.h"                 // Timer::set_delay 0x42cd20 (strat/gui_manager.cpp)
struct ptr_vec { void pop_back_if(); };                              // 0x438bf0 (agent_stats.cpp)
// the vector<int> members of the game object / scene (VC7.1 layout: allocator slot, first, last, end); size, push_back and the release are the library's
// instantiations (mesh_obj.cpp 0x40f010, agent_combat.cpp 0x440b30, entity.cpp 0x41f4b0), called out of line through their member pointers
typedef std::vector<int> IntVec;
static __forceinline unsigned vsize(IntVec* v) { unsigned (IntVec::*f)() const = &IntVec::size; return (v->*f)(); }
static __forceinline void vpush(IntVec* v, const int& x) { void (IntVec::*f)(const int&) = &IntVec::push_back; (v->*f)(x); }
struct mface;
template <class T> struct vec { void clear(); };                     // the growable array whose clear() frees the block (rgl/mesh.h, entity.h): only clear() 0x41f4b0 is called
static __forceinline void vclear(void* v) { void (vec<mface>::*f)() = &vec<mface>::clear; ((vec<mface>*)v->*f)(); }
struct PickedVec { int alloc; int* first; int* last; int* end; };      // a local vector<int>, zeroed
#define GAME_VEC ((IntVec*)((char*)g_game_ed + 0x1a1b8))              // the selected terrain indices (sel_array is its first at +0x1a1bc)
#define EDIT_OBJ ((char*)g_editor + 0x6698)
INT_PTR __stdcall options_dlg_proc(HWND, UINT, WPARAM, LPARAM);  // 0x4bf5f0

struct SelFrame { int v[16]; };                     // a terrain's frame (+0x18..+0x58)
struct MeshFlag { char pad[0x4c]; int flag; };     // a terrain mesh (flag at +0x4c, toggled by the mirror buttons)
struct EditorGameView {
    char pad0[0x1a094];
    unsigned mode;          // +0x1a094  0..4: active transform mode (0 = default/brush)
    int   rt_flag;          // +0x1a098  "scale everything" toggle
    rgl::vector4 rt;        // +0x1a09c
    int   rt_extra;         // +0x1a0ac
    char  pad1a0b0[0x1a0bc - 0x1a0b0];
    int   combo_sel;        // +0x1a0bc
    char  pad1a0c0[0x1a0c8 - 0x1a0c0];
    char  flag_c8;          // +0x1a0c8
    int   opt_cc;           // +0x1a0cc
    int   opt_d0;           // +0x1a0d0
    float mx, my, mz;       // +0x1a0d4  slider 1: [mode 1], [mode 2], [default]
    float sx, sy, sz;       // +0x1a0e0  slider 2
    float tx, ty, tz;       // +0x1a0ec  slider 3
    rgl::vector4 col;       // +0x1a0f8
    int   axis;             // +0x1a108  0/1/2 (radio buttons 0x474..0x476)
    SelFrame sel_copy;      // +0x1a10c  copy of a terrain's +0x18..+0x58
    char  pad1a14c[0x1a150 - 0x1a14c];
    Timer timer;            // +0x1a150
    char  pad1a158[0x1a1a4 - 0x1a150 - sizeof(Timer)];
    rgl::vector4 quad;      // +0x1a1a4
    int   sel_index;        // +0x1a1b4  selected terrain (-1 = none)
    IntVec sel;             // +0x1a1b8  the selected terrain indices
    char  pad1a1c8[0x1a1cc - 0x1a1b8 - sizeof(IntVec)];
    int   dirty;            // +0x1a1cc
    unsigned sub_mode;      // +0x1a1d0
};
// the layout the original addresses (ours had lost three gaps: combo_sel, flag_c8.. and sel_index.. were 12, 20 and 0x48 bytes early)
typedef char EditorGameView_check[offsetof(EditorGameView, combo_sel) == 0x1a0bc && offsetof(EditorGameView, flag_c8) == 0x1a0c8
    && offsetof(EditorGameView, mx) == 0x1a0d4 && offsetof(EditorGameView, axis) == 0x1a108 && offsetof(EditorGameView, sel_copy) == 0x1a10c
    && offsetof(EditorGameView, sel_index) == 0x1a1b4 && offsetof(EditorGameView, sel) == 0x1a1b8
    && offsetof(EditorGameView, dirty) == 0x1a1cc && offsetof(EditorGameView, sub_mode) == 0x1a1d0 ? 1 : -1];

struct EditorMeshObj {                // the object a terrain / the edit scene holds: its meshes
    char  pad0[0x118];
    std::vector<MeshFlag*> meshes;    // +0x118
};

struct EditorTerrainView {            // 0x1c8 bytes
    char  pad0[0x18];
    SelFrame frame;         // +0x18
    char  pad58[0x158 - 0x58];
    float rx, ry, rz;       // +0x158  scale components
    char  pad164[0x1a0 - 0x164];
    unsigned field_1a0;     // +0x1a0
    unsigned field_1a4;     // +0x1a4
    char  pad1a8[0x1b4 - 0x1a8];
    EditorMeshObj* obj;     // +0x1b4
    char  pad1b8[0x1c8 - 0x1b8];
};

struct EditorSceneView {
    char  pad0[0x63d0];
    int   brush_count;      // +0x63d0
    char  pad63d4[0x63ec - 0x63d4];
    int   brush_first;      // +0x63ec  brush_count records, 0x18 apart (index at +0)
    char  pad63f0[0x651c - 0x63f0];
    int   object_kind;      // +0x651c
    char  pad6520[0x65e4 - 0x6520];
    EditorMeshObj* edit_obj;  // +0x65e4
    char  pad65e8[0x6688 - 0x65e8];
    std::vector<EditorTerrainView> world;   // +0x6688 (first at +0x668c)  the 0x1c8-byte terrain objects
    char  pad6698[0x6710 - 0x6688 - sizeof(std::vector<EditorTerrainView>)];
    int   sel_count2;       // +0x6710
    char  pad6714[0x6744 - 0x6714];
    char  need_redraw[3];   // +0x6744..0x6746  per-sub-mode redraw flags
};

#include "../config/core.h"       // GameApp (hinstance = +0x358, m_hMapEditorDlg = +0x374)

// @ 0x4fe860
LRESULT GameApp::terrain_dialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        m_hMapEditorDlg = hDlg;
        ((CreateGameOptionsPage*)this)->update_checks();
        HWND objs = GetDlgItem(hDlg, 0x46e);                          // looked up once (0x4fe8ba, kept in ebx)
        SendMessageA(objs, CB_RESETCONTENT, 0, 0);                    // 0x46e is a combo box (0x4fe8c8)
        CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game_ed->axis + 0x474);
        // the three sliders' ranges (no redraw) and positions from the current values (0x4fe8ff..0x4fe9d7)
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETRANGE, FALSE, MAKELONG(1, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETRANGE, FALSE, MAKELONG(1, 100));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->sx * 100.0f));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->tx * 100.0f));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game_ed->mx);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETRANGE, FALSE, MAKELONG(0, 255));
        g_game_ed->col = rgl::vector4(1.0f, 1.0f, 1.0f);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETPOS, TRUE, 255);
        SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETPOS, TRUE, 255);
        for (int i = 0; i < g_editor->brush_count - 2; i++) {
            int j = *(int*)((char*)g_editor + i * 0x18 + 0x63ec);
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
            g_editor->object_kind = 0;                                // the object kind resets to the first entry (0x4fec16)
            SendMessageA(combo, CB_SETCURSEL, 0, 0);
            ((MapEditorDialog*)this)->update_object_type();
            return 1;
        }
        break;
    }
    case WM_NOTIFY:
        switch (LOWORD(wParam)) {
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
            rgl::vector4 c;
            c.x = (float)r * g_color_unit;
            c.y = (float)g * g_color_unit;
            c.z = (float)b * g_color_unit;
            g_game_ed->col = c;
            return 0;
        }
        case 0x473: {
            char buf[512];
            if (g_game_ed->mode == 2) {
                g_game_ed->sy = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->sy);
            } else if (g_game_ed->mode == 1) {
                g_game_ed->sx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->sx);
            } else {
                g_game_ed->sz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->sz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
            break;
        }
        case 0x477: {
            char buf[512];
            if (g_game_ed->mode == 2) {
                g_game_ed->ty = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->ty);
            } else if (g_game_ed->mode == 1) {
                g_game_ed->tx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->tx);
            } else {
                g_game_ed->tz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_GETPOS, 0, 0) * 0.01f;
                sprintf(buf, "%1.2f", (double)g_game_ed->tz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
            break;
        }
        case 0x472: {
            char buf[512];
            if (g_game_ed->mode == 2) {
                g_game_ed->my = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game_ed->my);
            } else if (g_game_ed->mode == 1) {
                g_game_ed->mx = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game_ed->mx);
            } else {
                g_game_ed->mz = (float)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_GETPOS, 0, 0);
                sprintf(buf, "%1.0f", (double)g_game_ed->mz);
            }
            SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
            break;
        }
        }
        return 0;
    case WM_COMMAND:
        // WM_COMMAND: LOWORD(wParam) is the control, HIWORD(wParam) the notification code (0x4ff24c reads both from
        // wParam; every case below tested HIWORD(lParam), the control's window handle)
        {
        int id = LOWORD(wParam);
        switch (id) {
        case 0x431:
            if (HIWORD(wParam) == 1) {
                ((MapEditorDialog*)this)->update_object_type();
                return 1;
            }
            break;
        case 2:
            restore_defaults();
            ShowWindow(m_hMapEditorDlg, SW_HIDE);
            return 1;
        case 0x433:
            if (HIWORD(wParam) == 2) {
                g_game_ed->flag_c8 = 1;
                return 1;
            } else {
                GetDlgItem(m_hMapEditorDlg, 0x431);                 // looked up and not used (0x4ff323)
                HWND c = GetDlgItem(m_hMapEditorDlg, 0x433);        // the object list is a list box: LB_ messages (0x4ff347)
                g_game_ed->combo_sel = (int)SendMessageA(c, LB_GETITEMDATA, SendMessageA(c, LB_GETCURSEL, 0, 0), 0);
                return 1;
            }
        case 0x438: {
            WORD code = HIWORD(wParam);
            if (code == 2) {
                HWND c = GetDlgItem(hDlg, 0x438);
                int cnt = (int)SendMessageA(c, LB_GETCOUNT, 0, 0);
                for (int i = 0; i < cnt; i++)
                    if (g_game_ed->sel_index != (int)SendMessageA(c, LB_GETITEMDATA, i, 0))
                        SendMessageA(c, LB_SETSEL, 0, i);
                g_game_ed->sel.resize(0);
                if (g_game_ed->sel_index >= 0)
                    g_game_ed->sel.push_back(g_game_ed->sel_index);
                ((EditorApp*)this)->refresh_dialog();
                if (g_game_ed->sel_index >= 0) {
                    g_game_ed->sel_copy = g_editor->world[g_game_ed->sel_index].frame;
                    g_game_ed->timer.set_delay(-2.0f);
                }
                ((MapEditorDialog*)this)->refresh();
                return 1;
            }
            if (code == 1 || code == 3) {
                HWND c = GetDlgItem(hDlg, 0x438);
                int sel[1024];
                int cnt = (int)SendMessageA(c, LB_GETSELITEMS, 0x400, (LPARAM)sel);
                g_game_ed->sel_index = -1;
                IntVec picked;                                        // the indices the list box reports
                for (int i = 0; i < cnt; i++) {
                    int v = (int)SendMessageA(c, LB_GETITEMDATA, sel[i], 0);
                    if (v >= 0) {
                        picked.push_back(v);
                        bool found = false;
                        for (unsigned k = 0; k < g_game_ed->sel.size(); k++) {
                            if (g_game_ed->sel[k] == v) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            g_game_ed->sel.push_back(v);
                            g_game_ed->sel_index = v;
                        }
                    }
                }
                if (picked.size() != g_game_ed->sel.size()) {
                    // drop the selected indices the list box no longer reports (swap with the last, pop)
                    for (unsigned k = 0; k < g_game_ed->sel.size(); k++) {
                        volatile bool kept = false;
                        for (unsigned j = 0; j < picked.size(); j++)
                            if (g_game_ed->sel[k] == picked[j])
                                kept = true;
                        if (!kept) {
                            g_game_ed->sel[k] = g_game_ed->sel[g_game_ed->sel.size() - 1];
                            g_game_ed->sel.pop_back();
                            k--;
                        }
                    }
                }
                if (g_game_ed->sel_index == -1 && g_game_ed->sel.size() > 0)
                    g_game_ed->sel_index = g_game_ed->sel[g_game_ed->sel.size() - 1];
                ((EditorApp*)this)->refresh_dialog();
                return 1;
            }
            break;
        }
        case 0x434:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x434), BM_GETCHECK, 0, 0); g_game_ed->rt_flag = v == BST_CHECKED; }
            g_game_ed->rt = rgl::vector4(1.0f, 1.0f, 1.0f);
            g_game_ed->rt_extra = 0;
            if (g_game_ed->rt_flag == 0) {
                ((MapEditorDialog*)this)->refresh();
                return 1;
            }
            break;
        case 0x43a:
            if (HIWORD(wParam) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43a, &ok, FALSE);
                if (ok != 0 && g_game_ed->sel_index >= 0 && v < 0x40) {
                    g_editor->world[g_game_ed->sel_index].field_1a0 = v;
                    g_game_ed->dirty = 1;
                    return 1;
                }
                SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
                return 1;
            }
            break;
        case 0x46d:
            if (HIWORD(wParam) == 0) {
                g_game_ed->mode = 2;
                ((CreateGameOptionsPage*)this)->update_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game_ed->my);
                sprintf(buf, "%1.0f", (double)g_game_ed->my);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->sy * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->sy);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->ty * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->ty);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                return 1;
            }
            break;
        case 0x43b:
            if (HIWORD(wParam) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hMapEditorDlg, 0x43b, &ok, FALSE);
                if (ok != 0 && g_game_ed->sel_index >= 0 && v < 0x40) {
                    g_editor->world[g_game_ed->sel_index].field_1a4 = v;
                    g_game_ed->dirty = 1;
                    return 1;
                }
                SetDlgItemTextA(m_hMapEditorDlg, 0x43a, "");
                return 1;
            }
            break;
        case 0x453:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x453), BM_GETCHECK, 0, 0); g_game_ed->opt_cc = v == BST_CHECKED; }
            return 1;
        case 0x454:
            { int v = (int)SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x454), BM_GETCHECK, 0, 0); g_game_ed->opt_d0 = v == BST_CHECKED; }
            return 1;
        case 0x46e:
            if (HIWORD(wParam) == 1) {
                HWND lst = GetDlgItem(m_hMapEditorDlg, 0x46e);
                g_editor->object_kind = (int)SendMessageA(lst, CB_GETITEMDATA, SendMessageA(lst, CB_GETCURSEL, 0, 0), 0);
                return 1;
            }
            break;
        case 0x475:
            g_game_ed->axis = 1;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game_ed->axis + 0x474);
            break;
        case 0x474:
            g_game_ed->axis = 0;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game_ed->axis + 0x474);
            break;
        case 0x476:
            g_game_ed->axis = 2;
            CheckRadioButton(m_hMapEditorDlg, 0x474, 0x476, g_game_ed->axis + 0x474);
            break;
        case 0x470:
            if (HIWORD(wParam) == 0) {
                g_game_ed->mode = 1;
                ((CreateGameOptionsPage*)this)->update_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game_ed->mx);
                sprintf(buf, "%1.0f", (double)g_game_ed->mx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->sx * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->sx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->tx * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->tx);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                return 1;
            }
            break;
        case 0x471:
            if (HIWORD(wParam) == 0) {
                g_game_ed->mode = 3;
                ((CreateGameOptionsPage*)this)->update_checks();
                char buf[128];
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x472), TBM_SETPOS, TRUE, (LPARAM)(int)g_game_ed->mz);
                sprintf(buf, "%1.0f", (double)g_game_ed->mz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x47f), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x473), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->sz * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->sz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x480), buf);
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x477), TBM_SETPOS, TRUE, (LPARAM)(int)(g_game_ed->tz * 100.0f));
                sprintf(buf, "%1.2f", (double)g_game_ed->tz);
                SetWindowTextA(GetDlgItem(m_hMapEditorDlg, 0x482), buf);
                return 1;
            }
            break;
        case 0x46f:
            if (HIWORD(wParam) == 0) {
                g_game_ed->mode = 0;
                ((CreateGameOptionsPage*)this)->update_checks();
                return 1;
            }
            break;
        case 0x489:
            DialogBoxParamA(hinstance, (LPCSTR)0xde, m_hMapEditorDlg, options_dlg_proc, 0);
            return 1;
        case 0x494:
            if (MessageBoxA(m_hMapEditorDlg, "Are you sure?", "Warning", 1) == 1) {
                ((MapScreen*)g_editor)->frame_step();
                return 1;
            }
            break;
        case 0x491:
        case 0x492:
        case 0x493:
            if (HIWORD(wParam) == 0 && (g_game_ed->sel_index > 0 || g_game_ed->rt_flag != 0)) {
                float fx = 1.0f;
                float fy = 1.0f;
                float fz = 1.0f;
                if ((short)wParam == 0x491)
                    fx = -1.0f;
                else if ((short)wParam == 0x492)
                    fy = -1.0f;
                else if ((short)wParam == 0x493)
                    fz = -1.0f;
                if (g_game_ed->rt_flag) {
                    rgl::vector4 old = g_game_ed->rt;
                    g_game_ed->rt.x = fx * g_game_ed->rt.x;
                    g_game_ed->rt.y = fy * g_game_ed->rt.y;
                    g_game_ed->rt.z = fz * g_game_ed->rt.z;
                    for (unsigned i = 0; i < g_editor->edit_obj->meshes.size(); i++) {
                        rgl::vector4 inv(1.0f / old.x, 1.0f / old.y, 1.0f / old.z);
                        ((rgl::mesh*)g_editor->edit_obj->meshes[i])->set_color(inv);
                        ((rgl::mesh*)g_editor->edit_obj->meshes[i])->set_color(g_game_ed->rt);
                    }
                    return 1;
                }
                g_editor->world[g_game_ed->sel_index].rx = fx * g_editor->world[g_game_ed->sel_index].rx;
                g_editor->world[g_game_ed->sel_index].ry = fy * g_editor->world[g_game_ed->sel_index].ry;
                g_editor->world[g_game_ed->sel_index].rz = fz * g_editor->world[g_game_ed->sel_index].rz;
                for (unsigned i = 0; i < g_editor->world[g_game_ed->sel_index].obj->meshes.size(); i++) {
                    g_editor->world[g_game_ed->sel_index].obj->meshes[i]->flag = 1 - g_editor->world[g_game_ed->sel_index].obj->meshes[i]->flag;
                }
                ((MapDialog*)g_editor)->set_object_target(g_game_ed->sel_index);
                g_game_ed->dirty = 1;
                return 1;
            }
            break;
        case 0x497:
            if (HIWORD(wParam) == 0) {
                g_game_ed->sub_mode = 0;
                ((CreateGameOptionsPage*)this)->update_checks();
                g_editor->need_redraw[0] = 1;
                g_editor->need_redraw[1] = 1;
                g_editor->need_redraw[2] = 1;
                return 1;
            }
            break;
        case 0x4af:
            if (MessageBoxA(m_hMapEditorDlg, "This action will clear all texture and height data of the terrain that are added manually. Are you sure?", "Warning", 1) == 1) {
                g_terrain.init_map((char*)g_editor + 0x63d0);
                g_game_ed->dirty = 1;
                g_terrain.conv_build_scene();
                return 1;
            }
            break;
        case 0x49a:
            if (HIWORD(wParam) == 0) {
                g_game_ed->mode = 4;
                ((CreateGameOptionsPage*)this)->update_checks();
                return 1;
            }
            break;
        case 0x499:
            if (HIWORD(wParam) == 0) {
                g_game_ed->sub_mode = 2;
                ((CreateGameOptionsPage*)this)->update_checks();
                g_editor->need_redraw[0] = 1;
                g_editor->need_redraw[1] = 1;
                g_editor->need_redraw[2] = 1;
                return 1;
            }
            break;
        case 0x498:
            if (HIWORD(wParam) == 0) {
                g_game_ed->sub_mode = 1;
                ((CreateGameOptionsPage*)this)->update_checks();
                g_editor->need_redraw[0] = 1;                   // joins the redraw tail after the dirty store (0x5005f0)
                g_editor->need_redraw[1] = 1;
                g_editor->need_redraw[2] = 1;
                return 1;
            }
            break;
        case 0x4a5:
            ((graph_obj*)EDIT_OBJ)->update_vertex_mode();
            g_game_ed->sub_mode = 0;
            ((CreateGameOptionsPage*)this)->update_checks();
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4a3:
            ((graph_obj*)EDIT_OBJ)->update_edge_mode();
            g_game_ed->sub_mode = 1;
            ((CreateGameOptionsPage*)this)->update_checks();
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4a4:
            ((graph_obj*)EDIT_OBJ)->update_poly_mode();
            g_game_ed->sub_mode = 2;
            ((CreateGameOptionsPage*)this)->update_checks();
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x49b:
            ((EditMesh*)EDIT_OBJ)->build_second_pass();
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4aa:
            if (!((TearObj*)EDIT_OBJ)->teardown()) {
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 3 or 4 vertices (or edges that has 3 or 4 different edges) that are not a member of the same face and that are not members of any edges with 2 faces.", "Warning", 0);
                return 1;
            }
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4a7:
            if (!((EditMesh*)EDIT_OBJ)->build_first_pass()) {
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of a face and do not have any edge neighborhood.", "Warning", 0);
                return 1;
            }
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4a6:
            ((graph_obj*)EDIT_OBJ)->refresh_selection();
            g_editor->need_redraw[g_game_ed->sub_mode] = 1;
            return 1;
        case 0x4ad:
            if (!((EditMesh*)EDIT_OBJ)->split_marked_edge()) {
                MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
                return 1;
            }
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4a8:
            ((EditMesh*)EDIT_OBJ)->finish();
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x49d:
            if (((graph_obj*)EDIT_OBJ)->split_edge(-1)) {
                g_game_ed->dirty = 1;
                g_editor->need_redraw[0] = 1;
                g_editor->need_redraw[1] = 1;
                g_editor->need_redraw[2] = 1;
                return 1;
            }
            MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge that has a neighborhood of 2 triangular faces.", "Warning", 0);
            return 1;
        case 0x4ac:
            if (!((graph_obj*)EDIT_OBJ)->combine()) {
                if (g_editor->sel_count2 == 0)
                    MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 2 vertices that are members of the same edge or that are members of any edge having a neighborhood to only one face.", "Warning", 0);
                else
                    MessageBoxA(m_hMapEditorDlg, "Action not performed. You must select only 1 edge.", "Warning", 0);
                return 1;
            }
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4ae:
            ((EditMesh*)EDIT_OBJ)->add_quad(&g_game_ed->quad);
            g_game_ed->dirty = 1;
            g_editor->need_redraw[0] = 1;
            g_editor->need_redraw[1] = 1;
            g_editor->need_redraw[2] = 1;
            return 1;
        case 0x4cd:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text, 0x40);
                int v;
                // the edit text is rewritten only when the value had to be corrected (empty, negative or above 255: 0x50078c);
                // an in-range value goes straight to the trackbar (0x5007b3)
                bool fix = true;
                if (text[0] == 0)
                    v = 0;
                else {
                    v = atol(text);
                    if (v > 0xff)
                        v = 0xff;
                    else if (v < 0)
                        v = 0;
                    else
                        fix = false;
                }
                if (fix) {
                    _itoa(v, text, 10);
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cd, text);
                }
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c8), TBM_SETPOS, TRUE, (LPARAM)v);
                return 1;
            }
            break;
        case 0x4cf:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text, 0x40);
                int v;
                bool fix = true;
                if (text[0] == 0)
                    v = 0;
                else {
                    v = atol(text);
                    if (v > 0xff)
                        v = 0xff;
                    else if (v < 0)
                        v = 0;
                    else
                        fix = false;
                }
                if (fix) {
                    _itoa(v, text, 10);
                    SetDlgItemTextA(m_hMapEditorDlg, 0x4cf, text);
                }
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4c9), TBM_SETPOS, TRUE, (LPARAM)v);
                return 1;
            }
            break;
        case 0x4ce:
            if (HIWORD(wParam) == 0x400) {
                char text[0x40];
                GetDlgItemTextA(m_hMapEditorDlg, 0x4ce, text, 0x40);
                int v;
                if (text[0] == 0 || (v = atol(text)) < 0)
                    v = 0;
                else if (v > 0xff)
                    v = 0xff;
                SendMessageA(GetDlgItem(m_hMapEditorDlg, 0x4ca), TBM_SETPOS, TRUE, (LPARAM)v);
                return 1;
            }
            break;
        }
        }
        break;
    case WM_MOUSEMOVE:
        break;
    default:
        return 0;
    }
    return 1;
}
