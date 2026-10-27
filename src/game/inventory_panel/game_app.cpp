// Slice s0050ec40: the main loop's per-frame "game update" (0x50ec40), the application's window
// procedure override (0x50f130, CD3DApplication::MsgProc at 0x403120) and the loading screen's
// step function (0x50f390).  Class layouts follow the verified wave-1 slices of the DirectX framework
// (s00401010: CD3DApplication = `Application`) and the engine's tstring (s0042a1f0) / widget vtables.
// Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <math.h>

// ---- tstring (0x10 bytes: read cursor, buffer, capacity, length) -------------------------------------
#include "../tstring.h"

// the engine's "null or pointer" read used all over the code base (buf == 0 -> the field itself)
static __forceinline const char* c_str(const tstring* s)
{
    char* p = s->buf;
    if (!p) p = (char*)&s->buf;
    return p;
}

// ---- CD3DApplication / GameApp: one definition (../application.h) -----------------------------------
#include "../application.h"

// ---- screens: g_windows[30] (0xa73980) is the global screen table; the id stack at 0xa739f8 keeps
//      the current screen id (a ring of 4-int blocks). ------------------------------------------------
// ---- the engine widget interface: only the slots this slice dispatches ---------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

struct ScreenObj : Widget {
};
void __cdecl frame_hook(void);                               // 0x4ec460 (game/char_sheet/screen.cpp: the end-of-frame callback, a free function)
extern ScreenObj* g_windows[30];       // 0xa73980 (.. 0xa739f4)

#include "../id_stack.h"               // IdStack g_id_stack (0xa739f8), current_screen_id

// ---- game (0x5e2930).  Only the members touched here. ------------------------------------------------
#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)

// ---- globals ------------------------------------------------------------------------------------------
extern int   g_counter;                 // 0x5e28cc
extern float g_8ee42f0;                 // 0x8ee42f0
extern __int64 g_5e2970;                // 0x5e2970 (frame time step, scaled)
extern __int64 g_ticks;                 // 0x5e2948
extern float g_5a9c20;                  // 0x5a9c20
extern int   g_5d8798;                  // 0x5d8798
extern int   g_5e293c;                  // 0x5e293c
extern char  g_5d75e0;                  // 0x5d75e0
extern char  g_saved;                  // 0x5e2a08
extern int   g_no_cam;                  // 0x5a9fb0
extern int   g_a17158, g_a17558, g_a17348, g_a17148;    // 0xa17158 ..
extern int   g_a1719c, g_a1759c, g_a171ac, g_a1717c;    // 0xa1719c ..
extern char  g_5e2630;                  // 0x5e2630
extern char  g_key_states[256];               // 0x5e27c8
// the Direct3D9 device (0xa17f28); __stdcall virtuals -> the COM-style call pattern
struct IDev9 {
    virtual long __stdcall s0(); virtual long __stdcall s1(); virtual long __stdcall s2();
    virtual long __stdcall s3(); virtual long __stdcall s4(); virtual long __stdcall s5();
    virtual long __stdcall s6(); virtual long __stdcall s7(); virtual long __stdcall s8();
    virtual long __stdcall s9(); virtual long __stdcall s10();
    virtual void __stdcall SetCursorPosition(int x, int y, unsigned flags);   // slot 11 (+0x2c)
    virtual int  __stdcall ShowCursor(int show);                              // slot 12 (+0x30)
};

extern char  g_a739b8[];                // 0xa739b8 (current editor entity)
// the current editor entity (a pointer at 0xa739b8)
#include "../editor_entity.h"
extern EditorEntity* g_entity;          // 0xa739b8

// gui manager object (0x5d7080) and a few of its member functions
#include "../gui_mgr.h"
extern GuiManager g_gui;        // 0x5d7080

#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
extern resmgr g_resmgr;                 // 0x5d6fa0

struct Slot {                           // game/ui (slot.cpp): the input device at 0xa170c8
    void update(float dt);              // 0x487930
};
extern Slot g_keyboard;                   // 0xa170c8

struct sound_engine {                   // rgl/mesh2/rec_tables.h: the music player at 0xa73a10
    void __fastcall tick();             // 0x4c4100
};
extern sound_engine g_music_situation;           // 0xa73a10

struct GuiScreen {
    void process(int param);   // 0x4ebfd0 (game/char_sheet/screen.h): the gui object at 0x5d7080 (the dt rides in as its bits)
};
struct GuiCtx;
void __fastcall pick_entity(GuiCtx* e);                // 0x4ed8d0 (game/char_sheet/pick_entity.cpp): EditorEntity::pick_entity
struct ID3DXEffect;
struct resource_manager {
    void resolve_effect_sets(ID3DXEffect* fx);   // 0x418ff0 (rgl/resource.h)
};
struct MapEditorDialog {
    void update_object_type();   // 0x4c7bf0 / 0x4c7e20 (rgl/mesh2/gui_edit.h): the dialog of the application object
    void update_objects();   // 0x4c7e20
};
struct CD3DApplication {
    virtual long MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x403120 (dxframework/d3dapp.h)
};

struct Output {
    virtual void o0(); virtual void o1(); virtual void o2(); virtual void o3();
    virtual void o4(); virtual void o5(); virtual void o6(); virtual void o7();
    virtual void o8(); virtual void o9();
    virtual void print(const char* s);  // +0x28
};
extern Output g_log;                 // 0x5a9f60

#include "../main_menu.h"                         // MainMenu (0xa73980): the one definition

#include "../map_editor_obj.h"        // MapEditor (the object at 0x8ee3c48)

// ---- free functions --------------------------------------------------------------------------------
extern "C" void __stdcall _FSOUND_Update(void);              // 0x53433c
void bump_counter(void);                                     // 0x42cb70 (?bump_counter@@YAXXZ)
long build_shader(void);                                    // 0x4c17c0 (game/triggers/trigger_vec.cpp, returns an HRESULT)
void mb_error(const char* fmt, ...);                          // 0x42cc80
void load_skyboxes(void);                                        // 0x4c3350
void load_data_files(void);                                        // 0x4d0c20 (options_panel/module_load.cpp)
void load_module_files(void);                                        // 0x50d5e0 (inventory_panel/module_files.cpp)
LRESULT __stdcall dialog_thunk(HWND, UINT, WPARAM, LPARAM);   // 0x50d250 (the dialog proc: inventory_panel/widget_text.cpp)
void script_dialog_proc();                                   // 0x4c3340
void global_4bf870(void);                                    // 0x4c3330 (rgl/mesh2/rec_tables.cpp: the thunk to the script debugger proc)

// ---- the loading screen (0x50f440 lives in slice s0050f440; see there) ------------------------------
#include "../loading_screen.h"       // LoadingScreen: update 0x50f390 here, advance 0x50f440 in loading_screen.cpp
#include "../../include/renderer.h"

// ---- the per-frame update ---------------------------------------------------------------------------
// @ 0x50ec40
int GameApp::Update()
{
    float dt;
    __int64 t;

    g_counter++;
    bump_counter();

    dt = m_fElapsedTime;
    if (dt > 0.1f) dt = 0.1f;
    *(float*)0x8ee42f0 = dt;
    t = (__int64)(dt * *(float*)0x54b86c);
    g_5e2970 = t;
    g_ticks += t;
    g_5a9c20 = dt;
    g_5d8798 = m_34c;
    m_34c = -1;

    ((GuiScreen*)&g_gui)->process(*(int*)&dt);
    g_keyboard.update(dt);
    g_music_situation.tick();
    _FSOUND_Update();

    if (g_no_cam) {
        if (g_a17158 && !g_a17558 && (g_a17348 || g_a17148)) {
            if (build_shader() < 0) {
                mb_error("Unable to compile the effect file.");
            } else {
                g_gui.add(tstring("Effect file recompiled."), -1);
            }
            ((resource_manager*)&g_resmgr)->resolve_effect_sets((ID3DXEffect*)g_5e293c);
        }
    }
    if (g_no_cam) {
        if (g_a1719c && !g_a1759c) {
            if ((g_a17348 || g_a17148) && (g_a171ac || g_a1717c)) {
                load_skyboxes();
                g_gui.add(tstring("Animation data restored."), -1);
            } else if (g_a17348 || g_a17148) {
                load_data_files();
                load_module_files();
                g_gui.add(tstring("Module data restored."), -1);
            }
        }
    }

    frame_hook();
    g_windows[current_screen_id()]->update(dt);

    if (g_game) {
        if (current_screen_id() == 0xe) {
            if (g_game->f1a090) {
                if (!g_game->f1a160) {
                    if (!m_hMapEditorDlg) {
                        m_hMapEditorDlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xc0, m_hWnd,
                                                             (DLGPROC)dialog_thunk, 0);
                    } else {
                        ((MapEditorDialog*)this)->update_object_type();
                    }
                    ((MapEditorDialog*)this)->update_objects();
                    ShowWindow(m_hMapEditorDlg, 5);
                    g_game->f1a160 = 1;
                }
                if (g_game->f1a1c8) {
                    HWND item = GetDlgItem(m_hMapEditorDlg, 0x438);
                    int n = (int)SendMessageA(item, 0x18b, 0, 0);
                    int i;
                    for (i = 0; i < n; i++)
                        SendMessageA(item, 0x185, 0, i);
                    if (g_game->f1a1b4 >= 0) {
                        for (i = 0; i < n; i++) {
                            int v = (int)SendMessageA(item, 0x199, i, 0);
                            int* p = g_game->f1a1bc;
                            unsigned j = 0;
                            while (p && j < (unsigned)(g_game->f1a1c0 - p)) {
                                if (v == p[j]) {
                                    SendMessageA(item, 0x185, 1, i);
                                    break;
                                }
                                j++;
                            }
                        }
                    }
                    pick_entity((GuiCtx*)g_entity);
                    g_entity->f2f9 = 1;
                    SetFocus(m_hWnd);
                    g_game->f1a1c8 = 0;
                }
            }
        } else if (!g_game->f1a090) {
            if (g_game->f1a160) {
                if (m_hMapEditorDlg) {
                    ShowWindow(m_hMapEditorDlg, 0);
                    g_game->f1a160 = 0;
                }
            }
        }
        if (g_game->f1a060) {
            if (!g_game->f1a064) {
                if (!m_hScriptDlg) {
                    m_hScriptDlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xdf, m_hWnd,
                                                      (DLGPROC)global_4bf870, 0);
                }
                ShowWindow(m_hScriptDlg, 5);
                g_game->f1a064 = 1;
            }
        } else if (g_game->f1a064) {
            if (m_hScriptDlg) {
                EndDialog(m_hScriptDlg, 2);
                m_hScriptDlg = 0;
            }
            g_game->f1a064 = 0;
        }
    }

    m_bShowCursorWhenFullscreen = g_5d75e0;
    if (g_saved) ((GameApp*)this)->f4 = true;
    return 0;
}

// ---- the window procedure ---------------------------------------------------------------------------
// @ 0x50f130
int GameApp::MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam)
{
    POINT pt;

    // the case bodies are laid out by the compiler in this order (see the disassembly at 0x50f163..)
    switch (msg) {
    case 0x101:                                 // WM_KEYUP
        *(char*)(0x5e27c8 + wparam) = 0;
        break;
    case 0x100:                                 // WM_KEYDOWN
        *(char*)(0x5e27c8 + wparam) = 1;
        break;
    case 0x20:                                  // WM_SETCURSOR
        if ((*(IDev9**)&g_render.dev) && m_bWindowed) {
            SetCursor(0);
            (*(IDev9**)&g_render.dev)->ShowCursor(1);
            return 1;
        }
        break;
    case 0x111:                                 // WM_COMMAND
        switch ((unsigned short)wparam) {
        case 0x9c5b:
            if (g_game) {
                g_game->save_language_files(0);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory!",
                            "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5c:
            if (g_game) {
                g_game->save_language_files(1);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory! "
                               "Non-translated texts are marked with a {***} sign. You can use this "
                               "as the search keyword.", "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5a:
            load_data_files();
            load_module_files();
            g_gui.add(tstring("Module data restored."), -1);
            break;
        case 0x9c56:
            if (!g_game) goto check_focus;
            g_game->f1a060 = 1;
            g_game->f1a068 = 0;
            ((tstring*)((char*)g_game + 0x1a06c))->clear();
            break;
        case 0x9c58:
            DialogBoxParamA(m_hInstance, (LPCSTR)0xe0, m_hWnd, (DLGPROC)script_dialog_proc, 0);
            // falls through
        }
        // falls through
    case 0x200:                                 // WM_MOUSEMOVE
    check_focus:
        if (g_game && (g_game->f1a090 || g_game->f1a060) && !g_5e2630)
            SetFocus(m_hWnd);
        if (m_bActive && m_pd3dDevice)
            GetCursorPos(&pt);
        break;
    case 0x109:
        if (wparam == 0xffff) return 1;
        m_34c = wparam;
        break;
    case 0x102:                                 // WM_CHAR
        m_34c = wparam;
        break;
    }
    return ((CD3DApplication*)this)->CD3DApplication::MsgProc(hWnd, msg, wparam, lparam);
}

// ---- the loading screen's step ----------------------------------------------------------------------
// @ 0x50f390
void LoadingScreen::update(int unused)
{
    char buf[0x100];
    tstring text;
    char tmp[8];
    bool finished = advance(&step, &total, &text);
    const char* fmt = "%d";
    _snprintf(buf, 0x100, fmt, step);
    ((ScreenObj*)hint)->set_text(tmp, text);
    ((ScreenObj*)logo)->set_value((float)step * 0.14285715f);
    {   // TEMP probe: which class is the +0x38 widget, and what does its slot 13 (the one 0x50f390 calls) point at?
        static int once_ = 0;
        FILE* pf_ = once_++ ? 0 : fopen("ls_probe.txt", "at");
        if (pf_) {
            void** vp = *(void***)(void*)logo;
            void* s13 = vp ? vp[13] : 0;
            fprintf(pf_, "ls +0x38 vptr=%p slot13=%p  (MissionUi vt=0x591c80, progress_bar vt=0x597f20, set_ratio=0x4e3f60) step=%d ratio=%g\n",
                    (void*)vp, s13, step, (float)step * 0.14285715f);
            fclose(pf_);
        }
    }
    if (finished) {
        g_main_menu.set_state(4);
        ((Output*)0x5a9f60)->print("Finished All...\n");
    }
}
